#include "standardizer.h"
#include <vector>
#include <stdexcept>

int Standardizer::childCount(Node *node) const {
    int n = 0;
    for (Node *c = node->child; c; c = c->sibling) n++;
    return n;
}

Node *Standardizer::nthChild(Node *node, int i) const {
    Node *c = node->child;
    while (i-- > 0 && c) c = c->sibling;
    return c;
}

Node *Standardizer::standardize(Node *root) {
    if (root) standardizeNode(root);
    return root;
}

// Post-order traversal: standardize children first, then this node.
void Standardizer::standardizeNode(Node *node) {
    if (!node) return;

    // Standardize all children first (and their subtrees).
    for (Node *c = node->child; c; c = c->sibling) {
        standardizeNode(c);
    }

    const std::string &lbl = node->label;

    if (lbl == "let") {
        // let(=(X,E), P) => gamma(lambda(X,P), E)
        Node *eq = node->child;        // '=' node
        Node *P  = eq->sibling;        // body
        Node *X  = eq->child;          // bound variable
        Node *E  = X->sibling;         // definition value

        Node *lambda = makeNode("lambda");
        lambda->child = X;
        X->sibling = P;
        P->sibling = nullptr;

        node->label = "gamma";
        node->child = lambda;
        lambda->sibling = E;
        E->sibling = nullptr;
    }
    else if (lbl == "where") {
        // where(P, =(X,E)) => gamma(lambda(X,P), E)
        Node *P  = node->child;        // body
        Node *eq = P->sibling;         // '=' node
        Node *X  = eq->child;
        Node *E  = X->sibling;

        Node *lambda = makeNode("lambda");
        lambda->child = X;
        X->sibling = P;
        P->sibling = nullptr;

        node->label = "gamma";
        node->child = lambda;
        lambda->sibling = E;
        E->sibling = nullptr;
    }
    else if (lbl == "fcn_form") {
        // fcn_form(F, V1..Vn, E) => =(F, lambda(V1, lambda(V2, ... E)))
        std::vector<Node *> kids;
        for (Node *c = node->child; c; c = c->sibling) kids.push_back(c);
        // kids[0] = F, kids[1..n] = V1..Vn, kids.back() = E
        Node *F = kids[0];
        Node *E = kids.back();
        int numV = (int)kids.size() - 2; // number of parameters

        // Build nested lambda from the innermost out.
        Node *body = E;
        for (int i = numV; i >= 1; --i) {
            Node *V = kids[i];
            Node *lam = makeNode("lambda");
            lam->child = V;
            V->sibling = body;
            body->sibling = nullptr;
            body = lam;
        }

        node->label = "=";
        node->child = F;
        F->sibling = body;
        body->sibling = nullptr;
    }
    else if (lbl == "lambda") {
        // Multi-parameter lambda(V1..Vn, E) =>
        //   lambda(V1, lambda(V2, ... lambda(Vn, E)))
        std::vector<Node *> kids;
        for (Node *c = node->child; c; c = c->sibling) kids.push_back(c);
        if (kids.size() > 2) {
            Node *E = kids.back();
            int numV = (int)kids.size() - 1;
            // Build nested lambdas for V2..Vn around E, keep V1 at this node.
            Node *body = E;
            for (int i = numV - 1; i >= 1; --i) {
                Node *V = kids[i];
                Node *lam = makeNode("lambda");
                lam->child = V;
                V->sibling = body;
                body->sibling = nullptr;
                body = lam;
            }
            Node *V1 = kids[0];
            node->child = V1;
            V1->sibling = body;
            body->sibling = nullptr;
        }
        // If exactly 2 children (V, E) it is already standard.
    }
    else if (lbl == "within") {
        // within(=(X1,E1), =(X2,E2)) =>
        //   =(X2, gamma(lambda(X1,E2), E1))
        Node *eq1 = node->child;
        Node *eq2 = eq1->sibling;
        Node *X1 = eq1->child;
        Node *E1 = X1->sibling;
        Node *X2 = eq2->child;
        Node *E2 = X2->sibling;

        Node *lambda = makeNode("lambda");
        lambda->child = X1;
        X1->sibling = E2;
        E2->sibling = nullptr;

        Node *gamma = makeNode("gamma");
        gamma->child = lambda;
        lambda->sibling = E1;
        E1->sibling = nullptr;

        node->label = "=";
        node->child = X2;
        X2->sibling = gamma;
        gamma->sibling = nullptr;
    }
    else if (lbl == "@") {
        // @(E1, N, E2) => gamma(gamma(N, E1), E2)
        Node *E1 = node->child;
        Node *N  = E1->sibling;
        Node *E2 = N->sibling;

        Node *innerGamma = makeNode("gamma");
        innerGamma->child = N;
        N->sibling = E1;
        E1->sibling = nullptr;

        node->label = "gamma";
        node->child = innerGamma;
        innerGamma->sibling = E2;
        E2->sibling = nullptr;
    }
    else if (lbl == "and") {
        // and(=(X1,E1) ... =(Xn,En)) =>
        //   =( ,(X1..Xn), tau(E1..En) )
        std::vector<Node *> eqs;
        for (Node *c = node->child; c; c = c->sibling) eqs.push_back(c);

        Node *comma = makeNode(",");
        Node *tau   = makeNode("tau");
        Node *commaTail = nullptr;
        Node *tauTail = nullptr;

        for (Node *eq : eqs) {
            Node *X = eq->child;
            Node *E = X->sibling;
            X->sibling = nullptr;
            E->sibling = nullptr;

            if (!commaTail) { comma->child = X; commaTail = X; }
            else { commaTail->sibling = X; commaTail = X; }

            if (!tauTail) { tau->child = E; tauTail = E; }
            else { tauTail->sibling = E; tauTail = E; }
            // The old '=' node `eq` is now detached; leak is acceptable here.
        }

        node->label = "=";
        node->child = comma;
        comma->sibling = tau;
        tau->sibling = nullptr;
    }
    else if (lbl == "rec") {
        // rec(=(X,E)) => =(X, gamma(Y*, lambda(X,E)))
        Node *eq = node->child;
        Node *X  = eq->child;
        Node *E  = X->sibling;

        // Need a copy of X to remain the LHS of the resulting '='.
        Node *Xcopy = makeNode(X->label);

        Node *lambda = makeNode("lambda");
        lambda->child = X;
        X->sibling = E;
        E->sibling = nullptr;

        Node *ystar = makeNode("<Y*>");

        Node *gamma = makeNode("gamma");
        gamma->child = ystar;
        ystar->sibling = lambda;
        lambda->sibling = nullptr;

        node->label = "=";
        node->child = Xcopy;
        Xcopy->sibling = gamma;
        gamma->sibling = nullptr;
    }
    // All other node labels are already in standard form.
}
