#include "ast.h"

/* Allocate a new AST node with no information */
static
AAstNode *ast_newnode() {
    AAstNode *newnode = malloc(sizeof(AAstNode));
    newnode->next = NULL;
    // check
    return newnode;
}

/* Allocate a new declaration node with no information */
static
ADeclNode *ast_newdecl() {
    ADeclNode *newnode = malloc(sizeof(ADeclNode));
    newnode->next = NULL;
    // check
    return newnode;
}

/* Pushing a value */
AAstNode *ast_valnode(unsigned int location, AValue *val) {
    AAstNode *newnode = ast_newnode();
    newnode->type = value_node;
    newnode->data.val = val;
    newnode->linenum = location;
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
ADeclNode *ast_decl(unsigned int location, ASymbol *sym, AWordSeqNode *body) {
    ADeclNode *newnode = ast_newdecl();
    newnode->sym = sym;
    newnode->node = body;
    newnode->linenum = location;
    return newnode;
}

/* Create a new node representing a declaration sequence. */
ADeclSeqNode *ast_declseq_new() {
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

/* Create a new node representing a word/value sequence. */
AWordSeqNode *ast_wordseq_new() {
    AWordSeqNode *newnode = malloc(sizeof(AWordSeqNode));
    newnode->first = NULL;
    newnode->last = NULL;
    return newnode;
}

/* Prepend a new node to the beginning of an AWordSeqNode. */
void ast_wordseq_prepend(AWordSeqNode *seq, AAstNode *node) {
    if (node->next != NULL) {
        printf("Prepending a node with already-existing following content. "
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

/* Concatenate two AWordSeqNodes together. Doesn't free the second one! */
void ast_wordseq_concat(AWordSeqNode *seq1, AWordSeqNode *seq2) {
    if (seq1->last == NULL) {
        seq1->first = seq2->first;
        seq1->last = seq2->last;
    } else {
        seq1->last->next = seq2->first;
        seq1->last = seq2->last;
    }
}

extern void print_symbol(ASymbol *s);
extern void print_val(AValue *v);

/* Print out an AST node but first print the stuff before it.
 * Even though stuff is linked in execution order, we still want
 * to print them out 'reversed.' */
/* This is a little hard on the call stack though? Hm. */
static
void print_linked_ast(AAstNode *x) {
    if (x->next != NULL) {
        print_linked_ast(x->next);
        printf(" ");
    }
    print_ast_node(x);
}

/* Print out an AST sequence. */
void print_wordseq_node(AWordSeqNode *x) {
    if (x == NULL) return;
    print_linked_ast(x->first);
}

/* Print out an AST node. */
void print_ast_node(AAstNode *x) {
    if (x->type == value_node) {
        print_val(x->data.val);
    } else if (x->type == word_node) {
        print_symbol(x->data.sym);
    } else if (x->type == paren_node) {
        printf("(");
        print_wordseq_node(x->data.inside);
        printf(")");
    }
}

/* Print out a single declaration. */
void print_declaration(ADeclNode *a) {
    printf("func ");
    print_symbol(a->sym);
    printf(" : ");
    print_wordseq_node(a->node);
    printf(" .");
}

/* Print out a declaration sequence. */
void print_decl_seq(ADeclSeqNode *x) {
    if (x == NULL) return;
    ADeclNode *current = x->first;
    while (current != NULL) {
        print_declaration(current);
        printf("\n");
        current = current->next;
    }
}
