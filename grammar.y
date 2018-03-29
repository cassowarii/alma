%code requires {
#include "alma.h"
#include "value.h"
#include "ast.h"
#include "symbols.h"
}

%{
#define YYERROR_VERBOSE
#include <stdio.h>

extern int yychar;
extern int yylineno;

int yylex();
int yyparse();

int errors = 0;

void do_error(const char *str, int linenum) {
    fprintf(stderr, "error at line %d: %s\n", linenum, str);
    errors++;
}

%}

%parse-param {void *scanner} {ADeclSeqNode **out} {ASymbolMapping **symtab}

%lex-param {void *scanner}

%define api.pure

%define parse.lac full

/* track bison locations */
%locations

/* Reserved words */
%token T_IMPORT     "import"    /* "include" */
%token T_AS         "as"        /* "as" (but only after an 'import') */
%token T_LET        "let"       /* "let" */
%token T_BIND       "->"        /* "->" or "→" */
%token T_FUNC       "func"      /* "func" */
%token T_IN         "in"        /* "in" */

%token CMTCLOSE_ERRORTOKEN "*)" /* a block comment end. always syntactically invalid */

%token END 0        "end-of-file"
%union
{
    int i;
    char c;
    char *cs; // cstring
    struct AUstr *s;
    double d;
    struct ASymbol *sym;
    struct AValue* val;
    struct AAstNode* ast;
    struct AProtoList* pls;
    struct ANameSeqNode *nsq;
    struct AWordSeqNode *wsq;
    struct ADeclNode *dec;
    struct ADeclSeqNode *dsq;
}
%token <cs> WORD    "word"
%token <cs> SYMBOL  "symbol"
%token <i>  INTEGER "integer"
%token <i>  CHAR    "character"
%token <s>  STRING  "string"
%token <d>  FLOAT   "float"

%type <val> value;
%type <sym> name;
%type <wsq> block;
%type <pls> listcontent;
%type <pls> list;
%type <nsq> names;
%type <nsq> names_opt;
%type <ast> word;
%type <ast> cmplx_word;
%type <wsq> words;
%type <wsq> wordseq;
%type <wsq> wordseq_opt;
/* %type <wsq> words_nonempty; */
%type <dec> declaration;
%type <dec> directive;
%type <dsq> dirlist;
%type <dsq> realdirlist;
%type <dsq> program;

%{

void yyerror(YYLTYPE *loc, void *scan, ADeclSeqNode **out, ASymbolTable *symtab, const char *str) {
    do_error(str, loc->first_line);
}

int yywrap() {
    return 1;
}

%}

%%

main
    :   program {
        if (errors > 0) {
            // TODO free on error here
            *out = NULL;
        } else {
            *out = $1;
        }
    }

program
    :   dirlist {
        $$ = $1;
    }

dirlist
    :   /* empty */ {
    } | sep dirlist {
        $$ = $2;
    } | realdirlist {
        $$ = $1;
    }

realdirlist
    :   directive {
        $$ = ast_declseq_new();
        ast_declseq_append($$, $1);
    } | realdirlist directive {
        $$ = $1;
        ast_declseq_append($$, $2);
    } | realdirlist sep {
        $$ = $1;
    }

directive
    :   declaration {
        $$ = $1;
    } | import '.' {
    } | error sep {
    }

import
    :   "import" STRING {
    } | "import" STRING "as" WORD {
    }

declaration
    :   "func" name names_opt ':' words '.' {
        if ($3 == NULL) {
            $$ = ast_declnode(@2.first_line, $2, $5);
        } else {
            AWordSeqNode *wrapper = ast_wordseq_new();
            ast_wordseq_prepend(wrapper, ast_bindnode(@3.first_line, $3, $5));
            $$ = ast_declnode(@2.first_line, $2, wrapper);
        }
    } | error '.' {
        $$ = NULL;
    }

block
    :   '[' words ']' {
        $$ = $2;
    } | '[' "->" names ':' words ']' {
        $$ = ast_wordseq_new();
        ast_wordseq_prepend ($$, ast_bindnode(@2.first_line, $3, $5));
    }

words
    :   wordseq_opt {
        $$ = $1;
    } | words sep wordseq_opt {
        $$ = $1;
        ast_wordseq_concat($$, $3);
        free($3);
    }

/* words_nonempty
    :   wordseq {
        $$ = $1;
    } | words_nonempty sep wordseq_opt {
        $$ = $1;
        ast_wordseq_concat($$, $3);
        free($3);
    } */

wordseq_opt
    :   /* nothing */ {
        $$ = ast_wordseq_new();
    } | wordseq {
        $$ = $1;
    }

wordseq
    :   word {
        if ($1->type == paren_node) {
            $$ = $1->data.inside;
            free($1);
        } else {
            $$ = ast_wordseq_new();
            ast_wordseq_prepend($$, $1);
        }
    } | wordseq word {
        if ($2->type == paren_node) {
            $$ = $2->data.inside;
            ast_wordseq_concat($$, $1);
            free($2);
        } else {
            $$ = $1;
            ast_wordseq_prepend($$, $2);
        }
    }

word
    :   name {
        $$ = ast_wordnode(@1.first_line, $1);
    } | cmplx_word {
        $$ = $1;
    }

cmplx_word
    :   value {
        $$ = ast_valnode(@1.first_line, $1);
    } | "let" dirlist "in" nlo word {
        AWordSeqNode *words;
        if ($5->type == paren_node) {
            words = $5->data.inside;
            free($5);
        } else {
            words = ast_wordseq_new();
            ast_wordseq_prepend(words, $5);
        }
        $$ = ast_letnode(@1.first_line, $2, words);
    } | "->" names nlo cmplx_word {
        AWordSeqNode *words;
        if ($4->type == paren_node) {
            words = $4->data.inside;
            free($4);
        } else {
            words = ast_wordseq_new();
            ast_wordseq_prepend(words, $4);
        }
        $$ = ast_bindnode(@1.first_line, $2, words);
    } | '(' "->" names ':' words ')' {
        $$ = ast_bindnode(@2.first_line, $3, $5);
    } | '(' words ')' {
        $$ = ast_parennode(@1.first_line, $2);
    }

value
    :   INTEGER {
        $$ = val_int($1);
    } | STRING {
        $$ = val_str($1);
    } | FLOAT {
        $$ = val_float($1);
    } | SYMBOL {
        ASymbol *sym = get_symbol(symtab, $1);
        $$ = val_sym(sym);
        free($1);
    } | list {
        $$ = val_protolist($1);
    } | block {
        $$ = val_block($1);
    }

name
    :   WORD {
        $$ = get_symbol(symtab, $1);
        free($1);
    }

names_opt
    :   /* nothing */ {
        $$ = NULL; // won't need it
    } | names {
        $$ = $1;
    }

names
    :   name {
        $$ = ast_nameseq_new();
        ANameNode *namenode = ast_namenode(@1.first_line, $1);
        ast_nameseq_append($$, namenode);
    } | names name {
        $$ = $1;
        ANameNode *namenode = ast_namenode(@2.first_line, $2);
        ast_nameseq_append($$, namenode);
    }

list
    :   '{' listcontent '}' {
        $$ = $2;
    }

listcontent
    :   wordseq_opt {
        $$ = ast_protolist_new();
        ast_protolist_append($$, $1);
    } | listcontent ',' wordseq_opt {
        $$ = $1;
        ast_protolist_append($$, $3);
    } | error ',' wordseq_opt {
        $$ = ast_protolist_new();
        ast_protolist_append($$, $3);
    }

sep :   '|' | '\n'

 /* newline optional, but not | because it's silly */
nlo :   /* nothing */ | nlo '\n'

%%
