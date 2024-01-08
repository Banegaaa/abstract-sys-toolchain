#include <string>
#include <vector>

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


    Symbol(std::string name, int section, int lc_value, bool is_global, bool is_section, int number, bool is_defined, int sectionSize=0);
    Symbol(std::string name, bool is_section, int sectionSize); // For sections

    // Need a function for setting the value
    void setValue(int value);

    // Need a function for setting the section size
    void setSectionSize(int size);
    void setSectionNumber(int number);


    std::string to_string();

    void allocateMachineCodeMemory(int bytes);

    void set1ByteOfMachineCode(int location, int value);
    void set2BytesOfMachineCode(int location, int value);
    void set3BytesOfMachineCode(int location, int value);
    // write declaration for 5 bytes, that has int location and 5 integer array as parameters
    void set5BytesOfMachineCode(int location, int value);

    void set4BytesOfMachineCode(int location, int value);

    void setZeroValueBytes(int location, int numOfBytes);

    ~Symbol();

};