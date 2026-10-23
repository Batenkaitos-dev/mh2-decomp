#ifndef MARKET_H
#define MARKET_H

#include "common.h"
#include "menu_task.h"

/*
 * The town's market: the hunt rotation (Rotation_Update: the wanted
 * monsters and items of the next days), the monster rates and prices
 * (Rate_*), and the price board that shows them (TOWN_MODE_PRICE_INFO:
 * PriceInfo_*).
 */

s32 Rate_UpdateDue(void);
s32 Monster_IsHunted(u16 em);
void Rotation_Update(s32 kind);
void Rate_RaiseRandom(void);
void Rate_UpdatePrices(void);
void PriceInfo_Open(void);
s32 PriceInfo_IsListed(s32 idIn);
void PriceInfo_Close(void);
s32 PriceInfo_SlideWindows(s16 fromX, s16 toX);
void PriceInfo_Update(void);
s32 PriceInfo_ListInput(u16 pad);
void PriceInfo_DrawDetails(s16 x, s16 y, void *wantedIn, u8 listIn, s8 line);
void PriceInfo_Draw(MenuTask *layer);
void PriceInfo_Draw2(MenuTask *layer);

#endif
