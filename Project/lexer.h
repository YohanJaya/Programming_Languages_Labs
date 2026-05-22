#pragma once

#include "token.h"

#include <set>
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string &source);

    std::vector<Token> tokenize();

private:
    const std::string src;
    size_t pos;
    int line;

    static const std::set<std::string> keywords;

    bool atEnd() const;
    char peek(int offset = 0) const;
    char advance();
    bool isLetter(char c) const;
    bool isDigit(char c) const;
    bool isOperatorSymbol(char c) const;
    Token scanIdentifier();
    Token scanInteger();
    Token scanOperator();
    Token scanString();
    void skipSpaces();
    void skipComment();
};
