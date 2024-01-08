#include "symbol.h"
#include <iostream>


Symbol::Symbol(std::string name, int section, int lc_value, bool is_global, bool is_section, int number, bool is_defined, int sectionSize){
    this->name = name;
    this->section = section;
    this->lc_value = lc_value;
    this->is_global = is_global;
    this->is_section = is_section;
    this->number = number;
    this->is_defined = is_defined;
    this->sectionSize = sectionSize;
}

Symbol::Symbol(std::string name, bool is_section){
    this->name = name;
    this->is_section = is_section;
}

std::string Symbol::to_string(){
    // write out the symbol values in one line (for debugging purposes)
    std::string symbolString = "Name: " + this->name + ", Section: " + std::to_string(this->section) + ", LC value: " + std::to_string(this->lc_value) + ", Is global: " + std::to_string(this->is_global) + ", Is section: " + std::to_string(this->is_section) + ", Number: " + std::to_string(this->number) + ", Defined: " + std::to_string(this->is_defined);
    // write out the same thing but with value as well
    symbolString += ", Value: " + std::to_string(this->value);
    // write out the same thing but with section size as well
    symbolString += ", Section size: " + std::to_string(this->sectionSize);
    return symbolString;
}

void Symbol::setSectionSize(int size){
    this->sectionSize = size;
}

void Symbol::setValue(int value){
    this->value = value;
}

void Symbol::set1ByteOfMachineCode(int location, int value){
    // Set the value at the location in the machine code array
    machineCode.push_back(value);
}

void Symbol::set2BytesOfMachineCode(int location, int value){
    int higher = (int)(value / 0x100);
    int lower = (int)(value % 0x100);

    // Set the value at the location in the machine code array
    machineCode.push_back(value);
    // Set the value at the location + 1 in the machine code array
    machineCode.push_back(value >> 8);
}

void Symbol::set3BytesOfMachineCode(int location, int value){
    // Set the value at the location in the machine code array
    machineCode.push_back(value / 0x10000);
    // Set the value at the location + 1 in the machine code array
    machineCode.push_back(value / 0x100 % 0x100);
    // Set the value at the location + 2 in the machine code array
    machineCode.push_back(value % 0x100);
}

void Symbol::set5BytesOfMachineCode(int location, int value){
    int valueArray[5] = {0, 0, 0, 0, 0};
    valueArray[0] = value / 0x100000000;
    valueArray[1] = value / 0x1000000 % 0x100;
    valueArray[2] = value / 0x10000 % 0x100;
    valueArray[3] = value / 0x100 % 0x100;
    valueArray[4] = value % 0x100;

    for(int i = 0; i < 5; i++){
        machineCode.push_back(valueArray[i]);
    }
}

void Symbol::set4BytesOfMachineCode(int location, int value){
    // // Set the value at the location in the machine code array
    machineCode.push_back(value / 0x1000000);
    // std::cout << "FIRST WE STORE: " << value / 0x1000000 << std::endl;
    // // Set the value at the location + 1 in the machine code array
    machineCode.push_back(value / 0x10000 % 0x100);
    // std::cout << "SECOND WE STORE: " << value / 0x10000 % 0x100 << std::endl;
    // // Set the value at the location + 2 in the machine code array
    machineCode.push_back(value / 0x100 % 0x100);
    // std::cout << "THIRD WE STORE: " << value / 0x100 % 0x100 << std::endl;
    // // Set the value at the location + 3 in the machine code array
    machineCode.push_back(value % 0x100);
    // std::cout << "FOURTH WE STORE: " << value % 0x100 << std::endl;

}

void Symbol::setZeroValueBytes(int location, int numOfBytes){
    for(int i = 0; i < numOfBytes; i++){
        machineCode.push_back(0);
    }
}

Symbol::~Symbol(){
}