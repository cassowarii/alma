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
    newentry->linenum = -1;
    return newentry;
}

/* Create an entry in the scope promising to fill in this function later. */
ACompileStatus scope_placehold(AScope *sc, ASymbol *symbol, unsigned int linenum) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);
    if (e == NULL) {
        AFunc *dummy = malloc(sizeof(AFunc));
        dummy->type = dummy_func;
        AScopeEntry *entry = scope_entry_new(symbol, dummy);
        entry->linenum = linenum;
        /* 'sym' below is the field in the struct, not the variable 'symbol' here */
        HASH_ADD_PTR(sc->content, sym, entry);
    } else {
        fprintf(stderr, "error: duplicate definition of '%s' at line %d.\n"
                        "(it was previously defined at line %d.)\n",
                        symbol->name, linenum, e->linenum);
        return compile_fail;
    }
    return compile_success;
}

/* Register a new function into scope using the symbol sym as a key. */
ACompileStatus scope_register(AScope *sc, ASymbol *symbol, AFunc *func) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);
    if (e != NULL) {
        if (e->func->type == dummy_func) {
            /* finally defining a dummy func */
            HASH_DEL(sc->content, e);
            free(e->func);
            free(e);
        } else if (e != NULL) {
            fprintf(stderr, "error: Attempt to register duplicate function '%s'\n",
                    symbol->name);
            return compile_fail;
        }
    }
    AScopeEntry *entry = scope_entry_new(symbol, func);
    /* 'sym' below is the field in the struct, not the variable 'symbol' here */
    HASH_ADD_PTR(sc->content, sym, entry);
    return compile_success;
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
    if (sc == NULL) return;
    AScopeEntry *current, *tmp;
    HASH_ITER(hh, sc->content, current, tmp) {
        HASH_DEL(sc->content, current);
        free_scope_entry(current);
    }
    free(sc);
}
