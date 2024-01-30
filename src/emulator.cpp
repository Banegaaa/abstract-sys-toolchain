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


void Emulator::execute(){

    bool halt = false;

    while (!halt) {

        string instructionCode = this->getByteFromMemory(this->gpr[15]);

        if (instructionCode.compare(HALT) == 0) {
            // HALT

            std::cout << "HALT" << endl;
            halt = true;
            this->gpr[15] += 4;
            continue;
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

            string byte = std::to_string(this->gpr[15] & 0xFF);
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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

            this->gpr[D] -= this->gpr[S];

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

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

            string byte = this->getByteFromMemory((this->gpr[15] + 1) & 0x0F);
            int D = stoi(byte);

            byte = this->getByteFromMemory((this->gpr[15] + 2) & 0xF0);
            int S = stoi(byte);

            this->gpr[D] >>= this->gpr[S];

            this->gpr[15] += 4;
        }
    }
}