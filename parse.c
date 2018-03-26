#include "parse.h"

/* Parse a file pointer, and set program and symbol table. */
void parse_file(FILE *f, ADeclSeqNode **prog_out, ASymbolTable *symtab_out) {
    if (f == NULL) {
        fprintf(stderr, "error: couldn't open file!\n");
        return;
    }

    yyscan_t scanner;

    yylex_init(&scanner);

    yyset_in(f, scanner);

    ADeclSeqNode *program = NULL;
    ASymbolTable symtab = NULL;

    yyparse(scanner, &program, &symtab);

    yylex_destroy(scanner);

    *prog_out = program;
    *symtab_out = symtab;
}
