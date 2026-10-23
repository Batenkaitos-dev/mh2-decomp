#ifndef VILLAGE_BOARD_H
#define VILLAGE_BOARD_H

#include "common.h"
#include "menu_task.h"
#include "lobby_menu.h"

/*
 * The village board (TOWN_MODE_VILLAGE_BOARD): a notice board of articles
 * in five categories, with the articles read marked in the save
 * (VillageBoard_MarkRead, VillageBoard_IsRead).
 */

void VillageBoard_MarkRead(s16 n);
s32 VillageBoard_IsRead(u16 n);
void VillageBoard_Open(void);
void VillageBoard_Update(void);
s32 VillageBoard_ArticleInput(void *board, u16 pad);
s32 VillageBoard_ListInput(VillageBoardMenu *board, u16 pad);
void VillageBoard_BuildPage(VillageBoardMenu *board, s16 page);
s32 VillageBoard_TopInput(VillageBoardMenu *board, u16 pad);
void VillageBoard_DrawTop(VillageBoardMenu *board, s16 x, s16 y);
void VillageBoard_DrawArticles(VillageBoardMenu *board, s16 x, s16 y);
void VillageBoard_DrawArticle(VillageBoardMenu *board, s16 x, s16 y);
void VillageBoard_Draw(MenuTask *task);

#endif
