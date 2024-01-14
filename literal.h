#include "symbol.h"

class Literal{
    public:
        int value;
        int location; // LC
        int size;
        Symbol* symbol;  // Pointer to the symbol that represents this literal

        Literal(int value, int location, int size, Symbol* symbol);
        Literal(int value, int location, int size);
};