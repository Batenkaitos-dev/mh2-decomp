/*
 * The area menu (TOWN_MODE_AREA_MENU) and the walk from one of the village's
 * or the town's areas to the next.
 *
 * AreaMenu_Open runs when the player uses a door (TOWN_SPOT_AREA_DOOR, the
 * house access menu's pick, the ship's and the town's own doors): it keeps
 * the area the player is in and the door's destination in AreaMenu
 * (gMenuWork). AreaMenu_Update asks which way to go only at the two doors
 * that lead on to AREA_ELDER_HALL too (between AREA_TAVERN and
 * AREA_HUB_0x85); any other door goes straight on. AreaMenu_GoTo sets the
 * player's start position in the next area (an entry point by the area the
 * player comes from, or the area's own one from gSpawnPoints), its area
 * and the screen's LOBBY_SCREEN_LEAVE_AREA, which loads it.
 *
 * The file's data are the entry points (SpawnPt).
 */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "area.h"
#include "lobby_text.h"
#include "village_cond.h"
#include "lobby_menu.h"
#include "pad.h"
#include "quest.h"
#include "text.h"
#include "area_menu.h"
#include "game_util.h"
#include "item_shop.h"
#include "menu_steps.h"
#include "my_house.h"
#include "quest_offers.h"
#include "libc.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s16 gHouseCloseTimer;

extern u8 gMenuWork[];

#pragma optimization_level 4
/*
 * Opens the area menu for a door to `destArea`. With a quest taken the door
 * stays shut (LOBBY_NOTICE_QUEST_TAKEN). Unless the house access menu
 * opened it, the player's own member slot is set back to its own player
 * slot ('0' + slot). The menu has no draw callback: AreaMenu_Update shows
 * its question in the lobby's text box. Built at optimization level 4, as
 * the game's code is.
 */
void AreaMenu_Open(u16 destArea)
{
    AreaMenu *menu = (AreaMenu *)gMenuWork;
    u8 slot;

    if (gLobby->questTaken == 1) {
        Menu_PushNoticeOnce(LOBBY_NOTICE_QUEST_TAKEN);
        return;
    }
    if (gTownMode != TOWN_MODE_HOUSE_ACCESS) {
        slot = gScene.localPlayer;
        gLobby->memberSlot[slot] = slot + '0';
    }
    gHouseCloseTimer = 0;
    gTownMode = TOWN_MODE_AREA_MENU;
    gTownMenuOpen = 1;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    memset(&gMenuWork, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    menu->fromArea = gScene.area;
    menu->destArea = destArea;
}

#pragma optimization_level reset

/* Closes the area menu: back to the town (TOWN_MODE_FREE). */
void AreaMenu_Close(void)
{
    gTownMode = TOWN_MODE_FREE;
    gTownMenuOpen = 0;
}

extern LobbyMenu *gLobbyMenu;
void LobbyText_Open(s32 a);
void LobbyDialog_Grow(void);
void Menu_SetYesNo(u8 a); /* unit's own declaration: declared with the u8 the caller passes; with the definition's s8, AreaMenu_Update compiles one instruction differently */
void LobbySe_Play(s32 se);
/*
 * The area menu's frame. A door between AREA_TAVERN and AREA_HUB_0x85 asks
 * which way to go: on to the door's own destination, or to AREA_ELDER_HALL
 * (its question says whether the hall is open yet: COND_HR_GATE_2_HR_31).
 * Circle takes the choice (the closed hall refuses it), cross closes the
 * menu. Any other door goes straight to its destination.
 */
void AreaMenu_Update(void)
{
    AreaMenu *menu = (AreaMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    switch (menu->step) {
    case AREA_MENU_START:
        switch (menu->destArea) {
        case AREA_HUB_0x85:
            if (menu->fromArea == AREA_TAVERN) {
                if (Cond_Check(COND_HR_GATE_2_HR_31) == 0) {
                    LobbyText_Open(LOBBY_TEXT_AREA_PICK_FROM_TAVERN);
                } else {
                    LobbyText_Open(LOBBY_TEXT_AREA_PICK_FROM_TAVERN_HALL);
                }
                LobbySe_Play(LOBBY_SE_CONFIRM);
                LobbyDialog_Grow();
                menu->step++;
                menu->sel = 0;
                menu->area[0] = AREA_HUB_0x85;
                menu->area[1] = AREA_ELDER_HALL;
                break;
            }
            goto other; /* not from AREA_TAVERN: straight on, as the default */
        case AREA_TAVERN:
            if (menu->fromArea == AREA_HUB_0x85) {
                if (Cond_Check(COND_HR_GATE_2_HR_31) == 0) {
                    LobbyText_Open(LOBBY_TEXT_AREA_PICK_FROM_85);
                } else {
                    LobbyText_Open(LOBBY_TEXT_AREA_PICK_FROM_85_HALL);
                }
                LobbySe_Play(LOBBY_SE_CONFIRM);
                LobbyDialog_Grow();
                menu->step++;
                menu->sel = 0;
                menu->area[0] = AREA_TAVERN;
                menu->area[1] = AREA_ELDER_HALL;
                break;
            }
        default:
        other: /* the doors that ask nothing go straight on */
            AreaMenu_GoTo(menu->destArea);
            break;
        }
        break;
    case AREA_MENU_PICK:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            if (menu->sel == 1 && Cond_Check(COND_HR_GATE_2_HR_31) == 0) {
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else {
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                AreaMenu_GoTo(menu->area[menu->sel]);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            AreaMenu_Close();
        } else {
            menu->sel = Menu_WrapUpDown(menu->sel, 2);
            Menu_SetYesNo(menu->sel);
        }
        break;
    }
}

extern SpawnPt gAreaGoSpawn89FromAA;
extern SpawnPt gAreaGoSpawnTavern;
extern SpawnPt gAreaGoSpawn85;
extern SpawnPt gAreaGoSpawn84FromTownArena;
extern SpawnPt gAreaGoSpawn84FromElderHall;
extern SpawnPt gAreaGoSpawn84From85;
extern SpawnPt gAreaGoSpawnElderHallFrom85;
extern SpawnPt gAreaGoSpawnElderHallFromTavern;
extern SpawnPt gAreaGoSpawn84FromHouse;
extern SpawnPt gAreaGoSpawnTownArena;
extern SpawnPt gAreaGoSpawn57;
extern s32 gLobbyStartPosIdx;
extern s8 gLobbyScreenMode;
extern s8 gLobbyScreenStep;
extern s8 gTownSpawnSet;
u8 Village_FacilityLevel(s32 k);
u8 Area_GetSetIndex(u16 id);

/*
 * Sends the local player to `area`. Online, AREA_HUB_0x89 is AREA_HUB_0x84;
 * the house's door leads to the house of the village's house level
 * (VILLAGE_FACILITY_HOUSE, offline or in the player's own house online),
 * and AREA_HUB_0xAA to one of three by the mine's level (VILLAGE_FACILITY_MINE
 * / 3). The entry point depends on the area the player comes from where the
 * area has several doors; otherwise it is the area's own (gSpawnPoints by
 * Area_GetSetIndex). Going back to the village from AREA_HUB_0x57 offline
 * starts the village's screen over (LOBBY_SCREEN_START) instead. Ends with
 * LOBBY_SCREEN_LEAVE_AREA and TOWN_MODE_AREA_CHANGE.
 */
void AreaMenu_GoTo(u16 area)
{
    AreaMenu *menu = (AreaMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    SpawnPt *spawn = NULL;
    u16 setIndex;

    switch (area) {
    case AREA_HUB_0x89:
        if (Game_IsOnline() == 1) {
            area = AREA_HUB_0x84;
            switch (menu->fromArea) {
            case AREA_ONLINE_HOUSE_0:
            case AREA_ONLINE_HOUSE_1:
            case AREA_ONLINE_HOUSE_2:
                spawn = &gAreaGoSpawn84FromHouse;
                break;
            }
        } else {
            switch (menu->fromArea) {
            case AREA_HUB_0xAA:
            case AREA_HUB_0xAB:
            case AREA_HUB_0xAC:
                spawn = &gAreaGoSpawn89FromAA;
                break;
            case AREA_HUB_0x57:
                Save_ClearEventFlag(EVENT_FLAG_TOUR_VILLAGE_RETURN);
                gSave.awayFromVillage = 0;
                gLobbyStartPosIdx = LOBBY_START_POS_AREA;
                QuestList_Rebuild();
                Game_ResetSubsystems();
                gLobbyScreenMode = LOBBY_SCREEN_START;
                gLobbyScreenStep = 0;
                return;
            }
        }
        break;
    case AREA_HUB_0x57:
        gSave.awayFromVillage = 1;
        spawn = &gAreaGoSpawn57;
        if (menu->fromArea == AREA_HUB_0x89) {
            gLobbyStartPosIdx = LOBBY_START_POS_AREA;
            QuestList_Rebuild();
        }
        break;
    case AREA_TAVERN:
        if (menu->fromArea == AREA_HUB_0x85 || menu->fromArea == AREA_ELDER_HALL) {
            spawn = &gAreaGoSpawnTavern;
        }
        break;
    case AREA_TOWN_ARENA:
        if (pl->pos[2] < 3500.0f) {
            spawn = &gAreaGoSpawnTownArena;
        }
        break;
    case AREA_ELDER_HALL:
        switch (menu->fromArea) {
        case AREA_HUB_0x85:
            spawn = &gAreaGoSpawnElderHallFrom85;
            break;
        case AREA_TAVERN:
            spawn = &gAreaGoSpawnElderHallFromTavern;
            break;
        }
        break;
    case AREA_ONLINE_HOUSE_0:
        if (Game_IsOnline() == 0 || MyHouse_IsOwner() == 1) {
            area = (u8)Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) + AREA_ONLINE_HOUSE_0;
        }
        break;
    case AREA_HUB_0xAA:
        area = (u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE) / 3 + AREA_HUB_0xAA;
        break;
    case AREA_HUB_0x85:
        if (menu->fromArea != AREA_HUB_0x84) {
            spawn = &gAreaGoSpawn85;
        }
        break;
    case AREA_HUB_0x84:
        switch (menu->fromArea) {
        case AREA_ELDER_HALL:
            spawn = &gAreaGoSpawn84FromElderHall;
            break;
        case AREA_TOWN_ARENA:
            spawn = &gAreaGoSpawn84FromTownArena;
            break;
        case AREA_HUB_0x85:
            spawn = &gAreaGoSpawn84From85;
            break;
        }
        break;
    }
    setIndex = Area_GetSetIndex(area);
    if (spawn == NULL) {
        spawn = &gSpawnPoints[setIndex];
    }
    pl->startPos[0] = spawn->pos.x;
    pl->startPos[1] = spawn->pos.y;
    pl->startPos[2] = spawn->pos.z;
    pl->startAngle = spawn->angle + ANGLE_TURN / 4;
    Player_AddSpawnOffset(pl, pl->startPos, pl->startAngle);
    gLobbyScreenMode = LOBBY_SCREEN_LEAVE_AREA;
    gTownSpawnSet = 1;
    pl->nextArea = area;
    gTownMode = TOWN_MODE_AREA_CHANGE;
}


/* The entry points: gSpawnPoints by the area's set index (Area_GetSetIndex),
 * then the doors' own (gAreaGoSpawn*: AREA_HUB_0x84 is entered from
 * AREA_HUB_0x85, AREA_TOWN_ARENA, AREA_ELDER_HALL and the online house,
 * AREA_ELDER_HALL from AREA_HUB_0x85 and AREA_TAVERN). No code reads
 * gAreaGoSpawnUnused or gAreaGoUnusedData. */
SpawnPt gSpawnPoints[15] = {
#include "lobby/007EBE10.inc"
};
SpawnPt gAreaGoSpawn89FromAA = {
#include "lobby/007EBF00.inc"
};
SpawnPt gAreaGoSpawnUnused = {
#include "lobby/007EBF10.inc"
};
SpawnPt gAreaGoSpawnTavern = {
#include "lobby/007EBF20.inc"
};
SpawnPt gAreaGoSpawn85 = {
#include "lobby/007EBF30.inc"
};
SpawnPt gAreaGoSpawn84FromTownArena = {
#include "lobby/007EBF40.inc"
};
SpawnPt gAreaGoSpawn84FromElderHall = {
#include "lobby/007EBF50.inc"
};
SpawnPt gAreaGoSpawn84From85 = {
#include "lobby/007EBF60.inc"
};
SpawnPt gAreaGoSpawnElderHallFrom85 = {
#include "lobby/007EBF70.inc"
};
SpawnPt gAreaGoSpawnElderHallFromTavern = {
#include "lobby/007EBF80.inc"
};
SpawnPt gAreaGoSpawn84FromHouse = {
#include "lobby/007EBF90.inc"
};
SpawnPt gAreaGoSpawnTownArena = {
#include "lobby/007EBFA0.inc"
};
SpawnPt gAreaGoSpawn57 = {
#include "lobby/007EBFB0.inc"
};
f32 gAreaGoUnusedData[12] = {
#include "lobby/007EBFC0.inc"
};
