#pragma once

class RelocSymbolLD {
    public:
    int offset;
    int symbolId;  // id of the SymbolLD

    RelocSymbolLD(int offset, int symbolId){
        this->offset = offset;
        this->symbolId = symbolId;
    }
};
