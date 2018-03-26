#ifndef _AL_SCOPE_H__
#define _AL_SCOPE_H__

#include "alma.h"

/* Create a new lexical scope with parent scope 'parent'. */
AScope *scope_new(AScope *parent);

/* Create an entry in the scope promising to fill in this function later. */
ACompileStatus scope_placehold(AScope *sc, ASymbol *symbol, unsigned int linenum);

/* Register a new function into scope using the symbol sym as a key. */
ACompileStatus scope_register(AScope *sc, ASymbol *sym, AFunc *func);

/* Look up the function bound to a given symbol in a certain lexical scope. */
AFunc *scope_lookup(AScope *sc, ASymbol *sym);

/* Free a function. */
void free_func(AFunc *f);

/* Free a scope entry. */
void free_scope_entry(AScopeEntry *entry);

/* Free a lexical scope at the end. */
void free_scope(AScope *sc);

#endif
