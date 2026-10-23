#ifndef WORD_FILTER_H
#define WORD_FILTER_H

#include "common.h"

/*
 * The chat word filter: WordFilter_Scan looks for the filter list's words
 * in a line, compared in upper case with look-alike characters folded, and
 * says whether the line is clean or stars the hits out; Kbd_RunFiltered
 * runs the chat keyboard through it.
 */

s32 WordFilter_Scan(s8 *str, s32 replace);
void WordFilter_Normalize(s8 *dst, s8 *src, s32 n);
s32 WordFilter_IsWordEnd(s8 *name);
s32 WordFilter_InList(s8 *name, s8 *list);
s32 WordFilter_ReadEntry(s8 *src, s8 *dst, s32 max);
s8 Kbd_RunFiltered(char *text, s16 held, s16 pressed);

#endif
