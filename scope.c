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

/* Create an entry in the scope promising to fill in this word later. */
ACompileStatus scope_placehold(AScope *sc, ASymbol *symbol, unsigned int linenum) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);
    if (e == NULL) {
        AUserFunc *dummy = malloc(sizeof(AUserFunc));
        dummy->type = dummy_func;
        dummy->words = NULL;

        /* The reason for this weird structure is, now we can change the AUserFunc
         * that dummyfunc has a pointer to, and anything that got a pointer to
         * dummyfunc before we defined it will get that same pointer. */
        AFunc *dummyfunc = malloc(sizeof(AFunc));
        dummyfunc->type = user_func;
        dummyfunc->data.userfunc = dummy;

        AScopeEntry *entry = scope_entry_new(symbol, dummyfunc);
        entry->linenum = linenum;

        /* 'sym' below is the field in the struct, not the variable 'symbol' here */
        HASH_ADD_PTR(sc->content, sym, entry);
    } else if (e->func && e->func->type != user_func) {
        fprintf(stderr, "error: cannot redefine builtin word ‘%s’\n",
                        symbol->name);
        return compile_fail;
    } else if (e->func) {
        fprintf(stderr, "error: duplicate definition of ‘%s’ at line %d.\n"
                        "(it was previously defined at line %d.)\n",
                        symbol->name, linenum, e->linenum);
        return compile_fail;
    } else {
        fprintf(stderr, "internal error: trying to placehold, but ‘%s’ has null func\n",
                        symbol->name);
        return compile_fail;
    }
    return compile_success;
}

/* Register a new word into scope using the symbol ‘symbol’ as a key. */
ACompileStatus scope_register(AScope *sc, ASymbol *symbol, AFunc *func) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);

    if (e != NULL) {
        if (e->func->type == user_func) {
            /* this isn't wrong but probably shouldn't happen */
            fprintf(stderr, "internal warning: calling scope_register rather than "
                    "scope_user_register with a func that was declared as user (%s)\n",
                    symbol->name);
            free(e->func);
            e->func = func;
            return compile_success;
        } else if (e != NULL) {
            /* this means, I guess that we called addlibfunc with the same name twice */
            fprintf(stderr, "internal error: Attempt to register duplicate word ‘%s’\n",
                    symbol->name);
            return compile_fail;
        } else {
            /* this won't happen but the compiler doesn't like leaving this off */
            fprintf(stderr, "internal error: e is somehow null and not null\n");
            return compile_fail;
        }
    } else {
        /* probably adding a primitive func */
        AScopeEntry *entry = scope_entry_new(symbol, func);

        /* 'sym' below is the field in the struct, not the variable 'symbol' here */
        HASH_ADD_PTR(sc->content, sym, entry);
        return compile_success;
    }
}

/* Register a new user word into scope. Requires that scope_placehold was already called. */
ACompileStatus scope_user_register(AScope *sc, ASymbol *symbol, AUserFuncType type, AWordSeqNode *words) {
    AScopeEntry *e = NULL;
    // TODO replace this with scope_lookup so we can warn about variable shadowing
    HASH_FIND_PTR(sc->content, &symbol, e);

    if (e == NULL) {
        fprintf(stderr, "internal error: registering non-dummied user word ‘%s’\n",
                symbol->name);
        return compile_fail;
    } else if (e->func == NULL) {
        fprintf(stderr, "internal error: somehow word ‘%s’ ended up with null func\n",
                symbol->name);
        return compile_fail;
    } else if (e->func->type != user_func) {
        fprintf(stderr, "error: cannot redefine builtin word ‘%s’\n",
                symbol->name);
        return compile_fail;
    } else if (e->func->data.userfunc->type != dummy_func) {
        fprintf(stderr, "internal error: Attempt to create duplicate word ‘%s’\n",
                symbol->name);
        return compile_fail;
    } else {
        /* ok i think we're good now */
        e->func->data.userfunc->type = type;
        e->func->data.userfunc->words = words;
    }

    return compile_success;
}

/* Look up the word bound to a given symbol in a certain lexical scope. */
AFunc *scope_lookup(AScope *sc, ASymbol *symbol) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);
    if (e == NULL) {
        return NULL;
    } else {
        return e->func;
    }
}

/* Free a word. */
void free_func(AFunc *f) {
    // Since we only have builtins for now, we don't need to free anything else.
    free(f);
}

/* Free a scope entry. */
void free_scope_entry(AScopeEntry *entry) {
    // TODO When we have pointers to words in the AST table rather than pointers
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
