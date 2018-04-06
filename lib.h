#ifndef _AL_LIB_H__
#define _AL_LIB_H__

#include "alma.h"
#include "scope.h"
#include "symbols.h"
#include "stack.h"
#include "eval.h"

/* Initialize builtin library functions into scope sc. */
void lib_init(ASymbolTable *symtab, AScope *sc, int verbose);

/* Initialize built-in functions. */
void funclib_init(ASymbolTable *symtab, AScope *sc);

/* Initialize built-in operators. */
void oplib_init(ASymbolTable *symtab, AScope *sc);

/* Initialize built-in stack operations. */
void stacklib_init(ASymbolTable *symtab, AScope *sc);

/* Initialize built-in control flow functions. */
void controllib_init(ASymbolTable *symtab, AScope *sc);

/* Initialize built-in control flow functions. */
void listlib_init(ASymbolTable *symtab, AScope *sc);

/* Add built in func to scope by wrapping it in a newly allocated AFunc */
void addlibfunc(AScope *sc, ASymbolTable *symtab, const char *name, APrimitiveFunc f);

#endif
