/*
 * The town's area changes.
 *
 * - Entering an area (Town_EnterArea, which LobbyScreen_LoadArea and the
 *   area moves call): the area's file (Town_LoadAreaFile), the common
 *   motions, the creatures of the area's set (gTownAreaSets, AreaModelSlot),
 *   the area's objects and props (Town_AreaEffects), the local player's
 *   place, and the messages that tell the other members. Then the
 *   townspeople (Town_SpawnNpcs, from the set's NpcAreaEntry table) and the
 *   music (Town_StartAreaMusic, the town's season and time tunes).
 *   AREA_HUB_0x57 and AREA_HOUSE are two halves of one place: moving
 *   between them keeps everything loaded (Town_SetNpcArea moves the
 *   townspeople across), and Town_FreeArea frees nothing.
 * - Two angle tests of the actors' shared header (Actor_IsFacingWithin,
 *   Actor_HeadingDiff) and a townsperson's map position.
 */
#include "common.h"
#include "quest.h"
#include "syssettings.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "objhead.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_npc.h"
#include "area.h"
#include "lobby_menu.h"
#include "bgm.h"
#include "obj.h"
#include "camera.h"
#include "area_draw.h"
#include "fishing.h"
#include "light.h"
#include "lobby_net.h"
#include "text.h"
#include "village_cond.h"
#include "town_area_change.h"
#include "draw_slots.h"
#include "file_load.h"
#include "game_util.h"
#include "house_props.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "mem_util.h"
#include "my_house.h"
#include "stage_data.h"
#include "town_members.h"
#include "village_records.h"
#include "libc.h"
#include "vec_math.h"

/* Declared as the callers use it, with s32 parameters:
 * unit's own declaration: with the definition's two u8, the calls compile differently. */
void SeasonBanner_CreateOnline(s32, s32);
extern s32 gTownMode;
u8 Area_GetSetIndex(u16 id);
extern s32 gAreaFiles[];          /* each area set's file (LoadCompressedFile), -1 for none */

/*
 * The area's own things on entry: in AREA_HUB_0xAC and AREA_HUB_0x57 the
 * prop of the bed's sleep event (AreaProp_Spawn) until that event is seen;
 * in each online house, the first time its owner enters, the house's prop
 * (HouseProp_Spawn), once. Then the area's effects (Effect17_SpawnArea).
 */
void Town_AreaEffects(u16 area)
{
    switch (area) {
    case AREA_HUB_0xAC:
        if (Save_TestEventFlag(EVENT_FLAG_MINE_STRONG_PICK) == 0) {
            AreaProp_Spawn(0);
        }
        break;
    case AREA_HUB_0x57:
        if (Save_TestEventFlag(EVENT_FLAG_HOUSE_SLEEP_EVENT) == 0) {
            AreaProp_Spawn(0);
        }
        break;
    case AREA_ONLINE_HOUSE_1:
        if ((MyHouse_IsOwner() == 1) && (Save_TestEventFlag(EVENT_FLAG_ONLINE_HOUSE_1_ENTERED) == 0)) {
            HouseProp_Spawn();
            Save_SetEventFlag(EVENT_FLAG_ONLINE_HOUSE_1_ENTERED);
        }
        break;
    case AREA_ONLINE_HOUSE_2:
        if ((MyHouse_IsOwner() == 1) && (Save_TestEventFlag(EVENT_FLAG_ONLINE_HOUSE_2_ENTERED) == 0)) {
            HouseProp_Spawn();
            Save_SetEventFlag(EVENT_FLAG_ONLINE_HOUSE_2_ENTERED);
        }
        break;
    }
    Effect17_SpawnArea(area);
}

/*
 * Starts the music of the area the player has just entered; enterMode is the
 * lobby screen's entry (enum LobbyEnterMode).
 * - AREA_TOWN_ARENA plays its own music.
 * - Elsewhere, unless the elder raid is on, a town tune may play first:
 *   offline, the season's tune always while gSysSettings.offline is 0, else
 *   only when the season (gLobbyLastSeason) or the time of day
 *   (gLobbyLastPm) changed since they were last played;
 *   LOBBY_ENTER_VILLAGE plays the season's tune; LOBBY_ENTER_TOWN_MEMBERS
 *   plays the season's or the time's tune when it changed.
 * - With no tune, the area's music (Bgm_PlayAreaMusic).
 * In TOWN_MODE_FREE the season's banner (SeasonBanner_CreateOnline) shows
 * too.
 */
void Town_StartAreaMusic(s32 enterMode)
{
    s32 arenaSeason;
    s32 bannerSeason;
    s32 isPm;
    s32 season;
    s32 tunePlayed;

    tunePlayed = 0;
    Bgm_ForgetCurrent();
    if (gScene.area == AREA_TOWN_ARENA) {
        if (gTownMode == TOWN_MODE_FREE) {
            arenaSeason = LobbyWorld_GetSeason() & 0xFF;
            SeasonBanner_CreateOnline(arenaSeason, LobbyTime_IsPm() & 0xFF);
        }
        Bgm_PlayAreaMusic(gScene.area);
        return;
    }
    if (ElderRaid_IsOn() == 0) {
        season = LobbyWorld_GetSeason();
        isPm = LobbyTime_IsPm();
        if (Game_IsOnline() == 0) {
            if (gSysSettings.offline == 0) {
                Bgm_PlayTownTune(TOWN_TUNE_SEASON, season);
                tunePlayed = 1;
            } else if (season != gLobbyLastSeason) {
                Bgm_PlayTownTune(TOWN_TUNE_SEASON, season);
                tunePlayed = 1;
            } else if (isPm != gLobbyLastPm) {
                Bgm_PlayTownTune(TOWN_TUNE_TIME, isPm);
                tunePlayed = 1;
            }
        } else if (enterMode == LOBBY_ENTER_VILLAGE) {
            Bgm_PlayTownTune(TOWN_TUNE_SEASON, season);
            tunePlayed = 1;
        } else if (enterMode == LOBBY_ENTER_TOWN_MEMBERS) {
            if (season != gLobbyLastSeason) {
                Bgm_PlayTownTune(TOWN_TUNE_SEASON, season);
                tunePlayed = 1;
            } else if (isPm != gLobbyLastPm) {
                Bgm_PlayTownTune(TOWN_TUNE_TIME, isPm);
                tunePlayed = 1;
            }
        }
    }
    if (tunePlayed == 0) {
        Bgm_PlayAreaMusic(gScene.area);
    }
    if (gTownMode == TOWN_MODE_FREE) {
        bannerSeason = LobbyWorld_GetSeason() & 0xFF;
        SeasonBanner_CreateOnline(bannerSeason, LobbyTime_IsPm() & 0xFF);
    }
}

/*
 * Loads the file of area's set (gAreaFiles, by Area_GetSetIndex) into the
 * guild's free memory (gLbguildFree); a set with no file has -1. Area 0
 * loads nothing.
 */
void Town_LoadAreaFile(u16 area)
{
    s32 file;

    if (area != 0) {
        file = gAreaFiles[Area_GetSetIndex(area)];
        if (file != -1) {
            LoadCompressedFile(gLbguildFree, file);
        }
    }
}

extern NetUser gNetUsers[];
extern s32 gTownMenuOpen;
void Town_SetDrawSky(s8 arg0);
/* Declared as the callers use it, with a u16 area:
 * unit's own declaration: with the definition's s32 area, Town_EnterArea compiles differently. */
void Town_PlaceLocalPlayer(u16 area);

/*
 * Enters area: the local player's area, then the set-up of the area unless
 * it is the other half of the same place (AREA_HUB_0x57 and AREA_HOUSE,
 * either way), where nothing is loaded again:
 * - the area's file (not in the gallery's play), the players' common models
 *   and the common motions (Anim_LoadCommon: set 2 online; offline set 3 in
 *   AREA_HUB_0x57 and AREA_HOUSE, else set 1), once;
 * - the creatures of the area's set (gTownAreaSets): AREA_MODEL_POOGIE's
 *   slot takes the Poogie of the house's owner in its outfit online
 *   (POOGIE_MODEL_OUTFIT_0 + the outfit, or the first outfit with no name),
 *   POOGIE_MODEL_OFFLINE offline;
 * - the area's objects, the players (TownPc_InitAll(newDrawSlots)), the
 *   lights, the fog and the area's props and effects.
 * The mode drops to TOWN_MODE_FREE unless a scene runs, the local player is
 * placed, online its place and state are sent, the other players' models
 * start loading where the area shows them, and the camera starts.
 * Returns 1 for the other half of the same place, 0 after a full set-up.
 */
u8 Town_EnterArea(u16 area, s32 newDrawSlots)
{
    Player *localPl = &gPlayers[gScene.localPlayer];
    AreaModelSlot *slot;
    u8 samePlace = 0;
    s8 i;
    s16 setIndex;
    u8 outfit;
    Player *pl;

    if (gScene.area == AREA_HUB_0x57) {
        if (gPlayers[gScene.localPlayer].nextArea == AREA_HOUSE) {
            samePlace = 1;
        }
    } else if (gScene.area == AREA_HOUSE && gPlayers[gScene.localPlayer].nextArea == AREA_HUB_0x57) {
        samePlace = 1;
    }
    localPl->area = area;
    if (samePlace == 0) {
        if (gSysSettings.galleryPlay == 0) {
            Town_LoadAreaFile(area);
        }
        if (gLobby->modelLoaded[localPl->index] == 0) {
            TownPc_LoadAllModels();
        }
        if (gLobby->areaLoaded == 0) {
            gLobby->areaLoaded = 1;
            if (Game_IsOnline() == 1) {
                Anim_LoadCommon(2);
            } else {
                switch (area) {
                case AREA_HUB_0x57:
                case AREA_HOUSE:
                    Anim_LoadCommon(3);
                    break;
                default:
                    Anim_LoadCommon(1);
                    break;
                }
            }
        }
        Mem_Compact();
        setIndex = Area_GetSetIndex(area);
        slot = gTownAreaSets[setIndex];
        for (i = 0; i < AREA_MODEL_SLOTS; i++, slot++) {
            if (slot->kind == AREA_MODEL_POOGIE) {
                if (Game_IsOnline() == 1) {
                    outfit = ((HunterCard *)gNetUsers[MyHouse_LocalSlot()].text)->poogieOutfit;
                    if (outfit != POOGIE_OUTFIT_NONE) {
                        EmSlot_LoadModel(outfit + POOGIE_MODEL_OUTFIT_0, 0, i);
                    } else {
                        EmSlot_LoadModel(POOGIE_MODEL_OUTFIT_0, 0, i);
                    }
                } else {
                    EmSlot_LoadModel(POOGIE_MODEL_OFFLINE, 0, i);
                }
            } else if (slot->kind != AREA_MODEL_NONE) {
                EmSlot_LoadModel(slot->kind, slot->variant, i);
            }
        }
    }
    gTownMenuOpen = 0;
    Area_Enter(area);
    if (samePlace == 0) {
        Town_SetDrawSky(1);
    }
    Camera_SetPreset(0);
    if (samePlace == 0) {
        Area_Load(area);
        Lobby_ReceiveWhileWaiting();
        TownPc_InitAll(newDrawSlots);
    }
    Light_InitAll(area);
    gCameraPreset = CAM_PRESET_NONE;
    Area_ApplyFog(area);
    Area_StartObjects(area);
    if (gTownMode != TOWN_MODE_SCENE) {
        gTownMode = TOWN_MODE_FREE;
        localPl->areaMoveState = PL_AREA_MOVE_NONE;
    }
    Town_AreaEffects(area);
    Town_PlaceLocalPlayer(area);
    gScene.area = area;
    if (Game_IsOnline() == 1) {
        TownMsg_SendPlaceOrigin();
        TownMsg_SendArea(gScene.area);
        TownMsg_SendPlace(localPl);
        TownMsg_SendFacing(localPl);
        TownMsg_SendStateNow(localPl);
    }
    if (Area_HasTownPcs(gScene.area) == 1) {
        for (i = 0, pl = gPlayers; i < LOBBY_MEMBER_COUNT; i++, pl++) {
            if (pl->active != 0 && (s8)gLobby->pcLoaded[i] == 0) {
                TownPc_StartLoad(pl);
            }
        }
    }
    CamCtl_Init();
    Players_UpdateMatrices();
    CamCtl_Update(0);
    return samePlace;
}

/* Moves every active Em (the townspeople) to area, for the other half of
 * the same place, where they are kept. */
void Town_SetNpcArea(u16 area)
{
    s32 i;
    Em *em = gEms;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0) {
            em->area = area;
        }
    }
}

extern NpcAreaEntry *gAreaNpcTables[];
/* Declared as the callers use it, with a u16 id:
 * unit's own declaration: with the definition's s32 id, Town_SpawnNpcs compiles differently. */
s32 AreaNpc_IsPresent(s32 idx, u16 id);

/*
 * Spawns the townspeople of area: each entry of its set's table
 * (gAreaNpcTables) present now (AreaNpc_IsPresent) gets an Em slot, run by
 * the lobby (ownTask), and is set up from the entry (Npc_SetupFromArea).
 * Offline in a house, the first of AREA_HUB_0x89's townspeople that has a
 * meal visit due (Village_HasMealVisit) comes too. Clears gNpcScene.
 */
void Town_SpawnNpcs(u16 area)
{
    u8 n;
    s32 i;
    s32 j;
    Em *npc;
    NpcAreaEntry *entry;
    u8 setIndex;

    n = gAreaNpcCounts[(u8)Area_GetSetIndex(area)];
    for (i = 0; i < n; i++) {
        if (AreaNpc_IsPresent(i, area) == 1) {
            npc = EmSlot_Alloc();
            if (npc != NULL) {
                npc->ownTask = 1;
                npc->modelSlot = 0;
                npc->index = i;
                npc->area = gScene.area;
                Npc_SetupFromArea(npc, 0, gScene.area);
            }
        }
    }
    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1) {
        setIndex = Area_GetSetIndex(AREA_HUB_0x89);
        entry = gAreaNpcTables[setIndex];
        if (entry == NULL) {
            return;
        }
        n = gAreaNpcCounts[setIndex];
        for (j = 0; j < n; j++, entry++) {
            if (Village_HasMealVisit(entry->id) == 1) {
                npc = EmSlot_Alloc();
                if (npc != NULL) {
                    npc->ownTask = 1;
                    npc->modelSlot = 0;
                    npc->index = j;
                    npc->area = gScene.area;
                    Npc_SetupFromArea(npc, 0, AREA_HUB_0x89);
                }
                break;
            }
        }
    }
    memset(&gNpcScene, 0, sizeof(gNpcScene));
}

/*
 * Frees the area the local player leaves: its Em slots, area data, objects,
 * free lists and draw slots, and its spot. Moving between the two halves of
 * the same place (AREA_HUB_0x57 and AREA_HOUSE) frees nothing; moving
 * between AREA_HUB_0x57 and AREA_HUB_0x89 also frees the common motions,
 * which the next area loads again. The notices' queue is reset where the
 * area shows the players.
 */
void Town_FreeArea(void)
{
    if (Area_HasTownPcs(gScene.area) == 1) {
        Notice_ResetQueue();
    }
    if (gScene.area == AREA_HUB_0x57) {
        switch (gPlayers[gScene.localPlayer].nextArea) {
        case AREA_HOUSE:
            return;
        case AREA_HUB_0x89:
            Motion_FreeCommon();
            gLobby->areaLoaded = 0;
            break;
        }
    } else if (gScene.area == AREA_HOUSE) {
        if (gPlayers[gScene.localPlayer].nextArea == AREA_HUB_0x57) {
            return;
        }
    } else if (gScene.area == AREA_HUB_0x89) {
        if (gPlayers[gScene.localPlayer].nextArea == AREA_HUB_0x57) {
            Motion_FreeCommon();
            gLobby->areaLoaded = 0;
        }
    }
    EmSlot_FreeAll();
    Area_FreeAll();
    Obj_FreeAll();
    FreeList_InitAll();
    DrawSlot_ResetAll();
    Mem_Compact();
    gPlayers[gScene.localPlayer].spot = NULL;
}

/*
 * 1 when target lies within deg degrees either side of the facing of obj
 * (a Player or an Em: their shared header), else 0. The heading is a
 * 16-bit angle; 65536.0f / 360 turns degrees into it.
 */
s32 Actor_IsFacingWithin(void *obj, f32 *target, s32 deg)
{
    s32 diff;
    s32 range;

    diff = (((((ObjHead *)obj)->rot[1] - (((Vec_Heading(((ObjHead *)obj)->pos, target) & 0xFFFF) + ANGLE_TURN / 4) & 0xFFFF)) & 0xFFFF) - ANGLE_TURN / 2) & 0xFFFF;
    range = (s32) (0.5f + ((65536.0f * (f32) deg) / 360.0f)) & 0xFFFF;
    if (diff > 0xFFFF - range || diff < range) {
        return 1;
    }
    return 0;
}

/* The heading from an object (a Player, an Em or an NPC: their shared header) to pos, less its
 * yaw, as a u16 angle. */
s32 Actor_HeadingDiff(void *obj, void *pos)
{
    return ((((Vec_Heading((f32 *) pos, ((ObjHead *)obj)->pos) & 0xFFFF) + ANGLE_TURN / 4) & 0xFFFF) - ((ObjHead *)obj)->rot[1]) & 0xFFFF;
}

/* Sets the townsperson's map position: its position in its area plus the
 * area's origin, and the area's middle height. */
void Npc_UpdateAreaPos(Em *npc)
{
    AreaRecord *rec;

    rec = Stage_GetAreaRecord(npc->area);
    npc->mapPos[0] = npc->posX + rec->originX;
    npc->mapPos[1] = npc->posY + (rec->bottom + rec->top) / 2.0f;
    npc->mapPos[2] = npc->posZ + rec->originZ;
}

