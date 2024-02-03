#include <sstream>
#include "../inc/emulator.h"


string collapseWhitespaces(const string& input) {
    string result;
    bool previousIsSpace = false;
    char ch;

    for (int i = 0; i < input.size(); i++) {
        ch = input[i];
        if (std::isspace(ch)) {
            if (!previousIsSpace) {
                result += ' ';
                previousIsSpace = true;
            }
        } else {
            result += ch;
            previousIsSpace = false;
        }
    }

    return result;
}


vector<string> tokenize(string line){
    std::vector<std::string> tokens;
    std::istringstream iss(collapseWhitespaces(line));
    std::string token;

    while (std::getline(iss, token, ' ')) {
        if (token.empty()){
            continue;
        }
        tokens.push_back(token);
    }
    return tokens;
}


Emulator::Emulator(string inputFileName) {
    this->inputFileName = inputFileName;

    this->inputFile = fstream(inputFileName);

    if (!this->inputFile.is_open()) {
        std::cout << "Error opening file " << inputFileName << endl;
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 16; i++) {
        this->gpr[i] = 0;
    }

    for (int i = 0; i < 3; i++) {
        this->csr[i] = 0;
    }

    this->gpr[15] = 0x40000000;  // PC
}


void Emulator::parseInputFileAndLoadIntoMemory(){

    string line;
    vector<string> tokens;

    int address;

    while (getline(this->inputFile, line)) {
        tokens = tokenize(line);

        if (tokens.size() == 0) {
            continue;
        }

        // First token is the address

        address = stoi(tokens[0].substr(0, tokens[0].size() - 1), nullptr, 16);

        // All that are left are machine code bytes
        // We take them and place them in memory, with their corresponding addresses

        for (int i = 1; i < tokens.size(); i++) {
            this->memory.push_back(make_pair(address, tokens[i]));
            address++;
        }
    }

    this->inputFile.close();

    // for (int i = 0; i < this->memory.size(); i++) {
    //     cout << hex << this->memory[i].first << " " << this->memory[i].second << endl;
    // }
}


string Emulator::getByteFromMemory(unsigned address) {
    for (int i = 0; i < this->memory.size(); i++) {
        if (this->memory[i].first == address) {
            return this->memory[i].second;
        }
    }
    return "";
}

void Emulator::writeByteToMemory(unsigned address, string byte) {
    for (int i = 0; i < this->memory.size(); i++) {
        if (this->memory[i].first == address) {
            this->memory[i].second = byte;
            return;
        }
    }
    this->memory.push_back(make_pair(address, byte));  // new address
}


int Emulator::getUpperHalfWordFromMemory(unsigned address) {
    string byte = this->getByteFromMemory(address);
    return stoi(byte.substr(0, 1), nullptr, 16);
}

int Emulator::getLowerHalfWordFromMemory(unsigned address) {
    string byte = this->getByteFromMemory(address);
    return stoi(byte.substr(1), nullptr, 16) & 0x0F;
}


void Emulator::execute(){

    std::cout << "Executing program... " << endl;

    bool halt = false;
    string byte;
    string machineCodeWord;

    while (!halt) {

        string instructionCode = this->getByteFromMemory(this->gpr[15]);

        if (instructionCode.compare(HALT) == 0) {
            // HALT

            std::cout << "HALT" << endl;
            halt = true;
            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(INT) == 0){
            // INT

            /*
                push pc;                     --->       gpr[15] += 4 (because we continue executing from the next instruction); gpr[14] = gpr[15]; gpr[14] -= 4;
                push status;                 --->       gpr[14] = csr[0]; gpr[14] -= 4;
                cause <= 4;                  --->       csr[2] = 4;         
                status <= status & (~0x1);   --->       csr[0] &= ~0x1;
                pc <= handle;                --->       gpr[15] = csr[1];
            */

            std::cout << "INT" << endl;

            // push pc

            this->gpr[15] += 4;

            byte = std::to_string(this->gpr[15] & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 1, byte);  // gpr[14] - 1 because sp is pointing to the last taken address

            byte = std::to_string((this->gpr[15] >> 8) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 2, byte);

            byte = std::to_string((this->gpr[15] >> 16) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 3, byte);

            byte = std::to_string((this->gpr[15] >> 24) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 4, byte);

            this->gpr[14] = this->gpr[15];


            // push status

            byte = std::to_string(this->csr[0] & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 1, byte);

            byte = std::to_string((this->csr[0] >> 8) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 2, byte);

            byte = std::to_string((this->csr[0] >> 16) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 3, byte);

            byte = std::to_string((this->csr[0] >> 24) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 4, byte);

            this->gpr[14] -= 4;

            // cause <= 4;
            this->csr[2] = 4;

            // status <= status & (~0x1);
            this->csr[0] &= ~0x1;

            // pc <= handle;
            this->gpr[15] = this->csr[1];
        }
        else if (instructionCode.compare(XCHG) == 0) {

            // xchg %gprS, %gprD
            /*
                tmp <= gpr[S];
                gpr[S] <= gpr[D];
                gpr[D] <= tmp;
            */

            std::cout << "XCHG" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the second half-byte in the following byte
            // gprS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            unsigned tmp = this->gpr[D];
            this->gpr[D] = this->gpr[S];
            this->gpr[S] = tmp;

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(ADD) == 0){

            // add %gprS, %gprD
            /*
                gpr[D] <= gpr[D] + gpr[S];
            */

            std::cout << "ADD" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gprS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] += this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(SUB) == 0){

            // sub %gprS, %gprD
            /*
                gpr[D] <= gpr[D] - gpr[S];
            */

            std::cout << "SUB" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gprS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            cout << "DESTINACIONI REGISTAR JE: " << D << endl;
            cout << "VREDNOST DESTINACIONOG REGISTRA JE: " << this->gpr[D] << endl;
            cout << "IZVORNI REGISTAR JE: " << S << endl;
            cout << "VREDNOST IZVORNOG REGISTRA JE: " << this->gpr[S] << endl;
            cout << "REZULTAT JE: " << this->gpr[D] - this->gpr[S] << endl;

            this->gpr[D] = this->gpr[D] - this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(MUL) == 0){

            // mul %gprS, %gprD
            /*
                gpr[D] <= gpr[D] * gpr[S];
            */

            std::cout << "MUL" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gprS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] *= this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(DIV) == 0){

            // div %gprS, %gprD
            /*
                gpr[D] <= gpr[D] / gpr[S];
            */

            std::cout << "DIV" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gprS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] /= this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(NOT) == 0){

            // not %gprD
            /*
                gpr[D] <= ~gpr[D];
            */

            std::cout << "NOT" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gprD == gprS

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);

            this->gpr[D] = ~this->gpr[D];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(AND) == 0){

            // and %gprS, %gprD
            /*
                gpr[D] <= gpr[D] & gpr[S];
            */

            std::cout << "AND" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gprS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] &= this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(OR) == 0){

            // or %gprS, %gprD
            /*
                gpr[D] <= gpr[D] | gpr[S];
            */

            std::cout << "OR" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gprS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] |= this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(XOR) == 0){

            // xor %gprS, %gprD
            /*
                gpr[D] <= gpr[D] ^ gpr[S];
            */

            std::cout << "XOR" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gpS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] ^= this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(SHL) == 0){

            // shl %gprS, %gprD
            /*
                gpr[D] <= gpr[D] << gpr[S];
            */

            std::cout << "SHL" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gpS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] <<= this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(SHR) == 0){

            // shr %gprS, %gprD
            /*
                gpr[D] <= gpr[D] >> gpr[S];
            */

            std::cout << "SHR" << endl;

            // We have the instruction code, now we need to get the operands
            // gprD is the first or second half-byte in the following byte
            // gpS is the first half-byte in the following byte after that

            int D = this->getLowerHalfWordFromMemory(this->gpr[15] + 1);
            int S = this->getUpperHalfWordFromMemory(this->gpr[15] + 2);

            this->gpr[D] >>= this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(CALL_L) == 0){
            // call literal

            /*
                push pc;
                pc <= gpr[A] + gpr[B] + D;    === pc <= D; A = 0; B = 0;
            */

            std::cout << "CALL_L" << endl;

            // extract D

            byte = this->getByteFromMemory(this->gpr[15] + 2);
            byte = byte.substr(1) + this->getByteFromMemory(this->gpr[15] + 3);

            int D = stoi(byte, nullptr, 16);

            // push pc

            this->gpr[15] += 4;

            byte = std::to_string(this->gpr[15] & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 1, byte);  // gpr[14] - 1 because sp is pointing to the last taken address

            byte = std::to_string((this->gpr[15] >> 8) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 2, byte);

            byte = std::to_string((this->gpr[15] >> 16) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 3, byte);

            byte = std::to_string((this->gpr[15] >> 24) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 4, byte);

            this->gpr[14] -= 4;

            // pc <= gpr[A] + gpr[B] + D;

            this->gpr[15] = this->gpr[15] + D;
        }
        else if (instructionCode.compare(CALL_S) == 0){

            // call symbol

            /*
                This is a case where we call symbol or literal that couldn't be placed in the D bits, so we need to extract it from memory.

                push pc;
                pc <= mem32[gpr[A] + gpr[B] + D];    === pc <= D; A = F; B = 0;
            */


            std::cout << "CALL_S" << endl;

            int A = 0xF;  // TODO: Or should we extract it?

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte 
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            // push pc

            this->gpr[15] += 4;

            byte = std::to_string(this->gpr[15] & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 1, byte);  // gpr[14] - 1 because sp is pointing to the last taken address

            byte = std::to_string((this->gpr[15] >> 8) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 2, byte);

            byte = std::to_string((this->gpr[15] >> 16) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 3, byte);

            byte = std::to_string((this->gpr[15] >> 24) & 0xFF);
            this->writeByteToMemory(this->gpr[14] - 4, byte);

            this->gpr[14] -= 4;

            // pc <= mem32[gpr[A] + gpr[B] + D]

            // TODO: CHECK IF THIS WILL GET THE CORRECT LITERAL FROM MEMORY

            int address = this->gpr[A] + Disp;

            // *** IMPORTANT ***
            // When we need to fetch 4 bytes from memory, we need to fetch them in reverse order, because of the little endianess of the system
            // When we were storing the bytes, we first stored the highest byte, because the stack pointer is decreasing when we push!!!

            machineCodeWord = this->getByteFromMemory(address + 3) + this->getByteFromMemory(address + 2) + this->getByteFromMemory(address + 1) + this->getByteFromMemory(address);

            this->gpr[15] = stoi(machineCodeWord, nullptr, 16);
        }
        else if (instructionCode.compare(JMP_L) == 0){

            // jmp literal

            /*
                pc <= gpr[A] + gpr[B] + D;    === pc <= D; A = F; B = 0;
                ==
                pc <= pc + D;
            */

            std::cout << "JMP_L" << endl;

            int A = 0xF;  // TODO: Or should we extract it?

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            // pc <= pc + D;

            this->gpr[15] += Disp;
        }
        else if (instructionCode.compare(JMP_S) == 0){

            // jmp symbol

            /*
                This is a case where we jmp symbol or literal that couldn't be placed in the D bits, so we need to extract it from memory.

                pc <= mem32[gpr[A] + gpr[B] + D];    === pc <= D; A = F; B = 0;
            */

            std::cout << "JMP_S" << endl;

            int A = 0xF;  // TODO: Or should we extract it?

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            // pc <= mem32[gpr[A] + gpr[B] + D]

            int address = this->gpr[A] + Disp;

            machineCodeWord = this->getByteFromMemory(address + 3) + this->getByteFromMemory(address + 2) + this->getByteFromMemory(address + 1) + this->getByteFromMemory(address);

            this->gpr[15] = stoi(machineCodeWord, nullptr, 16);
        }
        else if (instructionCode.compare(BEQ_L) == 0){
            
            // beq %gprB, %gprC, literal

            /*
                if (gpr[B] == gpr[C]) {
                    pc <= gpr[A] + gpr[B] + D;  => A = F = 15 = PC
                }
                else {
                    pc <= pc + 4;
                }
            */

            std::cout << "BEQ_L" << endl;

            int A = 0xF;  // pc

            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);  // TODO: Check

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            if (this->gpr[B] == this->gpr[C]){
                // pc <= gpr[A] + D;    === pc <= D; A = F;

                this->gpr[15] += Disp;
            }
            else {
                // pc <= pc + 4;
                this->gpr[15] += 4;
            }
        }
        else if (instructionCode.compare(BEQ_S) == 0){

            // beq %gprB, %gprC, symbol

            /*
                This is a case where we beq symbol or literal that couldn't be placed in the D bits, so we need to extract it from memory.

                if (gpr[B] == gpr[C]) {
                    pc <= mem32[gpr[A] + D];   =>  A = F = 15 = PC
                }
                else {
                    pc <= pc + 4;
                }
            */

            std::cout << "BEQ_S" << endl;

            int A = 0xF;  // pc

            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);  // TODO: Check

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            int address = this->gpr[A] + Disp;

            machineCodeWord = this->getByteFromMemory(address + 3) + this->getByteFromMemory(address + 2) + this->getByteFromMemory(address + 1) + this->getByteFromMemory(address);

            if (this->gpr[B] == this->gpr[C]){
                // pc <= mem32[gpr[A] + D];    === pc <= D; A = F;

                this->gpr[15] = stoi(machineCodeWord, nullptr, 16);
            }
            else {
                // pc <= pc + 4;
                this->gpr[15] += 4;
            }
        }
        else if (instructionCode.compare(BNE_L) == 0){
            
            // bne %gprB, %gprC, literal

            /*
                if (gpr[B] != gpr[C]) {
                    pc <= gpr[A] + D;    === A = PC
                }
                else {
                    pc <= pc + 4;
                }
            */

            std::cout << "BNE_L" << endl;

            int A = 0xF;  // pc

            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            if (this->gpr[B] != this->gpr[C]){
                // pc <= gpr[A] + D;    === pc <= D; A = F;

                this->gpr[15] += Disp;
            }
            else {
                // pc <= pc + 4;
                this->gpr[15] += 4;
            }
        }
        else if (instructionCode.compare(BNE_S) == 0){

            // bne %gprB, %gprC, symbol

            /*
                This is a case where we bne symbol or literal that couldn't be placed in the D bits, so we need to extract it from memory.

                if (gpr[B] != gpr[C]) {
                    pc <= mem32[gpr[A] + D];
                }
                else {
                    pc <= pc + 4;
                }
            */

            std::cout << "BNE_S" << endl;

            int A = 0xF;  // pc

            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            int address = this->gpr[A] + Disp;

            machineCodeWord = this->getByteFromMemory(address + 3) + this->getByteFromMemory(address + 2) + this->getByteFromMemory(address + 1) + this->getByteFromMemory(address);

            if (this->gpr[B] != this->gpr[C]){
                // pc <= mem32[gpr[A] + D];    === pc <= D; A = F;

                this->gpr[15] = stoi(machineCodeWord, nullptr, 16);
            }
            else {
                // pc <= pc + 4;
                this->gpr[15] += 4;
            }
        }
        else if (instructionCode.compare(BGT_L) == 0){
            
            // bgt %gprB, %gprC, literal

            /*
                if (gpr[B] > gpr[C]) {
                    pc <= gpr[A] + D;
                }
                else {
                    pc <= pc + 4;
                }
            */

            std::cout << "BGT_L" << endl;

            int A = 0xF;  // pc

            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            if (this->gpr[B] > this->gpr[C]){
                // pc <= gpr[A] + D;    === pc <= D; A = F;

                this->gpr[15] += Disp;
            }
            else {
                // pc <= pc + 4;
                this->gpr[15] += 4;
            }
        }
        else if (instructionCode.compare(BGT_S) == 0){

            // bgt %gprB, %gprC, symbol

            /*
                This is a case where we bgt symbol or literal that couldn't be placed in the D bits, so we need to extract it from memory.

                if (gpr[B] > gpr[C]) {
                    pc <= mem32[gpr[A] + D];
                }
                else {
                    pc <= pc + 4;
                }
            */

            std::cout << "BGT_S" << endl;

            int A = 0xF;  // pc

            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            string strDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            strDisp = strDisp.substr(1);                                      // take only the lower half-byte  
            strDisp = strDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(strDisp, nullptr, 16);

            int address = this->gpr[A] + Disp;

            machineCodeWord = this->getByteFromMemory(address + 3) + this->getByteFromMemory(address + 2) + this->getByteFromMemory(address + 1) + this->getByteFromMemory(address);

            if (this->gpr[B] > this->gpr[C]){
                // pc <= mem32[gpr[A] + D];    === pc <= D; A = F;

                this->gpr[15] = stoi(machineCodeWord, nullptr, 16);
            }
            else {
                // pc <= pc + 4;
                this->gpr[15] += 4;
            }
        }
        else if (instructionCode.compare(CSRRD) == 0){

            // csrrd %csr, %gpr

            /*
                gpr[D] <= csr[S];
            */

            std::cout << "CSRRD" << endl;

            string DS = this->getByteFromMemory(this->gpr[15] + 1);

            int D = stoi(DS.substr(0, 1), nullptr, 16);
            int S = stoi(DS.substr(1), nullptr, 16);

            this->gpr[D] = this->csr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(CSRWR) == 0){

            // csrwr %gpr, %csr

            /*
                csr[D] <= gpr[S];
            */

            std::cout << "CSRWR" << endl;

            string DS = this->getByteFromMemory(this->gpr[15] + 1);

            int D = stoi(DS.substr(0, 1), nullptr, 16);
            int S = stoi(DS.substr(1), nullptr, 16);

            this->csr[D] = this->gpr[S];

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(LD_1) == 0){

            // instruction code 0x91
            /*
                This is a case for two types of operand with ld instruction:
                    1. ld $literal, %gprD  ==>   gprD <= $literal     (where the literal fits in the D bits)
                    2. ld %gprS, %gprD     ==>   gprD <= gprS
            */

            std::cout << "LD_1" << endl;

            string DS = this->getByteFromMemory(this->gpr[15] + 1);

            int D = stoi(DS.substr(0, 1), nullptr, 16);
            int S = stoi(DS.substr(1), nullptr, 16);

            string stdDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            stdDisp = stdDisp.substr(1);                                      // take only the lower half-byte
            stdDisp = stdDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(stdDisp, nullptr, 16);

            this->gpr[D] = this->gpr[S] + Disp;

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(LD_2) == 0){

            // instruction code 0x92
            /*
                gpr[A] <= mem32[gpr[B] + gpr[C] + D];

                machine code word:   1001 0010 AAAA BBBB CCCC DDDD DDDD DDDD
            */

            std::cout << "LD_2" << endl;

            int A = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(0, 1), nullptr, 16);
            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            string stdDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            stdDisp = stdDisp.substr(1);                                      // take only the lower half-byte
            stdDisp = stdDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(stdDisp, nullptr, 16);

            int address = this->gpr[B] + this->gpr[C] + Disp;

            machineCodeWord = this->getByteFromMemory(address + 3) + this->getByteFromMemory(address + 2) + this->getByteFromMemory(address + 1) + this->getByteFromMemory(address);

            this->gpr[A] = stoi(machineCodeWord, nullptr, 16);

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(POP) == 0){
            
            // pop %gprD
            /*
                gpr[A] <= mem32[gpr[B]];   gpr[B] <= gpr[B] + 4;
                ===
                gpr[A] <= mem32[sp];   sp <= sp + 4;
                ===
                gpr[A] <= mem32[gpr[14]];   gpr[14] <= gpr[14] + 4;
            */

            std::cout << "POP" << endl;

            int A = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(0, 1), nullptr, 16);
            int B = 0xE;  // sp
            int D = 4;

            // we get the value from the address that sp is pointing to
            machineCodeWord = this->getByteFromMemory(this->gpr[B] + 3) + this->getByteFromMemory(this->gpr[B] + 2) + this->getByteFromMemory(this->gpr[B] + 1) + this->getByteFromMemory(this->gpr[B]);

            this->gpr[A] = stoi(machineCodeWord, nullptr, 16);

            // sp <= sp + 4;

            this->gpr[B] += D;

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(POP_S) == 0){

            // pop %special_reg
            /*
                csr[A] <= mem32[gpr[B]];   gpr[B] <= gpr[B] + 4;
                ===
                csr[A] <= mem32[sp];   sp <= sp + 4;
                ===
                csr[A] <= mem32[gpr[14]];   gpr[14] <= gpr[14] + 4;
            */

            std::cout << "POP_S" << endl;

            int A = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(0, 1), nullptr, 16);
            int B = 0xE;  // sp
            int D = 4;

            // we get the value from the address that sp is pointing to
            machineCodeWord = this->getByteFromMemory(this->gpr[B] + 3) + this->getByteFromMemory(this->gpr[B] + 2) + this->getByteFromMemory(this->gpr[B] + 1) + this->getByteFromMemory(this->gpr[B]);

            this->csr[A] = stoi(machineCodeWord, nullptr, 16);

            // sp <= sp + 4;

            this->gpr[B] += D;

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(ST_1) == 0){

            // st %gpr, operand
            /*
                mem32[gpr[A] + gpr[B] + D] <= gpr[C];
            */

            std::cout << "ST_1" << endl;

            int A = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(0, 1), nullptr, 16);
            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            string stdDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            stdDisp = stdDisp.substr(1);                                      // take only the lower half-byte
            stdDisp = stdDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(stdDisp, nullptr, 16);

            int address = this->gpr[A] + this->gpr[B] + Disp;  // Address from which we set gpr[C]

            this->writeByteToMemory(address, std::to_string(this->gpr[C] & 0xFF));
            this->writeByteToMemory(address + 1, std::to_string((this->gpr[C] >> 8) & 0xFF));
            this->writeByteToMemory(address + 2, std::to_string((this->gpr[C] >> 16) & 0xFF));
            this->writeByteToMemory(address + 3, std::to_string((this->gpr[C] >> 24) & 0xFF));

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(ST_2) == 0){

            /*
                mem32[mem32[gpr[A] + gpr[B] + D]] <= gpr[C];
            */

            std::cout << "ST_2" << endl;

            int A = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(0, 1), nullptr, 16);
            int B = stoi(this->getByteFromMemory(this->gpr[15] + 1).substr(1), nullptr, 16);
            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            string stdDisp = this->getByteFromMemory(this->gpr[15] + 2);      // get the byte
            stdDisp = stdDisp.substr(1);                                      // take only the lower half-byte
            stdDisp = stdDisp + this->getByteFromMemory(this->gpr[15] + 3);   // here we have displacement in string form

            int Disp = stoi(stdDisp, nullptr, 16);

            int address = this->gpr[A] + this->gpr[B] + Disp;  // Address from which we get the address for setting gpr[C]

            string strAddress2 = this->getByteFromMemory(address + 3) + this->getByteFromMemory(address + 2) + this->getByteFromMemory(address + 1) + this->getByteFromMemory(address);
            int address2 = stoi(strAddress2, nullptr, 16);

            this->writeByteToMemory(address2, std::to_string(this->gpr[C] & 0xFF));
            this->writeByteToMemory(address2 + 1, std::to_string((this->gpr[C] >> 8) & 0xFF));
            this->writeByteToMemory(address2 + 2, std::to_string((this->gpr[C] >> 16) & 0xFF));
            this->writeByteToMemory(address2 + 3, std::to_string((this->gpr[C] >> 24) & 0xFF));

            this->gpr[15] += 4;
        }
        else if (instructionCode.compare(PUSH) == 0){

            // push %gprC
            /*
                sp <= sp - 4;   mem32[sp] <= gpr[C];
            */

            std::cout << "PUSH" << endl;

            int C = stoi(this->getByteFromMemory(this->gpr[15] + 2).substr(0, 1), nullptr, 16);

            // sp <= sp - 4;

            this->gpr[14] -= 4;

            // mem32[sp] <= gpr[C];

            this->writeByteToMemory(this->gpr[14], std::to_string(this->gpr[C] & 0xFF));
            this->writeByteToMemory(this->gpr[14] + 1, std::to_string((this->gpr[C] >> 8) & 0xFF));
            this->writeByteToMemory(this->gpr[14] + 2, std::to_string((this->gpr[C] >> 16) & 0xFF));
            this->writeByteToMemory(this->gpr[14] + 3, std::to_string((this->gpr[C] >> 24) & 0xFF));

            this->gpr[15] += 4;

        }
    }
}