/*
 * Where the placed items may go: the rules of the barrel bombs, the traps and the placed bomb
 * (Item_CanPlace*, Item_CanUseSmallBarrelBomb, Scene_CanPlaceKind2), with two helpers of the
 * traps and arrows (ObjAttach_RollBatBack, Em_TrapReach). The file ends with four functions
 * that are empty or constant in this build: the debug switches (Debug_IsOn, Debug_SwitchStub,
 * Debug_SwitchStubAlt) and the monster kind of a quest without one (Quest_DefaultEmKind).
 */
#include "common.h"
#include "sprite.h"
#include "chat_log.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "syssettings.h"
#include "objhead.h"
#include "objattach.h"
#include "obj.h"
#include "window.h"
#include "render_state.h"
#include "area.h"
#include "hud.h"
#include "adx.h"
#include "sound.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "lobby_state.h"
#include "area_draw.h"
#include "kbd.h"
#include "save.h"
#include "text.h"
#include "village_cond.h"
#include "place_rules.h"

/* Whether the monster bomb can be set while riding: always 1. */
s32 Item_RideItemUsable(void)
{
    return 1;
}

/* 1 when a large barrel bomb can be placed: not in the camp's area nor where
 * barrel bombs are barred, and fewer than 2 of the large bombs placed
 * (gScene.placed1). */
s32 Item_CanPlaceLargeBarrelBomb(void)
{
    if (gScene.area == gScene.campArea) {
        return 0;
    }
    if (Area_NoBarrelBombs(gScene.area) == 1) {
        return 0;
    }
    if (gScene.placed1 < 2) {
        return 1;
    }
    return 0;
}

/* 1 unless the area bars barrel bombs. */
s32 Item_CanUseSmallBarrelBomb(void)
{
    return Area_NoBarrelBombs(gScene.area) != 1;
}

/* 1 when a trap can be set: none of the local player's is set
 * (gScene.placed2), and the player is not on the high ground (above 100)
 * of AREA_SNOWY_MOUNTAINS_3. */
s32 Scene_CanPlaceKind2(void)
{
    if (gScene.placed2 <= 0) {
        if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_3 && gPlayers[gScene.localPlayer].pos[1] > 100.0f) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 1 when a trap can be set (Scene_CanPlaceKind2), and not in the camp's area. */
s32 Item_CanPlaceKind2(void)
{
    if (gScene.area == gScene.campArea) {
        return 0;
    }
    return Scene_CanPlaceKind2();
}

/* 1 in an area that bars barrel bombs. */
s32 Scene_InNoBarrelBombArea(void)
{
    return Area_NoBarrelBombs(gScene.area) == 1;
}

/* 1 when ITEM_ID_PLACED_BOMB can be placed: not in the camp's area, and
 * fewer than 2 of the large bombs placed (gScene.placed1). */
s32 Item_CanPlaceKind1(void)
{
    if (gScene.area == gScene.campArea) {
        return 0;
    }
    if (gScene.placed1 < 2) {
        return 1;
    }
    return 0;
}

/*
 * Whether the monster bats back the deflected arrow att, by the random
 * number roll: never when an aura deflected it (ATTACH_DEFLECTED_AURA),
 * else 40 times in 100.
 */
s32 ObjAttach_RollBatBack(ObjAttach *att, u16 roll)
{
    if (att->state == ATTACH_DEFLECTED_AURA) {
        return 0;
    }
    if (roll % 100 < 40) {
        return 1;
    }
    return 0;
}

/* A trap's reach for a monster of kind kind: reach, or a quarter more for
 * the largest kinds. */
f32 Em_TrapReach(u16 kind, f32 reach)
{
    switch (kind) {
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_RAJANG:
        return 1.25f * reach;
    }
    return reach;
}

/* A debug switch (ids 23 to 174 are tested): always off in this build. */
s32 Debug_IsOn(s32 unused0)
{
    return 0;
}

/* The monster kind a quest without one would take: always 0 in this build. */
s32 Quest_DefaultEmKind(void)
{
    return 0;
}

/* Empty in this build. The scene's set-up gives it the debug switch ids
 * 100 to 104 in every area but area 13. */
void Debug_SwitchStub(s32 unused0)
{
}

/* Empty in this build. The scene's set-up gives it the debug switch ids
 * 100 to 102 in area 13. */
void Debug_SwitchStubAlt(s32 unused0)
{
}

