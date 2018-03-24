#include "value.h"

/* Allocates a value without any data attached */
static
AValue *alloc_val() {
    AValue *new_val = malloc(sizeof(AValue));
    if (new_val == NULL) {
        fprintf(stderr, "Couldn't allocate space for a new variable: Out of memory\n");
        return NULL;
    }
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
    // this does not bind lexical variables - later we can write another
    // function that creates a true block_val from this one
    // (it should leave this one untouched tho because we might want
    //  to reuse it)
    // (That's just uh, how lexical closures work)
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
