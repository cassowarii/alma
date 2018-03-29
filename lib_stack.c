#include "lib.h"

/* Duplicate the top value on the stack. */
void lib_dup(AStack* stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    stack_push(stack, a);
}

/* Drop the top value off the stack. */
void lib_drop(AStack* stack, AScope *scope) {
    stack_pop(stack, 1);
}

/* Apply the block value on top of the stack. */
void lib_apply(AStack *stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    stack_pop(stack, 1);

    eval_sequence(stack, scope, a->data.ast);

    delete_ref(a);
}

/* Apply the block value on top of the stack, but
 * ignore the thing directly underneath. */
void lib_dip(AStack *stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    if (a->type != block_val) {
        fprintf(stderr, "dip needs a block! (got %d)\n", a->type);
        return;
    }
    eval_sequence(stack, scope, a->data.ast);

    delete_ref(a);

    stack_push(stack, b);
}

/* Initialize built-in stack operations. */
void stacklib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "dup", &lib_dup);
    addlibfunc(sc, st, "dip", &lib_dip);
    addlibfunc(sc, st, "drop", &lib_drop);
    addlibfunc(sc, st, "apply", &lib_apply);
}
