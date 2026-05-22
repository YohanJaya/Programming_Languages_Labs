#include "cse_machine.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cstdlib>

CSEMachine::CSEMachine(Node *root) : stRoot(root) {}

// ---------------------------------------------------------------------------
// Helpers for parsing leaf labels of the form  <ID:x>  <INT:3>  <STR:'hi'>
//                                               <true> <false> <nil> <dummy> <Y*>
// ---------------------------------------------------------------------------
static bool startsWith(const std::string &s, const std::string &p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

CSElement CSEMachine::makeName(Node *node) {
    CSElement e;
    e.type = CSEType::NAME;
    const std::string &l = node->label;

    if (startsWith(l, "<ID:")) {
        e.name = l.substr(4, l.size() - 5); // strip "<ID:" and ">"
        e.kind = "id";
    } else if (startsWith(l, "<INT:")) {
        e.name = l.substr(5, l.size() - 6);
        e.kind = "int";
    } else if (startsWith(l, "<STR:")) {
        // strip "<STR:'" ... "'>"
        std::string inner = l.substr(6, l.size() - 8);
        e.name = inner;
        e.kind = "str";
    } else if (l == "<true>") {
        e.name = "true"; e.kind = "bool";
    } else if (l == "<false>") {
        e.name = "false"; e.kind = "bool";
    } else if (l == "<nil>") {
        // nil is the empty tuple.
        e.type = CSEType::NAME;
        e.name = "nil";
        e.kind = "tuple";
        e.tupleElems = std::make_shared<std::vector<CSElement>>();
    } else if (l == "<dummy>") {
        e.name = "dummy"; e.kind = "dummy";
    } else if (l == "<Y*>") {
        e.name = "Y*"; e.kind = "ystar";
    } else {
        // bare operator / function name (e.g. Print, +, eq used as a rator)
        e.name = l; e.kind = "id";
    }
    return e;
}

// ---------------------------------------------------------------------------
// Control structure construction.
//
// We walk the standardized tree pre-order. lambda and conditional (->) nodes
// spawn new control structures.
// ---------------------------------------------------------------------------
static const char *BUILTIN_OPS[] = {
    "+","-","*","/","**","neg",
    "or","&","not",
    "gr","ge","ls","le","eq","ne",
    "aug",
    nullptr
};

static bool isOperatorLabel(const std::string &l) {
    for (int i = 0; BUILTIN_OPS[i]; ++i) if (l == BUILTIN_OPS[i]) return true;
    return false;
}

void CSEMachine::buildControlStructures() {
    controlStructures.clear();
    controlStructures.emplace_back(); // delta_0
    preOrder(stRoot, 0);
}

// Collect the names bound by a lambda's variable child (single id, '()', or
// a ',' tuple of ids).
static void collectBound(Node *varNode, std::vector<std::string> &out, bool &multi) {
    multi = false;
    if (!varNode) return;
    if (varNode->label == ",") {
        multi = true;
        for (Node *c = varNode->child; c; c = c->sibling) {
            std::string nm = c->label;
            if (startsWith(nm, "<ID:")) nm = nm.substr(4, nm.size() - 5);
            out.push_back(nm);
        }
    } else if (varNode->label == "()") {
        // no bound variables
    } else {
        std::string nm = varNode->label;
        if (startsWith(nm, "<ID:")) nm = nm.substr(4, nm.size() - 5);
        out.push_back(nm);
    }
}

void CSEMachine::preOrder(Node *node, int currentDelta) {
    if (!node) return;
    const std::string &l = node->label;

    if (l == "lambda") {
        // children: V , body
        Node *V = node->child;
        Node *body = V->sibling;

        // Create a new control structure for the body.
        int newIndex = (int)controlStructures.size();
        controlStructures.emplace_back();

        CSElement lam;
        lam.type = CSEType::LAMBDA;
        lam.control = newIndex;
        collectBound(V, lam.boundVars, lam.multiBound);
        controlStructures[currentDelta].push_back(lam);

        preOrder(body, newIndex);
        return;
    }

    if (l == "->") {
        // conditional: children predicate, then, else
        Node *pred = node->child;
        Node *thenB = pred->sibling;
        Node *elseB = thenB->sibling;

        int thenIndex = (int)controlStructures.size();
        controlStructures.emplace_back();
        int elseIndex = (int)controlStructures.size();
        controlStructures.emplace_back();

        // Push delta_then, delta_else, beta, then predicate (in this order).
        CSElement dThen; dThen.type = CSEType::DELTA; dThen.thenIndex = thenIndex;
        CSElement dElse; dElse.type = CSEType::DELTA; dElse.thenIndex = elseIndex;
        CSElement beta;  beta.type = CSEType::BETA;
        controlStructures[currentDelta].push_back(dThen);
        controlStructures[currentDelta].push_back(dElse);
        controlStructures[currentDelta].push_back(beta);

        // The predicate is evaluated in the current control structure.
        preOrder(pred, currentDelta);

        // then / else branches go into their own control structures.
        preOrder(thenB, thenIndex);
        preOrder(elseB, elseIndex);
        return;
    }

    if (l == "tau") {
        int n = 0;
        for (Node *c = node->child; c; c = c->sibling) n++;
        // Preorder: emit the tau marker first, then the children.
        CSElement tau; tau.type = CSEType::TAU; tau.tauN = n;
        controlStructures[currentDelta].push_back(tau);
        for (Node *c = node->child; c; c = c->sibling) {
            preOrder(c, currentDelta);
        }
        return;
    }

    if (l == "gamma") {
        // Preorder: emit gamma first, then rator and rand.
        CSElement g; g.type = CSEType::GAMMA;
        controlStructures[currentDelta].push_back(g);
        for (Node *c = node->child; c; c = c->sibling) {
            preOrder(c, currentDelta);
        }
        return;
    }

    if (isOperatorLabel(l)) {
        // Preorder: emit the operator first, then its operands.
        CSElement op; op.type = CSEType::OPERATOR; op.name = l;
        controlStructures[currentDelta].push_back(op);
        for (Node *c = node->child; c; c = c->sibling) {
            preOrder(c, currentDelta);
        }
        return;
    }

    // Leaf / name (identifier, literal, builtin name, Y*, etc.)
    if (!node->child) {
        controlStructures[currentDelta].push_back(makeName(node));
        return;
    }

    // Any other internal node: emit the node, then traverse children.
    controlStructures[currentDelta].push_back(makeName(node));
    for (Node *c = node->child; c; c = c->sibling) {
        preOrder(c, currentDelta);
    }
}

// ---------------------------------------------------------------------------
// Value formatting (to match rpal.exe output).
// ---------------------------------------------------------------------------
std::string CSEMachine::valueToString(const CSElement &v) {
    if (v.type == CSEType::NAME) {
        if (v.kind == "int") return v.name;
        if (v.kind == "str") return v.name;
        if (v.kind == "bool") return v.name;
        if (v.kind == "dummy") return "dummy";
        if (v.kind == "tuple") {
            // nil prints as nil; non-empty tuple prints as (a, b, c)
            if (!v.tupleElems || v.tupleElems->empty()) return "nil";
            std::string s = "(";
            for (size_t i = 0; i < v.tupleElems->size(); ++i) {
                if (i) s += ", ";
                s += valueToString((*v.tupleElems)[i]);
            }
            s += ")";
            return s;
        }
        return v.name;
    }
    if (v.type == CSEType::TAU || (v.tupleElems)) {
        if (!v.tupleElems || v.tupleElems->empty()) return "nil";
        std::string s = "(";
        for (size_t i = 0; i < v.tupleElems->size(); ++i) {
            if (i) s += ", ";
            s += valueToString((*v.tupleElems)[i]);
        }
        s += ")";
        return s;
    }
    if (v.type == CSEType::LAMBDA) {
        return "[lambda closure: " + (v.boundVars.empty() ? std::string("") : v.boundVars[0])
               + ": " + std::to_string(v.control) + "]";
    }
    return v.name;
}

// ---------------------------------------------------------------------------
// Operator application.
// ---------------------------------------------------------------------------
static long long toInt(const CSElement &e) {
    return std::stoll(e.name);
}

static CSElement intValue(long long n) {
    CSElement e; e.type = CSEType::NAME; e.kind = "int"; e.name = std::to_string(n);
    return e;
}
static CSElement boolValue(bool b) {
    CSElement e; e.type = CSEType::NAME; e.kind = "bool"; e.name = b ? "true" : "false";
    return e;
}
static CSElement strValue(const std::string &s) {
    CSElement e; e.type = CSEType::NAME; e.kind = "str"; e.name = s;
    return e;
}

CSElement CSEMachine::applyUnary(const std::string &op, const CSElement &rand) {
    if (op == "neg") return intValue(-toInt(rand));
    if (op == "not") {
        bool b = (rand.name == "true");
        return boolValue(!b);
    }
    throw std::runtime_error("Unknown unary operator: " + op);
}

CSElement CSEMachine::applyBinary(const std::string &op, const CSElement &a, const CSElement &b) {
    if (op == "+") return intValue(toInt(a) + toInt(b));
    if (op == "-") return intValue(toInt(a) - toInt(b));
    if (op == "*") return intValue(toInt(a) * toInt(b));
    if (op == "/") {
        long long d = toInt(b);
        if (d == 0) throw std::runtime_error("Division by zero");
        return intValue(toInt(a) / d);
    }
    if (op == "**") {
        long long base = toInt(a), exp = toInt(b), r = 1;
        for (long long i = 0; i < exp; ++i) r *= base;
        return intValue(r);
    }
    if (op == "eq") {
        // works for ints, strings, bools
        if (a.kind == "int" && b.kind == "int") return boolValue(toInt(a) == toInt(b));
        return boolValue(a.name == b.name);
    }
    if (op == "ne") {
        if (a.kind == "int" && b.kind == "int") return boolValue(toInt(a) != toInt(b));
        return boolValue(a.name != b.name);
    }
    if (op == "gr") return boolValue(toInt(a) > toInt(b));
    if (op == "ge") return boolValue(toInt(a) >= toInt(b));
    if (op == "ls") return boolValue(toInt(a) < toInt(b));
    if (op == "le") return boolValue(toInt(a) <= toInt(b));
    if (op == "or")  return boolValue((a.name == "true") || (b.name == "true"));
    if (op == "&")   return boolValue((a.name == "true") && (b.name == "true"));
    if (op == "aug") {
        // a is a tuple; b is appended. If a is not a tuple (e.g. nil represented
        // elsewhere), treat it as a singleton sequence.
        CSElement res;
        res.type = CSEType::NAME; res.kind = "tuple";
        res.tupleElems = std::make_shared<std::vector<CSElement>>();
        if (a.kind == "tuple") {
            if (a.tupleElems) *res.tupleElems = *a.tupleElems;
        } else {
            res.tupleElems->push_back(a);
        }
        res.tupleElems->push_back(b);
        return res;
    }
    throw std::runtime_error("Unknown binary operator: " + op);
}

// ---------------------------------------------------------------------------
// The CSE machine evaluation loop.
// ---------------------------------------------------------------------------
void CSEMachine::run() {
    buildControlStructures();
    evaluate();
}

void CSEMachine::evaluate() {
    // Control and Stack are vectors used as stacks (back = top).
    std::vector<CSElement> control;
    std::vector<CSElement> stack;

    // Primitive environment e_0.
    auto e0 = std::make_shared<Environment>(envCounter++, nullptr);
    std::shared_ptr<Environment> currentEnv = e0;

    // Each env marker on the stack remembers the environment that was current
    // BEFORE the call (so we can restore it on exit), as well as the new env.
    // We store the new env in `.env` and the env to restore in a parallel field
    // by reusing tupleElems? No — use a dedicated map keyed by marker id.
    // Simpler: store restore-env pointer inside the marker via a side stack.
    std::vector<std::shared_ptr<Environment>> envStack; // restore points

    // Initialise: control = [ env0marker, delta_0... ], stack = [ env0marker ].
    CSElement m0; m0.type = CSEType::ENV_MARKER; m0.envIndex = e0->index; m0.env = e0;
    control.push_back(m0);
    stack.push_back(m0);
    envStack.push_back(e0);

    // Load a control structure onto the live control stack. Control structures
    // are stored in preorder. We pop from the back, so pushing them in forward
    // order leaves the LAST element on top (which is evaluated first), giving
    // the correct operand-before-operator evaluation order.
    auto loadControl = [&](int index) {
        auto &cs = controlStructures[index];
        for (auto &el : cs) control.push_back(el);
    };

    auto isBuiltinName = [](const std::string &n) {
        return n == "Print" || n == "print" || n == "Stem" || n == "Stern" ||
               n == "Conc" || n == "conc" || n == "Order" || n == "Isinteger" ||
               n == "Istruthvalue" || n == "Isstring" || n == "Istuple" ||
               n == "Isfunction" || n == "Isdummy" || n == "ItoS" || n == "Null" ||
               startsWith(n, "__Conc1__");
    };

    // Load the body of delta_0 on top of the initial environment marker.
    loadControl(0);

    while (!control.empty()) {
        CSElement c = control.back();
        control.pop_back();

        switch (c.type) {

        case CSEType::NAME: {
            if (c.kind == "id") {
                CSElement val;
                if (currentEnv->lookup(c.name, val)) {
                    stack.push_back(val);
                } else {
                    // Unbound name: treat as a built-in / free function name.
                    stack.push_back(c);
                }
            } else {
                stack.push_back(c);
            }
            break;
        }

        case CSEType::OPERATOR: {
            if (c.name == "neg" || c.name == "not") {
                CSElement rand = stack.back(); stack.pop_back();
                stack.push_back(applyUnary(c.name, rand));
            } else {
                CSElement a = stack.back(); stack.pop_back();
                CSElement b = stack.back(); stack.pop_back();
                stack.push_back(applyBinary(c.name, a, b));
            }
            break;
        }

        case CSEType::LAMBDA: {
            // Rule 2: stack a closure capturing the current environment.
            CSElement clo = c;
            clo.env = currentEnv;
            stack.push_back(clo);
            break;
        }

        case CSEType::GAMMA: {
            CSElement rator = stack.back(); stack.pop_back();

            if (rator.type == CSEType::LAMBDA) {
                // Rule 4 / 11: apply closure -> new environment.
                CSElement rand = stack.back(); stack.pop_back();
                auto newEnv = std::make_shared<Environment>(envCounter++, rator.env);

                if (rator.multiBound) {
                    if (rand.tupleElems) {
                        for (size_t i = 0; i < rator.boundVars.size(); ++i) {
                            CSElement v = (i < rand.tupleElems->size())
                                          ? (*rand.tupleElems)[i] : CSElement();
                            newEnv->bindings[rator.boundVars[i]] = v;
                        }
                    }
                } else if (!rator.boundVars.empty()) {
                    newEnv->bindings[rator.boundVars[0]] = rand;
                }

                CSElement marker; marker.type = CSEType::ENV_MARKER;
                marker.envIndex = newEnv->index; marker.env = newEnv;

                control.push_back(marker);
                loadControl(rator.control);
                stack.push_back(marker);

                envStack.push_back(currentEnv); // remember where to return
                currentEnv = newEnv;
            }
            else if (rator.type == CSEType::NAME && rator.kind == "ystar") {
                // Rule 12: Y* applied to a lambda -> eta.
                CSElement lam = stack.back(); stack.pop_back();
                CSElement eta;
                eta.type = CSEType::ETA;
                eta.control = lam.control;
                eta.boundVars = lam.boundVars;
                eta.multiBound = lam.multiBound;
                eta.env = lam.env;
                stack.push_back(eta);
            }
            else if (rator.type == CSEType::ETA) {
                // Rule 13: applying eta (recursion). Standard transformation:
                //   Control: ... gamma          Stack: ... rand  eta
                // becomes
                //   Control: ... gamma gamma     Stack: ... rand  eta  lambda
                // The first gamma applies lambda to eta (binding the recursive
                // name to eta), producing the real (inner) closure; the second
                // gamma then applies that closure to rand.
                CSElement lam;
                lam.type = CSEType::LAMBDA;
                lam.control = rator.control;
                lam.boundVars = rator.boundVars;
                lam.multiBound = rator.multiBound;
                lam.env = rator.env;

                // rand is currently on top of the stack (eta was popped as rator).
                control.push_back(c); // gamma (applies inner closure to rand)
                control.push_back(c); // gamma (applies lambda to eta)
                stack.push_back(rator); // eta  -> argument that binds the rec name
                stack.push_back(lam);   // lambda -> rator for the first gamma
            }
            else if (rator.type == CSEType::NAME && rator.kind == "tuple") {
                // Tuple selection: t i.
                CSElement idx = stack.back(); stack.pop_back();
                long long i = toInt(idx);
                if (!rator.tupleElems || i < 1 || i > (long long)rator.tupleElems->size())
                    throw std::runtime_error("Tuple index out of range");
                stack.push_back((*rator.tupleElems)[i - 1]);
            }
            else if (rator.type == CSEType::NAME &&
                     (isBuiltinName(rator.name) || rator.name == "Conc" || rator.name == "conc")) {
                applyBuiltin(rator.name, stack);
            }
            else {
                throw std::runtime_error("Cannot apply non-function value: '"
                                         + valueToString(rator) + "'");
            }
            break;
        }

        case CSEType::TAU: {
            CSElement tup;
            tup.type = CSEType::NAME; tup.kind = "tuple";
            tup.tupleElems = std::make_shared<std::vector<CSElement>>();
            tup.tupleElems->resize(c.tauN);
            // Elements were pushed e_n .. e_1 (e_1 on top), so the first value
            // popped is e_1 which belongs at index 0.
            for (int i = 0; i < c.tauN; ++i) {
                (*tup.tupleElems)[i] = stack.back();
                stack.pop_back();
            }
            stack.push_back(tup);
            break;
        }

        case CSEType::BETA: {
            // Conditional. The boolean is on the stack; the two delta refs were
            // pushed on the control just before beta as: dThen, dElse, beta.
            // So control top now is dElse, then dThen.
            CSElement boolVal = stack.back(); stack.pop_back();
            CSElement dElse = control.back(); control.pop_back();
            CSElement dThen = control.back(); control.pop_back();
            int chosen = (boolVal.name == "true") ? dThen.thenIndex : dElse.thenIndex;
            loadControl(chosen);
            break;
        }

        case CSEType::DELTA: {
            loadControl(c.thenIndex);
            break;
        }

        case CSEType::ENV_MARKER: {
            // Rule 5: exit environment. The result is on top of the stack and
            // the matching marker is directly beneath it.
            CSElement result = stack.back(); stack.pop_back();
            // Pop the marker (should be on top now).
            if (!stack.empty() && stack.back().type == CSEType::ENV_MARKER) {
                stack.pop_back();
            }
            stack.push_back(result);

            // Restore the environment that was current before this call.
            if (!envStack.empty()) {
                currentEnv = envStack.back();
                envStack.pop_back();
            }
            break;
        }

        default:
            throw std::runtime_error("CSE: unhandled control element");
        }
    }
}

// ---------------------------------------------------------------------------
// Built-in functions (matching rpal.exe).
// ---------------------------------------------------------------------------
bool CSEMachine::applyBuiltin(const std::string &fn, std::vector<CSElement> &stack) {
    CSElement arg = stack.back(); stack.pop_back();

    if (fn == "Print" || fn == "print") {
        std::cout << valueToString(arg);
        didPrint = true;
        // Print returns dummy.
        CSElement d; d.type = CSEType::NAME; d.kind = "dummy"; d.name = "dummy";
        stack.push_back(d);
        return true;
    }
    if (fn == "Order") {
        long long n = arg.tupleElems ? (long long)arg.tupleElems->size() : 0;
        stack.push_back(intValue(n));
        return true;
    }
    if (fn == "Null") {
        bool isNull = (arg.kind == "tuple" && (!arg.tupleElems || arg.tupleElems->empty()));
        stack.push_back(boolValue(isNull));
        return true;
    }
    if (fn == "Isinteger") { stack.push_back(boolValue(arg.kind == "int")); return true; }
    if (fn == "Istruthvalue") { stack.push_back(boolValue(arg.kind == "bool")); return true; }
    if (fn == "Isstring") { stack.push_back(boolValue(arg.kind == "str")); return true; }
    if (fn == "Istuple") { stack.push_back(boolValue(arg.kind == "tuple")); return true; }
    if (fn == "Isdummy") { stack.push_back(boolValue(arg.kind == "dummy")); return true; }
    if (fn == "Isfunction") {
        stack.push_back(boolValue(arg.type == CSEType::LAMBDA));
        return true;
    }
    if (fn == "Stem") {
        // first character of a string
        std::string s = arg.name;
        stack.push_back(strValue(s.empty() ? "" : s.substr(0, 1)));
        return true;
    }
    if (fn == "Stern") {
        // all but the first character
        std::string s = arg.name;
        stack.push_back(strValue(s.size() <= 1 ? "" : s.substr(1)));
        return true;
    }
    if (fn == "ItoS") {
        // integer to string
        stack.push_back(strValue(arg.name));
        return true;
    }
    if (fn == "Conc" || fn == "conc") {
        // Conc is curried: Conc s1 s2. First application returns a partial.
        // We model it by pushing back a special partial closure represented as
        // a tuple-tagged name. Simpler: handle via a partial built-in marker.
        CSElement partial;
        partial.type = CSEType::NAME;
        partial.kind = "id";
        partial.name = "__Conc1__" + arg.name; // encode first string
        stack.push_back(partial);
        return true;
    }
    if (startsWith(fn, "__Conc1__")) {
        std::string first = fn.substr(std::string("__Conc1__").size());
        stack.push_back(strValue(first + arg.name));
        return true;
    }
    throw std::runtime_error("Unknown built-in: " + fn);
}
