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

/* Add built in func to scope by wrapping it in a newly allocated AFunc */
static
void addlibfunc(AScope *sc, ASymbolTable symtab, const char *name, ABuiltInFunc f) {
    AFunc *newfunc = malloc(sizeof(AFunc));
    newfunc->type = builtin_func;
    newfunc->data.builtin = f;
    ASymbol *sym = get_symbol(&symtab, name);
    scope_register(sc, sym, newfunc);
}

/* Initialize builtin library functions into scope sc. */
void lib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "print", &lib_print);
    addlibfunc(sc, st, "println", &lib_print);
}
