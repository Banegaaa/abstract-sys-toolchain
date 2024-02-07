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


Linker::Linker(bool hexFlag, vector<pair<string, long> > placeSections, vector<string> inputFileNames, string outputFileName){
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

    for (int it = 0; it < this->placeSections.size(); it++){
        
        string sectionName = this->placeSections[it].first;  // first = key = section_name
        long address = this->placeSections[it].second;        // second = value = address


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

                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
                    if (it->first.compare(this->parsers[i]->inputFileName) == 0 && it->second.first->name.compare(sectionSymbol->name) == 0){
                        alreadyInSections = true;
                        break;
                    }
                }

                if (!alreadyInSections){
                    // If the section from this input file is not already in the sections vector, we look for the same section from another vector

                    bool existsFromAnotherParser = false;

                    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it = sections.begin(); it != sections.end(); ++it){
                        if (it->second.first->name.compare(sectionSymbol->name) == 0){
                            existsFromAnotherParser = true;

                            // IF IT EXISTS, WE WILL ADD IT EMEDIATELY AFTER THE LAST ADDED SECTION 
                            // But first, we need to check if we already did that, that is, maybe we already added it from another parser after this one
                            // We need to check if the next section is also the same name
                            // if it is -> we add it after the last added section with this name
                            // if it isn't -> we add it emediately after this one

                            std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator nextIt = it;
                            nextIt++;

                            if (nextIt != sections.end() && nextIt->second.first->name.compare(sectionSymbol->name) == 0){
                                // The next section is also the same name
                                // We need to add it after the last added section with the same name
                                // We need to find the last added section with the same name

                                std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator lastIt = it;

                                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it2 = it; it2 != sections.end(); ++it2){
                                    if (it2->second.first->name.compare(sectionSymbol->name) == 0){
                                        lastIt = it2;
                                    }
                                    else {
                                        break;
                                    }
                                }

                                this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, lastIt->second.second + lastIt->second.first->sectionSize)));
                                break;
                            }
                            else {
                                // The next section is not the same name
                                // We need to add it emediately after this one
                                this->sections.push_back(make_pair(this->parsers[i]->inputFileName, make_pair(sectionSymbol, it->second.second + it->second.first->sectionSize)));
                                break;
                            }
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
                    // TODO: Check if this is correct, maybe we need to append some sections
                }
                else {

                    // The section from this file is alredy in the sections vector -> we don't need to do anything
                    continue;
                }

            }
        }
    }

    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
        std::string fileName = it->first;
        SymbolLD* symbolPointer = it->second.first;
        int address = it->second.second;

        // Print in the specified format
        std::cout << "File: " << fileName << ", Section: " << symbolPointer->name << " ---> Address: " << hex << address << std::endl;
    }


}


void Linker::fixOverlaps(){
    // TODO: CHECK IF WE ARE ALLOWED TO MOVE THE SECTIONS THAT ARE PLACED BY THE -place FLAG?

    bool overlaps = false;

    if (this->sections.size() <= 1){
        // There is only 1 section, we don't need to fix anything
        return;
    }

    // Sort the sections by address in ascending order
    std::sort(this->sections.begin(), this->sections.end(), compareByAddress);

    // Iterate through sections, starting from the second section
    for (size_t i = 1; i < sections.size(); ++i) {
        
        overlaps = false;

        const std::pair<std::string, std::pair<SymbolLD*, int> >& currentSection = sections[i];
        const std::pair<std::string, std::pair<SymbolLD*, int> >& precedingSection = sections[i - 1];

        // Check for overlap with the preceding section
        if (precedingSection.second.second <= currentSection.second.second < precedingSection.second.second + precedingSection.second.first->sectionSize) {  // TODO: Check if it should be sectionSize or sectionSizeWithLiterals
            
            overlaps = true;
            
            // Adjust the address to avoid overlap
            int newAddress = precedingSection.second.second + precedingSection.second.first->sectionSize;
            sections[i].second.second = newAddress;

            std::cout << "Nova adresa za sekciju: " << currentSection.second.first->name << " je: " << hex << newAddress << std::endl;

            // Check for further overlaps with higher-addressed sections
            for (size_t j = i + 1; j < sections.size(); ++j) {
                if (sections[j].second.second < newAddress) {
                    // Move higher-addressed sections to avoid overlaps
                    sections[j].second.second = newAddress;
                    newAddress += sections[j].second.first->sectionSize;
                } else {
                    break;  // Stop checking once no more overlaps are found
                }
            }

            // Sort again after fixing the overlaps
            std::sort(this->sections.begin(), this->sections.end(), compareByAddress);
        }

        if (!overlaps) {

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


void Linker::fixOverlaps2(){

    std::cout << "FIXING OVERLAPS 2" << endl;

    bool overlaps = false;
    bool finished = false;

    if (this->sections.size() <= 1){
        // There is only 1 section, we don't need to fix anything
        return;
    }

    // print out the section file and names
    for (int i = 0; i < this->sections.size(); i++){
        std::cout << "File: " << this->sections[i].first << " Section: " << this->sections[i].second.first->name << " Address: " << hex << this->sections[i].second.second << endl;
    }

    // Sort the sections by address in ascending order
    std::sort(this->sections.begin(), this->sections.end(), compareByAddress);

    // print out the section file and names after sorting
    for (int i = 0; i < this->sections.size(); i++){
        std::cout << "File: " << this->sections[i].first << " Section: " << this->sections[i].second.first->name << " Address: " << hex << this->sections[i].second.second << endl;
    }

    // Start a loop for fixing overlaps until there are no more overlaps
    while (!finished){
        // start from the second section, so we have a previous section for comparing

        for (int i = 1; i < this->sections.size(); ++i){
            overlaps = false;

            int previousEndAddress = this->sections[i - 1].second.second + this->sections[i - 1].second.first->sectionSize;
            string previousSectionName = this->sections[i - 1].second.first->name;

            if (this->sections[i].second.second < previousEndAddress){

                std::cout << "PREKLAPANJE: " << this->sections[i].second.first->name << " i " << this->sections[i - 1].second.first->name << endl;
                std::cout << "ADRESA PREKLAPANJA: " << hex << this->sections[i].second.second << " i " << hex << previousEndAddress << endl;
                // There is an overlap
                overlaps = true;

                // We need to check if the following section is the same name as the previous
                // If it is, we need to move it after the last section that has the same name as the previous (previousSectionName)

                if (i + 1 < this->sections.size() && this->sections[i + 1].second.first->name.compare(previousSectionName) == 0){
                    // We need to move the following section after the last section that has the same name as the previous
                    int lastSectionWithSameNameIndex = i + 1;

                    for (int j = i + 1; j < this->sections.size(); j++){
                        if (this->sections[j].second.first->name.compare(previousSectionName) == 0){
                            lastSectionWithSameNameIndex = j;
                        }
                        else {
                            break;
                        }
                    }

                    // Move the section after the last section that has the same name as the previous
                    this->sections[i].second.second = this->sections[lastSectionWithSameNameIndex].second.second + this->sections[lastSectionWithSameNameIndex].second.first->sectionSize;
                }
                else {
                    // We need to move the section after the previous section
                    this->sections[i].second.second = previousEndAddress;
                }
            }
            
            // Sort the sections after fixing the overlaps
            std::sort(this->sections.begin(), this->sections.end(), compareByAddress);
        }

        finished = !overlaps;
    }
}


void Linker::fixOverlaps3(){

    if (this->sections.size() <= 1){
        // There is only 1 section, we don't need to fix anything
        return;
    }

    // We will create a vector from sections vector, where we will have only 1 element for each section name
    // We will use this vector to check for overlaps

    std::vector<std::pair<std::string, std::pair<int, int> > > mergedSections;

    // string element -> section name
    // first int -> start address of the lowest section with that name
    // second int -> end address of the highest section with that name

    // Now we need to fill the sectionsWithUniqueNames vector

    for (int i = 0; i < this->sections.size(); i++){
        string sectionName = this->sections[i].second.first->name;
        int sectionStartAddress = this->sections[i].second.second;
        int sectionEndAddress = this->sections[i].second.second + this->sections[i].second.first->sectionSize;

        if (mergedSections.empty()){
            mergedSections.push_back(make_pair(sectionName, make_pair(sectionStartAddress, sectionEndAddress)));
        }
        else if (mergedSections.back().first.compare(sectionName) == 0){
            // We have the same section name as the last added element in the mergedSections vector
            // We need to check if the start address is lower than the start address of the last added element
            // We also need to check if the end address is higher than the end address of the last added element

            if (sectionEndAddress > mergedSections.back().second.second){
                mergedSections.back().second.second = sectionEndAddress;
            }
        }
        else {
            // We have a new section name
            mergedSections.push_back(make_pair(sectionName, make_pair(sectionStartAddress, sectionEndAddress)));
        }
    }

    // print out the mergedSections vector

    // for (int i = 0; i < mergedSections.size(); i++){
    //     std::cout << "Section name: " << mergedSections[i].first << " Start address: " << hex << mergedSections[i].second.first << " End address: " << hex << mergedSections[i].second.second << endl;
    // }

    // Now we will fix the overlaps in the mergedSections vector
    bool overlaps = false;
    bool finished = false;

    while (!finished){

        // We start from the second section (that is the current one)
        for (int i = 1; i < mergedSections.size(); i++){
            overlaps = false;
            int previousStartAddress = mergedSections[i - 1].second.first;
            int previousEndAddress = mergedSections[i - 1].second.second;
            int currentStartAddress = mergedSections[i].second.first;

            if (previousStartAddress <= currentStartAddress && currentStartAddress < previousEndAddress){
                // There is an overlap
                overlaps = true;

                // We need to move the current section
                int difference = previousEndAddress - currentStartAddress;
                mergedSections[i].second.first = mergedSections[i - 1].second.second;
                mergedSections[i].second.second = mergedSections[i].second.second + difference;
            }
        }

        finished = !overlaps;
    }

    // print out the mergedSections vector after fixing the overlaps

    for (int i = 0; i < mergedSections.size(); i++){
        std::cout << "Section name: " << mergedSections[i].first << " Start address: " << hex << mergedSections[i].second.first << " End address: " << hex << mergedSections[i].second.second << endl;
    }

    // We have now fixed the overlaps in the mergedSections vector, we need to apply the changes to the sections vector
    // We will go through the sections vector, and for the first showing of each section (name), we will calculate the offset from it's original starting address and the new starting address
    // Then we will add that offset to the starting address of all the sections with the same name

    for (int i = 0; i < mergedSections.size(); i++){
        string sectionName = mergedSections[i].first;

        // We need to find the first showing of the section with the name sectionName in the sections vector
        int firstShowingIndex = -1;
        int offset = 0;

        for (int j = 0; j < this->sections.size(); j++){
            if (this->sections[j].second.first->name.compare(sectionName) == 0){
                firstShowingIndex = j;
                offset = mergedSections[i].second.first - this->sections[j].second.second;
                break;
            }
        }

        if (offset == 0){
            // We don't need to do anything
            continue;
        }
        else{
            std::cout << "Offset: " << hex << offset << endl;
        }

        // We need to add the offset to the starting address of all the sections with the same name
        for (int j = firstShowingIndex; j < this->sections.size(); j++){
            if (this->sections[j].second.first->name.compare(sectionName) == 0){
                this->sections[j].second.second += offset;
            }
        }
    }

    // print out the sections vector after fixing the overlaps

    for (int i = 0; i < this->sections.size(); i++){
        std::cout << "File: " << this->sections[i].first << " Section: " << this->sections[i].second.first->name << " Address: " << hex << this->sections[i].second.second << endl;
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

                int sectionNumber = this->parsers[i]->symbolTable[j]->section;

                if (sectionNumber == -1){
                    // This symbol is not defined in this file, we won't fix it's value
                    // TODO: Ok? Do we need to do anything else?
                    continue;
                }
                string sectionName = this->parsers[i]->getSectionSymbolById(sectionNumber)->name;
                

                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
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
                for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
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

                    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
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

                    for (std::vector<std::pair<std::string, std::pair<SymbolLD*, long> > >::iterator it = sections.begin(); it != sections.end(); ++it) {
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

        // Literals are counted into the machine code
        for (int j = 0; j < sectionSymbol->machineCode.size(); j++){

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

    // for (int i = 0; i < this->sections.size(); i++){
    //     SymbolLD* sectionSymbol = this->sections[i].second.first;
    //     std::cout << "Section name: " << sectionSymbol->name << endl;
    //     std::cout << "VELICINA NJENOG MASINSKOG KODA JE: " << sectionSymbol->machineCodeWithAddresses.size() << endl;
    //     for (int j = 0; j < sectionSymbol->machineCodeWithAddresses.size(); j++){
    //         std::cout << " Address: " << hex << sectionSymbol->machineCodeWithAddresses[j].first << " Machine code: " << sectionSymbol->machineCodeWithAddresses[j].second << endl;
    //     }
    // }
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

            std::cout << "Prepravljamo simbol: " << symbolName << " na adresi: " << hex << addressForFixing << endl;

            // Get the symbol from the global symbol table
            SymbolLD* symbol = this->getGlobalSymbolByName(symbolName);
            hexStream << std::hex << symbol->value;
            symbolValueString = hexStream.str();
            std::cout << "Vrednost simbola: " << symbolValueString << endl;

            // clear the stringstream
            hexStream.str(std::string());

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
    for (int i = 0; i < this->sections.size(); i++){
        SymbolLD* sectionSymbol = this->sections[i].second.first;
        std::cout << "Section name: " << sectionSymbol->name << endl;
        std::cout << "VELICINA NJENOG MASINSKOG KODA JE: " << sectionSymbol->machineCodeWithAddresses.size() << endl;
        for (int j = 0; j < sectionSymbol->machineCodeWithAddresses.size(); j++){
            std::cout << " Address: " << hex << sectionSymbol->machineCodeWithAddresses[j].first << " Machine code: " << sectionSymbol->machineCodeWithAddresses[j].second << endl;
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

   // print out the machineCodeWithAddresses vector for each section

    // Write out to the output file machineCodeWithAddresses vector for each section
    // Write it in format address: 8_bytes_of_machine_code_split_by_space

    for (int i = 0; i < this->sections.size(); i++){
        SymbolLD* sectionSymbol = this->sections[i].second.first;

        for (int j = 0; j < sectionSymbol->machineCodeWithAddresses.size(); j++){
            this->outputFile << std::setw(4) << std::setfill('0') << hex << sectionSymbol->machineCodeWithAddresses[j].first << ": ";

            // now i want to write out the next 8 bytes of machine code (counting the one at j)
            for (int k = 0; k < 8; k++){

                if (j >= sectionSymbol->machineCodeWithAddresses.size()){
                    break;
                }
                else {
                    this->outputFile << sectionSymbol->machineCodeWithAddresses[j].second << " ";
                }
                if (k != 7) j++;
            }

            this->outputFile << endl;
        }
    }

    this->outputFile.close();
    
}


// void Linker::writeOutputFile() {
//     if (!this->outputFile.is_open()) {
//         std::cout << "Output file could not be opened" << std::endl;
//         exit(EXIT_FAILURE);
//     }

//     // Keep track of the last written address
//     int lastAddress = -1;

//     for (int i = 0; i < this->sections.size(); i++) {
//         SymbolLD* sectionSymbol = this->sections[i].second.first;

//         for (int j = 0; j < sectionSymbol->machineCodeWithAddresses.size(); j++) {
//             int currentAddress = sectionSymbol->machineCodeWithAddresses[j].first;

//             // Check if the next section has the same name and the address continues the previous line
//             if (currentAddress == lastAddress) {
//                 for (int k = 0; k < 8; k++) {
//                     // Check if j is within bounds
//                     if (j < sectionSymbol->machineCodeWithAddresses.size()) {
//                         this->outputFile << sectionSymbol->machineCodeWithAddresses[j].second << " ";
//                         j++;  // Increment j to move to the next element
//                     } else {
//                         break;
//                     }
//                 }
//                 j--;  // Decrement j to avoid double increment in the outer loop
//             } else {
//                 // Start a new line
//                 this->outputFile << std::setw(4) << std::setfill('0') << std::hex << currentAddress << ": ";

//                 // Write the next 8 bytes of machine code
//                 for (int k = 0; k < 8; k++) {
//                     // Check if j is within bounds
//                     if (j < sectionSymbol->machineCodeWithAddresses.size()) {
//                         this->outputFile << sectionSymbol->machineCodeWithAddresses[j].second << " ";
//                         j++;  // Increment j to move to the next element
//                     } else {
//                         break;
//                     }
//                 }
//             }

//             // Update the last written address
//             lastAddress = currentAddress;

//             this->outputFile << std::endl;
//         }
//     }

//     this->outputFile.close();
// }
