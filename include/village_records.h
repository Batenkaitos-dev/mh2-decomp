#ifndef VILLAGE_RECORDS_H
#define VILLAGE_RECORDS_H

#include "common.h"
#include "em.h"

/*
 * The village's records in the save, kept between quests: the residents'
 * gifts (handed over by NpcGift_Open and NpcGift_Update), the residents'
 * meal visits, the milestones of the offer points, and the mine's count of
 * digs. The offers call Offer_CheckResidentRewards and
 * Village_CheckMilestone; the quest's end calls Village_ProcessEventFlags
 * and Village_QuestTick.
 */

s16 Village_MealVisitTalk(s32 resident);
void Village_ClearMealVisit(u16 resident);
void Offer_CheckResidentRewards(u32 before, u32 after, s32 offerIdx);
void Village_CheckMilestone(u32 before, u32 after, s32 idx);
s32 Village_MilestoneReady(s32 milestone);
s32 Village_HouseTourPending(void);
void Village_ProcessEventFlags(void);
void Village_QuestTick(void);
s16 Village_MilestoneState(s32 idx);
s32 Village_HasGift(u16 resident);
s32 Village_HasMealVisit(u16 resident);
void NpcGift_Open(Em *npc);
void NpcGift_Update(void);

#endif
