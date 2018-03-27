#ifndef _AL_AST_H__
#define _AL_AST_H__

#include "alma.h"

/* Pushing a value */
AAstNode *ast_valnode(unsigned int location, AValue *val);

/* Calling a word */
AAstNode *ast_wordnode(unsigned int location, ASymbol *sym);

/* A sequence of words inside parentheses - we have a pointer to the first one */
AAstNode *ast_parennode(unsigned int location, AWordSeqNode *content);

/* A node representing a "let" introducing a scope. */
AAstNode *ast_letnode(unsigned int location, ADeclSeqNode *decls, AWordSeqNode *words);

/* A node representing a declaration. */
ADeclNode *ast_declnode(unsigned int location, ASymbol *sym, AWordSeqNode *body);

/* Create a new node representing name sequence. */
ANameNode *ast_namenode(unsigned int location, ASymbol *symbol);

/* Create a new node representing a name binding. */
AAstNode *ast_bindnode(unsigned int location, ANameSeqNode *names, AWordSeqNode *words);

/* Create a new node representing a declaration sequence. */
ADeclSeqNode *ast_declseq_new();

/* Create a new node representing a name sequence. */
ANameSeqNode *ast_nameseq_new();

/* Append a new declaration to an ADeclSeqNode. */
void ast_declseq_append(ADeclSeqNode *seq, ADeclNode *node);

/* Create a new node representing a word/value sequence. */
AWordSeqNode *ast_wordseq_new();

/* Prepend a new node to the beginning of an AWordSeqNode. */
void ast_wordseq_prepend(AWordSeqNode *seq, AAstNode *node);

/* Concatenate two AWordSeqNodes together. Doesn't free the second one! */
void ast_wordseq_concat(AWordSeqNode *seq1, AWordSeqNode *seq2);

/* Create a new node representing a list. */
void ast_wordseq_concat(AWordSeqNode *seq1, AWordSeqNode *seq2);

/* Prepend a new node to the beginning of an ANameSeqNode. */
void ast_nameseq_append(ANameSeqNode *seq, ANameNode *node);

/* Allocate a new AProtoList. */
AProtoList *ast_protolist_new();

/* Append a new word-sequence to an AProtoList. */
void ast_protolist_append(AProtoList *list, AWordSeqNode *node);

/*--- Printing ---*/

/* Print out an AST node. */
void print_ast_node(AAstNode *x);

/* Print out a protolist. */
void print_protolist(AProtoList *pl);

/* Print out an AST sequence. */
void print_wordseq_node(AWordSeqNode *x);

/* Print out a single declaration. */
void print_declaration(ADeclNode *a);

/* Print out a declaration sequence. */
void print_decl_seq(ADeclSeqNode *x);

/*--- Freeing ---*/

/* Free an AST node. */
void free_ast_node(AAstNode *to_free);

/* Free a protolist. */
void free_protolist(AProtoList *pl);

/* Free a word-sequence node. */
void free_wordseq_node(AWordSeqNode *to_free);

/* Free a declaration node COMPLETELY. (Careful!) */
void free_decl_node(ADeclNode *to_free);

/* Free a declaration sequence node COMPLETELY. (Careful!) */
void free_decl_seq(ADeclSeqNode *to_free);

#endif
