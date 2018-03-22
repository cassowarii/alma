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

/*-*-* symbols.h *-*-*/

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
    float_val,
    str_val,
    sym_val,
    list_val,
    block_val,
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
        struct AAstNode *ast;
        // we'll... do lists later
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
        struct AAstNode *inside; // if parentheses
    } data;
    struct AAstNode *next;  // these are a linked list
    unsigned int linenum;   // location of the thing, for debugging
} AAstNode;

/* Struct representing a declaration node. */
typedef struct ADeclNode {
    ASymbol *sym;
    AAstNode *node;
    unsigned int linenum;
} ADeclNode;

#endif
