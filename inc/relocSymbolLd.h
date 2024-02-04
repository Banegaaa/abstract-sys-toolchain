#pragma once
#include <string>
using namespace std;

class RelocSymbolLD {
    public:
    int offset;
    int symbolIndex;
    int literalIndex;
    string symbolName;
    string sectionName;

    RelocSymbolLD(int offset, int symbolIndex, int literalIndex, string symbolName, string sectionName){
        this->offset = offset;
        this->symbolIndex = symbolIndex;
        this->literalIndex = literalIndex;
        this->symbolName = symbolName;
        this->sectionName = sectionName;
    }
};
