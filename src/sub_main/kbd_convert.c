/*
 * The software keyboard's conversion candidates.
 *
 * - The case candidates (Kbd_MakeCaseCands, CaseCands_*, Wide_*): typed
 *   full-width letters are offered as they are and in their upper-case,
 *   lower-case and capitalised forms (gCaseCands).
 * - The kana-kanji conversion (Kbd_StartConvert, Kbd_Convert): its
 *   candidates are counted (Kbd_CountCands), listed three to a page into
 *   gKbdCandLines (Kbd_ListCandidates), stepped through one by one or a
 *   page at a time, and committed into the text (Kbd_CommitConversion);
 *   Kbd_PrevSegment and Kbd_NextSegment move between the segments being
 *   converted. In the symbol mode the candidates are symbols.
 *
 * The keys' strings that kbd.c's tables point at lie among this file's
 * read-only data (the INCLUDE_RODATA lines).
 */
#include "common.h"
#include "kbd.h"
#include "sound.h"

char *strcat(char *dst, char *src); /* strcat */
s32 strcmp(char *a, char *b); /* strcmp */
char *strcpy(char *dst, const char *src); /* strcpy */
s32 strlen(char *s); /* strlen */
void strncpy(char *dst, const char *src, u32 n);
s32 ConvCmd_PrevCand(char *a, char *b);
s32 ConvCmd_NextCand(char *a, char *b);
s32 ConvCmd_CommitSegment(s32 a, u8 *b, char *c, char *d, s16 *e);
s32 ConvCmd_PrevSegment(char *a, char *b);
s32 ConvCmd_NextSegment(char *a, char *b);
s32 ConvCmd_SegmentForm2(char *a, char *b);
s32 ConvCmd_SegmentForm1(char *a, char *b);
extern u8 gKbdCands[];
extern u8 gKbdCandText[];
extern u8 gKbdSegText[];
extern u8 gKbdCandLines[];
extern s8 gKbdCandLinesCut[];
extern s8 gCaseCands;
extern s8 gCaseCandCount;
void *memset(void *dst, s32 c, u32 n);
/*
 * 0 when every two-byte character of str is a full-width capital, small
 * letter, digit or symbol (0x8141-0x81AC); 1 when str is empty or holds
 * any other character. Characters are read lead byte high.
 */
s32 Str_NotWideAlnum(u8 *str)
{
    s32 bad = 1;
    s32 len;
    s32 i;
    u16 code;

    len = strlen((char *)str);
    if (len == 0) {
        return bad;
    }
    for (i = 0; i < len; i += 2, str += 2) {
        code = (u16)(str[0] << 8) | str[1];
        if ((code >= 0x8260 && code <= 0x8279) || (code >= 0x8281 && code <= 0x829A) || (code >= 0x824F && code <= 0x8258) ||
            (code >= 0x8141 && code <= 0x81AC)) {
            bad = 0;
        } else {
            return 1;
        }
    }
    return bad;
}

s32 memcmp(char *a, char *b, s32 n); /* memcmp */
void *memcpy(void *dst, const void *src, u32 n);

/*
 * Fills gCaseCands (four strings of 256 bytes) with src first, then its
 * full-width upper-case, lower-case and capitalised forms that differ
 * from it (the capitalised one is dropped when it equals the upper-case
 * one); gCaseCandCount is their number. Copies src to out; returns 0.
 */
s32 Kbd_MakeCaseCands(char *src, char *out)
{
    char orig[256];
    char forms[3][256];
    s32 i;
    s32 count;
    char *form;
    s32 len;

    memset(&gCaseCands, 0, 1024);
    memset(orig, 0, 256);
    memset(forms, 0, 768);
    len = strlen(src);
    strncpy(orig, src, len);
    Wide_ToUpper(forms[0], src);
    Wide_ToLower(forms[1], src);
    Wide_Capitalize(forms[2], src);
    if (memcmp(forms[0], forms[2], len) == 0) {
        memset(forms[2], 0, len);
    }
    memcpy(&gCaseCands, orig, len);
    count = 0;
    for (i = 0, form = forms[0]; i < 3; i++, form += 256) {
        if (memcmp(form, orig, len) != 0 && *(u8 *)form != 0) {
            strncpy((char *)(&gCaseCands + (count + 1) * 256), form, len);
            count++;
        }
    }
    gCaseCandCount = count + 1;
    strncpy(out, orig, len);
    return 0;
}

/* Returns gCaseCandCount, the number of case candidates. */
s32 CaseCands_Count(void)
{
    return gCaseCandCount;
}

/*
 * Copies the first count strings of gCaseCands into the text of dst's
 * 257-byte candidate entries (gKbdCands); the id bytes are left as
 * they are. Returns 0.
 */
s32 CaseCands_CopyFirst(s8 *dst, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 257 + 1), (const char *)(&gCaseCands + (i << 8)), 255U);
    }
    return 0;
}

/*
 * Copies count strings of gCaseCands, from start on, into the text of
 * dst's 257-byte candidate entries; returns 0. Kbd_ListCandidates uses
 * it when listing forward.
 */
s32 CaseCands_Copy(s8 *dst, u8 start, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 257 + 1), (const char *)(&gCaseCands + ((start + i) << 8)), 255U);
    }
    return 0;
}

/*
 * The same as CaseCands_Copy; Kbd_ListCandidates uses it when listing
 * backward. Returns 0.
 */
s32 CaseCands_Copy2(s8 *dst, u8 start, u8 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        strncpy((char *)(dst + i * 257 + 1), (const char *)(&gCaseCands + ((start + i) << 8)), 255U);
    }
    return 0;
}

/*
 * Makes the typed text, the first case candidate, the only one: sets
 * gCaseCandCount to 1 and copies gCaseCands' first string into dst.
 * Returns 0.
 */
s32 CaseCands_First(char *dst)
{
    gCaseCandCount = 1;
    strncpy(dst, (const char *) &gCaseCands, 255U);
    return 0;
}

/*
 * Copies strlen(src) bytes of src to dst (no terminator is written) and
 * lowers the second byte by 33 for every two-byte character that is not
 * a full-width capital, digit or symbol, which makes full-width small
 * letters capitals. Returns 0.
 */
s32 Wide_ToUpper(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *cur;
    u16 code;

    len = strlen(src);
    strncpy(dst, src, len);
    i = 0;
    if (i < len) {
        cur = (u8 *)dst;
        do {
            code = (u16)(cur[0] << 8) | cur[1];
            if (!(code >= 0x8260 && code <= 0x8279) && !(code >= 0x824F && code <= 0x8258) && !(code >= 0x8141 && code <= 0x81AC)) {
                dst[i + 1] -= 33;
            }
            i += 2;
            cur += 2;
        } while (i < len);
    }
    return 0;
}

/*
 * The reverse of Wide_ToUpper: raises the second byte by 33 for every
 * two-byte character that is not a full-width small letter, digit or
 * symbol (capitals become small letters). Returns 0.
 */
s32 Wide_ToLower(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *cur;
    u16 code;

    len = strlen(src);
    strncpy(dst, src, len);
    i = 0;
    if (i < len) {
        cur = (u8 *)dst;
        do {
            code = (u16)(cur[0] << 8) | cur[1];
            if (!(code >= 0x8281 && code <= 0x829A) && !(code >= 0x824F && code <= 0x8258) && !(code >= 0x8141 && code <= 0x81AC)) {
                dst[i + 1] += 33;
            }
            i += 2;
            cur += 2;
        } while (i < len);
    }
    return 0;
}

/*
 * Wide_ToLower, then makes the first full-width small letter of dst a
 * capital. Returns 0.
 */
s32 Wide_Capitalize(char *dst, char *src)
{
    s32 len;
    s32 i;
    u8 *cur;
    u16 code;

    Wide_ToLower(dst, src);
    len = strlen(src);
    i = 0;
    if (i < len) {
        cur = (u8 *)dst;
        do {
            code = (u16)(cur[0] << 8) | cur[1];
            if (code >= 0x8281 && code < 0x829B) {
                dst[i + 1] -= 33;
                return 0;
            }
            i += 2;
            cur += 2;
        } while (i < len);
    }
    return 0;
}

/*
 * Clears the conversion state: KbdEntry from candIdx up to usbKey (1300
 * bytes: the candidate fields, kana, wide, convDone, cand and convRest)
 * and the candidate lines gKbdCandLines.
 */
void Kbd_ClearCands(void)
{
    memset(&gKbd->candIdx, 0, 1300);
    memset(gKbdCandLines, 0, KBD_CAND_PAGE * KBD_CAND_LINE_SIZE);
}

/*
 * Brings the converter back to the candidate cand shows on entry: after
 * ConvCmd_SegmentForm2, steps ConvCmd_NextCand (candId takes its result)
 * until cand holds that text again. Stops when candId is 0, and sets
 * candId to 0 when the walk returns to its first candidate without
 * meeting the text (at most 256 steps). Nothing in the symbol mode or
 * for case candidates.
 */
void Kbd_SkipSameCands(void)
{
    char before[256];
    char first[256];
    s32 i;

    if (gKbd->symbolMode != 0) {
        return;
    }
    if (gKbd->caseCands != 0) {
        return;
    }
    strncpy(before, gKbd->cand, 255);
    before[254] = 0;
    ConvCmd_SegmentForm2(gKbd->cand, gKbd->convRest);
    gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
    strncpy(first, gKbd->cand, 255);
    if (strcmp(gKbd->cand, before) == 0) {
        return;
    }
    if (gKbd->candId == 0) {
        return;
    }
    for (i = 0; i < 255; i++) {
        gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
        if (strcmp(gKbd->cand, before) == 0) {
            return;
        }
        if (gKbd->candId == 0) {
            return;
        }
        if (strcmp(gKbd->cand, first) == 0) {
            gKbd->candId = 0;
            return;
        }
    }
}

/*
 * Returns the number of conversion candidates, at least 1. In the symbol
 * mode, 35 for entry kind 8, else 31; for case candidates,
 * CaseCands_Count. Otherwise steps ConvCmd_NextCand into local buffers,
 * counting, until the candidate is cand's text again (a full cycle) or
 * candId is 0, at most 256 steps; when the walk returns to its first
 * candidate without meeting cand's text, the count is 1 and candId 0.
 */
s32 Kbd_CountCands(void)
{
    char before[256];
    char first[256];
    char cand[256];
    char work[256];
    s32 count;
    s32 i;

    if (gKbd->symbolMode != 0) {
        if (gKbd->kind == KBD_KIND_CHECKED) {
            count = 35;
        } else {
            count = 31;
        }
    } else if (gKbd->caseCands != 0) {
        return CaseCands_Count();
    } else {
        strncpy(before, gKbd->cand, 255);
        count = 0;
        gKbd->candId = ConvCmd_NextCand(cand, work);
        strncpy(first, cand, 255);
        count++;
        if (strcmp(cand, before) != 0 && gKbd->candId != 0) {
            for (i = 0; i < 255; i++) {
                gKbd->candId = ConvCmd_NextCand(cand, work);
                count++;
                if (strcmp(cand, before) == 0) {
                    break;
                }
                if (gKbd->candId == 0) {
                    break;
                }
                if (strcmp(cand, first) == 0) {
                    count = 1;
                    gKbd->candId = 0;
                    break;
                }
            }
        }
    }
    if (count == 0) {
        count = 1;
    }
    return count;
}

s32 ConvCmd_Start(char *src, char *a, char *b);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C990);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046C9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CA98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CAA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CAA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/kbd_convert", D_0046CAB8);

/*
 * Starts the conversion. In the symbol mode, kana becomes a space, cand
 * and convRest are emptied and the candidates counted. Otherwise, with
 * no kana pending (and not converting) plays SE_SYS_REFUSE and returns.
 * When the full-width kana is all full-width letters, digits or symbols
 * (Str_NotWideAlnum 0), the candidates are its case forms (caseCands 1,
 * Kbd_MakeCaseCands into cand). Else each typed n or N not followed by
 * y or Y becomes the kana N (gKanaNChars, picked by the pair's second
 * byte less 1), and ConvCmd_Start converts the full-width kana into cand
 * and convRest, its result candCount. Then the shown candidate is kept
 * (Kbd_SkipSameCands), convDone emptied, converting set to 1 and offGrid
 * to 1.
 */
void Kbd_StartConvert(void)
{
    s8 *cur;

    cur = (s8 *)gKbd->kana;
    gKbd->caseCands = 0;
    memset(gKbd->cand, 0, 256);
    memset(gKbd->convRest, 0, 256);
    if (gKbd->symbolMode != 0) {
        strcpy((char *)gKbd->kana, " ");
        gKbd->cand[0] = 0;
        gKbd->convRest[0] = 0;
        gKbd->candCount = Kbd_CountCands();
    } else {
        if (gKbd->converting == 0 && gKbd->kana[0] == 0) {
            gKbd->state = SE_SYS_REFUSE;
            return;
        }
        Str_ToWide((s8 *)gKbd->kana, (s8 *)gKbd->wide);
        if (Str_NotWideAlnum(gKbd->wide) == 0) {
            gKbd->caseCands = 1;
            Kbd_MakeCaseCands((char *)gKbd->wide, gKbd->cand);
        } else {
            for (; *cur != 0; cur += 2) {
                s8 next = cur[2];
                if (*cur == 'N' || *cur == 'n') {
                    if (next != 'Y' && next != 'y') {
                        strncpy((char *)cur, gKanaNChars[cur[1] - 1], 2);
                    }
                }
            }
            Str_ToWide((s8 *)gKbd->kana, (s8 *)gKbd->wide);
            gKbd->candCount = ConvCmd_Start((char *)gKbd->wide, gKbd->cand, gKbd->convRest);
        }
    }
    Kbd_SkipSameCands();
    gKbd->convDone[0] = 0;
    gKbd->converting = 1;
    gKbd->offGrid = 1;
}

/*
 * Outside the symbol mode and case candidates: with more than one
 * candidate, switches the segment to form 1 (ConvCmd_SegmentForm1),
 * keeps the shown candidate, counts and lists the candidates; with one
 * and candId 1, steps to the next candidate (ConvCmd_NextCand), counts
 * and lists them. Seen: Kbd_CommitInput calls it while converting.
 */
void Kbd_NextCandidate(void)
{
    if ((gKbd->symbolMode == 0) && (gKbd->caseCands == 0)) {
        if (gKbd->candCount > 1) {
            ConvCmd_SegmentForm1(gKbd->cand, gKbd->convRest);
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
            return;
        }
        if (gKbd->candId == 1) {
            gKbd->candId = ConvCmd_NextCand(gKbd->cand, gKbd->convRest);
            gKbd->candCount = Kbd_CountCands();
            Kbd_InitCandList();
        }
    }
}

/*
 * For entry kinds 8-10 outside the symbol mode, submits the text
 * (Kbd_Submit). Otherwise SE_SYS_SELECT, starts the conversion
 * (Kbd_StartConvert), counts the candidates and lists them.
 */
void Kbd_Convert(void)
{
    if ((Kbd_KindIs8To10() == 1) && (gKbd->symbolMode == 0)) {
        Kbd_Submit();
        return;
    }
    gKbd->state = SE_SYS_SELECT;
    Kbd_StartConvert();
    gKbd->candCount = Kbd_CountCands();
    Kbd_InitCandList();
}

/*
 * While converting with a candidate in cand: inserts cand into the text
 * at textPos (Str_InsertClip, cut to maxLen), moves textPos past it,
 * clears kana and ends the conversion (converting 0, offGrid 0,
 * kanaPos 0).
 */
void Kbd_CommitConversion(void)
{
    if (gKbd->cand[0] != 0 && gKbd->converting != 0) {
        s32 added = Str_InsertClip(gKbd->text, gKbd->cand,
                              gKbd->textPos, gKbd->maxLen);
        gKbd->textPos = gKbd->textPos + added;
        memset(gKbd->kana, 0, 255);
        gKbd->converting = 0;
        gKbd->offGrid = 0;
        gKbd->kanaPos = 0;
    }
}

/*
 * Copies candidate entry i's text (gKbdCands) into candidate line i
 * (gKbdCandLines, 256 bytes each) when it is not empty. A text longer
 * than 10 bytes is cut there and ends in three dots; the byte after them
 * is also cleared through gKbdCandLinesCut (gKbdCandLines + 13).
 */
void Kbd_SetCandLine(s32 i)
{
    s32 off = i << 8;
    s32 entryOff = off + i;
    u8 *line;

    if ((s8)gKbdCandText[entryOff] != 0) {
        line = gKbdCandLines + off;
        strcpy((char *)line, (char *)gKbdCands + entryOff + 1);
        if ((u32)strlen((char *)gKbdCandLines + off) > 10) {
            strcpy((char *)line + 10, "...");
            gKbdCandLinesCut[off] = 0;
        }
    }
}

/*
 * Sets candidate entry line of gKbdCands to symbol first + line: its id
 * byte is that index, its text gKbdSymbolCands[first + line].
 */
void Kbd_SetSymbolCand(s32 line, s32 first)
{
    s32 symbol = line + first;
    u8 *entry = gKbdCands + line * 257;

    *entry = symbol;
    strcpy((char *)entry + 1, gKbdSymbolCands[symbol]);
}

/*
 * Takes the symbol under the candidate cursor (entry candIdx % 3 of
 * gKbdCands) as cand, and copies it into kana too.
 */
void Kbd_TakeSymbolCand(void)
{
    memset(gKbd->cand, 0, 256);
    strncpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1, 255);
    strncpy((char *)gKbd->kana, gKbd->cand, 255);
}

/*
 * Lists the candidate page that starts at candidate first: clears
 * gKbdCands' entries (65535 bytes) and gKbdCandLines, then fills up to
 * 3 entries (candCount - first when fewer); dir 0 fills them forward,
 * else backward from the last. Symbols come from Kbd_SetSymbolCand (then
 * Kbd_TakeSymbolCand), case candidates from CaseCands_Copy or
 * CaseCands_Copy2, others from ConvCmd_NextCand (backward
 * ConvCmd_PrevCand) into each entry's id byte and text; the converter is
 * then stepped lines - 1 times the other way, so it rests on the page's
 * first entry (backward: its last). Sets the three lines; no sound when
 * there is only one candidate. The local copy of gKbd, through a cast,
 * keeps the game's code's registers.
 */
void Kbd_ListCandidates(s32 first, s32 dir)
{
    char buf[256];
    char rest[256];
    s32 count;
    s32 lines;
    s32 j;
    u8 *entry;
    KbdEntry *kbd;
    s32 i;

    memset(gKbdCands, 0, 65535);
    memset(gKbdCandLines, 0, KBD_CAND_PAGE * KBD_CAND_LINE_SIZE);
    if (dir == 0) {
        kbd = (KbdEntry *)gKbd; /* the cast keeps the original's registers */
        count = kbd->candCount;
        lines = count - first;
        if (count >= first + 3) {
            lines = 3;
        }
        if (kbd->symbolMode != 0) {
            for (j = 0; j < lines; j++) {
                Kbd_SetSymbolCand(j, first);
            }
            Kbd_TakeSymbolCand();
        } else if (kbd->caseCands != 0) {
            CaseCands_Copy((s8 *)gKbdCands, first & 0xFF, lines & 0xFF);
        } else {
            for (j = 0; j < lines; j++) {
                entry = gKbdCands + j * 257;
                *entry = ConvCmd_NextCand((char *)entry + 1, gKbd->convRest);
            }
            for (i = 0; i < lines - 1; i++) {
                ConvCmd_PrevCand(buf, rest);
            }
        }
        for (i = 0; i < 3; i++) {
            Kbd_SetCandLine(i);
        }
    } else {
        kbd = (KbdEntry *)gKbd; /* as above */
        lines = kbd->candCount - first;
        if (kbd->candCount >= first + 3) {
            lines = 3;
        }
        if (kbd->symbolMode != 0) {
            for (lines = lines - 1; lines >= 0; lines--) {
                Kbd_SetSymbolCand(lines, first);
            }
            Kbd_TakeSymbolCand();
        } else if (kbd->caseCands != 0) {
            CaseCands_Copy2((s8 *)gKbdCands, first & 0xFF, lines & 0xFF);
        } else {
            for (i = lines - 1; i >= 0; i--) {
                entry = gKbdCands + i * 257;
                *entry = ConvCmd_PrevCand((char *)entry + 1, gKbd->convRest);
            }
            for (i = 0; i < lines - 1; i++) {
                ConvCmd_NextCand(buf, rest);
            }
        }
        for (i = 2; i >= 0; i--) {
            Kbd_SetCandLine(i);
        }
    }
    if (gKbd->candCount == 1) {
        gKbd->state = KBD_SOUND_NONE;
    }
}

/*
 * Builds the first candidate page: candIdx and candPage 0; nothing more
 * with no candidates. Clears gKbdCands and gKbdCandLines and fills up to
 * 3 entries: the symbols from 0 (then Kbd_TakeSymbolCand), the first
 * case candidates, or cand as entry 0 (its id byte candId when there is
 * more than one) and the next ones from ConvCmd_NextCand, after which
 * the converter is stepped back to entry 0. Then sets the three lines.
 */
void Kbd_InitCandList(void)
{
    char candBuf[256];
    char restBuf[256];
    s32 extra;
    s32 i;

    gKbd->candIdx = 0;
    gKbd->candPage = 0;
    if (gKbd->candCount == 0) {
        return;
    }
    memset(gKbdCands, 0, 65535);
    memset(gKbdCandLines, 0, KBD_CAND_PAGE * KBD_CAND_LINE_SIZE);
    extra = (gKbd->candCount > 3) ? 2 : gKbd->candCount - 1;
    if (gKbd->symbolMode != 0) {
        for (i = 0; i < extra + 1; i++) {
            Kbd_SetSymbolCand(i, 0);
        }
        Kbd_TakeSymbolCand();
    } else if (gKbd->caseCands != 0) {
        CaseCands_CopyFirst((s8 *)gKbdCands, extra + 1);
    } else {
        strncpy((char *)gKbdCandText, gKbd->cand, 255);
        if (gKbd->candCount > 1) {
            gKbdCands[0] = gKbd->candId;
        }
        for (i = 0; i < extra; i++) {
            gKbdCands[(i + 1) * 257] = ConvCmd_NextCand((char *)gKbdCands + (i + 1) * 257 + 1, gKbd->convRest);
        }
        for (i = 0; i < extra; i++) {
            ConvCmd_PrevCand(candBuf, restBuf);
        }
    }
    for (i = 0; i < 3; i++) {
        Kbd_SetCandLine(i);
    }
}

/*
 * Shows the previous conversion candidate (only while converting is 1).
 * candIdx goes down: below 0 it wraps to the last candidate and that
 * page is listed (Kbd_ListCandidates, backward); below the page's start
 * the previous page is listed; else the converter steps back
 * (ConvCmd_PrevCand). cand and candId then come from entry candIdx % 3.
 * counter points at candIdx, then at candPage, through casts that keep
 * the game's code's registers.
 */
void Kbd_PrevCand(void)
{
    char candBuf[256];
    char restBuf[256];
    s32 *counter;

    if (gKbd->converting != 1) {
        return;
    }
    gKbd->candIdx -= 1;
    if (gKbd->candIdx < 0) {
        counter = (s32 *)&gKbd->candIdx; /* the casts keep the original's registers */
        *counter = gKbd->candCount - 1;
        gKbd->candPage = (gKbd->candCount - 1) / 3;
        Kbd_ListCandidates(gKbd->candPage * 3, 1);
    } else {
        counter = (s32 *)&gKbd->candPage;
        if (gKbd->candIdx < *counter * 3) {
            *counter = *counter - 1;
            Kbd_ListCandidates(gKbd->candPage * 3, 1);
        } else {
            ConvCmd_PrevCand(candBuf, restBuf);
        }
    }
    memset(gKbd->cand, 0, 256);
    strcpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1);
    gKbd->candId = gKbdCands[(gKbd->candIdx % 3) * 257];
}

/* Next conversion candidate. */
/*
 * Shows the next conversion candidate (only while converting is 1).
 * candIdx goes up: past the last it wraps to 0 and the first page is
 * listed; past the page's end the next page is listed from candIdx; else
 * the converter steps on (ConvCmd_NextCand). cand and candId then come
 * from entry candIdx % 3. counter points at candIdx, then at candPage,
 * through casts that keep the game's code's registers.
 */
void Kbd_NextCand(void)
{
    char candBuf[256];
    char restBuf[256];
    s32 *counter;

    if (gKbd->converting != 1) {
        return;
    }
    gKbd->candIdx += 1;
    if (gKbd->candIdx > gKbd->candCount - 1) {
        counter = (s32 *)&gKbd->candIdx; /* the casts keep the original's registers */
        *counter = 0;
        gKbd->candPage = 0;
        Kbd_ListCandidates(0, 0);
    } else {
        counter = (s32 *)&gKbd->candPage;
        if (*counter * 3 + 3 <= gKbd->candIdx) {
            *counter = *counter + 1;
            Kbd_ListCandidates(gKbd->candIdx, 0);
        } else {
            ConvCmd_NextCand(candBuf, restBuf);
        }
    }
    memset(gKbd->cand, 0, 256);
    strcpy(gKbd->cand,
                (char *)(gKbdCands + (gKbd->candIdx % 3) * 257) + 1);
    gKbd->candId = gKbdCands[(gKbd->candIdx % 3) * 257];
}

/*
 * Goes to the first candidate of the previous page by Kbd_PrevCand
 * steps; from the first page, to the first candidate of the last page.
 * SE_SYS_SCROLL when candIdx changed. Seen: USB Page Up, pad L2.
 */
void Kbd_PageUp(void)
{
    s32 start;
    s32 steps;
    s32 lastPageCount;

    start = gKbd->candIdx;
    if (start < 3) {
        lastPageCount = gKbd->candCount % 3;
        if (lastPageCount == 0) {
            steps = start % 3 + 3;
        } else {
            steps = start % 3 + lastPageCount;
        }
        for (; steps > 0; steps--) {
            Kbd_PrevCand();
        }
    } else {
        steps = start % 3 + 3;
        for (; steps > 0; steps--) {
            Kbd_PrevCand();
        }
    }
    if (start != gKbd->candIdx) {
        gKbd->state = SE_SYS_SCROLL;
    }
}

/*
 * Goes to the first candidate of the next page by Kbd_NextCand steps.
 * When fewer than 3 candidates remain from candIdx, steps that many,
 * which wraps to candidate 0. SE_SYS_SCROLL when candIdx changed. Seen:
 * USB Page Down, pad R2.
 */
void Kbd_PageDown(void)
{
    s32 start;
    s32 steps;

    start = gKbd->candIdx;
    steps = gKbd->candCount - start;
    if (steps < 3) {
        for (; steps > 0; steps--) {
            Kbd_NextCand();
        }
    } else {
        steps = start % 3;
        for (steps = 3 - steps; steps > 0; steps--) {
            Kbd_NextCand();
        }
    }
    if (start != gKbd->candIdx) {
        gKbd->state = SE_SYS_SCROLL;
    }
}

/*
 * Moves the conversion to the previous segment (while converting is 1,
 * outside the symbol mode); no sound when convDone is empty. Commits the
 * segment (ConvCmd_CommitSegment, its text into gKbdSegText), steps back
 * twice (ConvCmd_PrevSegment, the second result is candCount) and keeps
 * the shown candidate. When that count is not 0, cuts the new segment's
 * text (cand's length) off the end of convDone, counts and lists the
 * candidates, SE_SYS_SCROLL. count is reused for convDone's length.
 */
void Kbd_PrevSegment(void)
{
    s16 len;
    s32 count;

    if (gKbd->converting == 1) {
        if (gKbd->convDone[0] == 0) {
            gKbd->state = KBD_SOUND_NONE;
        } else if (gKbd->symbolMode == 0) {
            ConvCmd_CommitSegment(gKbd->candId, gKbdSegText, gKbd->cand,
                          gKbd->convRest, &len);
            ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            count = ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            gKbd->candCount = count;
            Kbd_SkipSameCands();
            if (count != 0) {
                count = strlen(gKbd->convDone);
                gKbd->convDone[count - strlen(gKbd->cand)] = 0;
                gKbd->candCount = Kbd_CountCands();
                Kbd_InitCandList();
                gKbd->state = SE_SYS_SCROLL;
            }
        }
    }
}

/*
 * Moves the conversion to the next segment (while converting is 1,
 * outside the symbol mode); no sound when convRest is empty. Commits the
 * segment (ConvCmd_CommitSegment, its text into gKbdSegText), steps back
 * once (ConvCmd_PrevSegment) then on (ConvCmd_NextSegment, its result
 * candCount), keeps the shown candidate, counts the candidates, appends
 * gKbdSegText to convDone, lists them, SE_SYS_SCROLL.
 */
void Kbd_NextSegment(void)
{
    s16 len;
    char *yomi;

    if (gKbd->converting == 1) {
        yomi = gKbd->convRest;
        if (*yomi == 0) {
            gKbd->state = KBD_SOUND_NONE;
        } else if (gKbd->symbolMode == 0) {
            ConvCmd_CommitSegment(gKbd->candId, gKbdSegText, gKbd->cand, yomi, &len);
            ConvCmd_PrevSegment(gKbd->cand, gKbd->convRest);
            gKbd->candCount = ConvCmd_NextSegment(gKbd->cand, gKbd->convRest);
            Kbd_SkipSameCands();
            gKbd->candCount = Kbd_CountCands();
            strcat(gKbd->convDone, (char *)gKbdSegText);
            Kbd_InitCandList();
            gKbd->state = SE_SYS_SCROLL;
        }
    }
}
