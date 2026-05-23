

#include "lexer.h"
#include <stdexcept>
#include <iostream>

// Reserved words of RPAL. Anything matching the Identifier pattern that is
// in this set is reclassified as a KEYWORD token.
const std::set<std::string> Lexer::keywords = {
    "let", "in", "fn", "where", "aug", "or", "not",
    "gr", "ge", "ls", "le", "eq", "ne",
    "true", "false", "nil", "dummy",
    "within", "and", "rec"
};

Lexer::Lexer(const std::string &source) : src(source), pos(0), line(1) {}

bool Lexer::atEnd() const { return pos >= src.size(); }

char Lexer::peek(int offset) const {
    size_t p = pos + offset;
    if (p >= src.size()) return '\0';
    return src[p];
}

char Lexer::advance() {
    char c = src[pos++];
    if (c == '\n') line++;  // track line number for error messages
    return c;
}

bool Lexer::isLetter(char c) const {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

// Operator_symbol set from RPAL_Lex.pdf.
bool Lexer::isOperatorSymbol(char c) const {
    switch (c) {
        case '+': case '-': case '*': case '<': case '>': case '&':
        case '.': case '@': case '/': case ':': case '=': case '~':
        case '|': case '$': case '!': case '#': case '%': case '^':
        case '_': case '[': case ']': case '{': case '}': case '"':
        case '`': case '?':
            return true;
        default:
            return false;
    }
}

Token Lexer::scanIdentifier() {
    int startLine = line;
    std::string lexeme;
    // Letter (Letter | Digit | '_')*
    lexeme += advance(); // first char is a letter (guaranteed by caller)
    while (!atEnd() && (isLetter(peek()) || isDigit(peek()) || peek() == '_')) {
        lexeme += advance();
    }
    /* Check if the identifier is a reserved keyword.
       If yes, return a KEYWORD token instead of IDENTIFIER. */
    if (keywords.count(lexeme)) {
        return Token(TokenType::KEYWORD, lexeme, startLine);
    }
    return Token(TokenType::IDENTIFIER, lexeme, startLine);
}

Token Lexer::scanInteger() {
    int startLine = line;
    std::string lexeme;
    while (!atEnd() && isDigit(peek())) {
        lexeme += advance();
    }
    return Token(TokenType::INTEGER, lexeme, startLine);
}

Token Lexer::scanOperator() {
    int startLine = line;
    std::string lexeme;
    // Operator_symbol+  (greedily consume all consecutive operator chars)
    while (!atEnd() && isOperatorSymbol(peek())) {
        lexeme += advance();
    }
    return Token(TokenType::OPERATOR, lexeme, startLine);
}

// String -> ''''  ( escapes | printable )* ''''
// In the lexicon the string delimiter is a single quote character '.
Token Lexer::scanString() {
    int startLine = line;
    std::string lexeme;
    advance(); // consume opening quote '
    while (!atEnd() && peek() != '\'') {
        char c = peek();
        if (c == '\\') {
            // escape sequences: \t \n \\ \'
            advance(); // consume backslash
            char esc = peek();
            switch (esc) {
                case 't': lexeme += '\t'; advance(); break;
                case 'n': lexeme += '\n'; advance(); break;
                case '\\': lexeme += '\\'; advance(); break;
                case '\'': lexeme += '\''; advance(); break;
                default:
                    // keep backslash literally if unknown escape
                    lexeme += '\\';
                    break;
            }
        } else {
            lexeme += advance();
        }
    }
    if (atEnd()) {
        throw std::runtime_error("Lexical error: unterminated string at line "
                                 + std::to_string(startLine));
    }
    advance(); // consume closing quote '
    return Token(TokenType::STRING, lexeme, startLine);
}

void Lexer::skipSpaces() {
    while (!atEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

// Comment -> '//' ( ... )* Eol
void Lexer::skipComment() {
    // consume the two slashes
    advance();
    advance();
    while (!atEnd() && peek() != '\n') {
        advance();
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!atEnd()) {
        char c = peek();

        // Spaces -> <DELETE>
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            skipSpaces();
            continue;
        }

        // Comment -> '//' ... Eol -> <DELETE>
        if (c == '/' && peek(1) == '/') {
            skipComment();
            continue;
        }

        // Identifier / Keyword
        if (isLetter(c)) {
            tokens.push_back(scanIdentifier());
            continue;
        }

        // Integer
        if (isDigit(c)) {
            tokens.push_back(scanInteger());
            continue;
        }

        // String
        if (c == '\'') {
            tokens.push_back(scanString());
            continue;
        }

        // Punctuation
        if (c == '(') { tokens.push_back(Token(TokenType::L_PAREN, "(", line)); advance(); continue; }
        if (c == ')') { tokens.push_back(Token(TokenType::R_PAREN, ")", line)); advance(); continue; }
        if (c == ';') { tokens.push_back(Token(TokenType::SEMICOLON, ";", line)); advance(); continue; }
        if (c == ',') { tokens.push_back(Token(TokenType::COMMA, ",", line)); advance(); continue; }

        // Operator (note: '/' that is not start of comment falls through here)
        if (isOperatorSymbol(c)) {
            tokens.push_back(scanOperator());
            continue;
        }

        throw std::runtime_error("Lexical error: unexpected character '"
                                 + std::string(1, c) + "' at line "
                                 + std::to_string(line));
    }
    // Always append EOF token so the parser knows when input is done
    tokens.push_back(Token(TokenType::END_OF_FILE, "", line));
    return tokens;
}