#include "common.h"
#include "net_save.h"
#include "syssettings.h"
#include "net_mode.h"
#include "em.h"
#include "sprite.h"
#include "lobby_globals.h"
#include "net_menu.h"

/* prototypes */
s32 LobbyCard_DecodeNetFile(u16 *p);
struct NetMenu;
s32 SaveMenu_SystemDateMatches(struct NetMenu *);
char * strstr(char *s, char *w);
extern char ***gLobbyMsgPages[];          /* the pages' texts: NULL-ended lists */
void *memset(void *dst, s32 c, u32 n);
s32 LobbyMsg_CenterX(char *str, s32 size);
void LobbyMsg_Print(char *str, s32 x, s32 y, s32 color, s32 size);
s32 LobbyMsg_Format(s32 page, char *src, char *dst);
void strncat(char *dst, char *src, s32 n);  /* strncat */
s32 sprintf(char *dst, const char *fmt, ...);   /* sprintf */
void Sjis_PutDigits3(char *dst, s32 n);
char *strcat(char *dst, const char *src);
s32 strlen(const char *s);
extern char **D_0081B030;
s32 LobbyMsg_DrawPart(s32 size, s32 page, s32 part, s32 y, s32 lineH);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColorIndex(s16 color);
s32 Str_Length(char *s);
void Font_Printf(char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
void Text_Printf(const char *, ...);
char *strcpy(char *dst, const char *src);
char *strchr(char *s, s32 c);      /* strchr */
extern s8 gNetVerDigits;               /* the version's digits */
extern s8 D_008EDF11;
extern s8 D_008EDF12;
extern s8 D_008EDF13;

/* the help pages (0x7D44B0-0x7D4E10, from unk_7D44B0.c) */
char **LobbyMsg_GetLines(s32 page, s32 part);
extern s16 gLobbyMsgPage4Layout[][3][2];
extern s16 gLobbyMsgLayout[][3][2];          /* per part of page 5, the same */

/* an icon of the network screen (gNetScreenIcons, 10 bytes; three frames each) */
typedef struct NetIcon {
    /* 0x0 */ u8 tex;               /* the texture (Tex_Prepare, Tex_Bind) */
    /* 0x1 */ u8 layer;             /* Gfx_SetTexFilter */
    /* 0x2 */ s16 u0, v0, u1, v1;
} NetIcon;

extern NetIcon gNetScreenIcons[];
void Gfx_SetTexFilter(s32);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 tex);
s32 Sprite_Draw(Sprite *);
void LobbyMsg_DrawIcon(s32 kind, s16 x, s16 y, s32 big);




extern u8 *gLoadBuf;
void *memcpy(void *dst, const void *src, u32 n);
void NetMode_BeginCardTask(s32 i);
void LobbyCard_OpenMsgBox(s8 step, s8 msg);
void LobbyCard_CloseMsgBox(s8 step);
s32 LobbyCard_LoadFiles(s32 mode);
s32 CardOp_SetFiles(s32 *data);
void CardTask_StartFind(s32 slot, u8 *buf, s32 a);
void CardTask_Update(void);
s32 CardTask_Result(void);
void CardTask_StartSave(s32 slot, u8 *buf);
void CardTask_StartPoll(void);
s32 Card_PortState(s32 slot);
void LobbyCard_FixNetValues(u8 *data);
s32 LoginPad_Decide(s32 sound);
void NetModeMsg_QueueCountdown(u8 id);
void Browser_SetErrorState(s32 msg);
void Browser_EmptyHook(void);
s32 NetMode_CardMsgError(s8 c);
/* end of prototypes */

/* the NET file's save with its messages (NetModeState.step, .next the step
 * after a message): 0-7 read the card (slot 0x6ADFDA) and write the
 * lobby's data, with message 1-6 between; 0x1E, 0x3C and 0x46 the errors
 * (no card, 0x3D a failed write, 0x47 a changed card) waiting for the card
 * or a button; 0x5A done (1), 0x5B and 0x5C given up (-1); 0x64-0x83 the
 * message box's animations. 0 while running. The chain's constant 1 stays in a0 for
 * NetMode_BeginCardTask(1) in case 0 (it passes the card operation on to CardTask_Begin). */
s32 LobbyCard_RunNetSave(void)
{
    s32 r;

    r = 0;
    switch (gNetMode.saveStep) {
    case 0:
        gNetMode.innerStep = 0;
        NetMode_BeginCardTask(1);
        gNetMode.unk8D = 0;
        gNetMode.slot = gSysSettings.unk4A;
        gNetMode.slotUsed = gNetMode.slot;
        LobbyCard_OpenMsgBox(1, 1);
        break;
    case 1:
        if (LobbyCard_LoadFiles(0) != 0) {
            gNetMode.saveNext = 0;
            gNetMode.saveStep++;
            gNetMode.cardFiles = CardOp_SetFiles((s32 *)((s32)gLoadBuf + 0x20000));
            CardTask_StartFind(gNetMode.slot, gLoadBuf, 0);
        }
        break;
    case 2:
        CardTask_Update();
        gNetMode.sel2 = CardTask_Result();
        switch (gNetMode.sel2) {
        case 0:
            gNetMode.saveStep++;
            break;
        case -0x100:
        case -0xFB:
            gNetMode.saveStep = 0x3C;
            break;
        case -0xFF:
            gNetMode.saveStep = 0x1E;
            break;
        case -0xFC:
        case -0xFD:
        case -0xFE:
            gNetMode.saveStep = 0x46;
            break;
        }
        gNetMode.saveNext = 0;
        break;
    case 3:
        LobbyCard_OpenMsgBox(4, 2);
        memset(gLoadBuf, 0, sizeof(NetSaveData));
        memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
        LobbyCard_FixNetValues(gLoadBuf);
        CardTask_StartSave(gNetMode.slot, gLoadBuf);
        break;
    case 4:
        CardTask_Update();
        gNetMode.sel2 = CardTask_Result();
        if (gNetMode.sel2 != -1) {
            LobbyCard_CloseMsgBox(5);
        }
        break;
    case 5:
        switch (gNetMode.sel2) {
        case 0:
            LobbyCard_OpenMsgBox(6, 3);
            break;
        default:
            gNetMode.saveStep = 0x3C;
            gNetMode.saveNext = 0;
            break;
        }
        break;
    case 6:
        gNetMode.saveStep++;
        gNetMode.percent = 0x96;
    case 7:
        if (--gNetMode.percent < 0x79) {
            if (gNetMode.percent == 0 || LoginPad_Decide(2)) {
                LobbyCard_CloseMsgBox(0x5A);
            }
        }
        break;
    case 0x1E:
        LobbyCard_OpenMsgBox(0x1F, 4);
        CardTask_StartPoll();
        break;
    case 0x1F:
        NetModeMsg_QueueCountdown(2);
        CardTask_Update();
        if (Card_PortState(gNetMode.slotUsed) != 0) {
            LobbyCard_CloseMsgBox(0);
        } else if (LoginPad_Decide(2) || gNetMode.timer == 0) {
            LobbyCard_CloseMsgBox(0x5C);
        }
        break;
    case 0x3C:
        LobbyCard_OpenMsgBox(0x3D, 5);
        break;
    case 0x3D:
        NetModeMsg_QueueCountdown(2);
        if (LoginPad_Decide(2)) {
            LobbyCard_CloseMsgBox(0x5B);
        } else if (gNetMode.timer == 0) {
            LobbyCard_CloseMsgBox(0x5C);
        }
        break;
    case 0x46:
        LobbyCard_OpenMsgBox(0x47, 6);
        CardTask_StartPoll();
        break;
    case 0x47:
        NetModeMsg_QueueCountdown(2);
        CardTask_Update();
        if (Card_PortState(gNetMode.slotUsed) == 0) {
            LobbyCard_CloseMsgBox(0);
        } else if (LoginPad_Decide(2) || gNetMode.timer == 0) {
            LobbyCard_CloseMsgBox(0x5C);
        }
        break;
    case 0x5A:
        r = 1;
    reset:
        gNetMode.saveNext = 0;
        gNetMode.saveStep = 0;
        gNetMode.percent = 0;
        gNetMode.timer = 0;
        break;
    case 0x5B:
        r = -1;
        goto reset;
    case 0x5C:
        r = -1;
        goto reset;
    case 0x64:
        gNetMode.saveStep++;
        Browser_SetErrorState(NetMode_CardMsgError(gNetMode.cardMsg));
        break;
    case 0x65:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = gNetMode.saveNext;
            gNetMode.saveNext = 0;
        }
        break;
    case 0x6E:
        gNetMode.saveStep++;
        gNetMode.percent = 8;
        Browser_EmptyHook();
        gNetMode.cardMsg = 0;
        break;
    case 0x6F:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = gNetMode.saveNext;
            gNetMode.saveNext = 0;
        }
        break;
    case 0x78:
        gNetMode.saveStep++;
        break;
    case 0x79:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = gNetMode.saveNext;
            gNetMode.saveNext = 0;
        }
        break;
    case 0x82:
        gNetMode.saveStep++;
        gNetMode.cardMsg = 0;
        break;
    case 0x83:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = 0x6E;
        }
        break;
    }
    return r;
}

s32 NetMode_CardMsgError(s8 c)
{
    s32 r;

    r = 0;
    switch (c) {
    case 0x1D:
        r = 0x14;
        break;
    case 0x31:
        r = 0x28;
        break;
    case 0x32:
        r = 0x29;
        break;
    case 0x2C:
        r = -0xA;
        break;
    case 0x23:
        r = -0x15;
        break;
    case 0x72:
        r = -0x17;
        break;
    }
    return r;
}

s32 LobbyCard_CheckNetValues(u8 *buf)
{
    s32 i;
    s8 v;

    i = 0;
loop_1:
    v = *(s8 *)((u8 *)((buf + i)) + 0xC7D);
    if ((v < 0) || (v > 2)) {
        return -1;
    }
    i += 1;
    if (i >= 0x10) {
        return 0;
    }
    goto loop_1;
}

/* clears the 16 values at +0xC7D of the NET data that are not 0-2 */
void LobbyCard_FixNetValues(u8 *data)
{
    s32 i;

    i = 0;
    do {
        if (((s8 *)(data + i))[0xC7D] < 0 || ((s8 *)(data + i))[0xC7D] > 2) {
            ((s8 *)(data + i))[0xC7D] = 0;
        }
        i++;
    } while (i < 0x10);
}

s32 LobbyCard_CheckNetFile(s32 unused)
{
    if (LobbyCard_DecodeNetFile((void *) (*(void * *)&gLoadBuf)) != 0) {
        return -1;
    }
    return (SaveMenu_SystemDateMatches((struct NetMenu *)&gCardMenu) == 0) ? -2 : 0;
}

/* decodes the NET file's data in place (0x2A0 u16 words after an 8-byte
 * header: version 0x100, the key, the checksum) and checks the sum: 0
 * when right, -1 when not (or not version 0x100) */
s32 LobbyCard_DecodeNetFile(u16 *p)
{
    u16 key;
    s32 i;
    u16 check;
    u16 sum;

    if (p[0] != 0x100) {
        return -1;
    }
    key = p[1];
    sum = 0;
    i = 0;
    check = *(p += 2);
    p += 2;
    do {
        *p ^= key;
        sum += *p;
        p++;
        if (key == 0) {
            key = 1;
        }
        key = (key * 0xB0) % 0xFF53;
        i++;
    } while (i < 0x2A0);
    return -(check != sum);
}

char **LobbyMsg_GetLines(s32 i, s32 j)
{
    return gLobbyMsgPages[i][j];
}

/* prints the text of a page (gLobbyMsgPages tables, by page and part) from y,
 * lineH apart, in characters of size; the yes/no pairs (table 0 entry
 * 0xE: yes, no, the line) highlight the choice (NetModeState.yesNo, or
 * sel2 for page 1 part 4). Returns the y after the text */
s32 LobbyMsg_DrawPart(s32 size, s32 page, s32 part, s32 y, s32 lineH)
{
    char **msgs;
    s32 x;
    NetModeState *st;
    char buf[0x80];

    st = &gNetMode;
    memset(buf, 0, 0x80);
    switch (page) {
    case 1:
        switch (part) {
        case 1:
            msgs = LobbyMsg_GetLines(page, 2);
            break;
        case 2:
            msgs = LobbyMsg_GetLines(page, 1);
            y += lineH;
            break;
        case 3:
            msgs = LobbyMsg_GetLines(0, 0xE);
            x = LobbyMsg_CenterX(msgs[2], size);
            LobbyMsg_Print(msgs[0], x, 0x144, st->yesNo == 0 ? 5 : 0, size);
            x += size * 5;
            LobbyMsg_Print(msgs[1], x, 0x144, st->yesNo == 1 ? 5 : 0, size);
            return lineH + 0x144;
        case 4:
            msgs = LobbyMsg_GetLines(page, 0x19);
            x = LobbyMsg_CenterX(msgs[0], 0x12);
            LobbyMsg_Print(msgs[0], x, 0x118, st->sel2 == 0 ? 5 : 0, 0x12);
            x = LobbyMsg_CenterX(msgs[1], 0x12);
            LobbyMsg_Print(msgs[1], x, 0x144, st->sel2 == 1 ? 5 : 0, 0x12);
            return lineH + 0x144;
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
        size = 0x12;
        switch (part) {
        case 1:
            break;
        case 2:
            msgs = LobbyMsg_GetLines(page, 4);
            y += lineH;
            goto lines;
        case 3:
            msgs = LobbyMsg_GetLines(0, 0xE);
            x = LobbyMsg_CenterX(msgs[2], size);
            LobbyMsg_Print(msgs[0], x, 0x144, st->yesNo == 0 ? 5 : 0, size);
            LobbyMsg_Print(msgs[1], x + 0x5A, 0x144, st->yesNo == 1 ? 5 : 0, 0x12);
            return lineH + 0x144;
        }
        break;
    }
    return y;
}

static s32 LobbyMsg_PartCount(u32 page)
{
    switch (page) {
    case 0:
        return 0x19;
    case 1:
        return 0x3D;
    case 2:
        return 0x1C;
    case 3:
        return 0x10;
    case 4:
        return 0x12;
    case 5:
        return 7;
    case 6:
        return 7;
    default:
        return 0;
    }
}

/* expands a help line of page `page` into dst: pages 1, 3 and 5 replace
 * "%s" (the card slot, full width), "%1", "%2", "%a" (the files count),
 * "%i" (the id, gNetSaveData.mmbbLoginId) and "%p" (the password gNetSaveData.mmbbPassword as stars), and
 * mark the line by the tags "%r" (1 when the timer is 0), "%t" (2), "%y"
 * (3), "%k" (4), "%l" (5, page 5) and "%i"/"%p" (6); page 2 replaces "%e"
 * (the error code) and "%f" (D_0081B030[0] when unk7E is set); other pages
 * copy it. Returns the tag */
s32 LobbyMsg_Format(s32 page, char *src, char *dst)
{
    char buf[0x10];
    char *p;
    s32 r;
    NetModeState *st;
    char num[0x10];
    char *q;
    char *p2;
    char *q2;
    s32 n;
    s32 i;

    st = &gNetMode;
    if (src == NULL) {
        return 0;
    }
    if (dst == NULL) {
        return 0;
    }
    *dst = 0;
    r = 0;
    switch (page) {
    case 5:
    case 1:
    case 3:
        p = src;
        for (;;) {
            if ((q = strstr(p, "%s")) != NULL) {
                strncat(dst, p, q - p);
                sprintf(num, "%d", st->slotUsed + 1);
                Sjis_Widen(num, buf);
                strcat(dst, buf);
                p = q + 2;
                continue;
            }
            if ((q = strstr(p, "%1")) != NULL) {
                strncat(dst, p, q - p);
                strcpy(num, "1");
                Sjis_Widen(num, buf);
                strcat(dst, buf);
                p = q + 2;
                continue;
            }
            if ((q = strstr(p, "%2")) != NULL) {
                strncat(dst, p, q - p);
                strcpy(num, "2");
                Sjis_Widen(num, buf);
                strcat(dst, buf);
                p = q + 2;
                continue;
            }
            if ((q = strstr(p, "%a")) != NULL) {
                strncat(dst, p, q - p);
                Sjis_PutDigits3(buf, st->files);
                strcat(dst, buf);
                p = q + 2;
                continue;
            }
            if ((q = strstr(p, "%r")) != NULL) {
                strncat(dst, p, q - p);
                if (st->timer == 0) {
                    r = 1;
                }
                p = q + 2;
                continue;
            }
            if ((q = strstr(p, "%t")) != NULL) {
                strncat(dst, p, q - p);
                p = q + 2;
                r = 2;
                continue;
            }
            if ((q = strstr(p, "%y")) != NULL) {
                strncat(dst, p, q - p);
                p = q + 2;
                r = 3;
                continue;
            }
            if ((q = strstr(p, "%k")) != NULL) {
                strncat(dst, p, q - p);
                p = q + 2;
                r = 4;
                continue;
            }
            if ((q = strstr(p, "%i")) != NULL) {
                strncat(dst, p, q - p);
                strcpy(buf, gNetSaveData.mmbbLoginId);
                strcat(dst, buf);
                p = q + 2;
                r = 6;
                continue;
            }
            if ((q = strstr(p, "%p")) != NULL) {
                strncat(dst, p, q - p);
                buf[0] = 0;
                n = strlen(gNetSaveData.mmbbPassword);
                if (n != 0) {
                    for (i = 0; i < n; i++) {
                        buf[i] = '*';
                    }
                    buf[i] = 0;
                }
                strcat(dst, buf);
                p = q + 2;
                r = 6;
                continue;
            }
            if (page == 5 && (q = strstr(p, "%l")) != NULL) {
                strncat(dst, p, q - p);
                p = q + 2;
                r = 5;
                continue;
            }
            strcat(dst, p);
            break;
        }
        break;
    case 2:
        p2 = src;
        for (;;) {
            if ((q2 = strstr(p2, "%e")) != NULL) {
                strncat(dst, p2, q2 - p2);
                sprintf(buf, "%d", gNetMode.err);
                strcat(dst, buf);
                p2 = q2 + 2;
                continue;
            }
            if ((q2 = strstr(p2, "%f")) != NULL) {
                if (st->unk7E != 0) {
                    strcat(dst, *D_0081B030);
                }
                p2 = q2 + 2;
            }
            strcat(dst, p2);
            break;
        }
        break;
    default:
        strcpy(dst, src);
        break;
    }
    return r;
}

void LobbyMsg_Measure(u8 *str, s32 *w, s32 *n)
{
    s32 i;
    u8 *s;
    u8 c;

    s = str;
    if ((s != NULL) && (n != NULL) && (w != NULL)) {
        *n = 0;
        i = 0;
        *w = 0;
loop_4:
        c = *s;
        if (c != 0) {
            if ((s32) c >= 0x80) {
                if ((s32) c > 159) {
                    goto block_8;
                }
                goto block_11;
            }
block_8:
            if (((s32) c >= 0xE0) && ((s32) c < 0x100)) {
block_11:
                s += 2;
                *w += 1;
            } else {
                s += 1;
                *n += 1;
            }
            i += 1;
            if (i < 0x80) {
                goto loop_4;
            }
        }
    }
}

/* the x that centres str in characters of size (its width and count from
 * LobbyMsg_Measure, less for a "~B00" or a "%v" in it) */
s32 LobbyMsg_CenterX(char *str, s32 size)
{
    s32 w;
    s32 n;

    w = 0;
    n = 0;
    LobbyMsg_Measure((u8 *)str, &w, &n);
    if (strstr(str, "~B00") != NULL) {
        n -= 2;
    }
    if (strstr(str, "%v") != NULL) {
        n -= 2;
        w += 5;
    }
    return (s16)((640 - size * w - n * (size / 2)) / 2);
}

/* prints str at x, y (characters of size, colour color); a "%v" in it is
 * replaced by the version (gNetVerDigits..13 as "a.bcd", through
 * Sjis_Widen) printed in colour 5 */
void LobbyMsg_Print(char *str, s32 x, s32 y, s32 color, s32 size)
{
    char buf[0x80];
    char num[0x10];
    char *p;

    buf[1] = '.';
    buf[5] = 0;
    buf[0] = gNetVerDigits;
    buf[2] = D_008EDF11;
    buf[3] = D_008EDF12;
    buf[4] = D_008EDF13;
    Sjis_Widen(buf, num);
    Font_SetSpacing(size, size);
    Font_SetColorIndex(color);
    Font_SetPos(x, y);
    strcpy(buf, str);
    p = strchr(buf, '%');
    if (p != NULL && p[1] == 'v') {
        *p = 0;
        Font_Printf(buf);
        x += size * Str_Length(buf) / 2;
        Font_SetPos(x, y);
        Font_SetColorIndex(5);
        Font_Printf(num);
        strcpy(buf, str + (p + 2 - buf));
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
 * (y, the size, x, the colour) by page and part; a line may add a
 * sub-list (LobbyMsg_DrawPart) or, 6, move the rest to x 200 */
void LobbyMsg_DrawPage(s32 page, s32 part)
{
    s32 x;
    s32 color;
    s32 y;
    s32 lineH;
    s32 size;
    s32 sub;
    char **msgs;
    char buf[0x80];

    if (part != 0 && part < LobbyMsg_PartCount(page)) {
        msgs = LobbyMsg_GetLines(page, part);
        size = 0x14;
        x = -1;
        lineH = size * 1.2f;
        color = 0;
        switch (page) {
        case 0:
            switch (part) {
            case 2:
            case 3:
                y = 0x118;
                break;
            case 4:
                y = 0x188;
                break;
            case 10:
            case 11:
                y = 0x5E;
                color = 5;
                break;
            case 0x14:
                y = 0x188;
                break;
            default:
                y = 0x64;
                break;
            }
            break;
        case 2:
            switch (part) {
            case 1:
                y = 0x118;
                break;
            case 2:
                y = 0x188;
                break;
            case 4:
                y = 0xE4;
                size = 0x10;
                break;
            case 3:
            case 5:
            case 6:
            case 7:
                x = 0x40;
                y = 0x5E;
                break;
            default:
                x = 0x40;
                y = 0x86;
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
            case 0x10:
            case 0xF:
            case 0x11:
                x = 0x30;
                y = 0x5C;
                break;
            default:
                y = 0x64;
                break;
            }
            break;
        case 1:
            size = 0x12;
        case 3:
        case 5:
        default:
            y = 0x64;
            break;
        case 6:
            y = 0x188;
            break;
        }
        while (*msgs != NULL) {
            sub = LobbyMsg_Format(page, (char *)*msgs, buf);
            if (sub == 6) {
                sub = 0;
                x = 0xC8;
            }
            LobbyMsg_Print(buf, x < 0 ? LobbyMsg_CenterX(buf, size) : x, y, color, size);
            msgs++;
            y += lineH;
            if (sub != 0) {
                y = LobbyMsg_DrawPart(size, page, sub, y, lineH);
            }
        }
    }
}

/* NetModeMsg_Flush calls each drawer with a message's (arg, id); this one reads neither. */
void func_007D44B0(s32 unused, s32 unused2) {
}

/* draws part `part` of help page 4 with the layout of gLobbyMsgPage4Layout (x -1:
 * each line centred) */
void LobbyMsg_DrawPage4(s32 page, s32 part)
{
    s16 *p;
    s16 color;
    s16 sx;
    s16 x;
    s16 y;
    s16 lineH;
    char **msgs;
    s16 size;

    if (part == 0 || part >= 0x12) {
        return;
    }
    p = gLobbyMsgPage4Layout[part][0];
    x = *p++;
    y = *p++;
    size = *p++;
    color = *p++;
    p++;
    lineH = *p;
    msgs = LobbyMsg_GetLines(4, part);
    sx = x;
    while (*msgs != NULL) {
        if (sx == -1) {
            x = LobbyMsg_CenterX(*msgs, size);
        }
        LobbyMsg_Print(*msgs, x, y, color, size);
        msgs++;
        y += lineH;
    }
}

/* draws part `part` of help page 5 like LobbyMsg_DrawPage4, each line through
 * LobbyMsg_Format first (gLobbyMsgLayout's layout; the lines one line height up) */
void LobbyMsg_DrawPage5(s32 part)
{
    s16 *p;
    char buf[0x200];
    s16 sx;
    s16 x;
    s16 y;
    s16 lineH;
    char **msgs;
    s16 color;
    s16 size;

    if (part == 0 || part >= 7) {
        return;
    }
    p = gLobbyMsgLayout[part][0];
    x = *p++;
    y = *p++;
    size = *p++;
    color = *p++;
    p++;
    lineH = *p;
    msgs = LobbyMsg_GetLines(5, part);
    sx = x;
    while (*msgs != NULL) {
        LobbyMsg_Format(5, (char *)*msgs, buf);
        if (sx == -1) {
            x = LobbyMsg_CenterX(buf, size);
        }
        LobbyMsg_Print(buf, x, y - lineH, color, size);
        msgs++;
        y += lineH;
    }
}

/* NetModeMsg_Flush calls each drawer with a message's (arg, id); this one reads neither. */
void func_007D4730(s32 unused, s32 unused2) {
}

void func_007D4740(void) {
}

/* draws icon `kind` of gNetScreenIcons at x, y: small, or big (big != 0) and
 * animated (frame 0x5D477B) */
/* Raw addresses kept here: the named form changes the code. */
void LobbyMsg_DrawIcon(s32 kind, s16 x, s16 y, s32 big)
{
    Sprite s;
    NetIcon *e;
    u32 color;

    if (big == 0) {
        e = &gNetScreenIcons[kind * 3];
        color = 0xFFA87600;
        x -= 0xE;
        y -= 5;
        s.w = 0x24;
        s.h = 0x1E;
    } else {
        e = &gNetScreenIcons[(*(s8 *)&gNetMode.padD9[2]) + kind * 3];
        color = 0xFFFFBD29;
        x -= 0x18;
        y -= 8;
        s.w = 0x2B;
        s.h = 0x24;
    }
    Gfx_SetTexFilter(e->layer);
    Tex_Prepare(1, e->tex);
    Tex_Bind(e->tex);
    s.x = x;
    s.y = y;
    s.color = color;
    s.u0 = e->u0;
    s.v0 = e->v0;
    s.u1 = e->u1;
    s.v1 = e->v1;
    Sprite_Draw(&s);
}

/* draws the list of help page `page` part `part` (0x15, 0x16: the network
 * settings' choices) with the current choice (sel3, or sel2 for 0x16) big
 * and its icon, and the choice's note line (parts 0x17, 0x18) at the
 * bottom; in part 0x15 the lines of a missing ID or password in colour 10.
 * The selected line is placed at y, the others at their own y2. */
void LobbyMsg_DrawList(s32 page, s32 part)
{
    char buf[0x80];
    NetModeState *st;
    s32 y;
    s32 y2;
    s32 top;
    s32 i;
    s32 flags;
    char **msgs;
    s32 h;
    s32 sz;
    s32 step;
    s32 w;
    s32 d;
    s32 sel;
    f32 fh, fsz, fstep, fw;

    st = &gNetMode;
    if (part != 0 && part < LobbyMsg_PartCount(page)) {
        msgs = LobbyMsg_GetLines(page, part);
        fh = 36.0f;
        fsz = 22.5f;
        fstep = 45.0f;
        fw = 27.0f;
        h = fh;
        sz = fsz;
        step = fstep;
        w = fw;
        top = 100;
        if (st->openH == 0) {
            st->openW++;
            if (st->openW >= 4) {
                st->openH = 1;
            }
        } else {
            st->openW--;
            if (st->openW <= 0) {
                st->openH = 0;
            }
        }
        if (part == 0x16) {
            sel = st->sel2;
        } else {
            sel = st->sel3;
        }
        flags = 0;
        if (part == 0x15) {
            if (gNetSaveData.setupResult == 0) {
                flags = 1;
            }
            if (gNetSaveData.mmbbLoginId[0] == 0) {
                flags |= 2;
            }
        }
        for (i = 0; *msgs != NULL; msgs++) {
            LobbyMsg_Format(page, (char *)*msgs, buf);
            if (i == sel) {
                y = top + (s32)(((f32)h - (f32)sz) / 2.0f);
                LobbyMsg_Print(*msgs, 0x71, y + 3, 1, sz);
                LobbyMsg_Print(*msgs, 0x6E, y, 0xB, sz);
                if (part == 0x16) {
                    if (i != 3) {
                        LobbyMsg_DrawIcon(i + 3, 0x6E - w, y, 1);
                    }
                } else if (i != 3) {
                    LobbyMsg_DrawIcon(i, 0x6E - w, y, 1);
                }
            } else {
                d = ((f32)h - 18.0f) / 2.0f;
                if (flags == 0) {
                    y2 = top + d;
                    LobbyMsg_Print(*msgs, 0x70, y2 + 2, 1, 0x12);
                    LobbyMsg_Print(*msgs, 0x6E, y2, 0, 0x12);
                } else {
                    switch (i) {
                    case 1:
                        if (flags & 1) {
                            y2 = top + d;
                            LobbyMsg_Print(*msgs, 0x70, y2 + 2, 1, 0x12);
                            LobbyMsg_Print(*msgs, 0x6E, y2, 0xA, 0x12);
                            flags &= 2;
                        } else {
                            y2 = top + d;
                            LobbyMsg_Print(*msgs, 0x70, y2 + 2, 1, 0x12);
                            LobbyMsg_Print(*msgs, 0x6E, y2, 0, 0x12);
                        }
                        break;
                    case 2:
                        if (flags & 2) {
                            y2 = top + d;
                            LobbyMsg_Print(*msgs, 0x70, y2 + 2, 1, 0x12);
                            LobbyMsg_Print(*msgs, 0x6E, y2, 0xA, 0x12);
                            flags = 0;
                        } else {
                            y2 = top + d;
                            LobbyMsg_Print(*msgs, 0x70, y2 + 2, 1, 0x12);
                            LobbyMsg_Print(*msgs, 0x6E, y2, 0, 0x12);
                        }
                        break;
                    default:
                        y2 = top + d;
                        LobbyMsg_Print(*msgs, 0x70, y2 + 2, 1, 0x12);
                        LobbyMsg_Print(*msgs, 0x6E, y2, 0, 0x12);
                        break;
                    }
                }
                if (part == 0x16) {
                    if (i != 3) {
                        LobbyMsg_DrawIcon(i + 3, 0x6E - w, y2, 0);
                    }
                } else if (i != 3) {
                    LobbyMsg_DrawIcon(i, 0x6E - w, y2, 0);
                }
            }
            i++;
            top += step;
            if (i == 3) {
                top += (s32)(0.2f * step);
            }
        }
        if (part == 0x16) {
            msgs = LobbyMsg_GetLines(page, 0x18);
        } else {
            msgs = LobbyMsg_GetLines(page, 0x17);
        }
        for (i = 0; *msgs != NULL; msgs++, i++) {
            if (i == sel) {
                LobbyMsg_Format(page, (char *)*msgs, buf);
                LobbyMsg_Print(*msgs, 0x30, 0x15E, 0, 0x12);
                return;
            }
        }
    }
}

/* unk_7D2C80's .data (0x81A300-0x81B650) */
extern char D_00847370[];
extern char D_008473A0[];
extern char D_008473D0[];
extern char gNetModeTitleTexts_0[];
extern char D_00847400[];
extern char D_00847420[];
extern char D_00847440[];
extern char D_00847470[];
extern char D_00847490[];
extern char D_008474C0[];
extern char D_008474F0[];
extern char D_00847510[];
extern char D_00847530[];
extern char D_00847550[];
extern char D_00847580[];
extern char D_008475A0[];
extern char D_008475D0[];
extern char D_008475F0[];
extern char D_00847610[];
extern char D_00847630[];
extern char D_00847660[];
extern char D_00847680[];
extern char D_008476B0[];
extern char D_008476D0[];
extern char D_00847700[];
extern char D_00847720[];
extern char D_00847750[];
extern char D_00847770[];
extern char D_008477A0[];
extern char D_008477D0[];
extern char D_00847800[];
extern char D_00847820[];
extern char D_00847850[];
extern char D_00847870[];
extern char D_00847890[];
extern char D_008478C0[];
extern char D_008478E0[];
extern char D_00847900[];
extern char D_00847930[];
extern char D_00847950[];
extern char D_00847970[];
extern char D_00847990[];
extern char D_008479C0[];
extern char D_008479E0[];
extern char D_00847A00[];
extern char D_00847A20[];
extern char D_00847A40[];
extern char D_00847A60[];
extern char D_00847A80[];
extern char D_00847AA0[];
extern char D_00847AC0[];
extern char D_00847AD0[];
extern char D_00847AF0[];
extern char D_00847B10[];
extern char D_00847B30[];
extern char D_00847B50[];
extern char D_00847B80[];
extern char D_00847BB0[];
extern char D_00847BE0[];
extern char D_00847C10[];
extern char D_00847C30[];
extern char D_00847C60[];
extern char D_00847C90[];
extern char D_00847CB0[];
extern char D_00847CE0[];
extern char D_00847D10[];
extern char D_00847D20[];
extern char D_00847D50[];
extern char D_00847D80[];
extern char D_00847DA0[];
extern char D_00847DC0[];
extern char D_00847DE0[];
extern char D_00847E00[];
extern char D_00847E30[];
extern char D_00847E50[];
extern char D_00847E70[];
extern char D_00847E88[];
extern char D_00847E90[];
extern char D_00847EB0[];
extern char D_00847EE0[];
extern char D_00847F00[];
extern char D_00847F30[];
extern char D_00847F60[];
extern char D_00847F90[];
extern char D_00847FB0[];
extern char D_00847FE0[];
extern char D_00848010[];
extern char D_00848050[];
extern char D_00848080[];
extern char D_008480B0[];
extern char D_008480C0[];
extern char D_00848100[];
extern char D_00848130[];
extern char D_00848180[];
extern char D_008481B0[];
extern char D_008481E0[];
extern char D_00848210[];
extern char D_00848270[];
extern char D_00848290[];
extern char D_008482E0[];
extern char D_00848300[];
extern char D_00848330[];
extern char D_008483A0[];
extern char D_008483C0[];
extern char D_008483F0[];
extern char D_00848420[];
extern char D_00848450[];
extern char D_00848470[];
extern char D_008484A0[];
extern char D_008484C0[];
extern char D_008484F0[];
extern char D_00848520[];
extern char D_00848550[];
extern char D_00848580[];
extern char D_008485B0[];
extern char D_008485B8[];
extern char D_008485C0[];
extern char D_008485C8[];
extern char D_008485D0[];
extern char D_008485E8[];
extern char D_008485F0[];
extern char D_008485F8[];
extern char D_00848608[];
extern char D_00848620[];
extern char D_00848640[];
extern char D_00848648[];
extern char D_00848650[];
extern char D_00848670[];
extern char D_00848688[];
extern char D_008486A0[];
extern char D_008486C0[];
extern char D_008486E0[];
extern char D_00848708[];
extern char D_00848720[];
extern char D_00848770[];
extern char D_008487C0[];
extern char D_00848800[];
extern char D_00848830[];
extern char D_00848880[];
extern char D_00848900[];
extern char D_00848980[];
extern char D_008489F8[];
extern char D_00848A00[];
extern char D_00848A80[];
extern char D_00848AA0[];
extern char D_00848AD0[];
extern char D_00848AE0[];
extern char D_00848B10[];
extern char D_00848B80[];
extern char D_00848BE0[];
extern char D_00848C10[];
extern char D_00848CA0[];
extern char D_00848CF0[];
extern char D_00848D10[];
extern char D_00848D70[];
extern char D_00848DA0[];
extern char D_00848DE0[];
extern char D_00848E00[];
extern char D_00848E70[];
extern char D_00848EA0[];
extern char D_00848ED0[];
extern char D_00848F00[];
extern char D_00848F30[];
extern char D_00848F90[];
extern char D_00848FB0[];
extern char D_00848FD0[];
extern char D_00848FF0[];
extern char D_00849050[];
extern char D_00849080[];
extern char D_008490C0[];
extern char D_008490E0[];
extern char D_00849100[];
extern char D_00849120[];
extern char D_00849140[];
extern char D_00849170[];
extern char D_008491A0[];
extern char D_008491B8[];
extern char D_008491D0[];
extern char D_00849200[];
extern char D_00849220[];
extern char D_00849240[];
extern char D_00849260[];
extern char D_00849290[];
extern char D_008492C0[];
extern char D_00849300[];
extern char D_00849330[];
extern char D_00849370[];
extern char D_008493A0[];
extern char D_008493D0[];
extern char D_00849400[];
extern char D_00849420[];
extern char D_00849450[];
extern char D_00849490[];
extern char D_008494D0[];
extern char D_00849500[];
extern char D_00849540[];
extern char D_00849560[];
extern char D_00849590[];
extern char D_008495C0[];
extern char D_008495F0[];
extern char D_00849620[];
extern char D_00849650[];
extern char D_00849680[];
extern char D_00849700[];
extern char D_00849730[];
extern char D_00849760[];
extern char D_00849780[];
extern char D_008497A0[];
extern char D_008497F0[];
extern char D_00849810[];
extern char D_00849830[];
extern char D_00849848[];
extern char D_00849860[];
extern char D_00849880[];
extern char D_008498A0[];
extern char D_008498D0[];
extern char D_00849900[];
extern char D_00849930[];
extern char D_00849970[];
extern char D_008499A0[];
extern char D_008499D0[];
extern char D_00849A00[];
extern char D_00849A40[];
extern char D_00849A80[];
extern char D_00849AB0[];
extern char D_00849AF0[];
extern char D_00849B30[];
extern char D_00849B60[];
extern char D_00849BA0[];
extern char D_00849BE0[];
extern char D_00849C00[];
extern char D_00849C20[];
extern char D_00849C50[];
extern char D_00849C68[];
extern char D_00849C80[];
extern char D_00849CA0[];
extern char D_00849CE0[];
extern char D_00849D00[];
extern char D_00849D58[];
extern char D_00849D70[];
extern char D_00849D88[];
extern char D_00849D98[];
extern char D_00849DB0[];
extern char D_00849DC0[];
extern char D_00849DD0[];
extern char *D_008FABC8[];
extern char *D_008FABD0[];
extern char *D_008FABD8[];
extern char *D_008FABE0[];
extern char *D_008FABE8[];
extern char *D_008FABF0[];
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
    D_00847370, D_008473A0, D_008473D0, NULL,
};
char *gLobbyMsgPages_1_2[4] = {
    gNetModeTitleTexts_0, D_00847400, NULL, NULL,
};
char *gLobbyMsgPages_1_3[8] = {
    D_00847420, D_00847440, gNetModeTitleTexts_0, D_00847470,
    D_00847490, gNetModeTitleTexts_0, D_008474C0, NULL,
};
char *gLobbyMsgPages_1_4[4] = {
    D_008474F0, D_00847510, NULL, NULL,
};
char *gLobbyMsgPages_1_5[4] = {
    D_00847530, D_00847550, NULL, NULL,
};
char *gLobbyMsgPages_1_6[4] = {
    D_00847580, D_008475A0, NULL, NULL,
};
char *gLobbyMsgPages_1_7[8] = {
    D_008475D0, D_008475F0, D_00847610, gNetModeTitleTexts_0,
    D_00847630, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_8[8] = {
    D_00847580, D_008475F0, D_00847610, gNetModeTitleTexts_0,
    D_00847630, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_9[8] = {
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_00847660, D_00847680,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_10[4] = {
    D_00847580, D_008476B0, D_008476D0, NULL,
};
char *gLobbyMsgPages_1_11[8] = {
    D_00847580, D_008476B0, D_00847700, D_00847720,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_12[4] = {
    D_00847580, D_00847750, D_00847770, NULL,
};
char *gLobbyMsgPages_1_13[12] = {
    D_00847580, D_00847750, D_008477A0, gNetModeTitleTexts_0,
    D_008477D0, D_00847800, D_00847820, D_00847580,
    D_00847850, D_00847870, NULL, NULL,
};
char *gLobbyMsgPages_1_14[8] = {
    D_00847580, D_00847750, D_00847700, D_00847890,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_15[8] = {
    D_00847580, D_008478C0, D_008478E0, gNetModeTitleTexts_0,
    D_00847470, D_00847900, NULL, NULL,
};
char *gLobbyMsgPages_1_16[8] = {
    D_00847580, D_008478C0, D_00847930, gNetModeTitleTexts_0,
    D_00847950, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_17[4] = {
    D_00847580, D_00847850, D_00847970, NULL,
};
char *gLobbyMsgPages_1_18[4] = {
    D_00847580, D_00847850, D_00847990, NULL,
};
char *gLobbyMsgPages_1_19[4] = {
    D_008479C0, D_008479E0, NULL, NULL,
};
char *gLobbyMsgPages_1_20[2] = {
    D_00847A00, NULL,
};
char *gLobbyMsgPages_1_21[2] = {
    D_00847A20, NULL,
};
char *gLobbyMsgPages_1_22[4] = {
    D_00847580, D_00847750, D_008477A0, NULL,
};
char *gLobbyMsgPages_1_23[12] = {
    D_00847A40, D_00847A60, D_00847A80, gNetModeTitleTexts_0,
    D_00847AA0, D_00847850, D_00847A40, D_00847AC0,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_24[10] = {
    D_00847580, D_008475F0, D_00847610, gNetModeTitleTexts_0,
    D_00847AA0, D_00847850, D_00847A40, D_00847AC0,
    NULL, NULL,
};
char *gLobbyMsgPages_1_25[3] = {
    D_00847AD0, D_00847AF0, NULL,
};
char *gLobbyMsgPages_1_26[4] = {
    D_00847A40, D_00847A60, D_00847B10, NULL,
};
char *gLobbyMsgPages_1_27[4] = {
    D_008475D0, D_00847B30, D_00847B50, NULL,
};
char *gLobbyMsgPages_1_28[4] = {
    D_00847580, D_00847B30, D_00847B50, NULL,
};
char *gLobbyMsgPages_1_29[4] = {
    D_008474F0, D_00847510, NULL, NULL,
};
char *gLobbyMsgPages_1_30[8] = {
    D_00847B80, D_00847BB0, D_00847BE0, D_00847C10,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_31[6] = {
    D_00847B80, D_00847BB0, D_00847C30, D_00847C60,
    NULL, NULL,
};
char *gLobbyMsgPages_1_32[3] = {
    D_00847C90, D_00847550, NULL,
};
char *gLobbyMsgPages_1_33[3] = {
    D_00847580, D_00847CB0, NULL,
};
char *gLobbyMsgPages_1_34[6] = {
    D_00847CE0, D_00847D10, D_00847D20, D_00847D50,
    NULL, NULL,
};
char *gLobbyMsgPages_1_35[3] = {
    D_00847D80, D_00847680, NULL,
};
char *gLobbyMsgPages_1_36[3] = {
    D_00847DA0, D_00847DC0, NULL,
};
char *gLobbyMsgPages_1_37[3] = {
    D_00847580, D_00847DC0, NULL,
};
char *gLobbyMsgPages_1_38[10] = {
    D_00847DE0, gNetModeTitleTexts_0, D_00847E00, D_00847E30,
    gNetModeTitleTexts_0, D_00847E50, D_00847E70, D_00847E88,
    NULL, NULL,
};
char *gLobbyMsgPages_1_39[2] = {
    D_00847E90, NULL,
};
char *gLobbyMsgPages_1_40[12] = {
    D_00847580, D_008476B0, D_00847EB0, gNetModeTitleTexts_0,
    D_008477D0, D_00847800, D_00847820, D_00847580,
    D_00847850, D_00847EE0, NULL, NULL,
};
char *gLobbyMsgPages_1_41[8] = {
    gNetModeTitleTexts_0, D_00847F00, D_00847F30, gNetModeTitleTexts_0,
    D_00847F60, D_00847F90, NULL, NULL,
};
char *gLobbyMsgPages_1_42[4] = {
    D_00847B80, D_00847FB0, D_00847FE0, NULL,
};
char *gLobbyMsgPages_1_43[8] = {
    D_00848010, D_00847BB0, D_00847BE0, D_00847C10,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_44[8] = {
    D_00848010, D_00847BB0, D_00847C30, D_00847C60,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_45[4] = {
    D_00848050, D_00848080, D_008480B0, NULL,
};
char *gLobbyMsgPages_1_46[4] = {
    D_008480C0, D_00848080, D_008480B0, NULL,
};
char *gLobbyMsgPages_1_47[4] = {
    D_008479C0, D_00847510, NULL, NULL,
};
char *gLobbyMsgPages_1_48[8] = {
    D_00847580, D_008475F0, D_00848100, D_00848130,
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
    gLobbyMsgPages_1_49_0, D_00847A80, gNetModeTitleTexts_0, D_00848180,
    D_00848080, D_008480B0, NULL, NULL,
};
char *gLobbyMsgPages_1_50[8] = {
    gLobbyMsgPages_1_49_0, D_00847A80, gNetModeTitleTexts_0, D_008481B0,
    D_00847BB0, D_00847BE0, D_00847C10, NULL,
};
char *gLobbyMsgPages_1_51[8] = {
    D_008481E0, D_00848080, D_00847D10, gNetModeTitleTexts_0,
    D_00848210, D_00847B10, NULL, NULL,
};
char *gLobbyMsgPages_1_52[12] = {
    D_008481E0, D_00848080, D_00847D10, gNetModeTitleTexts_0,
    D_008481B0, D_00847BB0, D_00847BE0, D_00847C10,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_53[8] = {
    gLobbyMsgPages_1_53_0, D_00847BB0, D_00847BE0, D_00848270,
    gNetModeTitleTexts_0, D_00848210, D_00847B10, NULL,
};
char *gLobbyMsgPages_1_54[12] = {
    gLobbyMsgPages_1_53_0, D_00847BB0, D_00847BE0, D_00848270,
    gNetModeTitleTexts_0, D_00848180, D_00848080, D_008480B0,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_55[8] = {
    gLobbyMsgPages_1_49_0, D_00847A80, gNetModeTitleTexts_0, D_008481B0,
    D_00847BB0, D_00848290, NULL, NULL,
};
char *gLobbyMsgPages_1_56[8] = {
    D_008481E0, D_00848080, D_00847D10, gNetModeTitleTexts_0,
    D_008481B0, D_00847BB0, D_00848290, NULL,
};
char *gLobbyMsgPages_1_57[8] = {
    gLobbyMsgPages_1_53_0, D_00847BB0, D_00847C30, gNetModeTitleTexts_0,
    D_00848210, D_00847B10, NULL, NULL,
};
char *gLobbyMsgPages_1_58[12] = {
    gLobbyMsgPages_1_53_0, D_00847BB0, D_00847C30, gNetModeTitleTexts_0,
    D_008481B0, D_00847BB0, D_00847BE0, D_00847C10,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_1_59[8] = {
    gLobbyMsgPages_1_53_0, D_00847BB0, D_00847C30, gNetModeTitleTexts_0,
    D_00848180, D_00848080, D_008480B0, NULL,
};
char *gLobbyMsgPages_1_60[12] = {
    gLobbyMsgPages_1_53_0, D_00847BB0, D_00847BE0, D_00848270,
    gNetModeTitleTexts_0, D_008481B0, D_00847BB0, D_00848290,
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
    D_008482E0, NULL,
};
char *gLobbyMsgPages_0_3[3] = {
    D_00848300, D_00848330, NULL,
};
char *gLobbyMsgPages_0_4[2] = {
    gLobbyMsgPages_0_4_0, NULL,
};
char *gLobbyMsgPages_0_10[8] = {
    D_008483A0, D_008483C0, D_008483F0, D_00848420,
    D_00848450, D_00848470, NULL, NULL,
};
char *gLobbyMsgPages_0_11[8] = {
    D_008484A0, D_008484C0, D_008484F0, D_00848520,
    gNetModeTitleTexts_0, D_00848550, D_00848580, NULL,
};
char *gLobbyMsgPages_0_13[4] = {
    D_008485B0, D_008485B8, NULL, NULL,
};
char *gLobbyMsgPages_0_14[4] = {
    D_008485C0, D_008485C8, D_008485D0, NULL,
};
char *gLobbyMsgPages_0_15[4] = {
    D_008485B0, D_008485E8, D_008485B8, NULL,
};
char *gLobbyMsgPages_0_16[4] = {
    D_008485F0, D_008485B8, NULL, NULL,
};
char *gLobbyMsgPages_0_17[4] = {
    D_008485F8, D_00848608, D_00848620, NULL,
};
char *gLobbyMsgPages_0_18[4] = {
    D_00848640, D_00848648, NULL, NULL,
};
char *gLobbyMsgPages_0_19[2] = {
    D_00848650, NULL,
};
char *gLobbyMsgPages_0_20[2] = {
    D_00847A00, NULL,
};
char *gLobbyMsgPages_0_21[8] = {
    D_00848670, D_00848688, D_008486A0, D_008485B8,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_0_22[8] = {
    D_008486C0, D_008486E0, D_00848708, D_008485B8,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_0_23[8] = {
    D_00848720, D_00848770, D_008487C0, D_00848800,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_0_24[8] = {
    D_00848830, D_00848880, D_00848900, D_00848980,
    NULL, NULL, NULL, NULL,
};
char **gLobbyMsgPages_0[26] = {
    NULL, gLobbyMsgPages_0_1, gLobbyMsgPages_0_2, gLobbyMsgPages_0_3,
    gLobbyMsgPages_0_4, D_008FABC8, D_008FABD0, D_008FABD8,
    D_008FABE0, D_008FABE8, gLobbyMsgPages_0_10, gLobbyMsgPages_0_11,
    D_008FABF0, gLobbyMsgPages_0_13, gLobbyMsgPages_0_14, gLobbyMsgPages_0_15,
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
    gLobbyMsgPages_2_3_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8,
    NULL, NULL,
};
char *gLobbyMsgPages_2_4[2] = {
    D_00848A00, NULL,
};
char *gLobbyMsgPages_2_5[2] = {
    gLobbyMsgPages_2_5_0, NULL,
};
char *gLobbyMsgPages_2_6[2] = {
    gLobbyMsgPages_2_6_0, NULL,
};
char *gLobbyMsgPages_2_7[4] = {
    D_00848A80, NULL, NULL, NULL,
};
char *gLobbyMsgPages_2_8[8] = {
    D_00848AA0, D_00848AD0, D_00848AE0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_9[8] = {
    D_00848B10, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_10[8] = {
    gLobbyMsgPages_2_10_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_11[8] = {
    D_00848B80, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_12[8] = {
    gLobbyMsgPages_2_12_0, D_00848AE0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_13[8] = {
    D_00848BE0, D_00848AE0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_14[8] = {
    D_00848C10, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_15[8] = {
    gLobbyMsgPages_2_15_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_16[8] = {
    gLobbyMsgPages_2_16_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_17[8] = {
    gLobbyMsgPages_2_17_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_18[8] = {
    D_00848CA0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_19[8] = {
    gLobbyMsgPages_2_19_0, D_00848AE0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_20[8] = {
    D_00848CF0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_21[8] = {
    D_00848D10, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_22[8] = {
    gLobbyMsgPages_2_22_0, D_00848D70, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_23[8] = {
    D_00848DA0, D_00848D70, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_24[8] = {
    gLobbyMsgPages_2_24_0, D_00848D70, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_25[8] = {
    D_00848D10, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_26[8] = {
    D_00848DE0, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *gLobbyMsgPages_2_27[8] = {
    D_00848C10, gNetModeTitleTexts_0, gNetModeTitleTexts_0, gNetModeTitleTexts_0,
    gNetModeTitleTexts_0, gNetModeTitleTexts_0, D_008489F8, NULL,
};
char *D_0081AFB0[4] = {
    D_00848E00, NULL, NULL, NULL,
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
char* *D_0081B030 = D_0081AFB0;
char *gLobbyMsgPages_3_1[10] = {
    gLobbyMsgPages_3_1_0, gNetModeTitleTexts_0, D_00848E70, D_00848EA0,
    D_00848ED0, gNetModeTitleTexts_0, D_00848F00, D_00848F30,
    NULL, NULL,
};
char *gLobbyMsgPages_3_2[3] = {
    gLobbyMsgPages_3_2_0, D_00848F90, NULL,
};
char *gLobbyMsgPages_3_3[3] = {
    D_00848FB0, D_00848FD0, NULL,
};
char *gLobbyMsgPages_3_4[4] = {
    D_00847370, D_008473A0, D_00848FF0, NULL,
};
char *gLobbyMsgPages_3_5[8] = {
    gLobbyMsgPages_3_5_0, D_00848F00, D_00849050, D_00849080,
    gNetModeTitleTexts_0, D_00848FB0, D_00848FD0, NULL,
};
char *gLobbyMsgPages_3_6[4] = {
    D_00847A40, D_00847A60, D_008490C0, NULL,
};
char *gLobbyMsgPages_3_7[4] = {
    D_00847580, D_008475F0, D_008490E0, NULL,
};
char *gLobbyMsgPages_3_8[8] = {
    D_00847580, D_008475F0, D_00849100, D_008490E0,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_3_9[4] = {
    D_00847580, D_00847A60, D_00849120, NULL,
};
char *gLobbyMsgPages_3_10[4] = {
    D_008479C0, D_00847510, NULL, NULL,
};
char *gLobbyMsgPages_3_11[4] = {
    D_00847580, D_00847750, D_00849140, NULL,
};
char *gLobbyMsgPages_3_12[4] = {
    D_00847580, D_00847750, D_00849170, NULL,
};
char *gLobbyMsgPages_3_13[6] = {
    D_008491A0, D_00847580, D_00847750, D_008491B8,
    D_008491D0, NULL,
};
char *gLobbyMsgPages_3_14[2] = {
    D_00849200, NULL,
};
char *gLobbyMsgPages_3_15[4] = {
    D_008491A0, D_00847580, D_00849220, D_00849240,
};
char **gLobbyMsgPages_3[16] = {
    NULL, gLobbyMsgPages_3_1, gLobbyMsgPages_3_2, gLobbyMsgPages_3_3,
    gLobbyMsgPages_3_4, gLobbyMsgPages_3_5, gLobbyMsgPages_3_6, gLobbyMsgPages_3_7,
    gLobbyMsgPages_3_8, gLobbyMsgPages_3_9, gLobbyMsgPages_3_10, gLobbyMsgPages_3_11,
    gLobbyMsgPages_3_12, gLobbyMsgPages_3_13, gLobbyMsgPages_3_14, gLobbyMsgPages_3_15,
};
char *gLobbyMsgPages_4_1[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849260, D_00849290,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_2[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849260, D_00849290,
    D_008492C0, D_00849300, D_00849330, NULL,
};
char *gLobbyMsgPages_4_3[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849370, D_008493A0,
    D_008493D0, D_00849400, NULL, NULL,
};
char *gLobbyMsgPages_4_4[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849420, gNetModeTitleTexts_0,
    D_00849450, D_00849490, NULL, NULL,
};
char *gLobbyMsgPages_4_5[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_008494D0, D_00849420,
    gNetModeTitleTexts_0, D_00849450, D_00849490, NULL,
};
char *gLobbyMsgPages_4_6[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849500, D_00849540,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_7[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849560, gNetModeTitleTexts_0,
    D_00849590, D_008495C0, D_008495F0, NULL,
};
char *gLobbyMsgPages_4_8[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849620, D_00849650,
    D_00849680, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_9[10] = {
    gLobbyMsgPages_4_9_0, gLobbyMsgPages_4_9_1, D_00849700, D_00849730,
    gNetModeTitleTexts_0, D_00849300, D_00849760, D_00849780,
    D_008497A0, NULL,
};
char *gLobbyMsgPages_4_10[3] = {
    gLobbyMsgPages_4_10_0, D_008497F0, NULL,
};
char *gLobbyMsgPages_4_11[3] = {
    D_00849810, D_008497F0, NULL,
};
char *gLobbyMsgPages_4_12[2] = {
    D_00849830, NULL,
};
char *gLobbyMsgPages_4_13[4] = {
    gLobbyMsgPages_4_9_1, D_00849848, D_00849860, NULL,
};
char *gLobbyMsgPages_4_14[4] = {
    D_00849880, D_008498A0, D_008498D0, NULL,
};
char *gLobbyMsgPages_4_15[12] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849900, D_00849930,
    D_00849970, D_008499A0, D_008499D0, D_00849A00,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_16[12] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849A40, D_00849A80,
    D_00849AB0, gNetModeTitleTexts_0, D_00849AF0, D_00849B30,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_4_17[8] = {
    D_008491A0, gNetModeTitleTexts_0, D_00849A40, D_00849B60,
    D_00849BA0, NULL, NULL, NULL,
};
char **gLobbyMsgPages_4[20] = {
    NULL, gLobbyMsgPages_4_1, gLobbyMsgPages_4_2, gLobbyMsgPages_4_3,
    gLobbyMsgPages_4_4, gLobbyMsgPages_4_5, gLobbyMsgPages_4_6, gLobbyMsgPages_4_7,
    gLobbyMsgPages_4_8, gLobbyMsgPages_4_9, gLobbyMsgPages_4_10, gLobbyMsgPages_4_11,
    gLobbyMsgPages_4_12, gLobbyMsgPages_4_13, gLobbyMsgPages_4_14, gLobbyMsgPages_4_15,
    gLobbyMsgPages_4_16, gLobbyMsgPages_4_17, NULL, NULL,
};
char *gLobbyMsgPages_5_1[8] = {
    D_008479C0, D_00849BE0, D_00847370, D_008473A0,
    D_00848FF0, NULL, NULL, NULL,
};
char *gLobbyMsgPages_5_2[8] = {
    D_00847580, D_00847750, D_00847700, D_00849C00,
    D_00847370, D_008473A0, D_00848FF0, NULL,
};
char *gLobbyMsgPages_5_3[4] = {
    D_00847580, D_00847750, D_00849C20, NULL,
};
char *gLobbyMsgPages_5_4[8] = {
    D_00847A40, D_00847A60, D_00849C50, D_00847AA0,
    D_00847850, D_00847A40, D_00849C68, NULL,
};
char *gLobbyMsgPages_5_5[8] = {
    D_00847580, D_00847750, D_00847700, D_00849C80,
    NULL, NULL, NULL, NULL,
};
char *gLobbyMsgPages_5_6[8] = {
    D_00847580, D_008475F0, D_00849CA0, D_00847AA0,
    D_00847850, D_00847A40, D_00849C68, NULL,
};
char **gLobbyMsgPages_5[8] = {
    NULL, gLobbyMsgPages_5_1, gLobbyMsgPages_5_2, gLobbyMsgPages_5_3,
    gLobbyMsgPages_5_4, gLobbyMsgPages_5_5, gLobbyMsgPages_5_6, NULL,
};
char *gLobbyMsgPages_6_1[2] = {
    D_00847A20, NULL,
};
char *gLobbyMsgPages_6_2[2] = {
    gLobbyMsgPages_6_2_0, NULL,
};
char *gLobbyMsgPages_6_3[2] = {
    D_00849CE0, NULL,
};
char *gLobbyMsgPages_6_4[2] = {
    D_00849D00, NULL,
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
    gNetModeTitleTexts_0, D_00849D58, D_00849D70, D_00849D88,
    D_00849D98, D_00849DB0, D_00849DC0, D_00849DD0,
};
s16 gLobbyMsgPage4Layout[16][3][2] = {
#include "lobby/0081B530.inc"
};
s16 gLobbyMsgLayout[8][3][2] = {
#include "lobby/0081B5F0.inc"
};

/* unk_7D2C80's .rodata (0x847370-0x849DF0) */
#pragma rodata on
char D_00847370[0x30] = {
#include "lobby/00847370.inc"
};
char D_008473A0[0x30] = {
#include "lobby/008473A0.inc"
};
char D_008473D0[0x28] = {
#include "lobby/008473D0.inc"
};
#pragma explicit_zero_data on
char gNetModeTitleTexts_0[8] = {
#include "lobby/008473F8.inc"
};
#pragma explicit_zero_data reset
char D_00847400[0x20] = {
#include "lobby/00847400.inc"
};
char D_00847420[0x20] = {
#include "lobby/00847420.inc"
};
char D_00847440[0x30] = {
#include "lobby/00847440.inc"
};
char D_00847470[0x20] = {
#include "lobby/00847470.inc"
};
char D_00847490[0x30] = {
#include "lobby/00847490.inc"
};
char D_008474C0[0x30] = {
#include "lobby/008474C0.inc"
};
char D_008474F0[0x20] = {
#include "lobby/008474F0.inc"
};
char D_00847510[0x20] = {
#include "lobby/00847510.inc"
};
char D_00847530[0x20] = {
#include "lobby/00847530.inc"
};
char D_00847550[0x30] = {
#include "lobby/00847550.inc"
};
char D_00847580[0x20] = {
#include "lobby/00847580.inc"
};
char D_008475A0[0x30] = {
#include "lobby/008475A0.inc"
};
char D_008475D0[0x20] = {
#include "lobby/008475D0.inc"
};
char D_008475F0[0x20] = {
#include "lobby/008475F0.inc"
};
char D_00847610[0x20] = {
#include "lobby/00847610.inc"
};
char D_00847630[0x30] = {
#include "lobby/00847630.inc"
};
char D_00847660[0x20] = {
#include "lobby/00847660.inc"
};
char D_00847680[0x30] = {
#include "lobby/00847680.inc"
};
char D_008476B0[0x20] = {
#include "lobby/008476B0.inc"
};
char D_008476D0[0x30] = {
#include "lobby/008476D0.inc"
};
char D_00847700[0x20] = {
#include "lobby/00847700.inc"
};
char D_00847720[0x30] = {
#include "lobby/00847720.inc"
};
char D_00847750[0x20] = {
#include "lobby/00847750.inc"
};
char D_00847770[0x30] = {
#include "lobby/00847770.inc"
};
char D_008477A0[0x30] = {
#include "lobby/008477A0.inc"
};
char D_008477D0[0x30] = {
#include "lobby/008477D0.inc"
};
char D_00847800[0x20] = {
#include "lobby/00847800.inc"
};
char D_00847820[0x30] = {
#include "lobby/00847820.inc"
};
char D_00847850[0x20] = {
#include "lobby/00847850.inc"
};
char D_00847870[0x20] = {
#include "lobby/00847870.inc"
};
char D_00847890[0x30] = {
#include "lobby/00847890.inc"
};
char D_008478C0[0x20] = {
#include "lobby/008478C0.inc"
};
char D_008478E0[0x20] = {
#include "lobby/008478E0.inc"
};
char D_00847900[0x30] = {
#include "lobby/00847900.inc"
};
char D_00847930[0x20] = {
#include "lobby/00847930.inc"
};
char D_00847950[0x20] = {
#include "lobby/00847950.inc"
};
char D_00847970[0x20] = {
#include "lobby/00847970.inc"
};
char D_00847990[0x30] = {
#include "lobby/00847990.inc"
};
char D_008479C0[0x20] = {
#include "lobby/008479C0.inc"
};
char D_008479E0[0x20] = {
#include "lobby/008479E0.inc"
};
char D_00847A00[0x20] = {
#include "lobby/00847A00.inc"
};
char D_00847A20[0x20] = {
#include "lobby/00847A20.inc"
};
char D_00847A40[0x20] = {
#include "lobby/00847A40.inc"
};
char D_00847A60[0x20] = {
#include "lobby/00847A60.inc"
};
char D_00847A80[0x20] = {
#include "lobby/00847A80.inc"
};
char D_00847AA0[0x20] = {
#include "lobby/00847AA0.inc"
};
char D_00847AC0[0x10] = {
#include "lobby/00847AC0.inc"
};
char D_00847AD0[0x20] = {
#include "lobby/00847AD0.inc"
};
char D_00847AF0[0x20] = {
#include "lobby/00847AF0.inc"
};
char D_00847B10[0x20] = {
#include "lobby/00847B10.inc"
};
char D_00847B30[0x20] = {
#include "lobby/00847B30.inc"
};
char D_00847B50[0x30] = {
#include "lobby/00847B50.inc"
};
char D_00847B80[0x30] = {
#include "lobby/00847B80.inc"
};
char D_00847BB0[0x30] = {
#include "lobby/00847BB0.inc"
};
char D_00847BE0[0x30] = {
#include "lobby/00847BE0.inc"
};
char D_00847C10[0x20] = {
#include "lobby/00847C10.inc"
};
char D_00847C30[0x30] = {
#include "lobby/00847C30.inc"
};
char D_00847C60[0x30] = {
#include "lobby/00847C60.inc"
};
char D_00847C90[0x20] = {
#include "lobby/00847C90.inc"
};
char D_00847CB0[0x30] = {
#include "lobby/00847CB0.inc"
};
char D_00847CE0[0x30] = {
#include "lobby/00847CE0.inc"
};
char D_00847D10[0x10] = {
#include "lobby/00847D10.inc"
};
char D_00847D20[0x30] = {
#include "lobby/00847D20.inc"
};
char D_00847D50[0x30] = {
#include "lobby/00847D50.inc"
};
char D_00847D80[0x20] = {
#include "lobby/00847D80.inc"
};
char D_00847DA0[0x20] = {
#include "lobby/00847DA0.inc"
};
char D_00847DC0[0x20] = {
#include "lobby/00847DC0.inc"
};
char D_00847DE0[0x20] = {
#include "lobby/00847DE0.inc"
};
char D_00847E00[0x30] = {
#include "lobby/00847E00.inc"
};
char D_00847E30[0x20] = {
#include "lobby/00847E30.inc"
};
char D_00847E50[0x20] = {
#include "lobby/00847E50.inc"
};
char D_00847E70[0x18] = {
#include "lobby/00847E70.inc"
};
char D_00847E88[0x8] = {
#include "lobby/00847E88.inc"
};
char D_00847E90[0x20] = {
#include "lobby/00847E90.inc"
};
char D_00847EB0[0x30] = {
#include "lobby/00847EB0.inc"
};
char D_00847EE0[0x20] = {
#include "lobby/00847EE0.inc"
};
char D_00847F00[0x30] = {
#include "lobby/00847F00.inc"
};
char D_00847F30[0x30] = {
#include "lobby/00847F30.inc"
};
char D_00847F60[0x30] = {
#include "lobby/00847F60.inc"
};
char D_00847F90[0x20] = {
#include "lobby/00847F90.inc"
};
char D_00847FB0[0x30] = {
#include "lobby/00847FB0.inc"
};
char D_00847FE0[0x30] = {
#include "lobby/00847FE0.inc"
};
char D_00848010[0x40] = {
#include "lobby/00848010.inc"
};
char D_00848050[0x30] = {
#include "lobby/00848050.inc"
};
char D_00848080[0x30] = {
#include "lobby/00848080.inc"
};
char D_008480B0[0x10] = {
#include "lobby/008480B0.inc"
};
char D_008480C0[0x40] = {
#include "lobby/008480C0.inc"
};
char D_00848100[0x30] = {
#include "lobby/00848100.inc"
};
char D_00848130[0x20] = {
#include "lobby/00848130.inc"
};
char gLobbyMsgPages_1_49_0[0x30] = {
#include "lobby/00848150.inc"
};
char D_00848180[0x30] = {
#include "lobby/00848180.inc"
};
char D_008481B0[0x30] = {
#include "lobby/008481B0.inc"
};
char D_008481E0[0x30] = {
#include "lobby/008481E0.inc"
};
char D_00848210[0x30] = {
#include "lobby/00848210.inc"
};
char gLobbyMsgPages_1_53_0[0x30] = {
#include "lobby/00848240.inc"
};
char D_00848270[0x20] = {
#include "lobby/00848270.inc"
};
char D_00848290[0x30] = {
#include "lobby/00848290.inc"
};
char gLobbyMsgPages_0_1_0[0x20] = {
#include "lobby/008482C0.inc"
};
char D_008482E0[0x20] = {
#include "lobby/008482E0.inc"
};
char D_00848300[0x30] = {
#include "lobby/00848300.inc"
};
char D_00848330[0x30] = {
#include "lobby/00848330.inc"
};
char gLobbyMsgPages_0_4_0[0x40] = {
#include "lobby/00848360.inc"
};
char D_008483A0[0x20] = {
#include "lobby/008483A0.inc"
};
char D_008483C0[0x30] = {
#include "lobby/008483C0.inc"
};
char D_008483F0[0x30] = {
#include "lobby/008483F0.inc"
};
char D_00848420[0x30] = {
#include "lobby/00848420.inc"
};
char D_00848450[0x20] = {
#include "lobby/00848450.inc"
};
char D_00848470[0x30] = {
#include "lobby/00848470.inc"
};
char D_008484A0[0x20] = {
#include "lobby/008484A0.inc"
};
char D_008484C0[0x30] = {
#include "lobby/008484C0.inc"
};
char D_008484F0[0x30] = {
#include "lobby/008484F0.inc"
};
char D_00848520[0x30] = {
#include "lobby/00848520.inc"
};
char D_00848550[0x30] = {
#include "lobby/00848550.inc"
};
char D_00848580[0x30] = {
#include "lobby/00848580.inc"
};
char D_008485B0[0x8] = {
#include "lobby/008485B0.inc"
};
char D_008485B8[0x8] = {
#include "lobby/008485B8.inc"
};
char D_008485C0[0x8] = {
#include "lobby/008485C0.inc"
};
char D_008485C8[0x8] = {
#include "lobby/008485C8.inc"
};
char D_008485D0[0x18] = {
#include "lobby/008485D0.inc"
};
char D_008485E8[0x8] = {
#include "lobby/008485E8.inc"
};
char D_008485F0[0x8] = {
#include "lobby/008485F0.inc"
};
char D_008485F8[0xC] = {
#include "lobby/008485F8.inc"
};
char D_00848608[0xC] = {
#include "lobby/00848608.inc"
};
char D_00848620[0x20] = {
#include "lobby/00848620.inc"
};
char D_00848640[0x8] = {
#include "lobby/00848640.inc"
};
char D_00848648[0x8] = {
#include "lobby/00848648.inc"
};
char D_00848650[0x20] = {
#include "lobby/00848650.inc"
};
char D_00848670[0x30] = {
#include "lobby/00848670.inc"
};
char D_008486A0[0x20] = {
#include "lobby/008486A0.inc"
};
char D_008486C0[0x20] = {
#include "lobby/008486C0.inc"
};
char D_008486E0[0x40] = {
#include "lobby/008486E0.inc"
};
char D_00848720[0x50] = {
#include "lobby/00848720.inc"
};
char D_00848770[0x50] = {
#include "lobby/00848770.inc"
};
char D_008487C0[0x40] = {
#include "lobby/008487C0.inc"
};
char D_00848800[0x30] = {
#include "lobby/00848800.inc"
};
char D_00848830[0x50] = {
#include "lobby/00848830.inc"
};
char D_00848880[0x80] = {
#include "lobby/00848880.inc"
};
char D_00848900[0x80] = {
#include "lobby/00848900.inc"
};
char D_00848980[0x30] = {
#include "lobby/00848980.inc"
};
char gLobbyMsgPages_2_1_0[0x20] = {
#include "lobby/008489B0.inc"
};
char gLobbyMsgPages_2_3_0[0x28] = {
#include "lobby/008489D0.inc"
};
char D_008489F8[0x8] = {
#include "lobby/008489F8.inc"
};
char D_00848A00[0x50] = {
#include "lobby/00848A00.inc"
};
char gLobbyMsgPages_2_5_0[0x20] = {
#include "lobby/00848A50.inc"
};
char gLobbyMsgPages_2_6_0[0x10] = {
#include "lobby/00848A70.inc"
};
char D_00848A80[0x20] = {
#include "lobby/00848A80.inc"
};
char D_00848AA0[0x30] = {
#include "lobby/00848AA0.inc"
};
char D_00848AD0[0x10] = {
#include "lobby/00848AD0.inc"
};
char D_00848AE0[0x30] = {
#include "lobby/00848AE0.inc"
};
char D_00848B10[0x30] = {
#include "lobby/00848B10.inc"
};
char gLobbyMsgPages_2_10_0[0x40] = {
#include "lobby/00848B40.inc"
};
char D_00848B80[0x30] = {
#include "lobby/00848B80.inc"
};
char gLobbyMsgPages_2_12_0[0x30] = {
#include "lobby/00848BB0.inc"
};
char D_00848BE0[0x30] = {
#include "lobby/00848BE0.inc"
};
char D_00848C10[0x20] = {
#include "lobby/00848C10.inc"
};
char gLobbyMsgPages_2_15_0[0x20] = {
#include "lobby/00848C30.inc"
};
char gLobbyMsgPages_2_16_0[0x20] = {
#include "lobby/00848C50.inc"
};
char gLobbyMsgPages_2_17_0[0x30] = {
#include "lobby/00848C70.inc"
};
char D_00848CA0[0x30] = {
#include "lobby/00848CA0.inc"
};
char gLobbyMsgPages_2_19_0[0x20] = {
#include "lobby/00848CD0.inc"
};
char D_00848CF0[0x20] = {
#include "lobby/00848CF0.inc"
};
char D_00848D10[0x30] = {
#include "lobby/00848D10.inc"
};
char gLobbyMsgPages_2_22_0[0x30] = {
#include "lobby/00848D40.inc"
};
char D_00848D70[0x30] = {
#include "lobby/00848D70.inc"
};
char D_00848DA0[0x30] = {
#include "lobby/00848DA0.inc"
};
char gLobbyMsgPages_2_24_0[0x10] = {
#include "lobby/00848DD0.inc"
};
char D_00848DE0[0x20] = {
#include "lobby/00848DE0.inc"
};
char D_00848E00[0x40] = {
#include "lobby/00848E00.inc"
};
char gLobbyMsgPages_3_1_0[0x30] = {
#include "lobby/00848E40.inc"
};
char D_00848E70[0x30] = {
#include "lobby/00848E70.inc"
};
char D_00848EA0[0x30] = {
#include "lobby/00848EA0.inc"
};
char D_00848ED0[0x30] = {
#include "lobby/00848ED0.inc"
};
char D_00848F00[0x30] = {
#include "lobby/00848F00.inc"
};
char D_00848F30[0x30] = {
#include "lobby/00848F30.inc"
};
char gLobbyMsgPages_3_2_0[0x30] = {
#include "lobby/00848F60.inc"
};
char D_00848F90[0x20] = {
#include "lobby/00848F90.inc"
};
char D_00848FB0[0x20] = {
#include "lobby/00848FB0.inc"
};
char D_00848FD0[0x20] = {
#include "lobby/00848FD0.inc"
};
char D_00848FF0[0x30] = {
#include "lobby/00848FF0.inc"
};
char gLobbyMsgPages_3_5_0[0x30] = {
#include "lobby/00849020.inc"
};
char D_00849050[0x30] = {
#include "lobby/00849050.inc"
};
char D_00849080[0x40] = {
#include "lobby/00849080.inc"
};
char D_008490C0[0x20] = {
#include "lobby/008490C0.inc"
};
char D_008490E0[0x20] = {
#include "lobby/008490E0.inc"
};
char D_00849100[0x20] = {
#include "lobby/00849100.inc"
};
char D_00849120[0x20] = {
#include "lobby/00849120.inc"
};
char D_00849140[0x30] = {
#include "lobby/00849140.inc"
};
char D_00849170[0x30] = {
#include "lobby/00849170.inc"
};
char D_008491A0[0x30] = {
#include "lobby/008491A0.inc"
};
char D_008491D0[0x30] = {
#include "lobby/008491D0.inc"
};
char D_00849200[0x20] = {
#include "lobby/00849200.inc"
};
char D_00849220[0x20] = {
#include "lobby/00849220.inc"
};
char D_00849240[0x20] = {
#include "lobby/00849240.inc"
};
char D_00849260[0x30] = {
#include "lobby/00849260.inc"
};
char D_00849290[0x30] = {
#include "lobby/00849290.inc"
};
char D_008492C0[0x40] = {
#include "lobby/008492C0.inc"
};
char D_00849300[0x30] = {
#include "lobby/00849300.inc"
};
char D_00849330[0x40] = {
#include "lobby/00849330.inc"
};
char D_00849370[0x30] = {
#include "lobby/00849370.inc"
};
char D_008493A0[0x30] = {
#include "lobby/008493A0.inc"
};
char D_008493D0[0x30] = {
#include "lobby/008493D0.inc"
};
char D_00849400[0x20] = {
#include "lobby/00849400.inc"
};
char D_00849420[0x30] = {
#include "lobby/00849420.inc"
};
char D_00849450[0x40] = {
#include "lobby/00849450.inc"
};
char D_00849490[0x40] = {
#include "lobby/00849490.inc"
};
char D_008494D0[0x30] = {
#include "lobby/008494D0.inc"
};
char D_00849500[0x40] = {
#include "lobby/00849500.inc"
};
char D_00849540[0x20] = {
#include "lobby/00849540.inc"
};
char D_00849560[0x30] = {
#include "lobby/00849560.inc"
};
char D_00849590[0x30] = {
#include "lobby/00849590.inc"
};
char D_008495C0[0x30] = {
#include "lobby/008495C0.inc"
};
char D_008495F0[0x30] = {
#include "lobby/008495F0.inc"
};
char D_00849620[0x30] = {
#include "lobby/00849620.inc"
};
char D_00849650[0x30] = {
#include "lobby/00849650.inc"
};
char D_00849680[0x30] = {
#include "lobby/00849680.inc"
};
char gLobbyMsgPages_4_9_0[0x20] = {
#include "lobby/008496B0.inc"
};
char gLobbyMsgPages_4_9_1[0x30] = {
#include "lobby/008496D0.inc"
};
char D_00849700[0x30] = {
#include "lobby/00849700.inc"
};
char D_00849730[0x30] = {
#include "lobby/00849730.inc"
};
char D_00849760[0x20] = {
#include "lobby/00849760.inc"
};
char D_00849780[0x20] = {
#include "lobby/00849780.inc"
};
char D_008497A0[0x20] = {
#include "lobby/008497A0.inc"
};
char gLobbyMsgPages_4_10_0[0x30] = {
#include "lobby/008497C0.inc"
};
char D_008497F0[0x20] = {
#include "lobby/008497F0.inc"
};
char D_00849810[0x20] = {
#include "lobby/00849810.inc"
};
char D_00849830[0x18] = {
#include "lobby/00849830.inc"
};
char D_00849848[0xC] = {
#include "lobby/00849848.inc"
};
char D_00849860[0x20] = {
#include "lobby/00849860.inc"
};
char D_00849880[0x20] = {
#include "lobby/00849880.inc"
};
char D_008498A0[0x30] = {
#include "lobby/008498A0.inc"
};
char D_008498D0[0x30] = {
#include "lobby/008498D0.inc"
};
char D_00849900[0x30] = {
#include "lobby/00849900.inc"
};
char D_00849930[0x40] = {
#include "lobby/00849930.inc"
};
char D_00849970[0x30] = {
#include "lobby/00849970.inc"
};
char D_008499A0[0x30] = {
#include "lobby/008499A0.inc"
};
char D_008499D0[0x30] = {
#include "lobby/008499D0.inc"
};
char D_00849A00[0x40] = {
#include "lobby/00849A00.inc"
};
char D_00849A40[0x40] = {
#include "lobby/00849A40.inc"
};
char D_00849A80[0x30] = {
#include "lobby/00849A80.inc"
};
char D_00849AB0[0x40] = {
#include "lobby/00849AB0.inc"
};
char D_00849AF0[0x40] = {
#include "lobby/00849AF0.inc"
};
char D_00849B30[0x30] = {
#include "lobby/00849B30.inc"
};
char D_00849B60[0x40] = {
#include "lobby/00849B60.inc"
};
char D_00849BA0[0x40] = {
#include "lobby/00849BA0.inc"
};
char D_00849BE0[0x20] = {
#include "lobby/00849BE0.inc"
};
char D_00849C00[0x20] = {
#include "lobby/00849C00.inc"
};
char D_00849C20[0x30] = {
#include "lobby/00849C20.inc"
};
char D_00849C50[0x30] = {
#include "lobby/00849C50.inc"
};
char D_00849C80[0x20] = {
#include "lobby/00849C80.inc"
};
char D_00849CA0[0x20] = {
#include "lobby/00849CA0.inc"
};
char gLobbyMsgPages_6_2_0[0x20] = {
#include "lobby/00849CC0.inc"
};
char D_00849CE0[0x20] = {
#include "lobby/00849CE0.inc"
};
char D_00849D00[0x20] = {
#include "lobby/00849D00.inc"
};
char gLobbyMsgPages_6_5_0[0x20] = {
#include "lobby/00849D20.inc"
};
char gLobbyMsgPages_6_6_0[0x18] = {
#include "lobby/00849D40.inc"
};
char D_00849D58[0xC] = {
#include "lobby/00849D58.inc"
};
char D_00849D70[0x40] = {
#include "lobby/00849D70.inc"
};
char D_00849DB0[0x10] = {
#include "lobby/00849DB0.inc"
};
char D_00849DC0[0x10] = {
#include "lobby/00849DC0.inc"
};
char D_00849DD0[0x20] = {
#include "lobby/00849DD0.inc"
};
#pragma rodata off
