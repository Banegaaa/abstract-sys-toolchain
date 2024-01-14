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

// todo set section number only when the symbol is defined!



vector<Symbol> symbolTable;

// helpers:
string tempSymbolName = "";
Symbol* tempSymbol;
bool tempBool;



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

Symbol* getSectionSymbolFirstPass(string name){
    for (int i = 0; i < symbolTable.size(); i++){
        if (symbolTable[i].name == name && symbolTable[i].is_section){
            return &symbolTable[i];
        }
    }
    return NULL;
}


// we need a function that checks if the given int value can be stored in 3 bytes
bool canBeStoredIn3Bytes(int i){
    if (tokenList[i].value == DEC_LITERAL){
        unsigned int temp = stoi(tokenList[i].name);

        // max value representable in 12 bits (signed)  -> 2^11 - 1
        if (temp > 2047){
            return false;
        }
        else {
            return true;
        }
    }
    else {
        // tokenList[i].value == HEX_LITERAL
        unsigned int temp = hexToInt(tokenList[i].name);

        // count the number of bits in the number
        int count = 0;
        while (temp){
            count++;
            temp >>= 1;
        }
        if (count > 12){
            return false;
        }
        else {
            return true;
        }
    }
}


void jmpAndCallCase(int &LC, int &i){
    i++; // get call value
    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL || tokenList[i].value == NAME){
        if (tokenList[i].value == DEC_LITERAL){
            if (!canBeStoredIn3Bytes(i)){
                // add the literal to the sections literal table
                tempSymbol = getSectionSymbolFirstPass(currentSection);
                tempSymbol->addLiteral(new Literal(stoi(tokenList[i].name), LC, 4, tempSymbol->literals.size(), -1));
            }
        }
        else if (tokenList[i].value == HEX_LITERAL){
            if (!canBeStoredIn3Bytes(i)){
                // add the literal to the sections literal table
                tempSymbol = getSectionSymbolFirstPass(currentSection);
                tempSymbol->addLiteral(new Literal(hexToInt(tokenList[i].name), LC, 4, tempSymbol->literals.size(), -1));
            }
        }
        if (tokenList[i].value == NAME){
            // check if symbol with that name already exists in the symbol table
            tempSymbol = symbolExists(tokenList[i].name);
            if (tempSymbol == nullptr){
                symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false)); // TODO: check global flag
            }
        }
        LC += 4;
    }
    else{
        std::cout << "Error: Invalid call value" << endl;
        exit(EXIT_FAILURE);
    }
}

void twoRegisterInstructions(int &LC, int &i){
    i++; // get %
    i++; // get register
    i++; // get ,
    i++; // get %
    i++; // get register
    LC += 4;
}

void jumpInstructions(int &LC, int &i){
    i++; // get %
    i++; // get register
    i++; // get ,
    i++; // get %
    i++; // get register
    i++; // get ,
    i++; // get DEC_LITERAL or HEX_LITERAL or NAME

    if (tokenList[i].value == NAME){
        // check if symbol with that name already exists in the symbol table
        tempSymbol = symbolExists(tokenList[i].name);
        if (tempSymbol == nullptr){
            symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false)); // TODO: check global flag
        }
    
    }
    LC += 4;
}

void loadAndStoreOperandHandling(int &LC, int &i){
    // 1. case: operand := literal || hex_literal || name
    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL || tokenList[i].value == NAME){
        if (tokenList[i].value == NAME){
            // check if symbol with that name already exists in the symbol table
            tempSymbol = symbolExists(tokenList[i].name);
            if (tempSymbol == nullptr){
                symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false));
            }
        }
    }
    // 2. case: operand := %register
    else if (tokenList[i].value == PC_REL_ADR){
        i++; // get register

    }
    // 3. case: operand := $literal || $hex_literal || $name
    else if (tokenList[i].value == IMM_ADR){
        i++; // get literal || hex_literal || name
        if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL || tokenList[i].value == NAME){
            if (tokenList[i].value == NAME){
                // check if symbol with that name already exists in the symbol table
                tempSymbol = symbolExists(tokenList[i].name);
                if (tempSymbol == nullptr){
                    symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false));
                }
            }
        }
    }
    // 3. case: operand := [%register] || [%register + literal]
    else if (tokenList[i].value == REG_IND_ADR_SINGLE_DIGIT || tokenList[i].value == REG_IND_ADR_DOUBLE_DIGIT ||
      tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT ||
       tokenList[i].value == REG_POM_HEXL_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_HEXL_ADR_DOUBLE_DIGIT){
        // do nothing in first pass
    }
    else if (tokenList[i].value == REG_POM_S_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_S_ADR_DOUBLE_DIGIT){
        // split tokenList[i].name by +
        // and remove the last character from the second part, which is ']', that's how we will get the symbol name
        string delimiter = "+";
        string firstPart = tokenList[i].name.substr(0, tokenList[i].name.find(delimiter));
        string secondPart = tokenList[i].name.substr(tokenList[i].name.find(delimiter) + 1, tokenList[i].name.length() - 2);
        //remove ] from the second part
        secondPart = secondPart.substr(0, secondPart.length() - 1);

        // check if symbol with that name already exists in the symbol table
        tempSymbol = symbolExists(secondPart);
        if (tempSymbol == nullptr){
            symbolTable.push_back(Symbol(secondPart, -1, 0, false, false, symbolTable.size(), false));
        }
    }
    else{
        std::cout << "Error: Invalid load value" << endl;
        exit(EXIT_FAILURE);
    }
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
                    std::cout << "Error: Invalid skip size" << endl;
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

                    i++;
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
    
                    i++;
                }
                break;
            
            case WORD:
                while(tokenList[i].value == WORD || tokenList[i].value == COMMA){
                    i++;
                    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL || tokenList[i].value == NAME){
                        if (tokenList[i].value == NAME){
                            // check if symbol with that name already exists in the symbol table
                            tempSymbol = symbolExists(tokenList[i].name);
                            if (tempSymbol == nullptr){
                                symbolTable.push_back(Symbol(tokenList[i].name, -1, LC, false, false, symbolTable.size(), false));
                            }
                        }
                        LC += 4;
                    }
                    else{
                        std::cout << "Error: Invalid word value" << endl;
                        exit(EXIT_FAILURE);
                    }

                    i++;
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
                        currentSectionNumber = symbolTable.size() - 1;
                    }
                    else {
                        // if the section already exists, set the LC to the size of the section
                        LC = tempSymbol->sectionSize;
                        currentSectionNumber = tempSymbol->number;
                    }
                    currentSection = tokenList[i].name;
                }
                else{
                    std::cout << "Error: Invalid section name" << endl;
                    exit(EXIT_FAILURE);
                }
                break;

            case EQU:
                i++; // get symbol name
                if (tokenList[i].value == NAME){
                    tempSymbol = symbolExists(tokenList[i].name);
                    tempSymbolName = tokenList[i].name;
                    if (tempSymbol == nullptr){
                        i++; // comma
                        i++; // get symbol value
                        if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
                            symbolTable.push_back(Symbol(tempSymbolName, currentSectionNumber, LC, false, false, symbolTable.size(), true));
                            tempSymbol = &symbolTable[symbolTable.size() - 1];
                            if (tokenList[i].value == DEC_LITERAL){
                                tempSymbol->setValue(stoi(tokenList[i].name));
                            }
                            else{
                                tempSymbol->setValue(hexToInt(tokenList[i].name));
                            }
                        }
                        else{
                            std::cout << "Error: Invalid symbol value" << endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                    else {
                        i++; // comma
                        i++; // get symbol value
                        if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
                            if (tokenList[i].value == DEC_LITERAL){
                                tempSymbol->setValue(stoi(tokenList[i].name));
                            }
                            else{
                                tempSymbol->setValue(hexToInt(tokenList[i].name));
                            }
                            tempSymbol->is_defined = true;
                        }
                        else{
                            std::cout << "Error: Invalid symbol value" << endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else{
                    std::cout << "Error: Invalid symbol name" << endl;
                    exit(EXIT_FAILURE);
                }
                break;

            case LABEL: // todo: is lc the value of label symbols? WE DID IT
                // label is in format: name:
                tempSymbolName = tokenList[i].name.substr(0, tokenList[i].name.length() - 1);
                tempSymbol = symbolExists(tempSymbolName);
                if (tempSymbol == nullptr){
                    symbolTable.push_back(Symbol(tempSymbolName, currentSectionNumber, LC, false, false, symbolTable.size(), true));
                    // set the value to lc
                    tempSymbol = &symbolTable[symbolTable.size() - 1];
                    tempSymbol->setValue(LC);

                }
                else{
                    tempSymbol->is_defined = true;
                    tempSymbol->section = currentSectionNumber;
                    tempSymbol->lc_value = LC;
                    tempSymbol->value = LC;
                }
                break;
            
            case HALT:
                LC += 4;
                // set the size of the last section
                tempSymbol = symbolExists(currentSection);
                tempSymbol->setSectionSize(LC);
                break;
            
            case INT:
                LC += 4;
                // set the size of the last section
                tempSymbol = symbolExists(currentSection);
                tempSymbol->setSectionSize(LC);
                break;
            
            case IRET:
                LC += 8;
                break;
            
            case RET:
                LC += 4;
                break;

            case CALL:
                jmpAndCallCase(LC, i);
                break;

            case JMP:
                jmpAndCallCase(LC, i);
                break;
            
            case PUSH:
                i++; //get %
                i++; // get register
                LC += 4;
                break;
            
            case POPP:
                i++; //get %
                i++; // get register
                LC += 4;
                break;

            case XCHG:
                twoRegisterInstructions(LC, i);
                break;
            
            case ADD:
                twoRegisterInstructions(LC, i);
                break;
            
            case SUB:
                twoRegisterInstructions(LC, i);
                break;

            case MUL:
                twoRegisterInstructions(LC, i);
                break;
            
            case DIV:
                twoRegisterInstructions(LC, i);
                break;
            
            case AND:
                twoRegisterInstructions(LC, i);
                break;
            
            case OR:
                twoRegisterInstructions(LC, i);
                break;
            
            case XOR:
                twoRegisterInstructions(LC, i);
                break;

            case NOT:
                i++; // get %
                i++; // get register
                LC += 4;
                break;
            
            case SHL:
                twoRegisterInstructions(LC, i);
                break;
            
            case SHR:
                twoRegisterInstructions(LC, i);
                break;

            case BEQ:
                jumpInstructions(LC, i);
                break;
            
            case BNE:
                jumpInstructions(LC, i);
                break;
            
            case BGT:
                jumpInstructions(LC, i);
                break;
            
            case LD:
                i++; // get operand
                loadAndStoreOperandHandling(LC, i);
                i++; // get ,
                i++; // get %
                i++; // get register
                LC += 4;
                break;

            case ST:
                i++; // get %
                i++; // get register
                i++; // get ,
                i++; // get operand
                loadAndStoreOperandHandling(LC, i);
                LC += 4;
                break;

            case CSRRD:
                i++; // get %
                i++; // get one of the special registers
                if (tokenList[i].value != STATUS_REG && tokenList[i].value != HANDLER_REG && tokenList[i].value != CAUSE_REG){
                    std::cout << "Error: Invalid special register" << endl;
                    exit(EXIT_FAILURE);
                }
                i++; // get ,
                i++; // get %
                i++; // get register
                LC += 4;
                break;
            
            case CSRWR:
                i++; // get %
                i++; // get register
                i++; // get ,
                i++; // get %
                i++; // get one of the special registers
                if (tokenList[i].value != STATUS_REG && tokenList[i].value != HANDLER_REG && tokenList[i].value != CAUSE_REG){
                    std::cout << "Error: Invalid special register" << endl;
                    exit(EXIT_FAILURE);
                }
                LC += 4;
                break;

            case ENDD:
                // set the size of the last section
                tempSymbol = symbolExists(currentSection);
                tempSymbol->setSectionSize(LC);
                std::cout << "Symbol table:" << endl;
                for (int i = 0; i < symbolTable.size(); i++){
                    std::cout << symbolTable[i].to_string() << endl;
                }
                // for each section, allocate memory for the machine code (equal to the size of the section)
                for (int i = 0; i < symbolTable.size(); i++){
                    if (symbolTable[i].is_section){
                        symbolTable[i].allocateMachineCodeMemory(symbolTable[i].sectionSize/4);
                    }
                }
                return EXIT_SUCCESS;
                
            default:
                break;
        }
    }
    // print the symbol table:
    std::cout << "Symbol table:" << endl;
    for (int i = 0; i < symbolTable.size(); i++){
        std::cout << symbolTable[i].to_string() << endl;
    }

    // for each section, allocate memory for the machine code (equal to the size of the section)
    for (int i = 0; i < symbolTable.size(); i++){
        if (symbolTable[i].is_section){
            symbolTable[i].allocateMachineCodeMemory(symbolTable[i].sectionSize/4);
            for (int j = 0; j < symbolTable[i].literals.size(); j++){
                symbolTable[i].allocateAndPopulateMachineCodeMemory(symbolTable[i].literals[j]->size, symbolTable[i].literals[j]->value);
            }
        }

    }

    return EXIT_SUCCESS;

    //  TODO: NA KRAJU DRUGOG PROLAZA POPUNI MACHINE CODE ZA LITERALE
}