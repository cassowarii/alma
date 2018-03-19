#include "cloth.h"

node_t *left(node_t *node) {
    return node->content.children.left;
}

void set_left(node_t *node, node_t *l) {
    node->content.children.left = l;
}

node_t *right(node_t *node) {
    return node->content.children.right;
}

void set_right(node_t *node, node_t *r) {
    node->content.children.right = r;
}
