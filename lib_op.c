#include "lib.h"

/* optimizing value-math functions (see bottom of file) */
static AValue *sum_int_val(AValue *a, AValue *b);
static AValue *diff_int_val(AValue *a, AValue *b);
static AValue *prod_int_val(AValue *a, AValue *b);
static AValue *gt_int_val(AValue *a, AValue *b);
static AValue *lt_int_val(AValue *a, AValue *b);
static AValue *gte_int_val(AValue *a, AValue *b);
static AValue *lte_int_val(AValue *a, AValue *b);

/* Add the top two values on the stack. */
void lib_add(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = sum_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Subtract the top value on the stack from the second value on the stack. */
void lib_subtract(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = diff_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Multiply the top two values on the stack. */
void lib_multiply(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = prod_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* given stack [A B ..., is B < A? */
void lib_lessthan(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = lt_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* given stack [A B ..., is B > A? */
void lib_greaterthan(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = gt_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* given stack [A B ..., is B < A? */
void lib_lessthanequal(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = lte_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* given stack [A B ..., is B > A? */
void lib_greaterthanequal(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = gte_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Initialize built-in operators. */
void oplib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "+", &lib_add);
    addlibfunc(sc, st, "-", &lib_subtract);
    addlibfunc(sc, st, "*", &lib_multiply);
    addlibfunc(sc, st, "<", &lib_lessthan);
    addlibfunc(sc, st, ">", &lib_greaterthan);
    addlibfunc(sc, st, "<=", &lib_lessthanequal);
    addlibfunc(sc, st, "≤", &lib_lessthanequal);
    addlibfunc(sc, st, ">=", &lib_greaterthanequal);
    addlibfunc(sc, st, "≥", &lib_greaterthanequal);
}

/* Save a memory allocation if possible when adding:
 * if one of the variables is only referenced once,
 * don't allocate a new one. */
static
AValue *sum_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i += b->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i += a->data.i;
        return ref(b);
    } else {
        return ref(val_int(a->data.i + b->data.i));
    }
}

/* Same but for subtraction. */
static
AValue *diff_int_val(AValue *a, AValue *b) {
    if (b->refs <= 1) {
        b->data.i -= a->data.i;
        return ref(b);
    } else if (a->refs <= 1) {
        a->data.i = b->data.i - a->data.i;
        return ref(a);
    } else {
        return ref(val_int(b->data.i - a->data.i));
    }
}

/* Same but for multiplication. */
static
AValue *prod_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i *= b->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i *= a->data.i;
        return ref(b);
    } else {
        return ref(val_int(b->data.i * a->data.i));
    }
}

/* Same but for lessthan. */
static
AValue *lt_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i = b->data.i < a->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = b->data.i < a->data.i;
        return ref(b);
    } else {
        return ref(val_int(b->data.i < a->data.i));
    }
}

/* Same but for lessthan-or-equal. */
static
AValue *lte_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i = b->data.i < a->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = b->data.i < a->data.i;
        return ref(b);
    } else {
        return ref(val_int(b->data.i <= a->data.i));
    }
}

/* Same but for greaterthan. */
static
AValue *gt_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i = b->data.i > a->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = b->data.i > a->data.i;
        return ref(b);
    } else {
        return ref(val_int(b->data.i > a->data.i));
    }
}

/* Same but for greaterthan-or-equal. */
static
AValue *gte_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i = b->data.i > a->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = b->data.i > a->data.i;
        return ref(b);
    } else {
        return ref(val_int(b->data.i >= a->data.i));
    }
}
