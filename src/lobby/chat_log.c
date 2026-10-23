/*
 * The lobby's chat log (gChatLog, chat_log.h's ChatLog): the last
 * CHAT_LOG_LINES chat lines in a ring, each with its sender, and the
 * window that shows them over the town.
 * - ChatLog_Clear empties it; ChatLog_Add adds a line, cut into rows;
 *   ChatLog_Receive adds a received message, with the chat sound and the
 *   window shown for CHAT_LOG_SHOW_FRAMES.
 * - The window comes in two heights, the large one (CHAT_LOG_LARGE_ROWS
 *   rows) and the compact one (CHAT_LOG_COMPACT_ROWS; ChatLog.compact,
 *   which the chat button flips while the log is open), and in the three
 *   text sizes of the options (gSysSave.options.chatSize). Its layout is
 *   designed for the text size CHAT_TEXT_BASE_SIZE and scaled to the
 *   others.
 * - ChatLog_DrawFrame draws the window and its title, ChatLog_DrawLines
 *   the lines; ChatLog_DrawFull and LobbyChat_DrawHud draw both.
 */
#include "common.h"
#include "chat_log.h"
#include "em.h"
#include "scene.h"
#include "lobby_globals.h"
#include "syssave.h"
#include "window.h"
#include "sprite.h"
#include "lobby/chat_log.h"   /* the texts this file uses (tools/content_gen.py) */
#include "area.h"
#include "game_menu.h"
#include "pad.h"
#include "sound.h"
#include "text.h"
#include "lobby_labels.h"
#include "render_state.h"
#include "bgm.h"
#include "kbd.h"
#include "chat_phrases.h"
#include "engine_math.h"
#include "font.h"
#include "item_shop.h"
#include "menu_util.h"
#include "libc.h"

/* prototypes */
static s32 ChatLog_Lines(void);
/* end of prototypes */
extern u8 gMyUserId[];

extern u8 gChatBlockList[];

#define CHAT_LOG_LINES 64           /* gChatLog.lines, a ring: indices wrap by & (CHAT_LOG_LINES - 1) */
#define CHAT_LOG_LINE_ROWS 2        /* the rows a line's text fills at most */
#define CHAT_LOG_ROW_BYTES 30       /* the bytes of text in a row (of ChatLogLine.text's 32) */
#define CHAT_LOG_LARGE_ROWS 11      /* the rows the large window shows */
#define CHAT_LOG_COMPACT_ROWS 4     /* the rows the compact window shows */
#define CHAT_LOG_SHOW_FRAMES 300    /* frames: 5 s, the window's time on screen after a message */
#define CHAT_LOG_BOTTOM_Y 420       /* the bottom of the window's text */
#define CHAT_LOG_TEXT_X 24          /* the left of the sender's name */
#define CHAT_TEXT_BASE_SIZE 21      /* the text size the layout is designed for */
#define CHAT_TEXT_SIZES 3          /* the options' text sizes (gChatTextSizes) */

/* ChatLog_DrawFrame's flags. */
#define CHAT_FRAME_NO_TITLE 0x01    /* the window without its title, one more row */
#define CHAT_FRAME_MARK 0x80        /* a mark after the title, while the keyboard is closed */

/* The title's marks (gAreaChatTexts' entries, gChatLogIcons' by mark - 1). */
#define CHAT_MARK_LARGE 1           /* the large window */
#define CHAT_MARK_COMPACT 2         /* the compact window */
#define CHAT_MARK_HOLD 3            /* while gChatWinHold runs; its icon is the next one in the compact window */

/* Empties the chat log: its window's state, the lines, the compact flag,
 * and the list of muted players (gChatBlockList). */
void ChatLog_Clear(void)
{
    gChatLog.win.head = 0;
    gChatLog.win.count = 0;
    gChatLog.win.atEnd = 1;
    gChatLog.win.scroll = 0;
    gChatLog.win.open = gChatLog.win.timer = 0;
    gChatLog.compact = 0;
    memset(gChatBlockList, 0, CHAT_BLOCK_LIST_LEN * CHAT_BLOCK_REC_SIZE);
}

/*
 * Adds a line to the chat log, when text isn't empty: the sender's id,
 * name and colours (from), and the text cut into rows of
 * CHAT_LOG_ROW_BYTES, CHAT_LOG_LINE_ROWS at most, with a two-byte
 * (Shift-JIS) character kept whole. The rest of a longer text is dropped. unused isn't read.
 */
void ChatLog_Add(char *text, ChatMsg *from, s32 unused)
{
    ChatLogLine *line;
    s32 rowCount;
    s32 left;
    char *dst;
    char *row;
    char byte;

    if (*text != 0) {
        line = &gChatLog.lines[gChatLog.win.head];
        line->unk5A = 255;
        line->cls = from->cls;
        line->color[0] = from->color[0];
        line->color[1] = from->color[1];
        line->color[2] = from->color[2];
        strcpy(line->id, from->id);
        strcpy(line->name, from->name);
        gChatLog.win.head++;
        gChatLog.win.count++;
        gChatLog.win.head &= CHAT_LOG_LINES - 1;
        if (gChatLog.win.count > CHAT_LOG_LINES) {
            gChatLog.win.count = CHAT_LOG_LINES;
        }
        line->unk59 = 1;
        rowCount = 0;
        line->cls = 0;
        row = line->text[0];
        line->rows = 0;
        do {
            dst = row;
            if (*text == 0) {
                *row = 0;
                return;
            }
            row[CHAT_LOG_ROW_BYTES] = 0;
            left = CHAT_LOG_ROW_BYTES;
            do {
                byte = *text;
                if (byte == 0) {
                    *dst = 0;
                    line->rows++;
                    return;
                }
                if (SJIS_IS_LEAD((u8)byte)) {
                    *dst = byte;
                    left -= 2;
                    dst[1] = text[1];
                    text += 2;
                    dst += 2;
                } else {
                    *dst = byte;
                    text++;
                    dst++;
                    left--;
                }
            } while (left > 0);
            *dst = 0;
            rowCount++;
            row += sizeof(line->text[0]);
            line->rows++;
        } while (rowCount < CHAT_LOG_LINE_ROWS);
    }
}

/*
 * Adds a received chat message to the log, its words filtered
 * (Chat_FilterWords), and scrolls the log on by a line once it holds more
 * rows than the large window. A message from another player while the
 * log is closed plays the chat sound: always in AREA_ONLINE_VILLAGE or when
 * chatting to the lobby (gChatSendTo CHAT_SEND_LOBBY), else only when the
 * option asks for a sound at every message (chatBeep 0). The window then
 * shows for CHAT_LOG_SHOW_FRAMES, opening if it wasn't showing. The
 * goto lets both conditions share one Chat_Beep call, as the game's code
 * does.
 */
void ChatLog_Receive(ChatMsg *msg)
{
    Chat_FilterWords(msg->text);
    ChatLog_Add(msg->text, msg, 1);
    if (ChatLog_Lines() > (u32)CHAT_LOG_LARGE_ROWS) {
        (gChatLog.win.scroll)++;
        if (gChatLog.win.scroll > CHAT_LOG_LINES - 1) {
            gChatLog.win.scroll = CHAT_LOG_LINES - 1;
        }
    }
    if (strcmp(msg->id, gMyUserId) != 0 && gChatLog.win.open == 0) {
        if ((gScene.area != AREA_ONLINE_VILLAGE) && (gChatSendTo != CHAT_SEND_LOBBY)) {
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
    gChatLog.win.timer = CHAT_LOG_SHOW_FRAMES;
}

/* The chat windows' panels (ListPanel_Draw): the large window's in the
 * three text sizes, then the compact window's. */
extern ListPanel gChatLogBoxes[];
extern Icon gChatLogIcons[][4];   /* by text size: the title's marks 1 to 4 (ChatLog_DrawFrame) */
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
struct Icon;

/*
 * Draws the chat window in text size size, large or compact: its panel
 * and title, with a marker before it. With CHAT_FRAME_MARK and the
 * keyboard closed, the title is followed by a mark and its icon: mark 1
 * in the large window, 2 in the compact one, and 3 (its icon 4 in the
 * compact window) while gChatWinHold runs. CHAT_FRAME_NO_TITLE draws no
 * title and gives the text one more row.
 */
void ChatLog_DrawFrame(s32 flags, u8 size, u8 compact)
{
    char buf[64];
    s32 noTitle = flags & CHAT_FRAME_NO_TITLE;
    u8 panelIndex;
    char *text;
    u8 lines;
    u8 mark = 0;
    ListPanel *panel;
    u8 textSize;

    if (compact != 0) {
        lines = CHAT_LOG_COMPACT_ROWS;
        panelIndex = size + CHAT_TEXT_SIZES;
    } else {
        panelIndex = size;
        lines = CHAT_LOG_LARGE_ROWS;
    }
    if (!noTitle) {
        text = gAreaChatTexts[0];
        if ((flags & CHAT_FRAME_MARK) && (s8)Kbd_IsOpen() == 0) {
            if (gChatWinHold == 0) {
                mark = (compact == 0) ? CHAT_MARK_LARGE : CHAT_MARK_COMPACT;
            } else {
                mark = CHAT_MARK_HOLD;
            }
            sprintf(buf, "%s%25s", gAreaChatTexts[0], gAreaChatTexts[mark]);
            text = buf;
        }
    } else {
        text = NULL;
        lines++;
    }
    panel = &gChatLogBoxes[panelIndex];
    panel->lines = lines;
    ListPanel_Draw(panel, 0, text, -1);
    if (text != NULL) {
        textSize = gChatTextSizes[size];
        Gfx_DrawMarker(panel->x, panel->y, textSize, textSize * 18 / CHAT_TEXT_BASE_SIZE, 0);
        if (mark != 0) {
            if (compact != 0 && mark == CHAT_MARK_HOLD) {
                mark++;
            }
            Draw_Icons(&gChatLogIcons[gSysSave.options.chatSize][mark - 1], 1);
        }
    }
}

/* The rows of the log's lines but the newest skip ones. */
u8 ChatLog_LinesBefore(u8 skip)
{
    s32 k;
    s32 n;
    u8 sum;

    k = gChatLog.win.head - 1 - skip;
    n = gChatLog.win.count - skip;
    sum = 0;
    while (n != 0) {
        k &= CHAT_LOG_LINES - 1;
        n--;
        sum += gChatLog.lines[k].rows;
        k--;
    }
    return sum;
}

/* The rows of all the log's lines. */
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
        sum += gChatLog.lines[k & (CHAT_LOG_LINES - 1)].rows;
        k--;
    }
    return sum;
}

/* Writes name into dst as a line's sender: in eight columns, then a
 * full-width colon. */
void ChatLog_FormatName(char *dst, char *name)
{
    sprintf(dst, TXT(0x00846BC0) /* a name (eight columns), then a full-width colon */, name);
}

#pragma optimization_level 4
/*
 * Draws the log's lines in the window, in text size size, yoff lower
 * (scaled to the size), each line's sender (its name, or its id when the
 * option hides names: gChatHideNames) before its first row. When the log
 * holds more rows than the window (CHAT_LOG_COMPACT_ROWS or
 * CHAT_LOG_LARGE_ROWS) and fromTop is 0, from the newest line upwards,
 * the newest skip lines left out; else from the oldest line down.
 * Compiled at optimization level 4, as the game's code was.
 */
void ChatLog_DrawLines(u8 skip, s16 yoff, u8 fromTop, u8 size, u8 compact)
{
    char name[32];
    ChatLogLine *line;
    s32 k;
    s32 n;
    s32 i;
    s16 fontH;
    s16 lineH;
    u8 rows;
    s16 textX;
    s16 y;
    s16 topY;

    if (gChatLog.win.count == 0) {
        return;
    }
    size = gChatTextSizes[size];
    fontH = (s16)size * 18 / CHAT_TEXT_BASE_SIZE;
    Font_SetSpacing(size, fontH);
    lineH = fontH + 1;
    rows = compact == 0 ? CHAT_LOG_LARGE_ROWS : CHAT_LOG_COMPACT_ROWS;
    textX = size * 100 / CHAT_TEXT_BASE_SIZE;
    yoff = yoff * size / CHAT_TEXT_BASE_SIZE;
    if ((u32)ChatLog_Lines() > rows && fromTop == 0) {
        topY = CHAT_LOG_BOTTOM_Y - lineH * rows;
        k = gChatLog.win.head - 1 - skip;
        n = gChatLog.win.count - skip;
        y = CHAT_LOG_BOTTOM_Y - lineH;
        while (n != 0) {
            k &= CHAT_LOG_LINES - 1;
            line = &gChatLog.lines[k];
            for (i = line->rows - 1; i >= 0; i--) {
                Text_SetColor(line->color[2]);
                Font_SetPos(textX + CHAT_LOG_TEXT_X, y + yoff);
                Text_Draw(line->text[i]);
                if (i == 0) {
                    if (gChatHideNames == 0) {
                        ChatLog_FormatName(name, line->name);
                    } else {
                        ChatLog_FormatName(name, line->id);
                    }
                    Text_DrawShadow2(CHAT_LOG_TEXT_X, y + yoff, 1, line->color[1], name);
                }
                y -= lineH;
                if (y < topY) {
                    return;
                }
            }
            n--;
            k--;
        }
    } else {
        n = gChatLog.win.count;
        k = gChatLog.win.head - n;
        y = CHAT_LOG_BOTTOM_Y - lineH * rows;
        while (n != 0) {
            k &= CHAT_LOG_LINES - 1;
            line = &gChatLog.lines[k];
            if (gChatHideNames == 0) {
                ChatLog_FormatName(name, line->name);
            } else {
                ChatLog_FormatName(name, line->id);
            }
            Text_DrawShadow2(CHAT_LOG_TEXT_X, y + yoff, 1, line->color[1], name);
            Text_SetColor(line->color[2]);
            for (i = 0; i < line->rows; i++) {
                Font_SetPos(textX + CHAT_LOG_TEXT_X, y + yoff);
                Text_Draw(line->text[i]);
                y += lineH;
                if (y >= CHAT_LOG_BOTTOM_Y) {
                    return;
                }
            }
            n--;
            k++;
        }
    }
}
#pragma optimization_level reset

/* Draws the chat window and its lines without the title's mark, and
 * clears the window's timer. The option's text size is read as an
 * unsigned byte, as the game's code reads it. */
void ChatLog_DrawFull(void)
{
    u8 size;

    size = *(u8 *)&gSysSave.options.chatSize;
    ChatLog_DrawFrame(0, size, gChatLog.compact);
    ChatLog_DrawLines(0, 0, 0, size, gChatLog.compact);
    gChatLog.win.timer = 0;
}

/* The lobby's chat on the screen: the chat window of gChatWinState while
 * the log is open, else the log's window, with its title's mark, while
 * its timer runs. */
void LobbyChat_DrawHud(void)
{
    u8 state;
    u8 size;

    if (gChatLog.win.open != 0) {
        state = gChatWinState;
        if (state != 0) {
            Chat_DrawWindow(state);
            return;
        }
    }
    if (gChatLog.win.timer != 0) {
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
        size = *(u8 *)&gSysSave.options.chatSize;
        ChatLog_DrawFrame(CHAT_FRAME_MARK, size, gChatLog.compact);
        ChatLog_DrawLines(0, 0, 0, size, gChatLog.compact);
    }
}

/* Scrolls the log back to its newest line, and sets atEnd when all its
 * rows fit in the large window. */
void ChatLog_ResetScroll(void)
{
    gChatLog.win.scroll = 0;
    if (ChatLog_Lines() < (u32)CHAT_LOG_LARGE_ROWS + 1) {
        gChatLog.win.atEnd = 1;
        return;
    }
    gChatLog.win.atEnd = 0;
}

/* Closes the scrolled log: hides the help line, scrolls back to the
 * newest line (ChatLog_ResetScroll), and returns 0. */
s32 ChatLog_Close(void)
{
    gHelpShow = 0;
    ChatLog_ResetScroll();
    return 0;
}

/*
 * Scrolls the open log by the buttons pad (PAD_UP back a line, PAD_DOWN
 * on a line) while it holds more rows than the large window, setting
 * the window's arrows (ChatWindow.flags, CHAT_WIN_*) and atEnd. Back at
 * the newest line, the window closes. Returns pad.
 */
u16 ChatLog_Scroll(u16 pad)
{
    u8 scroll;

    gHelpShow = 0;
    gChatLog.win.flags = CHAT_WIN_SCROLLING;
    if (ChatLog_Lines() > (u32)CHAT_LOG_LARGE_ROWS) {
        if (gChatLog.win.scroll != 0) {
            gChatLog.win.flags |= CHAT_WIN_MORE_AFTER;
        }
        if (ChatLog_LinesBefore(gChatLog.win.scroll) > CHAT_LOG_LARGE_ROWS) {
            gChatLog.win.flags |= CHAT_WIN_MORE_BEFORE;
            if (pad & PAD_UP) {
                scroll = gChatLog.win.scroll;
                if (scroll < gChatLog.win.count - 1) {
                    gChatLog.win.atEnd = 0;
                    gChatLog.win.scroll = scroll + 1;
                    gChatLog.win.flags |= CHAT_WIN_STEP_BACK;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                    if (ChatLog_LinesBefore(gChatLog.win.scroll) <= CHAT_LOG_LARGE_ROWS) {
                        gChatLog.win.atEnd = 1;
                    }
                }
            }
        } else {
            gChatLog.win.atEnd = 1;
        }
        if (pad & PAD_DOWN) {
            if (gChatLog.win.scroll > 0) {
                gChatLog.win.scroll--;
                gChatLog.win.flags |= CHAT_WIN_STEP_ON;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                gChatLog.win.atEnd = 0;
            }
        }
    }
    if (gChatLog.win.scroll == 0) {
        gChatLog.win.open = gChatLog.win.timer = 0;
    }
    return pad;
}

#define CHAT_ARROW_DIM_COLOR 0xD8707070 /* an arrow with nothing to scroll to */
#define CHAT_ARROW_RGB 0x10F213         /* a lit arrow, its alpha pulsing */

/*
 * Draws the scrolled log: the large window without a title and its
 * lines from the scroll position, and its two arrows at the left, up and
 * down. An arrow is lit, its alpha pulsing over 32 frames, while the log
 * can scroll that way (CHAT_WIN_MORE_BEFORE, CHAT_WIN_MORE_AFTER), and
 * nudged out by 2 in the frame it scrolled (CHAT_WIN_STEP_BACK,
 * CHAT_WIN_STEP_ON). The text size passed to ChatLog_DrawLines is cast
 * to int: so the compiler loads it before the other arguments, as the
 * game's code does.
 */
void ChatLog_Draw(void)
{
    u8 size;
    s16 x;
    s16 y;
    u32 litColor;
    u32 upColor;
    s8 pulse;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    size = gChatTextSizes[gSysSave.options.chatSize];
    ChatLog_DrawFrame(CHAT_FRAME_NO_TITLE, gSysSave.options.chatSize, 0);
    ChatLog_DrawLines(gChatLog.win.scroll, -10, gChatLog.win.atEnd, (s32)*(u8 *)&gSysSave.options.chatSize, 0);
    pulse = 48.0f * Math_Sin(9.5873801e-05f * (u16)((gFrameTick & 0x1F) << 11));
    litColor = ((pulse + 207) << 24) | CHAT_ARROW_RGB;
    x = size * 10 + 9;
    y = 404 - (size * 18 / CHAT_TEXT_BASE_SIZE + 1) * 12;
    if (gChatLog.win.flags & CHAT_WIN_MORE_BEFORE) {
        upColor = litColor;
        if (gChatLog.win.flags & CHAT_WIN_STEP_BACK) {
            y -= 2;
        }
    } else {
        upColor = CHAT_ARROW_DIM_COLOR;
    }
    Chat_DrawArrow(x, y, 30, 24, upColor, ARROW_TEXRECT);
    y = 411;
    if (gChatLog.win.flags & CHAT_WIN_MORE_AFTER) {
        if (gChatLog.win.flags & CHAT_WIN_STEP_ON) {
            y += 2;
        }
    } else {
        litColor = CHAT_ARROW_DIM_COLOR;
    }
    Chat_DrawArrow(x, y, 30, 24, litColor, ARROW_TEXRECT | ARROW_MIRROR);
}

/* 1 when id isn't among the n speakers of list, else 0. */
s32 ChatLog_IsNewSpeaker(char *id, ChatSpeaker *list, s32 n)
{
    ChatSpeaker *speaker;
    s32 k;

    for (speaker = list, k = n; k != 0; k--, speaker++) {
        if (strcmp(speaker->id, id) == 0) {
            return 0;
        }
    }
    return 1;
}

#define CHAT_NO_SPEAKER 0xFF    /* ChatLogLine.color[0] of a line with no speaker */
#define CHAT_STAFF_ID_LEN 5     /* the characters of the staff's ids' common start */

/*
 * Lists the log's speakers into out (CHAT_LOG_LINES at most) for the
 * menu that mutes them: each sender once, without the player, the lines
 * with no speaker (CHAT_NO_SPEAKER), the game's own messages (sent as
 * gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]) and the staff's ids. An empty entry follows the last when there is room.
 * Returns their number.
 */
s32 ChatLog_ListSpeakers(ChatSpeaker *out)
{
    ChatLogLine *line;
    s32 k;
    s32 n;

    k = gChatLog.win.count;
    if (k == 0) {
        out[0].id[0] = 0;
        out[0].name[0] = 0;
        return 0;
    }
    for (line = gChatLog.lines, n = 0; k != 0; k--, line++) {
        if (line->color[0] != CHAT_NO_SPEAKER && strcmp(line->id, gMyUserId) != 0 && strcmp(line->id, gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]) != 0
            && memcmp(line->id, "STAFF", CHAT_STAFF_ID_LEN) != 0 && ChatLog_IsNewSpeaker(line->id, out, n) == 1) {
            strcpy(out[n].id, line->id);
            strcpy(out[n].name, line->name);
            n++;
        }
    }
    if ((u32)n < CHAT_LOG_LINES) {
        out[n].id[0] = 0;
        out[n].name[0] = 0;
    }
    return n;
}

/* The chat windows' panels and title icons, by text size. */
ListPanel gChatLogBoxes[6] = {
#include "lobby/00819480.inc"
};
Icon gChatLogIcons[3][4] = {
#include "lobby/00819540.inc"
};
