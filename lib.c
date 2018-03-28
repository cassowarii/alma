#include "lib.h"

/* Print out the top value on the stack. */
void lib_print(AStack* stack, AScope *scope) {
    AValue *val = stack_get(stack, 0);
    stack_pop(stack, 1);
    print_val_simple(val);
    delete_ref(val);
}

/* Print out the top value on the stack with newline. */
void lib_println(AStack* stack, AScope *scope) {
    AValue *val = stack_get(stack, 0);
    stack_pop(stack, 1);
    print_val_simple(val);
    delete_ref(val);
    printf("\n");
}

/* Add the top two values on the stack. */
void lib_add(AStack* stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = ref(val_int(a->data.i + b->data.i));

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Subtract the top value on the stack from the second value on the stack. */
void lib_subtract(AStack* stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = ref(val_int(b->data.i - a->data.i));

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Multiply the top two values on the stack. */
void lib_multiply(AStack* stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = ref(val_int(a->data.i * b->data.i));

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* given stack [A B ..., is B < A? */
void lib_lessthan(AStack* stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = ref(val_int(b->data.i < a->data.i));

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* given stack [A B ..., is B > A? */
void lib_greaterthan(AStack* stack, AScope *scope) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = ref(val_int(b->data.i > a->data.i));

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

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

/* Given stack [A B C ..., apply A to the stack
 * below B and C, take the top element, and run
 * B if truthy, C if falsy. (integerwise.) */
void lib_if(AStack *stack, AScope *scope) {
    AValue *ifpart = stack_get(stack, 0);
    AValue *thenpart = stack_get(stack, 1);
    AValue *elsepart = stack_get(stack, 2);
    stack_pop(stack, 3);

    eval_sequence(stack, scope, ifpart->data.ast);

    AValue *condition = stack_get(stack, 0);
    stack_pop(stack, 1);

    if (condition->data.i) {
        eval_sequence(stack, scope, thenpart->data.ast);
    } else {
        eval_sequence(stack, scope, elsepart->data.ast);
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
void lib_ifstar(AStack *stack, AScope *scope) {
    AValue *ifpart = stack_get(stack, 0);
    AValue *thenpart = stack_get(stack, 1);
    AValue *elsepart = stack_get(stack, 2);
    AValue *top = ref(stack_get(stack, 3));

    /* don't pop off 'top' */
    stack_pop(stack, 3);

    eval_sequence(stack, scope, ifpart->data.ast);

    AValue *condition = stack_get(stack, 0);
    stack_pop(stack, 1);

    stack_push(stack, top);

    if (condition->data.i) {
        eval_sequence(stack, scope, thenpart->data.ast);
    } else {
        eval_sequence(stack, scope, elsepart->data.ast);
    }

    delete_ref(ifpart);
    delete_ref(condition);
    delete_ref(thenpart);
    delete_ref(elsepart);
}

/* Given stack [A B ..., repeatedly apply A to
 * the stack below B and apply B over and over
 * again until applying A gives a falsy value. */
void lib_while (AStack *stack, AScope *scope) {
    AValue *condpart = stack_get(stack, 0);
    AValue *looppart = stack_get(stack, 1);
    stack_pop(stack, 2);

    eval_sequence(stack, scope, condpart->data.ast);

    AValue *condition = stack_get(stack, 0);

    stack_pop(stack, 1);

    while (condition->data.i) {
        delete_ref(condition);

        eval_sequence(stack, scope, looppart->data.ast);

        eval_sequence(stack, scope, condpart->data.ast);

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
void lib_whilestar(AStack *stack, AScope *scope) {
    AValue *condpart = stack_get(stack, 0);
    AValue *looppart = stack_get(stack, 1);
    AValue *top = stack_get(stack, 2);

    stack_pop(stack, 2);

    eval_sequence(stack, scope, condpart->data.ast);

    AValue *condition = stack_get(stack, 0);
    stack_pop(stack, 1);

    while (condition->data.i) {
        delete_ref(condition);

        stack_push(stack, top);

        eval_sequence(stack, scope, looppart->data.ast);

        top = stack_get(stack, 0);

        eval_sequence(stack, scope, condpart->data.ast);

        condition = stack_get(stack, 0);
        stack_pop(stack, 1);
    }

    delete_ref(condpart);
    delete_ref(condition);
    delete_ref(looppart);
}

/* Add built in func to scope by wrapping it in a newly allocated AFunc */
static
void addlibfunc(AScope *sc, ASymbolTable symtab, const char *name, APrimitiveFunc f) {
    ASymbol *sym = get_symbol(&symtab, name);
    AFunc *newfunc = malloc(sizeof(AFunc));
    newfunc->type = primitive_func;
    newfunc->data.primitive = f;
    newfunc->sym = sym;
    scope_register(sc, sym, newfunc);
}

/* Initialize builtin library functions into scope sc. */
void lib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "print", &lib_print);
    addlibfunc(sc, st, "println", &lib_println);

    addlibfunc(sc, st, "+", &lib_add);
    addlibfunc(sc, st, "-", &lib_subtract);
    addlibfunc(sc, st, "*", &lib_multiply);
    addlibfunc(sc, st, "<", &lib_lessthan);
    addlibfunc(sc, st, ">", &lib_greaterthan);

    addlibfunc(sc, st, "dup", &lib_dup);
    addlibfunc(sc, st, "dip", &lib_dip);
    addlibfunc(sc, st, "drop", &lib_drop);
    addlibfunc(sc, st, "apply", &lib_apply);

    addlibfunc(sc, st, "if", &lib_if);
    addlibfunc(sc, st, "if*", &lib_ifstar);
    addlibfunc(sc, st, "while", &lib_while);
    addlibfunc(sc, st, "while*", &lib_whilestar);
}
