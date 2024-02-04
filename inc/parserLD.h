#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "symbolLD.h"


using namespace std;

// We will use this class to store all info from assembler file.

class ParserLD {
    public:
    ifstream inputFile;
    string inputFileName;

    vector<SymbolLD*> symbolTable;  // For symbols and sections
    vector<string> tokenize(string line);
    void initialize();

    ParserLD(string inputFileName);

    // Function to replace consecutive whitespaces with a single space (this works)
    string collapseWhitespaces(const string& input);

    // Parsing of symbol, literal and relocation tables
    void parseSymbolTable(vector<string> tokens);
    void parseLiteralTable(vector<string> tokens);
    void parseRelocationTable(vector<string> tokens);
    void parseMachineCode(vector<string> tokens, SymbolLD* sectionSymbol);

    SymbolLD* getSectionSymbolByName(string name);

    void printSymbolTable(){
        std::cout << "Symbol table:" << endl;
        for (int i = 0; i < this->symbolTable.size(); i++){
            std::cout << this->symbolTable[i]->name << " " << this->symbolTable[i]->section << " " << this->symbolTable[i]->value << endl;
        }
    }
    void printLiteralTable(){
        std::cout << "Literal table:" << endl;
        for (int i = 0; i < this->symbolTable.size(); i++){
            if (this->symbolTable[i]->isSection){
                for (int j = 0; j < this->symbolTable[i]->literalTable.size(); j++){
                    std::cout << this->symbolTable[i]->literalTable[j]->value << " " << this->symbolTable[i]->literalTable[j]->location << this->symbolTable[i]->literalTable[j]->index << endl;
                }
            }
        }
    }
    void printRelocationTable(){
        std::cout << "Relocation table:" << endl;
        for (int i = 0; i < this->symbolTable.size(); i++){
            if (this->symbolTable[i]->isSection){
                for (int j = 0; j < this->symbolTable[i]->relocationTable.size(); j++){
                    std::cout << this->symbolTable[i]->relocationTable[j]->symbolIndex << " " << this->symbolTable[i]->relocationTable[j]->offset << " " << this->symbolTable[i]->relocationTable[j]->literalIndex << endl;
                }
            }
        }
    }

    SymbolLD* getSectionSymbolById(int id){
        for (int i = 0; i < this->symbolTable.size(); i++){
            if (this->symbolTable[i]->id == id){
                if (!this->symbolTable[i]->isSection){
                    std::cout << "Symbol with id " << id << " is not a section" << endl;
                    exit(EXIT_FAILURE);
                }
                return this->symbolTable[i];
            }
        }
        return NULL;
    }

    string getSymbolNameById(int id){
        for (int i = 0; i < this->symbolTable.size(); i++){
            if (this->symbolTable[i]->id == id){
                return this->symbolTable[i]->name;
            }
        }
        return "";
    }
};