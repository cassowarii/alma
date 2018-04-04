#include "lib.h"

/* find length of list */
void lib_len(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    stack_pop(stack, 1);

    AValue *len = ref(val_int(a->data.list->length));

    stack_push(stack, len);
    delete_ref(a);
}

/* put a value onto front a list */
void lib_cons(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *vlist = stack_get(stack, 1);
    stack_pop(stack, 2);

    AValue *result = cons_list_val(a, vlist);
    stack_push(stack, result);

    delete_ref(a);
    delete_ref(vlist);
}

/* get head of list */
void lib_head(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    stack_pop(stack, 1);

    AValue *head = head_list_val(a);

    stack_push(stack, head);
    delete_ref(a);
}

/* get tail of list */
void lib_tail(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    stack_pop(stack, 1);

    AValue *tail = tail_list_val(a);

    stack_push(stack, tail);
    delete_ref(a);
}

/* split list on top of stack into head and tail */
void lib_uncons(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    stack_pop(stack, 1);

    AValue *head = head_list_val(a);
    AValue *tail = tail_list_val(a);

    stack_push(stack, tail);
    stack_push(stack, head);
    delete_ref(a);
}

/* Initialize built-in list operators. */
void listlib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "len", &lib_len);
    addlibfunc(sc, st, "cons", &lib_cons);
    addlibfunc(sc, st, "head", &lib_head);
    addlibfunc(sc, st, "tail", &lib_tail);
    addlibfunc(sc, st, "uncons", &lib_uncons);
}
