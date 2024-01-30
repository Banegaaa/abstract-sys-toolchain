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

            // inside gpr[15] is 

        }
    }

}