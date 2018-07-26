#include "alma.h"
#include "compile.h"
#include "parse.h"

/* Parse a file, compile it into scope using symtab and store its functions
 * in the User Func Registry. */
ACompileStatus put_file_into_scope(const char *filename, ASymbolTable *symtab,
        AScope *scope, AFuncRegistry *reg);

/* Find the filename referred to by a module by searching ALMA_PATH
 * (and the current directory) */
/* NOTE: allocates a new string! Don't forget to free it. */
char *resolve_import(const char *module_name, int append_suffix);

/* Given an import declaration, import it into the current scope
 * (prefixing qualified declaration as appropriate.) */
ACompileStatus handle_import (AScope *scope, ASymbolTable *symtab, AFuncRegistry *reg, AImportStmt *decl);
