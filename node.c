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
    n->flags = 0;
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

void do_string_node(char **s, node_t *t);
void do_string_elem(char **s, elem_t *e);

char *string_node(node_t *n) {
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_node(&s, n);
    return s;
}

void do_string_node(char **s, node_t *n) {
    if (n == NULL) return;
    char *tmp = NULL;
    switch(n->tag) {
        case N_COMPOSED:
            if (right(n) != NULL) {
                do_string_node(s, right(n));
                if (left(n) != NULL) {
                    rstrcat(s, " ");
                }
            }
            if (left(n) != NULL) {
                do_string_node(s, left(n));
            }
            break;
        case N_BLOCK:
            rstrcat(s, "[ ");
            do_string_node(s, left(n));
            rstrcat(s, " ]");
            break;
        case N_LIST:
            rstrcat(s, "{ ");
            do_string_node(s, left(n));
            rstrcat(s, " }");
            break;
        case N_WORD:
            rstrcat(s, n->content.n_str);
            break;
        case N_STRING:
            asprintf(&tmp, "\"%s\"", n->content.n_str);
            rstrcat(s, tmp);
            break;
        case N_INT:
            asprintf(&tmp, "%d", n->content.n_int);
            rstrcat(s, tmp);
            break;
        case N_FLOAT:
            asprintf(&tmp, "%g", n->content.n_float);
            rstrcat(s, tmp);
            break;
        case N_CHAR:
            asprintf(&tmp, "#\"%c\"", n->content.n_char);
            rstrcat(s, tmp);
            break;
        case N_DEFINE:
            rstrcat(s, "<define something-or-other>");
            break;
        case N_ELEM:
            do_string_elem(s, n->content.elem);
            break;
        default:
            printf("???");
    }
    free(tmp);
}

node_t *copy_node(node_t *n) {
    if (n == NULL) return NULL;
    node_t *n2 = new_node();
    n2->tag = n->tag;
    n2->flags |= NF_COPIED;
    if (n->tag == N_WORD || n->tag == N_STRING) {
        n2->content.n_str = malloc(strlen(n->content.n_str)+1);
        strcpy(n2->content.n_str, n->content.n_str);
    } else if (n->tag == N_COMPOSED || n->tag == N_BLOCK || n->tag == N_LIST || n->tag == N_DEFINE) {
        set_left(n2, copy_node(left(n)));
        set_right(n2, copy_node(right(n)));
    } else if (n->tag == N_INT) {
        n2->content.n_int = n->content.n_int;
    } else if (n->tag == N_FLOAT) {
        n2->content.n_float = n->content.n_float;
    } else if (n->tag == N_CHAR) {
        n2->content.n_char = n->content.n_char;
    } else if (n->tag == N_ELEM) {
        n2->content.elem = copy_elem(n->content.elem);
    }
    return n2;
}

int node_copied(node_t *n) {
    return (n->flags & NF_COPIED) > 0;
}
