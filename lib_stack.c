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

/* Copy the value below the top of the stack and put it
 * on top of the stack. ( a b -- a b a ) */
void lib_over(AStack* stack, AVarBuffer *buffer) {
    AValue *b = stack_get(stack, 1);

    stack_push(stack, b);
}

/* Move the value below NOS onto the top of the stack.
 * (i.e. ( a b c -- b c a ) where top of stack is to
 * the right) */
void lib_rot(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 2);
    AValue *b = stack_get(stack, 1);
    AValue *c = stack_get(stack, 0);

    stack_pop(stack, 3);

    stack_push(stack, b);
    stack_push(stack, c);
    stack_push(stack, a);
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
void stacklib_init(ASymbolTable *st, AScope *sc) {
    addlibfunc(sc, st, "dup", &lib_dup);
    addlibfunc(sc, st, "swap", &lib_swap);
    addlibfunc(sc, st, "over", &lib_over);
    addlibfunc(sc, st, "rot", &lib_rot);
    addlibfunc(sc, st, "dip", &lib_dip);
    addlibfunc(sc, st, "drop", &lib_drop);
    addlibfunc(sc, st, "apply", &lib_apply);
    addlibfunc(sc, st, "stack", &lib_stackprint);
}
