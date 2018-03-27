#ifndef _AL_REG_H__
#define _AL_REG_H__

#include "alma.h"

/* The user func registry keeps track of all user funcs that are
 * declared by scope_placehold. This means that we can safely free
 * a lexical scope when we're done with it, and a reference to its
 * declared functions will still exist, so they can be freed after
 * the program runs. */

/* Create a new User Func Registry. */
AFuncRegistry *registry_new(unsigned int initial_capacity);

/* Register a new function into the User Func Registry. */
void registry_register(AFuncRegistry *reg, AFunc *f);

/* Free all functions in the User Func Registry. */
void free_registry(AFuncRegistry *reg);

#endif
