#include "common.h"
#include "chat_log.h"
#include "em.h"
#include "scene.h"
#include "lobby_globals.h"
#include "syssave.h"
#include "window.h"
#include "sprite.h"
#include "lobby/unk_7B8610.h"   /* the texts this file uses (tools/content_gen.py) */
#include "area.h"
char *strcpy(char *, const char *);

/* prototypes */
void ChatLog_DrawFrame(s32, u8, u8);
static s32 ChatLog_Lines(void);
void ChatLog_DrawLines(u8 skip, s16 yoff, u8 fromTop, u8 size, u8 atEnd);
void Chat_DrawWindow(u8);
void Gfx_SetTexFilter(s32);
/* end of prototypes */
void Chat_FilterWords(char *s);
void ChatLog_Add(char *text, ChatMsg *from, s32 a);
s32 strcmp(void *a, void *b);
void Chat_Beep(void);
extern u8 gMyUserId[];

s32 sprintf(char *dst, const char *fmt, ...);  /* sprintf */

void *memset(void *dst, s32 c, u32 n);
extern u8 gChatBlockList[];

/* clears the chat log (the timer at +0, the unread flag at +2, the lines)
 * and the list at sub_main 0x5C3730 */
void ChatLog_Clear(void)
{
    gChatLog.win.head = 0;
    gChatLog.win.count = 0;
    gChatLog.win.atEnd = 1;
    gChatLog.win.scroll = 0;
    gChatLog.win.open = gChatLog.win.timer = 0;
    gChatLog.unk1848 = 0;
    memset(gChatBlockList, 0, 0x1E0);
}

/* adds a line to the chat log: the sender's id and name, its class and
 * colours, the text cut in rows of 30 bytes (two at most, a Shift-JIS pair
 * kept whole) */
void ChatLog_Add(char *text, ChatMsg *from, s32 a)
{
    ChatLogLine *line;
    s32 n;
    s32 left;
    char *d;
    char *row;
    char c;

    if (*text != 0) {
        line = &gChatLog.lines[gChatLog.win.head];
        line->unk5A = 0xFF;
        line->cls = from->cls;
        line->color[0] = from->color[0];
        line->color[1] = from->color[1];
        line->color[2] = from->color[2];
        strcpy(line->id, from->id);
        strcpy(line->name, from->name);
        gChatLog.win.head++;
        gChatLog.win.count++;
        gChatLog.win.head &= 0x3F;
        if (gChatLog.win.count > 0x40) {
            gChatLog.win.count = 0x40;
        }
        line->unk59 = 1;
        n = 0;
        line->cls = 0;
        row = line->text[0];
        line->rows = 0;
        do {
            d = row;
            if (*text == 0) {
                *row = 0;
                return;
            }
            row[0x1E] = 0;
            left = 0x1E;
            do {
                c = *text;
                if (c == 0) {
                    *d = 0;
                    line->rows++;
                    return;
                }
                if (((u8)c >= 0x80 && (u8)c <= 0x9F) || ((u8)c >= 0xE0 && (u8)c <= 0xFF)) {
                    *d = c;
                    left -= 2;
                    d[1] = text[1];
                    text += 2;
                    d += 2;
                } else {
                    *d = c;
                    text++;
                    d++;
                    left--;
                }
            } while (left > 0);
            *d = 0;
            n++;
            row += 0x20;
            line->rows++;
        } while (n < 2);
    }
}

/* adds a received chat line (a NetUser-like record: the text at +0x1C) to
 * the log; a line not from the player makes a sound when the chat window
 * isn't showing */
void ChatLog_Receive(ChatMsg *msg)
{
    Chat_FilterWords(msg->text);
    ChatLog_Add(msg->text, msg, 1);
    if (ChatLog_Lines() > 11U) {
        (gChatLog.win.scroll)++;
        if (gChatLog.win.scroll > 0x3F) {
            gChatLog.win.scroll = 0x3F;
        }
    }
    if (strcmp(msg->id, gMyUserId) != 0 && gChatLog.win.open == 0) {
        if ((gScene.area != AREA_HUB_0x83) && (gChatSendTo != 0)) {
            if (gSysSave.options.chatBeep == 0) {
                goto play;
            }
        } else {
play:
            Chat_Beep();
        }
    }
    if (gChatLog.win.timer == 0) {
        gChatLog.win.open = 1;
    }
    gChatLog.win.timer = 300;
}

/* the chat windows (ListPanel_Draw): by text size, then the log's */
extern ListPanel gChatLogBoxes[];
extern Icon gChatLogIcons[][4];   /* by text size: the title marks 1 to 4 (ChatLog_DrawFrame) */
extern char *gAreaChatTexts[];              /* the chat window's titles and marks */
extern u8 gChatTextSizes[];                   /* sub_main: the chat text sizes */
s8 Kbd_IsOpen(void);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Gfx_DrawGauge(s16 x, s16 y, s16 w, s16 h, s32 flags);
struct Icon;
void Draw_Icons(struct Icon *, u8 count);

/* the chat window of text size size (+3: the log's, when atEnd): its box
 * and title; with flags 0x80, a mark after the title (1 new, 2 more, 3-4 the
 * input...); flags 1: no title */
void ChatLog_DrawFrame(s32 flags, u8 size, u8 atEnd)
{
    char buf[0x40];
    s32 noTitle = flags & 1;
    u8 k;
    char *text;
    u8 lines;
    u8 mark = 0;
    ListPanel *b;
    u8 sz;

    if (atEnd != 0) {
        lines = 4;
        k = size + 3;
    } else {
        k = size;
        lines = 0xB;
    }
    if (!noTitle) {
        text = gAreaChatTexts[0];
        if ((flags & 0x80) && (s8)Kbd_IsOpen() == 0) {
            if (gChatWinHold == 0) {
                mark = (atEnd == 0) ? 1 : 2;
            } else {
                mark = 3;
            }
            sprintf(buf, "%s%25s", gAreaChatTexts[0], gAreaChatTexts[mark]);
            text = buf;
        }
    } else {
        text = NULL;
        lines++;
    }
    b = &gChatLogBoxes[k];
    b->lines = lines;
    ListPanel_Draw(b, 0, text, -1);
    if (text != NULL) {
        sz = gChatTextSizes[size];
        Gfx_DrawGauge(b->x, b->y, sz, sz * 18 / 21, 0);
        if (mark != 0) {
            if (atEnd != 0 && mark == 3) {
                mark++;
            }
            Draw_Icons(&gChatLogIcons[gSysSave.options.chatSize][mark - 1], 1);
        }
    }
}

/* the chat log (sub_main 0x5BC500: newest index at +3, count at +4, 64
 * lines of 0x61 bytes from +3): the sum of the line counts at +0x60 of the
 * lines before the last skip ones */
u8 ChatLog_LinesBefore(u8 skip)
{
    s32 k;
    s32 n;
    u8 sum;

    k = gChatLog.win.head - 1 - skip;
    n = gChatLog.win.count - skip;
    sum = 0;
    while (n != 0) {
        k &= 0x3F;
        n--;
        sum += gChatLog.lines[k].rows;
        k--;
    }
    return sum;
}

/* the sum of the chat log's line counts */
/* the sum of the chat log's line counts */
static s32 ChatLog_Lines(void)
{
    s32 sum;
    s32 k;
    s32 n;

    sum = 0;
    k = gChatLog.win.head - 1;
    n = gChatLog.win.count;
    while (n > 0) {
        n--;
        sum += gChatLog.lines[k & 0x3F].rows;
        k--;
    }
    return sum;
}

/* a name, 8 characters, and a full-width colon */
void ChatLog_FormatName(char *dst, char *name)
{
    sprintf(dst, TXT(0x00846BC0) /* a name (eight columns), then a full-width colon */, name);
}

void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Text_SetColor(s16 color);
void Text_Draw(char *s);
void Text_DrawShadow2(s16 x, s16 y, s32 c1, s32 c2, char *s);

/* the chat log's text: from the newest line upwards (skip lines skipped)
 * when it holds more rows than the window (4, or 11 for the log's), else
 * from the oldest line down; each line's sender (0x5BB097: which name)
 * before its first row. */
#pragma optimization_level 4
void ChatLog_DrawLines(u8 skip, s16 yoff, u8 fromTop, u8 size, u8 atEnd)
{
    char name[0x20];
    ChatLogLine *line;
    s32 k;
    s32 n;
    s32 i;
    s16 fh;
    s16 lh;
    u8 rows;
    s16 xo;
    s16 y;
    s16 lim;

    if (gChatLog.win.count == 0) {
        return;
    }
    size = gChatTextSizes[size];
    fh = (s16)size * 18 / 21;
    Font_SetSpacing(size, fh);
    lh = fh + 1;
    rows = atEnd == 0 ? 0xB : 4;
    xo = size * 100 / 21;
    yoff = yoff * size / 21;
    if ((u32)ChatLog_Lines() > rows && fromTop == 0) {
        lim = 0x1A4 - lh * rows;
        k = gChatLog.win.head - 1 - skip;
        n = gChatLog.win.count - skip;
        y = 0x1A4 - lh;
        while (n != 0) {
            k &= 0x3F;
            line = &gChatLog.lines[k];
            for (i = line->rows - 1; i >= 0; i--) {
                Text_SetColor(line->color[2]);
                Font_SetPos(xo + 0x18, y + yoff);
                Text_Draw(line->text[i]);
                if (i == 0) {
                    if (gChatHideNames == 0) {
                        ChatLog_FormatName(name, line->name);
                    } else {
                        ChatLog_FormatName(name, line->id);
                    }
                    Text_DrawShadow2(0x18, y + yoff, 1, line->color[1], name);
                }
                y -= lh;
                if (y < lim) {
                    return;
                }
            }
            n--;
            k--;
        }
    } else {
        n = gChatLog.win.count;
        k = gChatLog.win.head - n;
        y = 0x1A4 - lh * rows;
        while (n != 0) {
            k &= 0x3F;
            line = &gChatLog.lines[k];
            if (gChatHideNames == 0) {
                ChatLog_FormatName(name, line->name);
            } else {
                ChatLog_FormatName(name, line->id);
            }
            Text_DrawShadow2(0x18, y + yoff, 1, line->color[1], name);
            Text_SetColor(line->color[2]);
            for (i = 0; i < line->rows; i++) {
                Font_SetPos(xo + 0x18, y + yoff);
                Text_Draw(line->text[i]);
                y += lh;
                if (y >= 0x1A4) {
                    return;
                }
            }
            n--;
            k++;
        }
    }
}
#pragma optimization_level reset

void ChatLog_DrawFull(void)
{
    u8 size;

    size = *(u8 *)&gSysSave.options.chatSize;
    ChatLog_DrawFrame(0, size, gChatLog.unk1848);
    ChatLog_DrawLines(0, 0, 0, size, gChatLog.unk1848);
    *(s16 *)&gChatLog.win.timer = 0;
}

void LobbyChat_DrawHud(void)
{
    u8 kind;
    u8 size;

    if (gChatLog.win.open != 0) {
        kind = gChatWinState;
        if (kind != 0) {
            Chat_DrawWindow(kind);
            return;
        }
    }
    if (gChatLog.win.timer != 0) {
        Gfx_SetTexFilter(0);
        size = *(u8 *)&gSysSave.options.chatSize;
        ChatLog_DrawFrame(0x80, size, gChatLog.unk1848);
        ChatLog_DrawLines(0, 0, 0, size, gChatLog.unk1848);
    }
}

void ChatLog_ResetScroll(void)
{
    gChatLog.win.scroll = 0;
    if (ChatLog_Lines() < 0xCU) {
        gChatLog.win.atEnd = 1;
        return;
    }
    gChatLog.win.atEnd = 0;
}

/* unk_7B8610's .data (0x819480-0x8195A0) */
ListPanel gChatLogBoxes[6] = {
#include "lobby/00819480.inc"
};
Icon gChatLogIcons[3][4] = {
#include "lobby/00819540.inc"
};
