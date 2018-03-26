#include <stdio.h>
#include "alma.h"
#include "ast.h"
#include "eval.h"
#include "grammar.tab.h"

typedef void* yyscan_t;

int yyparse(yyscan_t scanner, ADeclSeqNode **out);
int yylex_init(yyscan_t scanner);
int yylex_destroy(yyscan_t scanner);

int main (int argc, char **argv) {
    yyscan_t scanner;

    yylex_init(&scanner);

    ADeclSeqNode *program = NULL;
    yyparse(scanner, &program);

    if (program == NULL) {
        fprintf(stderr, "Compilation aborted.\n");
    } else {
        AStack *stack = stack_new(20);

        yylex_destroy(scanner);

        print_decl_seq(program);

        /* For now, we don't have declarations so just
         * call the first declared function... */
        eval_sequence(stack, program->first->node);

        print_stack(stack);

        free_stack(stack);

        free_decl_seq(program);
    }
}
