#include <fstream>
#include <vector>
#include <string>
#include <iostream>
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

    outputFile << "Symbol table:" << endl;
    for (int i = 0; i < symbolTable.size(); i++)
    {
        outputFile << symbolTable[i].number;            // ID
        outputFile << " " << symbolTable[i].name;       // Name
        outputFile << " " << symbolTable[i].value;      // Value
        if (symbolTable[i].is_defined)                  // Defined
            outputFile << " " << "DEF";
        else
            outputFile << " " << "UNDEF";
        if (symbolTable[i].is_global)                   // Global
            outputFile << " " << "G";
        else
            outputFile << " " << "L";
        if (symbolTable[i].is_section)                  // Section/Variable
            outputFile << " " << "S";
        else
            outputFile << " " << "V";
        outputFile << " " << symbolTable[i].section;    // Section number
        outputFile << " " << symbolTable[i].lc_value;   // LC value
        outputFile << endl;
    }

    return EXIT_SUCCESS;
}