#pragma once
#include <string>
#include <vector>
#include "literalLD.h"
#include "relocSymbolLd.h"
using namespace std;


// We will use this class as an intermediate between parser and linker.
// If there is no need for a separate class, we will revert to using symbol from assembler.

class SymbolLD {
    public:
    int id;
    string name;
    int value;
    bool isDefined;
    bool isGlobal; // local or global
    bool isSection; // section or variable

    int location;

    int section;

    // section data
    string sectionName;
    int sectionSize;
    int baseAddress;  // will be filled when placing sections

    vector<RelocSymbolLD*> relocationTable;
    vector<LiteralLD*> literalTable;  // bazen literala - on pocinje na onom locCounteru na kom se sekcija zavrsava

    // Constructor for non-section symbol:
    SymbolLD(int id, string name, int value, bool is_defined, bool is_global, bool is_section, int section, int LC){
        this->id = id;
        this->name = name;
        this->value = value;
        this->isDefined = is_defined;
        this->isGlobal = is_global;
        this->isSection = is_section;
        this->section = section;
        this->location = LC;
    };

    // Constructor for section symbol:
    SymbolLD(int id, string name, bool is_section, int section_size){
        this->id = id;
        this->name = name;
        this->isSection = is_section;
        this->sectionSize = section_size;

        // These symbols will possibly have their relocation and literal tables
    };

};