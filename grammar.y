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
%token T_BIND       "bind"      /* "bind" */
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
    struct ASymbol* sym;
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

%type <sym> name;
%type <val> value;
%type <wsq> block;
%type <pls> listcontent;
%type <pls> list;
%type <nsq> names_colon;
%type <ast> cmplx_word;
%type <wsq> words;
%type <wsq> wordseq;
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
    :   "func" WORD names_colon words '.' {
        /* TODO add named parameters here */
        ASymbol *sym = get_symbol(symtab, $2);
        $$ = ast_declnode(@2.first_line, sym, $4);
        free($2);
    } | error '.' {
        $$ = NULL;
    }

block
    :   '[' words ']' {
        $$ = $2;
    }

words
    :   wordseq {
        $$ = $1;
    } | words sep wordseq {
        $$ = $1;
        ast_wordseq_concat($$, $3);
        free($3);
    }

wordseq
    :   /* nothing */ {
        $$ = ast_wordseq_new();
    } | wordseq cmplx_word {
        if ($2->type == paren_node) {
            $$ = $2->data.inside;
            ast_wordseq_concat($$, $1);
            free($2);
        } else {
            $$ = $1;
            ast_wordseq_prepend($$, $2);
        }
    } | wordseq name {
        $$ = $1;
        AAstNode *wn = ast_wordnode(@2.first_line, $2);
        ast_wordseq_prepend($$, wn);
    }

cmplx_word
    :   value {
        $$ = ast_valnode(@1.first_line, $1);
    } | "let" dirlist "in" nlo '(' wordseq ')' {
        $$ = ast_letnode(@1.first_line, $2, $6);
    } | '(' names_colon nlo wordseq ')' {
        /* NOTE: This syntax causes ambiguity: if we see a '(' and our
         * lookahead token is a name, we don't know if it's the beginning
         * of a parameter list or a word-sequence. I've gotten it to just
         * be a shift/reduce conflict, where bison's default 'shift' does
         * the right thing (I think?) but I still would like to figure out
         * how to be explicit about that. Unfortunately names_colon therefore
         * ends up being right-recursive... I really like this syntax tho :( */
        $$ = ast_bindnode(@1.first_line, $2, $4);
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

name:   WORD {
        $$ = get_symbol(symtab, $1);
        free($1);
     }

names_colon
    :   ':' {
        $$ = ast_nameseq_new();
    } | name names_colon {
        $$ = $2;
        ANameNode *namenode = ast_namenode(@2.first_line, $1);
        ast_nameseq_append($$, namenode);
    }

list
    :   '{' listcontent '}' {
        $$ = $2;
    }

listcontent
    :   wordseq {
        $$ = ast_protolist_new();
        ast_protolist_append($$, $1);
    } | listcontent ',' wordseq {
            $$ = $1;
        ast_protolist_append($$, $3);
    } | error ',' wordseq {
        $$ = ast_protolist_new();
        ast_protolist_append($$, $3);
    }

sep :   '|' | '\n'

 /* newline optional, but not | because it's silly */
nlo :   /* nothing */ | nlo '\n'

/* wrongimport
    :   "import" WORD "as" WORD {
        do_error("'import' directive expects a quoted path to the file to be imported.", @2.first_line);
    } | "import" WORD {
        do_error("'import' directive expects a quoted path to the file to be imported.", @2.first_line);
    } | "import" STRING "as" STRING {
        do_error("'import ... as' expects a bare name to prefix imported functions with.", @4.first_line);
    } | "import" WORD "as" STRING {
        do_error("'import' directive expects a quoted path to the file to be imported.", @2.first_line);
        do_error("'import ... as' expects a bare name to prefix imported functions with.", @4.first_line);
    } | "import" {
        do_error("'import' needs the path to the file to be imported.", @1.first_line);
    } */

/*program_after_barewords
    :   realdirlist {
    } | realdirlist words_nonempty program_after_barewords {
        / * Would be good to make this one somehow combine together, so we don't
         * print it out backwards... hmm. * /
        do_error("To run code at top level non-interactively, "
                 "put it in a function called 'main'.", @2.first_line);
    } | / * empty * / {
        // do nothing
    } */

%%
