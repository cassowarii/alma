#include "lib.h"

/* optimizing value-math functions (see bottom of file) */
static AValue *gt_int_val(AValue *a, AValue *b);
static AValue *lt_int_val(AValue *a, AValue *b);
static AValue *gte_int_val(AValue *a, AValue *b);
static AValue *lte_int_val(AValue *a, AValue *b);
static AValue *ne_int_val(AValue *a, AValue *b);
static AValue *eq_int_val(AValue *a, AValue *b);
/* TODO replace all these with set_2int_val.
 * I am not very smart. */
static AValue *set_int_val(AValue *a, int x);
static AValue *set_2int_val(AValue *a, AValue *b, long x);

/* Boolean-negate the top value on the stack. */
void lib_not(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    stack_pop(stack, 1);

    AValue *c = set_int_val(a, !a->data.i);

    stack_push(stack, c);
    delete_ref(a);
}

/* Add the top two values on the stack. */
void lib_add(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = set_2int_val(a, b, b->data.i + a->data.i);

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
    AValue *c = set_2int_val(a, b, b->data.i - a->data.i);

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
    AValue *c = set_2int_val(a, b, b->data.i * a->data.i);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Divide the NOS by TOS. (Integer division only!) */
void lib_div(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = set_2int_val(a, b, b->data.i / a->data.i);

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

/* are the two numbers on the top of the stack not equal? */
void lib_notequal(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = ne_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* are the two numbers on the top of the stack equal? */
void lib_equal(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 0);
    AValue *b = stack_get(stack, 1);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = eq_int_val(a, b);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Take mod of NOS by TOS. */
void lib_mod(AStack* stack, AVarBuffer *buffer) {
    AValue *a = stack_get(stack, 1);
    AValue *b = stack_get(stack, 0);
    stack_pop(stack, 2);

    // We don't do typechecking yet, so this might be garbage
    // if it's not actually an int... we'll fix this later!
    AValue *c = set_2int_val(a, b, a->data.i % b->data.i);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}


/* Initialize built-in operators. */
void oplib_init(ASymbolTable *st, AScope *sc) {
    addlibfunc(sc, st, "+", &lib_add);
    addlibfunc(sc, st, "-", &lib_subtract);
    addlibfunc(sc, st, "*", &lib_multiply);
    addlibfunc(sc, st, "/", &lib_div);
    addlibfunc(sc, st, "mod", &lib_mod);
    addlibfunc(sc, st, "<", &lib_lessthan);
    addlibfunc(sc, st, ">", &lib_greaterthan);
    addlibfunc(sc, st, "<=", &lib_lessthanequal);
    addlibfunc(sc, st, "≤", &lib_lessthanequal);
    addlibfunc(sc, st, ">=", &lib_greaterthanequal);
    addlibfunc(sc, st, "≥", &lib_greaterthanequal);
    addlibfunc(sc, st, "=", &lib_equal);
    addlibfunc(sc, st, "!=", &lib_notequal);
    addlibfunc(sc, st, "≠", &lib_notequal);
    addlibfunc(sc, st, "not", &lib_not);
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
        a->data.i = b->data.i <= a->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = b->data.i <= a->data.i;
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
        a->data.i = b->data.i >= a->data.i;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = b->data.i >= a->data.i;
        return ref(b);
    } else {
        return ref(val_int(b->data.i >= a->data.i));
    }
}

/* Same but for not-equal. */
static
AValue *ne_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i = (b->data.i != a->data.i);
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = (b->data.i != a->data.i);
        return ref(b);
    } else {
        return ref(val_int(b->data.i != a->data.i));
    }
}

/* Same but for equal. */
static
AValue *eq_int_val(AValue *a, AValue *b) {
    if (a->refs <= 1) {
        a->data.i = (b->data.i == a->data.i);
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = (b->data.i == a->data.i);
        return ref(b);
    } else {
        return ref(val_int(b->data.i == a->data.i));
    }
}

/* Same but lets you set to an arbitrary int val. */
static AValue *set_int_val(AValue *a, int x) {
    if (a->refs <= 1) {
        a->data.i = x;
        return ref(a);
    } else {
        return ref(val_int(x));
    }
}

static
AValue *set_2int_val(AValue *a, AValue *b, long x) {
    if (a->refs <= 1) {
        a->data.i = x;
        return ref(a);
    } else if (b->refs <= 1) {
        b->data.i = x;
        return ref(b);
    } else {
        return ref(val_int(x));
    }
}
