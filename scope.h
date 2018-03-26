#ifndef _AL_SCOPE_H__
#define _AL_SCOPE_H__

#include "alma.h"

/* Create a new lexical scope with parent scope 'parent'. */
AScope *scope_new(AScope *parent);

/* Register a new function into scope using the symbol sym as a key. */
void scope_register(AScope *sc, ASymbol *sym, AFunc *func);

/* Look up the function bound to a given symbol in a certain lexical scope. */
AFunc *scope_lookup(AScope *sc, ASymbol *sym);

#endif
