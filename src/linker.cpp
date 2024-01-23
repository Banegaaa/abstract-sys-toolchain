#include <string>
#include <vector>
#include <iostream>
#include "../inc/linker.h"


Linker::Linker(bool hexFlag, unordered_map<string, int> placeSections, vector<string> inputFileNames, string outputFileName){
    this->hexFlag = hexFlag;
    this->placeSections = placeSections;

    this->outputFile = ofstream(outputFileName);

    // We should create parsers for each input file
    
    for (int i = 0; i < inputFileNames.size(); i++){
        ParserLD* parser = new ParserLD(inputFileNames[i]);
        this->parsers.push_back(parser);
    }

    for (int i = 0; i < this->parsers.size(); i++){
        this->parsers[i]->initialize();
    }
}

Linker::~Linker(){
    for (int i = 0; i < this->parsers.size(); i++){
        delete this->parsers[i];
    }
}


void Linker::sectionPlacing(){

    // Go through placeSections map and for each section
    // for every section, loop through all parsers so we can check if that section exists in that parser

    for (unordered_map<string,int>::iterator it = this->placeSections.begin(); it != this->placeSections.end(); it++){
        
        string sectionName = it->first;  // first = key
        int address = it->second;        // second = value

        for (int i = 0; i < this->parsers.size(); i++){
            SymbolLD* sectionSymbol = this->parsers[i]->getSectionSymbolByName(sectionName);

            // if the section exists in the parser, we check if the sections vector is empty
            // 1. if it is empty, just add the new section to the vector
            // 2. if it is not emtpy, check if the last added section has the same name as the current section, if it does, add it to the vector with it's addres being the last added section's address + last added section's size

            if (sectionSymbol != nullptr){
                if (this->sections.empty()){
                    this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, address)));
                }
                else {
                    SymbolLD* lastSectionSymbol = this->sections.back().second.first; // second = <section_symbol, address> -> first = section_symbol
                    int lastAddress = this->sections.back().second.second; // second = <section_symbol, address> -> second = address

                    if (lastSectionSymbol->name.compare(sectionSymbol->name) == 0){
                        this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, lastAddress + lastSectionSymbol->sectionSize)));
                    }
                    else {
                        this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, address)));
                    }
                }
            }
            else {
                // That section doesn't exist in this parser -> we don't need to do anything
            }
        }
    }

    // We have finished placing all the sections from the placeSections map
    // Now we go through all the parsers and add it's sections to the sections vector

    for (int i = 0; i < this->parsers.size(); i++){
        for (int j = 0; j < this->parsers[i]->symbolTable.size(); j++){
            if (this->parsers[i]->symbolTable[j]->isSection){
                SymbolLD* sectionSymbol = this->parsers[i]->symbolTable[j];
                
                // Check if the section from this parser (input file) is already in the sections vector

                bool alreadyInSections = false;

                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                    if (it->first.compare(this->parsers[i]->inputFileName) == 0 && it->second.first->name.compare(sectionSymbol->name) == 0){
                        alreadyInSections = true;
                        break;
                    }
                }

                if (!alreadyInSections){
                    // If the section from this input file is not already in the sections vector, we look for the same section from another vector

                    bool existsFromAnotherParser = false;

                    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it){
                        if (it->second.first->name.compare(sectionSymbol->name) == 0){
                            existsFromAnotherParser = true;

                            // IF IT EXISTS, WE WILL ADD IT EMEDIATELY
                            this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, it->second.second + it->second.first->sectionSize)));
                            break;
                        }
                    }

                    if (!existsFromAnotherParser){
                        // If it doesn't exist from another parser, we add it after the last added section
                        if (this->sections.empty()){
                            this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, 0)));
                        }
                        else {
                            SymbolLD* lastSectionSymbol = this->sections.back().second.first; // second = <section_symbol, address> -> first = section_symbol
                            int lastAddress = this->sections.back().second.second; // second = <section_symbol, address> -> second = address

                            cout << "Velicina poslednje dodate sekcije je: " << lastSectionSymbol->sectionSize << endl;

                            this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, lastAddress + lastSectionSymbol->sectionSize)));
                        }
                    }
                    // THERE IS NO ELSE, BECAUSE IF IT EXISTS FROM ANOTHER PARSER, WE ALREADY ADDED IT
                }
                else {
                    // The section from this file is alredy in the sections vector -> we don't need to do anything

                    continue;
                }

            }
        }
    }

    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
        std::string fileName = it->first;
        SymbolLD* symbolPointer = it->second.first;
        int address = it->second.second;

        // Print in the specified format
        std::cout << "File: " << fileName << ", Section: " << symbolPointer->name << " ---> Address: " << hex << address << std::endl;
    }

}


void Linker::fixAddresses(){
    // Go through all tables (symbol, reloc, and literal) and fix their addresses (now that we have placed all the sections)
    
    for (int i = 0; i < this->parsers.size(); i++){

        // Symbol table (only non-section symbols)
        for(int j = 0; j < this->parsers[i]->symbolTable.size(); j++){
            
            if (!this->parsers[i]->symbolTable[j]->isSection){
                // We need to fix the address of this symbol

                // We need to find the section in which this symbol is defined
                // We will do this by looking for the section symbol in the sections vector

                string sectionName = this->parsers[i]->getSectionSymbolById(this->parsers[i]->symbolTable[j]->id)->name;
                

                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                    if (it->second.first->name.compare(sectionName) == 0){
                        // We have found the section in which this symbol is defined
                        // We need to add the address of this section to the value of this symbol
                        this->parsers[i]->symbolTable[j]->value += it->second.second;
                        break;
                    }
                }
            }
            else {
                // It is a section, here we will fix the relocation table
                for (int k = 0; k < this->parsers[i]->symbolTable[j]->relocationTable.size(); k++){
                    // We need to find the symbol in the symbol table
                    // We will do this by looking for the symbol in the symbol table

                    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                        if (it->second.first->name.compare(this->parsers[i]->symbolTable[j]->name) == 0){
                            // We have found the section
                            this->parsers[i]->symbolTable[j]->relocationTable[k]->offset += it->second.second;
                            break;
                        }
                    }
                }
            }
        }
    }
}