#include "lib.h"

/* Add built in func to scope by wrapping it in a newly allocated AFunc */
void addlibfunc(AScope *sc, ASymbolTable *symtab, const char *name, APrimitiveFunc f) {
    ASymbol *sym = get_symbol(symtab, name);
    AFunc *newfunc = malloc(sizeof(AFunc));
    newfunc->type = primitive_func;
    newfunc->data.primitive = f;
    newfunc->sym = sym;
    scope_register(sc, sym, newfunc);
}

/* Initialize builtin library functions into scope sc. */
void lib_init(ASymbolTable *st, AScope *sc, int verbose) {
    funclib_init(st, sc);
    oplib_init(st, sc);
    stacklib_init(st, sc);
    controllib_init(st, sc);
    listlib_init(st, sc);
}
