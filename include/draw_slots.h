#ifndef DRAW_SLOTS_H
#define DRAW_SLOTS_H

#include "common.h"
#include "sprite.h"
#include "model.h"

/*
 * The draw slots (DrawSlot) and the ordering tables they go into. A slot is
 * taken from a pool with DrawSlot_Alloc and freed with DrawSlot_Free
 * (DrawSlotB_* for the second pool); OT_AddByDepth sorts a slot into a
 * table by view depth, OT_AddAt puts it at a bucket, and Task_RunList runs
 * a table's draw callbacks. Also the players' world matrices
 * (Players_UpdateMatrices), two small render helpers and the colours'
 * conversion to floats.
 */

void Players_UpdateMatrices(void);
s32 Tex_CountLoaded(s32 start);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void OT_InsertSorted(DrawSlot *node, u32 *link);
void DrawPool_Clear(DrawSlot *slot, s32 count);
void DrawSlot_ResetAll(void);
void DrawSlotB_ResetAll(void);
void DrawPool_ClearHud(void);
DrawSlot *DrawSlot_Get(s32 index);
DrawSlot *DrawSlotB_Get(s32 index);
s16 DrawSlot_Alloc(void);
s16 DrawSlotB_Alloc(void);
void DrawSlot_Free(s32 index);
void DrawSlotB_Free(s32 index);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 bucketCount, s32 clamp);
s32 OT_AddAt(u32 *table, void *node, s32 bucket, s32 bucketCount);
void Task_RunList(u32 *list);
void Color_RgbToFloat(f32 *rgb, u32 color);
void Color_ArgbToFloat(f32 *rgba, u32 color);

#endif
