#include <iostream>
#include <sstream>
#include "../inc/parserLD.h"


SymbolLD* tmpSectionSymbol;

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

    int id = stoi(tokens[0], nullptr, 16);
    string name = tokens[1];
    int value = stoi(tokens[2], nullptr, 16);
    bool isDefined = (tokens[3].compare("DEF") == 0) ? true : false;
    bool isGlobal = (tokens[4].compare("G") == 0) ? true : false;
    bool isSection = (tokens[5].compare("S") == 0) ? true : false;
    
    int sectionNumber;
    if (tokens[6].compare("-") == 0){
        sectionNumber = -1;
    }
    else {
        sectionNumber = stoi(tokens[6], nullptr, 16);
    }

    int LC = stoi(tokens[7], nullptr, 16);
    
    if (isSection){
        int sectionSize = stoi(tokens[8], nullptr, 16);
        // we will add the section symbol here
        SymbolLD* symbol = new SymbolLD(id, name, isSection, sectionSize);
        this->symbolTable.push_back(symbol);
    }
    else {
        SymbolLD* section = new SymbolLD(id, name, value, isDefined, isGlobal, isSection, sectionNumber, LC);
        this->symbolTable.push_back(section);
    }
}


void ParserLD::parseLiteralTable(vector<string> tokens){

    // We should get 5 tokens: value (in hex), location, size, index, symbol_index, sectionName
    if (tokens.size() != 6){
        cout << "Wrong number of tokens in literal table" << endl;
        exit(EXIT_FAILURE);
    }

    string value = tokens[0];  // Value is in hex

    int location = hexToIntNo0x(tokens[1]);

    int size = stoi(tokens[2], nullptr, 16);

    int index = stoi(tokens[3], nullptr, 16);

    int symbolIndex;
    // std::cout << "PRAVI NAM PROBLEM: " << tokens[4] << std::endl;
    if (tokens[4] == "-"){
        symbolIndex = -1;
    }
    else{
        symbolIndex = stoi(tokens[4], nullptr, 16);
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
        symbolIndex = stoi(tokens[0], nullptr, 16);
    }
    int offset = hexToIntNo0x(tokens[1]);
    int literalIndex;
    if (tokens[2] == "-"){
        literalIndex = -1;
    }
    else{
        literalIndex = stoi(tokens[2], nullptr, 16);
    }
    string sectionName = tokens[3];

    string symbolName = this->getSymbolNameById(symbolIndex);

    RelocSymbolLD* relocation = new RelocSymbolLD(offset, symbolIndex, literalIndex, symbolName, sectionName);

    SymbolLD* section = this->getSectionSymbolByName(sectionName);
    section->relocationTable.push_back(relocation);
}


void ParserLD::parseMachineCode(vector<string> tokens, SymbolLD* sectionSymbol){

    // every token here is an element of machine code, and we should add it to the sectionSymbol
    for (int i = 0; i < tokens.size(); i++){
        sectionSymbol->machineCode.push_back(tokens[i]);
    }

    // Each element of the machine code is represented as a string (hex number)
    // they differ in size, but the largest one is 4 bytes (ex: 0x00000000)
    // so we will set the sectionSizeWithLiterals to number of bytes in machine code

    // append all machine code elements into 1 string

    string machineCodeString = "";
    for (int i = 0; i < sectionSymbol->machineCode.size(); i++){
        machineCodeString += sectionSymbol->machineCode[i];
    }

    // now we will get the number of bytes in machine code

    sectionSymbol->sectionSize = machineCodeString.size() / 2;


}


void ParserLD::initialize(){

    if (!inputFile.is_open()){
        cout << "Input file not opened" << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    enum TableType { NONE, SYMBOL, LITERAL, RELOCATION, MACHINE_CODE };
    TableType currentTable = NONE;

    while (std::getline(this->inputFile, line)){
        
        if (line.empty()){
            continue;
        }

        std::vector<std::string> tokens = tokenize(line);

        if (tokens.size() == 1){
            if (tokens[0] == "SymbolTable:"){
                // cout << "SymbolTable: PARSING" << endl;
                currentTable = SYMBOL;
                continue;                
            }
            else if (tokens[0] == "LiteralTable:"){
                // cout << "LiteralTable: PARSING" << endl;
                currentTable = LITERAL;
                continue;
            }
            else if (tokens[0] == "RelocationTable:"){
                // cout << "RelocationTable: PARSING" << endl;
                currentTable = RELOCATION;
                continue;
            }
            else if (tokens[0] == "MachineCode:"){
                // cout << "MachineCode: PARSING" << endl;
                currentTable = MACHINE_CODE;
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
            case MACHINE_CODE:
                if (tokens.size() == 2 && tokens[0].compare("Section") == 0){  // defining section
                    tmpSectionSymbol = this->getSectionSymbolByName(tokens[1]);
                    continue;  // we don't want to parse this line
                }
                this->parseMachineCode(tokens, tmpSectionSymbol);
                break;
            default:
                break;
        }
    }

    this->inputFile.close();

    // this->printSymbolTable();
    // this->printLiteralTable();
    // this->printRelocationTable();

    // print out machine code for each section

    for (int i = 0; i < this->symbolTable.size(); i++){
        if (this->symbolTable[i]->isSection){
            cout << "Section " << this->symbolTable[i]->name << endl;
            for (int j = 0; j < this->symbolTable[i]->machineCode.size(); j++){
                cout << std::setw(2) << std::setfill('0') << this->symbolTable[i]->machineCode[j] << " ";
            }
            cout << endl;
        }
    }
}
