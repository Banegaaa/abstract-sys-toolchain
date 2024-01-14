
class Literal{
    public:
        int value;
        int location; // LC
        int size;
        int index;
        int symbolIndex; // index of the symbol in the symbol table (-1 if it's not a symbol literal)
        Literal(int value, int location, int size, int index, int symbolIndex);

};