#ifndef _AL_LIB_H__
#define _AL_LIB_H__

#include "alma.h"
#include "scope.h"
#include "symbols.h"
#include "stack.h"
#include "eval.h"

/* Initialize builtin library functions into scope sc. */
void lib_init(ASymbolTable symtab, AScope *sc);

#endif
