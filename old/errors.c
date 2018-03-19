#include "alma.h"
#include <stdarg.h>
#define INITIAL_ERROR_SIZE 4

error *create_error() {
    error *e = malloc(sizeof(error));
    e->errs = calloc(INITIAL_ERROR_SIZE, sizeof(char*));
    e->size = 0;
    e->maxsize = INITIAL_ERROR_SIZE;
    e->line = -1;
}

error *error_msg(char *fmt, ...) {
    error *e = create_error();
    va_list ap;
    va_start(ap, fmt);
    vadd_info(e, fmt, ap);
    va_end(ap);
    return e;
}

void error_lineno(error *e, int line_num) {
    e->line = line_num;
}

void add_info(error *e, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vadd_info(e, fmt, ap);
    va_end(ap);
}

void vadd_info(error *e, char *fmt, va_list ap) {
    if (e->size + 1 > e->maxsize) {
        e->maxsize *= 2;
        e->errs = realloc(e->errs, e->maxsize * sizeof(char*));
    }
    va_list ap2;
    va_copy(ap2, ap);
    vasprintf(&e->errs[e->size], fmt, ap2);
    va_end(ap2);
    e->size += 1;
}

void print_error(error *e) {
    int i;
    for (i = 0; i < e->size; i++) {
        printf("%s\n", e->errs[i]);
    }
}

void error_concat(error *e, error *f) {
    int i;
    for (i = 0; i < f->size; i++) {
        add_info(e, f->errs[i]);
    }
}

void free_error(error *e) {
    int i;
    for (i = 0; i < e->size; i++) {
        free(e->errs[i]);
    }
    free(e->errs);
    free(e);
}
