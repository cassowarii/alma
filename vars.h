#ifndef _AL_VARS_H__
#define _AL_VARS_H__

#include "alma.h"

/* Create a new var-bind instruction, with the names from the
 * <names> ANameSeqNode. */
AVarBind *varbind_new(ANameSeqNode *names, AWordSeqNode *words);

#endif
