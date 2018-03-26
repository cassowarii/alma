#include "eval.h"

/* Evaluate a sequence of commands on a stack,
 * mutating the stack. */
void eval_sequence(AStack *st, AWordSeqNode *seq) {
    if (seq == NULL) return;        // it doesn't exist
    if (seq->first == NULL) return; // it's empty
    AAstNode *current = seq->first;
    while (current != NULL) {
        eval_node(st, current);
        current = current->next;
    }
}

/* Evaluate a single AST node on a stack, mutating
 * the stack.  */
void eval_node(AStack *st, AAstNode *node) {
    if (node->type == word_node) {
        fprintf(stderr, "error: word calls not yet implemented!!\n");
    } else if (node->type == value_node) {
        AValue *put = ref(node->data.val);
        stack_push(st, put);
    } else {
        fprintf(stderr, "error: unrecognized AST node type: %d\n", node->type);
    }
}
