#pragma once

#include "node.h"

class Standardizer {
public:
    Node *standardize(Node *root);

private:
    int childCount(Node *node) const;
    Node *nthChild(Node *node, int index) const;
    void standardizeNode(Node *node);
};
