#ifndef QUEST_OFFERS_H
#define QUEST_OFFERS_H

#include "common.h"

/*
 * Which quests the village and the lobby offer: whether a map's quests
 * are open (MapGroup_IsOpen), whether a quest runs in a season and at a
 * time of day, whether the taken quest can set out now (Quest_CheckOrder),
 * and whether a quest goes on the quest lists (Quest_IsOffered). The
 * QuestList_* functions fill the quest lists by list kind and thin them
 * out at random by map group; the elder's forecast adds its quests at
 * random.
 */

s32 MapGroup_IsOpen(u16 map);
s32 Quest_MatchesSeasonTime(u16 id, s32 season, s32 isPm);
s32 Quest_CheckOrder(u16 id);
s32 Quest_IsOffered(u16 id, s32 listKind, s32 withMustOrder);
s32 ElderForecast_IsUnlocked(void);
s32 ElderForecast_QuestAppears(u16 id, s32 listKind);
void Npc_RefreshOffers(void);
void QuestList_Rebuild(void);
void QuestList_Build(s32 kind);
s32 QuestList_CountGroup(s32 group, s8 set);
void QuestList_DropGroup(s32 group, s32 keep, s8 set);
void QuestList_DropRandom(s8 set);

#endif
