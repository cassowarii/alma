#ifndef _AL_STACK_H__
#define _AL_STACK_H__

#include "alma.h"

/* Allocate and initialize a new AStack. */
AStack *stack_new(size_t initial_size);

/* Get a value 'n' places down from the top of the stack. */
AValue *stack_get(AStack *st, int n);

/* Push something onto the stack. */
void stack_push(AStack *st, AValue *v);

/* Reduce the stack size by 'n'. */
void stack_pop(AStack *st, int n);

#endif
