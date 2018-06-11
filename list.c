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
    elem->prev = NULL;
    elem->next = list->first;
    if (list->first != NULL) {
        list->first->prev = elem;
    }
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
    elem->prev = list->last;
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
    while (current != NULL) {
        /* create a new tiny stack to evaluate this on */
        AStack *tmp = stack_new(2);
        eval_sequence(tmp, buf, current);
        /* get the top element of the stack, and issue a warning
         * if there's more than one element on the stack */
        if (tmp->size > 1) {
            fprintf(stderr, "warning: expression ‘");
            fprint_wordseq_node(stderr, current);
            fprintf(stderr, "’ in list at line %d generated %d elements, "
                            "but only the last one will be used\n",
                            linenum, tmp->size);
        } else if (tmp->size < 1) {
            fprintf(stderr, "warning: expression ‘");
            fprint_wordseq_node(stderr, current);
            fprintf(stderr, "’ in list at line %d generated no elements. "
                            "Skipping.\n",
                            linenum);
            current = current->next;
            continue;
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

/* Given a value of type 'list', return the tail
 * of the list. If it has no other references,
 * re-uses the original list value. */
/* Note: obviously this function is partial and
 * doesn't work on lists of length 0. */
AValue *tail_list_val(AValue *val) {
    if (val->data.list->length == 0) {
        fprintf(stderr, "error: attempt to take tail of empty list");
        return NULL;
    }

    if (val->refs == 1) {
        AListElem *oldfirst = val->data.list->first;
        AListElem *newfirst = val->data.list->first->next;
        AListElem *newlast = val->data.list->last;
        if (newfirst == NULL) {
            newlast = NULL;
        }
        val->data.list->first = newfirst;
        if (newfirst != NULL) {
            val->data.list->first->prev = NULL;
        }
        val->data.list->last = newlast;
        val->data.list->length --;

        /* don't need the old head element-holder anymore */
        delete_ref(oldfirst->val);
        free(oldfirst);

        return ref(val);
    } else {
        AList *newlist = list_new();
        AListElem *current = val->data.list->first->next;
        while (current) {
            list_append(newlist, ref(current->val));
            current = current->next;
        }
        return ref(val_list(newlist));
    }
}

/* Given a value of type 'list', return a list
 * containing all but the last element of the
 * original list.
 * of the list. If it has no other references,
 * re-uses the original list value. */
/* Also partial and returns NULL on lists
 * of length 0. */
AValue *init_list_val(AValue *val) {
    if (val->data.list->length == 0) {
        fprintf(stderr, "error: attempt to take init of empty list");
        return NULL;
    }

    if (val->refs == 1) {
        AListElem *oldlast = val->data.list->last;
        AListElem *newlast = val->data.list->last->prev;
        AListElem *newfirst = val->data.list->first;
        if (newlast == NULL) {
            newfirst = NULL;
        }
        val->data.list->last = newlast;
        if (newlast != NULL) {
            val->data.list->last->next = NULL;
        }
        val->data.list->first = newfirst;
        val->data.list->length --;

        /* don't need the old head element-holder anymore */
        delete_ref(oldlast->val);
        free(oldlast);

        return ref(val);
    } else {
        AList *newlist = list_new();
        AListElem *current = val->data.list->first;
        while (current->next) {
            list_append(newlist, ref(current->val));
            current = current->next;
        }
        return ref(val_list(newlist));
    }
}

/* Given a value of type 'list', return the head
 * of the list. (again, partial, list of length 0
 * has no head) */
/* (NOTE: doesn't destroy list object; returns a
 * fresh reference to head value) */
AValue *head_list_val(AValue *val) {
    if (val->data.list->length == 0) {
        fprintf(stderr, "error: attempt to take head of empty list");
        return NULL;
    }

    assert(val->data.list != NULL);
    assert(val->data.list->first != NULL);
    AValue *hval = ref(val->data.list->first->val);
    return hval;
}

/* Given a value of type 'list', return the last element
 * of the list. (again, partial, list of length 0
 * has no last) */
/* (NOTE: doesn't destroy list object; returns a
 * fresh reference to head value) */
AValue *last_list_val(AValue *val) {
    if (val->data.list->length == 0) {
        fprintf(stderr, "error: attempt to take last of empty list");
        return NULL;
    }

    assert(val->data.list != NULL);
    AValue *lval = ref(val->data.list->last->val);
    return lval;
}

/* Print out a list to an arbitrary filehandle. */
void fprint_list(FILE *out, AList *l) {
    AListElem *current = l->first;
    fprintf(out, "{ ");
    while (current) {
        fprint_val(out, current->val);
        if (current->next != NULL) {
            fprintf(out, ", ");
        } else {
            fprintf(out, " ");
        }
        current = current->next;
    }
    fprintf(out, "}");
}

/* Given a value and a value of type 'list', return
 * the value cons'd onto the front of the list.
 * Can reuse the list value if only has one reference. */
AValue *cons_list_val(AValue *val, AValue *l) {
    if (l->refs == 1) {
        list_cons(ref(val), l->data.list);
        return ref(l);
    } else {
        AList *newlist = list_new();
        AListElem *current = l->data.list->first;

        while (current) {
            assert(current->val != NULL);
            list_append(newlist, ref(current->val));
            current = current->next;
        }

        list_cons(ref(val), newlist);
        return ref(val_list(newlist));
    }
}

/* Given a value and a value of type 'list', return
 * the value appended to the end of the list.
 * Can reuse the list value if only has one reference. */
AValue *append_list_val(AValue *l, AValue *val) {
    if (l->refs == 1) {
        list_append(l->data.list, ref(val));
        return ref(l);
    } else {
        AList *newlist = list_new();
        AListElem *current = l->data.list->first;

        while (current) {
            assert(current->val != NULL);
            list_append(newlist, ref(current->val));
            current = current->next;
        }

        list_append(newlist, ref(val));
        return ref(val_list(newlist));
    }
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
