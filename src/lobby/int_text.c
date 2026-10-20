/*
 * Str_FromInt: writes a number as decimal text (sprintf with "%d"). The
 * lobby's text fills (Text_FillDollar, Text_FillDollar2) put it in place
 * of a template's '$'. The .rodata holds the format.
 */
#include "common.h"
#include "m2c_macros.h"

s32 sprintf(char *, const char *, ...);
extern s32 gIntFmt;

/* Writes n into buf as decimal text. */
void Str_FromInt(s32 n, char *buf)
{
    sprintf(buf, (const char *)&gIntFmt, n);
}

/* The file's .rodata: the "%d" format, kept as an s32 (its four bytes) as
 * the unit defines it. */
#pragma rodata on
s32 gIntFmt = {
#include "lobby/0083B7F8.inc"
};
#pragma rodata off
