#include "lib.h"

/* Duplicate the top value on the stack. */
void lib_dup(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    stack_push(stack, a);
}

/* Swap the top two values on the stack. */
void lib_swap(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);
    stack_push(stack, a);
    stack_push(stack, b);
}

/* Drop the top value off the stack. */
void lib_drop(AStack* stack, AVarBuffer *buffer) {
    stack_pop(stack, 1);
}

/* Apply the block value on top of the stack. */
void lib_apply(AStack *stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    stack_pop(stack, 1);

    eval_block(stack, buffer, a);

    delete_ref(a);
}

/* Apply the block value on top of the stack, but
 * ignore the top value underneath said block. */
void lib_dip(AStack *stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    if (a->type != block_val && a->type != bound_block_val) {
        fprintf(stderr, "dip needs a block! (got %d)\n", a->type);
        return;
    }
    eval_block(stack, buffer, a);

    delete_ref(a);

    stack_push(stack, b);
}

/* Print out the current stack, for debugging. */
void lib_stackprint(AStack* stack, AVarBuffer *buffer) {
    print_stack(stack);
}

/* Initialize built-in stack operations. */
void stacklib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "dup", &lib_dup);
    addlibfunc(sc, st, "swap", &lib_drop);
    addlibfunc(sc, st, "dip", &lib_dip);
    addlibfunc(sc, st, "drop", &lib_drop);
    addlibfunc(sc, st, "apply", &lib_apply);
    addlibfunc(sc, st, "stack", &lib_stackprint);
}
