#ifndef DOJO_MENU_H
#define DOJO_MENU_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "quest.h"

/*
 * The dojo's menu (TOWN_MODE_DOJO), opened at the instructor
 * (DojoMenu_Open): the drills (the dojo's quests, open by the season, the
 * time of day and the village's progress), the point exchange (PointShop_*),
 * the rules' pages and the training. DojoMenu_Update runs it each frame. The
 * Dojo_*Cleared checks tell whether a set of drills is all cleared, for the
 * townspeople's talk too.
 */

void DojoMenu_Open(struct Em *instructor);
void DojoMenu_Close(void);
s32 Dojo_IsOpen(void);
void DojoMenu_Update(void);
void DojoMenu_StartTraining(DojoMenu *dojo);
s32 DojoMenu_TrainingInput(u16 pad);
void DojoMenu_LoadTrainingQuests(DojoMenu *dojo);
s32 DojoMenu_BasicsInput(DojoMenu *dojo, u16 pad);
s32 DojoMenu_TrainingCategoryInput(DojoMenu *dojo, u16 pad);
s32 DojoMenu_ChoiceInput(u16 pad);
s32 DojoMenu_DrillsInput(u16 pad);
s32 DojoMenu_RulesInput(u16 pad);
s32 Dojo_OfflineDrillsCleared(void);
s32 Dojo_OfflineChainCleared(void);
s32 Dojo_OfflineAllCleared(void);
s32 Dojo_OnlineDrillsCleared(void);
s32 Dojo_OnlineChainCleared(void);
void PointShop_LoadPage(s16 page);
s32 PointShop_CanBuy(s16 row, s16 count);
s32 PointShop_Input(u16 pad);
s32 DojoMenu_QuestInput(u16 pad);
s32 DojoMenu_PostOnline(u16 pad);
s32 DojoMenu_SettingsInput(u16 pad);
s32 DojoMenu_IsDrillOpen(u16 id);
void DojoMenu_ListHuntDrills(void);
void DojoMenu_ListArenaDrills(void);
s32 DojoMenu_DrillsCategoryInput(u16 pad);
s32 DojoMenu_DrillKindInput(u16 pad);
void DojoMenu_DrawDrillsMenu(s16 x, s16 y, s8 byNext);
void DojoMenu_DrawDrillDetails(s16 x, s16 y, QuestInfo *quest);
void DojoMenu_DrawSettings(s16 x, s16 y);
void DojoMenu_DrawQuestPage(s16 x, s16 y);
void PointShop_Draw(s16 x, s16 y);
void DojoMenu_DrawRules(s16 x, s16 y);
s16 DojoMenu_RulesFirstPage(s16 topic);
void DojoMenu_DrawBasics(DojoMenu *dojo, s16 x, s16 y);
void DojoMenu_DrawPanels(MenuTask *task);
void DojoMenu_Draw(MenuTask *task);

#endif
