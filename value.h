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

/* Get a fresh pointer to the object that counts as a reference. */
AValue *ref(AValue *v);

/* Delete a reference to the object, reducing its refcount and
 * potentially freeing it. */
void delete_ref(AValue *v);

/* Print out a value */
void print_val(AValue *v);

/* Free a value. */
void free_value(AValue *to_free);

#endif
