#include <iostream>
#include <sstream>
#include "../inc/parserLD.h"

ParserLD::ParserLD(string inputFileName){
    this->inputFileName = inputFileName;
    this->inputFile.open(inputFileName);

    if (!this->inputFile.is_open()){
        cout << "Input file not opened" << endl;
        exit(EXIT_FAILURE);
    }
}

string ParserLD::collapseWhitespaces(const string& input) {
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

vector<string> ParserLD::tokenize(string line){
    std::vector<std::string> tokens;
    std::istringstream iss(collapseWhitespaces(line));
    std::string token;

    while (std::getline(iss, token, ' ')) {
        tokens.push_back(token);
    }
    return tokens;
}

void ParserLD::initialize(){
    std::ifstream inputFileStream(this->inputFileName);  // TODO: Check if we need this

    if (!inputFile.is_open()){
        cout << "Input file not opened" << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    enum TableType { NONE, SYMBOL, LITERAL, RELOCATION };
    TableType currentTable = NONE;

    while (std::getline(this->inputFile, line)){
        std::vector<std::string> tokens = tokenize(line);

        if (tokens.size() > 0){
            if (tokens[0] == "SymbolTable:"){
                cout << "SymbolTable: PARSING" << endl;
                currentTable = SYMBOL;
                continue;
            }
            else if (tokens[0] == "LiteralTable:"){
                cout << "LiteralTable: PARSING" << endl;
                currentTable = LITERAL;
                continue;
            }
            else if (tokens[0] == "RelocationTable:"){
                cout << "RelocationTable: PARSING" << endl;
                currentTable = RELOCATION;
                continue;
            }
        }
    }


}
