#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"

/* A part of a kind 6 effect object (StageObj.buf, 0x38 bytes each). */
typedef struct EffectPart {
    s16 slot;                   /* 0x00: its draw slot */
    s16 unk02;                  /* 0x02: its age (Effect06_StepPart) */
    f32 unk04[3];               /* 0x04 */
    f32 pos[3];                 /* 0x10 */
    s32 unk1C;                  /* 0x1C */
    u16 ang[3];                 /* 0x20: random (Effect06_RandomizePart) */
    s16 unk26;                  /* 0x26 */
    f32 scale;                  /* 0x28 */
    DrawSlot *draw;             /* 0x2C: DrawSlot_Get(slot), NULL when none */
    s16 unk30;                  /* 0x30 */
    u16 index;                  /* 0x32: the object's timer when the part was made */
    u16 unk34;                  /* 0x34 */
    u16 kind;                   /* 0x36: the object's +0x07 */
} EffectPart;

/* prototypes */
u16 Rand_Next(s32 a);
void StageObj_Free(StageObj *o);
void DrawSlot_Free(s32 id);
void Effect06_Setup(StageObj *o);
void Effect06_Run(StageObj *o);
void Effect06_End(StageObj *o);
void Effect06_Delete(StageObj *o);
void Effect0D_Setup(StageObj *o);
void Effect0D_Run(StageObj *o);
void Effect0D_End(StageObj *o);
void Effect0D_Delete(StageObj *o);
void Effect1A_Setup(StageObj *o);
void Effect1A_Run(StageObj *o);
void Effect1A_End(StageObj *o);
void Effect1A_Delete(StageObj *o);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
StageObj *StageObj_Alloc(s32 nblk);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Copy(f32 *dst, f32 *src);
void Effect06_PlaySound(StageObj *o, f32 *pos);
/* end of prototypes */

/* The update of the kind 6 effect objects (Effect06_New), by state: set up, run, end, free. */
void Effect06_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        Effect06_Setup(o);
        break;
    case 1:
        Effect06_Run(o);
        break;
    case 2:
        Effect06_End(o);
        break;
    case 3:
        Effect06_Delete(o);
        break;
    }
}

extern s16 gEff06PartCounts[];
extern s16 gEff06PartAges3[];
extern s16 gEff06PartAges4[];
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Effect06_PlaceAtBone(StageObj *o);
void Effect06_DrawPart(DrawSlot *s);
/* The set up of a kind 6 effect object: its parts (gEff06PartCounts: the count of each type). */
void Effect06_Setup(StageObj *o)
{
    EffectPart *part = (EffectPart *)o->buf;
    Em *em = o->owner;
    s16 n;
    s16 i;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    o->var2 = 0;
    n = gEff06PartCounts[o->type];
    switch (o->type) {
    case 0:
    case 6:
        Effect06_PlaySound(o, o->pos);
        break;
    case 4:
        o->angle = Rand_Next(1);
        o->scale -= 0.00040000002f * (Rand_Next(1) & 0x3FF);
        break;
    case 7:
        o->angle = 2;
        for (i = 0; i < n; i++, part++) {
            part->unk30 = (i * 240) / 2;
            part->index = (i * 60) / 2;
            part->scale = o->scale;
            Vec_Copy(part->pos, o->pos);
            part->unk1C = -1;
            part->ang[1] = Rand_Next(1);
            part->unk02 = 0;
            part->slot = DrawSlotB_Alloc();
            if (part->slot != -1) {
                part->draw = DrawSlotB_Get(part->slot);
                part->draw->owner = o;
                part->draw->part = i;
                part->draw->draw = Effect06_DrawPart;
            } else {
                part->draw = NULL;
            }
        }
        return;
    case 8:
        o->angle = Rand_Next(1);
        o->scale -= 0.00040000002f * (Rand_Next(1) & 0x3FF);
        o->pos[0] = 0.0f;
        o->pos[1] = 20.0f;
        o->pos[2] = 0.0f;
        break;
    case 10:
        Effect06_PlaceAtBone(o);
        break;
    default:
        o->angle = em->rotY;
        break;
    }
    for (i = 0; i < n; part++, i++) {
        part->unk30 = 0;
        part->scale = o->scale;
        Vec_Copy(part->pos, o->pos);
        part->unk1C = -1;
        switch (o->type) {
        case 0:
            part->ang[0] = 0;
            part->ang[1] = 0;
            if (i == 2) {
                part->unk34 = 0;
                part->ang[2] = 0;
                part->unk26 = 0;
                part->unk02 = 1;
            } else {
                if (i == 0 || i == 1) {
                    part->unk02 = 0;
                } else {
                    part->unk02 = 2;
                }
                part->unk34 = 1;
                part->ang[2] = 0;
                part->unk26 = 0;
            }
            break;
        case 1:
            part->ang[0] = Rand_Next(1);
            part->ang[1] = Rand_Next(1);
            part->ang[2] = 0;
            part->unk26 = -0x13B0;
            part->unk34 = 1;
            part->unk02 = i;
            part->unk30 = i * -3;
            part->scale += 0.1f * i;
            break;
        case 2:
            part->unk02 = i;
            part->unk34 = 0;
            part->unk30 = i * -2;
            break;
        case 3:
            part->scale = o->scale * (1.0f + 0.00020000001f * (Rand_Next(1) & 0x3FF));
            part->unk02 = i & 1;
            part->unk34 = 1;
            part->unk30 = gEff06PartAges3[i];
            part->ang[0] = 0;
            part->ang[1] = 0;
            if (part->unk02 == 0) {
                part->ang[2] = Rand_Next(1);
                part->pos[0] = (Rand_Next(1) & 0x1F) - 16;
                part->pos[1] = (Rand_Next(1) & 0x1F) - 16;
                part->pos[2] = (Rand_Next(1) & 0x1F) - 16;
            } else {
                part->ang[2] = 0;
                part->unk02 += (s16)(Rand_Next(1) % 3);
            }
            part->unk26 = 0.5f + 65536.0f * (0.001f * (Rand_Next(1) & 0x3FF)) / 360.0f;
            break;
        case 4:
        case 8:
            part->unk02 = i;
            part->unk34 = 1;
            part->unk30 = gEff06PartAges4[i];
            part->ang[0] = 0;
            part->ang[1] = 0;
            if (part->unk02 == 0) {
                part->ang[2] = 0;
            } else if (part->unk02 == 1) {
                part->ang[2] = Rand_Next(1);
            } else {
                part->ang[2] = Rand_Next(1);
                part->unk02 += (s16)(Rand_Next(1) % 3);
            }
            part->unk26 = 0;
            break;
        case 5:
        case 9:
            part->unk02 = i;
            part->unk04[0] = 1.0f;
            part->unk04[1] = 1.0f;
            part->unk04[2] = 1.0f;
            part->unk34 = 0;
            part->unk30 = 0;
            part->ang[0] = Rand_Next(1);
            part->ang[1] = Rand_Next(1);
            part->ang[2] = Rand_Next(1);
            part->unk26 = 0;
            break;
        case 6:
            switch (i) {
            case 0:
            case 1:
            case 2:
            case 3:
                part->unk02 = 0;
                part->unk30 = -i * 5;
                break;
            case 4:
                part->unk02 = 1;
                part->unk30 = 0;
                part->unk1C = 0xCCFFBF5F;
                break;
            case 5:
                part->unk02 = 2;
                part->unk30 = -2;
                break;
            }
            part->ang[2] = 0;
            part->unk26 = 0;
            part->unk34 = 1;
            part->kind = 0;
            break;
        case 10:
            part->unk02 = i;
            part->scale = 0.8f;
            part->ang[2] = Rand_Next(1);
            part->unk26 = 0x222;
            part->unk34 = 1;
            break;
        }
        part->slot = DrawSlot_Alloc();
        if (part->slot != -1) {
            part->draw = DrawSlot_Get(part->slot);
            part->draw->owner = o;
            part->draw->part = i;
            part->draw->draw = Effect06_DrawPart;
        } else {
            part->draw = NULL;
        }
    }
}

void Vec_Scale(f32 *out, f32 *v, f32 s);
/* A part of a kind 6 effect with o->unk005 frames of life (15 for 0 and 0xFF): 0 moves it on (shrinking, at a random offset turned with the owner's bone), 1 marks it done (+0x30 = -2) before its end. */
void Effect06_StepPart(StageObj *o, EffectPart *part, s16 step)
{
    f32 m[4][4];
    u8 n;

    n = o->var0;
    if (n == 0 || n == 0xFF) {
        n = 15;
    } else {
        n = o->var0;
    }
    switch (step) {
    case 0:
        part->unk02 += 4;
        part->scale = o->scale * (1.0f - 0.8f / n * part->unk02);
        part->ang[2] = Rand_Next(1);
        Mtx_Copy(m[0], (f32 *)(Em_GetBone(o->owner, o->var1)));
        part->pos[0] = o->scale * (f32)((Rand_Next(1) & 0x1F) - 16);
        part->pos[1] = o->scale * (f32)((Rand_Next(1) & 0x1F) - 16);
        part->pos[2] = o->scale * (f32)((Rand_Next(1) & 0x1F) - 16);
        if (o->var0 == 0xFF) {
            Vec_Scale(part->pos, part->pos, 0.6f);
            part->pos[0] += 45.0f;
            part->pos[1] += 10.0f;
            part->pos[2] += 32.0f;
        }
        Vec_RotateSelf(part->pos, m[0]);
        part->pos[0] += m[3][0];
        part->pos[1] += m[3][1];
        part->pos[2] += m[3][2];
        break;
    case 1:
        if (part->unk02 + 4 < n) {
            part->unk30 = -2;
        }
        break;
    }
}

/* Places part i of a kind 6 effect: its offset from the object, turned with the owner's bone +0x06, at the bone; copied to the next part (below 5), with the sound for the first. */
void Effect06_InitPartAtBone(StageObj *o, EffectPart *part, s16 i)
{
    f32 m[4][4];

    part->ang[2] = Rand_Next(1);
    Mtx_Copy(m[0], (f32 *)(Em_GetBone(o->owner, o->var1)));
    part->pos[0] += o->pos[0];
    part->pos[1] += o->pos[1];
    part->pos[2] += o->pos[2];
    Vec_RotateSelf(part->pos, m[0]);
    part->pos[0] += m[3][0];
    part->pos[1] += m[3][1];
    part->pos[2] += m[3][2];
    if (i < 5) {
        Vec_Copy(part[1].pos, part->pos);
    }
    if (i == 0) {
        Effect06_PlaySound(o, part->pos);
    }
}

/* Places part i of a kind 6 effect: the first at a random offset (-16..15) from the object, turned with the owner's bone +0x06; parts 1 and 2 where the first is. */
void Effect06_InitPartsJitter(StageObj *o, EffectPart *parts, s16 i)
{
    s32 n = i;
    f32 m[4][4];

    switch (n) {
    case 0:
        Mtx_Copy(m[0], (f32 *)(Em_GetBone(o->owner, o->var1)));
        parts[n].pos[0] = o->pos[0] + (f32)((Rand_Next(1) & 0x1F) - 16);
        parts[n].pos[1] = o->pos[1] + (f32)((Rand_Next(1) & 0x1F) - 16);
        parts[n].pos[2] = o->pos[2] + (f32)((Rand_Next(1) & 0x1F) - 16);
        Vec_RotateSelf(parts[n].pos, m[0]);
        parts[n].pos[0] += m[3][0];
        parts[n].pos[1] += m[3][1];
        parts[n].pos[2] += m[3][2];
        break;
    case 1:
    case 2:
        Vec_Copy(parts[n].pos, parts[0].pos);
        break;
    }
}

/* Starts a part of a kind 6 effect: a random angle, the object's +0x07, and its scale growing with the part's index (at most 0.8 of the object's; kinds 0 and 1). */
void Effect06_InitPartScaled(StageObj *o, EffectPart *part, s16 i)
{
    f32 max;

    part->ang[2] = Rand_Next(1);
    part->kind = o->var2;
    part->index = o->timer++;
    part->ang[2] = Rand_Next(1);
    switch (part->kind) {
    case 0:
    case 1:
        part->scale = o->scale * (0.5f + 0.05f * part->index);
        max = 0.8f * o->scale;
        if (part->scale > max) {
            part->scale = max;
            part->index--;
            o->timer--;
        }
        break;
    case 2:
    default:
        part->scale = o->scale;
        break;
    }
}

/* A part of an effect: random angles (+0x20..+0x24), its timer (+0x30) at 0. */
void Effect06_RandomizePart(StageObj *o, u8 *part)
{
    *(s16 *)(part + 0x30) = 0;
    *(u16 *)(part + 0x20) = Rand_Next(1);
    *(u16 *)(part + 0x22) = Rand_Next(1);
    *(u16 *)(part + 0x24) = Rand_Next(1);
}

EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *out, f32 *a, f32 *b);
/* Puts the object at an offset (45, 10, 32) from its owner's bone +0x06, turned with the bone. */
void Effect06_PlaceAtBone(StageObj *o)
{
    f32 v[3];
    f32 m[4][4];

    Mtx_Copy(m[0], (f32 *)(Em_GetBone(o->owner, o->var1)));
    v[0] = 45.0f;
    v[1] = 10.0f;
    v[2] = 32.0f;
    Vec_RotateSelf(v, m[0]);
    Mtx_GetTrans(o->pos, m[0]);
    Vec_Add(o->pos, o->pos, v);
}

s32 Effect06_OwnerGone(u8 *p)
{
    if (p[0] == 0 || p[4] == 3) {
        return 1;
    }
    return 0;
}

static void Effect06_CountPart(EffectPart *part, s16 *a, s16 b, s16 *c)
{
    *a += b;
    if (part->unk34 != 0) {
        (*c)++;
    }
}

/* 1 when none of the n records (0x38 bytes) has its +2 id set (0xFF = none). */
s32 Effect06_AllPartsDone(u8 *p, s16 n)
{
    s16 i;

    for (i = 0; i < n; i++, p += 0x38) {
        if (*(s16 *)(p + 2) != 0xFF) {
            return 0;
        }
    }
    return 1;
}

extern u32 gEff06PartDurTbls[];
extern u32 gEff06Times[];
extern u32 gEff06PosKeyStart[];
extern u32 gEff06PosKeyStep[];
extern u32 gEff06ColorKeyStart[];
extern u32 gEffect06PosKeys[];
extern u32 gEffect06ColorKeys[];
extern f32 gCamMatrix[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
f32 Math_Sin(f32 x);
void Vec_Sub(f32 *out, f32 *a, f32 *b);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void Effect0D_SpawnForPlayerScaled(Player *p, s16 arg, s32 type, f32 scale);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

/* Done with this part: advance the key indices, go to the next part. */
#define NEXT_PART { Effect06_CountPart(part, &a, n2, &c); part++; continue; }

/*
 * The run state of a kind 6 effect object: follows the owner's bone by type,
 * ends the object (state 2) when its time (gEff06Times) is up (type 6: when the
 * owner's weapon motion ends and every part is gone), then animates each part:
 * its age (unk30) against its duration, its scale keys (gEffect06PosKeys) and color
 * keys (gEffect06ColorKeys), and adds its draw slot to the ordering table. Type 7 (the
 * status icon effect) runs its own loop.
 */
void Effect06_Run(StageObj *o)
{
    EffectPart *part;
    EffectPart *part0;
    u16 *durs;
    s16 n;
    u16 dur6;
    s16 n2;
    s16 i;
    u16 dur;
    u8 flag;
    Player *pl;
    u8 k;
    void *p;
    s16 prev;
    f32 e[3];
    f32 d[3];
    s16 a;
    s16 c;

    part = (EffectPart *)o->buf;
    flag = 0;
    n = gEff06PartCounts[o->type];
    dur6 = ((u16 *)gEff06Times)[o->type];
    durs = ((u16 **)gEff06PartDurTbls)[o->type];
    a = ((s16 *)gEff06PosKeyStart)[o->type];
    n2 = ((s16 *)gEff06PosKeyStep)[o->type];
    c = ((s16 *)gEff06ColorKeyStart)[o->type];
    part0 = part;
    if (o->var2 != 0xFF) {
        if (Effect06_OwnerGone(o->owner) != 0) {
            o->var2 = 0xFF;
        } else if (o->type == 7) {
            o->var2 = 0;
        }
    }
    switch (o->type) {
    case 0:
        c += (s16)(o->var0 * 3);
        if (o->var2 != 0xFF) {
            if (o->timer == 10) {
                Effect06_SpawnAtBone(o->owner, 2, 0, o->var1, o->scale);
            }
            Em_GetBonePos(o->owner, o->var1, o->pos);
        }
        break;
    case 1:
        if (o->var2 != 0xFF) {
            Em_GetBonePos(o->owner, o->var1, o->pos);
        }
        dur = 11;
        break;
    case 2:
        k = o->var0;
        if (k == 0 || k == 0xFF) {
            k = 15;
        } else {
            k = o->var0;
        }
        dur = 6;
        dur6 = (k - 1) * 2 + 6;
        break;
    case 5:
        if (o->var2 != 0xFF) {
            Em_GetBonePos(o->owner, o->var1, o->pos);
            if (o->timer == 0) {
                Effect06_PlaySound(o, o->pos);
            }
        }
        dur = 12;
        break;
    case 6:
        if (o->var2 != 0xFF) {
            Em_GetBonePos(o->owner, o->var1, o->pos);
            prev = o->var2;
            o->var2 = ((Player *)o->owner)->weaponTimer / 30;
            pl = o->owner;
            if (pl->weaponType == WEAPON_BOW && pl->weaponTimer == 90) {
                Effect0D_SpawnForPlayerScaled(pl, 2, 0x23, 1.0f);
            }
            if (o->var2 > 2) {
                o->var2 = 2;
            }
            if (prev != o->var2) {
                Effect06_PlaySound(o, o->pos);
            }
        }
        break;
    case 7:
        if (o->var2 != 0xFF) {
            if (!((u8)Obj_InCurrentArea(o->owner) & 0xFF)) {
                return;
            }
            if (gScene.eventRunning == 1) {
                return;
            }
            Em_GetBonePos(o->owner, o->var1, e);
            Vec_Sub(d, e, o->pos);
            Vec_Copy(o->pos, e);
            Em_GetBonePos(o->owner, 2, e);
            e[1] = o->pos[1] - 20.0f;
        } else {
            return;
        }
        break;
    case 9:
        if (o->var2 != 0xFF) {
            Em_GetBonePos(o->owner, o->var1, o->pos);
            if (o->timer == 0) {
                Effect06_PlaySound(o, o->pos);
            }
        }
        dur = 4;
        break;
    case 10:
        dur = 10;
        if (o->var2 != 0xFF) {
            if (o->timer == 1) {
                Effect06_SpawnAtBone(o->owner, 2, 0xFF, o->var1, 4.0f * o->scale);
            }
            Em_GetBonePos(o->owner, o->var1, o->pos);
        }
        break;
    }
    if (o->type == 6) {
        if (o->var2 != 0xFF) {
            pl = o->owner;
            if (pl->weaponType == WEAPON_HAMMER) {
                if (pl->motionId[0] != 0x3ED && pl->motionId[0] != 0x3EE && pl->motionId[0] != 0x3EF) {
                    o->var2 = 0xFF;
                }
            } else if (pl->weaponType == WEAPON_BOW) {
                if (pl->motionId[0] != 0x57D && pl->motionId[0] != 0x3F1 && pl->motionId[0] != 0x3F4 &&
                    pl->motionId[0] != 0x3F5 && pl->motionId[0] != 0x58A && pl->motionId[0] != 0x58B &&
                    pl->motionId[0] != 0x58C) {
                    o->var2 = 0xFF;
                }
            } else {
                o->var2 = 0xFF;
            }
        } else if (Effect06_AllPartsDone((u8 *)part, n) != 0) {
            o->state++;
            o->visible = 0;
            return;
        }
    } else if (o->type == 7) {
        switch (((Player *)o->owner)->statusIcons & 0x30) {
        default:
            o->var0 = 0;
            break;
        case 0x10:
            o->var0 = 1;
            break;
        case 0x20:
            o->var0 = 2;
            break;
        case 0x30:
            o->var0 = 3;
            break;
        }
        for (i = 0; i < n; i++, part++) {
            switch (o->var0) {
            case 0:
                Vec_Copy(part->pos, e);
                continue;
            case 1:
            case 2:
                part->unk30 += 2;
                if (part->unk02 == 1) {
                    part->index += 4;
                } else {
                    part->index += 2;
                }
                break;
            case 3:
                part->unk30 += 4;
                if (part->unk02 == 1) {
                    part->index += 6;
                } else {
                    part->index += 3;
                }
                break;
            }
            if (part->unk30 >= 240) {
                part->unk30 = 0;
            }
            if (part->index >= 60) {
                o->angle++;
                if (o->angle >= 2) {
                    o->angle = 0;
                    part->unk02 = 1;
                } else {
                    part->unk02 = 0;
                }
                part->ang[1] = Rand_Next(1);
                part->index = 0;
                Vec_Copy(part->pos, e);
            } else {
                part->pos[0] += d[0];
                part->pos[1] += d[1];
                part->pos[2] += d[2];
            }
            if (part->draw != NULL) {
                Vec_Copy(part->draw->pos, part->pos);
                OT_AddByDepth(gDrawOtSorted, part->draw, 0x40, 0);
            }
        }
        return;
    } else if (++o->timer >= dur6) {
        o->state++;
        o->visible = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        switch (o->type) {
        case 6:
            switch (part->unk02) {
            case 0:
                switch (part->kind) {
                case 0:
                case 1:
                    dur = 14;
                    break;
                case 2:
                default:
                    dur = 12;
                    break;
                }
                break;
            case 1:
                switch (o->var2) {
                case 0:
                    dur = 30;
                    break;
                case 1:
                    dur = 30;
                    switch (part->kind) {
                    case 0:
                        part->unk30 = 0;
                        part->kind++;
                    }
                    break;
                case 2:
                    switch (part->kind) {
                    case 1:
                        part->unk30 = 0;
                        part->kind++;
                    }
                    dur = 6;
                    break;
                default:
                    part->unk02 = 0xFF;
                    NEXT_PART
                }
                break;
            case 2:
                switch (o->var2) {
                case 0:
                case 1:
                    NEXT_PART
                case 2:
                default:
                    dur = 4;
                    break;
                }
                break;
            }
            break;
        case 1:
            a = ((s16 *)gEff06PosKeyStart)[o->type];
            c = ((s16 *)gEff06ColorKeyStart)[o->type] + o->var0;
            break;
        default:
            if (durs != NULL) {
                dur = durs[i];
            } else {
                a = ((s16 *)gEff06PosKeyStart)[o->type];
                c = ((s16 *)gEff06ColorKeyStart)[o->type];
            }
            break;
        }
        if (++part->unk30 <= 0 || part->unk02 == 0xFF) {
            NEXT_PART
        }
        if (part->unk30 > dur) {
            switch (o->type) {
            case 6:
                switch (part->unk02) {
                case 0:
                    switch (o->var2) {
                    case 0:
                    case 1:
                        part->unk30 = -6;
                        break;
                    case 2:
                    default:
                        part->unk30 = -4;
                        break;
                    }
                    break;
                case 1:
                    switch (o->var2) {
                    case 0:
                    case 1:
                        part->unk30 = dur;
                        break;
                    case 2:
                    default:
                        part->unk02 = 0xFF;
                        NEXT_PART
                    }
                    break;
                case 2:
                    part->unk02 = 0xFF;
                    NEXT_PART
                }
                break;
            case 9:
                Effect06_RandomizePart(o, (u8 *)part);
                break;
            case 2:
                Effect06_StepPart(o, part, 1);
                NEXT_PART
            default:
                NEXT_PART
            }
        }
        switch (o->type) {
        case 0:
        case 1:
        case 5:
        case 9:
            Vec_Copy(part->pos, o->pos);
            break;
        case 2:
            if (part->unk30 == 1) {
                if (o->var2 != 0xFF) {
                    Effect06_StepPart(o, part, 0);
                } else {
                    part->unk02 = 0xFF;
                    NEXT_PART
                }
            }
            break;
        case 3:
            if (part->unk30 == 1) {
                if (o->var2 != 0xFF) {
                    if (part->unk02 == 0) {
                        Effect06_InitPartAtBone(o, part, i);
                    }
                } else {
                    part->unk02 = 0xFF;
                    NEXT_PART
                }
            }
            if (part->unk02 == 0) {
                part->pos[1] += o->scale * (25.0f / dur);
            } else {
                part->pos[1] += o->scale * (50.0f / dur);
            }
            break;
        case 4:
        case 8:
            if (part->unk02 == 0 || part->unk02 == 1) {
                d[0] = o->scale * ((o->angle & 0xF) + 15) *
                       (Math_Sin((6.2831855f * (o->timer << 11)) / 65536.0f) - 0.5f);
                d[1] = 0.5f * o->scale * ((((o->angle & 0x300) >> 8) + 2) * o->timer);
                d[2] = 0.0f;
                Vec_RotateSelf(d, gCamMatrix);
                flag = 1;
            }
            if (part->unk30 == 1) {
                if (part->unk02 == 0) {
                    if (o->var2 != 0xFF) {
                        Effect06_InitPartsJitter(o, part0, i);
                    } else {
                        part->unk02 = 0xFF;
                        NEXT_PART
                    }
                } else if (part0->unk02 != 0xFF) {
                    Effect06_InitPartsJitter(o, part0, i);
                } else {
                    part->unk02 = 0xFF;
                    NEXT_PART
                }
            }
            break;
        case 6:
            if (part->unk30 == 1) {
                if (o->var2 != 0xFF) {
                    if (part->unk02 == 0) {
                        Effect06_InitPartScaled(o, part, i);
                    }
                } else {
                    part->unk02 = 0xFF;
                    NEXT_PART
                }
            }
            switch (part->unk02) {
            case 0:
                switch (part->kind) {
                default:
                    a = 15;
                    c = 0x31;
                    Vec_Copy(part->pos, o->pos);
                    break;
                case 1:
                    a = 15;
                    c = 0x32;
                    Vec_Copy(part->pos, o->pos);
                    break;
                case 2:
                    a = 16;
                    c = 0x33;
                    if (o->var2 != 0xFF) {
                        Em_GetBonePos(o->owner, 0x12, part->pos);
                    }
                    break;
                }
                break;
            case 1:
                switch (o->var2) {
                case 0:
                    part->unk04[1] = part->unk04[0] = (1.5f * part->unk30) / 30.0f;
                    part->unk04[2] = part->unk04[0];
                    if (part->draw != NULL) {
                        part->draw->pos[0] = o->pos[0];
                        part->draw->pos[1] = o->pos[1];
                        part->draw->pos[2] = o->pos[2];
                        OT_AddByDepth(gDrawOtSorted, part->draw, 0x40, 0);
                    }
                    NEXT_PART
                case 1:
                    part->unk04[1] = part->unk04[0] = 1.5f + (1.5f * part->unk30) / 30.0f;
                    part->unk04[2] = part->unk04[0];
                    if (part->draw != NULL) {
                        part->draw->pos[0] = o->pos[0];
                        part->draw->pos[1] = o->pos[1];
                        part->draw->pos[2] = o->pos[2];
                        OT_AddByDepth(gDrawOtSorted, part->draw, 0x40, 0);
                    }
                    NEXT_PART
                case 2:
                    a = 0x11;
                    c = 0x34;
                    Vec_Copy(part->pos, o->pos);
                    break;
                }
                break;
            case 2:
                a = 0x12;
                c = 0x35;
                if (o->var2 != 0xFF) {
                    Em_GetBonePos(o->owner, 0x12, part->pos);
                } else {
                    part->unk02 = 0xFF;
                    NEXT_PART
                }
                break;
            }
            break;
        }
        if (n2 > 0) {
            p = (void *)gEffect06PosKeys[a++];
            Key_LerpVec3(p, part->unk04, part->unk30);
        }
        part->ang[2] += part->unk26;
        if (part->unk34 != 0 && c != -1) {
            Color_EvalKeys((s32 *)gEffect06ColorKeys[c++], part->unk30, (u32 *)&part->unk1C);
        }
        if (part->draw != NULL) {
            if (flag) {
                flag = 0;
                part->draw->pos[0] = part->pos[0] + d[0];
                part->draw->pos[1] = part->pos[1] + d[1];
                part->draw->pos[2] = part->pos[2] + d[2];
            } else {
                part->draw->pos[0] = part->pos[0];
                part->draw->pos[1] = part->pos[1];
                part->draw->pos[2] = part->pos[2];
            }
            if (o->type == 5 || o->type == 9) {
                OT_AddByDepth(gDrawOt32, part->draw, 0x20, 0);
            } else {
                OT_AddByDepth(gDrawOtSorted, part->draw, 0x40, 0);
            }
        }
        part++;
    }
}

#undef NEXT_PART

void DrawSlotB_Free(s32 id);
extern s16 gEff06PartCounts[];
/* Frees a kind 6 effect object's draw slots (the types listed in gEff06PartCounts have that many parts in their buffer). */
void Effect06_End(StageObj *o)
{
    EffectPart *part = (EffectPart *)o->buf;
    s16 n;
    s16 i;

    o->state++;
    n = gEff06PartCounts[o->type];
    if (n == 0) {
        if (*(s32 *)&o->node != 0) {
            if (o->keep) {
                DrawSlotB_Free(o->slot);
            } else {
                DrawSlot_Free(o->slot);
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

void Effect06_Delete(StageObj *o)
{
    StageObj_Free(o);
}

s32 Gs_SetState(u8 reg, u32 v);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
s32 Effect_DrawModelAdd(void *arg0, f32 *arg1, void *arg2);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
s16 Effect_MapTexId(s16 id);
extern f32 gCamMatrix[];
extern s32 *gEff06IconColorKeys[];
extern s32 *D_004666EC;
extern s32 *gEff06IconColorKeys3[];
extern s32 gEff06IconAlphaKeys[];
/* Draws a part of a kind 6 effect object (its draw slot): a model of set 0 by the object type, scaled, turned and placed at the slot. */
void Effect06_DrawPart(DrawSlot *s)
{
    StageObj *o = s->owner;
    EffectPart *part = &((EffectPart *)o->buf)[s->part];
    void *base;
    u16 flags = 0;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    f32 t[16];
    ModelMatSet *mdl;
    s32 *tbl;
    s32 *atbl;
    u32 color;
    u32 alpha;
    s16 id;
    Model *set = gCommonModels[0];

    if (set != NULL && set->active) {
        base = set->mats;
        if (o->type == 7) {
            switch (o->var0) {
            default:
                return;
            case 1:
                tbl = gEff06IconColorKeys[0];
                break;
            case 2:
                tbl = D_004666EC;
                break;
            case 3:
                tbl = gEff06IconColorKeys3[0];
                break;
            }
            atbl = gEff06IconAlphaKeys;
            Gs_SetState(0x6C, 0);
            if (part->unk02 == 0) {
                mdl = &set->matSets[61];
                Color_EvalKeys(tbl, part->unk30, &color);
            } else {
                mdl = &set->matSets[143];
                color = -1;
            }
            Color_EvalKeys(atbl, part->index, &alpha);
            color &= 0xFFFFFF;
            color |= alpha & 0xFF000000;
            scale[0] = part->scale;
            scale[1] = part->scale * part->index / 60.0f;
            scale[2] = part->scale;
            rot[1] = part->ang[1];
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 2, m);
            Effect_DrawModel(mdl, m, color, 2, base);
            Gs_SetState(0x6C, 1);
        } else {
            scale[0] = part->scale * part->unk04[0];
            scale[1] = part->scale * part->unk04[1];
            scale[2] = part->scale * part->unk04[2];
            rot[2] = 6.2831855f * part->ang[2] / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            switch (o->type) {
            case 0:
                Mtx_Concat3x3(m, gCamMatrix);
                Gs_SetState(0x6C, 0);
                flags |= 2;
                switch (part->unk02) {
                case 0:
                    id = 0x54;
                    break;
                case 1:
                    id = 0x25;
                    if (gScene.online == 0) {
                        Mtx_Translation(t, 0.0f, 0.125f, 0.0f);
                    } else {
                        Mtx_Translation(t, 0.0f, 0.25f, 0.0f);
                    }
                    Gs_SetState(0x19, (s32)t);
                    break;
                case 2:
                    id = 0x58;
                    break;
                }
                break;
            case 1:
                rot[0] = 6.2831855f * part->ang[0] / 65536.0f;
                rot[1] = 6.2831855f * part->ang[1] / 65536.0f;
                Mtx_RotateX(m, rot[0]);
                Mtx_RotateY(m, rot[1]);
                Gs_SetState(0x6C, 0);
                flags |= 2;
                id = 0x26;
                break;
            case 2:
                Mtx_Concat3x3(m, gCamMatrix);
                Gs_SetState(0x6C, 0);
                flags |= 2;
                id = 0x25;
                Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32)t);
                break;
            case 3:
                Mtx_Concat3x3(m, gCamMatrix);
                flags |= 2;
                atbl = gEff06IconAlphaKeys;
                Gs_SetState(0x6C, 0);
                if (part->unk02 == 0) {
                    id = 0x4A;
                } else {
                    id = part->unk02 + 0x2C;
                }
                break;
            case 4:
            case 8:
                Mtx_Concat3x3(m, gCamMatrix);
                flags |= 2;
                atbl = gEff06IconAlphaKeys;
                Gs_SetState(0x6C, 0);
                if (part->unk02 == 0) {
                    id = 0x27;
                } else if (part->unk02 == 1) {
                    id = 0x30;
                } else {
                    id = part->unk02 + 0x2B;
                }
                break;
            case 5:
            case 9:
                rot[0] = 6.2831855f * part->ang[0] / 65536.0f;
                rot[1] = 6.2831855f * part->ang[1] / 65536.0f;
                Mtx_RotateX(m, rot[0]);
                Mtx_RotateY(m, rot[1]);
                id = 0x31;
                Mtx_Translation(t, 0.0f, 0.03125f * part->unk30, 0.0f);
                Gs_SetState(0x19, (s32)t);
                break;
            case 6:
                Mtx_Concat3x3(m, gCamMatrix);
                Gs_SetState(0x6C, 0);
                flags |= 2;
                switch (part->unk02) {
                case 0:
                    id = 0x42;
                    Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
                    Gs_SetState(0x19, (s32)t);
                    break;
                case 1:
                case 2:
                    id = 0x40;
                    break;
                default:
                    return;
                }
                break;
            case 10:
                Mtx_Concat3x3(m, gCamMatrix);
                Gs_SetState(0x6C, 0);
                flags |= 2;
                id = 0x51;
                break;
            }
            id = Effect_MapTexId(id);
            if (id != -1) {
                mdl = &set->matSets[id];
                if (o->type == 5) {
                    Gs_SetState(0x67, -1);
                    Effect_DrawModelAdd(mdl, m, base);
                } else {
                    Effect_DrawModel(mdl, m, part->unk1C, flags, base);
                }
            }
            Gs_SetState(0x6C, 1);
        }
    }
}

s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);

/* The sound of a kind 6 effect object at pos, by type (type 6: by +0x07, from its owner). */
void Effect06_PlaySound(StageObj *o, f32 *pos)
{
    switch (o->type) {
    case 0:
        Sound_PlaySeAtPosChecked(1, 0x39, 0, pos, 1, 0);
        break;
    case 3:
        Sound_PlaySeAtPosChecked(1, 0x23, 0, pos, 1, 0);
        break;
    case 5:
    case 9:
        Sound_PlaySeAtPosChecked(1, 0x22, 0, pos, 1, 0);
        break;
    case 6:
        switch (o->var2) {
        case 0:
            Player_PlaySe(o->owner, 0xC, 0, o->pos, 1, 0);
            break;
        case 1:
            Player_PlaySe(o->owner, 0xD, 0, o->pos, 1, 0);
            break;
        case 2:
            Player_PlaySe(o->owner, 0xE, 0, o->pos, 1, 0);
        }
        break;
    }
}

/* A kind 6 effect object of the given type for a player (Effect06_Update updates it); NULL when the player is elsewhere (type 7 and lance users with +0x10/+0x1E set are always made). */
StageObj *Effect06_New(Player *p, s16 type, u8 sub)
{
    StageObj *o;

    if (type != 7 && (p->isEm == 0 || p->ownTask == 0 || p->weaponType != WEAPON_LANCE) && (u8)Obj_InCurrentArea(p) == 0) {
        return NULL;
    }
    o = StageObj_Alloc(1);
    if (o == NULL) {
        return NULL;
    }
    o->kind = 6;
    o->update = Effect06_Update;
    o->owner = p;
    o->type = type;
    o->var0 = sub;
    return o;
}

s32 Player_IsLocal(void *obj);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
/* A kind 6 effect of the given type at one of the player's bones (type 0 also makes type 1; type 6 only for the local player). */
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale)
{
    StageObj *o;

    if (type != 6 || Player_IsLocal(p) != 0) {
        if ((o = Effect06_New(p, type, sub)) != NULL) {
            o->scale = scale;
            Em_GetBonePos(p, bone, o->pos);
            o->var1 = bone;
            if (type == 0) {
                Effect06_SpawnAtBone(p, 1, sub, bone, scale);
            } else if (type == 7) {
                o->keep = 1;
            }
        }
    }
}

StageObj *Effect06_New(Player *p, s16 type, u8 sub);
void Vec_Copy(f32 *dst, f32 *src);

/* A kind 6 effect of the given type for a player, at pos. */
void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale)
{
    StageObj *o = Effect06_New(obj, type, 0);

    if (o != NULL) {
        o->scale = scale;
        o->var1 = bone;
        Vec_Copy(o->pos, pos);
    }
}

extern f32 gEffect06EmScales[];
extern f32 gEffect06Scales[];
/* A kind 6 effect at the player's bone 2, sized by weapon for types 0 (also makes type 1) and 9. */
void Effect06_SpawnWeapon(Player *p, s16 type, u8 sub)
{
    StageObj *o;

    if ((o = Effect06_New(p, type, sub)) != NULL) {
        o->var1 = 2;
        Em_GetBonePos(p, o->var1, o->pos);
        switch (o->type) {
        case 0:
            if (p->isEm == 0) {
                o->scale = 4.0f;
            } else {
                o->scale = p->scale[0] * gEffect06EmScales[p->weaponType];
            }
            Effect06_SpawnAtBone(p, 1, sub, o->var1, o->scale);
            break;
        case 9:
            if (p->isEm == 0) {
                o->scale = 0.75f;
            } else {
                o->scale = p->scale[0] * gEffect06Scales[p->weaponType];
            }
            break;
        default:
            o->scale = 1.0f;
            break;
        }
    }
}

/* unk_3CA790's .data (0x4657A0-0x466940) */
u32 gEffect06PosKeys_3[16] = {
#include "sub_main/004657A0.inc"
};
u32 gEffect06PosKeys_0[20] = {
#include "sub_main/004657E0.inc"
};
u32 gEffect06PosKeys_1[20] = {
#include "sub_main/00465830.inc"
};
u32 gEffect06PosKeys_2[20] = {
#include "sub_main/00465880.inc"
};
u32 gEffect06PosKeys_4[12] = {
#include "sub_main/004658D0.inc"
};
u32 gEffect06PosKeys_5[20] = {
#include "sub_main/00465900.inc"
};
u32 gEffect06PosKeys_6[16] = {
#include "sub_main/00465950.inc"
};
u32 gEffect06PosKeys_7[12] = {
#include "sub_main/00465990.inc"
};
u32 gEffect06PosKeys_12[40] = {
#include "sub_main/004659C0.inc"
};
u32 gEffect06PosKeys_13[20] = {
#include "sub_main/00465A60.inc"
};
u32 gEffect06PosKeys_14[12] = {
#include "sub_main/00465AB0.inc"
};
u32 gEffect06PosKeys_15[16] = {
#include "sub_main/00465AE0.inc"
};
u32 gEffect06PosKeys_16[12] = {
#include "sub_main/00465B20.inc"
};
u32 gEffect06PosKeys_17[12] = {
#include "sub_main/00465B50.inc"
};
u32 gEffect06PosKeys_18[12] = {
#include "sub_main/00465B80.inc"
};
u32 gEffect06PosKeys_19[16] = {
#include "sub_main/00465BB0.inc"
};
u32 gEffect06PosKeys[20] = {
    (u32)gEffect06PosKeys_0, (u32)gEffect06PosKeys_1, (u32)gEffect06PosKeys_2, (u32)gEffect06PosKeys_3,
    (u32)gEffect06PosKeys_4, (u32)gEffect06PosKeys_5, (u32)gEffect06PosKeys_6, (u32)gEffect06PosKeys_7,
    (u32)gEffect06PosKeys_6, (u32)gEffect06PosKeys_7, (u32)gEffect06PosKeys_6, (u32)gEffect06PosKeys_7,
    (u32)gEffect06PosKeys_12, (u32)gEffect06PosKeys_13, (u32)gEffect06PosKeys_14, (u32)gEffect06PosKeys_15,
    (u32)gEffect06PosKeys_16, (u32)gEffect06PosKeys_17, (u32)gEffect06PosKeys_18, (u32)gEffect06PosKeys_19,
};
u32 gEffect06ColorKeys_30[12] = {
#include "sub_main/00465C40.inc"
};
u32 gEffect06ColorKeys_2[12] = {
#include "sub_main/00465C70.inc"
};
u32 gEffect06ColorKeys_0[16] = {
#include "sub_main/00465CA0.inc"
};
u32 gEffect06ColorKeys_1[12] = {
#include "sub_main/00465CE0.inc"
};
u32 gEffect06ColorKeys_31[12] = {
#include "sub_main/00465D10.inc"
};
u32 gEffect06ColorKeys_5[12] = {
#include "sub_main/00465D40.inc"
};
u32 gEffect06ColorKeys_3[16] = {
#include "sub_main/00465D70.inc"
};
u32 gEffect06ColorKeys_4[12] = {
#include "sub_main/00465DB0.inc"
};
u32 gEffect06ColorKeys_32[12] = {
#include "sub_main/00465DE0.inc"
};
u32 gEffect06ColorKeys_8[12] = {
#include "sub_main/00465E10.inc"
};
u32 gEffect06ColorKeys_6[16] = {
#include "sub_main/00465E40.inc"
};
u32 gEffect06ColorKeys_7[12] = {
#include "sub_main/00465E80.inc"
};
u32 gEffect06ColorKeys_33[12] = {
#include "sub_main/00465EB0.inc"
};
u32 gEffect06ColorKeys_11[12] = {
#include "sub_main/00465EE0.inc"
};
u32 gEffect06ColorKeys_9[16] = {
#include "sub_main/00465F10.inc"
};
u32 gEffect06ColorKeys_10[12] = {
#include "sub_main/00465F50.inc"
};
u32 gEffect06ColorKeys_34[12] = {
#include "sub_main/00465F80.inc"
};
u32 gEffect06ColorKeys_14[12] = {
#include "sub_main/00465FB0.inc"
};
u32 gEffect06ColorKeys_12[16] = {
#include "sub_main/00465FE0.inc"
};
u32 gEffect06ColorKeys_13[12] = {
#include "sub_main/00466020.inc"
};
u32 gEffect06ColorKeys_35[8] = {
#include "sub_main/00466050.inc"
};
u32 gEffect06ColorKeys_17[8] = {
#include "sub_main/00466070.inc"
};
u32 gEffect06ColorKeys_15[12] = {
#include "sub_main/00466090.inc"
};
u32 gEffect06ColorKeys_16[12] = {
#include "sub_main/004660C0.inc"
};
u32 gEffect06ColorKeys_36[8] = {
#include "sub_main/004660F0.inc"
};
u32 gEffect06ColorKeys_20[8] = {
#include "sub_main/00466110.inc"
};
u32 gEffect06ColorKeys_18[12] = {
#include "sub_main/00466130.inc"
};
u32 gEffect06ColorKeys_19[12] = {
#include "sub_main/00466160.inc"
};
u32 gEffect06ColorKeys_37[8] = {
#include "sub_main/00466190.inc"
};
u32 gEffect06ColorKeys_23[12] = {
#include "sub_main/004661B0.inc"
};
u32 gEffect06ColorKeys_21[12] = {
#include "sub_main/004661E0.inc"
};
u32 gEffect06ColorKeys_22[12] = {
#include "sub_main/00466210.inc"
};
u32 gEffect06ColorKeys_38[8] = {
#include "sub_main/00466240.inc"
};
u32 gEffect06ColorKeys_26[8] = {
#include "sub_main/00466260.inc"
};
u32 gEffect06ColorKeys_24[12] = {
#include "sub_main/00466280.inc"
};
u32 gEffect06ColorKeys_25[12] = {
#include "sub_main/004662B0.inc"
};
u32 gEffect06ColorKeys_39[8] = {
#include "sub_main/004662E0.inc"
};
u32 gEffect06ColorKeys_29[8] = {
#include "sub_main/00466300.inc"
};
u32 gEffect06ColorKeys_27[12] = {
#include "sub_main/00466320.inc"
};
u32 gEffect06ColorKeys_28[12] = {
#include "sub_main/00466350.inc"
};
u32 gEffect06ColorKeys_40[16] = {
#include "sub_main/00466380.inc"
};
u32 gEffect06ColorKeys_41[16] = {
#include "sub_main/004663C0.inc"
};
u32 gEffect06ColorKeys_46[12] = {
#include "sub_main/00466400.inc"
};
u32 gEffect06ColorKeys_47[8] = {
#include "sub_main/00466430.inc"
};
u32 gEffect06ColorKeys_48[8] = {
#include "sub_main/00466450.inc"
};
u32 gEffect06ColorKeys_49[12] = {
#include "sub_main/00466470.inc"
};
u32 gEffect06ColorKeys_50[12] = {
#include "sub_main/004664A0.inc"
};
u32 gEffect06ColorKeys_51[12] = {
#include "sub_main/004664D0.inc"
};
u32 gEffect06ColorKeys_52[8] = {
#include "sub_main/00466500.inc"
};
u32 gEffect06ColorKeys_53[8] = {
#include "sub_main/00466520.inc"
};
u32 D_00466540[8] = {
#include "sub_main/00466540.inc"
};
u32 D_00466560[8] = {
#include "sub_main/00466560.inc"
};
u32 D_00466580[8] = {
#include "sub_main/00466580.inc"
};
s32 gEff06IconAlphaKeys[12] = {
#include "sub_main/004665A0.inc"
};
u32 D_004665D0[8] = {
#include "sub_main/004665D0.inc"
};
u32 D_004665F0[8] = {
#include "sub_main/004665F0.inc"
};
u32 gEffect06ColorKeys[54] = {
    (u32)gEffect06ColorKeys_0, (u32)gEffect06ColorKeys_1, (u32)gEffect06ColorKeys_2, (u32)gEffect06ColorKeys_3,
    (u32)gEffect06ColorKeys_4, (u32)gEffect06ColorKeys_5, (u32)gEffect06ColorKeys_6, (u32)gEffect06ColorKeys_7,
    (u32)gEffect06ColorKeys_8, (u32)gEffect06ColorKeys_9, (u32)gEffect06ColorKeys_10, (u32)gEffect06ColorKeys_11,
    (u32)gEffect06ColorKeys_12, (u32)gEffect06ColorKeys_13, (u32)gEffect06ColorKeys_14, (u32)gEffect06ColorKeys_15,
    (u32)gEffect06ColorKeys_16, (u32)gEffect06ColorKeys_17, (u32)gEffect06ColorKeys_18, (u32)gEffect06ColorKeys_19,
    (u32)gEffect06ColorKeys_20, (u32)gEffect06ColorKeys_21, (u32)gEffect06ColorKeys_22, (u32)gEffect06ColorKeys_23,
    (u32)gEffect06ColorKeys_24, (u32)gEffect06ColorKeys_25, (u32)gEffect06ColorKeys_26, (u32)gEffect06ColorKeys_27,
    (u32)gEffect06ColorKeys_28, (u32)gEffect06ColorKeys_29, (u32)gEffect06ColorKeys_30, (u32)gEffect06ColorKeys_31,
    (u32)gEffect06ColorKeys_32, (u32)gEffect06ColorKeys_33, (u32)gEffect06ColorKeys_34, (u32)gEffect06ColorKeys_35,
    (u32)gEffect06ColorKeys_36, (u32)gEffect06ColorKeys_37, (u32)gEffect06ColorKeys_38, (u32)gEffect06ColorKeys_39,
    (u32)gEffect06ColorKeys_40, (u32)gEffect06ColorKeys_41, (u32)gEffect06ColorKeys_40, (u32)gEffect06ColorKeys_41,
    (u32)gEffect06ColorKeys_40, (u32)gEffect06ColorKeys_41, (u32)gEffect06ColorKeys_46, (u32)gEffect06ColorKeys_47,
    (u32)gEffect06ColorKeys_48, (u32)gEffect06ColorKeys_49, (u32)gEffect06ColorKeys_50, (u32)gEffect06ColorKeys_51,
    (u32)gEffect06ColorKeys_52, (u32)gEffect06ColorKeys_53,
};
s32 *gEff06IconColorKeys[2] = {
    (s32 *)D_00466540, (s32 *)D_00466560,
};
s32 *gEff06IconColorKeys3[6] = {
    (s32 *)D_00466580, (s32 *)D_004665D0, (s32 *)gEffect06ColorKeys_47, (s32 *)gEffect06ColorKeys_48,
    (s32 *)D_004665F0, NULL,
};
u32 gEff06PartDurTbls_0[2] = {
#include "sub_main/00466708.inc"
};
s16 gEff06PartAges3[8] = {
#include "sub_main/00466710.inc"
};
u32 gEff06PartDurTbls_3[4] = {
#include "sub_main/00466720.inc"
};
s16 gEff06PartAges4[4] = {
#include "sub_main/00466730.inc"
};
u32 gEff06PartDurTbls_4[2] = {
#include "sub_main/00466738.inc"
};
s16 gEff06PartCounts[16] = {
#include "sub_main/00466740.inc"
};
u32 gEff06Times[8] = {
#include "sub_main/00466760.inc"
};
u32 gEff06PartDurTbls[12] = {
    (u32)gEff06PartDurTbls_0, 0, 0, (u32)gEff06PartDurTbls_3,
    (u32)gEff06PartDurTbls_4, 0, 0, 0,
    (u32)gEff06PartDurTbls_4, 0, 0, 0,
};
u32 gEff06PosKeyStart[8] = {
#include "sub_main/004667B0.inc"
};
u32 gEff06PosKeyStep[8] = {
#include "sub_main/004667D0.inc"
};
u32 gEff06ColorKeyStart[8] = {
#include "sub_main/004667F0.inc"
};
f32 gEffect06EmScales[76] = {
#include "sub_main/00466810.inc"
};
