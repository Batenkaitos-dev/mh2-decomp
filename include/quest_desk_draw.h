#ifndef QUEST_DESK_DRAW_H
#define QUEST_DESK_DRAW_H

#include "common.h"
#include "menu_task.h"
#include "quest.h"

/*
 * The quest desk's draws: the desk window, the quest lists, the quest
 * sheet (QuestBoard_Draw and its pages), the online recruit form, the
 * field guide and the elder's forecast. Monster_IsAvailable says whether a
 * monster has come to the quests yet, offline by the village's conditions
 * and online by the town's.
 */

void QuestDesk_DrawFieldInfo(s16 x, s16 y);
void QuestDesk_DrawIndexList(s16 x, s16 y);
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *quest);
void Quest_DrawTerms(s16 x, s16 y, QuestInfo *quest);
void Quest_DrawDetails(s16 x, s16 y, QuestInfo *quest);
void Quest_DrawClearMark(s16 x, s16 y);
void QuestBoard_Draw(s16 x, s16 y);
void QuestDesk_DrawRecruitForm(s16 x, s16 y);
s32 Monster_IsAvailableOffline(u16 em);
s32 Monster_IsAvailableOnline(u16 em);
s32 Monster_IsAvailable(u16 em);
void QuestDesk_DrawElderInfo(s16 x, s16 y);
void QuestDesk_DrawFieldGuide(s16 x, s16 y);
void QuestDesk_Draw(MenuTask *task);
void QuestDesk_Draw2(MenuTask *task);
extern s16 gQuestMapGroups[16];

#endif
