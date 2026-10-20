/*
 * A flock of birds in the town's hubs: a stage object (kind
 * LOBBY_OBJ_BIRD_FLOCK) whose buf is a BirdFlock of FLOCK_BIRDS birds
 * (FlockBird). The hub's set-up spawns one (BirdFlock_Spawn).
 *
 * Each bird sits on one of the FLOCK_BIRD_SPOTS spots of gFlockBirdSpots,
 * one bird to a spot, and turns about now and then. In time it spreads its
 * wings and flies off, climbing; once it is high enough it is moved to
 * another free spot far away and flies in to land on it, calling as it
 * comes. The birds are drawn with the common effect model: each bird shows
 * one material (a pose) of its material set, stepped through by the
 * animations of gFlockBirdAnims (lists of (part, frames) pairs). One bird in
 * eight gets one of three tints.
 *
 * The file's data: the birds' tints, the animations and their pair lists,
 * and the spots.
 */
#include "common.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "stageobj.h"
#include "lobby_menu.h"
#include "sprite.h"
#include "em.h"
#include "sound.h"
#include "render_state.h"
/* Built at optimisation level 4: the compiler gives these functions the
 * game's code only at this level. */
#pragma optimization_level 4

void BirdFlock_Update(StageObj *obj);
void BirdFlock_Init(StageObj *obj);
void BirdFlock_Run(StageObj *obj);
void BirdFlock_End(StageObj *obj);
void BirdFlock_Free(StageObj *obj);
void DrawSlot_Free(s32);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void FlockBird_Update(BirdFlock *flock, FlockBird *bird, s32 i);

s32 Sound_PlaySeAtPosChecked(s32, s32, s32, void *, s32, s32);

/* Plays sound soundId of the monsters' bank at the bird's position (pos is its
 * first field); param goes to the sound as it is. */
static void FlockBird_PlaySe(s32 soundId, FlockBird *bird, s32 param)
{
    Sound_PlaySeAtPosChecked(SE_SLOT_EM, soundId, 0, bird, param, 0);
}

/* The flock's update (StageObj.update): runs its state, LOBBY_OBJ_INIT to
 * LOBBY_OBJ_FREE. */
void BirdFlock_Update(StageObj *obj)
{
    u8 state;

    state = obj->state;
    switch (state) {
    case LOBBY_OBJ_INIT:
        BirdFlock_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        BirdFlock_Run(obj);
        break;
    case LOBBY_OBJ_END:
        BirdFlock_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        BirdFlock_Free(obj);
        break;
    }
}

extern FlockBirdAnim gFlockBirdAnims[];

f32 Math_Cos(f32 a);
f32 Math_Sin(f32 a);
u16 Rand_Next(s32 stream);

/* Starts a bird's flight in to its (new) spot: its destination is 20 above
 * the spot, and it starts 8000 away from it in one of 8 directions at
 * random, and 4000 above. Its speeds are set towards the spot, by the
 * flock's scale. */
static void FlockBird_StartArrival(FlockBird *bird, f32 scale)
{
    f32 angle;
    f32 dx;
    f32 dz;
    f32 speed;

    bird->ang = Rand_Next(1) % 8;
    bird->dest.x = bird->pos.x;
    bird->dest.y = 20.0f + bird->pos.y;
    bird->dest.z = bird->pos.z;
    angle = (6.2831855f * (u16)(((bird->ang + 4) & 7) << 13)) / 65536.0f;
    dx = 8000.0f * Math_Sin(angle);
    dz = 8000.0f * Math_Cos(angle);
    bird->pos.y += 4000.0f;
    bird->pos.x += dx;
    bird->pos.z += dz;
    bird->vel.z = -15.0f * scale;
    speed = 20.0f * scale;
    bird->vel.x = speed;
    bird->vel.y = speed;
    if (dx > 0.0f) {
        bird->vel.x *= -1.0f;
    }
    if (dz > 0.0f) {
        bird->vel.y *= -1.0f;
    }
}

/* Starts animation anim (enum FlockBirdAnimId) on a bird: its material set
 * and its first (part, frames) pair; an animation without a list shows
 * part 0 and stays. */
static void FlockBird_SetAnim(FlockBird *bird, s32 anim)
{
    FlockBirdPair *pair;

    bird->anim = anim;
    bird->matSet = gFlockBirdAnims[anim].matSet;
    bird->list = gFlockBirdAnims[anim].list;
    bird->idx = 0;
    if (bird->list == NULL) {
        bird->part = 0;
        bird->frames = -1;
    } else {
        pair = bird->list + bird->idx;
        bird->part = pair->part;
        bird->frames = pair->frames;
    }
}

/* Steps a bird's animation a frame: counts the part's frames down, then
 * takes the next pair (a part of -1 ends the list: loops back, or stops).
 * At the end of FLOCK_BIRD_ANIM_SPREAD the bird flies off; at the end of
 * FLOCK_BIRD_ANIM_FOLD it sits, 60 frames before it may fly off again. */
static void FlockBird_Animate(FlockBird *bird)
{
    FlockBirdPair *pair;

    if (bird->frames != -1) {
        if (bird->frames > 0) {
            bird->frames--;
        } else {
            pair = bird->list;
            bird->idx++;
            pair += bird->idx;
            if (pair->part == -1) {
                bird->idx = pair->frames;
            }
            bird->part = pair->part;
            bird->frames = pair->frames;
            if (bird->part == -1) {
                if (bird->anim == FLOCK_BIRD_ANIM_SPREAD) {
                    FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_FLAP);
                    bird->state = FLOCK_BIRD_LEAVING;
                    bird->step = 0;
                    bird->spreadCount = 0;
                } else if (bird->anim == FLOCK_BIRD_ANIM_FOLD) {
                    FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_SIT);
                    bird->state = FLOCK_BIRD_SITTING;
                    bird->step = 0;
                    bird->spreadCount = 0;
                    bird->timer = 60; /* frames: 2 s */
                } else {
                    bird->part = 0;
                }
            }
        }
    }
}

extern Vec3f gFlockBirdSpots[];

/* Puts bird i on a spot of gFlockBirdSpots at random that no other bird
 * holds, sitting, facing one of 8 directions, 1 to 5 seconds before it may
 * fly off. */
static void FlockBird_PickSpot(BirdFlock *flock, s32 i)
{
    FlockBird *bird = &flock->bird[i];
    s32 spot;
    s32 j;
    f32 *spotPos;

    /* draw spots until one is free */
retry:
    spot = Rand_Next(1) % FLOCK_BIRD_SPOTS;
    for (j = 0; j < FLOCK_BIRDS; j++) {
        if (j != i && spot == flock->spot[j]) {
            goto retry;
        }
    }
    flock->spot[i] = spot;
    spotPos = (f32 *)&gFlockBirdSpots[spot];
    bird->pos.x = *spotPos++;
    bird->pos.y = *spotPos++;
    bird->pos.z = *spotPos;
    bird->ang = 0;
    bird->unk3C = 20.0f;
    bird->state = FLOCK_BIRD_SITTING;
    bird->step = 0;
    bird->timer = (Rand_Next(1) % 5 + 1) * 30;
    bird->turnWait = 30;
    bird->dir = Rand_Next(1) % 8;
    bird->ang = (u16)bird->dir << 13;
    FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_SIT);
}

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void BirdFlock_Draw(DrawSlot *slot);
extern s32 gFlockBirdColors[];

/* The flock's set-up: takes a draw slot (none free: the object is freed),
 * draws the flock there (BirdFlock_Draw) at the object's position, sets the
 * scale 1.2 and puts the birds on their spots. One bird in eight gets one of
 * three tints, the others none. */
void BirdFlock_Init(StageObj *obj)
{
    BirdFlock *flock;
    FlockBird *bird;
    s32 i;
    DrawSlot *slot;

    obj->slot = DrawSlot_Alloc();
    if (obj->slot == -1) {
        obj->state = LOBBY_OBJ_FREE;
        return;
    }
    flock = (BirdFlock *)obj->buf;
    slot = DrawSlot_Get(obj->slot);
    obj->node = slot;
    slot->owner = (StageObj *)flock;
    slot->draw = BirdFlock_Draw;
    *(Vec3f *)slot->pos = *(Vec3f *)obj->pos;
    obj->state = LOBBY_OBJ_RUN;
    obj->var0 = 0;
    flock->scale = 1.2f;
    for (i = 0; i < FLOCK_SPOT_SLOTS; i++) {
        flock->spot[i] = -1;
    }
    for (bird = flock->bird, i = 0; i < FLOCK_BIRDS; i++, bird++) {
        FlockBird_PickSpot(flock, i);
        if (Rand_Next(1) % 8 == 2) {
            bird->color = gFlockBirdColors[Rand_Next(1) % 3];
        } else {
            bird->color = -1;
        }
    }
}

f32 Vec_DistanceXZ(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
s32 LobbyWorld_GetSeason(void);

/* Moves bird i a frame, by its state, then steps its animation.
 * - Sitting: every 30 frames it may turn to one of 16 directions; its wait
 *   over, it spreads its wings 3 times in 10 when facing between 0 and
 *   0x5FFF.
 * - Arriving: it flies at its spot, calls (FLOCK_BIRD_SE_CALL) about 42
 *   frames out, drops onto the spot over 12 frames and folds its wings.
 * - Leaving: it lifts off over 6 frames (FLOCK_BIRD_SE_TAKE_OFF), then flies
 *   off climbing; 4000 up it is moved to a new spot to fly in to, with a new
 *   tint. */
void FlockBird_Update(BirdFlock *flock, FlockBird *bird, s32 i)
{
    Vec3f dest;
    f32 frames;
    Vec3f *spot;

    switch (bird->state) {
    case FLOCK_BIRD_SITTING:
        switch (bird->step) {
        case 0:
            if (bird->turnWait != 0) {
                bird->turnWait--;
            } else {
                bird->turnWait = 30;
                if (!(Rand_Next(1) & 1)) {
                    bird->dir = Rand_Next(1) & 0xF;
                    bird->ang = (u16)bird->dir << 12;
                    break;
                }
            }
            if (bird->timer != 0) {
                bird->timer--;
            } else if (Rand_Next(1) % 10 <= 2 && bird->ang > 0 && bird->ang <= 0x5FFF) {
                FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_SPREAD);
                bird->spreadCount++;
            } else {
                bird->timer = (Rand_Next(1) % 5 + 1) * 30;
            }
            break;
        case 1:
            break;
        }
        break;
    case FLOCK_BIRD_ARRIVING:
        switch (bird->step) {
        case 0:
            if (bird->called == 0) {
                dest.x = bird->dest.x;
                dest.y = bird->dest.y;
                dest.z = bird->dest.z;
                frames = Vec_DistanceXZ((f32 *) &dest, (f32 *) &bird->pos);
                frames = Math_Absf(frames) / Math_Absf(bird->vel.x);
                if ((s32)frames <= 42) {
                    FlockBird_PlaySe(FLOCK_BIRD_SE_CALL, bird, 0x13);
                    bird->called = 1;
                }
            }
            if (Math_Absf(bird->dest.x - bird->pos.x) <= 20.0f) {
                bird->pos.x = bird->dest.x;
            } else {
                bird->pos.x += bird->vel.x;
            }
            if (Math_Absf(bird->dest.z - bird->pos.z) <= 20.0f) {
                bird->pos.z = bird->dest.z;
            } else {
                bird->pos.z += bird->vel.y;
            }
            if (Math_Absf(bird->dest.y - bird->pos.y) <= 20.0f) {
                bird->pos.y = bird->dest.y;
            } else {
                bird->pos.y += bird->vel.z;
            }
            if (bird->pos.x == bird->dest.x && bird->pos.z == bird->dest.z && bird->pos.y == bird->dest.y) {
                FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_FLAP);
                bird->vel.z = -4.0f;
                bird->timer = 12;
                bird->step++;
            } else {
                bird->ang = Math_Heading(bird->dest.x, bird->dest.z, bird->pos.x, bird->pos.z);
                bird->ang += 0x4000;
            }
            break;
        case 1:
            bird->timer--;
            bird->pos.y += bird->vel.z;
            if (bird->vel.z < -0.1f) {
                bird->vel.z += 0.3f;
            } else {
                bird->vel.z = -0.1f;
            }
            spot = &gFlockBirdSpots[flock->spot[i]];
            if (spot->y > bird->pos.y) {
                bird->pos.y = spot->y;
            }
            if (bird->timer == 0) {
                bird->pos.y = spot->y;
                FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_FOLD);
                bird->step++;
            }
            break;
        case 2:
            break;
        }
        break;
    case FLOCK_BIRD_LEAVING:
        switch (bird->step) {
        case 0:
            bird->vel.x = 0.5f;
            bird->vel.y = 0.5f;
            bird->vel.z = 4.0f * flock->scale;
            bird->accel = 0.3f * flock->scale;
            bird->climb = 0.0f;
            bird->timer = 6;
            bird->step++;
            FlockBird_PlaySe(FLOCK_BIRD_SE_TAKE_OFF, bird, 0x13);
        case 1:
            bird->timer--;
            bird->pos.y += bird->vel.z;
            bird->pos.x += bird->vel.x * Math_Sin((6.2831855f * bird->ang) / 65536.0f);
            bird->pos.z += bird->vel.y * Math_Cos((6.2831855f * bird->ang) / 65536.0f);
            if (bird->vel.z > 0.1f) {
                bird->vel.z -= 0.3f * flock->scale;
            } else {
                bird->vel.z = 0.1f;
            }
            if (bird->vel.z < 15.0f) {
                bird->vel.z += bird->accel;
            }
            if (bird->vel.y < 40.0f) {
                bird->vel.y += 0.2f * flock->scale;
            }
            if (bird->vel.x < 40.0f) {
                bird->vel.x += 0.2f * flock->scale;
            }
            if (bird->timer != 0) {
                break;
            }
            FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_FLY);
            bird->step++;
        case 2:
            bird->pos.x += bird->vel.x * Math_Sin((6.2831855f * bird->ang) / 65536.0f);
            bird->pos.z += bird->vel.y * Math_Cos((6.2831855f * bird->ang) / 65536.0f);
            bird->pos.y += bird->vel.z;
            if (bird->vel.z < 15.0f) {
                bird->vel.z += bird->accel;
            }
            if (bird->vel.y < 40.0f) {
                bird->vel.y += 1.5f * flock->scale;
            }
            if (bird->vel.x < 40.0f) {
                bird->vel.x += 1.5f * flock->scale;
            }
            bird->climb += bird->vel.z;
            if (bird->climb >= 4000.0f) {
                flock->spot[i] = -1;
                FlockBird_PickSpot(flock, i);
                bird->step = 0;
                bird->spreadCount = 0;
                bird->called = 0;
                /* overwritten below: the tint is drawn again */
                bird->color = gFlockBirdColors[LobbyWorld_GetSeason()];
                bird->state = FLOCK_BIRD_ARRIVING;
                FlockBird_SetAnim(bird, FLOCK_BIRD_ANIM_FLY);
                FlockBird_StartArrival(bird, flock->scale);
                if (Rand_Next(1) % 8 == 2) {
                    bird->color = gFlockBirdColors[Rand_Next(1) % 3];
                } else {
                    bird->color = -1;
                }
            }
            break;
        }
        break;
    }
    FlockBird_Animate(bird);
}

/* The flock's run state: moves the birds and queues its draw slot. */
void BirdFlock_Run(StageObj *obj)
{
    BirdFlock *flock;
    FlockBird *bird;
    s32 i;

    flock = (BirdFlock *)obj->buf;
    i = 0;
    bird = flock->bird;
    do {
        FlockBird_Update(flock, bird, i);
        i += 1;
        bird += 1;
    } while (i < FLOCK_BIRDS);
    OT_AddByDepth(gDrawOt32, obj->node, DRAW_OT32_LEN, 0);
}

/* The flock's end state: frees its draw slot and goes on to LOBBY_OBJ_FREE. */
void BirdFlock_End(StageObj *obj)
{
    obj->state = (u8) (obj->state + 1);
    DrawSlot_Free(obj->slot);
}

/* The flock's free state: frees the object. */
void BirdFlock_Free(StageObj *obj)
{
    StageObj_Free((StageObj *) obj);
}

void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Gs_SetState(u8, u32);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);

/* The flock's draw (its draw slot's): each bird as its material set of the
 * common effect model with only its part's material shown (the set's first
 * material is left as it is), at the flock's scale, turned to its heading,
 * in its tint. Draws nothing while the common model isn't loaded. */
void BirdFlock_Draw(DrawSlot *slot)
{
    Vec3f trans;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    Model *model;
    ModelMat *mats;
    BirdFlock *flock;
    ModelMatSet *set;
    s32 i;
    FlockBird *bird;
    s32 j;
    ModelMat *mat;

    flock = (BirdFlock *)slot->owner;
    model = gCommonModels[0];
    bird = flock->bird;
    if (model == NULL || model->active == 0) {
        return;
    }
    for (i = 0; i < FLOCK_BIRDS; i++, bird++) {
        mats = model->mats;
        set = model->matSets + bird->matSet;
        for (j = 1; j < set->count; j++) {
            mat = &mats[set->mat[j]];
            if (j == bird->part) {
                mat->alpha = 1.0f;
            } else {
                mat->alpha = 0.0f;
            }
        }
        trans = bird->pos;
        scale[0] = flock->scale;
        scale[1] = flock->scale;
        scale[2] = flock->scale;
        rot[0] = 0.0f;
        rot[1] = (6.2831855f * bird->ang) / 65536.0f;
        rot[2] = 0.0f;
        /* rotation order 2: about y only */
        Mtx_FromScaleRotTrans((f32 *) scale, (f32 *) rot, (f32 *) &trans, 2, (f32 *) mtx);
        Gs_SetState(RS_COLOR2, bird->color);
        Effect_DrawModelAdd(set, mtx, mats);
    }
}

/* Spawns a flock (type 0 in AREA_HUB_0x83, 1 in AREA_HUB_0x89; the flock's
 * code doesn't read it). Does nothing when no stage object is free. */
void BirdFlock_Spawn(s8 type)
{
    StageObj *obj;

    obj = StageObj_Alloc(1);
    if (obj != NULL) {
        obj->kind = LOBBY_OBJ_BIRD_FLOCK;
        obj->update = BirdFlock_Update;
        obj->draw = NULL;
        obj->type = type;
    }
}

/* The birds' tints (RS_COLOR2; the last is none), the animations' (part,
 * frames) pair lists, written as words, the animations (enum
 * FlockBirdAnimId) and the spots. */
s32 gFlockBirdColors[4] = {
#include "lobby/007DD170.inc"
};
u32 gFlockBirdFoldParts[4] = {
#include "lobby/007DD180.inc"
};
u32 gFlockBirdSpreadParts[4] = {
#include "lobby/007DD190.inc"
};
u32 gFlockBirdFlyParts[4] = {
#include "lobby/007DD1A0.inc"
};
u32 gFlockBirdFlapParts[4] = {
#include "lobby/007DD1B0.inc"
};
FlockBirdAnim gFlockBirdAnims[6] = {
#include "lobby/007DD1C0.inc"
};
Vec3f gFlockBirdSpots[FLOCK_BIRD_SPOTS] = {
#include "lobby/007DD1F0.inc"
};
