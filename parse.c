#include "parse.h"

/* Hi, welcome to the recursive-descent parser!!!
 * Alma grammar is pretty simple!
 *
 *      program     ::= declseq EOF
 *      declseq     ::= ø | decl declseq
 *      decl*       ::= "func" names name block
 *                    | "import" string ["as" name]
 *      names       ::= ø | names name
 *      words       ::= word-line | words separator word-line
 *      separator   ::= ";" | "\n"
 *      word-line   ::= {{word}* ":"}*
 *      word        ::= name | cmplx-word
 *      cmplx-word  ::= literal | list | block
 *                    | "let" declseq "in" cmplx-word
 *                    | "(" words ")"
 *                    | "(" "->" names ":" words ")"
 *                    | "->" names cmplx-word
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
        case T_USE:
            fprintf(out, "‘let’"); break;
        case T_BIND:
            fprintf(out, "‘->’"); break;
        case T_FUNC:
            fprintf(out, "‘func’"); break;
        case T_IN:
            fprintf(out, "‘in’"); break;
        case T_END:
            fprintf(out, "‘end’"); break;
        case T_MATCH:
            fprintf(out, "‘match’"); break;
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
        case '\'':
            fprintf(out, "‘'’"); break;
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
        fprintf(out, " ‘%ld’", tok.value.i);
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

/* Advance parse state to next token, skipping over block comments. */
static
void next(AParseState *state) {
    state->currtok = state->nexttok;
    AToken tk = next_token(state->scan);
    state->nexttok = tk;

    /* If a comment is coming up next... */
    if (state->nexttok.id == CMTOPEN) {
        state->nested_comments ++;
        /* If we put a comment at the beginning of a line in interactive mode,
         * we need to change the prompt manually, because we're filtering out
         * comments before they even get to the REPL loop. */
        state->beginning_line = 0;

        int score = 0;
        while (score >= 0 && state->nexttok.id != 0) {
            state->nexttok = next_token(state->scan);
            if (state->currtok.id == WORD || state->currtok.id == SYMBOL) {
                free(state->currtok.value.cs);
            }
            if (state->nexttok.id == CMTOPEN) score ++;
            if (state->nexttok.id == CMTCLOSE) score --;
        }
        /* skip closing ) */
        state->nexttok = next_token(state->scan);
        state->nested_comments --;
    } else if (state->nexttok.id == '#') {
        /* Mark we're in a comment so that the interactive prompt will change
         * if we end our comment with a \ */
        state->beginning_line = 0;
        state->nested_comments ++;
        /* Skip ahead to the next newline. */
        while (state->nexttok.id != '\n' && state->nexttok.id != 0) {
            state->nexttok = next_token(state->scan);
            if (state->nexttok.id == WORD || state->nexttok.id == SYMBOL) {
                free(state->nexttok.value.cs);
            }
        }
        state->nested_comments --;
    }
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

/* Should the token be invisible when printing out
 * 'things that were expected'? */
static
int hide_try(ATokenType type) {
    return (type == ';' || type == ':' || type == '\n');
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
        /* If it was a literal, just say we tried "a literal." */
        try_token(state, TOKENLIT);
    } else if (!hide_try(type)) {
        /* Otherwise, mark we tried it unless we said not to above. */
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
    /* Now print out all tokens we tried to match. */
    for (ATokenTypeList *curr = state->tries; curr; curr = curr->next) {
        if (curr->next == NULL && curr->prev != NULL) {
            /* last one, but not only one */
            fprintf(stderr, ", or ");
        } else if (curr->prev != NULL) {
            /* not first one */
            fprintf(stderr, ", ");
        }
        fprint_token_type(stderr, curr->tok);
    }
    fprintf(stderr, ".\n");
    reset_tries(state);

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
        if (state->currtok.id == WORD || state->currtok.id == SYMBOL) {
            free(state->currtok.value.cs);
        }
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
            /* then we get an 'unexpected EOF' error later */
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
            || id == '\''
            || id == SYMBOL
            || id == INTEGER
            || id == FLOAT
            || id == STRING
            || id == T_BIND
            || id == T_USE) {
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

    while (ACCEPT(WORD)) {
        ASymbol *sym = get_symbol(state->symtab, state->currtok.value.cs);
        free(state->currtok.value.cs);

        ANameNode *nnode = ast_namenode(LINENUM, sym);
        ast_nameseq_append(result, nnode);
    }

    return result;
}

/* Parse either a ';' or a newline. Doesn't matter which. */
int parse_separator(AParseState *state) {
    if (ACCEPT(';')) {
        return 1;
    } else if (ACCEPT('\n')) {
        return 1;
    }
    return 0;
}

AAstNode *parse_cmplx_word(AParseState *state);

/* Parse the inside of a set of square brackets. */
/* (Probably could replace the inside-of-parens parsing
 *  with this too. Just a thought) */
AWordSeqNode *parse_block_guts(AParseState *state) {
    AWordSeqNode *inner_block = NULL;
    eat_newlines(state);

    inner_block = parse_words(state);

    return inner_block;
}

AAstNode *parse_word(AParseState *state);

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
    } else if (ACCEPT(T_BIND)) {
        unsigned int bindline = LINENUM;
        ANameSeqNode *names = parse_nameseq_opt(state);

        if (complex_word_leadin(state->nexttok.id)) {
            /* It's a bind-arrow with a set of brackets or
             * something, like -> a b (b a). */
            AAstNode *node = parse_cmplx_word(state);
            if (node != NULL) {
                AWordSeqNode *innerbind = unwrap_node(node);
                AAstNode *bind = ast_bindnode(bindline, names, innerbind);
                return bind;
            }
            return NULL;
        } else if (ACCEPT(';') || ACCEPT('\n')) {
            /* The rest of the tokens will be implicitly subsumed
             * into the scope of this bind. This is just syntactic
             * sugar, so we don't have to add a superfluous set of
             * parentheses every time we want to bind a variable
             * name. 'flat is better than nested', as they say. */
            AWordSeqNode *words = parse_words(state);
            if (words != NULL) {
                AAstNode *bind = ast_bindnode(bindline, names, words);
                return bind;
            }
            return NULL;
        } else {
            return NULL;
        }
    } else if (ACCEPT('(')) {
        state->nested_parens ++;

        AWordSeqNode *inner_block = parse_block_guts(state);

        EXPECT_MATCH('(', ')');

        state->nested_parens --;

        if (inner_block != NULL) {
            return ast_parennode(line, inner_block);
        } else {
            return NULL;
        }
    } else if (ACCEPT('[')) {
        state->nested_brackets ++;

        AWordSeqNode *inner_block = parse_block_guts(state);

        EXPECT_MATCH('[', ']');

        state->nested_brackets --;

        if (inner_block != NULL) {
            AValue *blockval = val_block(inner_block);
            return ast_valnode(line, blockval);
        } else {
            return NULL;
        }
    } else if (ACCEPT('{')) {
        state->nested_curlies ++;
        AProtoList *proto = parse_list_guts(state);

        EXPECT_MATCH('{', '}');
        state->nested_curlies --;

        return ast_valnode(line, val_protolist(proto));
    } else if (ACCEPT('\'')) {
        /* It's a reference to a single word. */
        /* For now, we'll just model this so that 'abcd is syntactic sugar
         * for '[abcd]'. */
        /* Also since we're using parse_word here we also get other sorts of
         * things for free, like '[a b c] to represent [[a b c]], or '4 for [4].
         * You could even write '(a b c) to get [a b c]. But why would you do that?
         * Okay, maybe for lisp reasons. */
        AAstNode *inner_word = parse_word(state);
        AWordSeqNode *block_contents;
        if (inner_word->type == paren_node) {
            block_contents = inner_word->data.inside;
            free(inner_word);
        } else {
            block_contents = ast_wordseq_new();
            ast_wordseq_append(block_contents, inner_word);
        }

        if (inner_word != NULL) {
            AValue *blockval = val_block(block_contents);
            return ast_valnode(line, blockval);
        } else {
            return NULL;
        }
    } else if (ACCEPT(T_USE)) {
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
        if (complex_word_leadin(state->nexttok.id)) {
            AAstNode *content = parse_cmplx_word(state);
            if (content != NULL) {
                return ast_letnode(line, decls, unwrap_node(content));
            } else {
                return NULL;
            }
        } else if (state->nexttok.id == WORD) {
            /* This means it was something like "let func c: 3 in c", which due
             * to the above rule is illegal and should instead be "let func c: 3 in (c)". */
            fprintf(stderr, "Syntax error at line %d: let-block cannot be scoped over "
                            "the single word ‘%s’.\n", LINENUM, state->nexttok.value.cs);
            fprintf(stderr, "Parentheses are required around the scope of the ‘let’; "
                            "should be ‘let ... in (%s)’.\n", state->nexttok.value.cs);
            state->errors ++;
            return NULL;
        } else {
            /* Something wacky is going on, but we'll probably catch it up above? */
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
 * from other word-lines by semicolons or newlines. i.e. the
 * part that's executed right to left. */
/* This has two parts (both optional):
 *  - a sequence of words
 *  - a binding:
 *      - an arrow, a list of names, a cmplx_word.
 * Thus we can write something like:
 *  5 6 -> a b (a b +)
 * and this counts as a 'full' wordline. */
AWordSeqNode *parse_wordline(AParseState *state) {
    AWordSeqNode *result = ast_wordseq_new();

    while (1) {
        /* Represents everything between the last colon we've
         * seen and the next colon (or end line or whatever.) */
        AWordSeqNode *section = ast_wordseq_new();

        /* If there's no words before the binding arrow,
         * we'll never enter this loop in the first place
         * -- perfect! */
        AAstNode *word = parse_word(state);

        while (word != &nonword) {
            if (word == NULL) {
                free_wordseq_node(result);
                free_wordseq_node(section);
                return NULL;
            }

            if (word->type == paren_node) {
                /* Unwrap the paren node so everything gets flattened. */
                AWordSeqNode *newresult = word->data.inside;
                ast_wordseq_concat(section, newresult);
            } else {
                ast_wordseq_append(section, word);
            }

            word = parse_word(state);
        }

        if (ACCEPT(':')) {
            /* Everything we've done so far is to be done AFTER the stuff we find next.
             * So we put these at the beginning of the overall sequence. */
            /* TODO this is inefficient, fix up later */
            ast_wordseq_concat(section, result);
            free(result);
            result = section;
        } else {
            /* We probably hit a new line. Stick what we've got at the beginning and
             * call it a day. */
            ast_wordseq_concat(section, result);
            free(result);
            result = section;
            break;
        }
    }

    return result;
}

/* Parse a generalized "sequence of words."
 * This is what makes up the elements of lists,
 * the inside of blocks, etc.
 * A series of 'word-lines' separated by newlines
 * or semicolons. */
AWordSeqNode *parse_words(AParseState *state) {
    AWordSeqNode *result = ast_wordseq_new();

    do {
        AWordSeqNode *line = parse_wordline(state);
        if (line == NULL) {
            free_wordseq_node(result);
            return NULL;
        }
        ast_wordseq_concat(result, line);
        free(line);
    } while (parse_separator(state));

    return result;
}

/* Parse a generalized "sequence of words," in
 * interactive mode at top level. This permits
 * separating things by semicolons, but not newlines.
 * (since a newline means you're finished with
 *  the command.) */
AWordSeqNode *parse_interactive_words(AParseState *state) {
    AWordSeqNode *result = ast_wordseq_new();

    do {
        AWordSeqNode *line = parse_wordline(state);
        if (line == NULL) {
            free(result);
            return NULL;
        }
        ast_wordseq_concat(result, line);
        free(line);
    } while (ACCEPT(';'));

    if (!EXPECT('\n')) {
        PANIC('\n');
        free_wordseq_node(result);
        return NULL;
    }

    return result;
}

/* Parse a declaration, which is either a function
 * or an import right now.
 * (Later: type declarations?!) */
ADeclNode *parse_decl(AParseState *state) {
    unsigned int line = LINENUM;
    if (ACCEPT(T_IMPORT)) {
        /* syntax for imports e.g.:
         *  import time         <- imports everything qualified with 'time' (from file 'time.alma')
         *  import json as J    <- imports everything qualified with 'J'
         *  import math: pi cos <- imports 'pi' and 'cos' from 'math' unqualified
         *  import math as M: pi cos <- imports 'pi' and 'cos' as 'M.pi' and 'M.cos' (sort of useless)
         *  import "my-file.alma" <- imports a raw file name (incl. suffix)
         *  import "my-file.alma": func <- imports 'func' unqualified */
        /* Note these libraries don't yet exist. Just examples! */
        int just_string = 0;
        char *module;
        ASymbol *as = NULL;
        ANameSeqNode *names = NULL;
        if (ACCEPT(WORD)) {
            /* we're importing something like the first 4 */
            module = state->currtok.value.cs;
        } else {
            /* we're importing a raw file */
            EXPECT(STRING);
            module = ustr_unparse(state->currtok.value.s);
            just_string = 1;
        }
        if (ACCEPT(T_AS)) {
            EXPECT(WORD);
            as = get_symbol(state->symtab, state->currtok.value.cs);
        }
        if (ACCEPT(':')) {
            names = parse_nameseq_opt(state);
        }
        return ast_importdeclnode(line, just_string, module, as, names);
    } else if (EXPECT(T_FUNC)) {
        /* Mark we're inside a function now, so we can know to
         * ask for more lines in interactive mode. */
        state->infuncs ++;

        /* func name */
        ASymbol *name = NULL;

        /* func params */
        ANameSeqNode *params = parse_nameseq_opt(state);

        ANameNode *funcname_node = ast_nameseq_pop(params);
        name = funcname_node->sym;
        free(funcname_node);

        AWordSeqNode *body = NULL;

        /* func body */
        EXPECT('[');

        body = parse_block_guts(state);

        EXPECT_MATCH('[', ']');

        if (body == NULL) {
            state->infuncs --;
            free_nameseq_node(params);
            return NULL;
        }

        /* Leave function. */
        state->infuncs --;

        if (params->length == 0) {
            free_nameseq_node(params);
            return ast_funcdeclnode(line, name, body);
        } else {
            /* Convert func name a b: words ; to func name: -> a b (words) ; */
            AWordSeqNode *param_wrapper = ast_wordseq_new();
            ast_wordseq_prepend(param_wrapper, ast_bindnode(params->first->linenum, params, body));
            return ast_funcdeclnode(line, name, param_wrapper);
        }
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

/* Parse a sequence of declarations separated by newlines. */
ADeclSeqNode *parse_declseq(AParseState *state) {
    ADeclSeqNode *result = ast_declseq_new();

    eat_newlines(state);
    do {
        ADeclNode *dec = parse_decl(state);
        if (dec == NULL) {
            free_decl_seq(result);
            return NULL;
        }
        ast_declseq_append(result, dec);
        eat_newlines(state);
    } while (state->nexttok.id && state->nexttok.id != T_IN);

    eat_newlines(state);

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
    AToken notoken = { 0, { 0 }, { 0, 0 } };
    AParseState initial_state = {
        symtab,     /* Symbol table */
        NULL,       /* Scanner (to be filled in later) */
        notoken,    /* Curr token */
        notoken,    /* Next token */
        NULL,       /* Attempted matches list */
        0,          /* # errors */
        0,          /* Interactive? */
        NULL,       /* current interactive string */
        0,          /* chars left to read from string */
        0,          /* current index into string */
        NULL,       /* Prompt #1 */
        NULL,       /* Prompt #2 */
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

    state.scan = scan;

    yyset_extra(&state, scan);

    yyset_in(infile, scan);

    next(&state);

    ADeclSeqNode *result = parse_program(&state);

    yylex_destroy(scan);

    if (state.errors == 0) {
        return result;
    } else {
        return NULL;
    }
}

/* Parse interactive; ask for more text if necessary */
void interact(ASymbolTable *symtab, AScope *scope, AFuncRegistry *reg) {
    printf("-- Alma v"ALMA_VERSION" ‘"ALMA_VNAME"’ --\n");

    AToken notoken = { TOKENNONE, { 0 }, { 0, 0 } };
    AParseState initial_state = {
        symtab,     /* Symbol table */
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

    state.scan = scan;

    yyset_extra(&state, scan);

    AStack *stack = stack_new(20);

    next(&state);
    do {
        state.beginning_line = 1;
        eat_newlines(&state);
        if (state.nexttok.id == 0) {
            /* Can only exit between expressions. An EOF between expressions
             * gives a syntax error (as seems to be common in REPLs) */
            break;
        }
        state.beginning_line = 0;
        if (decl_leadin(state.nexttok.id)) {
            ADeclNode *result = parse_decl(&state);
            if (result->type == import_decl) {
                result->data.imp->interactive = 1;
            }
            if (state.errors != 0 || result == NULL) {
                /* If syntax error, reset */
                state = initial_state;
                state.scan = scan;
                state.beginning_line = 1;
                free(result);
                next(&state);
            } else if (state.errors == 0) {
                /* Delete previously defined functions from scope, so we can
                 * rewrite them to fix. */
                if (result->type == func_decl) {
                    scope_delete(scope, result->data.func->sym);
                }

                ADeclSeqNode *program = ast_declseq_new();
                ast_declseq_append(program, result);
                compile_in_context(program, symtab, reg, scope);

                free(result);
                free(program);
            }
        } else if (state.nexttok.id != 0) {
            /* Otherwise, it isn't a declaration, it's just
             * a regular command. */
            AWordSeqNode *result = parse_interactive_words(&state);
            if (state.errors == 0 && result != NULL) {
                ACompileStatus stat = compile_seq_context(result, symtab, reg, scope);
                if (stat == compile_success) {
                    eval_sequence(stack, NULL, result);
                }
                free_wordseq_node(result);
            } else {
                /* Reset on syntax error */
                state = initial_state;
                state.scan = scan;
                state.beginning_line = 1;
                next(&state);
            }
        }
        if (stack->size > 0) {
            printf("  ");
            print_stack(stack);
        }
        state.beginning_line = 1;
        eat_newlines(&state);
    } while (1);

    free(state.current_string);

    reset_tries(&state);
    free_stack(stack);

    yylex_destroy(scan);
}
