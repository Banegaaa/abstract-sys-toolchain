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

};