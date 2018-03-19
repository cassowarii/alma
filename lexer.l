%{
#include <stdio.h>
#include <string.h>
#include "grammar.tab.h"
#define yyterminate() return END

extern FILE * yyin;
extern void yyerror(const char *str);
int nested_blocks = 0;
int nested_lists = 0;
int nested_parens = 0;
int nested_comments = 0;
%}

%option nounput noinput yylineno bison-bridge
%x COMMENT LINECMT

 /* unicode char classes taken from http://www.kylheku.com/cgit/txr/tree/parser.l */
ASC     [\x00-\x7f]
ASCN    [\x00-\t\v-\x7f]
U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]

UANY    {ASC}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UANYN   {ASCN}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UONLY   {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

%%

  /* Reserved words. */
"import"    return T_IMPORT;
"include"   return T_INCLUDE;
"let"       return T_LET;
"with"      return T_WITH;

  /* Special tokens. */
"{"  {
    nested_blocks ++;
    return '{';
}
"}" {
    nested_blocks --;
    return '}';
}
"["  {
    nested_lists ++;
    return '[';
}
"]" {
    nested_lists --;
    return ']';
}
\( {
    nested_parens ++;
    return '(';
}
\) {
    nested_parens --;
    return ')';
}
":"     return ':';
 /* @.+ {
    yylval->s = (char*)calloc(strlen(yytext), sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-1);
    return T_REF;
} */
"|"     return '|';
\n      return '|';

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
        nested_comments = 0;
    }
}

# {
    BEGIN(LINECMT);
}

<COMMENT,LINECMT>. { /* throw it into the trash ! */ }
<LINECMT>\n|\r {
    BEGIN(INITIAL);
    return '|';
}

  /***** Various literals. *****/

  /* Character literal. */
$\\?{UANYN} {
    yylval->c = yytext[2];
    return T_CHAR;
}

  /* String literal. */
\"(\\{UANYN}|[^"\r\n]|\\\n)*\" {
    yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    return T_STRING;
}

\'(\\{UANYN}|[^"\r\n]|\\\n)*\' {
    yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    return T_STRING;
}

\"(\\{UANYN}|[^"\r\n])*(\r|\n) {
    yyerror("syntax error, unterminated double-quoted string");
}

\'(\\{UANYN}|[^"\r\n])*(\r|\n) {
    yyerror("syntax error, unterminated single-quoted string");
}

:([A-Za-z]|{UONLY})([A-Za-z0-9\-_.]|{UONLY})* {
    /*:[^{}\[\]\" \t]+ {*/
    yylval->s = (char*)calloc(strlen(yytext), sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-1);
    return T_STRING;
}

[0-9]+                  yylval->i=atoi(yytext); return T_INTEGER;

[0-9]+\.[0-9]+([Ee][0-9]+)? {
    yylval->d = atof(yytext);
    return T_FLOAT;
}

  /* Pretty much anything else is valid as something... */
[^{}\[\]\" \t;\n]+  {
    yylval->s=strdup(yytext);
    return T_WORD;
}

  /* Ignored. */
[ \t]+                  /* skip whitespace */;

  /* End of file! */
<<EOF>> { return 0; /*static int once = 0; return once++ ? 0 : '';*/ }
%%
