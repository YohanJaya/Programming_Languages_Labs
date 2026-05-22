#pragma once

#include "node.h"

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <utility>

struct Environment;

enum class CSEType {
    NAME,
    OPERATOR,
    LAMBDA,
    GAMMA,
    TAU,
    BETA,
    DELTA,
    ENV_MARKER,
    ETA
};

struct CSElement {
    CSEType type = CSEType::NAME;
    std::string name;
    std::string kind;
    int control = 0;
    int thenIndex = 0;
    int tauN = 0;
    int envIndex = 0;
    bool multiBound = false;
    std::vector<std::string> boundVars;
    std::shared_ptr<std::vector<CSElement>> tupleElems;
    std::shared_ptr<Environment> env;
};

struct Environment {
    int index;
    std::shared_ptr<Environment> parent;
    std::map<std::string, CSElement> bindings;

    Environment(int idx, std::shared_ptr<Environment> p) : index(idx), parent(std::move(p)) {}

    bool lookup(const std::string &name, CSElement &out) const {
        auto it = bindings.find(name);
        if (it != bindings.end()) {
            out = it->second;
            return true;
        }
        if (parent) {
            return parent->lookup(name, out);
        }
        return false;
    }
};

class CSEMachine {
public:
    explicit CSEMachine(Node *root);

    void run();
    bool producedOutput() const { return didPrint; }

private:
    Node *stRoot;
    std::vector<std::vector<CSElement>> controlStructures;
    bool didPrint = false;
    int envCounter = 0;

    CSElement makeName(Node *node);
    void buildControlStructures();
    void preOrder(Node *node, int currentDelta);
    std::string valueToString(const CSElement &value);
    CSElement applyUnary(const std::string &op, const CSElement &rand);
    CSElement applyBinary(const std::string &op, const CSElement &a, const CSElement &b);
    void evaluate();
    bool applyBuiltin(const std::string &fn, std::vector<CSElement> &stack);
};
