#ifndef TOWN_AREA_CHANGE_H
#define TOWN_AREA_CHANGE_H

#include "common.h"
#include "em.h"

/*
 * The town's area changes: the townspeople of a new area (Town_SpawnNpcs) and
 * its music (Town_StartAreaMusic), the townspeople's move between two areas
 * that are one place (Town_SetNpcArea), and the area's end (Town_FreeArea).
 * With them, two angle tests of the actors (Actor_IsFacingWithin,
 * Actor_HeadingDiff) and a townsperson's map position (Npc_UpdateAreaPos).
 */

void Town_StartAreaMusic(s32 enterMode);
void Town_SetNpcArea(u16 area);
void Town_SpawnNpcs(u16 area);
void Town_FreeArea(void);
s32 Actor_IsFacingWithin(void *obj, f32 *target, s32 deg);
s32 Actor_HeadingDiff(void *obj, void *pos);
void Npc_UpdateAreaPos(Em *npc);

#endif
