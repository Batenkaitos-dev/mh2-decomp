#ifndef SPARK_FX_H
#define SPARK_FX_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "fxobj.h"

/*
 * The spark effect: emitters of kind 8 (SparkEmitter) in the stage object
 * list, whose sparks follow key tracks and draw a model of the common set.
 * SparkFx_SpawnAtEm starts one at a monster; the SparkFx_* states run it
 * and Spark_Draw draws it.
 */

void SparkFx_Start(SparkEmitter *emitter);
void SparkFx_Run(SparkEmitter *emitter);
void SparkFx_End(SparkEmitter *emitter);
void SparkFx_Free(SparkEmitter *emitter);
void Spark_Draw(DrawSlot *slot);
f32 SparkFx_RandSpread(void);
void SparkFx_Spawn(f32 *pos, s32 sparkType, s32 tintKind, f32 scale);
void SparkFx_SpawnAtEm(Em *em, s32 sparkType, s32 tintKind, f32 scale, f32 y);

#endif
