#include "alma.h"
#include "y.tab.h"

void eval(node_t *nn, elem_t **top) {
    if (nn != NULL) {
        switch (nn->tag) {
            case N_COMPOSED:
                eval(left(nn), top);
                eval(right(nn), top);
                break;
            case N_BLOCK:
                //printf("{ ... }\n");
                push(elem_from(nn), top);
                break;
            case N_LIST:
                //printf("[\n");
                push(elem_from(nn), top);
                //printf("]\n");
                break;
            case N_STRING:
                //printf("string %s\n", nn->content.n_str);
                push(elem_from(nn), top);
                break;
            case N_INT:
                //printf("int %d\n", nn->content.n_int);
                push(elem_from(nn), top);
                break;
            case N_BOOL:
                //printf("int %d\n", nn->content.n_int);
                push(elem_from(nn), top);
                break;
            case N_FLOAT:
                //printf("float %g\n", nn->content.n_float);
                push(elem_from(nn), top);
                break;
            case N_CHAR:
                //printf("char %c\n", nn->content.n_char);
                push(elem_from(nn), top);
                break;
            case N_ELEM:
                push(copy_elem(nn->content.elem), top);
                break;
            case N_WORD:
                //printf("WORD %s\n", nn->content.n_str);
                do_word(nn->content.n_str, top);
                break;
            default:
                fprintf(stderr, "unrecognized node type %d, wtf\n", nn->tag);
        }
    }
}

elem_t *new_elem() {
    elem_t *new_elem = malloc(sizeof(elem_t));
    new_elem->next = NULL;
    new_elem->prev = NULL;
    return new_elem;
}

elem_t *list_from_string(char *str) {
    int index = 0;
    while (str[index] != '\0') {
        // find the end of the string...
        index ++;
    }
    elem_t *elem = NULL;
    while (index > 0) {
        index --;
        elem_t *elem2 = elem_char(str[index]);
        push(elem2, &elem);
    }
    return elem;
}

elem_t *list_from(elem_t *n) {
    elem_t *elem = new_elem();
    elem->tag = E_LIST;
    elem->content.list = n;
}

elem_t *elem_from(node_t *n) {
    if (n->tag == N_ELEM) { return n->content.elem; }
    elem_t *elem = new_elem();
    if (n->tag == N_FLOAT) {
        elem->tag = E_FLOAT;
        set_type(elem, vt_num);
        elem->content.e_float = n->content.n_float;
    } else if (n->tag == N_INT) {
        elem->tag = E_INT;
        set_type(elem, vt_num);
        elem->content.e_int = n->content.n_int;
    } else if (n->tag == N_BOOL) {
        elem->tag = E_BOOL;
        set_type(elem, vt_bool);
        elem->content.e_int = n->content.n_int;
    } else if (n->tag == N_CHAR) {
        elem->tag = E_CHAR;
        set_type(elem, vt_char);
        elem->content.e_char = n->content.n_char;
    } else if (n->tag == N_LIST) {
        elem->tag = E_LIST;
        elem_t *inlist = NULL;
        eval(left(n), &inlist);
        elem->content.list = inlist;
        value_type *inner_type = get_list_type(elem->content.list);
        if (inner_type == NULL) {
            inner_type = type_var();
        } else {
            if (inner_type->tag == V_ERROR) {
                fprintf(stderr, "Error, type mismatch in list. This should've been caught during compilation?\n");
                return NULL;
            }
        }
        set_type(elem, list_of(inner_type));
    } else if (n->tag == N_STRING) {
        elem->content.list = list_from_string(n->content.n_str);
        elem->tag = E_LIST;
        set_type(elem, list_of(vt_char));
        //elem->content.e_str = n->content.n_str;
    } else if (n->tag == N_BLOCK) {
        elem->tag = E_BLOCK;
        if (!interactive_mode) {
            elem->content.block = left(n);
        } else {
            elem->content.block = copy_node(left(n));
        }
        set_type(elem, infer_type(elem->content.block));
    } else {
        printf("Unimplemented element type %d.\n", n->tag);
        elem->tag = E_INT;
        elem->content.e_int = 0;
    }
    //printf("type: ");
    //print_type(elem->type);
    return elem;
}

void push(elem_t *new_elem, elem_t **top) {
    if (new_elem == NULL) return;

    if (new_elem->next == NULL) {
        //printf("Push elem %p.\n", new_elem);
        new_elem->next = *top;
        if (*top == NULL) {
            new_elem->height = 1;
        } else {
            new_elem->height = (*top)->height + 1;
            (*top)->prev = new_elem;
        }
    } else {
        //printf("Push elem %p, but first:\n", new_elem);
        push(new_elem->next, top);
        new_elem->height = (*top)->height + 1;
        //printf("Push elem %p.\n", new_elem);
    }
    *top = new_elem;
}

// TODO I think this is old and busted as well maybe
unsigned int length(elem_t *e) {
    if (!is_list(e)) {
        fprintf(stderr, "Internal error: shouldn't call `length` on non-list data");
    }
    if (e->content.list == NULL) {
        return 0;
    } else {
        return e->content.list->height;
    }
}

elem_t *elem_int (int val) {
    elem_t *e = new_elem();
    e->tag = E_INT;
    set_type(e, vt_num);
    e->content.e_int = val;
    return e;
}

elem_t *elem_bool (int val) {
    elem_t *e = new_elem();
    e->tag = E_BOOL;
    set_type(e, vt_num);
    e->content.e_int = (val != 0);
    return e;
}

elem_t *elem_float (double val) {
    elem_t *e = new_elem();
    e->tag = E_FLOAT;
    set_type(e, vt_num);
    e->content.e_float = val;
    return e;
}

elem_t *elem_char (char val) {
    elem_t *e = new_elem();
    e->tag = E_CHAR;
    set_type(e, vt_char);
    e->content.e_char = val;
    return e;
}

elem_t *elem_str (char *val) {
    elem_t *e = new_elem();
    e->tag = E_LIST;
    set_type(e, list_of(vt_char));
    e->content.list = list_from_string(val);
    return e;
}

elem_t *elem_list (elem_t *content) {
    elem_t *e = new_elem();
    e->tag = E_LIST;
    e->content.list = content;
    value_type *inner_type = get_list_type(e->content.list);
    if (inner_type->tag == V_ERROR) {
        fprintf(stderr, "Error, type mismatch in list");
        return NULL;
    }
    set_type(e, list_of(inner_type));
    return e;
}

elem_t *elem_product (elem_t *l, elem_t *r) {
    elem_t *e = new_elem();
    set_type(e, product_type(l->type, r->type));
    e->tag = E_PRODUCT;
    e->content.product.left = l;
    e->content.product.right = r;
    return e;
}

void do_word(const char *word, elem_t **top) {
    lib_entry_t *e = NULL;
    HASH_FIND_STR (lib.table, word, e);
    if (e) {
        elem_t *result = apply(e, top);
        push(result, top);
    } else {
        fprintf(stderr, "unimplemented word %s\n", word);
    }
}

elem_t *apply(lib_entry_t *e, elem_t **top) {
    /*value_type *type = copy_type(e->type);
    printf("Attempt-type: ");
    print_type(type);
    int could_unify = confirm_stack_type(type->content.func_type.in, *top);
    if (could_unify) {
        //bestow_varnames(type);
        //print_type(type);
    } else {
        printf("failure\n");
        return NULL;
    }
    free_type(type);*/
    if (e->internal) {
        elem_t *result = (*e->impl.func)(top);
        return result;
    } else {
        eval(e->impl.node, top);
    }
    // Check types, and check that auto-zipped vectors are the same length
    /*for (i = 0; i < e->in_type_length; i++) {
        if (check_elem != NULL) {
            if (e->in_types[i] == K_LIST) {
                if (!is_list(check_elem)) {
                    fprintf(stderr, "argument #%d of `%s` must be a list!", i+1, e->name);
                    return NULL;
                }
            } else if (e->in_types[i] == K_ANY) {
                // do nothing, it's good no matter what!!
            } else if (scalar_type(e->in_types[i])) {
                if (is_list(check_elem)) {
                    unsigned int l = length(check_elem);
                    if (zip_height == -1 || zip_height == l) {
                        zip_height = l;
                    } else {
                        fprintf(stderr, "Can't operate on vectors of different lengths!");
                        return NULL;
                    }
                }
            }
            check_elem = check_elem->next;
        } else {
            fprintf(stderr, "Insufficient arguments to `%s`! (expected %d, found %d)\n", e->name, e->in_type_length, i);
            return NULL;
        }
    }
    if (zip_height == -1) {
        elem_t *result = (*e->func)(top);
        return result;
    } else {
        elem_t **stacks = malloc(zip_height * sizeof(elem_t*));
        int i;
        for (i = 0; i < zip_height; i++) {
            stacks[i] = NULL;
        }
        // Find last element that's going to be relevant
        int count = 1; // starting at 1 because otherwise it'll snarf up an extra argument and copy it uselessly
                        // (or if it's null, probably segfault...)
        elem_t *arg_elem = *top;
        while (count < e->in_type_length) {
            arg_elem = arg_elem->next;
            count ++;
        }
        // ok now we have the last argument -- now we walk backwards up the stack, pushing a copy of each argument onto
        // the sub-argument stacks if it's not auto-zipping, or pushing the requisite element if it is
        while (arg_elem != NULL) {
            count --;
            if (scalar_type(e->in_types[count]) && is_list(arg_elem)) {
                // expecting scalar, found list: push one element of list onto each arg-stack
                int j = 0;
                elem_t *el = arg_elem->content.list;
                while (el) {
                    elem_t *p = pop(&el);
                    push (p, &stacks[j]);
                    j++;
                }
            } else {
                // otherwise normal: create a copy for each arglist
                int i;
                for (i = 0; i < zip_height; i++) {
                    push(copy_elem(arg_elem), &stacks[i]);
                }
            }
            arg_elem = arg_elem->prev;
        }
        // pop off the arguments we used -- they've probably been free'd already anyway so don't want them sticking around
        for (i = 0; i < e->in_type_length; i++) {
            pop(top);
        }
        // okay, now we've constructed the special argument stacks -- time to mush everything back into one list
        elem_t *result = NULL; // list we're going to stick everything on
        for (i = zip_height-1; i >= 0; i--) {
            push(apply(e, &stacks[i]), &result);
        }
        // clean up anything remaining on the stacks
        for (i = 0; i < zip_height; i++) {
            free_elems_below(stacks[i]);
        }
        free(stacks);
        return elem_list(result);
    }*/
    return NULL;
}

elem_t *copy_elem(elem_t *e) {
    elem_t *clone = new_elem();
    clone->tag = e->tag;
    switch(e->tag) {
        case E_CHAR:
            clone->content.e_char = e->content.e_char;
            break;
        case E_BOOL:
        case E_INT:
            clone->content.e_int = e->content.e_int;
            break;
        case E_FLOAT:
            clone->content.e_float = e->content.e_float;
            break;
        case E_BLOCK:
            clone->content.block = copy_node(e->content.block);
            break;
        case E_LIST:
            clone->content.list = copy_list(e->content.list);
            break;
        default:
            fprintf(stderr, "don't know how to clone an element of type %d!\n", e->tag);
    }
    set_type(clone, copy_type(e->type));
    return clone;
}

elem_t *copy_list(elem_t *l) {
    if (l == NULL) return l;
    elem_t *list = copy_list(l->next);
    elem_t *head_of_list = copy_elem(l);
    push(head_of_list, &list);
    return list;
}

int truthy(elem_t *e) {
    if (e->tag == E_INT) {
        return e->content.e_int != 0;
    }
    if (e->tag == E_FLOAT) {
        return e->content.e_float != 0.0;
    }
    if (e->tag == E_CHAR) {
        return e->content.e_char != '\0';
    }
    if (e->tag == E_LIST) {
        return length(e) != 0;
    }
    if (e->tag == E_BOOL) {
        return e->content.e_int;
    }
    return 1;
}

int scalar_type(enum type_tag t) {
    return (t == K_CHAR || t == K_NUM || t == K_BLOCK || t == K_SCALAR);
}

int is_scalar(elem_t *e) {
    if (e->tag == E_CHAR
            || e->tag == E_INT
            || e->tag == E_FLOAT
            || e->tag == E_BLOCK) {
        return 1;
    }
    return 0;
}

int is_list(elem_t *e) {
    if (e->tag == E_LIST) {
        return 1;
    }
    return 0;
}

int is_block(elem_t *e) {
    if (e->tag == E_BLOCK) {
        return 1;
    }
    return 0;
}

void _do_string_elem_optquotes(char **s, elem_t *e, int quote_strings);
void do_string_list(char **s, elem_t *e);
void do_string_node(char **s, node_t *t);

void do_string_elem(char **s, elem_t *e) {
    _do_string_elem_optquotes(s, e, 1);
}

// Called for top layer of recursion (don't quote strings, for printing)
void do_string_elem_top(char **s, elem_t *e) {
    _do_string_elem_optquotes(s, e, 0);
}

void _do_string_elem_optquotes(char **s, elem_t *e, int quote_strings) {
    if (e == NULL) return;
    char *tmp = NULL;
    switch(e->tag) {
        case E_CHAR:
            asprintf(&tmp, "#\"%c\"", e->content.e_char);
            rstrcat(s, tmp);
            break;
        case E_INT:
            asprintf(&tmp, "%d", e->content.e_int);
            rstrcat(s, tmp);
            break;
        case E_BOOL:
            if (e->content.e_int) {
                rstrcat(s, "true");
            } else {
                rstrcat(s, "false");
            }
            break;
        case E_FLOAT:
            asprintf(&tmp, "%g", e->content.e_float);
            rstrcat(s, tmp);
            break;
        case E_LIST:
            if (e->type->content.v == vt_char) {
                if (quote_strings) {
                    rstrcat(s, "\"");
                }
                do_string_list(s, e->content.list);
                if (quote_strings) {
                    rstrcat(s, "\"");
                }
            } else {
                rstrcat(s, "{ ");
                do_string_list(s, e->content.list);
                rstrcat(s, " }");
            }
            break;
        case E_PRODUCT:
            do_string_elem(s, e->content.product.left);
            rstrcat(s, " * ");
            do_string_elem(s, e->content.product.right);
            break;
        case E_BLOCK:
            rstrcat(s, "[ ");
            do_string_node(s, e->content.block);
            rstrcat(s, " ]");
            break;
        default:
            printf("???");
    }
    free(tmp);
}

void do_string_list(char **s, elem_t *e) {
    if (e == NULL) return;
    if (e->type != vt_char) {
        do_string_elem(s, e);
        if (e->next != NULL) {
            rstrcat(s, " ");
            do_string_list(s, e->next);
        }
    } else {
        //printf("CHAR: %d / %c\n", e->content.e_char, e->content.e_char);
        char *tmp = NULL;
        asprintf(&tmp, "%c", e->content.e_char);
        rstrcat(s, tmp);
        free(tmp);
        do_string_list(s, e->next);
    }
}

char *string_elem(elem_t *e) {
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_elem_top(&s, e);
    return s;
}

char *repr_elem(elem_t *e) {
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_elem(&s, e);
    return s;
}

void print_elem(elem_t *e) {
    char *p = string_elem(e);
    printf("%s", p);
    free(p);
}

void print_repr_elem(elem_t *e) {
    char *p = repr_elem(e);
    printf("%s", p);
    free(p);
}

void print_string(elem_t *e) {
    if (e == NULL) return;
    printf("%c", e->content.e_char);
    print_string(e->next);
}

void free_elems_below(elem_t *e) {
    elem_t *to_free = e;
    while (to_free != NULL) {
        elem_t *elem_next = to_free->next;
        free_elem(to_free);
        to_free = elem_next;
    }
}

void free_elem(elem_t *e) {
    if (e == NULL) return;
    //printf("Freeing elem --\n");
    if (e->tag == E_LIST) {
        free_elems_below(e->content.list);
    }
    if (e->tag == E_PRODUCT) {
        free_elem(e->content.product.left);
        free_elem(e->content.product.right);
    }
    if (e->tag == E_BLOCK) {
        /* if a node was copied, it can be freed when its elem
         * is popped off. if it wasn't copied, it corresponds to
         * something in the AST of the actual program, & thus will
         * be freed at the end of evaluation. so we don't want to
         * double-free it! :O */
        if (e->content.block != NULL && node_copied(e->content.block)) {
            free_node(e->content.block);
        }
    }
    free_type(e->type);
    free(e);
    ///printf("-- elem freed.");
}

void free_node(node_t *n) {
    if (n == NULL) return;
    if (n->tag == N_WORD || n->tag == N_STRING) {
        free(n->content.n_str);
    }
    if (n->tag == N_ELEM) {
        free_elem(n->content.elem);
    }
    if (n->tag == N_COMPOSED || n->tag == N_BLOCK || n->tag == N_LIST) {
        free_node(left(n));
        free_node(right(n));
    }
    free(n);
}
