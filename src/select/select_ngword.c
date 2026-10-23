/*
 * select: the name check of character creation.
 *
 * NgWord_Check (called by select_chara.c when a name is entered) refuses a
 * name that contains a listed word. The list, gNgWords, is the file's
 * .data: NgWordRec records of 16 bytes, a word longer
 * than 14 characters continuing in the next records. The name is compared
 * in upper case, with look-alike characters folded to letters
 * (NgWord_Normalize). The lobby's WordFilter_Scan is the
 * same check for chat.
 */
#include "common.h"
#include "m2c_macros.h"
#include "select.h"

extern s8 gCtypeTable[];       /* main: the ctype table (1 + c: 2 lower case, 7 letter or digit) */

s32 strlen(s8 *);
s32 strncmp(s8 *, s8 *, s32);

/* .data */
NgWordRec gNgWords[684] = {
#include "select/006C80A0.inc"
};


/*
 * The name check: returns 0 when the name contains a listed word, else 1.
 * The search starts at each word of the name (a letter or digit after
 * anything else). The name is compared in upper case; 1 and ! also stand
 * for L, and ( or < are skipped before the first letter matches. A match
 * that runs on into more letters counts only when the rest starts a listed
 * word too (NgWord_MatchAt).
 * wordCh and nameCh serve a loop of their own first (the current character
 * of upper, the first character of a record) before the word loop: the
 * game's register use needs the reuse.
 */
s32 NgWord_Check(s8 *name)
{
    s8 upper[80];
    NgWordRec *rec;
    s32 len;
    s8 word[80];
    s32 start;
    s32 found;
    s8 *cursor;
    s32 wordLen;
    s32 k;
    s32 j;
    s8 wordCh;
    s8 nameCh;
    NgWordRec *list;
    list = gNgWords;
    len = strlen(name);
    NgWord_Normalize(upper, name, len);
    start = 0;
    cursor = upper;
    for (wordCh = *cursor; wordCh != 0; wordCh = *cursor) {
        found = 0;
        for (rec = list, nameCh = rec->text[0]; nameCh != 0; rec++, nameCh = rec->text[0]) {
            wordLen = NgWord_ReadRecord(rec, word, len);
            if (wordLen == -1) {
                continue;
            }
            k = 0;
            j = start;
            for (; !(wordCh = word[k]) == 0 && (nameCh = name[j]) != 0; j++) {
                if (gCtypeTable[upper[j] + 1] & 7) {
                    if (upper[j] != wordCh) {
                        if (nameCh == '1' || nameCh == '!') {
                            if (wordCh != 'L') {
                                break;
                            }
                        } else if (nameCh == '(' || nameCh == '<') {
                            if (k != 0) {
                                break;
                            }
                            continue;
                        } else {
                            break;
                        }
                    }
                    k++;
                    if (k == wordLen) {
                        found = 1;
                        j++;
                        break;
                    }
                }
            }
            if (found == 1) {
                if ((gCtypeTable[name[j] + 1] & 7) && NgWord_MatchAt(&upper[j]) == 0) {
                    found = 0;
                    continue;
                }
                return 0;
            }
        }
        while (*cursor != 0) {
            if (found) {
                break;
            }
            cursor++;
            if (!(gCtypeTable[name[start++] + 1] & 7) && (gCtypeTable[*cursor + 1] & 7)) {
                break;
            }
        }
    }
    return 1;
}

/* Copies the name (n characters and its terminator) in upper case, with
 * look-alikes folded: @ to A, $ and 5 to S, < and ( to C, ! and 1 to I,
 * 2 to Z, 0 to O. */
void NgWord_Normalize(s8 *dst, s8 *src, s32 n)
{
    s32 i;

    for (i = 0; i <= n; i++) {
        dst[i] = src[i];
        if ((gCtypeTable + 1)[src[i]] & 2) {
            dst[i] -= 32;
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

/* Where a match ends inside the name: returns 1 when the text there is at
 * its end, is not a letter or digit, or starts a listed word itself; 0 when
 * the match is only the start of a longer word. */
s32 NgWord_MatchAt(s8 *text)
{
    s32 result;
    if (*text == 0) {
        result = 1;
    } else if (!((gCtypeTable + 1)[*text] & 7)) {
        result = 1;
    } else {
        result = NgWord_StartsWithListed(text, gNgWords) != 0;
    }
    return result;
}

/* 1 when the name starts with a word of the list. */
s32 NgWord_StartsWithListed(s8 *name, NgWordRec *list)
{
    s8 word[96];
    s32 len;
    s32 wordLen;
    len = strlen(name);
    while (list->text[0] != 0) {
        wordLen = NgWord_ReadRecord(list, word, len);
        if (wordLen != -1 && strncmp(name, word, wordLen) == 0) {
            return 1;
        }
        list++;
    }
    return 0;
}

/* Copies the word that starts at rec into out (joining its continuation
 * records) and returns its length; -1, without copying, when it is longer
 * than max. */
s32 NgWord_ReadRecord(NgWordRec *rec, s8 *out, s32 max)
{
    s32 len;
    s32 part;
    s32 left;
    s32 i;
    len = rec->len;
    if (max < len) {
        return -1;
    }
    part = 0;
    left = len;
    while (rec[1].len == -1) {
        for (i = 0; i < NGWORD_PART_LEN; i++) {
            out[part * NGWORD_PART_LEN + i] = rec->text[i];
        }
        rec++;
        part++;
        left -= NGWORD_PART_LEN;
    }
    for (i = 0; i < left; i++) {
        out[part * NGWORD_PART_LEN + i] = rec->text[i];
    }
    out[part * NGWORD_PART_LEN + i] = 0;
    return len;
}
