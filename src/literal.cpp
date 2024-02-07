#include "../inc/literal.h"

Literal::Literal(std::string value, int location, int size, int index, int symbolIndex){
    this->value = value;
    this->location = location;
    this->size = size;
    this->index = index;
    this->symbolIndex = symbolIndex;
}
