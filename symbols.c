#include "symbols.h"

/* Create new symbol... */
static
ASymbol *create_symbol(const char *name) {
    ASymbol *newsym = malloc(sizeof(ASymbol));

    newsym->name = malloc(strlen(name)+1);

    strcpy(newsym->name, name);

    return newsym;
}

/* Looks up a symbol in the given symbol table, or adds it
 * if it doesn't already exist. */
ASymbol *get_symbol(ASymbolTable *t, const char *name) {
    ASymbolMapping *m = NULL;

    HASH_FIND_STR( *t, name, m );

    if (m != NULL) {
        return m->sym;
    } else {
        ASymbol *newsym = create_symbol(name);
        ASymbolMapping *mapping = malloc(sizeof(ASymbolMapping));

        mapping->name = malloc(strlen(name)+1);
        strcpy(mapping->name, name);
        mapping->sym = newsym;

        HASH_ADD_KEYPTR( hh, *t, mapping->name, strlen(mapping->name), mapping );

        return mapping->sym;
    }
}

/* Print a symbol. */
void print_symbol(ASymbol *s) {
    printf("%s", s->name);
}

/* Free a symbol. (Should only be called at the end.) */
void free_symbol(ASymbol *to_free) {
    free(to_free->name);
    free(to_free);
}

/* Free a symbol mapping. (Likewise!) */
void free_symbol_mapping(ASymbolMapping *to_free) {
    free(to_free->name);
    free_symbol(to_free->sym);
    free(to_free);
}

/* Free the symbol table at the end of the program. */
void free_symbol_table(ASymbolTable *t) {
    ASymbolMapping *current, *tmp;
    HASH_ITER(hh, *t, current, tmp) {
        HASH_DEL(*t, current);
        free_symbol_mapping(current);
    }
}
