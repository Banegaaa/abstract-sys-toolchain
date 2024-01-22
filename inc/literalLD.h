#pragma once

class LiteralLD {
    public:
    int value;
    int location;
    int size;

    LiteralLD(int value, int location, int size){
        this->value = value;
        this->location = location;
        this->size = size;
    }
};