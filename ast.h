#ifndef _AL_AST_H__
#define _AL_AST_H__

#include "alma.h"

AAstNode *ast_valnode(unsigned int location, AValue *val);

AAstNode *ast_wordnode(unsigned int location, ASymbol *sym);

AAstNode *ast_parennode(unsigned int location, AAstNode *content);

ADeclNode *ast_decl(unsigned int location, ASymbol *sym, AAstNode *body);

#endif
