#ifndef OFFICIAL_DESK_H
#define OFFICIAL_DESK_H

#include "common.h"
#include "menu_task.h"
#include "lobby_menu.h"
#include "quest.h"

/*
 * The official desk (TOWN_MODE_OFFICIAL_DESK), the lobby's counter for the
 * official quests: rows of the downloaded event quests, the next quest of
 * the HR gate and the elder dragon raid's quests. A quest chosen is checked,
 * given its room settings, posted to the lobby and accepted as official.
 * OfficialDesk_Open and OfficialDesk_Update run it, OfficialDesk_Draw and
 * OfficialDesk_Draw2 draw it; the must-order quests' first two sheet pages
 * are drawn here too (Quest_DrawMustOrderOverview,
 * Quest_DrawMustOrderTerms).
 */

void OfficialDesk_Open(struct Em *npc);
void *Quest_DownloadedInfo(u8 slot);
void OfficialDesk_ListEvents(OfficialDeskMenu *desk);
void OfficialDesk_FindGateQuest(OfficialDeskMenu *desk, s32 gate);
s32 OfficialDesk_GateIndex(struct Em *npc);
void OfficialDesk_ListRaid(OfficialDeskMenu *desk);
void OfficialDesk_Close(void);
void OfficialDesk_Update(void);
s32 OfficialDesk_Post(OfficialDeskMenu *menu, u16 pad);
s32 OfficialDesk_CheckOrder(OfficialDeskMenu *desk, u16 pad);
s32 OfficialDesk_ChooseQuest(OfficialDeskMenu *desk, u16 pad);
s32 OfficialDesk_ChooseRow(OfficialDeskMenu *desk, u16 pad);
void OfficialDesk_DrawRows(s16 x, s16 y);
void Quest_DrawMustOrderOverview(s16 x, s16 y, QuestInfo *quest);
void Quest_DrawMustOrderTerms(s16 x, s16 y, QuestInfo *quest);
void OfficialDesk_DrawSheet(OfficialDeskMenu *desk, s16 x, s16 y);
void OfficialDesk_DrawRoomSettings(OfficialDeskMenu *desk, s16 x, s16 y);
void OfficialDesk_Draw(MenuTask *task);
void OfficialDesk_Draw2(MenuTask *task);

#endif
