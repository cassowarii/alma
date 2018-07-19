#include "import.h"

/* Parse a file, compile it into scope using symtab and store its functions
 * in the User Func Registry. */
ACompileStatus put_file_into_scope(const char *filename, ASymbolTable *symtab,
        AScope *scope, AFuncRegistry *reg) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        char errbuf[512];
        int err_result = strerror_r(errno, errbuf, 512);
        if (err_result == 0) {
            fprintf(stderr, "Couldn't open file %s: [Errno %d] %s\n", filename, errno, errbuf);
        } else {
            fprintf(stderr, "Couldn't open file %s: [Errno %d]\n", filename, errno);
            fprintf(stderr, "Also, an error occurred trying to figure out what error occurred. "
                            "May god have mercy on our souls.\n");
        }
        return compile_fail;
    } else {
        ADeclSeqNode *file_parsed = parse_file(file, symtab);
        fclose(file);

        if (file_parsed == NULL) {
            fprintf(stderr, "Compilation aborted.\n");
            return compile_fail;
        }

        ACompileStatus stat = compile_in_context(file_parsed, symtab, reg, scope);
        free_decl_seq_top(file_parsed);
        return stat;
    }
}

/* Find the filename referred to by a module by searching ALMA_PATH
 * (and the current directory) */
/* NOTE: allocates a new string! Don't forget to free it. */
char *resolve_import(const char *module_name) {
    char *tokiter = malloc(strlen(ALMA_PATH)+1);
    strcpy(tokiter, ALMA_PATH);

    char *buf = malloc(strlen(ALMA_PATH)+1); // for strtok_r

    char *modulepath;

    for (char *token = strtok_r(tokiter, ":", &buf); token; token = strtok_r(NULL, ":", &buf)) {
        int extra_slash = 0;
        int extra_extension = 0;
        if (ALMA_PATH[strlen(ALMA_PATH)-1] != '/') extra_slash = 1;
        if (strcmp(module_name + (strlen(module_name) - 5), ".alma") != 0) {
            extra_extension = 5;
        }

        char *modulepath = malloc(strlen(ALMA_PATH) + strlen(module_name)
                + 1 + extra_slash + extra_extension);

        strcpy(modulepath, ALMA_PATH);
        if (extra_slash) strcat(modulepath, "/");
        strcat(modulepath, module_name);
        if (extra_extension) strcat(modulepath, ".alma");
    }

    free(buf);
    free(tokiter);

    return modulepath;
}
