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

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;

%}

%option nounput noinput yylineno bison-bridge bison-locations
%x COMMENT LINECMT
%x SEPARATORS
%s IMPORT

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
"import"        { BEGIN(IMPORT); return T_IMPORT; }
"let"           return T_LET;
"with"          return T_WITH;
<IMPORT>"as"    { BEGIN(INITIAL); return T_AS; }
<IMPORT>\|      return '|';
<IMPORT>[\r\n]  return '\n';

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
"|"             return '|';
\r|\n           return '\n';

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
    return '\n';
}

  /***** Various literals. *****/

  /* Character literal. */
$\\?{UANYN} {
    yylval->c = yytext[2];
    return CHAR;
}

  /* String literal. */
\"(\\{UANYN}|[^"\r\n]|\\\n)*\" {
    yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    return STRING;
}

\'(\\{UANYN}|[^"\r\n]|\\\n)*\' {
    yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    return STRING;
}

\"(\\{UANYN}|[^"\r\n])*(\r|\n) {
    yyerror("syntax error, unterminated double-quoted string");
}

\'(\\{UANYN}|[^"\r\n])*(\r|\n) {
    yyerror("syntax error, unterminated single-quoted string");
}

 /* :([A-Za-z]|{UONLY})([A-Za-z0-9\-_.]|{UONLY})* {
    / * :[^{}\[\]\" \t]+ { * / // <-- !!
    yylval->s = (char*)calloc(strlen(yytext), sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-1);
    return T_STRING;
} */

[0-9]+                  yylval->i=atoi(yytext); return INTEGER;

[0-9]+\.[0-9]+([Ee][0-9]+)? {
    yylval->d = atof(yytext);
    return FLOAT;
}

  /* Pretty much anything else is valid as something... */
[^{}\[\]\" \t;\n]+  {
    yylval->s=strdup(yytext);
    return WORD;
}

  /* Ignored. */
[ \t]+                  /* skip whitespace */;

  /* End of file! */
<<EOF>> { return 0; /*static int once = 0; return once++ ? 0 : '';*/ }
%%
