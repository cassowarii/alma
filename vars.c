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
        if (index == newbind->count) {
            fprintf(stderr, "internal error: received too many symbols at bind "
                            "(expected %d)\n", newbind->count);
            fprintf(stderr, "probably the nameseq misreported its length.\n");
            return newbind;
        }
        index++;
        curr = curr->next;
    }

    return newbind;
}

/* Create a new VarBuffer with size <size> and parent <parent>. */
AVarBuffer *varbuf_new(AVarBuffer *parent, unsigned int size) {
    AVarBuffer *newbuf = malloc(sizeof(AVarBuffer));
    // TODO check buf alloc
    newbuf->vars = malloc(size * sizeof(AValue*));
    newbuf->size = size;
    newbuf->parent = parent;
    return newbuf;
}

/* Put a value into <buf> at index <index> */
void varbuf_put(AVarBuffer *buf, unsigned int index, AValue *val) {
    if (index >= buf->size) {
        fprintf(stderr, "internal error: trying to put too many values into bind "
                        "(bufsize: %d, index %d)\n", buf->size, index);
        return;
    }
    buf->vars[index] = val;
}

/* Get the <num>'th variable from a VarBuffer, looking it up
 * in the parent if necessary. */
/* Returns a new reference to the value. */
AValue *varbuf_get(AVarBuffer *buf, unsigned int index) {
    if (buf == NULL) {
        /* this means there are no more variables to look at! :( */
        /* this shouldn't happen because we only look for variables within the range */
        fprintf(stderr, "internal error: attempt to get nonexistent variable\n");
        return NULL;
    }
    if (index < buf->size) {
        return buf->vars[index];
    } else {
        return ref(varbuf_get(buf->parent, index - buf->size));
    }
}
