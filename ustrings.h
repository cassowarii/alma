#ifndef _AL_USTR_H__
#define _AL_USTR_H__

#include "alma.h"

/* Create a new string (actually a sequence of 32-bit integers
 * representing UTF8 codepoints) */
AUstr   *ustr_new(size_t initial_size);

/* Append a new codepoint to a ustr.
 * AUstr's are immutable in alma -- this function is
 * only called during compilation. */
void    ustr_append(AUstr *u, uint32_t ch);

/* Finish off a string by cutting off the unused
 * space on the end (again, only happens in compilation
 * phase) */
void    ustr_finish(AUstr *u);

/* Print a character represented by a Unicode codepoint. */
void    print_char(uint32_t utf8);

/* Print an AUstr character-by-character. */
void    ustr_print(AUstr *u);

/* Print an AUstr character-by-character, to an arbitrary filehandle. */
void    ustr_fprint(FILE *out, AUstr *u);

/* Parse a UTF8 character-literal into a 4-byte int. */
uint32_t char_parse(const char *utf8, unsigned int length);

/* Parse a const char * into an AUstr using char_parse */
AUstr   *parse_string(const char *bytes, unsigned int length);

/* Turn a ustring back into a char*. Allocates a new string. */
char *ustr_unparse(AUstr *ustr);

/* Compare two ustrings to see if they're equal. */
int ustr_eq(AUstr *str1, AUstr *str2);

/* Free a ustring. */
void free_ustring(AUstr *str);

#endif
