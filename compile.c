#include "compile.h"

/* Mutate an ADeclSeqNode by replacing compile-time-resolvable
 * symbol references with references to AFunc*'s. */
ACompileStatus compile(AScope *scope, ADeclSeqNode *program) {
    if (program == NULL) return compile_success;
    unsigned int errors = 0;
    ADeclNode *current = program->first;

    while (current != NULL) {
        ACompileStatus stat = scope_placehold(scope, current->sym);
        if (stat != compile_success) errors ++;

        current = current->next;
    }

    if (errors == 0) {
        return compile_success;
    } else {
        return compile_fail;
    }
}
