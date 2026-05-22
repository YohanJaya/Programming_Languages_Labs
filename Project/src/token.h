#pragma once

#include <string>

enum class TokenType {
    IDENTIFIER,
    INTEGER,
    STRING,
    KEYWORD,
    OPERATOR,
    L_PAREN,
    R_PAREN,
    SEMICOLON,
    COMMA,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;

    Token(TokenType t = TokenType::END_OF_FILE, const std::string &v = "", int l = 0)
        : type(t), value(v), line(l) {}
};
