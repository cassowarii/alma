%{
#include <stdio.h>
#include <string.h>
#define YYERROR_VERBOSE

FILE *yyin;
extern int yychar;
extern int yylineno;

int yylex();
int yyparse();

void do_error(const char *str, int linenum) {
    //throw_error(str, yylineno);
    printf("error at line %d: %s\n", linenum, str);
}

void yyerror(const char *str) {
    do_error(str, yylineno);
}

int yywrap() {
    return 1;
}

%}

%define api.pure

%define parse.lac full

/* track bison locations */
%locations

/* Reserved words */
%token T_IMPORT     "import"    /* "include" */
%token T_AS         "as"        /* "as" (but only after an 'import') */
%token T_LET        "let"       /* "let" */
%token T_WITH       "with"      /* "with" */

%token END 0        "end-of-file"
%union
{
    int i;
    char c;
    char *s;
    double d;
    //struct node_t *n;
}
%token <s> WORD   "word"
%token <i> INTEGER "integer"
%token <c> CHAR   "character"
%token <s> STRING "string"
%token <d> FLOAT  "float"

%%

program
    :   dirlist {
    } | dirlist words_nonempty program_after_barewords {
        /* if interactive mode, handle thing */
        /* if not interactive... */
        do_error("To run code at top level non-interactively, "
                 "put it in a function called 'main'.", @2.first_line);
    } | /* empty */ {
        // do nothing
    }

dirlist
    :   sep dirlist {
    } | realdirlist {
    }

realdirlist
    :   directive {
    } | realdirlist directive {
    } | realdirlist sep {
    }

directive
    :   declaration {
    } | import sep {
        /* Imports need a sep after them because (a) otherwise weird looking
         * and (b) otherwise you can't tell if it's `import "a"` or `import | "a"`
         * (an error'd pathless import followed by an error'd bare string.)
         * Obviously the first one is intended, but this is an easy way to enforce it. */
    } | err {
    }

import
    :   "import" STRING {
    } | "import" STRING "as" WORD {
    } | wrongimport {
    }

declaration
    :   ':' WORD so block {
    }

block
    :   '[' words ']' {
    }

 /* words_opt
    :  <nothing>  {
    } | words {
    } */

words
    :   wordseq_opt {
    } | words sep wordseq_opt {
    }

words_nonempty
    :   wordseq {
    } | words_nonempty sep wordseq_opt {
    }

wordseq_opt
    :   /* nothing */ {
    } | wordseq {
    }

wordseq
    :   word {
    } | wordseq word {
    }

word
    :   INTEGER {
    } | WORD {
    } | STRING {
    } | CHAR {
    } | FLOAT {
    } | list {
    } | block {
    } | "let" so '{' dirlist '}' so block {
    } | "with" so '{' names_opt '}' so block {
    } | '(' words ')' {
    }

names_opt
    :   so /*nothing*/ {
    } | so names {
    }

names
    :   WORD {
    } | names WORD {
    } | names sep {
    }

list
    :   '{' words '}' {
    }

sep : '|' | '\n'

 /* separator optional */
so : /* nothing */ | so sep

err
    :   ':' sep WORD block {
        do_error("Can't have a newline or '|' between colon and function being defined.", @2.first_line);
    } | ':' multword block {
        do_error("Spaces not permitted in word names.", @1.first_line);
    }

multword: WORD WORD | multword WORD

wrongimport
    /* who knew there were so many ways to violate this simple import syntax */
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
    }

program_after_barewords
    :   realdirlist {
    } | realdirlist words_nonempty program_after_barewords {
        /* Would be good to make this one somehow combine together, so we don't
         * print it out backwards... hmm. */
        do_error("To run code at top level non-interactively, "
                 "put it in a function called 'main'.", @2.first_line);
    } | /* empty */ {
        // do nothing
    }

%%
