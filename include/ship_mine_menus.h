#ifndef SHIP_MINE_MENUS_H
#define SHIP_MINE_MENUS_H

#include "common.h"
#include "menu_task.h"
#include "lobby_vobj.h"

/*
 * Two townspeople's menus: the ship menu (TOWN_MODE_SHIP_MENU), which boards
 * the ship with a camera tour and a jingle or shows the offers (ShipMenu_*),
 * and the village mine's menu (TOWN_MODE_MINE), where the player gives the
 * miner a pickaxe and later takes the find (Mine_OpenMenu, Mine_UpdateMenu,
 * Mine_IsOpen).
 */

s32 ShipMenu_IsTouring(void);
s32 ShipMenu_IsOffered(void);
void ShipMenu_Open(u8 *npc);
void ShipMenu_Update(void);
s32 ShipMenu_Input(ShipMenu *menu, s32 pad);
void ShipMenu_Draw(MenuTask *task);
void Mine_OpenMenu(u8 *npc);
void Mine_UpdateMenu(void);
s32 Mine_IsOpen(void);

#endif
