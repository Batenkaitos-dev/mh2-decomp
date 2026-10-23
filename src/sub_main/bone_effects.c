/*
 * The kind 6 effect objects (StageObj kind 6, made by Effect06_New and the
 * Spawn functions): small effects that follow a hunter's or a monster's bone.
 * An object holds gEff06PartCounts[type] parts (EffectPart, in its buffer);
 * each part has its own draw slot, and Effect06_DrawPart draws it with a model
 * of the common set. The types, as the callers make them: 0 an item's glow
 * (var0 an ITEM_GLOW_* colour; it also makes type 1), 2 the glow when a skill
 * cuts a hit's damage, 3 the poison effect, 4 the sleep puffs, 5 the status 4
 * effect, 6 the hammer's and the bow's charge, 7 the attack and defense up
 * icons over a hunter, 9 a hit's effect, 10 the gourmet steak's sparkle.
 * Effect06_Run moves and animates the parts along their scale and colour key
 * curves (gEffect06PosKeys and the colour keys).
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"
#include "render_state.h"
#include "sound.h"
#include "objhead.h"
#include "card.h"
#include "bone_effects.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "lock_on_mark.h"
#include "part_effects.h"
#include "rand.h"
#include "area.h"
#include "vec_math.h"

/* A part of a kind 6 effect object (StageObj.buf, 56 bytes each). */
typedef struct EffectPart {
    s16 slot;                   /* 0x00: its draw slot */
    s16 variant;                /* 0x02: picks the part's model (Effect06_DrawPart); type 2: +4 each round (Effect06_StepPart); 255: done */
    f32 keyScale[3];            /* 0x04: the scale per axis from the scale keys (Key_LerpVec3), times scale */
    f32 pos[3];                 /* 0x10 */
    s32 color;                  /* 0x1C: 0xAARRGGBB from the colour keys (Color_EvalKeys); -1 by default */
    u16 ang[3];                 /* 0x20: random (Effect06_RandomizePart) */
    s16 spin;                   /* 0x26: added to ang[2] every frame (Effect06_Run) */
    f32 scale;                  /* 0x28 */
    DrawSlot *draw;             /* 0x2C: DrawSlot_Get(slot), NULL when none */
    s16 age;                    /* 0x30: frames since it started, counted up each frame; 0 or less: waiting to start */
    u16 index;                  /* 0x32: the object's timer when the part was made */
    u16 hasColorKeys;           /* 0x34: nonzero: Effect06_Run evaluates its colour keys */
    u16 kind;                   /* 0x36: the object's var2 when the part started (type 6: the charge level) */
} EffectPart;

/* prototypes */
/* the calls cast to u8: this unit's code takes the value as u8 */
/* end of prototypes */

/* The update of the kind 6 effect objects (Effect06_New), by state: set up, run, end, free. */
void Effect06_Update(StageObj *effect)
{
    switch (effect->state) {
    case 0:
        Effect06_Setup(effect);
        break;
    case 1:
        Effect06_Run(effect);
        break;
    case 2:
        Effect06_End(effect);
        break;
    case 3:
        Effect06_Delete(effect);
        break;
    }
}

extern s16 gEff06PartCounts[];
extern s16 gEff06PartAges3[];
extern s16 gEff06PartAges4[];
/* The set up of a kind 6 effect object: its parts (gEff06PartCounts: the count of each type). */
void Effect06_Setup(StageObj *effect)
{
    EffectPart *part = (EffectPart *)effect->buf;
    Em *em = effect->owner;
    s16 n;
    s16 i;

    effect->state++;
    effect->visible = 1;
    effect->draw = NULL;
    effect->timer = 0;
    effect->var2 = 0;
    n = gEff06PartCounts[effect->type];
    switch (effect->type) {
    case 0:
    case 6:
        Effect06_PlaySound(effect, effect->pos);
        break;
    case 4:
        effect->angle = Rand_Next(1);
        effect->scale -= 0.00040000002f * (Rand_Next(1) & 0x3FF);
        break;
    case 7:
        effect->angle = 2;
        for (i = 0; i < n; i++, part++) {
            part->age = (i * 240) / 2;
            part->index = (i * 60) / 2;
            part->scale = effect->scale;
            Vec_Copy(part->pos, effect->pos);
            part->color = -1;
            part->ang[1] = Rand_Next(1);
            part->variant = 0;
            part->slot = DrawSlotB_Alloc();
            if (part->slot != -1) {
                part->draw = DrawSlotB_Get(part->slot);
                part->draw->owner = effect;
                part->draw->part = i;
                part->draw->draw = Effect06_DrawPart;
            } else {
                part->draw = NULL;
            }
        }
        return;
    case 8:
        effect->angle = Rand_Next(1);
        effect->scale -= 0.00040000002f * (Rand_Next(1) & 0x3FF);
        effect->pos[0] = 0.0f;
        effect->pos[1] = 20.0f;
        effect->pos[2] = 0.0f;
        break;
    case 10:
        Effect06_PlaceAtBone(effect);
        break;
    default:
        effect->angle = em->rotY;
        break;
    }
    for (i = 0; i < n; part++, i++) {
        part->age = 0;
        part->scale = effect->scale;
        Vec_Copy(part->pos, effect->pos);
        part->color = -1;
        switch (effect->type) {
        case 0:
            part->ang[0] = 0;
            part->ang[1] = 0;
            if (i == 2) {
                part->hasColorKeys = 0;
                part->ang[2] = 0;
                part->spin = 0;
                part->variant = 1;
            } else {
                if (i == 0 || i == 1) {
                    part->variant = 0;
                } else {
                    part->variant = 2;
                }
                part->hasColorKeys = 1;
                part->ang[2] = 0;
                part->spin = 0;
            }
            break;
        case 1:
            part->ang[0] = Rand_Next(1);
            part->ang[1] = Rand_Next(1);
            part->ang[2] = 0;
            part->spin = -5040;
            part->hasColorKeys = 1;
            part->variant = i;
            part->age = i * -3;
            part->scale += 0.1f * i;
            break;
        case 2:
            part->variant = i;
            part->hasColorKeys = 0;
            part->age = i * -2;
            break;
        case 3:
            part->scale = effect->scale * (1.0f + 0.00020000001f * (Rand_Next(1) & 0x3FF));
            part->variant = i & 1;
            part->hasColorKeys = 1;
            part->age = gEff06PartAges3[i];
            part->ang[0] = 0;
            part->ang[1] = 0;
            if (part->variant == 0) {
                part->ang[2] = Rand_Next(1);
                part->pos[0] = (Rand_Next(1) & 0x1F) - 16;
                part->pos[1] = (Rand_Next(1) & 0x1F) - 16;
                part->pos[2] = (Rand_Next(1) & 0x1F) - 16;
            } else {
                part->ang[2] = 0;
                part->variant += (s16)(Rand_Next(1) % 3);
            }
            part->spin = 0.5f + 65536.0f * (0.001f * (Rand_Next(1) & 0x3FF)) / 360.0f;
            break;
        case 4:
        case 8:
            part->variant = i;
            part->hasColorKeys = 1;
            part->age = gEff06PartAges4[i];
            part->ang[0] = 0;
            part->ang[1] = 0;
            if (part->variant == 0) {
                part->ang[2] = 0;
            } else if (part->variant == 1) {
                part->ang[2] = Rand_Next(1);
            } else {
                part->ang[2] = Rand_Next(1);
                part->variant += (s16)(Rand_Next(1) % 3);
            }
            part->spin = 0;
            break;
        case 5:
        case 9:
            part->variant = i;
            part->keyScale[0] = 1.0f;
            part->keyScale[1] = 1.0f;
            part->keyScale[2] = 1.0f;
            part->hasColorKeys = 0;
            part->age = 0;
            part->ang[0] = Rand_Next(1);
            part->ang[1] = Rand_Next(1);
            part->ang[2] = Rand_Next(1);
            part->spin = 0;
            break;
        case 6:
            switch (i) {
            case 0:
            case 1:
            case 2:
            case 3:
                part->variant = 0;
                part->age = -i * 5;
                break;
            case 4:
                part->variant = 1;
                part->age = 0;
                part->color = 0xCCFFBF5F;
                break;
            case 5:
                part->variant = 2;
                part->age = -2;
                break;
            }
            part->ang[2] = 0;
            part->spin = 0;
            part->hasColorKeys = 1;
            part->kind = 0;
            break;
        case 10:
            part->variant = i;
            part->scale = 0.8f;
            part->ang[2] = Rand_Next(1);
            part->spin = 546;
            part->hasColorKeys = 1;
            break;
        }
        part->slot = DrawSlot_Alloc();
        if (part->slot != -1) {
            part->draw = DrawSlot_Get(part->slot);
            part->draw->owner = effect;
            part->draw->part = i;
            part->draw->draw = Effect06_DrawPart;
        } else {
            part->draw = NULL;
        }
    }
}

/*
 * Steps a part of a type 2 effect, whose life is var0 (15 for 0 and 255).
 * step 0, at the start of a round: adds 4 to the part's variant, shrinks it by
 * that, and puts it at a random offset (-16..15, scaled) turned with the
 * owner's bone var1 (var0 255: a smaller offset moved by (45, 10, 32)).
 * step 1, when a round ends: sets its age to -2 (two frames' wait, then the
 * next round) while variant + 4 is below the life.
 */
void Effect06_StepPart(StageObj *effect, EffectPart *part, s16 step)
{
    f32 boneMtx[4][4];
    u8 life;

    life = effect->var0;
    if (life == 0 || life == 255) {
        life = 15;
    } else {
        life = effect->var0;
    }
    switch (step) {
    case 0:
        part->variant += 4;
        part->scale = effect->scale * (1.0f - 0.8f / life * part->variant);
        part->ang[2] = Rand_Next(1);
        Mtx_Copy(boneMtx[0], (f32 *)(Em_GetBone(effect->owner, effect->var1)));
        part->pos[0] = effect->scale * (f32)((Rand_Next(1) & 0x1F) - 16);
        part->pos[1] = effect->scale * (f32)((Rand_Next(1) & 0x1F) - 16);
        part->pos[2] = effect->scale * (f32)((Rand_Next(1) & 0x1F) - 16);
        if (effect->var0 == 255) {
            Vec_Scale(part->pos, part->pos, 0.6f);
            part->pos[0] += 45.0f;
            part->pos[1] += 10.0f;
            part->pos[2] += 32.0f;
        }
        Vec_RotateSelf(part->pos, boneMtx[0]);
        part->pos[0] += boneMtx[3][0];
        part->pos[1] += boneMtx[3][1];
        part->pos[2] += boneMtx[3][2];
        break;
    case 1:
        if (part->variant + 4 < life) {
            part->age = -2;
        }
        break;
    }
}

/* Places part i of a type 3 effect: its offset plus the object's, turned with the owner's bone
 * var1, at that bone; the position is copied to the next part (i below 5), and the first part plays
 * the effect's sound. */
void Effect06_InitPartAtBone(StageObj *effect, EffectPart *part, s16 i)
{
    f32 boneMtx[4][4];

    part->ang[2] = Rand_Next(1);
    Mtx_Copy(boneMtx[0], (f32 *)(Em_GetBone(effect->owner, effect->var1)));
    part->pos[0] += effect->pos[0];
    part->pos[1] += effect->pos[1];
    part->pos[2] += effect->pos[2];
    Vec_RotateSelf(part->pos, boneMtx[0]);
    part->pos[0] += boneMtx[3][0];
    part->pos[1] += boneMtx[3][1];
    part->pos[2] += boneMtx[3][2];
    if (i < 5) {
        Vec_Copy(part[1].pos, part->pos);
    }
    if (i == 0) {
        Effect06_PlaySound(effect, part->pos);
    }
}

/* Places part i of a type 4 or 8 effect (parts is the first part): part 0 at a random offset
 * (-16..15) from the object, turned with the owner's bone var1, at that bone; parts 1 and 2 where
 * part 0 is. */
void Effect06_InitPartsJitter(StageObj *effect, EffectPart *parts, s16 i)
{
    s32 n = i;
    f32 boneMtx[4][4];

    switch (n) {
    case 0:
        Mtx_Copy(boneMtx[0], (f32 *)(Em_GetBone(effect->owner, effect->var1)));
        parts[n].pos[0] = effect->pos[0] + (f32)((Rand_Next(1) & 0x1F) - 16);
        parts[n].pos[1] = effect->pos[1] + (f32)((Rand_Next(1) & 0x1F) - 16);
        parts[n].pos[2] = effect->pos[2] + (f32)((Rand_Next(1) & 0x1F) - 16);
        Vec_RotateSelf(parts[n].pos, boneMtx[0]);
        parts[n].pos[0] += boneMtx[3][0];
        parts[n].pos[1] += boneMtx[3][1];
        parts[n].pos[2] += boneMtx[3][2];
        break;
    case 1:
    case 2:
        Vec_Copy(parts[n].pos, parts[0].pos);
        break;
    }
}

/* Starts a part of a type 6 effect: a random angle, its kind from the object's var2 (the charge
 * level), its index from the object's timer (counted up), and for kinds 0 and 1 a scale growing with
 * the index (at most 0.8 of the object's; the index and timer step back when capped). */
void Effect06_InitPartScaled(StageObj *effect, EffectPart *part, s16 i)
{
    f32 max;

    part->ang[2] = Rand_Next(1);
    part->kind = effect->var2;
    part->index = effect->timer++;
    part->ang[2] = Rand_Next(1);
    switch (part->kind) {
    case 0:
    case 1:
        part->scale = effect->scale * (0.5f + 0.05f * part->index);
        max = 0.8f * effect->scale;
        if (part->scale > max) {
            part->scale = max;
            part->index--;
            effect->timer--;
        }
        break;
    case 2:
    default:
        part->scale = effect->scale;
        break;
    }
}

/* Restarts a part of a type 9 effect: new random angles, its age back to 0. */
void Effect06_RandomizePart(StageObj *effect, EffectPart *part)
{
    part->age = 0;
    part->ang[0] = Rand_Next(1);
    part->ang[1] = Rand_Next(1);
    part->ang[2] = Rand_Next(1);
}

/* Puts the object at an offset (45, 10, 32) from its owner's bone var1, turned with the bone
 * (type 10). */
void Effect06_PlaceAtBone(StageObj *effect)
{
    f32 offset[3];
    f32 boneMtx[4][4];

    Mtx_Copy(boneMtx[0], (f32 *)(Em_GetBone(effect->owner, effect->var1)));
    offset[0] = 45.0f;
    offset[1] = 10.0f;
    offset[2] = 32.0f;
    Vec_RotateSelf(offset, boneMtx[0]);
    Mtx_GetTrans(effect->pos, boneMtx[0]);
    Vec_Add(effect->pos, effect->pos, offset);
}

/* 1 when the effect's owner (a hunter or a monster) is gone: its slot is free (active 0) or its
 * unk004 is 3 (the monster is being freed); else 0. */
s32 Effect06_OwnerGone(void *owner)
{
    ObjHead *head = owner;

    if (head->active == 0 || head->unk004 == 3) {
        return 1;
    }
    return 0;
}

/* Moves the key indices past a part that is skipped this frame: the scale key index by its step,
 * the colour key index by one when the part has colour keys (NEXT_PART). */
static void Effect06_CountPart(EffectPart *part, s16 *posKey, s16 posKeyStep, s16 *colorKey)
{
    *posKey += posKeyStep;
    if (part->hasColorKeys != 0) {
        (*colorKey)++;
    }
}

/* 1 when every one of the n parts is done (its variant 255), else 0. */
s32 Effect06_AllPartsDone(EffectPart *part, s16 n)
{
    s16 i;

    for (i = 0; i < n; i++, part++) {
        if (part->variant != 255) {
            return 0;
        }
    }
    return 1;
}

extern u16 *gEff06PartDurTbls[];
extern u16 gEff06Times[];
extern s16 gEff06PosKeyStart[];
extern s16 gEff06PosKeyStep[];
extern s16 gEff06ColorKeyStart[];
extern Vec3Key *gEffect06PosKeys[];
extern ColorKey *gEffect06ColorKeys[];
extern f32 gCamMatrix[];

/* Done with this part: advance the key indices, go to the next part. */
#define NEXT_PART { Effect06_CountPart(part, &posKey, posKeyStep, &colorKey); part++; continue; }

/*
 * The run state of a kind 6 effect object: follows the owner's bone by type,
 * ends the object (state 2) when its time (gEff06Times) is up (type 6: when the
 * owner's weapon motion ends and every part is gone), then animates each part:
 * its age against its duration, its scale keys (gEffect06PosKeys) and color
 * keys (gEffect06ColorKeys), and adds its draw slot to the ordering table. Type 7 (the
 * status icon effect) runs its own loop.
 */
void Effect06_Run(StageObj *effect)
{
    EffectPart *part;
    EffectPart *firstPart;
    u16 *partDurs;
    s16 n;
    u16 lifetime;
    s16 posKeyStep;
    s16 i;
    u16 partDur;
    u8 swayPending;
    Player *pl;
    u8 life;
    void *posKeys;
    s16 prevLevel;
    f32 basePos[3];
    f32 delta[3];
    s16 posKey;
    s16 colorKey;

    part = (EffectPart *)effect->buf;
    swayPending = 0;
    n = gEff06PartCounts[effect->type];
    lifetime = gEff06Times[effect->type];
    partDurs = gEff06PartDurTbls[effect->type];
    posKey = gEff06PosKeyStart[effect->type];
    posKeyStep = gEff06PosKeyStep[effect->type];
    colorKey = gEff06ColorKeyStart[effect->type];
    firstPart = part;
    if (effect->var2 != 255) {
        if (Effect06_OwnerGone(effect->owner) != 0) {
            effect->var2 = 255;
        } else if (effect->type == 7) {
            effect->var2 = 0;
        }
    }
    switch (effect->type) {
    case 0:
        colorKey += (s16)(effect->var0 * 3);
        if (effect->var2 != 255) {
            if (effect->timer == 10) {
                Effect06_SpawnAtBone(effect->owner, 2, 0, effect->var1, effect->scale);
            }
            Em_GetBonePos(effect->owner, effect->var1, effect->pos);
        }
        break;
    case 1:
        if (effect->var2 != 255) {
            Em_GetBonePos(effect->owner, effect->var1, effect->pos);
        }
        partDur = 11;
        break;
    case 2:
        life = effect->var0;
        if (life == 0 || life == 255) {
            life = 15;
        } else {
            life = effect->var0;
        }
        partDur = 6;
        lifetime = (life - 1) * 2 + 6;
        break;
    case 5:
        if (effect->var2 != 255) {
            Em_GetBonePos(effect->owner, effect->var1, effect->pos);
            if (effect->timer == 0) {
                Effect06_PlaySound(effect, effect->pos);
            }
        }
        partDur = 12;
        break;
    case 6:
        if (effect->var2 != 255) {
            Em_GetBonePos(effect->owner, effect->var1, effect->pos);
            prevLevel = effect->var2;
            effect->var2 = ((Player *)effect->owner)->weaponTimer / 30;
            pl = effect->owner;
            if (pl->weaponType == WEAPON_BOW && pl->weaponTimer == 90) {
                PartFx_SpawnForPlayerScaled(pl, 2, 35, 1.0f);
            }
            if (effect->var2 > 2) {
                effect->var2 = 2;
            }
            if (prevLevel != effect->var2) {
                Effect06_PlaySound(effect, effect->pos);
            }
        }
        break;
    case 7:
        if (effect->var2 != 255) {
            if (!((u8)Obj_InCurrentArea(effect->owner) & 0xFF)) {
                return;
            }
            if (gScene.eventRunning == 1) {
                return;
            }
            Em_GetBonePos(effect->owner, effect->var1, basePos);
            Vec_Sub(delta, basePos, effect->pos);
            Vec_Copy(effect->pos, basePos);
            Em_GetBonePos(effect->owner, 2, basePos);
            basePos[1] = effect->pos[1] - 20.0f;
        } else {
            return;
        }
        break;
    case 9:
        if (effect->var2 != 255) {
            Em_GetBonePos(effect->owner, effect->var1, effect->pos);
            if (effect->timer == 0) {
                Effect06_PlaySound(effect, effect->pos);
            }
        }
        partDur = 4;
        break;
    case 10:
        partDur = 10;
        if (effect->var2 != 255) {
            if (effect->timer == 1) {
                Effect06_SpawnAtBone(effect->owner, 2, 255, effect->var1, 4.0f * effect->scale);
            }
            Em_GetBonePos(effect->owner, effect->var1, effect->pos);
        }
        break;
    }
    if (effect->type == 6) {
        if (effect->var2 != 255) {
            pl = effect->owner;
            if (pl->weaponType == WEAPON_HAMMER) {
                if (pl->motionId[0] != PL_MOT_HAMMER_CHARGE_WALK && pl->motionId[0] != PL_MOT_HAMMER_CHARGE_START
                    && pl->motionId[0] != PL_MOT_HAMMER_CHARGE_HOLD) {
                    effect->var2 = 255;
                }
            } else if (pl->weaponType == WEAPON_BOW) {
                if (pl->motionId[0] != PL_MOT_BOW_CHARGE && pl->motionId[0] != PL_MOT_BOW_CHARGE_WALK
                    && pl->motionId[0] != PL_MOT_BOW_CHARGE_FROM_DRAW && pl->motionId[0] != PL_MOT_BOW_CHARGE_FROM_STAB
                    && pl->motionId[0] != PL_MOT_BOW_CHARGE_FROM_SHOT && pl->motionId[0] != 1419 &&
                    pl->motionId[0] != 1420) {
                    effect->var2 = 255;
                }
            } else {
                effect->var2 = 255;
            }
        } else if (Effect06_AllPartsDone(part, n) != 0) {
            effect->state++;
            effect->visible = 0;
            return;
        }
    } else if (effect->type == 7) {
        switch (((Player *)effect->owner)->statusIcons & (PL_STATUS_ICON_ATTACK_UP | PL_STATUS_ICON_DEFENSE_UP)) {
        default:
            effect->var0 = 0;
            break;
        case PL_STATUS_ICON_ATTACK_UP:
            effect->var0 = 1;
            break;
        case PL_STATUS_ICON_DEFENSE_UP:
            effect->var0 = 2;
            break;
        case PL_STATUS_ICON_ATTACK_UP | PL_STATUS_ICON_DEFENSE_UP:
            effect->var0 = 3;
            break;
        }
        for (i = 0; i < n; i++, part++) {
            switch (effect->var0) {
            case 0:
                Vec_Copy(part->pos, basePos);
                continue;
            case 1:
            case 2:
                part->age += 2;
                if (part->variant == 1) {
                    part->index += 4;
                } else {
                    part->index += 2;
                }
                break;
            case 3:
                part->age += 4;
                if (part->variant == 1) {
                    part->index += 6;
                } else {
                    part->index += 3;
                }
                break;
            }
            if (part->age >= 240) {
                part->age = 0;
            }
            if (part->index >= 60) {
                effect->angle++;
                if (effect->angle >= 2) {
                    effect->angle = 0;
                    part->variant = 1;
                } else {
                    part->variant = 0;
                }
                part->ang[1] = Rand_Next(1);
                part->index = 0;
                Vec_Copy(part->pos, basePos);
            } else {
                part->pos[0] += delta[0];
                part->pos[1] += delta[1];
                part->pos[2] += delta[2];
            }
            if (part->draw != NULL) {
                Vec_Copy(part->draw->pos, part->pos);
                OT_AddByDepth(gDrawOtSorted, part->draw, 64, 0);
            }
        }
        return;
    } else if (++effect->timer >= lifetime) {
        effect->state++;
        effect->visible = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        switch (effect->type) {
        case 6:
            switch (part->variant) {
            case 0:
                switch (part->kind) {
                case 0:
                case 1:
                    partDur = 14;
                    break;
                case 2:
                default:
                    partDur = 12;
                    break;
                }
                break;
            case 1:
                switch (effect->var2) {
                case 0:
                    partDur = 30;
                    break;
                case 1:
                    partDur = 30;
                    switch (part->kind) {
                    case 0:
                        part->age = 0;
                        part->kind++;
                    }
                    break;
                case 2:
                    switch (part->kind) {
                    case 1:
                        part->age = 0;
                        part->kind++;
                    }
                    partDur = 6;
                    break;
                default:
                    part->variant = 255;
                    NEXT_PART
                }
                break;
            case 2:
                switch (effect->var2) {
                case 0:
                case 1:
                    NEXT_PART
                case 2:
                default:
                    partDur = 4;
                    break;
                }
                break;
            }
            break;
        case 1:
            posKey = gEff06PosKeyStart[effect->type];
            colorKey = gEff06ColorKeyStart[effect->type] + effect->var0;
            break;
        default:
            if (partDurs != NULL) {
                partDur = partDurs[i];
            } else {
                posKey = gEff06PosKeyStart[effect->type];
                colorKey = gEff06ColorKeyStart[effect->type];
            }
            break;
        }
        if (++part->age <= 0 || part->variant == 255) {
            NEXT_PART
        }
        if (part->age > partDur) {
            switch (effect->type) {
            case 6:
                switch (part->variant) {
                case 0:
                    switch (effect->var2) {
                    case 0:
                    case 1:
                        part->age = -6;
                        break;
                    case 2:
                    default:
                        part->age = -4;
                        break;
                    }
                    break;
                case 1:
                    switch (effect->var2) {
                    case 0:
                    case 1:
                        part->age = partDur;
                        break;
                    case 2:
                    default:
                        part->variant = 255;
                        NEXT_PART
                    }
                    break;
                case 2:
                    part->variant = 255;
                    NEXT_PART
                }
                break;
            case 9:
                Effect06_RandomizePart(effect, part);
                break;
            case 2:
                Effect06_StepPart(effect, part, 1);
                NEXT_PART
            default:
                NEXT_PART
            }
        }
        switch (effect->type) {
        case 0:
        case 1:
        case 5:
        case 9:
            Vec_Copy(part->pos, effect->pos);
            break;
        case 2:
            if (part->age == 1) {
                if (effect->var2 != 255) {
                    Effect06_StepPart(effect, part, 0);
                } else {
                    part->variant = 255;
                    NEXT_PART
                }
            }
            break;
        case 3:
            if (part->age == 1) {
                if (effect->var2 != 255) {
                    if (part->variant == 0) {
                        Effect06_InitPartAtBone(effect, part, i);
                    }
                } else {
                    part->variant = 255;
                    NEXT_PART
                }
            }
            if (part->variant == 0) {
                part->pos[1] += effect->scale * (25.0f / partDur);
            } else {
                part->pos[1] += effect->scale * (50.0f / partDur);
            }
            break;
        case 4:
        case 8:
            if (part->variant == 0 || part->variant == 1) {
                delta[0] = effect->scale * ((effect->angle & 0xF) + 15) *
                       (Math_Sin((6.2831855f * (effect->timer << 11)) / 65536.0f) - 0.5f);
                delta[1] = 0.5f * effect->scale * ((((effect->angle & 0x300) >> 8) + 2) * effect->timer);
                delta[2] = 0.0f;
                Vec_RotateSelf(delta, gCamMatrix);
                swayPending = 1;
            }
            if (part->age == 1) {
                if (part->variant == 0) {
                    if (effect->var2 != 255) {
                        Effect06_InitPartsJitter(effect, firstPart, i);
                    } else {
                        part->variant = 255;
                        NEXT_PART
                    }
                } else if (firstPart->variant != 255) {
                    Effect06_InitPartsJitter(effect, firstPart, i);
                } else {
                    part->variant = 255;
                    NEXT_PART
                }
            }
            break;
        case 6:
            if (part->age == 1) {
                if (effect->var2 != 255) {
                    if (part->variant == 0) {
                        Effect06_InitPartScaled(effect, part, i);
                    }
                } else {
                    part->variant = 255;
                    NEXT_PART
                }
            }
            switch (part->variant) {
            case 0:
                switch (part->kind) {
                default:
                    posKey = 15;
                    colorKey = 49;
                    Vec_Copy(part->pos, effect->pos);
                    break;
                case 1:
                    posKey = 15;
                    colorKey = 50;
                    Vec_Copy(part->pos, effect->pos);
                    break;
                case 2:
                    posKey = 16;
                    colorKey = 51;
                    if (effect->var2 != 255) {
                        Em_GetBonePos(effect->owner, PL_BONE_OTHER_HAND, part->pos);
                    }
                    break;
                }
                break;
            case 1:
                switch (effect->var2) {
                case 0:
                    part->keyScale[1] = part->keyScale[0] = (1.5f * part->age) / 30.0f;
                    part->keyScale[2] = part->keyScale[0];
                    if (part->draw != NULL) {
                        part->draw->pos[0] = effect->pos[0];
                        part->draw->pos[1] = effect->pos[1];
                        part->draw->pos[2] = effect->pos[2];
                        OT_AddByDepth(gDrawOtSorted, part->draw, 64, 0);
                    }
                    NEXT_PART
                case 1:
                    part->keyScale[1] = part->keyScale[0] = 1.5f + (1.5f * part->age) / 30.0f;
                    part->keyScale[2] = part->keyScale[0];
                    if (part->draw != NULL) {
                        part->draw->pos[0] = effect->pos[0];
                        part->draw->pos[1] = effect->pos[1];
                        part->draw->pos[2] = effect->pos[2];
                        OT_AddByDepth(gDrawOtSorted, part->draw, 64, 0);
                    }
                    NEXT_PART
                case 2:
                    posKey = 17;
                    colorKey = 52;
                    Vec_Copy(part->pos, effect->pos);
                    break;
                }
                break;
            case 2:
                posKey = 18;
                colorKey = 53;
                if (effect->var2 != 255) {
                    Em_GetBonePos(effect->owner, PL_BONE_OTHER_HAND, part->pos);
                } else {
                    part->variant = 255;
                    NEXT_PART
                }
                break;
            }
            break;
        }
        if (posKeyStep > 0) {
            posKeys = (void *)gEffect06PosKeys[posKey++];
            Key_LerpVec3(posKeys, part->keyScale, part->age);
        }
        part->ang[2] += part->spin;
        if (part->hasColorKeys != 0 && colorKey != -1) {
            Color_EvalKeys((s32 *)gEffect06ColorKeys[colorKey++], part->age, (u32 *)&part->color);
        }
        if (part->draw != NULL) {
            if (swayPending) {
                swayPending = 0;
                part->draw->pos[0] = part->pos[0] + delta[0];
                part->draw->pos[1] = part->pos[1] + delta[1];
                part->draw->pos[2] = part->pos[2] + delta[2];
            } else {
                part->draw->pos[0] = part->pos[0];
                part->draw->pos[1] = part->pos[1];
                part->draw->pos[2] = part->pos[2];
            }
            if (effect->type == 5 || effect->type == 9) {
                OT_AddByDepth(gDrawOt32, part->draw, 32, 0);
            } else {
                OT_AddByDepth(gDrawOtSorted, part->draw, 64, 0);
            }
        }
        part++;
    }
}

#undef NEXT_PART

/* Frees a kind 6 effect object's draw slots (the types listed in gEff06PartCounts have that many parts in their buffer). */
void Effect06_End(StageObj *effect)
{
    EffectPart *part = (EffectPart *)effect->buf;
    s16 n;
    s16 i;

    effect->state++;
    n = gEff06PartCounts[effect->type];
    if (n == 0) {
        if (*(s32 *)&effect->node != 0) {
            if (effect->keep) {
                DrawSlotB_Free(effect->slot);
            } else {
                DrawSlot_Free(effect->slot);
            }
        }
    } else {
        for (i = 0; i < n; i++, part++) {
            if (part->draw != NULL) {
                DrawSlot_Free(part->slot);
            }
        }
    }
}

void Effect06_Delete(StageObj *effect)
{
    StageObj_Free(effect);
}

extern ColorKey *gEff06IconColorKeys[];
extern ColorKey *gEff06IconColorKeys3[];
extern s32 gEff06IconAlphaKeys[];
/* Draws a part of a kind 6 effect object (its draw slot): a model of set 0 by the object type, scaled, turned and placed at the slot. */
void Effect06_DrawPart(DrawSlot *slot)
{
    StageObj *effect = slot->owner;
    EffectPart *part = &((EffectPart *)effect->buf)[slot->part];
    void *base;
    u16 flags = 0;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    ModelMatSet *mdl;
    s32 *colorKeys;
    s32 *alphaKeys;
    u32 color;
    u32 alpha;
    s16 id;
    Model *set = gCommonModels[0];

    if (set != NULL && set->active) {
        base = set->mats;
        if (effect->type == 7) {
            switch (effect->var0) {
            default:
                return;
            case 1:
                colorKeys = (s32 *)gEff06IconColorKeys[0];
                break;
            case 2:
                colorKeys = (s32 *)gEff06IconColorKeys[1];
                break;
            case 3:
                colorKeys = (s32 *)gEff06IconColorKeys3[0];
                break;
            }
            alphaKeys = gEff06IconAlphaKeys;
            Gs_SetState(RS_ZWRITE, 0);
            if (part->variant == 0) {
                mdl = &set->matSets[61];
                Color_EvalKeys(colorKeys, part->age, &color);
            } else {
                mdl = &set->matSets[143];
                color = -1;
            }
            Color_EvalKeys(alphaKeys, part->index, &alpha);
            color &= 0xFFFFFF;
            color |= alpha & 0xFF000000;
            scale[0] = part->scale;
            scale[1] = part->scale * part->index / 60.0f;
            scale[2] = part->scale;
            rot[1] = part->ang[1];
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 2, mtx);
            Effect_DrawModel(mdl, mtx, color, 2, base);
            Gs_SetState(RS_ZWRITE, 1);
        } else {
            scale[0] = part->scale * part->keyScale[0];
            scale[1] = part->scale * part->keyScale[1];
            scale[2] = part->scale * part->keyScale[2];
            rot[2] = 6.2831855f * part->ang[2] / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 3, mtx);
            switch (effect->type) {
            case 0:
                Mtx_Concat3x3(mtx, gCamMatrix);
                Gs_SetState(RS_ZWRITE, 0);
                flags |= 2;
                switch (part->variant) {
                case 0:
                    id = 84;
                    break;
                case 1:
                    id = 37;
                    if (gScene.inLobby == 0) {
                        Mtx_Translation(uvMtx, 0.0f, 0.125f, 0.0f);
                    } else {
                        Mtx_Translation(uvMtx, 0.0f, 0.25f, 0.0f);
                    }
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    break;
                case 2:
                    id = 88;
                    break;
                }
                break;
            case 1:
                rot[0] = 6.2831855f * part->ang[0] / 65536.0f;
                rot[1] = 6.2831855f * part->ang[1] / 65536.0f;
                Mtx_RotateX(mtx, rot[0]);
                Mtx_RotateY(mtx, rot[1]);
                Gs_SetState(RS_ZWRITE, 0);
                flags |= 2;
                id = 38;
                break;
            case 2:
                Mtx_Concat3x3(mtx, gCamMatrix);
                Gs_SetState(RS_ZWRITE, 0);
                flags |= 2;
                id = 37;
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                break;
            case 3:
                Mtx_Concat3x3(mtx, gCamMatrix);
                flags |= 2;
                alphaKeys = gEff06IconAlphaKeys;
                Gs_SetState(RS_ZWRITE, 0);
                if (part->variant == 0) {
                    id = 74;
                } else {
                    id = part->variant + 44;
                }
                break;
            case 4:
            case 8:
                Mtx_Concat3x3(mtx, gCamMatrix);
                flags |= 2;
                alphaKeys = gEff06IconAlphaKeys;
                Gs_SetState(RS_ZWRITE, 0);
                if (part->variant == 0) {
                    id = 39;
                } else if (part->variant == 1) {
                    id = 48;
                } else {
                    id = part->variant + 43;
                }
                break;
            case 5:
            case 9:
                rot[0] = 6.2831855f * part->ang[0] / 65536.0f;
                rot[1] = 6.2831855f * part->ang[1] / 65536.0f;
                Mtx_RotateX(mtx, rot[0]);
                Mtx_RotateY(mtx, rot[1]);
                id = 49;
                Mtx_Translation(uvMtx, 0.0f, 0.03125f * part->age, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                break;
            case 6:
                Mtx_Concat3x3(mtx, gCamMatrix);
                Gs_SetState(RS_ZWRITE, 0);
                flags |= 2;
                switch (part->variant) {
                case 0:
                    id = 66;
                    Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    break;
                case 1:
                case 2:
                    id = 64;
                    break;
                default:
                    return;
                }
                break;
            case 10:
                Mtx_Concat3x3(mtx, gCamMatrix);
                Gs_SetState(RS_ZWRITE, 0);
                flags |= 2;
                id = 81;
                break;
            }
            id = Effect_MapTexId(id);
            if (id != -1) {
                mdl = &set->matSets[id];
                if (effect->type == 5) {
                    Gs_SetState(RS_COLOR2, -1);
                    Effect_DrawModelAdd(mdl, mtx, base);
                } else {
                    Effect_DrawModel(mdl, mtx, part->color, flags, base);
                }
            }
            Gs_SetState(RS_ZWRITE, 1);
        }
    }
}


/* Plays a kind 6 effect's sound at pos, by type (type 6: the sound of its charge level var2, from
 * the owner's own bank at the object's position). */
void Effect06_PlaySound(StageObj *effect, f32 *pos)
{
    switch (effect->type) {
    case 0:
        Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, 57, 0, pos, 1, 0);
        break;
    case 3:
        Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, 35, 0, pos, 1, 0);
        break;
    case 5:
    case 9:
        Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, 34, 0, pos, 1, 0);
        break;
    case 6:
        switch (effect->var2) {
        case 0:
            Player_PlaySe(effect->owner, 12, 0, effect->pos, 1, 0);
            break;
        case 1:
            Player_PlaySe(effect->owner, 13, 0, effect->pos, 1, 0);
            break;
        case 2:
            Player_PlaySe(effect->owner, 14, 0, effect->pos, 1, 0);
        }
        break;
    }
}

/* Makes a kind 6 effect object of the given type for a hunter or a monster, with sub in var0
 * (Effect06_Update runs it). Returns it, or NULL when none is free or the owner is not in the
 * current area; type 7, and a monster run by another console (isEm and ownTask set) whose kind
 * byte is 3, are made in any area. */
StageObj *Effect06_New(Player *pl, s16 type, u8 sub)
{
    StageObj *effect;

    if (type != 7 && (pl->isEm == 0 || pl->ownTask == 0 || pl->weaponType != 3) && (u8)Obj_InCurrentArea(pl) == 0) { /* a monster sub_main runs of kind 3 (Em.type) is shown in any area */
        return NULL;
    }
    effect = StageObj_Alloc(1);
    if (effect == NULL) {
        return NULL;
    }
    effect->kind = 6;
    effect->update = Effect06_Update;
    effect->owner = pl;
    effect->type = type;
    effect->var0 = sub;
    return effect;
}

/* A kind 6 effect of the given type at one of the player's bones (type 0 also makes type 1; type 6 only for the local player). */
void Effect06_SpawnAtBone(Player *pl, s16 type, u8 sub, s16 bone, f32 scale)
{
    StageObj *effect;

    if (type != 6 || Player_IsLocal(pl) != 0) {
        if ((effect = Effect06_New(pl, type, sub)) != NULL) {
            effect->scale = scale;
            Em_GetBonePos(pl, bone, effect->pos);
            effect->var1 = bone;
            if (type == 0) {
                Effect06_SpawnAtBone(pl, 1, sub, bone, scale);
            } else if (type == 7) {
                effect->keep = 1;
            }
        }
    }
}

/* A kind 6 effect of the given type for a player, at pos. */
void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale)
{
    StageObj *effect = Effect06_New(obj, type, 0);

    if (effect != NULL) {
        effect->scale = scale;
        effect->var1 = bone;
        Vec_Copy(effect->pos, pos);
    }
}

extern f32 gEffect06EmScales[];
extern f32 gEffect06Scales[];
/* Makes a kind 6 effect at the owner's bone 2. Types 0 (which also makes type 1) and 9 are sized: a
 * hunter by a fixed scale, a monster by its scale times its kind's entry (gEffect06EmScales for
 * type 0, gEffect06Scales for type 9); others 1.0. */
void Effect06_SpawnWeapon(Player *pl, s16 type, u8 sub)
{
    StageObj *effect;

    if ((effect = Effect06_New(pl, type, sub)) != NULL) {
        effect->var1 = 2;
        Em_GetBonePos(pl, effect->var1, effect->pos);
        switch (effect->type) {
        case 0:
            if (pl->isEm == 0) {
                effect->scale = 4.0f;
            } else {
                effect->scale = pl->scale[0] * gEffect06EmScales[pl->weaponType];
            }
            Effect06_SpawnAtBone(pl, 1, sub, effect->var1, effect->scale);
            break;
        case 9:
            if (pl->isEm == 0) {
                effect->scale = 0.75f;
            } else {
                effect->scale = pl->scale[0] * gEffect06Scales[pl->weaponType];
            }
            break;
        default:
            effect->scale = 1.0f;
            break;
        }
    }
}

/* The kind 6 effects' data: the parts' scale and colour key curves, the status icons' colour keys,
 * and the per-type tables Effect06_Run reads. */
Vec3Key gEffect06PosKeys_3[4] = {
#include "sub_main/004657A0.inc"
};
Vec3Key gEffect06PosKeys_0[5] = {
#include "sub_main/004657E0.inc"
};
Vec3Key gEffect06PosKeys_1[5] = {
#include "sub_main/00465830.inc"
};
Vec3Key gEffect06PosKeys_2[5] = {
#include "sub_main/00465880.inc"
};
Vec3Key gEffect06PosKeys_4[3] = {
#include "sub_main/004658D0.inc"
};
Vec3Key gEffect06PosKeys_5[5] = {
#include "sub_main/00465900.inc"
};
Vec3Key gEffect06PosKeys_6[4] = {
#include "sub_main/00465950.inc"
};
Vec3Key gEffect06PosKeys_7[3] = {
#include "sub_main/00465990.inc"
};
Vec3Key gEffect06PosKeys_12[10] = {
#include "sub_main/004659C0.inc"
};
Vec3Key gEffect06PosKeys_13[5] = {
#include "sub_main/00465A60.inc"
};
Vec3Key gEffect06PosKeys_14[3] = {
#include "sub_main/00465AB0.inc"
};
Vec3Key gEffect06PosKeys_15[4] = {
#include "sub_main/00465AE0.inc"
};
Vec3Key gEffect06PosKeys_16[3] = {
#include "sub_main/00465B20.inc"
};
Vec3Key gEffect06PosKeys_17[3] = {
#include "sub_main/00465B50.inc"
};
f32 gEffect06PosKeys_18[12] = {
#include "sub_main/00465B80.inc"
};
Vec3Key gEffect06PosKeys_19[4] = {
#include "sub_main/00465BB0.inc"
};
Vec3Key *gEffect06PosKeys[20] = {
    gEffect06PosKeys_0, gEffect06PosKeys_1, gEffect06PosKeys_2, gEffect06PosKeys_3,
    gEffect06PosKeys_4, gEffect06PosKeys_5, gEffect06PosKeys_6, gEffect06PosKeys_7,
    gEffect06PosKeys_6, gEffect06PosKeys_7, gEffect06PosKeys_6, gEffect06PosKeys_7,
    gEffect06PosKeys_12, gEffect06PosKeys_13, gEffect06PosKeys_14, gEffect06PosKeys_15,
    gEffect06PosKeys_16, gEffect06PosKeys_17, (Vec3Key *)gEffect06PosKeys_18, gEffect06PosKeys_19,
};
ColorKey gEffect06ColorKeys_30[6] = {
#include "sub_main/00465C40.inc"
};
ColorKey gEffect06ColorKeys_2[6] = {
#include "sub_main/00465C70.inc"
};
ColorKey gEffect06ColorKeys_0[8] = {
#include "sub_main/00465CA0.inc"
};
ColorKey gEffect06ColorKeys_1[6] = {
#include "sub_main/00465CE0.inc"
};
ColorKey gEffect06ColorKeys_31[6] = {
#include "sub_main/00465D10.inc"
};
ColorKey gEffect06ColorKeys_5[6] = {
#include "sub_main/00465D40.inc"
};
ColorKey gEffect06ColorKeys_3[8] = {
#include "sub_main/00465D70.inc"
};
ColorKey gEffect06ColorKeys_4[6] = {
#include "sub_main/00465DB0.inc"
};
ColorKey gEffect06ColorKeys_32[6] = {
#include "sub_main/00465DE0.inc"
};
ColorKey gEffect06ColorKeys_8[6] = {
#include "sub_main/00465E10.inc"
};
ColorKey gEffect06ColorKeys_6[8] = {
#include "sub_main/00465E40.inc"
};
ColorKey gEffect06ColorKeys_7[6] = {
#include "sub_main/00465E80.inc"
};
ColorKey gEffect06ColorKeys_33[6] = {
#include "sub_main/00465EB0.inc"
};
ColorKey gEffect06ColorKeys_11[6] = {
#include "sub_main/00465EE0.inc"
};
ColorKey gEffect06ColorKeys_9[8] = {
#include "sub_main/00465F10.inc"
};
ColorKey gEffect06ColorKeys_10[6] = {
#include "sub_main/00465F50.inc"
};
ColorKey gEffect06ColorKeys_34[6] = {
#include "sub_main/00465F80.inc"
};
ColorKey gEffect06ColorKeys_14[6] = {
#include "sub_main/00465FB0.inc"
};
ColorKey gEffect06ColorKeys_12[8] = {
#include "sub_main/00465FE0.inc"
};
ColorKey gEffect06ColorKeys_13[6] = {
#include "sub_main/00466020.inc"
};
ColorKey gEffect06ColorKeys_35[4] = {
#include "sub_main/00466050.inc"
};
ColorKey gEffect06ColorKeys_17[4] = {
#include "sub_main/00466070.inc"
};
ColorKey gEffect06ColorKeys_15[6] = {
#include "sub_main/00466090.inc"
};
ColorKey gEffect06ColorKeys_16[6] = {
#include "sub_main/004660C0.inc"
};
ColorKey gEffect06ColorKeys_36[4] = {
#include "sub_main/004660F0.inc"
};
ColorKey gEffect06ColorKeys_20[4] = {
#include "sub_main/00466110.inc"
};
ColorKey gEffect06ColorKeys_18[6] = {
#include "sub_main/00466130.inc"
};
ColorKey gEffect06ColorKeys_19[6] = {
#include "sub_main/00466160.inc"
};
ColorKey gEffect06ColorKeys_37[4] = {
#include "sub_main/00466190.inc"
};
ColorKey gEffect06ColorKeys_23[6] = {
#include "sub_main/004661B0.inc"
};
ColorKey gEffect06ColorKeys_21[6] = {
#include "sub_main/004661E0.inc"
};
ColorKey gEffect06ColorKeys_22[6] = {
#include "sub_main/00466210.inc"
};
ColorKey gEffect06ColorKeys_38[4] = {
#include "sub_main/00466240.inc"
};
ColorKey gEffect06ColorKeys_26[4] = {
#include "sub_main/00466260.inc"
};
ColorKey gEffect06ColorKeys_24[6] = {
#include "sub_main/00466280.inc"
};
ColorKey gEffect06ColorKeys_25[6] = {
#include "sub_main/004662B0.inc"
};
ColorKey gEffect06ColorKeys_39[4] = {
#include "sub_main/004662E0.inc"
};
ColorKey gEffect06ColorKeys_29[4] = {
#include "sub_main/00466300.inc"
};
ColorKey gEffect06ColorKeys_27[6] = {
#include "sub_main/00466320.inc"
};
ColorKey gEffect06ColorKeys_28[6] = {
#include "sub_main/00466350.inc"
};
ColorKey gEffect06ColorKeys_40[8] = {
#include "sub_main/00466380.inc"
};
ColorKey gEffect06ColorKeys_41[8] = {
#include "sub_main/004663C0.inc"
};
ColorKey gEffect06ColorKeys_46[6] = {
#include "sub_main/00466400.inc"
};
ColorKey gEffect06ColorKeys_47[4] = {
#include "sub_main/00466430.inc"
};
ColorKey gEffect06ColorKeys_48[4] = {
#include "sub_main/00466450.inc"
};
ColorKey gEffect06ColorKeys_49[6] = {
#include "sub_main/00466470.inc"
};
ColorKey gEffect06ColorKeys_50[6] = {
#include "sub_main/004664A0.inc"
};
ColorKey gEffect06ColorKeys_51[6] = {
#include "sub_main/004664D0.inc"
};
ColorKey gEffect06ColorKeys_52[4] = {
#include "sub_main/00466500.inc"
};
ColorKey gEffect06ColorKeys_53[4] = {
#include "sub_main/00466520.inc"
};
ColorKey gEff06IconKeysAttackUp[4] = {
#include "sub_main/00466540.inc"
};
ColorKey gEff06IconKeysDefenseUp[4] = {
#include "sub_main/00466560.inc"
};
ColorKey gEff06IconKeysBothUp[4] = {
#include "sub_main/00466580.inc"
};
s32 gEff06IconAlphaKeys[12] = {
#include "sub_main/004665A0.inc"
};
ColorKey gEff06IconColorKeys3_1[4] = {
#include "sub_main/004665D0.inc"
};
ColorKey gEff06IconColorKeys3_4[4] = {
#include "sub_main/004665F0.inc"
};
ColorKey *gEffect06ColorKeys[54] = {
    gEffect06ColorKeys_0, gEffect06ColorKeys_1, gEffect06ColorKeys_2, gEffect06ColorKeys_3,
    gEffect06ColorKeys_4, gEffect06ColorKeys_5, gEffect06ColorKeys_6, gEffect06ColorKeys_7,
    gEffect06ColorKeys_8, gEffect06ColorKeys_9, gEffect06ColorKeys_10, gEffect06ColorKeys_11,
    gEffect06ColorKeys_12, gEffect06ColorKeys_13, gEffect06ColorKeys_14, gEffect06ColorKeys_15,
    gEffect06ColorKeys_16, gEffect06ColorKeys_17, gEffect06ColorKeys_18, gEffect06ColorKeys_19,
    gEffect06ColorKeys_20, gEffect06ColorKeys_21, gEffect06ColorKeys_22, gEffect06ColorKeys_23,
    gEffect06ColorKeys_24, gEffect06ColorKeys_25, gEffect06ColorKeys_26, gEffect06ColorKeys_27,
    gEffect06ColorKeys_28, gEffect06ColorKeys_29, gEffect06ColorKeys_30, gEffect06ColorKeys_31,
    gEffect06ColorKeys_32, gEffect06ColorKeys_33, gEffect06ColorKeys_34, gEffect06ColorKeys_35,
    gEffect06ColorKeys_36, gEffect06ColorKeys_37, gEffect06ColorKeys_38, gEffect06ColorKeys_39,
    gEffect06ColorKeys_40, gEffect06ColorKeys_41, gEffect06ColorKeys_40, gEffect06ColorKeys_41,
    gEffect06ColorKeys_40, gEffect06ColorKeys_41, gEffect06ColorKeys_46, gEffect06ColorKeys_47,
    gEffect06ColorKeys_48, gEffect06ColorKeys_49, gEffect06ColorKeys_50, gEffect06ColorKeys_51,
    gEffect06ColorKeys_52, gEffect06ColorKeys_53,
};
ColorKey *gEff06IconColorKeys[2] = {
    gEff06IconKeysAttackUp, gEff06IconKeysDefenseUp,
};
ColorKey *gEff06IconColorKeys3[6] = {
    gEff06IconKeysBothUp, gEff06IconColorKeys3_1, gEffect06ColorKeys_47, gEffect06ColorKeys_48,
    gEff06IconColorKeys3_4, NULL,
};
u16 gEff06PartDurTbls_0[4] = {
#include "sub_main/00466708.inc"
};
s16 gEff06PartAges3[8] = {
#include "sub_main/00466710.inc"
};
u16 gEff06PartDurTbls_3[8] = {
#include "sub_main/00466720.inc"
};
s16 gEff06PartAges4[4] = {
#include "sub_main/00466730.inc"
};
u16 gEff06PartDurTbls_4[4] = {
#include "sub_main/00466738.inc"
};
s16 gEff06PartCounts[16] = {
#include "sub_main/00466740.inc"
};
u16 gEff06Times[16] = {
#include "sub_main/00466760.inc"
};
u16 *gEff06PartDurTbls[12] = {
    gEff06PartDurTbls_0, NULL, NULL, gEff06PartDurTbls_3,
    gEff06PartDurTbls_4, NULL, NULL, NULL,
    gEff06PartDurTbls_4, NULL, NULL, NULL,
};
s16 gEff06PosKeyStart[16] = {
#include "sub_main/004667B0.inc"
};
s16 gEff06PosKeyStep[16] = {
#include "sub_main/004667D0.inc"
};
s16 gEff06ColorKeyStart[16] = {
#include "sub_main/004667F0.inc"
};
f32 gEffect06EmScales[76] = {
#include "sub_main/00466810.inc"
};
