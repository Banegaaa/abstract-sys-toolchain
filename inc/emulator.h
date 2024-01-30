#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "instructionCodes.h"

using namespace std;

class Emulator {
    
    public:

    string inputFileName;

    fstream inputFile;

    unsigned gpr[16];  // General purpose registers
    unsigned csr[3];  // Control and status registers

    vector<pair<unsigned, string> > memory;

    Emulator(string inputFileName);
    ~Emulator() {};

    void parseInputFileAndLoadIntoMemory();

    string getByteFromMemory(unsigned address);
    void writeByteToMemory(unsigned address, string byte);

    void execute();

};