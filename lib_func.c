#include "lib.h"

/* Print out the top value on the stack. */
void lib_print(AStack* stack, AVarBuffer *buffer) {
    AValue *val = stack_get(stack, 0);
    stack_pop(stack, 1);
    print_val_simple(val);
    delete_ref(val);
}

/* Print out the top value on the stack with newline. */
void lib_println(AStack* stack, AVarBuffer *buffer) {
    AValue *val = stack_get(stack, 0);
    stack_pop(stack, 1);
    print_val_simple(val);
    delete_ref(val);
    printf("\n");
}

/* Print out the top value on the stack with newline. */
void lib_quit(AStack* stack, AVarBuffer *buffer) {
    exit(0);
}

/* Initialize built-in functions. */
void funclib_init(ASymbolTable *st, AScope *sc) {
    addlibfunc(sc, st, "print", &lib_print);
    addlibfunc(sc, st, "println", &lib_println);
    addlibfunc(sc, st, "say", &lib_println); /* Alias */
    addlibfunc(sc, st, "quit", &lib_quit);
    addlibfunc(sc, st, "exit", &lib_quit);
}
