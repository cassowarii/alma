#ifndef _AL_SYM_H__
#define _AL_SYM_H__

#include "alma.h"

/* Used for the actual table, since uthash just passes around an
 * ASymbolMapping* for access to the hash table */
typedef ASymbolMapping* ASymbolTable;

ASymbol *get_symbol(ASymbolTable *t, const char *name);

#endif
