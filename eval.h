#ifndef _AL_EVAL_H__
#define _AL_EVAL_H__

#include "alma.h"
#include "value.h"
#include "stack.h"
#include "ast.h"

/* Evaluate a sequence of commands on a stack,
 * mutating the stack. */
void eval_sequence(AStack *st, AWordSeqNode *seq);

/* Evaluate a single AST node on a stack, mutating
 * the stack.  */
void eval_node(AStack *st, AAstNode *seq);

#endif
