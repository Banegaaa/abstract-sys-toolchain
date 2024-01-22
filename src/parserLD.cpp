#include <iostream>
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

vector<string> ParserLD::tokenize(){
    // The idea here is to split the line into non-blank tokens and return them in a vector for parsing.
    string line;

    while (getline(this->inputFile, line)){
        
        // Replace multiple whitespaces with a single space which we will use for delimiter
        line = this->collapseWhitespaces(line);

        /*
            3 cases:
                - SymbolTable:
                - LiteralTable:
                - RelocationTable:

            Go through the file, find these 3 sections, and parse them.
        */
    }

}

void ParserLD::initialize(){
    // TODO: 
}
