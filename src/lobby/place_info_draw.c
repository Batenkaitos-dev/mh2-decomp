/*
 * The draw of the field menu's place info page in the town: picks the
 * window's version for where the player is (online, away from the village,
 * or in the village) and draws it at the window's slide position. The
 * versions themselves, and the page's open and input, are the place info
 * page's own (PlaceInfo_DrawOnline, PlaceInfo_DrawPlace,
 * PlaceInfo_DrawVillage).
 */
#include "common.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_menu.h"
#include "place_info_draw.h"
#include "font.h"
#include "game_util.h"

/* unit's own declaration: as the caller uses it; with the definition's s16 (s16 x, s16 y, s32 h), the call in PlaceInfo_DrawAt compiles differently */
s32 PlaceInfo_DrawPlace(s32, s32, s32);
/* unit's own declaration: as the caller uses it; with the definition's void (s16 x, s16 y), the call in PlaceInfo_DrawAt compiles differently */
void PlaceInfo_DrawOnline(s32, s32);
/* unit's own declaration: as the caller uses it; with the definition's void (s16 x, s16 y), the call in PlaceInfo_DrawAt compiles differently */
void PlaceInfo_DrawVillage(s32, s32);

/* Draws the place info window at (x, y), with 18-pixel character spacing:
 * the online version when online; only the place block while the player is
 * away from the village (SaveData.awayFromVillage); else the village's
 * version with its facilities. */
void PlaceInfo_DrawAt(s32 x, s32 y)
{
    Font_SetSpacing(18, 18);
    if (Game_IsOnline() == 1) {
        PlaceInfo_DrawOnline(x, y);
        return;
    }
    if (gSave.awayFromVillage != 0) {
        PlaceInfo_DrawPlace(x, y, PLACE_INFO_H_PLACE);
        return;
    }
    PlaceInfo_DrawVillage(x, y);
}

extern MenuWork gMenuWork;

/* The page's draw (the info page calls it in the town): the window at its
 * slide position, PlaceInfoMenu.win.pos. */
void PlaceInfo_Draw(void)
{
    PlaceInfo_DrawAt(gMenuWork.win.pos, 58);
}
