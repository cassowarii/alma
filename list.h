#ifndef _AL_LIST_H__
#define _AL_LIST_H__

#include "alma.h"
#include "stack.h"
#include "eval.h"
#include "value.h"

/* Allocate a new blank list. */
AList *list_new();

/* Push an AValue* onto the front of a list,
 * mutating the list. */
void list_cons(AValue *val, AList *list);

/* Put an AValue* at the end of a list,
 * mutating the list. */
void list_append(AList *list, AValue *val);

/* Create a new list from a proto-list. Note
 * that it (for now) just takes the top element
 * of the stack resulting from evaluating each
 * wordseq node, even though in theory you could
 * have a list structured like {1 2, 3 4, 5 6}.
 * I don't even know what the semantics of that
 * would be though? Maybe I'll make it do something
 * useful if I can think of what would be useful.
 * (Tuples? Matrices???? Too crazy?) */
/* Takes a varbuffer because, hey, there might
 * be lexical variables in that list! */
AList *list_reify(AVarBuffer *buf, AProtoList *proto, unsigned int linenum);

/* Given a value of type 'list', return the tail
 * of the list. If it has no other references,
 * re-uses the original list value. */
/* Note: obviously this function is partial and
 * doesn't work on lists of length 0. */
AValue *tail_list_val(AValue *val);

/* Given a value of type 'list', return the head
 * of the list. (again, partial, list of length 0
 * has no head) */
/* (NOTE: doesn't destroy list object; returns a
 * fresh reference to head value) */
AValue *head_list_val(AValue *val);

/* Print out a list. */
void print_list(AList *l);

/* Print out a list to an arbitrary filehandle. */
void fprint_list(FILE *out, AList *l);

/* Free a list. */
void free_list(AList *l);

#endif
