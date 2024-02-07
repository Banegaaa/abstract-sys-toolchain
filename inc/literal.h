#include <string>

class Literal{
    public:
        std::string value;
        int location; // LC
        int size;
        int index;
        int symbolIndex;  // index of symbol in the symbol tables

        Literal(std::string value, int location, int size, int index, int symbolIndex);

};