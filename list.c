#include "list.h"

/* Allocate a new blank list. */
AList *list_new() {
    AList *list = malloc(sizeof(AList));
    list->first = NULL;
    list->last = NULL;
    list->length = 0;
    return list;
}

/* Allocate a new list-element and set it to
 * point to <val>. */
static
AListElem *create_element(AValue *val) {
    AListElem *elem = malloc(sizeof(AListElem));
    elem->val = val;
    elem->next = NULL;
    return elem;
}

/* Push an AValue* onto the front of a list,
 * mutating the list. */
void list_cons(AValue *val, AList *list) {
    AListElem *elem = create_element(val);
    elem->next = list->first;
    list->first = elem;
    if (list->last == NULL) {
        list->last = elem;
    }
    list->length ++;
}

/* Put an AValue* at the end of a list,
 * mutating the list. */
void list_append(AList *list, AValue *val) {
    AListElem *elem = create_element(val);
    if (list->last != NULL) {
        list->last->next = elem;
    }
    list->last = elem;
    if (list->first == NULL) {
        list->first = elem;
    }
    list->length ++;
}

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
AList *list_reify(AVarBuffer *buf, AProtoList *proto, unsigned int linenum) {
    AWordSeqNode *current = proto->first;
    AList *list = list_new();

    while (current) {
        /* create a new tiny stack to evaluate this on */
        AStack *tmp = stack_new(2);
        eval_sequence(tmp, buf, current);
        /* get the top element of the stack, and issue a warning
         * if there's more than one element on the stack */
        if (tmp->size > 1) {
            /* TODO resolve this weird mismatch -- the expression will
             * be printed out to stdout, but everything else to stderr */
            fprintf(stderr, "warning: expression ‘");
            fprint_wordseq_node(stderr, current);
            fprintf(stderr, "’ in list at line %d generated %d elements, "
                            "but only the first one will be used\n",
                            linenum, tmp->size);
        }
        /* get a new reference to the first element */
        AValue *val = stack_get(tmp, 0);
        free_stack(tmp);
        /* put the element onto the list */
        list_append(list, val);
        /* move to the next element */
        current = current->next;
    }

    return list;
}

/* Print out a list to an arbitrary filehandle. */
void fprint_list(FILE *out, AList *l) {
    AListElem *current = l->first;
    fprintf(out, "{ ");
    while (current) {
        fprint_val(out, current->val);
        if (current->next != NULL) fprintf(out, ", ");
        current = current->next;
    }
    fprintf(out, " }");
}

/* Print out a list. */
void print_list(AList *l) {
    fprint_list(stdout, l);
}

/* Free a list. */
void free_list(AList *l) {
    AListElem *current = l->first;
    while (current) {
        AListElem *next = current->next;
        delete_ref(current->val);
        free(current);
        current = next;
    }
    free(l);
}
