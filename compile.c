#include "compile.h"

/* Mutate an ADeclSeqNode by replacing compile-time-resolvable
 * symbol references with references to AFunc*'s. */
ACompileStatus compile(AScope *scope, ADeclSeqNode *program) {
    if (program == NULL) return compile_success;
    unsigned int errors = 0;
    ADeclNode *current = program->first;

    while (current != NULL) {
        ACompileStatus stat = scope_placehold(scope, current->sym, current->linenum);

        if (stat == compile_fail) {
            errors ++;
        } else if (stat != compile_success) {
            /* in the future, i will probably add another status and
             * forget to check for it here. future proofing */
            fprintf(stderr, "internal error: unrecognized compile status %d.\n", stat);
            errors ++;
        }

        current = current->next;
    }

    if (errors == 0) {
        return compile_success;
    } else {
        return compile_fail;
    }
}
