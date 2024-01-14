#include <string>
#include <vector>
#include "literal.h"
using namespace std;

class Symbol {
    public:
    std::string name;
    int section;
    int lc_value;
    bool is_global;
    bool is_section;
    int number; // redni broj
    int value;
    int sectionSize;
    bool is_defined;

    std::vector<int> machineCode;
    int machineCodeSize;
    vector<Literal*> literals;


    Symbol(std::string name, int section, int lc_value, bool is_global, bool is_section, int number, bool is_defined, int sectionSize=0);
    Symbol(std::string name, bool is_section, int sectionSize); // For sections

    // SETTERS
    void setValue(int value);
    void setSectionSize(int size);
    void setSectionNumber(int number);

    // LITERALS
    void addLiteral(Literal* literal);
    Literal* literalExists(int value);

    // MACHINE CODE
    void allocateMachineCodeMemory(int bytes);
    void allocateAndPopulateMachineCodeMemory(int bytes, int value);
    void set1ByteOfMachineCode(int location, int value);
    void set4BytesOfMachineCode(int location, int value);
    void setZeroValueBytes(int location, int numOfBytes);

    std::string to_string();

    ~Symbol();

};