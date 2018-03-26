#ifndef _AL_EVAL_H__
#define _AL_EVAL_H__

#include "alma.h"
#include "value.h"
#include "stack.h"
#include "scope.h"
#include "ast.h"

/* Evaluate a sequence of commands on a stack,
 * mutating the stack. */
void eval_sequence(AStack *st, AScope *sc, AWordSeqNode *seq);

/* Evaluate a single AST node on a stack, mutating
 * the stack.  */
void eval_node(AStack *st, AScope *sc, AAstNode *seq);

/* Evaluate a given word (whether declared or built-in)
 * on the stack. */
void eval_word(AStack *st, AScope *sc, AFunc *f);

#endif
