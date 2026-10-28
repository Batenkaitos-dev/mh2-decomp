/*
 * The STAGEOBJ_KIND_PART_FX effect objects: small effects made of parts (PartFxPart,
 * gPartFxPartCounts[type] of them in the object's buffer). For
 * gPartFxLens[type] frames each part follows its key curves
 * (gPartFxKeys: its size, its offset for most types, its alpha) and is
 * drawn from its own draw slot as a model of the common model set, most
 * types facing the camera: dust at the feet, a glow at a weapon's tip
 * (type 21), the item offer's sparkle (type 26) and the like.
 * The spawners place one at a bone of a player (PartFx_PlayerPos) or a
 * monster (PartFx_EmPos), or at a point. Where the type has a ground kind
 * (gPartFxGroundKinds) and there is ground, the ground puffs of
 * Effect_SpawnGround are made instead. The players' motion effects and the
 * monsters' code call them. Last come the ground effects themselves:
 * Effect_SpawnGround, the puffs a footfall or a landing raises by the
 * ground's kind (spark emitters, with their sounds), and
 * Player_SpawnGroundEffect, a part effect on the ground at a player's
 * weapon (the motion effects' slams). The .data holds the key curves and
 * the tables by type.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"
#include "area.h"
#include "render_state.h"
#include "sound.h"
#include "fishing.h"
#include "text.h"
#include "part_effects.h"
#include "attack_objects.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "rand.h"
#include "spark_fx.h"
#include "vec_math.h"

/* prototypes */
/* The calls cast Obj_InCurrentArea's result to u8: the game's code reads only its low byte. */
extern f32 gCamMatrix[];

/* PartFx_Run builds as the game's code only at optimization level 4; the
 * other functions build the same at levels 3 and 4. */
#pragma optimization_level 4

/* A STAGEOBJ_KIND_PART_FX effect object's update, by its state: set up, run, end, free. */
void PartFx_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        PartFx_Setup(o);
        break;
    case 1:
        PartFx_Run(o);
        break;
    case 2:
        PartFx_End(o);
        break;
    case 3:
        PartFx_Delete(o);
        break;
    }
}

/* A part of a STAGEOBJ_KIND_PART_FX effect object (StageObj.buf, 48 bytes each): a model or
 * sprite whose size and alpha follow key curves over its frames. */
typedef struct PartFxPart {
    s16 variant;                /* 0x00: its role in types 14 (0/1), 35 (0-5) and 36 (0/1: mirrored); 255: gone (type 13) */
    s16 frame;                  /* 0x02: counted up by PartFx_Run; 0 or less: still waiting */
    f32 pos[3];                 /* 0x04: its offset from the object (most types) or its place */
    f32 size[3];                /* 0x10: from its first key curve; times scale, the model's size */
    f32 scale;                  /* 0x1C */
    f32 alpha;                  /* 0x20: from its last key curve (0 to 1) */
    u16 heading;                /* 0x24: type 13: the owner's bone 2 heading plus ANGLE_TURN * 3 / 8 */
    u16 roll;                   /* 0x26: the model's turn about the view axis */
    s16 rollSpeed;              /* 0x28: added to roll every frame */
    s16 slot;                   /* 0x2A: its draw slot */
    DrawSlot *draw;             /* 0x2C: NULL when none */
} PartFxPart;
extern s16 gPartFxPartCounts[];
extern s16 gPartFxType14Starts[];
extern s16 gPartFxType19Starts[];
extern s16 gPartFxType35Starts[];
extern s16 gPartFxType36Starts[];
/*
 * Starts a STAGEOBJ_KIND_PART_FX effect object: type 9 moves 10 to the side of its foot
 * (var0) and 100 back, some types take a random heading, then its sound
 * (PartFx_PlaySound). Each part (gPartFxPartCounts[type]) gets its size,
 * its start roll and roll speed, its first frame (below 1 delays the part)
 * and a draw slot that PartFx_DrawPart draws.
 */
void PartFx_Setup(StageObj *fx)
{
    s16 partCount;
    s16 i;
    PartFxPart *part = (PartFxPart *)fx->buf;
    f32 offset[3];

    fx->state++;
    fx->visible = 1;
    fx->draw = NULL;
    fx->timer = 0;
    partCount = gPartFxPartCounts[fx->type];
    switch (fx->type) {
    case 9:
        if (fx->var0 == 1) {
            offset[0] = 10.0f;
            offset[1] = 0.0f;
            offset[2] = -100.0f;
        } else {
            offset[0] = -10.0f;
            offset[1] = 0.0f;
            offset[2] = -100.0f;
        }
        Vec_RotateY(offset, 6.2831855f * fx->angle / 65536.0f);
        fx->pos[0] += offset[0];
        fx->pos[1] += offset[1];
        fx->pos[2] += offset[2];
        break;
    case 19:
    case 20:
    case 21:
    case 26:
    case 29:
    case 40:
        fx->angle = Rand_Next(1);
        break;
    }
    PartFx_PlaySound(fx);
    for (i = 0; i < partCount; part++, i++) {
        part->scale = 1.0f;
        part->frame = 0;
        switch (fx->type) {
        case 0:
            part->scale = fx->scale;
            part->roll = Rand_Next(1);
            part->rollSpeed = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 3:
        case 18:
        case 30:
        case 32:
        case 33:
            part->scale = fx->scale;
            part->roll = (s16)(0.5f + 65536.0f * (0.09f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 5:
        case 10:
            part->scale = fx->scale * (1.0f + 0.0005f * (Rand_Next(1) & 0x3FF));
            part->roll = Rand_Next(1);
            part->rollSpeed = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 4:
        case 6:
        case 7:
        case 8:
        case 9:
        case 11:
        case 12:
        case 15:
        case 17:
        case 23:
        case 24:
        case 25:
        case 38:
        case 39:
            part->scale = fx->scale;
            part->roll = Rand_Next(1);
            part->rollSpeed = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 13:
            part->frame = -i * 2;
            part->scale = fx->scale * (1.0f + 0.2f * (0.001f * (Rand_Next(1) & 0x3FF)));
            part->roll = Rand_Next(1);
            part->rollSpeed = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 14:
            part->scale = fx->scale;
            part->frame = gPartFxType14Starts[i];
            switch (i) {
            case 0:
                part->variant = 0;
                break;
            case 1:
            case 2:
                part->variant = 1;
                break;
            }
            part->roll = 0;
            part->rollSpeed = 0;
            break;
        case 16:
        case 31:
        case 37:
            part->frame = -1;
            part->scale = fx->scale;
            part->roll = 0;
            part->rollSpeed = 0;
            break;
        case 19:
            part->frame = gPartFxType19Starts[i] - 6;
            part->scale = fx->scale;
            part->roll = Rand_Next(1);
            part->rollSpeed = (s16)(0.5f + 65536.0f * (0.002f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 20:
        case 27:
            part->frame = 0;
            part->scale = fx->scale;
            part->roll = Rand_Next(1);
            break;
        case 21:
        case 26:
        case 29:
            part->scale = fx->scale;
            if (i == 0) {
                part->scale *= 2.0f;
                part->frame = 0;
            } else if (i == 5) {
                part->frame = -2;
            } else {
                part->frame = 0;
            }
            part->roll = 0;
            part->rollSpeed = 0;
            break;
        case 22:
        case 28:
            part->frame = 0;
            part->scale = fx->scale;
            part->roll = Rand_Next(1);
            part->rollSpeed = (Rand_Next(1) & 0xFF) - 128;
            break;
        case 34:
            part->frame = -i * 3;
            part->scale = fx->scale;
            part->roll = Rand_Next(1);
            Vec_Copy(part->pos, fx->pos);
            break;
        case 35:
            part->frame = gPartFxType35Starts[i];
            part->scale = fx->scale;
            part->variant = i;
            Vec_Copy(part->pos, fx->pos);
            part->pos[1] += 30.0f;
            switch (part->variant) {
            case 0:
            case 5:
                part->roll = 0;
                part->rollSpeed = 0;
                break;
            case 1:
                part->roll = Rand_Next(1);
                part->rollSpeed = 0x1111;
                break;
            case 2:
            case 3:
            case 4:
            default:
                part->roll = Rand_Next(1);
                part->rollSpeed = 0xCCD;
                break;
            }
            break;
        case 36:
            part->frame = gPartFxType36Starts[i];
            part->scale = fx->scale;
            part->variant = i & 1;
            Vec_Copy(part->pos, fx->pos);
            part->roll = Rand_Next(1);
            if (part->variant == 0) {
                part->rollSpeed = 0x666;
            } else {
                part->rollSpeed = -0x665;
            }
            break;
        case 40:
            part->frame = gPartFxType19Starts[i];
            part->scale = fx->scale;
            part->roll = Rand_Next(1);
            part->rollSpeed = (s16)(0.5f + 65536.0f * (0.002f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        }
        part->slot = DrawSlot_Alloc();
        if (part->slot != -1) {
            part->draw = DrawSlot_Get(part->slot);
            part->draw->owner = fx;
            part->draw->part = i;
            part->draw->draw = PartFx_DrawPart;
        } else {
            part->draw = NULL;
        }
    }
}

extern void *gPartFxKeys[];
extern u16 gPartFxLens[];
extern s16 gPartFxKeyStarts[];
extern s16 gPartFxKeyStrides[];
extern u16 gPartFxType14Lens[];
extern u16 gPartFxType21Lens[];
extern u16 gPartFxType35Lens[];
/*
 * Runs a STAGEOBJ_KIND_PART_FX effect object for a frame; it ends after
 * gPartFxLens[type] frames. Each part counts its frame: it waits while
 * the frame is 0 or less and stops after its length. It reads its key
 * curves (gPartFxKeys from gPartFxKeyStarts[type], gPartFxKeyStrides[type]
 * apart: size, offset for most types, alpha) at that frame, turns its roll,
 * places its draw slot around the object (turned by the object's heading,
 * the parts `step` apart in some types) and adds it to the sorted ordering
 * table. Type 13 starts each part on the floor at its owner's bone 2, and
 * the whole run stops once the owner is gone (variant 255).
 */
void PartFx_Run(StageObj *fx)
{
    s16 i;
    s16 partCount = gPartFxPartCounts[fx->type];
    u16 dur = gPartFxLens[fx->type];
    s16 key = gPartFxKeyStarts[fx->type];
    s16 stride = gPartFxKeyStrides[fx->type];
    s16 step;
    PartFxPart *part = (PartFxPart *)fx->buf;
    u16 len;
    void *keys;
    f32 boneMtx[16];
    f32 offset[3];

    switch (fx->type) {
    case 6:
        step = 10922;
        break;
    case 8:
        step = 10922;
        break;
    case 7:
    case 11:
    case 12:
    case 15:
    case 23:
    case 24:
    case 25:
    case 38:
    case 39:
        step = 13107;
        break;
    case 19:
        step = 8192;
        break;
    case 40:
        step = 8192;
        break;
    case 21:
    case 26:
    case 29:
        fx->angle += 182;
        break;
    }
    if (++fx->timer > dur) {
        fx->state++;
        fx->visible = 0;
        return;
    }
    for (i = 0; i < partCount; i++) {
        switch (fx->type) {
        case 6:
        case 8:
        case 7:
        case 11:
        case 12:
        case 15:
        case 17:
        case 19:
        case 23:
        case 24:
        case 25:
        case 38:
        case 39:
        case 40:
            key = gPartFxKeyStarts[fx->type];
            len = dur;
            break;
        case 13:
            key = gPartFxKeyStarts[fx->type];
            len = 12;
            break;
        case 14:
            len = gPartFxType14Lens[i];
            break;
        case 21:
        case 26:
        case 29:
            len = gPartFxType21Lens[i];
            break;
        case 34:
            key = gPartFxKeyStarts[fx->type];
            len = 14;
            break;
        case 35:
            len = gPartFxType35Lens[i];
            break;
        case 36:
            key = gPartFxKeyStarts[fx->type];
            len = 10;
            break;
        default:
            len = dur;
            break;
        }
        if (++part->frame <= 0) {
            part++;
            key += stride;
            continue;
        }
        if (part->frame == 1) {
            if (fx->type == 13) {
                if (((Player *)fx->owner)->active != 0) {
                    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(fx->owner, 2)));
                    Mtx_GetTrans(part->pos, boneMtx);
                    part->pos[1] = ((Player *)fx->owner)->floorY;
                    part->heading = Mtx_Heading(boneMtx) + ANGLE_TURN * 3 / 8;
                } else {
                    part->variant = 255;
                    part++;
                    key += stride;
                    continue;
                }
            }
        } else if (part->frame > len) {
            part++;
            key += stride;
            continue;
        }
        if (part->variant == 255) {
            return;
        }
        Key_LerpVec3(gPartFxKeys[key], part->size, part->frame);
        switch (fx->type) {
        case 0:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 22:
        case 23:
        case 24:
        case 25:
        case 28:
        case 30:
        case 31:
        case 32:
        case 33:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
            Key_LerpVec3(gPartFxKeys[(s16)(key + 1)], part->pos, part->frame);
            part->pos[0] *= fx->scale;
            part->pos[1] *= fx->scale;
            part->pos[2] *= fx->scale;
            keys = gPartFxKeys[(s16)(key + 2)];
            key += 3;
            break;
        case 13:
            Key_LerpVec3(gPartFxKeys[(s16)(key + 1)], offset, part->frame);
            offset[0] *= fx->scale;
            offset[1] *= fx->scale;
            offset[2] *= fx->scale;
            keys = gPartFxKeys[(s16)(key + 2)];
            key += 3;
            break;
        default:
            keys = gPartFxKeys[(s16)(key + 1)];
            key += 2;
            break;
        }
        Key_LerpFloat(keys, &part->alpha, part->frame);
        if (part->draw != NULL) {
            switch (fx->type) {
            case 3:
            case 18:
            case 30:
            case 32:
            case 33:
                offset[0] = part->pos[0];
                offset[1] = part->pos[1];
                offset[2] = 20.0f + part->pos[2];
                Vec_RotateY(offset, 6.2831855f * fx->angle / 65536.0f);
                part->draw->pos[0] = offset[0] + fx->pos[0];
                part->draw->pos[1] = offset[1] + fx->pos[1];
                part->draw->pos[2] = offset[2] + fx->pos[2];
                break;
            case 0:
            case 4:
            case 5:
            case 9:
            case 10:
            case 14:
            case 16:
            case 17:
            case 22:
            case 28:
            case 31:
            case 37:
                part->roll += (u16)part->rollSpeed;
                Vec_Copy(offset, part->pos);
                Vec_RotateY(offset, 6.2831855f * fx->angle / 65536.0f);
                part->draw->pos[0] = offset[0] + fx->pos[0];
                part->draw->pos[1] = offset[1] + fx->pos[1];
                part->draw->pos[2] = offset[2] + fx->pos[2];
                break;
            case 6:
            case 7:
            case 8:
            case 11:
            case 12:
            case 15:
            case 19:
            case 23:
            case 24:
            case 25:
            case 38:
            case 39:
            case 40:
                part->roll += (u16)part->rollSpeed;
                Vec_Copy(offset, part->pos);
                Vec_RotateY(offset, 6.2831855f * (fx->angle + i * step) / 65536.0f);
                part->draw->pos[0] = offset[0] + fx->pos[0];
                part->draw->pos[1] = offset[1] + fx->pos[1];
                part->draw->pos[2] = offset[2] + fx->pos[2];
                break;
            case 13:
                part->roll += (u16)part->rollSpeed;
                Vec_RotateY(offset, 6.2831855f * part->heading / 65536.0f);
                part->draw->pos[0] = offset[0] + part->pos[0];
                part->draw->pos[1] = offset[1] + part->pos[1];
                part->draw->pos[2] = offset[2] + part->pos[2];
                break;
            case 20:
            case 27:
                Vec_Copy(part->draw->pos, fx->pos);
                break;
            case 21:
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 30.0f;
                if (fx->owner != NULL && ((Player *)fx->owner)->active != 0 && ((Player *)fx->owner)->weaponType == WEAPON_HUNTING_HORN) {
                    offset[2] += 60.0f;
                }
                Vec_RotateSelf(offset, gCamMatrix);
                part->draw->pos[0] = fx->pos[0] + offset[0];
                part->draw->pos[1] = fx->pos[1] + offset[1];
                part->draw->pos[2] = fx->pos[2] + offset[2];
                break;
            case 26:
            case 29:
                part->draw->pos[0] = fx->pos[0];
                part->draw->pos[1] = fx->pos[1];
                part->draw->pos[2] = fx->pos[2];
                break;
            case 34:
                part->pos[1] += 2.0f;
                part->draw->pos[0] = part->pos[0];
                part->draw->pos[1] = part->pos[1];
                part->draw->pos[2] = part->pos[2];
                break;
            case 35:
                part->roll += (u16)part->rollSpeed;
                switch (part->variant) {
                case 0:
                    if (part->frame > 2) {
                        part->pos[1] += 33.3f;
                    }
                    break;
                case 2:
                    if (part->frame > 2) {
                        part->pos[1] += 25.0f;
                    }
                    break;
                case 3:
                    if (part->frame > 2) {
                        part->pos[1] += 18.7f;
                    }
                    break;
                case 4:
                    if (part->frame > 2) {
                        part->pos[1] += 12.5f;
                    }
                    break;
                }
                Vec_Copy(part->draw->pos, part->pos);
                break;
            case 36:
                part->roll += (u16)part->rollSpeed;
                Vec_Copy(offset, part->pos);
                if (part->variant != 0) {
                    offset[0] = -offset[0];
                }
                Vec_RotateY(offset, 6.2831855f * fx->angle / 65536.0f);
                part->draw->pos[0] = offset[0] + fx->pos[0];
                part->draw->pos[1] = offset[1] + fx->pos[1];
                part->draw->pos[2] = offset[2] + fx->pos[2];
                break;
            }
            OT_AddByDepth(gDrawOtSorted, part->draw, 64, 0);
        }
        part++;
    }
}

/* Frees a STAGEOBJ_KIND_PART_FX effect object's draw slots (gPartFxPartCounts: the parts of each type). */
void PartFx_End(StageObj *fx)
{
    s16 partCount;
    s16 i;
    PartFxPart *part = (PartFxPart *)fx->buf;

    fx->state++;
    partCount = gPartFxPartCounts[fx->type];
    for (i = 0; i < partCount; i++, part++) {
        if (part->draw != NULL) {
            DrawSlot_Free(part->slot);
        }
    }
}

/* Frees a STAGEOBJ_KIND_PART_FX effect object. */
void PartFx_Delete(StageObj *fx)
{
    StageObj_Free(fx);
}

extern f32 gCamYawMtx[];
extern u8 gPartFxOfferColors[][3];
/*
 * Draws one part of a STAGEOBJ_KIND_PART_FX effect object from its draw slot: a model of
 * common model set 0 picked by type (and part), sized by the part's size
 * curve times its scale, turned by its roll, facing the camera in most
 * types. Its colour is the area's light (Effect_GetColor) or the type's own
 * colour; types 1, 2 and 16 fade by the part's alpha (Model_DrawFade), the
 * others draw with it in the colour's top byte.
 */
void PartFx_DrawPart(DrawSlot *drawSlot)
{
    StageObj *fx = drawSlot->owner;
    PartFxPart *part = &((PartFxPart *)fx->buf)[drawSlot->part];
    void *base;
    u16 flags = 0;
    f32 mtx[16];
    f32 rot[3];
    f32 scale[3];
    u8 red;
    u8 green;
    u8 blue;
    u8 lightLevel;
    s16 id;
    f32 bright;
    ModelMatSet *mdl;
    Model *set = gCommonModels[0];

    if (set != NULL && set->active) {
        base = set->mats;
        Effect_GetColor(gScene.area, &red, &green, &blue, &lightLevel);
        bright = lightLevel / 255.0f;
        switch (fx->type) {
        case 0:
            Gs_SetState(RS_ZWRITE, 0);
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            id = 74;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 3:
        case 4:
        case 6:
        case 7:
        case 9:
        case 11:
        case 12:
        case 13:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 23:
        case 24:
        case 25:
        case 27:
        case 31:
        case 32:
        case 33:
        case 34:
        case 37:
        case 38:
        case 39:
        case 40:
            Gs_SetState(RS_ZWRITE, 0);
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            id = 74;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 5:
        case 10:
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            id = 74;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 8:
            scale[0] = 0.8f * part->size[0];
            scale[1] = 0.8f * part->size[1];
            scale[2] = 0.8f * part->size[2];
            id = 74;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 14:
            switch (part->variant) {
            case 0:
                id = 74;
                red = 255;
                green = 255;
                blue = 255;
                bright = 1.0f;
                break;
            case 1:
                id = 35;
                red = 51;
                green = 51;
                blue = 51;
                bright = 1.0f;
                break;
            }
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 21:
        case 26:
        case 29:
            if (drawSlot->part == 0) {
                id = 65;
            } else {
                id = 64;
            }
            flags |= 2;
            if (fx->type == 21) {
                red = 191;
                blue = 255;
                green = 191;
            } else if (fx->type == 29) {
                red = 255;
                green = 127;
                blue = 127;
            } else if (gScene.inLobby == 0) {
                red = gPartFxOfferColors[fx->ownerIndex][0];
                green = gPartFxOfferColors[fx->ownerIndex][1];
                blue = gPartFxOfferColors[fx->ownerIndex][2];
            } else {
                red = 255;
                green = 255;
                blue = 255;
            }
            bright = 1.0f;
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            rot[2] = 6.2831855f * (part->roll + fx->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 22:
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            red = 255;
            green = 255;
            blue = 255;
            bright = 1.0f;
            id = 72;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 28:
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            red = 216;
            green = 206;
            blue = 185;
            bright = 1.0f;
            id = 72;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 30:
            scale[0] = part->scale * part->size[0];
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            red = 255;
            green = 255;
            blue = 255;
            bright = 1.0f;
            id = 74;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 35:
            Gs_SetState(RS_ZWRITE, 0);
            flags |= 2;
            scale[0] = part->scale * part->size[0];
            bright = 1.0f;
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            switch (part->variant) {
            case 0:
                red = 255;
                id = 74;
                green = 51;
                blue = 51;
                Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 0, mtx);
                Mtx_Concat3x3(mtx, gCamYawMtx);
                break;
            case 1:
                red = 255;
                green = 255;
                blue = 255;
                id = 81;
                rot[2] = 6.2831855f * part->roll / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
                Mtx_Concat3x3(mtx, gCamMatrix);
                break;
            case 2:
            case 3:
            case 4:
            default:
                id = 38;
                red = 255;
                green = 51;
                blue = 51;
                switch (part->variant) {
                case 2:
                    rot[0] = -0.7853982f;
                    rot[1] = -0.7853982f;
                    break;
                case 3:
                    rot[0] = 0.7853982f;
                    rot[1] = -0.7853982f;
                    break;
                case 4:
                    rot[0] = 2.3561945f;
                    rot[1] = 0.0f;
                    break;
                default:
                    rot[0] = 0.0f;
                    rot[1] = 0.0f;
                    break;
                }
                rot[1] += 6.2831855f * fx->angle / 65536.0f;
                rot[2] = 6.2831855f * part->roll / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 4, mtx);
                Mtx_TurnZ(mtx, rot[2]);
                break;
            case 5:
                id = 59;
                red = 255;
                green = 175;
                blue = 111;
                rot[2] = 6.2831855f * part->roll / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
                Mtx_Concat3x3(mtx, gCamMatrix);
                break;
            }
            break;
        case 36:
            Gs_SetState(RS_ZWRITE, 0);
            flags |= 2;
            scale[0] = part->scale * part->size[0];
            bright = 1.0f;
            scale[1] = part->scale * part->size[1];
            scale[2] = part->scale * part->size[2];
            red = 255;
            green = 51;
            blue = 51;
            rot[0] = -0.5235988f;
            id = 38;
            if (part->variant == 0) {
                rot[1] = -1.308997f;
            } else {
                rot[1] = 1.308997f;
            }
            rot[1] += 6.2831855f * fx->angle / 65536.0f;
            rot[2] = 6.2831855f * part->roll / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 4, mtx);
            Mtx_TurnZ(mtx, rot[2]);
            break;
        }
        id = Effect_MapTexId(id);
        if (id != -1) {
            mdl = &set->matSets[id];
            /* types 1, 2 and 16; the unsigned compare is the game's form of 1 <= type <= 2 */
            if ((u32)(fx->type - 1) <= 1 || fx->type == 16) {
                Model_DrawFade(mdl, mtx, flags, base, part->alpha);
            } else {
                Effect_DrawModel(mdl, mtx, ((u8)(u32)(255.0f * bright * part->alpha) << 24) | (red << 16) | (green << 8) | blue, flags, base);
            }
        }
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * A STAGEOBJ_KIND_PART_FX effect's sound as it starts, by type: type 26 (the item
 * offer's sparkle) plays system sound 26 when the giver (its owner) or
 * the player offered to (ownerIndex) is the local player; type 35 plays
 * the player's sound 12 when the player holds a long sword.
 */
void PartFx_PlaySound(StageObj *fx)
{
    Player *pl;

    switch (fx->type) {
    case 26:
        if (((Player *)fx->owner)->index == gScene.localPlayer || fx->ownerIndex == gScene.localPlayer) {
            Sound_PlaySeAtPosChecked(SE_SLOT_SYSTEM, 26, 0, fx->pos, 1, 0);
        }
        break;
    case 35:
        pl = fx->owner;
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            Player_PlaySe(pl, 12, 0, pl->pos, 1, 0);
        }
        break;
    }
}

/* unit's own declaration: returning s32, though the function returns u8: PartFx_PutOnGround builds as the game's code only with this declaration. */
s32 AreaGrid2_GetFloorHeight(f32 *pos, f32 *h);
extern s16 gPartFxGroundKinds[];
extern s16 gAreaKinds[];
/*
 * Puts pos on the ground for an effect of the given type and returns the
 * type's ground kind (gPartFxGroundKinds: the puffs Effect_SpawnGround makes).
 * With pl, pos starts at pl's floor height. pos goes 5 above the floor
 * found under it, unless that floor is more than 18 below; with no such
 * floor, in the fortress areas it goes 5 plus the area's ground offset up.
 * Returns 0 when the type has no ground kind or no ground was found.
 */
s32 PartFx_PutOnGround(Player *pl, f32 *pos, s32 type)
{
    f32 groundY;
    s16 *groundKind = &gPartFxGroundKinds[(s16)type];
    s16 areaKind;

    if (*groundKind == 0) {
        return 0;
    }
    if (pl != NULL) {
        pos[1] = pl->floorY;
    }
    if (AreaGrid2_GetFloorHeight(pos, &groundY) != 0 && !(groundY < pos[1] - 18.0f)) {
        pos[1] = 5.0f + groundY;
        return *groundKind;
    }
    areaKind = gAreaKinds[gScene.area];
    if (areaKind == AREA_FORTRESS_0 || areaKind == AREA_FORTRESS_1) {
        pos[1] += 5.0f;
        pos[1] += Area_GroundOffset(gScene.area);
        return *groundKind;
    }
    return 0;
}

/*
 * Makes fx a STAGEOBJ_KIND_PART_FX effect object of the given type that follows pl,
 * facing pl's heading. Types 0, 3, 4, 9, 18 and 20 note the foot in var0
 * (0 for bone 8, else 1); type 10 is 0.7 the size; type 13 keeps the bone
 * in var0; type 22 faces across the bone's heading; type 26 (the item
 * offer's sparkle) takes the player offered to (Player.offerTo) as its
 * ownerIndex.
 */
void PartFx_InitForPlayer(Player *pl, s16 bone, s32 type, StageObj *fx)
{
    f32 boneMtx[4][4];

    fx->kind = STAGEOBJ_KIND_PART_FX;
    fx->update = PartFx_Update;
    fx->type = type;
    fx->angle = pl->rot[1];
    switch (fx->type) {
    case 0:
    case 3:
    case 4:
    case 9:
    case 18:
    case 20:
        if (bone == 8) {
            fx->var0 = 0;
        } else {
            fx->var0 = 1;
        }
        break;
    case 10:
        fx->scale = 0.7f;
        break;
    case 13:
        fx->var0 = bone;
        break;
    case 22:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        fx->angle = Mtx_Heading(boneMtx[0]) + ANGLE_TURN / 4;
        break;
    case 26:
        fx->ownerIndex = pl->offerTo;
        break;
    }
    fx->owner = pl;
}

void AttackBurst_CreateAtScaled(f32 *pos, u16 ang, s32 n, f32 scale); /* unit's own declaration: the definition (game) is void AttackBurst_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale); the calls here match only with this one */
/*
 * Sets pos to where an effect of the given type goes on pl: at or near
 * bone `bone`, on pl's floor or in front of pl, by type (types 8 and 39
 * move 50 ahead during the hard landing). Then puts it on the ground
 * (PartFx_PutOnGround) and returns the ground kind; type 19 with no
 * ground kind adds an attack burst 20 above pos and a type 14 effect.
 */
s32 PartFx_PlayerPos(f32 *pos, Player *pl, s16 bone, s32 type)
{
    f32 boneMtx[4][4];
    f32 offset[3];
    f32 groundY;
    s32 groundKind;

    switch ((s16)type) {
    case 0:
    case 3:
    case 4:
    case 6:
    case 7:
    case 9:
    case 18:
    case 20:
        Em_GetBonePos(pl, bone, pos);
        break;
    case 5:
        Em_GetBonePos(pl, bone, pos);
        offset[0] = 0.0f;
        offset[1] = -19.0f;
        offset[2] = -13.0f;
        Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
        Vec_Add(pos, pos, offset);
        break;
    case 8:
    case 39:
        pos[0] = pl->pos[0];
        pos[1] = pl->floorY;
        pos[2] = pl->pos[2];
        if (pl->motionId[0] == PL_MOT_LAND_HARD) {
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 50.0f;
            Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
            pos[0] += offset[0];
            pos[1] += offset[1];
            pos[2] += offset[2];
        }
        break;
    case 10:
        Em_GetBonePos(pl, bone, pos);
        break;
    case 11:
        Em_GetBonePos(pl, bone, pos);
        pos[1] = pl->floorY;
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 20.0f;
        Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
        pos[0] += offset[0];
        pos[1] += offset[1];
        pos[2] += offset[2];
        break;
    case 12:
    case 19:
    case 40:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        offset[0] = 44.0f;
        offset[1] = 0.0f;
        offset[2] = 123.0f;
        Vec_RotateSelf(offset, boneMtx[0]);
        pos[0] = boneMtx[3][0] + offset[0];
        pos[1] = boneMtx[3][1] + offset[1];
        pos[2] = boneMtx[3][2] + offset[2];
        groundY = AreaGrid2_GetFloorY(pos);
        if (pos[1] < 50.0f + groundY) {
            pos[1] = groundY;
        }
        break;
    case 13:
        Em_GetBonePos(pl, bone, pos);
        pos[1] = pl->floorY;
        break;
    case 14:
    case 15:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        offset[0] = 44.0f;
        offset[1] = 0.0f;
        offset[2] = 123.0f;
        Vec_RotateSelf(offset, boneMtx[0]);
        pos[0] = boneMtx[3][0] + offset[0];
        pos[1] = boneMtx[3][1] + offset[1];
        pos[2] = boneMtx[3][2] + offset[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        break;
    case 16:
        pos[0] = pl->pos[0];
        pos[1] = pl->floorY;
        pos[2] = pl->pos[2];
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = -50.0f;
        Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
        pos[0] += offset[0];
        pos[1] += offset[1];
        pos[2] += offset[2];
        break;
    case 17:
        Em_GetBonePos(pl, bone, pos);
        pos[1] = pl->floorY;
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = -25.0f;
        Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
        Vec_Add(pos, pos, offset);
        break;
    case 21:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
        case WEAPON_LONG_SWORD:
            offset[0] = 10.0f;
            offset[1] = -2.0f;
            offset[2] = 190.0f;
            break;
        case WEAPON_SWORD_SHIELD:
            offset[0] = -4.9f;
            offset[1] = -0.7f;
            offset[2] = 90.0f;
            break;
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
            offset[0] = -7.8f;
            offset[1] = -10.0f;
            offset[2] = 220.0f;
            break;
        case WEAPON_HAMMER:
            offset[0] = -5.0f;
            offset[1] = -20.0f;
            offset[2] = 155.0f;
            break;
        case WEAPON_HUNTING_HORN:
            offset[0] = 0.0f;
            offset[1] = -50.0f;
            offset[2] = 180.0f;
            break;
        case WEAPON_DUAL_SWORDS:
            offset[0] = -4.9f;
            offset[1] = -0.7f;
            offset[2] = 90.0f;
            break;
        default:
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 0.0f;
            break;
        }
        Vec_RotateSelf(offset, boneMtx[0]);
        pos[0] = boneMtx[3][0] + offset[0];
        pos[1] = boneMtx[3][1] + offset[1];
        pos[2] = boneMtx[3][2] + offset[2];
        break;
    case 22:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        offset[0] = 0.0f;
        offset[1] = -2.0f;
        offset[2] = 16.0f;
        Vec_RotateSelf(offset, boneMtx[0]);
        pos[0] = boneMtx[3][0] + offset[0];
        pos[1] = boneMtx[3][1] + offset[1];
        pos[2] = boneMtx[3][2] + offset[2];
        break;
    case 23:
    case 35:
        Em_GetBonePos(pl, bone, pos);
        break;
    case 24:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        offset[0] = -5.0f;
        offset[1] = 0.0f;
        offset[2] = 50.0f;
        Vec_RotateSelf(offset, boneMtx[0]);
        pos[0] = boneMtx[3][0] + offset[0];
        pos[1] = pl->floorY;
        pos[2] = boneMtx[3][2] + offset[2];
        break;
    case 25:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 125.0f;
        Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
        pos[0] = pl->pos[0] + offset[0];
        pos[1] = pl->pos[1] + offset[1];
        pos[2] = pl->pos[2] + offset[2];
        break;
    case 26:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        offset[0] = 5.0f;
        offset[1] = -15.0f;
        offset[2] = 10.0f;
        Vec_RotateSelf(offset, boneMtx[0]);
        pos[0] = boneMtx[3][0] + offset[0];
        pos[1] = boneMtx[3][1] + offset[1];
        pos[2] = boneMtx[3][2] + offset[2];
        break;
    case 31:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_GetBone(pl, bone)));
        offset[0] = 44.0f;
        offset[1] = 0.0f;
        offset[2] = 123.0f;
        Vec_RotateSelf(offset, boneMtx[0]);
        pos[0] = boneMtx[3][0] + offset[0];
        pos[1] = boneMtx[3][1] + offset[1];
        pos[2] = boneMtx[3][2] + offset[2];
        groundY = AreaGrid2_GetFloorY(pos);
        if (pos[1] < 50.0f + groundY) {
            pos[1] = groundY;
        }
        break;
    case 33:
        pos[0] = 13446.23f;
        pos[1] = 70.0f;
        pos[2] = 14372.39f;
        break;
    case 36:
        Em_GetBonePos(pl, bone, pos);
        pos[1] = 90.0f + pl->floorY;
        break;
    case 37:
    case 38:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 130.0f;
        Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
        pos[0] = pl->pos[0] + offset[0];
        pos[1] = pl->floorY;
        pos[2] = pl->pos[2] + offset[2];
        break;
    }
    groundKind = (s16)PartFx_PutOnGround(pl, pos, type);
    if ((s16)type == 19 && (s16)groundKind == 0) {
        offset[0] = pos[0];
        offset[1] = 20.0f + pos[1];
        offset[2] = pos[2];
        AttackBurst_CreateAtScaled(offset, pl->rot[1], 10, 0.4f);
        PartFx_SpawnForPlayerScaled(pl, 14, 14, 1.0f);
    }
    return groundKind;
}

/*
 * When pl is in the current area: the ground puffs of the type's ground
 * kind at its place on pl (PartFx_PlayerPos), or, when they make
 * nothing, a STAGEOBJ_KIND_PART_FX effect object of the given type there.
 */
void PartFx_SpawnForPlayer(Player *pl, s16 bone, s32 type)
{
    f32 pos[3];
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(pl) != 0 && Effect_SpawnGround(pl, pos, PartFx_PlayerPos(pos, pl, bone, type), 1.0f) == 0) {
        if ((fx = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(fx->pos, pos);
            fx->scale = 1.0f;
            PartFx_InitForPlayer(pl, bone, type, fx);
        }
    }
}

/* PartFx_SpawnForPlayer at the given size (the ground puffs keep size 1). */
void PartFx_SpawnForPlayerScaled(Player *pl, s16 bone, s32 type, f32 scale)
{
    f32 pos[3];
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(pl) != 0 && Effect_SpawnGround(pl, pos, PartFx_PlayerPos(pos, pl, bone, type), 1.0f) == 0) {
        if ((fx = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(fx->pos, pos);
            PartFx_InitForPlayer(pl, bone, type, fx);
            fx->scale = scale;
        }
    }
}

/* unit's own declaration: with an s32 bone, though the function takes an s16: PartFx_EmPos and PartFx_InitForEm build as the game's code only with this declaration. */
EmBone *Em_Bone(void *obj, s32 a);
u8 Em_TypeUsesK14(u8 type); /* unit's own declaration: the definition (game) is s32 Em_TypeUsesK14(u8 type); the calls here match only with this one */
void RathalosAttack_CreateOnObj(StageObj *s, s8 kind);
/*
 * Makes fx a STAGEOBJ_KIND_PART_FX effect object of the given type that follows em,
 * facing its heading. Types 0, 3, 6, 7, 19/40 and 20 take a size by em's
 * kind and scale. Type 7 at bone 2 of the Rathalos family adds a Rathalos
 * attack object (47 for the kinds Em_TypeUsesK14 names, else 14); types
 * 19 and 40 on Lao-Shan Lung add its attack object 80. Types 28 (the
 * trader, or kind 1 in the town) and 30 face across the bone's heading.
 */
void PartFx_InitForEm(Em *em, s32 bone, s32 type, StageObj *fx)
{
    f32 boneMtx[4][4];

    fx->kind = STAGEOBJ_KIND_PART_FX;
    fx->update = PartFx_Update;
    fx->type = type;
    fx->angle = em->rotY;
    fx->owner = em;
    switch (fx->type) {
    case 0:
        switch (em->type) {
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
            fx->scale = 4.0f * em->scaleX;
            break;
        case EM_KIND_FELYNE_CART:
            fx->scale = 0.5f * em->scaleX;
            break;
        case EM_KIND_VELOCIPREY:
        case EM_KIND_GIAPREY:
        case EM_KIND_GENPREY:
        case EM_KIND_VELOCIDROME:
        case EM_KIND_GENDROME:
        case EM_KIND_IOPREY:
        case EM_KIND_IODROME:
            fx->scale = 2.0f * em->scaleX;
            break;
        }
        break;
    case 3:
        switch (em->type) {
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
        case EM_KIND_BULLDROME:
        case EM_KIND_PIGLET:
            break;
        case EM_KIND_FELYNE:
        case EM_KIND_MELYNX:
            fx->scale = 2.0f * em->scaleX;
            break;
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
        case EM_KIND_DIABLOS:
        case EM_KIND_BLACK_DIABLOS:
        case EM_KIND_MONOBLOS:
        case EM_KIND_WHITE_MONOBLOS:
        case EM_KIND_KELBI:
        case EM_KIND_ANTEKA:
        case EM_KIND_KIRIN:
        case EM_KIND_BLANGONGA:
        case EM_KIND_CONGALALA:
        case EM_KIND_RAJANG:
        case EM_KIND_BLANGO:
        case EM_KIND_CONGA:
            fx->scale = em->scaleX;
            break;
        }
        break;
    case 6:
        switch (em->type) {
        case EM_KIND_RATHIAN:
        case EM_KIND_GOLD_RATHIAN:
        case EM_KIND_PINK_RATHIAN:
        case EM_KIND_YIAN_KUT_KU:
        case EM_KIND_BLUE_YIAN_KUT_KU:
        case EM_KIND_RATHALOS:
        case EM_KIND_SILVER_RATHALOS:
        case EM_KIND_AZURE_RATHALOS:
        case EM_KIND_DIABLOS:
        case EM_KIND_BLACK_DIABLOS:
        case EM_KIND_MONOBLOS:
        case EM_KIND_WHITE_MONOBLOS:
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
        case EM_KIND_GRAVIOS:
        case EM_KIND_BLACK_GRAVIOS:
        case EM_KIND_GYPCEROS:
        case EM_KIND_PURPLE_GYPCEROS:
        case EM_KIND_PLESIOTH:
        case EM_KIND_GREEN_PLESIOTH:
        case EM_KIND_YIAN_GARUGA:
            fx->scale = 1.3f * em->scaleX;
            break;
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
            fx->scale = 2.0f * em->scaleX;
            break;
        case EM_KIND_FELYNE_CART:
            fx->scale = em->scaleX;
            break;
        }
        break;
    case 7:
        switch (em->type) {
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
        case EM_KIND_BULLDROME:
        case EM_KIND_PIGLET:
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
            fx->scale = em->scaleX;
            break;
        case EM_KIND_FELYNE:
        case EM_KIND_MELYNX:
            fx->scale = 0.5f * em->scaleX;
            break;
        case EM_KIND_FELYNE_CART:
            if ((s16)bone == 0) {
                fx->scale = 2.0f * em->scaleX;
            } else {
                fx->scale = 0.5f * em->scaleX;
            }
            break;
        case EM_KIND_GRAVIOS:
        case EM_KIND_BLACK_GRAVIOS:
        case EM_KIND_BASARIOS:
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
        case EM_KIND_DIABLOS:
        case EM_KIND_BLACK_DIABLOS:
        case EM_KIND_MONOBLOS:
        case EM_KIND_WHITE_MONOBLOS:
        case EM_KIND_RATHALOS:
        case EM_KIND_SILVER_RATHALOS:
        case EM_KIND_AZURE_RATHALOS:
        case EM_KIND_RATHIAN:
        case EM_KIND_GOLD_RATHIAN:
        case EM_KIND_PINK_RATHIAN:
        case EM_KIND_GYPCEROS:
        case EM_KIND_PURPLE_GYPCEROS:
            fx->scale = em->scaleX;
            if ((s16)bone == 2) {
                if (Em_TypeUsesK14(em->type) == 0) {
                    RathalosAttack_CreateOnObj(fx, 14);
                } else {
                    RathalosAttack_CreateOnObj(fx, 47);
                }
            }
            break;
        }
        break;
    case 19:
    case 40:
        switch (em->type) {
        case EM_KIND_DIABLOS:
        case EM_KIND_BLACK_DIABLOS:
        case EM_KIND_MONOBLOS:
        case EM_KIND_WHITE_MONOBLOS:
        case EM_KIND_BLANGONGA:
        case EM_KIND_CONGALALA:
        case EM_KIND_RAJANG:
        case EM_KIND_BLANGO:
        case EM_KIND_CONGA:
            fx->scale = em->scaleX;
            break;
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
            LaoShanAttack_CreateOnObj(fx, 80);
            break;
        }
        /* no break: the game's code goes on into type 20's test (it only
         * changes kinds that types 19 and 40 do not list) */
    case 20:
        switch (em->type) {
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
            fx->scale = 5.0f * em->scaleX;
            break;
        }
        break;
    case 28:
        if ((gScene.inLobby != 0 && em->type == 1) || (gScene.inLobby == 0 && em->type == EM_KIND_TRADER)) {
            Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_Bone(em, bone)));
            fx->angle = Mtx_Heading(boneMtx[0]) + ANGLE_TURN / 4;
        }
        break;
    case 30:
        Mtx_Copy((f32 *) boneMtx[0], (f32 *) (Em_Bone(em, bone)));
        fx->angle = Mtx_Heading(boneMtx[0]) - ANGLE_TURN / 4;
        break;
    }
}

/* unit's own declaration: with an s32 bone, though the function takes an s16: PartFx_EmPos builds as the game's code only with this declaration. */
void Em_BonePos(void *obj, s32 part, f32 *out);
/*
 * Sets pos to where an effect of the given type goes on em: mostly bone
 * `bone` at em's floor height, with offsets by em's kind for some types.
 * Then puts it on the ground (PartFx_PutOnGround) and returns the
 * ground kind.
 */
s32 PartFx_EmPos(f32 *pos, Em *em, s32 bone, s32 fxType)
{
    f32 boneMtx[16];
    f32 offset[3];

    switch ((s16)fxType) {
    case 0:
        switch (em->type) {
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = -10.0f;
            Vec_RotateY(offset, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += offset[0];
            pos[1] += offset[1];
            pos[2] += offset[2];
            break;
        case EM_KIND_FELYNE_CART:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = -20.0f;
            Vec_RotateY(offset, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += offset[0];
            pos[1] += offset[1];
            pos[2] += offset[2];
            break;
        default:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            break;
        }
        break;
    case 3:
        switch (em->type) {
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
        case EM_KIND_BULLDROME:
        case EM_KIND_PIGLET:
            if ((s16)bone == 17) {
                Em_BonePos(em, bone, pos);
                Em_BonePos(em, 14, offset);
                Vec_Add(pos, pos, offset);
                Vec_Scale(pos, pos, 0.5f);
            } else {
                Em_BonePos(em, bone, pos);
            }
            pos[1] = em->floorY;
            break;
        case EM_KIND_FELYNE:
        case EM_KIND_MELYNX:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            offset[0] = 0.02f * ((Rand_Next(1) & 0x3FF) - 512);
            offset[1] = 0.0f;
            offset[2] = 0.0f;
            Vec_RotateY(offset, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += offset[0];
            pos[1] += offset[1];
            pos[2] += offset[2];
            break;
        default:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            break;
        }
        break;
    case 6:
        switch (em->type) {
        default:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            break;
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
            pos[0] = em->posX;
            pos[1] = em->floorY;
            pos[2] = em->posZ;
            break;
        }
        break;
    case 7:
        switch (em->type) {
        default:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            break;
        case EM_KIND_FELYNE:
        case EM_KIND_MELYNX:
            Em_BonePos(em, bone, pos);
            break;
        case EM_KIND_GRAVIOS:
        case EM_KIND_BLACK_GRAVIOS:
        case EM_KIND_BASARIOS:
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, bone)));
            offset[0] = 100.0f;
            offset[1] = 0.0f;
            offset[2] = 0.0f;
            Vec_RotateSelf(offset, boneMtx);
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            pos[0] += offset[0];
            pos[2] += offset[2];
            break;
        }
        break;
    case 19:
    case 40:
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        break;
    case 20:
        switch (em->type) {
        default:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            break;
        case EM_KIND_HERBIVORE_0:
        case EM_KIND_POPO:
        case EM_KIND_APCEROS:
            Em_BonePos(em, bone, pos);
            pos[1] = em->floorY;
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = -10.0f;
            Vec_RotateY(offset, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += offset[0];
            pos[1] += offset[1];
            pos[2] += offset[2];
            break;
        }
        break;
    case 28:
        if ((gScene.inLobby != 0 && em->type == 1) || (gScene.inLobby == 0 && em->type == EM_KIND_TRADER)) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, bone)));
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 15.0f;
            Vec_RotateSelf(offset, boneMtx);
            Mtx_GetTrans(pos, boneMtx);
            pos[0] += offset[0];
            pos[1] += offset[1];
            pos[2] += offset[2];
        }
        break;
    case 30:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, bone)));
        offset[0] = 0.0f;
        offset[1] = -14.0f;
        offset[2] = 45.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(pos, boneMtx);
        pos[0] += offset[0];
        pos[1] += offset[1];
        pos[2] += offset[2];
        break;
    case 32:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, bone)));
        offset[0] = 58.2f;
        offset[1] = 0.0f;
        offset[2] = -474.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(pos, boneMtx);
        pos[0] += offset[0];
        pos[1] = em->floorY;
        pos[2] += offset[2];
        break;
    }
    return (s16)PartFx_PutOnGround((Player *)em, pos, fxType);
}

/*
 * When em is in the current area: the ground puffs of the type's ground
 * kind at its place on em (PartFx_EmPos), or, when they make nothing, a
 * STAGEOBJ_KIND_PART_FX effect object of the given type there.
 */
void PartFx_SpawnForEm(Em *em, s32 bone, s32 fxType)
{
    f32 pos[3];
    StageObj *fx;

    if (Player_InCurrentArea(em) != 0 && Effect_SpawnGround(em, pos, PartFx_EmPos(pos, em, bone, fxType), 1.0f) == 0) {
        if ((fx = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(fx->pos, pos);
            fx->scale = 1.0f;
            PartFx_InitForEm(em, bone, fxType, fx);
        }
    }
}

/* PartFx_SpawnForEm at the given size; the effect object is also scaled by em's size. */
void PartFx_SpawnForEmScaled(Em *em, s32 bone, s32 fxType, f32 scale)
{
    f32 pos[3];
    StageObj *fx;

    if (Player_InCurrentArea(em) != 0 && Effect_SpawnGround(em, pos, PartFx_EmPos(pos, em, bone, fxType), scale) == 0) {
        if ((fx = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(fx->pos, pos);
            PartFx_InitForEm(em, bone, fxType, fx);
            fx->scale = scale * em->scaleX;
        }
    }
}

/* A STAGEOBJ_KIND_PART_FX effect object of the given type at pos (PartFx_Update updates it). */
void PartFx_SpawnAt(f32 *pos, s32 type, f32 scale)
{
    StageObj *fx;

    if ((fx = StageObj_Alloc(1)) != NULL) {
        fx->kind = STAGEOBJ_KIND_PART_FX;
        fx->update = PartFx_Update;
        fx->owner = NULL;
        fx->type = type;
        Vec_Copy(fx->pos, pos);
        fx->scale = scale;
    }
}

/* PartFx_SpawnAt when owner is in the current area. */
void PartFx_SpawnIfHere(void *owner, f32 *pos, s32 type, f32 scale)
{
    if ((u8)Obj_InCurrentArea(owner) != 0) {
        PartFx_SpawnAt(pos, type, scale);
    }
}

/* The ground puffs of the type's ground kind at pos, or, when they make nothing, PartFx_SpawnAt. */
void PartFx_SpawnOnGround(f32 *pos, s32 type, f32 scale)
{
    if (Effect_SpawnGround(NULL, pos, PartFx_PutOnGround(NULL, pos, type), scale) == 0) {
        PartFx_SpawnAt(pos, type, scale);
    }
}

/*
 * Spawns the ground effects of ground kind `groundKind` at pos (kind 8
 * effects: dust, splashes and the like, two of them a frame apart in
 * kinds 2 and 8) and raises pos by 5. Kind 1 (and any unknown kind) plays
 * the landing playSound 46; the others play step playSound 118 when the puffs'
 * scale is above 4, else 112, except kind 4 and, seven frames in eight,
 * kind 5. Kind 5 follows its owner (an Em). Returns 0 for kind 0 (no
 * ground effect) and for kind 5 without an owner, else 1.
 */
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 groundKind, f32 scale)
{
    s32 playSound = 1;
    f32 puffScale;

    switch (groundKind) {
    case 0:
        return 0;
    case 1:
    default:
        puffScale = 1.2f * scale;
        SparkFx_Spawn(pos, 3, 0, puffScale);
        pos[1] += 5.0f;
        SparkFx_Spawn(pos, 0, 0, puffScale);
        Sound_PlaySeAtPosChecked(1, 46, 0, pos, 1, 0);
        return 1;
    case 2:
        if (gScene.frame & 2) {
            SparkFx_Spawn(pos, 4, 0, 1.5f * scale);
        }
        pos[1] += 5.0f;
        puffScale = 2.0f * scale;
        SparkFx_Spawn(pos, 2, 0, puffScale);
        break;
    case 3:
        puffScale = 1.5f * scale;
        SparkFx_Spawn(pos, 4, 0, puffScale);
        pos[1] += 5.0f;
        SparkFx_Spawn(pos, 5, 0, puffScale);
        break;
    case 4:
        playSound = 0;
    case 9:
        puffScale = 2.7f * scale;
        pos[1] += 5.0f;
        SparkFx_Spawn(pos, 1, 0, puffScale);
        break;
    case 5:
        if (owner == NULL) {
            return 0;
        }
        puffScale = 2.0f * scale;
        pos[1] += 5.0f;
        SparkFx_SpawnAtEm(owner, 6, 0, puffScale, pos[1]);
        if (!(gScene.frame & 7)) {
            playSound = 0;
        }
        break;
    case 6:
        puffScale = 2.5f * scale;
        SparkFx_Spawn(pos, 4, 0, puffScale);
        pos[1] += 5.0f;
        SparkFx_Spawn(pos, 5, 0, puffScale);
        break;
    case 7:
        puffScale = 3.0f * scale;
        SparkFx_Spawn(pos, 4, 0, puffScale);
        pos[1] += 5.0f;
        SparkFx_Spawn(pos, 5, 0, puffScale);
        break;
    case 8:
        if (gScene.frame & 4) {
            SparkFx_Spawn(pos, 4, 0, 1.5f * scale);
        }
        puffScale = 2.0f * scale;
        pos[1] += 5.0f;
        SparkFx_Spawn(pos, 2, 0, puffScale);
        break;
    }
    if (playSound != 0) {
        if (puffScale > 4.0f) {
            Sound_PlaySeAtPosChecked(1, 118, 0, pos, 1, 0);
        } else {
            Sound_PlaySeAtPosChecked(1, 112, 0, pos, 1, 0);
        }
    }
    return 1;
}

/*
 * When the player is in the current area: a STAGEOBJ_KIND_PART_FX effect of type `kind`
 * on the ground (at the player's floor height) below its weapon model's
 * bone 3. Type 19 adds an attack burst along the player's heading and a
 * type 14 effect. The calls take only the low byte of Obj_InCurrentArea's
 * result and of `kind` (the casts), as the game's code does.
 */
void Player_SpawnGroundEffect(Player *pl, s32 kind, f32 scale)
{
    f32 pos[3];

    if ((u8)Obj_InCurrentArea(pl) != 0) {
        Mtx_GetTrans(pos, pl->weaponModel->partBones[0][3].world);
        pos[1] = pl->floorY;
        PartFx_SpawnOnGround(pos, (u8)kind, scale);
        if ((u8)kind == 19) {
            AttackBurst_CreateAtScaled(pos, *(u16 *)&pl->rot[1], 10, 0.3f);
            PartFx_SpawnOnGround(pos, 14, 0.9f);
        }
    }
}

/* The .data: the parts' key curves, then the tables by type. */
f32 gPartFxKeys_0[20] = {
#include "sub_main/00466940.inc"
};
Vec3Key gPartFxKeys_1[4] = {
#include "sub_main/00466990.inc"
};
FloatKey gPartFxKeys_2[4] = {
#include "sub_main/004669D0.inc"
};
Vec3Key gPartFxKeys_3[6] = {
#include "sub_main/004669F0.inc"
};
Vec3Key gPartFxKeys_4[5] = {
#include "sub_main/00466A50.inc"
};
FloatKey gPartFxKeys_5[6] = {
#include "sub_main/00466AA0.inc"
};
Vec3Key gPartFxKeys_6[6] = {
#include "sub_main/00466AD0.inc"
};
Vec3Key gPartFxKeys_7[5] = {
#include "sub_main/00466B30.inc"
};
FloatKey gPartFxKeys_8[6] = {
#include "sub_main/00466B80.inc"
};
Vec3Key gPartFxKeys_9[6] = {
#include "sub_main/00466BB0.inc"
};
Vec3Key gPartFxKeys_10[5] = {
#include "sub_main/00466C10.inc"
};
FloatKey gPartFxKeys_11[6] = {
#include "sub_main/00466C60.inc"
};
Vec3Key gPartFxKeys_12[6] = {
#include "sub_main/00466C90.inc"
};
Vec3Key gPartFxKeys_13[4] = {
#include "sub_main/00466CF0.inc"
};
FloatKey gPartFxKeys_14[4] = {
#include "sub_main/00466D30.inc"
};
Vec3Key gPartFxKeys_15[5] = {
#include "sub_main/00466D50.inc"
};
Vec3Key gPartFxKeys_16[4] = {
#include "sub_main/00466DA0.inc"
};
FloatKey gPartFxKeys_17[4] = {
#include "sub_main/00466DE0.inc"
};
Vec3Key gPartFxKeys_18[6] = {
#include "sub_main/00466E00.inc"
};
Vec3Key gPartFxKeys_19[3] = {
#include "sub_main/00466E60.inc"
};
FloatKey gPartFxKeys_20[6] = {
#include "sub_main/00466E90.inc"
};
Vec3Key gPartFxKeys_21[4] = {
#include "sub_main/00466EC0.inc"
};
Vec3Key gPartFxKeys_22[4] = {
#include "sub_main/00466F00.inc"
};
FloatKey gPartFxKeys_23[4] = {
#include "sub_main/00466F40.inc"
};
Vec3Key gPartFxKeys_24[4] = {
#include "sub_main/00466F60.inc"
};
Vec3Key gPartFxKeys_27[4] = {
#include "sub_main/00466FA0.inc"
};
Vec3Key gPartFxKeys_30[4] = {
#include "sub_main/00466FE0.inc"
};
Vec3Key gPartFxKeys_25[4] = {
#include "sub_main/00467020.inc"
};
Vec3Key gPartFxKeys_28[7] = {
#include "sub_main/00467060.inc"
};
Vec3Key gPartFxKeys_31[6] = {
#include "sub_main/004670D0.inc"
};
FloatKey gPartFxKeys_26[4] = {
#include "sub_main/00467130.inc"
};
FloatKey gPartFxKeys_29[4] = {
#include "sub_main/00467150.inc"
};
FloatKey gPartFxKeys_32[4] = {
#include "sub_main/00467170.inc"
};
s16 gPartFxType14Starts[4] = {
#include "sub_main/00467190.inc"
};
u16 gPartFxType14Lens[4] = {
#include "sub_main/00467198.inc"
};
Vec3Key gPartFxKeys_33[4] = {
#include "sub_main/004671A0.inc"
};
Vec3Key gPartFxKeys_34[4] = {
#include "sub_main/004671E0.inc"
};
FloatKey gPartFxKeys_35[4] = {
#include "sub_main/00467220.inc"
};
Vec3Key gPartFxKeys_36[6] = {
#include "sub_main/00467240.inc"
};
Vec3Key gPartFxKeys_37[4] = {
#include "sub_main/004672A0.inc"
};
FloatKey gPartFxKeys_38[6] = {
#include "sub_main/004672E0.inc"
};
s16 gPartFxType19Starts[8] = {
#include "sub_main/00467310.inc"
};
Vec3Key gPartFxKeys_39[4] = {
#include "sub_main/00467320.inc"
};
FloatKey gPartFxKeys_40[4] = {
#include "sub_main/00467360.inc"
};
Vec3Key gPartFxKeys_41[4] = {
#include "sub_main/00467380.inc"
};
Vec3Key gPartFxKeys_43[4] = {
#include "sub_main/004673C0.inc"
};
Vec3Key gPartFxKeys_45[4] = {
#include "sub_main/00467400.inc"
};
Vec3Key gPartFxKeys_47[5] = {
#include "sub_main/00467440.inc"
};
Vec3Key gPartFxKeys_49[4] = {
#include "sub_main/00467490.inc"
};
Vec3Key gPartFxKeys_51[4] = {
#include "sub_main/004674D0.inc"
};
FloatKey gPartFxKeys_42[4] = {
#include "sub_main/00467510.inc"
};
FloatKey gPartFxKeys_44[4] = {
#include "sub_main/00467530.inc"
};
FloatKey gPartFxKeys_46[4] = {
#include "sub_main/00467550.inc"
};
FloatKey gPartFxKeys_50[4] = {
#include "sub_main/00467570.inc"
};
u16 gPartFxType21Lens[8] = {
#include "sub_main/00467590.inc"
};
Vec3Key gPartFxKeys_53[5] = {
#include "sub_main/004675A0.inc"
};
Vec3Key gPartFxKeys_54[4] = {
#include "sub_main/004675F0.inc"
};
FloatKey gPartFxKeys_55[4] = {
#include "sub_main/00467630.inc"
};
u8 gPartFxOfferColors[10][3] = {
#include "sub_main/00467650.inc"
};
Vec3Key gPartFxKeys_56[5] = {
#include "sub_main/00467670.inc"
};
FloatKey gPartFxKeys_57[4] = {
#include "sub_main/004676C0.inc"
};
Vec3Key gPartFxKeys_58[5] = {
#include "sub_main/004676E0.inc"
};
Vec3Key gPartFxKeys_60[4] = {
#include "sub_main/00467730.inc"
};
Vec3Key gPartFxKeys_62[4] = {
#include "sub_main/00467770.inc"
};
Vec3Key gPartFxKeys_68[3] = {
#include "sub_main/004677B0.inc"
};
FloatKey gPartFxKeys_59[4] = {
#include "sub_main/004677E0.inc"
};
FloatKey gPartFxKeys_61[4] = {
#include "sub_main/00467800.inc"
};
FloatKey gPartFxKeys_63[4] = {
#include "sub_main/00467820.inc"
};
FloatKey gPartFxKeys_69[3] = {
#include "sub_main/00467840.inc"
};
s16 gPartFxType35Starts[6] = {
#include "sub_main/00467858.inc"
};
u16 gPartFxType35Lens[6] = {
#include "sub_main/00467868.inc"
};
Vec3Key gPartFxKeys_70[4] = {
#include "sub_main/00467880.inc"
};
Vec3Key gPartFxKeys_71[4] = {
#include "sub_main/004678C0.inc"
};
FloatKey gPartFxKeys_72[4] = {
#include "sub_main/00467900.inc"
};
s16 gPartFxType36Starts[8] = {
#include "sub_main/00467920.inc"
};
void *gPartFxKeys[76] = {
    (void *)gPartFxKeys_0, (void *)gPartFxKeys_1, (void *)gPartFxKeys_2, (void *)gPartFxKeys_3,
    (void *)gPartFxKeys_4, (void *)gPartFxKeys_5, (void *)gPartFxKeys_6, (void *)gPartFxKeys_7,
    (void *)gPartFxKeys_8, (void *)gPartFxKeys_9, (void *)gPartFxKeys_10, (void *)gPartFxKeys_11,
    (void *)gPartFxKeys_12, (void *)gPartFxKeys_13, (void *)gPartFxKeys_14, (void *)gPartFxKeys_15,
    (void *)gPartFxKeys_16, (void *)gPartFxKeys_17, (void *)gPartFxKeys_18, (void *)gPartFxKeys_19,
    (void *)gPartFxKeys_20, (void *)gPartFxKeys_21, (void *)gPartFxKeys_22, (void *)gPartFxKeys_23,
    (void *)gPartFxKeys_24, (void *)gPartFxKeys_25, (void *)gPartFxKeys_26, (void *)gPartFxKeys_27,
    (void *)gPartFxKeys_28, (void *)gPartFxKeys_29, (void *)gPartFxKeys_30, (void *)gPartFxKeys_31,
    (void *)gPartFxKeys_32, (void *)gPartFxKeys_33, (void *)gPartFxKeys_34, (void *)gPartFxKeys_35,
    (void *)gPartFxKeys_36, (void *)gPartFxKeys_37, (void *)gPartFxKeys_38, (void *)gPartFxKeys_39,
    (void *)gPartFxKeys_40, (void *)gPartFxKeys_41, (void *)gPartFxKeys_42, (void *)gPartFxKeys_43,
    (void *)gPartFxKeys_44, (void *)gPartFxKeys_45, (void *)gPartFxKeys_46, (void *)gPartFxKeys_47,
    (void *)gPartFxKeys_46, (void *)gPartFxKeys_49, (void *)gPartFxKeys_50, (void *)gPartFxKeys_51,
    (void *)gPartFxKeys_50, (void *)gPartFxKeys_53, (void *)gPartFxKeys_54, (void *)gPartFxKeys_55,
    (void *)gPartFxKeys_56, (void *)gPartFxKeys_57, (void *)gPartFxKeys_58, (void *)gPartFxKeys_59,
    (void *)gPartFxKeys_60, (void *)gPartFxKeys_61, (void *)gPartFxKeys_62, (void *)gPartFxKeys_63,
    (void *)gPartFxKeys_62, (void *)gPartFxKeys_63, (void *)gPartFxKeys_62, (void *)gPartFxKeys_63,
    (void *)gPartFxKeys_68, (void *)gPartFxKeys_69, (void *)gPartFxKeys_70, (void *)gPartFxKeys_71,
    (void *)gPartFxKeys_72, NULL, NULL, NULL,
};
s16 gPartFxPartCounts[48] = {
#include "sub_main/00467A60.inc"
};
u16 gPartFxLens[48] = {
#include "sub_main/00467AC0.inc"
};
s16 gPartFxKeyStarts[48] = {
#include "sub_main/00467B20.inc"
};
s16 gPartFxKeyStrides[48] = {
#include "sub_main/00467B80.inc"
};
s16 gPartFxGroundKinds[48] = {
#include "sub_main/00467BE0.inc"
};
