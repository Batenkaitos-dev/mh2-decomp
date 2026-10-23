#ifndef DRAW_HELPERS_H
#define DRAW_HELPERS_H

#include "common.h"

/*
 * Small drawing helpers of the menus and the HUD: the pad button icons
 * (Hud_DrawButtonIcon), the wrapping of Shift-JIS text (Text_DrawWrapped),
 * the play-time strings (Time_FormatMinSec), the black line, the pulsing
 * highlight bar (Menu_DrawGlowBar), and the letters that bob along the
 * screen while the game loads (Loading_Request, Loading_DrawLetters).
 */

void Hud_DrawButtonIcon(s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale);
void Hud_DrawButtonIconWhite(s32 id, s32 x, s32 y, s32 depth, f32 scale);
void Text_DrawWrapped33(s16 x, s16 y, s16 lineh, char *text);
s32 Text_DrawWrapped(s16 x, s16 y, s16 lineh, char *text, s16 width);
char *Time_FormatMinSec(u32 totalSeconds);
char *Time_FormatMinSecInHour(u32 totalSeconds);
void Gfx_DrawBlackLineFixed(void);
void Gfx_DrawBlackLine(s32 x, s32 y, s32 width, s32 height);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 height, u32 color);
void Loading_DrawLetters(void);
void Loading_Request(void);

#endif
