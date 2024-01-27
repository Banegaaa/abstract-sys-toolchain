#include "../inc/symbol.h"
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
    this->value = 0;
}

Symbol::Symbol(std::string name, bool is_section, int sectionSize){
    this->name = name;
    this->is_section = is_section;
    this->sectionSize = sectionSize;
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

void Symbol::setSectionNumber(int number){
    this->section = number;
}

void Symbol::setValue(int value){
    this->value = value;
}

void Symbol::addLiteral(Literal* literal){
    literals.push_back(literal);
}

void Symbol::set1ByteOfMachineCode(int location, int value){
    // Set the value at the location in the machine code array
    machineCode.push_back(value);
}

void Symbol::set4BytesOfMachineCode(int location, int value){
    // // std::cout << "FIRST WE STORE: " << value / 0x1000000 << std::endl;
    // machineCode.push_back(value / 0x1000000);
    // // std::cout << "SECOND WE STORE: " << value / 0x10000 % 0x100 << std::endl;
    // machineCode.push_back(value / 0x10000 % 0x100);
    // // std::cout << "THIRD WE STORE: " << value / 0x100 % 0x100 << std::endl;
    // machineCode.push_back(value / 0x100 % 0x100);
    // // std::cout << "FOURTH WE STORE: " << value % 0x100 << std::endl;
    // machineCode.push_back(value % 0x100);

    machineCode.push_back(value);

    // // for (int i = 0; i < machineCode.size(); i++){
    // //     std::cout << "MACHINE CODE: " << machineCode[i] << std::endl;
    // // }

}

void Symbol::setZeroValueBytes(int location, int numOfBytes){
    for(int i = 0; i < numOfBytes; i++){
        machineCode.push_back(0);
    }
}

Symbol::~Symbol(){
}