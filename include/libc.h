#ifndef LIBC_H
#define LIBC_H

#include "common.h"

/*
 * The C library's functions that the game's code calls: memory, strings,
 * number conversion and formatted printing (newlib, linked into the boot
 * executable). Each is declared as the game's code calls it.
 */

int abs(int);
void free(void *p);
s32 memcmp(void *a, void *b, s32 n);
void *memcpy(void *dst, const void *src, u32 n);
void *memmove(void *dst, const void *src, u32 n);
void *memset(void *p, s32 c, u32 n);
s32 printf(const char *fmt, ...); /* printf */
s32 rand(void);
s32 sprintf(char *, const char *, ...);
char *strcat(char *d, const char *s);
char *strchr(char *s, s32 c); /* strchr */
s32 strcmp(void *a, void *b);
char *strcpy(char *dst, const char *src);
u32 strlen(const void *s);
s32 strncmp(const char *, const char *, s32);
char *strncpy(void *, void *, u32); /* strncpy */
u8 *strstr(const u8 *s, const char *w); /* strstr */

#endif
