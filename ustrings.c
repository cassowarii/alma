#include "alma.h"

/* Create a new string (actually a sequence of 32-bit integers
 * representing UTF8 codepoints) */
AUstr *ustr_new(size_t initial_size) {
    AUstr *newstr = malloc(sizeof(AUstr));
    if (newstr == NULL) {
        fprintf(stderr, "Couldn't allocate space for a new string: Out of memory\n");
        return NULL;
    }
    newstr->data = malloc(initial_size * sizeof(uint32_t));
    if (newstr->data == NULL) {
        fprintf(stderr, "Couldn't allocate space for a new string: Out of memory\n");
        return NULL;
    }
    newstr->capacity = initial_size;
    newstr->length = 0;
    return newstr;
}

/* Append a new codepoint to a ustr.
 * ustr's are immutable in alma -- this function is
 * only called during compilation. */
void ustr_append(AUstr *u, uint32_t ch) {
    if (u->length == u->capacity) {
        uint32_t *newdata = realloc(u->data, u->capacity * 2 * sizeof(uint32_t));
        if (newdata == NULL) {
            printf("Couldn't resize string to append character: Out of memory\n");
            return;
        }
        u->capacity *= 2;
        u->data = newdata;
    }
    u->data[u->length] = ch;
    u->length ++;
}

/* Finish off a string by cutting off the unused
 * space on the end (again, only happens in compilation
 * phase) */
void ustr_finish(AUstr *u) {
    if (u->capacity > u->length) {
        uint32_t *newdata = realloc(u->data, u->length * 2 * sizeof(uint32_t));
        if (newdata == NULL) {
            printf("Couldn't resize string to finalize: Out of memory\n");
            return;
        }
        u->data = newdata;
    }
}

/* Print a character represented by a Unicode codepoint. */
void print_char(uint32_t utf8) {
    for (int i = 3; i >= 0; i--) {
        char byte = (((unsigned)utf8 & (0xFF << (8 * i))) >> (8 * i));
        if (byte != '\0') {
            printf("%c", byte);
        }
    }
}

/* Print a AUstr character-by-character. */
void ustr_print(AUstr *u) {
    for (int i = 0; i < u->length; i++) {
        print_char(u->data[i]);
    }
}

/* Parse a UTF8 character-literal into a 4-byte int. */
uint32_t char_parse(const char *utf8, unsigned int length) {
    uint32_t total = 0;
    if (utf8[0] == '\\') {
        /* escape character */
        if (length == 2) {
            switch(utf8[1]) {
                case 'a':
                    return 0x07;
                case 'b':
                    return 0x08;
                case 'f':
                    return 0x0C;
                case 'n':
                    return 0x0A;
                case 'r':
                    return 0x0D;
                case 't':
                    return 0x09;
                case 'v':
                    return 0x0B;
                case '\\':
                    return '\\';
                case '\'':
                    return '\'';
                case '"':
                    return '"';
                default:
                    printf("Unrecognized escape sequence %.*s\n", length, utf8);
                    return utf8[1];
            }
        } else {
            printf("Unrecognized escape sequence %.*s\n", length, utf8);
            return utf8[1];
        }
    } else {
        for (int i = 0; i < length; i++) {
            total <<= 8;
            total += (unsigned char)utf8[i];
        }
        return total;
    }
}

int is_u2(unsigned char x) {
    return (0xC2 <= x && x <= 0xDF);
}

int is_u3(unsigned char x) {
    return (0xE0 <= x && x <= 0xEF);
}

int is_u4(unsigned char x) {
    return (0xF0 <= x && x <= 0xF4);
}

/* Parse a const char * into a ustring using char_parse */
AUstr *parse_string(const char *bytes, unsigned int length) {
    /* The number of codepoints in the string is AT MOST the number
     * bytes in the string. If it's less, the extra gets clipped off
     * at the end. */
    AUstr *newstr = ustr_new(length);
    if (newstr == NULL) {
        fprintf(stderr, "Couldn't allocate a new ustring.\n");
        return NULL;
    }

    int index = 0;
    while (index != length) {
        if (index > length) {
            fprintf(stderr, "String encode error: String doesn't form valid UTF-8");
            break;
        }

        // Calculate the length of the character.
        unsigned int char_length = 1;

        unsigned char checkbyte = bytes[index];
        unsigned int extra = 0; // in case there's a \ character
        if (checkbyte == '\\') {
            checkbyte = bytes[index + 1];
            char_length = 2; // by default
            extra = 1;
        }
        if (is_u2(checkbyte)) char_length = 2 + extra;
        if (is_u3(checkbyte)) char_length = 3 + extra;
        if (is_u4(checkbyte)) char_length = 4 + extra;

        uint32_t ch = char_parse(bytes + index, char_length);

        ustr_append(newstr, ch);

        index += char_length;
    }

    ustr_finish(newstr);

    return newstr;
}
