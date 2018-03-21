#include "alma.h"

/* Create new symbol... */
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
