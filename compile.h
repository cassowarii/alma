#ifndef _AL_COMP_H__
#define _AL_COMP_H__

#include "alma.h"
#include "ast.h"
#include "scope.h"
#include "vars.h"

/* Mutate an ADeclSeqNode by replacing compile-time-resolvable
 * symbol references with references to AFunc*'s. */
ACompileStatus compile(AScope *scope, AFuncRegistry *reg, ADeclSeqNode *program);

#endif
