#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "../inc/linker.h"


string intToHexToString(int value){
    std::stringstream stream;
    stream << std::hex << value;
    std::string result( stream.str() );
    return result;
}

bool compareByAddress(const std::pair<std::string, std::pair<SymbolLD*, int> >& a,
                      const std::pair<std::string, std::pair<SymbolLD*, int> >& b) {
    return a.second.second < b.second.second;
}


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


int Linker::findTheNextAvailablePossition(){
    int highestAddress = 0;
    int sectionWithHighestAddressIndex = 0;

    // Go through sections vector and find the highest address

    for (int i = 0; i < this->sections.size(); i++){
        if (this->sections[i].second.second > highestAddress){
            highestAddress = this->sections[i].second.second;
            sectionWithHighestAddressIndex = i;
        }
    }

    return highestAddress + this->sections[sectionWithHighestAddressIndex].second.first->sectionSize;
}


SymbolLD* Linker::getGlobalSymbolByIdAndSectionName(int id, string sectionName){
    // TODO: This is not enough, we could have a symbol with the same id and section_name in different files

    std::cout << "ID: " << id << endl;
    std::cout << "Section name: " << sectionName << endl;

    for (int i = 0; i < this->globalSymbolTable.size(); i++){
        if (this->globalSymbolTable[i]->id == id && this->globalSymbolTable[i]->sectionName.compare(sectionName) == 0){
            return this->globalSymbolTable[i];
        }
    }

    std::cout << "Symbol with id: " << id << " and section name: " << sectionName << " not found in global symbol table" << endl;
    return nullptr;
}


int Linker::getSymbolIdByNameInLinker(string name){
    for (int i = 0; i < this->globalSymbolTable.size(); i++){
        if (this->globalSymbolTable[i]->name.compare(name) == 0){
            return this->globalSymbolTable[i]->id;
        }
    }
    return -1;
}


SymbolLD* Linker::getGlobalSymbolByName(string name){
    for (int i = 0; i < this->globalSymbolTable.size(); i++){
        if (this->globalSymbolTable[i]->name.compare(name) == 0){
            return this->globalSymbolTable[i];
        }
    }
    return nullptr;
}


void Linker::sectionPlacing(){

    // Go through placeSections map
    // for every section, loop through all parsers so we can check if that section exists in that parser

    for (unordered_map<string,int>::iterator it = this->placeSections.begin(); it != this->placeSections.end(); it++){
        
        string sectionName = it->first;  // first = key = section_name
        int address = it->second;        // second = value = address

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

                            // IF IT EXISTS, WE WILL ADD IT EMEDIATELY AFTER THE LAST ADDED SECTION 
                            this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, it->second.second + it->second.first->sectionSize)));
                            break;
                        }
                    }

                    if (!existsFromAnotherParser){
                        // If it doesn't exist from another parser, we add it after the last added section

                        // If the sections vector is empty, we just add it
                        if (this->sections.empty()){
                            this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, 0)));
                        }
                        else {
                            // If the sections vector is not empty, we find the address of the last added section and add it after it

                            // std::cout << "Velicina poslednje dodate sekcije je: " << lastSectionSymbol->sectionSize << endl;

                            this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, this->findTheNextAvailablePossition())));
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

    // for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
    //     std::string fileName = it->first;
    //     SymbolLD* symbolPointer = it->second.first;
    //     int address = it->second.second;

    //     // Print in the specified format
    //     // std::std::cout << "File: " << fileName << ", Section: " << symbolPointer->name << " ---> Address: " << hex << address << std::endl;
    // }

}


void Linker::fixOverlaps(){
    // TODO: CHECK IF WE ARE ALLOWED TO MOVE THE SECTIONS THAT ARE PLACED BY THE -place FLAG?

    if (this->sections.size() <= 1){
        // There is only 1 section, we don't need to fix anything
        return;
    }

    // Sort the sections by address in ascending order
    std::sort(this->sections.begin(), this->sections.end(), compareByAddress);

    // Iterate through sections, starting from the second section
    for (size_t i = 1; i < sections.size(); ++i) {
        const std::pair<std::string, std::pair<SymbolLD*, int> >& currentSection = sections[i];
        const std::pair<std::string, std::pair<SymbolLD*, int> >& precedingSection = sections[i - 1];

        // Check for overlap with the preceding section
        if (currentSection.second.second < precedingSection.second.second + precedingSection.second.first->sectionSizeWithLiterals) {  // TODO: Check if it should be sectionSize or sectionSizeWithLiterals
            // Adjust the address to avoid overlap
            int newAddress = precedingSection.second.second + precedingSection.second.first->sectionSizeWithLiterals;
            sections[i].second.second = newAddress;

            // Check for further overlaps with higher-addressed sections
            for (size_t j = i + 1; j < sections.size(); ++j) {
                if (sections[j].second.second < newAddress) {
                    // Move higher-addressed sections to avoid overlaps
                    sections[j].second.second = newAddress;
                    newAddress += sections[j].second.first->sectionSizeWithLiterals;
                } else {
                    break;  // Stop checking once no more overlaps are found
                }
            }
        }
    }

    // print the entire sections vector

    // for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
    //     std::string fileName = it->first;
    //     SymbolLD* symbolPointer = it->second.first;
    //     int address = it->second.second;

    //     // Print in the specified format
    //     std::std::cout << "File: " << fileName << ", Section: " << symbolPointer->name << " ---> Address: " << hex << address << 
    //     " - " << hex << address + symbolPointer->sectionSizeWithLiterals << std::endl;
    // }
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

                int sectionNumber = this->parsers[i]->symbolTable[j]->section;

                if (sectionNumber == -1){
                    // This symbol is not defined in this file, we won't fix it's value
                    // TODO: Ok? Do we need to do anything else?
                    continue;
                }
                string sectionName = this->parsers[i]->getSectionSymbolById(sectionNumber)->name;
                

                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                    if (it->first.compare(this->parsers[i]->inputFileName) == 0 && it->second.first->name.compare(sectionName) == 0){
                        // We have found the section in which this symbol is defined
                        // We need to add the address of this section to the value of this symbol
                        this->parsers[i]->symbolTable[j]->value += it->second.second;
                        break;
                    }
                }
            }
            else {
                // Section symbol

                // We set the value of the section symbol to the starting address of that section from the sections vector
                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                    if (it->first.compare(this->parsers[i]->inputFileName) == 0 && it->second.first->name.compare(this->parsers[i]->symbolTable[j]->name) == 0){
                        // We have found the section
                        this->parsers[i]->symbolTable[j]->value = it->second.second;
                        // std::cout << "WE STORED THE VALUE OF THE SECTION SYMBOL: " << this->parsers[i]->symbolTable[j]->value << endl;
                        break;
                    }
                }

                // Fix the section's the relocation table
                for (int k = 0; k < this->parsers[i]->symbolTable[j]->relocationTable.size(); k++){
                    // We need to find the symbol in the symbol table
                    // We will do this by looking for the symbol in the symbol table

                    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                        if (it->first.compare(this->parsers[i]->inputFileName) == 0 && it->second.first->name.compare(this->parsers[i]->symbolTable[j]->name) == 0){
                            // We have found the section
                            this->parsers[i]->symbolTable[j]->relocationTable[k]->offset += it->second.second;
                            break;
                        }
                    }
                }

                // Fix the section's literal table
                for (int k = 0; k < this->parsers[i]->symbolTable[j]->literalTable.size(); k++){
                    // We need to find the symbol in the symbol table
                    // We will do this by looking for the symbol in the symbol table

                    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, int> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                        if (it->first.compare(this->parsers[i]->inputFileName) == 0 && it->second.first->name.compare(this->parsers[i]->symbolTable[j]->name) == 0){
                            // We have found the section
                            this->parsers[i]->symbolTable[j]->literalTable[k]->location += it->second.second;
                            break;
                        }
                    }
                }
            }
        }

        // Write out the symbol table for each parser
        std::cout << "Symbol table for: " << this->parsers[i]->inputFileName << endl;
        for (int j = 0; j < this->parsers[i]->symbolTable.size(); j++){
            std::cout << " Symbol name: " << this->parsers[i]->symbolTable[j]->name << " Symbol value: " << hex << this->parsers[i]->symbolTable[j]->value << " Section name: " << this->parsers[i]->symbolTable[j]->sectionName << endl;
        }

    }
}


void Linker::createGlobalSymbolTable(){


    for (int i = 0; i < this->parsers.size(); i++){

        for (int j = 0; j < this->parsers[i]->symbolTable.size(); j++){

            if (this->parsers[i]->symbolTable[j]->isSection){
                // We don't need to add section symbols to the global symbol table
                continue;
            }

            // Here we set sectionName for symbols that are defined in some section inside parser[i]
            int symbolSectionNumber = this->parsers[i]->symbolTable[j]->section;

            if (symbolSectionNumber != -1){
                string sectionName = this->parsers[i]->getSectionSymbolById(symbolSectionNumber)->name;
                this->parsers[i]->symbolTable[j]->sectionName = sectionName;
            }


            // Here we check if the symbol is already in the global symbol table
            SymbolLD* symbol = this->getGlobalSymbolByName(this->parsers[i]->symbolTable[j]->name);

            // If the symbol doesn't already exist in the global symbol table and is not an extern symbol (section number != -1) we add it
            // TODO: Only add defined symbols? -> Is that solved with section number != -1?
            if (symbol == nullptr && symbolSectionNumber != -1){
                this->globalSymbolTable.push_back(this->parsers[i]->symbolTable[j]);
            }
            else if (symbol && symbolSectionNumber != -1 && !symbol->isGlobal && symbol->sectionName.compare(this->parsers[i]->symbolTable[j]->sectionName) == 0){
                // If the symbol with the same name is already in the global symbol table AND
                // if the symbol is not global (meaning it is defined inside the current file) AND
                // it is defined inside the same section as the current symbol
                // We have a multiply defined symbol
                std::cout << "Multiply defined symbol: " << this->parsers[i]->symbolTable[j]->name << endl;
                exit(EXIT_FAILURE);
            }
            else if (symbol && symbol->sectionName.compare(this->parsers[i]->symbolTable[j]->sectionName) != 0){
                // We have a symbol with the same name in the global symbol table, but it is defined in a different section
                // We need to add this symbol to the global symbol table
                this->globalSymbolTable.push_back(this->parsers[i]->symbolTable[j]);
            }

        }
    }

    // Go through all the parsers once again, and find if there are any unresolved symbols left

    for (int i = 0; i < this->parsers.size(); i++){

        for (int j = 0; j < this->parsers[i]->symbolTable.size(); j++){

            if (this->parsers[i]->symbolTable[j]->isSection) continue;

            if (this->parsers[i]->symbolTable[j]->section == -1){
                
                // We have an unresolved symbol
                // We need to find it in the global symbol table

                SymbolLD* symbol = this->getGlobalSymbolByName(this->parsers[i]->symbolTable[j]->name);


                if (symbol == nullptr){
                    std::cout << "Unresolved symbol: " << this->parsers[i]->symbolTable[j]->name << endl;
                    exit(EXIT_FAILURE);
                }
                else {
                    // We have found the symbol in the global symbol table
                    // We need to set the value of the symbol in the parser's symbol table to the value of the symbol in the global symbol table
                    this->parsers[i]->symbolTable[j]->value = symbol->value;
                }
            }
        }
    }

    // print out the global symbol table

    std::cout << "Global symbol table:" << endl;
    for (int i = 0; i < this->globalSymbolTable.size(); i++){
        std::cout << " Name: " << this->globalSymbolTable[i]->name << " Value: " << this->globalSymbolTable[i]->value << " Section name: " << this->globalSymbolTable[i]->sectionName << endl;
    }
}


void Linker::setMachineCodeAddresses(){
    // Go through linker's vector<pair<string, pair<SymbolLD*,int> > > sections;
    // For each section, we are going to pair an address with each machine code byte
    // Keep in mind, some machine code vector elements are 1 byte, and some are 4 bytes
    // It would be ideal to place them in the section's machineCodeWithAddresses vector byte by byte
    // So we will split the 4-bytes into single bytes and place them in the vector

    string concatenatedMachineCode;

    for (int i = 0; i < this->sections.size(); i++){

        concatenatedMachineCode = "";

        SymbolLD* sectionSymbol = this->sections[i].second.first;
        int address = this->sections[i].second.second;

        // HERE WE ONLY WANT DIRECTIVES/INSTRUCTION MACHINE CODE, WE DO NOT WANT LITERAL POOL MACHINE CODE
        for (int j = 0; j < sectionSymbol->machineCode.size() - sectionSymbol->literalTable.size(); j++){

            // TODO: What if the size of some literal is 3 half-bytes? (1.5 bytes, ex: 0x123)

            // machineCode is a vector of strings
            // each string character represents 1 half-byte (4 bits)
            // the minimum size of a string is 2 (1 byte)
            // the maximum size of a string is 8 (4 bytes)
            // split the ones that are > 1
            concatenatedMachineCode += sectionSymbol->machineCode[j];

        }

        if (concatenatedMachineCode.size() / 2 != 0){
            
            // we will take byte by byte (2 chars) from the concatenatedMachineCode string and place it in the machineCodeWithAddresses vector

            for (int k = 0; k < concatenatedMachineCode.size(); k += 2){
                string byte = concatenatedMachineCode.substr(k, 2);
                sectionSymbol->machineCodeWithAddresses.push_back(make_pair(address, byte));
                address++;
            }
        }
        else {
            std::cout << "Size of concatenated machine code is not even. It is: " << concatenatedMachineCode.size() << endl;
            exit(EXIT_FAILURE);
        }
        
    }

    // print out the machineCodeWithAddresses vector for each section

    for (int i = 0; i < this->sections.size(); i++){
        SymbolLD* sectionSymbol = this->sections[i].second.first;
        std::cout << "Section name: " << sectionSymbol->name << endl;
        std::cout << "VELICINA NJENOG MASINSKOG KODA JE: " << sectionSymbol->machineCodeWithAddresses.size() << endl;
        for (int j = 0; j < sectionSymbol->machineCodeWithAddresses.size(); j++){
            std::cout << " Address: " << hex << sectionSymbol->machineCodeWithAddresses[j].first << " Machine code: " << sectionSymbol->machineCodeWithAddresses[j].second << endl;
        }
    }
}


void Linker::processRelocations(){

    int addressForFixing = 0;
    string symbolName = "";
    string symbolValueString = "";
    std::stringstream hexStream;

    for (int i = 0; i < this->sections.size(); i++){


        for (int j = 0; j < this->sections[i].second.first->relocationTable.size(); j++){  // sections[i].second.first == SymbolLD*
            
            // we overwrite the data from that section where the relocation is happening
            // addressForFixing = this->sections[i].second.second + this->sections[i].second.first->relocationTable[j]->offset;  // TODO: Maybe we can do it like this, and then there is no need for fixing offset/addres of relocations/literals

            addressForFixing = this->sections[i].second.first->relocationTable[j]->offset;

            // Get the symbols name, so we can get it's true value
            symbolName = this->sections[i].second.first->relocationTable[j]->symbolName;

            // Get the symbol from the global symbol table
            SymbolLD* symbol = this->getGlobalSymbolByName(symbolName);
            hexStream << std::hex << symbol->value;
            symbolValueString = hexStream.str();

            // Now go through machineCodeWithAddresses vector and write 4B of symbol value starting from addressForFixing
            // TODO: Check if the value is right! (It should be, we should've fixed it in the fixAddresses function)

            // Go through the machineCodeWithAddresses vector and fix the bytes

            for (int k = 0; k < this->sections[i].second.first->machineCodeWithAddresses.size(); k++){
                
                if (this->sections[i].second.first->machineCodeWithAddresses[k].first == addressForFixing){    
                    // We found it, fix the bytes and break
                    this->sections[i].second.first->machineCodeWithAddresses[k].second = symbolValueString.substr(0, 2);
                    this->sections[i].second.first->machineCodeWithAddresses[k + 1].second = symbolValueString.substr(2, 2);
                    this->sections[i].second.first->machineCodeWithAddresses[k + 2].second = symbolValueString.substr(4, 2);
                    this->sections[i].second.first->machineCodeWithAddresses[k + 3].second = symbolValueString.substr(6, 2);
                    break;
                }
            }
        }
    }
}


void Linker::writeOutputFile(){

    // The output of the linker should be address: 8_bytes_of_machine_code_split_by_space, example:
    /*
        0000: 00 01 02 03 04 05 06 07
        0008: 08 09 0A 0B 0C 0D 0E 0F
        0010: 10 11 12 13 14 15 16 17
        etc...
    */
   // We will use machineCodeWithAddress here

   // Open output file

   if (!this->outputFile.is_open()){
       std::cout << "Output file could not be opened" << endl;
       exit(EXIT_FAILURE);
   }

   for (int i = 0; i < this->sections.size(); i++){
       SymbolLD* sectionSymbol = this->sections[i].second.first;

       for (int j = 0; j < sectionSymbol->machineCodeWithAddresses.size(); j++){
        
            this->outputFile << std::setw(4) << std::setfill('0') << hex << sectionSymbol->machineCodeWithAddresses[j].first << ": ";

            for (int k = 0; k < 8; k++){
                this->outputFile << sectionSymbol->machineCodeWithAddresses[j].second << " ";
                if (k != 7) j++;
            }

            this->outputFile << endl;
       }
   }

    this->outputFile.close();
    
}