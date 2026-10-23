#ifndef MY_HOUSE_H
#define MY_HOUSE_H

#include "common.h"

/*
 * The house queries: whether the local player owns the house it is in,
 * whether it is in its own house, its house slot from its member slot, and
 * whether an area is one of the online house's areas.
 */

s32 MyHouse_IsOwner(void);
s32 MyHouse_InOwnHouse(void);
s8 MyHouse_LocalSlot(void);
s32 Area_IsOnlineHouse(s32 area);

#endif
