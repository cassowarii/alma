#include "alma.h"

typedef void* yyscan_t;

int yyparse(yyscan_t scanner, ADeclSeqNode **out, ASymbolTable *t);
int yylex_init(yyscan_t scanner);
int yylex_destroy(yyscan_t scanner);
void yyset_in(FILE *in_str, yyscan_t scanner);

/* Parse a file pointer, and set program and symbol table. */
void parse_file(FILE *f, ADeclSeqNode **prog_out, ASymbolTable *symtab_out);
