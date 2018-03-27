#ifndef _AL_SCOPE_H__
#define _AL_SCOPE_H__

#include "alma.h"
#include "registry.h"

/* Create a new lexical scope with parent scope 'parent'. */
AScope *scope_new(AScope *parent);

/* Create an entry in the scope promising to fill in this function later. */
ACompileStatus scope_placehold(AScope *sc, AFuncRegistry *reg, ASymbol *symbol, unsigned int linenum);

/* Register a new function into scope using the symbol sym as a key. */
ACompileStatus scope_register(AScope *sc, ASymbol *sym, AFunc *func);

/* Register a new user word into scope. Requires that scope_placehold was already called. */
ACompileStatus scope_user_register(AScope *sc, ASymbol *symbol, AUserFuncType type, AWordSeqNode *words);

/* Look up the function bound to a given symbol in a certain lexical scope.
 * Returns NULL if not found. */
AScopeEntry *scope_lookup(AScope *sc, ASymbol *symbol);

/* Free a function. */
void free_func(AFunc *f);

/* Free a function. */
void free_user_func(AUserFunc *f);

/* Free a scope entry. */
void free_scope_entry(AScopeEntry *entry);

/* Free a lexical scope during compilation. */
void free_scope(AScope *sc);

/* Free the library scope underneath everything else. (This calls free_func
 * on its elements, unlike the regular free_scope.) */
void free_lib_scope(AScope *sc);

#endif
