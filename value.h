#ifndef _AL_VAL_H__
#define _AL_VAL_H__

#include "alma.h"

/* Create a value holding an int */
AValue *val_int(int data);

/* Create a value holding a float */
AValue *val_float(float data);

/* Create a value holding a string */
AValue *val_str(AUstr *str);

/* Create a value holding a symbol */
AValue *val_sym(ASymbol *sym);

/* Create a value holding a block */
AValue *val_block(AWordSeqNode *block);

/* Create a value holding a proto-list (when parsing) */
AValue *val_protolist(AProtoList *pl);

/* Print out a value */
void print_val(AValue *v);

/* Free a symbol value. */
void free_symbol(ASymbol *sym);

#endif
