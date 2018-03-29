#include "eval.h"

/* Evaluate a sequence of commands on a stack,
 * mutating the stack. */
void eval_sequence(AStack *st, AVarBuffer *buf, AWordSeqNode *seq) {
    if (seq == NULL) return;        // it doesn't exist
    if (seq->first == NULL) return; // it's empty
    AAstNode *current = seq->first;
    while (current != NULL) {
        eval_node(st, buf, current);
        current = current->next;
    }
}

/* Evaluate a single AST node on a stack, mutating
 * the stack.  */
void eval_node(AStack *st, AVarBuffer *buf, AAstNode *node) {
    if (node->type == func_node) {
        eval_word(st, buf, node->data.func);
    } else if (node->type == value_node) {
        AValue *put = ref(node->data.val);
        stack_push(st, put);
    } else if (node->type == let_node) {
        eval_sequence(st, buf, node->data.let->words);
    } else if (node->type == var_bind) {
        AVarBuffer *newbuf = varbuf_new(buf, node->data.vbind->count);

        /* get the variables from the stack and put them in the var buffer */
        for (int i = 0; i < node->data.vbind->count; i++) {
            AValue *var = stack_get(st, i);
            varbuf_put(newbuf, i, var);
        }
        stack_pop(st, node->data.vbind->count);

        /* evaluate it with this new var-buffer */
        eval_sequence(st, newbuf, node->data.vbind->words);
    } else if (node->type == word_node || node->type == bind_node || node->type == paren_node) {
        fprintf(stderr, "internal error: node with obsolete type %d "
                        "made it to eval stage\n", node->type);
    } else {
        fprintf(stderr, "error: unrecognized AST node type: %d\n", node->type);
    }
}

/* Evaluate a given word (whether declared or built-in)
 * on the stack. */
void eval_word(AStack *st, AVarBuffer *buf, AFunc *f) {
    if (f->type == primitive_func) {
        f->data.primitive(st, buf);
    } else if (f->type == user_func) {
        if (f->data.userfunc->type == const_func) {
            eval_sequence(st, buf, f->data.userfunc->words);
        } else if (f->data.userfunc->type == dummy_func) {
            fprintf(stderr, "internal error: dummy func made it to eval stage\n");
        }
    } else if (f->type == var_push) {
        /* varbuf_get increments reference count */
        AValue *var = varbuf_get(buf, f->data.varindex);
        /* now put the variable on the stack */
        stack_push(st, var);
    } else {
        fprintf(stderr, "error: unrecognized word type: %d\n", f->type);
    }
}
