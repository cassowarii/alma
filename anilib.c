#include <stdio.h>
#include <ctype.h>
#include "alma.h"

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
    return NULL;
}

value_type *type_println() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(a, X), X);
}

elem_t *word_println(elem_t **top) {
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
        print_elem(elem);
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
    elem_t *clone = copy_elem(elem);
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

value_type *type_unpair() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    value_type *b = type_var();
    return func_type(stack_of(product_type(a, b), X), stack_of(a, stack_of(b, X)));
}

elem_t *word_unpair(elem_t **top) {
    elem_t *e = pop(top);
    elem_t *l = e->content.product.left;
    elem_t *r = e->content.product.right;
    e->content.product.left = NULL;
    e->content.product.right = NULL;
    free_elem(e);
    push(l, &r);
    return r;
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

value_type *type_split() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(list_of(a), X), stack_of(a, stack_of(list_of(a), X)));
}

elem_t *word_split(elem_t **top) {
    // TODO this needs to handle empty lists
    elem_t *e_l = pop(top);
    elem_t *e_a = pop(&e_l->content.list);
    push(e_a, &e_l);
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

/* These all have exactly the same implementation as `apply.` They just
 * have more restrictive types. */

value_type *type_0to0() {
    stack_type *X = stack_var();
    return func_type(stack_of(func_type(zero_stack(), zero_stack()), X), X);
}

value_type *type_0to1() {
    value_type *a = type_var();
    stack_type *X = stack_var();
    return func_type(stack_of(func_type(zero_stack(), stack_of(a, zero_stack())), X), stack_of(a, X));
}

value_type *type_1more() {
    value_type *a = type_var();
    stack_type *X = stack_var();
    stack_type *Y = stack_var();
    return func_type(stack_of(func_type(X, stack_of(a, X)), Y), stack_of(a, Y));
}

value_type *type_dip() {
    stack_type *X = stack_var();
    stack_type *Y = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(func_type(X, Y), stack_of(a, X)), stack_of(a, Y));
}

elem_t *word_dip(elem_t **top) {
    elem_t *block = pop(top);
    elem_t *saved = pop(top);
    eval(block->content.block, top);
    free_elem(block);
    return saved;
}

value_type *type_curry() {
    stack_type *X = stack_var();
    stack_type *Y = stack_var();
    stack_type *Z = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(func_type(stack_of(a, X), Y),     // { { 'a X → 'Y }
                stack_of(a, Z)), stack_of(func_type(X, Y), Z)); //   'a 'Z → { 'X → 'Y } 'Z }
}

elem_t *word_curry(elem_t **top) {
    elem_t *block = pop(top);
    elem_t *param = pop(top);
    node_t *funcnode = copy_node(block->content.block);
    free_elem(block);
    elem_t *newparam = copy_elem(param);
    free_elem(param);
    node_t *newnode = _node_lineno(N_COMPOSED, node_elem(newparam, -2), funcnode, -2);
    newnode->flags |= NF_COPIED;
    left(newnode)->flags |= NF_COPIED;
    elem_t *e = new_elem();
    e->tag = E_BLOCK;
    e->content.block = newnode;
    set_type(e, infer_type(e->content.block));
    /*printf("BLOCK TYPE: <@ %p> ", block->type);
    print_type(block->type);
    printf("\t(w/ refs %d)\n", block->type->refs);*/
    return e;
}

value_type *type_typeof() {
    stack_type *X = stack_var();
    value_type *a = type_var();
    return func_type(stack_of(a, X), stack_of(list_of(vt_char), X));
}

elem_t *word_typeof(elem_t **top) {
    elem_t *e_a = pop(top);
    char *s = string_type(e_a->type);
    elem_t *str = elem_str(s);
    free_elem(e_a);
    free(s);
    return str;
}

value_type *type_if() {
    stack_type *X = stack_var();
    stack_type *Y = stack_var();
    stack_type *Z = stack_var();
    //value_type *a = type_var();
    return func_type(stack_of(func_type(X, stack_of(vt_bool, Y)),       // { { 'X → bool 'Y }
                stack_of(func_type(Y, Z), stack_of(func_type(Y, Z),     //   { 'Y → 'Z } { 'Y → 'Z }
                        X))), Z);                                       //   'X → 'Z }
}

elem_t *word_if(elem_t **top) {
    elem_t *ifpart = pop(top);
    elem_t *thenpart = pop(top);
    elem_t *elsepart = pop(top);
    eval(ifpart->content.block, top);
    free_elem(ifpart);
    elem_t *cond = pop(top);
    if (truthy(cond)) {
        eval(thenpart->content.block, top);
    } else {
        eval(elsepart->content.block, top);
    }
    free_elem(thenpart);
    free_elem(elsepart);
    return NULL;
}

value_type *type_while() {
    stack_type *X = stack_var();
    stack_type *Y = stack_var();
    return func_type(stack_of(func_type(X, stack_of(vt_bool, Y)),       // { { 'X → bool 'Y }
                stack_of(func_type(Y, X), X)), X);                      //   { 'Y → 'X } 'X → 'X }
}

elem_t *word_while(elem_t **top) {
    elem_t *ifpart = pop(top);
    elem_t *thenpart = pop(top);
    eval(ifpart->content.block, top);
    elem_t *cond = pop(top);
    while (truthy(cond)) {
        free_elem(cond);
        eval(thenpart->content.block, top);
        eval(ifpart->content.block, top);
        cond = pop(top);
    }
    free_elem(cond);
    free_elem(ifpart);
    free_elem(thenpart);
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
    // kind of goes backwards so that apply [ - 1 ] 5 => 4
    elem_t *e_b = pop(top);
    elem_t *e_a = pop(top);
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
    // backwards!!
    elem_t *e_b = pop(top);
    elem_t *e_a = pop(top);
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
    // BACKWARDS
    elem_t *e_b = pop(top);
    elem_t *e_a = pop(top);
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
    // BACKWARDS!!
    elem_t *e_b = pop(top);
    elem_t *e_a = pop(top);
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
    return func_type(stack_of(a, stack_of(a, X)), stack_of(vt_bool, X));
}

int compare_elems(elem_t *e_a, elem_t *e_b);
int compare_lists(elem_t *e_a, elem_t *e_b);

elem_t *word_EQUAL(elem_t **top) {
    elem_t *e_a = pop(top);
    elem_t *e_b = pop(top);
    elem_t *result = elem_bool(compare_elems(e_a, e_b));
    return result;
}

int compare_elems(elem_t *e_a, elem_t *e_b) {
    if (e_a->tag == E_INT && e_b->tag == E_FLOAT) {
        int a = e_a->content.e_int;
        double b = e_b->content.e_float;
        return (double)a == b ? 1 : 0;
    } else if (e_a->tag == E_FLOAT && e_b->tag == E_INT) {
        double a = e_a->content.e_float;
        int b = e_b->content.e_int;
        return a == (double)b ? 1 : 0;
    } else if (e_a->tag != e_b->tag) {
        return 0;
    } else {
        // tags equal
        if (e_a->tag == E_INT) {
            int a = e_a->content.e_int;
            int b = e_b->content.e_int;
            return a == b ? 1 : 0;
        } else if (e_a->tag == E_BOOL) {
            return (e_a->content.e_int == e_b->content.e_int);
        } else if (e_a->tag == E_FLOAT) {
            double a = e_a->content.e_float;
            double b = e_b->content.e_float;
            return a == b ? 1 : 0;
        } else if (e_a->tag == E_CHAR) {
            char a = e_a->content.e_char;
            char b = e_b->content.e_char;
            return a == b ? 1 : 0;
        } else if (e_a->tag == E_LIST) {
            return compare_lists(e_a->content.list, e_b->content.list);
        } else if (e_a->tag == E_BLOCK) {
            fprintf(stderr, "block comparison unimplemented\n");
            return 0;
        } else {
            fprintf(stderr, "unknown comparison type");
            return 0;
        }
    }
}

int compare_lists(elem_t *e_a, elem_t *e_b) {
    if (e_a == NULL && e_b == NULL) {
        return 1;
    } else if (e_a == NULL || e_b == NULL) {
        return 0;
    } else {
        return compare_elems(e_a, e_b) && compare_lists(e_a->next, e_b->next);
    }
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
    e->internal = 0;
}

lib_entry_t *construct(const char *name, value_type *type, Word *func) {
    lib_entry_t *e = create_entry();
    e->name = malloc((strlen(name) + 1) * sizeof(char));
    strcpy(e->name, name);
    e->type = type;
    e->impl.func = func;
    e->internal = 1;
    return e;
}

void add_lib_entry(library *l, lib_entry_t *entry) {
    //printf("Adding lib entry: %s.\n", entry->name);
    HASH_ADD_KEYPTR (hh, l->table, entry->name, strlen(entry->name), entry);
}

void init_library(library *l) {
    l->table = NULL;
    add_lib_entry(l, construct("pop",       type_pop(),      &word_pop));
    add_lib_entry(l, construct("print",     type_print(),    &word_print));
    add_lib_entry(l, construct("println",   type_println(),  &word_println));
    add_lib_entry(l, construct("list",      type_list(),     &word_list));
    add_lib_entry(l, construct("copy",      type_copy(),     &word_copy));
    add_lib_entry(l, construct("swap",      type_swap(),     &word_swap));
    add_lib_entry(l, construct("map",       type_map(),      &word_map));
    add_lib_entry(l, construct("outer",     type_outer(),    &word_outer));
    add_lib_entry(l, construct("apply",     type_apply(),    &word_apply));
    add_lib_entry(l, construct("apply-0to0",  type_0to0(),   &word_apply));
    add_lib_entry(l, construct("apply-0to1",  type_0to1(),   &word_apply));
    add_lib_entry(l, construct("apply-1more", type_1more(),  &word_apply));
    add_lib_entry(l, construct("curry",     type_curry(),    &word_curry));
    add_lib_entry(l, construct("typeof",    type_typeof(),   &word_typeof));
    add_lib_entry(l, construct("if",        type_if(),       &word_if));
    add_lib_entry(l, construct("while",     type_while(),    &word_while));
    add_lib_entry(l, construct("dip",       type_dip(),      &word_dip));
    add_lib_entry(l, construct("pair",      type_pair(),     &word_pair));
    add_lib_entry(l, construct("unpair",    type_unpair(),   &word_unpair));
    add_lib_entry(l, construct("cons",      type_cons(),     &word_cons));
    add_lib_entry(l, construct("split",     type_split(),    &word_split));
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
