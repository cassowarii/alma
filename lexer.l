%{
#include <stdio.h>
#include <string.h>
#include "alma.h"
#include "y.tab.h"
#define yyterminate() return END

extern FILE * yyin;
int nested_blocks = 0;
int nested_lists = 0;
int nested_comments = 0;
%}

%option nounput noinput yylineno bison-bridge
%x COMMENT
%%
  /* Special tokens. */
"["  {
    nested_blocks ++;
    return BLOCKOPEN;
}
"]" {
    nested_blocks --;
    if (nested_blocks < 0) {
        // uh oh, this'll be a syntax error later
        // we won't report it here though, yacc'll catch it
        nested_blocks = 0;
    }
    return BLOCKCLOSE;
}
"{"  {
    nested_lists ++;
    return LISTOPEN;
}
"}"  {
    nested_lists --;
    if (nested_lists < 0) {
        // see above, uh oh
        nested_lists = 0;
    }
    return LISTCLOSE;
}
"|" { return SEPARATOR; }
\n  {
    if (yyin == stdin) {
        if (nested_lists == 0 && nested_blocks == 0) {
            newlined = 1;
            return 0;
        } else {
            if (tty) {
                printf("%s", secondary_prompt);
            }
            return SEPARATOR;
        }
    } else {
        return SEPARATOR;
    }
}
  /* Comments. */
<COMMENT,INITIAL>"(*" {
    BEGIN(COMMENT);
    nested_comments ++;
}
<COMMENT,INITIAL>"*)" {
    nested_comments --;
    if (nested_comments == 0) {
        BEGIN(INITIAL);
    } else if (nested_comments < 0) {
        fprintf(stderr, "found `*)` with no matching `(*`!\n");
    }
}
<COMMENT>. { /* throw it into the trash ! */ }
<COMMENT>\n { /* throw it into the trash ! */ }

  /* The few keywords. */
":" { return T_MACROCOLON; }
"," { return T_MACROCOMMA; }
true { return TRUE; }
false { return FALSE; }

  /* Various literals. */
#\"\\?.\" {
    yylval->c = yytext[2];
    return T_CHAR;
}
\"(\\.|[^"])*\" {
    yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    return T_STRING;
}
:[A-Za-z0-9][A-Za-z0-9\-_]* {
    /*:[^{}\[\]\" \t]+ {*/
    yylval->s = (char*)calloc(strlen(yytext), sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-1);
    return T_STRING;
}
-?[0-9]+                  yylval->i=atoi(yytext); return T_INTEGER;
[0-9]+\.[0-9]+([Ee][0-9]+)? {
    yylval->d = atof(yytext);
    return T_FLOAT;
}
  /* Pretty much anything else is valid as something... */
[^{}\[\]\" \t;\n,]+  {
    yylval->s=strdup(yytext);
    return T_WORD;
}
  /* Ignored. */
[ \t]+                  /* skip whitespace */;
  /* End of file! */
<<EOF>> { repling = 0; newlined = 0; return 0; /*static int once = 0; return once++ ? 0 : '';*/ }
%%
