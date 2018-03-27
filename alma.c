#include <stdio.h>
#include "alma.h"
#include "parse.h"
#include "ast.h"
#include "eval.h"
#include "scope.h"
#include "lib.h"
#include "compile.h"
#include "grammar.tab.h"
#include "registry.h"

#define CLEANUP() \
        free_stack(stack); \
        free_registry(reg); \
        free_decl_seq(program); \
        free_lib_scope(lib_scope); \
        free_symbol_table(&symtab)

int run_program(ADeclSeqNode *program, ASymbolTable symtab);

int main (int argc, char **argv) {
    ADeclSeqNode *program = NULL;
    ASymbolTable symtab = NULL;

    FILE *infile = NULL;
    if (argc == 2) {
        infile = fopen(argv[1], "r");
    } else {
        fprintf(stderr, "Please supply a file name.\n");
        return 0;
    }

    parse_file(infile, &program, &symtab);

    fclose(infile);

    int result = run_program(program, symtab);

    return result;
}

int run_program(ADeclSeqNode *program, ASymbolTable symtab) {
    if (program == NULL) {
        fprintf(stderr, "Compilation aborted.\n");
        return 0;
    } else {
        AStack *stack = stack_new(20);
        AScope *lib_scope = scope_new(NULL);

        AFuncRegistry *reg = registry_new(20);

        lib_init(symtab, lib_scope);

        AScope *real_scope = scope_new(lib_scope);

        ACompileStatus stat = compile(real_scope, reg, program);

        free_scope(real_scope);

        if (stat == compile_fail) {
            CLEANUP();
            return 1;
        }

        /* For now, we don't have declarations so just
         * call the first declared function... */
        eval_sequence(stack, real_scope, program->first->node);

        CLEANUP();
        return 0;
    }
}
