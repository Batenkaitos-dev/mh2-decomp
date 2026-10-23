/*
 * The lobby's tagged text printer.
 *
 * TagText_Print draws a text written with tags in angle brackets: BODY
 * starts the text, SIZE=n, COLOR=n and C=n set the size and the colour
 * (one hex digit), CENTER, LEFT and RIGHT the alignment, BR and LF=n break
 * the line (LF by n lines) and END ends the text. A backslash starts a new
 * run of text. The message dialog prints its wait texts with it.
 *
 * The text is read a line at a time (TagText_ReadLine) into runs (TagTextRun):
 * each run is a piece of text with the flags of the tags before it. The
 * line is then drawn run by run (TagText_DrawLine), the tags' flags
 * applied first.
 */
#include "common.h"
#include "lobby_state.h"
#include "em.h"
#include "text.h"
#include "lobby_text.h"
#include "font.h"
#include "lobby_util.h"

void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);

/* The tags, in the order of gTagNames: a tag's index is its number, and
 * TagTextRun.flags' bit n - 1 is tag n. CENTER, LEFT and RIGHT are also the
 * alignment's values (gTagTextAlign). */
enum TagTextTag {
    TAG_NONE = 0,       /* no tag: gTagNames' first name never matches a tag of the texts */
    TAG_BODY = 1,       /* the text starts */
    TAG_SIZE = 2,       /* SIZE=n: the text's size, gTagSizeWidths' index */
    TAG_COLOR = 3,      /* COLOR=n: the colour, gTagColors' index */
    TAG_BR = 4,         /* a new line */
    TAG_CENTER = 5,
    TAG_LEFT = 6,
    TAG_RIGHT = 7,
    TAG_END = 8,        /* the text ends */
    TAG_LF = 9,         /* LF=n: n new lines */
    TAG_C = 10,         /* C=n: as COLOR */
    TAG_COUNT = 11
};
#define TAG_FLAG(tag) (1 << ((tag) - 1))
#define TAG_FLAGS_LINE_END (TAG_FLAG(TAG_BR) | TAG_FLAG(TAG_END) | TAG_FLAG(TAG_LF))

/* gTagTextEnd */
#define TAGTEXT_GOING 0         /* more lines to read */
#define TAGTEXT_ENDED 1         /* the text's end, END or too many runs */
#define TAGTEXT_BAD_TAG 2       /* a tag too long or with a Shift-JIS byte: nothing more is drawn */

/* gTagTextState */
#define TAGTEXT_HEAD 0          /* before BODY */
#define TAGTEXT_BODY 1          /* in the text */
#define TAGTEXT_NEW_LINE 2      /* after BR or LF: the next character starts a run */

#define TAG_NAME_SIZE 30        /* gTagTextTagName's bytes */
#define TAG_NAME_MAX 25         /* a longer tag name reads as END */
#define TAG_RUNS_MAX 30         /* runs a line may start; one more ends the text */
#define TAG_LINES_MAX 26
#define TAG_DEFAULT_SIZE 5
#define TAG_DEFAULT_COLOR 7     /* COLOR and C past 15 */
#define TAG_LF_MAX 9
#define TAG_TEXT_SPACING 20     /* Font_SetSpacing's spacing outside the printer */
#define TAG_SCREEN_CENTER_X 320
#define TAG_RIGHT_EDGE 396.0f   /* RIGHT's end, from the left margin */
#define TAG_ESCAPE '\\'         /* starts a run (and is not drawn) */

extern char gTagTextTagName[TAG_NAME_SIZE];   /* the tag name read */
extern char *gTagTextPtr;                     /* the text read */
extern u8 gTagTextEnd;                        /* TAGTEXT_GOING, _ENDED, _BAD_TAG */

/* A run of a line: its text and length, the flags of the tags before it
 * (TAG_FLAG), and the values SIZE, COLOR or C and LF give. */
typedef struct TagTextRun {
    char *text;
    s32 flags;
    u8 colour;
    u8 size;
    u8 lfLines;
    u8 len;
} TagTextRun;
extern TagTextRun gTagTextRuns[50];          /* the line's runs */
extern TagTextRun *gTagTextRun;              /* the run being read, or drawn */
extern TagTextRun *gTagTextReadRun;          /* the last run started: the text read goes to it */
extern u16 gTagTextLineBytes;             /* the line's length in bytes (a Shift-JIS character two) */
extern char *gTagNames[];                 /* the tags' names, by enum TagTextTag */
extern u8 gTagTextLastTag;                /* the last tag read (enum TagTextTag) */
char *strstr(const char *s, const char *w);
extern u8 gTagTextAlign;                  /* TAG_CENTER, TAG_LEFT or TAG_RIGHT */
extern u8 gTagTextState;                  /* TAGTEXT_HEAD, _BODY, _NEW_LINE */
extern s32 gTagTextSize;                  /* the size drawn, gTagSizeWidths' index */
extern s32 D_008FA538;
extern f32 gTagTextTop;
extern f32 gTagTextLeft;
extern f32 gTagTextY;
extern f32 gTagTextX;
extern u32 gTagTextFlags;                 /* the run's flags left to handle */
extern char gTagTextLine[768];          /* a run's text, terminated */
extern s16 gTagSizeWidths[];              /* the sizes' widths */
extern u8 gTagColors[];                   /* the colours, Text_SetColorIndex's */
void *memcpy(void *dst, const void *src, u32 n);

#pragma rodata on
char gTagNameEnd[4] = {
#include "lobby/00846D58.inc"
};

char gTagNameDummy[6] = {
#include "lobby/00846D60.inc"
};

char gTagNameBody[5] = {
#include "lobby/00846D68.inc"
};

char gTagNameSize[5] = {
#include "lobby/00846D70.inc"
};

char gTagNameColor[6] = {
#include "lobby/00846D78.inc"
};

char gTagNameBr[3] = {
#include "lobby/00846D80.inc"
};

char gTagNameCenter[7] = {
#include "lobby/00846D88.inc"
};

char gTagNameLeft[5] = {
#include "lobby/00846D90.inc"
};

char gTagNameRight[6] = {
#include "lobby/00846D98.inc"
};

char gTagNameLf[3] = {
#include "lobby/00846DA0.inc"
};

char gTagNameC[2] = {
#include "lobby/00846DA8.inc"
};
#pragma rodata off

/* Reads a tag's name, up to the '>', from the text (gTagTextPtr) into
 * gTagTextTagName. A name of TAG_NAME_MAX bytes or more, or one with a
 * Shift-JIS character, reads as END and stops the printing
 * (TAGTEXT_BAD_TAG). The loop is written with a goto: it compiles as the
 * game's code does. */
void TagText_ReadTag(void)
{
    char *dst;
    u16 n;
    s32 lead;
    char textChar;
    dst = gTagTextTagName;
    memset(gTagTextTagName, 0, TAG_NAME_SIZE);
    n = 0;
loop:
    textChar = *gTagTextPtr;
    *dst = textChar;
    if (textChar != '>') {
        gTagTextPtr++;
        n++;
        dst++;
        if (n < TAG_NAME_MAX) {
            lead = SJIS_IS_LEAD((u8)*gTagTextPtr);
            if (lead == 0) {
                goto loop;
            }
        }
        memset(gTagTextTagName, 0, TAG_NAME_SIZE);
        strcpy(gTagTextTagName, gTagNameEnd);
        gTagTextEnd = TAGTEXT_BAD_TAG;
        return;
    }
    gTagTextPtr++;
}

/*
 * Handles the tag read into gTagTextTagName: the first of gTagNames found in
 * it is the tag (TAG_NONE if none), kept in gTagTextLastTag. The run being
 * read gets the tag's flag and, for SIZE, COLOR and C, the hex digit after
 * the '=' (past 15: TAG_DEFAULT_SIZE or TAG_DEFAULT_COLOR), for LF its line
 * count (at most TAG_LF_MAX). BODY starts the text; BR and LF start a new
 * line.
 */
void TagText_HandleTag(void)
{
    u8 unusedBuf[8];
    s32 tag;
    char textChar;

    for (tag = 0; tag < TAG_COUNT; tag++) {
        if (strstr(gTagTextTagName, gTagNames[tag]) != NULL) {
            break;
        }
    }
    switch (tag) {
    case TAG_NONE:
        break;
    case TAG_BODY:
        gTagTextRun->flags |= TAG_FLAG(TAG_BODY);
        gTagTextState = TAGTEXT_BODY;
        break;
    case TAG_BR:
        gTagTextRun->flags |= TAG_FLAG(TAG_BR);
        gTagTextState = TAGTEXT_NEW_LINE;
        break;
    case TAG_CENTER:
        gTagTextRun->flags |= TAG_FLAG(TAG_CENTER);
        break;
    case TAG_LEFT:
        gTagTextRun->flags |= TAG_FLAG(TAG_LEFT);
        break;
    case TAG_RIGHT:
        gTagTextRun->flags |= TAG_FLAG(TAG_RIGHT);
        break;
    case TAG_END:
        gTagTextRun->flags |= TAG_FLAG(TAG_END);
        break;
    case TAG_SIZE:
        gTagTextRun->flags |= TAG_FLAG(TAG_SIZE);
        textChar = gTagTextTagName[5]; /* the digit after the name and its = */
        if (textChar >= '0' && textChar <= '9') {
            gTagTextRun->size = textChar - '0';
        } else if (textChar >= 'a' && textChar <= 'f') {
            gTagTextRun->size = textChar - 'a' + 10;
        } else {
            gTagTextRun->size = textChar - 'A' + 10;
        }
        if (gTagTextRun->size > 15) {
            gTagTextRun->size = TAG_DEFAULT_SIZE;
        }
        break;
    case TAG_COLOR:
        memset(unusedBuf, 0, 8);
        gTagTextRun->flags |= TAG_FLAG(TAG_COLOR);
        textChar = gTagTextTagName[6]; /* the digit after the name and its = */
        if (textChar >= '0' && textChar <= '9') {
            gTagTextRun->colour = textChar - '0';
        } else if (textChar >= 'a' && textChar <= 'f') {
            gTagTextRun->colour = textChar - 'a' + 10;
        } else {
            gTagTextRun->colour = textChar - 'A' + 10;
        }
        if (gTagTextRun->colour > 15) {
            gTagTextRun->colour = TAG_DEFAULT_COLOR;
        }
        break;
    case TAG_C:
        memset(unusedBuf, 0, 8);
        gTagTextRun->flags |= TAG_FLAG(TAG_COLOR);
        textChar = gTagTextTagName[2]; /* the digit after the name and its = */
        if (textChar >= '0' && textChar <= '9') {
            gTagTextRun->colour = textChar - '0';
        } else if (textChar >= 'a' && textChar <= 'f') {
            gTagTextRun->colour = textChar - 'a' + 10;
        } else {
            gTagTextRun->colour = textChar - 'A' + 10;
        }
        if (gTagTextRun->colour > 15) {
            gTagTextRun->colour = TAG_DEFAULT_COLOR;
        }
        break;
    case TAG_LF:
        gTagTextRun->flags |= TAG_FLAG(TAG_LF);
        gTagTextRun->lfLines = gTagTextTagName[3] - '0'; /* the digit after the name and its = */
        if (gTagTextRun->lfLines > TAG_LF_MAX) {
            gTagTextRun->lfLines = TAG_LF_MAX;
        }
        gTagTextState = TAGTEXT_NEW_LINE;
        break;
    }
    gTagTextLastTag = tag;
}

/* Reads the tags at the text (gTagTextPtr) while it is at a '<' and no tag
 * has ended the line (BR, END, LF). The exits inside the loop jump to the
 * end, while the loop's own end returns: the compiler lays the two out as
 * the game's code does. */
void TagText_ReadTags(void)
{
    s32 lead;
    u8 textChar;

    if (*gTagTextPtr != TAG_ESCAPE && *gTagTextPtr == '<') {
        do {
            textChar = *gTagTextPtr;
            lead = SJIS_IS_LEAD(textChar);
            if (lead != 0 || textChar != '<') {
                goto end;
            }
            gTagTextPtr++;
            TagText_ReadTag();
            TagText_HandleTag();
        } while (!(gTagTextRun->flags & TAG_FLAGS_LINE_END));
        return;
    }
end:;
}

/*
 * Reads a line of the text into runs (gTagTextRuns). A run starts at each
 * tag (but BR, END and LF, which end the line, END the text too), after a
 * backslash, and at the first character after a new line. The run counts
 * its bytes, and gTagTextLineBytes the line's (a Shift-JIS character two, a
 * newline none). The text's end or a run past TAG_RUNS_MAX ends the text.
 */
void TagText_ReadLine(void)
{
    u8 runs;
    s32 lead;

    memset(gTagTextRuns, 0, sizeof(gTagTextRuns));
    gTagTextLineBytes = 0;
    gTagTextRun = gTagTextRuns;
    gTagTextLastTag = TAG_NONE;
    runs = 0;
    gTagTextReadRun = gTagTextRuns;
    while (1) {
        if (runs > TAG_RUNS_MAX) {
            gTagTextEnd = TAGTEXT_ENDED;
            return;
        }
        lead = SJIS_IS_LEAD((u8)*gTagTextPtr);
        if (lead == 0) {
            if (*gTagTextPtr == 0) {
                gTagTextEnd = TAGTEXT_ENDED;
                return;
            }
            gTagTextLastTag = TAG_NONE;
            TagText_ReadTags();
            if (gTagTextLastTag == TAG_NONE) {
                gTagTextLineBytes++;
                if (*gTagTextPtr == TAG_ESCAPE) {
                    gTagTextPtr++;
                    gTagTextRun->flags = TAG_FLAG(TAG_BODY);
                    runs++;
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
            } else if (gTagTextLastTag == TAG_END || gTagTextLastTag == TAG_BR || gTagTextLastTag == TAG_LF) {
                if (gTagTextEnd != TAGTEXT_BAD_TAG && gTagTextLastTag == TAG_END) {
                    gTagTextEnd = TAGTEXT_ENDED;
                }
                return;
            } else {
                if (*gTagTextPtr == TAG_ESCAPE) {
                    gTagTextPtr++;
                    gTagTextRun->len = 1;
                    gTagTextLineBytes++;
                } else {
                    gTagTextRun->len = 0;
                }
                gTagTextRun->text = gTagTextPtr;
                runs++;
                gTagTextReadRun = gTagTextRun;
                gTagTextRun++;
            }
        } else {
            if (gTagTextState == TAGTEXT_NEW_LINE) {
                gTagTextState = TAGTEXT_BODY;
                gTagTextRun->flags = TAG_FLAG(TAG_BODY);
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

/*
 * Draws the line read into the runs. For each run, its tags' flags in turn
 * (bit n - 1 is tag n): BODY places the line by the alignment, CENTER, LEFT
 * and RIGHT set the alignment, SIZE the size, COLOR the colour, BR and LF
 * go down a line (LF by its count), END stops. Then the run's text, at
 * gTagTextX, gTagTextY with the size's width (gTagSizeWidths); a character
 * is half the width wide. A bad tag draws nothing.
 */
void TagText_DrawLine(void)
{
    s32 i;
    s32 bit;

    if (gTagTextEnd == TAGTEXT_BAD_TAG) {
        return;
    }
    gTagTextRun = gTagTextRuns;
    for (i = 0; i < TAG_RUNS_MAX; i++) {
        if (gTagTextRun->len == 0 && gTagTextRun->flags == 0) {
            return;
        }
        gTagTextFlags = gTagTextRun->flags;
        for (bit = 0; bit < 16; bit++) {  /* the bits of the flags */
            if (gTagTextRun->flags & TAG_FLAG(TAG_SIZE)) {
                gTagTextSize = gTagTextRun->size;
            }
            if (gTagTextFlags & 1) {
                gTagTextFlags >>= 1;
                switch (bit + 1) {
                case TAG_BODY:
                    switch (gTagTextAlign) {
                    case TAG_CENTER:
                        gTagTextX = TAG_SCREEN_CENTER_X - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 4;
                        break;
                    case TAG_LEFT:
                        gTagTextX = gTagTextLeft;
                        break;
                    case TAG_RIGHT:
                        gTagTextX = TAG_RIGHT_EDGE + gTagTextLeft - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 2;
                        break;
                    }
                    break;
                case TAG_BR:
                    gTagTextX = gTagTextLeft;
                    gTagTextY += gTagSizeWidths[gTagTextSize];
                    break;
                case TAG_CENTER:
                    gTagTextAlign = TAG_CENTER;
                    gTagTextX = TAG_SCREEN_CENTER_X - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 4;
                    break;
                case TAG_LEFT:
                    gTagTextAlign = TAG_LEFT;
                    gTagTextX = gTagTextLeft;
                    break;
                case TAG_RIGHT:
                    gTagTextAlign = TAG_RIGHT;
                    gTagTextX = TAG_RIGHT_EDGE + gTagTextLeft - gTagTextLineBytes * gTagSizeWidths[gTagTextSize] / 2;
                    break;
                case TAG_END:
                    return;
                case TAG_SIZE:
                    gTagTextSize = gTagTextRun->size;
                    break;
                case TAG_COLOR:
                case TAG_C:
                    Text_SetColorIndex(gTagColors[gTagTextRun->colour]);
                    break;
                case TAG_LF:
                    gTagTextX = gTagTextLeft;
                    gTagTextY += gTagTextRun->lfLines * gTagSizeWidths[gTagTextSize];
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
            memset(gTagTextLine, 0, sizeof(gTagTextLine));
            memcpy(gTagTextLine, gTagTextRun->text, gTagTextRun->len);
            gTagTextLine[gTagTextRun->len] = 0;
            Font_SetSpacing(gTagSizeWidths[gTagTextSize], gTagSizeWidths[gTagTextSize]);
            Text_PrintAt(gTagTextLine, gTagTextX, gTagTextY);
            gTagTextX += gTagTextRun->len * gTagSizeWidths[gTagTextSize] / 2;
        }
        gTagTextRun++;
    }
}

/* Prints a text with tags at x, y (the left margin and the top), left
 * aligned in TAG_DEFAULT_SIZE and colour 0 to begin with: up to
 * TAG_LINES_MAX lines, each read and drawn, until the text ends. A text
 * that starts with a Shift-JIS character isn't printed. */
void TagText_Print(char *text, f32 x, f32 y)
{
    s32 lead;
    s32 i;
    u8 textChar;

    textChar = *text;
    lead = SJIS_IS_LEAD(textChar);
    if (lead == 0) {
        gTagTextX = x;
        gTagTextLeft = x;
        gTagTextSize = TAG_DEFAULT_SIZE;
        gTagTextAlign = TAG_LEFT;
        gTagTextY = y;
        gTagTextTop = y;
        D_008FA538 = 0;
        gTagTextPtr = text;
        gTagTextState = TAGTEXT_HEAD;
        gTagTextEnd = TAGTEXT_GOING;
        Text_SetColorIndex(0);
        Font_SetSpacing(TAG_TEXT_SPACING, TAG_TEXT_SPACING);
        for (i = 0; i < TAG_LINES_MAX; i++) {
            TagText_ReadLine();
            TagText_DrawLine();
            if (gTagTextEnd != TAGTEXT_GOING) {
                break;
            }
        }
        Text_SetColorIndex(0);
        Font_SetSpacing(TAG_TEXT_SPACING, TAG_TEXT_SPACING);
    }
}

/* The tags' names, by enum TagTextTag, and the colours and the sizes' widths. */
char *gTagNames[12] = {
    gTagNameDummy, gTagNameBody, gTagNameSize, gTagNameColor,
    gTagNameBr, gTagNameCenter, gTagNameLeft, gTagNameRight,
    gTagNameEnd, gTagNameLf, gTagNameC, NULL,
};
u8 gTagColors[16] = {
#include "lobby/008196D0.inc"
};
s16 gTagSizeWidths[16] = {
#include "lobby/008196E0.inc"
};
