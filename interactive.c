#include "alma.h"

void setup_interactives() {
    repling = 1;
    newlined = 1;
    asprintf(&primary_prompt, ">>> ");
    asprintf(&secondary_prompt, "... ");
#ifdef __GNUC__
    char *compiler = "GCC";
    int compvers[3] = { __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__ };
#else
    char *compiler = "something else";
    char *compvers = { 0, 0, 0 };
#endif
#if defined(__linux__)
    char *platform = "linux";
#elif defined(__APPLE__)
    char *platform = "apple";
#elif defined(WIN32)
    char *platform = "windows";
#else
    char *platform = "computers";
#endif
    asprintf(&motd, "Alma language interpreter version %s\nCompiled on %s at %s, with %s %d.%d.%d for %s.\n",
            ALMA_VERSION, __DATE__, __TIME__, compiler, compvers[0], compvers[1], compvers[2], platform);
}

// Only used interactively - we want to allow things that might cause
// the stack to bottom out normally, since there might already be stuff
// on the stack.
stack_type *type_of_current_stack(elem_t *top) {
    if (top == NULL) {
        return zero_stack();
    }
    return stack_of(top->type, type_of_current_stack(top->next));
}
