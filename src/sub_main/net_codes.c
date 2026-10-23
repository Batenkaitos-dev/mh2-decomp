/*
 * Two encodings the lobby's packets use: Net_LockrockObfuscate scrambles
 * a packet's bytes with an 8-byte key (gLockrockKey, this file's data)
 * and a seed, and Net_ChallengeReply builds the 10-digit answer to the
 * lobby server's challenge from the two 4-digit halves of the account id
 * and a key. Networking code stays byte-identical.
 */

#include "common.h"
#include "net_codes.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

const char D_0046A848[9] = {
#include "sub_main/0046A848.inc"
};

extern u8 *gLockrockKey;              /* the 8-byte key */

/* Obfuscates len bytes of in into out: each byte xored with the key byte (i & 7) and with
 * salt + seed + i; returns the sum of the input bytes & 0x7FFF, -1 without buffers. */
s32 Net_LockrockObfuscate(u8 *out, u8 *in, u8 seed, s32 len, s32 salt)
{
    s32 i;
    s32 sum = 0;

    if (out == NULL || in == NULL) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        sum += *in;
        *out++ = *in++ ^ gLockrockKey[i & 7] ^ (salt + (seed + i));
    }
    return sum & 0x7FFF;
}

/* Writes the n lowest decimal digits of v at out (no terminator). */
static s32 Str_FormatDigits(char *out, s32 value, s32 width)
{
    s32 i;

    out += width - 1;
    for (i = 0; i < width; i++) {
        *out-- = value % 10 + '0';
        value /= 10;
    }
    return 0;
}

/* The number in the first n characters (digits only; others are skipped). */
static s32 Str_ParseDigits(s8 *str, s32 len)
{
    s32 value = 0;
    s32 i;

    for (i = 0; i < len; i++, str++) {
        if (*str >= '0' && *str <= '9') {
            value *= 10;
            value += *str - '0';
        }
    }
    return value;
}

/* The reply to a lobby challenge: out = the two 4-digit halves of ch plus key, as two 5-digit
 * numbers (10 characters); -1 without a buffer or a challenge. */
s32 Net_ChallengeReply(char *out, s8 *challenge, u16 key)
{
    s32 half0;
    s32 half1;

    if (out == NULL || challenge == NULL) {
        return -1;
    }
    half0 = key + Str_ParseDigits(challenge, 4);
    half1 = key + Str_ParseDigits(challenge + 4, 4);
    Str_FormatDigits(out, half0, 5);
    Str_FormatDigits(out + 5, half1, 5);
    out[10] = 0;
    return 0;
}

/* This file's initialised data. */
u8 *gLockrockKey = (u8 *)D_0046A848;
