#include <iostream>
#include <fstream>
#include <string>
#include "../inc/parserLD.h"
#include "../inc/linker.h"

using namespace std;

unordered_map<string, int> placeSections;  // (section_name, address)

string arg;
string sectionName;
string address;
string outputFileName;
vector<string> inputFileNames;

string tmp;

int main(int argc, char *argv[])
{
    /*
        Example of linker running:

        ./linker -hex -o output.o input1.o input2.o input3.o
        or
        ./linker -hex -place=data@0x4000F000 -o output.o input1.o input2.o

        - hex is required
        - place is optional and can be used multiple times for multiple different sections
        - o is required
        - input files are required, and there can be any number of them
    */

    if (argc < 4){
        cout << "Not enough arguments" << endl;
        return EXIT_FAILURE;
    }
    
    // ------------------------------------------------------------

    // Check if hex flag is set

    bool hexFlag = false;

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-hex") == 0){
            hexFlag = true;
        }
    }
    if (!hexFlag){
        cout << "Hex flag not set" << endl;
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------

    // Check for -place flags (go through all arguments and check if any of them contain "-place")

    for (int i = 0; i < argc; i++){
        
        arg = argv[i];
        
        if (arg.find("-place") != string::npos){
            // arg == -place=data@0x4000F000
            // We need to extract data and 0x4000F000 from this string
            tmp = arg.substr(7);  // tmp == data@0x4000F000
            sectionName = tmp.substr(0, tmp.find("@"));  // sectionName == data
            address = tmp.substr(tmp.find("@") + 1);  // address == 0x4000F000

            // Add to placeSections map
            placeSections.insert(std::make_pair(sectionName, stoi(address, nullptr, 16)));
            //
        }
    }

    // ------------------------------------------------------------

    // Check for -o flag and get output file name

    bool oFlag = false;
    int startingIndexOfInputFiles = 0;

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-o") == 0){
            oFlag = true;

            // Get output file name
            outputFileName = argv[i + 1];
            startingIndexOfInputFiles = i + 2;
        }
    }
    if (!oFlag){
        cout << "Output file not specified" << endl;
        return EXIT_FAILURE;
    }

    // ------------------------------------------------------------

    // Get input file names

    for (int i = startingIndexOfInputFiles; i < argc; i++){
        inputFileNames.push_back(argv[i]);
    }

    // ------------------------------------------------------------

    // Create Linker object

    Linker* linker = new Linker(hexFlag, placeSections, inputFileNames, outputFileName);

    linker->sectionPlacing();

    linker->fixAddresses();
    
    linker->createGlobalSymbolTable();
}