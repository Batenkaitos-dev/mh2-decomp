#ifndef MOVE_MENU_H
#define MOVE_MENU_H

#include "common.h"
#include "menu_task.h"

/*
 * The move menu (TOWN_MODE_MOVE_MENU): from the town, a move to another
 * area of the lobby server, or leaving the lobby. Its server panels
 * (LobbySrv_DrawListNamePanel) are drawn for the server menu and the area
 * list too.
 */

void MoveMenu_Open(void);
void MoveMenu_Close(void);
void MoveMenu_Update(void);
void MoveMenu_OpenPick(void);
s32 MoveMenu_PickInput(u16 pad);
void LobbySrv_DrawListNamePanel(s16 x, s16 y);
void MoveMenu_DrawAreaList(s16 x, s16 y);
void MoveMenu_DrawBack(MenuTask *task);
void MoveMenu_DrawFront(MenuTask *task);
void MoveMenu_DrawAreaPage(MenuTask *task);

#endif
