#ifndef ITEM_SHOP_H
#define ITEM_SHOP_H

#include "common.h"
#include "lobby_menu.h"
#include "lobby_globals.h"
#include "menu_task.h"

/*
 * The item shop: a townsperson's counter menu, the buy window over the
 * townsperson's stock, the sell window over the pouch, and the item info
 * panel both windows draw. With them, three helpers of the lobby's menus
 * (Win_DrawText, a plain window with a text; LobbyPad_Read and LobbyPad_Get,
 * the menus' pad read), and some of the lobby's label tables, which the
 * file's data holds: the area, meal and monster-class names, the notices,
 * the message table (gLobbyMsgTexts) and others.
 */

void ItemShop_Open(u8 *npc, s8 sel);
void TownMenu_Close(void);
void ItemShop_Update(void);
void Shop_BuildSellList(ItemListWin *list, s16 page);
void Shop_StartSell(ItemListWin *list);
s32 Shop_SellStep(u16 pad, ItemListWin *list);
void Shop_Sell(ItemListWin *list);
s32 Shop_SellAmountInput(u16 pad, ItemListWin *list);
s32 Item_IsDeco(u16 id);
s32 Shop_SellListInput(u16 pad, ItemListWin *list);
void ItemShop_SetRows(u16 pad);
void Shop_BuildStock(ItemListWin *list, s16 page);
void Shop_StartBuy(ItemListWin *list, u16 kind);
s32 Shop_BuyStep(u16 pad, ItemListWin *list);
void Shop_Buy(ItemListWin *list);
s32 Shop_BuyAmountInput(u16 pad, ItemListWin *list);
s32 Shop_BuyListInput(u16 pad, ItemListWin *list);
s32 Shop_GetBuyPrice(s32 item);
s32 Item_GetSellPrice(s32 item);
s32 Shop_CanBuy(u16 item, s16 n);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color);
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Shop_DrawBuy(void *work);
void Shop_DrawSell(void *work);
void ItemShop_Draw(MenuTask *task);
void Win_DrawText(s16 x, s16 y, s16 w, s16 height, u32 fillColor, u32 edgeColor, char *text);
u16 LobbyPad_Read(s8 port);
u16 LobbyPad_Get(s32 unused);
extern LobbyMsgDef gLobbyMsgTexts[152];

#endif
