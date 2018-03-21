#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "uthash.h"

typedef struct ustr {
    unsigned int capacity;
    unsigned int length;
    uint32_t *data;
} ustr;

/*-*-*-* ustrings.c *-*-*-*/

/* Create a new string (actually a sequence of 32-bit integers
 * representing UTF8 codepoints) */
ustr   *ustr_new(size_t initial_size);
/* Append a new codepoint to a ustr.
 * ustr's are immutable in alma -- this function is
 * only called during compilation. */
void    ustr_append(ustr *u, uint32_t ch);
/* Finish off a string by cutting off the unused
 * space on the end (again, only happens in compilation
 * phase) */
void    ustr_finish(ustr *u);
/* Print a character represented by a Unicode codepoint. */
void print_char(uint32_t utf8);
/* Print a ustr character-by-character. */
void ustr_print(ustr *u);
/* Parse a UTF8 character-literal into a 4-byte int. */
uint32_t char_parse(const char *utf8, unsigned int length);
/* Parse a const char * into a ustring using char_parse */
ustr *parse_string(const char *bytes, unsigned int length);

/*-*-*-* symbols.c *-*-*-*/


