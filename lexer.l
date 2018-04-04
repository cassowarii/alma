%{
#include "grammar.tab.h"
#include "alma.h"
#include "ustrings.h"
#define yyterminate() return END

extern void yyerror(const char *str);
/* we want both 'let func x: a in blah' and 'let func x: a; in blah'
   to be accepted, (to track if we're between a : and a ; for
   interactive move so we have to keep track of whether we just saw
   a semicolon in order to not decrease 'yyextra->nested_colons' by one too many. */
int just_saw_semicolon = 0;

int yywrap(yyscan_t scanner) {
    return 1;
}

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;

uint32_t char_parse(const char *utf8, unsigned int length);
AUstr *parse_string(const char *bytes, unsigned int length);

%}

%option extra-type="struct AInteractive*"
%option reentrant nounput noinput yylineno bison-bridge bison-locations
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

 /* ([^{}\[\]\" \t\n()]|{UONLY}) */
IDBEGIN [^{[(\"\' \t\n,]|{UONLY}
IDMID   [^ \t\n,]|{UONLY}
IDEND   [^}\]) \t\n,:;]|{UONLY}
IDONECH [^{}[\]()\"\' \t\n,;]|{UONLY}

%%

  /* Reserved words. */
"import"                { BEGIN(IMPORT); just_saw_semicolon = 0; return T_IMPORT; }
"let"                   return T_LET;
->|→                    return T_BIND;
"func"                  just_saw_semicolon = 0; return T_FUNC;
"in"                    if (!just_saw_semicolon) { yyextra->nested_colons --; } return T_IN;
\\[\r\n]                ; // allow escaping newlines
<IMPORT>"as"            { BEGIN(INITIAL); return T_AS; }
<IMPORT>\|              return '|';
<IMPORT>[\r\n]          return '\n';

  /* Special tokens. */
"{"  {
    yyextra->nested_blocks ++;
    return '{';
}
"}" {
    yyextra->nested_blocks --;
    if (yyextra->nested_blocks < 0) yyextra->nested_blocks = 0;
    return '}';
}
"["  {
    yyextra->nested_lists ++;
    return '[';
}
"]" {
    yyextra->nested_lists --;
    if (yyextra->nested_lists < 0) yyextra->nested_lists = 0;
    return ']';
}
\( {
    yyextra->nested_parens ++;
    return '(';
}
\) {
    yyextra->nested_parens --;
    if (yyextra->nested_parens < 0) yyextra->nested_parens = 0;
    return ')';
}
":"     yyextra->nested_colons ++; return ':';
","     return ',';

";"         { BEGIN(INITIAL); yyextra->nested_colons --; just_saw_semicolon = 1; return ';'; }

 /* @.+ {
    yylval->s = (char*)calloc(strlen(yytext), sizeof(char));
    strncpy(yylval->s, &yytext[1], strlen(yytext)-1);
    return T_REF;
} */

"|"             return '|';
\r|\n {
    if (!yyextra->is_interactive) {
        return '\n';
    }
    /* For interactive mode: if encounter an end-of-line with
       some blocks not yet closed etc., don't be done yet;
       otherwise, if we're in interactive mode, a newline with
       completely closed set of brackets is an EOF. */
    printf("newline: %d %d %d %d %d\n",
            yyextra->nested_blocks, yyextra->nested_lists,
            yyextra->nested_parens, yyextra->nested_comments,
            yyextra->nested_colons);
    if (yyextra->nested_blocks == 0 && yyextra->nested_lists == 0
            && yyextra->nested_parens == 0 && yyextra->nested_comments == 0
            && yyextra->nested_colons <= 0) {
        yyextra->beginning = 1;
        return 0;
    } else {
        yyextra->beginning = 0;
        return '\n';
    }
}

  /* Comments. */
<COMMENT,INITIAL>"(*" {
    BEGIN(COMMENT);
    yyextra->nested_comments ++;
}

<COMMENT,INITIAL>"*)" {
    yyextra->nested_comments --;
    if (yyextra->nested_comments == 0) {
        BEGIN(INITIAL);
    } else if (yyextra->nested_comments < 0) {
        yyextra->nested_comments = 0;
        return CMTCLOSE_ERRORTOKEN;
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
  /* $\\?{UANYN} {
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
} */

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

[0-9]+                  yylval->i=atoi(yytext); return INTEGER;

[0-9]+\.[0-9]+([Ee][0-9]+)? {
    yylval->d = atof(yytext);
    return FLOAT;
}

  /* Symbol literals e.g. /whatever */
\/([A-Za-z0-9]|{UONLY})([^{}\[\]\" \t\n()]|{UONLY})*  {
    yylval->cs = (char*)calloc(strlen(yytext) - 1 + 1, sizeof(char));
    strncpy(yylval->cs, yytext + 1, strlen(yytext) - 1);
    return SYMBOL;
}

  /* Pretty much anything else is valid as something... */
{IDBEGIN}({IDMID})*{IDEND}|{IDONECH} {
    // (We just treat these as bytes though, since we don't treat them like strings.)
    yylval->cs = (char*)calloc(strlen(yytext) + 1, sizeof(char));
    strncpy(yylval->cs, yytext, strlen(yytext));
    return WORD;
}

  /* Ignored. */
[ \t]+                  /* skip whitespace */;

  /* End of file! */
<COMMENT><<EOF>> { yyerror("syntax error, unterminated comment"); return 0; }
<<EOF>> { return 0; }

%%
