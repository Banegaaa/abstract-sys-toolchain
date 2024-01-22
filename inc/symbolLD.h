#include <string>
#include <vector>
#include "literalLD.h"
#include "relocSymbolLD.h"
using namespace std;


// We will use this class as an intermediate between parser and linker.
// If there is no need for a separate class, we will revert to using symbol from assembler.

class SymbolLD {
    public:
    int id;
    string name;
    string bind; // local or global
    string type; // section or variable

    int section;

    // section data
    string sectionName;
    int sectionSize;
    int baseAddress;

    vector<RelocSymbolLD*> relocationTable;
    vector<LiteralLD*> literalTable;  // bazen literala - on pocinje na onom locCounteru na kom se sekcija zavrsava

};