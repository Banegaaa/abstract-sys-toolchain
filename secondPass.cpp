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


// helpers
int numOfBytes = 0;
int value = 0;
unsigned int machineCodeWord;
int registerNumber;
string registerName;

// we'll need a map of section names and their current LC values
std::unordered_map<std::string, int> sectionLCMap;





// we need a function that returns the section symbol based on the given section name,
// so we can write machine code for that section
Symbol* getSectionSymbol(string name){
    for (int i = 0; i < symbolTable.size(); i++){
        if (symbolTable[i].name == name && symbolTable[i].is_section){
            return &symbolTable[i];
        }
    }
    return NULL;
}

void twoRegisterInstructionMachineCode(int initialMachineCode, int &LC, int &i){
    machineCodeWord = initialMachineCode;
    i++; // get %
    i++; // get register
    registerName = tokenList[i].name;
    registerNumber = stoi(registerName.substr(1));
    // shift register number to the 4th byte (from the right) of the machine code word
    machineCodeWord |= (registerNumber << 12);
    
    i++; // get ,
    i++; // get %
    i++; // get register
    registerName = tokenList[i].name;
    registerNumber = stoi(registerName.substr(1));
    // shift register number to the 5th and 6th byte (from the right) of the machine code word
    machineCodeWord |= (registerNumber << 16);
    machineCodeWord |= (registerNumber << 20);
    
    // write machine code word to the current section
    tempSymbolSecondPass = getSectionSymbol(currentSection);
    tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
    LC += 4;
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
                numOfBytes = 0;
                i++; // get the number of bytes
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
                }
                break;

            case EXTERN:
                // nothing to do in second pass
                while (tokenList[i].value == EXTERN || tokenList[i].value == COMMA){
                    i++; // get the symbol name
                }
                break;

            case WORD:
                while(tokenList[i].value == WORD || tokenList[i].value == COMMA){
                    value = 0;
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
                    i++;
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

            case HALT:
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, 0x00000000);
                LC += 4;
                break;

            case INT:
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, 0x10000000);
                LC += 4;
                break;

            case IRET:
                // TODO: write machine code for IRET
                break;
            
            case RET:
                // TODO: write machine code for RET
                break;

            case CALL:
                // TODO: write machine code for CALL
                break;

            case JMP:
                // TODO: write machine code for JMP, should be the same as CALL
                break;

            case PUSH:
                // TODO: write machine code for PUSH
                break;

            case POPP:
                // TODO: write machine code for POPP
                break;

            case XCHG:
                // OC for XCHG is 0x40
                machineCodeWord = 0x40000000;
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                // remove "r" (first letter) from register name
                registerNumber = stoi(registerName.substr(1));
                // shift register number to the 5th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 16);
                i++; // get ,
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                registerNumber = stoi(registerName.substr(1));
                // shift register number to the 4th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 12);
                
                // write machine code word to the current section
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
                LC += 4;
                break;

            case ADD:
                twoRegisterInstructionMachineCode(0x50000000, LC, i);
                break;

            case SUB:
                twoRegisterInstructionMachineCode(0x51000000, LC, i);
                break;
            
            case MUL:
                twoRegisterInstructionMachineCode(0x52000000, LC, i);
                break;

            case DIV:
                twoRegisterInstructionMachineCode(0x53000000, LC, i);
                break;

            case NOT:
                // OC for NOT is 0x60000000
                machineCodeWord = 0x60000000;
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                registerNumber = stoi(registerName.substr(1));
                // shift register number to the 4th and 5th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 16);
                cout << "machineCodeWord: " << hex << machineCodeWord << endl;
                machineCodeWord |= (registerNumber << 20);
                cout << "machineCodeWord: " << hex << machineCodeWord << endl;
                
                // write machine code word to the current section
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
                LC += 4;
                break;

            case AND:
                twoRegisterInstructionMachineCode(0x61000000, LC, i);
                break;

            case OR:
                twoRegisterInstructionMachineCode(0x62000000, LC, i);
                break;

            case XOR:
                twoRegisterInstructionMachineCode(0x63000000, LC, i);
                break;

            case SHL:
                twoRegisterInstructionMachineCode(0x70000000, LC, i);
                break;

            case SHR:
                twoRegisterInstructionMachineCode(0x71000000, LC, i);
                break;
            
            case ENDD:
                // set the LC of the current section to the sectionLCMap just in case
                sectionLCMap[currentSection] = LC;
                // for each section, go through machine code and write out (on std::cout) in hex format each byte of the section size
                cout << "-------------------------------------" << endl;
                for (int j = 0; j < symbolTable.size(); j++){
                    cout << "-------------------------------------" << endl;
                    if (symbolTable[j].is_section){
                        cout << "SEKCIJA: " << symbolTable[j].name << endl;
                        for (int k = 0; k < symbolTable[j].machineCode.size(); k++){
                            cout << hex << symbolTable[j].machineCode[k] << endl;
                        }
                    }
                }

                break;
            default:
                break;
        }
    }
    // for each section, go through machine code and write out (on std::cout) in hex format each byte of the section siz
    // for (int i = 0; i < symbolTable.size(); i++){
    //     cout << "-------------------------------------" << endl;
    //     if (symbolTable[i].is_section){
    //         cout << "SEKCIJA: " << symbolTable[i].name << endl;
    //         for (int j = 0; j < symbolTable[i].sectionSize; j++){
    //             cout << hex << (int)symbolTable[i].machineCode[j] << endl;
    //         }
    //     }
    // }
    
    return EXIT_SUCCESS;

}