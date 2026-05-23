

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

/* Convert a tree leaf node into a CSElement (the runtime value type).
   Leaf labels are strings like "<ID:x>", "<INT:3>", etc. — we strip the tags. */
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
    controlStructures.emplace_back(); // delta_0 is the top-level control structure
    preOrder(stRoot, 0);
}

// Collect the names bound by a lambda's variable child (single id, '()', or
// a ',' tuple of ids).
static void collectBound(Node *varNode, std::vector<std::string> &out, bool &multi) {
    multi = false;
    if (!varNode) return;
    if (varNode->label == ",") {
        multi = true;  // tuple pattern: lambda (x, y) -> ...
        for (Node *c = varNode->child; c; c = c->sibling) {
            std::string nm = c->label;
            if (startsWith(nm, "<ID:")) nm = nm.substr(4, nm.size() - 5);
            out.push_back(nm);
        }
    } else if (varNode->label == "()") {
        // no bound variables (empty parameter list)
    } else {
        std::string nm = varNode->label;
        if (startsWith(nm, "<ID:")) nm = nm.substr(4, nm.size() - 5);
        out.push_back(nm);
    }
}

/* Walk the standardized AST in pre-order and flatten it into numbered
   control structures (arrays of CSElements). Each lambda/conditional
   body gets its own control structure index (like a basic block). */
void CSEMachine::preOrder(Node *node, int currentDelta) {
    if (!node) return;
    const std::string &l = node->label;

    if (l == "lambda") {
        // children: V , body
        Node *V = node->child;
        Node *body = V->sibling;

        // Create a new control structure for the lambda body.
        int newIndex = (int)controlStructures.size();
        controlStructures.emplace_back();

        CSElement lam;
        lam.type = CSEType::LAMBDA;
        lam.control = newIndex;  // index of the body's control structure
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

        /* Push delta_then, delta_else, beta (branch instruction), then predicate.
           At runtime: evaluate predicate, beta checks the bool and picks a branch. */
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
        /* 'aug' appends an element to a tuple.
           If a is nil/empty tuple, start a new tuple; otherwise copy and extend. */
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


void CSEMachine::run() {
    buildControlStructures();
    evaluate();
}

void CSEMachine::evaluate() {
    // Control and Stack are vectors used as stacks (back = top).
    std::vector<CSElement> control;
    std::vector<CSElement> stack;

    // Primitive environment e_0 (the global/empty environment).
    auto e0 = std::make_shared<Environment>(envCounter++, nullptr);
    std::shared_ptr<Environment> currentEnv = e0;

    /* envStack tracks the environment to restore after each function call returns.
       When we enter a closure, we push the old env here and restore it on exit. */
    std::vector<std::shared_ptr<Environment>> envStack;

    // Initialise: control = [ env0marker, delta_0... ], stack = [ env0marker ].
    CSElement m0; m0.type = CSEType::ENV_MARKER; m0.envIndex = e0->index; m0.env = e0;
    control.push_back(m0);
    stack.push_back(m0);
    envStack.push_back(e0);

    /* loadControl copies a control structure (by index) onto the live control vector.
       Elements are pushed in forward order so the LAST element ends up on top,
       which is evaluated first — this matches the pre-order encoding. */
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

    /* Main CSE machine loop. Each iteration pops one element from the control
       and handles it according to the CSE machine rules. */
    while (!control.empty()) {
        CSElement c = control.back();
        control.pop_back();

        switch (c.type) {

        case CSEType::NAME: {
            if (c.kind == "id") {
                CSElement val;
                if (currentEnv->lookup(c.name, val)) {
                    stack.push_back(val);  // found in environment — push its value
                } else {
                    // Unbound name: treat as a built-in / free function name.
                    stack.push_back(c);
                }
            } else {
                stack.push_back(c);  // literals (int, str, bool) go straight to stack
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
            // Rule 2: stack a closure — capture the lambda + current environment.
            CSElement clo = c;
            clo.env = currentEnv;  // closure = code + env snapshot
            stack.push_back(clo);
            break;
        }

        case CSEType::GAMMA: {
            CSElement rator = stack.back(); stack.pop_back();  // the function

            if (rator.type == CSEType::LAMBDA) {
                // Rule 4 / 11: apply closure -> create a new environment.
                CSElement rand = stack.back(); stack.pop_back();  // the argument
                auto newEnv = std::make_shared<Environment>(envCounter++, rator.env);

                /* Bind parameter(s) in the new environment.
                   multiBound = true means the parameter is a tuple pattern (x, y). */
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

                // Push an environment marker so we know when this call frame ends.
                CSElement marker; marker.type = CSEType::ENV_MARKER;
                marker.envIndex = newEnv->index; marker.env = newEnv;

                control.push_back(marker);
                loadControl(rator.control);  // load the lambda body
                stack.push_back(marker);

                envStack.push_back(currentEnv); // save caller's env
                currentEnv = newEnv;
            }
            else if (rator.type == CSEType::NAME && rator.kind == "ystar") {
                // Rule 12: Y* applied to a lambda -> eta (lazy self-application).
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
                /* Eta rule for recursion: applying an eta forces the fixed-point expansion.
                   This re-applies the lambda to itself (via Y*) before applying to the argument. */
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
                // Tuple selection: t i  (1-based index).
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
            /* TAU: pop tauN values from the stack and bundle them into a tuple.
               Values were pushed in left-to-right order, so index 0 is on top. */
            CSElement tup;
            tup.type = CSEType::NAME; tup.kind = "tuple";
            tup.tupleElems = std::make_shared<std::vector<CSElement>>();
            tup.tupleElems->resize(c.tauN);
            for (int i = 0; i < c.tauN; ++i) {
                (*tup.tupleElems)[i] = stack.back();
                stack.pop_back();
            }
            stack.push_back(tup);
            break;
        }

        case CSEType::BETA: {
            /* BETA: conditional branch. Pop the boolean result, then pick
               the delta_then or delta_else control structure to load next. */
            CSElement boolVal = stack.back(); stack.pop_back();
            CSElement dElse = control.back(); control.pop_back();
            CSElement dThen = control.back(); control.pop_back();
            int chosen = (boolVal.name == "true") ? dThen.thenIndex : dElse.thenIndex;
            loadControl(chosen);
            break;
        }

        case CSEType::DELTA: {
            loadControl(c.thenIndex);  // should not normally be reached directly
            break;
        }

        case CSEType::ENV_MARKER: {
            /* ENV_MARKER: function call is done. Save the return value,
               remove the marker from the stack, restore the caller's environment. */
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


bool CSEMachine::applyBuiltin(const std::string &fn, std::vector<CSElement> &stack) {
    CSElement arg = stack.back(); stack.pop_back();

    if (fn == "Print" || fn == "print") {
        std::cout << valueToString(arg);
        didPrint = true;
        // Print returns dummy (like void in C++).
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
        /* Conc is curried: Conc str1 str2 concatenates.
           First call stores str1 in a tagged partial-application value,
           second call finishes and returns the concatenated string. */
        CSElement partial;
        partial.type = CSEType::NAME;
        partial.kind = "id";
        partial.name = "__Conc1__" + arg.name; // encode first string in the name
        stack.push_back(partial);
        return true;
    }
    if (startsWith(fn, "__Conc1__")) {
        // Second call to Conc: extract stored first string and concatenate.
        std::string first = fn.substr(std::string("__Conc1__").size());
        stack.push_back(strValue(first + arg.name));
        return true;
    }
    throw std::runtime_error("Unknown built-in: " + fn);
}