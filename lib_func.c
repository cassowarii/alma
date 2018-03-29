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

/* Initialize built-in functions. */
void funclib_init(ASymbolTable st, AScope *sc) {
    addlibfunc(sc, st, "print", &lib_print);
    addlibfunc(sc, st, "println", &lib_println);
}
