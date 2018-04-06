#include "parse.h"

/* Hi, welcome to the recursive-descent parser!!!
 * Alma grammar is pretty simple!
 *
 *      program     ::= declseq EOF
 *      declseq     ::= ø | decl ";" declseq
 *      decl        ::= "func" name nameseq ":" words
 *                    | "import" string ["as" name]
 *      names       ::= ø | names name
 *      words       ::= word-line | words separator word-line
 *      separator   ::= "|" | "\n"
 *      word-line   ::= {word}* ["->" cmplx-word]
 *      word        ::= name | cmplx-word
 *      cmplx-word  ::= literal | list | block
 *                    | "let" declseq "in" cmplx-word
 *                    | "(" words ")"
 *                    | "(" "->" names ":" words ")"
 *      block       ::= "[" words "]"
 *                    | "[" "->" names ":" words "]"
 *      list        ::= "{" list-guts "}"
 *      list-guts   ::= ø | words "," list-guts
 *      literal     ::= string | int | float | symbol
 *
 * The current parse state is passed around in the
 * AParseState* object; it holds:
 *      - the current symbol table (for name lookup)
 *      - the Flex scanner's internal state (for getting
 *          new tokens)
 *      - the current token (for getting information
 *          like function names or integer values)
 *      - the next token (for lookahead purposes)
 *      - the number of syntax errors so far
 */

#define ACCEPT(x) do_accept(x, state)
#define EXPECT(x) do_expect(x, state)
#define EXPECT_MATCH(match, tok) do_expect_match(match, tok, line, state)
#define PANIC_MATCH(match, tok) do_panic_mode(match, tok, state)
#define PANIC(tok) do_panic_mode(0, tok, state)
#define LINENUM state->currtok.loc.first_line

void do_error(char *msg, unsigned int line) {
    fprintf(stderr, "Syntax error at line %d: %s\n", line, msg);
}

/* Sentinel value returned when the thing parsed
 * cannot be a word. (end of word-sequence) */
static AAstNode nonword = { 0, { 0 }, NULL, 0 };

/* Print a representation of a general token type.
 * This is what shows up when we say "error, unexpected
 * blah, was expecting blah." The second blah is
 * this function. */
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
        case TOKENLIT:
            fprintf(out, "literal"); break;
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

/* This prints out a representation of a specific
 * token: its type, and, if it has a more specific
 * value, that value. So we get things like
 * "integer literal ‘5’" or "word ‘hello’".
 * A much nicer way to report errors than the
 * bison way.
 * ("syntax error, unexpected word". What word?!) */
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

/* Add a token-type to an ATokenTypeList, to mark
 * that we tried to accept that token but failed. */
static
void try_token(AParseState *state, ATokenType type) {
    for (ATokenTypeList *curr = state->tries; curr; curr = curr->next) {
        /* don't put duplicate tokens in the list! */
        if (curr->tok == type) {
            return;
        }
    }
    ATokenTypeList *newnode = malloc(sizeof(ATokenTypeList));
    newnode->tok = type;
    newnode->next = state->tries;
    newnode->prev = NULL;
    if (state->tries != NULL) {
        state->tries->prev = newnode;
    }
    state->tries = newnode;
}

/* After successfully accepting a token, reset the
 * 'tried' token list. */
static
void reset_tries(AParseState *state) {
    ATokenTypeList *curr = state->tries;
    while (curr != NULL) {
        ATokenTypeList *next = curr->next;
        free(curr);
        curr = next;
    }
    state->tries = NULL;
}

/* Is the token a literal (FLOAT, STRING, SYMBOL, INTEGER)
 * or something else? */
static
int is_literal_type(ATokenType type) {
    return (type == FLOAT
         || type == STRING
         || type == SYMBOL
         || type == INTEGER);
}

/* Move to the next token and return true
 * iff the lookahead matches <type>. */
static
int do_accept(ATokenType type, AParseState *state) {
    if (state->nexttok.id == type) {
        /* Move next and return true */
        reset_tries(state);
        next(state);
        return 1;
    }
    if (is_literal_type(type)) {
        try_token(state, TOKENLIT);
    } else {
        try_token(state, type);
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
    fprintf(stderr, "Syntax error at line %d: unexpected ", LINENUM);
    fprint_token(stderr, state->nexttok);
    fprintf(stderr, "; expecting ");
    /* Move to end of list and go backwards, so it prints them
     * out from most to least specific. (i.e. it prints out
     * the ones it tries first first.) */
    ATokenTypeList *last = state->tries;
    for ( ; last->next; last = last->next);
    /* Now move backwards and print them all out. */
    for (ATokenTypeList *curr = last; curr; curr = curr->prev) {
        if (curr->prev == NULL && curr->next != NULL) {
            /* last one, but not only one */
            fprintf(stderr, " or ");
        } else if (curr != last) {
            /* not first one */
            fprintf(stderr, ", ");
        }
        fprint_token_type(stderr, curr->tok);
    }
    fprintf(stderr, ".\n");

    state->errors ++;
    return 0;
}

/* Move forward and discard tokens until the
 * lookahead token matches <type>, unless there's
 * a matching <match>. (or until EOF) */
static
int do_panic_mode(ATokenType match, ATokenType type, AParseState *state) {
    int score = 0;
    while (score >= 0 && state->nexttok.id != 0) {
        next(state);
        if (state->nexttok.id == match) score ++;
        if (state->nexttok.id == type) score --;
    }
    /* If ran off end of file, return 0; else return 1 */
    return state->nexttok.id != 0;
}

static
void do_expect_match(ATokenType match, ATokenType type, unsigned int line, AParseState *state) {
    /* Now we're at the end of the parentheses/block/list.... right?? */
    if (EXPECT(type)) {
        /* great! the parentheses ended */
    } else {
        /* so was not a ), but some other unrecognized token?
         * panic! skip to the next ) without a matching ( */
        if (!PANIC_MATCH(match, type)) {
            /* if we ran off end of file... */
            fprintf(stderr, "Looks like a mismatched ");
            fprint_token_type(stderr, match);
            fprintf(stderr, " at line %d.\n", line);
        } else {
            /* otherwise, now we're looking at the matching ), so
             * just eat it. */
            EXPECT(type);
        }
    }
}

/* Check if the token could represent the
 * beginning of a declaration. */
int decl_leadin(ATokenType id) {
    return (id == T_FUNC || id == T_IMPORT);
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

/* given a node, if it's a paren_node, return the
 * sequence of words contained within; otherwise
 * return a word-sequence that only contains
 * the single node */
static
AWordSeqNode *unwrap_node(AAstNode *content) {
    AWordSeqNode *words = NULL;
    if (content != NULL) {
        if (content->type == paren_node) {
            words = content->data.inside;
            free(content);
        } else {
            words = ast_wordseq_new();
            ast_wordseq_prepend(words, content);
        }
    }
    return words;
}

ADeclSeqNode *parse_declseq(AParseState *state);
AWordSeqNode *parse_words(AParseState *state);

void eat_newlines(AParseState *state);

/* Parse the inside of a list: sequences of words, separated by commas. */
AProtoList *parse_list_guts(AParseState *state) {
    AProtoList *result = ast_protolist_new();

    do {
        AWordSeqNode *seq = parse_words(state);
        if (seq->first != NULL) {
            ast_protolist_append(result, seq);
        } else {
            /* Don't append empty things to protolist */
            free(seq);
        }
    } while (ACCEPT(','));

    return result;
}

/* Parse a sequence of space-separated names (e.g. parameters to functions.) */
/* They are allowed to have newlines between them. */
ANameSeqNode *parse_nameseq_opt(AParseState *state) {
    ANameSeqNode *result = ast_nameseq_new();

    eat_newlines(state);

    while (ACCEPT(WORD)) {
        ASymbol *sym = get_symbol(state->symtab, state->currtok.value.cs);
        free(state->currtok.value.cs);

        ANameNode *nnode = ast_namenode(LINENUM, sym);
        ast_nameseq_append(result, nnode);

        eat_newlines(state);
    }

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
    } else if (ACCEPT('(')) {
        state->nested_parens ++;
        eat_newlines(state);
        AAstNode *result = NULL;
        if (ACCEPT(T_BIND)) {
            /* ( -> a b  ...  ) */
            /* Now there's an ambiguity: is this going to be
             * (-> a b : ... ) or (-> a b ( ... ) | ... ) ? So we
             * have to do a little bit of weirdness here. */
            unsigned int bindline = LINENUM;
            ANameSeqNode *names = parse_nameseq_opt(state);
            if (complex_word_leadin(state->nexttok.id)) {
                /* (-> a b ( stuff ) | more things? ) */
                AWordSeqNode *innerbind = unwrap_node(parse_cmplx_word(state));

                /* construct the bound node */
                AAstNode *bind = ast_bindnode(bindline, names, innerbind);
                AWordSeqNode *content = ast_wordseq_new();
                ast_wordseq_append(content, bind);

                /* now see if there's any more in the outer set of parentheses */
                if (parse_separator(state)) {
                    /* there might be more stuff after the cmplx_word,
                     * in this case. */
                    AWordSeqNode *rest = parse_words(state);
                    ast_wordseq_concat(content, rest);
                    free(rest);
                }

                result = ast_parennode(line, content);
            } else {
                /* (-> a b : stuff) */
                AWordSeqNode *param_wrapper = NULL;
                if (EXPECT(':')) {
                    AWordSeqNode *words = parse_words(state);

                    param_wrapper = ast_wordseq_new();
                    ast_wordseq_prepend(param_wrapper, ast_bindnode(bindline, names, words));
                }
                result = ast_parennode(line, param_wrapper);
            }
        } else {
            /* ( stuff ) */
            AWordSeqNode *words = parse_words(state);
            result = ast_parennode(line, words);
        }

        /* EXPECT_MATCH will expect the second parameter. If it doesn't find it, it panics
         * until it finds one that doesn't have a corresponding occurrence of the first
         * parameter. Or until it runs off the end of the file, in which case it will
         * print out that there was an unmatched '(' at the initial line. */
        /* (Note also that it finds the line of the '(' by grabbing the variable 'line'
         * from this scope... it's a little macro-magic-y, but.. just something to keep
         * in mind.) */
        EXPECT_MATCH('(', ')');
        state->nested_parens --;

        return result;
    } else if (ACCEPT('[')) {
        state->nested_brackets ++;
        AWordSeqNode *inner_block = NULL;
        eat_newlines(state);
        if (ACCEPT(T_BIND)) {
            /* we have the same ambiguity here:
             * [ -> a b : stuff ] vs [ -> a b ( stuff ) | more-stuff ] */
            /* [ -> a b : stuff ] */
            unsigned int bindline = LINENUM;
            ANameSeqNode *names = parse_nameseq_opt(state);
            AWordSeqNode *words;
            if (complex_word_leadin(state->nexttok.id)) {
                /* [ -> a b ( stuff ) ?? ] */
                AWordSeqNode *innerbind = unwrap_node(parse_cmplx_word(state));

                /* construct the bound node */
                AAstNode *bind = ast_bindnode(bindline, names, innerbind);
                inner_block = ast_wordseq_new();
                ast_wordseq_append(inner_block, bind);

                /* there might be more stuff afterwards */
                if (parse_separator(state)) {
                    AWordSeqNode *rest = parse_words(state);
                    ast_wordseq_concat(inner_block, rest);
                    free(rest);
                }
            } else {
                /* [ -> a b : stuff ] */
                if (EXPECT(':')) {
                    words = parse_words(state);
                    /* now build the block */
                    inner_block = ast_wordseq_new();
                    ast_wordseq_prepend(inner_block, ast_bindnode(bindline, names, words));
                }
            }
        } else {
            /* [ stuff ] */
            inner_block = parse_words(state);
        }

        EXPECT_MATCH('[', ']');
        state->nested_brackets --;

        AValue *blockval = val_block(inner_block);
        return ast_valnode(line, blockval);
    } else if (ACCEPT('{')) {
        state->nested_curlies ++;
        AProtoList *proto = parse_list_guts(state);

        EXPECT_MATCH('{', '}');
        state->nested_curlies --;

        return ast_valnode(line, val_protolist(proto));
    } else if (ACCEPT(T_LET)) {
        /* Mark that we're inside the 'let..in' for interactive mode
         * so we can ask for more lines. */
        state->inlets ++;
        ADeclSeqNode *decls = parse_declseq(state);
        if (decls == NULL) {
            state->inlets --;
            return NULL;
        }

        EXPECT(T_IN);
        state->inlets --;

        eat_newlines(state);
        /* We only allow a cmplx_word to be the subject of a let..in,
         * because allowing single words is kind of bizarre:
         * cf. "let func x: 0 in y x" fails because only y is in the
         * scope of the let. By only permitting cmplx_words here,
         * we force let-blocks that have plain words as their scope
         * to use parentheses. */
        AAstNode *content = parse_cmplx_word(state);
        if (content != NULL) {
            return ast_letnode(line, decls, unwrap_node(content));
        } else {
            return NULL;
        }
    } else {
        return &nonword;
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
    } else {
        return parse_cmplx_word(state);
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
    while (word != &nonword) {
        if (word == NULL) return NULL;

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
            AWordSeqNode *innerbind = unwrap_node(node);
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
        if (line == NULL) return NULL;
        ast_wordseq_concat(result, line);
        free(line);
    } while (parse_separator(state));

    return result;
}

/* Parse a generalized "sequence of words," in
 * interactive mode at top level. This permits
 * separating things by pipes, but not newlines.
 * (since a newline means you're finished with
 *  the command.) */
AWordSeqNode *parse_interactive_words(AParseState *state) {
    AWordSeqNode *result = ast_wordseq_new();

    do {
        AWordSeqNode *line = parse_wordline(state);
        if (line == NULL) return NULL;
        ast_wordseq_concat(result, line);
        free(line);
    } while (ACCEPT('|'));

    return result;
}

/* Parse a declaration, which is either a function
 * or an import right now.
 * (Later: type declarations?!) */
ADeclNode *parse_decl(AParseState *state) {
    if (ACCEPT(T_FUNC)) {
        /* Mark we're inside a function now, so we can know to
         * ask for more lines in interactive mode. */
        state->infuncs ++;
        unsigned int line = LINENUM;

        /* func name */
        ASymbol *name = NULL;
        if (EXPECT(WORD)) {
            name = get_symbol(state->symtab, state->currtok.value.cs);
            free(state->currtok.value.cs);
        } else {
            state->infuncs--;
            return NULL;
        }

        /* func params */
        ANameSeqNode *params = parse_nameseq_opt(state);

        AWordSeqNode *body = NULL;
        /* func body */
        if (EXPECT(':')) {
            body = parse_words(state);
        } else {
            free_nameseq_node(params);
            state->infuncs--;
            return NULL;
        }

        if (body == NULL) {
            state->infuncs--;
            return NULL;
        }

        /* Leave function. */
        state->infuncs --;

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
        printf("ok!\n");
        return NULL;
    }
}

/* Parse an interactive-mode declaration, which must be
 * terminated by a semicolon. */
ADeclNode *parse_interactive_decl(AParseState *state) {
    if (ACCEPT(T_FUNC)) {
        /* Mark we're inside a function now, so we can know to
         * ask for more lines in interactive mode. */
        state->infuncs ++;
        unsigned int line = LINENUM;

        /* func name */
        ASymbol *name = NULL;
        if (EXPECT(WORD)) {
            name = get_symbol(state->symtab, state->currtok.value.cs);
            free(state->currtok.value.cs);
        } else {
            state->infuncs --;
            free(name);
            return NULL;
        }

        /* func params */
        ANameSeqNode *params = parse_nameseq_opt(state);

        AWordSeqNode *body = NULL;
        /* func body */
        if (EXPECT(':')) {
            body = parse_words(state);
        } else {
            state->infuncs --;
            free_nameseq_node(params);
            free(name);
            return NULL;
        }

        if (body == NULL) {
            state->infuncs --;
            free_nameseq_node(params);
            free(name);
            return NULL;
        }

        /* Leave function. */
        state->infuncs --;

        EXPECT(';');
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
        EXPECT(';');
        return NULL;
    } else {
        ACCEPT(';');
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

    eat_newlines_or_semicolons(state);
    do {
        ADeclNode *dec = parse_decl(state);
        if (dec == NULL) return NULL;
        ast_declseq_append(result, dec);
        eat_newlines_or_semicolons(state);
    } while (decl_leadin(state->nexttok.id));

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
    AParseState initial_state = {
        symtab,    /* Symbol table */
        scan,       /* Scanner */
        notoken,    /* Curr token */
        notoken,    /* Next token */
        NULL,       /* Attempted matches list */
        0,          /* # errors */
        0,          /* Interactive? */
        NULL,       /* current interactive string */
        0,          /* chars left to read from string */
        0,          /* current index into string */
        "alma> ",   /* Prompt #1 */
        "... > ",   /* Prompt #2 */
        0,          /* Nested let..in */
        0,          /* Nested function decls */
        0,          /* Nested comments */
        0,          /* Nested ( ) */
        0,          /* Nested [ ] */
        0,          /* Nested { } */
        1,          /* At beginning of line */
    };
    next(&initial_state);

    ADeclSeqNode *result = parse_program(&initial_state);

    if (initial_state.errors == 0) {
        yylex_destroy(scan);
        return result;
    } else {
        return NULL;
    }
}

/* Parse interactive; ask for more text if necessary */
void interact(ASymbolTable *symtab) {
    printf("-- alma v"ALMA_VERSION" ‘"ALMA_VNAME"’ --\n");

    AToken notoken = { TOKENNONE, { 0 }, { 0, 0 } };
    AParseState initial_state = {
        symtab,       /* Symbol table */
        NULL,       /* Scanner */
        notoken,    /* Curr token */
        notoken,    /* Next token */
        NULL,       /* Attempted matches list */
        0,          /* # errors */
        1,          /* Interactive? */
        NULL,       /* current interactive string */
        0,          /* chars left to read from string */
        0,          /* current index into string */
        "alma> ",   /* Prompt #1 */
        "... > ",   /* Prompt #2 */
        0,          /* Nested let..in */
        0,          /* Nested function decls */
        0,          /* Nested comments */
        0,          /* Nested ( ) */
        0,          /* Nested [ ] */
        0,          /* Nested { } */
        1,          /* At beginning of line */
    };

    AParseState state = initial_state;

    yyscan_t scan;
    yylex_init_extra(&state, &scan);

    AStack *stack = stack_new(20);
    AScope *lib_scope = scope_new(NULL);

    state.scan = scan;

    yyset_extra(&state, scan);
    AScope *real_scope = scope_new(lib_scope);

    AFuncRegistry *reg = registry_new(20);
    lib_init(symtab, lib_scope, 1);

    next(&state);
    do {
        state.beginning_line = 1;
        eat_newlines_or_semicolons(&state);
        state.beginning_line = 0;
        if (decl_leadin(state.nexttok.id)) {
            ADeclNode *result = parse_interactive_decl(&state);
            if (result == NULL && state.nexttok.id != 0) {
                /* If syntax error, reset */
                state = initial_state;
                state.scan = scan;
                next(&state);
                continue;
            } else if (result != NULL) {
                /* Delete previously defined functions from scope, so we can
                 * rewrite them to fix. */
                scope_delete(real_scope, result->sym);

                ADeclSeqNode *program = ast_declseq_new();
                ast_declseq_append(program, result);
                compile_in_context(program, *symtab, reg, real_scope);
                free(program);
            }
        } else {
            AWordSeqNode *result = parse_interactive_words(&state);
            if (result != NULL) {
                ACompileStatus stat = compile_seq_context(result, *symtab, reg, real_scope);
                if (stat == compile_success) {
                    eval_sequence(stack, NULL, result);
                }
            } else if (state.nexttok.id != 0) {
                /* Reset on syntax error */
                state = initial_state;
                state.scan = scan;
                next(&state);
                continue;
            }
        }
    } while (state.nexttok.id != 0);

    yylex_destroy(scan);
}
