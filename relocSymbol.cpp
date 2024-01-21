#include "relocSymbol.h"

RelocationSymbol::RelocationSymbol(int symbolIndex, int location, int literalIndex) {
    this->symbolIndex = symbolIndex;
    this->location = location;
    this->literalIndex = literalIndex;
}