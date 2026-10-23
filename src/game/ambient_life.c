/*
 * AmbientLife, the area objects of kind 9: the small animals and the leaves
 * that move about some areas.
 *
 * Area_StartObjects makes one spawner (type 0) for an area with an ambient
 * set (AmbientLife_Create). The spawner makes the area's lasting children
 * when it is set up and tops them up every frame, keeping up to 32 in its
 * list; Area_Update also makes type 8 children with no spawner at the
 * bug-net gathering points (AmbientLife_CreateAt). Each child's type is a
 * kind of mover: a falling leaf, a wanderer, a darter, a steady flyer, a
 * butterfly, a spinner, a circling bird, a glow bug or a crossing flyer.
 *
 * AmbientLife_Update switches on Obj.state to AmbientLife_Setup, _Run, _End
 * and _Free, and AmbientLife_Setup, AmbientLife_Run and AmbientLife_Draw call
 * the type's own Setup, Run and Draw by Obj.type. The movers keep their
 * orientation and speed in an AmbientLifeMover; the butterfly has a draw
 * slot for each wing. The models are the area's ambient models.
 */

#include "common.h"
#include "render_state.h"
#include "obj.h"
#include "capsule.h"
#include "sprite.h"
#include "scene.h"
#include "stageobj.h"
#include "area.h"
#include "ambient_life.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "rand.h"
#include "text.h"
#include "area_draw.h"
#include "vec_math.h"

/* The buffer of AmbientLife's movers (falling leaf, wanderer, darter, steady flyer, spinner,
 * circling bird): m is the orientation they draw with, v the velocity in their own frame, rot the
 * angles; the wanderer uses spinX as its pitching mode and spinZ as that mode's frames, the darter
 * both as frame counters. */
typedef struct AmbientLifeMover {
    f32 m[16];                  /* 0x00: the orientation */
    f32 v[3];                   /* 0x40: the velocity, in the object's frame */
    u16 rot[3];                 /* 0x4C: the angles about x, y and z */
    s16 spinX;                  /* 0x52 */
    s16 circleStep;             /* 0x54: the circling bird's turn a frame */
    s16 spinZ;                  /* 0x56 */
} AmbientLifeMover;

extern f32 gCamMatrix[];
/* The buffer of AmbientLife's butterflies (AmbientLife_SetupButterfly sets it up,
 * AmbientLife_RunButterfly moves it): a draw slot per wing; pitch is the wing angle, goal the angle
 * it beats to. */
typedef struct ButterflyBuf {
    s16 slot[2];
    f32 speed;                  /* 0x04: forward */
    f32 side;                   /* 0x08 */
    u16 heading;                /* 0x0C */
    s16 pitch;                  /* 0x0E */
    u16 goal;                   /* 0x10: the pitch it turns to */
    s16 steps;                  /* 0x12: the frames left to reach it */
    DrawSlot *ds[2];
} ButterflyBuf;

extern f32 AmbientLife_WandererPoints[];
/* The y and z of the wanderers' points: AmbientLife_SetupWanderer compiles as the game's code only with them as symbols of their own. */
extern f32 AmbientLife_WandererPointsY[];

extern f32 AmbientLife_WandererPointsZ[];
extern f32 gStartPos[];
typedef struct AmbientLifeChildren {
    Obj *obj[32];
    s16 count[16];
} AmbientLifeChildren;

extern f32 D_008B0A14[];
extern f32 D_008B0A18[];
extern f32 D_008B0A94[];
extern f32 D_008B0A98[];
f32 AmbientLife_WandererPoints[12] = {
#include "game/008B09E0.inc"
};

f32 AmbientLife_GlowBugTowerPos[32] = {
#include "game/008B0A10.inc"
};

f32 AmbientLife_CrossingFlyerPoints[6] = {
#include "game/008B0A90.inc"
};

/*
 * Makes the area's AmbientLife spawner (type 0); Area_StartObjects calls it
 * for an area with an ambient set and passes the set, which is unused.
 */
void AmbientLife_Create(u8 unusedSet)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 9;
        obj->type = 0;
        obj->var4 = 0;
        obj->update = AmbientLife_Update;
    }
}

/*
 * Allocates an AmbientLife object of type childType: Obj_Alloc(0) for types
 * 8, 10 and 11, Obj_Alloc(1) for the others.
 */
void *AmbientLife_Alloc(s16 childType)
{
    switch (childType) {
    case 8:
    case 10:
    case 11:
        return Obj_Alloc(0);
    default:
        return Obj_Alloc(1);
    }
}

/*
 * Makes a child of type childType for the spawner parent in the first free
 * entry of its list of 32, with index in Obj.var4, and counts it (per type,
 * and all of them in count[0]); nothing when the list is full or no object is
 * free.
 */
void AmbientLife_SpawnChild(Obj *parent, s16 childType, u8 index)
{
    AmbientLifeChildren *children = parent->buf;
    s16 freeIdx;
    Obj *child;

    freeIdx = AmbientLife_FindFreeChild(parent);
    if (freeIdx != -1) {
        child = AmbientLife_Alloc(childType);
        if (child != NULL) {
            child->kind = 9;
            child->type = childType;
            child->update = AmbientLife_Update;
            child->var4 = index;
            children->obj[freeIdx] = child;
            children->count[childType]++;
            children->count[0]++;
        }
    }
}

/*
 * As AmbientLife_SpawnChild, but the third argument goes to Obj.var3
 * (Obj.var4 is 0): the number of points a type 11 child moves between.
 */
void AmbientLife_SpawnChildWithPoints(Obj *parent, s16 childType, s16 pointCount)
{
    AmbientLifeChildren *children = parent->buf;
    s16 freeIdx;
    Obj *child;

    freeIdx = AmbientLife_FindFreeChild(parent);
    if (freeIdx != -1) {
        child = AmbientLife_Alloc(childType);
        if (child != NULL) {
            child->kind = 9;
            child->type = childType;
            child->update = AmbientLife_Update;
            child->var3 = pointCount;
            child->var4 = 0;
            children->obj[freeIdx] = child;
            children->count[childType]++;
            children->count[0]++;
        }
    }
}

/*
 * Returns the first free entry of the spawner parent's list of 32 children,
 * or -1.
 */
s16 AmbientLife_FindFreeChild(Obj *parent)
{
    Obj **entry = parent->buf;
    s16 i;

    for (i = 0; i < 32; i++) {
        if (*entry == NULL) {
            return i;
        }
        entry++;
    }
    return -1;
}

/*
 * Makes an AmbientLife object of type childType at spot with no spawner
 * (Obj.var4 255); Area_Update makes type 8 this way at the bug-net gathering
 * points, offline, every 128 frames.
 */
void AmbientLife_CreateAt(void *spot, s16 childType)
{
    Obj *obj = AmbientLife_Alloc(childType);

    if (obj != NULL) {
        obj->kind = 9;
        obj->type = childType;
        obj->update = AmbientLife_Update;
        Vec_Copy((f32 *)obj->pos, (f32 *)spot);
        obj->var4 = 255;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AmbientLife_Setup, _Run, _End and _Free.
 */
void AmbientLife_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AmbientLife_Setup(obj);
        break;
    case 1:
        AmbientLife_Run(obj);
        break;
    case 2:
        AmbientLife_End(obj);
        break;
    case 3:
        AmbientLife_Free(obj);
        break;
    }
}

/*
 * Sets an object up by its type (AmbientLife_Setup*); every type but the
 * spawner and the butterfly then takes a draw slot (AmbientLife_Draw), or
 * frees itself without one.
 */
void AmbientLife_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    switch (obj->type) {
    case 0:
        AmbientLife_SetupSpawner(obj);
        break;
    case 1:
        AmbientLife_SetupFallingLeaf(obj);
        break;
    case 2:
        AmbientLife_SetupWanderer(obj);
        break;
    case 3:
        AmbientLife_SetupDarter(obj);
        break;
    case 4:
        AmbientLife_SetupSteadyFlyer(obj);
        break;
    case 5:
        AmbientLife_SetupButterfly(obj);
        break;
    case 6:
        AmbientLife_SetupSpinner(obj);
        break;
    case 7:
        AmbientLife_SetupCirclingBird(obj);
        break;
    case 8:
        AmbientLife_SetupGlowBug(obj);
        break;
    case 9:
        AmbientLife_SetupUnused();
        break;
    case 10:
        AmbientLife_SetupUnusedNext();
        break;
    case 11:
        AmbientLife_SetupCrossingFlyer(obj);
        break;
    }
    if (obj->type == 0 || obj->type == 5) {
        return;
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        if (obj->ds != NULL) {
            obj->ds->owner = obj;
            obj->ds->draw = AmbientLife_Draw;
        }
    } else {
        AmbientLife_Free(obj);
    }
}

/* Runs the object by its type. */
void AmbientLife_Run(Obj *obj)
{
    switch (obj->type) {
    case 0:
        AmbientLife_RunSpawner(obj);
        return;
    case 1:
        AmbientLife_RunFallingLeaf(obj);
        return;
    case 2:
        AmbientLife_RunWanderer(obj);
        return;
    case 3:
        AmbientLife_RunDarter(obj);
        return;
    case 4:
        AmbientLife_RunSteadyFlyer(obj);
        return;
    case 5:
        AmbientLife_RunButterfly(obj);
        return;
    case 6:
        AmbientLife_RunSpinner(obj);
        return;
    case 7:
        AmbientLife_RunCirclingFlyer(obj);
        return;
    case 8:
        AmbientLife_RunGlowBug(obj);
        return;
    case 9:
        AmbientLife_RunUnused();
        return;
    case 10:
        AmbientLife_RunUnusedNext();
        return;
    case 11:
        AmbientLife_RunCrossingFlyer(obj);
        
    default:
        return;
    }
}

typedef struct ButterflySlotsView { s16 id[2]; u8 unk4[20 - 4]; s32 h[2]; } ButterflySlotsView;

/*
 * Hides the object and frees its draw slot, or a butterfly's two (read
 * through ButterflySlotsView); it is freed on the next update.
 */
void AmbientLife_End(Obj *obj)
{
    ButterflySlotsView *slots = obj->buf;
    s16 i;

    obj->state++;
    obj->visible = 0;
    if (obj->type != 5) {
        if (obj->ds != NULL) {
            DrawSlot_Free(obj->slot);
        }
    } else {
        for (i = 0; i < 2; i++) {
            if (slots->h[i] != 0) {
                DrawSlot_Free(slots->id[i]);
            }
        }
    }
}

/* Frees the object (Obj_Free). */
void AmbientLife_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* Draws the object by its type, read as a byte of the owner (Obj.type). */
void AmbientLife_Draw(DrawSlot *drawSlot)
{
    u8 type;

    type = ((u8 *) drawSlot->owner)[3];
    switch (type) {
    case 1:
        AmbientLife_DrawFallingLeaf(drawSlot);
        break;
    case 2:
        AmbientLife_DrawWanderer(drawSlot);
        break;
    case 3:
        AmbientLife_DrawDarter(drawSlot);
        break;
    case 4:
        AmbientLife_DrawSteadyFlyer(drawSlot);
        break;
    case 5:
        AmbientLife_DrawButterfly(drawSlot);
        break;
    case 6:
        AmbientLife_DrawSpinner(drawSlot);
        break;
    case 7:
        AmbientLife_DrawCirclingFlyer(drawSlot);
        break;
    case 8:
        AmbientLife_DrawGlowBug(drawSlot);
        break;
    case 9:
        AmbientLife_DrawUnused();
        break;
    case 10:
        AmbientLife_DrawUnusedNext();
        break;
    case 11:
        AmbientLife_DrawCrossingFlyer(drawSlot);
        
    case 0:
    default:
        break;
    }
}

/*
 * Draws model set with the matrix at address mtx (render slot 0) when it has
 * a model, then restores the default draw state.
 */
void AmbientLife_DrawModel(ModelMatSet *set, s32 mtx)
{
    Gs_SetState(RS_MTX_SLOT(0), mtx);
    if ((set != NULL) && (set->id != -1)) {
        Draw_ApplyStates(set->flags);
        Model_Draw(set->id, 0);
    }
    Draw_SetDefaultState();
}

/*
 * Clears the spawner's list and counts, then makes the area's lasting
 * children: four circling birds in the forest's area 6, the great arena's
 * area 1 and the desert's areas 2, 8 and 9; a spinner in the forest's area 7;
 * ten type 8 children in the tower's area 8.
 */
void AmbientLife_SetupSpawner(Obj *spawner)
{
    AmbientLifeChildren *children = spawner->buf;
    s16 i;

    spawner->timer = 0;
    for (i = 0; i < 32; i++) {
        children->obj[i] = NULL;
    }
    children->count[0] = 0;
    for (i = 0; i < 11; i++) {
        children->count[i + 1] = 0;
    }
    switch (gScene.area) {
    case AREA_FOREST_HILLS_6:
    case AREA_GREAT_ARENA_1:
    case AREA_DESERT_2:
    case AREA_DESERT_8:
    case AREA_DESERT_9:
        if (children->count[0] < 32) {
            for (i = 0; i < 4; i++) {
                if (children->count[7] < 4) {
                    AmbientLife_SpawnChild(spawner, 7, i);
                }
            }
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (children->count[0] < 32 && children->count[6] <= 0) {
            AmbientLife_SpawnChild(spawner, 6, 0);
        }
        break;
    case AREA_TOWER_8:
        if (children->count[0] < 32) {
            for (i = 0; i < 10; i++) {
                AmbientLife_SpawnChild(spawner, 8, i);
            }
        }
        break;
    }
}

/*
 * Tops the area's children up every frame: two butterflies in the snowy
 * mountains' area 9; one or two type 11 children every 256 frames in the
 * forest's area 3 (up to two); up to two type 8 children in its area 9 and
 * two steady flyers in its area 5; in its area 7 a falling leaf every 60
 * frames, three darters, five steady flyers and two butterflies. Then forgets
 * the children that ended (state 2 or more).
 */
void AmbientLife_RunSpawner(Obj *spawner)
{
    AmbientLifeChildren *children = spawner->buf;
    s16 i;
    s32 n;
    Obj *child;

    spawner->timer++;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        if (children->count[0] < 32 && children->count[5] < 2) {
            AmbientLife_SpawnChild(spawner, 5, 0);
        }
        break;
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        if (*(u8 *) &spawner->timer == 0 && children->count[0] < 32 && children->count[11] < 2) {
            n = (s16) (Rand_Next(1) & 1) + 1;
            for (i = 0; i < n; i++) {
                AmbientLife_SpawnChildWithPoints(spawner, 11, 2);
                if (children->count[0] >= 32) {
                    break;
                }
            }
        }
        break;
    case AREA_FOREST_HILLS_9:
    case AREA_FOREST_HILLS_NIGHT_9:
        if (children->count[0] < 32 && children->count[8] < 2) {
            AmbientLife_SpawnChild(spawner, 8, 0);
        }
        break;
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        if (children->count[0] < 32 && children->count[4] < 2) {
            AmbientLife_SpawnChild(spawner, 4, 0);
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (spawner->timer >= 60 && children->count[0] < 32) {
            AmbientLife_SpawnChild(spawner, 1, 0);
            spawner->timer = 0;
        }
        if (children->count[0] < 32 && children->count[3] < 3) {
            AmbientLife_SpawnChild(spawner, 3, 0);
        }
        if (children->count[0] < 32 && children->count[4] < 5) {
            AmbientLife_SpawnChild(spawner, 4, 0);
        }
        if (children->count[0] < 32 && children->count[5] < 2) {
            AmbientLife_SpawnChild(spawner, 5, 0);
        }
        break;
    }
    for (i = 0; i < 32; i++) {
        child = children->obj[i];
        if (child != NULL && child->state >= 2) {
            children->count[child->type]--;
            children->obj[i] = NULL;
            children->count[0]--;
        }
    }
}

/*
 * A falling leaf: one of four texture variants (Obj.var0), a random place
 * 3000 up in the forest's area 7's box, a random orientation and no speed.
 */
void AmbientLife_SetupFallingLeaf(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    obj->var0 = Rand_Next(1) & 3;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 3000.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    }
    obj->timer = 0;
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 0.0f;
    mover->rot[0] = Rand_Next(1);
    mover->rot[1] = Rand_Next(1);
    mover->rot[2] = Rand_Next(1);
    mover->spinX = 0;
    mover->spinZ = 0;
}

/*
 * Tumbles the leaf with damped random spins and lets it slide down along its
 * own tilt, moving it in its own frame; it ends below height 0.
 */
void AmbientLife_RunFallingLeaf(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    mover->spinX = 0.95f * mover->spinX;
    mover->spinZ = 0.975f * mover->spinZ;
    mover->spinX += (s16) ((Rand_Next(1) & 0xFF) - 128);
    mover->spinZ += (s16) ((Rand_Next(1) & 0x3F) - 32);
    mover->rot[0] += mover->spinX;
    mover->rot[1] += (u16) ((Rand_Next(1) & 0x1F) - 15);
    mover->rot[2] += mover->spinZ;
    Mtx_Identity(mover->m);
    Mtx_RotateX(mover->m, 6.2831855f * mover->rot[0] / 65536.0f);
    Mtx_TurnY(mover->m, 6.2831855f * mover->rot[1] / 65536.0f);
    Mtx_TurnZ(mover->m, 6.2831855f * mover->rot[2] / 65536.0f);
    mover->v[0] += -2.0f * mover->m[1];
    mover->v[1] += -2.0f * mover->m[5];
    mover->v[2] += -2.0f * mover->m[9];
    mover->v[0] *= 0.6f;
    mover->v[1] *= 0.25f;
    mover->v[2] *= 0.7f;
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    if (!(0.0f <= obj->pos[1])) {
        obj->state++;
    } else {
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
    }
}

/*
 * Draws the leaf with its orientation (the area's ambient model 2), its
 * texture moved to its variant.
 */
void AmbientLife_DrawFallingLeaf(DrawSlot *drawSlot)
{
    f32 *orient;
    ModelMatSet *set;
    f32 uvMtx[16];
    f32 variantU;
    s16 idx;
    u8 variant;
    Model *subModels;
    Obj *w;

    w = drawSlot->owner;
    subModels = gAreaSubModels[0];
    orient = w->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][2];
        if (idx >= 0) {
            variant = w->var0;
            set = &subModels->matSets[idx];
            variantU = (f32) (u32) variant;
            Mtx_Translation(uvMtx, 0.25f * variantU, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
        }
    }
}

/*
 * A wanderer: starts at one of four points (AmbientLife_WandererPoints)
 * heading for another, its nose about level, flying 6 to 8 units a frame.
 */
void AmbientLife_SetupWanderer(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    u8 goal;
    f32 toGoalX;
    f32 toGoalZ;

    obj->var0 = Rand_Next(1) % 4;
    goal = Rand_Next(1) % 3;
    if (goal >= obj->var0) {
        goal++;
    }
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    obj->pos[0] = AmbientLife_WandererPoints[obj->var0 * 3];
    obj->pos[1] = AmbientLife_WandererPointsY[obj->var0 * 3];
    obj->pos[2] = AmbientLife_WandererPointsZ[obj->var0 * 3];
    toGoalX = AmbientLife_WandererPoints[goal * 3] - obj->pos[0];
    toGoalZ = AmbientLife_WandererPointsZ[goal * 3] - obj->pos[2];
    mover->rot[0] = (u8) Rand_Next(1) - 239;
    mover->rot[1] = (s32) (65536.0f * Math_Atan2(toGoalX, toGoalZ) / 6.2831855f + 0.5f);
    mover->rot[2] = 0;
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 6.0f + 0.002f * (Rand_Next(1) & 0x3FF);
    mover->spinX = 0;
    mover->spinZ = 0;
}

/*
 * Flies the wanderer along its heading for 500 frames: random turns of 10 to
 * 25 frames, random pitching (96 a frame, down to -0x1000) and sharp turns
 * while its pitch is negative; spinX and spinZ serve as that mode and its
 * frames.
 */
void AmbientLife_RunWanderer(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    obj->timer++;
    if (obj->timer >= 500) {
        obj->state++;
        return;
    }
    switch (obj->var1) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        obj->var1 = Rand_Next(1) & 7;
        if (obj->var1 > 5) {
            obj->var2 = (Rand_Next(1) & 0xF) + 10;
        }
        break;
    case 6:
        obj->var2--;
        mover->rot[1] -= 32;
        if (obj->var2 <= 0) {
            obj->var1 = 0;
        }
        break;
    case 7:
        obj->var2--;
        mover->rot[1] += 32;
        if (obj->var2 <= 0) {
            obj->var1 = 0;
        }
        break;
    }
    switch (mover->spinX) {
    case 0:
    case 1:
        if (--mover->spinZ < 0) {
            mover->spinX = Rand_Next(1) & 3;
            if (mover->spinX > 1) {
                mover->spinZ = (Rand_Next(1) & 7) + 3;
            } else {
                mover->spinZ = 0;
            }
        }
        break;
    case 2:
        if (mover->rot[0] > -0x1000) {
            mover->rot[0] -= 96;
        }
        mover->spinX = 0;
        break;
    case 3:
        if (mover->rot[0] < 0) {
            mover->rot[1] += 0x600;
        }
        mover->spinX = 0;
        break;
    }
    Mtx_Identity(mover->m);
    Mtx_RotateXYZ(mover->m, 6.2831855f * mover->rot[0] / 65536.0f, 6.2831855f * mover->rot[1] / 65536.0f,
                  6.2831855f * mover->rot[2] / 65536.0f);
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/*
 * Draws the wanderer with its orientation (the area's ambient model 3)
 * without alpha test.
 */
void AmbientLife_DrawWanderer(DrawSlot *drawSlot)
{
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;

    subModels = gAreaSubModels[0];
    orient = ((Obj *) drawSlot->owner)->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][3];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Gs_SetState(RS_ALPHA_REF, 0);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
            Gs_SetState(RS_ALPHA_REF, 128);
        }
    }
}

/*
 * A darter: a random place 300 up in the forest's area 7's box with a random
 * heading, creeping forward at 0.5 a frame.
 */
void AmbientLife_SetupDarter(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 300.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    }
    mover->rot[1] = Rand_Next(1);
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 0.5f;
    mover->spinX = Rand_Next(1) & 0x1F;
    mover->spinZ = 0;
}

/*
 * Moves the darter in bursts: it waits, speeds up toward 10, cruises, then
 * cuts its speed by half or more (Obj.var0, spinX the frames); turns 0x180 or
 * 0x200 a frame for random spells (Obj.var1, Obj.timer) and climbs or sinks
 * (not below 250) for others (Obj.var2, spinZ). It ends once out of the
 * forest's area 7's box.
 */
void AmbientLife_RunDarter(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (!(obj->pos[0] >= 9773.0f && obj->pos[0] <= 11174.0f && obj->pos[1] <= 2000.0f && obj->pos[2] >= 9173.0f &&
              obj->pos[2] <= 10576.0f)) {
            obj->state++;
            return;
        }
        break;
    }
    switch (obj->var0) {
    case 0:
        if (--mover->spinX <= 0) {
            obj->var0++;
            mover->spinX = Rand_Next(1) % 6 + 2;
        }
        break;
    case 1:
        if (mover->v[2] < 10.0f) {
            if ((s16) (Rand_Next(1) & 1) == 0) {
                mover->v[2] += 0.5f;
            } else {
                mover->v[2] += 0.3f;
            }
        }
        if (--mover->spinX <= 0) {
            obj->var0++;
            mover->spinX = Rand_Next(1) % 8 + 3;
        }
        break;
    case 2:
        if (--mover->spinX <= 0) {
            obj->var0++;
            mover->spinX = (Rand_Next(1) & 4) + 2;
        }
        break;
    case 3:
        if ((s16) (Rand_Next(1) & 1) == 0) {
            mover->v[2] *= 0.5f;
        } else {
            mover->v[2] *= 0.4f;
        }
        if (--mover->spinX <= 0) {
            obj->var0 = 0;
            mover->spinX = Rand_Next(1) % 20 + 7;
        }
        break;
    }
    switch (obj->var1) {
    case 0:
        obj->timer = Rand_Next(1) % 20 + 5;
        obj->var1 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var1 = 0;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] -= 0x180;
        break;
    case 3:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] += 0x180;
        break;
    case 4:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] -= 0x200;
        break;
    case 5:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] += 0x200;
        break;
    }
    switch (obj->var2) {
    case 0:
        mover->spinZ = (Rand_Next(1) & 0xF) + 5;
        obj->var2 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--mover->spinZ <= 0) {
            obj->var2 = 0;
        }
        break;
    case 2:
        if (--mover->spinZ <= 0) {
            mover->spinZ = (Rand_Next(1) & 0xF) + 5;
            obj->var2 = 1;
        }
        obj->pos[1] += 1.0f;
        break;
    case 3:
        if (--mover->spinZ <= 0) {
            mover->spinZ = (Rand_Next(1) & 0xF) + 5;
            obj->var2 = 1;
        }
        if (obj->pos[1] > 250.0f) {
            obj->pos[1] -= 1.0f;
        }
        break;
    }
    Mtx_Identity(mover->m);
    Mtx_RotateY(mover->m, 6.2831855f * mover->rot[1] / 65536.0f);
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/*
 * Draws the darter facing its heading (the area's ambient model 4) without
 * alpha test.
 */
void AmbientLife_DrawDarter(DrawSlot *drawSlot)
{
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;

    subModels = gAreaSubModels[0];
    orient = ((Obj *) drawSlot->owner)->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][4];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Gs_SetState(RS_ALPHA_REF, 0);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
            Gs_SetState(RS_ALPHA_REF, 128);
        }
    }
}

/*
 * A steady flyer: a random place 450 up in the forest's area 5's box or 150
 * up in its area 7's, flying 5 units a frame.
 */
void AmbientLife_SetupSteadyFlyer(Obj *obj)
{
    u16 area;
    AmbientLifeMover *mover;

    mover = obj->buf;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    area = gScene.area;
    switch (area) {
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        obj->pos[0] = 10750.0f + (f32) (Rand_Next(1) & 0xFF);
        obj->pos[1] = 450.0f;
        obj->pos[2] = 11700.0f + (f32) (Rand_Next(1) & 0xFF);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0xFFFF & 0x7FF);
        obj->pos[1] = 150.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
        break;
    }
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 5.0f;
}

/*
 * Flies it at a steady speed with random turns (0x120 a frame) and climbs or
 * sinks (not below 50); it ends once out of its area's box.
 */
void AmbientLife_RunSteadyFlyer(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    switch (gScene.area) {
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        if (obj->pos[0] < 10450.0f || obj->pos[0] > 11850.0f || obj->pos[1] > 1500.0f || obj->pos[2] < 11400.0f ||
            obj->pos[2] > 12500.0f) {
            obj->state++;
            return;
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (obj->pos[0] < 9773.0f || obj->pos[0] > 11174.0f || obj->pos[1] > 2000.0f || obj->pos[2] < 9173.0f ||
            obj->pos[2] > 10576.0f) {
            obj->state++;
            return;
        }
        break;
    }
    switch (obj->var0) {
    case 0:
        obj->timer = Rand_Next(1) % 40 + 10;
        obj->var0 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var0 = 0;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            obj->var0 = 1;
        }
        mover->rot[1] -= 0x120;
        break;
    case 3:
        if (--obj->timer <= 0) {
            obj->var0 = 1;
        }
        mover->rot[1] += 0x120;
        break;
    }
    switch (obj->var1) {
    case 0:
        obj->var2 = (Rand_Next(1) & 0xF) + 5;
        obj->var1 = Rand_Next(1) % 2 + 1;
        break;
    case 1:
        if (--obj->var2 <= 0) {
            obj->var1 = 0;
        }
        break;
    case 2:
        if (--obj->var2 <= 0) {
            obj->var2 = (Rand_Next(1) & 0xF) + 5;
            obj->var1 = 1;
        }
        obj->pos[1] += 1.0f;
        break;
    case 3:
        if (--obj->var2 <= 0) {
            obj->var2 = (Rand_Next(1) & 0xF) + 5;
            obj->var1 = 1;
        }
        if (obj->pos[1] > 50.0f) {
            obj->pos[1] -= 1.0f;
        }
        break;
    }
    Mtx_Identity(mover->m);
    Mtx_RotateY(mover->m, 6.2831855f * mover->rot[1] / 65536.0f);
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/* Draws the steady flyer facing its heading (the area's ambient model 5). */
void AmbientLife_DrawSteadyFlyer(DrawSlot *drawSlot)
{
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;

    subModels = gAreaSubModels[0];
    orient = ((Obj *) drawSlot->owner)->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][5];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
        }
    }
}

/*
 * A butterfly: a random place 150 up in the snowy mountains' area 9's or the
 * forest's area 7's box (elsewhere a default box, with a random heading), at
 * rest with a random wing angle, and a draw slot for each wing.
 */
void AmbientLife_SetupButterfly(Obj *obj)
{
    ButterflyBuf *butterfly = obj->buf;
    s16 i;

    obj->var0 = 0;
    obj->var1 = 0;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        obj->pos[0] = 6500.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 150.0f;
        obj->pos[2] = 12750.0f + (f32) (Rand_Next(1) & 0x7FF);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 150.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    default:
        obj->pos[0] = (Rand_Next(1) & 0x7FF) + 1500;
        obj->pos[1] = (Rand_Next(1) & 0xF) + 30;
        obj->pos[2] = (Rand_Next(1) & 0xFFF) + 2000;
        butterfly->heading = Rand_Next(1);
        break;
    }
    obj->timer = 0;
    butterfly->speed = 0.0f;
    butterfly->side = 0.0f;
    butterfly->pitch = Rand_Next(1) % (ANGLE_TURN * 3 / 8) - ANGLE_TURN * 3 / 16;
    for (i = 0; i < 2; i++) {
        butterfly->slot[i] = DrawSlot_Alloc();
        if (butterfly->slot[i] != -1) {
            butterfly->ds[i] = DrawSlot_Get(butterfly->slot[i]);
            butterfly->ds[i]->owner = obj;
            butterfly->ds[i]->part = i;
            butterfly->ds[i]->draw = AmbientLife_Draw;
        } else {
            butterfly->ds[i] = NULL;
        }
    }
}

/*
 * Flutters the butterfly: random changes of speed (up to 4) and of sideways
 * drift, wing beats to a random angle on the other side (each downbeat
 * lifting it), random turns (0x180 a frame) and a sink by the wing angle;
 * both wings' slots follow it. It ends once out of its area's box.
 */
void AmbientLife_RunButterfly(Obj *obj)
{
    ButterflyBuf *butterfly = obj->buf;
    f32 speedScale;
    s16 i;
    s16 choice;
    s16 swing;
    s16 pitchStep;
    f32 *stepY;
    f32 *stepZ;
    f32 step[3];

    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        if (!(obj->pos[0] >= 6500.0f && obj->pos[0] <= 9000.0f && obj->pos[1] <= 2000.0f && obj->pos[1] >= -100.0f &&
              obj->pos[2] >= 12750.0f && obj->pos[2] <= 15000.0f)) {
            obj->state++;
            return;
        }
        speedScale = 1.0f;
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (!(obj->pos[0] >= 9773.0f && obj->pos[0] <= 11174.0f && obj->pos[1] <= 2000.0f && obj->pos[1] >= -100.0f &&
              obj->pos[2] >= 9173.0f && obj->pos[2] <= 10576.0f)) {
            obj->state++;
            return;
        }
        speedScale = 1.0f;
        break;
    default:
        if (!(obj->pos[0] >= 1500.0f && obj->pos[0] <= 4000.0f && obj->pos[1] <= 2000.0f && obj->pos[1] >= -100.0f &&
              obj->pos[2] >= 3000.0f && obj->pos[2] <= 7000.0f)) {
            obj->state++;
            return;
        }
        speedScale = 1.0f;
        break;
    }
    choice = Rand_Next(1) % 5;
    switch (choice) {
    case 0:
        break;
    case 1:
        if (butterfly->speed < 4.0f * speedScale) {
            butterfly->speed += 0.15f;
        }
        break;
    case 2:
        butterfly->speed -= 0.15f * speedScale;
        if (butterfly->speed < 0.0f) {
            butterfly->speed = 0.0f;
        }
        break;
    case 3:
        if (butterfly->speed < 4.0f * speedScale) {
            butterfly->speed += 0.2f;
        }
        break;
    case 4:
        butterfly->speed *= 0.5f;
        break;
    }
    choice = Rand_Next(1) % 3;
    switch (choice) {
    case 0:
        break;
    case 1:
        if (butterfly->side < 0.8f * speedScale) {
            butterfly->side += 0.05f;
        }
        break;
    case 2:
        if (butterfly->side > 0.8f * -speedScale) {
            butterfly->side -= 0.05f;
        }
        break;
    }
    stepY = &step[1];
    stepZ = &step[2];
    step[0] = butterfly->side;
    *stepY = 0.0f;
    *stepZ = butterfly->speed;
    switch (obj->var0) {
    case 0:
        obj->var0++;
        swing = Rand_Next(1) % 6144;
        butterfly->steps = (s16) (2.0f * speedScale) + Rand_Next(1) % 3;
        if (obj->pos[1] < 200.0f) {
            butterfly->steps--;
        } else if (obj->pos[1] > 300.0f) {
            butterfly->steps++;
        }
        if (butterfly->pitch >= 0) {
            butterfly->goal = -8192 - swing;
        } else {
            butterfly->goal = swing + 8192;
        }
        break;
    case 1:
        pitchStep = (s16) (butterfly->goal - butterfly->pitch) / butterfly->steps;
        if (pitchStep < 0) {
            obj->pos[1] -= 0.6f * ((1.5f + speedScale) * ((pitchStep - 0x300) / 1600.0f));
        }
        *(u16 *) &butterfly->pitch += pitchStep;
        butterfly->steps--;
        if (butterfly->steps <= 0) {
            obj->var0++;
        }
        break;
    case 2:
        obj->var0 = 0;
        break;
    }
    switch (obj->var1) {
    case 0:
        obj->timer = Rand_Next(1) % 20 + 10;
        obj->var1 = Rand_Next(1) % 3 + 1;
    case 1:
        if (--obj->timer <= 0) {
            obj->var1 = 0;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            obj->var1 = 1;
        }
        butterfly->heading -= 0x180;
        break;
    case 3:
        if (--obj->timer <= 0) {
            obj->var1 = 1;
        }
        butterfly->heading += 0x180;
        break;
    }
    obj->pos[1] += -2.9f * (((s16) Math_Absf(butterfly->pitch) + ANGLE_TURN / 16) / 15000.0f);
    Vec_RotateY(step, 6.2831855f * butterfly->heading / 65536.0f);
    obj->pos[0] += step[0];
    obj->pos[1] += *stepY;
    obj->pos[2] += *stepZ;
    for (i = 0; i < 2; i++) {
        if (butterfly->ds[i] != NULL) {
            butterfly->ds[i]->pos[0] = obj->pos[0];
            butterfly->ds[i]->pos[1] = obj->pos[1];
            butterfly->ds[i]->pos[2] = obj->pos[2];
            OT_AddByDepth(gDrawOt32, butterfly->ds[i], 32, 0);
        }
    }
}

/*
 * Draws one wing (the area's ambient model 6 plus the wing), turned about the
 * body by the wing angle (the second wing mirrored) and then to the heading.
 */
void AmbientLife_DrawButterfly(DrawSlot *drawSlot)
{
    ButterflyBuf *butterfly = ((Obj *) drawSlot->owner)->buf;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *model;
    f32 mtx[16];
    s16 modelIdx;

    if (stage != NULL && stage->active != 0) {
        modelIdx = gAreaFxModels[gScene.area][6];
        if (modelIdx >= 0) {
            model = &stage->matSets[modelIdx] + drawSlot->part;
            Mtx_Identity(mtx);
            if (drawSlot->part == 0) {
                Mtx_RotateZ(mtx, (6.2831855f * (u16) butterfly->pitch) / 65536.0f);
            } else {
                Mtx_RotateZ(mtx, (6.2831855f * -(u16) butterfly->pitch) / 65536.0f);
            }
            Mtx_RotateY(mtx, (6.2831855f * butterfly->heading) / 65536.0f);
            Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(model, (s32) mtx);
        }
    }
}

/*
 * A spinner: 100 above the area's start position (gStartPos), with a small
 * random turn per frame about each axis.
 */
void AmbientLife_SetupSpinner(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    obj->pos[0] = gStartPos[gScene.area * 3];
    obj->pos[1] = 100.0f + gStartPos[gScene.area * 3 + 1];
    obj->pos[2] = gStartPos[gScene.area * 3 + 2];
    obj->timer = 0;
    mover->rot[0] = (Rand_Next(1) & 0xF) - 8;
    mover->rot[1] = (Rand_Next(1) & 0xF) - 8;
    mover->rot[2] = (Rand_Next(1) & 0xF) - 8;
    Mtx_Identity(mover->m);
}

/*
 * Turns the spinner by its per-frame turn, steps its 10-frame texture
 * animation (Obj.timer by 2, up to 20) and adds it to the 32-entry draw list.
 */
void AmbientLife_RunSpinner(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    Mtx_RotateXYZ(mover->m, (6.2831855f * mover->rot[0]) / 65536.0f,
                  (6.2831855f * mover->rot[1]) / 65536.0f,
                  (6.2831855f * mover->rot[2]) / 65536.0f);
    obj->timer += 2;
    if (obj->timer >= 20) {
        obj->timer = 0;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/*
 * Draws the spinner with its orientation (the area's ambient model 7), its
 * texture moved to the frame.
 */
void AmbientLife_DrawSpinner(DrawSlot *drawSlot)
{
    f32 uvMtx[16];
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;
    Obj *w;

    w = drawSlot->owner;
    subModels = gAreaSubModels[0];
    orient = w->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][7];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Mtx_Translation(uvMtx, 0.05f * (f32) w->timer, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
        }
    }
}

/*
 * A circling bird: a random centre high over its area (the snowy mountains'
 * night area 9, the forest's area 6, the great arena's area 1, the desert's
 * areas 2, 8 and 9) and a radius (in the snowy area the distance to the
 * area's nearest edge, at most 6000); it starts at a random angle and goes
 * round at 33 units a frame (AmbientLifeMover.circleStep), one way or the
 * other at random (Obj.var2).
 */
void AmbientLife_SetupCirclingBird(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 radius;
    f32 edgeDist;

    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_NIGHT_9:
        obj->pos[0] = 2000.0f + (Rand_Next(1) & 0x3FFF);
        obj->pos[1] = 2000.0f;
        obj->pos[2] = 2000.0f + (Rand_Next(1) & 0x3FFF);
        radius = obj->pos[0];
        if (radius < 10000.0f) {
            radius = radius;
        } else {
            radius = 20000.0f - radius;
        }
        edgeDist = obj->pos[2];
        if (edgeDist < 10000.0f) {
            edgeDist = edgeDist;
        } else {
            edgeDist = 20000.0f - edgeDist;
        }
        if (edgeDist < radius) {
            radius = edgeDist;
        }
        if (radius > 6000.0f) {
            radius = 6000.0f;
        }
        break;
    case AREA_FOREST_HILLS_6:
        obj->pos[0] = 9737.0f + ((Rand_Next(1) & 0x3FF) - 512);
        obj->pos[1] = 8412.0f + (Rand_Next(1) & 0x3FF);
        obj->pos[2] = 6057.0f + ((Rand_Next(1) & 0x3FF) - 512);
        radius = 2000.0f + (Rand_Next(1) & 0x3FF);
        break;
    case AREA_GREAT_ARENA_1:
        obj->pos[0] = 10500.0f + ((Rand_Next(1) & 0x7FF) - 1024);
        obj->pos[1] = 3000.0f + (Rand_Next(1) & 0x3FF);
        obj->pos[2] = 9500.0f + ((Rand_Next(1) & 0x7FF) - 1024);
        radius = 2000.0f + (Rand_Next(1) & 0xFF);
        break;
    case AREA_DESERT_2:
        obj->pos[0] = 10500.0f + ((Rand_Next(1) & 0x7FF) - 1024);
        obj->pos[1] = 3000.0f + (Rand_Next(1) & 0x7FF);
        obj->pos[2] = 10000.0f + ((Rand_Next(1) & 0x7FF) - 1024);
        radius = 2000.0f + (Rand_Next(1) & 0x1FF);
        break;
    case AREA_DESERT_8:
        obj->pos[0] = 6286.0f + ((Rand_Next(1) & 0x3FF) - 512);
        obj->pos[1] = 1500.0f + (Rand_Next(1) & 0x3FF);
        obj->pos[2] = 5300.0f + ((Rand_Next(1) & 0x3FF) - 512);
        radius = 1000.0f + (Rand_Next(1) & 0xFF);
        break;
    case AREA_DESERT_9:
        obj->pos[0] = 10000.0f + ((Rand_Next(1) & 0x7FF) - 1024);
        obj->pos[1] = 2800.0f + (Rand_Next(1) & 0x7FF);
        obj->pos[2] = 9500.0f + ((Rand_Next(1) & 0x7FF) - 1024);
        radius = 3500.0f + (Rand_Next(1) & 0x1FF);
        break;
    }
    mover->circleStep = 65536.0f * (33.0f / (6.2831855f * radius));
    obj->timer = 0;
    mover->rot[1] = Rand_Next(1);
    Mtx_Translation(mover->m, 0.0f, 0.0f, radius);
    if (Rand_Next(1) & 1) {
        Mtx_RotateY(mover->m, 1.5707964f);
        obj->var2 = 0;
    } else {
        mover->circleStep = -mover->circleStep;
        Mtx_RotateY(mover->m, 4.712389f);
        obj->var2 = 1;
    }
}

/*
 * Run state of AmbientLife's circling flyer (type 7): adds its turn step
 * (AmbientLifeMover.circleStep) to its heading (AmbientLifeMover.rot[1]), places the draw slot
 * at the circle's centre (Obj.pos) plus the circle offset kept in AmbientLifeMover.m
 * turned by that heading, and adds the slot to gDrawOt32. Returns
 * OT_AddByDepth's result.
 */
u32 AmbientLife_RunCirclingFlyer(Obj *obj)
{
    u8 *tumble = obj->buf;
    f32 offset[4];

    *(u16 *) (tumble + 78) += (s32) *(u16 *) (tumble + 84);
    Mtx_GetTrans(offset, (f32 *)tumble);
    Vec_RotateY(offset, (6.2831855f * *(u16 *) (tumble + 78)) / 65536.0f);
    obj->ds->pos[0] = obj->pos[0] + offset[0];
    obj->ds->pos[1] = obj->pos[1] + offset[1];
    obj->ds->pos[2] = obj->pos[2] + offset[2];
    return OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/*
 * Draws the circling flyer: model 8 of the area's ambient set
 * (gAreaFxModels) from the area's sub-model, turned by its heading and its
 * facing (AmbientLifeMover.m), at the draw slot, banked 10 degrees about z into the
 * turn (Obj.var2 is the direction); AREA_FOREST_HILLS_6 draws it three times
 * bigger. Draws nothing without the sub-model or the model. The switch keeps
 * default first: the compiler then lays out the branches, with their empty
 * delay slots, as the game's code does.
 */
void AmbientLife_DrawCirclingFlyer(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    f32 *orient = obj->buf;
    ModelMatSet *model;
    f32 mtx[16];
    s16 k;
    u16 area;

    if (stage != NULL) {
        if (stage->active == 0) {
            return;
        }
        area = gScene.area;
        k = gAreaFxModels[area][8];
        if (k >= 0) {
            model = &stage->matSets[k];
            switch (area) {
            default:
                Mtx_Identity(mtx);
                break;
            case AREA_FOREST_HILLS_6:
                Mtx_Scale(mtx, 3.0f, 3.0f, 3.0f);
                break;
            }
            Mtx_RotateY(mtx, (6.2831855f * *(u16 *) ((u8 *) orient + 78)) / 65536.0f);
            Mtx_Concat3x3(mtx, orient);
            Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            if (obj->var2 == 0) {
                Mtx_TurnZ(mtx, -0.17453294f);
            } else {
                Mtx_TurnZ(mtx, 0.17453294f);
            }
            AmbientLife_DrawModel(model, (s32) mtx);
        }
    }
}

/*
 * Starts a life of AmbientLife's glow bug (type 8): unless it was placed at a
 * gathering point (Obj.var4 255), moves it near a fixed point of
 * AREA_FOREST_HILLS_9 and its night, or to its place Obj.var4 of
 * AmbientLife_GlowBugTowerPos in AREA_TOWER_8; then picks a life of 5 to 8
 * seconds (Obj.var3), its drift directions on x and z (Obj.var1, Obj.var2)
 * and full brightness (Obj.fvar 1.0).
 */
void AmbientLife_ResetGlowBug(Obj *obj)
{

    if (obj->var4 != 255) {
        switch (gScene.area) {
        case AREA_FOREST_HILLS_9:
        case AREA_FOREST_HILLS_NIGHT_9:
            obj->pos[0] = 11750.0f + (f32) ((Rand_Next(1) & 0x1FF) - 256);
            obj->pos[1] = 200.0f;
            obj->pos[2] = 9500.0f + (f32) ((Rand_Next(1) & 0x1FF) - 256);
            break;
        case AREA_TOWER_8:
            obj->pos[0] = AmbientLife_GlowBugTowerPos[obj->var4 * 3];
            obj->pos[1] = D_008B0A14[obj->var4 * 3];
            obj->pos[2] = D_008B0A18[obj->var4 * 3];
            break;
        }
    }
    obj->timer = 0;
    obj->var3 = ((Rand_Next(1) & 3) + 5) * 60;
    obj->var1 = Rand_Next(1) & 1;
    obj->var2 = Rand_Next(1) & 1;
    obj->fvar = 1.0f;
}

/* Set-up of the glow bug: AmbientLife_ResetGlowBug. */
void AmbientLife_SetupGlowBug(Obj *obj)
{
    AmbientLife_ResetGlowBug(obj);
}

/*
 * Run state of the glow bug: while it lives (Obj.var0 0) its brightness
 * Obj.fvar pulses (|cos| of the frame count, a turn per 64 frames) and it
 * drifts 0.2 to 1.5 a frame on x and z, rising by their mean; then it dims by
 * a tenth a frame for 10 frames and starts a new life, or ends when it came
 * from a gathering point. Adds the draw slot to gDrawOtSorted.
 */
void AmbientLife_RunGlowBug(Obj *obj)
{
    f32 x;
    f32 z;

    switch (obj->var0) {
    case 0:
        obj->timer++;
        if (obj->timer >= obj->var3) {
            obj->var0++;
            obj->timer = 10;
        } else {
            obj->fvar = Math_Absf(Math_Cos((6.2831855f * (obj->timer << 10)) / 65536.0f));
        }
        x = 0.2f + 0.02f * (Rand_Next(1) & 0x3F);
        z = 0.2f + 0.02f * (Rand_Next(1) & 0x3F);
        if (obj->var1) {
            x *= -1.0f;
        }
        if (obj->var2) {
            z *= -1.0f;
        }
        obj->pos[0] += x;
        obj->pos[2] += z;
        obj->pos[1] += (x + z) / 2.0f;
        break;
    case 1:
        if (--obj->timer <= 0) {
            if (obj->var4 == 255) {
                obj->state++;
                return;
            }
            obj->var0 = 0;
            AmbientLife_ResetGlowBug(obj);
        } else {
            obj->fvar -= obj->fvar / 10.0f;
        }
        break;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOtSorted, obj->ds, 64, 0);
}

/*
 * Draws the glow bug facing the camera: model 9 of the area's ambient set,
 * blended by Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE) without depth writes, scaled by its
 * brightness (not in AREA_TOWER_8), with alpha 255 * Obj.fvar; then restores
 * the blend and depth writes.
 */
void AmbientLife_DrawGlowBug(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *model;
    f32 mtx[16];
    s16 k;

    if (stage != NULL && stage->active != 0) {
        k = gAreaFxModels[gScene.area][9];
        if (k >= 0) {
            model = &stage->matSets[k];
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
            Gs_SetState(RS_ALPHA_REF, 0);
            Gs_SetState(RS_ZWRITE, 0);
            if (gScene.area == AREA_TOWER_8) {
                Mtx_Identity(mtx);
            } else {
                Mtx_Scale(mtx, obj->fvar, obj->fvar, obj->fvar);
            }
            Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Gs_SetState(RS_COLOR2, ((u8) (u32) (255.0f * obj->fvar) << 24) | 0xFFFFFF);
            AmbientLife_DrawModel(model, (s32) mtx);
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
            Gs_SetState(RS_ZWRITE, 1);
        }
    }
}

/* Set-up of AmbientLife's type 9, which no spawner creates: does nothing. */
void AmbientLife_SetupUnused(void)
{
}

/* Run state of the unused type 9: does nothing. */
void AmbientLife_RunUnused(void)
{
}

/* Draw of the unused type 9: does nothing. */
void AmbientLife_DrawUnused(void)
{
}

/* Set-up of AmbientLife's type 10, which no spawner creates: does nothing. */
void AmbientLife_SetupUnusedNext(void)
{
}

/* Run state of the unused type 10: does nothing. */
void AmbientLife_RunUnusedNext(void)
{
}

/* Draw of the unused type 10: does nothing. */
void AmbientLife_DrawUnusedNext(void)
{
}

/*
 * Set-up of AmbientLife's crossing flyer (type 11), which arrives with the
 * number of its points in Obj.var3 (2, from AmbientLife_SpawnChildWithPoints): picks a
 * start point (Obj.var0) and another end point (Obj.var1) of
 * AmbientLife_CrossingFlyerPoints, a bob height of 12.5 to 37.5 (Obj.fvar)
 * and a random phase (Obj.timer). In AREA_FOREST_HILLS_3 and its night it
 * starts at its point (height jittered by up to 128) heading for the end
 * point (Obj.var3). Obj.var0 then becomes half its bob period, 48 to 79
 * frames.
 */
void AmbientLife_SetupCrossingFlyer(Obj *obj)
{

    obj->var0 = Rand_Next(1) % obj->var3;
    obj->var1 = Rand_Next(1) % (obj->var3 - 1) + 1;
    obj->var1 = obj->var1 + obj->var0;
    if (obj->var1 >= obj->var3) {
        obj->var1 -= obj->var3;
    }
    obj->fvar = 25.0f * (0.5f + 0.001f * (Rand_Next(1) & 0x3FF));
    obj->var2 = 0;
    obj->timer = Rand_Next(1);
    switch (gScene.area) {
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        obj->pos[0] = AmbientLife_CrossingFlyerPoints[obj->var0 * 3];
        obj->pos[1] = D_008B0A94[obj->var0 * 3] + (f32) ((u8) Rand_Next(1) - 128);
        obj->pos[2] = D_008B0A98[obj->var0 * 3];
        obj->var3 = Math_Heading(AmbientLife_CrossingFlyerPoints[obj->var1 * 3], D_008B0A98[obj->var1 * 3], obj->pos[0], obj->pos[2]) + ANGLE_TURN / 4;
        break;
    }
    obj->var0 = (Rand_Next(1) & 0x1F) + 48;
}

/*
 * Run state of the crossing flyer: steps its wing frame (Obj.var2, 0-3); ends
 * once within 100 of its end point; else moves 10 a frame along its heading
 * and places the draw slot bobbing by Obj.fvar times a sine of the frame
 * count (a wave per 2 * Obj.var0 frames), added to gDrawOt32.
 */
void AmbientLife_RunCrossingFlyer(Obj *obj)
{
    f32 step[4];
    f32 target[4];
    f32 *stepY;
    f32 *stepZ;
    f32 wave;
    f32 bob;

    obj->timer++;
    obj->var2++;
    obj->var2 &= 3;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        target[0] = AmbientLife_CrossingFlyerPoints[obj->var1 * 3];
        target[1] = D_008B0A94[obj->var1 * 3];
        target[2] = D_008B0A98[obj->var1 * 3];
        break;
    }
    if (Cyl_Contains(obj->pos, target, 100.0f, -1.0f, -1.0f) != 0) {
        obj->state++;
        return;
    }
    step[0] = 0.0f;
    stepY = &step[1];
    stepZ = &step[2];
    *stepY = 0.0f;
    *stepZ = 10.0f;
    Vec_RotateY(step, (6.2831855f * obj->var3) / 65536.0f);
    obj->pos[0] += step[0];
    obj->pos[1] += *stepY;
    obj->pos[2] += *stepZ;
    wave = Math_Sin((6.2831855f * (360.0f * ((f32) obj->timer / (f32) (obj->var0 * 2)))) / 360.0f);
    bob = obj->fvar * wave;
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1] + bob;
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/*
 * Draws the crossing flyer: model 8 + Obj.var2 of the area's sub-model (its
 * four wing frames) at the draw slot, turned by its heading, with alpha test
 * reference 128.
 */
void AmbientLife_DrawCrossingFlyer(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    f32 mtx[16];
    ModelMatSet *mdl;
    s16 k;

    if (stage != NULL && stage->active != 0) {
        k = obj->var2;
        mdl = stage->matSets + k + 8;
        Gs_SetState(RS_ALPHA_REF, 128);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_RotateY(mtx, (6.2831855f * obj->var3) / 65536.0f);
        AmbientLife_DrawModel(mdl, (s32) mtx);
    }
}
