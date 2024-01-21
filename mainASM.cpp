#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include "symbol.h"
#include "token.h"
#include "firstPass.h"
#include "secondPass.h"

using namespace std;

extern int yylex(void);
extern const char *yytext;
extern FILE* yyin;


vector<Token> tokenList;
vector<Symbol> symbolTable;

ifstream inputFile;
ofstream outputFile;

int main(int argc, char *argv[])
{

    if (argc < 4){
        cout << "Not enough arguments" << endl;
        return EXIT_FAILURE;
    }
    else if (argc > 4){
        cout << "Too many arguments" << endl;
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-o") != 0){
        cout << "Wrong first argument" << endl;
        return EXIT_FAILURE;
    }

    outputFile.open(argv[2]);
    if (!outputFile.is_open()){
        cout << "Output file not opened" << endl;
        return EXIT_FAILURE;
    }

    inputFile.open(argv[3]);
    if (!inputFile.is_open()){
        cout << "Input file not opened" << endl;
        return EXIT_FAILURE;
    }


    int ntoken;

    yyin = fopen(argv[3], "r");
    if (yyin == NULL)
    {
        cout << "Input file not opened" << endl;
        return EXIT_FAILURE;
    }

    ntoken = yylex();
    while (ntoken != 1)
    {
        // Ubacujemo tokene {kod, string} u listu tokena
        tokenList.push_back(Token(ntoken, (string)yytext));
        ntoken = yylex();
        cout << "Token: " << yytext << endl;
    }

    firstPass();

    secondPass();

    // write "token_value - token_name" to stdout (using standard for loop)
    // for (int i = 0; i < tokenList.size(); i++)
    // {
    //     cout << tokenList[i].value << " - " << tokenList[i].name << endl;
    // }
    
    
    // PRINT OUT THE SYMBOL TABLE
    outputFile << "SymbolTable:" << endl;
    outputFile << endl;

    for (int i = 0; i < symbolTable.size(); i++){

        outputFile << std::setw(5) << symbolTable[i].number;       // ID
        outputFile << std::internal << std::setw(20) << symbolTable[i].name;       // Name
        outputFile << std::internal << std::setw(15) << symbolTable[i].value;      // Value
        if (symbolTable[i].is_defined)                                             // Defined
            outputFile << std::internal << std::setw(5) << "DEF";
        else
            outputFile << std::internal << std::setw(5) << "UNDEF";
        if (symbolTable[i].is_global)                                               // Global
            outputFile << std::internal << std::setw(3) << "G";
        else
            outputFile << std::internal << std::setw(3) << "L";
        if (symbolTable[i].is_section)                                              // Section/Variable
            outputFile << std::internal << std::setw(3) << "S";
        else
            outputFile << std::internal << std::setw(3) << "V";
        outputFile << std::internal << std::setw(3) << symbolTable[i].section;    // Section number
        outputFile << std::internal << std::setw(3) << symbolTable[i].lc_value;   // LC value
        outputFile << endl;
    }

    outputFile << endl;


    // PRINT OUT THE SECTION TABLE

    outputFile << "SectionTable:" << endl;
    outputFile << endl;
    for (int i = 0; i < symbolTable.size(); i++){
        if (symbolTable[i].is_section){
            outputFile << std::setw(20) << symbolTable[i].name;                             // Name
            outputFile << std::internal << std::setw(4) << symbolTable[i].sectionSize;      // Size
            outputFile << endl;
        }
    }

    outputFile << endl;


    // PRINT OUT THE LITERAL TABLE

    outputFile << "LiteralTable:" << endl;
    outputFile << endl;
    for (int i = 0; i < symbolTable.size(); i++){
        if (symbolTable[i].is_section){
            for (int j = 0; j < symbolTable[i].literals.size(); j++){
                outputFile << std::setw(5) << hex << symbolTable[i].literals[j]->value;                   // Value
                outputFile << std::internal << std::setw(4) << symbolTable[i].literals[j]->location;      // Location
                outputFile << std::internal << std::setw(4) << symbolTable[i].literals[j]->size;          // Size
                outputFile << std::internal << std::setw(4) << symbolTable[i].literals[j]->index;         // Index
                if (symbolTable[i].literals[j]->symbolIndex != -1)                                        // Symbol index
                    outputFile << std::internal << std::setw(4) << symbolTable[i].literals[j]->symbolIndex;
                outputFile << endl;
            }
        }
    }
    outputFile << endl;

    return EXIT_SUCCESS;
}