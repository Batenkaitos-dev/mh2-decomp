/*
 * The fishing code (fish, lure, countdown) with the helpers only it uses (Effect08_Spawn,
 * Fx_HeadingTo, Fish_Run/3EE0 which call Fish_Update). Level 3: Fish_BaitMatches_Unk breaks at
 * level 4, while Tri_SegmentHit (unk_3E0EC0.c) matches only at 4. Where this unit starts
 * (after 0x3E13E0, by 0x3E3FB0) is not known; 0x3E3D00 follows the calls.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "sprite.h"
#include "prim.h"
#include "fxobj.h"

u16 Rand_Next(s32 a);
void Text_SetSize(s32 size);
struct StageObj *StageObj_AllocTail(s32 n);
void StageObj_Free(struct StageObj *p);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void DrawSlot_Free(s32 id);
void * Stage_FindData(u16 id);
void Fish_Start(FxObj *o);
struct FxObj;
struct DrawSlot;
void Fish_Update(struct FxObj *o);
void Fish_End(FxObj *o);
void Fish_Free(FxObj *o);
void Lure_Start(struct FxObj *o);
void Lure_Update(struct FxObj *o);
void Lure_End(FxObj *o);
void Lure_Free(FxObj *o);
void CountdownFx_Start(FxObj *o);
void CountdownFx_Run(FxObj *o);
void CountdownFx_End(FxObj *o);
void CountdownFx_Free(FxObj *o);
void Countdown_Draw(s16 t);
void Letterbox_Draw(s8);
s32 Gs_SetState(u8 reg, u32 v);
s16 Effect_MapTexId(s16 id);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 f);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void Vec_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Lure_Draw(struct DrawSlot *slot);
void Vec_RotateY(f32 *v, f32 angle);
void Vib_StartLocal(void *player, s32 i);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
s32 Prim_FillTri(PrimTri *t);
void Vec_Normalize(f32 *v);
void Mtx_Identity(f32 *m);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void SparkFx_Update(SparkEmitter *e);

SparkEmitter *Effect08_Alloc(s32 unused)
{
    return (SparkEmitter *)StageObj_AllocTail(0x1);
}

void Vec_Copy(f32 *dst, f32 *src);

/* Starts an effect object of kind 8 (update SparkFx_Update) at pos, with +3 = k, +7 = b and the scale. */
void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale)
{
    SparkEmitter *e = Effect08_Alloc(k);

    if (e != NULL) {
        e->kind = 8;
        e->update = SparkFx_Update;
        e->owner = NULL;
        e->type = k;
        e->tint = b;
        e->scale = scale;
        Vec_Copy(e->pos, pos);
    }
}

s32 Obj_InCurrentArea(void *em);

/* Effect08_Spawn at monster em's position (height y), when em is in the current area; +0x34 = em. */
void Effect08_SpawnAtEm(Em *em, s32 k, s32 b, f32 scale, f32 y)
{
    SparkEmitter *e;

    if ((u8)Obj_InCurrentArea(em)) {
        if ((e = Effect08_Alloc(k)) != NULL) {
            e->kind = 8;
            e->update = SparkFx_Update;
            e->owner = em;
            e->type = k;
            e->tint = b;
            e->scale = scale;
            Vec_Copy(e->pos, &em->posX);
            e->pos[1] = y;
        }
    }
}

void Fish_Run(FxObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        Fish_Start(o);
        break;
    case 1:
        Fish_Update(o);
        break;
    case 2:
        Fish_End(o);
        break;
    case 3:
        Fish_Free(o);
        break;
    }
}

void Fish_Draw(DrawSlot *slot);

/* Next state: takes a draw slot (+0x3C, +0x38) that draws with Fish_Draw, then Fish_Update. */
void Fish_Start(FxObj *o)
{
    o->state++;
    o->step = 0;
    o->visible = 1;
    o->unk14 = 0;
    o->slotId = DrawSlot_Alloc();
    if (o->slotId != -1) {
        o->slot = DrawSlot_Get(o->slotId);
        o->slot->owner = (void *)o;
        o->slot->draw = Fish_Draw;
    } else {
        o->slot = NULL;
    }
    Fish_Update(o);
}

/* The heading from a to b, a quarter turn back (the effects' angle convention). */
u16 Fx_HeadingTo(f32 *a, f32 *b)
{
    return Math_Heading(a[0], a[2], b[0], b[2]) - 0x4000;
}

s32 Fish_BaitMatches_Unk(Player *p, s8 k)
{
    switch (p->useItem) {
    case 0x8E:
    case 0x8F:
    case 0x90:
    case 0x95:
        break;
    case 0x91:
        return 0;
    case 0x92:
        if (k == 3 || k == 10 || k == 11 || k == 20 || k == 23) {
            break;
        }
        return 0;
    case 0x93:
        if (k == 7 || k == 8 || k == 22) {
            break;
        }
        return 0;
    case 0x94:
        if (k == 5) {
            break;
        }
        return 0;
    default:
        return 1;
    }
    return 1;
}

extern FxObj gStageObjs[];
s32 Player_IsLocal(void *obj);
f32 Vec_DistanceXZ(f32 *a, f32 *b);

/* A local player's lure (kind 0x16, in the water, not taken) that the fish may bite: within its
 * radius of home, allowed for its kind (Fish_BaitMatches_Unk) and in front (+-0x3000 of its heading).
 * Mode 0 aims the fish at it; otherwise the fish takes it (lure +7) once within 30. NULL when none. */
FxObj *Fish_FindLure(FxObj *o, FishWork *w, s32 mode)
{
    s32 i;
    FxObj *lure;
    u16 h;

    for (i = 0, lure = gStageObjs; i < 128; i++, lure++) {
        if (lure->kind == 0x16 && lure->type != 1 && lure->step == 2 && lure->claimed == 0
            && Player_IsLocal(lure->owner) == 1 && Fish_BaitMatches_Unk(lure->owner, w->kind)
            && Vec_DistanceXZ(w->home, lure->pos) < w->radius) {
            h = Fx_HeadingTo(o->pos, lure->pos);
            if ((u16)(h - w->angles[1] + 0x3000) <= 0x5FFF) {
                if (mode == 0) {
                    w->heading = h;
                    return lure;
                }
                if ((w->dist = Vec_DistanceXZ(o->pos, lure->pos)) < 30.0f) {
                    lure->claimed = 1;
                    return lure;
                }
            }
        }
    }
    return NULL;
}

extern FishKind gFishKindsOffline[];
extern FishKind gFishKindsOnline[];

/* The record of fish kind i: gFishKindsOffline's, or gFishKindsOnline's when gScene.online is 1. */
static FishKind *Fish_GetKind(s8 i)
{
    if (gScene.online != 1) {
        return &gFishKindsOffline[i];
    }
    return &gFishKindsOnline[i];
}

s8 Fish_PickKind(FxObj *o);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/* Fish step 0: picks the fish's kind (Fish_PickKind; none: the object ends), places it at a
 * random point within half its radius of home, then speeds up to its kind's velocity
 * (Fish_GetKind +8) over 10 frames and goes to step 1. */
void Fish_StepSpawn(FxObj *o, FishWork *w)
{
    FishKind *rec = Fish_GetKind(w->kind);
    f32 v[3];
    f32 d[3];
    s32 a[3];
    f32 m[16];
    s32 r;

    switch (o->phase) {
    case 0:
        o->phase++;
        w->unk49 = 0;
        w->kind = Fish_PickKind(o);
        if (w->kind == 0) {
            o->state++;
            o->step = 0;
            o->visible = 0;
        }
        Vec_Copy(o->pos, w->home);
        a[0] = 0;
        a[1] = Rand_Next(1);
        a[2] = 0;
        Mtx_FromAnglesXYZ(a, m);
        r = (s32)w->radius / 2;
        v[2] = Rand_Next(1) % r;
        v[0] = 0.0f;
        v[1] = 0.0f;
        Vec_Rotate(d, v, m);
        o->pos[0] += d[0];
        o->pos[1] += d[1];
        o->pos[2] += d[2];
        o->timer = 10;
        w->angles[2] = 0;
        w->angles[0] = 0;
        w->heading = w->angles[1] = (Rand_Next(1) & 0xF) << 12;
        w->turn = 0x20;
        w->unk38 = 0;
        w->scale[0] = w->scale[1] = w->scale[2] = 0.1f;
        w->angles[0] = 0;
        break;
    case 1:
        w->scale[0] += rec->scale[0] / 10.0f;
        w->scale[1] += rec->scale[1] / 10.0f;
        w->scale[2] += rec->scale[2] / 10.0f;
        if (--o->timer <= 0.0f) {
            o->step = 1;
            o->phase = 0;
            o->timer = (Rand_Next(1) & 0x1F) + 10;
            w->scale[0] = rec->scale[0];
            w->scale[1] = rec->scale[1];
            w->scale[2] = rec->scale[2];
        }
        break;
    }
}

f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/* Fish step 1: turns toward a heading (a lure's, or a random one back toward home), then
 * swims forward at a slowing speed; on a lure with Player +0x842 set, step 2. */
void Fish_StepSwim(FxObj *o, FishWork *w)
{
    f32 v[3];
    f32 d[3];
    f32 m[16];
    s32 h;

    switch (o->phase) {
    case 0:
        if (--o->timer <= 0 || w->angles[1] - w->heading == 0) {
            o->phase++;
            o->scale = 3.0f;
            w->unk38 = 0x100;
            if (Fish_FindLure(o, w, 0) == NULL) {
                h = Fx_HeadingTo(o->pos, w->home);
                w->heading = h - 0x4000 + ((Rand_Next(1) & 0xF) << 11);
            }
        }
        w->turn += (0x20 - w->turn) / 8;
        Angle_TurnToward(&w->angles[1], &w->heading, w->turn);
        break;
    case 1:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = o->scale;
        Mtx_FromAnglesXYZ(w->angles, m);
        Vec_Rotate(d, v, m);
        o->pos[0] += d[0];
        o->pos[1] += d[1];
        o->pos[2] += d[2];
        w->unk38 = -w->unk38;
        w->unk38 -= w->unk38 / 2;
        if ((w->lure = Fish_FindLure(o, w, 1)) != NULL && w->lure->owner->baited != 0) {
            o->step = 2;
            o->phase = 0;
            o->timer = 3;
            w->unk38 = 0;
        } else if ((o->scale += -0.1f) <= 0.0f) {
            o->phase = 0;
            o->timer = (Rand_Next(1) & 0x1F) + 10;
            w->unk38 = 0;
        }
        w->turn += (0x100 - w->turn) / 8;
        Angle_TurnToward(&w->angles[1], &w->heading, w->turn);
        break;
    }
}

s16 Player_IsState(void *obj, u8 a, u8 b);
s32 Player_HasSkill(Player *p, u8 skill);
f32 Lure_GetWaterHeight(FxObj *lure);
extern u16 gFishBiteJitter[];

/* Fish step 2: at the lure. It nibbles (up to its kind's count, none with bait 0x95), darting
 * forward and back, then bites: Player.fishTimer gets the kind's value (doubled with skill 0xA0) and
 * a vibration; a nibble splashes (effect 7) and vibrates. Step 1 when the lure is gone or the player
 * is in state 0x64 or 0x54 (0x65 offline); step 3 when he strikes (state 0x53, 0x63 offline) with
 * no catch pending (+0x840, set to the kind's item). */
void Fish_StepBite(FxObj *o, FishWork *w)
{
    FxObj *lure = w->lure;
    FishKind *k = Fish_GetKind(w->kind);
    f32 v[3];
    f32 d[3];
    f32 m[16];
    f32 *vy;
    f32 *vz;

    if (gScene.online != 1) {
        if (lure->kind != 0x16 || lure->active == 0 || lure->state >= 2
            || Player_IsState(lure->owner, 0, 0x64) || Player_IsState(lure->owner, 0, 0x54)) {
            o->step = 1;
            o->phase = 0;
            o->timer = (Rand_Next(1) & 0x1F) + 10;
            return;
        }
        if (lure->owner->gotItemId == 0 && Player_IsState(lure->owner, 0, 0x53)) {
            o->step = 3;
            o->phase = 0;
            lure->owner->gotItemId = k->item;
            return;
        }
    } else {
        if (lure->kind != 0x16 || lure->active == 0 || lure->state >= 2
            || Player_IsState(lure->owner, 0, 0x64) || Player_IsState(lure->owner, 0, 0x65)) {
            o->step = 1;
            o->phase = 0;
            o->timer = (Rand_Next(1) & 0x1F) + 10;
            return;
        }
        if (lure->owner->gotItemId == 0 && Player_IsState(lure->owner, 0, 0x63)) {
            o->step = 3;
            o->phase = 0;
            lure->owner->gotItemId = k->item;
            return;
        }
    }
    switch (o->phase) {
    case 0:
        o->phase++;
        if (lure->owner->useItem == 0x95) {
            w->nibbles = 0;
        } else {
            w->nibbles = Rand_Next(1) % k->nibbles + 1;
        }
    case 1:
        if (--w->nibbles <= 0) {
            o->phase = 5;
        } else {
            o->phase++;
        }
        o->timer = 5;
        o->scale = Vec_DistanceXZ(o->pos, lure->pos) / o->timer;
        w->heading = w->angles[1] = Fx_HeadingTo(o->pos, w->lure->pos);
        break;
    case 2:
    case 5:
        v[0] = 0.0f;
        vy = &v[1];
        *vy = 0.0f;
        vz = &v[2];
        *vz = o->scale;
        Mtx_FromAnglesXYZ(w->angles, m);
        Vec_Rotate(d, v, m);
        o->pos[0] += d[0];
        o->pos[1] += d[1];
        o->pos[2] += d[2];
        if (--o->timer <= 0) {
            if (o->phase == 5) {
                lure->owner->fishTimer = k->unk2 + gFishBiteJitter[Rand_Next(1) & 3];
                if (Player_HasSkill(lure->owner, 0xA0) == 1) {
                    lure->owner->fishTimer *= 2;
                }
                if (!(Rand_Next(1) & 3)) {
                    lure->owner->baited = 0;
                }
                Vib_StartLocal(lure->owner, 1);
            } else {
                v[0] = lure->pos[0];
                *vy = 10.0f + Lure_GetWaterHeight(lure);
                *vz = lure->pos[2];
                Effect08_Spawn(v, 7, 0, 0.5f);
                Vib_StartLocal(lure->owner, 4);
            }
            o->phase++;
            o->scale = -5.0f;
            w->accel = 0.5f;
        }
        break;
    case 3:
    case 6:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = o->scale;
        Mtx_FromAnglesXYZ(w->angles, m);
        Vec_Rotate(d, v, m);
        o->pos[0] += d[0];
        o->pos[1] += d[1];
        o->pos[2] += d[2];
        if ((o->scale += w->accel) >= 0.0f) {
            o->phase++;
            o->timer = 20;
        }
        break;
    case 4:
        if (--o->timer <= 0) {
            o->phase = 1;
        }
        break;
    case 7:
        if (--o->timer <= 0) {
            o->phase++;
            o->timer = (Rand_Next(1) & 0x1F) + 60;
            w->heading = w->angles[1] + ((Rand_Next(1) & 7) << 12);
            lure->claimed = 0;
            o->scale = 8.0f;
            w->accel = -0.3f;
        }
        break;
    case 8:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = o->scale;
        Mtx_FromAnglesXYZ(w->angles, m);
        Vec_Rotate(d, v, m);
        o->pos[0] += d[0];
        o->pos[1] += d[1];
        o->pos[2] += d[2];
        if ((o->scale += w->accel) <= 0.0f) {
            o->step = 1;
            o->phase = 0;
            o->timer = (Rand_Next(1) & 0x1F) + 10;
        }
        break;
    }
    Angle_TurnToward(&w->angles[1], &w->heading, 0x100);
}

s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
void Vib_StartLocal(void *player, s32 i);

/* Fish step 3: hooked, it follows the lure (turning toward it, a vibration every 10 frames)
 * until the lure's step 4; step 4 when the lure is gone. */
void Fish_StepHooked(FxObj *o, FishWork *w)
{
    FxObj *lure = w->lure;

    if (lure->kind != 0x16 || lure->active == 0 || lure->state >= 2) {
        o->step = 4;
        o->phase = 0;
        return;
    }
    switch (o->phase) {
    case 0:
        o->phase++;
        o->timer = 10;
    case 1:
        w->heading = Fx_HeadingTo(o->pos, lure->pos);
        Angle_TurnToward(&w->angles[1], &w->heading, 0x800);
        o->pos[0] = lure->pos[0];
        o->pos[2] = lure->pos[2];
        if (--o->timer <= 0) {
            o->timer = 10;
            Vib_StartLocal(lure->owner, 1);
        }
        if (lure->step > 3) {
            o->phase++;
            w->unk49 = 1;
            Vib_StartLocal(lure->owner, 3);
        }
        break;
    case 2:
        w->angles[0] = -0x4000;
        Vec_Copy(o->pos, lure->pos);
        break;
    }
}

extern s8 gFishWaitSecs[];

extern s8 gFishWaitSecs[];

/* Fish step 4: waits 30 to 30 x gFishWaitSecs[] frames at home (velocity zero), then starts again at step 0. */
void Fish_StepWait(FxObj *o, FishWork *w)
{
    switch (o->phase) {
    case 0:
        o->phase++;
        w->unk49 = 0;
        o->timer = gFishWaitSecs[Rand_Next(1) & 3] * 30;
        w->scale[0] = 0.0f;
        w->scale[1] = 0.0f;
        w->scale[2] = 0.0f;
        Vec_Copy(o->pos, w->home);
        break;
    case 1:
        if (--o->timer <= 0.0f) {
            o->step = 0;
            o->phase = 0;
        }
        break;
    }
}

void Fish_StepSpawn(FxObj *o, FishWork *w);
void Fish_StepSwim(FxObj *o, FishWork *w);
void Fish_StepBite(FxObj *o, FishWork *w);
void Fish_StepHooked(FxObj *o, FishWork *w);
void Fish_StepWait(FxObj *o, FishWork *w);
void Vec_Sub(f32 *out, f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
void Vec_Normalize(f32 *v);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

/* A fish's update: its step, kept within radius of home (horizontally), then drawn. */
void Fish_Update(FxObj *o)
{
    u8 step = o->step;
    FishWork *w = o->work;
    f32 d[3];
    f32 *dy;

    switch (step) {
    case 0:
        Fish_StepSpawn(o, w);
        break;
    case 1:
        Fish_StepSwim(o, w);
        break;
    case 2:
        Fish_StepBite(o, w);
        break;
    case 3:
        Fish_StepHooked(o, w);
        break;
    case 4:
        Fish_StepWait(o, w);
        break;
    }
    if (o->visible) {
        if (o->step <= 2) {
            Vec_Sub(d, o->pos, w->home);
            dy = &d[1];
            *dy = 0.0f;
            if (Vec_Length(d) > w->radius) {
                Vec_Normalize(d);
                o->pos[0] = w->home[0] + d[0] * w->radius;
                o->pos[1] = w->home[1] + *dy * w->radius;
                o->pos[2] = w->home[2] + d[2] * w->radius;
            }
        }
        o->slot->pos[0] = o->pos[0];
        o->slot->pos[1] = o->pos[1];
        o->slot->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOt32, o->slot, 0x20, 0);
    }
}

void Fish_End(FxObj *o)
{
    o->state++;
    DrawSlot_Free(o->slotId);
}

void Fish_Free(FxObj *o)
{
    StageObj_Free((struct StageObj *)o);
}

extern u8 gFishColors[][3];
void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z);

/* Draws a fish (the draw slot's update): its kind's model scaled, turned and placed, in its
 * kind's color (gFishColors), at half brightness unless +0x49 is set. */
void Fish_Draw(DrawSlot *slot)
{
    FxObj *o = (FxObj *)slot->owner;
    FishWork *w = o->work;
    FishKind *k = Fish_GetKind(w->kind);
    Model *set = gCommonModels[0];
    f32 m[16];
    ModelMat *base;
    ModelMatSet *e;
    f32 s;
    u32 color;

    if (set == NULL || set->active == 0) {
        return;
    }
    Gs_SetState(0x60, 0x80);
    e = &set->matSets[k->model];
    base = set->mats;
    Mtx_Scale(m, w->scale[0], w->scale[1], w->scale[2]);
    Mtx_RotateZXY(m, 6.2831855f * w->angles[0] / 65536.0f, 6.2831855f * (w->angles[1] + w->unk38) / 65536.0f,
                  6.2831855f * w->angles[2] / 65536.0f);
    Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
    Gs_SetState(0x67, -1);
    if (e != NULL && e->id != -1) {
        Gs_SetState(0x60, 0x80);
        Gs_SetState(0x1A, (u32)m);
        if (w->unk49 == 0) {
            s = 0.5f;
        } else {
            s = 1.0f;
        }
        color = 0xFF000000 | ((u8)(s * gFishColors[k->color][0]) << 16) | ((u8)(s * gFishColors[k->color][1]) << 8) | (u8)(s * gFishColors[k->color][2]);
        Gs_SetState(0x67, color);
        Gs_SetLightList(base, e);
        Draw_ApplyStates(e->flags);
        Model_Draw(e->id, 0);
        Draw_SetDefaultState();
    }
}

/* A random fish kind from the {weight, kind} list of the fish's work (weight 0xFF ends it). */
s8 Fish_PickKind(FxObj *o)
{
    FishWork *w = o->work;
    u8 *p;
    u8 total;
    u8 r;
    u8 sum;

    total = 0;
    for (p = w->table; *p != 0xFF; p += 2) {
        total += *p;
    }
    sum = Rand_Next(0) % total;
    r = sum;
    for (sum = 0, p = w->table; *p != 0xFF; p += 2) {
        sum += *p;
        if (r < sum) {
            return p[1];
        }
    }
    return 0;
}

void Fish_Run(FxObj *o);
struct StageObj *StageObj_Alloc(s32 n);

/* Starts an effect object of kind 0x17 (update Fish_Run) from record src: x, y, z to it and its work, the fourth value to the work +0x18. */
void Effect17_Spawn(s32 k, f32 *src, u8 *table)
{
    FxObj *e = (FxObj *)StageObj_Alloc(1);

    if (e != NULL) {
        FishWork *w = e->work;

        e->kind = 0x17;
        e->update = Fish_Run;
        e->unk14 = 0;
        e->type = k;
        w->home[0] = e->pos[0] = *src++;
        w->home[1] = e->pos[1] = *src++;
        w->home[2] = e->pos[2] = *src++;
        w->radius = *src;
        w->table = table;
    }
}

void *Stage_GetSeasonalData(s32 i, s32 *out);
void Effect17_Spawn(s32 k, f32 *src, u8 *table);

/* For each record (0x18 bytes, +0x10 a table, negative ends) of the area's list: Effect17_Spawn once per entry of the table. */
/* A fishing spot of the stage data (Stage_FindData(area); 0x18 bytes, a negative kind ends the
 * list): Effect17_Spawn starts Stage_GetSeasonalData(kind)'s count of fish there. */
typedef struct FishSpot {
    f32 circle[4];              /* 0x00: the fish's home x, y, z and its radius (Effect17_Spawn) */
    s32 kind;                   /* 0x10: the entry of Stage_GetSeasonalData: the fish table and count */
    s32 unk14;
} FishSpot;
typedef char FishSpot_size_check[(sizeof(FishSpot) == 0x18) ? 1 : -1];

void Effect17_SpawnArea(s32 area)
{
    FishSpot *rec = Stage_FindData(area);
    u8 *table;
    s32 n;

    if (rec != NULL) {
        for (;;) {
            if (rec->kind < 0) {
                break;
            }
            table = Stage_GetSeasonalData(rec->kind, &n);
            while (n > 0) {
                Effect17_Spawn(rec->kind, rec->circle, table);
                n--;
            }
            rec++;
        }
    }
}

f32 Stage_GetDataPoint(s32 area, f32 *pos)
{
    f32 *data;

    data = Stage_FindData(area & 0xFFFF);
    if (data == NULL) {
        return -1.0f;
    }
    pos[0] = data[0];
    pos[1] = data[1];
    pos[2] = data[2];
    return data[3];
}

struct StageObj *StageObj_Alloc(s32 n);
void Lure_Run(FxObj *o);

void Lure_Spawn(Player *pl, u8 k)
{
    FxObj *e = (FxObj *)StageObj_Alloc(1);

    if (e != NULL) {
        e->kind = 0x16;
        e->update = Lure_Run;
        e->unk14 = 0;
        e->owner = pl;
        e->ownerIndex = (u8)pl->index & 3;
        e->type = k;
    }
}

void Lure_Run(FxObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        Lure_Start(o);
        break;
    case 1:
        Lure_Update(o);
        break;
    case 2:
        Lure_End(o);
        break;
    case 3:
        Lure_Free(o);
        break;
    }
}

u16 Area_DayId(u16 a);
f32 Rand_Unit(void);
f32 Rand_Signed(void);

/* The lure's first state: at the rod (bone 0xE of the player), a random throw velocity turned to
 * the player's heading, its scale (0.3 for variant 1), the water height of the area (+0x1C), and a
 * draw slot drawn by Lure_Draw (none: the lure ends). */
void Lure_Start(FxObj *o)
{
    Player *p = o->owner;
    LureWork *w = o->work;

    o->state++;
    o->step = 0;
    o->phase = 0;
    o->visible = 1;
    o->unk14 = 0;
    Em_GetBonePos(p, 0xE, o->pos);
    Vec_Copy(w->target, o->pos);
    o->angle = p->rot[1];
    w->unk02 = 0;
    w->vel[0] = Rand_Signed();
    w->vel[1] = 2.0f + 2.0f * Rand_Unit();
    w->vel[2] = 30.0f + 2.0f * Rand_Unit();
    w->gravity = -2.0f;
    switch (o->type) {
    default:
        w->scale = 1.0f;
        break;
    case 1:
        w->scale = 0.3f;
        break;
    }
    switch (Area_DayId(gScene.area)) {
    case 1:
        w->unk1C = -94.0f;
        break;
    case 3:
        w->unk1C = -131.0f;
        break;
    case 0x15:
        w->unk1C = -10.0f;
        break;
    case 0x2A:
        w->unk1C = -45.0f;
        break;
    case 0x30:
        w->unk1C = -20.0f;
        break;
    case 0x3B:
        w->unk1C = -222.0f;
        break;
    case 0x3D:
        w->unk1C = -41.0f;
        break;
    case 0x43:
        w->unk1C = -20.0f;
        break;
    case 0x4A:
        w->unk1C = -10.0f;
        break;
    case 0x4B:
        w->unk1C = -20.0f;
        break;
    case 0x63:
        w->unk1C = -50.0f;
        break;
    case 0x6E:
        w->unk1C = -57.0f;
        break;
    case 0x71:
        w->unk1C = -12.0f;
        break;
    case 0x72:
        w->unk1C = -64.0f;
        break;
    case 0x74:
        w->unk1C = -37.0f;
        break;
    case 0x79:
        w->unk1C = -447.0f;
        break;
    case 0x82:
        w->unk1C = -270.0f;
        break;
    case 0x84:
        w->unk1C = 810.0f;
        break;
    case 0x89:
        w->unk1C = -595.0f;
        break;
    case 0x8E:
        w->unk1C = 30.0f;
        break;
    case 0x90:
        w->unk1C = 0.0f;
        break;
    case 0x92:
        w->unk1C = 5.0f;
        break;
    case 0x97:
        w->unk1C = -47.5f;
        break;
    case 0x9A:
        w->unk1C = -10.0f;
        break;
    case 0xB3:
        w->unk1C = 810.0f;
        break;
    default:
        w->unk1C = 0.0f;
        break;
    }
    w->line = 0;
    Vec_RotateY(w->vel, 6.2831855f * o->angle / 65536.0f);
    o->slotId = DrawSlot_Alloc();
    if (o->slotId != -1) {
        o->slot = DrawSlot_Get(o->slotId);
        o->slot->owner = (void *)o;
        o->slot->draw = Lure_Draw;
    } else {
        StageObj_Free((struct StageObj *) o);
    }
}

/* Moves an object (position at +0x24) by a velocity (b + 4) and adds the vertical acceleration (b + 0x20) to it. */
void Obj_ApplyVelocity(f32 *a, f32 *b)
{
    a[9] += b[1];
    a[10] += b[2];
    a[11] += b[3];
    b[2] += b[8];
}

void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale);

/* Effects of kind 8 above pos (at height y + 15): variant 7, or 3 and 0 (k 1), or 4 and 1 (k 2) 5 higher. */
void Fx_SpawnSplash(f32 *pos, f32 y, f32 scale, u8 k)
{
    f32 p[3];
    f32 *py = &p[1];

    p[0] = pos[0];
    *py = 15.0f + y;
    p[2] = pos[2];
    switch (k) {
    default:
        Effect08_Spawn(p, 7, 0, scale);
        break;
    case 1:
        Effect08_Spawn(p, 3, 0, scale);
        *py += 5.0f;
        Effect08_Spawn(p, 0, 0, scale);
        break;
    case 2:
        Effect08_Spawn(p, 4, 0, scale);
        *py += 5.0f;
        Effect08_Spawn(p, 1, 0, scale);
        break;
    }
}

void Vec_RotateY(f32 *v, f32 angle);
void Vec_Scale(f32 *out, f32 *v, f32 s);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Sub(f32 *out, f32 *a, f32 *b);
f32 Vec_Distance(f32 *a, f32 *b);
void Lure_Cast(FxObj *o, LureWork *w);
void Fishing_PlaySe(FxObj *o, f32 *pos, s16 k);
u32 Player_TestFlag(void *obj, u32 flag);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
void Fish_StepState_Unk(FxObj *o, LureWork *w);

/* The lure while it is out: flying (step 0), waiting for the cast motion (1), cast (2), in the
 * water (3), reeled in (4) and pulled up (5); then kept within 100 of its target, drawn, and
 * ended when the player leaves the area or drops the rod. */
void Lure_Update(FxObj *o)
{
    LureWork *w;
    Player *p;
    f32 v[3];
    f32 s;
    f32 d;
    s16 n;

    w = o->work;
    p = o->owner;
    o->timer++;
    switch (o->step) {
    case 0:
        Obj_ApplyVelocity((f32 *)o, (f32 *)w);
        if (o->pos[1] < w->unk1C - 5.0f) {
            o->pos[1] = w->unk1C - 5.0f;
            Vec_Copy(w->target, o->pos);
            Fx_SpawnSplash(w->target, w->unk1C, 1.0f, 0);
            w->vel[0] = 0.0f;
            w->vel[1] = 0.0f;
            w->vel[2] = 0.0f;
            w->gravity = 0.0f;
            o->step++;
            w->unk02 = 0;
        }
        break;
    case 1:
        if (Player_TestFlag(p, 0x80000) == 0) {
            o->state++;
            return;
        }
        if (p->motionId[0] == 0x326) {
            o->step++;
        }
        break;
    case 2:
        if (Player_TestFlag(p, 0x80000) == 0) {
            o->state++;
            return;
        }
        if (w->unk02 < p->fishTimer) {
            Fx_SpawnSplash(o->pos, w->unk1C, 1.0f, 0);
            Fishing_PlaySe(o, o->pos, 0);
            w->vel[1] -= 20.0f + p->fishTimer;
            o->ownerIndex |= 0x80;
        }
        w->unk02 = p->fishTimer;
        if (w->unk02 <= 0) {
            o->ownerIndex &= 0x7F;
        }
        if (o->pos[1] > w->target[1]) {
            o->pos[1] = w->target[1];
        }
        if (o->pos[1] < w->target[1]) {
            w->gravity = 1.0f;
        } else {
            w->vel[1] *= 0.2f;
            w->gravity = 2.0f;
        }
        if ((o->ownerIndex & 0x80) && w->vel[1] > 0.0f) {
            w->vel[1] = 0.0f;
        }
        switch (p->motionId[0]) {
        case 0x327:
            o->ownerIndex &= 0x7F;
            o->step++;
            w->vel[0] = o->pos[0] - p->pos[0];
            w->vel[2] = o->pos[2] - p->pos[2];
            Vec_Normalize(w->vel);
            switch (p->gotItemId) {
            case 0xEE:
            case 0xEF:
            case 0xEC:
                Vec_Scale(w->vel, w->vel, 1.0f);
                *(s16 *)w->pad28 = 0;
                break;
            default:
                Vec_Scale(w->vel, w->vel, 3.0f);
                *(s16 *)w->pad28 = 1;
                break;
            }
            Vec_RotateY(w->vel, (6.2831855f * (f32)((Rand_Next(1) & 0x3F) - 0x20)) / 65536.0f);
            break;
        case 0x328:
            o->ownerIndex &= 0x7F;
            w->mode = 0;
            Lure_Cast(o, w);
            break;
        case 0x329:
            if (Motion_ReachedFrame(p, 0, 26.0f)) {
                o->ownerIndex &= 0x7F;
                w->mode = 1;
                Lure_Cast(o, w);
            }
            break;
        case 0x32A:
            o->ownerIndex &= 0x7F;
            w->mode = 2;
            Lure_Cast(o, w);
            break;
        }
        Obj_ApplyVelocity((f32 *)o, (f32 *)w);
        break;
    case 3:
        Vec_Copy(v, w->vel);
        if (*(s16 *)w->pad28 == 0) {
            Vec_Scale(v, v, 0.2f);
            Vec_RotateY(v, (6.2831855f * (f32)((Rand_Next(1) & 0x7FFF) - 0x4000)) / 65536.0f);
            s = 1.5f;
        } else {
            Vec_Scale(v, v, 0.5f);
            Vec_RotateY(v, (6.2831855f * (f32)((Rand_Next(1) & 0x3FFF) - 0x2000)) / 65536.0f);
            s = 3.0f;
        }
        Vec_Add(w->vel, w->vel, v);
        if (o->pos[1] > w->target[1]) {
            o->pos[1] = w->target[1];
        }
        if ((o->timer & 7) == 0) {
            Fx_SpawnSplash(o->pos, w->unk1C, s, 1);
            Fishing_PlaySe(o, o->pos, 0);
        }
        if (o->pos[1] < w->target[1]) {
            w->gravity = 1.0f;
        } else {
            w->vel[1] *= 0.2f;
            w->gravity = 2.0f;
        }
        switch (p->motionId[0]) {
        case 0x327:
            break;
        case 0x328:
            if (Motion_PassedFrame(p, 0, 16.0f)) {
                w->mode = 0;
                Lure_Cast(o, w);
                Fx_SpawnSplash(o->pos, w->unk1C, s, 2);
                Fishing_PlaySe(o, o->pos, 1);
            }
            break;
        default:
            o->state++;
            return;
        }
        Obj_ApplyVelocity((f32 *)o, (f32 *)w);
        break;
    case 4:
        if ((w->mode == 0 && p->motionId[0] == 0x328) || (w->mode == 1 && p->motionId[0] == 0x329) ||
            (w->mode == 2 && p->motionId[0] == 0x32A)) {
            w->unk02++;
            switch (w->mode) {
            case 0:
                s = 0.3f;
                n = 35;
                break;
            case 1:
                s = 0.5f;
                n = 27;
                break;
            default:
                s = 0.5f;
                n = 40;
                break;
            }
            switch (o->type) {
            default:
                if (w->scale > s) {
                    w->scale -= (1.0f - s) / n;
                    if (w->scale < s) {
                        w->scale = s;
                    }
                }
                break;
            case 1:
                s = 0.3f;
                break;
            }
            switch (w->mode) {
            case 0:
            case 1:
                if (w->unk02 >= n) {
                    Em_GetBonePos(p, 14, o->pos);
                    o->pos[1] -= 10.0f;
                    o->step++;
                    w->unk02 = 0;
                    w->vel[0] = 0.0f;
                    w->vel[1] = 0.0f;
                    w->vel[2] = 0.0f;
                    w->gravity = 0.0f;
                    w->scale = s;
                }
                break;
            default:
                if (w->unk02 < n) {
                    v[0] = w->target[0] - o->pos[0];
                    v[1] = w->target[1] - o->pos[1];
                    v[2] = w->target[2] - o->pos[2];
                    w->vel[0] = 0.09f * v[0];
                    w->vel[1] = 0.09f * v[1];
                    w->vel[2] = 0.09f * v[2];
                } else {
                    o->state++;
                    return;
                }
                break;
            }
        } else {
            o->state++;
            return;
        }
        Obj_ApplyVelocity((f32 *)o, (f32 *)w);
        break;
    case 5:
        if ((w->mode == 0 && p->motionId[0] == 0x328) || (w->mode == 1 && p->motionId[0] == 0x329)) {
            n = w->mode == 0 ? 50 : 20;
            if (++w->unk02 < n) {
                Em_GetBonePos(p, 14, o->pos);
                o->pos[1] -= 10.0f;
            } else {
                o->state++;
                return;
            }
        } else {
            o->state++;
            return;
        }
        break;
    }
    Fish_StepState_Unk(o, w);
    if (o->step == 3) {
        d = Vec_Distance(o->pos, w->target);
        if (d > 100.0f) {
            Vec_Sub(v, o->pos, w->target);
            Vec_Scale(v, v, 100.0f / d);
            Vec_Add(o->pos, w->target, v);
        }
    }
    o->slot->pos[0] = o->pos[0];
    o->slot->pos[1] = o->pos[1];
    o->slot->pos[2] = o->pos[2];
    if ((u8)Obj_InCurrentArea(p) == 0) {
        o->state++;
        return;
    }
    OT_AddByDepth(gDrawOt32, o->slot, 0x20, 0);
}

void Lure_End(FxObj *o)
{
    o->state++;
    DrawSlot_Free(o->slotId);
}

void Lure_Free(FxObj *o)
{
    StageObj_Free((struct StageObj *)o);
}

extern u32 gLureColors[];
extern u32 gLureColorDefault;
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Player_GetRodTip(f32 *out, Player *p);

/* Draws a lure (the draw slot's update): its model (entry 24 of model set 0 for variant 1, else
 * the one Effect_MapTexId(0x71) picks), turned and placed at the slot; while the line is out
 * (+0x2A), the line model from the rod tip (Player_GetRodTip) to the lure. */
void Lure_Draw(DrawSlot *slot)
{
    FxObj *o = (FxObj *)slot->owner;
    LureWork *w = o->work;
    Model *set = gCommonModels[0];
    f32 t[3];
    f32 d[3];
    f32 m[16];
    f32 uv[16];
    f32 rx;
    f32 rz;
    ModelMat *base;
    ModelMatSet *e;
    ModelMatSet *line;
    f32 *mp;
    f32 *dy;

    if (set != NULL && set->active) {
        base = set->mats;
        Mtx_Scale(m, w->scale, w->scale, w->scale);
        switch (o->type) {
        case 1:
            e = &set->matSets[24];
            break;
        default:
            e = &set->matSets[Effect_MapTexId(0x71)];
            break;
        }
        Mtx_RotateY(m, 6.2831855f * o->angle / 65536.0f);
        Mtx_SetTrans(m, slot->pos[0], slot->pos[1], slot->pos[2]);
        if (o->ownerIndex & 0x80) {
            Gs_SetState(0x67, gLureColorDefault);
        } else {
            Gs_SetState(0x67, gLureColors[o->ownerIndex]);
        }
        Gs_SetState(0x1A, (u32)(mp = m));
        Gs_SetLightList(base, e);
        Draw_ApplyStates(e->flags);
        Model_Draw(e->id, 0);
        if (w->line) {
            line = &set->matSets[Effect_MapTexId(0x7F)];
            Player_GetRodTip(t, o->owner);
            Vec_Sub(d, slot->pos, t);
            dy = &d[1];
            rx = Math_Atan2(-d[2], Math_Sqrtf(d[0] * d[0] + *dy * *dy));
            rz = Math_Atan2(d[0], -*dy);
            Mtx_Scale(mp, 1.0f, Vec_Length(d), 1.0f);
            Mtx_RotateX(mp, rx);
            Mtx_RotateZ(mp, rz);
            Mtx_SetTrans(mp, t[0], t[1], t[2]);
            Gs_SetState(0x67, -1);
            Gs_SetState(0x1A, (u32)mp);
            Mtx_Translation(uv, 0.0f, 0.03125f * w->frame, 0.0f);
            Gs_SetState(0x19, (u32)uv);
            Gs_SetLightList(base, line);
            Draw_ApplyStates(line->flags);
            Model_Draw(line->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* The water height under a lure (its work +0x1C, set by area in Lure_Start). */
f32 Lure_GetWaterHeight(FxObj *lure)
{
    return ((LureWork *)lure->work)->unk1C;
}

static void Lure_SetLaunchVel(f32 *vel, f32 fparg0, f32 fparg1, f32 fparg2, f32 fparg3)
{
    vel[1] = (f32) (((fparg1 - fparg0) / fparg3) - (0.5f * fparg2 * fparg3));
}

void Vec_RotateY(f32 *v, f32 angle);
void Vec_Scale(f32 *out, f32 *v, f32 s);

/* Casts the lure: its target in front of the player (20, 150, 100 turned by his angle), and a
 * velocity reaching it in 35 frames (mode 0) or 27 (mode 1) under the gravity. */
void Lure_Cast(FxObj *o, LureWork *w)
{
    f32 v[3];
    f32 a[3];
    f32 b[3];
    Player *p = o->owner;
    f32 *vy = &v[1];
    f32 *vz = &v[2];
    f32 len;

    v[0] = 20.0f;
    *vy = 150.0f;
    *vz = 100.0f;
    Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
    w->target[0] = p->pos[0] + v[0];
    w->target[1] = p->pos[1] + *vy;
    w->target[2] = p->pos[2] + *vz;
    o->step = 4;
    switch (w->mode) {
    case 0:
        w->gravity = -2.0f;
        a[0] = o->pos[0];
        a[1] = 0.0f;
        a[2] = o->pos[2];
        b[0] = w->target[0];
        b[1] = 0.0f;
        b[2] = w->target[2];
        Vec_Sub(a, b, a);
        len = Vec_Length(a);
        Vec_Normalize(a);
        len /= 35.0f;
        Vec_Scale(w->vel, a, len);
        Lure_SetLaunchVel(w->vel, o->pos[1], w->target[1], w->gravity, 35.0f);
        break;
    case 1:
        w->gravity = -1.0f;
        a[0] = o->pos[0];
        a[1] = 0.0f;
        a[2] = o->pos[2];
        b[0] = w->target[0];
        b[1] = 0.0f;
        b[2] = w->target[2];
        Vec_Sub(a, b, a);
        len = Vec_Length(a);
        Vec_Normalize(a);
        len /= 27.0f;
        Vec_Scale(w->vel, a, len);
        Lure_SetLaunchVel(w->vel, o->pos[1], w->target[1], w->gravity, 27.0f);
        break;
    default:
        w->gravity = 0.0f;
        w->vel[0] = 0.0f;
        w->vel[1] = 0.0f;
        w->vel[2] = 0.0f;
        break;
    }
    w->unk02 = 0;
}

void Fish_StepState_Unk(FxObj *o, LureWork *w)
{
    switch (o->step) {
    case 3:
        if (w->line == 0) {
            if ((Rand_Next(1) & 7) == 0) {
                w->line = 1;
                w->frame = 0;
            }
        } else if (++w->frame >= 4) {
            w->line = 0;
        }
        break;
    default:
        w->line = 0;
        break;
    }
}

s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
struct EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Identity(f32 *m);
extern f32 gRodTipOffsets[][3];

/* The tip of the player's rod: bone 0x12 plus (-9.2, -1.7, 12) in its frame, plus the offset
 * gRodTipOffsets[k] of the cast's bend k, which the casting motions (0x327-0x32A) set by frame. */
void Player_GetRodTip(f32 *out, Player *p)
{
    f32 v[3];
    f32 t[3];
    f32 m[16];
    f32 r[16];
    f32 *vy;
    f32 *ty;
    f32 *vz;
    f32 *tz;
    s16 k;

    switch (p->motionId[0]) {
    case 0x327:
        if (!Motion_ReachedFrame(p, 0, 20.0f)) {
            k = 0;
        } else if (!Motion_ReachedFrame(p, 0, 30.0f)) {
            k = 1;
        } else {
            k = 2;
        }
        break;
    case 0x328:
        if (!Motion_ReachedFrame(p, 0, 20.0f)) {
            k = 1;
        } else if (!Motion_ReachedFrame(p, 0, 26.0f)) {
            k = 2;
        } else if (!Motion_ReachedFrame(p, 0, 28.0f)) {
            k = 1;
        } else {
            k = 0;
        }
        break;
    case 0x329:
        if (!Motion_ReachedFrame(p, 0, 36.0f)) {
            k = 1;
        } else if (!Motion_ReachedFrame(p, 0, 40.0f)) {
            k = 2;
        } else if (!Motion_ReachedFrame(p, 0, 42.0f)) {
            k = 1;
        } else {
            k = 0;
        }
        break;
    case 0x32A:
        if (!Motion_ReachedFrame(p, 0, 20.0f)) {
            k = 1;
        } else if (Motion_ReachedFrame(p, 0, 22.0f)) {
            k = 2;
        } else if (Motion_ReachedFrame(p, 0, 24.0f)) {
            k = 1;
        } else {
            k = 0;
        }
        break;
    default:
        k = 0;
        break;
    }
    Mtx_Copy(m, (void *)Em_GetBone(p, 0x12));
    v[0] = -9.2f;
    vy = &v[1];
    *vy = -1.7f;
    vz = &v[2];
    *vz = 12.0f;
    Vec_RotateSelf(v, m);
    Mtx_Identity(r);
    Mtx_RotateZ(r, -1.5707964f);
    Mtx_Concat3x3(r, m);
    t[0] = gRodTipOffsets[k][0];
    ty = &t[1];
    *ty = gRodTipOffsets[k][1];
    tz = &t[2];
    *tz = gRodTipOffsets[k][2];
    Vec_RotateSelf(t, r);
    out[0] = t[0] + (m[12] + v[0]);
    out[1] = *ty + (m[13] + *vy);
    out[2] = *tz + (m[14] + *vz);
}

void Fishing_PlaySe(FxObj *o, f32 *pos, s16 k)
{
    switch (k) {
    case 0:
        Sound_PlaySeAtPosChecked(1, 0x2E, 0, pos, 1, 0);
        break;
    case 1:
        Sound_PlaySeAtPosChecked(1, 0x70, 0, pos, 1, 0);
        break;
    }
}

void CountdownFx_Update(FxObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        CountdownFx_Start(o);
        break;
    case 1:
        CountdownFx_Run(o);
        break;
    case 2:
        CountdownFx_End(o);
        break;
    case 3:
        CountdownFx_Free(o);
        break;
    }
}

void CountdownFx_Draw(DrawSlot *arg0);

/* Starts the effect: state 1 with a draw slot drawn by CountdownFx_Draw (then CountdownFx_Run), or StageObj_Free when no slot is free. */
void CountdownFx_Start(FxObj *o)
{
    o->step = 0;
    o->phase = 0;
    o->claimed = 0;
    o->timer = 0;
    o->state = 1;
    o->visible = 1;
    o->unk14 = 0;
    o->slotId = DrawSlot_Alloc();
    if (o->slotId != -1) {
        o->slot = DrawSlot_Get(o->slotId);
        o->slot->owner = (void *)o;
        o->slot->draw = CountdownFx_Draw;
        CountdownFx_Run(o);
        return;
    }
    StageObj_Free((struct StageObj *) o);
}

s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

/* The countdown's run state: a short delay, a drop (unk0A counts down to -9), then a wait on the
 * timer (sound 7/0x1F unless gScene.online is 1) before the end; drawn every frame. */
void CountdownFx_Run(FxObj *o)
{
    if (o->slot == NULL) {
        o->state = 2;
        return;
    }
    o->angle--;
    switch (o->step) {
    case 0:
        if (++o->timer >= 5) {
            o->step++;
        }
        break;
    case 1:
        if (o->angle < -8) {
            o->step++;
        }
        break;
    case 2:
        if (--o->timer <= 0) {
            if (gScene.online != 1) {
                Sound_PlaySeFlat(7, 0x1F, 0);
            }
            o->state = 2;
        }
        break;
    }
    OT_AddAt(gDrawOtSorted, o->slot, 0, 1);
}

void CountdownFx_End(FxObj *o)
{
    o->state = 3;
    if (o->slot != NULL) {
        DrawSlot_Free(o->slotId);
    }
}

void CountdownFx_Free(FxObj *o)
{
    StageObj_Free((struct StageObj *)o);
}

void CountdownFx_Draw(DrawSlot *slot)
{
    FxObj *o;

    o = slot->owner;
    if (o->visible != 0) {
        Text_SetSize(0);
        Countdown_Draw(o->angle);
        Letterbox_Draw((s8)o->timer);
    }
}

void CountdownFx_Update(FxObj *o);

/* Starts an effect object of kind 0x2C (update CountdownFx_Update) with +0xA = n. */
void Effect2C_Spawn(s16 n)
{
    FxObj *e = (FxObj *)StageObj_Alloc(0);

    if (e != NULL) {
        e->kind = 0x2C;
        e->type = 0;
        e->update = CountdownFx_Update;
        e->angle = n;
    }
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0798);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B07A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B07B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B07D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B07F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0820);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0840);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0860);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0870);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0898);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B08A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B08C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B08E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0900);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0920);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0940);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0960);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0988);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B09A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B09C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0A00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0A20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0A40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0A80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0AA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0AC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0AE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0B00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0B20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0B40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0B60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0B80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0B90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0BA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0BB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0BD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0BF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0C10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0C30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0C50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0C70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0C88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0CA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0CB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0CC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0CD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0CE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0D00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0D20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0D40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0D60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0D80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0DA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0DC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0DE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0E00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0E20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0E40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0E60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0E80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0EA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0EC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0EE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0F00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0F20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0F58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0F60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0F80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0FA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0FC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0FE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0FE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B0FF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1000);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1030);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1070);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B10A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B10D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B10F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1110);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1130);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1160);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B11C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B11F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B12A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B12C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B12F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B13A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B13C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B13F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1440);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B14B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B14D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B14F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1550);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1590);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B15B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B15D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B15F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1620);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1650);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1670);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1690);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B16A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B16B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B16C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B16D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B16D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B16E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1730);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1780);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B17A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B17D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1800);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1830);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1860);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1878);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1888);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3E3D00", D_004B1890);

f32 Math_Absf(f32 x);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Font_SetSpacing(u8 a, u8 b);
void Countdown_DrawDigit(u8 n, u32 color, f32 scale);
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);


/* The countdown's last seconds: the number (3, 2, 1) with its sound at the start of each second,
 * drawn larger and fading while the second passes. */
void Countdown_Draw(s16 t)
{
    u8 sec;
    u8 k;
    u8 lvl;
    s32 alpha;
    u32 color;
    f32 scale;

    if (t <= 0) {
        return;
    }
    sec = (t - 1) / 30 + 1;
    switch (sec) {
    case 3:
        k = Math_Absf(t - 90);
        if (k == 6) {
            if (gScene.online != 1) {
                Sound_PlaySeFlat(7, 0x1C, 0);
            } else {
                Sound_PlaySeFlat(6, 0x55, 0);
            }
        }
        break;
    case 2:
        k = Math_Absf(t - 60);
        if (k == 6) {
            if (gScene.online != 1) {
                Sound_PlaySeFlat(7, 0x1D, 0);
            } else {
                Sound_PlaySeFlat(6, 0x56, 0);
            }
        }
        break;
    case 1:
        k = Math_Absf(t - 30);
        if (k == 6) {
            if (gScene.online != 1) {
                Sound_PlaySeFlat(7, 0x1E, 0);
            } else {
                Sound_PlaySeFlat(6, 0x57, 0);
            }
        }
        break;
    default:
        k = 100;
        break;
    }
    if (k < 2) {
        lvl = 0;
    } else if (k < 4 && k >= 2) {
        lvl = 1;
    } else if (k < 6 && k >= 4) {
        lvl = 2;
    } else if (k > 27) {
        lvl = 5;
    } else if (k < 28 && k >= 25) {
        lvl = 4;
    } else {
        lvl = 3;
    }
    switch (lvl) {
    case 0:
        scale = 8.0f;
        alpha = 0x4B;
        break;
    case 1:
        scale = 3.0f;
        alpha = 0x80;
        break;
    case 2:
        scale = 1.0f;
        alpha = 0x99;
        break;
    case 3:
        scale = 1.1f;
        alpha = 0xAF;
        break;
    case 4:
        scale = 0.8f;
        alpha = 0x32;
        break;
    case 5:
    default:
        alpha = 0;
        scale = 0.0f;
        break;
    }
    color = (alpha << 24) | 0xA01B00;
    if (scale > 0.0f) {
        Countdown_DrawDigit(sec, color, scale);
    }
    if (lvl < 5) {
        Font_SetSpacing(0x20, 0x30);
        Text_PrintfAt(0x138, 0x186, 0, "%i", sec);
    }
}

/* A countdown digit (1, 2 or 3) at the screen's center, as one to three triangles of the given
 * color and size (124 x scale high). */
void Countdown_DrawDigit(u8 n, u32 color, f32 scale)
{
    PrimTri a, b, c;
    f32 h = 124.0f * scale;
    f32 w = 0.865f * h;

    switch (n) {
    case 1:
        a.x0 = 320.0f - 0.25f * h;
        a.y0 = 224.0f + 0.5f * w;
        a.x1 = 320.0f + 0.25f * h;
        a.y1 = a.y0;
        a.x2 = 320;
        a.y2 = 224.0f - 0.5f * w;
        a.color = color;
        Prim_FillTri(&a);
        break;
    case 2:
        a.x0 = 320.0f - 0.5f * h;
        a.y0 = 224.0f + 0.5f * w;
        a.x1 = 320;
        a.y1 = a.y0;
        a.x2 = 320.0f - 0.25f * h;
        a.y2 = 224.0f - 0.5f * w;
        b.x0 = a.x1;
        b.y0 = a.y1;
        b.x1 = 320.0f + 0.5f * h;
        b.y1 = b.y0;
        b.x2 = 320.0f + 0.25f * h;
        b.y2 = 224.0f - 0.5f * w;
        a.color = b.color = color;
        Prim_FillTri(&a);
        Prim_FillTri(&b);
        break;
    case 3:
        a.x0 = 320.0f - 0.5f * h;
        a.y0 = 224.0f + 0.5f * w;
        a.x1 = 320;
        a.y1 = a.y0;
        a.x2 = 320.0f - 0.25f * h;
        a.y2 = 224;
        b.x0 = a.x1;
        b.y0 = a.y1;
        b.x1 = 320.0f + 0.5f * h;
        b.y1 = b.y0;
        b.x2 = 320.0f + 0.25f * h;
        b.y2 = a.y2;
        c.x0 = a.x2;
        c.y0 = a.y2;
        c.x1 = b.x2;
        c.y1 = b.y2;
        c.x2 = 320;
        c.y2 = 224.0f - 0.5f * w;
        a.color = b.color = c.color = color;
        Prim_FillTri(&a);
        Prim_FillTri(&b);
        Prim_FillTri(&c);
        break;
    }
}

/* The last function of .text: the 32 nops after it, up to vu1_microcode, are the YAML's
 * text_end piece. */
/* Black bars at the top and bottom of the screen (16 per unit of n, at most 72 high), two
 * triangles each. */
void Letterbox_Draw(s8 n)
{
    PrimTri a, b, c, d;
    f32 h = 16.0f * n;

    if (!(h < 72.0f)) {
        h = 72.0f;
    }
    a.x0 = 0;
    a.y0 = 0;
    a.x1 = 0x280;
    a.y1 = h;
    a.x2 = 0x280;
    a.y2 = 0;
    a.color = 0xFF000000;
    b.x0 = 0;
    b.y0 = 0;
    b.x1 = 0x280;
    b.y1 = h;
    b.x2 = 0;
    b.y2 = h;
    b.color = 0xFF000000;
    c.x0 = 0;
    c.y0 = 448.0f - h;
    c.x1 = 0x280;
    c.y1 = 0x1C0;
    c.x2 = 0x280;
    c.y2 = 448.0f - h;
    c.color = 0xFF000000;
    d.x0 = 0;
    d.y0 = 448.0f - h;
    d.x1 = 0x280;
    d.y1 = 0x1C0;
    d.x2 = 0;
    d.y2 = 0x1C0;
    d.color = 0xFF000000;
    Prim_FillTri(&a);
    Prim_FillTri(&b);
    Prim_FillTri(&c);
    Prim_FillTri(&d);
}

/* Zeros up to the end of the section (0x4B1900): after Countdown_Draw's jump table (to 0x4B18B8)
 * and the alignment of this array. */
const u32 D_004B18C0[16] = { 0 };

/* unk_3E3D00's .data (0x4698E0-0x46A100) */
extern char D_004B0FE8[];
extern char D_004B16E0[];
extern char D_004B1730[];
extern char D_004B1780[];
extern char D_004B17A0[];
extern char D_004B17D0[];
extern char D_004B1800[];
extern char D_004B1830[];
extern char D_004B1848[];
extern char D_004B1860[];
extern char D_004B1878[];
extern char D_004B1888[];
extern char D_004B1890[];
FishKind gFishKindsOffline[24] = {
#include "sub_main/004698E0.inc"
};
FishKind gFishKindsOnline[24] = {
#include "sub_main/00469AC0.inc"
};
u32 gOnlineStageData_1[12] = {
#include "sub_main/00469CA0.inc"
};
u32 gOnlineStageData_3[12] = {
#include "sub_main/00469CD0.inc"
};
u32 gOnlineStageData[12] = {
#include "sub_main/00469D00.inc"
};
u32 gOnlineSeasonalData_0[12] = {
#include "sub_main/00469D30.inc"
};
u32 gOnlineSeasonalData_2[12] = {
#include "sub_main/00469D60.inc"
};
u32 gOnlineSeasonalData[4] = {
    (u32)gOnlineSeasonalData_0, (u32)gOnlineSeasonalData_0, (u32)gOnlineSeasonalData_2, (u32)gOnlineSeasonalData_2,
};
u16 gFishBiteJitter[4] = {
#include "sub_main/00469DA0.inc"
};
s8 gFishWaitSecs[8] = {
#include "sub_main/00469DA8.inc"
};
u32 gLureColors[4] = {
#include "sub_main/00469DB0.inc"
};
u32 gLureColorDefault = {
#include "sub_main/00469DC0.inc"
};
f32 gRodTipOffsets[4][3] = {
#include "sub_main/00469DD0.inc"
};
extern char D_004B0798[];
extern char D_004B07A0[];
extern char D_004B07B8[];
extern char D_004B07D0[];
extern char D_004B07F0[];
extern char D_004B0820[];
extern char D_004B0840[];
extern char D_004B0860[];
extern char D_004B0868[];
extern char D_004B0870[];
extern char D_004B0898[];
extern char D_004B08A8[];
extern char D_004B08C0[];
extern char D_004B08E0[];
extern char D_004B0900[];
extern char D_004B0920[];
extern char D_004B0940[];
extern char D_004B0960[];
extern char D_004B0988[];
extern char D_004B09A0[];
extern char D_004B09C0[];
extern char D_004B0A00[];
extern char D_004B0A20[];
extern char D_004B0A40[];
extern char D_004B0A80[];
extern char D_004B0AA0[];
extern char D_004B0AC0[];
extern char D_004B0AE0[];
extern char D_004B0B00[];
extern char D_004B0B20[];
extern char D_004B0B40[];
extern char D_004B0B60[];
extern char D_004B0B80[];
extern char D_004B0B90[];
extern char D_004B0BA0[];
extern char D_004B0BB0[];
extern char D_004B0BD0[];
extern char D_004B0BF0[];
extern char D_004B0C10[];
extern char D_004B0C30[];
extern char D_004B0C50[];
extern char D_004B0C70[];
extern char D_004B0C88[];
extern char D_004B0CA0[];
extern char D_004B0CB8[];
extern char D_004B0CC8[];
extern char D_004B0CD8[];
extern char D_004B0CE8[];
extern char D_004B0D00[];
extern char D_004B0D20[];
extern char D_004B0D40[];
extern char D_004B0D60[];
extern char D_004B0D80[];
extern char D_004B0DA0[];
extern char D_004B0DC0[];
extern char D_004B0DE0[];
extern char D_004B0E00[];
extern char D_004B0E20[];
extern char D_004B0E40[];
extern char D_004B0E60[];
extern char D_004B0E80[];
extern char D_004B0EA0[];
extern char D_004B0EC0[];
extern char D_004B0EE0[];
extern char D_004B0F00[];
extern char D_004B0F20[];
extern char D_004B0F58[];
extern char D_004B0F60[];
extern char D_004B0F80[];
extern char D_004B0FA0[];
extern char D_004B0FC0[];
extern char D_004B0FE0[];
extern char D_004B0FF0[];
extern char D_004B1000[];
extern char D_004B1030[];
extern char D_004B1070[];
extern char D_004B10A0[];
extern char D_004B10D0[];
extern char D_004B10F0[];
extern char D_004B1110[];
extern char D_004B1130[];
extern char D_004B1160[];
extern char D_004B1190[];
extern char D_004B11C0[];
extern char D_004B11F0[];
extern char D_004B1220[];
extern char D_004B1250[];
extern char D_004B1280[];
extern char D_004B12A0[];
extern char D_004B12C0[];
extern char D_004B12F0[];
extern char D_004B1320[];
extern char D_004B1350[];
extern char D_004B1380[];
extern char D_004B13A0[];
extern char D_004B13C0[];
extern char D_004B13F0[];
extern char D_004B1410[];
extern char D_004B1440[];
extern char D_004B1470[];
extern char D_004B1490[];
extern char D_004B14B0[];
extern char D_004B14D0[];
extern char D_004B14F0[];
extern char D_004B1510[];
extern char D_004B1530[];
extern char D_004B1550[];
extern char D_004B1570[];
extern char D_004B1590[];
extern char D_004B15B0[];
extern char D_004B15D0[];
extern char D_004B15F0[];
extern char D_004B1620[];
extern char D_004B1650[];
extern char D_004B1670[];
extern char D_004B1690[];
extern char D_004B16A8[];
extern char D_004B16B8[];
extern char D_004B16C8[];
extern char D_004B16D0[];
extern char D_004B16D8[];
/* 168 text pointers (into this file's .rodata) that no code names. */
char *gPlselTexts[168] = {
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, D_004B07A0,
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, D_004B07A0,
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B07B8, D_004B07D0,
    D_004B07F0, D_004B0820, D_004B0840, D_004B07A0,
    D_004B0860, D_004B0868, D_004B0870, D_004B0798,
    D_004B0798, D_004B0898, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, NULL,
    D_004B08A8, D_004B08C0, D_004B08E0, D_004B0900,
    D_004B0920, D_004B0940, D_004B08A8, D_004B0960,
    D_004B0988, D_004B09A0, D_004B09C0, D_004B0A00,
    D_004B08A8, D_004B0A20, D_004B0A40, D_004B0A80,
    D_004B0AA0, D_004B0AC0, D_004B0AE0, D_004B0B00,
    D_004B0B20, D_004B08A8, D_004B0B40, D_004B0988,
    D_004B0B60, D_004B0B80, D_004B0B90, D_004B0BA0,
    D_004B0BB0, D_004B0BD0, D_004B0BF0, D_004B0C10,
    D_004B0C30, D_004B0C50, D_004B0C70, D_004B0C88,
    D_004B0CA0, D_004B0CB8, D_004B0CC8, D_004B0CD8,
    D_004B0CE8, D_004B0D00, D_004B0D20, D_004B0D40,
    D_004B0D60, D_004B0D80, D_004B0DA0, D_004B0DC0,
    D_004B0DE0, D_004B0E00, D_004B0E20, D_004B0E40,
    D_004B0E60, D_004B0E80, D_004B0EA0, D_004B0EC0,
    D_004B0EE0, D_004B0F00, NULL, NULL,
    D_004B0F20, D_004B0F58, D_004B0F60, D_004B0F80,
    D_004B0FA0, D_004B0FC0, D_004B0FE0, (char *)D_004B0FE8,
    D_004B0FF0, D_004B1000, D_004B1030, D_004B1070,
    D_004B10A0, D_004B10D0, NULL, NULL,
    D_004B10F0, D_004B1110, D_004B1130, D_004B1160,
    D_004B1190, D_004B11C0, D_004B11F0, D_004B1220,
    D_004B1250, D_004B1250, D_004B1220, D_004B1280,
    D_004B12A0, D_004B12C0, D_004B12F0, D_004B1320,
    D_004B1350, D_004B1380, D_004B13A0, D_004B13C0,
    D_004B13F0, D_004B1410, D_004B1440, D_004B1220,
    D_004B1470, D_004B1490, D_004B14B0, D_004B14D0,
    D_004B14F0, D_004B1510, D_004B1530, D_004B1550,
    D_004B1570, D_004B1590, D_004B15B0, D_004B15D0,
    D_004B15F0, D_004B1620, D_004B1650, D_004B1670,
    D_004B1690, D_004B16A8, D_004B16B8, D_004B16C8,
    D_004B16D0, D_004B16D8, NULL, NULL,
};
char *gHelpTexts_6[24] = {
    D_004B16E0, D_004B1730, D_004B1780, D_004B17A0,
    D_004B17D0, D_004B1800, NULL, NULL,
    D_004B1830, D_004B1848, D_004B1830, D_004B0FE8,
    NULL, NULL, NULL, NULL,
    D_004B1860, D_004B1878, D_004B1888, D_004B1890,
    NULL, NULL, NULL, NULL,
};
