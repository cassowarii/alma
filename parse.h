#ifndef _AL_PARSE_H__
#define _AL_PARSE_H__

struct AParseState;

#include "alma.h"
#include "ast.h"
#include "symbols.h"
#include "value.h"
#include "lib.h"
#include "compile.h"

typedef struct YYLTYPE {
    unsigned int first_line;
    unsigned int last_line;
} YYLTYPE;

union lexresult {
    long i;
    char c;
    char *cs; // cstring
    struct AUstr *s;
    double d;
};

#define YYSTYPE union lexresult

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
    TOKENQUOTE  = '\'',
    TOKENLINE   = '\n',
    TOKENCMTLN  = '#',
    TOKENNONE   = 'N', /* "default" token at beginning */
    /* This one gets put into the try-list instead of the
     * individual literal tokens, so we don't say
     * "expecting integer literal or float literal or ...",
     * we can just say "expecting literal" */
    TOKENLIT    = 1000,
    /* This is returned for things inside comments which
     * would normally have something allocated for them
     * (e.g. strings.) Since we're in a comment, we don't
     * actually care about stuff that happens to be inside
     * of quotes, so we just BLAH them out. */
    TOKENBLAH   = 1010,
    /* Here are the actual token tokens. */
    T_IMPORT    = 257,
    T_AS,
    T_USE,
    T_BIND,
    T_DEF,
    T_IN,
    T_MATCH,
    T_END,
    WORD,
    SYMBOL,
    INTEGER,
    FLOAT,
    STRING,
    /* Opening and closing comments */
    CMTOPEN,
    CMTCLOSE,
} ATokenType;


typedef struct AToken {
    int id;
    YYSTYPE value;
    YYLTYPE loc;
} AToken;

typedef struct ATokenTypeList {
    ATokenType tok;
    struct ATokenTypeList *next;
    struct ATokenTypeList *prev;
} ATokenTypeList;

typedef struct AParseState {
    ASymbolTable *symtab;
    void *scan;
    AToken currtok;
    AToken nexttok;
    ATokenTypeList *tries;
    unsigned int errors;

    /* Interactive stuff */
    int interactive;
    char *current_string;
    unsigned int chars_left;
    unsigned int str_index;
    const char *prompt1;
    const char *prompt2;
    /* Parse counts:
     *  inlets: are we between a "let" and an "in"?
     *      (and if so how many nested ones?)
     *  infuncs: are we in the body of a func?
     *      (and if so how many nested ones?)
     *  nested_comments, etc: should be obvious
     *  beginning_line: are we at the beginning of a line?
     *      (if not, e.g. we used a backslash to escape a
     *       newline, so use prompt2 instead of prompt1)
     * These are used in interactive mode -- we stop
     * requesting more input when we see a ';' and aren't
     * inside of the declaration-sequence of a let.
     */
    unsigned short inlets;
    unsigned short infuncs;
    unsigned short nested_comments;
    unsigned short nested_parens;
    unsigned short nested_brackets;
    unsigned short nested_curlies;
    unsigned short beginning_line;
} AParseState;

#ifndef FLEX_SCANNER
/* this include will result in a couple
 * macros getting undef'd that are needed
 * by lex.yy.c. so we don't want to
 * include it if this file is included
 * by lex.yy.c */
#include "lex.h"
#endif

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

/* Parse interactive; ask for more text if necessary */
void interact(ASymbolTable *symtab, AScope *scope, AFuncRegistry *reg);

/* Reset parse state to state specified by <initial_state>. */
void reset_state(AParseState *state, AParseState initial_state);

#endif
