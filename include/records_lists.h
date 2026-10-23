#ifndef RECORDS_LISTS_H
#define RECORDS_LISTS_H

#include "common.h"
#include "item.h"
#include "lobby_vobj.h"

/*
 * The records menu's lists and replies (TOWN_MODE_RECORDS). The records
 * shown are those of the house's owner: from the save, or asked of the owner
 * by town message, whose answers the Records_Recv* functions take. Also the
 * counts of the equipment the owner can wear and has owned, the equipment
 * list's pages, and the menu's open (Records_Open).
 */

void Records_RecvEquipBits(void *from, s32 equipType, void *bits);
s32 Weapon_IsInGenre(EquipItem *item, s16 genre);
void Records_TallyWeapons(s32 *out);
void Records_TallyArmor(s32 *out);
void Records_SetEquipPage(RecordMenu *menu, s16 page);
void Records_RecvEquipTally(void *from, void *tally);
void Records_RecvItemBits(void *from, void *bits);
void Records_RecvTitleBits(void *from, void *bits);
void Records_RecvMonsterRecords(void *from, u8 *data, s8 part);
void Records_Open(void);

#endif
