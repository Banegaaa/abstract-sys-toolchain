#include <iostream>
#include <string>
#include <vector>
#include "token.h"
#include "define.h"
#include "symbol.h"
using namespace std;

extern vector<Token> tokenList;
extern vector<Symbol> symbolTable;
// get extern helper functions from firstPass.cpp
extern int hexToInt(string hex);
extern Symbol* symbolExists(string name);
extern int getSectionNumber(string name);

extern int LC;
extern string currentSection;
extern int currentSectionNumber;

string tempSymbolNameSecondPass;
Symbol* tempSymbolSecondPass;

// we'll need a map of section names and their current LC values
std::unordered_map<std::string, int> sectionLCMap;



// we need a function that returns the section symbol based on the given section name,
// so we can write machine code for that section
Symbol* getSectionSymbol(string name){
    for (int i = 0; i < symbolTable.size(); i++){
        if (symbolTable[i].name == name && symbolTable[i].section){
            return &symbolTable[i];
        }
    }
    return NULL;
}


int secondPass(){
    LC = 0;
    currentSection = "";
    currentSectionNumber = -1;
    
    for (int i = 0; i < tokenList.size(); i++){
        
        switch (tokenList[i].value){
        // in the second pass, we won't be checking for syntax errors, we did that in the first pass

        case COMMENT:
            while (tokenList[i].value != NEWLINE)
                i++;
            break;

        case SKIP:
            int numOfBytes = 0;
            if (tokenList[i].value == DEC_LITERAL){
                numOfBytes = stoi(tokenList[i].name);
            }
            else if (tokenList[i].value == HEX_LITERAL){
                numOfBytes = hexToInt(tokenList[i].name);
            }

            // write LC bytes of 0 to the current section
            tempSymbolSecondPass = getSectionSymbol(currentSection);
            tempSymbolSecondPass->setZeroValueBytes(LC, numOfBytes);
            LC += numOfBytes;
            break;

        case GLOBAL:
            // nothing to do in second pass
            while (tokenList[i].value == GLOBAL || tokenList[i].value == COMMA){
                i++; // get the symbol name
                i++; // get ,
            }
            break;

        case EXTERN:
            // nothing to do in second pass
            while (tokenList[i].value == EXTERN || tokenList[i].value == COMMA){
                i++; // get the symbol name
                i++; // get ,
            }
            break;

        case WORD:
            int value;
            while(tokenList[i].value == WORD || tokenList[i].value == COMMA){
                i++;
                if (tokenList[i].value == DEC_LITERAL){
                    value = stoi(tokenList[i].name);
                }
                else if (tokenList[i].value == HEX_LITERAL){
                    value = hexToInt(tokenList[i].name);
                }
                else if (tokenList[i].value == NAME){
                    tempSymbolSecondPass = symbolExists(tokenList[i].name);
                    if (tempSymbolSecondPass->is_defined){
                        value = tempSymbolSecondPass->value;
                    }
                    else {
                        // write zeros to the machine code
                        // and create a relocation entry
                        value = 0;
                        // TODO: create a relocation entry
                    }
                }
                // for each value write 4 bytes of machine code to the current section
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, value);
                LC += 4;
            } 
            break;

        case SECTION:
            // first, we need to set the current LC value to the sectionLCMap, so we'll know from where to continue if this section comes up again
            if (currentSection != ""){
                sectionLCMap[currentSection] = LC;
            }
            // then, we need to set the current section to the new section
            i++; // get the section name
            currentSection = tokenList[i].name;
            currentSectionNumber = getSectionNumber(currentSection);
            
            // we check if the section is already in sectionLCMap, if it is, we set the LC to that value, if not we set it to 0
            if (sectionLCMap.find(currentSection) != sectionLCMap.end()){
                LC = sectionLCMap[currentSection];
            }
            else {
                LC = 0;
            }
            break;
        
        case EQU:
            // nothing to do in second pass (the symbol is definately defined in the first pass)
            i++; // get the symbol name
            i++; // get ,
            i++; // get the symbol value
            break;

        case LABEL:
            i++; // get the label name
            break;
        
        default:
            break;
        }
    }
}