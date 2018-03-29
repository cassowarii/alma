#include "compile.h"

/* Mutate an AWordSeqNode by replacing compile-time-resolvable words
 * by their corresponding AFunc*s found in scope. */
static
ACompileStatus compile_wordseq(AScope *scope, AFuncRegistry *reg, AWordSeqNode *seq) {
    if (seq == NULL) return compile_success;
    unsigned int errors = 0;

    AAstNode *current = seq->first;
    while (current != NULL) {
        if (current->type == value_node) {
            if (current->data.val->type == proto_block) {
                /* Block values need special extra compilation. */
                /* Compile the block. */
                ACompileStatus blockstat = compile_wordseq(scope,
                        reg, current->data.val->data.ast);
                if (blockstat == compile_fail) {
                    errors ++;
                } else if (blockstat != compile_success) {
                    fprintf(stderr, "internal error: unrecognized compile status %d "
                                    "while compiling a block.\n", blockstat);
                    errors ++;
                } else {
                    /* The block is compiled. */
                    current->data.val->type = block_val;
                }
            }
            /* For now, we otherwise just assume the value is fine as is. */
        } else if (current->type == func_node) {
            /* we shouldn't find this in an uncompiled AST! */
            fprintf(stderr, "internal error: node already compiled\n");
            errors ++;
        } else if (current->type == paren_node) {
            /* these should all be gone by this point (only used in parse stage) */
            fprintf(stderr, "internal error: paren_node found in compilation stage\n");
            errors ++;
        } else if (current->type == word_node) {
            /* Find the function bound to its name. */
            AScopeEntry *e = scope_lookup(scope, current->data.sym);
            if (e == NULL) {
                fprintf(stderr, "error: unknown word ‘%s’ at line %d.\n",
                        current->data.sym->name, current->linenum);
                errors ++;
            } else {
                /* Change symbol pointer to function pointer. */
                current->type = func_node;
                current->data.func = e->func;
            }
        } else if (current->type == let_node) {
            /* Create a new lexical scope! */
            AScope *child_scope = scope_new(scope);

            /* Compile the declarations into this lexical scope. */
            ACompileStatus stat = compile(child_scope, reg, current->data.let->decls);

            if (stat == compile_fail) {
                errors ++;
            } else if (stat != compile_success) {
                fprintf(stderr, "internal error: unrecognized compile status %d in pass 2.\n",
                        stat);
                errors ++;
            }

            /* Compile the executed part using the new scope. */
            stat = compile_wordseq(child_scope, reg, current->data.let->words);

            if (stat == compile_fail) {
                errors ++;
            } else if (stat != compile_success) {
                fprintf(stderr, "internal error: unrecognized compile status %d in pass 2.\n",
                        stat);
                errors ++;
            }

            /* Free the new scope. (Don't worry, its functions will stay behind!) */
            free_scope(child_scope);
        } else if (current->type == bind_node) {
            /* Create a new bind instruction */
            AVarBind *newbind = varbind_new(current->data.bind->names, current->data.bind->words);

            AScope *scope_with_vars = scope_new(scope);

            /* ... TODO register the names into this scope ... */

            ACompileStatus stat = compile_wordseq(scope_with_vars, reg, newbind->words);

            if (stat == compile_fail) {
                errors ++;
            } else if (stat != compile_success) {
                fprintf(stderr, "internal error: unrecognized compile status %d compiling bindnode\n",
                        stat);
                errors ++;
            }

            /* Free scope. */
            free_scope(scope_with_vars);
        } else {
            fprintf(stderr, "Don't yet know how to compile node type %d\n", current->type);
        }
        current = current->next;
    }

    if (errors > 0) {
        return compile_fail;
    }

    return compile_success;
}

/* Mutate an ADeclSeqNode by replacing compile-time-resolvable
 * symbol references with references to AFunc*'s. */
ACompileStatus compile(AScope *scope, AFuncRegistry *reg, ADeclSeqNode *program) {
    if (program == NULL) return compile_success;
    unsigned int errors = 0;
    ADeclNode *current;

    /*-- PASS 1: check names being defined --*/
    current = program->first;
    /* (We do this in a separate pass so that functions being defined can
     * refer to functions later without fear.) */
    while (current != NULL) {
        /* Mark that the function will be compiled later. */
        ACompileStatus stat = scope_placehold(scope, reg, current->sym, current->linenum);

        if (stat == compile_fail) {
            errors ++;
        } else if (stat != compile_success) {
            /* in the future, i will probably add another status and
             * forget to check for it here. future proofing */
            fprintf(stderr, "internal error: unrecognized compile status %d in pass 1.\n", stat);
            errors ++;
        }

        current = current->next;
    }

    if (errors != 0) {
        /* If we accidentally defined two functions with the same name in the
         * same scope, bail out now. */
        return compile_fail;
    }

    /*-- PASS 2: compile symbols we can resolve, convert to func ptrs --*/
    current = program->first;
    while (current != NULL) {
        ACompileStatus stat = compile_wordseq(scope, reg, current->node);

        /* ... check for errors ... */
        if (stat == compile_fail) {
            fprintf(stderr, "Failed to compile word ‘%s’.\n", current->sym->name);
            errors ++;
            current = current->next;
            continue;
        } else if (stat != compile_success) {
            fprintf(stderr, "internal error: unrecognized compile status %d in pass 2.\n", stat);
            current = current->next;
            errors ++;
            continue;
        }

        // This is also where we'll eventually typecheck stuff before registering it.
        // .. Or will we do that in a third pass? Hmm.

        stat = scope_user_register(scope, current->sym, const_func, current->node);

        if (stat == compile_fail) {
            fprintf(stderr, "Failed to compile word ‘%s’.\n", current->sym->name);
            errors ++;
        } else if (stat != compile_success) {
            fprintf(stderr, "internal error: unrecognized compile status %d in pass 2.\n", stat);
        }

        current = current->next;
    }

    if (errors != 0) {
        return compile_fail;
    }

    return compile_success;
}
