#include <iostream>
#include <fstream>
#include <string>
#include "../inc/emulator.h"

using namespace std;

int main(int argc, char** argv) {

    // Emulator is run using the following command:
    // ./emulator <input_file_name>

    if (argc != 2) {
        cout << "Invalid number of arguments" << endl;
        return EXIT_FAILURE;
    }

    string inputFileName = argv[1];

    Emulator* emulator = new Emulator(inputFileName);  // Create emulator
    
    emulator->parseInputFileAndLoadIntoMemory();  // Parse input file and load it into memory

    emulator->execute();  // Execute program

    // EXECUTION FINISHED -> Print results

    std::cout << "Emulated processor executed halt instruction" << std::endl;
    std::cout << "Emulated processor state:" << std::endl;

    for (int i = 0; i < 16; i++){
        
        if (i != 0 && i % 4 == 0) {
            std::cout << std::endl;
        }
        std::cout << "r" << i << "=" << "0x" << std::setw(8) << std::setfill('0') << hex << emulator->gpr[i] << std::setw(1) << "\t";
    }

    return EXIT_SUCCESS;
}