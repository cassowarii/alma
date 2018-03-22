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
    v->type = str_val;
    v->data.sym = sym;
    return v;
}

AValue *val_block(AAstNode *block) {
    // TODO This is going to need lexical scoping, eventally.
    AValue *v = alloc_val();
    v->type = str_val;
    v->data.ast = block;
    return v;
}
