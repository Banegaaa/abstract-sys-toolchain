#pragma once
#include <string>
using namespace std;

class LiteralLD {
    public:
    int value;
    int location;
    int size;
    int index;
    int symbolIndex;
    string sectionName;

    LiteralLD(int value, int location, int size, int index, int symbolIndex, string sectionName){
        this->value = value;
        this->location = location;
        this->size = size;
        this->index = index;
        this->symbolIndex = symbolIndex;
        this->sectionName = sectionName;
    }
};