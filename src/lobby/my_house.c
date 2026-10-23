/*
 * The house queries: whether the local player owns the house it is in
 * (MyHouse_IsOwner; always offline), whether it is in its own house
 * (MyHouse_InOwnHouse), its house slot, from its member slot
 * (MyHouse_LocalSlot), and whether an area is one of the online house's
 * areas (Area_IsOnlineHouse). The town's players and townspeople, its
 * menus, the records and the talks ask them.
 *
 * Built at optimization level 4, as the town players' code around it.
 */

#include "common.h"
#include "lobby_menu.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "area.h"
#include "my_house.h"
#include "game_util.h"

#pragma optimization_level 4

/*
 * Returns 1 when the local player owns the house it is in (always
 * offline).
 */
s32 MyHouse_IsOwner(void)
{
    s8 slot;

    if (Game_IsOnline() == 0) {
        return 1;
    }
    slot = MyHouse_LocalSlot();
    return slot == gScene.localPlayer;
}

/*
 * Returns 1 when the local player is in its own house: the house area
 * offline, or one of the online house areas as its owner.
 */
s32 MyHouse_InOwnHouse(void)
{
    switch (gScene.area) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        break;
    case AREA_HOUSE:
        if (Game_IsOnline() == 0) {
            return 1;
        }
        break;
    default:
        return 0;
    }
    if (Game_IsOnline() == 0) {
        return 1;
    }
    return MyHouse_IsOwner() != 0;
}

/*
 * Returns the house slot the local player is in (0-9, from its member
 * slot, gLobby->memberSlot's '0'-'9'), or -1 outside the house areas
 * (AREA_HOUSE, AREA_ONLINE_HOUSE_0 to _2). memberSlot's address is written
 * as an integer sum and the test as `0 <= slot`: the game's code adds the
 * index first and compares that way.
 */
s8 MyHouse_LocalSlot(void)
{
    s32 i = gScene.localPlayer;
    u16 area = gPlayers[i].area;
    s8 slot;

    switch (area) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
    case AREA_HOUSE:
        break;
    default:
        return -1;
    }
    if (*(u8 *)(i + (s32)gLobby + 5432) == 0) {
        return -1;
    }
    slot = *(u8 *)(i + (s32)gLobby + 5432) - '0';
    if (0 <= slot && slot < 10) {
        return slot;
    }
    return -1;
}

/*
 * Returns 1 when area is one of the online house's areas (one per house
 * level), else 0.
 */
s32 Area_IsOnlineHouse(s32 area)
{
    switch (area & 0xFFFF) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        return 1;
    }
    return 0;
}
