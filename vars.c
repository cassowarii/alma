#include "vars.h"

/* Create a new var-bind instruction, with the names from the
 * <names> ANameSeqNode. */
AVarBind *varbind_new(ANameSeqNode *names, AWordSeqNode *words) {
    if (names->length == 0) return NULL; /* bind no new names */

    AVarBind *newbind = malloc(sizeof(AVarBind));
    newbind->count = names->length;
    newbind->words = words;

    ANameNode *curr = names->first;
    unsigned int index = 0;
    while (curr) {
        assert(index < newbind->count && "received too many symbols at bind");
        index++;
        curr = curr->next;
    }

    return newbind;
}

/* Create a new VarBuffer with size <size> and parent <parent>. */
AVarBuffer *varbuf_new(AVarBuffer *parent, unsigned int size) {
    AVarBuffer *newbuf = malloc(sizeof(AVarBuffer));
    if (newbuf == NULL) {
        fprintf(stderr, "error: cannot allocate space for a new var buffer: out of memory\n");
        return NULL;
    }
    newbuf->vars = malloc(size * sizeof(AValue*));
    newbuf->size = size;
    if (parent != NULL) {
        newbuf->base = parent->base + parent->size;
    } else {
        newbuf->base = 0;
    }
    newbuf->parent = parent;
    return newbuf;
}

/* Put a value into <buf> at index <index> */
void varbuf_put(AVarBuffer *buf, unsigned int index, AValue *val) {
    assert(index < buf->size && "trying to put value too far into varbuf");
    buf->vars[index] = val;
}

/* Get the <num>'th variable from a VarBuffer, looking it up
 * in the parent if necessary. */
/* Returns a new reference to the value. */
AValue *varbuf_get(AVarBuffer *buf, unsigned int index) {
    assert (buf != NULL);
    if (index >= buf->base) {
        /* this shouldn't happen because we only create 'get' instructions
         * whose number is less than the max. varbuf index */
        assert(index - buf->base < buf->size && "attempt to get var with too-high index");
        return ref(buf->vars[index - buf->base]);
    } else {
        return varbuf_get(buf->parent, index);
    }
}
