#include "stack.h"

/* Allocate and initialize a new AStack. */
AStack *stack_new(size_t initial_size) {
    AStack *st = malloc(sizeof(AStack));
    st->content = malloc(initial_size * sizeof(AValue*));
    st->size = 0;
    st->capacity = initial_size;
    return st;
}

/* Get a value 'n' places down from the top of the stack. */
AValue *stack_get(AStack *st, int n) {
    if (n >= st->size) {
        fprintf(stderr, "Error: attempt to access too many elements from stack\n"
                        "(element accessed: #%d; stack size: %d)\n", n, st->size);
        return NULL;
    }
    /* Return a fresh reference. This allows us to then pop
     * stuff off and delete those references. */
    return ref(st->content[st->size - 1 - n]);
}

/* Peek at the value on the stack, but don't get a fresh reference to it.
 * (Useful in the unit tests) */
AValue *stack_peek(AStack *st, int n) {
    if (n >= st->size) {
        fprintf(stderr, "Error: attempt to access too many elements from stack\n"
                        "(element accessed: #%d; stack size: %d)\n", n, st->size);
        return NULL;
    }
    return st->content[st->size - 1 - n];
}

/* Push something onto the stack. */
void stack_push(AStack *st, AValue *v) {
    if (st->size == st->capacity) {
        AValue **new_array = realloc(st->content, st->capacity * 2 * sizeof(AValue*));
        if (new_array == NULL) {
            fprintf(stderr, "Error: couldn't grow stack from size %d to %d. Out of memory.",
                    st->capacity, st->capacity * 2);
        }
        st->capacity *= 2;
    }
    st->content[st->size] = v;
    st->size ++;
}

/* Reduce the stack size by 'n'. */
void stack_pop(AStack *st, int n) {
    for (unsigned int i = 0; i < n; i++) {
        /* Decrease the reference counter when values
         * get popped off. */
        printf("[");
        print_val(st->content[st->size - 1 - i]);
        printf("]");
        printf("Refs to <POP> %p: %d\n", (void*)st->content[st->size - 1 - i], st->content[st->size - 1 - i]->refs - 1);
        delete_ref(st->content[st->size - 1 - i]);
    }
    st->size -= n;
}

/* Print the contents of the stack. */
void print_stack(AStack *st) {
    for (int i = 0; i < st->size; i++) {
        if (i != 0) printf(" ; ");
        print_val(stack_get(st, i));
    }
    printf("\n");
}

/* Clear the stack, un-referencing all the variables on it,
 * then free the stack.
 * For cleanup at the end of the program. */
void free_stack(AStack *st) {
    if (st == NULL) return;
    stack_pop(st, st->size);
    free(st->content);
    free(st);
}
