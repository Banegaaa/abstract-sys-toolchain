#include "literal.h"

Literal::Literal(int value, int location, int size, Symbol* symbol){
    this->value = value;
    this->location = location;
    this->size = size;
    this->symbol = symbol;
}

Literal::Literal(int value, int location, int size){
    this->value = value;
    this->location = location;
    this->size = size;
    this->symbol = nullptr;
}