/*
 * select: the name check of character creation (0x6B4B30-0x6B51B0). Its
 * .data (0x6C80A0-0x6CAB60) is the list of names that can't be used
 * (16-byte records: the name, then its length at byte 15).
 */
#include "common.h"
#include "m2c_macros.h"
#include "select.h"

extern s8 gCtypeTable[];       /* main: the ctype table (1 + c: 2 lower case, 7 letter or digit) */

s32 strlen(s8 *);      /* strlen */
s32 strncmp(s8 *, s8 *, s32);
void NgWord_Normalize(s8 *dst, s8 *src, s32 n);
s32 NgWord_MatchAt(s8 *p);
s32 NgWord_StartsWithListed(s8 *name, s8 *list);
s32 NgWord_ReadRecord(s8 *rec, s8 *out, s32 max);

/* .data */
u32 gNgWords[2736] = {
#include "select/006C80A0.inc"
};


/* The name check: 0 when the name contains a listed word (compared in upper
 * case; 1 and ! also stand for L, and ( or < are skipped before the first
 * letter matches). The lobby's WordFilter_Scan (unk_793070.c) is its twin.
 * c1 and c2 each serve a loop of their own first (the current character of
 * buf, the first character of a record) and are reused in the word loop (the
 * word's and the name's character): MWCC numbers a reused variable's later
 * values after the loop optimizer's walkers, which gives the game's t2-t5. */
s32 NgWord_Check(s8 *name) {
    s8 buf[0x50];
    s8 *w;
    s32 len;
    s8 word[0x50];
    s32 i;
    s32 found;
    s8 *p;
    s32 n;
    s32 k;
    s32 j;
    s8 c1;
    s8 c2;
    s8 *list;

    list = (s8 *)gNgWords;
    len = strlen(name);
    NgWord_Normalize(buf, name, len);
    i = 0;
    p = buf;
    for (c1 = *p; c1 != 0; c1 = *p) {
        found = 0;
        for (w = list, c2 = *w; c2 != 0; w += 0x10, c2 = *w) {
            n = NgWord_ReadRecord(w, word, len);
            if (n == -1) {
                continue;
            }
            k = 0;
            j = i;
            for (; !(c1 = word[k]) == 0 && (c2 = name[j]) != 0; j++) {
                if (gCtypeTable[buf[j] + 1] & 7) {
                    if (buf[j] != c1) {
                        if (c2 == '1' || c2 == '!') {
                            if (c1 != 'L') {
                                break;
                            }
                        } else if (c2 == '(' || c2 == '<') {
                            if (k != 0) {
                                break;
                            }
                            continue;
                        } else {
                            break;
                        }
                    }
                    k++;
                    if (k == n) {
                        found = 1;
                        j++;
                        break;
                    }
                }
            }
            if (found == 1) {
                if ((gCtypeTable[name[j] + 1] & 7) && NgWord_MatchAt(&buf[j]) == 0) {
                    found = 0;
                    continue;
                }
                return 0;
            }
        }
        while (*p != 0) {
            if (found) {
                break;
            }
            p++;
            if (!(gCtypeTable[name[i++] + 1] & 7) && (gCtypeTable[*p + 1] & 7)) {
                break;
            }
        }
    }
    return 1;
}

void NgWord_Normalize(s8 *dst, s8 *src, s32 n) {
    s32 i;

    for (i = 0; i <= n; i++) {
        dst[i] = src[i];
        if ((gCtypeTable + 1)[src[i]] & 2) {
            dst[i] -= 0x20;
        }
        if (dst[i] == '@') {
            dst[i] = 'A';
        }
        if (dst[i] == '$') {
            dst[i] = 'S';
        }
        if (dst[i] == '5') {
            dst[i] = 'S';
        }
        if (dst[i] == '<') {
            dst[i] = 'C';
        }
        if (dst[i] == '(') {
            dst[i] = 'C';
        }
        if (dst[i] == '!') {
            dst[i] = 'I';
        }
        if (dst[i] == '1') {
            dst[i] = 'I';
        }
        if (dst[i] == '2') {
            dst[i] = 'Z';
        }
        if (dst[i] == '0') {
            dst[i] = 'O';
        }
    }
}

s32 NgWord_MatchAt(s8 *p) {
    s32 ret;

    if (*p == 0) {
        ret = 1;
    } else if (!((gCtypeTable + 1)[*p] & 7)) {
        ret = 1;
    } else {
        ret = NgWord_StartsWithListed(p, (s8 *)gNgWords) != 0;
    }
    return ret;
}

s32 NgWord_StartsWithListed(s8 *name, s8 *list) {
    s8 word[0x60];
    s32 len;
    s32 n;

    len = strlen(name);
    while (*list != 0) {
        n = NgWord_ReadRecord(list, word, len);
        if (n != -1 && strncmp(name, word, n) == 0) {
            return 1;
        }
        list += 0x10;
    }
    return 0;
}

s32 NgWord_ReadRecord(s8 *rec, s8 *out, s32 max) {
    s32 len;
    s32 k;
    s32 n;
    s32 i;

    len = rec[0xF];
    if (max < len) {
        return -1;
    }
    k = 0;
    n = len;
    while (rec[0x1F] == -1) {
        for (i = 0; i < 0xE; i++) {
            out[k * 0xE + i] = rec[i];
        }
        rec += 0x10;
        k++;
        n -= 0xE;
    }
    for (i = 0; i < n; i++) {
        out[k * 0xE + i] = rec[i];
    }
    out[k * 0xE + i] = 0;
    return len;
}
