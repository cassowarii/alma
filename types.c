#include <stdio.h>
#include "alma.h"
#include "types.h"

/*
 * Right now, this is unused. This is copied over from the old version
 * which worked very differently. Currently I am going through and
 * commenting on each method to figure out the logic of this so that
 * I can plug it in to the new version instead without needing to
 * rewrite all this logic.
 */

void do_print_type(ATValueType *t);
void do_print_stack_type(ATStackType *t);
void do_string_type(char **s, ATValueType *t);
void do_string_stack_type(char **s, ATStackType *t);

char *string_type_nocopy(ATValueType *t);

long int typevar_id = 0;

/* This creates a new value representing a type variable.
 * A type variable represents a single type; all instances
 * of the same type variable represent the same type.
 * When displaying types, they are represented (in homage
 * to Standard ML) as an apostrophe followed by a lowercase
 * letter. ('a, 'b, 'c, etc.) */
ATValueType *new_typevar() {
    ATValueType *t = malloc(sizeof(ATValueType));
    t->refs = 0;
    t->id = typevar_id;
    typevar_id++;
    return t;
}

/* This creates a new value representing a stack variable.
 * This idea is completely stolen from Christopher Diggins's
 * "Cat" language type system. They are represented by apostrophes
 * followed by capital letters, which is indeed very exciting.
 * One of these appears on either side of a function and represents
 * a particular layout of the stack, where we don't much care which.
 * (For example, the type signature of `add` is ( 'A num num -> 'A num ),
 * and the type signature of `apply` is ( 'A [ 'A -> 'B ] -> 'B ). */
ATStackType *new_stackvar() {
    ATStackType *t = malloc(sizeof(ATStackType));
    t->refs = 0;
    t->id = typevar_id;
    typevar_id++;
    return t;
}

void set_type (elem_t *e, ATValueType *t) {
    e->type = t;
    t->refs++;
}

ATValueType *base_type(const char *name) {
    ATValueType *t = new_typevar();
    t->tag = basetype_vtype;
    t->content.type_name = malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(t->content.type_name, name);
    //printf("Created base type with name %s.\n", t->content.type_name);
    return t;
}

/* A type constructor for list literals. (Ideally in the future we will
 * have lists be built with type constructors etc but we don't have those
 * atm.) */
ATValueType *list_of(ATValueType *what) {
    if (what->tag != error_vtype) {
        ATValueType *t = new_typevar();
        what->refs ++;
        t->tag = list_vtype;
        t->content.v = what;
        return t;
    } else {
        return what;
    }
}

/* A type constructor for functions. */
ATValueType *func_type(ATStackType *from, ATStackType *to) {
    if (from->tag != error_stype && to->tag != error_stype) {
        ATValueType *t = new_typevar();
        from->refs ++;
        to->refs ++;
        t->tag = func_vtype;
        t->content.func_type.in = from;
        t->content.func_type.out = to;
        return t;
    } else {
        error *err;
        if (from->tag == error_stype) {
            err = from->content.err;
        } else {
            err = to->content.err;
        }
        free_stack_type(from);
        free_stack_type(to);
        return error_type(err);
    }
}

/* A type constructor for product types. */
ATValueType *product_type(ATValueType *left, ATValueType *right) {
    if (left->tag != error_vtype && right->tag != error_vtype) {
        ATValueType *t = new_typevar();
        left->refs ++;
        right->refs ++;
        t->tag = product_vtype;
        t->content.prod_type.left = left;
        t->content.prod_type.right = right;
        return t;
    } else if (left->tag == error_vtype) {
        free_type(right);
        return left;
    } else if (right->tag == error_vtype) {
        free_type(left);
        return right;
    }
}

ATValueType *type_var() {
    ATValueType *t = new_typevar();
    t->tag = var_vtype;
    t->content.var_name = '_'; // will get named after unification!
    return t;
}

ATValueType *error_type(error *e) {
    ATValueType *t = new_typevar();
    t->tag = error_vtype;
    t->content.err = e;
    return t;
}

ATValueType *named_type_var(char name) {
    ATValueType *t = new_typevar();
    t->tag = var_vtype;
    t->content.var_name = name;
    return t;
}

ATValueType *scalar_var() {
    ATValueType *t = new_typevar();
    t->tag = scalarvar_vtype;
    t->content.var_name = '_'; // will get named after unification!
    return t;
}

ATStackType *error_stacktype(error *e) {
    ATStackType *t = new_stackvar();
    t->tag = error_stype;
    t->content.err = e;
    return t;
}

ATStackType *stack_of(ATValueType *top, ATStackType *rest) {
    if (top->tag != error_vtype) {
        ATStackType *t = new_stackvar();
        t->tag = toptype_stype;
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

ATStackType *stack_var() {
    ATStackType *t = new_stackvar();
    t->tag = var_stype;
    t->content.var_name = '_';
    return t;
}

ATStackType *zero_stack() {
    ATStackType *t = new_stackvar();
    t->tag = zero_stype;
    return t;
}

ATStackType *named_stack_var(char name) {
    ATStackType *t = new_stackvar();
    t->tag = var_stype;
    t->id = typevar_id;
    typevar_id++;
    t->content.var_name = name;
    return t;
}

int compare_stack_types(ATStackType *a, ATStackType *b);

int compare_types(ATValueType *a, ATValueType *b) {
    while (a->tag == unified_vtype) { a = a->content.v; }
    while (b->tag == unified_vtype) { b = b->content.v; }
    if (a->tag != b->tag
            && !(a->tag == var_vtype && b->tag == scalarvar_vtype)
            && !(a->tag == scalarvar_vtype && b->tag == var_vtype)) {
        return 0;
    }
    switch(a->tag) {
        case var_vtype:
        case scalarvar_vtype:
            return (a->id == b->id);
        case basetype_vtype:
            return !(strcmp(a->content.type_name, b->content.type_name));
        case func_vtype:
            if (compare_stack_types(a->content.func_type.in, b->content.func_type.in)
                    && compare_stack_types(a->content.func_type.out, b->content.func_type.out)) {
                return 1;
            } else {
                return 0;
            }
        case product_vtype:
            if (compare_types(a->content.prod_type.left, b->content.prod_type.left)
                    && compare_types(a->content.prod_type.right, b->content.prod_type.right)) {
                return 1;
            } else {
                return 0;
            }
        case list_vtype:
            return compare_types(a->content.v, b->content.v);
    }
    return 1;
}

int compare_stack_types(ATStackType *a, ATStackType *b) {
    while (a->tag == unified_stype) { a = a->content.unif; }
    while (b->tag == unified_stype) { b = b->content.unif; }
    if (a->tag != b->tag) {
        return 0;
    }
    if (a->tag == toptype_stype) {
        return (compare_types(a->content.top_type.top, b->content.top_type.top)
            && compare_stack_types(a->content.top_type.rest, b->content.top_type.rest));
    }
    return 1;
}

int v_count_in_v(ATValueType *a, ATValueType *b);
int v_count_in_s(ATValueType *a, ATStackType *b);
int s_count_in_v(ATStackType *a, ATValueType *b);
int s_count_in_s(ATStackType *a, ATStackType *b);

int v_count_in_v(ATValueType *a, ATValueType *b) {
    while (a->tag == unified_vtype) { a = a->content.v; }
    while (b->tag == unified_vtype) { b = b->content.v; }
    if (a == b) {
        return 1;
    } else if (b->tag == func_vtype) {
        return v_count_in_s(a, b->content.func_type.in) + v_count_in_s(a, b->content.func_type.out);
    } else if (b->tag == list_vtype) {
        return v_count_in_v(a, b->content.v);
    } else if (b->tag == product_vtype) {
        return v_count_in_v(a, b->content.prod_type.left) + v_count_in_v(a, b->content.prod_type.right);
    } else {
        return 0;
    }
}

int v_count_in_s(ATValueType *a, ATStackType *b) {
    while (a->tag == unified_vtype) { a = a->content.v; }
    while (b->tag == unified_stype) { b = b->content.unif; }
    if (b->tag == toptype_stype) {
        if (b->content.top_type.top == a) {
            return 1 + v_count_in_s(a, b->content.top_type.rest);
        } else {
            return v_count_in_s(a, b->content.top_type.rest);
        }
    } else if (b->tag == var_stype) {
        return 0;
    } else {
        return 0;
    }
}

int s_count_in_v(ATStackType *a, ATValueType *b) {
    while (a->tag == unified_stype) { a = a->content.unif; }
    while (b->tag == unified_vtype) { b = b->content.v; }
    if (b->tag == func_vtype) {
        return s_count_in_s(a, b->content.func_type.in) + s_count_in_s(a, b->content.func_type.out);
    } else if (b->tag == list_vtype) {
        return s_count_in_v(a, b->content.v);
    } else if (b->tag == product_vtype) {
        return s_count_in_v(a, b->content.prod_type.left) + s_count_in_v(a, b->content.prod_type.right);
    } else {
        return 0;
    }
}

int s_count_in_s(ATStackType *a, ATStackType *b) {
    while (a->tag == unified_stype) { a = a->content.unif; }
    while (b->tag == unified_stype) { b = b->content.unif; }
    if (a == b) {
        return 1;
    } else if (b->tag == toptype_stype) {
        return s_count_in_v(a, b->content.top_type.top) + s_count_in_s(a, b->content.top_type.rest);
    } else {
        return 0;
    }
}

ATStackType *bottom_of_stack(ATStackType *t) {
    while (t->tag == unified_stype) { t = t->content.unif; }
    if (t->tag == toptype_stype) {
        return bottom_of_stack(t->content.top_type.rest);
    }
    return t;
}

void regeneralize(ATValueType **tpt);

void regeneralize_stack(ATStackType *t) {
    if (t->tag == toptype_stype) {
        regeneralize(&t->content.top_type.top);
        regeneralize_stack(t->content.top_type.rest);
    }
}

void regeneralize(ATValueType **tpt) {
    ATValueType *t = *tpt;
    if (t->tag == func_vtype) {
        regeneralize_stack(t->content.func_type.in);
        regeneralize_stack(t->content.func_type.out);
        ATStackType *lb = bottom_of_stack(t->content.func_type.in);
        ATStackType *rb = bottom_of_stack(t->content.func_type.out);
        if (lb == rb) {
            if (s_count_in_v(lb, t) == 2) {
                ATValueType *copy = copy_type(t);
                free_type(*tpt);
                *tpt = copy;
            }
        }
    } else {
        switch (t->tag) {
            case list_vtype:
                regeneralize(&t->content.v);
                break;
            case product_vtype:
                regeneralize(&t->content.prod_type.left);
                regeneralize(&t->content.prod_type.right);
                break;
            case unified_vtype:
                regeneralize(&t->content.v);
                break;
            case error_vtype:
            case basetype_vtype:
            case var_vtype:
            case scalarvar_vtype:
                // do nothing for these!
                break;
            case func_vtype:
                // how did you even get here?
                break;
        }
    }
}

ATStackType *unify_stack(ATStackType *a, ATStackType *b);

ATValueType *unify(ATValueType *a, ATValueType *b) {
    while (a->tag == unified_vtype) { a = a->content.v; }
    while (b->tag == unified_vtype) { b = b->content.v; }
#ifdef TYPEDEBUG
    printf("[U] %ld<%d> := %ld<%d>; ", a->id, a->tag, b->id, b->tag);
    do_print_type(a); printf("; "); do_print_type(b); printf("; ");
#endif
    if (a->tag == var_vtype || a->tag == scalarvar_vtype) {
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
            a->tag = unified_vtype;
            a->content.v = b;
            b->refs ++;
        }
        return b;
    } else if (b->tag == var_vtype || b->tag == scalarvar_vtype) {
        return unify(b, a);
    } else if (a->tag == func_vtype && b->tag == func_vtype) {
#ifdef TYPEDEBUG
        printf("Unify func-type %ld with %ld\n", a->id, b->id);
#endif
        ATStackType *s_in = unify_stack(a->content.func_type.in, b->content.func_type.in);
        if (s_in->tag == error_stype) {
            error *e = s_in->content.err;
            free_stack_type(s_in);
            return error_type(e);
        }
        ATStackType *s_out = unify_stack(a->content.func_type.out, b->content.func_type.out);
        if (s_out->tag == error_stype) {
            error *e = s_out->content.err;
            free_stack_type(s_out);
            return error_type(e);
        }
        return b;
    } else if (a->tag == list_vtype && b->tag == list_vtype) {
#ifdef TYPEDEBUG
        printf("Unify list-type %ld with %ld\n", a->id, b->id);
#endif
        return unify(a->content.v, b->content.v);
    } else if (a->tag == product_vtype && b->tag == product_vtype) {
        ATValueType *v_left = unify(a->content.prod_type.left, b->content.prod_type.left);
        if (v_left->tag == error_vtype) {
            return v_left;
        }
        ATValueType *v_right = unify(a->content.prod_type.right, b->content.prod_type.right);
        if (v_right->tag == error_vtype) {
            return v_right;
        }
        return b;
    } else if (a->tag == basetype_vtype && b->tag == basetype_vtype) {
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

ATStackType *unify_stack(ATStackType *a, ATStackType *b) {
    while (a->tag == unified_stype) { a = a->content.unif; }
    while (b->tag == unified_stype) { b = b->content.unif; }
    if (a->tag == var_stype) {
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
            a->tag = unified_stype;
            a->content.unif = b;
            b->refs ++;
        }
        return b;
    } else if (b->tag == var_stype) {
        return unify_stack(b, a);
    } else if (a->tag == zero_stype) {
#ifdef TYPEDEBUG
        printf("Unify 0-stack %ld with %ld\n", a->id, b->id);
#endif
        if (b->tag == zero_stype || b->tag == var_stype) {
            return b;
        } else {
            char *sa = string_stack_type(a);
            char *sb = string_stack_type(b);
            error *e = error_msg("* Couldn't unify types: %s and %s", sa, sb);
            free(sa);
            free(sb);
            return error_stacktype(e);
        }
    } else if (b->tag == zero_stype) {
        return unify_stack(b, a);
    } else if (a->tag == toptype_stype) {
#ifdef TYPEDEBUG
        printf("Unify tt-stack %ld with %ld\n", a->id, b->id);
#endif
        ATValueType *x = unify(a->content.top_type.top, b->content.top_type.top);
        ATStackType *result;
        if (x->tag != error_vtype) {
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

int confirm_stack_type(ATStackType *t, elem_t *e) {
    while (t->tag == unified_stype) { t = t->content.unif; }
    if (t->tag == var_stype) {
        return 1;
    }
    if (t->tag == toptype_stype) {
        if (unify(t->content.top_type.top, e->type)) {
            return confirm_stack_type(t->content.top_type.rest, e->next);
        } else {
            return 0;
        }
    }
}

ATValueType *get_list_type(elem_t *e) {
    if (e == NULL) return NULL;
    ATValueType *below_type = get_list_type(e->next);
    if (below_type == NULL) {
        return copy_type(e->type);
    } else if (below_type->tag == error_vtype) {
        // mismatch below -- throw error up the stack
        return below_type;
    } else {
        ATValueType *check = copy_type(e->type);
        ATValueType *u = unify(below_type, check);
        if (u->tag != error_vtype) {
            free_type(check);
            return below_type;
        } else {
            free_type(check);
            // first mismatch
            ATValueType *err = error_type(error_msg("Mismatched types in list: %s and %s", string_type(below_type), string_type(e->type)));
            return err;
        }
    }
}

// Ensure all values on top of a stack variable
// are the same. Used to check lists added inside
// of blocks.
// (Basically the same as get_list_type, but up
// a level of abstraction.)
ATValueType *get_abstract_list_type(ATStackType *t, int line_num) {
    if (t == NULL) return NULL;
    while (t->tag == unified_stype) { t = t->content.unif; }
    if (t->tag == var_stype) return NULL;
    ATValueType *below_type = get_abstract_list_type(t->content.top_type.rest, line_num);
    if (below_type == NULL) {
        return t->content.top_type.top;
    } else {
        if (below_type->tag == error_vtype) {
            // mismatch below -- throw error up the stack
            return below_type;
        } else {
            ATValueType *u = unify(below_type, t->content.top_type.top);
            if (u->tag != error_vtype) {
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

ATStackType *do_stacktype_copy(ATStackType *t, type_mapping **map);

ATValueType *do_type_copy(ATValueType *t, type_mapping **map) {
    if (t == NULL) return NULL;
    switch(t->tag) {
        case basetype_vtype:
            return t;
        case var_vtype:
        case scalarvar_vtype:;
            type_mapping *v = NULL;
            HASH_FIND_PTR(*map, &t, v);
            if (!v) {
                v = malloc(sizeof(type_mapping));
                v->in = t;
                if (t->tag == var_vtype) {
                    v->out = type_var();
                } else {
                    v->out = scalar_var();
                }
                HASH_ADD_PTR(*map, in, v);
            }
            return v->out;
        case func_vtype:
            return func_type(do_stacktype_copy(t->content.func_type.in, map), do_stacktype_copy(t->content.func_type.out, map));
        case list_vtype:
            return list_of(do_type_copy(t->content.v, map));
        case product_vtype:
            return product_type(do_type_copy(t->content.prod_type.left, map), do_type_copy(t->content.prod_type.right, map));
        case unified_vtype:
            return do_type_copy(t->content.v, map);
        case error_vtype:
            return t;
    }
}

ATStackType *do_stacktype_copy(ATStackType *t, type_mapping **map) {
    if (t == NULL) return NULL;
    switch(t->tag) {
        case var_stype:;
            type_mapping *v = NULL;
            HASH_FIND_PTR(*map, &t, v);
            if (!v) {
                v = malloc(sizeof(type_mapping));
                v->in = t;
                v->out = stack_var();
                HASH_ADD_PTR(*map, in, v);
            }
            return v->out;
        case toptype_stype:
            return stack_of(do_type_copy(t->content.top_type.top, map), do_stacktype_copy(t->content.top_type.rest, map));
        case zero_stype:
            return zero_stack();
        case unified_stype:
            return do_stacktype_copy(t->content.unif, map);
        case error_stype:
            return t;
    }
}

ATValueType *copy_type(ATValueType *t) {
    type_mapping *map = NULL;
    ATValueType *c = do_type_copy(t, &map);
    type_mapping *curr, *tmp;
    HASH_ITER(hh, map, curr, tmp) {
        HASH_DEL(map, curr);
        free(curr);
    }
    return c;
}

ATStackType *copy_stack_type(ATStackType *t) {
    type_mapping *map = NULL;
    ATStackType *c = do_stacktype_copy(t, &map);
    type_mapping *curr, *tmp;
    HASH_ITER(hh, map, curr, tmp) {
        HASH_DEL(map, curr);
        free(curr);
    }
    return c;
}

ATValueType *infer_type(node_t *nn) {
    ATStackType *X;
    lib_entry_t *e;
    ATValueType *l, *r, *result;
    ATStackType *ok;
    if (nn == NULL) {
        // an empty node is the identity function...
        X = stack_var();
        return func_type(X, X);
    }
    switch (nn->tag) {
        case N_COMPOSED:
            if (right(nn)) {
                l = infer_type(left(nn));
                if (l->tag == error_vtype) {
                    result = l;
                    break;
                }
                r = infer_type(right(nn));
                if (r->tag == error_vtype) {
                    result = r;
                    free_type(l);
                    break;
                }
                ok = unify_stack(l->content.func_type.out, r->content.func_type.in);
                if (ok->tag != error_stype) {
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
            if (result->tag == error_vtype) {
                add_info(result->content.err, "while evaluating a block starting at line %d", nn->line);
            }
            break;
        case N_LIST:
            X = stack_var();
            l = infer_type(left(nn));
            if (l->tag == error_vtype) {
                result = l;
                add_info(result->content.err, "while evaluating a list starting at line %d", nn->line);
                break;
            }
            ATValueType *ltype = get_abstract_list_type(l->content.func_type.out, nn->line);
            ATValueType *list;
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
                if (e->type->tag != error_vtype) {
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

void stack_give_names(ATStackType *t, char *current_v, char *current_s);

void give_names(ATValueType *t, char *current_v, char *current_s) {
    if (t == NULL) return;
    switch(t->tag) {
        case var_vtype:
        case scalarvar_vtype:
            if (t->content.var_name == '_') {
                //printf("Name var %p \"%c.\"\n", t, *current_v);
                t->content.var_name = *current_v;
                (*current_v)++;
            }
            return;
        case func_vtype:
            //printf("FUNC %p name inputs\n", t);
            stack_give_names(t->content.func_type.in, current_v, current_s);
            //printf("FUNC %p name outputs\n", t);
            stack_give_names(t->content.func_type.out, current_v, current_s);
            return;
        case list_vtype:
            //printf("LIST %p name innards\n", t);
            give_names(t->content.v, current_v, current_s);
            return;
        case product_vtype:
            give_names(t->content.prod_type.left, current_v, current_s);
            give_names(t->content.prod_type.right, current_v, current_s);
            return;
        case basetype_vtype:
            //printf("BASETYPE %p go away\n", t);
            // do nothing
            return;
        case unified_vtype:
            give_names(t->content.v, current_v, current_s);
            return;
        case error_vtype:
            return;
        default:
            fprintf(stderr, "Can't name type of type %d!", t->tag);
    }
}

void stack_give_names(ATStackType *t, char *current_v, char *current_s) {
    if (t == NULL) return;
    switch(t->tag) {
        case toptype_stype:
            //printf("STACK %p TOP name\n", t);
            give_names(t->content.top_type.top, current_v, current_s);
            //printf("STACK %p REST name\n", t);
            stack_give_names(t->content.top_type.rest, current_v, current_s);
            return;
        case var_stype:
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
        case unified_stype:
            stack_give_names(t->content.unif, current_v, current_s);
            return;
        case zero_stype:
        case error_stype:
            return;
    }
}

void bestow_varnames (ATValueType *t) {
    char cv = 'a';
    char cs = 'Z';
    give_names(t, &cv, &cs);
}

void init_types() {
    //printf("INITIALIZING TYPES\n");
    vt_num = base_type("num");
    vt_char = base_type("char");
    vt_bool = base_type("bool");
    /*ATStackType *X = named_stack_var('X');
    print_type(func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X)));
    ATValueType *a = named_type_var('a');
    ATValueType *b = named_type_var('b');
    ATStackType *X2 = named_stack_var('X');
    print_type(func_type(stack_of(a, stack_of(b, X2)), stack_of(b, stack_of(a, X2))));*/
}

void print_type(ATValueType *t) {
    char *s = string_type(t);
    printf("%s\n", s);
    free(s);
}

void do_print_type(ATValueType *t) {
    char *s = string_type(t);
    printf("%s", s);
    free(s);
}

void print_stack_type(ATStackType *t) {
    char *s = string_stack_type(t);
    printf("%s\n", s);
    free(s);
}

char *string_type_nocopy(ATValueType *t) {
    ATValueType *x = t;
    bestow_varnames(x);
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_type(&s, x);
    return s;
}

char *string_type(ATValueType *t) {
#ifndef TYPEDEBUG
    ATValueType *x = copy_type(t);
    x->refs ++;
#else
    ATValueType *x = t;
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

char *string_stack_type(ATStackType *t) {
    ATStackType *x = copy_stack_type(t);
    char cv = 'a';
    char cs = 'Z';
    stack_give_names(x, &cv, &cs);
    char *s = malloc(1);
    strcpy(s, "\0");
    do_string_stack_type(&s, x);
    free_stack_type(x);
    return s;
}

void do_string_type(char **s, ATValueType *t) {
    if (t == NULL) return;
    char *tmp = NULL;
    switch(t->tag) {
        case scalarvar_vtype:
        case var_vtype:
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
        case func_vtype:
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
        case list_vtype:
#if defined(TYPEDEBUG) || defined(DIVDEBUG)
            asprintf(&tmp, "%ld{", t->id);
            rstrcat(s, tmp);
#else
            rstrcat(s, "{");
#endif
            do_string_type(s, t->content.v);
            rstrcat(s, "}");
            break;
        case product_vtype:
            do_string_type(s, t->content.prod_type.left);
            rstrcat(s, " * ");
            do_string_type(s, t->content.prod_type.right);
            break;
        case basetype_vtype:
            rstrcat(s, t->content.type_name);
            break;
        case unified_vtype:
            do_string_type(s, t->content.v);
            break;
        case error_vtype:
            printf("<error>");
            break;
        default:
            printf("???");
    }
    free(tmp);
}

void do_print_stack_type(ATStackType *t) {
}

void do_string_stack_type(char **s, ATStackType *t) {
    char *tmp = NULL;
    switch(t->tag) {
        case var_stype:
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
        case toptype_stype:
#if defined(TYPEDEBUG) || defined(DIVDEBUG)
            asprintf(&tmp, "%ld ", t->id);
            rstrcat(s, tmp);
#endif
            do_string_type(s, t->content.top_type.top);
            rstrcat(s, " ");
            do_string_stack_type(s, t->content.top_type.rest);
            break;
        case zero_stype:
            rstrcat(s, "'0");
            break;
        case unified_stype:
            do_string_stack_type(s, t->content.unif);
            break;
        case error_stype:
            printf("<error>");
            break;
        default:
            printf("???stack");
    }
    free(tmp);
}

void free_type(ATValueType *t) {
    if (t == NULL) return;
    //printf("Free type of tag %d @ %p.", t->tag, t);
    if (t->tag == basetype_vtype) return;
    t->refs --;
    //printf(" refs: %d\n", t->refs);
    if (t->refs <= 0) {
        switch(t->tag) {
            case var_vtype:
            case scalarvar_vtype:
                break;
            case func_vtype:
                free_stack_type(t->content.func_type.in);
                free_stack_type(t->content.func_type.out);
                break;
            case list_vtype:
                free_type(t->content.v);
                break;
            case unified_vtype:
                free_type(t->content.v);
                break;
            case product_vtype:
                free_type(t->content.prod_type.left);
                free_type(t->content.prod_type.right);
                break;
            case error_vtype:
                break;
        }
        free(t);
    }
}

void free_stack_type(ATStackType *t) {
    if (t == NULL) return;
    t->refs --;
    if (t->refs <= 0) {
        switch(t->tag) {
            case var_stype:
                break;
            case toptype_stype:
                free_type(t->content.top_type.top);
                free_stack_type(t->content.top_type.rest);
                break;
            case unified_stype:
                free_stack_type(t->content.unif);
                break;
        }
        free(t);
    }
}
