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


    return EXIT_SUCCESS;
}