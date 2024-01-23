#pragma once
#include <string>
using namespace std;

class RelocSymbolLD {
    public:
    int offset;
    int symbolIndex;
    int literalIndex;
    string sectionName;

    RelocSymbolLD(int offset, int symbolIndex, int literalIndex, string sectionName){
        this->offset = offset;
        this->symbolIndex = symbolIndex;
        this->literalIndex = literalIndex;
        this->sectionName = sectionName;
    }
};
