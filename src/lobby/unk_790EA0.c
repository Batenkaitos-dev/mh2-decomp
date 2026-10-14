/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

s32 sprintf(char *, const char *, ...);
extern M2C_UNK D_0083B7F8;

void Str_FromInt(s32 n, char *buf) {
    sprintf(buf, (const char *)&D_0083B7F8, n);
}

/* unk_790EA0's .rodata (0x83B7F8-0x83B800) */
#pragma rodata on
s32 D_0083B7F8 = {
#include "lobby/0083B7F8.inc"
};
#pragma rodata off
