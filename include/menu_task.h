#ifndef MENU_TASK_H
#define MENU_TASK_H

#include "common.h"

/*
 * A menu task of the lobby (created by the menu system, lobby 0x7BCA00; the
 * lobby menu drives eight of them, sub_main's gLobbyMenuLayer0..gLobbyMenuLayer7, 32 bytes apart):
 * its draw callback gets the task and sets the text size from +0x18 first.
 */
typedef struct MenuTask {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ void *draw;              /* the draw callback (LobbyMenu_SetDraw) */
    /* 0x18 */ s32 textSize;            /* Text_SetSize */
    /* 0x1C */ u8 unk1C[4];
} MenuTask;

#endif /* MENU_TASK_H */
