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

/* Evaluate a block (bound, constant, whatever) on the stack,
 * mutating the stack. */
void eval_block(AStack *st, AVarBuffer *buf, AValue *block) {
    assert(block->type != free_block_val && "can't apply a free block!");
    if (block->type == block_val) {
        /* It's fine, just evaluate it */
        eval_sequence(st, buf, block->data.ast);
    } else if (block->type == bound_block_val) {
        /* It has an attached closure, so we need to load the closure
         * and interpret its contents in light of that */
        /* (Note how we pass block->data.uf->closure as the varbuffer
         * rather than buf) */
        eval_sequence(st, block->data.uf->closure, block->data.uf->words);
    } else {
        fprintf(stderr, "error: cannot apply non-block to stack\n");
    }
}

/* Evaluate a single AST node on a stack, mutating
 * the stack.  */
void eval_node(AStack *st, AVarBuffer *buf, AAstNode *node) {
    if (node->type == func_node) {
        eval_word(st, buf, node->data.func);
    } else if (node->type == value_node) {
        AValue *put;
        if (node->data.val->type != free_block_val) {
            /* If it's not a free block, we can just push its value
             * onto the stack without doing anything */
            put = ref(node->data.val);
        } else {
            /* Otherwise, we need to create a new bound-block from
             * this free block, which will save the current set of
             * variables. */
            put = ref(val_boundblock(node->data.val, buf));
        }
        stack_push(st, put);
    } else if (node->type == let_node) {
        /* We already handled all the declaration stuff in compilation,
         * so all we have to do for a let..in node is to execute the
         * 'in' part. */
        eval_sequence(st, buf, node->data.let->words);
    } else if (node->type == var_bind) {
        AVarBuffer *newbuf = varbuf_new(buf, node->data.vbind->count);
        varbuf_ref(newbuf);

        /* get the variables from the stack and put them in the var buffer */
        for (int i = 0; i < node->data.vbind->count; i++) {
            AValue *var = stack_get(st, i);
            varbuf_put(newbuf, i, var);
        }
        stack_pop(st, node->data.vbind->count);

        /* evaluate it with this new var-buffer */
        eval_sequence(st, newbuf, node->data.vbind->words);

        /* delete our reference to newbuf - this will clear it if we didn't
         * create any closures */
        varbuf_unref(newbuf);
    } else {
        /* Word nodes, bind nodes, and paren nodes should have been replaced
         * during the compile stage. If they made it here, that means something
         * has gone horribly wrong. */
        assert(node->type != word_node && "word-node in eval stage");
        assert(node->type != bind_node && "bind-node in eval stage");
        assert(node->type != paren_node && "paren-node in eval stage");
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
            AVarBuffer *func_buffer = varbuf_findparent(buf, f->data.userfunc->vars_below);
            varbuf_ref(func_buffer);
            eval_sequence(st, func_buffer, f->data.userfunc->words);
            varbuf_unref(func_buffer);
        } else if (f->data.userfunc->type == dummy_func) {
            assert(0 && "dummy-func in eval stage");
        } else {
            fprintf(stderr, "error: unrecognized userfunc type: %d\n",
                    f->data.userfunc->type);
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
