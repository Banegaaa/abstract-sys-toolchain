#include <string>
#include <vector>
#include <iostream>
#include "token.h"
#include "define.h"
#include "symbol.h"
using namespace std;

extern vector<Token> tokenList;
int LC = 0;
string currentSection = "";
int currentSectionNumber = 0;


vector<Symbol> symbolTable;

// helpers:
string tempSymbolName = "";
Symbol* tempSymbol;



int hexToInt(std::string s)
{
    std::string numPart = s.substr(2);
    return std::stoi(numPart, 0, 16);
}

Symbol* symbolExists(string name){
    for (int j = 0; j < symbolTable.size(); j++){
        if (symbolTable[j].name == name){
            return &symbolTable[j];
        }
    }
    return nullptr;
}

int getSectionNumber(string name){
    for (int j = 0; j < symbolTable.size(); j++){
        if (symbolTable[j].name == name && symbolTable[j].is_section){
            return symbolTable[j].number;
        }
    }
    return -1;
}


int firstPass(){
    for (int i = 0; i < tokenList.size(); i++){
    
        switch(tokenList[i].value){

            case COMMENT:
                while (tokenList[i].value != NEWLINE){
                    i++;
                }
                break;
            
            case SKIP:
                i++; // get skip size
                if (tokenList[i].value == DEC_LITERAL){
                    LC += stoi(tokenList[i].name);
                }
                else if (tokenList[i].value == HEX_LITERAL){
                    LC += hexToInt(tokenList[i].name);
                }
                else{
                    cout << "Error: Invalid skip size" << endl;
                    exit(EXIT_FAILURE);
                }
                break;
            
            case GLOBAL:
                while (tokenList[i].value == GLOBAL || tokenList[i].value == COMMA){
                    i++;
                    if (tokenList[i].value == NAME){
                        // check if symbol with that name already exists in the symbol table
                        tempSymbol = symbolExists(tokenList[i].name);
                        if (tempSymbol == nullptr){
                            symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, true, false, symbolTable.size(), false));
                        }
                    }

                    i++;
                }
                break;

            case EXTERN:
                while (tokenList[i].value == EXTERN || tokenList[i].value == COMMA){
                    i++;
                    if (tokenList[i].value == NAME){
                        // check if symbol with that name already exists in the symbol table (TODO: MAYBE NOT NEEDED - IF WE HAVE EXTERN, WE KNOW IT DOESN'T EXIST)
                        tempSymbol = symbolExists(tokenList[i].name);
                        if (tempSymbol == nullptr){
                            symbolTable.push_back(Symbol(tokenList[i].name, -1, 0, true, false, symbolTable.size(), false));
                        }
                    }
    
                    i++;
                }
                break;
            
            case WORD:
                while(tokenList[i].value == WORD || tokenList[i].value == COMMA){
                    i++;
                    if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL || tokenList[i].value == NAME){
                        if (tokenList[i].value == NAME){
                            // check if symbol with that name already exists in the symbol table
                            tempSymbol = symbolExists(tokenList[i].name);
                            if (tempSymbol == nullptr){
                                symbolTable.push_back(Symbol(tokenList[i].name, currentSectionNumber, LC, false, false, symbolTable.size(), false));
                            }
                        }
                        LC += 4;
                    }
                    else{
                        cout << "Error: Invalid word value" << endl;
                        exit(EXIT_FAILURE);
                    }

                    i++;
                }
                break;
            
            case SECTION:
                // section is in format: .section name
                i++;
                if (tokenList[i].value == NAME){

                    // first, set the size of the previous section (if there is one)
                    if (currentSection != ""){
                        tempSymbol = symbolExists(currentSection);
                        tempSymbol->setSectionSize(LC);
                    }
                    
                    tempSymbol = symbolExists(tokenList[i].name);
                    if (tempSymbol == nullptr){
                        // if it is a new section, add it to the symbol table and set the LC to 0
                        symbolTable.push_back(Symbol(tokenList[i].name, symbolTable.size(), LC, false, true, symbolTable.size(), true));
                        LC = 0;
                        currentSectionNumber = symbolTable.size() - 1;
                    }
                    else {
                        // if the section already exists, set the LC to the size of the section
                        LC = tempSymbol->sectionSize;
                        currentSectionNumber = tempSymbol->number;
                    }
                    currentSection = tokenList[i].name;
                }
                else{
                    cout << "Error: Invalid section name" << endl;
                    exit(EXIT_FAILURE);
                }
                break;

            case EQU:
                i++; // get symbol name
                if (tokenList[i].value == NAME){
                    tempSymbol = symbolExists(tokenList[i].name);
                    tempSymbolName = tokenList[i].name;
                    if (tempSymbol == nullptr){
                        i++; // comma
                        i++; // get symbol value
                        if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
                            symbolTable.push_back(Symbol(tempSymbolName, currentSectionNumber, LC, false, false, symbolTable.size(), true));
                            tempSymbol = &symbolTable[symbolTable.size() - 1];
                            if (tokenList[i].value == DEC_LITERAL){
                                tempSymbol->setValue(stoi(tokenList[i].name));
                            }
                            else{
                                tempSymbol->setValue(hexToInt(tokenList[i].name));
                            }
                        }
                        else{
                            cout << "Error: Invalid symbol value" << endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                    else {
                        i++; // comma
                        i++; // get symbol value
                        if (tokenList[i].value == DEC_LITERAL || tokenList[i].value == HEX_LITERAL){
                            if (tokenList[i].value == DEC_LITERAL){
                                tempSymbol->setValue(stoi(tokenList[i].name));
                            }
                            else{
                                tempSymbol->setValue(hexToInt(tokenList[i].name));
                            }
                            tempSymbol->is_defined = true;
                        }
                        else{
                            cout << "Error: Invalid symbol value" << endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else{
                    cout << "Error: Invalid symbol name" << endl;
                    exit(EXIT_FAILURE);
                }
                break;

            case LABEL:
                // label is in format: name:
                tempSymbolName = tokenList[i].name.substr(0, tokenList[i].name.length() - 1);
                tempSymbol = symbolExists(tempSymbolName);
                if (tempSymbol == nullptr){
                    symbolTable.push_back(Symbol(tempSymbolName, currentSectionNumber, LC, false, false, symbolTable.size(), true));
                }
                else{
                    tempSymbol->is_defined = true;
                    tempSymbol->section = currentSectionNumber;
                    tempSymbol->lc_value = LC;
                }
                break;

            case ENDD:
                // set the size of the last section
                tempSymbol = symbolExists(currentSection);
                tempSymbol->setSectionSize(LC);
                cout << "Symbol table:" << endl;
                for (int i = 0; i < symbolTable.size(); i++){
                    cout << symbolTable[i].to_string() << endl;
                }
                return EXIT_SUCCESS;
            
            default:
                break;
        }
    }
    // print the symbol table:
    cout << "Symbol table:" << endl;
    for (int i = 0; i < symbolTable.size(); i++){
        cout << symbolTable[i].to_string() << endl;
    }
    return EXIT_SUCCESS;
}