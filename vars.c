#include "vars.h"

/* Create a new var-bind instruction, with the names from the
 * <names> ANameSeqNode. */
AVarBind *varbind_new(ANameSeqNode *names, AWordSeqNode *words) {
    if (names->length == 0) return NULL; /* bind no new names */

    AVarBind *new_bind = malloc(sizeof(AVarBind));
    new_bind->count = names->length;
    new_bind->syms = malloc(new_bind->count * sizeof(ASymbol*));
    new_bind->words = words;

    ANameNode *curr = names->first;
    unsigned int index = 0;
    while (curr) {
        new_bind->syms[index] = curr->sym;
        index++;
        if (index == new_bind->count) {
            fprintf(stderr, "internal error: received too many symbols at bind\n");
            return new_bind;
        }
        curr = curr->next;
    }

    return new_bind;
}
