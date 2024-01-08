#include <string>
#include <vector>
#include <iostream>
#include "token.h"
#include "define.h"
#include "symbol.h"
using namespace std;

extern vector<Token> tokenList;
int LC = 0;
string currentSection = "";
int currentSectionNumber = 0;


vector<Symbol> symbolTable;

// helpers:
string tempSymbolName = "";
Symbol* tempSymbol;



int hexToInt(std::string s)
{
    std::string numPart = s.substr(2);
    return std::stoi(numPart, 0, 16);
}

Symbol* symbolExists(string name){
    for (int j = 0; j < symbolTable.size(); j++){
        if (symbolTable[j].name == name){
            return &symbolTable[j];
        }
    }
    return nullptr;
}

int getSectionNumber(string name){
    for (int j = 0; j < symbolTable.size(); j++){
        if (symbolTable[j].name == name){
            return symbolTable[j].number;
        }
    }
    return -1;
}


int firstPass(){
    for (int i = 0; i < tokenList.size(); i++){
    
        switch(tokenList[i].value){

            case COMMENT:
                while (tokenList[i].value != NEWLINE){
                    i++;
                }
                break;
            
            case SKIP:
                i++; // get skip size
                if (tokenList[i].value == DEC_LITERAL){
                    LC += stoi(tokenList[i].name);
                }
                else if (tokenList[i].value == HEX_LITERAL){
                    LC += hexToInt(tokenList[i].name);
                }
                else{
                    cout << "Error: Invalid skip size" << endl;
                    exit(EXIT_FAILURE);
                }
                break;
            
            case GLOBAL:
                while (tokenList[i].value == GLOBAL || tokenList[i].value == COMMA){
                    i++;
                    if (tokenList[i].value == NAME){
                        // check if symbol with that name already exists in the symbol table
                        tempSymbol = symbolExists(tokenList[i].name);
                        if (tempSymbol == nullptr){
                            symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, true, false, symbolTable.size(), false));
                        }
                    }
                }
                break;

            case EXTERN:
                while (tokenList[i].value == EXTERN || tokenList[i].value == COMMA){
                    i++;
                    if (tokenList[i].value == NAME){
                        // check if symbol with that name already exists in the symbol table (TODO: MAYBE NOT NEEDED - IF WE HAVE EXTERN, WE KNOW IT DOESN'T EXIST)
                        tempSymbol = symbolExists(tokenList[i].name);
                        if (tempSymbol == nullptr){
                            symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, true, false, symbolTable.size(), false));
                        }
                    }
                }
                break;
            
            case WORD:
                while(tokenList[i].value == WORD || tokenList[i].value == COMMA){
                    i++;
                    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL || tokenList[i].value == NAME){
                        LC += 4;
                        // TODO: should we write the symbol in the symbol table if it doesn't exist?
                        // if (tokenList[i].value == NAME){
                        //     // check if symbol with that name already exists in the symbol table
                        //     if (!symbolExists(tokenList[i].name)){
                        //         symbolTable.push_back(Symbol(tokenList[i].name, currentSection, LC, false, false, symbolTable.size(), false));
                        // }

                    }
                    else{
                        cout << "Error: Invalid word value" << endl;
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            
            case SECTION:
                // section is in format: .section name
                i++;
                if (tokenList[i].value == NAME){

                    // first, set the size of the previous section (if there is one)
                    if (currentSection != ""){
                        tempSymbol = symbolExists(currentSection);
                        tempSymbol->setSectionSize(LC);
                    }
                    
                    tempSymbol = symbolExists(tokenList[i].name);
                    if (tempSymbol == nullptr){
                        // if it is a new section, add it to the symbol table and set the LC to 0
                        symbolTable.push_back(Symbol(tokenList[i].name, symbolTable.size(), LC, false, true, symbolTable.size(), true));
                        LC = 0;
                    }
                    else {
                        // if the section already exists, set the LC to the size of the section
                        LC = tempSymbol->sectionSize;
                    }
                    currentSection = tokenList[i].name;
                }
                else{
                    cout << "Error: Invalid section name" << endl;
                    exit(EXIT_FAILURE);
                }
                break;

            case ENDD:
                // set the size of the last section
                tempSymbol = symbolExists(currentSection);
                tempSymbol->setSectionSize(LC);
                break;
            
            
            default:
                break;
        }
    }
    return EXIT_SUCCESS;
}