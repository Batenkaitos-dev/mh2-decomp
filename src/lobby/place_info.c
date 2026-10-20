/*
 * The field menu's place info page in the town (the info page's location
 * entry): its open, its input, and the window that says where the player
 * is.
 *
 * Every version of the window draws the place block: the season with its
 * mark and the area's name (PlaceInfo_DrawPlace). Online the page adds the
 * server, the gate (level 0) and room (level 1) chosen with their marks, the
 * room's recruiting message, group limit and target quest, and the gate's
 * users (PlaceInfo_DrawOnline); in the village it adds the four facilities'
 * levels (PlaceInfo_DrawVillage). The page keeps its window's slide and
 * step in the menu work area (PlaceInfoMenu). Which version is drawn, and
 * where, is decided by the page's draw (PlaceInfo_DrawAt, PlaceInfo_Draw).
 */
#include "common.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "scene.h"
#include "lobby_menu.h"
#include "save.h"
#include "pad.h"
#include "lobby_labels.h"

extern u8 gMenuWork[];
void GameMenu_Back(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);

void *memset(void *dst, s32 c, u32 n);

/* Opens the place info page: hides the help line, clears the menu work
 * area and starts the window's slide in from the right edge. Returns 0. */
s32 PlaceInfo_Open(void)
{
    gHelpShow = 0;
    memset(gMenuWork, 0, MENU_WORK_SIZE);
    WinSlide_Step(&((PlaceInfoMenu *)gMenuWork)->win, 0x280, PLACE_INFO_X);
    return 0;
}

/* The page's input: PAD_START closes the field menu and returns 0; the
 * window slides in, and once it is in, PAD_CROSS returns PAD_CROSS (the info
 * page goes back to its list). Returns the buttons otherwise. */
s32 PlaceInfo_Input(s32 buttons)
{
    PlaceInfoMenu *menu = (PlaceInfoMenu *)gMenuWork;
    u16 pad = buttons;

    if (pad & PAD_START) {
        GameMenu_Back();
        return 0;
    }
    switch (menu->step) {
    case PLACE_INFO_STEP_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 0x280, PLACE_INFO_X)) {
        case MENU_DONE:
            menu->step++;
            break;
        }
        break;
    case PLACE_INFO_STEP_SHOWN:
        if (pad & PAD_CROSS) {
            return PAD_CROSS;
        }
        break;
    }
    return buttons;
}

extern char *gAreaNameTexts[];
u8 Area_GetSetIndex(u16 id);

/* Returns an area's name: the entry of its area set in gAreaNameTexts, or
 * the table's last entry (15) for area 0. */
void *LobbyArea_GetName(u16 id)
{
    u8 setIndex = Area_GetSetIndex(id);

    if (id == 0) {
        return gAreaNameTexts[15];
    }
    return gAreaNameTexts[setIndex];
}

s32 LobbyWorld_GetSeason(void);
/* unit's own declaration: as the callers use it; with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind), the call in PlaceInfo_DrawPlace compiles differently */
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
extern char gPlaceInfoFmtString[];
extern char *gSeasonNames[];
extern u32 gSeasonColors[];

/* Draws the page's window of the given height at (x, y), its title bar and title,
 * then the place block: the season's name with its coloured mark, and the
 * name of the area the player is in. Returns the y of the next row. (The
 * area's set index is looked up and not used.) */
s16 PlaceInfo_DrawPlace(s16 x, s16 y, s32 height)
{
    s32 season;
    s16 rowY;
    s32 seasonOffset;
    s32 left;
    s32 valueX;

    season = LobbyWorld_GetSeason();
    rowY = y;
    Area_GetSetIndex(gScene.area);
    Win_Draw(x, y, PLACE_INFO_W, height, 0xBDB59F6C, 0xC0202020, 7);
    Gfx_DrawLine(x, y, PLACE_INFO_W, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x, rowY + 1);
    Text_PrintfColored(gInfoMenuLabels[0]);
    Font_SetPos(x, rowY += 0x16);
    Text_PrintfColored(gPlaceInfoFmtString, gPlaceInfoTexts[PLACE_INFO_TEXT_SEASON]);
    Font_SetColor(0x807F7F7F);
    left = x;
    valueX = left + 0x6E;
    Font_SetPos(valueX, rowY);
    /* the two tables read at one byte offset: the game's code keeps
     * season * 4 in one register for both */
    seasonOffset = season * 4;
    Text_PrintfColored(gPlaceInfoFmtString, *(char **)((u8 *)gSeasonNames + seasonOffset));
    Icon_Draw(left + 0x56, rowY - 1, 0x14, 0x14, *(u32 *)((u8 *)gSeasonColors + seasonOffset), season + ICON_SEASON_FIRST);
    Font_SetPos(x, rowY += 0x14);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gPlaceInfoFmtString, gPlaceInfoTexts[PLACE_INFO_TEXT_PLACE_DETAIL]);
    Font_SetPos(valueX, rowY);
    Font_SetColor(0x807F7F7F);
    Text_PrintfColored(gPlaceInfoFmtString, LobbyArea_GetName(gScene.area));
    return rowY + 0x14;
}

void Text_Printf(const char *fmt, ...);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
s8 *LobbySrv_CurName(void);
char *LobbySrv_MyName(void);
s32 LobbySrv_CurIndex(void);
s32 LobbySrv_ColorAt(s32 i);
u32 AreaList_KindColor(u8 slot);
s32 LobbyLevel0_HasGroups(void);
extern char gPlaceInfoFmtLabel[];
extern char gPlaceInfoFmtValue[];
extern char gPlaceInfoFmtUsers[];
extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
extern LevelEntry gLobbyLevel0Entries[];
extern LevelEntry gLobbyLevel1Entries[];

/* The online window: the place block, then the location, the server's name
 * with one mark in the server's colour, the chosen gate's name with two
 * marks in its kind's colour, the chosen room's name with three, the room's
 * recruiting message, its group limit and its target quest (a dash without a
 * room, or while the gate has no groups), and the gate's users out of its
 * maximum. */
void PlaceInfo_DrawOnline(s16 x, s16 y)
{
    u32 color;
    s16 rowY;
    s32 left;
    char *questName;
    s32 iconX;

    rowY = PlaceInfo_DrawPlace(x, y, PLACE_INFO_H_ONLINE);
    Font_SetPos(x, rowY);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gPlaceInfoFmtString, gPlaceInfoTexts[PLACE_INFO_TEXT_PLACE]);
    left = x;
    Font_SetPos(left + 0x6E, rowY);
    Font_SetColor(0x807F7F7F);
    Text_PrintfColored(gPlaceInfoFmtString, LobbySrv_CurName());
    Font_SetPos(left + 0x6E, rowY += 0x14);
    Text_PrintfColored(gPlaceInfoFmtString, LobbySrv_MyName());
    iconX = (s16)(left + 0x6E) - 0xE;
    LobbyUi_DrawIcon(iconX, rowY + 9, 0x14, LobbySrv_ColorAt(LobbySrv_CurIndex()) | 0xFF000000, 1);
    Font_SetPos(left + 0x6E, rowY += 0x14);
    if (gLobbyLevel0Sel != 0) {
        Text_PrintfColored(gPlaceInfoFmtString, gLobbyLevel0Entries[gLobbyLevel0Sel - 1].name);
        color = AreaList_KindColor(gLobbyLevel0Entries[gLobbyLevel0Sel - 1].kind);
        color |= 0xFF000000;
        LobbyUi_DrawIcon(iconX, rowY + 9, 0x14, color, 2);
    }
    Font_SetPos(left + 0x6E, rowY += 0x14);
    if (gLobbyLevel1Sel != 0) {
        Text_PrintfColored(gPlaceInfoFmtString, gLobbyLevel1Entries[gLobbyLevel1Sel - 1].name);
        LobbyUi_DrawIcon(iconX, rowY + 9, 0x14, color, 3);
    }
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gNetLabels[NET_LABEL_RECRUIT_MSG_LABEL]);
    if (gLobbyLevel1Sel != 0) {
        Font_SetPos(left + 0x6E, rowY);
        Text_Printf(gPlaceInfoFmtValue, gLobbyLevel1Entries[gLobbyLevel1Sel - 1].text);
    } else {
        Font_SetPos(left + 0x6E, rowY);
        Text_Printf(gPlaceInfoFmtValue, gQuestSheetTexts[QUEST_SHEET_TEXT_NO_VALUE]);
    }
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gNetLabels[NET_LABEL_LIMIT_COND]);
    Font_SetPos(left + 0x6E, rowY);
    if (LobbyLevel0_HasGroups() == 1 && gLobbyLevel1Sel != 0) {
        Text_Printf(gPlaceInfoFmtValue, gGroupLimitTexts[gLobbyLevel1Entries[gLobbyLevel1Sel - 1].groupInfo[0]]);
    } else {
        Text_Printf(gPlaceInfoFmtValue, gQuestSheetTexts[QUEST_SHEET_TEXT_NO_VALUE]);
    }
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gNetLabels[NET_LABEL_TARGET_COLON]);
    questName = ((RoomData *)((s32)(*(s32 *)&gLobbyWorkBuf + 0x6EF4) + 0x20))->quests[gLobbyLevel1Entries[gLobbyLevel1Sel - 1].groupInfo[1]].name;
    Font_SetPos(left + 0x6E, rowY);
    if (LobbyLevel0_HasGroups() == 1 && gLobbyLevel1Sel != 0) {
        Text_Printf(gPlaceInfoFmtValue, questName);
    } else {
        Text_Printf(gPlaceInfoFmtValue, gQuestSheetTexts[QUEST_SHEET_TEXT_NO_VALUE]);
    }
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gPlaceInfoTexts[PLACE_INFO_TEXT_AREA_POP]);
    Font_SetPos(left + 0x6E, rowY);
    Text_Printf(gPlaceInfoFmtUsers, gLobbyLevel0Entries[gLobbyLevel0Sel - 1].users, gLobbyLevel0Entries[gLobbyLevel0Sel - 1].maxUsers);
}

u8 Village_FacilityLevel(s32 kind);
extern char *gTavernLevelTexts[];
extern char *gPortLevelTexts[];
extern char *gSmithyLevelTexts[];
extern char *gMineLevelTexts[];

/* The village's window: the place block, a heading, then the levels of the
 * tavern, the port, the smithy and the mine (the mine's levels share a text
 * three by three). */
void PlaceInfo_DrawVillage(s16 x, s16 y)
{
    s16 rowY;
    s32 left;

    rowY = PlaceInfo_DrawPlace(x, y, PLACE_INFO_H_VILLAGE);
    Font_SetPos(x, rowY);
    Text_Printf(gPlaceInfoFmtLabel, gPlaceInfoTexts[PLACE_INFO_TEXT_VILLAGE_HEAD]);
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gPlaceInfoTexts[PLACE_INFO_TEXT_TAVERN]);
    left = x;
    Font_SetPos(left + 0x3C, rowY);
    Text_Printf(gPlaceInfoFmtValue, gTavernLevelTexts[Village_FacilityLevel(VILLAGE_FACILITY_TAVERN)]);
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gPlaceInfoTexts[PLACE_INFO_TEXT_PORT]);
    Font_SetPos(left + 0x3C, rowY);
    Text_Printf(gPlaceInfoFmtValue, gPortLevelTexts[Village_FacilityLevel(VILLAGE_FACILITY_PORT)]);
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gPlaceInfoTexts[PLACE_INFO_TEXT_SMITHY]);
    Font_SetPos(left + 0x3C, rowY);
    Text_Printf(gPlaceInfoFmtValue, gSmithyLevelTexts[Village_FacilityLevel(VILLAGE_FACILITY_SMITHY)]);
    Font_SetPos(x, rowY += 0x14);
    Text_Printf(gPlaceInfoFmtLabel, gPlaceInfoTexts[PLACE_INFO_TEXT_MINE]);
    Font_SetPos(left + 0x3C, rowY);
    Text_Printf(gPlaceInfoFmtValue, gMineLevelTexts[Village_FacilityLevel(VILLAGE_FACILITY_MINE) / 3]);
}

/* The page's formats: a plain string, a label, a value (each in its text
 * colour), and the users out of the maximum. */
#pragma rodata on
char gPlaceInfoFmtString[0x8] = {
#include "lobby/00825F28.inc"
};
char gPlaceInfoFmtLabel[0x8] = {
#include "lobby/00825F30.inc"
};
char gPlaceInfoFmtValue[0x8] = {
#include "lobby/00825F38.inc"
};
char gPlaceInfoFmtUsers[0x10] = {
#include "lobby/00825F40.inc"
};
#pragma rodata off
