/*
 * The network mode's message pages.
 *
 * The file draws the message pages. gLobbyMsgPages holds seven pages, each
 * a NULL-ended table of parts, each part a NULL-ended list of lines
 * (Shift-JIS strings in .rodata). NetModeMsg_Flush calls the drawers queued
 * for the frame: LobbyMsg_DrawPage (a part, laid out by page and part),
 * LobbyMsg_DrawPage4 and LobbyMsg_DrawPage5 (layout tables) and
 * LobbyMsg_DrawList (a list of choices with gNetScreenIcons' icons).
 * LobbyMsg_Format fills a line's %-codes (the card slot, a count, the login
 * id, the password as stars, the error code) and returns its tag, which may
 * add a sub-list or a yes/no pair under the line (LobbyMsg_DrawPart);
 * LobbyMsg_Print puts the version number in place of a "%v". The window
 * titles (gNetModeTitleTexts) and the layouts of pages 4 and 5 end the
 * .data.
 */
#include "common.h"
#include "card.h"
#include "net_save.h"
#include "syssettings.h"
#include "net_mode.h"
#include "em.h"
#include "sprite.h"
#include "lobby_globals.h"
#include "net_menu.h"
#include "text.h"
#include "font.h"
#include "prim.h"

/* prototypes */
char * strstr(char *s, char *w);
extern char ***gLobbyMsgPages[];          /* the pages' texts: NULL-ended lists */
void *memset(void *dst, s32 c, u32 n);

/* LobbyMsg_Format's result: what a line's %-code adds under it, which
 * LobbyMsg_DrawPart draws. */
enum LobbyMsgTag {
    LOBBY_MSG_TAG_NONE = 0,
    LOBBY_MSG_TAG_TIMEOUT = 1,      /* %r, only while gNetMode.timer is 0 */
    LOBBY_MSG_TAG_SUBLIST = 2,      /* %t: a part's lines */
    LOBBY_MSG_TAG_YES_NO = 3,       /* %y: the yes/no pair */
    LOBBY_MSG_TAG_CHOICE = 4,       /* %k: the two-line choice (gNetMode.sel2) */
    LOBBY_MSG_TAG_LIST = 5,         /* %l, on page 5 */
    LOBBY_MSG_TAG_LOGIN = 6         /* %i or %p: the login id or the password; the line moves to x 200 */
};

void strncat(char *dst, char *src, s32 n);  /* strncat */
s32 sprintf(char *dst, const char *fmt, ...);   /* sprintf */
char *strcat(char *dst, const char *src);
s32 strlen(const char *s);
extern char **gLobbyMsgDnasNotePart;
s32 Str_Length(char *s);
void Font_Printf(char *fmt, ...);
char *strcpy(char *dst, const char *src);
char *strchr(char *s, s32 c);      /* strchr */
extern s8 gNetVerDigits;               /* the version's digits */
extern s8 gNetVerDigits_1;
extern s8 gNetVerDigits_2;
extern s8 gNetVerDigits_3;

/* the drawers of pages 4 and 5 and their layout tables */
extern s16 gLobbyMsgPage4Layout[][3][2];
extern s16 gLobbyMsgPage5Layout[][3][2];          /* per part of page 5, the same */

/* an icon of the network screen (gNetScreenIcons, 10 bytes; three frames each) */
typedef struct NetIcon {
    /* 0x0 */ u8 tex;               /* the texture (Tex_Prepare, Tex_Bind) */
    /* 0x1 */ u8 layer;             /* Gfx_SetTexFilter */
    /* 0x2 */ s16 u0, v0, u1, v1;
} NetIcon;

extern NetIcon gNetScreenIcons[];




/* end of prototypes */

char **LobbyMsg_GetLines(s32 page, s32 part)
{
    return gLobbyMsgPages[page][part];
}

/* draws what a line's tag (`tag`, LobbyMsg_Format's result) adds under it,
 * from y, lineH apart, in characters of size. Page 1: LOBBY_MSG_TAG_TIMEOUT
 * the lines of part 2, LOBBY_MSG_TAG_SUBLIST those of part 1 one line
 * lower, LOBBY_MSG_TAG_YES_NO the yes/no pair (page 0 part 14: yes, no,
 * and the line that places them) with the choice gNetMode.yesNo in colour
 * 5, LOBBY_MSG_TAG_CHOICE the two lines of part 25 with the choice sel2.
 * Page 3, at size 18: LOBBY_MSG_TAG_SUBLIST the lines of part 4,
 * LOBBY_MSG_TAG_YES_NO the yes/no pair. Returns the y after what it drew */
s32 LobbyMsg_DrawPart(s32 size, s32 page, s32 tag, s32 y, s32 lineH)
{
    char **msgs;
    s32 x;
    NetModeState *netMode;
    char buf[128];

    netMode = &gNetMode;
    memset(buf, 0, 128);
    switch (page) {
    case 1:
        switch (tag) {
        case LOBBY_MSG_TAG_TIMEOUT:
            msgs = LobbyMsg_GetLines(page, 2);
            break;
        case LOBBY_MSG_TAG_SUBLIST:
            msgs = LobbyMsg_GetLines(page, 1);
            y += lineH;
            break;
        case LOBBY_MSG_TAG_YES_NO:
            msgs = LobbyMsg_GetLines(0, 14);
            x = LobbyMsg_CenterX(msgs[2], size);
            LobbyMsg_Print(msgs[0], x, 324, netMode->yesNo == 0 ? 5 : 0, size);
            x += size * 5;
            LobbyMsg_Print(msgs[1], x, 324, netMode->yesNo == 1 ? 5 : 0, size);
            return lineH + 324;
        case LOBBY_MSG_TAG_CHOICE:
            msgs = LobbyMsg_GetLines(page, 25);
            x = LobbyMsg_CenterX(msgs[0], 18);
            LobbyMsg_Print(msgs[0], x, 280, netMode->sel2 == 0 ? 5 : 0, 18);
            x = LobbyMsg_CenterX(msgs[1], 18);
            LobbyMsg_Print(msgs[1], x, 324, netMode->sel2 == 1 ? 5 : 0, 18);
            return lineH + 324;
        }
    lines:
        while (*msgs != NULL) {
            LobbyMsg_Print(*msgs, LobbyMsg_CenterX(*msgs, size), y, 0, size);
            msgs++;
            y += lineH;
        }
        break;
    case 5:
        break;
    case 3:
        size = 18;
        switch (tag) {
        case LOBBY_MSG_TAG_TIMEOUT:
            break;
        case LOBBY_MSG_TAG_SUBLIST:
            msgs = LobbyMsg_GetLines(page, 4);
            y += lineH;
            goto lines;
        case LOBBY_MSG_TAG_YES_NO:
            msgs = LobbyMsg_GetLines(0, 14);
            x = LobbyMsg_CenterX(msgs[2], size);
            LobbyMsg_Print(msgs[0], x, 324, netMode->yesNo == 0 ? 5 : 0, size);
            LobbyMsg_Print(msgs[1], x + 90, 324, netMode->yesNo == 1 ? 5 : 0, 18);
            return lineH + 324;
        }
        break;
    }
    return y;
}

/* one more than the last part of page `page` (a page's parts run from 1;
 * part 0 is never drawn); 0 for a page that does not exist */
static s32 LobbyMsg_PartCount(u32 page)
{
    switch (page) {
    case 0:
        return 25;
    case 1:
        return 61;
    case 2:
        return 28;
    case 3:
        return 16;
    case 4:
        return 18;
    case 5:
        return 7;
    case 6:
        return 7;
    default:
        return 0;
    }
}

/* expands a help line of page `page` into dst: pages 1, 3 and 5 replace "%s"
 * (the card slot, full width), "%1", "%2", "%a" (the files count), "%i" (the
 * id, gNetSaveData.mmbbLoginId) and "%p" (the password
 * gNetSaveData.mmbbPassword as stars), and mark the line by its tag (enum
 * LobbyMsgTag); page 2 replaces "%e" (the error code) and "%f" (the DNAS
 * note's line, gLobbyMsgDnasNote, while the DNAS step runs: gNetMode.unk7E);
 * other pages copy it. Returns the tag */
s32 LobbyMsg_Format(s32 page, char *src, char *dst)
{
    char buf[16];
    char *rest;
    s32 tag;
    NetModeState *netMode;
    char num[16];
    char *code;
    char *errRest;
    char *errCode;
    s32 passLen;
    s32 i;

    netMode = &gNetMode;
    if (src == NULL) {
        return 0;
    }
    if (dst == NULL) {
        return 0;
    }
    *dst = 0;
    tag = LOBBY_MSG_TAG_NONE;
    switch (page) {
    case 5:
    case 1:
    case 3:
        rest = src;
        for (;;) {
            if ((code = strstr(rest, "%s")) != NULL) {
                strncat(dst, rest, code - rest);
                sprintf(num, "%d", netMode->slotUsed + 1);
                Sjis_Widen(num, buf);
                strcat(dst, buf);
                rest = code + 2;
                continue;
            }
            if ((code = strstr(rest, "%1")) != NULL) {
                strncat(dst, rest, code - rest);
                strcpy(num, "1");
                Sjis_Widen(num, buf);
                strcat(dst, buf);
                rest = code + 2;
                continue;
            }
            if ((code = strstr(rest, "%2")) != NULL) {
                strncat(dst, rest, code - rest);
                strcpy(num, "2");
                Sjis_Widen(num, buf);
                strcat(dst, buf);
                rest = code + 2;
                continue;
            }
            if ((code = strstr(rest, "%a")) != NULL) {
                strncat(dst, rest, code - rest);
                Sjis_PutDigits3(buf, netMode->files);
                strcat(dst, buf);
                rest = code + 2;
                continue;
            }
            if ((code = strstr(rest, "%r")) != NULL) {
                strncat(dst, rest, code - rest);
                if (netMode->timer == 0) {
                    tag = LOBBY_MSG_TAG_TIMEOUT;
                }
                rest = code + 2;
                continue;
            }
            if ((code = strstr(rest, "%t")) != NULL) {
                strncat(dst, rest, code - rest);
                rest = code + 2;
                tag = LOBBY_MSG_TAG_SUBLIST;
                continue;
            }
            if ((code = strstr(rest, "%y")) != NULL) {
                strncat(dst, rest, code - rest);
                rest = code + 2;
                tag = LOBBY_MSG_TAG_YES_NO;
                continue;
            }
            if ((code = strstr(rest, "%k")) != NULL) {
                strncat(dst, rest, code - rest);
                rest = code + 2;
                tag = LOBBY_MSG_TAG_CHOICE;
                continue;
            }
            if ((code = strstr(rest, "%i")) != NULL) {
                strncat(dst, rest, code - rest);
                strcpy(buf, gNetSaveData.mmbbLoginId);
                strcat(dst, buf);
                rest = code + 2;
                tag = LOBBY_MSG_TAG_LOGIN;
                continue;
            }
            if ((code = strstr(rest, "%p")) != NULL) {
                strncat(dst, rest, code - rest);
                buf[0] = 0;
                passLen = strlen(gNetSaveData.mmbbPassword);
                if (passLen != 0) {
                    for (i = 0; i < passLen; i++) {
                        buf[i] = '*';
                    }
                    buf[i] = 0;
                }
                strcat(dst, buf);
                rest = code + 2;
                tag = LOBBY_MSG_TAG_LOGIN;
                continue;
            }
            if (page == 5 && (code = strstr(rest, "%l")) != NULL) {
                strncat(dst, rest, code - rest);
                rest = code + 2;
                tag = LOBBY_MSG_TAG_LIST;
                continue;
            }
            strcat(dst, rest);
            break;
        }
        break;
    case 2:
        errRest = src;
        for (;;) {
            if ((errCode = strstr(errRest, "%e")) != NULL) {
                strncat(dst, errRest, errCode - errRest);
                sprintf(buf, "%d", gNetMode.err);
                strcat(dst, buf);
                errRest = errCode + 2;
                continue;
            }
            if ((errCode = strstr(errRest, "%f")) != NULL) {
                if (netMode->unk7E != 0) {
                    strcat(dst, *gLobbyMsgDnasNotePart);
                }
                errRest = errCode + 2;
            }
            strcat(dst, errRest);
            break;
        }
        break;
    default:
        strcpy(dst, src);
        break;
    }
    return tag;
}

/* counts the characters of str, up to 128: the two-byte Shift-JIS ones (a
 * first byte from 128 to 159 or from 224 to 255) in *w, the one-byte ones
 * in *n; nothing when a pointer is NULL */
void LobbyMsg_Measure(u8 *str, s32 *wideCount, s32 *narrowCount)
{
    s32 i;
    u8 *cursor;
    u8 ch;

    cursor = str;
    if ((cursor != NULL) && (narrowCount != NULL) && (wideCount != NULL)) {
        *narrowCount = 0;
        i = 0;
        *wideCount = 0;
loop_4:
        ch = *cursor;
        if (ch != 0) {
            if ((s32) ch >= 128) {
                if ((s32) ch > 159) {
                    goto block_8;
                }
                goto block_11;
            }
block_8:
            if (((s32) ch >= 224) && ((s32) ch < 256)) {
block_11:
                cursor += 2;
                *wideCount += 1;
            } else {
                cursor += 1;
                *narrowCount += 1;
            }
            i += 1;
            if (i < 128) {
                goto loop_4;
            }
        }
    }
}

/* the x that centres str on the 640-pixel screen in characters of size: a
 * two-byte character is size wide, a one-byte one half that
 * (LobbyMsg_Measure); a "~B00" (a button picture) takes two one-byte
 * characters off, a "%v" (the version, five two-byte characters) puts its
 * width in place of two */
s32 LobbyMsg_CenterX(char *str, s32 size)
{
    s32 wideCount;
    s32 narrowCount;

    wideCount = 0;
    narrowCount = 0;
    LobbyMsg_Measure((u8 *)str, &wideCount, &narrowCount);
    if (strstr(str, "~B00") != NULL) {
        narrowCount -= 2;
    }
    if (strstr(str, "%v") != NULL) {
        narrowCount -= 2;
        wideCount += 5;
    }
    return (s16)((640 - size * wideCount - narrowCount * (size / 2)) / 2);
}

/* prints str at x, y (characters of size, colour color); a "%v" in it is
 * replaced by the version (the four digits of gNetVerDigits as "a.bcd",
 * widened by Sjis_Widen) printed in colour 5 */
void LobbyMsg_Print(char *str, s32 x, s32 y, s32 color, s32 size)
{
    char buf[128];
    char num[16];
    char *mark;

    buf[1] = '.';
    buf[5] = 0;
    buf[0] = gNetVerDigits;
    buf[2] = gNetVerDigits_1;
    buf[3] = gNetVerDigits_2;
    buf[4] = gNetVerDigits_3;
    Sjis_Widen(buf, num);
    Font_SetSpacing(size, size);
    Font_SetColorIndex(color);
    Font_SetPos(x, y);
    strcpy(buf, str);
    mark = strchr(buf, '%');
    if (mark != NULL && mark[1] == 'v') {
        *mark = 0;
        Font_Printf(buf);
        x += size * Str_Length(buf) / 2;
        Font_SetPos(x, y);
        Font_SetColorIndex(5);
        Font_Printf(num);
        strcpy(buf, str + (mark + 2 - buf));
        if (buf[0] != 0) {
            x += size * Str_Length(num) / 2;
            Font_SetPos(x, y);
            Font_SetColorIndex(color);
            Font_Printf(buf);
        }
    } else {
        Text_Printf(str);
    }
}

/* draws part `part` of page `page` (its lines, gLobbyMsgPages; each through
 * LobbyMsg_Format into buf, then LobbyMsg_Print centred or at x): the layout
 * (y, the size, x, the colour) by page and part, x -1 centring each line. A
 * line tagged LOBBY_MSG_TAG_LOGIN (the login id or the password) puts it and
 * the lines after at x 200; any other tag draws its sub-list
 * (LobbyMsg_DrawPart). Nothing for part 0 or a part past the page's */
void LobbyMsg_DrawPage(s32 page, s32 part)
{
    s32 x;
    s32 color;
    s32 y;
    s32 lineH;
    s32 size;
    s32 tag;
    char **msgs;
    char buf[128];

    if (part != 0 && part < LobbyMsg_PartCount(page)) {
        msgs = LobbyMsg_GetLines(page, part);
        size = 20;
        x = -1;
        lineH = size * 1.2f;
        color = 0;
        switch (page) {
        case 0:
            switch (part) {
            case 2:
            case 3:
                y = 280;
                break;
            case 4:
                y = 392;
                break;
            case 10:
            case 11:
                y = 94;
                color = 5;
                break;
            case 20:
                y = 392;
                break;
            default:
                y = 100;
                break;
            }
            break;
        case 2:
            switch (part) {
            case 1:
                y = 280;
                break;
            case 2:
                y = 392;
                break;
            case 4:
                y = 228;
                size = 16;
                break;
            case 3:
            case 5:
            case 6:
            case 7:
                x = 64;
                y = 94;
                break;
            default:
                x = 64;
                y = 134;
                break;
            }
            break;
        case 4:
            switch (part) {
            case 3:
            case 4:
            case 5:
            case 1:
            case 2:
            case 6:
            case 7:
            case 8:
            case 16:
            case 15:
            case 17:
                x = 48;
                y = 92;
                break;
            default:
                y = 100;
                break;
            }
            break;
        case 1:
            size = 18;
        case 3:
        case 5:
        default:
            y = 100;
            break;
        case 6:
            y = 392;
            break;
        }
        while (*msgs != NULL) {
            tag = LobbyMsg_Format(page, (char *)*msgs, buf);
            if (tag == LOBBY_MSG_TAG_LOGIN) {
                tag = LOBBY_MSG_TAG_NONE;
                x = 200;
            }
            LobbyMsg_Print(buf, x < 0 ? LobbyMsg_CenterX(buf, size) : x, y, color, size);
            msgs++;
            y += lineH;
            if (tag != LOBBY_MSG_TAG_NONE) {
                y = LobbyMsg_DrawPart(size, page, tag, y, lineH);
            }
        }
    }
}

/* the drawer of message kind 4: NetModeMsg_Flush calls each drawer with a
 * message's (arg, id); this one draws nothing and reads neither. */
void LobbyMsg_DrawNoneAlt(s32 unused, s32 unused2)
{
}

/* draws part `part` (1-17) of page 4 with its record of
 * gLobbyMsgPage4Layout: x (-1 centres each line), y, the size, the colour,
 * one unused value, the line height. The table has 16 records, so parts 16
 * and 17 read the next table's first two, page 5's (the second is page 5
 * part 1's layout), as the game's code does */
void LobbyMsg_DrawPage4(s32 page, s32 part)
{
    s16 *layout;
    s16 color;
    s16 layoutX;
    s16 x;
    s16 y;
    s16 lineH;
    char **msgs;
    s16 size;

    if (part == 0 || part >= 18) {
        return;
    }
    layout = gLobbyMsgPage4Layout[part][0];
    x = *layout++;
    y = *layout++;
    size = *layout++;
    color = *layout++;
    layout++;
    lineH = *layout;
    msgs = LobbyMsg_GetLines(4, part);
    layoutX = x;
    while (*msgs != NULL) {
        if (layoutX == -1) {
            x = LobbyMsg_CenterX(*msgs, size);
        }
        LobbyMsg_Print(*msgs, x, y, color, size);
        msgs++;
        y += lineH;
    }
}

/* draws part `part` of help page 5 like LobbyMsg_DrawPage4, each line through
 * LobbyMsg_Format first (gLobbyMsgPage5Layout's layout; the lines one line
 * height up) */
void LobbyMsg_DrawPage5(s32 part)
{
    s16 *layout;
    char buf[512];
    s16 layoutX;
    s16 x;
    s16 y;
    s16 lineH;
    char **msgs;
    s16 color;
    s16 size;

    if (part == 0 || part >= 7) {
        return;
    }
    layout = gLobbyMsgPage5Layout[part][0];
    x = *layout++;
    y = *layout++;
    size = *layout++;
    color = *layout++;
    layout++;
    lineH = *layout;
    msgs = LobbyMsg_GetLines(5, part);
    layoutX = x;
    while (*msgs != NULL) {
        LobbyMsg_Format(5, (char *)*msgs, buf);
        if (layoutX == -1) {
            x = LobbyMsg_CenterX(buf, size);
        }
        LobbyMsg_Print(buf, x, y - lineH, color, size);
        msgs++;
        y += lineH;
    }
}

/* the drawer of message kind 1: NetModeMsg_Flush calls each drawer with a
 * message's (arg, id); this one draws nothing and reads neither. */
void LobbyMsg_DrawNone(s32 unused, s32 unused2)
{
}

/* empty: LobbyLogin_BlinkWait calls it in the shown half of its blink */
void LobbyLogin_DrawBlinkOn(void)
{
}

/* draws icon `kind` of gNetScreenIcons at x, y: small, or big (big != 0) and
 * animated (its frame, 0-2, is the icon frame NetMode_StepAnim steps every
 * 4 frames). The frame is read through an s8 cast: the field is declared
 * u8, and reading it as one would change the load. */
void LobbyMsg_DrawIcon(s32 kind, s16 x, s16 y, s32 big)
{
    Sprite sprite;
    NetIcon *icon;
    u32 color;

    if (big == 0) {
        icon = &gNetScreenIcons[kind * 3];
        color = 0xFFA87600;
        x -= 14;
        y -= 5;
        sprite.w = 36;
        sprite.h = 30;
    } else {
        icon = &gNetScreenIcons[(*(s8 *)&gNetMode.animFrame) + kind * 3];
        color = 0xFFFFBD29;
        x -= 24;
        y -= 8;
        sprite.w = 43;
        sprite.h = 36;
    }
    Gfx_SetTexFilter(icon->layer);
    Tex_Prepare(1, icon->tex);
    Tex_Bind(icon->tex);
    sprite.x = x;
    sprite.y = y;
    sprite.color = color;
    sprite.u0 = icon->u0;
    sprite.v0 = icon->v0;
    sprite.u1 = icon->u1;
    sprite.v1 = icon->v1;
    Sprite_Draw(&sprite);
}

/* draws the list of page `page` part `part` (21, 22: the network
 * settings' choices) with the current choice (sel3, or sel2 for 22) big,
 * with a shadow and its icon, and the choice's note line (from part 23,
 * or 24 for 22) at the bottom; in part 21 the line of the set-up not yet
 * done and the line of a missing login id are drawn in colour 10. Rows
 * are 45 apart, with a fifth of a row more after the third. */
void LobbyMsg_DrawList(s32 page, s32 part)
{
    char buf[128];
    NetModeState *netMode;
    s32 y;
    s32 lineY;
    s32 top;
    s32 i;
    s32 missing;
    char **msgs;
    s32 rowH;
    s32 bigSize;
    s32 step;
    s32 iconW;
    s32 rowPad;
    s32 sel;
    f32 rowHF, bigSizeF, stepF, iconWF;

    netMode = &gNetMode;
    if (part != 0 && part < LobbyMsg_PartCount(page)) {
        msgs = LobbyMsg_GetLines(page, part);
        rowHF = 36.0f;
        bigSizeF = 22.5f;
        stepF = 45.0f;
        iconWF = 27.0f;
        rowH = rowHF;
        bigSize = bigSizeF;
        step = stepF;
        iconW = iconWF;
        top = 100;
        if (netMode->openH == 0) {
            netMode->openW++;
            if (netMode->openW >= 4) {
                netMode->openH = 1;
            }
        } else {
            netMode->openW--;
            if (netMode->openW <= 0) {
                netMode->openH = 0;
            }
        }
        if (part == 22) {
            sel = netMode->sel2;
        } else {
            sel = netMode->sel3;
        }
        missing = 0;
        if (part == 21) {
            if (gNetSaveData.setupResult == 0) {
                missing = 1;
            }
            if (gNetSaveData.mmbbLoginId[0] == 0) {
                missing |= 2;
            }
        }
        for (i = 0; *msgs != NULL; msgs++) {
            LobbyMsg_Format(page, (char *)*msgs, buf);
            if (i == sel) {
                y = top + (s32)(((f32)rowH - (f32)bigSize) / 2.0f);
                LobbyMsg_Print(*msgs, 113, y + 3, 1, bigSize);
                LobbyMsg_Print(*msgs, 110, y, 11, bigSize);
                if (part == 22) {
                    if (i != 3) {
                        LobbyMsg_DrawIcon(i + 3, 110 - iconW, y, 1);
                    }
                } else if (i != 3) {
                    LobbyMsg_DrawIcon(i, 110 - iconW, y, 1);
                }
            } else {
                rowPad = ((f32)rowH - 18.0f) / 2.0f;
                if (missing == 0) {
                    lineY = top + rowPad;
                    LobbyMsg_Print(*msgs, 112, lineY + 2, 1, 18);
                    LobbyMsg_Print(*msgs, 110, lineY, 0, 18);
                } else {
                    switch (i) {
                    case 1:
                        if (missing & 1) {
                            lineY = top + rowPad;
                            LobbyMsg_Print(*msgs, 112, lineY + 2, 1, 18);
                            LobbyMsg_Print(*msgs, 110, lineY, 10, 18);
                            missing &= 2;
                        } else {
                            lineY = top + rowPad;
                            LobbyMsg_Print(*msgs, 112, lineY + 2, 1, 18);
                            LobbyMsg_Print(*msgs, 110, lineY, 0, 18);
                        }
                        break;
                    case 2:
                        if (missing & 2) {
                            lineY = top + rowPad;
                            LobbyMsg_Print(*msgs, 112, lineY + 2, 1, 18);
                            LobbyMsg_Print(*msgs, 110, lineY, 10, 18);
                            missing = 0;
                        } else {
                            lineY = top + rowPad;
                            LobbyMsg_Print(*msgs, 112, lineY + 2, 1, 18);
                            LobbyMsg_Print(*msgs, 110, lineY, 0, 18);
                        }
                        break;
                    default:
                        lineY = top + rowPad;
                        LobbyMsg_Print(*msgs, 112, lineY + 2, 1, 18);
                        LobbyMsg_Print(*msgs, 110, lineY, 0, 18);
                        break;
                    }
                }
                if (part == 22) {
                    if (i != 3) {
                        LobbyMsg_DrawIcon(i + 3, 110 - iconW, lineY, 0);
                    }
                } else if (i != 3) {
                    LobbyMsg_DrawIcon(i, 110 - iconW, lineY, 0);
                }
            }
            i++;
            top += step;
            if (i == 3) {
                top += (s32)(0.2f * step);
            }
        }
        if (part == 22) {
            msgs = LobbyMsg_GetLines(page, 24);
        } else {
            msgs = LobbyMsg_GetLines(page, 23);
        }
        for (i = 0; *msgs != NULL; msgs++, i++) {
            if (i == sel) {
                LobbyMsg_Format(page, (char *)*msgs, buf);
                LobbyMsg_Print(*msgs, 48, 350, 0, 18);
                return;
            }
        }
    }
}

/* The .data, from gNetScreenIcons (0x81A300) to the end of
 * gLobbyMsgPage5Layout: the icons, the pages' tables, the window titles and the
 * layouts of pages 4 and 5. The lines are declared first: the tables come
 * before them and point into .rodata. */
extern char gLobbyMsgPages_1_1_0[];
extern char gLobbyMsgPages_1_1_1[];
extern char gLobbyMsgPages_1_1_2[];
extern char gLobbyMsgEmptyLine[];
extern char gLobbyMsgPages_1_2_1[];
extern char gLobbyMsgPages_1_3_0[];
extern char gLobbyMsgPages_1_3_1[];
extern char gLobbyMsgPages_1_3_3[];
extern char gLobbyMsgPages_1_3_4[];
extern char gLobbyMsgPages_1_3_6[];
extern char gLobbyMsgPages_1_4_0[];
extern char gLobbyMsgPages_1_4_1[];
extern char gLobbyMsgPages_1_5_0[];
extern char gLobbyMsgPages_1_5_1[];
extern char gLobbyMsgSlotLine[];
extern char gLobbyMsgPages_1_6_1[];
extern char gLobbyMsgPages_1_7_0[];
extern char gLobbyMsgPages_1_7_1[];
extern char gLobbyMsgPages_1_7_2[];
extern char gLobbyMsgPages_1_7_4[];
extern char gLobbyMsgPages_1_9_2[];
extern char gLobbyMsgPages_1_9_3[];
extern char gLobbyMsgPages_1_10_1[];
extern char gLobbyMsgPages_1_10_2[];
extern char gLobbyMsgPages_1_11_2[];
extern char gLobbyMsgPages_1_11_3[];
extern char gLobbyMsgPages_1_12_1[];
extern char gLobbyMsgPages_1_12_2[];
extern char gLobbyMsgPages_1_13_2[];
extern char gLobbyMsgPages_1_13_4[];
extern char gLobbyMsgPages_1_13_5[];
extern char gLobbyMsgPages_1_13_6[];
extern char gLobbyMsgPages_1_13_8[];
extern char gLobbyMsgPages_1_13_9[];
extern char gLobbyMsgPages_1_14_3[];
extern char gLobbyMsgPages_1_15_1[];
extern char gLobbyMsgPages_1_15_2[];
extern char gLobbyMsgPages_1_15_5[];
extern char gLobbyMsgPages_1_16_2[];
extern char gLobbyMsgPages_1_16_4[];
extern char gLobbyMsgPages_1_17_2[];
extern char gLobbyMsgPages_1_18_2[];
extern char gLobbyMsgPages_1_19_0[];
extern char gLobbyMsgPages_1_19_1[];
extern char gLobbyMsgPages_1_20_0[];
extern char gLobbyMsgPages_1_21_0[];
extern char gLobbyMsgPages_1_23_0[];
extern char gLobbyMsgPages_1_23_1[];
extern char gLobbyMsgPages_1_23_2[];
extern char gLobbyMsgPages_1_23_4[];
extern char gLobbyMsgPages_1_23_7[];
extern char gLobbyMsgPages_1_25_0[];
extern char gLobbyMsgPages_1_25_1[];
extern char gLobbyMsgPages_1_26_2[];
extern char gLobbyMsgPages_1_27_1[];
extern char gLobbyMsgPages_1_27_2[];
extern char gLobbyMsgPages_1_30_0[];
extern char gLobbyMsgPages_1_30_1[];
extern char gLobbyMsgPages_1_30_2[];
extern char gLobbyMsgPages_1_30_3[];
extern char gLobbyMsgPages_1_31_2[];
extern char gLobbyMsgPages_1_31_3[];
extern char gLobbyMsgPages_1_32_0[];
extern char gLobbyMsgPages_1_33_1[];
extern char gLobbyMsgPages_1_34_0[];
extern char gLobbyMsgPages_1_34_1[];
extern char gLobbyMsgPages_1_34_2[];
extern char gLobbyMsgPages_1_34_3[];
extern char gLobbyMsgPages_1_35_0[];
extern char gLobbyMsgPages_1_36_0[];
extern char gLobbyMsgPages_1_36_1[];
extern char gLobbyMsgPages_1_38_0[];
extern char gLobbyMsgPages_1_38_2[];
extern char gLobbyMsgPages_1_38_3[];
extern char gLobbyMsgPages_1_38_5[];
extern char gLobbyMsgPages_1_38_6[];
extern char gLobbyMsgPages_1_38_7[];
extern char gLobbyMsgPages_1_39_0[];
extern char gLobbyMsgPages_1_40_2[];
extern char gLobbyMsgPages_1_40_9[];
extern char gLobbyMsgPages_1_41_1[];
extern char gLobbyMsgPages_1_41_2[];
extern char gLobbyMsgPages_1_41_4[];
extern char gLobbyMsgPages_1_41_5[];
extern char gLobbyMsgPages_1_42_1[];
extern char gLobbyMsgPages_1_42_2[];
extern char gLobbyMsgPages_1_43_0[];
extern char gLobbyMsgPages_1_45_0[];
extern char gLobbyMsgPages_1_45_1[];
extern char gLobbyMsgPages_1_45_2[];
extern char gLobbyMsgPages_1_46_0[];
extern char gLobbyMsgPages_1_48_2[];
extern char gLobbyMsgPages_1_48_3[];
extern char gLobbyMsgPages_1_49_3[];
extern char gLobbyMsgPages_1_50_3[];
extern char gLobbyMsgPages_1_51_0[];
extern char gLobbyMsgPages_1_51_4[];
extern char gLobbyMsgPages_1_53_3[];
extern char gLobbyMsgPages_1_55_5[];
extern char gLobbyMsgPages_0_2_0[];
extern char gLobbyMsgPages_0_3_0[];
extern char gLobbyMsgPages_0_3_1[];
extern char gLobbyMsgPages_0_10_0[];
extern char gLobbyMsgPages_0_10_1[];
extern char gLobbyMsgPages_0_10_2[];
extern char gLobbyMsgPages_0_10_3[];
extern char gLobbyMsgPages_0_10_4[];
extern char gLobbyMsgPages_0_10_5[];
extern char gLobbyMsgPages_0_11_0[];
extern char gLobbyMsgPages_0_11_1[];
extern char gLobbyMsgPages_0_11_2[];
extern char gLobbyMsgPages_0_11_3[];
extern char gLobbyMsgPages_0_11_5[];
extern char gLobbyMsgPages_0_11_6[];
extern char gLobbyMsgPages_0_13_0[];
extern char gLobbyMsgPages_0_13_1[];
extern char gLobbyMsgPages_0_14_0[];
extern char gLobbyMsgPages_0_14_1[];
extern char gLobbyMsgPages_0_14_2[];
extern char gLobbyMsgPages_0_15_1[];
extern char gLobbyMsgPages_0_16_0[];
extern char gLobbyMsgPages_0_17_0[];
extern char gLobbyMsgPages_0_17_1[];
extern char gLobbyMsgPages_0_17_2[];
extern char gLobbyMsgPages_0_18_0[];
extern char gLobbyMsgPages_0_18_1[];
extern char gLobbyMsgPages_0_19_0[];
extern char gLobbyMsgPages_0_21_0[];
extern char gLobbyMsgPages_0_21_1[];
extern char gLobbyMsgPages_0_21_2[];
extern char gLobbyMsgPages_0_22_0[];
extern char gLobbyMsgPages_0_22_1[];
extern char gLobbyMsgPages_0_22_2[];
extern char gLobbyMsgPages_0_23_0[];
extern char gLobbyMsgPages_0_23_1[];
extern char gLobbyMsgPages_0_23_2[];
extern char gLobbyMsgPages_0_23_3[];
extern char gLobbyMsgPages_0_24_0[];
extern char gLobbyMsgPages_0_24_1[];
extern char gLobbyMsgPages_0_24_2[];
extern char gLobbyMsgPages_0_24_3[];
extern char gLobbyMsgDnasNoteMark[];
extern char gLobbyMsgPages_2_4_0[];
extern char gLobbyMsgPages_2_7_0[];
extern char gLobbyMsgPages_2_8_0[];
extern char gLobbyMsgPages_2_8_1[];
extern char gLobbyMsgPages_2_8_2[];
extern char gLobbyMsgPages_2_9_0[];
extern char gLobbyMsgPages_2_11_0[];
extern char gLobbyMsgPages_2_13_0[];
extern char gLobbyMsgPages_2_14_0[];
extern char gLobbyMsgPages_2_18_0[];
extern char gLobbyMsgPages_2_20_0[];
extern char gLobbyMsgPages_2_21_0[];
extern char gLobbyMsgPages_2_22_1[];
extern char gLobbyMsgPages_2_23_0[];
extern char gLobbyMsgPages_2_26_0[];
extern char gLobbyMsgDnasNoteLine[];
extern char gLobbyMsgPages_3_1_2[];
extern char gLobbyMsgPages_3_1_3[];
extern char gLobbyMsgPages_3_1_4[];
extern char gLobbyMsgPages_3_1_6[];
extern char gLobbyMsgPages_3_1_7[];
extern char gLobbyMsgPages_3_2_1[];
extern char gLobbyMsgPages_3_3_0[];
extern char gLobbyMsgPages_3_3_1[];
extern char gLobbyMsgPages_3_4_2[];
extern char gLobbyMsgPages_3_5_2[];
extern char gLobbyMsgPages_3_5_3[];
extern char gLobbyMsgPages_3_6_2[];
extern char gLobbyMsgPages_3_7_2[];
extern char gLobbyMsgPages_3_8_2[];
extern char gLobbyMsgPages_3_9_2[];
extern char gLobbyMsgPages_3_11_2[];
extern char gLobbyMsgPages_3_12_2[];
extern char gLobbyMsgPages_3_13_0[];
extern char gLobbyMsgPages_3_13_3[];
extern char gLobbyMsgPages_3_13_4[];
extern char gLobbyMsgPages_3_14_0[];
extern char gLobbyMsgPages_3_15_2[];
extern char gLobbyMsgPages_3_15_3[];
extern char gLobbyMsgPages_4_1_2[];
extern char gLobbyMsgPages_4_1_3[];
extern char gLobbyMsgPages_4_2_4[];
extern char gLobbyMsgPages_4_2_5[];
extern char gLobbyMsgPages_4_2_6[];
extern char gLobbyMsgPages_4_3_2[];
extern char gLobbyMsgPages_4_3_3[];
extern char gLobbyMsgPages_4_3_4[];
extern char gLobbyMsgPages_4_3_5[];
extern char gLobbyMsgPages_4_4_2[];
extern char gLobbyMsgPages_4_4_4[];
extern char gLobbyMsgPages_4_4_5[];
extern char gLobbyMsgPages_4_5_2[];
extern char gLobbyMsgPages_4_6_2[];
extern char gLobbyMsgPages_4_6_3[];
extern char gLobbyMsgPages_4_7_2[];
extern char gLobbyMsgPages_4_7_4[];
extern char gLobbyMsgPages_4_7_5[];
extern char gLobbyMsgPages_4_7_6[];
extern char gLobbyMsgPages_4_8_2[];
extern char gLobbyMsgPages_4_8_3[];
extern char gLobbyMsgPages_4_8_4[];
extern char gLobbyMsgPages_4_9_2[];
extern char gLobbyMsgPages_4_9_3[];
extern char gLobbyMsgPages_4_9_6[];
extern char gLobbyMsgPages_4_9_7[];
extern char gLobbyMsgPages_4_9_8[];
extern char gLobbyMsgPages_4_10_1[];
extern char gLobbyMsgPages_4_11_0[];
extern char gLobbyMsgPages_4_12_0[];
extern char gLobbyMsgPages_4_13_1[];
extern char gLobbyMsgPages_4_13_2[];
extern char gLobbyMsgPages_4_14_0[];
extern char gLobbyMsgPages_4_14_1[];
extern char gLobbyMsgPages_4_14_2[];
extern char gLobbyMsgPages_4_15_2[];
extern char gLobbyMsgPages_4_15_3[];
extern char gLobbyMsgPages_4_15_4[];
extern char gLobbyMsgPages_4_15_5[];
extern char gLobbyMsgPages_4_15_6[];
extern char gLobbyMsgPages_4_15_7[];
extern char gLobbyMsgPages_4_16_2[];
extern char gLobbyMsgPages_4_16_3[];
extern char gLobbyMsgPages_4_16_4[];
extern char gLobbyMsgPages_4_16_6[];
extern char gLobbyMsgPages_4_16_7[];
extern char gLobbyMsgPages_4_17_3[];
extern char gLobbyMsgPages_4_17_4[];
extern char gLobbyMsgPages_5_1_1[];
extern char gLobbyMsgPages_5_2_3[];
extern char gLobbyMsgPages_5_3_2[];
extern char gLobbyMsgPages_5_4_2[];
extern char gLobbyMsgPages_5_4_6[];
extern char gLobbyMsgPages_5_5_3[];
extern char gLobbyMsgPages_5_6_2[];
extern char gLobbyMsgPages_6_3_0[];
extern char gLobbyMsgPages_6_4_0[];
extern char gNetModeTitleTexts_1[];
extern char gNetModeTitleTexts_2[];
extern char gNetModeTitleTexts_3[];
extern char gNetModeTitleTexts_4[];
extern char gNetModeTitleTexts_5[];
extern char gNetModeTitleTexts_6[];
extern char gNetModeTitleTexts_7[];
extern char *gLobbyMsgPages_0_5[];
extern char *gLobbyMsgPages_0_6[];
extern char *gLobbyMsgPages_0_7[];
extern char *gLobbyMsgPages_0_8[];
extern char *gLobbyMsgPages_0_9[];
extern char *gLobbyMsgPages_0_12[];
extern char **gLobbyMsgPages_0[];
extern char **gLobbyMsgPages_1[];
extern char **gLobbyMsgPages_2[];
extern char **gLobbyMsgPages_3[];
extern char **gLobbyMsgPages_4[];
extern char **gLobbyMsgPages_5[];
extern char **gLobbyMsgPages_6[];
NetIcon gNetScreenIcons[19] = {
#include "lobby/0081A300.inc"
};
char ***gLobbyMsgPages[8] = {
    gLobbyMsgPages_0, gLobbyMsgPages_1, gLobbyMsgPages_2, gLobbyMsgPages_3,
    gLobbyMsgPages_4, gLobbyMsgPages_5, gLobbyMsgPages_6, NULL,
};
char *gLobbyMsgPages_1_1[4] = {
    gLobbyMsgPages_1_1_0, gLobbyMsgPages_1_1_1, gLobbyMsgPages_1_1_2, NULL,
};
char *gLobbyMsgPages_1_2[4] = {
    gLobbyMsgEmptyLine, gLobbyMsgPages_1_2_1, NULL, NULL,
};
char *gLobbyMsgPages_1_3[8] = {
    gLobbyMsgPages_1_3_0, gLobbyMsgPages_1_3_1, gLobbyMsgEmptyLine, gLobbyMsgPages_1_3_3,
    gLobbyMsgPages_1_3_4, gLobbyMsgEmptyLine, gLobbyMsgPages_1_3_6, NULL,
};
char *gLobbyMsgPages_1_4[4] = {
    gLobbyMsgPages_1_4_0, gLobbyMsgPages_1_4_1, NULL, NULL,
};
char *gLobbyMsgPages_1_5[4] = {
    gLobbyMsgPages_1_5_0, gLobbyMsgPages_1_5_1, NULL, NULL,
};
char *gLobbyMsgPages_1_6[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_6_1, NULL, NULL,
};
char *gLobbyMsgPages_1_7[8] = {
    gLobbyMsgPages_1_7_0, gLobbyMsgPages_1_7_1, gLobbyMsgPages_1_7_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_7_4, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_8[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_7_1, gLobbyMsgPages_1_7_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_7_4, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_9[8] = {
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgPages_1_9_2, gLobbyMsgPages_1_9_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_10[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_10_1, gLobbyMsgPages_1_10_2, NULL,
};
char *gLobbyMsgPages_1_11[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_10_1, gLobbyMsgPages_1_11_2, gLobbyMsgPages_1_11_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_12[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_1_12_2, NULL,
};
char *gLobbyMsgPages_1_13[12] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_1_13_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_13_4, gLobbyMsgPages_1_13_5, gLobbyMsgPages_1_13_6, gLobbyMsgSlotLine,
    gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_13_9, NULL, NULL,
};
char *gLobbyMsgPages_1_14[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_1_11_2, gLobbyMsgPages_1_14_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_15[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_15_1, gLobbyMsgPages_1_15_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_3_3, gLobbyMsgPages_1_15_5, NULL, NULL,
};
char *gLobbyMsgPages_1_16[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_15_1, gLobbyMsgPages_1_16_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_16_4, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_17[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_17_2, NULL,
};
char *gLobbyMsgPages_1_18[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_18_2, NULL,
};
char *gLobbyMsgPages_1_19[4] = {
    gLobbyMsgPages_1_19_0, gLobbyMsgPages_1_19_1, NULL, NULL,
};
char *gLobbyMsgPages_1_20[2] = {
    gLobbyMsgPages_1_20_0, NULL,
};
char *gLobbyMsgPages_1_21[2] = {
    gLobbyMsgPages_1_21_0, NULL,
};
char *gLobbyMsgPages_1_22[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_1_13_2, NULL,
};
char *gLobbyMsgPages_1_23[12] = {
    gLobbyMsgPages_1_23_0, gLobbyMsgPages_1_23_1, gLobbyMsgPages_1_23_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_23_4, gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_23_0, gLobbyMsgPages_1_23_7,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_24[10] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_7_1, gLobbyMsgPages_1_7_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_23_4, gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_23_0, gLobbyMsgPages_1_23_7,
    NULL, NULL,
};
char *gLobbyMsgPages_1_25[3] = {
    gLobbyMsgPages_1_25_0, gLobbyMsgPages_1_25_1, NULL,
};
char *gLobbyMsgPages_1_26[4] = {
    gLobbyMsgPages_1_23_0, gLobbyMsgPages_1_23_1, gLobbyMsgPages_1_26_2, NULL,
};
char *gLobbyMsgPages_1_27[4] = {
    gLobbyMsgPages_1_7_0, gLobbyMsgPages_1_27_1, gLobbyMsgPages_1_27_2, NULL,
};
char *gLobbyMsgPages_1_28[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_27_1, gLobbyMsgPages_1_27_2, NULL,
};
char *gLobbyMsgPages_1_29[4] = {
    gLobbyMsgPages_1_4_0, gLobbyMsgPages_1_4_1, NULL, NULL,
};
char *gLobbyMsgPages_1_30[8] = {
    gLobbyMsgPages_1_30_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_30_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_31[6] = {
    gLobbyMsgPages_1_30_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_31_2, gLobbyMsgPages_1_31_3,
    NULL, NULL,
};
char *gLobbyMsgPages_1_32[3] = {
    gLobbyMsgPages_1_32_0, gLobbyMsgPages_1_5_1, NULL,
};
char *gLobbyMsgPages_1_33[3] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_33_1, NULL,
};
char *gLobbyMsgPages_1_34[6] = {
    gLobbyMsgPages_1_34_0, gLobbyMsgPages_1_34_1, gLobbyMsgPages_1_34_2, gLobbyMsgPages_1_34_3,
    NULL, NULL,
};
char *gLobbyMsgPages_1_35[3] = {
    gLobbyMsgPages_1_35_0, gLobbyMsgPages_1_9_3, NULL,
};
char *gLobbyMsgPages_1_36[3] = {
    gLobbyMsgPages_1_36_0, gLobbyMsgPages_1_36_1, NULL,
};
char *gLobbyMsgPages_1_37[3] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_36_1, NULL,
};
char *gLobbyMsgPages_1_38[10] = {
    gLobbyMsgPages_1_38_0, gLobbyMsgEmptyLine, gLobbyMsgPages_1_38_2, gLobbyMsgPages_1_38_3,
    gLobbyMsgEmptyLine, gLobbyMsgPages_1_38_5, gLobbyMsgPages_1_38_6, gLobbyMsgPages_1_38_7,
    NULL, NULL,
};
char *gLobbyMsgPages_1_39[2] = {
    gLobbyMsgPages_1_39_0, NULL,
};
char *gLobbyMsgPages_1_40[12] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_10_1, gLobbyMsgPages_1_40_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_13_4, gLobbyMsgPages_1_13_5, gLobbyMsgPages_1_13_6, gLobbyMsgSlotLine,
    gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_40_9, NULL, NULL,
};
char *gLobbyMsgPages_1_41[8] = {
    gLobbyMsgEmptyLine, gLobbyMsgPages_1_41_1, gLobbyMsgPages_1_41_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_41_4, gLobbyMsgPages_1_41_5, NULL, NULL,
};
char *gLobbyMsgPages_1_42[4] = {
    gLobbyMsgPages_1_30_0, gLobbyMsgPages_1_42_1, gLobbyMsgPages_1_42_2, NULL,
};
char *gLobbyMsgPages_1_43[8] = {
    gLobbyMsgPages_1_43_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_30_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_44[8] = {
    gLobbyMsgPages_1_43_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_31_2, gLobbyMsgPages_1_31_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_45[4] = {
    gLobbyMsgPages_1_45_0, gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_45_2, NULL,
};
char *gLobbyMsgPages_1_46[4] = {
    gLobbyMsgPages_1_46_0, gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_45_2, NULL,
};
char *gLobbyMsgPages_1_47[4] = {
    gLobbyMsgPages_1_19_0, gLobbyMsgPages_1_4_1, NULL, NULL,
};
char *gLobbyMsgPages_1_48[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_7_1, gLobbyMsgPages_1_48_2, gLobbyMsgPages_1_48_3,
    NULL, NULL, NULL, NULL,
};
extern char gLobbyMsgPages_1_49_0[];
extern char gLobbyMsgPages_1_53_0[];
extern char gLobbyMsgPages_0_1_0[];
extern char gLobbyMsgPages_0_4_0[];
extern char gLobbyMsgPages_2_1_0[];
extern char gLobbyMsgPages_2_3_0[];
extern char gLobbyMsgPages_2_5_0[];
extern char gLobbyMsgPages_2_6_0[];
extern char gLobbyMsgPages_2_10_0[];
extern char gLobbyMsgPages_2_12_0[];
extern char gLobbyMsgPages_2_15_0[];
extern char gLobbyMsgPages_2_16_0[];
extern char gLobbyMsgPages_2_17_0[];
extern char gLobbyMsgPages_2_19_0[];
extern char gLobbyMsgPages_2_22_0[];
extern char gLobbyMsgPages_2_24_0[];
extern char gLobbyMsgPages_3_1_0[];
extern char gLobbyMsgPages_3_2_0[];
extern char gLobbyMsgPages_3_5_0[];
extern char gLobbyMsgPages_4_9_0[];
extern char gLobbyMsgPages_4_9_1[];
extern char gLobbyMsgPages_4_10_0[];
extern char gLobbyMsgPages_6_2_0[];
extern char gLobbyMsgPages_6_5_0[];
extern char gLobbyMsgPages_6_6_0[];
char *gLobbyMsgPages_1_49[8] = {
    gLobbyMsgPages_1_49_0, gLobbyMsgPages_1_23_2, gLobbyMsgEmptyLine, gLobbyMsgPages_1_49_3,
    gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_45_2, NULL, NULL,
};
char *gLobbyMsgPages_1_50[8] = {
    gLobbyMsgPages_1_49_0, gLobbyMsgPages_1_23_2, gLobbyMsgEmptyLine, gLobbyMsgPages_1_50_3,
    gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_30_3, NULL,
};
char *gLobbyMsgPages_1_51[8] = {
    gLobbyMsgPages_1_51_0, gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_34_1, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_51_4, gLobbyMsgPages_1_26_2, NULL, NULL,
};
char *gLobbyMsgPages_1_52[12] = {
    gLobbyMsgPages_1_51_0, gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_34_1, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_50_3, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_30_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_53[8] = {
    gLobbyMsgPages_1_53_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_53_3,
    gLobbyMsgEmptyLine, gLobbyMsgPages_1_51_4, gLobbyMsgPages_1_26_2, NULL,
};
char *gLobbyMsgPages_1_54[12] = {
    gLobbyMsgPages_1_53_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_53_3,
    gLobbyMsgEmptyLine, gLobbyMsgPages_1_49_3, gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_45_2,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_55[8] = {
    gLobbyMsgPages_1_49_0, gLobbyMsgPages_1_23_2, gLobbyMsgEmptyLine, gLobbyMsgPages_1_50_3,
    gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_55_5, NULL, NULL,
};
char *gLobbyMsgPages_1_56[8] = {
    gLobbyMsgPages_1_51_0, gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_34_1, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_50_3, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_55_5, NULL,
};
char *gLobbyMsgPages_1_57[8] = {
    gLobbyMsgPages_1_53_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_31_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_51_4, gLobbyMsgPages_1_26_2, NULL, NULL,
};
char *gLobbyMsgPages_1_58[12] = {
    gLobbyMsgPages_1_53_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_31_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_50_3, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_30_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_59[8] = {
    gLobbyMsgPages_1_53_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_31_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_1_49_3, gLobbyMsgPages_1_45_1, gLobbyMsgPages_1_45_2, NULL,
};
char *gLobbyMsgPages_1_60[12] = {
    gLobbyMsgPages_1_53_0, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_30_2, gLobbyMsgPages_1_53_3,
    gLobbyMsgEmptyLine, gLobbyMsgPages_1_50_3, gLobbyMsgPages_1_30_1, gLobbyMsgPages_1_55_5,
    NULL, NULL, NULL, NULL,
};
char **gLobbyMsgPages_1[62] = {
    NULL, gLobbyMsgPages_1_1, gLobbyMsgPages_1_2, gLobbyMsgPages_1_3,
    gLobbyMsgPages_1_4, gLobbyMsgPages_1_5, gLobbyMsgPages_1_6, gLobbyMsgPages_1_7,
    gLobbyMsgPages_1_8, gLobbyMsgPages_1_9, gLobbyMsgPages_1_10, gLobbyMsgPages_1_11,
    gLobbyMsgPages_1_12, gLobbyMsgPages_1_13, gLobbyMsgPages_1_14, gLobbyMsgPages_1_15,
    gLobbyMsgPages_1_16, gLobbyMsgPages_1_17, gLobbyMsgPages_1_18, gLobbyMsgPages_1_19,
    gLobbyMsgPages_1_20, gLobbyMsgPages_1_21, gLobbyMsgPages_1_22, gLobbyMsgPages_1_23,
    gLobbyMsgPages_1_24, gLobbyMsgPages_1_25, gLobbyMsgPages_1_26, gLobbyMsgPages_1_27,
    gLobbyMsgPages_1_28, gLobbyMsgPages_1_29, gLobbyMsgPages_1_30, gLobbyMsgPages_1_31,
    gLobbyMsgPages_1_32, gLobbyMsgPages_1_33, gLobbyMsgPages_1_34, gLobbyMsgPages_1_35,
    gLobbyMsgPages_1_36, gLobbyMsgPages_1_37, gLobbyMsgPages_1_38, gLobbyMsgPages_1_39,
    gLobbyMsgPages_1_40, gLobbyMsgPages_1_41, gLobbyMsgPages_1_42, gLobbyMsgPages_1_43,
    gLobbyMsgPages_1_44, gLobbyMsgPages_1_45, gLobbyMsgPages_1_46, gLobbyMsgPages_1_47,
    gLobbyMsgPages_1_48, gLobbyMsgPages_1_49, gLobbyMsgPages_1_50, gLobbyMsgPages_1_51,
    gLobbyMsgPages_1_52, gLobbyMsgPages_1_53, gLobbyMsgPages_1_54, gLobbyMsgPages_1_55,
    gLobbyMsgPages_1_56, gLobbyMsgPages_1_57, gLobbyMsgPages_1_58, gLobbyMsgPages_1_59,
    gLobbyMsgPages_1_60, NULL,
};
char *gLobbyMsgPages_0_1[2] = {
    gLobbyMsgPages_0_1_0, NULL,
};
char *gLobbyMsgPages_0_2[2] = {
    gLobbyMsgPages_0_2_0, NULL,
};
char *gLobbyMsgPages_0_3[3] = {
    gLobbyMsgPages_0_3_0, gLobbyMsgPages_0_3_1, NULL,
};
char *gLobbyMsgPages_0_4[2] = {
    gLobbyMsgPages_0_4_0, NULL,
};
char *gLobbyMsgPages_0_10[8] = {
    gLobbyMsgPages_0_10_0, gLobbyMsgPages_0_10_1, gLobbyMsgPages_0_10_2, gLobbyMsgPages_0_10_3,
    gLobbyMsgPages_0_10_4, gLobbyMsgPages_0_10_5, NULL, NULL,
};
char *gLobbyMsgPages_0_11[8] = {
    gLobbyMsgPages_0_11_0, gLobbyMsgPages_0_11_1, gLobbyMsgPages_0_11_2, gLobbyMsgPages_0_11_3,
    gLobbyMsgEmptyLine, gLobbyMsgPages_0_11_5, gLobbyMsgPages_0_11_6, NULL,
};
char *gLobbyMsgPages_0_13[4] = {
    gLobbyMsgPages_0_13_0, gLobbyMsgPages_0_13_1, NULL, NULL,
};
char *gLobbyMsgPages_0_14[4] = {
    gLobbyMsgPages_0_14_0, gLobbyMsgPages_0_14_1, gLobbyMsgPages_0_14_2, NULL,
};
char *gLobbyMsgPages_0_15[4] = {
    gLobbyMsgPages_0_13_0, gLobbyMsgPages_0_15_1, gLobbyMsgPages_0_13_1, NULL,
};
char *gLobbyMsgPages_0_16[4] = {
    gLobbyMsgPages_0_16_0, gLobbyMsgPages_0_13_1, NULL, NULL,
};
char *gLobbyMsgPages_0_17[4] = {
    gLobbyMsgPages_0_17_0, gLobbyMsgPages_0_17_1, gLobbyMsgPages_0_17_2, NULL,
};
char *gLobbyMsgPages_0_18[4] = {
    gLobbyMsgPages_0_18_0, gLobbyMsgPages_0_18_1, NULL, NULL,
};
char *gLobbyMsgPages_0_19[2] = {
    gLobbyMsgPages_0_19_0, NULL,
};
char *gLobbyMsgPages_0_20[2] = {
    gLobbyMsgPages_1_20_0, NULL,
};
char *gLobbyMsgPages_0_21[8] = {
    gLobbyMsgPages_0_21_0, gLobbyMsgPages_0_21_1, gLobbyMsgPages_0_21_2, gLobbyMsgPages_0_13_1,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_0_22[8] = {
    gLobbyMsgPages_0_22_0, gLobbyMsgPages_0_22_1, gLobbyMsgPages_0_22_2, gLobbyMsgPages_0_13_1,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_0_23[8] = {
    gLobbyMsgPages_0_23_0, gLobbyMsgPages_0_23_1, gLobbyMsgPages_0_23_2, gLobbyMsgPages_0_23_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_0_24[8] = {
    gLobbyMsgPages_0_24_0, gLobbyMsgPages_0_24_1, gLobbyMsgPages_0_24_2, gLobbyMsgPages_0_24_3,
    NULL, NULL, NULL, NULL,
};
char **gLobbyMsgPages_0[26] = {
    NULL, gLobbyMsgPages_0_1, gLobbyMsgPages_0_2, gLobbyMsgPages_0_3,
    gLobbyMsgPages_0_4, gLobbyMsgPages_0_5, gLobbyMsgPages_0_6, gLobbyMsgPages_0_7,
    gLobbyMsgPages_0_8, gLobbyMsgPages_0_9, gLobbyMsgPages_0_10, gLobbyMsgPages_0_11,
    gLobbyMsgPages_0_12, gLobbyMsgPages_0_13, gLobbyMsgPages_0_14, gLobbyMsgPages_0_15,
    gLobbyMsgPages_0_16, gLobbyMsgPages_0_17, gLobbyMsgPages_0_18, gLobbyMsgPages_0_19,
    gLobbyMsgPages_0_20, gLobbyMsgPages_0_21, gLobbyMsgPages_0_22, gLobbyMsgPages_0_23,
    gLobbyMsgPages_0_24, NULL,
};
char *gLobbyMsgPages_2_1[2] = {
    gLobbyMsgPages_2_1_0, NULL,
};
char *gLobbyMsgPages_2_2[4] = {
    gLobbyMsgPages_0_4_0, NULL, NULL, NULL,
};
char *gLobbyMsgPages_2_3[10] = {
    gLobbyMsgPages_2_3_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark,
    NULL, NULL,
};
char *gLobbyMsgPages_2_4[2] = {
    gLobbyMsgPages_2_4_0, NULL,
};
char *gLobbyMsgPages_2_5[2] = {
    gLobbyMsgPages_2_5_0, NULL,
};
char *gLobbyMsgPages_2_6[2] = {
    gLobbyMsgPages_2_6_0, NULL,
};
char *gLobbyMsgPages_2_7[4] = {
    gLobbyMsgPages_2_7_0, NULL, NULL, NULL,
};
char *gLobbyMsgPages_2_8[8] = {
    gLobbyMsgPages_2_8_0, gLobbyMsgPages_2_8_1, gLobbyMsgPages_2_8_2, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_9[8] = {
    gLobbyMsgPages_2_9_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_10[8] = {
    gLobbyMsgPages_2_10_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_11[8] = {
    gLobbyMsgPages_2_11_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_12[8] = {
    gLobbyMsgPages_2_12_0, gLobbyMsgPages_2_8_2, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_13[8] = {
    gLobbyMsgPages_2_13_0, gLobbyMsgPages_2_8_2, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_14[8] = {
    gLobbyMsgPages_2_14_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_15[8] = {
    gLobbyMsgPages_2_15_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_16[8] = {
    gLobbyMsgPages_2_16_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_17[8] = {
    gLobbyMsgPages_2_17_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_18[8] = {
    gLobbyMsgPages_2_18_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_19[8] = {
    gLobbyMsgPages_2_19_0, gLobbyMsgPages_2_8_2, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_20[8] = {
    gLobbyMsgPages_2_20_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_21[8] = {
    gLobbyMsgPages_2_21_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_22[8] = {
    gLobbyMsgPages_2_22_0, gLobbyMsgPages_2_22_1, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_23[8] = {
    gLobbyMsgPages_2_23_0, gLobbyMsgPages_2_22_1, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_24[8] = {
    gLobbyMsgPages_2_24_0, gLobbyMsgPages_2_22_1, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_25[8] = {
    gLobbyMsgPages_2_21_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_26[8] = {
    gLobbyMsgPages_2_26_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgPages_2_27[8] = {
    gLobbyMsgPages_2_14_0, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgEmptyLine,
    gLobbyMsgEmptyLine, gLobbyMsgEmptyLine, gLobbyMsgDnasNoteMark, NULL,
};
char *gLobbyMsgDnasNote[4] = {
    gLobbyMsgDnasNoteLine, NULL, NULL, NULL,
};
char **gLobbyMsgPages_2[28] = {
    NULL, gLobbyMsgPages_2_1, gLobbyMsgPages_2_2, gLobbyMsgPages_2_3,
    gLobbyMsgPages_2_4, gLobbyMsgPages_2_5, gLobbyMsgPages_2_6, gLobbyMsgPages_2_7,
    gLobbyMsgPages_2_8, gLobbyMsgPages_2_9, gLobbyMsgPages_2_10, gLobbyMsgPages_2_11,
    gLobbyMsgPages_2_12, gLobbyMsgPages_2_13, gLobbyMsgPages_2_14, gLobbyMsgPages_2_15,
    gLobbyMsgPages_2_16, gLobbyMsgPages_2_17, gLobbyMsgPages_2_18, gLobbyMsgPages_2_19,
    gLobbyMsgPages_2_20, gLobbyMsgPages_2_21, gLobbyMsgPages_2_22, gLobbyMsgPages_2_23,
    gLobbyMsgPages_2_24, gLobbyMsgPages_2_25, gLobbyMsgPages_2_26, gLobbyMsgPages_2_27,
};
char* *gLobbyMsgDnasNotePart = gLobbyMsgDnasNote;
char *gLobbyMsgPages_3_1[10] = {
    gLobbyMsgPages_3_1_0, gLobbyMsgEmptyLine, gLobbyMsgPages_3_1_2, gLobbyMsgPages_3_1_3,
    gLobbyMsgPages_3_1_4, gLobbyMsgEmptyLine, gLobbyMsgPages_3_1_6, gLobbyMsgPages_3_1_7,
    NULL, NULL,
};
char *gLobbyMsgPages_3_2[3] = {
    gLobbyMsgPages_3_2_0, gLobbyMsgPages_3_2_1, NULL,
};
char *gLobbyMsgPages_3_3[3] = {
    gLobbyMsgPages_3_3_0, gLobbyMsgPages_3_3_1, NULL,
};
char *gLobbyMsgPages_3_4[4] = {
    gLobbyMsgPages_1_1_0, gLobbyMsgPages_1_1_1, gLobbyMsgPages_3_4_2, NULL,
};
char *gLobbyMsgPages_3_5[8] = {
    gLobbyMsgPages_3_5_0, gLobbyMsgPages_3_1_6, gLobbyMsgPages_3_5_2, gLobbyMsgPages_3_5_3,
    gLobbyMsgEmptyLine, gLobbyMsgPages_3_3_0, gLobbyMsgPages_3_3_1, NULL,
};
char *gLobbyMsgPages_3_6[4] = {
    gLobbyMsgPages_1_23_0, gLobbyMsgPages_1_23_1, gLobbyMsgPages_3_6_2, NULL,
};
char *gLobbyMsgPages_3_7[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_7_1, gLobbyMsgPages_3_7_2, NULL,
};
char *gLobbyMsgPages_3_8[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_7_1, gLobbyMsgPages_3_8_2, gLobbyMsgPages_3_7_2,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_3_9[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_23_1, gLobbyMsgPages_3_9_2, NULL,
};
char *gLobbyMsgPages_3_10[4] = {
    gLobbyMsgPages_1_19_0, gLobbyMsgPages_1_4_1, NULL, NULL,
};
char *gLobbyMsgPages_3_11[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_3_11_2, NULL,
};
char *gLobbyMsgPages_3_12[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_3_12_2, NULL,
};
char *gLobbyMsgPages_3_13[6] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_3_13_3,
    gLobbyMsgPages_3_13_4, NULL,
};
char *gLobbyMsgPages_3_14[2] = {
    gLobbyMsgPages_3_14_0, NULL,
};
char *gLobbyMsgPages_3_15[4] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgSlotLine, gLobbyMsgPages_3_15_2, gLobbyMsgPages_3_15_3,
};
char **gLobbyMsgPages_3[16] = {
    NULL, gLobbyMsgPages_3_1, gLobbyMsgPages_3_2, gLobbyMsgPages_3_3,
    gLobbyMsgPages_3_4, gLobbyMsgPages_3_5, gLobbyMsgPages_3_6, gLobbyMsgPages_3_7,
    gLobbyMsgPages_3_8, gLobbyMsgPages_3_9, gLobbyMsgPages_3_10, gLobbyMsgPages_3_11,
    gLobbyMsgPages_3_12, gLobbyMsgPages_3_13, gLobbyMsgPages_3_14, gLobbyMsgPages_3_15,
};
char *gLobbyMsgPages_4_1[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_1_2, gLobbyMsgPages_4_1_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_2[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_1_2, gLobbyMsgPages_4_1_3,
    gLobbyMsgPages_4_2_4, gLobbyMsgPages_4_2_5, gLobbyMsgPages_4_2_6, NULL,
};
char *gLobbyMsgPages_4_3[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_3_2, gLobbyMsgPages_4_3_3,
    gLobbyMsgPages_4_3_4, gLobbyMsgPages_4_3_5, NULL, NULL,
};
char *gLobbyMsgPages_4_4[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_4_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_4_4_4, gLobbyMsgPages_4_4_5, NULL, NULL,
};
char *gLobbyMsgPages_4_5[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_5_2, gLobbyMsgPages_4_4_2,
    gLobbyMsgEmptyLine, gLobbyMsgPages_4_4_4, gLobbyMsgPages_4_4_5, NULL,
};
char *gLobbyMsgPages_4_6[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_6_2, gLobbyMsgPages_4_6_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_7[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_7_2, gLobbyMsgEmptyLine,
    gLobbyMsgPages_4_7_4, gLobbyMsgPages_4_7_5, gLobbyMsgPages_4_7_6, NULL,
};
char *gLobbyMsgPages_4_8[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_8_2, gLobbyMsgPages_4_8_3,
    gLobbyMsgPages_4_8_4, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_9[10] = {
    gLobbyMsgPages_4_9_0, gLobbyMsgPages_4_9_1, gLobbyMsgPages_4_9_2, gLobbyMsgPages_4_9_3,
    gLobbyMsgEmptyLine, gLobbyMsgPages_4_2_5, gLobbyMsgPages_4_9_6, gLobbyMsgPages_4_9_7,
    gLobbyMsgPages_4_9_8, NULL,
};
char *gLobbyMsgPages_4_10[3] = {
    gLobbyMsgPages_4_10_0, gLobbyMsgPages_4_10_1, NULL,
};
char *gLobbyMsgPages_4_11[3] = {
    gLobbyMsgPages_4_11_0, gLobbyMsgPages_4_10_1, NULL,
};
char *gLobbyMsgPages_4_12[2] = {
    gLobbyMsgPages_4_12_0, NULL,
};
char *gLobbyMsgPages_4_13[4] = {
    gLobbyMsgPages_4_9_1, gLobbyMsgPages_4_13_1, gLobbyMsgPages_4_13_2, NULL,
};
char *gLobbyMsgPages_4_14[4] = {
    gLobbyMsgPages_4_14_0, gLobbyMsgPages_4_14_1, gLobbyMsgPages_4_14_2, NULL,
};
char *gLobbyMsgPages_4_15[12] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_15_2, gLobbyMsgPages_4_15_3,
    gLobbyMsgPages_4_15_4, gLobbyMsgPages_4_15_5, gLobbyMsgPages_4_15_6, gLobbyMsgPages_4_15_7,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_16[12] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_16_2, gLobbyMsgPages_4_16_3,
    gLobbyMsgPages_4_16_4, gLobbyMsgEmptyLine, gLobbyMsgPages_4_16_6, gLobbyMsgPages_4_16_7,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_17[8] = {
    gLobbyMsgPages_3_13_0, gLobbyMsgEmptyLine, gLobbyMsgPages_4_16_2, gLobbyMsgPages_4_17_3,
    gLobbyMsgPages_4_17_4, NULL, NULL, NULL,
};
char **gLobbyMsgPages_4[20] = {
    NULL, gLobbyMsgPages_4_1, gLobbyMsgPages_4_2, gLobbyMsgPages_4_3,
    gLobbyMsgPages_4_4, gLobbyMsgPages_4_5, gLobbyMsgPages_4_6, gLobbyMsgPages_4_7,
    gLobbyMsgPages_4_8, gLobbyMsgPages_4_9, gLobbyMsgPages_4_10, gLobbyMsgPages_4_11,
    gLobbyMsgPages_4_12, gLobbyMsgPages_4_13, gLobbyMsgPages_4_14, gLobbyMsgPages_4_15,
    gLobbyMsgPages_4_16, gLobbyMsgPages_4_17, NULL, NULL,
};
char *gLobbyMsgPages_5_1[8] = {
    gLobbyMsgPages_1_19_0, gLobbyMsgPages_5_1_1, gLobbyMsgPages_1_1_0, gLobbyMsgPages_1_1_1,
    gLobbyMsgPages_3_4_2, NULL, NULL, NULL,
};
char *gLobbyMsgPages_5_2[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_1_11_2, gLobbyMsgPages_5_2_3,
    gLobbyMsgPages_1_1_0, gLobbyMsgPages_1_1_1, gLobbyMsgPages_3_4_2, NULL,
};
char *gLobbyMsgPages_5_3[4] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_5_3_2, NULL,
};
char *gLobbyMsgPages_5_4[8] = {
    gLobbyMsgPages_1_23_0, gLobbyMsgPages_1_23_1, gLobbyMsgPages_5_4_2, gLobbyMsgPages_1_23_4,
    gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_23_0, gLobbyMsgPages_5_4_6, NULL,
};
char *gLobbyMsgPages_5_5[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_12_1, gLobbyMsgPages_1_11_2, gLobbyMsgPages_5_5_3,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_5_6[8] = {
    gLobbyMsgSlotLine, gLobbyMsgPages_1_7_1, gLobbyMsgPages_5_6_2, gLobbyMsgPages_1_23_4,
    gLobbyMsgPages_1_13_8, gLobbyMsgPages_1_23_0, gLobbyMsgPages_5_4_6, NULL,
};
char **gLobbyMsgPages_5[8] = {
    NULL, gLobbyMsgPages_5_1, gLobbyMsgPages_5_2, gLobbyMsgPages_5_3,
    gLobbyMsgPages_5_4, gLobbyMsgPages_5_5, gLobbyMsgPages_5_6, NULL,
};
char *gLobbyMsgPages_6_1[2] = {
    gLobbyMsgPages_1_21_0, NULL,
};
char *gLobbyMsgPages_6_2[2] = {
    gLobbyMsgPages_6_2_0, NULL,
};
char *gLobbyMsgPages_6_3[2] = {
    gLobbyMsgPages_6_3_0, NULL,
};
char *gLobbyMsgPages_6_4[2] = {
    gLobbyMsgPages_6_4_0, NULL,
};
char *gLobbyMsgPages_6_5[2] = {
    gLobbyMsgPages_6_5_0, NULL,
};
char *gLobbyMsgPages_6_6[2] = {
    gLobbyMsgPages_6_6_0, NULL,
};
char **gLobbyMsgPages_6[8] = {
    NULL, gLobbyMsgPages_6_1, gLobbyMsgPages_6_2, gLobbyMsgPages_6_3,
    gLobbyMsgPages_6_4, gLobbyMsgPages_6_5, gLobbyMsgPages_6_6, NULL,
};
char *gNetModeTitleTexts[8] = {
    gLobbyMsgEmptyLine, gNetModeTitleTexts_1, gNetModeTitleTexts_2, gNetModeTitleTexts_3,
    gNetModeTitleTexts_4, gNetModeTitleTexts_5, gNetModeTitleTexts_6, gNetModeTitleTexts_7,
};
s16 gLobbyMsgPage4Layout[16][3][2] = {
#include "lobby/0081B530.inc"
};
s16 gLobbyMsgPage5Layout[8][3][2] = {
#include "lobby/0081B5F0.inc"
};

/* gLobbyMsgPages' lines and the window titles, from
 * gLobbyMsgPages_1_1_0 (0x847370), Shift-JIS strings in .rodata in the
 * game's order; the functions' own constants come before them in the same
 * piece. */
#pragma rodata on
char gLobbyMsgPages_1_1_0[48] = {
#include "lobby/00847370.inc"
};
char gLobbyMsgPages_1_1_1[48] = {
#include "lobby/008473A0.inc"
};
char gLobbyMsgPages_1_1_2[40] = {
#include "lobby/008473D0.inc"
};
#pragma explicit_zero_data on
char gLobbyMsgEmptyLine[8] = {
#include "lobby/008473F8.inc"
};
#pragma explicit_zero_data reset
char gLobbyMsgPages_1_2_1[32] = {
#include "lobby/00847400.inc"
};
char gLobbyMsgPages_1_3_0[32] = {
#include "lobby/00847420.inc"
};
char gLobbyMsgPages_1_3_1[48] = {
#include "lobby/00847440.inc"
};
char gLobbyMsgPages_1_3_3[32] = {
#include "lobby/00847470.inc"
};
char gLobbyMsgPages_1_3_4[48] = {
#include "lobby/00847490.inc"
};
char gLobbyMsgPages_1_3_6[48] = {
#include "lobby/008474C0.inc"
};
char gLobbyMsgPages_1_4_0[32] = {
#include "lobby/008474F0.inc"
};
char gLobbyMsgPages_1_4_1[32] = {
#include "lobby/00847510.inc"
};
char gLobbyMsgPages_1_5_0[32] = {
#include "lobby/00847530.inc"
};
char gLobbyMsgPages_1_5_1[48] = {
#include "lobby/00847550.inc"
};
char gLobbyMsgSlotLine[32] = {
#include "lobby/00847580.inc"
};
char gLobbyMsgPages_1_6_1[48] = {
#include "lobby/008475A0.inc"
};
char gLobbyMsgPages_1_7_0[32] = {
#include "lobby/008475D0.inc"
};
char gLobbyMsgPages_1_7_1[32] = {
#include "lobby/008475F0.inc"
};
char gLobbyMsgPages_1_7_2[32] = {
#include "lobby/00847610.inc"
};
char gLobbyMsgPages_1_7_4[48] = {
#include "lobby/00847630.inc"
};
char gLobbyMsgPages_1_9_2[32] = {
#include "lobby/00847660.inc"
};
char gLobbyMsgPages_1_9_3[48] = {
#include "lobby/00847680.inc"
};
char gLobbyMsgPages_1_10_1[32] = {
#include "lobby/008476B0.inc"
};
char gLobbyMsgPages_1_10_2[48] = {
#include "lobby/008476D0.inc"
};
char gLobbyMsgPages_1_11_2[32] = {
#include "lobby/00847700.inc"
};
char gLobbyMsgPages_1_11_3[48] = {
#include "lobby/00847720.inc"
};
char gLobbyMsgPages_1_12_1[32] = {
#include "lobby/00847750.inc"
};
char gLobbyMsgPages_1_12_2[48] = {
#include "lobby/00847770.inc"
};
char gLobbyMsgPages_1_13_2[48] = {
#include "lobby/008477A0.inc"
};
char gLobbyMsgPages_1_13_4[48] = {
#include "lobby/008477D0.inc"
};
char gLobbyMsgPages_1_13_5[32] = {
#include "lobby/00847800.inc"
};
char gLobbyMsgPages_1_13_6[48] = {
#include "lobby/00847820.inc"
};
char gLobbyMsgPages_1_13_8[32] = {
#include "lobby/00847850.inc"
};
char gLobbyMsgPages_1_13_9[32] = {
#include "lobby/00847870.inc"
};
char gLobbyMsgPages_1_14_3[48] = {
#include "lobby/00847890.inc"
};
char gLobbyMsgPages_1_15_1[32] = {
#include "lobby/008478C0.inc"
};
char gLobbyMsgPages_1_15_2[32] = {
#include "lobby/008478E0.inc"
};
char gLobbyMsgPages_1_15_5[48] = {
#include "lobby/00847900.inc"
};
char gLobbyMsgPages_1_16_2[32] = {
#include "lobby/00847930.inc"
};
char gLobbyMsgPages_1_16_4[32] = {
#include "lobby/00847950.inc"
};
char gLobbyMsgPages_1_17_2[32] = {
#include "lobby/00847970.inc"
};
char gLobbyMsgPages_1_18_2[48] = {
#include "lobby/00847990.inc"
};
char gLobbyMsgPages_1_19_0[32] = {
#include "lobby/008479C0.inc"
};
char gLobbyMsgPages_1_19_1[32] = {
#include "lobby/008479E0.inc"
};
char gLobbyMsgPages_1_20_0[32] = {
#include "lobby/00847A00.inc"
};
char gLobbyMsgPages_1_21_0[32] = {
#include "lobby/00847A20.inc"
};
char gLobbyMsgPages_1_23_0[32] = {
#include "lobby/00847A40.inc"
};
char gLobbyMsgPages_1_23_1[32] = {
#include "lobby/00847A60.inc"
};
char gLobbyMsgPages_1_23_2[32] = {
#include "lobby/00847A80.inc"
};
char gLobbyMsgPages_1_23_4[32] = {
#include "lobby/00847AA0.inc"
};
char gLobbyMsgPages_1_23_7[16] = {
#include "lobby/00847AC0.inc"
};
char gLobbyMsgPages_1_25_0[32] = {
#include "lobby/00847AD0.inc"
};
char gLobbyMsgPages_1_25_1[32] = {
#include "lobby/00847AF0.inc"
};
char gLobbyMsgPages_1_26_2[32] = {
#include "lobby/00847B10.inc"
};
char gLobbyMsgPages_1_27_1[32] = {
#include "lobby/00847B30.inc"
};
char gLobbyMsgPages_1_27_2[48] = {
#include "lobby/00847B50.inc"
};
char gLobbyMsgPages_1_30_0[48] = {
#include "lobby/00847B80.inc"
};
char gLobbyMsgPages_1_30_1[48] = {
#include "lobby/00847BB0.inc"
};
char gLobbyMsgPages_1_30_2[48] = {
#include "lobby/00847BE0.inc"
};
char gLobbyMsgPages_1_30_3[32] = {
#include "lobby/00847C10.inc"
};
char gLobbyMsgPages_1_31_2[48] = {
#include "lobby/00847C30.inc"
};
char gLobbyMsgPages_1_31_3[48] = {
#include "lobby/00847C60.inc"
};
char gLobbyMsgPages_1_32_0[32] = {
#include "lobby/00847C90.inc"
};
char gLobbyMsgPages_1_33_1[48] = {
#include "lobby/00847CB0.inc"
};
char gLobbyMsgPages_1_34_0[48] = {
#include "lobby/00847CE0.inc"
};
char gLobbyMsgPages_1_34_1[16] = {
#include "lobby/00847D10.inc"
};
char gLobbyMsgPages_1_34_2[48] = {
#include "lobby/00847D20.inc"
};
char gLobbyMsgPages_1_34_3[48] = {
#include "lobby/00847D50.inc"
};
char gLobbyMsgPages_1_35_0[32] = {
#include "lobby/00847D80.inc"
};
char gLobbyMsgPages_1_36_0[32] = {
#include "lobby/00847DA0.inc"
};
char gLobbyMsgPages_1_36_1[32] = {
#include "lobby/00847DC0.inc"
};
char gLobbyMsgPages_1_38_0[32] = {
#include "lobby/00847DE0.inc"
};
char gLobbyMsgPages_1_38_2[48] = {
#include "lobby/00847E00.inc"
};
char gLobbyMsgPages_1_38_3[32] = {
#include "lobby/00847E30.inc"
};
char gLobbyMsgPages_1_38_5[32] = {
#include "lobby/00847E50.inc"
};
char gLobbyMsgPages_1_38_6[24] = {
#include "lobby/00847E70.inc"
};
char gLobbyMsgPages_1_38_7[8] = {
#include "lobby/00847E88.inc"
};
char gLobbyMsgPages_1_39_0[32] = {
#include "lobby/00847E90.inc"
};
char gLobbyMsgPages_1_40_2[48] = {
#include "lobby/00847EB0.inc"
};
char gLobbyMsgPages_1_40_9[32] = {
#include "lobby/00847EE0.inc"
};
char gLobbyMsgPages_1_41_1[48] = {
#include "lobby/00847F00.inc"
};
char gLobbyMsgPages_1_41_2[48] = {
#include "lobby/00847F30.inc"
};
char gLobbyMsgPages_1_41_4[48] = {
#include "lobby/00847F60.inc"
};
char gLobbyMsgPages_1_41_5[32] = {
#include "lobby/00847F90.inc"
};
char gLobbyMsgPages_1_42_1[48] = {
#include "lobby/00847FB0.inc"
};
char gLobbyMsgPages_1_42_2[48] = {
#include "lobby/00847FE0.inc"
};
char gLobbyMsgPages_1_43_0[64] = {
#include "lobby/00848010.inc"
};
char gLobbyMsgPages_1_45_0[48] = {
#include "lobby/00848050.inc"
};
char gLobbyMsgPages_1_45_1[48] = {
#include "lobby/00848080.inc"
};
char gLobbyMsgPages_1_45_2[16] = {
#include "lobby/008480B0.inc"
};
char gLobbyMsgPages_1_46_0[64] = {
#include "lobby/008480C0.inc"
};
char gLobbyMsgPages_1_48_2[48] = {
#include "lobby/00848100.inc"
};
char gLobbyMsgPages_1_48_3[32] = {
#include "lobby/00848130.inc"
};
char gLobbyMsgPages_1_49_0[48] = {
#include "lobby/00848150.inc"
};
char gLobbyMsgPages_1_49_3[48] = {
#include "lobby/00848180.inc"
};
char gLobbyMsgPages_1_50_3[48] = {
#include "lobby/008481B0.inc"
};
char gLobbyMsgPages_1_51_0[48] = {
#include "lobby/008481E0.inc"
};
char gLobbyMsgPages_1_51_4[48] = {
#include "lobby/00848210.inc"
};
char gLobbyMsgPages_1_53_0[48] = {
#include "lobby/00848240.inc"
};
char gLobbyMsgPages_1_53_3[32] = {
#include "lobby/00848270.inc"
};
char gLobbyMsgPages_1_55_5[48] = {
#include "lobby/00848290.inc"
};
char gLobbyMsgPages_0_1_0[32] = {
#include "lobby/008482C0.inc"
};
char gLobbyMsgPages_0_2_0[32] = {
#include "lobby/008482E0.inc"
};
char gLobbyMsgPages_0_3_0[48] = {
#include "lobby/00848300.inc"
};
char gLobbyMsgPages_0_3_1[48] = {
#include "lobby/00848330.inc"
};
char gLobbyMsgPages_0_4_0[64] = {
#include "lobby/00848360.inc"
};
char gLobbyMsgPages_0_10_0[32] = {
#include "lobby/008483A0.inc"
};
char gLobbyMsgPages_0_10_1[48] = {
#include "lobby/008483C0.inc"
};
char gLobbyMsgPages_0_10_2[48] = {
#include "lobby/008483F0.inc"
};
char gLobbyMsgPages_0_10_3[48] = {
#include "lobby/00848420.inc"
};
char gLobbyMsgPages_0_10_4[32] = {
#include "lobby/00848450.inc"
};
char gLobbyMsgPages_0_10_5[48] = {
#include "lobby/00848470.inc"
};
char gLobbyMsgPages_0_11_0[32] = {
#include "lobby/008484A0.inc"
};
char gLobbyMsgPages_0_11_1[48] = {
#include "lobby/008484C0.inc"
};
char gLobbyMsgPages_0_11_2[48] = {
#include "lobby/008484F0.inc"
};
char gLobbyMsgPages_0_11_3[48] = {
#include "lobby/00848520.inc"
};
char gLobbyMsgPages_0_11_5[48] = {
#include "lobby/00848550.inc"
};
char gLobbyMsgPages_0_11_6[48] = {
#include "lobby/00848580.inc"
};
char gLobbyMsgPages_0_13_0[8] = {
#include "lobby/008485B0.inc"
};
char gLobbyMsgPages_0_13_1[8] = {
#include "lobby/008485B8.inc"
};
char gLobbyMsgPages_0_14_0[8] = {
#include "lobby/008485C0.inc"
};
char gLobbyMsgPages_0_14_1[8] = {
#include "lobby/008485C8.inc"
};
char gLobbyMsgPages_0_14_2[24] = {
#include "lobby/008485D0.inc"
};
char gLobbyMsgPages_0_15_1[8] = {
#include "lobby/008485E8.inc"
};
char gLobbyMsgPages_0_16_0[8] = {
#include "lobby/008485F0.inc"
};
char gLobbyMsgPages_0_17_0[12] = {
#include "lobby/008485F8.inc"
};
char gLobbyMsgPages_0_17_1[12] = {
#include "lobby/00848608.inc"
};
char gLobbyMsgPages_0_17_2[32] = {
#include "lobby/00848620.inc"
};
char gLobbyMsgPages_0_18_0[8] = {
#include "lobby/00848640.inc"
};
char gLobbyMsgPages_0_18_1[8] = {
#include "lobby/00848648.inc"
};
char gLobbyMsgPages_0_19_0[32] = {
#include "lobby/00848650.inc"
};
char gLobbyMsgPages_0_21_0[48] = {
#include "lobby/00848670.inc"
};
char gLobbyMsgPages_0_21_2[32] = {
#include "lobby/008486A0.inc"
};
char gLobbyMsgPages_0_22_0[32] = {
#include "lobby/008486C0.inc"
};
char gLobbyMsgPages_0_22_1[64] = {
#include "lobby/008486E0.inc"
};
char gLobbyMsgPages_0_23_0[80] = {
#include "lobby/00848720.inc"
};
char gLobbyMsgPages_0_23_1[80] = {
#include "lobby/00848770.inc"
};
char gLobbyMsgPages_0_23_2[64] = {
#include "lobby/008487C0.inc"
};
char gLobbyMsgPages_0_23_3[48] = {
#include "lobby/00848800.inc"
};
char gLobbyMsgPages_0_24_0[80] = {
#include "lobby/00848830.inc"
};
char gLobbyMsgPages_0_24_1[128] = {
#include "lobby/00848880.inc"
};
char gLobbyMsgPages_0_24_2[128] = {
#include "lobby/00848900.inc"
};
char gLobbyMsgPages_0_24_3[48] = {
#include "lobby/00848980.inc"
};
char gLobbyMsgPages_2_1_0[32] = {
#include "lobby/008489B0.inc"
};
char gLobbyMsgPages_2_3_0[40] = {
#include "lobby/008489D0.inc"
};
char gLobbyMsgDnasNoteMark[8] = {
#include "lobby/008489F8.inc"
};
char gLobbyMsgPages_2_4_0[80] = {
#include "lobby/00848A00.inc"
};
char gLobbyMsgPages_2_5_0[32] = {
#include "lobby/00848A50.inc"
};
char gLobbyMsgPages_2_6_0[16] = {
#include "lobby/00848A70.inc"
};
char gLobbyMsgPages_2_7_0[32] = {
#include "lobby/00848A80.inc"
};
char gLobbyMsgPages_2_8_0[48] = {
#include "lobby/00848AA0.inc"
};
char gLobbyMsgPages_2_8_1[16] = {
#include "lobby/00848AD0.inc"
};
char gLobbyMsgPages_2_8_2[48] = {
#include "lobby/00848AE0.inc"
};
char gLobbyMsgPages_2_9_0[48] = {
#include "lobby/00848B10.inc"
};
char gLobbyMsgPages_2_10_0[64] = {
#include "lobby/00848B40.inc"
};
char gLobbyMsgPages_2_11_0[48] = {
#include "lobby/00848B80.inc"
};
char gLobbyMsgPages_2_12_0[48] = {
#include "lobby/00848BB0.inc"
};
char gLobbyMsgPages_2_13_0[48] = {
#include "lobby/00848BE0.inc"
};
char gLobbyMsgPages_2_14_0[32] = {
#include "lobby/00848C10.inc"
};
char gLobbyMsgPages_2_15_0[32] = {
#include "lobby/00848C30.inc"
};
char gLobbyMsgPages_2_16_0[32] = {
#include "lobby/00848C50.inc"
};
char gLobbyMsgPages_2_17_0[48] = {
#include "lobby/00848C70.inc"
};
char gLobbyMsgPages_2_18_0[48] = {
#include "lobby/00848CA0.inc"
};
char gLobbyMsgPages_2_19_0[32] = {
#include "lobby/00848CD0.inc"
};
char gLobbyMsgPages_2_20_0[32] = {
#include "lobby/00848CF0.inc"
};
char gLobbyMsgPages_2_21_0[48] = {
#include "lobby/00848D10.inc"
};
char gLobbyMsgPages_2_22_0[48] = {
#include "lobby/00848D40.inc"
};
char gLobbyMsgPages_2_22_1[48] = {
#include "lobby/00848D70.inc"
};
char gLobbyMsgPages_2_23_0[48] = {
#include "lobby/00848DA0.inc"
};
char gLobbyMsgPages_2_24_0[16] = {
#include "lobby/00848DD0.inc"
};
char gLobbyMsgPages_2_26_0[32] = {
#include "lobby/00848DE0.inc"
};
char gLobbyMsgDnasNoteLine[64] = {
#include "lobby/00848E00.inc"
};
char gLobbyMsgPages_3_1_0[48] = {
#include "lobby/00848E40.inc"
};
char gLobbyMsgPages_3_1_2[48] = {
#include "lobby/00848E70.inc"
};
char gLobbyMsgPages_3_1_3[48] = {
#include "lobby/00848EA0.inc"
};
char gLobbyMsgPages_3_1_4[48] = {
#include "lobby/00848ED0.inc"
};
char gLobbyMsgPages_3_1_6[48] = {
#include "lobby/00848F00.inc"
};
char gLobbyMsgPages_3_1_7[48] = {
#include "lobby/00848F30.inc"
};
char gLobbyMsgPages_3_2_0[48] = {
#include "lobby/00848F60.inc"
};
char gLobbyMsgPages_3_2_1[32] = {
#include "lobby/00848F90.inc"
};
char gLobbyMsgPages_3_3_0[32] = {
#include "lobby/00848FB0.inc"
};
char gLobbyMsgPages_3_3_1[32] = {
#include "lobby/00848FD0.inc"
};
char gLobbyMsgPages_3_4_2[48] = {
#include "lobby/00848FF0.inc"
};
char gLobbyMsgPages_3_5_0[48] = {
#include "lobby/00849020.inc"
};
char gLobbyMsgPages_3_5_2[48] = {
#include "lobby/00849050.inc"
};
char gLobbyMsgPages_3_5_3[64] = {
#include "lobby/00849080.inc"
};
char gLobbyMsgPages_3_6_2[32] = {
#include "lobby/008490C0.inc"
};
char gLobbyMsgPages_3_7_2[32] = {
#include "lobby/008490E0.inc"
};
char gLobbyMsgPages_3_8_2[32] = {
#include "lobby/00849100.inc"
};
char gLobbyMsgPages_3_9_2[32] = {
#include "lobby/00849120.inc"
};
char gLobbyMsgPages_3_11_2[48] = {
#include "lobby/00849140.inc"
};
char gLobbyMsgPages_3_12_2[48] = {
#include "lobby/00849170.inc"
};
char gLobbyMsgPages_3_13_0[48] = {
#include "lobby/008491A0.inc"
};
char gLobbyMsgPages_3_13_4[48] = {
#include "lobby/008491D0.inc"
};
char gLobbyMsgPages_3_14_0[32] = {
#include "lobby/00849200.inc"
};
char gLobbyMsgPages_3_15_2[32] = {
#include "lobby/00849220.inc"
};
char gLobbyMsgPages_3_15_3[32] = {
#include "lobby/00849240.inc"
};
char gLobbyMsgPages_4_1_2[48] = {
#include "lobby/00849260.inc"
};
char gLobbyMsgPages_4_1_3[48] = {
#include "lobby/00849290.inc"
};
char gLobbyMsgPages_4_2_4[64] = {
#include "lobby/008492C0.inc"
};
char gLobbyMsgPages_4_2_5[48] = {
#include "lobby/00849300.inc"
};
char gLobbyMsgPages_4_2_6[64] = {
#include "lobby/00849330.inc"
};
char gLobbyMsgPages_4_3_2[48] = {
#include "lobby/00849370.inc"
};
char gLobbyMsgPages_4_3_3[48] = {
#include "lobby/008493A0.inc"
};
char gLobbyMsgPages_4_3_4[48] = {
#include "lobby/008493D0.inc"
};
char gLobbyMsgPages_4_3_5[32] = {
#include "lobby/00849400.inc"
};
char gLobbyMsgPages_4_4_2[48] = {
#include "lobby/00849420.inc"
};
char gLobbyMsgPages_4_4_4[64] = {
#include "lobby/00849450.inc"
};
char gLobbyMsgPages_4_4_5[64] = {
#include "lobby/00849490.inc"
};
char gLobbyMsgPages_4_5_2[48] = {
#include "lobby/008494D0.inc"
};
char gLobbyMsgPages_4_6_2[64] = {
#include "lobby/00849500.inc"
};
char gLobbyMsgPages_4_6_3[32] = {
#include "lobby/00849540.inc"
};
char gLobbyMsgPages_4_7_2[48] = {
#include "lobby/00849560.inc"
};
char gLobbyMsgPages_4_7_4[48] = {
#include "lobby/00849590.inc"
};
char gLobbyMsgPages_4_7_5[48] = {
#include "lobby/008495C0.inc"
};
char gLobbyMsgPages_4_7_6[48] = {
#include "lobby/008495F0.inc"
};
char gLobbyMsgPages_4_8_2[48] = {
#include "lobby/00849620.inc"
};
char gLobbyMsgPages_4_8_3[48] = {
#include "lobby/00849650.inc"
};
char gLobbyMsgPages_4_8_4[48] = {
#include "lobby/00849680.inc"
};
char gLobbyMsgPages_4_9_0[32] = {
#include "lobby/008496B0.inc"
};
char gLobbyMsgPages_4_9_1[48] = {
#include "lobby/008496D0.inc"
};
char gLobbyMsgPages_4_9_2[48] = {
#include "lobby/00849700.inc"
};
char gLobbyMsgPages_4_9_3[48] = {
#include "lobby/00849730.inc"
};
char gLobbyMsgPages_4_9_6[32] = {
#include "lobby/00849760.inc"
};
char gLobbyMsgPages_4_9_7[32] = {
#include "lobby/00849780.inc"
};
char gLobbyMsgPages_4_9_8[32] = {
#include "lobby/008497A0.inc"
};
char gLobbyMsgPages_4_10_0[48] = {
#include "lobby/008497C0.inc"
};
char gLobbyMsgPages_4_10_1[32] = {
#include "lobby/008497F0.inc"
};
char gLobbyMsgPages_4_11_0[32] = {
#include "lobby/00849810.inc"
};
char gLobbyMsgPages_4_12_0[24] = {
#include "lobby/00849830.inc"
};
char gLobbyMsgPages_4_13_1[12] = {
#include "lobby/00849848.inc"
};
char gLobbyMsgPages_4_13_2[32] = {
#include "lobby/00849860.inc"
};
char gLobbyMsgPages_4_14_0[32] = {
#include "lobby/00849880.inc"
};
char gLobbyMsgPages_4_14_1[48] = {
#include "lobby/008498A0.inc"
};
char gLobbyMsgPages_4_14_2[48] = {
#include "lobby/008498D0.inc"
};
char gLobbyMsgPages_4_15_2[48] = {
#include "lobby/00849900.inc"
};
char gLobbyMsgPages_4_15_3[64] = {
#include "lobby/00849930.inc"
};
char gLobbyMsgPages_4_15_4[48] = {
#include "lobby/00849970.inc"
};
char gLobbyMsgPages_4_15_5[48] = {
#include "lobby/008499A0.inc"
};
char gLobbyMsgPages_4_15_6[48] = {
#include "lobby/008499D0.inc"
};
char gLobbyMsgPages_4_15_7[64] = {
#include "lobby/00849A00.inc"
};
char gLobbyMsgPages_4_16_2[64] = {
#include "lobby/00849A40.inc"
};
char gLobbyMsgPages_4_16_3[48] = {
#include "lobby/00849A80.inc"
};
char gLobbyMsgPages_4_16_4[64] = {
#include "lobby/00849AB0.inc"
};
char gLobbyMsgPages_4_16_6[64] = {
#include "lobby/00849AF0.inc"
};
char gLobbyMsgPages_4_16_7[48] = {
#include "lobby/00849B30.inc"
};
char gLobbyMsgPages_4_17_3[64] = {
#include "lobby/00849B60.inc"
};
char gLobbyMsgPages_4_17_4[64] = {
#include "lobby/00849BA0.inc"
};
char gLobbyMsgPages_5_1_1[32] = {
#include "lobby/00849BE0.inc"
};
char gLobbyMsgPages_5_2_3[32] = {
#include "lobby/00849C00.inc"
};
char gLobbyMsgPages_5_3_2[48] = {
#include "lobby/00849C20.inc"
};
char gLobbyMsgPages_5_4_2[48] = {
#include "lobby/00849C50.inc"
};
char gLobbyMsgPages_5_5_3[32] = {
#include "lobby/00849C80.inc"
};
char gLobbyMsgPages_5_6_2[32] = {
#include "lobby/00849CA0.inc"
};
char gLobbyMsgPages_6_2_0[32] = {
#include "lobby/00849CC0.inc"
};
char gLobbyMsgPages_6_3_0[32] = {
#include "lobby/00849CE0.inc"
};
char gLobbyMsgPages_6_4_0[32] = {
#include "lobby/00849D00.inc"
};
char gLobbyMsgPages_6_5_0[32] = {
#include "lobby/00849D20.inc"
};
char gLobbyMsgPages_6_6_0[24] = {
#include "lobby/00849D40.inc"
};
char gNetModeTitleTexts_1[12] = {
#include "lobby/00849D58.inc"
};
char gNetModeTitleTexts_2[64] = {
#include "lobby/00849D70.inc"
};
char gNetModeTitleTexts_5[16] = {
#include "lobby/00849DB0.inc"
};
char gNetModeTitleTexts_6[16] = {
#include "lobby/00849DC0.inc"
};
char gNetModeTitleTexts_7[32] = {
#include "lobby/00849DD0.inc"
};
#pragma rodata off
