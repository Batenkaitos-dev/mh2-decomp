#ifndef ARENA_MENU_H
#define ARENA_MENU_H

#include "common.h"
#include "menu_task.h"

/*
 * The versus quest's preparation in the town. Town_CheckQuestParty waits
 * for the room's members and both sides' monsters before setting out; the
 * arena menu (ArenaMenu_*) picks one of the player's monster records and an
 * item to feed it (ArenaFeed_*), and sends the choice.
 */

void Town_CheckQuestParty(void);
void ArenaMenu_Open(void);
void ArenaMenu_Send(void);
void ArenaMenu_Update(void);
s32 ArenaMenu_ListInput(u16 pad);
void ArenaFeed_Open(void);
void ArenaFeed_LoadPage(s32 page);
void ArenaFeed_DrawList(s16 x, s16 y);
void ArenaMenu_Draw(MenuTask *task);
void ArenaMenu_Draw2(MenuTask *task);

#endif
