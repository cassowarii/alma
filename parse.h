#ifndef _AL_PARSE_H__
#define _AL_PARSE_H__

#define YYSTYPE union lexresult

union lexresult {
    int i;
    char c;
    char *cs; // cstring
    struct AUstr *s;
    double d;
};

typedef struct YYLTYPE {
    unsigned int first_line;
    unsigned int last_line;
} YYLTYPE;

#define YY_EXTRA_TYPE struct extra *

struct extra {
    unsigned int nested_comments;
};

#ifndef FLEX_SCANNER
/* this include will result in a couple
 * macros getting undef'd that are needed
 * by lex.yy.c. so we don't want to
 * include it if this file is included
 * by lex.yy.c */
#include "lex.h"
#endif

#include "alma.h"
#include "ast.h"
#include "symbols.h"
#include "value.h"

void do_error(char *msg, unsigned int line);

struct yy_buffer_state;

/* Lex/bison functions we need to know about */
//int yyparse(yyscan_t scanner, ADeclSeqNode **out, ASymbolTable *t);
int yylex_init(yyscan_t *scanner);
int yylex_destroy(yyscan_t scanner);
void yyset_in(FILE *in_str, yyscan_t scanner);
YY_BUFFER_STATE yy_scan_string(yyconst char *str, yyscan_t scanner);
void yy_flush_buffer(YY_BUFFER_STATE buf, yyscan_t scanner);
void yy_switch_to_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);
void yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);

/* Parse a file pointer, and set program and symbol table. */
ADeclSeqNode *parse_file(FILE *infile, ASymbolTable *symtab);

typedef enum {
    /* These ones are just referred to by their character
     * anyway -- just putting them in here so the compiler
     * won't get confused about putting them in a switch. */
    TOKENEOF    = 0,
    TOKENCOLON  = ':',
    TOKENSEMI   = ';',
    TOKENCOMMA  = ',',
    TOKENPIPE   = '|',
    TOKENOPAREN = '(',
    TOKENCPAREN = ')',
    TOKENOBRACE = '{',
    TOKENCBRACE = '}',
    TOKENOBRACK = '[',
    TOKENCBRACK = ']',
    TOKENLINE   = '\n',
    /* Here are the actual token tokens. */
    T_IMPORT    = 257,
    T_AS,
    T_LET,
    T_BIND,
    T_FUNC,
    T_IN,
    WORD,
    SYMBOL,
    INTEGER,
    FLOAT,
    STRING,
    CMTCLOSE_ERRORTOKEN,
} ATokenType;

typedef struct AToken {
    int id;
    YYSTYPE value;
    YYLTYPE loc;
} AToken;

typedef struct AParseState {
    ASymbolTable *symtab;
    yyscan_t scan;
    AToken currtok;
    AToken nexttok;
    unsigned int errors;
} AParseState;

#endif
