#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "uthash.h"
#define node(x, y, z) _node_lineno((x), (y), (z), yylineno)

enum node_tag {
    N_BLOCK,
    N_LIST,
    N_WORD,
    N_STRING,
    N_INT,
    N_BOOL,
    N_FLOAT,
    N_CHAR,
    N_COMPOSED,
    N_DEFINE,
    N_ELEM,
};

enum node_flags {
    NF_COPIED = 1,
};

typedef struct node_t {
    enum node_tag tag;
    union {
        int n_int;
        double n_float;
        char n_char;
        char *n_str;
        struct elem_t *elem;
        struct {
            struct node_t* left;
            struct node_t* right;
        } children;
    } content;
    int flags;
    int line;
} node_t;

struct stack_type;

// tag for types of single values
enum vtype_tag {
    V_VAR,
    V_SCALARVAR,
    V_FUNC,
    V_LIST,
    V_BASETYPE,
    V_PRODUCT,
    V_UNIFIED,
    V_ERROR,
};

typedef struct value_type {
    enum vtype_tag tag;
    long int id;
    union {
        char var_name;
        struct {
            struct stack_type *in;
            struct stack_type *out;
        } func_type;
        struct {
            struct value_type *left;
            struct value_type *right;
        } prod_type;
        struct value_type *v;
        char *type_name;
        struct error *err;
    } content;
    int refs;
} value_type;

value_type *vt_num;
value_type *vt_bool;
value_type *vt_char;

// tag for types of stacks
enum stype_tag {
    S_VAR,
    S_TOPTYPE,
    S_UNIFIED,
    S_ZERO,
    S_ERROR,
};

typedef struct stack_type {
    enum stype_tag tag;
    long int id;
    union {
        char var_name;
        struct {
            value_type *top;
            struct stack_type *rest;
        } top_type;
        struct stack_type *unif;
        struct error *err;
    } content;
    int refs;
} stack_type;

typedef struct type_mapping {
    UT_hash_handle hh;
    void *in;
    void *out;
} type_mapping;

enum elem_tag {
    E_CHAR,
    E_INT,
    E_BOOL,
    E_FLOAT,
    E_LIST,
    E_PRODUCT,
    E_BLOCK,
};

typedef struct elem_t {
    enum elem_tag tag;
    value_type *type;
    union {
        int e_int;
        double e_float;
        char e_char;
        char *e_str;
        struct elem_t *list;
        struct {
            struct elem_t *left;
            struct elem_t *right;
        } product;
        node_t *block;
    } content;
    unsigned int height;
    struct elem_t *next;
    struct elem_t *prev;
} elem_t;

typedef elem_t *Word(elem_t **top);

enum type_tag {
    K_ANY,
    K_SCALAR,
    K_LIST,
    K_NUM,
    K_CHAR,
    K_BLOCK,
};

typedef struct lib_entry_t {
    UT_hash_handle hh;
    char *name;
    value_type *type;
    int internal;       // is it implemented in C or in alma itself??
    union {
        Word *func;     // c implementation
        node_t *node;   // Alma implementation
    } impl;
} lib_entry_t;

lib_entry_t *create_entry();

lib_entry_t *construct(const char *name, value_type *type, Word *func);

typedef struct library {
    lib_entry_t *table;
} library;

void init_library(library *l);

void add_lib_entry(library *l, lib_entry_t *entry);

library lib;

elem_t *copy_elem(elem_t *e);
elem_t *copy_list(elem_t *l);

// 0, empty list, empty string = false; everything else = true
int truthy(elem_t *e);
// E_CHAR, E_INT, E_FLOAT, E_BLOCK
int scalar_type(enum type_tag t);
int is_scalar(elem_t *e);
// E_LIST, E_LAZYSTRING (converts to LIST when this called)
int is_list(elem_t *e);
// E_BLOCK
int is_block(elem_t *e);

void eval(node_t *node, elem_t **top);

void do_word(const char *word, elem_t **top);
elem_t *apply(lib_entry_t *e, elem_t **top);

elem_t *new_elem();
elem_t *list_from(elem_t *head);
elem_t *elem_from(node_t *node);

void print_elem(elem_t *e);
void print_string(elem_t *e);

void push(elem_t *new_elem, elem_t **top);

elem_t *stack_top;

void free_elem(elem_t *e);

void free_elems_below(elem_t *e);

void free_node(node_t *n);

void throw_error(const char *errstr, int line_no);

typedef struct error {
    int size;
    int maxsize;
    char **errs;
    int line;
} error;

error *create_error();
error *error_msg(char *msg, ...);
void error_lineno(error *e, int line_num);
void vadd_info(error *e, char *fmt, va_list ap);
void add_info(error *e, char *fmt, ...);
void print_error(error *e);
void error_concat(error *e, error *f);
void free_error(error *e);

//node_t *node (enum node_tag tag, node_t *left, node_t *right);

node_t *new_node();

node_t *first_node_in(node_t *nn);
node_t *last_node_in(node_t *nn);

char *string_node(node_t *n);

void set_left(node_t *node, node_t *l);
void set_right(node_t *node, node_t *r);
node_t *left(node_t *node);
node_t *right(node_t *node);

node_t *_node_lineno(enum node_tag tag, node_t *nleft, node_t *nright, int line_num);
node_t *node_elem (elem_t *val, int line_num);
node_t *node_str (char *str, int line_num);
node_t *node_word (char *name, int line_num);
node_t *node_int (int val, int line_num);
node_t *node_bool (int val, int line_num);
node_t *node_float (double val, int line_num);
node_t *node_char (char val, int line_num);

node_t *copy_node(node_t *n);
int node_copied(node_t *n);

elem_t *elem_int (int val);
elem_t *elem_bool (int val);
elem_t *elem_float (double val);
elem_t *elem_char (char val);
elem_t *elem_str (char *val);
elem_t *elem_list (elem_t *content);
elem_t *elem_product (elem_t *l, elem_t *r);

elem_t *pop(elem_t **top);

/* -- Util -- */
// alloc'ing functions
int vasprintf(char **strp, const char *fmt, va_list ap);
int asprintf(char **strp, const char *fmt, ...);
// realloc'ing functions
int vrasprintf(char *str, const char *fmt, va_list ap);
int rasprintf(char *str, const char *fmt, ...);
char *rstrcat(char **dest, char *src);

/* -- Library -- */
elem_t *word_pop(elem_t **top);
elem_t *word_list(elem_t **top);
void do_list(elem_t **top);

/* -- Types -- */
void set_type (elem_t *e, value_type *t);
value_type *base_type(const char *name);
value_type *list_of(value_type *what);
value_type *func_type(stack_type *from, stack_type *to);
value_type *product_type(value_type *left, value_type *right);
value_type *type_var();
value_type *scalar_var();
value_type *error_type(error *e);
stack_type *stack_of(value_type *top, stack_type *rest);
stack_type *stack_var();
stack_type *zero_stack();
stack_type *error_stacktype(error *e);

void regeneralize(value_type **tpt);

int compare_types(value_type *a, value_type *b);

value_type *get_list_type(elem_t *e);

value_type *copy_type(value_type *t);
stack_type *copy_stack_type(stack_type *t);

value_type *infer_type(node_t *nn);

void init_types();
char *string_type(value_type *t);
void print_stack_type(stack_type *t);
char *string_stack_type(stack_type *t);
void print_type(value_type *t);

int confirm_stack_type(stack_type *t, elem_t *e);
stack_type *unify_stack(stack_type *a, stack_type *b);
value_type *unify(value_type *a, value_type *b);
void bestow_varnames (value_type *t);

void free_type(value_type *t);
void free_stack_type(stack_type *t);
