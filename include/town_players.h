#ifndef TOWN_PLAYERS_H
#define TOWN_PLAYERS_H

#include "common.h"
#include "quest.h"
#include "em.h"
#include "objhead.h"
#include "player.h"

/*
 * The lobby's town players and the actor core they share with the
 * townspeople: the motion starts (Actor_SetMotion and its forms), the state
 * changes, the posture and the flags (Actor_*), the pad input, the move
 * kind and the turning (TownPc_*), and the area's action spots
 * (ActSpot_GetList, ActSpot_IsUsable).
 */

void Actor_SetMotion(void *obj, s32 motion, s32 blend, s32 startFrame);
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 blend, s32 startFrame, s32 layer);
void Actor_SetMotionLayer(ObjHead *obj, s32 motion, s32 blend, s32 startFrame, s32 layer);
void TownPc_UpdateInput(void);
void TownPc_ReadPad(s32 playerNo);
void Actor_StartMotions(Player *pl);
void Actor_TickTimers(Player *obj);
void Actor_SetStateNow(Player *pl, u8 group, u8 newState, u16 flags);
void Actor_SetStateFlags(Player *pl, u8 group, u8 newState, u16 flags);
s32 Actor_NeedsStateSend(void *obj);
void Actor_ReturnToIdle(void *obj, s32 posture, s32 blend, s32 startFrame);
void Actor_ClearStateWork(void *obj);
void Actor_StopMoving(void *obj);
void Actor_StartIdle(void *obj, s32 blend, s32 startFrame);
s16 Actor_IsState(void *obj, s32 group, s32 state);
u8 TownPc_MoveKind(Player *pl);
u16 TownPc_InputYaw(Player *pl, s32 stick);
void Actor_SetPosture(void *obj, u16 posture, s32 setFlag1, s32 setFlag2);
void TownPc_StartMoveTo(Player *pl, s16 frames);
void TownPc_StepMoveTo(Player *pl);
void TownPc_SmoothRemoteYaw(Player *pl);
void Actor_ClearFlag(void *obj, u32 flag);
void Actor_SetFlag(void *obj, u32 flag);
u8 *ActSpot_GetList(u16 area);
void TownPc_FindSpot(Player *pl);
s32 ActSpot_IsUsable(Player *pl, ActionSpot *spot);
s32 TownPc_IsInMyArea(Player *pl);
void Actor_SetState(void *obj, s32 group, s32 state);

#endif
