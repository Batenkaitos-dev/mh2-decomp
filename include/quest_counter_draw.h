#ifndef QUEST_COUNTER_DRAW_H
#define QUEST_COUNTER_DRAW_H

#include "common.h"
#include "menu_task.h"

/*
 * The quest counter's draws: its two draw layers, QuestCounter_Draw (the
 * recruit board, then the chosen quest's window) and QuestCounter_Draw2
 * (the arena cup's team choice), with the quest window and the team
 * choice's window they draw.
 */

void QuestCounter_DrawQuestWindow(s16 x, s16 y);
void QuestCounter_Draw(MenuTask *task);
void QuestCounter_DrawTeamChoice(s32 x, s32 winY);
void QuestCounter_Draw2(MenuTask *task);

#endif
