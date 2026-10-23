#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "common.h"
#include "grid.h"

/* A speaker as the chat log lists them (25 bytes; ChatLog_ListSpeakers,
 * lobby_screen.c): the hunter's id and name. */
typedef struct ChatSpeaker {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x11];
} ChatSpeaker;

/*
 * The game menu's work (gSubMenuWork -> gSubMenuWorkMem, 0x8B4400, 1744
 * bytes): the town and lobby menu (GameMenu_*) and its pages:
 * search, chat, mail and quit (their steps in subStep, page and pageStep,
 * their cursor in subEntry), the chat's hunter list and muted list (ChatPage_*, ChatPage_DrawSpeakers).
 */
/* GameMenu.kind: which list the menu shows (set by GameMenu_Open). */
enum GameMenuKind {
    GAME_MENU_OFFLINE = 0,
    GAME_MENU_HUB = 1,          /* online, in AREA_ONLINE_VILLAGE */
    GAME_MENU_ONLINE = 2        /* online, elsewhere */
};

/* GameMenu.step. */
enum GameMenuStep {
    GAME_MENU_LIST = 0,         /* the list */
    GAME_MENU_PAGE = 1,         /* in an entry's page */
    GAME_MENU_CLOSING = 2       /* the list slides out */
};

/* The member list's screens (LobbyMenu.screen; LobbyMember_ListInput). */
enum MemberListScreen {
    MEMBER_LIST_ROWS = 0,       /* the list of the town's members */
    MEMBER_LIST_CARD = 1,       /* a member's card */
    MEMBER_LIST_FRIEND_ASK = 2, /* whether to add the member to the friend list */
    MEMBER_LIST_NOTICE = 3      /* LOBBY_TEXT_FRIEND_RESTRICTED, until PAD_CIRCLE */
};

/* The chat page's list (GameMenu.subEntry; gChatMenuTexts, whose entries 4
 * and 5 are the hunter and state entries' other texts). */
#define CHAT_ENTRY_LOG      0   /* the chat log */
#define CHAT_ENTRY_PHRASES  1   /* the phrase menu */
#define CHAT_ENTRY_HUNTERS  2   /* the hunters in the chat log and the muted list */
#define CHAT_ENTRY_STATE    3   /* where the chat goes (gChatWindowSel) */
#define CHAT_ENTRIES        4

/* The mail page's list (GameMenu.subEntry; GameMenu_UpdateMail). */
#define MAIL_ENTRY_BOX          0   /* the mail box (MailMenu_*) */
#define MAIL_ENTRY_NEW          1   /* a new short mail, the addressee's id typed in */
#define MAIL_ENTRY_SAVED        2   /* the short mail from the last one kept */
#define MAIL_ENTRY_BLOCK_LIST   3   /* the mail block list (BlockList_*) */
#define MAIL_ENTRIES            4
#define MAIL_HELP_LINE          0x20    /* HELP_PAGE_LOBBY's line of MAIL_ENTRY_BOX; one line per entry */

/* The hunter page's two lists (GameMenu.page). */
#define CHAT_PAGE_HUNTERS   0   /* the hunters in the chat log */
#define CHAT_PAGE_MUTED     1   /* the muted list (gChatBlockList) */

/* The actions on a hunter of the list (GameMenu.action; gChatUserActTexts). */
#define CHAT_ACT_INFO       0   /* the hunter's card (HunterInfo_Open) */
#define CHAT_ACT_MUTE       1   /* onto the muted list (MuteList_Add) */
#define CHAT_ACT_UNMUTE     2   /* off it (MuteList_Remove) */
#define CHAT_ACTS           3

#define CHAT_LIST_ROWS      10  /* the rows of a column of either list */

/* The quit entry's two ways (GameMenu.subEntry; GameMenu_Quit's boot
 * mode). */
#define QUIT_WAY_ONCE       0   /* the next boot goes straight to gm_sub's task, once (BOOT_MODE_GMSUB) */
#define QUIT_WAY_EVERY_BOOT 1   /* every boot does (BOOT_MODE_GMSUB_KEEP); needs the system data saved */
#define QUIT_WAYS           2

/* The quit entry's pages (GameMenu.page, GameMenu_UpdateQuit and
 * GameMenu_DrawQuit). */
#define QUIT_PAGE_CONFIRM       0   /* the yes/no on the way chosen */
#define QUIT_PAGE_ASK_SAVE      1   /* the first way: save first? */
#define QUIT_PAGE_ASK_AGAIN     2   /* asked again: no quits without saving */
#define QUIT_PAGE_SAVE          3   /* the save menu (TownNetMenu_Update), then QUITTING */
#define QUIT_PAGE_QUITTING      4   /* the quit runs (GameMenu_Quit) */
#define QUIT_PAGE_SAVE_DIRECT   5   /* the second way's save menu, then the page after it */

typedef struct GameMenu {
    /* 0x00 */ u8 open;             /* GameMenu_Open sets it; 0 closes the menu */
    /* 0x01 */ u8 kind;             /* the list (enum GameMenuKind; gGameMenuOpen0..2) */
    /* 0x02 */ u8 count;            /* the list's entries (9 or 10) */
    /* 0x03 */ u8 entry;            /* the list's cursor */
    /* 0x04 */ union {
        s32 steps;                  /* the four step bytes, cleared together */
        struct {
            u8 step;                /* enum GameMenuStep */
            u8 subStep;             /* the page's step: 0 its own list, 1 in its entry */
            u8 page;                /* chat hunters: CHAT_PAGE_*; quit: QUIT_PAGE_* */
            u8 pageStep;            /* chat hunters: 0 the list, 1 the action list, 2 the hunter's info */
        };
    };
    /* 0x08 */ u16 pad;             /* the buttons (GameMenu_Poll), 0 while a page takes them */
    /* 0x0A */ u8 subEntry;         /* the page's cursor (search, chat, mail and quit entries) */
    /* 0x0B */ u8 unkB;
    /* 0x0C */ u8 yesNo;            /* the yes/no answer (gYesNoTexts, GameMenu_YesNoInput) */
    /* 0x0D */ u8 windowSel;        /* the chat state's cursor (gChatWindowSel) */
    /* 0x0E */ u8 busy;             /* 1 while the menu cannot quit (GameMenu_IsBusy) */
    /* 0x0F */ u8 action;           /* the hunter action's cursor (CHAT_ACT_*) */
    /* 0x10 */ u8 speakerCount;     /* ChatLog_ListSpeakers */
    /* 0x11 */ u8 mutedCount;       /* gChatBlockList's records (CHAT_BLOCK_LIST_LEN at most) */
    /* 0x12 */ u8 speakerMuted[0x41]; /* 1 for a speaker on gChatBlockList */
    /* 0x53 */ ChatSpeaker speakers[0x41]; /* the speakers, an empty id after the last */
    /* 0x6AC */ Grid speakerGrid;   /* the hunter list's cursor */
    /* 0x6B8 */ Grid mutedGrid;     /* the muted list's cursor */
    /* 0x6C4 */ u8 unk6C4[0x6D0 - 0x6C4];
} GameMenu;

extern GameMenu *gSubMenuWork;
extern GameMenu gSubMenuWorkMem;

s32 ChatMenu_CanListSpeakers(void);
s32 ChatMenu_CanSetState(void);
s32 GameMenu_OpenChat(void);
u16 GameMenu_UpdateChat(u16 pad);
void GameMenu_DrawChat(void);
s32 ChatPage_OpenSpeakers(void);
u16 ChatPage_UpdateSpeakers(u16 pad);
void ChatPage_MarkMuted(void);
void ChatPage_LoadSpeakers(void);
s32 MuteList_Add(ChatSpeaker *speaker);
s32 MuteList_Remove(ChatSpeaker *speaker);
u8 ChatPage_CountMuted(void);
void ChatPage_DrawSpeakers(void);
void ChatPage_DrawMutedTitle(s16 *pos);

void GameMenu_Reset(void);
void GameMenu_Open(void);
void GameMenu_Start(void);
void GameMenu_Clear(void);
s32 GameMenu_Poll(void);
u16 GameMenu_GetPad(void);
s32 GameMenu_Update(u16 pad);
void GameMenu_Back(void);
void GameMenu_SetHelp(void);
s32 GameMenu_IsBusy(void);
void GameMenu_Draw(void);
void GameMenu_DrawList(s8 dim);
s32 GameMenu_OpenSearch(void);
s32 GameMenu_OpenQuit(void *unused0);
u16 GameMenu_UpdateQuit(u16 pad);
s32 GameMenu_YesNoInput(u16 *pad);
s32 GameMenu_OpenPause(void *menu);
u16 GameMenu_UpdatePause(u16 pad);
void GameMenu_DrawPause(void);
void GameMenu_DrawQuit(void);
s32 GameMenu_OpenMail(void *unused);
u16 GameMenu_UpdateMail(u16 pad);
void GameMenu_DrawMail(void);
s32 GameMenu_OpenExit(void *menu);
s32 GameMenu_UpdateExit(s32 pad);

#endif /* GAME_MENU_H */
