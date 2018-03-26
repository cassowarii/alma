#ifndef _AL_SYM_H__
#define _AL_SYM_H__

#include "alma.h"

/* Looks up a symbol in the given symbol table, or adds it
 * if it doesn't already exist. */
ASymbol *get_symbol(ASymbolTable *t, const char *name);

/* Print a symbol. */
void print_symbol(ASymbol *s);

#endif
