#include "alma.h"

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

node_t *new_node() {
    node_t *n = malloc(sizeof(node_t));
    n->line = -1;
    return n;
}

node_t *node_str (char *str, int line_num) {
    node_t *n = new_node();
    n->tag = N_STRING;
    n->content.n_str = str;
    n->line = line_num;
    return n;
}

node_t *node_elem (elem_t *val, int line_num) {
    node_t *n = new_node();
    n->tag = N_ELEM;
    n->content.elem = val;
    n->line = line_num;
    return n;
}

node_t *node_word (char *name, int line_num) {
    node_t *n = new_node();
    n->tag = N_WORD;
    n->content.n_str = name;
    n->line = line_num;
    return n;
}

node_t *node_int (int val, int line_num) {
    node_t *n = new_node();
    n->tag = N_INT;
    n->content.n_int = val;
    n->line = line_num;
    return n;
}

node_t *node_float (double val, int line_num) {
    node_t *n = new_node();
    n->tag = N_FLOAT;
    n->content.n_float = val;
    n->line = line_num;
    return n;
}

node_t *node_char (char val, int line_num) {
    node_t *n = new_node();
    n->tag = N_CHAR;
    n->content.n_char = val;
    n->line = line_num;
    return n;
}

node_t *_node_lineno(enum node_tag tag, node_t *nleft, node_t *nright, int line_num) {
    node_t *n = new_node();
    n->tag = tag;
    n->line = line_num;
    set_left(n, nleft);
    set_right(n, nright);
    return n;
}

// Returns the last-executed non-branching node which is a child of this node.
node_t *last_node_in(node_t *nn) {
    if (nn->tag == N_COMPOSED) {
        if (right(nn) == NULL) {
            return last_node_in(left(nn));
        } else {
            return last_node_in(right(nn));
        }
    } else {
        return nn;
    }
}

// Returns the first-executed non-branching node which is a child of this node.
node_t *first_node_in(node_t *nn) {
    if (nn->tag == N_COMPOSED) {
        if (left(nn) == NULL) {
            return last_node_in(right(nn));
        } else {
            return last_node_in(left(nn));
        }
    } else {
        return nn;
    }
}
