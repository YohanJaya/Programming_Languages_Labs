#pragma once

#include <iostream>
#include <string>

struct Node {
    std::string label;
    Node *child;
    Node *sibling;

    explicit Node(const std::string &l) : label(l), child(nullptr), sibling(nullptr) {}
};

inline Node *makeNode(const std::string &label) {
    return new Node(label);
}

inline void deleteTree(Node *root) {
    if (!root) {
        return;
    }
    deleteTree(root->child);
    deleteTree(root->sibling);
    delete root;
}

inline void printASTImpl(Node *node, int depth) {
    if (!node) {
        return;
    }
    std::cout << std::string(depth * 2, ' ') << node->label << '\n';
    printASTImpl(node->child, depth + 1);
    printASTImpl(node->sibling, depth);
}

inline void printAST(Node *root, int depth = 0) {
    printASTImpl(root, depth);
}
