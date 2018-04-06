#include <stdio.h>
#include "alma.h"
#include "parse.h"
#include "ast.h"
#include "eval.h"
#include "scope.h"
#include "lib.h"
#include "compile.h"
#include "registry.h"

#define CLEANUP() \
        free_stack(stack); \
        free_registry(reg); \
        free_decl_seq(program); \
        free_lib_scope(lib_scope); \
        free_symbol_table(&symtab)

int compile_and_run(ADeclSeqNode *program, ASymbolTable symtab);

int main (int argc, char **argv) {
    ASymbolTable symtab = NULL;

    FILE *infile = NULL;
    if (argc == 2) {
        infile = fopen(argv[1], "r");
        if (infile == NULL) {
            fprintf(stderr, "couldn't open file\n");
            exit(1);
        }
    } else if (argc == 1) {
        interact(&symtab);
        exit(0);
    } else {
        fprintf(stderr, "Please supply one file name.\n");
        return 0;
    }

    //parse_file(infile, &program, &symtab);
    ADeclSeqNode *program = parse_file(infile, &symtab);

    fclose(infile);

    int result = compile_and_run(program, symtab);

    return result;
}

int compile_and_run(ADeclSeqNode *program, ASymbolTable symtab) {
    if (program == NULL) {
        fprintf(stderr, "Compilation aborted.\n");
        return 0;
    } else {
        AStack *stack = stack_new(20);
        AScope *lib_scope = scope_new(NULL);

        AFuncRegistry *reg = registry_new(20);
        lib_init(&symtab, lib_scope, 0);
        AScope *real_scope = scope_new(lib_scope);

        ACompileStatus stat = compile_in_context(program, symtab, reg, real_scope);

        /* Find main, before we free top-level scope.. */
        AFunc *mainfunc = scope_find_func(real_scope, symtab, "main");

        if (mainfunc == NULL) {
            fprintf(stderr, "error: cannot find ‘main’ function\n");
            free_scope(real_scope);
            CLEANUP();
            return 1;
        }

        free_scope(real_scope);

        /* Call main. */
        eval_word(stack, NULL, mainfunc);

        CLEANUP();
        return 0;
    }
}
