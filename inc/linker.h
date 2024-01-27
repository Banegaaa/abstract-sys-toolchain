#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "parserLD.h"
#include "symbolLD.h"

using namespace std;

class Linker {
    public:

    bool hexFlag;
    
    unordered_map<string, int> placeSections;  // (section_name, address)
    
    vector<ifstream> inputFiles;

    vector<unsigned char> outputData; // This will be the output file

    vector<ParserLD*> parsers;

    ofstream outputFile;

    vector<pair<string, pair<SymbolLD*,int> > > sections; // (file_name, (section_symbol, base_address))

    vector<SymbolLD*> globalSymbolTable;

    Linker(bool hexFlag, unordered_map<string, int> placeSections, std::vector<string> inputFileNames, string outputFileName);
    ~Linker();

    int findTheNextAvailablePossition();  // This function will find the next available possition to place a section

    void sectionPlacing();  // This function will fill the sections vector

    void fixOverlaps();  // This function will fix overlaps in the sections vector (if there are any)

    void fixAddresses();  // Fixing symbol values, literal and relocation offsets

    int getSymbolIdByNameInLinker(string name);
    SymbolLD* getGlobalSymbolByIdAndSectionName(int id, string sectionName);

    void createGlobalSymbolTable();  // Creating global symbol table (checking for undefined or multiply defined symbols)

    void setMachineCodeAddresses();  // Setting machine code addresses for each section

    void processRelocations();  // Processing relocations

};