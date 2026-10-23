#ifndef SUPPLY_BOX_H
#define SUPPLY_BOX_H

#include "common.h"
#include "player.h"

/*
 * The two menus a player's state runs during a quest: the supply box
 * (SupplyBox_Update) and the quest's delivery box (Player_RunDeliveryMenu,
 * Quest_DeliveryMenu). Then three small queries: whether a trader is near
 * (Player_NearTrader), whether the pouch holds meat (Pouch_HasMeat), and
 * the sound of using an item (Item_PlayUseSound).
 */

void SupplyBox_Update(Player *pl);
void Player_RunDeliveryMenu(Player *pl);
void Quest_DeliveryMenu(Player *pl);
s32 Player_NearTrader(void *obj);
s32 Pouch_HasMeat(Player *pl);
void Item_PlayUseSound(Player *pl, u16 item);

#endif
