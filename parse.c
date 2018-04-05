#include "parse.h"

#define ACCEPT(x) do_accept(x, state)
#define EXPECT(x) do_expect(x, state)
#define LINENUM state->currtok.loc.first_line

void do_error(char *msg, unsigned int line) {
    fprintf(stderr, "error at line %d: %s\n", line, msg);
}

void fprint_token_type(FILE *out, ATokenType type) {
    switch(type) {
        case T_IMPORT:
            fprintf(out, "‘import’"); break;
        case T_AS:
            fprintf(out, "‘as’"); break;
        case T_LET:
            fprintf(out, "‘let’"); break;
        case T_BIND:
            fprintf(out, "‘->’"); break;
        case T_FUNC:
            fprintf(out, "‘func’"); break;
        case T_IN:
            fprintf(out, "‘in’"); break;
        case WORD:
            fprintf(out, "word"); break;
        case SYMBOL:
            fprintf(out, "symbol literal"); break;
        case INTEGER:
            fprintf(out, "integer literal"); break;
        case FLOAT:
            fprintf(out, "floating-point literal"); break;
        case STRING:
            fprintf(out, "string literal"); break;
        case CMTCLOSE_ERRORTOKEN:
            fprintf(out, "‘*)’"); break;
        case '[':
            fprintf(out, "‘[’"); break;
        case ']':
            fprintf(out, "‘]’"); break;
        case '{':
            fprintf(out, "‘{’"); break;
        case '}':
            fprintf(out, "‘}’"); break;
        case '(':
            fprintf(out, "‘(’"); break;
        case ')':
            fprintf(out, "‘)’"); break;
        case ':':
            fprintf(out, "‘:’"); break;
        case ';':
            fprintf(out, "‘;’"); break;
        case '|':
            fprintf(out, "‘|’"); break;
        case '\n':
            fprintf(out, "newline"); break;
        case ',':
            fprintf(out, "‘,’"); break;
        case 0:
            fprintf(out, "end-of-file"); break;
        default:
            fprintf(out, "?!?@!%d", type);
    }
}

void fprint_token(FILE *out, AToken tok) {
    fprint_token_type(out, tok.id);
    if (tok.id == WORD) {
        fprintf(out, " ‘%s’", tok.value.cs);
    } else if (tok.id == STRING) {
        fprintf(out, " \"");
        ustr_fprint(out, tok.value.s);
        fprintf(out, "\"");
    } else if (tok.id == INTEGER) {
        fprintf(out, " ‘%d’", tok.value.i);
    } else if (tok.id == FLOAT) {
        fprintf(out, " ‘%g’", tok.value.d);
    } else if (tok.id == SYMBOL) {
        fprintf(out, " ‘/%s’", tok.value.cs);
    }
}

/* Get next token from lexer. */
AToken next_token(yyscan_t s) {
    YYSTYPE st;
    YYLTYPE lt;
    int type = yylex(&st, &lt, s);
    AToken result = { type, st, lt };
    return result;
}

/* Advance parse state to next token. */
static
void next(AParseState *state) {
    state->currtok = state->nexttok;
    AToken tk = next_token(state->scan);
    state->nexttok = tk;
}

/* Move to the next token and return true
 * iff the lookahead matches <type>. */
static
int do_accept(ATokenType type, AParseState *state) {
    /* TODO 'accept' should add to the state somehow
     * to say the things it wanted to accept, so we can
     * say 'expecting blah or blah or blah' rather than
     * just 'expecting end-of-file' all the time */
    if (state->nexttok.id == type) {
        /* Move next and return true */
        next(state);
        return 1;
    }
    return 0;
}

/* Move to the next token and return true
 * if lookahead matches <type>, otherwise
 * throw error. */
static
int do_expect(ATokenType type, AParseState *state) {
    if (ACCEPT(type)) {
        return 1;
    }
    /* TODO some kind of panic mode */
    fprintf(stderr, "error at line %d: unexpected ", LINENUM);
    fprint_token(stderr, state->nexttok);
    fprintf(stderr, "; expecting ");
    fprint_token_type(stderr, type);
    fprintf(stderr, ".\n");
    next(state);

    state->errors ++;
    return 0;
}

ADeclSeqNode *parse_declseq(AParseState *state);
AWordSeqNode *parse_words(AParseState *state);

void eat_newlines(AParseState *state);

/* Parse the inside of a list: a bunch of wordses, separated by commas. */
AProtoList *parse_list_guts(AParseState *state) {
    AProtoList *result = ast_protolist_new();

    do {
        AWordSeqNode *seq = parse_words(state);
        ast_protolist_append(result, seq);
    } while (ACCEPT(','));

    return result;
}

/* Parse a sequence of space-separated names (e.g. parameters to functions. */
/* They are allowed to have newlines between them. */
ANameSeqNode *parse_nameseq_opt(AParseState *state) {
    ANameSeqNode *result = ast_nameseq_new();

    while (ACCEPT(WORD)) {
        eat_newlines(state);

        ASymbol *sym = get_symbol(state->symtab, state->currtok.value.cs);
        free(state->currtok.value.cs);

        ANameNode *nnode = ast_namenode(LINENUM, sym);
        ast_nameseq_append(result, nnode);
    }
    eat_newlines(state);

    return result;
}

/* Parse either a '|' or a newline. Doesn't matter which. */
int parse_separator(AParseState *state) {
    if (ACCEPT('|')) {
        return 1;
    } else if (ACCEPT('\n')) {
        return 1;
    }
    return 0;
}

/* Parse something that's counts as a single 'word', but
 * doesn't consist of just a name. Includes values,
 * parenthesized dealies, blocks, and let-nodes. */
AAstNode *parse_cmplx_word(AParseState *state) {
    unsigned int line = LINENUM;

    if (ACCEPT(INTEGER)) {
        return ast_valnode(line, val_int(state->currtok.value.i));
    } else if (ACCEPT(FLOAT)) {
        return ast_valnode(line, val_float(state->currtok.value.d));
    } else if (ACCEPT(STRING)) {
        return ast_valnode(line, val_str(state->currtok.value.s));
    } else if (ACCEPT(SYMBOL)) {
        ASymbol *sym = get_symbol(state->symtab, state->currtok.value.cs);
        free(state->currtok.value.cs);
        return ast_valnode(line, val_sym(sym));
    } else if (ACCEPT('[')) {
        AWordSeqNode *inner_block = NULL;
        if (ACCEPT(T_BIND)) {
            /* [ -> a b : stuff ] */
            unsigned int bindline = LINENUM;
            ANameSeqNode *names = parse_nameseq_opt(state);
            EXPECT(':');
            AWordSeqNode *words = parse_words(state);
            EXPECT(']');

            inner_block = ast_wordseq_new();
            ast_wordseq_prepend(inner_block, ast_bindnode(bindline, names, words));
        } else {
            /* [ stuff ] */
            inner_block = parse_words(state);
            EXPECT(']');
        }

        AValue *blockval = val_block(inner_block);
        return ast_valnode(line, blockval);
    } else if (ACCEPT('(')) {
        if (ACCEPT(T_BIND)) {
            /* ( -> a b : stuff ) */
            unsigned int bindline = LINENUM;
            ANameSeqNode *names = parse_nameseq_opt(state);
            EXPECT(':');
            AWordSeqNode *words = parse_words(state);

            AWordSeqNode *param_wrapper = ast_wordseq_new();
            ast_wordseq_prepend(param_wrapper, ast_bindnode(bindline, names, words));
            EXPECT(')');
            return ast_parennode(line, param_wrapper);
        } else {
            /* ( stuff ) */
            AWordSeqNode *words = parse_words(state);
            EXPECT(')');
            return ast_parennode(line, words);
        }
    } else if (ACCEPT('{')) {
        AProtoList *proto = parse_list_guts(state);
        EXPECT('}');
        return ast_valnode(line, val_protolist(proto));
    } else if (ACCEPT(T_LET)) {
        ADeclSeqNode *decls = parse_declseq(state);
        EXPECT(T_IN);
        eat_newlines(state);
        /* We only allow a cmplx_word to be the subject of a let..in,
         * because allowing single words is kind of bizarre:
         * cf. "let func x: 0 in y x" fails because only y is in the
         * scope of the let. By only permitting cmplx_words here,
         * we force let-blocks that have plain words as their scope
         * to use parentheses. */
        AWordSeqNode *words;
        AAstNode *content = parse_cmplx_word(state);
        if (content != NULL) {
            if (content->type == paren_node) {
                words = content->data.inside;
                free(content);
            } else {
                words = ast_wordseq_new();
                ast_wordseq_prepend(words, content);
            }
            return ast_letnode(line, decls, words);
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

/* Check if the token could represent the
 * beginning of a cmplx_word. */
int complex_word_leadin(ATokenType id) {
    if (id == '['
            || id == '('
            || id == '{'
            || id == SYMBOL
            || id == INTEGER
            || id == FLOAT
            || id == STRING
            || id == T_LET) {
        return 1;
    } else {
        return 0;
    }
}

/* Parse a single "word", which is either just a plain name
 * (confusingly named WORD), or some more complex arrangement
 * handled by parse_cmplx_word. */
AAstNode *parse_word(AParseState *state) {
    if (ACCEPT(WORD)) {
        ASymbol *sym = get_symbol(state->symtab, state->currtok.value.cs);
        free(state->currtok.value.cs);
        return ast_wordnode(LINENUM, sym);
    } else if (complex_word_leadin(state->nexttok.id)) {
        return parse_cmplx_word(state);
    } else {
        return NULL;
    }
}

/* Parse a word-line. This is something that's separated
 * from other word-lines by pipes or newlines. i.e. the
 * part that's executed right to left. */
/* This has two parts (both optional):
 *  - a sequence of words
 *  - a binding:
 *      - an arrow, a list of names, a cmplx_word.
 * Thus we can write something like:
 *  5 6 -> a b (+ a b)
 * and this counts as a 'full' wordline. */
AWordSeqNode *parse_wordline(AParseState *state) {
    AWordSeqNode *result = ast_wordseq_new();

    /* If there's no words before the binding arrow,
     * we'll never enter this loop in the first place
     * -- perfect! */
    AAstNode *word = parse_word(state);
    while (word != NULL) {
        if (word->type == paren_node) {
            /* Unwrap the paren node, and put the stuff
             * we've done before AFTER it. */
            AWordSeqNode *newresult = word->data.inside;
            ast_wordseq_concat(newresult, result);
            /* Essentially the inside of the paren-node
             * becomes the new main sequence. */
            free(result);
            free(word);
            result = newresult;
        } else {
            ast_wordseq_prepend(result, word);
        }
        word = parse_word(state);
    }

    /* If we found a bind arrow, then the optional
     * second part must be here. */
    if (ACCEPT(T_BIND)) {
        unsigned int bindline = LINENUM;
        ANameSeqNode *names = parse_nameseq_opt(state);

        AAstNode *node = parse_cmplx_word(state);
        if (node != NULL) {
            AWordSeqNode *innerbind;
            if (node->type == paren_node) {
                innerbind = node->data.inside;
                free(node);
            } else {
                innerbind = ast_wordseq_new();
                ast_wordseq_prepend(innerbind, node);
            }

            AAstNode *bind = ast_bindnode(bindline, names, innerbind);
            ast_wordseq_append(result, bind);
        }
    }

    return result;
}

/* Parse a generalized "sequence of words."
 * This is what makes up the elements of lists,
 * the inside of blocks, etc.
 * A series of 'word-lines' separated by newlines
 * or pipe characters ('|'). */
AWordSeqNode *parse_words(AParseState *state) {
    AWordSeqNode *result = ast_wordseq_new();

    do {
        AWordSeqNode *line = parse_wordline(state);
        ast_wordseq_concat(result, line);
        free(line);
    } while (parse_separator(state));

    return result;
}

/* Parse a declaration, which is either a function
 * or an import right now.
 * (Later: type declarations?!) */
ADeclNode *parse_decl(AParseState *state) {
    if (ACCEPT(T_FUNC)) {
        unsigned int line = LINENUM;

        /* func name */
        ASymbol *name = NULL;
        if (EXPECT(WORD)) {
            name = get_symbol(state->symtab, state->currtok.value.cs);
            free(state->currtok.value.cs);
        } else {
            /* whoops error */
        }

        /* func params */
        ANameSeqNode *params = parse_nameseq_opt(state);

        AWordSeqNode *body = NULL;
        /* func body */
        if (EXPECT(':')) {
            body = parse_words(state);
        } else {
            /* error */
        }

        if (params->length == 0) {
            free_nameseq_node(params);
            return ast_declnode(line, name, body);
        } else {
            /* Convert func name a b: words ; to func name: -> a b (words) ; */
            AWordSeqNode *param_wrapper = ast_wordseq_new();
            ast_wordseq_prepend(param_wrapper, ast_bindnode(params->first->linenum, params, body));
            return ast_declnode(line, name, param_wrapper);
        }
    } else if (ACCEPT(T_IMPORT)) {
        /* TODO parse imports */
        return NULL;
    } else {
        return NULL;
    }
}

void eat_newlines(AParseState *state) {
    /* Allow any number of newlines
     * Useful where a newline is semantically
     * irrelevant and we don't really care.
     * (e.g. after 'in', between names in a
     *  name-sequence) */
    while (ACCEPT('\n'));
}

void eat_newlines_or_semicolons(AParseState *state) {
    /* Allow any number of newlines
     * (or additional semicolons.) Useful
     * in between declarations, where we don't
     * actually care about line spacing, and
     * can have optional semicolons anywhere. */
    while (ACCEPT('\n') || ACCEPT(';'));
}

/* Parse a sequence of declarations separated by
 * semicolons. */
ADeclSeqNode *parse_declseq(AParseState *state) {
    ADeclSeqNode *result = ast_declseq_new();

    do {
        eat_newlines_or_semicolons(state);
        ADeclNode *dec = parse_decl(state);
        ast_declseq_append(result, dec);
    } while (ACCEPT(';'));

    eat_newlines_or_semicolons(state);

    return result;
}

/* Parse a full program, which is just a sequence
 * of declarations followed by an end-of-file. */
ADeclSeqNode *parse_program(AParseState *state) {
    ADeclSeqNode *result = parse_declseq(state);
    EXPECT(0); /* end of file */
    return result;
}


/* Parse a file pointer, and set program and symbol table. */
ADeclSeqNode *parse_file(FILE *infile, ASymbolTable *symtab) {
    yyscan_t scan;
    yylex_init(&scan);
    yyset_in(infile, scan);

    AToken notoken = { 0, { 0 }, { 0, 0 } };
    AParseState initial_state = { symtab, scan, notoken, notoken, 0 };
    next(&initial_state);

    ADeclSeqNode *result = parse_program(&initial_state);

    if (initial_state.errors == 0) {
        yylex_destroy(scan);

        return result;
    } else {
        return NULL;
    }
}
