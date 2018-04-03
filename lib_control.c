#include "lib.h"

/* Given stack [A B C ..., apply A to the stack
 * below B and C, take the top element, and run
 * B if truthy, C if falsy. (integerwise.) */
void lib_if(AStack *stack, AVarBuffer *buffer) {
    AValue *ifpart = stack_get(stack, 0);
    AValue *thenpart = stack_get(stack, 1);
    AValue *elsepart = stack_get(stack, 2);
    stack_pop(stack, 3);

    eval_block(stack, buffer, ifpart);

    AValue *condition = stack_get(stack, 0);
    stack_pop(stack, 1);

    if (condition->data.i) {
        eval_block(stack, buffer, thenpart);
    } else {
        eval_block(stack, buffer, elsepart);
    }

    delete_ref(ifpart);
    delete_ref(condition);
    delete_ref(thenpart);
    delete_ref(elsepart);
}

/* Given stack [A B C ..., apply A to the stack
 * below B and C, take the top element, and run
 * B if truthy, C if falsy. But put the top element
 * of the stack back before running B or C. */
void lib_ifstar(AStack *stack, AVarBuffer *buffer) {
    AValue *ifpart = stack_get(stack, 0);
    AValue *thenpart = stack_get(stack, 1);
    AValue *elsepart = stack_get(stack, 2);
    AValue *top = stack_get(stack, 3);

    /* don't pop off 'top' */
    stack_pop(stack, 3);

    eval_block(stack, buffer, ifpart);

    AValue *condition = stack_get(stack, 0);
    stack_pop(stack, 1);

    stack_push(stack, top);

    if (condition->data.i) {
        eval_block(stack, buffer, thenpart);
    } else {
        eval_block(stack, buffer, elsepart);
    }

    delete_ref(ifpart);
    delete_ref(condition);
    delete_ref(thenpart);
    delete_ref(elsepart);
}

/* Given stack [A B ..., repeatedly apply A to
 * the stack below B and apply B over and over
 * again until applying A gives a falsy value. */
void lib_while (AStack *stack, AVarBuffer *buffer) {
    AValue *condpart = stack_get(stack, 0);
    AValue *looppart = stack_get(stack, 1);
    stack_pop(stack, 2);

    eval_block(stack, buffer, condpart);

    AValue *condition = stack_get(stack, 0);

    stack_pop(stack, 1);

    while (condition->data.i) {
        delete_ref(condition);

        eval_block(stack, buffer, looppart);

        eval_block(stack, buffer, condpart);

        condition = stack_get(stack, 0);
        stack_pop(stack, 1);
    }

    delete_ref(condpart);
    delete_ref(condition);
    delete_ref(looppart);
}

/* Given stack [A B ..., repeatedly apply A to
 * the stack below B and apply B over and over
 * again until applying A gives a falsy value.
 * But keep the top value on the stack after
 * applying A each time. */
void lib_whilestar(AStack *stack, AVarBuffer *buffer) {
    AValue *condpart = stack_get(stack, 0);
    AValue *looppart = stack_get(stack, 1);
    AValue *top = stack_get(stack, 2);

    stack_pop(stack, 2);

    eval_block(stack, buffer, condpart);

    AValue *condition = stack_get(stack, 0);
    stack_pop(stack, 1);

    while (condition->data.i) {
        delete_ref(condition);

        stack_push(stack, top);

        eval_block(stack, buffer, looppart);

        top = stack_get(stack, 0);

        eval_block(stack, buffer, condpart);

        condition = stack_get(stack, 0);
        stack_pop(stack, 1);
    }

    stack_push(stack, top);

    delete_ref(condpart);
    delete_ref(condition);
    delete_ref(looppart);
}

/* Initialize built-in control flow functions. */
void controllib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "if", &lib_if);
    addlibfunc(sc, st, "if*", &lib_ifstar);
    addlibfunc(sc, st, "while", &lib_while);
    addlibfunc(sc, st, "while*", &lib_whilestar);
}
