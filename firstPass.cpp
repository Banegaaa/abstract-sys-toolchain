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
int tempInt; // needed for LD/ST instruction token value (because we need to know if it is LD or ST in the operand handling)

// todo set section number only when the symbol is defined!



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
        if (symbolTable[j].name == name && symbolTable[j].is_section){
            return symbolTable[j].number;
        }
    }
    return -1;
}


bool canFitIn12Bits(string valueStr, int i){
    if (tokenList[i].value == DEC_LITERAL){
        int value = stoi(valueStr);
        return value <= 2047;
    }
    else if (tokenList[i].value == HEX_LITERAL){
        int value = hexToInt(valueStr);
        return value <= 2047;
    }
    else{
        std::cout << "Error: Invalid call value" << endl;
        exit(EXIT_FAILURE);
    }
}

bool literalExists(int value, string currentSection){
    for (int i = 0; i < symbolTable.size(); i++){
        if (symbolTable[i].name == currentSection){
            for (int j = 0; j < symbolTable[i].literals.size(); j++){
                if (symbolTable[i].literals[j]->value == value){
                    return true;
                }
            }
        }
    }
    return false;
}

void jmpAndCallCase(int &LC, int &i){
    i++; // get call/jmp value
    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL || tokenList[i].value == NAME){
        if (tokenList[i].value == NAME){
            // check if symbol with that name already exists in the symbol table
            tempSymbol = symbolExists(tokenList[i].name);
            if (tempSymbol == nullptr){
                symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false)); // TODO: check global flag
            }
        }
        else {
            // check if we need to add literal to pool of literals
            if (tokenList[i].value == DEC_LITERAL){
                tempSymbol = symbolExists(currentSection);
                if (!canFitIn12Bits(tokenList[i].name, i)){
                    tempSymbol->addLiteral(new Literal(stoi(tokenList[i].name), LC, sizeof(stoi(tokenList[i].name)), tempSymbol->literals.size(), -1));
                }
            }
            else if (tokenList[i].value == HEX_LITERAL){
                tempSymbol = symbolExists(currentSection);
                if (!canFitIn12Bits(tokenList[i].name, i)){
                    tempSymbol->addLiteral(new Literal(hexToInt(tokenList[i].name), LC, sizeof(hexToInt(tokenList[i].name)), tempSymbol->literals.size(), -1));
                }
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
    else {  // Literal
        // check if we need to add literal to pool of literals
        if (tokenList[i].value == DEC_LITERAL){
            tempSymbol = symbolExists(currentSection);
            if (!canFitIn12Bits(tokenList[i].name, i)){
                tempSymbol->addLiteral(new Literal(stoi(tokenList[i].name), LC, sizeof(stoi(tokenList[i].name)), tempSymbol->literals.size(), -1));
            }
        }
        else if (tokenList[i].value == HEX_LITERAL){
            tempSymbol = symbolExists(currentSection);
            if (!canFitIn12Bits(tokenList[i].name, i)){
                tempSymbol->addLiteral(new Literal(hexToInt(tokenList[i].name), LC, sizeof(hexToInt(tokenList[i].name)), tempSymbol->literals.size(), -1));
            }
        }
    }
    LC += 4;
}


void loadOperand(int instructionTokenValue, int &LC, int &i){

    if (tokenList[i].value == IMM_ADR){
        i++; // get literal || hex_literal || name
        int value = 0;
        if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
            // OPERAND == $literal || $hex_literal
            if (tokenList[i].value == DEC_LITERAL){
                value = stoi(tokenList[i].name);
            }
            else {  // HEX_LITERAL
                value = hexToInt(tokenList[i].name);
            }
            if (!canFitIn12Bits(tokenList[i].name, i)){
                tempSymbol = symbolExists(currentSection);
                tempSymbol->addLiteral(new Literal(value, LC, sizeof(value), tempSymbol->literals.size(), -1));
            }
        }
        else {
            // OPERAND == $name

            // check if symbol with that name already exists in the symbol table
            tempSymbol = symbolExists(tokenList[i].name);
            if (tempSymbol == nullptr){
                symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false));
            }
        }
        LC += 4;
    }
    else if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){  // load mem32[literal]
        // OPERAND == literal || hex_literal

        // Here, we will always create a literal, no matter if it can fit in 12 bits or not
        // We will code this with 2 instructions -> LC += 8

        int value = 0;
        if (tokenList[i].value == DEC_LITERAL){
            value = stoi(tokenList[i].name);
        }
        else { // HEX_LITERAL
            value = hexToInt(tokenList[i].name);
        }
        if (!literalExists(value, currentSection)){
            tempSymbol = symbolExists(currentSection);
            tempSymbol->addLiteral(new Literal(value, LC, sizeof(value), tempSymbol->literals.size(), -1));
        }
        LC += 8;
    }
    else if (tokenList[i].value == NAME){  // load mem32[symbol->value]
        // OPERAND == name

        // check if the symbol already exists, if it doesn't, add it to the symbol table
        // We will always do this with 2 instructions -> LC += 8

        tempSymbol = symbolExists(tokenList[i].name);
        if (tempSymbol == nullptr){
            symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false));
        }
        LC += 8;
    }
    else if (tokenList[i].value == PC_REL_ADR){ // %
        // OPERAND == %register

        i++; // get register
        if (tokenList[i].value != REG){
            std::cout << "Error: Invalid register" << endl;
            exit(EXIT_FAILURE);
        }
        LC += 4;
    }
    else if (tokenList[i].value == REG_IND_ADR_SINGLE_DIGIT || tokenList[i].value == REG_IND_ADR_DOUBLE_DIGIT){
        // OPERAND == [%register]
        LC += 4;
    }
    else if (tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT){
        // OPERAND == [%register + dec_literal]

        // extract the dec_literal, and see if it fits to 12b, if it doesn't -> error
        string delimiter = "+";
        string operand = tokenList[i].name;
        string extracted = operand.substr(operand.find(delimiter) + 1, operand.length() - 2);
        //remove blank space on the begginning of extracted, and also remove ] from the end
        extracted = extracted.substr(1, extracted.length() - 2);
        cout << "FULLY EXTRACTED: " << extracted << endl;

        
        if (stoi(extracted) > 2047){  // TODO: converted into signed?
            std::cout << "Error: Literal doesn't fit in 12 bits" << endl;
            exit(EXIT_FAILURE);
        }
        // TODO: Should we put literal in the literal pool?
        
        LC += 4;
    }
    else if (tokenList[i].value == REG_POM_HEXL_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_HEXL_ADR_DOUBLE_DIGIT){
        // OPERAND == [%register + hex_literal]

        // extract the hex_literal, and see if it fits to 12b, if it doesn't -> error
        string delimiter = "+";
        string operand = tokenList[i].name;
        string extracted = operand.substr(operand.find(delimiter) + 1, operand.length() - 2);
        //remove ] from the second part
        extracted = extracted.substr(1, extracted.length() - 2);
        cout << "FULLY EXTRACTED: " << extracted << endl;

        if (hexToInt(extracted) > 2047){  // TODO: converted into signed?
            std::cout << "Error: Literal doesn't fit in 12 bits" << endl;
            exit(EXIT_FAILURE);
        }
        LC += 4;
    }
    else if (tokenList[i].value == REG_POM_S_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_S_ADR_DOUBLE_DIGIT){
        // OPERAND == [%register + name]
        // extract the name, and see if the symbol exists, if it doesn't -> add it to the symbol table
        string delimiter = "+";
        string operand = tokenList[i].name;
        string extracted = operand.substr(operand.find(delimiter) + 1, operand.length() - 2);
        //remove ] from the second part
        extracted = extracted.substr(1, extracted.length() - 2);
        cout << "FULLY EXTRACTED: " << extracted << endl;


        tempSymbol = symbolExists(extracted);
        if (tempSymbol == nullptr){
            symbolTable.push_back(Symbol(extracted, -1, 0, false, false, symbolTable.size(), false));
        }
        LC += 4;
    }
    else{
        std::cout << "Error: Invalid load value" << endl;
        exit(EXIT_FAILURE);
    }
}


void storeOperand(int &LC, int i){
    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
        // OPERAND == literal || hex_literal
        if (!canFitIn12Bits(tokenList[i].name, i)){
            tempSymbol = symbolExists(currentSection);
            if (tokenList[i].value == DEC_LITERAL){
                tempSymbol->addLiteral(new Literal(stoi(tokenList[i].name), LC, sizeof(stoi(tokenList[i].name)), tempSymbol->literals.size(), -1));
            }
            else {  // HEX_LITERAL
                tempSymbol->addLiteral(new Literal(hexToInt(tokenList[i].name), LC, sizeof(hexToInt(tokenList[i].name)), tempSymbol->literals.size(), -1));
            }
        }
        LC += 4;
    }
    else if (tokenList[i].value == NAME){
        // OPERAND == name
        // check if the symbol already exists, if it doesn't, add it to the symbol table
        tempSymbol = symbolExists(tokenList[i].name);
        if (tempSymbol == nullptr){
            symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, false, false, symbolTable.size(), false));
        }
        LC += 4;
    }
    else if (tokenList[i].value == REG_IND_ADR_SINGLE_DIGIT || tokenList[i].value == REG_IND_ADR_DOUBLE_DIGIT){
        LC += 4;
    }
    else if (tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT || 
    tokenList[i].value == REG_POM_HEXL_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_HEXL_ADR_DOUBLE_DIGIT){
        // OPERAND == [%register + dec_literal] || [%register + hex_literal]

        // extract the dec_literal, and see if it fits to 12b, if it doesn't -> error
        string delimiter = "+";
        string operand = tokenList[i].name;
        string extracted = operand.substr(operand.find(delimiter) + 1, operand.length() - 2);
        //remove blank space on the begginning of extracted, and also remove ] from the end
        extracted = extracted.substr(1, extracted.length() - 2);
        cout << "FULLY EXTRACTED: " << extracted << endl;

        // Check if the value can fit in 12b
        int value;
        if (tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT){
            value = stoi(extracted);
        }
        else {
            value = hexToInt(extracted);
        }   
        if (value > 2047){  // TODO: converted into signed?
            std::cout << "Error: Literal doesn't fit in 12 bits" << endl;
            exit(EXIT_FAILURE);
        }

        LC += 4;
    }
    else if (tokenList[i].value == REG_POM_S_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_S_ADR_DOUBLE_DIGIT){
        // OPERAND == [%register + name]
        // extract the name, and see if the symbol exists, if it doesn't -> add it to the symbol table
        string delimiter = "+";
        string operand = tokenList[i].name;
        string extracted = operand.substr(operand.find(delimiter) + 1, operand.length() - 2);
        //remove ] from the second part
        extracted = extracted.substr(1, extracted.length() - 2);
        cout << "FULLY EXTRACTED: " << extracted << endl;

        // check if the symbol already exists, if it doesn't, add it to the symbol table
        tempSymbol = symbolExists(extracted);
        if (tempSymbol == nullptr){
            symbolTable.push_back(Symbol(extracted, -1, 0, false, false, symbolTable.size(), false));
        }
        LC += 4;
    }
    else {
        // Invalid operand
        std::cout << "Error: Invalid store value" << endl;
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
                tempInt = tokenList[i].value;
                i++; // get operand
                loadOperand(tempInt, LC, i);  // This will set LC
                i++; // get ,
                i++; // get %
                i++; // get register
                break;

            case ST:
                tempInt = tokenList[i].value;
                i++; // get %
                i++; // get register
                i++; // get ,
                i++; // get operand
                storeOperand(LC, i); // This will set LC
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

                std::cout << "LITERALI" << endl;
                for (int i = 0; i < symbolTable.size(); i++){
                    if (symbolTable[i].is_section && symbolTable[i].literals.size() > 0){
                        for (int j = 0; j < symbolTable[i].literals.size(); j++){
                            std::cout << hex << symbolTable[i].literals[j]->value << endl;
                        }
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
    return EXIT_SUCCESS;
}