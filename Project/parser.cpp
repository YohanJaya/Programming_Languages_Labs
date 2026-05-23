

#include "parser.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token> &t) : tokens(t), idx(0) {}

const Token &Parser::peek() const { return tokens[idx]; }

const Token &Parser::peekAt(int o) const {
    size_t p = idx + o;
    if (p >= tokens.size()) return tokens.back(); // EOF
    return tokens[p];
}

void Parser::advance() {
    if (idx < tokens.size() - 1) idx++;
}

bool Parser::isKeyword(const std::string &kw) const {
    return peek().type == TokenType::KEYWORD && peek().value == kw;
}

bool Parser::isOperator(const std::string &op) const {
    return peek().type == TokenType::OPERATOR && peek().value == op;
}

void Parser::error(const std::string &msg) const {
    throw std::runtime_error("Parse error (line " + std::to_string(peek().line)
                             + ", near '" + peek().value + "'): " + msg);
}

void Parser::expectKeyword(const std::string &kw) {
    if (!isKeyword(kw)) error("expected keyword '" + kw + "'");
    advance();
}

void Parser::expectOperator(const std::string &op) {
    if (!isOperator(op)) error("expected operator '" + op + "'");
    advance();
}

void Parser::expect(TokenType t, const std::string &what) {
    if (peek().type != t) error("expected " + what);
    advance();
}

/* Pop n nodes off the parse stack, link them as siblings (left to right),
   attach as children of a new parent node, then push the parent.
   This is the core tree-building step used after every grammar rule. */
void Parser::buildTree(const std::string &label, int n) {
    Node *parent = makeNode(label);
    Node *children = nullptr; // build sibling chain
    for (int i = 0; i < n; ++i) {
        if (st.empty()) error("internal: stack underflow building '" + label + "'");
        Node *top = st.top();
        st.pop();
        top->sibling = children;
        children = top;
    }
    parent->child = children;
    st.push(parent);
}

void Parser::pushLeafIdentifier() {
    st.push(makeNode("<ID:" + peek().value + ">"));
    advance();
}

void Parser::pushLeafInteger() {
    st.push(makeNode("<INT:" + peek().value + ">"));
    advance();
}

void Parser::pushLeafString() {
    st.push(makeNode("<STR:'" + peek().value + "'>"));
    advance();
}

Node *Parser::parse() {
    E();
    if (peek().type != TokenType::END_OF_FILE) {
        error("unexpected token after end of program");
    }
    if (st.size() != 1) {
        throw std::runtime_error("Parse error: malformed program (stack size "
                                 + std::to_string(st.size()) + ")");
    }
    return st.top();
}

// E -> 'let' D 'in' E   => 'let'
//   -> 'fn' Vb+ '.' E   => 'lambda'
//   -> Ew ;
void Parser::E() {
    if (isKeyword("let")) {
        advance();
        D();
        expectKeyword("in");
        E();
        buildTree("let", 2);  // let node has 2 children: definition and body
    } else if (isKeyword("fn")) {
        advance();
        int n = 0;
        // Vb+  (one or more parameter bindings)
        do {
            Vb();
            n++;
        } while (peek().type == TokenType::IDENTIFIER || isOperator("(") ||
                 peek().type == TokenType::L_PAREN);
        expectOperator(".");
        E();
        buildTree("lambda", n + 1); // n Vb children + 1 E child
    } else {
        Ew();
    }
}

// Ew -> T 'where' Dr => 'where'
//    -> T ;
void Parser::Ew() {
    T();
    if (isKeyword("where")) {
        advance();
        Dr();
        buildTree("where", 2);
    }
}

// T -> Ta ( ',' Ta )+ => 'tau'
//   -> Ta ;
void Parser::T() {
    Ta();
    int n = 1;
    while (peek().type == TokenType::COMMA) {
        advance();
        Ta();
        n++;
    }
    // Only wrap in 'tau' if there are multiple comma-separated elements (i.e., a tuple)
    if (n > 1) {
        buildTree("tau", n);
    }
}

// Ta -> Ta 'aug' Tc => 'aug'
//    -> Tc ;
void Parser::Ta() {
    Tc();
    while (isKeyword("aug")) {
        advance();
        Tc();
        buildTree("aug", 2);
    }
}

// Tc -> B '->' Tc '|' Tc => '->'
//    -> B ;
void Parser::Tc() {
    B();
    if (isOperator("->")) {
        advance();
        Tc();
        expectOperator("|");
        Tc();
        buildTree("->", 3);  // 3 children: condition, then-branch, else-branch
    }
}

// B -> B 'or' Bt => 'or'
//   -> Bt ;
void Parser::B() {
    Bt();
    while (isKeyword("or")) {
        advance();
        Bt();
        buildTree("or", 2);
    }
}

// Bt -> Bt '&' Bs => '&'
//    -> Bs ;
void Parser::Bt() {
    Bs();
    while (isOperator("&")) {
        advance();
        Bs();
        buildTree("&", 2);
    }
}

// Bs -> 'not' Bp => 'not'
//    -> Bp ;
void Parser::Bs() {
    if (isKeyword("not")) {
        advance();
        Bp();
        buildTree("not", 1);
    } else {
        Bp();
    }
}

// Bp -> A ('gr'|'>')  A => 'gr'
//    -> A ('ge'|'>=') A => 'ge'
//    -> A ('ls'|'<')  A => 'ls'
//    -> A ('le'|'<=') A => 'le'
//    -> A 'eq' A => 'eq'
//    -> A 'ne' A => 'ne'
//    -> A ;
void Parser::Bp() {
    A();
    if (isKeyword("gr") || isOperator(">")) {
        advance(); A(); buildTree("gr", 2);
    } else if (isKeyword("ge") || isOperator(">=")) {
        advance(); A(); buildTree("ge", 2);
    } else if (isKeyword("ls") || isOperator("<")) {
        advance(); A(); buildTree("ls", 2);
    } else if (isKeyword("le") || isOperator("<=")) {
        advance(); A(); buildTree("le", 2);
    } else if (isKeyword("eq")) {
        advance(); A(); buildTree("eq", 2);
    } else if (isKeyword("ne")) {
        advance(); A(); buildTree("ne", 2);
    }
}

// A -> A '+' At => '+'
//   -> A '-' At => '-'
//   -> '+' At
//   -> '-' At => 'neg'
//   -> At ;
void Parser::A() {
    if (isOperator("+")) {
        advance();
        At();
        // unary plus: no tree node (just the operand)
    } else if (isOperator("-")) {
        advance();
        At();
        buildTree("neg", 1);  // unary minus becomes 'neg' node
    } else {
        At();
    }
    while (isOperator("+") || isOperator("-")) {
        if (isOperator("+")) {
            advance();
            At();
            buildTree("+", 2);
        } else {
            advance();
            At();
            buildTree("-", 2);
        }
    }
}

// At -> At '*' Af => '*'
//    -> At '/' Af => '/'
//    -> Af ;
void Parser::At() {
    Af();
    while (isOperator("*") || isOperator("/")) {
        if (isOperator("*")) {
            advance();
            Af();
            buildTree("*", 2);
        } else {
            advance();
            Af();
            buildTree("/", 2);
        }
    }
}

// Af -> Ap '**' Af => '**'
//    -> Ap ;
void Parser::Af() {
    Ap();
    if (isOperator("**")) {
        advance();
        Af();
        buildTree("**", 2);
    }
}

// Ap -> Ap '@' '<IDENTIFIER>' R => '@'
//    -> R ;
void Parser::Ap() {
    R();
    while (isOperator("@")) {
        advance();
        if (peek().type != TokenType::IDENTIFIER) error("expected identifier after '@'");
        pushLeafIdentifier();
        R();
        buildTree("@", 3);  // infix function application: E1 @ fname E2
    }
}

// R -> R Rn => 'gamma'
//   -> Rn ;
// (Function application by juxtaposition.)
void Parser::R() {
    Rn();
    /* Keep applying as long as the next token can start another Rn.
       Each application creates a 'gamma' (function-apply) node in the AST. */
    while (peek().type == TokenType::IDENTIFIER ||
           peek().type == TokenType::INTEGER ||
           peek().type == TokenType::STRING ||
           isKeyword("true") || isKeyword("false") ||
           isKeyword("nil") || isKeyword("dummy") ||
           peek().type == TokenType::L_PAREN) {
        Rn();
        buildTree("gamma", 2);  // gamma = function application: rator + rand
    }
}

// Rn -> '<IDENTIFIER>'
//    -> '<INTEGER>'
//    -> '<STRING>'
//    -> 'true'  => 'true'
//    -> 'false' => 'false'
//    -> 'nil'   => 'nil'
//    -> '(' E ')'
//    -> 'dummy' => 'dummy' ;
void Parser::Rn() {
    if (peek().type == TokenType::IDENTIFIER) {
        pushLeafIdentifier();
    } else if (peek().type == TokenType::INTEGER) {
        pushLeafInteger();
    } else if (peek().type == TokenType::STRING) {
        pushLeafString();
    } else if (isKeyword("true")) {
        st.push(makeNode("<true>")); advance();
    } else if (isKeyword("false")) {
        st.push(makeNode("<false>")); advance();
    } else if (isKeyword("nil")) {
        st.push(makeNode("<nil>")); advance();
    } else if (peek().type == TokenType::L_PAREN) {
        advance();
        E();
        expect(TokenType::R_PAREN, "')'");
    } else if (isKeyword("dummy")) {
        st.push(makeNode("<dummy>")); advance();
    } else {
        error("expected an operand");
    }
}

// D -> Da 'within' D => 'within'
//   -> Da ;
void Parser::D() {
    Da();
    if (isKeyword("within")) {
        advance();
        D();
        buildTree("within", 2);
    }
}

// Da -> Dr ( 'and' Dr )+ => 'and'
//    -> Dr ;
void Parser::Da() {
    Dr();
    int n = 1;
    while (isKeyword("and")) {
        advance();
        Dr();
        n++;
    }
    if (n > 1) {
        buildTree("and", n);
    }
}

// Dr -> 'rec' Db => 'rec'
//    -> Db ;
void Parser::Dr() {
    if (isKeyword("rec")) {
        advance();
        Db();
        buildTree("rec", 1);  // 'rec' marks a recursive definition
    } else {
        Db();
    }
}

// Db -> Vl '=' E => '='
//    -> '<IDENTIFIER>' Vb+ '=' E => 'fcn_form'
//    -> '(' D ')' ;
void Parser::Db() {
    if (peek().type == TokenType::L_PAREN) {
        advance();
        D();
        expect(TokenType::R_PAREN, "')'");
        return;
    }
    if (peek().type == TokenType::IDENTIFIER) {
        /* Look 1 token ahead to decide which rule applies:
           - If next is '=' or ',', it's a variable list definition: Vl '=' E
           - Otherwise it's a function form: ID Vb+ '=' E */
        const Token &next = peekAt(1);
        bool nextIsEq = (next.type == TokenType::OPERATOR && next.value == "=");
        bool nextIsComma = (next.type == TokenType::COMMA);
        if (nextIsEq || nextIsComma) {
            // Vl '=' E
            Vl();
            expectOperator("=");
            E();
            buildTree("=", 2);
        } else {
            // fcn_form: '<ID>' Vb+ '=' E
            pushLeafIdentifier();
            int n = 0;
            do {
                Vb();
                n++;
            } while (peek().type == TokenType::IDENTIFIER ||
                     peek().type == TokenType::L_PAREN);
            expectOperator("=");
            E();
            buildTree("fcn_form", n + 2); // identifier + n Vb + E
        }
        return;
    }
    error("expected a definition");
}

// Vb -> '<IDENTIFIER>'
//    -> '(' Vl ')'
//    -> '(' ')' => '()' ;
void Parser::Vb() {
    if (peek().type == TokenType::IDENTIFIER) {
        pushLeafIdentifier();
    } else if (peek().type == TokenType::L_PAREN) {
        advance();
        if (peek().type == TokenType::R_PAREN) {
            advance();
            st.push(makeNode("()"));  // empty parameter list
        } else {
            Vl();
            expect(TokenType::R_PAREN, "')'");
        }
    } else {
        error("expected a variable");
    }
}

// Vl -> '<IDENTIFIER>' list ',' => ','?
// (A comma-separated list of identifiers; build a ',' node only if >1.)
void Parser::Vl() {
    if (peek().type != TokenType::IDENTIFIER) error("expected identifier in variable list");
    pushLeafIdentifier();
    int n = 1;
    while (peek().type == TokenType::COMMA) {
        advance();
        if (peek().type != TokenType::IDENTIFIER) error("expected identifier after ','");
        pushLeafIdentifier();
        n++;
    }
    // Only create a ',' node if there are multiple identifiers (tuple pattern)
    if (n > 1) {
        buildTree(",", n);
    }
}
