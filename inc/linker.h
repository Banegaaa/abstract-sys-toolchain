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

    Linker(bool hexFlag, unordered_map<string, int> placeSections, std::vector<string> inputFileNames, string outputFileName);
    ~Linker();

    void sectionPlacing();

    void fixAddresses();
};