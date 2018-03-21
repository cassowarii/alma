%{
#include "grammar.tab.h"
#include "alma.h"
#define yyterminate() return END

extern FILE * yyin;
extern void yyerror(const char *str);
int nested_blocks = 0;
int nested_lists = 0;
int nested_parens = 0;
int nested_comments = 0;

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;

uint32_t char_parse(const char *utf8, unsigned int length);
ustr *parse_string(const char *bytes, unsigned int length);

%}

%option nounput noinput yylineno bison-bridge bison-locations
%x COMMENT LINECMT
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
"import"                { BEGIN(IMPORT); return T_IMPORT; }
"let"                   return T_LET;
"with"                  return T_WITH;
"def"                   return T_DEF;
"in"                    return T_IN;
<IMPORT>"as"            { BEGIN(INITIAL); return T_AS; }
<IMPORT>\|              return '|';
<IMPORT>[\r\n]          return '\n';

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

";"         { BEGIN(INITIAL); return ';'; }

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
<COMMENT>\n|\r     { /* in case of block comment, also eat newlines */ }
<LINECMT>\n|\r {
    BEGIN(INITIAL);
    return '\n';
}

  /***** Various literals. *****/

  /* Character literal. */
$\\?{UANYN} {
    // TODO this gives actually the wrong value for unicode things
    // (since it'll only look at a byte)
    yylval->i = char_parse(yytext+1, strlen(yytext)-1);
    return CHAR;
}

$\"\\?{UANYN}\" {
    yylval->i = char_parse(yytext+2, strlen(yytext)-3);
    return CHAR;
}

$'\\?{UANYN}' {
    yylval->c = yytext[3];
    return CHAR;
}

  /* String literal. */
\"(\\{UANYN}|[^"\r\n]|\\\n)*\" {
    //yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    //strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    yylval->s = parse_string(yytext + 1, strlen(yytext)-2);
    return STRING;
}

\'(\\{UANYN}|[^'\r\n]|\\\n)*\' {
    //yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    //strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    yylval->s = parse_string(yytext + 1, strlen(yytext)-2);
    return STRING;
}

\"(\\{UANYN}|[^"\r\n])*$ {
    yyerror("syntax error, unterminated double-quoted string");
}

\'(\\{UANYN}|[^'\r\n])*$ {
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
([^{}\[\]\" \t\n()]|{UONLY})+  {
    yylval->cs = strdup(yytext);
    return WORD;
}

  /* Ignored. */
[ \t]+                  /* skip whitespace */;

  /* End of file! */
<COMMENT><<EOF>> { yyerror("syntax error, unterminated comment"); return 0; }
<<EOF>> { return 0; /*static int once = 0; return once++ ? 0 : '';*/ }

%%
