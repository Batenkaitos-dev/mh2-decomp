#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "common.h"
#include "grid.h"

/* A speaker as the chat log lists them (0x19 bytes; ChatLog_ListSpeakers,
 * unk_7B91F0.c): the hunter's id and name. */
typedef struct ChatSpeaker {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[0x11];
} ChatSpeaker;

/*
 * The game menu's work (gSubMenuWork -> gSubMenuWorkMem, 0x8B4400, 0x6D0
 * bytes): the town and lobby menu (GameMenu_*, unk_770970.c) and its pages:
 * search, chat, mail and quit (their steps at 5-7, their cursor at 0xA), the
 * chat's hunter list and muted list (ChatPage_*, Chat_DrawSpeakers).
 */
typedef struct GameMenu {
    /* 0x00 */ u8 open;             /* GameMenu_Open sets it; 0 closes the menu */
    /* 0x01 */ u8 kind;             /* the list: 0 offline, 1 online in AREA_HUB_0x83, 2 online (gGameMenuOpen0..2) */
    /* 0x02 */ u8 count;            /* the list's entries (9 or 10) */
    /* 0x03 */ u8 entry;            /* the list's cursor */
    /* 0x04 */ union {
        s32 steps;                  /* the four step bytes, cleared together */
        struct {
            u8 step;                /* 0 the list, 1 in a page, 2 closing */
            u8 subStep;             /* the page's step: 0 its own list, 1 in its entry */
            u8 page;                /* chat hunters: 0 the hunter list, 1 the muted list; quit: the confirmation's step */
            u8 pageStep;            /* chat hunters: 0 the list, 1 the action list, 2 the hunter's info */
        };
    };
    /* 0x08 */ u16 pad;             /* the buttons (GameMenu_Poll), 0 while a page takes them */
    /* 0x0A */ u8 subEntry;         /* the page's cursor (search, chat, mail and quit entries) */
    /* 0x0B */ u8 unkB;
    /* 0x0C */ u8 yesNo;            /* the yes/no answer (gYesNoTexts, GameMenu_YesNoInput) */
    /* 0x0D */ u8 windowSel;        /* the chat state's cursor (gChatWindowSel) */
    /* 0x0E */ u8 busy;             /* 1 while the menu cannot quit (GameMenu_IsBusy) */
    /* 0x0F */ u8 action;           /* the hunter action's cursor: 0 info, 1 mute, 2 unmute */
    /* 0x10 */ u8 speakerCount;     /* ChatLog_ListSpeakers */
    /* 0x11 */ u8 mutedCount;       /* gChatBlockList's entries (20 at most) */
    /* 0x12 */ u8 speakerMuted[0x41]; /* 1 for a speaker on gChatBlockList */
    /* 0x53 */ ChatSpeaker speakers[0x41]; /* the speakers, an empty id after the last */
    /* 0x6AC */ Grid speakerGrid;   /* the hunter list's cursor */
    /* 0x6B8 */ Grid mutedGrid;     /* the muted list's cursor */
    /* 0x6C4 */ u8 unk6C4[0x6D0 - 0x6C4];
} GameMenu;

extern GameMenu *gSubMenuWork;
extern GameMenu gSubMenuWorkMem;

#endif /* GAME_MENU_H */
