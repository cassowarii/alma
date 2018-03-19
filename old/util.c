#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int my_vasprintf(char **strp, const char *fmt, va_list ap) {
    // okay, dunno why this doesn't work but we'll just pretend like
    // everyone's running glibc for now and reimplement this later...
    int len = vsnprintf(NULL, 0, fmt, ap);
    if (len == -1) {
        return -1;
    }
    size_t size = (size_t)(len + 1);
    char *str = malloc(size);
    if (!str) {
        return -1;
    }
    printf("len is %d; str is %p; fmt is %p; ap is %p\n", len + 1, str, fmt, ap);
    va_list ap2;
    va_copy(ap2, ap);
    int r = vsnprintf(str, len + 1, fmt, ap2);
    va_end(ap2);
    if (r == -1) {
        free(str);
        return -1;
    }
    printf("strp is %p, *strp is %p\n", strp, *strp);
    *strp = str;
    printf("strp is %p, *strp is %p\n", strp, *strp);
    return r;
}

int my_asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = my_vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}

int my_vrasprintf(char *str, const char *fmt, va_list ap) {
    int len = vsnprintf(NULL, 0, fmt, ap);
    if (len == -1) {
        return -1;
    }
    size_t size = (size_t)len + 1;
    str = realloc(str, size);
    if (!str) {
        return -1;
    }
    int r = vsnprintf(str, len + 1, fmt, ap);
    if (r == -1) {
        free(str);
        return -1;
    }
    return r;
}

int my_rasprintf(char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = my_vrasprintf(str, fmt, ap);
    va_end(ap);
    return r;
}

char *rstrcat(char **dest, char *src) {
    int len = snprintf(NULL, 0, "%s%s", src, *dest);
    *dest = realloc(*dest, len + 1);
    strcat(*dest, src);
    return *dest;
}
