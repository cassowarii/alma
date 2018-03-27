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
ACompileStatus scope_placehold(AScope *sc, AFuncRegistry *reg, ASymbol *symbol, unsigned int linenum) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);

    if (e == NULL) {
        AScopeEntry *f = scope_lookup(sc->parent, symbol);
        if (f != NULL) {
            if (f->linenum != -1) {
                fprintf(stderr, "warning: declaration of word ‘%s’ at line %d "
                                "shadows previous definition at line %d\n",
                                symbol->name, linenum, f->linenum);
            } else {
                fprintf(stderr, "warning: declaration of word ‘%s’ at line %d "
                                "shadows built-in word\n", symbol->name,
                                linenum);
            }
        }

        HASH_FIND_PTR(sc->content, &symbol, e);
        AUserFunc *dummy = malloc(sizeof(AUserFunc));
        dummy->type = dummy_func;
        dummy->words = NULL;

        /* The reason for this weird structure is, now we can change the AUserFunc
         * that dummyfunc has a pointer to, and anything that got a pointer to
         * dummyfunc before we defined it will get that same pointer. */
        AFunc *dummyfunc = malloc(sizeof(AFunc));
        dummyfunc->type = user_func;
        dummyfunc->data.userfunc = dummy;
        registry_register(reg, dummyfunc);

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
            /* we're clobbering the user's function here, while loading library somehow? */
            /* this shouldn't happen */
            fprintf(stderr, "internal error: calling scope_register rather than "
                    "scope_user_register with a func that was declared as user "
                    " (‘%s’)\n", symbol->name);
            return compile_fail;
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
AScopeEntry *scope_lookup(AScope *sc, ASymbol *symbol) {
    if (sc == NULL) {
        return NULL;
    }
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);
    if (e == NULL) {
        return scope_lookup(sc->parent, symbol);
    } else {
        return e;
    }
}

/* Look up a function by name in the scope. */
AFunc *scope_find_func(AScope *sc, ASymbolTable symtab, const char *name) {
    ASymbol *sym = get_symbol(&symtab, "main");
    if (sym == NULL) {
        return NULL;
    }

    AScopeEntry *entry = scope_lookup(sc, sym);
    if (entry == NULL) {
        return NULL;
    }

    return entry->func;
}

/* Free a function. */
void free_user_func(AUserFunc *f) {
    if (f->type == const_func) {
        /* if const func, its code pointer is just a pointer to something in the
           'program' ast, so we don't need to free that. */
    }
    free(f);
}

/* Free a word. */
void free_func(AFunc *f) {
    /* The user-func portion (if it's a user-func) will get freed when we free
     * the User Func Registry. */
    if (f->type == user_func) {
        free_user_func(f->data.userfunc);
    }
    free(f);
}

/* Free a scope entry. */
void free_scope_entry(AScopeEntry *entry) {
    /* This doesn't free the function associated with it, since we want
     * to dispose of the scope without disturbing the delicate functions
     * within (since we're probably freeing the scope at compile-time.) */
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

/* Free the library scope underneath everything else. (This calls free_func
 * on its elements, unlike the regular free_scope.) */
void free_lib_scope(AScope *sc) {
    if (sc == NULL) return;
    AScopeEntry *current, *tmp;
    HASH_ITER(hh, sc->content, current, tmp) {
        HASH_DEL(sc->content, current);
        free_func(current->func);
        free_scope_entry(current);
    }
    free(sc);
}
