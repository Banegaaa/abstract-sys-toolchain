class RelocationSymbol {
    public:
    int symbolIndex;  // we have to use index instead of pointer because of circular dependency
    int location;
    int literalIndex; // -1 if not literal

    RelocationSymbol(int symbolIndex, int location, int literalIndex=-1);
    ~RelocationSymbol();
};