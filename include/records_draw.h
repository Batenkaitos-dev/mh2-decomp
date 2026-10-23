#ifndef RECORDS_DRAW_H
#define RECORDS_DRAW_H

#include "common.h"
#include "menu_task.h"
#include "lobby_vobj.h"

/*
 * The records menu's input on its first screens and all its draws
 * (TOWN_MODE_RECORDS): the kinds menu's input (Records_MenuInput) and the
 * house access page's (Records_HouseAccessInput), the draws of every screen
 * and list, and the menu's two draw callbacks (Records_Draw,
 * Records_DrawMenuLayer).
 */

void Records_TallyItems(RecordMenu *menu);
s32 Records_MenuInput(RecordMenu *menu, u16 pad);
s32 Records_HouseAccessInput(RecordMenu *menu, u16 pad);
void Records_DrawMenu(RecordMenu *menu, s16 x, s16 y);
void Records_DrawTitleList(RecordMenu *menu, s16 x, s16 y);
void Records_DrawItemList(RecordMenu *menu, s16 x, s16 y);
void Records_DrawWeaponList(RecordMenu *menu, s16 x, s16 y);
void Records_DrawArmorList(RecordMenu *menu, s16 x, s16 y);
void Records_Draw(MenuTask *task);
void Records_DrawMenuLayer(MenuTask *task);
void Records_DrawHunts(RecordMenu *menu, s16 x, s16 y);
s32 Records_CountHuntPages(u8 *save);
extern RecordCat gRecordGuideCats[];

#endif
