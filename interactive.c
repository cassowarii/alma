#include "interactive.h"

#ifdef __GNUC__
#define ALMACOMPILER "GCC"
#define ACVER1 __GNUC__
#define ACVER2 __GNUC_MINOR__
#define ACVER3 __GNUC_PATCHLEVEL__
#else
#define ALMACOMPILER "something else"
#define ACVER1 0
#define ACVER2 0
#define ACVER3 0
#endif
#if defined(__linux__)
#define ACPLATFORM "linux"
#elif defined(__APPLE__)
#define ACPLATFORM "apple"
#elif defined(WIN32)
#define ACPLATFORM "windows"
#else
#define ACPLATFORM "computers"
#endif

void interactive_mode() {
    printf("Alma language interpreter version "ALMA_VERSION" \""ALMA_VNAME"\"\n");
    printf("Compiled on "__DATE__" at "__TIME__", with "ALMACOMPILER" "
           "%d.%d.%d for "ACPLATFORM".\n", ACVER1, ACVER2, ACVER3);

    char *p;
    do {
        p = readline("alma> ");
    } while (p != NULL);
}
