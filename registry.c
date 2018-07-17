#include "registry.h"
#include "ast.h"

/* Create a new User Func Registry. */
AFuncRegistry *registry_new(unsigned int initial_capacity) {
    AFuncRegistry *reg = malloc(sizeof(AFuncRegistry));
    reg->size = 0;
    reg->capacity = initial_capacity;
    reg->funcs = malloc(reg->capacity * sizeof(AUserFunc*));
    return reg;
}

/* Register a new function into the User Func Registry. */
void registry_register(AFuncRegistry *reg, AFunc *f) {
    if (reg->size == reg->capacity) {
        AFunc **newdata = realloc(reg->funcs, reg->capacity * 2 * sizeof(AFunc*));
        if (newdata == NULL) {
            printf("Could not expand the User Func Registry: Out of memory.\n");
            return;
        }
        reg->capacity *= 2;
        reg->funcs = newdata;
    }
    reg->funcs[reg->size] = f;
    reg->size ++;
}

extern void free_func(AFunc *f);

/* Free all functions in the User Func Registry. */
void free_registry(AFuncRegistry *reg) {
    for (int i = 0; i < reg->size; i++) {
        if (reg->funcs[i]->type == user_func) {
            free_wordseq_node(reg->funcs[i]->data.userfunc->words);
        }
        free_func(reg->funcs[i]);
    }
    free(reg->funcs);
    free(reg);
}
