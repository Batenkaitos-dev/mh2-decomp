#ifndef GM_LOAD_H
#define GM_LOAD_H

#include "common.h"
#include "em.h"

/*
 * The monsters' data files in gm_sub's scenes: the stage's emcmd partition
 * is read (GmLoad_LoadStagePartition), each monster kind's data file is
 * loaded and its pointers relocated in place (GmLoad_LoadEmData), and
 * GmLoad_GetEmData returns it. Also the sounds and effects played at a
 * monster's motion frames (GmEm_UpdateSfx, GmEm_MotionEffects) and the
 * small callbacks the per-kind monster tables point to.
 */

void GmEm_MotionEffects(Em *em, u8 *state);
void GmEm_UpdateSfx(Em *em);
void GmEm_RunMotionFx(Em *em);
void GmEm_SpawnShadow(Em *em);
void GmEm_Nop(void);
s32 GmEm_ReturnZero(void);
void GmLoad_Reset(void);
void GmLoad_LoadStagePartition(void);
void GmLoad_LoadEmData(s32 kind, s32 idx);
void *GmLoad_GetEmData(s32 i);

#endif
