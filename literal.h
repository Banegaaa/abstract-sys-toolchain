
class Literal{
    public:
        int value;
        int location; // LC
        int size;
        int index;
        int symbolIndex;  // index of symbol in the symbol tables

        Literal(int value, int location, int size, int index, int symbolIndex);

};