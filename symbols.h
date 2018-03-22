#include "alma.h"

#ifndef _AL_SYM_H__
#define _AL_SYM_H__

/* A mapping from names to unique symbols. */
typedef struct ASymbol {
    char *name;
} ASymbol;

/* A table mapping names to symbol pointers.
 * (Used during compilation -- symbols replaced by symbol
 * pointers during run b/c fast comparison/hash, a la scheme/ruby) */
typedef struct ASymbolMapping {
    char *name;
    ASymbol *sym;
    UT_hash_handle hh;
} ASymbolMapping;

/* Used for the actual table, since uthash just passes around an
 * ASymbolMapping* for access to the hash table */
typedef ASymbolMapping* ASymbolTable;

ASymbol *get_symbol(ASymbolTable *t, const char *name);

#endif
