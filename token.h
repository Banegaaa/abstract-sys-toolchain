#include <string>

struct Token {
    int value;
    std::string name;

    Token(const int& tokenValue, const std::string& tokenName)
    : name(tokenName), value(tokenValue) {}

};
