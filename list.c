#include "list.h"

/* TODO need list free code */

/* Allocate a new blank list. */
AList *list_new() {
    AList *list = malloc(sizeof(list));
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
AList *list_reify(AVarBuffer *buf, AProtoList *proto) {
    AWordSeqNode *current = proto->first;
    AList *list = list_new();

    while (current) {
        /* create a new tiny stack to evaluate this on */
        AStack *tmp = stack_new(2);
        eval_sequence(tmp, buf, current);
        /* get the top element of the stack, and issue a warning
         * if there's more than one element on the stack */
        /* TODO add a more specific error msg */
        if (tmp->size > 1) {
            fprintf(stderr, "warning: more than one stack element generated "
                            "by expression in list; ignoring all but top\n");
        }
        /* get a new reference to the first element */
        AValue *val = stack_get(tmp, 0);
        free_stack(tmp);
        /* put the element onto the list */
        list_cons(val, list);
        /* move to the next element */
        current = current->next;
    }

    return list;
}

/* Print out a list. */
void print_list(AList *l) {
    AListElem *current = l->first;
    printf("{ ");
    while (current) {
        print_val(current->val);
        if (current->next != NULL) printf(", ");
        current = current->next;
    }
    printf(" }");
}
