#include "common.h"
#include "lobby_state.h"
#include "em.h"

/* prototypes */
void Game_ResetSubsystems(void);
void Money_PayPending(u8 silent);
s32 LobbyStep_6501(void);
void Lobby_LoadTextures(void);
void Lobby_Disconnect(s32);
void LobbyEnter_Begin(void);
extern LobbyMenu *gLobbyMenu;
/* end of prototypes */
void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
extern char gTagTextTagName[0x1E];           /* the tag name read */
extern char *gTagTextPtr;                /* the text read */
extern u8 gTagTextEnd;
/* a run of a line: its text, the tags' flags (bit n is tag n + 1 of
 * gTagNames: 1 BODY or a new run, 2 SIZE, 4 COLOR, 8 BR, 0x10 CENTER,
 * 0x20 LEFT, 0x40 RIGHT, 0x80 END, 0x100 LF; C sets COLOR), the colour,
 * the size, LF's line count and the run's length */
typedef struct TextRun {
    char *text;
    s32 flags;
    u8 colour;
    u8 size;
    u8 unkA;
    u8 len;
} TextRun;
extern TextRun gTagTextRuns[50];          /* the line's runs */
extern TextRun *gTagTextRun;             /* the run being read */
extern TextRun *gTagTextReadRun;             /* the last run started */
extern u16 gTagTextLineBytes;                  /* the line's length */
extern char *gTagNames[];              /* the tag names */
extern u8 gTagTextLastTag;                   /* the last tag's index */
char *strstr(const char *s, const char *w);  /* strstr */
void TagText_ReadTag(void);
void TagText_HandleTag(void);
void TagText_ReadLine(void);
void TagText_DrawLine(void);
void Text_SetColorIndex(s16 i);
void Font_SetSpacing(u8 a, u8 b);
extern u8 gTagTextAlign;
extern u8 gTagTextState;
extern s32 gTagTextSize;
extern s32 D_008FA538;
extern f32 gTagTextTop;
extern f32 gTagTextLeft;
extern f32 gTagTextY;
extern f32 gTagTextX;
extern u32 gTagTextFlags;                  /* the run's flags left to handle */
extern char gTagTextLine[0x300];          /* a run's text */
extern s16 gTagSizeWidths[];                /* the sizes' widths */
extern u8 gTagColors[];                 /* the colours */
void Text_PrintAt(char *s, f32 x, f32 y);
void *memcpy(void *dst, const void *src, u32 n);
extern char D_00846D58[];               /* "END" */

void LobbyRoom_Close4(void)
{
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8) (step + 1);
        gLobby->seqStep = 0;
        Game_ResetSubsystems();
        Lobby_LoadTextures();
        *(s8 *)((u8 *)gLobbyMenu + 0x3D) = 1;
        gLobby->connected = 1;
        Money_PayPending(1U);
        break;
    case 1:
        if (LobbyStep_6501() == 1) {
            LobbyEnter_Begin();
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D58);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D60);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D68);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D70);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D78);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D80);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D88);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D90);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846D98);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846DA0);

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7BE430", D_00846DA8);

/* reads a tag name, up to '>', from the text (gTagTextPtr) into gTagTextTagName;
 * a name of 25 bytes or more, or one with a Shift-JIS character, reads as
 * "END" and sets gTagTextEnd to 2 */
void TagText_ReadTag(void)
{
    char *d;
    u16 n;
    s32 lead;
    char c;
    d = gTagTextTagName;
    memset(gTagTextTagName, 0, 0x1E);
    n = 0;
loop:
    c = *gTagTextPtr;
    *d = c;
    if (c != '>') {
        gTagTextPtr++;
        n++;
        d++;
        if (n < 0x19) {
            lead = ((u8)*gTagTextPtr >= 0x80 && (u8)*gTagTextPtr < 0xA0) || ((u8)*gTagTextPtr >= 0xE0 && (u8)*gTagTextPtr < 0x100);
            if (lead == 0) {
                goto loop;
            }
        }
        memset(gTagTextTagName, 0, 0x1E);
        strcpy(gTagTextTagName, D_00846D58);
        gTagTextEnd = 2;
        return;
    }
    gTagTextPtr++;
}

/* handles the tag read into gTagTextTagName: its index in gTagNames (11 names,
 * found anywhere in the tag) goes to gTagTextLastTag; the style (gTagTextRun)
 * gets its flag and, for SIZE, COLOR and the short colour tag, the hex digit
 * after the '=' ("SIZE=n", "COLOR=n", "C=n"), LF its line count; BODY
 * starts the text (gTagTextState = 1), BR and LF a new line (2). The names:
 * DUMMY, BODY, SIZE, COLOR, BR, CENTER, LEFT, RIGHT, END, LF, C */
void TagText_HandleTag(void)
{
    u8 buf[8];
    s32 i;
    char c;

    for (i = 0; i < 0xB; i++) {
        if (strstr(gTagTextTagName, gTagNames[i]) != NULL) {
            break;
        }
    }
    switch (i) {
    case 0:
        break;
    case 1:
        gTagTextRun->flags |= 1;
        gTagTextState = 1;
        break;
    case 4:
        gTagTextRun->flags |= 8;
        gTagTextState = 2;
        break;
    case 5:
        gTagTextRun->flags |= 0x10;
        break;
    case 6:
        gTagTextRun->flags |= 0x20;
        break;
    case 7:
        gTagTextRun->flags |= 0x40;
        break;
    case 8:
        gTagTextRun->flags |= 0x80;
        break;
    case 2:
        gTagTextRun->flags |= 2;
        c = gTagTextTagName[5];
        if (c >= '0' && c <= '9') {
            gTagTextRun->size = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            gTagTextRun->size = c - 'a' + 10;
        } else {
            gTagTextRun->size = c - 'A' + 10;
        }
        if (gTagTextRun->size > 15) {
            gTagTextRun->size = 5;
        }
        break;
    case 3:
        memset(buf, 0, 8);
        gTagTextRun->flags |= 4;
        c = gTagTextTagName[6];
        if (c >= '0' && c <= '9') {
            gTagTextRun->colour = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            gTagTextRun->colour = c - 'a' + 10;
        } else {
            gTagTextRun->colour = c - 'A' + 10;
        }
        if (gTagTextRun->colour > 15) {
            gTagTextRun->colour = 7;
        }
        break;
    case 10:
        memset(buf, 0, 8);
        gTagTextRun->flags |= 4;
        c = gTagTextTagName[2];
        if (c >= '0' && c <= '9') {
            gTagTextRun->colour = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            gTagTextRun->colour = c - 'a' + 10;
        } else {
            gTagTextRun->colour = c - 'A' + 10;
        }
        if (gTagTextRun->colour > 15) {
            gTagTextRun->colour = 7;
        }
        break;
    case 9:
        gTagTextRun->flags |= 0x100;
        gTagTextRun->unkA = gTagTextTagName[3] - '0';
        if (gTagTextRun->unkA > 9) {
            gTagTextRun->unkA = 9;
        }
        gTagTextState = 2;
        break;
    }
    gTagTextLastTag = i;
}

/* reads the tags at the text (gTagTextPtr) while it is at a '<' (not a
 * backslash) and no tag ended the text (gTagTextRun's flags 0x188). The
 * exits inside the loop jump to the end, the loop's own exit returns (a
 * `b` of its own). */
void TagText_ReadTags(void)
{
    s32 lead;
    u8 c;

    if (*gTagTextPtr != 0x5C && *gTagTextPtr == '<') {
        do {
            c = *gTagTextPtr;
            lead = (c >= 0x80 && c < 0xA0) || (c >= 0xE0 && c < 0x100);
            if (lead != 0 || c != '<') {
                goto end;
            }
            gTagTextPtr++;
            TagText_ReadTag();
            TagText_HandleTag();
        } while (!(gTagTextRun->flags & 0x188));
        return;
    }
end:;
}

/* reads a line of the text into runs (gTagTextRuns): a run starts at each tag
 * and after a backslash and counts its bytes (gTagTextLineBytes counts the line's,
 * a Shift-JIS character as two); the text's end or 31 runs end the text
 * (gTagTextEnd = 1), BR, END and LF end the line (END the text too) */
void TagText_ReadLine(void)
{
    u8 n;
    s32 lead;

    memset(gTagTextRuns, 0, 0x258);
    gTagTextLineBytes = 0;
    gTagTextRun = gTagTextRuns;
    gTagTextLastTag = 0;
    n = 0;
    gTagTextReadRun = gTagTextRuns;
    while (1) {
        if (n > 0x1E) {
            gTagTextEnd = 1;
            return;
        }
        lead = ((u8)*gTagTextPtr >= 0x80 && (u8)*gTagTextPtr < 0xA0) || ((u8)*gTagTextPtr >= 0xE0 && (u8)*gTagTextPtr < 0x100);
        if (lead == 0) {
            if (*gTagTextPtr == 0) {
                gTagTextEnd = 1;
                return;
            }
            gTagTextLastTag = 0;
            TagText_ReadTags();
            if (gTagTextLastTag == 0) {
                gTagTextLineBytes++;
                if (*gTagTextPtr == 0x5C) {
                    gTagTextPtr++;
                    gTagTextRun->flags = 1;
                    n++;
                    gTagTextRun->text = gTagTextPtr;
                    gTagTextRun->len = 1;
                    gTagTextReadRun = gTagTextRun;
                    gTagTextRun++;
                } else {
                    if (*gTagTextPtr == '\n') {
                        gTagTextLineBytes--;
                    }
                    gTagTextPtr++;
                    gTagTextReadRun->len++;
                }
            } else if (gTagTextLastTag == 8 || gTagTextLastTag == 4 || gTagTextLastTag == 9) {
                if (gTagTextEnd != 2 && gTagTextLastTag == 8) {
                    gTagTextEnd = 1;
                }
                return;
            } else {
                if (*gTagTextPtr == 0x5C) {
                    gTagTextPtr++;
                    gTagTextRun->len = 1;
                    gTagTextLineBytes++;
                } else {
                    gTagTextRun->len = 0;
                }
                gTagTextRun->text = gTagTextPtr;
                n++;
                gTagTextReadRun = gTagTextRun;
                gTagTextRun++;
            }
        } else {
            if (gTagTextState == 2) {
                gTagTextState = 1;
                gTagTextRun->flags = 1;
                gTagTextRun->text = gTagTextPtr;
                gTagTextRun->len = 0;
                gTagTextReadRun = gTagTextRun;
                gTagTextRun++;
            }
            gTagTextPtr += 2;
            gTagTextReadRun->len += 2;
            gTagTextLineBytes += 2;
        }
    }
}

/* draws the line read into the runs: each run's flags in turn (bit n is tag
 * n + 1: the alignment, the size, the colour, a new line), then its text at
 * gTagTextX, gTagTextY in the size's width (gTagSizeWidths) */
void TagText_DrawLine(void)
{
    s32 i;
    s32 b;

    if (gTagTextEnd == 2) {
        return;
    }
    gTagTextRun = gTagTextRuns;
    for (i = 0; i < 0x1E; i++) {
        if (gTagTextRun->len == 0 && gTagTextRun->flags == 0) {
            return;
        }
        gTagTextFlags = gTagTextRun->flags;
        for (b = 0; b < 0x10; b++) {
            if (gTagTextRun->flags & 2) {
                gTagTextSize = gTagTextRun->size;
            }
            if (gTagTextFlags & 1) {
                gTagTextFlags >>= 1;
                switch (b + 1) {
                case 1:
                    switch (gTagTextAlign) {
                    case 5:
                        gTagTextX = 0x140 - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 4;
                        break;
                    case 6:
                        gTagTextX = gTagTextLeft;
                        break;
                    case 7:
                        gTagTextX = 396.0f + gTagTextLeft - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 2;
                        break;
                    }
                    break;
                case 4:
                    gTagTextX = gTagTextLeft;
                    gTagTextY += gTagSizeWidths[gTagTextSize];
                    break;
                case 5:
                    gTagTextAlign = 5;
                    gTagTextX = 0x140 - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 4;
                    break;
                case 6:
                    gTagTextAlign = 6;
                    gTagTextX = gTagTextLeft;
                    break;
                case 7:
                    gTagTextAlign = 7;
                    gTagTextX = 396.0f + gTagTextLeft - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 2;
                    break;
                case 8:
                    return;
                case 2:
                    gTagTextSize = gTagTextRun->size;
                    break;
                case 3:
                case 10:
                    Text_SetColorIndex(gTagColors[gTagTextRun->colour]);
                    break;
                case 9:
                    gTagTextX = gTagTextLeft;
                    gTagTextY += gTagTextRun->unkA * gTagSizeWidths[gTagTextSize];
                    break;
                }
            } else {
                gTagTextFlags >>= 1;
                if (gTagTextFlags == 0) {
                    break;
                }
            }
        }
        if (gTagTextRun->len != 0 && gTagTextRun->text != NULL) {
            memset(gTagTextLine, 0, 0x300);
            memcpy(gTagTextLine, gTagTextRun->text, gTagTextRun->len);
            gTagTextLine[gTagTextRun->len] = 0;
            Font_SetSpacing(gTagSizeWidths[gTagTextSize], gTagSizeWidths[gTagTextSize]);
            Text_PrintAt(gTagTextLine, gTagTextX, gTagTextY);
            gTagTextX += gTagTextRun->len * gTagSizeWidths[gTagTextSize] / 2;
        }
        gTagTextRun++;
    }
}

/* prints a text with tags at x, y: 26 lines at most, each read
 * (TagText_ReadLine) and drawn (TagText_DrawLine) until the end (gTagTextEnd); not
 * a text that starts with a Shift-JIS character */
void TagText_Print(char *text, f32 x, f32 y)
{
    s32 lead;
    s32 i;
    u8 c;

    c = *text;
    lead = (c >= 0x80 && c < 0xA0) || (c >= 0xE0 && c < 0x100);
    if (lead == 0) {
        gTagTextX = x;
        gTagTextLeft = x;
        gTagTextSize = 5;
        gTagTextAlign = 6;
        gTagTextY = y;
        gTagTextTop = y;
        D_008FA538 = 0;
        gTagTextPtr = text;
        gTagTextState = 0;
        gTagTextEnd = 0;
        Text_SetColorIndex(0);
        Font_SetSpacing(0x14, 0x14);
        for (i = 0; i < 0x1A; i++) {
            TagText_ReadLine();
            TagText_DrawLine();
            if (gTagTextEnd != 0) {
                break;
            }
        }
        Text_SetColorIndex(0);
        Font_SetSpacing(0x14, 0x14);
    }
}

/* unk_7BE430's .data (0x8196A0-0x819700) */
extern u32 D_00846D60[];
extern u32 D_00846D68[];
extern u32 D_00846D70[];
extern u32 D_00846D78[];
extern u32 D_00846D80[];
extern u32 D_00846D88[];
extern u32 D_00846D90[];
extern u32 D_00846D98[];
extern u32 D_00846DA0[];
extern u32 D_00846DA8[];
char *gTagNames[12] = {
    (char *)D_00846D60, (char *)D_00846D68, (char *)D_00846D70, (char *)D_00846D78,
    (char *)D_00846D80, (char *)D_00846D88, (char *)D_00846D90, (char *)D_00846D98,
    D_00846D58, (char *)D_00846DA0, (char *)D_00846DA8, NULL,
};
u8 gTagColors[16] = {
#include "lobby/008196D0.inc"
};
s16 gTagSizeWidths[16] = {
#include "lobby/008196E0.inc"
};
