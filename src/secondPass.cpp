#include <iostream>
#include <string>
#include <vector>
#include "../inc/token.h"
#include "../inc/define.h"
#include "../inc/symbol.h"
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
extern bool canFitIn12Bits(string valueStr, int i);

string tempSymbolNameSecondPass;
Symbol* tempSymbolSecondPass;


// helpers
int numOfBytes = 0;
int value = 0;
unsigned int machineCodeWord;
int registerNumber;
string registerName;

RelocationSymbol* relSym;

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


int literalExists(int value){ // returns the index of the literal if it exists in the current section, -1 otherwise
    tempSymbolSecondPass = getSectionSymbol(currentSection);
    for (int i = 0; i < tempSymbolSecondPass->literals.size(); i++){
        if (tempSymbolSecondPass->literals[i]->value == value){
            return i;
        }
    }
    return -1;
}

int findLiteralLocation(int value){ // returns the location of the literal if it exists in the current section, -1 otherwise
    // We will use this to calculate offset from current LC to the location of the literal
    // so we can calculate the correct addres from which we will get the literal from memory during emulation!

    tempSymbolSecondPass = getSectionSymbol(currentSection);
    for (int i = 0; i < tempSymbolSecondPass->literals.size(); i++){
        if (tempSymbolSecondPass->literals[i]->value == value){
            return tempSymbolSecondPass->literals[i]->location;
        }
    }
    return -1;
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


void callAndJmpCase(int &LC, int &i){
    int operation = tokenList[i].value; // CALL or JMP, we'll use this to set the machine code word
    i++;
    int literalLocation;  // literal location
    int index;
    
    // *** we have two different instruction codes, one if the literal/symbol_value can fit in 3 bytes and another one if it can't ***
    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
        if (tokenList[i].value == DEC_LITERAL){
            value = stoi(tokenList[i].name);
        }
        else if (tokenList[i].value == HEX_LITERAL){
            value = hexToInt(tokenList[i].name);
        }

        if (canFitIn12Bits(tokenList[i].name, i)){
            if (operation == CALL){
                machineCodeWord = 0x20F00000;
            }
            else {  // operation == JMP
                machineCodeWord = 0x30F00000;
            }
            
            machineCodeWord |= value;
        }
        else {
            if (operation == CALL){
                machineCodeWord = 0x21F00000;
            }
            else {  // operation == JMP
                machineCodeWord = 0x38F00000;
            }
            literalLocation = findLiteralLocation(value);
            if (literalLocation == -1){  // This should never happen, TODO: PROVE IT
                // create a new literal
                std::cout << "This should never happen" << std::endl;
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                Literal* newLiteral = new Literal(value, 0, sizeof(value), tempSymbolSecondPass->literals.size(), -1);
                tempSymbolSecondPass->addLiteral(newLiteral);
            }
            machineCodeWord |= (literalLocation - LC);
        }
    }
    else {  // tokenList[i].value == NAME
        // IN ANY CASE, THE VALUE OF THE SYMBOL WILL BE SET IN THE POOL OF LITERALS (machineCodeWord = mem32[...])
        if (operation == CALL){
            machineCodeWord = 0x21F00000;
        }
        else {  // operation == JMP
            machineCodeWord = 0x38F00000;
        }

        tempSymbolSecondPass = symbolExists(tokenList[i].name);
        int symbolIndex = tempSymbolSecondPass->number;

        tempSymbolSecondPass = getSectionSymbol(currentSection);
        index = tempSymbolSecondPass->literals.size();
        Literal* newLiteral = new Literal(0, LC, 4, index, symbolIndex);  // we don't know the value of the symbol yet, so we'll set it to 0, size is 4 bytes
        tempSymbolSecondPass->addLiteral(newLiteral);

        // Create a relocation entry
        relSym = new RelocationSymbol(symbolIndex, LC, index);
        tempSymbolSecondPass->relocationTable.push_back(relSym);

        // set index to the last 3 bytes of the literal
        machineCodeWord |= index;
    }

    tempSymbolSecondPass = getSectionSymbol(currentSection);
    tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
    LC += 4;
}


void jumpInstructionsCase(int &LC, int &i){
    int operation = tokenList[i].value; // BNE, BEQ, BGT
    i++; // get %
    i++; // get register
    int reg1 = stoi(tokenList[i].name.substr(1));
    
    i++; // get ,
    i++; // get %
    i++; // get register
    int reg2 = stoi(tokenList[i].name.substr(1));

    i++; // get ,
    i++; // get operand

    int literalLocation;  // literal locatio
    int index;

    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
        if (tokenList[i].value == DEC_LITERAL){
            value = stoi(tokenList[i].name);
        }
        else if (tokenList[i].value == HEX_LITERAL){
            value = hexToInt(tokenList[i].name);
        }

        if (canFitIn12Bits(tokenList[i].name, i)){
            if (operation == BEQ){
                machineCodeWord = 0x31F00000;
            }
            else if (operation == BNE){
                machineCodeWord = 0x32F00000;
            }
            else {  // operation == BGT
                machineCodeWord = 0x33F00000;
            }

            machineCodeWord |= (reg1 << 16);
            machineCodeWord |= (reg2 << 12);
            machineCodeWord |= value;
        }
        else {
            if (operation == BEQ){
                machineCodeWord = 0x39F00000;
            }
            else if (operation == BNE){
                machineCodeWord = 0x3AF00000;
            }
            else {  // operation == BGT
                machineCodeWord = 0x3BF00000;
            }

            literalLocation = findLiteralLocation(value);
            if (literalLocation == -1){
                // create a new literal
                std::cout << "This should never happen" << std::endl;
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                Literal* newLiteral = new Literal(value, LC, sizeof(value), tempSymbolSecondPass->literals.size(), -1);
                tempSymbolSecondPass->addLiteral(newLiteral);
            }
            machineCodeWord |= (reg1 << 16);
            machineCodeWord |= (reg2 << 12);
            machineCodeWord |= (literalLocation - LC);
        }
    }
    else {  // tokenList[i].value == NAME
        // IN ANY CASE, THE VALUE OF THE SYMBOL WILL BE SET IN THE POOL OF LITERALS (machineCodeWord = mem32[...])
        if (operation == BEQ){
            machineCodeWord = 0x39F00000;
        }
        else if (operation == BNE){
            machineCodeWord = 0x3AF00000;
        }
        else {  // operation == BGT
            machineCodeWord = 0x3BF00000;
        }

        machineCodeWord |= (reg1 << 16);
        machineCodeWord |= (reg2 << 12);

        tempSymbolSecondPass = symbolExists(tokenList[i].name);
        int symbolIndex = tempSymbolSecondPass->number;        

        tempSymbolSecondPass = getSectionSymbol(currentSection);
        index = tempSymbolSecondPass->literals.size();
        Literal* newLiteral = new Literal(0, LC, 4, index, symbolIndex);
        tempSymbolSecondPass->addLiteral(newLiteral);

        // Create a relocation entry
        relSym = new RelocationSymbol(symbolIndex, LC, index);
        tempSymbolSecondPass->relocationTable.push_back(relSym);

        // set index to the last 3 bytes of the literal
        machineCodeWord |= index;
    }

    tempSymbolSecondPass = getSectionSymbol(currentSection);
    tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
    LC += 4;
}


void loadInstruction(int &LC, int &i){
    i++; // get operand

    int literalLocation; // literal location
    int index;

    if (tokenList[i].value == IMM_ADR){
        i++; // get literal/symbol
        
        if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
            if (tokenList[i].value == DEC_LITERAL){
                value = stoi(tokenList[i].name);
            }
            else if (tokenList[i].value == HEX_LITERAL){
                value = hexToInt(tokenList[i].name);
            }
            if (canFitIn12Bits(tokenList[i].name, i)){
                // If it fits, OC = 0x9, M = 0x1, A = regNum, B = 0, C = 0 (regNum will be set later, this is only for the "operand" part)
                machineCodeWord = 0x91000000;
                machineCodeWord |= value;
            }
            else {
                // If it doesn't fit, OC = 0x9, M = 0x2, A = regNum, B = 15 (pc rel), C = 0
                machineCodeWord = 0x92000000;
                registerNumber = 15;  // pc
                machineCodeWord |= (registerNumber << 16);

                literalLocation = findLiteralLocation(value);  // it will exist, because we set it in the first pass
                machineCodeWord |= (literalLocation - LC);
            }
        }
        else {
            // symbol
            tempSymbolSecondPass = symbolExists(tokenList[i].name);

            int symbolIndex = tempSymbolSecondPass->number;

            tempSymbolSecondPass = getSectionSymbol(currentSection);
            tempSymbolSecondPass->literals.push_back(new Literal(0, LC, 4, tempSymbolSecondPass->literals.size(), symbolIndex));
            std::cout << "LC LITERALA JE: " << LC << std::endl;

            // Create relocation entry
            relSym = new RelocationSymbol(symbolIndex, LC, tempSymbolSecondPass->literals.size() - 1);
            tempSymbolSecondPass->relocationTable.push_back(relSym);

            // machine code just as the case above (where the value doesn't fit in 12 bits)
            machineCodeWord = 0x92000000;
            registerNumber = 15;  // pc
            machineCodeWord |= (registerNumber << 16);

            machineCodeWord |= tempSymbolSecondPass->literals.size() - 1;  // the index of the last literal in the pool of literals
        }

        // finish up the instruction
        i++; // get ,
        i++; // get %
        i++; // get register
        registerName = tokenList[i].name;
        registerNumber = stoi(registerName.substr(1));
        machineCodeWord |= (registerNumber << 20);

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);

        LC+=4;
    }
    else if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
        // This will be coded as 2 instructions
        // -> 2 times OC: 0x9, M = 0x2
        
        if (tokenList[i].value == DEC_LITERAL){
            value = stoi(tokenList[i].name);
        }
        else if (tokenList[i].value == HEX_LITERAL){
            value = hexToInt(tokenList[i].name);
        }

        // 1. B = 15, C = 0, A = regNum  (gpr[A] <= mem32[pc + D])  //TODO: should this be gpr[A] <= PC + D?

        machineCodeWord = 0x92000000;
        registerNumber = 15;  // pc
        machineCodeWord |= (registerNumber << 16);

        literalLocation = findLiteralLocation(value);  // it will exist
        machineCodeWord |= (literalLocation - LC);

        // finish up the instruction
        i++; // get ,
        i++; // get %
        i++; // get register
        registerName = tokenList[i].name;
        registerNumber = stoi(registerName.substr(1));
        machineCodeWord |= (registerNumber << 20);

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
        LC += 4;

        // 2. OC = 0x9, M = 0x2, A = regNum, B = A, C = 0, D = 0  (gprA <= mem32[gprA])
        machineCodeWord = 0x92000000;
        machineCodeWord |= (registerNumber << 20);  // A
        machineCodeWord |= (registerNumber << 16); // B

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
        LC += 4;
    }
    else if (tokenList[i].value == NAME){
        // This will be coded as 2 instructions
        // -> 2 times OC: 0x9, M = 0x2

        // 1. B = 15, C = 0, A = regNum  (gpr[A] <= mem32[pc + D])

        machineCodeWord = 0x92000000;
        registerNumber = 15;  // pc
        machineCodeWord |= (registerNumber << 16);

        tempSymbolSecondPass = symbolExists(tokenList[i].name);
        int symbolIndex = tempSymbolSecondPass->number;

        // Add symbol value to the pool of literals
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        index = tempSymbolSecondPass->literals.size();
        Literal* newLiteral = new Literal(0, LC, 4, index, symbolIndex);
        tempSymbolSecondPass->addLiteral(newLiteral);

        // Create a relocation entry
        relSym = new RelocationSymbol(symbolIndex, LC, index);
        tempSymbolSecondPass->relocationTable.push_back(relSym);

        // set index to the last 3 bytes of the literal
        machineCodeWord |= index;

        // Finish up the instruction
        i++; // get ,
        i++; // get %
        i++; // get register
        registerName = tokenList[i].name;
        registerNumber = stoi(registerName.substr(1));
        machineCodeWord |= (registerNumber << 20);

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
        LC += 4;

        // 2. OC = 0x9, M = 0x2, A = regNum, B = A, C = 0, D = 0  (gprA <= mem32[gprA])
        machineCodeWord = 0x92000000;
        machineCodeWord |= (registerNumber << 20);  // A
        machineCodeWord |= (registerNumber << 16); // B

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
        
        LC += 4;
    }
    else if (tokenList[i].value == PC_REL_ADR){  // %
        i++;  // get register
        registerName = tokenList[i].name;
        registerNumber = stoi(registerName.substr(1));  // regB

        machineCodeWord = 0x91000000;
        machineCodeWord |= (registerNumber << 16);  // regB

        // Finish up the instruction
        i++; // get ,
        i++; // get %
        i++; // get register
        registerName = tokenList[i].name;
        registerNumber = stoi(registerName.substr(1));  // regA
        machineCodeWord |= (registerNumber << 20);  // regA

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
        
        LC += 4;
    }
    else if (tokenList[i].value == REG_IND_ADR_SINGLE_DIGIT || tokenList[i].value == REG_IND_ADR_DOUBLE_DIGIT){
        machineCodeWord = 0x92000000;

        // We'll get register name from [%ri] by removing [% and ]
        registerName = tokenList[i].name.substr(2, tokenList[i].name.size() - 3);
        registerNumber = stoi(registerName);
        machineCodeWord |= (registerNumber << 16);  // regB

        // Finish up the instruction
        i++; // get ,
        i++; // get %
        i++; // get register
        registerName = tokenList[i].name;
        registerNumber = stoi(registerName.substr(1));  // regA
        machineCodeWord |= (registerNumber << 20);  // regA

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);

        LC += 4;
    }
    else if (tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT ||
     tokenList[i].value == REG_POM_HEXL_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_HEXL_ADR_DOUBLE_DIGIT){
        // operand is [%ri + D]
        // first we need to extract i and D
        string operand = tokenList[i].name;
        size_t percentPos = operand.find('%');
        size_t plusPos = operand.find('+');
        size_t closingBracketPos = operand.find(']');

        if (percentPos != std::string::npos && plusPos != std::string::npos && closingBracketPos != std::string::npos) {
            // Extract the substring between '%' and '+', and between '+' and ']'
            std::string iStr = operand.substr(percentPos + 1, plusPos - percentPos - 1); // ri -> we'll remove the r before int conversion
            std::string DStr = operand.substr(plusPos + 1, closingBracketPos - plusPos - 1);  // " D" -> we'll remove the space before int conversion

            // Convert the extracted substrings to integers
            registerNumber = std::stoi(iStr.substr(1));
            int D;
            if (tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT){
                D = std::stoi(DStr.substr(1));  // D (this literal will definately fit in 12 bits)
            }
            else {
                D = hexToInt(DStr.substr(1));  // D (this literal will definately fit in 12 bits)
            }

            machineCodeWord = 0x92000000;
            machineCodeWord |= (i << 16);  // regB
            machineCodeWord |= D;  // D

            // Finish up the instruction
            i++; // get ,
            i++; // get %
            i++; // get register
            registerName = tokenList[i].name;
            registerNumber = stoi(registerName.substr(1));  // regA
            machineCodeWord |= (registerNumber << 20);  // regA

            // write machine code word to the current section
            tempSymbolSecondPass = getSectionSymbol(currentSection);
            tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);

            LC += 4;
        }
        else {
            cout << "ERROR: Invalid operand" << endl;
            exit(EXIT_FAILURE);
        }
    }
    else if (tokenList[i].value == REG_POM_S_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_S_ADR_DOUBLE_DIGIT){
        cout << "ERROR: Invalid operand, the value of the symbol is not known during asembling, so ld " << tokenList[i].name << " cannot be done." << endl;
        exit(EXIT_FAILURE);
    }
}


void storeInstruction(int &LC, int &i){
    i++; // get %
    i++; // get register
    int reg1 = stoi(tokenList[i].name.substr(1));  // regC

    i++; // get ,
    i++; // get operand

    int literalLocation; // literal location
    int index;

    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
        
        if (tokenList[i].value == DEC_LITERAL){
            value = stoi(tokenList[i].name);
        }
        else if (tokenList[i].value == HEX_LITERAL){
            value = hexToInt(tokenList[i].name);
        }
        
        if (canFitIn12Bits(tokenList[i].name, i)){
            // OC = 0x8, M = 0x0, A = 0, B = 0, C = regNum, D = value
            machineCodeWord = 0x80000000;
            machineCodeWord |= (reg1 << 12);  // regC
            machineCodeWord |= value;
        }
        else {
            // OC = 0x8, M = 0x2, A = 15 (pc), B = 0, C = regNum, D = value
            machineCodeWord = 0x82000000;
            registerNumber = 15;  // pc
            machineCodeWord |= (registerNumber << 20);  // regA
            machineCodeWord |= (reg1 << 12);  // regC

            literalLocation = findLiteralLocation(value);  // it will exist, because we set it in the first pass
            machineCodeWord |= (literalLocation - LC);
        }

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);

        LC += 4;
    }
    else if (tokenList[i].value == NAME){
        // get the symbol value
        tempSymbolSecondPass = symbolExists(tokenList[i].name);
        int symbolIndex = tempSymbolSecondPass->number;

        tempSymbolSecondPass = getSectionSymbol(currentSection);
        index = tempSymbolSecondPass->literals.size();
        tempSymbolSecondPass->literals.push_back(new Literal(0, LC, 4, tempSymbolSecondPass->literals.size(), symbolIndex));
        std::cout << "LC LITERALA JE: " << LC << std::endl;

        // Create a relocation entry
        relSym = new RelocationSymbol(symbolIndex, LC, tempSymbolSecondPass->literals.size() - 1);
        tempSymbolSecondPass->relocationTable.push_back(relSym);

        // OC = 0x8, M = 0x2, A = 15 (pc), B = 0, C = regNum, D = index
        machineCodeWord = 0x82000000;
        registerNumber = 15;  // pc
        machineCodeWord |= (registerNumber << 20);  // regA
        machineCodeWord |= (reg1 << 12);  // regC
        machineCodeWord |= index;

        // write machine code word to the current section
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);

        LC += 4;
    }
    else if (tokenList[i].value == REG_IND_ADR_SINGLE_DIGIT || tokenList[i].value == REG_IND_ADR_DOUBLE_DIGIT){
        // operand == [%ri]
        machineCodeWord = 0x80000000;

        // We'll get register name from [%ri] by removing [% and ]
        registerName = tokenList[i].name.substr(2, tokenList[i].name.size() - 3);
        registerNumber = stoi(registerName.substr(1));

        machineCodeWord |= (registerNumber << 20);  // regA
        machineCodeWord |= (reg1 << 12);  // regC

        // write machine code word to the current section
        tempSymbolSecondPass = getSectionSymbol(currentSection);
        tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);

        LC += 4;
    }
    else if (tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT ||
    tokenList[i].value == REG_POM_HEXL_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_HEXL_ADR_DOUBLE_DIGIT){

        // operand is [%ri + D]
        machineCodeWord = 0x80000000;

        // first we need to extract i and D
        string operand = tokenList[i].name;
        size_t percentPos = operand.find('%');
        size_t plusPos = operand.find('+');
        size_t closingBracketPos = operand.find(']');

        if (percentPos != std::string::npos && plusPos != std::string::npos && closingBracketPos != std::string::npos) {
            // Extract the substring between '%' and '+', and between '+' and ']'
            std::string iStr = operand.substr(percentPos + 1, plusPos - percentPos - 1);  // "ri"  -> we'll remove the r before int conversion
            std::string DStr = operand.substr(plusPos + 1, closingBracketPos - plusPos - 1); // " D" -> we'll remove the space before int conversion

            // Convert the extracted substrings to integers
            registerNumber = std::stoi(iStr.substr(1));  // regA
            int D;
            if (tokenList[i].value == REG_POM_L_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_L_ADR_DOUBLE_DIGIT){
                D = std::stoi(DStr.substr(1));  // D (this literal will definately fit in 12 bits)
            }
            else {
                D = hexToInt(DStr.substr(1));  // D (this literal will definately fit in 12 bits)
            }

            machineCodeWord |= (registerNumber << 20);  // regA
            machineCodeWord |= (reg1 << 12);  // regC
            machineCodeWord |= D;  // D

            // write machine code word to the current section
            tempSymbolSecondPass = getSectionSymbol(currentSection);
            tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);

            LC += 4;
        }
        else {
            cout << "ERROR: Invalid operand" << endl;
            exit(EXIT_FAILURE);
        }
    }
    else if (tokenList[i].value == REG_POM_S_ADR_SINGLE_DIGIT || tokenList[i].value == REG_POM_S_ADR_DOUBLE_DIGIT){
        cout << "ERROR: Invalid operand, the value of the symbol is not known during asembling, so st " << tokenList[i].name << " cannot be done." << endl;
        exit(EXIT_FAILURE);
        // TODO: Check, maybe we can do this, it doesn't make sense not to have it for either ld or st
    }
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
                            // Write zeros to the machine code and create a relocation entry
                            value = 0;
                            relSym = new RelocationSymbol(tempSymbolSecondPass->number, LC);
                            tempSymbolSecondPass = getSectionSymbol(currentSection);
                            tempSymbolSecondPass->relocationTable.push_back(relSym);
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
                if (currentSection != ""){

                    // we need to add jmp pool_of_literals to machine code
                    tempSymbolSecondPass = getSectionSymbol(currentSection);

                    tempSymbolSecondPass->machineCode.push_back(0x30F00000 | (tempSymbolSecondPass->literals.size() * 4 + 4));  // jmp pool_of_literals

                    // increment LC, because we added jmp instruction
                    LC += 4;

                    // set the section size
                    tempSymbolSecondPass->sectionSize = LC + tempSymbolSecondPass->literals.size() * 4;

                    // TODO: Do we need to update locations table? There are some new literals added in the second pass
                    // For now, let's do it
                    LC = tempSymbolSecondPass->updateLiteralTable(LC);
                    
                    // Update the relocation table as well:
                    tempSymbolSecondPass->updateRelocationTable();

                    // Remember where this section should continue from if it appears again                    
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
                // iret == pop %status; pop %pc
                // pop %status => status <= mem32[sp]; sp <= sp + 4;  (r14 == sp, status == 0) => OC = 0x970e0004
                // ----------------------------------------------------------------------------------------------
                // pop %pc => pc <= mem32[sp]; sp <= sp + 4;  (r14 == sp, r15 == pc) => OC = 0x93fe0004

                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, 0x970e0004);  // pop %status
                LC += 4;

                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, 0x93fe0004);  // pop %pc
                LC += 4;

                break;
            
            case RET:
                // ret => returning from a function == pop %pc  (we have pop %pc machine code from IRET)
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, 0x93fe0004);  // pop %pc
                LC += 4;
                break;

            case CALL:
                callAndJmpCase(LC, i);
                break;

            case JMP:
                callAndJmpCase(LC, i);
                break;

            case PUSH:
                // push %gpr -> sp <= sp - 4; mem32[sp] <= gpr;  (r14 == sp; 14 == 0xe)
                machineCodeWord = 0x81e00000;
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                registerNumber = stoi(registerName.substr(1));

                // shift register number to the 4th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 12);

                // add the 4 to the last 3 bytes
                machineCodeWord |= 4;

                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
                LC += 4;

                break;

            case POPP:
                // pop %gpr -> gpr <= mem32[sp]; sp <= sp + 4;  (r14 == sp)
                machineCodeWord = 0x93000000;
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                registerNumber = stoi(registerName.substr(1));

                // shift register number to the 6th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 20);

                registerNumber = 14;  // r14 == sp
                // shift register number to the 5th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 16);

                // add the 4 to the last 3 bytes
                machineCodeWord |= 4;

                // write machine code word to the current section
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
                LC += 4;
                break;

            case XCHG:
                // OC for XCHG is 0x40
                machineCodeWord = 0x40000000;
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                // remove "r" (first letter) from register name
                registerNumber = stoi(registerName.substr(1));
                machineCodeWord |= (registerNumber << 12);  // gprS
                
                i++; // get ,
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                registerNumber = stoi(registerName.substr(1));
                machineCodeWord |= (registerNumber << 16);  // gprD
                
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
                machineCodeWord |= (registerNumber << 20);
                
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

            case BEQ:
                jumpInstructionsCase(LC, i);
                break;
            
            case BNE:
                jumpInstructionsCase(LC, i);
                break;
            
            case BGT:
                jumpInstructionsCase(LC, i);
                break;

            case LD:
                loadInstruction(LC, i);
                break;

            case ST:
                storeInstruction(LC, i);
                break;

            case CSRRD:
                machineCodeWord = 0x90000000;
                i++; // get %
                i++; // get special register
                if (tokenList[i].value == STATUS_REG){
                    registerNumber = 0;
                }
                else if (tokenList[i].value == HANDLER_REG){
                    registerNumber = 1;
                }
                else if (tokenList[i].value == CAUSE_REG){
                    registerNumber = 2;
                }
                // shift register number to the 5th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 16);

                i++; // get ,
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                registerNumber = stoi(registerName.substr(1));
                // shift register number to the 6th (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 20);

                // write machine code word to the current section
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
                LC += 4;
                break;

            case CSRWR:
                machineCodeWord = 0x94000000;
                i++; // get %
                i++; // get register
                registerName = tokenList[i].name;
                registerNumber = stoi(registerName.substr(1));
                // shift register number to the 5th (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 16);

                i++; // get ,
                i++; // get %
                i++; // get special register
                if (tokenList[i].value == STATUS_REG){
                    registerNumber = 0;
                }
                else if (tokenList[i].value == HANDLER_REG){
                    registerNumber = 1;
                }
                else if (tokenList[i].value == CAUSE_REG){
                    registerNumber = 2;
                }
                // shift register number to the 6th byte (from the right) of the machine code word
                machineCodeWord |= (registerNumber << 20);

                // write machine code word to the current section
                tempSymbolSecondPass = getSectionSymbol(currentSection);
                tempSymbolSecondPass->set4BytesOfMachineCode(LC, machineCodeWord);
                LC += 4;
                break;
                
            
            case ENDD:
                // we need to add jmp pool_of_literals to machine code
                tempSymbolSecondPass = getSectionSymbol(currentSection);

                tempSymbolSecondPass->machineCode.push_back(0x30F00000 | (tempSymbolSecondPass->literals.size() * 4 + 4));  // jmp pool_of_literals

                // increment LC, because we added jmp instruction
                LC += 4;

                // We need to reset the section size, because there may be some new literals added in the second pass
                tempSymbolSecondPass->sectionSize = LC + tempSymbolSecondPass->literals.size() * 4;

                // Update the literal table
                LC = tempSymbolSecondPass->updateLiteralTable(LC);

                // Update the relocation table as well:
                tempSymbolSecondPass->updateRelocationTable();

                // set the LC of the current section to the sectionLCMap just in case
                sectionLCMap[currentSection] = LC;


                // for each section, go through machine code and write out (on std::cout) in hex format each byte of the section size
                std::cout << "-------------------------------------" << endl;
                for (int j = 0; j < symbolTable.size(); j++){
                    std::cout << "-------------------------------------" << endl;
                    if (symbolTable[j].is_section){
                        std::cout << "SEKCIJA: " << symbolTable[j].name << endl;
                        for (int k = 0; k < symbolTable[j].machineCode.size(); k++){
                            std::cout << hex << symbolTable[j].machineCode[k] << endl;
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