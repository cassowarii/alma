#ifndef _AL_EVAL_H__
#define _AL_EVAL_H__

#include "alma.h"
#include "value.h"
#include "vars.h"
#include "stack.h"
#include "scope.h"
#include "ast.h"
#include "list.h"

/* Evaluate a sequence of commands on a stack,
 * mutating the stack. */
void eval_sequence(AStack *st, AVarBuffer *buf, AWordSeqNode *seq);

/* Evaluate a block (bound, constant, whatever) on the stack,
 * mutating the stack. */
void eval_block(AStack *st, AVarBuffer *buf, AValue *block);

/* Evaluate a single AST node on a stack, mutating
 * the stack.  */
void eval_node(AStack *st, AVarBuffer *buf, AAstNode *node);

/* Evaluate a given word (whether declared or built-in)
 * on the stack. */
void eval_word(AStack *st, AVarBuffer *buf, AFunc *f);

#endif
