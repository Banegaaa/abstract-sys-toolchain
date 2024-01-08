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

vector<Token> tokenList;


int main(int argc, char *argv[])
{
    int ntoken;

    ntoken = yylex();
    while (ntoken != 1)
    {
        // Ubacujemo tokene {kod, string} u listu tokena
        tokenList.push_back(Token(ntoken, (string)yytext));
        ntoken = yylex();
    }

    //call firstPass
    firstPass();

    // call secondPass
    // secondPass();

    // write "token_value - token_name" to stdout (using standard for loop)
    // for (int i = 0; i < tokenList.size(); i++)
    // {
    //     cout << tokenList[i].value << " - " << tokenList[i].name << endl;
    // }

    return EXIT_SUCCESS;
}