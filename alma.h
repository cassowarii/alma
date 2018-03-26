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

/* Used for the actual table, since uthash just passes around an
 * ASymbolMapping* for access to the hash table */
typedef ASymbolMapping* ASymbolTable;

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
    int refs;         // refcounting
} AValue;

/*-*-* ast.h *-*-*/

/* Possible types of AST nodes. */
typedef enum {
    value_node, // a value to push
    word_node,  // a symbol pointer (non-looked-up function)
    func_node,  // a function that's already been looked up
    paren_node, // only used temporarily during parsing
    let_node,   // let-block
} ANodeType;

/* Struct representing an AST node. */
typedef struct AAstNode {
    ANodeType type;         // what kind is it?
    union {
        AValue *val;        // if value
        ASymbol *sym;       // free variable
        struct AWordSeqNode *inside; // if parentheses
        struct AFunc *func; // word known at compile time
        struct ALetNode *let; // if a new scope
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

/* Struct representing a "let" introducing a new scope. */
typedef struct ALetNode {
    ADeclSeqNode *decls;
    AWordSeqNode *words;
} ALetNode;

/*-*-* compile.h *-*-*/

typedef enum {
    compile_success,
    compile_fail,
} ACompileStatus;

/*-*-* stack.h *-*-*/

/* Struct representing the stack. */
typedef struct AStack {
    AValue **content;
    int size;
    int capacity;
} AStack;

/*-*-* scope.h *-*-*/

struct AScope;

/* Typedef for built-in functions. */
typedef void (*APrimitiveFunc)(AStack *, struct AScope*);

/* Builtin or declared function bound to symbol? */
typedef enum {
    primitive_func, // function written in C
    const_func,     // function with no free variables
    dummy_func,     // function found in scope but not yet compiled
} AFuncType;

/* Struct representing a callable function
 * (either built-in or defined) */
/* (we only have built-in functions right now, tho */
typedef struct AFunc {
    AFuncType type;
    union {
        APrimitiveFunc primitive;
        AWordSeqNode *func;
    } data;
} AFunc;

/* Struct representing a mapping between symbols and functions */
typedef struct AScopeEntry {
    ASymbol *sym;
    AFunc *func;
    UT_hash_handle hh;
    unsigned int linenum;   // where was it declared?
} AScopeEntry;

/* Struct representing a (possibly nested) lexical scope. */
typedef struct AScope {
    struct AScope *parent;
    AScopeEntry *content;
} AScope;

#endif
