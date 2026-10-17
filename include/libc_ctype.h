#ifndef LIBC_CTYPE_H
#define LIBC_CTYPE_H

/*
 * The character classes of newlib's ctype table, which the boot executable
 * links (main's gCtypeTable). gCtypeTable[1 + c] holds the class bits of
 * the byte c (entry 0 is for EOF). The game tests them directly instead of
 * calling isalnum and the like (select_ngword.c, the lobby's word filter).
 */
#define CTYPE_UPPER 0x01
#define CTYPE_LOWER 0x02
#define CTYPE_DIGIT 0x04
#define CTYPE_SPACE 0x08
#define CTYPE_PUNCT 0x10
#define CTYPE_CONTROL 0x20
#define CTYPE_HEX 0x40
#define CTYPE_BLANK 0x80
#define CTYPE_ALNUM (CTYPE_UPPER | CTYPE_LOWER | CTYPE_DIGIT) /* 7: a letter or a digit */

#endif /* LIBC_CTYPE_H */
