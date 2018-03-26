#include <stdio.h>
#include "alma.h"
#include "ast.h"
#include "eval.h"
#include "scope.h"
#include "lib.h"
#include "parse.h"
#include "grammar.tab.h"

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

    if (program == NULL) {
        fprintf(stderr, "Compilation aborted.\n");
    } else {
        AStack *stack = stack_new(20);

        AScope *scope = scope_new(NULL);

        lib_init(symtab, scope);

        /* For now, we don't have declarations so just
         * call the first declared function... */
        eval_sequence(stack, scope, program->first->node);

        print_stack(stack);

        free_stack(stack);
        free_decl_seq(program);
        free_scope(scope);
        free_symbol_table(&symtab);
    }

    fclose(infile);
}
