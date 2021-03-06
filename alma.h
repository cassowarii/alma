#ifndef _ALMA_H__
#define _ALMA_H__

#define ALMA_VERSION "0.2"
#define ALMA_VNAME "Foreign Affairs"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "uthash.h"

const char *ALMA_PATH;

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
    unsigned int byte_length;
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
    proto_block,
        /* A block that hasn't yet been compiled. */
    block_val,
        /* A 'constant' block value: has no free variables,
         * because it's 'constant' like [+ 1] and has no
         * variables at all. */
    free_block_val,
        /* A block value that has some free variables. (Only lives in
         * the AST, not pushed to the stack.) */
    bound_block_val,
        /* A block value that was created from a free_block_val,
         * and needs to have its closure loaded when run.
         * Uses the data.uf pointer. */
    proto_list,
        /* An unevaluated list that lives in the AST. Contains
         * AST nodes to evaluate, rather than values. */
        /* (Future optimization: detect constant lists and skip
         * this step for them.) */
    list_val,
        /* A real, honest-to-god list. */
} AValueType;

/* Struct representing a value.
 * Immutable by running code, so we can just
 * sling pointers to these around without any
 * copying taking place. */
typedef struct AValue {
    AValueType type;
    union {
        long i;
        double fl;
        AUstr *str;
        ASymbol *sym;
        struct AWordSeqNode *ast;
        struct AUserFunc *uf;
        struct AProtoList *pl;
        struct AList *list;
    } data;
    int refs;         // refcounting
} AValue;

/*-*-* list.h *-*-*/

/* A struct holding an element in a list.
 * (An element of a double-linked list -- points
 * to the value and the next and previous elements) */
typedef struct AListElem {
    AValue *val;
    struct AListElem *next;
    struct AListElem *prev;
} AListElem;

/* A linked list!
 * Maintains a pointer to the last element
 * for convenience. Dunno if that will actually
 * come in handy. */
typedef struct AList {
    AListElem *first;
    AListElem *last;
    unsigned int length;
} AList;

/*-*-* vars.h *-*-*/

/* A struct representing the vars bound to names at a given point
 * in the program at runtime. It has a size, and a parent scope
 * containing variables declared in any outer lexical scopes. */
typedef struct AVarBuffer {
    AValue **vars;              // values of vars
    unsigned int size;          // number of vars in this one
    unsigned int base;          // number of vars below this one (for looking up in scopes below)
    struct AVarBuffer *parent;  // where to find more vars
    unsigned int refs;          // refcount to know whether closures point to it
} AVarBuffer;

/* An instruction telling the interpreter to place the top <count>
 * elements from the stack into a var-buffer while executing the
 * word-sequence <words>. compile() replaces* BindNodes with this. */
typedef struct AVarBind {
    int count;
    struct AWordSeqNode *words;
} AVarBind;

/*-*-* ast.h *-*-*/

/* Possible types of AST nodes. */
typedef enum {
    value_node, // a value to push
    word_node,  // a symbol pointer (non-looked-up function)
    func_node,  // a function that's already been looked up
    paren_node, // only used temporarily during parsing
    let_node,   // let-block
    bind_node,  // bind-block
    var_bind,   // bind-instruction
} ANodeType;

/* Struct representing an AST node. */
typedef struct AAstNode {
    ANodeType type;             // what kind is it?
    union {
        AValue *val;            // if value
        ASymbol *sym;           // free variable
        struct AWordSeqNode *inside; // if parentheses
        struct AFunc *func;     // word known at compile time
        struct ALetNode *let;   // if a new scope
        struct ABindNode *bind; // if a new lexical binding (uncompiled)
        struct AVarBind *vbind; // if a compiled binding
    } data;
    struct AAstNode *next;      // these are a linked list
    unsigned int linenum;       // location of the thing, for debugging
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

/* Possible types of declarations. */
typedef enum {
    func_decl,              // function declaration
    import_decl,            // import
    type_decl,              // future ??!
} ADeclType;

/* Struct representing a function declaration. */
typedef struct AFuncDecl {
    ASymbol *sym;           // symbol to bind node to
    AWordSeqNode *node;     // node to bind to name
} AFuncDecl;

/* Struct representing an 'import' statement. */
typedef struct AImportStmt {
    const char *module;     // name of module to import
    int just_string;        // boolean: is it imported like 'import A' or like 'import "A.al"'?
    ASymbol *as;            // qualified-import name (may be null)
    int interactive;        // was it created interactively? (print extra stuff)
    struct ANameSeqNode *names; // names to import from module (may be null)
} AImportStmt;

/* Struct representing a declaration node. */
typedef struct ADeclNode {
    ADeclType type;         // what kind of declaration?
    union {
        AFuncDecl *func;    // function declaration
        AImportStmt *imp;   // import
    } data;
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
    ADeclSeqNode *decls;    // things to declare
    AWordSeqNode *words;    // words to execute in scope
} ALetNode;

/* Struct representing a "bind" binding variables from top of stack. */
typedef struct ANameNode {
    ASymbol *sym;
    struct ANameNode *next;
    unsigned int linenum;
} ANameNode;

/* Struct representing a series of names that can be bound with a 'bind'. */
typedef struct ANameSeqNode {
    ANameNode *first;
    ANameNode *penult;
    ANameNode *last;
    unsigned int length;
} ANameSeqNode;

/* Struct representing a "bind" binding variables from top of stack. */
typedef struct ABindNode {
    ANameSeqNode *names;    // names to bind
    AWordSeqNode *words;    // words to execute
    unsigned int length;    // how many words get bound here?
} ABindNode;

/*-*-* compile.h *-*-*/

/* Possible compile statuses */
typedef enum {
    compile_success,        // Compilation succeeded. Great.
    compile_fail,           // Oh no, compilation failed :(
} ACompileStatus;

/* Sentinel value for a function with no free variables. */
unsigned int NOFREEVARS; /* we want it to compare > than everything */

/* Result of compilation: whether compilation succeeded,
 * and the index of the lowest free variable (NOFREEVARS if no
 * free variables) */
typedef struct ACompileResult {
    ACompileStatus status;
    unsigned int lowest_free;
} ACompileResult;

/* Information about current binding status in compilation */
typedef struct ABindInfo {
    unsigned int var_depth; // How many named variables are below us?
    unsigned int last_block_depth; // What was the depth of the last block?
        /* We keep track of the last block depth so that we know
         * which variables are created inside the block, and which
         * ones need to be closed over from the outside. */
} ABindInfo;

/* Stuff allocated by compile_file (in alma.c); returned so
 * we can keep track of the various scopes and whatnot that
 * it allocates */
typedef struct ACompileAllocation {
    ACompileStatus stat;
    struct AFuncRegistry *reg;
    struct AScope *scope;
} ACompileAllocation;

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
typedef void (*APrimitiveFunc)(AStack *, struct AVarBuffer*);

/* Tag for user functions: have we compiled them yet? */
typedef enum {
    dummy_func,     // function found in scope but not yet compiled
    const_func,     // function with no free variables
    bound_func,     // block with bound variables
} AUserFuncType;

/* Struct representing a function defined by the user. */
typedef struct AUserFunc {
    AUserFuncType type;
    AWordSeqNode *words;
    AVarBuffer *closure;
    unsigned int free_var_index;    // Index of its lowest free var.
        /* We track its lowest free variable so that blocks
         * containing it know whether they have to save a
         * closure or not. */
    unsigned int vars_below;        // how many vars below it when declared?
        /* We save how many variables were below it when it
         * was declared, so that when we call the function,
         * we can jump back to the appropriate varbuffer --
         * since variables are referenced by index rather than
         * name in running code, it's important to have only
         * as many variables in scope as there were when the
         * function was declared. */
} AUserFunc;

/* Builtin or declared function bound to symbol? */
typedef enum {
    primitive_func, // function written in C
    user_func,      // function defined in alma code
    var_push,       // just an instruction to push a named variable
} AFuncType;

/* Struct representing a callable function
 * (either built-in or defined) */
/* (we only have built-in functions right now, tho */
typedef struct AFunc {
    AFuncType type;
    ASymbol *sym;   // we might want to print it at runtime
                    // this is sort of weird because functions might
                    // be known as different things due to imports
                    // or w/e, but this means they'll always show up
                    // as the thing they were first named...
                    // maybe something to change later
    union {
        int varindex; // if var_push, which var to push?
        APrimitiveFunc primitive;
        AUserFunc *userfunc;
            /* using a UserFunc here rather than just a WordSeq
             * pointer, means that the underlying function can
             * be defined later and this one will update
             * automatically */
            /* this is still gonna cause problems for functions
             * that call each other and need to know their
             * types though :( */
    } data;
} AFunc;

/* Struct representing a mapping between symbols and functions */
typedef struct AScopeEntry {
    ASymbol *sym;
    AFunc *func;
    UT_hash_handle hh;
    unsigned int linenum;   // where was it declared?
    int imported;           // was it imported? (shouldn't get imported transitively)
        /* track this so that e.g. if B imports C.x and C.y, and A imports B,
         * A shouldn't get those ones as B.C.x and B.C.y, bc that's extremely silly */
} AScopeEntry;

/* Struct representing a (possibly nested) lexical scope. */
typedef struct AScope {
    struct AScope *parent;
    struct AScope *libscope;
    AScopeEntry *content;
} AScope;

/* Struct that keeps track of all user-defined functions, so that
 * we can free them at the end without keeping track of the number
 * of references to them. */
typedef struct AFuncRegistry {
    AFunc **funcs;
    int size;
    int capacity;
} AFuncRegistry;

#endif
