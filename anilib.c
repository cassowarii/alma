#include <stdio.h>
#include <ctype.h>
#include "cloth.h"

elem_t *pop(elem_t **top) {
    elem_t *res = *top;
    *top = (*top)->next;
    if (*top != NULL) {
        (*top)->prev = NULL;
    }
    res->next = NULL;
    return res;
}

value_type *type_pop() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(a, X), X);
}

elem_t *word_pop(elem_t **top) {
    //*top = (*top)->next;
    pop(top);
    return NULL;
}

value_type *type_print() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(a, X), X);
}

elem_t *word_print(elem_t **top) {
    elem_t *e = pop(top);
    print_elem(e);
    free_elem(e);
    printf("\n");
    return NULL;
}

value_type *type_list() {
    stack_type *X = stack_var();
    return func_type(X, X);
}

elem_t *word_list(elem_t **top) {
    do_list(top);
    printf("\n");
    return NULL;
}

void do_list(elem_t **top) {
    elem_t *elem = *top;
    while (elem) {
        repr_elem(elem);
        if (elem->next != NULL) {
            printf(" ; ");
        }
        elem = elem->next;
    }
}

value_type *type_copy() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(a, X), stack_of(a, stack_of(a, X)));
}

elem_t *word_copy(elem_t **top) {
    elem_t *elem = *top;
    elem_t *clone = clone_elem(elem);
    return clone;
}

value_type *type_swap() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    value_type *b = type_var();
    return func_type(stack_of(a, stack_of(b, X)), stack_of(b, stack_of(a, X)));
}

elem_t *word_swap(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    push(e_b, &e_a);
    return e_a;
}

value_type *type_pair() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    value_type *b = type_var();
    return func_type(stack_of(a, stack_of(b, X)), stack_of(product_type(a, b), X));
}

elem_t *word_pair(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    return elem_product(e_a, e_b);
}

value_type *type_cons() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(a, stack_of(list_of(a), X)), stack_of(list_of(a), X));
}

elem_t *word_cons(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_l = pop(top);
    push(e_a, &e_l->content.list);
    return e_l;
}

value_type *type_map() {
    stack_type *X = zero_stack();
    stack_type *Y = stack_var();
    value_type *a = type_var();
    value_type *b = type_var();
    return func_type(                                               // {
            stack_of(func_type(stack_of(a, X), stack_of(b, X)),     //   { 'a '0 → 'b '0 }
                stack_of(list_of(a), Y)),                           //   [ 'a ] 'Y →
            stack_of(list_of(b), Y));                               //   [ 'b ] 'Y }
}

elem_t *do_map(node_t *func, elem_t **rest) {
    if (rest == NULL || *rest == NULL) return NULL; // base case
    elem_t *stack = NULL;
    elem_t *top = pop(rest);
    push(top, &stack);
    eval(func, &stack);
    elem_t *result = pop(&stack);
    // get rid of any extra elements loitering around
    free_elems_below(stack);
    elem_t *mapped_rest = do_map(func, rest);
    push(result, &mapped_rest);
    return mapped_rest;
}

elem_t *word_map(elem_t **top) {
    elem_t *e_func = pop(top);
    elem_t *e_list = pop(top);
    elem_t *e_result = do_map(e_func->content.block, &e_list->content.list);
    free_elem(e_func);
    free_elem(e_list);
    return elem_list(e_result);
}

value_type *type_outer() {
    stack_type *X = stack_var();
    stack_type *Y = stack_var();
    value_type *a = type_var();
    value_type *b = type_var();
    value_type *c = type_var();
    return func_type(                                           // {
            stack_of(func_type(stack_of(a, stack_of(b, X)),     //   { 'a 'b 'X →
                    stack_of(c, X)),                            //        'c 'X }
                stack_of(list_of(a), stack_of(list_of(b), Y))), //   [ 'a ] [ 'b ] 'Y →
            stack_of(list_of(list_of(c)), Y));                  //   [ [ 'c ] ] 'Y }
}

elem_t *word_outer(elem_t **top) {
    // not implemented yet...
}

value_type *type_apply() {
    stack_type *X = stack_var();
    stack_type *Y = stack_var();
    return func_type(stack_of(func_type(X, Y), X), Y);
}

elem_t *word_apply(elem_t **top) {
    elem_t *block = pop(top);
    eval(block->content.block, top);
    free_elem(block);
    return NULL;
}

value_type *type_PLUS() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X));
}

elem_t *word_PLUS(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    if (e_a->tag == E_INT && e_b->tag == E_INT) {
        int a = e_a->content.e_int;
        int b = e_b->content.e_int;
        free_elem(e_a);
        free_elem(e_b);
        int result = a + b;
        return elem_int(result);
    } else {
        double a;
        if (e_a->tag == E_INT) {
            a = (double)e_a->content.e_int;
        } else {
            a = e_a->content.e_float;
        }
        free_elem(e_a);
        double b;
        if (e_b->tag == E_INT) {
            b = (double)e_b->content.e_int;
        } else {
            b = e_b->content.e_float;
        }
        free_elem(e_b);
        double result = a + b;
        return elem_float(result);
    }
}

value_type *type_MINUS() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X));
}

elem_t *word_MINUS(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    if (e_a->tag == E_INT && e_b->tag == E_INT) {
        int a = e_a->content.e_int;
        int b = e_b->content.e_int;
        free_elem(e_a);
        free_elem(e_b);
        int result = a - b;
        return elem_int(result);
    } else {
        double a;
        if (e_a->tag == E_INT) {
            a = (double)e_a->content.e_int;
        } else {
            a = e_a->content.e_float;
        }
        free_elem(e_a);
        double b;
        if (e_b->tag == E_INT) {
            b = (double)e_b->content.e_int;
        } else {
            b = e_b->content.e_float;
        }
        free_elem(e_b);
        double result = a - b;
        return elem_float(result);
    }
}

value_type *type_TIMES() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X));
}

elem_t *word_TIMES(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    if (e_a->tag == E_INT && e_b->tag == E_INT) {
        int a = e_a->content.e_int;
        int b = e_b->content.e_int;
        free_elem(e_a);
        free_elem(e_b);
        int result = a * b;
        return elem_int(result);
    } else {
        double a;
        if (e_a->tag == E_INT) {
            a = (double)e_a->content.e_int;
        } else {
            a = e_a->content.e_float;
        }
        free_elem(e_a);
        double b;
        if (e_b->tag == E_INT) {
            b = (double)e_b->content.e_int;
        } else {
            b = e_b->content.e_float;
        }
        free_elem(e_b);
        double result = a * b;
        return elem_float(result);
    }
}

value_type *type_FLDIV() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X));
}

elem_t *word_FLDIV(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    double a;
    if (e_a->tag == E_INT) {
        a = (double)e_a->content.e_int;
    } else {
        a = e_a->content.e_float;
    }
    free_elem(e_a);
    double b;
    if (e_b->tag == E_INT) {
        b = (double)e_b->content.e_int;
    } else {
        b = e_b->content.e_float;
    }
    free_elem(e_b);
    double result = a / b;
    return elem_float(result);
}

value_type *type_INTDIV() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X));
}

elem_t *word_INTDIV(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    if (e_a->tag == E_INT && e_b->tag == E_INT) {
        int a = e_a->content.e_int;
        int b = e_b->content.e_int;
        free_elem(e_a);
        free_elem(e_b);
        int result = a / b;
        return elem_int(result);
    } else {
        double a;
        if (e_a->tag == E_INT) {
            a = (double)e_a->content.e_int;
        } else {
            a = e_a->content.e_float;
        }
        free_elem(e_a);
        double b;
        if (e_b->tag == E_INT) {
            b = (double)e_b->content.e_int;
        } else {
            b = e_b->content.e_float;
        }
        free_elem(e_b);
        int result = a / b;
        return elem_float(result);
    }
}

value_type *type_MOD() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_num, stack_of(vt_num, X)), stack_of(vt_num, X));
}

elem_t *word_MOD(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    if (e_a->tag == E_INT && e_b->tag == E_INT) {
        int a = e_a->content.e_int;
        int b = e_b->content.e_int;
        free_elem(e_a);
        free_elem(e_b);
        int result = a % b;
        return elem_int(result);
    } else {
        double a;
        if (e_a->tag == E_INT) {
            a = (double)e_a->content.e_int;
        } else {
            a = e_a->content.e_float;
        }
        free_elem(e_a);
        double b;
        if (e_b->tag == E_INT) {
            b = (double)e_b->content.e_int;
        } else {
            b = e_b->content.e_float;
        }
        free_elem(e_b);
        //int result = a % b;
        while (a > b) {
            a -= b;
        }
        return elem_float(a);
    }
}

value_type *type_select() {
    stack_type *X = stack_var();
    value_type *a = scalar_var();
    return func_type(stack_of(vt_num, stack_of(a, X)), stack_of(a, X));
}

elem_t *word_select(elem_t **top) {
    elem_t *selector = pop(top);
    elem_t *selected = pop(top);
    if (truthy(selector)) {
        free_elem(selector);
        return selected;
    } else {
        free_elem(selector);
        free_elem(selected);
        return NULL;
    }
}

value_type *type_EQUAL() {
    stack_type *X = stack_var();
    value_type *a = scalar_var();
    return func_type(stack_of(a, stack_of(a, X)), stack_of(vt_num, X));
}

elem_t *word_EQUAL(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    elem_t *result = NULL;
    if (e_a->tag == E_INT && e_b->tag == E_FLOAT) {
        int a = e_a->content.e_int;
        double b = e_b->content.e_float;
        result = elem_int((double)a == b ? 1 : 0);
    } else if (e_a->tag == E_FLOAT && e_b->tag == E_INT) {
        double a = e_a->content.e_float;
        int b = e_b->content.e_int;
        result = elem_int(a == (double)b ? 1 : 0);
    } else if (e_a->tag != e_b->tag) {
        result = elem_int(0);
    } else {
        // tags equal
        if (e_a->tag == E_INT) {
            int a = e_a->content.e_int;
            int b = e_b->content.e_int;
            result = elem_int(a == b ? 1 : 0);
        } else if (e_a->tag == E_FLOAT) {
            double a = e_a->content.e_float;
            double b = e_b->content.e_float;
            result = elem_int(a == b ? 1 : 0);
        } else if (e_a->tag == E_CHAR) {
            char a = e_a->content.e_char;
            char b = e_b->content.e_char;
            result = elem_int(a == b ? 1 : 0);
        } else if (e_a->tag == E_BLOCK) {
            fprintf(stderr, "block comparison unimplemented\n");
        } else {
            fprintf(stderr, "unknown comparison type");
        }
    }
    free_elem(e_a);
    free_elem(e_b);
    return result;
}

value_type *type_upper() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_char, X), stack_of(vt_char, X));
}

elem_t *word_upper(elem_t **top) {
    elem_t *e_c = pop(top);
    char c = e_c->content.e_char;
    free_elem(e_c);
    return elem_char(toupper(c));
}

value_type *type_lower() {
    stack_type *X = stack_var();
    return func_type(stack_of(vt_char, X), stack_of(vt_char, X));
}

elem_t *word_lower(elem_t **top) {
    elem_t *e_c = pop(top);
    char c = e_c->content.e_char;
    free_elem(e_c);
    return elem_char(tolower(c));
}

lib_entry_t *create_entry() {
    lib_entry_t *e = malloc(sizeof(lib_entry_t));
    e->name = NULL;
    e->type = NULL;
    e->func = NULL;
}

lib_entry_t *construct(const char *name, value_type *type, Word *func) {
    lib_entry_t *e = create_entry();
    e->name = malloc((strlen(name) + 1) * sizeof(char));
    strcpy(e->name, name);
    e->type = type;
    e->func = func;
    return e;
}

void add_lib_entry(library *l, lib_entry_t *entry) {
    HASH_ADD_KEYPTR (hh, l->table, entry->name, strlen(entry->name), entry);
}

void init_library(library *l) {
    l->table = NULL;
    add_lib_entry(l, construct("pop",       type_pop(),      &word_pop));
    add_lib_entry(l, construct("print",     type_print(),    &word_print));
    add_lib_entry(l, construct("list",      type_list(),     &word_list));
    add_lib_entry(l, construct("copy",      type_copy(),     &word_copy));
    add_lib_entry(l, construct("swap",      type_swap(),     &word_swap));
    add_lib_entry(l, construct("map",       type_map(),      &word_map));
    add_lib_entry(l, construct("outer",     type_outer(),    &word_outer));
    add_lib_entry(l, construct("apply",     type_apply(),    &word_apply));
    add_lib_entry(l, construct("pair",      type_pair(),     &word_pair));
    add_lib_entry(l, construct("cons",      type_cons(),     &word_cons));
    add_lib_entry(l, construct("+",         type_PLUS(),     &word_PLUS));
    add_lib_entry(l, construct("-",         type_MINUS(),    &word_MINUS));
    add_lib_entry(l, construct("*",         type_TIMES(),    &word_TIMES));
    add_lib_entry(l, construct("/",         type_FLDIV(),    &word_FLDIV));
    add_lib_entry(l, construct("div",       type_INTDIV(),   &word_INTDIV));
    add_lib_entry(l, construct("%",         type_MOD(),      &word_MOD));
    add_lib_entry(l, construct("select",    type_select(),   &word_select));
    add_lib_entry(l, construct("upper",     type_upper(),    &word_upper));
    add_lib_entry(l, construct("lower",     type_lower(),    &word_lower));
    add_lib_entry(l, construct("=",         type_EQUAL(),    &word_EQUAL));
}
