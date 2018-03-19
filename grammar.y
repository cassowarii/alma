%{
#include <stdio.h>
#include <string.h>
#define YYERROR_VERBOSE

FILE *yyin;
extern int yychar;
extern int yylineno;

int yylex();
int yyparse();

void yyerror(const char *str) {
    //throw_error(str, yylineno);
    printf("error at line %d: %s\n", yylineno, str);
}

int yywrap() {
    return 1;
}

%}

%define api.pure

/* Reserved words */
%token T_IMPORT     "import"    /* "import" */
%token T_INCLUDE    "include"   /* "include" */
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
%token <s> T_WORD   "word"
%token <i> T_INTEGER "integer"
%token <c> T_CHAR   "character"
%token <s> T_STRING "string"
%token <d> T_FLOAT  "float"

%%

program
    :   dirlist_opt {
    }

dirlist_opt
    :   /* nothing */ {
    } | dirlist {
    }

dirlist
    :   directive {
    } | dirlist directive {
    }

directive
    :   declaration '|' {
    } | import '|' {
    } | '|' /* blank lines OK */ {
    }

import
    :   T_INCLUDE T_STRING {
    } | T_IMPORT T_WORD T_STRING {
    }

declaration
    :   ':' T_WORD block {
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
    } | words '|' wordseq_opt {
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
    :   T_INTEGER {
    } | T_WORD {
    } | T_STRING {
    } | T_CHAR {
    } | list {
    } | block {
    } | T_LET '{' dirlist '}' block {
    } | T_WITH '{' names_opt '}' block {
    } | '(' words ')' {
    }

names_opt
    :   /* nothing */ {
    } | names {
    }

names
    :   T_WORD {
    } | names T_WORD {
    }

list
    :   '{' words '}' {
    }

%%
