#include <stdio.h>
#include "alma.h"

void do_print_type(value_type *t);
void do_print_stack_type(stack_type *t);
void do_string_type(char **s, value_type *t);
void do_string_stack_type(char **s, stack_type *t);

char *string_type_nocopy(value_type *t);

long int typevar_id = 0;

value_type *new_typevar() {
    value_type *t = malloc(sizeof(value_type));
    t->refs = 0;
    t->id = typevar_id;
    typevar_id++;
    return t;
}

stack_type *new_stackvar() {
    stack_type *t = malloc(sizeof(stack_type));
    t->refs = 0;
    t->id = typevar_id;
    typevar_id++;
    return t;
}

void set_type (elem_t *e, value_type *t) {
    e->type = t;
    t->refs++;
}

value_type *base_type(const char *name) {
    value_type *t = new_typevar();
    t->tag = V_BASETYPE;
    t->content.type_name = malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(t->content.type_name, name);
    //printf("Created base type with name %s.\n", t->content.type_name);
    return t;
}

value_type *list_of(value_type *what) {
    if (what->tag != V_ERROR) {
        value_type *t = new_typevar();
        what->refs ++;
        t->tag = V_LIST;
        t->content.v = what;
        return t;
    } else {
        return what;
    }
}

value_type *func_type(stack_type *from, stack_type *to) {
    if (from->tag != S_ERROR && to->tag != S_ERROR) {
        value_type *t = new_typevar();
        from->refs ++;
        to->refs ++;
        t->tag = V_FUNC;
        t->content.func_type.in = from;
        t->content.func_type.out = to;
        return t;
    } else {
        error *err;
        if (from->tag == S_ERROR) {
            err = from->content.err;
        } else {
            err = to->content.err;
        }
        free_stack_type(from);
        free_stack_type(to);
        return error_type(err);
    }
}

value_type *product_type(value_type *left, value_type *right) {
    if (left->tag != V_ERROR && right->tag != V_ERROR) {
        value_type *t = new_typevar();
        left->refs ++;
        right->refs ++;
        t->tag = V_PRODUCT;
        t->content.prod_type.left = left;
        t->content.prod_type.right = right;
        return t;
    } else if (left->tag == V_ERROR) {
        free_type(right);
        return left;
    } else if (right->tag == V_ERROR) {
        free_type(left);
        return right;
    }
}

value_type *type_var() {
    value_type *t = new_typevar();
    t->tag = V_VAR;
    t->content.var_name = '_'; // will get named after unification!
    return t;
}

value_type *error_type(error *e) {
    value_type *t = new_typevar();
    t->tag = V_ERROR;
    t->content.err = e;
    return t;
}

value_type *named_type_var(char name) {
    value_type *t = new_typevar();
    t->tag = V_VAR;
    t->content.var_name = name;
    return t;
}

value_type *scalar_var() {
    value_type *t = new_typevar();
    t->tag = V_SCALARVAR;
    t->content.var_name = '_'; // will get named after unification!
    return t;
}

stack_type *error_stacktype(error *e) {
    stack_type *t = new_stackvar();
    t->tag = S_ERROR;
    t->content.err = e;
    return t;
}

stack_type *stack_of(value_type *top, stack_type *rest) {
    if (top->tag != V_ERROR) {
        stack_type *t = new_stackvar();
        t->tag = S_TOPTYPE;
        top->refs ++;
        rest->refs ++;
        t->content.top_type.top = top;
        t->content.top_type.rest = rest;
        return t;
    } else {
        error *e = top->content.err;
        free_type(top);
        return error_stacktype(e);
    }
}

stack_type *stack_var() {
    stack_type *t = new_stackvar();
    t->tag = S_VAR;
    t->content.var_name = '_';
    return t;
}

stack_type *zero_stack() {
    stack_type *t = new_stackvar();
    t->tag = S_ZERO;
    return t;
}

stack_type *named_stack_var(char name) {
    stack_type *t = new_stackvar();
    t->tag = S_VAR;
    t->id = typevar_id;
    typevar_id++;
    t->content.var_name = name;
    return t;
}

int compare_stack_types(stack_type *a, stack_type *b);

int compare_types(value_type *a, value_type *b) {
    while (a->tag == V_UNIFIED) { a = a->content.v; }
    while (b->tag == V_UNIFIED) { b = b->content.v; }
    if (a->tag != b->tag
            && !(a->tag == V_VAR && b->tag == V_SCALARVAR)
            && !(a->tag == V_SCALARVAR && b->tag == V_VAR)) {
        return 0;
    }
    switch(a->tag) {
        case V_VAR:
        case V_SCALARVAR:
            return (a->id == b->id);
        case V_BASETYPE:
            return !(strcmp(a->content.type_name, b->content.type_name));
        case V_FUNC:
            if (compare_stack_types(a->content.func_type.in, b->content.func_type.in)
                    && compare_stack_types(a->content.func_type.out, b->content.func_type.out)) {
                return 1;
            } else {
                return 0;
            }
        case V_PRODUCT:
            if (compare_types(a->content.prod_type.left, b->content.prod_type.left)
                    && compare_types(a->content.prod_type.right, b->content.prod_type.right)) {
                return 1;
            } else {
                return 0;
            }
        case V_LIST:
            return compare_types(a->content.v, b->content.v);
    }
    return 1;
}

int compare_stack_types(stack_type *a, stack_type *b) {
    while (a->tag == S_UNIFIED) { a = a->content.unif; }
    while (b->tag == S_UNIFIED) { b = b->content.unif; }
    if (a->tag != b->tag) {
        return 0;
    }
    if (a->tag == S_TOPTYPE) {
        return (compare_types(a->content.top_type.top, b->content.top_type.top)
            && compare_stack_types(a->content.top_type.rest, b->content.top_type.rest));
    }
    return 1;
}

int v_count_in_v(value_type *a, value_type *b);
int v_count_in_s(value_type *a, stack_type *b);
int s_count_in_v(stack_type *a, value_type *b);
int s_count_in_s(stack_type *a, stack_type *b);

int v_count_in_v(value_type *a, value_type *b) {
    while (a->tag == V_UNIFIED) { a = a->content.v; }
    while (b->tag == V_UNIFIED) { b = b->content.v; }
    if (a == b) {
        return 1;
    } else if (b->tag == V_FUNC) {
        return v_count_in_s(a, b->content.func_type.in) + v_count_in_s(a, b->content.func_type.out);
    } else if (b->tag == V_LIST) {
        return v_count_in_v(a, b->content.v);
    } else if (b->tag == V_PRODUCT) {
        return v_count_in_v(a, b->content.prod_type.left) + v_count_in_v(a, b->content.prod_type.right);
    } else {
        return 0;
    }
}

int v_count_in_s(value_type *a, stack_type *b) {
    while (a->tag == V_UNIFIED) { a = a->content.v; }
    while (b->tag == S_UNIFIED) { b = b->content.unif; }
    if (b->tag == S_TOPTYPE) {
        if (b->content.top_type.top == a) {
            return 1 + v_count_in_s(a, b->content.top_type.rest);
        } else {
            return v_count_in_s(a, b->content.top_type.rest);
        }
    } else if (b->tag == S_VAR) {
        return 0;
    } else {
        return 0;
    }
}

int s_count_in_v(stack_type *a, value_type *b) {
    while (a->tag == S_UNIFIED) { a = a->content.unif; }
    while (b->tag == V_UNIFIED) { b = b->content.v; }
    if (b->tag == V_FUNC) {
        return s_count_in_s(a, b->content.func_type.in) + s_count_in_s(a, b->content.func_type.out);
    } else if (b->tag == V_LIST) {
        return s_count_in_v(a, b->content.v);
    } else if (b->tag == V_PRODUCT) {
        return s_count_in_v(a, b->content.prod_type.left) + s_count_in_v(a, b->content.prod_type.right);
    } else {
        return 0;
    }
}

int s_count_in_s(stack_type *a, stack_type *b) {
    while (a->tag == S_UNIFIED) { a = a->content.unif; }
    while (b->tag == S_UNIFIED) { b = b->content.unif; }
    if (a == b) {
        return 1;
    } else if (b->tag == S_TOPTYPE) {
        return s_count_in_v(a, b->content.top_type.top) + s_count_in_s(a, b->content.top_type.rest);
    } else {
        return 0;
    }
}

stack_type *bottom_of_stack(stack_type *t) {
    while (t->tag == S_UNIFIED) { t = t->content.unif; }
    if (t->tag == S_TOPTYPE) {
        return bottom_of_stack(t->content.top_type.rest);
    }
    return t;
}

void regeneralize(value_type **tpt);

void regeneralize_stack(stack_type *t) {
    if (t->tag == S_TOPTYPE) {
        regeneralize(&t->content.top_type.top);
        regeneralize_stack(t->content.top_type.rest);
    }
}

void regeneralize(value_type **tpt) {
    value_type *t = *tpt;
    if (t->tag == V_FUNC) {
        regeneralize_stack(t->content.func_type.in);
        regeneralize_stack(t->content.func_type.out);
        stack_type *lb = bottom_of_stack(t->content.func_type.in);
        stack_type *rb = bottom_of_stack(t->content.func_type.out);
        if (lb == rb) {
            if (s_count_in_v(lb, t) == 2) {
                value_type *copy = copy_type(t);
                free_type(*tpt);
                *tpt = copy;
            }
        }
    } else {
        switch (t->tag) {
            case V_LIST:
                regeneralize(&t->content.v);
                break;
            case V_PRODUCT:
                regeneralize(&t->content.prod_type.left);
                regeneralize(&t->content.prod_type.right);
                break;
            case V_UNIFIED:
                regeneralize(&t->content.v);
                break;
            case V_ERROR:
            case V_BASETYPE:
            case V_VAR:
            case V_SCALARVAR:
                // do nothing for these!
                break;
            case V_FUNC:
                // how did you even get here?
                break;
        }
    }
}

stack_type *unify_stack(stack_type *a, stack_type *b);

value_type *unify(value_type *a, value_type *b) {
    while (a->tag == V_UNIFIED) { a = a->content.v; }
    while (b->tag == V_UNIFIED) { b = b->content.v; }
#ifdef TYPEDEBUG
    printf("[U] %ld<%d> := %ld<%d>; ", a->id, a->tag, b->id, b->tag);
    do_print_type(a); printf("; "); do_print_type(b); printf("; ");
#endif
    if (a->tag == V_VAR || a->tag == V_SCALARVAR) {
#ifdef TYPEDEBUG
        printf("Unify var-type %ld with %ld\n", a->id, b->id);
#endif
        if (a != b) {
            if (v_count_in_v(a, b) > 0) {
                error *e = error_msg("* Found one type inside the other!");
                char *sa = string_type(a);
                char *sb = string_type(b);
                add_info(e, "    (specifically, found %s inside %s!)", sa, sb);
#ifndef TYPEDEBUG
                add_info(e, "    (incidentally, this error message is useless! i don't know how to improve it.)");
#else
                add_info(e, "    (you have TYPEDEBUG on, so this is hopefully useful to you.)");
#endif
                free(sa);
                free(sb);
                return error_type(e);
            }
            a->tag = V_UNIFIED;
            a->content.v = b;
            b->refs ++;
        }
        return b;
    } else if (b->tag == V_VAR || b->tag == V_SCALARVAR) {
        return unify(b, a);
    } else if (a->tag == V_FUNC && b->tag == V_FUNC) {
#ifdef TYPEDEBUG
        printf("Unify func-type %ld with %ld\n", a->id, b->id);
#endif
        stack_type *s_in = unify_stack(a->content.func_type.in, b->content.func_type.in);
        if (s_in->tag == S_ERROR) {
            error *e = s_in->content.err;
            free_stack_type(s_in);
            return error_type(e);
        }
        stack_type *s_out = unify_stack(a->content.func_type.out, b->content.func_type.out);
        if (s_out->tag == S_ERROR) {
            error *e = s_out->content.err;
            free_stack_type(s_out);
            return error_type(e);
        }
        return b;
    } else if (a->tag == V_LIST && b->tag == V_LIST) {
#ifdef TYPEDEBUG
        printf("Unify list-type %ld with %ld\n", a->id, b->id);
#endif
        return unify(a->content.v, b->content.v);
    } else if (a->tag == V_PRODUCT && b->tag == V_PRODUCT) {
        value_type *v_left = unify(a->content.prod_type.left, b->content.prod_type.left);
        if (v_left->tag == V_ERROR) {
            return v_left;
        }
        value_type *v_right = unify(a->content.prod_type.right, b->content.prod_type.right);
        if (v_right->tag == V_ERROR) {
            return v_right;
        }
        return b;
    } else if (a->tag == V_BASETYPE && b->tag == V_BASETYPE) {
#ifdef TYPEDEBUG
        printf("Unify base-type %ld with %ld\n", a->id, b->id);
#endif
        int compares = compare_types(a, b);
        if (compares) {
            return b;
        } else {
            char *sa = string_type(a);
            char *sb = string_type(b);
            error *e = error_msg("* Basic types don't match: %s and %s", sa, sb);
            free(sa);
            free(sb);
            return error_type(e);
        }
    } else {
        char *sa = string_type(a);
        char *sb = string_type(b);
        error *e = error_msg("* Couldn't figure out how to match %s and %s", sa, sb);
        free(sa);
        free(sb);
        return error_type(e);
    }
}

stack_type *unify_stack(stack_type *a, stack_type *b) {
    while (a->tag == S_UNIFIED) { a = a->content.unif; }
    while (b->tag == S_UNIFIED) { b = b->content.unif; }
    if (a->tag == S_VAR) {
#ifdef TYPEDEBUG
        printf("Unify var-stack %ld with %ld\n", a->id, b->id);
        char *sa = string_stack_type(a);
        char *sb = string_stack_type(b);
        printf("it's %s and %s, if you're wondering.\n", sa, sb);
        free(sa);
        free(sb);
#endif
        if (a != b) {
            if (s_count_in_s(a, b) > 0) {
                error *e = error_msg("* Found one type inside the other!");
                char *sa = string_stack_type(a);
                char *sb = string_stack_type(b);
                add_info(e, "    (specifically, found %s inside %s!)", sa, sb);
                free(sa);
                free(sb);
                return error_stacktype(e);
            }
            a->tag = S_UNIFIED;
            a->content.unif = b;
            b->refs ++;
        }
        return b;
    } else if (b->tag == S_VAR) {
        return unify_stack(b, a);
    } else if (a->tag == S_ZERO) {
#ifdef TYPEDEBUG
        printf("Unify 0-stack %ld with %ld\n", a->id, b->id);
#endif
        if (b->tag == S_ZERO || b->tag == S_VAR) {
            return b;
        } else {
            char *sa = string_stack_type(a);
            char *sb = string_stack_type(b);
            error *e = error_msg("* Couldn't unify types: %s and %s", sa, sb);
            free(sa);
            free(sb);
            return error_stacktype(e);
        }
    } else if (b->tag == S_ZERO) {
        return unify_stack(b, a);
    } else if (a->tag == S_TOPTYPE) {
#ifdef TYPEDEBUG
        printf("Unify tt-stack %ld with %ld\n", a->id, b->id);
#endif
        value_type *x = unify(a->content.top_type.top, b->content.top_type.top);
        stack_type *result;
        if (x->tag != V_ERROR) {
            result = unify_stack(a->content.top_type.rest, b->content.top_type.rest);
        } else {
            error *e = x->content.err;
            free_type(x);
            return error_stacktype(e);
        }
        return result;
    } else {
        char *sa = string_stack_type(a);
        char *sb = string_stack_type(b);
        error *e = error_msg("* Couldn't unify types: %s and %s", sa, sb);
        free(sa);
        free(sb);
        return error_stacktype(e);
    }
}

int confirm_stack_type(stack_type *t, elem_t *e) {
    while (t->tag == S_UNIFIED) { t = t->content.unif; }
    if (t->tag == S_VAR) {
        return 1;
    }
    if (t->tag == S_TOPTYPE) {
        if (unify(t->content.top_type.top, e->type)) {
            return confirm_stack_type(t->content.top_type.rest, e->next);
        } else {
            return 0;
        }
    }
}

value_type *get_list_type(elem_t *e) {
    if (e == NULL) return NULL;
    value_type *below_type = get_list_type(e->next);
    if (below_type == NULL) {
        return copy_type(e->type);
    } else if (below_type->tag == V_ERROR) {
        // mismatch below -- throw error up the stack
        return below_type;
    } else {
        value_type *check = copy_type(e->type);
        value_type *u = unify(below_type, check);
        if (u->tag != V_ERROR) {
            free_type(check);
            return below_type;
        } else {
            free_type(check);
            // first mismatch
            value_type *err = error_type(error_msg("Mismatched types in list: %s and %s", string_type(below_type), string_type(e->type)));
            return err;
        }
    }
}

// Ensure all values on top of a stack variable
// are the same. Used to check lists added inside
// of blocks.
// (Basically the same as get_list_type, but up
// a level of abstraction.)
value_type *get_abstract_list_type(stack_type *t, int line_num) {
    if (t == NULL) return NULL;
    while (t->tag == S_UNIFIED) { t = t->content.unif; }
    if (t->tag == S_VAR) return NULL;
    value_type *below_type = get_abstract_list_type(t->content.top_type.rest, line_num);
    if (below_type == NULL) {
        return t->content.top_type.top;
    } else {
        if (below_type->tag == V_ERROR) {
            // mismatch below -- throw error up the stack
            return below_type;
        } else {
            value_type *u = unify(below_type, t->content.top_type.top);
            if (u->tag != V_ERROR) {
                return below_type;
            } else {
                // first mismatch
                error *e = error_msg("Mismatched types in list: %s and %s", string_type(below_type), string_type(t->content.top_type.top));
                error_concat(e, u->content.err);
                error_lineno(e, line_num);
                return error_type(e);
            }
        }
    }
}

stack_type *do_stacktype_copy(stack_type *t, type_mapping **map);

value_type *do_type_copy(value_type *t, type_mapping **map) {
    if (t == NULL) return NULL;
    switch(t->tag) {
        case V_BASETYPE:
            return t;
        case V_VAR:
        case V_SCALARVAR:;
            type_mapping *v = NULL;
            HASH_FIND_PTR(*map, &t, v);
            if (!v) {
                v = malloc(sizeof(type_mapping));
                v->in = t;
                if (t->tag == V_VAR) {
                    v->out = type_var();
                } else {
                    v->out = scalar_var();
                }
                HASH_ADD_PTR(*map, in, v);
            }
            return v->out;
        case V_FUNC:
            return func_type(do_stacktype_copy(t->content.func_type.in, map), do_stacktype_copy(t->content.func_type.out, map));
        case V_LIST:
            return list_of(do_type_copy(t->content.v, map));
        case V_PRODUCT:
            return product_type(do_type_copy(t->content.prod_type.left, map), do_type_copy(t->content.prod_type.right, map));
        case V_UNIFIED:
            return do_type_copy(t->content.v, map);
        case V_ERROR:
            return t;
    }
}

stack_type *do_stacktype_copy(stack_type *t, type_mapping **map) {
    if (t == NULL) return NULL;
    switch(t->tag) {
        case S_VAR:;
            type_mapping *v = NULL;
            HASH_FIND_PTR(*map, &t, v);
            if (!v) {
                v = malloc(sizeof(type_mapping));
                v->in = t;
                v->out = stack_var();
                HASH_ADD_PTR(*map, in, v);
            }
            return v->out;
        case S_TOPTYPE:
            return stack_of(do_type_copy(t->content.top_type.top, map), do_stacktype_copy(t->content.top_type.rest, map));
        case S_ZERO:
            return zero_stack();
        case S_UNIFIED:
            return do_stacktype_copy(t->content.unif, map);
        case S_ERROR:
            return t;
    }
}

value_type *copy_type(value_type *t) {
    type_mapping *map = NULL;
    value_type *c = do_type_copy(t, &map);
    type_mapping *curr, *tmp;
    HASH_ITER(hh, map, curr, tmp) {
        HASH_DEL(map, curr);
        free(curr);
    }
    return c;
}

stack_type *copy_stack_type(stack_type *t) {
    type_mapping *map = NULL;
    stack_type *c = do_stacktype_copy(t, &map);
    type_mapping *curr, *tmp;
    HASH_ITER(hh, map, curr, tmp) {
        HASH_DEL(map, curr);
        free(curr);
    }
    return c;
}

value_type *infer_type(node_t *nn) {
    stack_type *X;
    lib_entry_t *e;
    value_type *l, *r, *result;
    stack_type *ok;
    if (nn == NULL) {
        // an empty node is the identity function...
        X = stack_var();
        return func_type(X, X);
    }
    switch (nn->tag) {
        case N_COMPOSED:
            if (right(nn)) {
                l = infer_type(left(nn));
                if (l->tag == V_ERROR) {
                    result = l;
                    break;
                }
                r = infer_type(right(nn));
                if (r->tag == V_ERROR) {
                    result = r;
                    free_type(l);
                    break;
                }
                ok = unify_stack(l->content.func_type.out, r->content.func_type.in);
                if (ok->tag != S_ERROR) {
                    result = func_type(l->content.func_type.in, r->content.func_type.out);
                } else {
                    char *sl = string_type(l);
                    char *sr = string_type(r);
                    char *wl = string_node(last_node_in(left(nn)));
                    char *wr = string_node(first_node_in(right(nn)));
                    error *e = error_msg("* Couldn't compose functions:");
                    add_info(e, "    couldn't match output stack of %s\n        type: %s", wl, sl);
                    add_info(e, "    with input stack of %s\n        type: %s", wr, sr);
                    error_concat(e, ok->content.err);
                    error_lineno(e, nn->line);
                    result = error_type(e);
                    free_error(ok->content.err);
                    free(ok);
                    free(wl);
                    free(wr);
                    free(sl);
                    free(sr);
                }
                free_type(l);
                free_type(r);
            } else {
                l = infer_type(left(nn));
                result = l;
            }
            break;
        case N_BLOCK:
            X = stack_var();
            result = func_type(X, stack_of(infer_type(left(nn)), X));
            if (result->tag == V_ERROR) {
                add_info(result->content.err, "while evaluating a block starting at line %d", nn->line);
            }
            break;
        case N_LIST:
            X = stack_var();
            l = infer_type(left(nn));
            if (l->tag == V_ERROR) {
                result = l;
                add_info(result->content.err, "while evaluating a list starting at line %d", nn->line);
                break;
            }
            value_type *ltype = get_abstract_list_type(l->content.func_type.out, nn->line);
            value_type *list;
            if (ltype == NULL) {
                r = type_var();
                list = list_of(r);
            } else {
                list = list_of(ltype);
            }
            result = func_type(X, stack_of(list, X));
            free_type(l);
            break;
        case N_STRING:
            X = stack_var();
            result = func_type(X, stack_of(list_of(vt_char), X));
            break;
        case N_INT:
        case N_FLOAT:
            X = stack_var();
            result = func_type(X, stack_of(vt_num, X));
            break;
        case N_CHAR:
            X = stack_var();
            result = func_type(X, stack_of(vt_char, X));
            break;
        case N_BOOL:
            X = stack_var();
            result = func_type(X, stack_of(vt_bool, X));
            break;
        case N_ELEM:
            X = stack_var();
            result = func_type(X, stack_of(nn->content.elem->type, X));
            break;
        case N_WORD:
#ifdef TYPEDEBUG
            printf("== consider word %s ==\n", nn->content.n_str);
#endif
            e = NULL;
            HASH_FIND_STR (lib.table, nn->content.n_str, e);
            if (e) {
                if (e->type->tag != V_ERROR) {
                    result = copy_type(e->type);
                } else {
                    result = e->type;
                }
            } else {
                result = error_type(error_msg("Unknown word `%s`", nn->content.n_str));
                if (nn->content.n_str[0] == ':') {
                    add_info(result->content.err, "(Did you try to start a :symbol with a non-alphanumeric character?)");
                }
                error_lineno(result->content.err, nn->line);
            }
            break;
        default:
            fprintf(stderr, "unrecognized node type %d, wtf\n", nn->tag);
    }
#ifdef TYPEDEBUG
    printf("[INF] "); do_print_type(result); printf("\n");
#endif
    regeneralize(&result);
    return result;
}

void stack_give_names(stack_type *t, char *current_v, char *current_s);

void give_names(value_type *t, char *current_v, char *current_s) {
    if (t == NULL) return;
    switch(t->tag) {
        case V_VAR:
        case V_SCALARVAR:
            if (t->content.var_name == '_') {
                //printf("Name var %p \"%c.\"\n", t, *current_v);
                t->content.var_name = *current_v;
                (*current_v)++;
            }
            return;
        case V_FUNC:
            //printf("FUNC %p name inputs\n", t);
            stack_give_names(t->content.func_type.in, current_v, current_s);
            //printf("FUNC %p name outputs\n", t);
            stack_give_names(t->content.func_type.out, current_v, current_s);
            return;
        case V_LIST:
            //printf("LIST %p name innards\n", t);
            give_names(t->content.v, current_v, current_s);
            return;
        case V_PRODUCT:
            give_names(t->content.prod_type.left, current_v, current_s);
            give_names(t->content.prod_type.right, current_v, current_s);
            return;
        case V_BASETYPE:
            //printf("BASETYPE %p go away\n", t);
            // do nothing
            return;
        case V_UNIFIED:
            give_names(t->content.v, current_v, current_s);
            return;
        case V_ERROR:
            return;
        default:
            fprintf(stderr, "Can't name type of type %d!", t->tag);
    }
}

void stack_give_names(stack_type *t, char *current_v, char *current_s) {
    if (t == NULL) return;
    switch(t->tag) {
        case S_TOPTYPE:
            //printf("STACK %p TOP name\n", t);
            give_names(t->content.top_type.top, current_v, current_s);
            //printf("STACK %p REST name\n", t);
            stack_give_names(t->content.top_type.rest, current_v, current_s);
            return;
        case S_VAR:
            if (t->content.var_name == '_') {
                if (*current_s == 'X') {
                    t->content.var_name = 'Z';
                } else if (*current_s == 'Z') {
                    t->content.var_name = 'X';
                } else {
                    t->content.var_name = *current_s;
                }
                //printf("STACK-VAR %p name %c\n", t, t->content.var_name);
                (*current_s)--;
            }
            return;
        case S_UNIFIED:
            stack_give_names(t->content.unif, current_v, current_s);
            return;
        case S_ZERO:
        case S_ERROR:
            return;
    }
}

void bestow_varnames (value_type *t) {
    char cv = 'a';
    char cs = 'Z';
    give_names(t, &cv, &cs);
}

void init_types() {
    //printf("INITIALIZING TYPES\n");
    vt_num = base_type("num");
    vt_char = base_type("char");
    vt_bool = base_type("bool");
    /*stack_type *X = named_stack_var('X');
    print_type(func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X)));
    value_type *a = named_type_var('a');
    value_type *b = named_type_var('b');
    stack_type *X2 = named_stack_var('X');
    print_type(func_type(stack_of(a, stack_of(b, X2)), stack_of(b, stack_of(a, X2))));*/
}

void print_type(value_type *t) {
    char *s = string_type(t);
    printf("%s\n", s);
    free(s);
}

void do_print_type(value_type *t) {
    char *s = string_type(t);
    printf("%s", s);
    free(s);
}

void print_stack_type(stack_type *t) {
    char *s = string_stack_type(t);
    printf("%s\n", s);
    free(s);
}

char *string_type_nocopy(value_type *t) {
    value_type *x = t;
    bestow_varnames(x);
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_type(&s, x);
    return s;
}

char *string_type(value_type *t) {
#ifndef TYPEDEBUG
    value_type *x = copy_type(t);
    x->refs ++;
#else
    value_type *x = t;
#endif
    bestow_varnames(x);
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_type(&s, x);
#ifndef TYPEDEBUG
    free_type(x);
#endif
    return s;
}

char *string_stack_type(stack_type *t) {
    stack_type *x = copy_stack_type(t);
    char cv = 'a';
    char cs = 'Z';
    stack_give_names(x, &cv, &cs);
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_stack_type(&s, x);
    free_stack_type(x);
    return s;
}

void do_string_type(char **s, value_type *t) {
    if (t == NULL) return;
    char *tmp = NULL;
    switch(t->tag) {
        case V_SCALARVAR:
        case V_VAR:
            if (t->content.var_name == '_') {
                asprintf(&tmp, "'<%ld>", t->id);
            } else {
#if defined(TYPEDEBUG) || defined(DIVDEBUG)
                asprintf(&tmp, "'%c<%ld>", t->content.var_name, t->id);
#else
                asprintf(&tmp, "'%c", t->content.var_name);
#endif
            }
            rstrcat(s, tmp);
            break;
        case V_FUNC:
#if defined(TYPEDEBUG) || defined(DIVDEBUG)
            asprintf(&tmp, "%ld[ ", t->id);
            rstrcat(s, tmp);
#else
            rstrcat(s, "[ ");
#endif
            do_string_stack_type(s, t->content.func_type.in);
            rstrcat(s, " -> ");
            do_string_stack_type(s, t->content.func_type.out);
            rstrcat(s, " ]");
            break;
        case V_LIST:
#if defined(TYPEDEBUG) || defined(DIVDEBUG)
            asprintf(&tmp, "%ld{", t->id);
            rstrcat(s, tmp);
#else
            rstrcat(s, "{");
#endif
            do_string_type(s, t->content.v);
            rstrcat(s, "}");
            break;
        case V_PRODUCT:
            do_string_type(s, t->content.prod_type.left);
            rstrcat(s, " * ");
            do_string_type(s, t->content.prod_type.right);
            break;
        case V_BASETYPE:
            rstrcat(s, t->content.type_name);
            break;
        case V_UNIFIED:
            do_string_type(s, t->content.v);
            break;
        case V_ERROR:
            printf("<error>");
            break;
        default:
            printf("???");
    }
    free(tmp);
}

void do_print_stack_type(stack_type *t) {
}

void do_string_stack_type(char **s, stack_type *t) {
    char *tmp = NULL;
    switch(t->tag) {
        case S_VAR:
            if (t->content.var_name == '_') {
                asprintf(&tmp, "'S<%ld>", t->id);
            } else {
#if defined(TYPEDEBUG) || defined(DIVDEBUG)
                asprintf(&tmp, "'%c<%ld>", t->content.var_name, t->id);
#else
                asprintf(&tmp, "'%c", t->content.var_name);
#endif
            }
            rstrcat(s, tmp);
            break;
        case S_TOPTYPE:
#if defined(TYPEDEBUG) || defined(DIVDEBUG)
            asprintf(&tmp, "%ld ", t->id);
            rstrcat(s, tmp);
#endif
            do_string_type(s, t->content.top_type.top);
            rstrcat(s, " ");
            do_string_stack_type(s, t->content.top_type.rest);
            break;
        case S_ZERO:
            rstrcat(s, "'0");
            break;
        case S_UNIFIED:
            do_string_stack_type(s, t->content.unif);
            break;
        case S_ERROR:
            printf("<error>");
            break;
        default:
            printf("???stack");
    }
    free(tmp);
}

void free_type(value_type *t) {
    if (t == NULL) return;
    //printf("Free type of tag %d @ %p.", t->tag, t);
    if (t->tag == V_BASETYPE) return;
    t->refs --;
    //printf(" refs: %d\n", t->refs);
    if (t->refs <= 0) {
        switch(t->tag) {
            case V_VAR:
            case V_SCALARVAR:
                break;
            case V_FUNC:
                free_stack_type(t->content.func_type.in);
                free_stack_type(t->content.func_type.out);
                break;
            case V_LIST:
                free_type(t->content.v);
                break;
            case V_UNIFIED:
                free_type(t->content.v);
                break;
            case V_PRODUCT:
                free_type(t->content.prod_type.left);
                free_type(t->content.prod_type.right);
                break;
            case V_ERROR:
                break;
        }
        free(t);
    }
}

void free_stack_type(stack_type *t) {
    if (t == NULL) return;
    t->refs --;
    if (t->refs <= 0) {
        switch(t->tag) {
            case S_VAR:
                break;
            case S_TOPTYPE:
                free_type(t->content.top_type.top);
                free_stack_type(t->content.top_type.rest);
                break;
            case S_UNIFIED:
                free_stack_type(t->content.unif);
                break;
        }
        free(t);
    }
}
