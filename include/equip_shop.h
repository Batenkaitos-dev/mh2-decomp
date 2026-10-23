#ifndef EQUIP_SHOP_H
#define EQUIP_SHOP_H

#include "common.h"
#include "item.h"
#include "capsule.h"

/*
 * The equipment shop (TOWN_MODE_EQUIP_SHOP): buying and selling weapons and
 * armour, the page of equipment on sale with its details and the model
 * preview on the hunter, the purchase's questions and the box to sell from.
 * EquipShop_Open and EquipShop_Close open and end it, EquipShop_Update runs
 * it each frame and EquipShop_DrawTask draws it. The file's data also holds
 * the lobby's monster hit tables (gEmBoneHitsLobby, gEmPushSpheresLobby).
 */

void EquipShop_Open(u8 *npc);
void EquipShop_Close(void);
void EquipShop_Update(void);
s32 EquipShop_InputTop(u16 pad);
void EquipShop_BuildPage(void);
void EquipShop_LoadCompare(u8 type);
void EquipShop_OpenList(void);
void EquipShop_TryOn(void);
void EquipPreview_Hide(u8 hide);
s32 EquipShop_InputList(u16 pad);
void EquipBox_FirstPage(void);
s32 EquipShop_InputSell(u16 pad);
void EquipShop_Choose(void);
s32 EquipShop_InputBuy(u16 pad);
void EquipBox_SellPrice(void);
s32 EquipBox_SellMenu(u16 pad);
void EquipShop_DrawMenu(void);
void EquipShop_DrawLine(s16 x, s16 y, EquipItem *piece);
void EquipShop_DrawNote(void);
void EquipShop_DrawPage(s16 x, s16 y);
void EquipShop_DrawPageSmall(void);
void Equip_DrawPreviewHelp(u32 color, s8 mode);
void EquipShop_DrawList(void);
void EquipShop_DrawPreview(void);
void EquipShop_DrawPageDetail(void);
void EquipShop_DrawTitle(void);
void Equip_DrawWearer(EquipItem *piece);
void EquipBox_DrawSellNote(s16 x, s16 y);
void EquipBox_DrawSellPrice(s16 x, s16 y);
void EquipShop_DrawTask(u8 *task);
extern u32 gHeaderBoxColors[4];
extern HitShape *gEmBoneHitsLobby[4];
extern f32 *gEmPushSpheresLobby[4];

#endif
