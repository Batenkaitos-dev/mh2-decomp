#ifndef OFFER_MENU_H
#define OFFER_MENU_H

#include "common.h"
#include "menu_task.h"
#include "lobby_menu.h"

/*
 * The offer menu a townsperson opens when talked to (TOWN_MODE_OFFER_MENU):
 * the deliveries the townsperson wants and the quests it gives.
 * OfferMenu_BuildList fills the menu for a townsperson, or only probes it
 * for the mark over its head; OfferMenu_Open, OfferMenu_Update and
 * OfferMenu_Draw run it. Also the item helpers other lobby menus use: the
 * counts across the pouch and the item box, taking and giving items
 * (Items_*), and the item rows and need lines the forge, workshop and
 * combination menus draw.
 */

s16 OfferMenu_BuildList(struct Em *t, s32 b);
void OfferMenu_AddQuest(OfferMenu *menu, s16 source, u16 quest);
void OfferMenu_AddDelivery(OfferMenu *menu, s16 origin, s16 storyIdx);
void OfferMenu_AddRequest(OfferMenu *menu, s16 origin, s32 offer);
void OfferMenu_Open(struct Em *t);
void OfferMenu_Close(void);
void OfferMenu_Update(void);
s32 OfferMenu_QuestList(u16 pad);
s32 OfferMenu_DeliveryList(u16 pad);
void Items_Take(s32 item, s32 count);
s8 Items_Give(s32 item, s32 count);
s32 Items_Room(s32 item);
void OfferMenu_Deliver(void);
s32 Items_HaveEnough(u16 item, s32 count);
s32 Items_Total(u16 item);
s32 OfferMenu_HasItems(void);
void OfferMenu_DrawDeliveryPrompt(s16 x, s16 y, s8 prompt);
s32 Items_DrawNeedLine(s16 x, s16 y, u16 item, s32 count, u32 color);
s32 Items_DrawNeedLineAfter(s16 x, s16 y, u16 item, s32 count, u32 color);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);
void Menu_DrawItemPriceRow(s16 x, s16 y, u16 item, u32 color, s32 price);
void OfferMenu_DrawDelivery(s16 x, s16 y);
void OfferMenu_DrawQuest(s16 x, s16 y);
void OfferMenu_Draw(MenuTask *task);

#endif
