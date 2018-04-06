#include "compile.h"

/* Okay, this technically limits the number of lexical variables that can
 * be in scope at a time to 100,000. I think that is a reasonable restriction. */
unsigned int NOFREEVARS = 100000;

/* Mutate an AWordSeqNode by replacing compile-time-resolvable words
 * by their corresponding AFunc*s found in scope. (var_depth is how
 * many variables are in scopes below, so we can pass the correct indices
 * to scope_create_push. */
static
ACompileResult compile_wordseq(AScope *scope, AFuncRegistry *reg, AWordSeqNode *seq, ABindInfo bindinfo) {
    if (seq == NULL) {
        ACompileResult nil_result = {compile_success, NOFREEVARS};
        return nil_result;
    }
    unsigned int errors = 0;

    int free_variable_index = NOFREEVARS;

    AAstNode *current = seq->first;
    while (current != NULL) {
        if (current->type == value_node) {
            if (current->data.val->type == proto_block) {
                /* Block values need special extra compilation. */
                /* Set the last-block-depth to the current var depth. (so any variables from
                 * outside the block will be correctly recognized as 'free' variables.) */
                ABindInfo bindinfo_block = {bindinfo.var_depth, bindinfo.var_depth};
                ACompileResult blockstat = compile_wordseq(scope,
                        reg, current->data.val->data.ast, bindinfo_block);
                if (blockstat.status == compile_fail) {
                    errors ++;
                } else if (blockstat.status == compile_success) {
                    if (blockstat.lowest_free == NOFREEVARS) {
                        /* The block is compiled. */
                        current->data.val->type = block_val;
                    } else {
                        /* The block is compiled, but it'll need to hold onto a closure. */
                        current->data.val->type = free_block_val;
                        if (blockstat.lowest_free < free_variable_index) {
                            free_variable_index = blockstat.lowest_free;
                        }
                    }
                } else {
                    fprintf(stderr, "internal error: unrecognized compile status %d "
                                    "while compiling a block.\n", blockstat.status);
                    errors ++;
                }
            } else if (current->data.val->type == proto_list) {
                /* Compile the things in the protolist. */
                AWordSeqNode *plcurrent = current->data.val->data.pl->first;
                while (plcurrent != NULL) {
                    ACompileResult plstat = compile_wordseq(scope, reg, plcurrent, bindinfo);
                    if (plstat.status == compile_fail) {
                        errors ++;
                    } else if (plstat.status == compile_success) {
                        if (plstat.lowest_free == NOFREEVARS) {
                            /* Great! */
                        } else {
                            /* A free variable was referenced within the list. */
                            if (plstat.lowest_free < free_variable_index) {
                                free_variable_index = plstat.lowest_free;
                            }
                        }
                    } else {
                        fprintf(stderr, "internal error: unrecognized compile status %d "
                                        "while compiling a list.\n", plstat.status);
                        errors ++;
                    }
                    plcurrent = plcurrent->next;
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
                fprintf(stderr, "error: unknown word ‘%s’ <%p> at line %d.\n",
                        current->data.sym->name, current->data.sym, current->linenum);
                errors ++;
            } else {
                /* Change symbol pointer to function pointer. */
                current->type = func_node;
                current->data.func = e->func;
                if (e->func->type == var_push && e->func->data.varindex < bindinfo.last_block_depth) {
                    /* It's a free variable (comes from outside the innermost containing
                     * block.) Thus, this block needs to be closed over at runtime. */
                    /* So, update the 'lowest-index free variable' if necessary. */
                    if (e->func->data.varindex < free_variable_index) {
                        free_variable_index = e->func->data.varindex;
                    }
                }
                if (e->func->type == user_func) {
                    if (e->func->data.userfunc->free_var_index < free_variable_index) {
                        /* It is a function that contains a free variable. So wherever it
                         * occurs, we should consider to have a free variable as well so the
                         * runtime knows to save a closure for that block. */
                        free_variable_index = e->func->data.userfunc->free_var_index;
                    }
                }
            }
        } else if (current->type == let_node) {
            /* Create a new lexical scope! */
            AScope *child_scope = scope_new(scope);

            /* We need to create a new closure for all functions being declared in this
             * node. (This lets us detect which variables are external to the
             * particular function declarations in the let..in part.) */
            ABindInfo closed = {bindinfo.var_depth, bindinfo.var_depth};

            /* Compile the declarations into this lexical scope. */
            ACompileStatus stat = compile(child_scope, reg, current->data.let->decls, closed);

            if (stat == compile_fail) {
                errors ++;
            } else if (stat != compile_success) {
                fprintf(stderr, "internal error: unrecognized compile status %d in pass 2.\n",
                        stat);
                errors ++;
            }

            /* Compile the executed part using the new scope. */
            ACompileResult r = compile_wordseq(child_scope, reg, current->data.let->words, bindinfo);

            if (r.status == compile_fail) {
                errors ++;
            } else if (r.status == compile_success) {
                /* Free variables inside the body-section of the let, so free variables
                 * in the larger expression. */
                if (r.lowest_free < free_variable_index) {
                    free_variable_index = r.lowest_free;
                }
            } else {
                fprintf(stderr, "internal error: unrecognized compile status %d in pass 2.\n",
                        r.status);
                errors ++;
            }

            /* Free the new scope. (Don't worry, its functions will stay behind!) */
            free_scope(child_scope);
        } else if (current->type == bind_node) {
            /* Create a new bind instruction */
            AVarBind *newbind = varbind_new(current->data.bind->names, current->data.bind->words);

            AScope *scope_with_vars = scope_new(scope);

            ACompileStatus stat;

            ANameNode *currname = current->data.bind->names->first;

            for (int i = 0; i < newbind->count; i++) {
                stat = scope_create_push(scope_with_vars, reg, currname->sym,
                                         bindinfo.var_depth + i, current->linenum);
                if (stat == compile_fail) {
                    errors ++;
                } else if (stat != compile_success) {
                    fprintf(stderr, "internal error: scope_create_push returned weird status %d", stat);
                }
                currname = currname->next;
            }

            ABindInfo bindinfo_with_vars = {bindinfo.var_depth + newbind->count, bindinfo.last_block_depth};

            ACompileResult r = compile_wordseq(scope_with_vars, reg, newbind->words, bindinfo_with_vars);

            free_nameseq_node(current->data.bind->names);

            if (r.status == compile_fail) {
                errors ++;
            } else if (r.status == compile_success) {
                /* Successfully compiled the inner scope, so alter the node. */
                free(current->data.bind);
                current->type = var_bind;
                current->data.vbind = newbind;
                if (r.lowest_free < bindinfo.var_depth) {
                    /* If the lowest free variable inside the bind-expression came from below
                     * this bind scope, we still have free variables. Otherwise, this is a
                     * self-contained bind expression and stuff outside it doesn't need to
                     * worry about closures. */
                    if (r.lowest_free < free_variable_index) {
                        free_variable_index = r.lowest_free;
                    }
                }
            } else {
                fprintf(stderr, "internal error: unrecognized compile status %d compiling bindnode\n",
                        r.status);
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
        ACompileResult fail_result = {compile_fail, free_variable_index};
        return fail_result;
    }

    ACompileResult result = {compile_success, free_variable_index};
    return result;
}

/* Mutate an ADeclSeqNode by replacing compile-time-resolvable
 * symbol references with references to AFunc*'s. */
/* var_depth = how many variables are bound below this scope */
ACompileStatus compile(AScope *scope, AFuncRegistry *reg, ADeclSeqNode *program, ABindInfo bindinfo) {
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
        ACompileStatus stat;
        ACompileResult r = compile_wordseq(scope, reg, current->node, bindinfo);

        /* ... check for errors ... */
        if (r.status == compile_fail) {
            fprintf(stderr, "Failed to compile word ‘%s’.\n", current->sym->name);
            errors ++;
            current = current->next;
            continue;
        } else if (r.status == compile_success) {
            stat = scope_user_register(scope, current->sym, r.lowest_free,
                                       bindinfo.var_depth, current->node);
        } else {
            fprintf(stderr, "internal error: unrecognized compile status %d in pass 2.\n", r.status);
            current = current->next;
            errors ++;
            continue;
        }

        // This is also where we'll eventually typecheck stuff before registering it.
        // .. Or will we do that in a third pass? Hmm.

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

/* Compile a given declseq in context of preexisting symbol table, registry, scope. */
ACompileStatus compile_in_context(ADeclSeqNode *program,
        ASymbolTable symtab, AFuncRegistry *reg, AScope *scope) {
    /* We start with no variables! */
    ABindInfo bi = {0, 0};
    ACompileStatus stat = compile(scope, reg, program, bi);

    return stat;
}

/* Compile a given wordseq in context of preexisting symbol table, registry, scope. */
ACompileStatus compile_seq_context(AWordSeqNode *seq,
        ASymbolTable symtab, AFuncRegistry *reg, AScope *scope) {
    /* We start with no variables! */
    ABindInfo bi = {0, 0};
    ACompileResult r = compile_wordseq(scope, reg, seq, bi);

    return r.status;
}
