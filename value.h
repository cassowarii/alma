#ifndef _AL_VAL_H__
#define _AL_VAL_H__

#include "alma.h"

AValue *val_int(int data);
AValue *val_float(float data);
AValue *val_str(AUstr *str);
AValue *val_sym(ASymbol *sym);
AValue *val_block(AAstNode *block);

#endif
