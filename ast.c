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
AAstNode *ast_parennode(unsigned int location, AAstNode *content) {
    AAstNode *newnode = ast_newnode();
    newnode->type = paren_node;
    newnode->data.inside = content;
    newnode->linenum = location;
    return newnode;
}

/* A node representing a declaration. */
ADeclNode *ast_decl(unsigned int location, ASymbol *sym, AAstNode *body) {
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
