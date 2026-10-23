#ifndef NAME_TAGS_H
#define NAME_TAGS_H

#include "common.h"
#include "menu_task.h"

/*
 * The town's draws over its people, and small draws the lobby's menus
 * share. LobbyMenu_AddHudLayers queues the lobby menu's layers each frame:
 * the name tags (NameTags_*: the names over the members' heads, the offer
 * marks over the townspeople), the members' names window and the village
 * clock; TownPc_DrawTargetBanner draws the banner of what the action button
 * acts on. Then a pulsing gradient bar, the button icons
 * (Lobby_DrawButtonIcon) and the lobby's square sprites (LobbySprite_Draw).
 */

void LobbyMenu_AddHudLayers(void);
void MyHouse_DrawLabel(void);
void NameTags_DrawLayer(MenuTask *task);
void NameTags_Draw(MenuTask *task);
void NameTags_DrawNpcs(void);
void TownPc_DrawTargetBanner(MenuTask *task);
void Gfx_DrawPulseGradient(s16 x, s16 y, s16 width, s16 height, u32 color, u8 dir);
s16 Lobby_DrawButtonIcon(s16 x, s16 y, s32 icon);
void LobbySprite_Draw(s32 x, s16 y, s32 w, s32 color, s16 kind);
extern s32 gQuestCategoryColors[8];
extern u32 gLabelIconsNeg[20];
extern u32 gLabelIconsPos[20];
extern char gLobbyStrFmt[8];

#endif
