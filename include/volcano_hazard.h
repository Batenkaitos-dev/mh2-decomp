#ifndef VOLCANO_HAZARD_H
#define VOLCANO_HAZARD_H

#include "common.h"
#include "objattach.h"

/*
 * VolcanoHazard, a kind of ObjAttach with no owner: an attack that an area
 * object (AreaObj_K19_Run) starts at one of the area's points, lasting a
 * number of frames and hitting players only. Here are its creator and its
 * state handlers.
 */

void VolcanoHazard_Create(f32 *pos, u8 attackType, u8 life);
void VolcanoHazard_Update(ObjAttach *o);
void VolcanoHazard_Setup(ObjAttach *o);
void VolcanoHazard_Run(ObjAttach *o);
void VolcanoHazard_End(ObjAttach *o);
void VolcanoHazard_Free(ObjAttach *o);

#endif
