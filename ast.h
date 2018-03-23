#ifndef _AL_AST_H__
#define _AL_AST_H__

#include "alma.h"

/* Pushing a value */
AAstNode *ast_valnode(unsigned int location, AValue *val);

/* Calling a word */
AAstNode *ast_wordnode(unsigned int location, ASymbol *sym);

/* A sequence of words inside parentheses - we have a pointer to the first one */
AAstNode *ast_parennode(unsigned int location, AWordSeqNode *content);

/* A node representing a declaration. */
ADeclNode *ast_decl(unsigned int location, ASymbol *sym, AWordSeqNode *body);

/* Create a new node representing a declaration sequence. */
ADeclSeqNode *ast_declseq_new();

/* Append a new declaration to an ADeclSeqNode. */
void ast_declseq_append(ADeclSeqNode *seq, ADeclNode *node);

/* Create a new node representing a word/value sequence. */
AWordSeqNode *ast_wordseq_new();

/* Prepend a new node to the beginning of an AWordSeqNode. */
void ast_wordseq_prepend(AWordSeqNode *seq, AAstNode *node);

/* Concatenate two AWordSeqNodes together. Doesn't free the second one! */
void ast_wordseq_concat(AWordSeqNode *seq1, AWordSeqNode *seq2);

/* Print out an AST node. */
void print_ast_node(AAstNode *x);

/* Print out a single declaration. */
void print_declaration(ADeclNode *a);

/* Print out a declaration sequence. */
void print_decl_seq(ADeclSeqNode *x);

#endif
