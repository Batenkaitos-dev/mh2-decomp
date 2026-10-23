/*
 * The spark effect (sub_main): effect objects of kind 8 (SparkEmitter,
 * fxobj.h) in the stage object list. SparkFx_Update's states start, run,
 * end and free an emitter's sparks, which follow key tracks
 * (gSparkFxKeys) and draw a model of the common set (Spark_Draw). Last,
 * the spawners: SparkFx_Spawn starts an emitter at a point and
 * SparkFx_SpawnAtEm at a monster (SparkFx_Alloc takes the object).
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "fxobj.h"
#include "render_state.h"
#include "card.h"
#include "sound.h"
#include "spark_fx.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "rand.h"
#include "area.h"

/* prototypes */
struct SparkEmitter;
/* end of prototypes */

/* The emitter's update: its state (0 start, 1 run, 2 end, 3 free) picks the step. */
void SparkFx_Update(SparkEmitter *emitter)
{
    u8 state;

    state = emitter->state;
    switch (state) {
    case 0:
        SparkFx_Start(emitter);
        break;
    case 1:
        SparkFx_Run(emitter);
        break;
    case 2:
        SparkFx_End(emitter);
        break;
    case 3:
        SparkFx_Free(emitter);
        break;
    }
}

/* The emitter's sound, by its type: only type 7 has one. */
void SparkFx_PlaySe(SparkEmitter *emitter)
{
    switch (emitter->type) {
    case 7:
        Sound_PlaySeAtPosChecked(1, 44, 0, emitter->pos, 1, 0);
        break;
    }
}

extern s16 gSparkFxCounts[];

/* The emitter's first state: sets up its sparks (gSparkFxCounts by type), each with a draw slot that
 * Spark_Draw draws, their delays, spins and spreads by type; type 6 keeps its height for the sparks
 * and moves to the origin (it follows its monster's bone), type 0 starts 20 higher. Plays the
 * emitter's sound. */
void SparkFx_Start(SparkEmitter *emitter)
{
    Spark *spark = emitter->sparks;
    f32 height;
    s16 n;
    s16 i;

    emitter->state++;
    emitter->visible = 1;
    emitter->draw = NULL;
    emitter->timer = 0;
    n = gSparkFxCounts[emitter->type];
    switch (emitter->type) {
    case 0:
        emitter->pos[1] += 20.0f;
        break;
    case 6:
        height = emitter->pos[1];
        emitter->pos[0] = 0.0f;
        emitter->pos[1] = 0.0f;
        emitter->pos[2] = 0.0f;
        break;
    }
    for (i = 0; i < n; i++, spark++) {
        spark->id = DrawSlot_Alloc();
        spark->variant = i;
        spark->age = 0;
        spark->scale = emitter->scale;
        spark->fixedColor = 0;
        spark->vel[0] = 0.0f;
        spark->vel[1] = 0.0f;
        spark->vel[2] = 0.0f;
        switch (emitter->type) {
        case 0:
            spark->spin = Rand_Next(1);
            spark->spinSpeed = 0;
            break;
        case 1:
        case 5:
            spark->variant = Rand_Next(1) & 7;
            spark->age = -(i & 1) * 4;
            spark->spin = 0;
            spark->spinSpeed = 0;
            spark->vel[0] = i * (10.0f * emitter->scale * SparkFx_RandSpread());
            spark->vel[1] = 0.0f;
            spark->vel[2] = i * (10.0f * emitter->scale * SparkFx_RandSpread());
            break;
        case 2:
            spark->variant = Rand_Next(1) & 7;
            spark->spin = 0;
            spark->spinSpeed = 0;
            break;
        case 3:
        case 4:
        case 7:
            spark->spin = Rand_Next(1);
            spark->spinSpeed = (u8)Rand_Next(1) - 128;
            break;
        case 6:
            spark->age = -i * 2;
            spark->vel[1] = height;
            spark->variant = Rand_Next(1) & 7;
            spark->spin = 0;
            spark->spinSpeed = 0;
            break;
        }
        if (spark->id != -1) {
            spark->obj = DrawSlot_Get(spark->id);
            spark->obj->owner = emitter;
            spark->obj->part = i;
            spark->obj->draw = Spark_Draw;
        } else {
            spark->obj = NULL;
        }
    }
    SparkFx_PlaySe(emitter);
}

extern s16 gSparkFxLives[];
extern s16 gSparkFxSteps[];
extern s16 gSparkFxFirstKeys[];
extern f32 *gSparkFxKeys[];

/* The emitter's run state: until its life is over (gSparkFxLives by type), its sparks start one after
 * another (age from below 0), follow their key tracks (gSparkFxKeys from gSparkFxFirstKeys[type],
 * gSparkFxSteps apart), spin, and are drawn at the emitter's position plus their offset. A spark
 * lives 20 frames for types 1 and 5, 10 for type 6 and the emitter's life otherwise; type 0's fall,
 * and type 6's take the position of bone 2 of the monster the emitter follows. */
void SparkFx_Run(SparkEmitter *emitter)
{
    s16 n = gSparkFxCounts[emitter->type];
    Spark *spark = emitter->sparks;
    u16 life = gSparkFxLives[emitter->type];
    s16 i;
    s16 key = gSparkFxFirstKeys[emitter->type];
    s16 step = gSparkFxSteps[emitter->type];
    f32 bonePos[3];
    f32 *track;
    u16 sparkLife;

    switch (emitter->type) {
    default:
        sparkLife = life;
        break;
    case 1:
    case 5:
        sparkLife = 20;
        break;
    case 6:
        sparkLife = 10;
        break;
    }
    if (++emitter->timer > life) {
        emitter->state++;
        emitter->visible = 0;
        return;
    }

    for (i = 0; i < n; i++) {
        switch (emitter->type) {
        case 1:
        case 5:
        case 6:
            key = gSparkFxFirstKeys[emitter->type];
            break;
        }
        if (++spark->age <= 0) {
            key += step;
            spark++;
            continue;
        }
        if (spark->age == 1) {
            if (emitter->type == 6) {
                if (emitter->owner->active) {
                    Em_GetBonePos(emitter->owner, 2, bonePos);
                    spark->vel[0] = bonePos[0];
                    spark->vel[2] = bonePos[2];
                } else {
                    key += step;
                    spark++;
                    continue;
                }
            }
        } else if (spark->age > sparkLife) {
            key += step;
            spark++;
            continue;
        }
        track = gSparkFxKeys[key++];
        Key_LerpVec3(track, spark->size, spark->age);
        if (spark->fixedColor == 0) {
            track = gSparkFxKeys[key++];
            Key_LerpFloat(track, &spark->alpha, spark->age);
        }
        spark->spin += (s32)spark->spinSpeed;
        switch (emitter->type) {
        case 0:
            if (spark->age > 4) {
                spark->vel[1] -= 10.0f / (sparkLife - 4);
            }
            break;
        }
        if (spark->obj != NULL) {
            spark->obj->pos[0] = emitter->pos[0] + spark->vel[0];
            spark->obj->pos[1] = emitter->pos[1] + spark->vel[1];
            spark->obj->pos[2] = emitter->pos[2] + spark->vel[2];
            OT_AddByDepth(gDrawOtSorted, spark->obj, 64, 0);
        }
        spark++;
    }
}

/* The emitter's end state: frees its sparks' draw slots. */
void SparkFx_End(SparkEmitter *emitter)
{
    s16 n;
    Spark *spark;
    s16 i;

    spark = emitter->sparks;
    emitter->state++;
    n = gSparkFxCounts[emitter->type];
    for (i = 0; i < n; i++, spark++) {
        if (spark->obj != NULL) {
            DrawSlot_Free(spark->id);
        }
    }
}

/* The emitter's last state: frees the emitter. */
void SparkFx_Free(SparkEmitter *emitter)
{
    StageObj_Free((struct StageObj *)emitter);
}

extern f32 gCamMatrix[];

/* Draws a spark (its draw slot's draw): a model of the common set 0 picked by the emitter's type
 * (texture ids 94, 26, 93), scaled by the spark's size, spun, and placed at the slot, facing
 * the camera for types 0-2, 5 and 6, with a texture offset by the spark's variant; tinted by the
 * emitter (white, orange, beige) and the spark's alpha, or the spark's fixed colour. The depth
 * buffer isn't written meanwhile. */
void Spark_Draw(DrawSlot *slot)
{
    SparkEmitter *emitter = (SparkEmitter *)slot->owner;
    Spark *spark = &emitter->sparks[slot->part];
    Model *models = gCommonModels[0];
    u16 flags = 0;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    void *matList;
    ModelMatSet *matSet;
    u8 red;
    u8 green;
    u8 blue;
    u32 color;

    if (models != NULL && models->active) {
        matList = models->mats;
        scale[0] = spark->scale * spark->size[0];
        scale[1] = spark->scale * spark->size[1];
        scale[2] = spark->scale * spark->size[2];
        Gs_SetState(RS_ZWRITE, 0);
        if (emitter->tint == 0) {
            blue = green = red = 255;
        } else if (emitter->tint == 1) {
            red = 255;
            green = 127;
            blue = 0;
        } else {
            red = 216;
            green = 182;
            blue = 138;
        }
        switch (emitter->type) {
        case 0:
            if (emitter->tint == 1) {
                flags |= 2;
            }
            matSet = &models->matSets[Effect_MapTexId(94)];
            rot[2] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        case 1:
        case 2:
        case 5:
        case 6:
            if (emitter->tint == 1) {
                flags |= 2;
            }
            matSet = &models->matSets[Effect_MapTexId(26)];
            rot[0] = 0.0f;
            if (spark->variant & 1) {
                rot[1] = 3.1415927f;
            } else {
                rot[1] = 0.0f;
            }
            rot[2] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 6, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Mtx_Translation(uvMtx, 0.125f * (spark->variant >> 1), 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        case 3:
        case 4:
            matSet = &models->matSets[Effect_MapTexId(93)];
            rot[1] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 2, mtx);
            if (gScene.inLobby == 0) {
                Mtx_Translation(uvMtx, 0.75f, 0.75f, 0.0f);
            } else {
                Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
            }
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        case 7:
            matSet = &models->matSets[Effect_MapTexId(93)];
            rot[1] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 2, mtx);
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        }
        if (spark->fixedColor == 0) {
            color = ((u8)(255.0f * spark->alpha) << 24) | (red << 16) | (green << 8) | blue;
        } else {
            color = spark->color;
        }
        Effect_DrawModel(matSet, mtx, color, flags, matList);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/* A random spread between -0.512 and 0.511. */
f32 SparkFx_RandSpread(void)
{
    return 0.001f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 512);
}

SparkEmitter *SparkFx_Alloc(s32 unused)
{
    return (SparkEmitter *)StageObj_AllocTail(1);
}

/* Starts an effect object of kind 8 (update SparkFx_Update) at pos, with +3 = k, +7 = b and the scale. */
void SparkFx_Spawn(f32 *pos, s32 sparkType, s32 tintKind, f32 scale)
{
    SparkEmitter *emitter = SparkFx_Alloc(sparkType);

    if (emitter != NULL) {
        emitter->kind = 8;
        emitter->update = SparkFx_Update;
        emitter->owner = NULL;
        emitter->type = sparkType;
        emitter->tint = tintKind;
        emitter->scale = scale;
        Vec_Copy(emitter->pos, pos);
    }
}

/* SparkFx_Spawn at monster em's position (height y), when em is in the current area; em is the emitter's owner. */
void SparkFx_SpawnAtEm(Em *em, s32 sparkType, s32 tintKind, f32 scale, f32 y)
{
    SparkEmitter *emitter;

    if ((u8)Obj_InCurrentArea(em)) {
        if ((emitter = SparkFx_Alloc(sparkType)) != NULL) {
            emitter->kind = 8;
            emitter->update = SparkFx_Update;
            emitter->owner = em;
            emitter->type = sparkType;
            emitter->tint = tintKind;
            emitter->scale = scale;
            Vec_Copy(emitter->pos, &em->posX);
            emitter->pos[1] = y;
        }
    }
}

/* The file's initialised data, in the game's order */
f32 gSparkFxKeys_0[20] = {
#include "sub_main/00469590.inc"
};
f32 gSparkFxKeys_1[8] = {
#include "sub_main/004695E0.inc"
};
f32 gSparkFxKeys_2[28] = {
#include "sub_main/00469600.inc"
};
f32 gSparkFxKeys_3[8] = {
#include "sub_main/00469670.inc"
};
f32 gSparkFxKeys_4[28] = {
#include "sub_main/00469690.inc"
};
f32 gSparkFxKeys_5[8] = {
#include "sub_main/00469700.inc"
};
f32 gSparkFxKeys_6[20] = {
#include "sub_main/00469720.inc"
};
f32 gSparkFxKeys_7[8] = {
#include "sub_main/00469770.inc"
};
f32 gSparkFxKeys_8[16] = {
#include "sub_main/00469790.inc"
};
f32 gSparkFxKeys_9[8] = {
#include "sub_main/004697D0.inc"
};
f32 gSparkFxKeys_10[24] = {
#include "sub_main/004697F0.inc"
};
f32 gSparkFxKeys_11[8] = {
#include "sub_main/00469850.inc"
};
f32 *gSparkFxKeys[12] = {
    gSparkFxKeys_0, gSparkFxKeys_1, gSparkFxKeys_2, gSparkFxKeys_3,
    gSparkFxKeys_4, gSparkFxKeys_5, gSparkFxKeys_6, gSparkFxKeys_7,
    gSparkFxKeys_8, gSparkFxKeys_9, gSparkFxKeys_10, gSparkFxKeys_11,
};
s16 gSparkFxCounts[8] = {
#include "sub_main/004698A0.inc"
};
s16 gSparkFxLives[8] = {
#include "sub_main/004698B0.inc"
};
s16 gSparkFxSteps[8] = {
#include "sub_main/004698C0.inc"
};
s16 gSparkFxFirstKeys[8] = {
#include "sub_main/004698D0.inc"
};
