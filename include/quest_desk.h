#ifndef QUEST_DESK_H
#define QUEST_DESK_H

#include "common.h"
#include "lobby_menu.h"

/*
 * The quest desk (TOWN_MODE_QUEST_DESK), where quests are taken in the
 * village and online: QuestDesk_Open, QuestDesk_Update and
 * QuestDesk_Close run its menu, QuestDesk_Accept makes a quest the taken
 * one, and online the quest
 * is posted to the quest room first (QuestDesk_Post). Lobby_EnterRollRare
 * is the roll made on entering the lobby, which the next quest's flyer
 * carries.
 */

void QuestDesk_Open(u8 *npc);
void QuestDesk_Close(void);
void QuestDesk_Update(void);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
s32 QuestDesk_Greet(void);
s32 QuestDesk_TopMenu(u16 pad);
s32 QuestDesk_ChooseList(u16 pad);
s32 QuestDesk_ChooseQuest(u16 pad);
s32 QuestDesk_CheckFee(u16 pad);
s32 QuestDesk_Post(u16 pad);
s32 QuestDesk_Cancel(QuestDeskCancel *cancel, u16 pad);
s32 QuestDesk_ForecastInput(u16 pad);
s32 QuestDesk_GuideInput(u16 pad);
void QuestDesk_SayBye(void);
void QuestDesk_BuildLists(void);
s32 QuestDesk_CountMapLists(void);
s32 Lobby_EnterRollRare(void);

#endif
