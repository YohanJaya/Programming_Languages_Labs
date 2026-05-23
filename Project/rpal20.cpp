

#include "lexer.h"
#include "parser.h"
#include "standardizer.h"
#include "cse_machine.h"
#include "node.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Read the entire contents of a file into a string.
static std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open input file: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Usage:
//   ./rpal20 file_name        -> run the program (CSE machine output)
//   ./rpal20 -ast file_name   -> print the Abstract Syntax Tree
//   ./rpal20 -st  file_name   -> print the Standardized Tree
int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-ast|-st] file_name\n";
        return 1;
    }

    bool printAst = false;
    bool printSt = false;
    std::string filename;

    // Parse command-line flags
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-ast") printAst = true;
        else if (a == "-st") printSt = true;
        else filename = a;
    }

    if (filename.empty()) {
        std::cerr << "Error: no input file specified.\n";
        return 1;
    }

    try {
        std::string source = readFile(filename);

        // 1. Lexical analysis: break source text into tokens
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        // 2. Parsing -> AST: build the Abstract Syntax Tree from tokens
        Parser parser(tokens);
        Node *ast = parser.parse();

        if (printAst) {
            printAST(ast);
            deleteTree(ast);
            return 0;
        }

        // 3. Standardize -> ST: transform AST into the standard form for the CSE machine
        Standardizer standardizer;
        Node *st = standardizer.standardize(ast);

        if (printSt) {
            printAST(st);
            return 0;
        }

        // 4. CSE machine evaluation: execute the standardized tree
        CSEMachine machine(st);
        machine.run();
        /* rpal.exe appends a newline after Print output;
           a program that prints nothing produces no output at all. */
        if (machine.producedOutput()) {
            std::cout << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}