#ifndef RECORDS_MENU_H
#define RECORDS_MENU_H

#include "common.h"
#include "lobby_vobj.h"

/*
 * The records menu's update (TOWN_MODE_RECORDS): Records_Update runs its
 * modes (the window sliding, the menu of record kinds, one screen per kind),
 * with each screen's input: the equipment, the scores, the monsters, the
 * guides, the titles and the items. Each input returns MENU_RUNNING, or
 * MENU_CANCEL when the player leaves the screen.
 */

void Records_Update(void);
s32 Records_EquipInput(RecordMenu *menu, u16 pad);
s32 Records_EquipListInput(RecordMenu *menu, u16 pad);
s32 Records_ScoresInput(RecordMenu *menu, u16 pad);
void Records_MonstersInit(RecordMenu *menu);
s32 Records_MonstersInput(RecordMenu *menu, u16 pad);
void Records_GuidesInit(RecordMenu *menu);
s32 Records_GuidesInput(RecordMenu *menu, u16 pad);
s32 Records_TitlesInput(RecordMenu *menu, u16 pad);
void Records_SetTitlePage(RecordMenu *menu, s16 page);
s32 Records_ItemsInput(RecordMenu *menu, u16 pad);
void Records_TallyTitles(RecordMenu *menu);
void Records_EquipInit(RecordMenu *menu);

#endif
