#include "alma.h"

void setup_interactives() {
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
