#ifndef _AL_VARS_H__
#define _AL_VARS_H__

#include "alma.h"
#include "value.h"

/* Create a new var-bind instruction, with the names from the
 * <names> ANameSeqNode. */
AVarBind *varbind_new(ANameSeqNode *names, AWordSeqNode *words);

/* Create a new VarBuffer with size <size> and parent <parent>. */
AVarBuffer *varbuf_new(AVarBuffer *parent, unsigned int size);

/* Put a value into <buf> at index <index> */
void varbuf_put(AVarBuffer *buf, unsigned int index, AValue *val);

/* Get the <num>'th variable from a VarBuffer, looking it up
 * in the parent if necessary. */
/* Returns a new reference to the value. */
AValue *varbuf_get(AVarBuffer *buf, unsigned int index);

/* Increase the refcount of a varbuffer. Used when a closure is created,
 * so we don't free the varbuffer too early. */
void varbuf_ref(AVarBuffer *buf);

/* Decrease the refcount of a varbuffer, potentially freeing it if the
 * count drops to 0. */
void varbuf_unref(AVarBuffer *buf);

/* Free a varbuffer. Unreferences all the variables contained within,
 * and unreferences its parent as well. */
void varbuf_free(AVarBuffer *buf);

#endif
