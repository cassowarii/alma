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
    newbuf->vars = calloc(size, sizeof(AValue*));
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
    if (index >= buf->size) {
        fprintf(stderr, "internal error: trying to put value too far into varbuf "
                        "(bufsize: %d, index %d)\n", buf->size, index);
        return;
    }
    if (buf->vars[index] != NULL) {
        fprintf(stderr, "internal error: trying to put value in same slot in varbuf (%d)", index);
        return;
    }
    buf->vars[index] = val;
}

/* Get the <num>'th variable from a VarBuffer, looking it up
 * in the parent if necessary. */
/* Returns a new reference to the value. */
AValue *varbuf_get(AVarBuffer *buf, unsigned int index) {
    if (buf == NULL) {
        fprintf(stderr, "internal error: attempt to get nonexistent variable\n");
        return NULL;
    }
    if (index >= buf->base) {
        if (index - buf->base < buf->size) {
            return ref(buf->vars[index - buf->base]);
        } else {
            /* this shouldn't happen because we only create 'get' instructions
             * whose number is less than the max. varbuf index */
            fprintf(stderr, "internal error: attempt to get var with too-high index %d", index);
            return NULL;
        }
    } else {
        return varbuf_get(buf->parent, index);
    }
}
