#include "scope.h"

/* Create a new lexical scope with parent scope 'parent'. */
AScope *scope_new(AScope *parent) {
    AScope *newscope = malloc(sizeof(AScope));
    newscope->parent = parent;
    newscope->content = NULL;
    return newscope;
}

/* Allocate and initialize a new scope entry mapping 'sym' to 'func'. */
static
AScopeEntry *scope_entry_new(ASymbol *sym, AFunc *func) {
    AScopeEntry *newentry = malloc(sizeof(AScopeEntry));
    newentry->sym = sym;
    newentry->func = func;
    return newentry;
}

/* Register a new function into scope using the symbol sym as a key. */
void scope_register(AScope *sc, ASymbol *symbol, AFunc *func) {
    AScopeEntry *entry = scope_entry_new(symbol, func);
    /* 'sym' below is the field in the struct, not the variable 'symbol' here */
    HASH_ADD_PTR(sc->content, sym, entry);
}

/* Look up the function bound to a given symbol in a certain lexical scope. */
AFunc *scope_lookup(AScope *sc, ASymbol *symbol) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);
    if (e == NULL) {
        fprintf(stderr, "Error: Unrecognized word '%s'.\n", symbol->name);
        return NULL;
    } else {
        return e->func;
    }
}

/* Free a function. */
void free_func(AFunc *f) {
    // Since we only have builtins for now, we don't need to free anything else.
    free(f);
}

/* Free a scope entry. */
void free_scope_entry(AScopeEntry *entry) {
    // TODO When we have pointers to functions in the AST table rather than pointers
    // to symbols, we'll be able to free scope before running the program (since all
    // symbols will already be resolved.) When that happens, we'll need to give AFunc
    // a reference counter and only free it at the end rather than here.
    free_func(entry->func);
    free(entry);
}

/* Free a lexical scope at the end. */
void free_scope(AScope *sc) {
    AScopeEntry *current, *tmp;
    HASH_ITER(hh, sc->content, current, tmp) {
        HASH_DEL(sc->content, current);
        free_scope_entry(current);
    }
    free(sc);
}
