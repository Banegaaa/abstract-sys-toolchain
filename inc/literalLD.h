#pragma once
#include <string>
using namespace std;

class LiteralLD {
    public:
    string value;
    int location;
    int size;
    int index;
    int symbolIndex;
    string sectionName;

    LiteralLD(string value, int location, int size, int index, int symbolIndex, string sectionName){
        this->value = value;
        this->location = location;
        this->size = size;
        this->index = index;
        this->symbolIndex = symbolIndex;
        this->sectionName = sectionName;
    }
};