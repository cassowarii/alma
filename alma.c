#include <stdio.h>
#include "alma.h"
#include "ast.h"
#include "eval.h"
#include "scope.h"
#include "lib.h"
#include "grammar.tab.h"

typedef void* yyscan_t;

int yyparse(yyscan_t scanner, ADeclSeqNode **out, ASymbolTable *t);
int yylex_init(yyscan_t scanner);
int yylex_destroy(yyscan_t scanner);

int main (int argc, char **argv) {
    yyscan_t scanner;

    yylex_init(&scanner);

    ADeclSeqNode *program = NULL;
    ASymbolTable symtab = NULL;

    yyparse(scanner, &program, &symtab);

    if (program == NULL) {
        fprintf(stderr, "Compilation aborted.\n");
    } else {
        yylex_destroy(scanner);

        AStack *stack = stack_new(20);

        AScope *scope = scope_new(NULL);

        lib_init(symtab, scope);

        /* For now, we don't have declarations so just
         * call the first declared function... */
        eval_sequence(stack, scope, program->first->node);

        print_stack(stack);

        free_stack(stack);

        free_decl_seq(program);
    }
}
