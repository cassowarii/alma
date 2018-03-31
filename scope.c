#include "scope.h"

extern unsigned int NOFREEVARS;

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
        /* If we only have a placeholder for a function, we assume it has the
         * maximum number of free variables. Being cautious like this means
         * we might save a few extra closures, but if we actually do need
         * one, we always have one. */
        dummy->free_var_index = 0;

        /* The reason for this weird structure is, now we can change the AUserFunc
         * that dummyfunc has a pointer to, and anything that got a pointer to
         * dummyfunc before we defined it will get that same pointer. */
        AFunc *dummyfunc = malloc(sizeof(AFunc));
        dummyfunc->type = user_func;
        dummyfunc->data.userfunc = dummy;
        dummyfunc->sym = symbol;
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
        assert(0 && "scope entry exists, but has null func pointer");
        return compile_fail;
    }
    return compile_success;
}

/* Register a new word into scope using the symbol ‘symbol’ as a key. */
/* Only used for stdlib/primitive functions, not user functions */
ACompileStatus scope_register(AScope *sc, ASymbol *symbol, AFunc *func) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);

    assert(e == NULL && "duplicate func name. fix your dang library loading code");

    /* create new entry */
    AScopeEntry *entry = scope_entry_new(symbol, func);

    /* 'sym' below is the field in the struct, not the variable 'symbol' here */
    HASH_ADD_PTR(sc->content, sym, entry);
    return compile_success;
}

/* Register a new user word into scope. Requires that scope_placehold was already called. */
ACompileStatus scope_user_register(AScope *sc, ASymbol *symbol, unsigned int free_index,
                                   unsigned int vars_below, AWordSeqNode *words) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);

    /* handle various error conditions (which shouldn't happen if the other compilation
     * code works right...) */
    assert (e != NULL && "registering non-dummied user word");
    assert (e->func != NULL && "user func is null somehow");
    assert (e->func->type == user_func && "somehow defining word in lowest scope, and shadowing builtin");
    assert (e->func->data.userfunc->type == dummy_func && "creating duplicate word, but this wasn't "
                                                          "caught in scope_placehold somehow");
    /* ok i think we're good now */
    e->func->data.userfunc->closure = NULL; /* named funcs don't use closure */
    e->func->data.userfunc->type = const_func;
    e->func->data.userfunc->words = words;
    e->func->data.userfunc->free_var_index = free_index;
    e->func->data.userfunc->vars_below = vars_below;

    return compile_success;
}

/* Create an entry in the scope telling it to push the
 * <num>'th bound variable to the stack. */
ACompileStatus scope_create_push(AScope *sc, AFuncRegistry *reg, ASymbol *symbol,
                                 unsigned int index, unsigned int linenum) {
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);

    if (e != NULL) {
        fprintf(stderr, "error: variable ‘%s’ declared more than once at line %d\n",
                        symbol->name, linenum);
        return compile_fail;
    }

    AScopeEntry *f = scope_lookup(sc->parent, symbol);
    if (f != NULL) {
        if (f->linenum != -1) {
            fprintf(stderr, "warning: declaration of variable ‘%s’ at line %d "
                            "shadows previous word defined at line %d\n",
                            symbol->name, linenum, f->linenum);
        } else {
            fprintf(stderr, "warning: declaration of variable ‘%s’ at line %d "
                            "shadows built-in word\n", symbol->name,
                            linenum);
        }
    }


    AFunc *pushfunc = malloc(sizeof(AFunc));
    pushfunc->type = var_push;
    pushfunc->data.varindex = index;
    pushfunc->sym = symbol;
    registry_register(reg, pushfunc);

    AScopeEntry *entry = scope_entry_new(symbol, pushfunc);
    entry->linenum = linenum;

    /* 'sym' below is the field in the struct, not the variable 'symbol' here */
    HASH_ADD_PTR(sc->content, sym, entry);

    return compile_success;
}

/* Look up the word that's bound to a given symbol in a certain lexical scope. */
AScopeEntry *scope_lookup(AScope *sc, ASymbol *symbol) {
    if (sc == NULL) {
        return NULL;
    }
    AScopeEntry *e = NULL;
    HASH_FIND_PTR(sc->content, &symbol, e);
    if (e == NULL) {
        /* If we didn't find it here, look it up in the parent. */
        return scope_lookup(sc->parent, symbol);
    } else {
        return e;
    }
}

/* Look up a function by name in the scope. */
AFunc *scope_find_func(AScope *sc, ASymbolTable symtab, const char *name) {
    /* this takes a const char* and looks up the symbol itself; mostly useful
     * for calling user funcs from C code (ie: calling main) */
    ASymbol *sym = get_symbol(&symtab, "main");
    if (sym == NULL) {
        return NULL;
    }

    AScopeEntry *entry = scope_lookup(sc, sym);
    if (entry == NULL) {
        /* We don't print an error message here -- calling code should handle it. */
        return NULL;
    }

    return entry->func;
}

/* Free a function. */
void free_user_func(AUserFunc *f) {
    if (f->type == const_func) {
        /* if const func, its code pointer is just a pointer to something in the
           'program' ast, so we don't need to free that. */
    } else if (f->type == bound_func) {
        /* if it's a bound_func, then we need to free its closure (or at least
         * decrease its closure's refcount.) */
        varbuf_unref(f->closure);
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
