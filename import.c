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

    char *buf;

    char *modulepath;

    int found = 0;
    char *path = NULL;
    for (char *token = strtok_r(tokiter, ":", &buf); token; token = strtok_r(NULL, ":", &buf)) {
        int extra_slash = 0;
        int extra_extension = 0;
        if (token[strlen(token)-1] != '/') {
            extra_slash = 1;
        }
        if (strcmp(module_name + (strlen(module_name) - 5), ".alma") != 0) {
            extra_extension = 5;
        }

        free(path);
        path = malloc(strlen(token) + strlen(module_name)
                + 1 + extra_slash + extra_extension);

        strcpy(path, token);
        if (extra_slash) strcat(path, "/");
        strcat(path, module_name);
        if (extra_extension) strcat(path, ".alma");

        struct stat buffer;
        if (stat(path, &buffer) == 0) {
            found = 1;
            break;
        }
    }

    if (found) {
        modulepath = path;
    } else {
        free(path);
        modulepath = NULL;
    }

    free(tokiter);
    //free(buf);

    return modulepath;
}

/* Given a symbol, a prefix, and a delimiter, generate a new symbol with the
 * prefix attached using the delimiter.
 * e.g. symbol "abc", prefix "xyz", delim "." -> symbol "xyz.abc" */
static
ASymbol *prefix_symbol(ASymbolTable *symtab, char *prefix,
            const char *delim, ASymbol *sym) {
    char *newname = malloc(strlen(prefix) + strlen(delim) + strlen(sym->name) + 1);
    strcpy(newname, prefix);
    strcat(newname, delim);
    strcat(newname, sym->name);
    return get_symbol(symtab, newname);
}

/* Given a string like abc/def/ghi, extract the last part (ghi) and
 * return it in a new string. If strip_ext is true, also removes everything
 * after the last '.' as well. */
/* Hooray, C string handling! */
static
char *extract_mod_prefix(const char *path, int strip_ext) {
    int last_slash = -1;
    int pathlen = strlen(path);
    int last_dot = pathlen;
    for (int i = 0; i < pathlen; i++) {
        if (path[i] == '/') {
            last_slash = i;
        }
        if (strip_ext && path[i] == '.') {
            last_dot = i;
        }
    }
    /* If the last dot was actually in a directory name or something,
     * ignore it. */
    if (last_slash > last_dot) last_dot = pathlen;
    /* last_dot - last_slash is actually 1 more character than we need
     * (since this range is inclusive on both ends) but that's ok bc
     * we need a \0 */
    char *result = malloc(last_dot - last_slash);
    strncpy(result, path + last_slash + 1, last_dot - last_slash - 1);
    result[last_dot - last_slash - 1] = '\0';
    return result;
}

/* Given an import declaration, import it into the current scope
 * (prefixing qualified declaration as appropriate.) */
ACompileStatus handle_import (AScope *scope, ASymbolTable *symtab,
            AFuncRegistry *reg, AImportStmt *decl) {
    AScope *module_scope = scope_new(scope);

    char *filename;
    if (decl->just_string) {
        filename = malloc(strlen(decl->module)+1);
        strcpy(filename, decl->module);
    } else {
        filename = malloc(strlen(decl->module)+6);
        strcpy(filename, decl->module);
        strcat(filename, ".alma");
    }

    ACompileStatus result = compile_fail;
    char *file_loc = resolve_import(filename);
    if (file_loc == NULL) {
        fprintf(stderr, "Couldn't find ‘%s’ anywhere in ALMA_PATH\n"
                "(ALMA_PATH is: %s)\n", filename, ALMA_PATH);
    } else {
        result = put_file_into_scope(file_loc, symtab, module_scope, reg);
        free(file_loc);
    }
    free(filename);

    if (result == compile_fail) {
        return result;
    }

    /* Now that we have successfully compiled the module to import,
     * we need to put it into our scope with optional prefix. */
    char *prefix = extract_mod_prefix(decl->module, decl->just_string);

    /* Move all the functions from module scope into our scope,
     * prefixing them with the module name. */
    AScopeEntry *current, *tmp;
    HASH_ITER(hh, module_scope->content, current, tmp) {
        ASymbol *prefixed_name = prefix_symbol(symtab, prefix, ".", current->sym);
        scope_register(scope, prefixed_name, current->func);
    }

    free(prefix);
    return result;
}
