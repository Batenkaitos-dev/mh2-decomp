#ifndef MENU_UTIL_H
#define MENU_UTIL_H

#include "common.h"
#include "grid.h"
#include "sprite.h"
#include "player.h"

/*
 * The in-game menus' common helpers: the HUD's textures, pausing
 * (Game_Pause, Game_Resume), cursors through a list, a grid, the pouch's
 * columns, the phrase menu's columns and a yes/no choice, the left stick
 * read as a D-pad (Pad_StickToDpad), whether an item can be used now, the
 * pouch's sorting, the menus' cursor bar, icons and rarity colours, and the
 * queries the map draws with (the detect skill, the map's halves).
 */

void Hud_LoadTextures(void);
void Game_Pause(u8 withStreams);
void Game_Resume(u8 withStreams);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);
void Menu_GridCursor(Grid *grid, u16 buttons);
u16 PouchMenu_MoveCursor(u8 *cur, u16 buttons);
void Pouch_MoveCursor(u8 *cur, u16 buttons);
u8 PhraseMenu_MoveCursor(u16 buttons, u8 cur);
void PouchMenu_DecoCursor(u16 item, u16 buttons);
u8 YesNo_Toggle(u16 buttons, u8 choice, u16 mask);
u16 Pad_StickToDpad(u8 pad);
s32 Chat_InInputMode(void);
s32 Pouch_IsSlotUsable(Player *pl, u16 slot);
s32 Item_IsUsable(u16 id);
s32 Player_IsFree(Player *pl);
s32 Save_HasRate(void);
s32 StatusPage_HasSymbolColor(void);
s16 Pouch_FixCursor(void);
void Pouch_Compact(u8 box);
s32 Player_IsAiming(Player *pl);
void ListPanel_DrawCursorBar(s32 left, s32 right, s16 top, s16 step, s32 index, s32 rgb);
void Draw_Icons(Icon *icons, u8 count);
s16 Rarity_Color(u8 rarity);
u8 Player_DetectLevel(Player *pl);
s32 Player_GetMapHalf(Player *pl);
s32 Map_GetHalf(u8 stage, u8 area);
void Text_DrawDigit(u8 digit);

#endif
