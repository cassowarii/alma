%{
#include <stdio.h>
#include <string.h>
#include "alma.h"
#define YYERROR_VERBOSE

FILE *yyin;
extern int yychar;
extern int yylineno;
extern int interactive_mode;
extern int repling;
extern int newlined; // Was a newline the last thing printed?

int yylex();
int yyparse();

node_t *root;

void yyerror(const char *str) {
    throw_error(str, yylineno);
}

int yywrap() {
    return 1;
}

int main(int argc, char **argv) {
    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr,
                    "Couldn't open file %s. Are you sure it exists?\n",
                    argv[argc-1]);
            return 1;
        }
        interactive_mode = 0;
    } else {
        interactive_mode = 1;
    }
    stack_top = NULL;
    init_types();
    init_library(&lib);
    if (interactive_mode) {
        setup_interactives();
        printf("%s", motd);
    }
    do {
        if (interactive_mode) {
            if (!newlined) {
                printf("\n");
                newlined = 0;
            }
            printf("%s", primary_prompt);
        }
        yyparse();
    } while (repling);
    free_elems_below(stack_top);
    free_node(root);
    if (yyin) fclose(yyin);
    return 0;
}

%}

%define api.pure

%token LISTOPEN "{"
%token LISTCLOSE "}"
%token BLOCKOPEN "["
%token BLOCKCLOSE "]"
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
%token <s> T_MACRO "macro"
%token <i> T_INTEGER "integer"
%token <c> T_CHAR "character"
%token <s> T_STRING "string"
%token <d> T_FLOAT "float"
%token <c> SEPARATOR
%type <n> sequence_list
%type <n> sequence
%type <n> item
%type <n> block
%type <n> list
%type <n> definition

%%
program: sequence_list {
        root = $1;
        if (root != NULL) {
            value_type *t = infer_type(root);
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
                stack_type *s = NULL;
                stack_type *current;
                if (!interactive_mode) {
                    current = zero_stack();
                } else {
                    current = type_of_current_stack(stack_top);
                }
                s = unify_stack(t->content.func_type.in, current);
                if (s->tag == S_ERROR) {
                    if (s->content.err->line == -1) {
                        printf("Error in compilation at unknown line:\n");
                    } else {
                        printf("Error in compilation at line %d:\n", s->content.err->line);
                    }
                    print_error(s->content.err);
                    printf("Not enough values on stack!\n");
                    printf("Compilation aborted.\n");
                } else {
                    eval(root, &stack_top);
                }
            }
            free_type(t);
            if (repling) {
                free_node(root);
            }
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
    T_MACRO T_WORD block {
        if (!strcmp($1, "define")) {
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
            free($1);
        }
    }

%%
