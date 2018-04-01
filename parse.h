#ifndef _AL_PARSE_H__
#define _AL_PARSE_H__

#include "alma.h"

typedef void* yyscan_t;

struct yy_buffer_state;

/* Lex/bison functions we need to know about */
int yyparse(yyscan_t scanner, ADeclSeqNode **out, ASymbolTable *t);
int yylex_init(yyscan_t scanner);
int yylex_destroy(yyscan_t scanner);
void yyset_in(FILE *in_str, yyscan_t scanner);
AInteractive* yyset_extra(AInteractive* arbitrary_data, yyscan_t scanner);
AInteractive* yyset_extra(AInteractive* arbitrary_data, yyscan_t scanner);
void yylex(yyscan_t scanner);
struct yy_buffer_state *yy_scan_string(char *str, yyscan_t scanner);
void yy_flush_buffer(struct yy_buffer_state *buf, yyscan_t scanner);
void yy_switch_to_buffer(struct yy_buffer_state *buffer, yyscan_t scanner);
void yy_delete_buffer(struct yy_buffer_state *buffer, yyscan_t scanner);

/* Parse a file pointer, and set program and symbol table. */
void parse_file(FILE *f, ADeclSeqNode **prog_out, ASymbolTable *symtab_out);

#endif
