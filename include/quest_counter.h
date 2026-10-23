#ifndef QUEST_COUNTER_H
#define QUEST_COUNTER_H

#include "common.h"
#include "lobby_state.h"
#include "quest.h"
#include "lobby_menu.h"

/*
 * The town's quest counter (TOWN_MODE_QUEST_COUNTER), where a player joins
 * a quest another player posted in one of level 2's rooms.
 * QuestCounter_Open opens it and QuestCounter_Step runs it each frame: the
 * board of rooms, the room's quest, the checks, the password or the arena
 * cup's role, then the entry. Its steps and the draws of the board and the
 * quest window's pages are here too.
 */

void QuestCounter_Open(void);
void QuestCounter_Close(void);
void QuestCounter_Step(void);
s32 QuestCounter_ChooseArenaRole(u16 pad);
s32 QuestCounter_ChooseQuest(u16 pad);
s32 QuestCounter_ViewQuest(u16 pad);
s32 QuestCounter_CheckQuest(void);
s32 QuestCounter_InputPassword(void *pass);
s32 QuestCounter_CanJoin(s16 idx);
s32 QuestCounter_OpenWindows(s32 fromX, s32 toX);
s32 QuestCounter_DrawBoardRow(s16 x, s16 y, s16 row);
void QuestCounter_DrawBoard(WinSlide *win, s16 y);
void QuestCounter_DrawMembers(s16 x, s16 y, QuestInfo *quest);
void Quest_DrawArenaTeams(s16 x, s16 y, NetUser *list, s8 room);
void QuestCounter_DrawRoomSettings(s16 x, s32 startY, s32 room);

#endif
