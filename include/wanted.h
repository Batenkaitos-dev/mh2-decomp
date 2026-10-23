#ifndef WANTED_H
#define WANTED_H

#include "common.h"
#include "lobby_globals.h"
#include "quest.h"

/*
 * The update of the town mode that only waits for cancel
 * (TownMode_WaitCancel), and the town's wanted lists: per season, three
 * entries for each of the tavern and the elder hall (Wanted_Count,
 * Wanted_IsSet). The quest desk pays a bonus for a quest whose monster is
 * wanted (Wanted_MatchesQuest, QuestDesk_IsWanted, QuestDesk_GetWantedFee).
 */

void TownMode_WaitCancel(void);
s8 Wanted_Count(void);
s32 Wanted_IsSet(s16 *values);
s32 Wanted_MatchesQuest(QuestGoal *goal, TownEntry *entry);
s32 QuestDesk_IsWanted(u8 *questBytes);
s16 QuestDesk_GetWantedFee(void);

#endif
