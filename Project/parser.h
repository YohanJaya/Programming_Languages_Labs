#pragma once

#include "node.h"
#include "token.h"

#include <stack>
#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(const std::vector<Token> &tokens);

    Node *parse();

private:
    const std::vector<Token> tokens;
    size_t idx;
    std::stack<Node *> st;

    const Token &peek() const;
    const Token &peekAt(int offset) const;
    void advance();
    bool isKeyword(const std::string &kw) const;
    bool isOperator(const std::string &op) const;
    void error(const std::string &msg) const;
    void expectKeyword(const std::string &kw);
    void expectOperator(const std::string &op);
    void expect(TokenType type, const std::string &what);
    void buildTree(const std::string &label, int n);
    void pushLeafIdentifier();
    void pushLeafInteger();
    void pushLeafString();

    void E();
    void Ew();
    void T();
    void Ta();
    void Tc();
    void B();
    void Bt();
    void Bs();
    void Bp();
    void A();
    void At();
    void Af();
    void Ap();
    void R();
    void Rn();
    void D();
    void Da();
    void Dr();
    void Db();
    void Vb();
    void Vl();
};
