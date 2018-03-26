#include "eval.h"

/* Evaluate a sequence of commands on a stack,
 * mutating the stack. */
void eval_sequence(AStack *st, AScope *sc, AWordSeqNode *seq) {
    if (seq == NULL) return;        // it doesn't exist
    if (seq->first == NULL) return; // it's empty
    AAstNode *current = seq->first;
    while (current != NULL) {
        eval_node(st, sc, current);
        current = current->next;
    }
}

/* Evaluate a single AST node on a stack, mutating
 * the stack.  */
void eval_node(AStack *st, AScope *sc, AAstNode *node) {
    if (node->type == word_node) {
        AFunc *f = scope_lookup(sc, node->data.sym);
        eval_word(st, sc, f);
    } else if (node->type == value_node) {
        AValue *put = ref(node->data.val);
        stack_push(st, put);
    } else {
        fprintf(stderr, "error: unrecognized AST node type: %d\n", node->type);
    }
}

/* Evaluate a given word (whether declared or built-in)
 * on the stack. */
void eval_word(AStack *st, AScope *sc, AFunc *f) {
    if (f->type == builtin_func) {
        f->data.builtin(st, sc);
    } else if (f->type == declared_func) {
        fprintf(stderr, "declared func calls not yet implemented!\n"
                "also how are you even doing this\n");
    } else {
        fprintf(stderr, "error: unrecognized word type: %d\n", f->type);
    }
}
