#include "cloth.h"

int main(int argc, char **argv) {
    char *s = malloc(5);
    strcpy(s, "test");
    char *t = malloc(6);
    strcpy(t, "test!");
    error *e = error_msg("this is my %s and my %s", s, t);
    print_error(e);
}
