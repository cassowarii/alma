#ifndef _ALMA_H__
#define _ALMA_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "uthash.h"

/*-*-* symbols.h *-*-*/

/* A mapping from names to unique symbols. */
typedef struct ASymbol {
    char *name;
} ASymbol;

/* A table mapping names to symbol pointers.
 * (Used during compilation -- symbols replaced by symbol
 * pointers during run b/c fast comparison/hash, a la scheme/ruby) */
typedef struct ASymbolMapping {
    char *name;
    ASymbol *sym;
    UT_hash_handle hh;
} ASymbolMapping;

/*-*-* ustrings.h *-*-*/

/* A UTF32 string. Characters stored as an array of ints. */
/* I don't think it's actually UTF32. It's utf8 grouped by codepoint
 * for ease of manipulation. */
typedef struct AUstr {
    unsigned int capacity;
    unsigned int length;
    uint32_t *data;
} AUstr;

/*-*-* value.h *-*-*/

/* Possible types of values. */
typedef enum {
    int_val,
        /* Integer, obvi. */
    float_val,
        /* Floating point value. */
    str_val,
        /* A unicode string (AUstr*) */
    sym_val,
        /* A symbol (representing a word name, or w/e.) */
    list_val,
        /* A fully instantiated list, holding only values. */
        /* (Once we have actual lists.) */
    block_val,
        /* A fully instantiated block that's bound to some closure. */
        /* (When we have closures.) */
    proto_list,
        /* An unevaluated list that lives in the AST. Contains
         * AST nodes to evaluate, rather than values. */
        /* (Future optimization: detect constant lists and skip
         * this step for them.) */
    proto_block,
        /* An un-bound block that has free variables and is waiting
         * to be pushed inside a 'bind' to take on a concrete value.
         * Uses the 'ast' pointer here. */
} AValueType;

/* Struct representing a value.
 * Immutable by running code, so we can just
 * sling pointers to these around without any
 * copying taking place. */
typedef struct AValue {
    AValueType type;
    union {
        int i;
        float fl;
        AUstr *str;
        ASymbol *sym;
        struct AWordSeqNode *ast;
        struct AProtoList *pl;
    } data;
} AValue;

/*-*-* ast.h *-*-*/

/* Possible types of AST nodes. */
typedef enum {
    value_node,
    word_node,
    paren_node,
} ANodeType;

/* Struct representing an AST node. */
typedef struct AAstNode {
    ANodeType type;         // is it a value push or a word call?
    union {
        AValue *val;        // if value
        ASymbol *sym;       // if word
        struct AWordSeqNode *inside; // if parentheses
    } data;
    struct AAstNode *next;  // these are a linked list
    unsigned int linenum;   // location of the thing, for debugging
} AAstNode;

/* Struct representing a sequence of AST nodes. */
typedef struct AWordSeqNode {
    AAstNode *first;
    AAstNode *last;
    struct AWordSeqNode *next; // for when they're in a list
} AWordSeqNode;

/* Struct representing a list yet-to-be-evaluated. */
typedef struct AProtoList {
    AWordSeqNode *first;    // first thing *evaluated* (last thing in list)
    AWordSeqNode *last;     // (not sure exec. order matters here but w/e)
} AProtoList;

/* Struct representing a declaration node. */
typedef struct ADeclNode {
    ASymbol *sym;           // symbol to bind node to
    AWordSeqNode *node;     // node to bind to name
    unsigned int linenum;   // where is?
    struct ADeclNode *next; // also a linked list
} ADeclNode;

/* Struct representing a series of declaration/imports. */
typedef struct ADeclSeqNode {
    ADeclNode *first;       // first one to execute
    ADeclNode *last;        // last one, to append to
} ADeclSeqNode;

/*-*-* stack.h *-*-*/

/* Struct representing the stack. */
typedef struct AStack {
    AValue **content;
    int size;
    int capacity;
} AStack;

#endif
