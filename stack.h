#ifndef _AL_STACK_H__
#define _AL_STACK_H__

#include "alma.h"
#include "value.h"

/* Allocate and initialize a new AStack. */
AStack *stack_new(size_t initial_size);

/* Get a value 'n' places down from the top of the stack. */
AValue *stack_get(AStack *st, int n);

/* Peek at the value on the stack, but don't get a fresh reference to it.
 * (Useful in the unit tests) */
AValue *stack_peek(AStack *st, int n);

/* Push something onto the stack. */
void stack_push(AStack *st, AValue *v);

/* Reduce the stack size by 'n'. */
void stack_pop(AStack *st, int n);

/* Print the contents of the stack. */
void print_stack(AStack *st);

/* Clear the stack, dereferencing all the variables on it,
 * then free the stack.
 * For cleanup at the end of the program. */
void free_stack(AStack *st);

#endif
