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
    AValue *c = val_int(a->data.i + b->data.i);

    stack_push(stack, c);
    delete_ref(a);
    delete_ref(b);
}

/* Add built in func to scope by wrapping it in a newly allocated AFunc */
static
void addlibfunc(AScope *sc, ASymbolTable symtab, const char *name, APrimitiveFunc f) {
    AFunc *newfunc = malloc(sizeof(AFunc));
    newfunc->type = primitive_func;
    newfunc->data.primitive = f;
    ASymbol *sym = get_symbol(&symtab, name);
    scope_register(sc, sym, newfunc);
}

/* Initialize builtin library functions into scope sc. */
void lib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "print", &lib_print);
    addlibfunc(sc, st, "println", &lib_println);
    addlibfunc(sc, st, "+", &lib_add);
}
