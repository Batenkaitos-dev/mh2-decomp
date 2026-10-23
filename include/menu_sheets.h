#ifndef MENU_SHEETS_H
#define MENU_SHEETS_H

#include "common.h"

/*
 * Drawing helpers the lobby's menus share: the paper sheets turned per
 * page, with their tabs and frame strip (Menu_DrawQuestSheet,
 * Menu_DrawFrameStrip), the decorated frames (Menu_DrawDecoFrame,
 * Menu_DrawDecoBox), the quest's stamp (Menu_DrawStamp,
 * QuestBoard_DrawStamp) and the bobbing arrows beside a text
 * (Menu_DrawArrows and its forms).
 */

/* One page's turn of the quest sheet pile (gStripTurnOffsets, 12 of them):
 * the page's x and y offsets, its pivot's offsets from the centre, its angle. */
typedef struct SheetTurn {
    s16 dx;
    s16 dy;
    s16 pivotDx;
    s16 pivotDy;
    s16 angle;
} SheetTurn;

void Menu_DrawDecoFrame(s16 left, s16 top, s16 width, s16 height, s8 kind, s8 emblem);
void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 height, s8 kind);
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 height, s16 *tabs, s16 page, s8 kind, s32 emblem);
void Menu_DrawFrameStrip(s16 x, s16 y, s16 w, s16 height, u8 kind, s8 page);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 height, u16 seed, s16 pressTimer);
void QuestBoard_DrawStamp(s16 x, s16 y, s16 w, s16 height, u16 seed, s16 pressTimer);
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Menu_DrawArrowsSized(s16 x, s16 y, s16 arrowSize, s16 size, char *text, s32 color);

#endif
