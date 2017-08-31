%{
#include <stdio.h>
#include <string.h>
#include "alma.h"
#define YYERROR_VERBOSE

FILE *yyin;
extern int yychar;
extern int yylineno;

int yylex();
int yyparse();

void yyerror(const char *str) {
    throw_error(str, yylineno);
}

int yywrap() {
    return 1;
}

int main(int argc, char **argv) {
    if (argc == 2) {
        yyin = fopen(argv[1], "r");
    } else {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }
    if (!yyin) {
        fprintf(stderr,
                "Couldn't open file %s. Are you sure it exists?\n",
                argv[argc-1]);
        return 1;
    }
    stack_top = NULL;
    init_types();
    init_library(&lib);
    yyparse();
    if (yyin) fclose(yyin);
    free_elems_below(stack_top);
    return 0;
}

%}

%define api.pure

%token LISTOPEN '['
%token LISTCLOSE ']'
%token BLOCKOPEN '{'
%token BLOCKCLOSE '}'
%token SEPARATOR
%token END 0 "end-of-file"
%union
{
    int i;
    char c;
    char *s;
    double d;
    struct node_t *n;
}
%token <s> T_WORD
%token <i> T_INTEGER
%token <c> T_CHAR
%token <s> T_STRING
%token <d> T_FLOAT
%type <n> section
%type <n> sequence_list
%type <n> sequence
%type <n> item
%type <n> block
%type <n> list

%%
program: section {
        node_t *root = $1;
        value_type *t = infer_type(root);
        if (t->tag == V_ERROR) {
            printf("Type error:\n");
            print_error(t->content.err);
            printf("Compilation aborted.\n");
            free_error(t->content.err);
        } else {
            eval(root, &stack_top);
        }
        free_type(t);
        free_node(root);
    }

section: sequence_list {
        $$ = $1;
    }

sequence_list: sequence {
        $$ = node(N_SEQUENCE, $1, NULL);
    } | sequence_list SEPARATOR {
        $$ = $1;
    } | SEPARATOR sequence_list {
        $$ = $2;
    } | sequence_list SEPARATOR sequence {
        $$ = node(N_SEQUENCE, $1, $3);
    }

sequence:
    item {
        $$ = node(N_ITEM, $1, NULL);
    } | sequence item {
        $$ = node(N_ITEM, $2, $1);
    }

item:
    block {
        $$ = node(N_BLOCK, $1, NULL);
    } | list {
        $$ = node(N_LIST, $1, NULL);
    } | T_WORD {
        $$ = node_word($1);
    } | T_STRING {
        $$ = node_str($1);
    } | T_INTEGER {
        $$ = node_int($1);
    } | T_FLOAT {
        $$ = node_float($1);
    } | T_CHAR {
        $$ = node_char($1);
    }

block:
    BLOCKOPEN section BLOCKCLOSE {
        $$ = $2;
    }

list:
    LISTOPEN sequence LISTCLOSE {
        $$ = $2;
    }
%%
