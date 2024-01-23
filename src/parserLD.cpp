#include <iostream>
#include <sstream>
#include "../inc/parserLD.h"

int hexToIntNo0x(std::string s)
{
    return std::stoi(s, 0, 16);
}


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
        if (token.empty()){
            continue;
        }
        tokens.push_back(token);
    }
    return tokens;
}


SymbolLD* ParserLD::getSectionSymbolByName(string name){
    for (int i = 0; i < this->symbolTable.size(); i++){
        if (this->symbolTable[i]->isSection && this->symbolTable[i]->name == name){
            return this->symbolTable[i];
        }
    }
    return nullptr;
}


void ParserLD::parseSymbolTable(vector<string> tokens){

    // We should get 9 tokens: id, name, value, is_defined, bind, section/symbol, section_number, LC, sectionSize
    if (tokens.size() != 9){
        cout << "Wrong number of tokens in symbol table" << endl;
        exit(EXIT_FAILURE);
    }

    int id = stoi(tokens[0]);
    string name = tokens[1];
    int value = stoi(tokens[2]);
    bool isDefined = (tokens[3] == "DEF") ? true : false;
    bool isGlobal = (tokens[4] == "G") ? true : false;
    bool isSection = (tokens[5] == "S") ? true : false;
    int sectionNumber = stoi(tokens[6]);
    int LC = stoi(tokens[7]);
    if (isSection){
        int sectionSize = stoi(tokens[8]);
    }

    if (isSection){
        SymbolLD* section = new SymbolLD(id, name, value, isDefined, isGlobal, isSection, sectionNumber, LC);
        this->symbolTable.push_back(section);
    }
    else{
        SymbolLD* symbol = new SymbolLD(id, name, isSection);
        this->symbolTable.push_back(symbol);
    }
}


void ParserLD::parseLiteralTable(vector<string> tokens){

    // We should get 5 tokens: value (in hex), location, size, index, symbol_index, sectionName
    if (tokens.size() != 6){
        cout << "Wrong number of tokens in literal table" << endl;
        exit(EXIT_FAILURE);
    }

    int value = stoi(tokens[0], nullptr, 16);

    int location = hexToIntNo0x(tokens[1]);

    int size = stoi(tokens[2]);

    int index = stoi(tokens[3]);


    int symbolIndex;
    if (tokens[4] == "-"){
        symbolIndex = -1;
    }
    else{
        symbolIndex = stoi(tokens[4]);
    }
    string sectionName = tokens[5];

    LiteralLD* literal = new LiteralLD(value, location, size, index, symbolIndex, sectionName);

    SymbolLD* section = this->getSectionSymbolByName(sectionName);
    section->literalTable.push_back(literal);
}


void ParserLD::parseRelocationTable(vector<string> tokens){
    
    // We should get 4 tokens: symbolIndex, offset, literalIndex, sectionName
    if (tokens.size() != 4){
        cout << "Wrong number of tokens in relocation table" << endl;
        exit(EXIT_FAILURE);
    }

    int symbolIndex;
    if (tokens[0] == "-"){
        symbolIndex = -1;
    }
    else{
        symbolIndex = stoi(tokens[0]);
    }
    int offset = hexToIntNo0x(tokens[1]);
    int literalIndex;
    if (tokens[2] == "-"){
        literalIndex = -1;
    }
    else{
        literalIndex = stoi(tokens[2]);
    }
    string sectionName = tokens[3];

    RelocSymbolLD* relocation = new RelocSymbolLD(symbolIndex, offset, literalIndex, sectionName);

    SymbolLD* section = this->getSectionSymbolByName(sectionName);
    section->relocationTable.push_back(relocation);
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
        
        if (line.empty()){
            continue;
        }

        std::vector<std::string> tokens = tokenize(line);

        if (tokens.size() == 1){
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

        switch(currentTable){
            case SYMBOL:
                this->parseSymbolTable(tokens);
                break;
            case LITERAL:
                this->parseLiteralTable(tokens);
                break;
            case RELOCATION:
                this->parseRelocationTable(tokens);
                break;
            default:
                break;
        }
    }

    this->inputFile.close();

    this->printSymbolTable();
    this->printLiteralTable();
    this->printRelocationTable();

}
