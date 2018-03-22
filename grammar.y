%{
#include "alma.h"
#include "value.h"
#include "ast.h"
#include "symbols.h"

#define YYERROR_VERBOSE

FILE *yyin;
extern int yychar;
extern int yylineno;

int yylex();
int yyparse();

int errors = 0;

void do_error(const char *str, int linenum) {
    fprintf(stderr, "error at line %d: %s\n", linenum, str);
    errors++;
}

void yyerror(const char *str) {
    do_error(str, yylineno);
}

int yywrap() {
    return 1;
}

ASymbolTable symtab = NULL;

%}

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
    struct AValue* val;
    struct AAstNode* ast;
    struct ADeclNode *dec;
}
%token <cs> WORD    "word"
%token <cs> SYMBOL  "symbol"
%token <i>  INTEGER "integer"
%token <i>  CHAR    "character"
%token <s>  STRING  "string"
%token <d>  FLOAT   "float"

%type <val> value;
%type <ast> block;
%type <ast> word;
%type <ast> words;
%type <ast> wordseq;
%type <ast> wordseq_opt;
%type <dec> declaration;

%%

main
    :   program {
        if (errors > 0) {
            // oh no!
        } else {
            printf("Syntax OK\n");
        }
    }

program
    :   dirlist {
    } | dirlist words_nonempty program_after_barewords {
        /* if interactive mode, handle thing */
        /* if not interactive... */
        do_error("To run code at top level non-interactively, "
                 "put it in a function called 'main'.", @2.first_line);
    }

dirlist
    :   /* empty */ {
    } | sep dirlist {
    } | realdirlist {
    }

realdirlist
    :   directive {
    } | realdirlist directive {
    } | realdirlist sep {
    }

directive
    :   declaration {
    } | import '.' {
    } | error sep {
        yyerrok ;
    }

import
    :   "import" STRING {
    } | "import" STRING "as" WORD {
    } | wrongimport {
    }

declaration
    :   "func" WORD ':' words '.' {
        ASymbol *sym = get_symbol(&symtab, $2);
        printf("Symbol '%s' at %p\n", $2, sym);
        $$ = ast_decl(@2.first_line, sym, $4);
    }

block
    :   '[' words ']' {
        $$ = $2;
    }

words
    :   wordseq_opt {
        $$ = ast_parennode(@1.first_line, $1);
    } | words sep wordseq_opt {
        $$ = $1;
        $$->next = ast_parennode(@3.first_line, $3);
    }

words_nonempty
    :   wordseq {
    } | words_nonempty sep wordseq_opt {
    }

wordseq_opt
    :   /* nothing */ {
        $$ = NULL;
    } | wordseq {
        $$ = $1;
    }

wordseq
    :   word {
        $$ = $1;
    } | wordseq word {
        $$ = $2;
        $$->next = $1;
    }

word
    :   value {
        printf("New value @ %p\n", $1);
        $$ = ast_valnode(@1.first_line, $1);
    } | WORD {
        ASymbol *sym = get_symbol(&symtab, $1);
        printf("Symbol '%s' at %p\n", $1, sym);
        $$ = ast_wordnode(@1.first_line, sym);
    } | "let" dirlist "in" nlo word {
    } | "bind" names nlo "in" nlo word {
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
        ASymbol *sym = get_symbol(&symtab, $1);
        printf("Symbol '%s' at %p\n", $1, sym);
        $$ = val_sym(sym);
    } | list {
        // I'll do this later!!!!!
    } | block {
        $$ = val_block($1);
    }

names
    :   WORD {
    } | names WORD {
    }

list
    :   '{' words '}' {
    }

sep : '|' | '\n'

 /* newline optional, but not | because it's silly */
nlo : /* nothing */ | nlo '\n'

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
