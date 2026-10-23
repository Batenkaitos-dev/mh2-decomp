#ifndef FIELD_OBJECTS_H
#define FIELD_OBJECTS_H

#include "common.h"
#include "sprite.h"
#include "obj.h"

/*
 * Two small objects of the field and the town: the season banner
 * (SeasonBanner_Create: the season's name over its icon, faded in, slid
 * apart and faded out) and the object a character of the house tosses onto a
 * spot of the area (ThrownObj_Spawn, ThrownObj_SpawnForHairMenu). The other
 * functions are the two objects' tasks, states and draws.
 */

/* The flight of a thrown object (its Obj.buf). */
typedef struct ThrownFlight {
    f32 vel[3];                 /* 0x00: per frame; gravity takes 2 off vel[1] each frame */
    s16 turn;                   /* 0x0C: angle per frame (ANGLE_TURN to a turn) */
} ThrownFlight;

void SeasonBanner_Create(void);
void SeasonBanner_Update(Obj *banner);
void SeasonBanner_Init(Obj *banner);
void SeasonBanner_Run(Obj *banner);
void SeasonBanner_End(Obj *banner);
void SeasonBanner_Free(Obj *banner);
void SeasonBanner_Draw(DrawSlot *slot);
void ThrownObj_Spawn(void *holder, s32 spot, s32 step);
void ThrownObj_SpawnForHairMenu(void *holder, s32 spot);
void ThrownObj_HoldInHands(Obj *obj, void *holder);
void ThrownObj_SpotPos(Obj *obj, f32 *out);
void ThrownObj_SpotAngle(Obj *obj, s16 *angle);
s16 ThrownObj_AreaModel(Obj *obj);
void ThrownObj_Launch(Obj *obj, ThrownFlight *out);
void ThrownObj_Task(Obj *obj);
void ThrownObj_Start(Obj *obj);
void ThrownObj_Run(Obj *obj);
void ThrownObj_End(Obj *obj);
void ThrownObj_Free(Obj *obj);
void ThrownObj_Draw(DrawSlot *slot);

#endif
