#include "alma.h"

int main(int argc, char **argv) {
    stack_type *X = stack_var();
    value_type *a = type_var();
    value_type *thing = func_type(stack_of(a, X), X);
    regeneralize(thing);
    free_type(thing);
}
