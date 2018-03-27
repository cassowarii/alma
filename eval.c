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
    if (node->type == func_node) {
        /*AFunc *f = scope_lookup(sc, node->data.sym);*/
        eval_word(st, sc, node->data.func);
    } else if (node->type == word_node) {
        /* do nothing (for now at least) */
    } else if (node->type == value_node) {
        AValue *put = ref(node->data.val);
        stack_push(st, put);
    } else if (node->type == let_node) {
        eval_sequence(st, sc, node->data.let->words);
    } else {
        fprintf(stderr, "error: unrecognized AST node type: %d\n", node->type);
    }
}

/* Evaluate a given word (whether declared or built-in)
 * on the stack. */
void eval_word(AStack *st, AScope *sc, AFunc *f) {
    if (f->type == primitive_func) {
        f->data.primitive(st, sc);
    } else if (f->type == user_func) {
        if (f->data.userfunc->type == const_func) {
            eval_sequence(st, sc, f->data.userfunc->words);
        } else if (f->data.userfunc->type == dummy_func) {
            fprintf(stderr, "internal error: dummy func made it to eval stage\n");
        }
    } else {
        fprintf(stderr, "error: unrecognized word type: %d\n", f->type);
    }
}
