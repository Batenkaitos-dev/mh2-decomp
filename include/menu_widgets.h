#ifndef MENU_WIDGETS_H
#define MENU_WIDGETS_H

#include "common.h"
#include "lobby_menu.h"

/*
 * The lobby's menu drawing kit: the icon sheet's entries (gIconSprites;
 * Icon_Draw and its forms), the pieces built from them (the money and
 * points boxes, dashed lines, row bars, cursors, side arrows, page numbers,
 * the framed box, the mail icon, the level marks), and the pulsing colours
 * of cursors and highlights (Color_Pulse, Color_PulseAlpha).
 */

void Str_FromInt(s32 n, char *buf);
void Menu_DrawMoney(s16 x, s16 y, s8 dim);
void Menu_DrawPoints(s16 x, s16 y);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Menu_DrawBarRotated(s16 x, s16 y, s16 w, s16 height, u32 color, s32 angle, s16 pivotX, s16 pivotY, u8 k);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 sel, u8 kind);
void Menu_DrawRowBar(s16 x, s16 y, s16 w, u32 color, u8 sel, u8 kind);
void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 pageCount, u32 color);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 pageCount, u8 narrow);
void Menu_DrawCursorIcon(s16 x, s16 y, s16 size, u32 color);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 gap, u32 color, u32 cursorColor);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);
u32 Color_Pulse(void);
u32 Color_PulseAlpha(u32 color);
void Menu_DrawRowHighlight(s16 x, s16 y, s16 w, s16 height, s8 still);
void LobbyUi_DrawLevelMarks(s16 x, s16 y, s16 size, u32 color, s8 kind);
void Menu_DrawFrameBox(s16 x, s16 y, s16 w, s16 height, u32 color);
void Menu_DrawMailIcon(s16 x, s16 y);
void Icon_Draw(s16 x, s16 y, s16 w, s16 height, u32 color, s32 i);
void Icon_DrawRotatedAt(s16 x, s16 y, s16 w, s16 height, u32 color, s32 angle, s32 i, s16 pivotX, s16 pivotY);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 height, u32 color, s32 i, u8 mode);
void Icon_DrawPreset(s32 i);
extern IconSprite gIconSprites[ICON_SPRITE_COUNT];

#endif
