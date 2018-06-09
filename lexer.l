%{
#include <readline/readline.h>
#include <readline/history.h>
#include "alma.h"
#include "parse.h"
#include "ustrings.h"
#define yyterminate() return END

#define ERROR(msg) do_error(msg, yylineno)

extern int yyget_lineno (yyscan_t yyscanner );

extern void yyset_lineno (int _line_number ,yyscan_t yyscanner );


int yywrap(yyscan_t scanner) {
    return 1;
}

size_t get_input(FILE *file, char *buf, size_t max, AParseState *state) {
    if (file != stdin) {
        /* There's an actual file to read */
        size_t len = fread(buf, 1, max, file);
        if (len == 0 && ferror(file)) {
            /* ERROR ! :( */
            return YY_NULL; /* TODO handle this */
        }
        return len;
    } else {
        /* Interactive! We have to read from stdin. */
        if (state->chars_left == 0) {
            if (state->beginning_line) {
                state->current_string = readline(state->prompt1);
            } else {
                state->current_string = readline(state->prompt2);
            }

            if (state->current_string == NULL) {
                return YY_NULL;
            }

            state->chars_left = strlen(state->current_string);
            state->str_index = 0;

            /* If the line is not empty, store it in the history
               so we can access it with up-arrow. */
            if (state->chars_left > 0) {
                add_history(state->current_string);
            }
        }

        /* Copy the string into the buffer for lex. */
        int chars_read = 0;
        for (int i = 0; i < max-2 && state->chars_left > 0; i++) {
            buf[i] = state->current_string[state->str_index + i];
            state->chars_left --;
            chars_read ++;
        }

        /* Put a newline at the end, since editline 'helpfully'
         * strips it off for us. */
        buf[chars_read] = '\n';
        chars_read ++;
        if (buf[chars_read-2] != '\\') {
            /* (Actually put two newlines, so we can consume the first one
               without having to read in another line to grab a lookahead
               token.) */
            /* (Except if there's a backslash, since the second newline wouldn't
               get escaped. But if there's a backslash we have to ask for more
               input anyway, so it's fine. */
            buf[chars_read] = '\n';
            chars_read ++;
        }
        /* Decrease lineno by one, to account for the extra newline
           we added. */
        yyset_lineno(yyget_lineno(state->scan) - 1, state->scan);
        state->str_index += chars_read;

        if (state->chars_left == 0) {
            free(state->current_string);
        }

        return chars_read;
    }
}

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;
#define YY_INPUT(buf, result, max_size) result = get_input(yyin, buf, max_size, yyextra)

uint32_t char_parse(const char *utf8, unsigned int length);
AUstr *parse_string(const char *bytes, unsigned int length);

%}

%option header-file="lex.h"
%option extra-type="AParseState*"
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
"import"                BEGIN(IMPORT); return T_IMPORT;
"let"                   return T_LET;
->|→                    return T_BIND;
"fn"                    return T_FUNC;
"in"                    return T_IN;
\\[\r\n]                ; // allow escaping newlines
<IMPORT>"as"            { BEGIN(INITIAL); return T_AS; }
<IMPORT>\|              return '|';
<IMPORT>[\r\n]          return '\n';

  /* Special characters. */
\{  |
\}  |
\[  |
\]  |
\(  |
\)  |
:   |
;   |
,   |
#   return yytext[0];

\r|\n {
    return '\n';
}

  /* Comments. */
 /*<COMMENT,INITIAL>"#(" {
    BEGIN(COMMENT);
    yyextra->nested_comments ++;
}

<COMMENT,INITIAL>")" {
    yyextra->nested_comments --;
    if (yyextra->nested_comments == 0) {
        BEGIN(INITIAL);
    } else if (yyextra->nested_comments < 0) {
        yyextra->nested_comments = 0;
        return CMTCLOSE_ERRORTOKEN;
    }
}*/

"#{" {
    return CMTOPEN;
}

 /*<COMMENT>. { / * throw it into the trash ! * / }
<COMMENT>\n|\r     { / * in case of block comment, also eat newlines * / }
<LINECMT>\n|\r {
    BEGIN(INITIAL);
    return '\n';
} */

  /***** Various literals. *****/

  /* String literal. */
\"(\\{UANYN}|[^"\r\n]|\\\n)*\" {
    //yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    //strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    if (yyextra->nested_comments == 0) {
        yylval->s = parse_string(yytext + 1, strlen(yytext)-2);
        return STRING;
    } else {
        return TOKENBLAH;
    }
}

\'(\\{UANYN}|[^'\r\n]|\\\n)*\' {
    //yylval->s = (char*)calloc(strlen(yytext)-1, sizeof(char));
    //strncpy(yylval->s, &yytext[1], strlen(yytext)-2);
    if (yyextra->nested_comments == 0) {
        yylval->s = parse_string(yytext + 1, strlen(yytext)-2);
        return STRING;
    } else {
        return TOKENBLAH;
    }
}

\"(\\{UANYN}|[^"\r\n])*$ {
    if (yyextra->nested_comments == 0) {
        ERROR("syntax error, unterminated double-quoted string");
    } else {
        return TOKENBLAH;
    }
}

\'(\\{UANYN}|[^'\r\n])*$ {
    if (yyextra->nested_comments == 0) {
        ERROR("syntax error, unterminated single-quoted string");
    } else {
        return TOKENBLAH;
    }
}

[0-9]+                  yylval->i=atoi(yytext); return INTEGER;

[0-9]+\.[0-9]+([Ee][0-9]+)? {
    yylval->d = atof(yytext);
    return FLOAT;
}

  /* Symbol literals e.g. /whatever */
\/([A-Za-z0-9]|{UONLY})([^{}\[\]\" \t\n()]|{UONLY})*  {
    if (yyextra->nested_comments == 0) {
        yylval->cs = (char*)calloc(strlen(yytext) - 1 + 1, sizeof(char));
        strncpy(yylval->cs, yytext + 1, strlen(yytext) - 1);
        return SYMBOL;
    } else {
        return TOKENBLAH;
    }
}

  /* Pretty much anything else is valid as something... */
{IDBEGIN}({IDMID})*{IDEND}|{IDONECH} {
    // (We just treat these as bytes though, since we don't treat them like strings.)
    if (yyextra->nested_comments == 0) {
        yylval->cs = (char*)calloc(strlen(yytext) + 1, sizeof(char));
        strncpy(yylval->cs, yytext, strlen(yytext));
        return WORD;
    } else {
        return TOKENBLAH;
    }
}

  /* Ignored. */
[ \t]+                  /* skip whitespace */;

  /* End of file! */
<COMMENT><<EOF>> { ERROR("syntax error, unterminated comment"); return 0; }
<<EOF>> { return 0; }

%%
