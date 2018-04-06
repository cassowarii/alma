#ifndef _AL_COMP_H__
#define _AL_COMP_H__

#include "alma.h"
#include "ast.h"
#include "scope.h"
#include "vars.h"

/* Mutate an ADeclSeqNode by replacing compile-time-resolvable
 * symbol references with references to AFunc*'s. */
ACompileStatus compile(AScope *scope, AFuncRegistry *reg, ADeclSeqNode *program, ABindInfo bindinfo);

/* Mutate an ADeclSeqNode by replacing compile-time-resolvable
 * symbol references with references to AFunc*'s. */
/* var_depth = how many variables are bound below this scope */
ACompileStatus compile(AScope *scope, AFuncRegistry *reg, ADeclSeqNode *program, ABindInfo bindinfo);

/* Compile a given declseq in context of preexisting symbol table, registry, scope. */
ACompileStatus compile_in_context(ADeclSeqNode *program,
        ASymbolTable symtab, AFuncRegistry *reg, AScope *scope);

/* Compile a given wordseq in context of preexisting symbol table, registry, scope. */
ACompileStatus compile_seq_context(AWordSeqNode *seq,
        ASymbolTable symtab, AFuncRegistry *reg, AScope *scope);

#endif
