#ifndef AREA_BOARD_H
#define AREA_BOARD_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The area board (TOWN_MODE_AREA_BOARD), at a town's board spot when the
 * player's area holds rooms (LobbyLevel0_HasGroups): it lists the area's
 * level-1 entries, joins an open one or opens a free one with a recruiting
 * text, and draws them. The group board runs its join, text picker, entry
 * release and members panel too.
 */

s32 LobbyLevel0_HasGroups(void);
void AreaBoard_Open(void);
void AreaBoard_Close(void);
void AreaBoard_Update(void);
s32 AreaBoard_Join(u16 pad, AreaBoardMenu *menu, s32 entryNo);
s32 AreaBoard_ListInput(u16 pad);
s32 AreaBoard_JoinInput(u16 pad, AreaBoardMenu *menu, s32 entryNo);
s32 AreaBoard_RecruitInput(u16 pad);
s32 AreaBoard_LeaveEntry(u16 pad);
void AreaBoard_DrawList(s16 x, s16 y);
void AreaBoard_DrawMembers(s16 x, s16 y, s32 entryNo, WinShake *shake, u16 stampSeed, s8 stamped);
void AreaBoard_DrawRecruitForm(s16 x, s16 y);
void AreaBoard_DrawTextPicker(s16 x, s16 y, s16 cursor, s16 count);
void AreaBoard_Draw(MenuTask *task);
void AreaBoard_DrawPanel(MenuTask *task);

#endif
