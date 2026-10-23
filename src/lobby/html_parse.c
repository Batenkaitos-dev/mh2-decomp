/*
 * The lobby browser's HTML parser, its driver and the page's own blocks.
 *
 * - The blocks the portal's pages carry for the game, outside HTML:
 *   <!--MMBB-FLAG-PASS-n--> sections, a <GAME-STYLE> list (the page's
 *   style bits, enum GameStyleKeyword), and comments that open with a page
 *   keyword (PAGE_KW_*: the page's kind, an access key, the kind of <CSV>
 *   block that follows). Html_Csv reads a <CSV> block's quoted items into
 *   the MMBB data (MmbbData, MMBB_FIELD_*), and the lobby servers' rows
 *   into gLobbyServers, which Lbs_Sort sorts by key.
 * - Html_ParseTag, Html_ParseSome and Html_Parse: the tag loop over the
 *   page (gHtmlModeTags: the tag set of each parser mode) and the passes
 *   Html_Parse runs, measuring then drawing. The tags' handlers are the
 *   Html_Tag* functions; the layout is the Layout_* functions.
 * - Html_ResetLoadStep, which sends Html_Parse back to its first step for
 *   the next page, and Html_DecodeEntities, which decodes the character
 *   entities (&lt; ...) in an attribute's text.
 *
 * HtmlState (gHtml, in the browser's work area) is the parser's state;
 * BrowserWork and BrowserState are the browser's.
 */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "hw_regs.h"
#include "render_state.h"
#include "syswork.h"

/* This unit was compiled at optimization level 4 (Html_ParseSome). */
#pragma optimization_level 4

s32 strncmp(const void *a, const void *b, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
u32 strlen(const void *s);
s32 strcmp(const void *a, const void *b); /* strcmp */
s32 atoi(const char *s);               /* atoi */
void Mmbb_SetField(s8 which, s8 idx, char *value); /* unit's own declaration: the definition takes idx as a u8; declared with an s8, Html_Csv passes it the way the game's code does */

u8 *strstr(const void *s, const void *sub); /* strstr */

extern HtmlKeyword gGameStyleKeywords[12]; /* the <GAME-STYLE> keywords */
extern HtmlKeyword gPageCommentKeywords[43]; /* the page keywords in comments */
extern u8 gMmbbPass2Shown;           /* never set: MMBB-FLAG-PASS-2 sections are skipped */
char *strncpy(char *dst, const char *src, u32 n); /* main: strncpy */
void *strcpy(char *dst, const char *src);
s32 Html_ParseColor(const char *s);             /* Html_ParseColor */
void Page_SetStyle(u16 flags, char *msg, u8 mode, u8 scroll); /* unit's own declaration: the definition takes the scroll as an s32; declared with a u8, Html_Parse passes it the way the game's code does */
void Page_ExtendU16(u16 x, u16 y);

extern HtmlTagSet gHtmlModeTags[]; /* the tag set of each mode */
extern u8 gHtmlParseStep;           /* Html_Parse's step */
#define HTML_ENTITY_COUNT 9
extern char gHtmlEntityChars[];  /* '<', '>', '"', '&', ' ', '"', '"', '\'', '\'' */
extern char gHtmlAmpersand[];  /* "&" */

/* The page button's label: Html_Parse copies it into a local from
 * gHtmlLabelInit, the local's initial value. */
typedef struct HtmlLabel {
    char s[7];
} HtmlLabel;
extern HtmlLabel gHtmlLabelInit;

/* The page keywords' names (read-only data): the strings that
 * gPageCommentKeywords' entries point at. */
#pragma rodata on
char gGameStyleKeywords_0[8] = {
#include "lobby/008393E0.inc"
};
char gGameStyleKeywords_1[8] = {
#include "lobby/008393E8.inc"
};
char gGameStyleKeywords_2[8] = {
#include "lobby/008393F0.inc"
};
char gGameStyleKeywords_3[8] = {
#include "lobby/008393F8.inc"
};
char gGameStyleKeywords_4[8] = {
#include "lobby/00839400.inc"
};
char gGameStyleKeywords_5[8] = {
#include "lobby/00839408.inc"
};
char gGameStyleKeywords_6[8] = {
#include "lobby/00839410.inc"
};
char gGameStyleKeywords_7[8] = {
#include "lobby/00839418.inc"
};
char gGameStyleKeywords_8[8] = {
#include "lobby/00839420.inc"
};
char gGameStyleKeywords_9[12] = {
#include "lobby/00839428.inc"
};
char gGameStyleKeywords_10[8] = {
#include "lobby/00839438.inc"
};
char gGameStyleKeywords_11[16] = {
#include "lobby/00839440.inc"
};
char gPageCommentKeywords_0[16] = {
#include "lobby/00839450.inc"
};
char gPageCommentKeywords_1[16] = {
#include "lobby/00839460.inc"
};
char gPageCommentKeywords_2[16] = {
#include "lobby/00839470.inc"
};
char gPageCommentKeywords_3[16] = {
#include "lobby/00839480.inc"
};
char gPageCommentKeywords_4[16] = {
#include "lobby/00839490.inc"
};
char gPageCommentKeywords_5[16] = {
#include "lobby/008394A0.inc"
};
char gPageCommentKeywords_6[16] = {
#include "lobby/008394B0.inc"
};
char gPageCommentKeywords_7[16] = {
#include "lobby/008394C0.inc"
};
char gPageCommentKeywords_8[16] = {
#include "lobby/008394D0.inc"
};
char gPageCommentKeywords_9[16] = {
#include "lobby/008394E0.inc"
};
char gPageCommentKeywords_10[16] = {
#include "lobby/008394F0.inc"
};
char gPageCommentKeywords_11[16] = {
#include "lobby/00839500.inc"
};
char gPageCommentKeywords_12[16] = {
#include "lobby/00839510.inc"
};
char gPageCommentKeywords_14[24] = {
#include "lobby/00839520.inc"
};
u32 gPageCommentKeywords_15[3] = {
#include "lobby/00839538.inc"
};
char gPageCommentKeywords_16[8] = {
#include "lobby/00839548.inc"
};
char gPageCommentKeywords_17[16] = {
#include "lobby/00839550.inc"
};
char gPageCommentKeywords_18[32] = {
#include "lobby/00839560.inc"
};
char gPageCommentKeywords_19[32] = {
#include "lobby/00839580.inc"
};
char gPageCommentKeywords_20[32] = {
#include "lobby/008395A0.inc"
};
char gPageCommentKeywords_21[32] = {
#include "lobby/008395C0.inc"
};
char gPageCommentKeywords_22[32] = {
#include "lobby/008395E0.inc"
};
char gPageCommentKeywords_23[32] = {
#include "lobby/00839600.inc"
};
char gPageCommentKeywords_24[32] = {
#include "lobby/00839620.inc"
};
char gPageCommentKeywords_25[32] = {
#include "lobby/00839640.inc"
};
char gPageCommentKeywords_26[32] = {
#include "lobby/00839660.inc"
};
char gPageCommentKeywords_27[32] = {
#include "lobby/00839680.inc"
};
char gPageCommentKeywords_28[32] = {
#include "lobby/008396A0.inc"
};
char gPageCommentKeywords_29[32] = {
#include "lobby/008396C0.inc"
};
char gPageCommentKeywords_30[32] = {
#include "lobby/008396E0.inc"
};
char gPageCommentKeywords_31[16] = {
#include "lobby/00839700.inc"
};
char gPageCommentKeywords_32[16] = {
#include "lobby/00839710.inc"
};
char gPageCommentKeywords_33[16] = {
#include "lobby/00839720.inc"
};
char gPageCommentKeywords_34[16] = {
#include "lobby/00839730.inc"
};
char gPageCommentKeywords_35[16] = {
#include "lobby/00839740.inc"
};
char gPageCommentKeywords_36[32] = {
#include "lobby/00839750.inc"
};
char gPageCommentKeywords_37[32] = {
#include "lobby/00839770.inc"
};
char gPageCommentKeywords_38[32] = {
#include "lobby/00839790.inc"
};
char gPageCommentKeywords_39[24] = {
#include "lobby/008397B0.inc"
};
char gPageCommentKeywords_40[12] = {
#include "lobby/008397C8.inc"
};
char gPageCommentKeywords_41[32] = {
#include "lobby/008397E0.inc"
};
char gPageCommentKeywords_42[16] = {
#include "lobby/00839800.inc"
};
#pragma rodata off

/* Reads the next "..." from the parse position into dst (at most max - 1
 * characters; newlines dropped when skipNewlines). Returns 0, or 1 at a
 * '<' before the quote, -1 at the end of the page. state: 1 before the
 * opening quote, 2 inside, 3 past max (skipping to the closing quote). */
s32 Html_ReadQuoted(char *dst, s32 max, s32 skipNewlines)
{
    u8 *src;
    s32 state = 1;
    s32 count;
    u8 ch;

    src = gHtml->cur;
    count = 0;

    for (ch = *src; ch != 0; ch = *++src) {
        switch (state) {
        case 1:
            if (ch == '"') {
                state = 2;
            } else if (ch == '<') {
                return 1;
            }
            break;
        case 2:
            if (ch == '"') {
                *dst = 0;
                gHtml->cur = src + 1;
                return 0;
            }
            if (skipNewlines && (ch == '\r' || ch == '\n')) {
                src++;
            }
            *dst++ = *src;
            count++;
            if (max < count + 1) {
                *dst = 0;
                state = 3;
            }
            break;
        case 3:
            if (ch == '"') {
                gHtml->cur = src + 1;
                return 0;
            }
            break;
        }
    }
    return -1;
}

/* The next quoted item, its newlines dropped (Html_ReadQuoted). */
s32 Html_ReadQuotedText(char *dst, s32 max)
{
    return Html_ReadQuoted(dst, max, 1);
}

/* The next quoted item as it is (Html_ReadQuoted). */
s32 Html_ReadQuotedRaw(char *dst, s32 max)
{
    return Html_ReadQuoted(dst, max, 0);
}

/* Html_PageKeyword: the value of the keyword at s (upper-cased, up to '>')
 * in a table of size bytes; 0 when none. */
u8 Html_PageKeyword(u8 *text, HtmlKeyword *tbl, u32 size)
{
    u8 buf[32];
    u16 k;
    u16 count;
    u8 ch;

    for (k = 0; k < 30; k++) {
        ch = text[k];
        if (ch == 0) {
            break;
        }
        if (gCharClass[ch] & HTML_CHAR_SJIS_LEAD) {
            buf[k] = ch;
            k++;
            buf[k] = text[k];
        } else {
            if (ch == '>') {
                break;
            }
            buf[k] = gToUpper[ch];
        }
    }
    buf[k] = 0;
    count = size / HTML_KEYWORD_SIZE;
    for (k = 0; k < count; k++) {
        if (strncmp(buf, tbl[k].name, tbl[k].size - 1) == 0) {
            return tbl[k].value;
        }
    }
    return 0;
}

/* Reads a <GAME-STYLE> list, "KEYWORD=ON" ... (enum GameStyleKeyword):
 * what the page lets the player do (HtmlState.pageStyle, a PAGE_STYLE_*
 * bit per keyword), the page's exit number (EXIT=..:n), the back link
 * (BACK=ON:url, or BACK=ON;url;url... chosen by the exit number), the
 * scroll and front label settings. */
void Html_GameStyle(void)
{
    char buf[272];
    u8 kind;
    u8 *eq;
    u8 turnOn;
    u8 *url;
    s32 i;
    u8 ch;

    gHtml->cur += 12;   /* past "<GAME-STYLE>" */
    for (;;) {
        memset(buf, 0, sizeof(buf));
        if (Html_ReadQuotedText(buf, sizeof(buf)) != 0) {
            return;
        }
        kind = Html_LookupKeyword(buf, gGameStyleKeywords, sizeof(gGameStyleKeywords));
        eq = strstr(buf, "=");
        if (eq == NULL) {
            continue;
        }
        turnOn = 0;
        eq[1] = gToUpper[eq[1]];
        eq[2] = gToUpper[eq[2]];
        if (strncmp(eq + 1, "ON", strlen("ON")) == 0) {
            turnOn = 1;
        }
        switch (kind) {
        case GAME_STYLE_MOUSE:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_POINTER) | turnOn;
            break;
        case GAME_STYLE_SCROLL:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_VSCROLL) | (turnOn << 1);
            gHtml->scroll = 0;
            if (turnOn == 1 && eq[3] == ':') {
                ch = eq[4];
                if (ch >= '0' && ch <= '9') {
                    gHtml->scroll = ch - '0';
                }
            } else if (turnOn == 0 && eq[4] == ':') {
                ch = eq[5];
                if (ch >= '0' && ch <= '9') {
                    gHtml->scroll = ch - '0';
                }
            }
            break;
        case GAME_STYLE_TITLE:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_STATUS_BAR) | (turnOn << 2);
            break;
        case GAME_STYLE_BACK:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_BACK) | (turnOn << 3);
            memset(gHtml->backUrl, 0, sizeof(gHtml->backUrl));
            if (turnOn != 0) {
                if (eq[3] == ':') {
                    memcpy(gHtml->backUrl, eq + 4, 255);
                } else if (eq[3] == ';') {
                    url = eq + 4;
                    for (i = 0; i < gBrowserWork->exitArgs; i++) {
                        eq = strstr(url, ";");
                        if (eq == NULL) {
                            return;
                        }
                        url = eq + 1;
                    }
                    for (i = 0; i < 255; i++) {
                        ch = url[i];
                        if (ch == ';' || ch == 0) {
                            break;
                        }
                        gHtml->backUrl[i] = ch;
                    }
                }
            } else if (eq[4] == ':') {
                memcpy(gHtml->backUrl, eq + 5, 255);
            }
            break;
        case GAME_STYLE_FORWARD:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_FORWARD) | (turnOn << 4);
            break;
        case GAME_STYLE_CANCEL:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_CANCEL) | (turnOn << 5);
            break;
        case GAME_STYLE_RELOAD:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_RELOAD) | (turnOn << 6);
            break;
        case GAME_STYLE_LINK_U:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_UNDERLINE_LINKS) | (turnOn << 7);
            break;
        case GAME_STYLE_X_SHOW:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_MSG_ICON) | (turnOn << 8);
            break;
        case GAME_STYLE_CHOICE_MV:
            gHtml->pageStyle = (gHtml->pageStyle & ~PAGE_STYLE_NO_MSG_ICON) | (turnOn << 9);
            break;
        case GAME_STYLE_EXIT:
            gBrowserWork->exitArgs = 0;
            if (eq[3] == ':') {
                ch = eq[4];
                if (ch >= '0' && ch <= '9') {
                    gBrowserWork->exitArgs = ch - '0';
                }
            }
            break;
        case GAME_STYLE_FRONT_LABEL:
            gHtml->frontLabel = 0;
            if (eq[3] == ':') {
                ch = eq[4];
                if (ch >= '0' && ch <= '9') {
                    gHtml->frontLabel = ch - '0';
                }
            }
            break;
        }
    }
}

/* Reads a lobby server's MMBB_FIELD_SERVER_ROW item, "key,text,value",
 * into its row idx of gLobbyServers (the value "0x..." in hex, else
 * decimal). */
void Lbs_ParseRow(u8 idx, u8 *field)
{
    s32 i;
    s32 len;
    s32 number;
    u8 ch;

    for (i = 0; i < 12; i++) {
        ch = field[i];
        if (ch == ',') {
            break;
        }
        if (ch == 0) {
            return;
        }
        gLobbyServers.row[idx].key[i] = ch;
    }
    if (field[i] != ',') {
        for (; i < 256; i++) {
            ch = field[i];
            if (ch == ',') {
                break;
            }
            if (ch == 0) {
                return;
            }
        }
    }
    i++;
    len = 0;
    while (i < 256 && len < 244) {
        ch = field[i];
        if (gCharClass[ch] & HTML_CHAR_SJIS_LEAD) {
            gLobbyServers.row[idx].text[len++] = ch;
            gLobbyServers.row[idx].text[len++] = field[i + 1];
            i += 2;
        } else if (ch == ',') {
            break;
        } else if (ch == 0) {
            return;
        } else {
            gLobbyServers.row[idx].text[len] = ch;
            i++;
            len++;
        }
    }
    if (field[i] != ',') {
        return;
    }
    i++;
    number = 0;
    len = 0;
    if (field[i] == '0' && (field[i + 1] == 'x' || field[i + 1] == 'X')) {
        for (i += 2; i < 256 || len < 8; i++, len++) {
            number = number * 16 + (gHexDigitValues[field[i]] - 1);
            if (field[i + 1] == 0) {
                break;
            }
        }
    } else {
        for (; i < 256 || len < 11; i++, len++) {
            number *= 10;
            number += gHexDigitValues[field[i]] - 1;
            if (field[i + 1] == 0) {
                break;
            }
        }
    }
    gLobbyServers.row[idx].value = number;
}

/* Swaps n bytes of first and second (at most 272). */
void Mem_Swap(void *first, void *second, s32 n)
{
    u8 scratch[272];

    memcpy(scratch, first, n);
    memcpy(first, second, n);
    memcpy(second, scratch, n);
}

/* Sorts the lobby servers by key: a bubble sort of their order, then
 * the rows and their MMBB data swapped into place. */
void Lbs_Sort(void)
{
    LbsRow swapRow;
    u8 order[48];
    u8 i;
    u8 j;
    u8 k;

    for (i = 0; i < gLobbyServers.count; i++) {
        order[i] = i;
    }
    for (i = 0; i < gLobbyServers.count - 1; i++) {
        for (j = 0; j < gLobbyServers.count - i - 1; j++) {
            if (strcmp(gLobbyServers.row[order[j]].key, gLobbyServers.row[order[j + 1]].key) > 0) {
                k = order[j];
                order[j] = order[j + 1];
                order[j + 1] = k;
            }
        }
    }
    for (i = 0; i < gLobbyServers.count; i++) {
        k = order[i];
        swapRow = gLobbyServers.row[i];
        gLobbyServers.row[i] = gLobbyServers.row[k];
        gLobbyServers.row[k] = swapRow;
        Mem_Swap(gMmbbData->serverAddr[i], gMmbbData->serverAddr[k], sizeof(gMmbbData->serverAddr[0]));
        Mem_Swap(gMmbbData->serverUsers[i], gMmbbData->serverUsers[k], sizeof(gMmbbData->serverUsers[0]));
        Mem_Swap(gMmbbData->serverCapacity[i], gMmbbData->serverCapacity[k], sizeof(gMmbbData->serverCapacity[0]));
        Mem_Swap(gMmbbData->serverRow[i], gMmbbData->serverRow[k], sizeof(gMmbbData->serverRow[0]));
        Mem_Swap(gMmbbData->f9[i], gMmbbData->f9[k], sizeof(gMmbbData->f9[0]));
        for (j = i + 1; j < gLobbyServers.count; j++) {
            if (order[j] == i) {
                order[j] = order[i];
            }
        }
    }
}

/* The longest item of a <CSV> block Html_Csv reads (with its terminator). */
#define CSV_ITEM_LEN 256

/* Reads the page's <CSV> block: the quoted items after it, by the block's
 * kind (CSV_KIND_*, HtmlState.csvKind), into the MMBB data's fields (and,
 * for the lobby servers' page, their rows; at most 40 servers). Fields 4,
 * 7, 9 and 14-19 are kept by number: no use of them shows what they are. */
void Html_Csv(u8 kind)
{
    char buf[784];
    u8 entries;

    gHtml->cur += 5;    /* past "<CSV>" */
    switch (kind) {
    case CSV_KIND_SESSION:
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_RESULT, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_CONNECTION_ID, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(4, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_NEXT_URL, 0, buf);
        break;
    case CSV_KIND_ACCOUNT:
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_RESULT, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_USER_ID, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_CONNECTION_ID, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(4, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_NEXT_URL, 0, buf);
        break;
    case CSV_KIND_SERVERS:
        memset(gLobbyServers.row, 0, sizeof(gLobbyServers.row));
        gLobbyServers.count = 0;
        gHtml->sortServers = 1;
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_RESULT, 0, buf);
        if (strncmp(buf, "OK", 2) == 0) {
            entries = 0;
            while (Html_ReadQuotedText(buf, CSV_ITEM_LEN) == 0 && entries < 40) {
                entries++;
                Mmbb_SetField(7, entries - 1, buf);
                if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                    break;
                }
                Mmbb_SetField(MMBB_FIELD_SERVER_USERS, entries - 1, buf);
                gLobbyServers.row[entries - 1].users = atoi(buf);
                if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                    break;
                }
                Mmbb_SetField(MMBB_FIELD_SERVER_CAPACITY, entries - 1, buf);
                gLobbyServers.row[entries - 1].capacity = atoi(buf);
                if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                    break;
                }
                Mmbb_SetField(MMBB_FIELD_SERVER_ROW, entries - 1, buf);
                Lbs_ParseRow(entries - 1, (u8 *)buf);
                if (Html_ReadQuotedRaw(buf, CSV_ITEM_LEN) != 0) {
                    break;
                }
                Mmbb_SetField(9, entries - 1, buf);
                if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                    break;
                }
            }
            gLobbyServers.count = entries;
        } else {
            if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                break;
            }
            Mmbb_SetField(MMBB_FIELD_ERROR_TEXT, 0, buf);
        }
        break;
    case CSV_KIND_RESULT:
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_RESULT, 0, buf);
        if (strncmp(buf, "OK", 2) == 0) {
            break;
        }
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_ERROR_TEXT, 0, buf);
        break;
    case CSV_KIND_MENU:
        if (gHtml->csvArmed != 0) {
            gHtml->csvArmed = 0;
            Page_SetDataMode(kind);
            if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                break;
            }
            Mmbb_SetField(14, 0, buf);
            if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                break;
            }
            Mmbb_SetField(15, 0, buf);
            return;
        }
        Page_SetDataMode(kind + 1);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(16, 0, buf);
        if (strncmp(buf, "00", 2) == 0) {
            break;
        }
        entries = 0;
        while (Html_ReadQuotedText(buf, CSV_ITEM_LEN) == 0 && entries < 10) {
            entries++;
            Mmbb_SetField(17, entries - 1, buf);
            if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                break;
            }
            Mmbb_SetField(18, entries - 1, buf);
            if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
                break;
            }
            Mmbb_SetField(19, entries - 1, buf);
        }
        break;
    case CSV_KIND_PORTAL:
        if (gPortalPage == PORTAL_PAGE_NEXT) {
            Page_SetDataMode(CSV_KIND_RESULT);
        }
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_RESULT, 0, buf);
        if (Html_ReadQuotedText(buf, CSV_ITEM_LEN) != 0) {
            break;
        }
        Mmbb_SetField(MMBB_FIELD_ERROR_TEXT, 0, buf);
        break;
    }
    gHtml->csvKind = CSV_KIND_NONE;
}

/* Html_ParseTag: at a '<' or '&' (or inside a tag), looks the upper-cased
 * text up in the tag set: moves *cur past the tag's name, sets the mode
 * (or the comment flag) and calls the tag's handler. 0 when a tag was
 * handled (or an unknown tag skipped), 1 when the text is not a tag, -1
 * on an error. */
s32 Html_ParseTag(u8 **cur, HtmlTagSet *set, u8 *text)
{
    u8 name[16];
    HtmlTag *tag;
    s32 i;
    s32 k;
    u8 *end;

    switch (gHtml->parseState) {
    case 0:
        if (**cur != '<' && **cur != '&') {
            break;
        }
        gHtml->parseState = 1;
    case 1:
        tag = set->tags;
        for (k = 0; k < 13 && (*cur)[k] != 0; k++) {
            name[k] = gToUpper[(*cur)[k]];
        }
        for (i = 0; i < set->count; i++, tag++) {
            if (strncmp(name, tag->name, tag->size - 1) == 0) {
                gHtml->tagCount++;
                if (tag->next != 0) {
                    gHtml->mode = tag->next;
                } else {
                    gHtml->inComment = 1;
                }
                *cur += tag->size - 1;
                if (tag->handler(cur, text) < 0) {
                    return -1;
                }
                {
                    HtmlState *html = gHtml;
                    s32 nextMode = html->mode;

                    if (nextMode == 2 || nextMode == 18) { /* the body, and mode 18 */
                        html->parseState = 0;
                    } else {
                        html->parseState = 2;
                    }
                }
                return 0;
            }
        }
        gHtml->parseState = 0;
        if (i == set->count && **cur != '&') {
            end = strstr(*cur, ">");
            if (end != NULL) {
                *cur = end + 1;
                return 0;
            }
            return -1;
        }
        break;
    default:
        tag = set->tags;
        for (k = 0; k < 13 && (*cur)[k] != 0; k++) {
            name[k] = gToUpper[(*cur)[k]];
        }
        for (i = 0; i < set->count; i++, tag++) {
            if (strncmp(name, tag->name, tag->size - 1) == 0) {
                if (tag->next != 0) {
                    gHtml->mode = tag->next;
                } else {
                    gHtml->inComment = 1;
                }
                *cur += tag->size - 1;
                if (tag->handler(cur, text) < 0) {
                    return -1;
                }
                if (gHtml->mode == 2) {
                    gHtml->parseState = 0;
                }
                return 0;
            }
        }
        break;
    }
    return 1;
}

/* Reads a comment of the page. <!--MMBB-FLAG-PASS-n-->: with 2 (and
 * gMmbbPass2Shown clear, as it always is) the page skips to
 * <!--/MMBB-FLAG-PASS-->. On the second measuring pass (measure 1), a
 * <GAME-STYLE> before the comment's end; on the drawing pass, the page
 * keyword at the comment's start (PAGE_KW_*: the page's kind in pageKind,
 * an access key in accessKey, the <CSV> kind in csvKind) and, with a
 * <CSV> kind, a <CSV> block before the comment's end. */
void Html_Comment(void)
{
    char id[16];
    s32 skip = 0;
    u8 *pos;
    u8 keyword;
    s32 ch;

    pos = gHtml->cur;
    if (*pos == 'M' && strncmp(pos, "MMBB-FLAG-PASS", 14) == 0) {
        gHtml->cur += 14;   /* past "MMBB-FLAG-PASS" */
        pos = gHtml->cur;
        switch (pos[1]) {
        case '1':
            skip = 0;
            break;
        case '2':
            if (gMmbbPass2Shown != 0) {
                skip = 0;
            } else {
                skip = 1;
            }
            break;
        case '3':
            skip = 0;
            break;
        }
        if (skip) {
            pos = strstr(pos, "<!--/MMBB-FLAG-PASS-->");
            if (pos != NULL) {
                gHtml->cur = pos + 19;  /* at the "-->" that ends the comment */
            }
        }
    } else if (gHtml->measure == HTML_PASS_LAYOUT) {
        while ((ch = *(pos = gHtml->cur)) != 0) {
            if (ch == '-') {
                if (strncmp(pos, "-->", 3) == 0) {
                    return;
                }
            } else if (ch == '<') {
                if (strncmp(pos, "<GAME-STYLE>", 12) == 0) {
                    Html_GameStyle();
                }
                break;
            }
            gHtml->cur++;
        }
    } else if (gHtml->measure == HTML_PASS_DRAW) {
        keyword = Html_PageKeyword(gHtml->cur, gPageCommentKeywords, sizeof(gPageCommentKeywords));
        switch (keyword) {
        case PAGE_KW_TOP_SIGNUP:
        case 2:
        case PAGE_KW_COMP_SIGNUP:
            gHtml->pageButton = 1;
        case PAGE_KW_INP_SIGNUP:
        case 5:
        case PAGE_KW_INP_PWCHANGE:
        case PAGE_KW_CMP_PWCHANGE:
        case PAGE_KW_CRS_TOP:
        case PAGE_KW_COMP_LOGIN:
        case PAGE_KW_KEICAN_MMBB:
        case PAGE_KW_CANCAN_MMBB:
            gHtml->pageKind = keyword;
            break;
        case 8:     /* the access keys */
        case 10:
        case 11:
        case 40:
        case 44:
        case 45:
        case 46:
        case 47:
            gHtml->accessKey = keyword;
            break;
        case PAGE_KW_LOGIN_ID:
            if (gHtml->pageKind == PAGE_KW_COMP_SIGNUP) {
                gHtml->accessKey = keyword;
                strncpy(id, (char *)gHtml->cur + gPageCommentKeywords[8].size + 2, 10);
                id[10] = 0;
                Form_SetLoginId((u8 *) id);
            }
            break;
        case PAGE_KW_PAGE_BUTTON:
            gHtml->pageButton = 1;
            break;
        case PAGE_KW_CSV_SESSION:
        case PAGE_KW_CSV_ACCOUNT:
        case PAGE_KW_CSV_SERVERS:
        case PAGE_KW_CSV_RESULT:
            gHtml->csvKind = keyword - (PAGE_KW_CSV_SESSION - CSV_KIND_SESSION);
            break;
        case PAGE_KW_CRS_MENU:
            gHtml->csvKind = CSV_KIND_MENU;
            gHtml->pageKind = keyword;
            break;
        case 20:  /* 20-32: nothing */
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
            break;
        case PAGE_KW_CSV_PORTAL:
            gHtml->csvKind = CSV_KIND_PORTAL;
            gHtml->pageKind = keyword;
            break;
        case PAGE_KW_CSV_ADD_USER:
            if (gHtml->csvKind == CSV_KIND_SESSION) {
                gHtml->csvKind = CSV_KIND_ACCOUNT;
            }
            break;
        case PAGE_KW_CSV_ARM:
            gHtml->csvArmed = 1;
            break;
        case PAGE_KW_NONE:
            if (gHtml->csvKind == CSV_KIND_NONE) {
                break;
            }
            while ((ch = *(pos = gHtml->cur)) != 0) {
                if (ch == '-') {
                    if (strncmp(pos, "-->", 3) == 0) {
                        return;
                    }
                } else if (ch == '<') {
                    if (strncmp(pos, "<CSV>", 5) == 0) {
                        Html_Csv(gHtml->csvKind);
                    }
                    break;
                }
                gHtml->cur++;
            }
            break;
        }
    }
}

/* Html_Reset: the parser's and layout's state for a pass over the page
 * (measure: -10 the first, measuring pass, 1 the second, 0 the drawing
 * pass). */
void Html_Reset(s8 measure)
{
    gHtml->parseState = 0;
    gHtml->measure = measure;
    gHtml->lineNo = 0;
    gHtml->link = 0;
    gHtml->mode = 0;
    gHtml->inComment = 0;
    gHtml->textLen = 0;
    gHtml->text[0] = 0;
    gHtml->pen.x = 0;
    gHtml->pen.runX = 0;
    gHtml->pen.y = 0;
    gHtml->pen.runY = 0;
    gHtml->selectWidth = 0;
    gHtml->pen.runW = 0;
    gHtml->pen.lineUsed = 0;
    gHtml->lineLen = 0;
    gHtml->cellCount = 0;
    gHtml->tableDepth = 0;
    gHtml->curCell = 0;
    gHtml->unkD896 = 0;
    gHtml->flags = 0;
    gHtml->pre = 0;
    gHtml->paragraph = 0;
    gHtml->center = 0;
    gHtml->bold = 0;
    gHtml->underline = 0;
    gHtml->strike = 0;
    gHtml->indent = 0;
    strcpy(gHtml->textColor, "#000000");
    strcpy(gHtml->linkColor, "#0000FF");
    strcpy(gHtml->bgColor, "#FFFFFF");
    strcpy(gHtml->background, "");
    gHtml->fontDepth = 0;
    gHtml->fontSize[gHtml->fontDepth] = 3;
    gHtml->fontColor[gHtml->fontDepth] = Html_ParseColor(gHtml->textColor);
    gHtml->bgColorValue = Html_ParseColor(gHtml->bgColor);
    gHtml->linkColorValue = Html_ParseColor(gHtml->linkColor);
    Html_SetCharWidths(gHtml->bold);
    memset(&gHtml->meta, 0, sizeof(gHtml->meta));
    gHtml->pageKind = 0;
    gHtml->accessKey = 0;
    gHtml->pageButton = 0;
    gHtml->csvKind = 0;
    gHtml->csvArmed = 0;
    memset(gHtml->mapName, 0, sizeof(gHtml->mapName));
    gHtml->sortServers = 0;
}

/* Runs the parser from cur until the page's end (returns 0) or until
 * maxTags tags were handled (1); -1 on an error. Text between tags (in
 * <PRE> with its tabs and newlines) collects in text[], which is laid out
 * when it nears its 128 bytes. */
s32 Html_ParseSome(u16 maxTags)
{
    HtmlTagSet *set;
    s32 kind;
    s32 result;
    u8 ch;

    gHtml->tagCount = 0;
    while (*gHtml->cur != 0) {
        if (gHtml->inComment) {
            Html_Comment();
            gHtml->commentEnd = (char *)strstr(gHtml->cur, "-->");
            if (gHtml->commentEnd != NULL) {
                gHtml->inComment = 0;
                gHtml->cur = (u8 *)gHtml->commentEnd + 3;
            } else {
                gHtml->cur += strlen((char *)gHtml->cur);
            }
        } else {
            set = &gHtmlModeTags[gHtml->mode];
            kind = set->collectText;
            result = Html_ParseTag(&gHtml->cur, set, gHtml->text);
            if (result != 0) {
                if (result < 0) {
                    Html_CloseTables(&gHtml->cur, gHtml->text);
                    return -1;
                }
                if (kind == 1) {
                    ch = *gHtml->cur;
                    if (ch != '\r') {
                        if (gHtml->pre) {
                            if (ch == '\n') {
                                Layout_FlushBreak(gHtml->text);
                            } else if (ch == '\t') {
                                /* a tab: four HTML_CHAR_NBSP spaces, if text[] has room */
                                if (gHtml->textLen > HTML_TEXT_FULL - 3) {
                                    Layout_TextFull(gHtml->text);
                                }
                                gHtml->text[gHtml->textLen++] = HTML_CHAR_NBSP;
                                gHtml->text[gHtml->textLen++] = HTML_CHAR_NBSP;
                                gHtml->text[gHtml->textLen++] = HTML_CHAR_NBSP;
                                gHtml->text[gHtml->textLen++] = HTML_CHAR_NBSP;
                                gHtml->text[gHtml->textLen] = 0;
                            } else {
                                if (gHtml->textLen > HTML_TEXT_FULL) {
                                    Layout_TextFull(gHtml->text);
                                }
                                gHtml->text[gHtml->textLen++] = *gHtml->cur;
                                gHtml->text[gHtml->textLen] = 0;
                            }
                        } else if (ch != '\n') {
                            if (gHtml->textLen > HTML_TEXT_FULL) {
                                Layout_TextFull(gHtml->text);
                            }
                            gHtml->text[gHtml->textLen++] = *gHtml->cur;
                            gHtml->text[gHtml->textLen] = 0;
                        }
                    }
                }
                gHtml->cur++;
            }
        }
        if (gHtml->tagCount >= maxTags) {
            return 1;
        }
    }
    Html_CloseTables(&gHtml->cur, gHtml->text);
    return 0;
}

/* Lays out and draws the page in steps (gHtmlParseStep: 0 start, 1 the
 * measuring pass, 2 the second pass, 3 the page's frame, 4 the drawing
 * pass), at most maxTags tags per call. Returns 0 while measuring, 1
 * while drawing, 2 when done (or with no page). At the end of the drawing
 * pass it draws the page button, gives the page's kind (Page_SetKind) and
 * sorts the lobby servers the page listed. */
s32 Html_Parse(u8 *page, u16 maxTags, u8 inFrame)
{
    HtmlLabel label;
    u16 extra;
    u16 labelWidth;

    if (page == NULL) {
        return 2;
    }
    switch (gHtmlParseStep) {
    case 0:
        gHtml = (HtmlState *)gBrowserWork->buf64k;
        memset(gHtml, 0, sizeof(HtmlState));
        gHtml->pageWidth = PAGE_VIEW_W;   /* the narrowest page */
        gHtml->width = PAGE_VIEW_W;
        gHtml->cur = page;
        Html_Reset(HTML_PASS_MEASURE);
        gHtml->inFrame = inFrame;
        gHtml->pageStyle = HTML_PAGE_STYLE_DEFAULT;
        gHtmlParseStep = 1;
    case 1:
        if (Html_ParseSome(maxTags) > 0) {
            return 0;
        }
        if (gHtml->pageWidth < PAGE_VIEW_W) {
            gHtml->pageWidth = PAGE_VIEW_W;
        }
        memset(gHtml->lines, 0, sizeof(gHtml->lines));
        gHtml->bottom = 380;      /* 380: the page's least height */
        gHtml->width = gHtml->pageWidth;
        gHtmlParseStep = 2;
        gHtml->cur = page;
        Html_Reset(HTML_PASS_LAYOUT);
    case 2:
        if (Html_ParseSome(maxTags) > 0) {
            return 0;
        }
        gHtmlParseStep = 3;
        gHtml->cur = page;
        Html_Reset(HTML_PASS_DRAW);
        return 1;
    case 3:
        if (gHtml->inFrame == 0) {
            Html_NewPage();
            Page_SetStyle(gHtml->pageStyle, (char *)gHtml->backUrl, gHtml->frontLabel, gHtml->scroll);
        }
        gHtmlParseStep = 4;
    case 4:
        label = gHtmlLabelInit;
        extra = 0;
        if (Html_ParseSome(maxTags) > 0) {
            return 1;
        }
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->pageButton != 0 && gHtml->inFrame == 0) {
            labelWidth = strlen(label.s) * 8 + 8;
            Draw_PageButton(16, gHtml->bottom, labelWidth + 16, gHtml->bottom + 20, (u8 *)label.s);
            extra = 40;
        }
        if (gHtml->inFrame == 0) {
            Page_ExtendU16(gHtml->width, gHtml->bottom + extra);
        }
        if (gHtml->measure == HTML_PASS_DRAW) {
            switch (gHtml->pageKind) {
            case PAGE_KW_CRS_TOP:
                Page_SetKind(PAGE_KIND_TOP);
                break;
            case PAGE_KW_CRS_MENU:
                Page_SetKind(PAGE_KIND_MENU);
                break;
            case PAGE_KW_INP_SIGNUP:
                Page_SetKind(PAGE_KIND_SIGNUP_FORM);
                break;
            case PAGE_KW_COMP_SIGNUP:
                Page_SetKind(PAGE_KIND_SIGNUP_DONE);
                break;
            case PAGE_KW_INP_PWCHANGE:
                Page_SetKind(PAGE_KIND_PASSWORD_FORM);
                break;
            case PAGE_KW_CMP_PWCHANGE:
                Page_SetKind(PAGE_KIND_PASSWORD_DONE);
                break;
            case PAGE_KW_TOP_SIGNUP:
                Page_SetKind(PAGE_KIND_SIGNUP_TOP);
                break;
            case PAGE_KW_COMP_LOGIN:
                Page_SetKind(PAGE_KIND_LOGIN_DONE);
                break;
            case PAGE_KW_KEICAN_MMBB:
                Page_SetKind(PAGE_KIND_MMBB_KEICAN);
                break;
            case PAGE_KW_CANCAN_MMBB:
                Page_SetKind(PAGE_KIND_MMBB_CANCAN);
                break;
            }
            if (gHtml->sortServers != 0) {
                Lbs_Sort();
            }
        }
        break;
    }
    return 2;
}

/* Sends Html_Parse back to its first step for the next page. */
void Html_ResetLoadStep(void)
{
    gHtmlParseStep = 0;
}

/* Replaces the character entities of s in place (&lt; &gt; &quot; &amp;
 * &nbsp; &#34; &#39;): <IMG ALT=...> and <INPUT ALT=...>'s text. */
void Html_DecodeEntities(char *str)
{
    char *src;
    char *entity;
    s32 i;
    char **name;
    char *next;
    char *amp;

    if ((amp = (char *)strstr(str, gHtmlAmpersand)) != NULL) {
        do {
            next = amp + 1;
            for (i = 0, name = gHtmlEntityNames; i < HTML_ENTITY_COUNT; i++, name++) {
                entity = *name;
                if (strncmp(amp + 1, entity, strlen(entity)) == 0) {
                    *amp = gHtmlEntityChars[i];
                    src = amp + strlen(entity);
                    do {
                        src++;
                        amp++;
                        *amp = *src;
                    } while (*src != 0);
                    break;
                }
            }
        } while ((amp = (char *)strstr(next, gHtmlAmpersand)) != NULL);
    }
}

/* The names and tag handlers the tables below point at, then the
 * primitives' packet templates, the keyword tables and the tag sets of
 * each parser mode. */
extern char gGameStyleKeywords_0[];
extern char gGameStyleKeywords_1[];
extern char gGameStyleKeywords_2[];
extern char gGameStyleKeywords_3[];
extern char gGameStyleKeywords_4[];
extern char gGameStyleKeywords_5[];
extern char gGameStyleKeywords_6[];
extern char gGameStyleKeywords_7[];
extern char gGameStyleKeywords_8[];
extern char gGameStyleKeywords_9[];
extern char gGameStyleKeywords_10[];
extern char gGameStyleKeywords_11[];
extern char gPageCommentKeywords_1[];
extern char gPageCommentKeywords_2[];
extern char gPageCommentKeywords_3[];
extern char gPageCommentKeywords_4[];
extern char gPageCommentKeywords_5[];
extern char gPageCommentKeywords_6[];
extern char gPageCommentKeywords_7[];
extern char gPageCommentKeywords_8[];
extern char D_00839908[];
extern char gHtmlModeTags_0_1[];
extern char gHtmlModeTags_0_2[];
extern char gHtmlModeTags_0_3[];
extern char gHtmlModeTags_0_4[];
extern char gHtmlModeTags_0_5[];
extern char gHtmlModeTags_0_6[];
extern char gHtmlModeTags_0_7[];
extern char gHtmlModeTags_0_8[];
extern char gHtmlModeTags_0_9[];
extern char gHtmlModeTags_0_10[];
extern char gHtmlModeTags_0_11[];
extern char gHtmlModeTags_2_1[];
extern char gHtmlModeTags_2_2[];
extern char gHtmlModeTags_2_3[];
extern char gHtmlModeTags_2_4[];
extern char gHtmlModeTags_2_5[];
extern char gHtmlModeTags_2_6[];
extern char gHtmlModeTags_2_7[];
extern char gHtmlModeTags_2_8[];
extern char gHtmlModeTags_2_9[];
extern char gHtmlModeTags_2_10[];
extern char gHtmlModeTags_2_11[];
extern char gHtmlModeTags_2_12[];
extern char gHtmlModeTags_2_13[];
extern char gHtmlModeTags_2_14[];
extern char gHtmlModeTags_2_15[];
extern char gHtmlModeTags_2_16[];
extern char gHtmlModeTags_2_17[];
extern char gHtmlModeTags_2_18[];
extern char gHtmlModeTags_2_19[];
extern char gHtmlModeTags_2_20[];
extern char gHtmlModeTags_2_21[];
extern char gHtmlModeTags_2_22[];
extern char gHtmlModeTags_2_23[];
extern char gHtmlModeTags_2_24[];
extern char gHtmlModeTags_2_25[];
extern char gHtmlModeTags_2_26[];
extern char gHtmlModeTags_2_27[];
extern char gHtmlModeTags_2_28[];
extern char gHtmlModeTags_2_29[];
extern char gHtmlModeTags_2_30[];
extern char gHtmlModeTags_2_31[];
extern char gHtmlModeTags_2_32[];
extern char gHtmlModeTags_2_33[];
extern char gHtmlModeTags_2_34[];
extern char gHtmlModeTags_2_35[];
extern char gHtmlModeTags_2_36[];
extern char gHtmlModeTags_2_37[];
extern char gHtmlModeTags_2_38[];
extern char gHtmlModeTags_2_39[];
extern char gHtmlModeTags_2_40[];
extern char gHtmlModeTags_2_41[];
extern char gHtmlModeTags_2_42[];
extern char gHtmlModeTags_2_43[];
extern char gHtmlModeTags_2_44[];
extern char gHtmlModeTags_2_45[];
extern char gHtmlModeTags_2_46[];
extern char gHtmlModeTags_2_47[];
extern char gHtmlModeTags_2_48[];
extern char gHtmlModeTags_2_49[];
extern char gHtmlModeTags_2_50[];
extern char gHtmlModeTags_2_51[];
extern char gHtmlModeTags_2_52[];
extern char gHtmlModeTags_2_53[];
extern char gHtmlModeTags_2_54[];
extern char gHtmlModeTags_2_55[];
extern char gHtmlModeTags_2_56[];
extern char gHtmlModeTags_2_57[];
extern char gHtmlModeTags_2_58[];
extern char gHtmlModeTags_2_59[];
extern char gHtmlModeTags_2_60[];
extern char gHtmlModeTags_2_61[];
extern char gHtmlModeTags_2_62[];
extern char gHtmlModeTags_2_63[];
extern char gHtmlModeTags_2_64[];
extern char gHtmlModeTags_2_65[];
extern char gHtmlModeTags_2_66[];
extern char gHtmlModeTags_2_67[];
extern char gHtmlModeTags_2_68[];
extern char gHtmlModeTags_2_69[];
extern char gHtmlModeTags_2_70[];
extern char gHtmlModeTags_2_71[];
extern char gHtmlModeTags_2_72[];
extern char gHtmlModeTags_2_73[];
extern char gHtmlModeTags_2_74[];
extern char gHtmlModeTags_2_75[];
extern char gHtmlModeTags_2_76[];
extern char gHtmlModeTags_2_308[];
extern char gHtmlModeTags_2_78[];
extern char gHtmlModeTags_2_79[];
extern char gHtmlModeTags_2_80[];
extern char gHtmlModeTags_2_81[];
extern char gHtmlModeTags_2_82[];
extern char gHtmlModeTags_2_83[];
extern char gHtmlModeTags_2_84[];
extern char gHtmlModeTags_2_85[];
extern char gHtmlModeTags_2_86[];
extern char gHtmlModeTags_2_87[];
extern char gHtmlModeTags_2_88[];
extern char gHtmlModeTags_24_1[];
extern char gHtmlModeTags_24_2[];
extern char D_00839CB8[];
extern char D_00839CC0[];
extern char D_00839CC8[];
extern char gHtmlModeTags_22_2[];
extern char gHtmlModeTags_21_2[];
extern char D_00839CF0[];
extern char gHtmlModeTags_14_1[];
extern char gHtmlModeTags_14_2[];
extern char gHtmlModeTags_14_3[];
extern char gHtmlModeTags_14_4[];
extern char D_00839D30[];
extern char D_00839D38[];
extern char D_00839D40[];
extern char D_00839D48[];
extern char D_00839D58[];
extern char D_00839D60[];
extern char D_00839D68[];
extern char D_00839D70[];
extern char gHtmlModeTags_13_1[];
extern char gHtmlModeTags_13_2[];
extern char gHtmlModeTags_13_3[];
extern char D_00839D98[];
extern char gHtmlModeTags_4_4[];
extern char gHtmlModeTags_5_1[];
extern char gHtmlModeTags_5_2[];
extern char gHtmlModeTags_6_1[];
extern char D_00839DC0[];
extern char gHtmlModeTags_7_1[];
extern char D_00839DD0[];
extern char gHtmlModeTags_7_5[];
extern char gHtmlModeTags_7_6[];
extern char D_00839DF0[];
extern char gHtmlModeTags_9_2[];
extern char gHtmlModeTags_12_5[];
HtmlKeyword gGameStyleKeywords[12] = {
#include "lobby/007EEF10.inc"
};
HtmlKeyword gPageCommentKeywords[43] = {
#include "lobby/007EEF70.inc"
};
HtmlLabel gHtmlLabelInit = {
#include "lobby/007EF0C8.inc"
};
u8 gHtmlHeadingSizes[16] = {
#include "lobby/007EF0D0.inc"
};
u8 gFullCharWidths[2][8] = {
#include "lobby/007EF0E0.inc"
};
u8 gHalfCharWidths[2][8] = {
#include "lobby/007EF0F0.inc"
};
HtmlTag gHtmlModeTags_0[12] = {
#include "lobby/007EF100.inc"
};
HtmlTag gHtmlModeTags_2[89] = {
#include "lobby/007EF1C0.inc"
};
HtmlTag gHtmlModeTags_24[5] = {
#include "lobby/007EF750.inc"
};
HtmlTag gHtmlModeTags_23[2] = {
#include "lobby/007EF7A0.inc"
};
HtmlTag gHtmlModeTags_22[3] = {
#include "lobby/007EF7C0.inc"
};
HtmlTag gHtmlModeTags_21[3] = {
#include "lobby/007EF7F0.inc"
};
HtmlTag gHtmlModeTags_20[2] = {
#include "lobby/007EF820.inc"
};
HtmlTag gHtmlModeTags_19[1] = {
#include "lobby/007EF840.inc"
};
HtmlTag gHtmlModeTags_18[1] = {
#include "lobby/007EF850.inc"
};
HtmlTag gHtmlModeTags_14[9] = {
#include "lobby/007EF860.inc"
};
HtmlTag gHtmlModeTags_15[4] = {
#include "lobby/007EF8F0.inc"
};
HtmlTag gHtmlModeTags_16[10] = {
#include "lobby/007EF930.inc"
};
HtmlTag gHtmlModeTags_17[10] = {
#include "lobby/007EF9D0.inc"
};
HtmlTag gHtmlModeTags_3[2] = {
#include "lobby/007EFA70.inc"
};
HtmlTag gHtmlModeTags_13[4] = {
#include "lobby/007EFA90.inc"
};
HtmlTag gHtmlModeTags_4[5] = {
#include "lobby/007EFAD0.inc"
};
HtmlTag gHtmlModeTags_5[3] = {
#include "lobby/007EFB20.inc"
};
HtmlTag gHtmlModeTags_6[3] = {
#include "lobby/007EFB50.inc"
};
HtmlTag gHtmlModeTags_7[11] = {
#include "lobby/007EFB80.inc"
};
HtmlTag gHtmlModeTags_8[2] = {
#include "lobby/007EFC30.inc"
};
HtmlTag gHtmlModeTags_9[3] = {
#include "lobby/007EFC50.inc"
};
HtmlTag gHtmlModeTags_10[3] = {
#include "lobby/007EFC80.inc"
};
HtmlTag gHtmlModeTags_11[3] = {
#include "lobby/007EFCB0.inc"
};
HtmlTag gHtmlModeTags_12[6] = {
#include "lobby/007EFCE0.inc"
};
HtmlTagSet gHtmlModeTags[25] = {
#include "lobby/007EFD40.inc"
};

/* The tags' and attributes' names (read-only data): the strings that
 * the tag sets' entries point at. */
#pragma rodata on
char D_00839908[8] = {
#include "lobby/00839908.inc"
};
char gHtmlModeTags_0_1[8] = {
#include "lobby/00839910.inc"
};
char gHtmlModeTags_0_2[8] = {
#include "lobby/00839918.inc"
};
char gHtmlModeTags_0_3[8] = {
#include "lobby/00839920.inc"
};
char gHtmlModeTags_0_4[8] = {
#include "lobby/00839928.inc"
};
char gHtmlModeTags_0_5[8] = {
#include "lobby/00839930.inc"
};
char gHtmlModeTags_0_6[8] = {
#include "lobby/00839938.inc"
};
char gHtmlModeTags_0_7[8] = {
#include "lobby/00839940.inc"
};
char gHtmlModeTags_0_8[8] = {
#include "lobby/00839948.inc"
};
char gHtmlModeTags_0_9[8] = {
#include "lobby/00839950.inc"
};
char gHtmlModeTags_0_10[8] = {
#include "lobby/00839958.inc"
};
char gHtmlModeTags_0_11[8] = {
#include "lobby/00839960.inc"
};
char gHtmlModeTags_2_1[8] = {
#include "lobby/00839968.inc"
};
char gHtmlModeTags_2_2[8] = {
#include "lobby/00839970.inc"
};
char gHtmlModeTags_2_3[8] = {
#include "lobby/00839978.inc"
};
char gHtmlModeTags_2_4[8] = {
#include "lobby/00839980.inc"
};
char gHtmlModeTags_2_5[8] = {
#include "lobby/00839988.inc"
};
char gHtmlModeTags_2_6[8] = {
#include "lobby/00839990.inc"
};
char gHtmlModeTags_2_7[8] = {
#include "lobby/00839998.inc"
};
char gHtmlModeTags_2_8[8] = {
#include "lobby/008399A0.inc"
};
char gHtmlModeTags_2_9[8] = {
#include "lobby/008399A8.inc"
};
char gHtmlModeTags_2_10[8] = {
#include "lobby/008399B0.inc"
};
char gHtmlModeTags_2_11[8] = {
#include "lobby/008399B8.inc"
};
char gHtmlModeTags_2_12[16] = {
#include "lobby/008399C0.inc"
};
char gHtmlModeTags_2_13[8] = {
#include "lobby/008399D0.inc"
};
char gHtmlModeTags_2_14[8] = {
#include "lobby/008399D8.inc"
};
char gHtmlModeTags_2_15[8] = {
#include "lobby/008399E0.inc"
};
char gHtmlModeTags_2_16[8] = {
#include "lobby/008399E8.inc"
};
char gHtmlModeTags_2_17[8] = {
#include "lobby/008399F0.inc"
};
char gHtmlModeTags_2_18[8] = {
#include "lobby/008399F8.inc"
};
char gHtmlModeTags_2_19[16] = {
#include "lobby/00839A00.inc"
};
char gHtmlModeTags_2_20[16] = {
#include "lobby/00839A10.inc"
};
char gHtmlModeTags_2_21[8] = {
#include "lobby/00839A20.inc"
};
char gHtmlModeTags_2_22[8] = {
#include "lobby/00839A28.inc"
};
char gHtmlModeTags_2_23[8] = {
#include "lobby/00839A30.inc"
};
char gHtmlModeTags_2_24[8] = {
#include "lobby/00839A38.inc"
};
char gHtmlModeTags_2_25[8] = {
#include "lobby/00839A40.inc"
};
char gHtmlModeTags_2_26[8] = {
#include "lobby/00839A48.inc"
};
char gHtmlModeTags_2_27[8] = {
#include "lobby/00839A50.inc"
};
char gHtmlModeTags_2_28[8] = {
#include "lobby/00839A58.inc"
};
char gHtmlModeTags_2_29[8] = {
#include "lobby/00839A60.inc"
};
char gHtmlModeTags_2_30[8] = {
#include "lobby/00839A68.inc"
};
char gHtmlModeTags_2_31[8] = {
#include "lobby/00839A70.inc"
};
char gHtmlModeTags_2_32[8] = {
#include "lobby/00839A78.inc"
};
char gHtmlModeTags_2_33[8] = {
#include "lobby/00839A80.inc"
};
char gHtmlModeTags_2_34[8] = {
#include "lobby/00839A88.inc"
};
char gHtmlModeTags_2_35[8] = {
#include "lobby/00839A90.inc"
};
char gHtmlModeTags_2_36[8] = {
#include "lobby/00839A98.inc"
};
char gHtmlModeTags_2_37[16] = {
#include "lobby/00839AA0.inc"
};
char gHtmlModeTags_2_38[16] = {
#include "lobby/00839AB0.inc"
};
char gHtmlModeTags_2_39[16] = {
#include "lobby/00839AC0.inc"
};
char gHtmlModeTags_2_40[16] = {
#include "lobby/00839AD0.inc"
};
char gHtmlModeTags_2_41[16] = {
#include "lobby/00839AE0.inc"
};
char gHtmlModeTags_2_42[16] = {
#include "lobby/00839AF0.inc"
};
char gHtmlModeTags_2_43[8] = {
#include "lobby/00839B00.inc"
};
char gHtmlModeTags_2_44[12] = {
#include "lobby/00839B08.inc"
};
char gHtmlModeTags_2_45[12] = {
#include "lobby/00839B18.inc"
};
char gHtmlModeTags_2_46[8] = {
#include "lobby/00839B28.inc"
};
char gHtmlModeTags_2_47[8] = {
#include "lobby/00839B30.inc"
};
char gHtmlModeTags_2_48[8] = {
#include "lobby/00839B38.inc"
};
char gHtmlModeTags_2_49[8] = {
#include "lobby/00839B40.inc"
};
char gHtmlModeTags_2_50[12] = {
#include "lobby/00839B48.inc"
};
char gHtmlModeTags_2_51[12] = {
#include "lobby/00839B58.inc"
};
char gHtmlModeTags_2_52[8] = {
#include "lobby/00839B68.inc"
};
char gHtmlModeTags_2_53[8] = {
#include "lobby/00839B70.inc"
};
char gHtmlModeTags_2_54[8] = {
#include "lobby/00839B78.inc"
};
char gHtmlModeTags_2_55[8] = {
#include "lobby/00839B80.inc"
};
char gHtmlModeTags_2_56[8] = {
#include "lobby/00839B88.inc"
};
char gHtmlModeTags_2_57[8] = {
#include "lobby/00839B90.inc"
};
char gHtmlModeTags_2_58[8] = {
#include "lobby/00839B98.inc"
};
char gHtmlModeTags_2_59[8] = {
#include "lobby/00839BA0.inc"
};
char gHtmlModeTags_2_60[8] = {
#include "lobby/00839BA8.inc"
};
char gHtmlModeTags_2_61[8] = {
#include "lobby/00839BB0.inc"
};
char gHtmlModeTags_2_62[8] = {
#include "lobby/00839BB8.inc"
};
char gHtmlModeTags_2_63[8] = {
#include "lobby/00839BC0.inc"
};
char gHtmlModeTags_2_64[8] = {
#include "lobby/00839BC8.inc"
};
char gHtmlModeTags_2_65[8] = {
#include "lobby/00839BD0.inc"
};
char gHtmlModeTags_2_66[8] = {
#include "lobby/00839BD8.inc"
};
char gHtmlModeTags_2_67[8] = {
#include "lobby/00839BE0.inc"
};
char gHtmlModeTags_2_68[8] = {
#include "lobby/00839BE8.inc"
};
char gHtmlModeTags_2_69[8] = {
#include "lobby/00839BF0.inc"
};
char gHtmlModeTags_2_70[8] = {
#include "lobby/00839BF8.inc"
};
char gHtmlModeTags_2_71[8] = {
#include "lobby/00839C00.inc"
};
char gHtmlModeTags_2_72[8] = {
#include "lobby/00839C08.inc"
};
char gHtmlModeTags_2_73[8] = {
#include "lobby/00839C10.inc"
};
char gHtmlModeTags_2_74[8] = {
#include "lobby/00839C18.inc"
};
char gHtmlModeTags_2_75[16] = {
#include "lobby/00839C20.inc"
};
char gHtmlModeTags_2_76[8] = {
#include "lobby/00839C30.inc"
};
char gHtmlModeTags_2_308[8] = {
#include "lobby/00839C38.inc"
};
char gHtmlModeTags_2_78[8] = {
#include "lobby/00839C40.inc"
};
char gHtmlModeTags_2_79[8] = {
#include "lobby/00839C48.inc"
};
char gHtmlModeTags_2_80[8] = {
#include "lobby/00839C50.inc"
};
char gHtmlModeTags_2_81[12] = {
#include "lobby/00839C58.inc"
};
char gHtmlModeTags_2_82[8] = {
#include "lobby/00839C68.inc"
};
char gHtmlModeTags_2_83[8] = {
#include "lobby/00839C70.inc"
};
char gHtmlModeTags_2_84[8] = {
#include "lobby/00839C78.inc"
};
char gHtmlModeTags_2_85[8] = {
#include "lobby/00839C80.inc"
};
char gHtmlModeTags_2_86[8] = {
#include "lobby/00839C88.inc"
};
char gHtmlModeTags_2_87[8] = {
#include "lobby/00839C90.inc"
};
char gHtmlModeTags_2_88[8] = {
#include "lobby/00839C98.inc"
};
char gHtmlTagClose[8] = {
#include "lobby/00839CA0.inc"
};
char gHtmlModeTags_24_1[8] = {
#include "lobby/00839CA8.inc"
};
char gHtmlModeTags_24_2[8] = {
#include "lobby/00839CB0.inc"
};
char D_00839CB8[8] = {
#include "lobby/00839CB8.inc"
};
char D_00839CC0[8] = {
#include "lobby/00839CC0.inc"
};
char D_00839CC8[8] = {
#include "lobby/00839CC8.inc"
};
char gHtmlModeTags_22_2[16] = {
#include "lobby/00839CD0.inc"
};
char gHtmlModeTags_21_2[16] = {
#include "lobby/00839CE0.inc"
};
char D_00839CF0[8] = {
#include "lobby/00839CF0.inc"
};
char gHtmlModeTags_14_1[12] = {
#include "lobby/00839CF8.inc"
};
char gHtmlModeTags_14_2[8] = {
#include "lobby/00839D08.inc"
};
char gHtmlModeTags_14_3[16] = {
#include "lobby/00839D10.inc"
};
char gHtmlModeTags_14_4[16] = {
#include "lobby/00839D20.inc"
};
char D_00839D30[8] = {
#include "lobby/00839D30.inc"
};
char D_00839D38[8] = {
#include "lobby/00839D38.inc"
};
char D_00839D40[8] = {
#include "lobby/00839D40.inc"
};
char D_00839D48[12] = {
#include "lobby/00839D48.inc"
};
char D_00839D58[8] = {
#include "lobby/00839D58.inc"
};
char D_00839D60[8] = {
#include "lobby/00839D60.inc"
};
char D_00839D68[8] = {
#include "lobby/00839D68.inc"
};
char D_00839D70[8] = {
#include "lobby/00839D70.inc"
};
char gHtmlModeTags_13_1[12] = {
#include "lobby/00839D78.inc"
};
char gHtmlModeTags_13_2[8] = {
#include "lobby/00839D88.inc"
};
char gHtmlModeTags_13_3[8] = {
#include "lobby/00839D90.inc"
};
char D_00839D98[8] = {
#include "lobby/00839D98.inc"
};
char gHtmlModeTags_4_4[8] = {
#include "lobby/00839DA0.inc"
};
char gHtmlModeTags_5_1[8] = {
#include "lobby/00839DA8.inc"
};
char gHtmlModeTags_5_2[8] = {
#include "lobby/00839DB0.inc"
};
char gHtmlModeTags_6_1[8] = {
#include "lobby/00839DB8.inc"
};
char D_00839DC0[8] = {
#include "lobby/00839DC0.inc"
};
char gHtmlModeTags_7_1[8] = {
#include "lobby/00839DC8.inc"
};
char D_00839DD0[8] = {
#include "lobby/00839DD0.inc"
};
char gHtmlModeTags_7_5[12] = {
#include "lobby/00839DD8.inc"
};
char gHtmlModeTags_7_6[8] = {
#include "lobby/00839DE8.inc"
};
char D_00839DF0[8] = {
#include "lobby/00839DF0.inc"
};
char gHtmlModeTags_9_2[12] = {
#include "lobby/00839DF8.inc"
};
char gHtmlModeTags_12_5[8] = {
#include "lobby/00839E08.inc"
};
#pragma rodata off
