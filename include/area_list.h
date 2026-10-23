#ifndef AREA_LIST_H
#define AREA_LIST_H

#include "common.h"
#include "lobby_menu.h"
#include "lobby_net.h"

/*
 * The lobby's area list (LOBBY_PHASE_AREA_LIST), the screen between the
 * server choice and the village. AreaList_Begin opens it and AreaList_Run
 * runs it: the server's areas are fetched (LobbyStep_Level0List), grouped by
 * kind into pages, and the player chooses one to enter. The AreaList_Draw*
 * functions draw it.
 */

void AreaList_Begin(void);
void AreaList_Run(void);
void AreaList_Init(void);
void AreaList_Group(AreaList *list);
s32 AreaList_WaitSlide(void);
s32 AreaList_Input(void);
u32 AreaList_KindColor(u8 kind);
void AreaList_DrawEntries(s16 x, s16 y);
void AreaList_DrawFrame(MenuTask *layer);
void AreaList_DrawPanels(MenuTask *layer);
void AreaList_DrawGuide(void);
void AreaList_DrawList(void);
s32 LobbyStep_Level0List(void);
void LobbyStep_Level0ListDone(NetMsg msg, NetMsg *msgPtr);

#endif
