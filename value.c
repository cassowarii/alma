#include "value.h"

/* Allocates a value without any data attached */
static
AValue *alloc_val() {
    AValue *new_val = malloc(sizeof(AValue));
    if (new_val == NULL) {
        fprintf(stderr, "Couldn't allocate space for a new variable: Out of memory\n");
        return NULL;
    }
    new_val->refs = 0;
    return new_val;
}

AValue *val_int(int data) {
    AValue *v = alloc_val();
    v->type = int_val;
    v->data.i = data;
    return v;
}

AValue *val_float(float data) {
    AValue *v = alloc_val();
    v->type = float_val;
    v->data.fl = data;
    return v;
}

AValue *val_str(AUstr *str) {
    AValue *v = alloc_val();
    v->type = str_val;
    v->data.str = str;
    return v;
}

AValue *val_sym(ASymbol *sym) {
    AValue *v = alloc_val();
    v->type = sym_val;
    v->data.sym = sym;
    return v;
}

AValue *val_block(AWordSeqNode *block) {
    /* this does not bind lexical variables - later we can write another
    .* function that creates a true block_val from this one
    .* (it should leave this one untouched tho because we might want
    .*  to reuse it)
     * (That's just uh, how lexical closures work) */
    AValue *v = alloc_val();
    v->type = proto_block;
    v->data.ast = block;
    return v;
}

/* Create a value holding a proto-list (when parsing) */
AValue *val_protolist(AProtoList *pl) {
    AValue *v = alloc_val();
    v->type = proto_list;
    v->data.pl = pl;
    return v;
}

/* Get a fresh pointer to the object that counts as a reference. */
AValue *ref(AValue *v) {
    v->refs ++;
    return v;
}

/* Delete a reference to the object, reducing its refcount and
 * potentially freeing it. */
void delete_ref(AValue *v) {
    v->refs --;
    if (v->refs < 1) {
        free_value(v);
    }
}

extern void print_wordseq_node(AWordSeqNode *ast);
extern void print_symbol(ASymbol *s);
extern void ustr_print(AUstr *u);
extern void print_protolist(AProtoList *pl);

void print_val(AValue *v) {
    if (v->type == int_val) {
        printf("%d", v->data.i);
    } else if (v->type == block_val || v->type == proto_block) {
        printf("[ ");
        print_wordseq_node(v->data.ast);
        printf(" ]");
    } else if (v->type == str_val) {
        printf("\"");
        ustr_print(v->data.str);
        printf("\"");
    } else if (v->type == float_val) {
        printf("%g", v->data.fl);
    } else if (v->type == proto_list) {
        printf("{ ");
        print_protolist(v->data.pl);
        printf(" }");
    } else if (v->type == sym_val) {
        printf("/");
        print_symbol(v->data.sym);
    } else {
        printf("?");
    }
}

/* Print out a value without quoting strings etc.
 * (called by 'print' word) */
void print_val_simple(AValue *v) {
    if (v->type == str_val) {
        /* no quotes!!! */
        ustr_print(v->data.str);
    } else {
        print_val(v);
    }
}

extern void free_wordseq_node(AWordSeqNode *to_free);
extern void free_ustring(AUstr *str);
extern void free_protolist(AProtoList *pl);

/* Free a value. */
void free_value(AValue *to_free) {
    switch(to_free->type) {
        case int_val:
        case float_val:
        case sym_val:
            /* if int, float, or symbol ptr, we just free this one
             * which will free attached int or float. Symbols are
             * all freed at program exit. */
            free(to_free);
            break;
        case proto_block:
        case block_val:
            free_wordseq_node(to_free->data.ast);
            free(to_free);
            break;
        case str_val:
            free_ustring(to_free->data.str);
            free(to_free);
            break;
        case proto_list:
            free_protolist(to_free->data.pl);
            free(to_free);
            break;
        default:
            fprintf(stderr,
                    "warning, freeing node of unrecognized type %d.",
                    to_free->type);
            free(to_free);
    }
}
