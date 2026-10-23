#ifndef BAIT_MENU_H
#define BAIT_MENU_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The bait menu at the fishing spot (TOWN_MODE_BAIT_MENU): the townsperson
 * there lists the bait the player holds, seven a page, and the player casts
 * the bait chosen at the spot, again and again, until it runs out or the
 * player cancels. BaitMenu_Open opens it when the townsperson is talked to,
 * BaitMenu_Update runs it each frame and BaitMenu_Draw is its draw callback.
 */

s32 BaitMenu_IsOpen(void);
void BaitMenu_Open(void);
void BaitMenu_LoadPage(BaitMenu *menu, s16 page);
void BaitMenu_Update(void);
void BaitMenu_StartFishing(void);
s32 BaitMenu_FishInput(u16 unused);
s32 BaitMenu_ListInput(u16 pad);
void BaitMenu_DrawList(s16 x, s16 y);
void BaitMenu_DrawCastHint(void);
void BaitMenu_Draw(MenuTask *task);

#endif
