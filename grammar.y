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
    return 0;
}

%}

%define api.pure

%token LISTOPEN "{"
%token LISTCLOSE "}"
%token BLOCKOPEN "["
%token BLOCKCLOSE "]"
%token SEPARATOR
%token END 0 "end-of-file"
%token DEFINE "define"
%token TRUE "true"
%token FALSE "false"
%union
{
    int i;
    char c;
    char *s;
    double d;
    struct node_t *n;
}
%token <s> T_WORD "word"
%token <i> T_INTEGER "integer"
%token <c> T_CHAR "character"
%token <s> T_STRING "string"
%token <d> T_FLOAT "float"
%type <n> sequence_list
%type <n> sequence
%type <n> item
%type <n> block
%type <n> list
%type <n> definition

%%
program: sequence_list {
        node_t *root = $1;
        if (root != NULL) {
            value_type *t = infer_type(root);
            // TODO prevent stack from bottoming out by somehow stack-unifying program
            // input with a zero stack
            if (t->tag == V_ERROR) {
                if (t->content.err->line == -1) {
                    printf("Error in compilation at unknown line:\n");
                } else {
                    printf("Error in compilation at line %d:\n", t->content.err->line);
                }
                print_error(t->content.err);
                printf("Compilation aborted.\n");
                free_error(t->content.err);
            } else {
                eval(root, &stack_top);
            }
            free_type(t);
            free_elems_below(stack_top);
            free_node(root);
        }
    }

sequence_list: sequence {
        $$ = $1;
    } | sequence_list SEPARATOR {
        $$ = $1;
    } | SEPARATOR sequence_list {
        $$ = $2;
    } | sequence_list SEPARATOR sequence {
        if ($3 == NULL) {
            $$ = $1;
        } else {
            $$ = node(N_COMPOSED, $1, $3);
        }
    } | {
        $$ = NULL;
    }

sequence:
    item {
        $$ = node(N_COMPOSED, $1, NULL);
    } | sequence item {
        $$ = node(N_COMPOSED, $2, $1);
    } | definition {
        $$ = $1;
    }

item:
    block {
        $$ = node(N_BLOCK, $1, NULL);
    } | list {
        $$ = node(N_LIST, $1, NULL);
    } | T_WORD {
        $$ = node_word($1, yylineno);
    } | T_STRING {
        $$ = node_str($1, yylineno);
    } | T_INTEGER {
        $$ = node_int($1, yylineno);
    } | T_FLOAT {
        $$ = node_float($1, yylineno);
    } | T_CHAR {
        $$ = node_char($1, yylineno);
    } | TRUE {
        $$ = node_bool(1, yylineno);
    } | FALSE {
        $$ = node_bool(0, yylineno);
    }

block:
    BLOCKOPEN sequence_list BLOCKCLOSE {
        $$ = $2;
    }

list:
    LISTOPEN sequence_list LISTCLOSE {
        $$ = $2;
    }

definition:
    DEFINE T_WORD block {
        lib_entry_t *def = create_entry();
        def->name = $2;
        def->type = infer_type($3);
        if (def->type->tag != V_ERROR) {
            def->impl.node = $3;
        } else {
            add_info(def->type->content.err, "in definition of function %s", $2);
        }
        add_lib_entry(&lib, def);
        $$ = NULL;
    }

%%
