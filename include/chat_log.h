#ifndef CHAT_LOG_H
#define CHAT_LOG_H

#include "common.h"

/*
 * A chat window's state (8 bytes): the chat log's (gChatLog.win) and the
 * chat window's (sub_main gChatWinTimer, 0x3A in the chat state). sub_main's
 * Chat_ToggleWindow opens and closes both the same way.
 */
typedef struct ChatWindow {
    u16 timer;                  /* 0x0: frames the window stays open (0x12C) */
    u8 open;                    /* 0x2: the log: set by a line from another player */
    u8 head;                    /* 0x3: the log: the next line (of 0x40) */
    u8 count;                   /* 0x4: the log: lines in it (at most 0x40) */
    u8 scroll;                  /* 0x5: the log: lines scrolled back */
    u8 atEnd;                   /* 0x6: the log: 1 at the newest page */
    u8 flags;                   /* 0x7: 0x80: closed by the player (Chat_ToggleWindow); the log: 0x80 and its arrows (2 up, 1 more above, 4 scrolled up, 8 down) */
} ChatWindow;

/*
 * A chat message (0x124 bytes): as LobbyNet_Get6702 reads it (lobby
 * unk_7C1540.c) and as the lobby's event lines (unk_7A7560.c) and
 * Chat_SendText make it; ChatLog_Receive adds it to the chat log.
 */
typedef struct ChatMsg {
    char id[8];                 /* 0x000 */
    char name[0x14];            /* 0x008 */
    char text[0x100];           /* 0x01C */
    u8 cls;                     /* 0x11C: the sender's class (gChatClassColors: its colour) */
    u8 color[3];                /* 0x11D: digits '0'-'7' (ChatMsg_ColorDigit); the log draws the name in [1], the text in [2] */
    u8 kind;                    /* 0x120: 1 the chat log, 2 a member's balloon (Chat_Receive) */
    u8 pad121[3];
} ChatMsg;
typedef char ChatMsg_size_check[(sizeof(ChatMsg) == 0x124) ? 1 : -1];

/*
 * A chat log line (0x61 bytes): the text, the speaker (ChatLog_ListSpeakers)
 * and the message's class and colours (ChatLog_Add copies them).
 */
typedef struct ChatLogLine {
    char text[2][0x20];         /* 0x00: two rows of 30 bytes (a Shift-JIS pair kept whole) */
    char id[8];                 /* 0x40 */
    char name[0x11];            /* 0x48 */
    u8 unk59;                   /* 0x59: 1 (ChatLog_Add) */
    u8 unk5A;                   /* 0x5A: 0xFF (ChatLog_Add) */
    u8 rows;                    /* 0x5B: rows of text (ChatLog_Lines sums them) */
    u8 unk5C;                   /* 0x5C */
    u8 cls;                     /* 0x5D: the message's class, then cleared (ChatLog_Add) */
    u8 color[3];                /* 0x5E: the message's colours: [0] 0xFF no speaker, [1] the name's, [2] the text's */
} ChatLogLine;
typedef char ChatLogLine_size_check[(sizeof(ChatLogLine) == 0x61) ? 1 : -1];

/*
 * The online chat log (sub_main gChatLog, 0x5BC500, 0x1849 bytes): its
 * window's state, then 64 lines in a ring. Cleared by the lobby's
 * ChatLog_Clear, filled by ChatLog_Add, scrolled by ChatLog_Scroll.
 */
typedef struct ChatLog {
    ChatWindow win;             /* 0x0 */
    ChatLogLine lines[0x40];    /* 0x8 */
    u8 unk1848;                 /* 0x1848: cleared by ChatLog_Clear, flipped by the chat button while the log is open (Chat_UpdateWindowsOnline), the last argument of ChatLog_DrawFrame and ChatLog_DrawLines */
} ChatLog;

/* One chat line (0x4F bytes); 64 of them in a ring in the chat state. */
typedef struct ChatLine {
    s8 text[2][0x18];           /* 0x00: up to two rows of text */
    u8 rows;                    /* 0x30: text rows */
    u8 who;                     /* 0x31: the sender */
    u8 color[4];                /* 0x32: from the message (ChatMsg.cls, then its color[3]) */
    s8 id[8];                   /* 0x36: the sender's id; nonzero: one more row */
    s8 name[0x11];              /* 0x3E: the sender's name */
} ChatLine;

/* gChatSendTo: where Chat_SendInput sends the chat line (Chat_PickSendTarget sets it). */
#define CHAT_SEND_LOBBY     0       /* the lobby's chat (its gChatLog window); online, in the town area */
#define CHAT_SEND_TARGETS   1       /* Chat_SendToTargets, by gChatMsgClass; online, by gChatWindowSel */
#define CHAT_SEND_PARTY     2       /* the quest's party (QSync_SendChat), logged here too */
#define CHAT_SEND_LOCAL     3       /* offline: only logged here, as gChatSendRecord */

/* Chat_OpenKbd's flags, over Kbd_Open's kind in the low byte. */
#define CHAT_KBD_UNAVAILABLE 0x4            /* Kbd_Open's kind when chat is not available (Chat_IsAvailable) */
#define CHAT_KBD_INACTIVE   0x40000000      /* the keyboard opens without the focus (Kbd_SetStartActive(0)) */
#define CHAT_KBD_TARGETED   0x80000000      /* Chat_SetTarget(0), else -1 */

/* ChatWindow.flags (gChatLines for the field chat's window): the log's scroll state. */
#define CHAT_WIN_MORE_BEFORE 0x01   /* older rows above: the up arrow lit */
#define CHAT_WIN_MORE_AFTER 0x02    /* scrolled back: the down arrow lit */
#define CHAT_WIN_STEP_BACK  0x04    /* scrolled back this frame: the up arrow nudged */
#define CHAT_WIN_STEP_ON    0x08    /* scrolled on this frame: the down arrow nudged */
#define CHAT_WIN_SCROLLING  0x80    /* the log is open for scrolling (Chat_LogScroll) */

/* gNoticeFlags: the notice window's state (Notice_OpenText sets it, Notice_Draw draws by it). */
#define NOTICE_OPEN         0x01    /* a text is shown */
#define NOTICE_PROMPT       0x02    /* a yes/no prompt under the text (NOTICE_MODE_PROMPT) */
#define NOTICE_PROMPT_YES   0x04    /* the prompt's "yes" selected */
#define NOTICE_NEXT         0x08    /* the mark that more text follows (NOTICE_MODE_NEXT) */
#define NOTICE_CLOSED       0x10    /* nothing drawn (NOTICE_MODE_CLOSE) */
#define NOTICE_LIST         0x20    /* gNoticeList under the text, gNoticeSel's entry in brackets */
#define NOTICE_LIST_PICKED  0x40    /* only gNoticeList's selected entry */

/* Notice_OpenText's modes: what follows the text once it is all shown. */
#define NOTICE_MODE_TEXT    0       /* nothing */
#define NOTICE_MODE_PROMPT  1       /* a yes/no prompt, "yes" selected when sel is 0 */
#define NOTICE_MODE_NEXT    2       /* the more-text mark, with a sound the first time */
#define NOTICE_MODE_CLOSE   3       /* closes the window */
#define NOTICE_MODE_LIST    4       /* a list with entry sel selected */
#define NOTICE_MODE_PICKED  5       /* a list's entry sel alone */

/* The chat window state at 0x5BB080. Most of its scalar fields are also
 * accessed through their own symbols (gChatHidden .. gChatLines). */
typedef struct ChatState {
    char *notice;               /* 0x00: the notice window's text (Notice_Draw) */
    u8 unk04[0x18 - 0x04];      /* 0x04: gNoticeShownChars the notice's length, gNoticeFlags its flags, gNoticeList its list */
    u16 recipients;             /* 0x18: gChatRecipients */
    u8 unk1A;                   /* 0x1A: gChatToAll */
    u8 unk1B;                   /* 0x1B: gChatMsgClass */
    u8 unk1C[0x2C - 0x1C];
    u8 helpShow;                /* 0x2C: the help line is shown (also read as gHelpShow) */
    u8 unk2D[0x3A - 0x2D];
    ChatWindow win;             /* 0x3A: gChatWinTimer (timer), gChatWinOpen (open), gChatLogHead (head), gChatLogCount (count), gChatLogScroll (scroll), gChatLogAtEnd (atEnd), gChatLines (flags: scroll arrows and buttons shown) */
    ChatLine lines[64];         /* 0x42 */
    u8 unk1402[0x1420 - 0x1402]; /* cleared with the rest (0x1420 bytes, unk_33D7D0.c) */
} ChatState;


extern ChatLog gChatLog;
extern ChatState gChat;

#endif /* CHAT_LOG_H */
