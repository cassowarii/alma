#include "ast.h"

/* Allocate a new AST node with no information */
static
AAstNode *ast_newnode(void) {
    AAstNode *newnode = malloc(sizeof(AAstNode));
    if (newnode == NULL) {
        fprintf(stderr, "Error: can't allocate a new AST node: out of memory");
        return NULL;
    }
    newnode->next = NULL;
    return newnode;
}

/* Allocate a new declaration node with no information */
static
ADeclNode *ast_newdecl(void) {
    ADeclNode *newnode = malloc(sizeof(ADeclNode));
    if (newnode == NULL) {
        fprintf(stderr, "Error: can't allocate a new declaration node: out of memory");
        return NULL;
    }
    newnode->next = NULL;
    return newnode;
}

/* Allocate a new let node with no information */
static
ALetNode *ast_newlet(void) {
    ALetNode *newnode = malloc(sizeof(ALetNode));
    if (newnode == NULL) {
        fprintf(stderr, "Error: can't allocate a new let node: out of memory");
        return NULL;
    }
    newnode->decls = NULL;
    newnode->words = NULL;
    return newnode;
}

/* Pushing a value */
AAstNode *ast_valnode(unsigned int location, AValue *val) {
    AAstNode *newnode = ast_newnode();
    newnode->type = value_node;
    newnode->data.val = val;
    newnode->linenum = location;

    /* We want to store an extra reference to 'val' so it
     * doesn't get freed. Because it lives in the AST, it
     * will get freed at program end, as opposed to being
     * created dynamically. */
    val->refs ++;

    return newnode;
}

/* Calling a word */
AAstNode *ast_wordnode(unsigned int location, ASymbol *sym) {
    AAstNode *newnode = ast_newnode();
    newnode->type = word_node;
    newnode->data.sym = sym;
    newnode->linenum = location;
    return newnode;
}

/* A sequence of words inside parentheses - we have a pointer to the first one */
AAstNode *ast_parennode(unsigned int location, AWordSeqNode *content) {
    AAstNode *newnode = ast_newnode();
    newnode->type = paren_node;
    newnode->data.inside = content;
    newnode->linenum = location;
    return newnode;
}

/* A node representing a declaration. */
ADeclNode *ast_declnode(unsigned int location, ASymbol *sym, AWordSeqNode *body) {
    ADeclNode *newnode = ast_newdecl();
    newnode->sym = sym;
    newnode->node = body;
    newnode->linenum = location;
    return newnode;
}

/* A node representing a "let" introducing a scope. */
AAstNode *ast_letnode(unsigned int location, ADeclSeqNode *decls, AWordSeqNode *words) {
    ALetNode *newnode = ast_newlet();
    newnode->decls = decls;
    newnode->words = words;
    AAstNode *wrap = ast_newnode();
    wrap->type = let_node;
    wrap->data.let = newnode;
    wrap->linenum = location;
    return wrap;
}

/* Create a new node representing name sequence. */
ANameNode *ast_namenode(unsigned int location, ASymbol *symbol) {
    ANameNode *newnode = malloc(sizeof(ANameNode));
    newnode->sym = symbol;
    newnode->next = NULL;
    newnode->linenum = location;
    return newnode;
}

/* Create a new node representing a name binding. */
AAstNode *ast_bindnode(unsigned int location, ANameSeqNode *names, AWordSeqNode *words) {
    ABindNode *newnode = malloc(sizeof(ABindNode));
    newnode->names = names;
    newnode->words = words;
    newnode->length = names->length;
    AAstNode *wrap = ast_newnode();
    wrap->type = bind_node;
    wrap->data.bind = newnode;
    wrap->linenum = location;
    return wrap;
}

/* Create a new node representing a word/value sequence. */
AWordSeqNode *ast_wordseq_new(void) {
    AWordSeqNode *newnode = malloc(sizeof(AWordSeqNode));
    newnode->first = NULL;
    newnode->last = NULL;
    newnode->next = NULL;
    return newnode;
}

/* Create a new node representing name sequence. */
ANameSeqNode *ast_nameseq_new(void) {
    ANameSeqNode *newnode = malloc(sizeof(ANameSeqNode));
    newnode->first = NULL;
    newnode->last = NULL;
    newnode->length = 0;
    return newnode;
}

/* Create a new node representing a declaration sequence. */
ADeclSeqNode *ast_declseq_new(void) {
    ADeclSeqNode *newnode = malloc(sizeof(ADeclSeqNode));
    newnode->first = NULL;
    newnode->last = NULL;
    return newnode;
}

/* Append a new declaration to an ADeclSeqNode. */
void ast_declseq_append(ADeclSeqNode *seq, ADeclNode *node) {
    if (seq->last == NULL) {
        seq->first = seq->last = node;
    } else if (seq->last->next == NULL) {
        seq->last->next = node;
        seq->last = node;
    } else {
        /* Somehow, we're appending to the middle of the list. */
        fprintf(stderr, "Somehow appending to middle of declaration list. "
                "This probably shouldn't happen.\n");
    }
}

/* Prepend a new node to the beginning of an AWordSeqNode. */
void ast_wordseq_prepend(AWordSeqNode *seq, AAstNode *node) {
    if (node == NULL) return;
    if (node->next != NULL) {
        fprintf(stderr, "Prepending a node with already-existing following content. "
                "This probably shouldn't happen!\n");
        return;
    }
    if (seq->first == NULL) {
        seq->first = seq->last = node;
    } else {
        node->next = seq->first;
        seq->first = node;
    }
}

/* Append a new node to the end of an AWordSeqNode. */
void ast_wordseq_append(AWordSeqNode *seq, AAstNode *node) {
    if (node == NULL) return;
    if (seq->last == NULL) {
        seq->first = seq->last = node;
    } else if (seq->last->next == NULL) {
        seq->last->next = node;
        seq->last = node;
    } else {
        /* Somehow, we're appending to the middle of the list. */
        fprintf(stderr, "Somehow appending to middle of wordseq. "
                "This probably shouldn't happen.\n");
    }
}

/* Concatenate two AWordSeqNodes together. Doesn't free the second one! */
void ast_wordseq_concat(AWordSeqNode *seq1, AWordSeqNode *seq2) {
    if (seq1->last == NULL) {
        seq1->first = seq2->first;
        seq1->last = seq2->last;
    } else if (seq2->first != NULL) {
        seq1->last->next = seq2->first;
        seq1->last = seq2->last;
    }
}

/* Prepend a new node to the beginning of an ANameSeqNode. */
void ast_nameseq_append(ANameSeqNode *seq, ANameNode *node) {
    if (node == NULL) return;
    if (seq->last == NULL) {
        seq->first = seq->last = node;
        seq->length ++;
    } else if (seq->last->next == NULL) {
        seq->last->next = node;
        seq->last = node;
        seq->length ++;
    } else {
        /* Somehow, we're appending to the middle of the name-list. */
        fprintf(stderr, "Somehow appending to middle of name list. "
                "This probably shouldn't happen.\n");
    }
}

/* Allocate a new AProtoList. */
AProtoList *ast_protolist_new(void) {
    AProtoList *newlist = malloc(sizeof(AProtoList));
    newlist->first = NULL;
    newlist->last = NULL;
    return newlist;
}

/* Append a new word-sequence to an AProtoList. */
void ast_protolist_append(AProtoList *list, AWordSeqNode *node) {
    if (node == NULL) return;
    if (list->last == NULL) {
        list->first = list->last = node;
    } else if (list->last->next == NULL) {
        list->last->next = node;
        list->last = node;
    } else {
        /* Somehow, we're appending to the middle of the list. */
        fprintf(stderr, "Somehow appending to middle of a list. "
                        "This probably shouldn't happen.\n");
    }
}

extern void fprint_symbol(FILE *out, ASymbol *s);
extern void fprint_val(FILE *out, AValue *v);

/* Print out an AST node. */
void print_ast_node(AAstNode *x) {
    fprint_ast_node(stdout, x);
}

/* Print out a protolist. */
void print_protolist(AProtoList *pl) {
    fprint_protolist(stdout, pl);
}

/* Print out an AST sequence. */
void print_wordseq_node(AWordSeqNode *x) {
    fprint_wordseq_node(stdout, x);
}

/* Print out a single declaration. */
void print_declaration(ADeclNode *a) {
    fprint_declaration(stdout, a);
}

/* Print out a declaration sequence. */
void print_decl_seq(ADeclSeqNode *x) {
    fprint_decl_seq(stdout, x);
}

/* Print out a sequence of AST nodes. */
static
void fprint_linked_ast(FILE *out, AAstNode *x) {
    while (x != NULL) {
        fprint_ast_node(out, x);
        fprintf(out, " ");
        x = x->next;
    }
}

/* Print out an AST sequence. */
void fprint_wordseq_node(FILE *out, AWordSeqNode *x) {
    if (x == NULL) return;
    if (x->first == NULL) return;
    fprint_linked_ast(out, x->first);
}

/* Print out a protolist. */
void fprint_protolist(FILE *out, AProtoList *pl) {
    if (pl == NULL) return;
    AWordSeqNode *current = pl->first;
    while (current != NULL) {
        fprint_wordseq_node(out, current);
        if (current->next != NULL) fprintf(out, ", ");
        current = current->next;
    }
}

static
void fprint_name_seq(FILE *out, ANameSeqNode *x) {
    ANameNode *current = x->first;
    while (current) {
        fprintf(out, "%s ", current->sym->name);
        current = current->next;
    }
}

/* Print out an AST node. */
void fprint_ast_node(FILE *out, AAstNode *x) {
    if (x->type == value_node) {
        fprint_val(out, x->data.val);
    } else if (x->type == word_node) {
        fprint_symbol(out, x->data.sym);
    } else if (x->type == paren_node) {
        fprintf(out, "(");
        fprint_wordseq_node(out, x->data.inside);
        fprintf(out, ")");
    } else if (x->type == func_node) {
        fprintf(out, "%s", x->data.func->sym->name);
    } else if (x->type == let_node) {
        fprintf(out, "let ");
        fprint_decl_seq(out, x->data.let->decls);
        fprintf(out, " in (");
        fprint_wordseq_node(out, x->data.let->words);
        fprintf(out, ")");
    } else if (x->type == bind_node) {
        fprintf(out, "(→ ");
        fprint_name_seq(out, x->data.bind->names);
        fprintf(out, ": ");
        fprint_wordseq_node(out, x->data.bind->words);
        fprintf(out, ")");
    } else {
        fprintf(out, "??%d", x->type);
    }
}

/* Print out a single declaration. */
void fprint_declaration(FILE *out, ADeclNode *a) {
    fprintf(out, "func ");
    fprint_symbol(out, a->sym);
    fprintf(out, " : ");
    fprint_wordseq_node(out, a->node);
    fprintf(out, " ;");
}

/* Print out a declaration sequence. */
void fprint_decl_seq(FILE *out, ADeclSeqNode *x) {
    if (x == NULL) return;
    ADeclNode *current = x->first;
    while (current != NULL) {
        fprint_declaration(out, current);
        fprintf(out, "\n");
        current = current->next;
    }
}

//extern void free_symbol(ASymbol*);
extern void delete_ref(AValue*);

void free_wordseq_node(AWordSeqNode *to_free);
void free_let(ALetNode *to_free);

/* Free an AST node. */
void free_ast_node(AAstNode *to_free) {
    if (to_free->type == value_node) {
        delete_ref(to_free->data.val);
    } else if (to_free->type == word_node) {
        /* do nothing, symbols freed at end! */
    } else if (to_free->type == func_node) {
        /* again do nothing! */
    } else if (to_free->type == paren_node) {
        free_wordseq_node(to_free->data.inside);
    } else if (to_free->type == let_node) {
        free_let(to_free->data.let);
    } else if (to_free->type == bind_node) {
        free_nameseq_node(to_free->data.bind->names);
        free_wordseq_node(to_free->data.bind->words);
        free(to_free->data.bind);
    } else if (to_free->type == var_bind) {
        free_wordseq_node(to_free->data.vbind->words);
        free(to_free->data.vbind);
    } else {
        fprintf(stderr, "internal error: don't know how to free "
                "ast node of type %d\n", to_free->type);
    }
    free(to_free);
}

/* Free a protolist. */
void free_protolist(AProtoList *to_free) {
    AWordSeqNode *current = to_free->first;
    while (current != NULL) {
        AWordSeqNode *next = current->next;
        free_wordseq_node(current);
        current = next;
    }
    free(to_free);
}

/* Free a word-sequence node. */
void free_wordseq_node(AWordSeqNode *to_free) {
    if (to_free == NULL) return;
    AAstNode *current = to_free->first;
    while (current != NULL) {
        AAstNode *next = current->next;
        free_ast_node(current);
        current = next;
    }
    free(to_free);
}

/* Free a name-sequence node. */
void free_nameseq_node(ANameSeqNode *to_free) {
    ANameNode *current = to_free->first;
    while (current != NULL) {
        ANameNode *next = current->next;
        free(current);
        current = next;
    }
    free(to_free);
}

/* Free a declaration node COMPLETELY. (Careful!) */
void free_decl_node(ADeclNode *to_free) {
    free_wordseq_node(to_free->node);
    free(to_free);
}

/* Free a declaration sequence node COMPLETELY. (Careful!) */
void free_decl_seq(ADeclSeqNode *to_free) {
    if (to_free == NULL) return;

    ADeclNode *current = to_free->first;
    while (current != NULL) {
        ADeclNode *next = current->next;
        free_decl_node(current);
        current = next;
    }
    free(to_free);
}

/* Free a let node. */
void free_let(ALetNode *to_free) {
    free_decl_seq(to_free->decls);
    free_wordseq_node(to_free->words);
    free(to_free);
}
