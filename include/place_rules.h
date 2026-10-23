#ifndef PLACE_RULES_H
#define PLACE_RULES_H

#include "common.h"
#include "objattach.h"

/*
 * Where the placed items may go: the rules of the barrel bombs, the traps
 * and the placed bomb (Item_CanPlace*, Item_CanUseSmallBarrelBomb,
 * Item_RideItemUsable, Scene_CanPlaceKind2, Scene_InNoBarrelBombArea), and
 * two helpers of the traps and arrows (Em_TrapReach, ObjAttach_RollBatBack).
 * Also four functions that are empty or constant in this build: the debug
 * switches (Debug_IsOn, Debug_SwitchStub, Debug_SwitchStubAlt) and
 * Quest_DefaultEmKind.
 */

s32 Item_RideItemUsable(void);
s32 Item_CanPlaceLargeBarrelBomb(void);
s32 Item_CanUseSmallBarrelBomb(void);
s32 Scene_CanPlaceKind2(void);
s32 Item_CanPlaceKind2(void);
s32 Scene_InNoBarrelBombArea(void);
s32 Item_CanPlaceKind1(void);
s32 ObjAttach_RollBatBack(ObjAttach *att, u16 roll);
f32 Em_TrapReach(u16 kind, f32 reach);
s32 Debug_IsOn(s32 unused0);
s32 Quest_DefaultEmKind(void);
void Debug_SwitchStub(s32 unused0);
void Debug_SwitchStubAlt(s32 unused0);

#endif
