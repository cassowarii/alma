#include <stdio.h>
#include "alma.h"
#include "parse.h"
#include "ast.h"
#include "eval.h"
#include "scope.h"
#include "lib.h"
#include "compile.h"
#include "registry.h"

ACompileAllocation initialize_compilation(ASymbolTable *symtab);
ACompileStatus compile_file(ADeclSeqNode *program, ASymbolTable symtab,
        AScope *scope, AFuncRegistry *reg);
ACompileStatus put_file_into_scope(const char *filename, ASymbolTable *symtab,
        AScope *scope, AFuncRegistry *reg);
AFunc *finalize_compilation(AScope *scope, ASymbolTable symtab, AFuncRegistry *reg);
int run_main(AFunc *mainfunc);

int main (int argc, char **argv) {
    ASymbolTable symtab = NULL;

    ACompileAllocation comp = initialize_compilation(&symtab);

    ADeclSeqNode *stdlib_parsed = NULL;

    ACompileStatus stdlib_stat = put_file_into_scope("lib/std.alma", &symtab, comp.scope, comp.reg);
    if (stdlib_stat == compile_fail) {
        fprintf(stderr, "failed to init stdlib file\n");
        exit(1);
    }

    free_decl_seq_top(stdlib_parsed);

    FILE *infile = NULL;
    if (argc == 2) {
        infile = fopen(argv[1], "r");
        if (infile == NULL) {
            fprintf(stderr, "couldn't open file\n");
            exit(1);
        }
    } else if (argc == 1) {
        interact(&symtab);
        exit(0);
    } else {
        fprintf(stderr, "Please supply one file name.\n");
        return 0;
    }

    //parse_file(infile, &program, &symtab);
    ADeclSeqNode *program = parse_file(infile, &symtab);

    fclose(infile);

    ACompileStatus stat = compile_file(program, symtab, comp.scope, comp.reg);

    free_decl_seq_top(program);

    AScope *libscope = comp.scope->parent;
    AFunc *mainfunc = finalize_compilation(comp.scope, symtab, comp.reg);

    if (stat == compile_success) {
        run_main(mainfunc);
    }

    free_lib_scope(libscope);
    free_registry(comp.reg);
    free_symbol_table(&symtab);

    return stat == compile_success? 0 : 1;
}

ACompileStatus put_file_into_scope(const char *filename, ASymbolTable *symtab,
        AScope *scope, AFuncRegistry *reg) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return compile_fail;
    } else {
        ADeclSeqNode *file_parsed = parse_file(file, symtab);
        fclose(file);
        ACompileStatus stat = compile_file(file_parsed, *symtab, scope, reg);
        free_decl_seq_top(file_parsed);
        return stat;
    }
}

ACompileAllocation initialize_compilation(ASymbolTable *symtab) {
    AScope *lib_scope = scope_new(NULL);

    AFuncRegistry *reg = registry_new(20);
    lib_init(symtab, lib_scope, 0);

    AScope *real_scope = scope_new(lib_scope);

    ACompileAllocation result = { compile_success, reg, real_scope };
    return result;
}

ACompileStatus compile_file(ADeclSeqNode *program, ASymbolTable symtab,
        AScope *scope, AFuncRegistry *reg) {

    if (program == NULL) {
        fprintf(stderr, "Compilation aborted.\n");
        return compile_fail;
    }

    ACompileStatus stat = compile_in_context(program, symtab, reg, scope);

    if (stat == compile_fail) {
        fprintf(stderr, "Compilation aborted.\n");
    }

    return stat;
}

/* Finalizes compilation; frees compilation data structures and returns
 * a pointer to the main function. */
AFunc *finalize_compilation(AScope *scope, ASymbolTable symtab, AFuncRegistry *reg) {
    AFunc *mainfunc = scope_find_func(scope, symtab, "main");

    if (mainfunc == NULL) {
        fprintf(stderr, "error: cannot find ‘main’ function\n");
    }

    free_scope(scope);

    return mainfunc;
}

int run_main(AFunc *mainfunc) {
    AStack *stack = stack_new(20);

    if (mainfunc == NULL) {
        free_stack(stack);
        return 1;
    }

    /* Call main. */
    eval_word(stack, NULL, mainfunc);

    free_stack(stack);

    return 0;
}
