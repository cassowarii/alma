#include "value.h"

/* Allocates a value without any data attached */
static
AValue *alloc_val(void) {
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
    AValue *v = alloc_val();
    v->type = proto_block;
    v->data.ast = block;
    return v;
}

/* A block with bound variables, created from a value of type free_block_val */
AValue *val_boundblock(AValue *fb, AVarBuffer *buf) {
    assert(fb->type == free_block_val && "can't create a bound block from a not free block");

    AValue *v = alloc_val();
    v->type = bound_block_val;

    AUserFunc *uf = malloc(sizeof(AUserFunc));
    uf->type = bound_func;
    uf->words = fb->data.ast;
    uf->closure = buf;

    varbuf_ref(buf); /* Make sure buf doesn't get deleted out from under us */

    v->data.uf = uf;
    return v;
}

/* Create a value holding a proto-list (when parsing) */
AValue *val_protolist(AProtoList *pl) {
    AValue *v = alloc_val();
    v->type = proto_list;
    v->data.pl = pl;
    return v;
}

/* Create a value holding a real list */
AValue *val_list(AList *l) {
    AValue *v = alloc_val();
    v->type = list_val;
    v->data.list = l;
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

/* Print out a value to an arbitrary filehandle. */
void fprint_val(FILE *out, AValue *v) {
    if (v->type == int_val) {
        fprintf(out, "%d", v->data.i);
    } else if (v->type == block_val
            || v->type == proto_block
            || v->type == free_block_val) {
        fprintf(out, "[ ");
        fprint_wordseq_node(out, v->data.ast);
        fprintf(out, "]");
    } else if (v->type == bound_block_val) {
        /* the '*' means it's attached to a closure.
         * does this make sense? idk. */
        fprintf(out, "*[ ");
        fprint_wordseq_node(out, v->data.uf->words);
        fprintf(out, "]");
    } else if (v->type == str_val) {
        fprintf(out, "\"");
        ustr_fprint(out, v->data.str);
        fprintf(out, "\"");
    } else if (v->type == float_val) {
        fprintf(out, "%g", v->data.fl);
    } else if (v->type == proto_list) {
        fprintf(out, "{ ");
        fprint_protolist(out, v->data.pl);
        fprintf(out, " }");
    } else if (v->type == list_val) {
        fprint_list(out, v->data.list);
    } else if (v->type == sym_val) {
        fprintf(out, "/");
        fprint_symbol(out, v->data.sym);
    } else {
        fprintf(out, "?");
    }
}

/* Print out a value. */
void print_val(AValue *v) {
    fprint_val(stdout, v);
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
            break;
        case proto_block:
        case free_block_val:
        case block_val:
            free_wordseq_node(to_free->data.ast);
            break;
        case bound_block_val:
            free_user_func(to_free->data.uf);
            break;
        case str_val:
            free_ustring(to_free->data.str);
            break;
        case proto_list:
            free_protolist(to_free->data.pl);
            break;
        case list_val:
            free_list(to_free->data.list);
            break;
        default:
            fprintf(stderr,
                    "warning, freeing value of unrecognized type %d.",
                    to_free->type);
    }
    free(to_free);
}
