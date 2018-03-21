#include "alma.h"

/* Looks up a symbol in the given symbol table, or adds it
 * if it doesn't already exist. */
ASymbol *get_symbol(ASymbolTable t, const char *name) {
    ASymbolMapping *m = NULL;
    HASH_FIND_STR( t, name, m );
    if (m != NULL) {
        return m->sym;
    } else {
        // do the needful
    }
}
