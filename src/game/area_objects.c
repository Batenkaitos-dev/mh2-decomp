/*
 * The first area objects.
 *
 * An area object is an Obj that Area_StartObjects makes for the area just
 * entered: per kind a creator, an update that switches on Obj.state (0 Setup,
 * 1 Run, 2 End, 3 Free) and a draw callback on a DrawSlot that draws models
 * of the area's model set (gAreaSubModels[0]).
 *
 * The kinds here, by Obj.kind: LightShaft (0), SkyStreak (2), AreaDrop (3),
 * AreaFlag (4), AreaWeapon (5), AnimatedProps (7) and AnimatedGroundTiles
 * (8). Kind 9, the area's small animals and leaves (AmbientLife), comes next
 * in a file of its own.
 */
#include "common.h"
#include "render_state.h"
#include "sound.h"
#include "quest.h"
#include "obj.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "scene.h"
#include "stageobj.h"
#include "area.h"
#include "area_draw.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "hit_effects.h"
#include "hit_setup.h"
#include "part_effects.h"
#include "rand.h"
#include "text.h"
#include "vec_math.h"
/* Declarations of the functions called below. */

/* unit's own declaration: declared here to return s32 though the function returns s16: AnimatedProps_Run compiles as the game's code only when it takes the result as an s32. */
s32 Area_MonsterState(u16 area);
extern s16 AnimatedProps_SpinSnowyMountains6[];
extern s16 AnimatedProps_SpinSnowyNight6[];
typedef struct AnimatedPropsBuf {
    u8 state[13];               /* 0x00 */
    u8 level[13];               /* 0x0D */
    u16 rot[13];                /* 0x1A */
    s16 slot[13];               /* 0x34: the draw slots (DrawSlot_Alloc) */
    u8 pad4E[80 - 78];
    DrawSlot *ds[13];           /* 0x50 */
} AnimatedPropsBuf;
extern f32 AreaDrop_ZonesDesertNight4[];
extern f32 AreaDrop_ZoneRadiiDesertNight4[];
extern f32 AreaDrop_ZonesTower8[];
extern f32 AreaDrop_ZoneRadiiTower8[];
extern f32 AreaDrop_ZonesDesert4[];
extern f32 AreaDrop_ZoneRadiiDesert4[];
extern f32 AreaDrop_ZonesSwamp3[];
extern f32 AreaDrop_ZoneRadiiSwamp3[];
extern f32 AreaDrop_ZonesSwamp4[];
extern f32 AreaDrop_ZoneRadiiSwamp4[];
extern f32 AreaDrop_ZonesSwamp8[];
extern f32 AreaDrop_ZoneRadiiSwamp8[];
extern f32 gCamYawMtx[];
extern s16 LightShaft_TiltsForestHills7[];
extern f32 LightShaft_ScalesForestHills7[];
/* The y and z of the scale triples: the three loads compile as the game's code only from three symbols. */
extern f32 LightShaft_ScalesForestHills7Y[];
extern f32 LightShaft_ScalesForestHills7Z[];
extern s16 LightShaft_TiltsForestHills11[];
extern f32 LightShaft_WidthsForestHills11[];
extern s16 LightShaft_TiltsForestHills12[];
extern s16 LightShaft_ModelsDesert4[];
extern f32 AreaDrop_FallPointsDesert10[];
extern f32 gCamMatrix[];
extern f32 LightShaft_PosForestHills7[];
extern f32 LightShaft_PosForestHills11[];
extern f32 LightShaft_PosForestHills12[];
extern f32 LightShaft_PosSnowyMountains1[];
extern f32 LightShaft_PosSnowyMountains3[];
extern f32 LightShaft_PosTower8[];
extern f32 LightShaft_PosDesert4[];
extern f32 LightShaft_PosSwamp3[];
extern f32 LightShaft_PosSwamp4[];
typedef struct LightShaftBuf {
    s16 slot[6];
    DrawSlot *ds[6];
} LightShaftBuf;
extern f32 AnimatedGroundTiles_UvSteps[];
extern u8 *AreaWeapon_PartKinds[];
extern f32 *AreaWeapon_PosFortress5[];
extern f32 *AreaWeapon_PosCastle1[];
extern f32 *AreaWeapon_PosBattleTown1[];
extern f32 *AreaWeapon_PosBattleTown3[];
extern f32 *AreaWeapon_PartPos[];
extern u16 AreaWeapon_DayAreas[];
extern s16 *AreaWeapon_Models[];
extern u16 AreaWeapon_YawCastle1[];
extern s16 SkyStreak_RandRange[];
extern s16 SkyStreak_WaitBase[];
extern u16 SkyStreak_WaitMask[];
typedef struct SkyStreakPart {
    s16 variant;
    s16 slot;
    f32 pos[3];
    u8 pad10[12];
    DrawSlot *ds;
    s16 unk20[2];
    s16 frame;
    s16 wait;
    s16 phase;
    s16 unk2A;
    u8 pad2C[4];
} SkyStreakPart;
extern s16 SkyStreak_Models[];
extern s16 SkyStreak_Frames[];
extern f32 AnimatedProps_PosForestHills11[];
extern f32 AnimatedProps_PosSnowyMountains6[];
extern f32 AnimatedProps_PosSnowyNight5[];
extern f32 AnimatedProps_PosSnowyNight6[];
extern f32 AnimatedProps_PosSnowyNight8[];
extern f32 AnimatedProps_PosBattleTown0[];
extern s16 SkyStreak_Counts[];
/* end of prototypes */

/*
 * Makes the area's LightShaft object; Area_StartObjects calls it in the areas
 * LightShaft_Setup places shafts in.
 */
void LightShaft_Create(void)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 0;
        obj->update = LightShaft_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * LightShaft_Setup, _Run, _End and _Free.
 */
void LightShaft_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        LightShaft_Setup(obj);
        break;
    case 1:
        LightShaft_Run(obj);
        break;
    case 2:
        LightShaft_End(obj);
        break;
    case 3:
        LightShaft_Free(obj);
        break;
    }
}

/*
 * Sets the object up for the current area: one or two shafts at the area's
 * positions (the LightShaft_Pos* tables), each on a draw slot of its own
 * drawn by LightShaft_Draw (DrawSlot.part is the shaft). Without a slot for
 * the first shaft it frees itself; else its first frame runs.
 */
void LightShaft_Setup(Obj *obj)
{
    DrawSlot *drawSlot;
    s16 count;
    s16 i;
    f32 *pos;
    LightShaftBuf *shafts;

    shafts = obj->buf;
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        count = 1;
        pos = LightShaft_PosForestHills7;
        break;
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        count = 2;
        pos = LightShaft_PosForestHills11;
        break;
    case AREA_FOREST_HILLS_12:
    case AREA_FOREST_HILLS_NIGHT_12:
        count = 1;
        pos = LightShaft_PosForestHills12;
        break;
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
        count = 1;
        pos = LightShaft_PosSnowyMountains1;
        break;
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        count = 1;
        pos = LightShaft_PosSnowyMountains3;
        break;
    case AREA_TOWER_8:
        count = 2;
        pos = LightShaft_PosTower8;
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_DESERT_4:
        count = 2;
        pos = LightShaft_PosDesert4;
        break;
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_3:
        count = 2;
        pos = LightShaft_PosSwamp3;
        break;
    case AREA_SWAMP_NIGHT_4:
    case AREA_SWAMP_4:
        count = 2;
        pos = LightShaft_PosSwamp4;
        break;
    }
    for (i = 0; i < 6; i++, pos += 3) {
        if (i < count) {
            shafts->slot[i] = DrawSlot_Alloc();
            if (shafts->slot[i] != -1) {
                drawSlot = DrawSlot_Get(shafts->slot[i]);
                shafts->ds[i] = drawSlot;
                drawSlot->owner = obj;
                drawSlot->part = i;
                drawSlot->draw = LightShaft_Draw;
                drawSlot->pos[0] = pos[0];
                drawSlot->pos[1] = pos[1];
                drawSlot->pos[2] = pos[2];
            } else if (i == 0) {
                LightShaft_Free(obj);
                return;
            } else {
                shafts->ds[i] = NULL;
            }
        } else {
            shafts->ds[i] = NULL;
        }
    }
    LightShaft_Run(obj);
}

/*
 * Counts the frames of the texture scroll (a 512-frame period in the snowy
 * mountains' areas 1 and 3, else 100) and adds each shaft's draw slot to the
 * sorted draw list. It reads the slots through a pointer view of its buffer:
 * entry i + 3 is LightShaftBuf.ds[i].
 */
void LightShaft_Run(Obj *obj)
{
    void **slotPtrs = obj->buf;
    s16 period;
    s16 i;

    obj->timer++;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        period = 512;
        break;
    default:
        period = 100;
        break;
    }
    if (obj->timer >= period) {
        obj->timer = 0;
    }
    for (i = 0; i < 6; i++) {
        if (slotPtrs[i + 3] != NULL) {
            OT_AddByDepth(gDrawOtSorted, slotPtrs[i + 3], 64, 0);
        }
    }
}

/* Hides the object; it is freed on the next update. */
void LightShaft_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void LightShaft_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws one shaft with additive blending, no alpha test and no depth writes:
 * a model of the area's set facing the camera's yaw (not turned in the snowy
 * mountains' areas), scaled and tilted per shaft in the forest's area 7,
 * widened and tilted in its area 11, tilted in its area 12, its texture
 * scrolled along u once per period (Obj.timer).
 */
void LightShaft_Draw(DrawSlot *drawSlot)
{
    Model *stage = gAreaSubModels[0];
    Obj *obj = drawSlot->owner;
    ModelMatSet *mdl;
    s32 period;
    f32 factor;
    s16 tiltZ;
    f32 mtx[16];
    f32 uvMtx[16];

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(RS_ZWRITE, 0);
        Gs_SetState(RS_ALPHA_REF, 0);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
        Mtx_Copy(mtx, gCamYawMtx);
        period = 100;
        switch (gScene.area) {
        case AREA_FOREST_HILLS_7:
        case AREA_FOREST_HILLS_NIGHT_7:
            Mtx_ScaleBy(mtx, LightShaft_ScalesForestHills7[drawSlot->part * 3], LightShaft_ScalesForestHills7Y[drawSlot->part * 3], LightShaft_ScalesForestHills7Z[drawSlot->part * 3]);
            tiltZ = LightShaft_TiltsForestHills7[drawSlot->part * 2 + 1];
            mdl = stage->matSets + 1;
            Mtx_RotateX(mtx, 6.2831855f * LightShaft_TiltsForestHills7[drawSlot->part * 2] / 360.0f);
            Mtx_RotateZ(mtx, 6.2831855f * tiltZ / 360.0f);
            break;
        case AREA_FOREST_HILLS_11:
        case AREA_FOREST_HILLS_NIGHT_11:
            factor = LightShaft_WidthsForestHills11[drawSlot->part];
            Mtx_ScaleBy(mtx, factor, 1.0f, factor);
            mdl = stage->matSets;
            Mtx_RotateZ(mtx, 6.2831855f * LightShaft_TiltsForestHills11[drawSlot->part] / 360.0f);
            break;
        case AREA_FOREST_HILLS_12:
        case AREA_FOREST_HILLS_NIGHT_12:
            mdl = stage->matSets;
            Mtx_RotateZ(mtx, 6.2831855f * LightShaft_TiltsForestHills12[drawSlot->part] / 360.0f);
            break;
        case AREA_SNOWY_MOUNTAINS_1:
        case AREA_SNOWY_MOUNTAINS_NIGHT_1:
            Mtx_Identity(mtx);
            period = 512;
            mdl = stage->matSets + 3;
            break;
        case AREA_SNOWY_MOUNTAINS_3:
        case AREA_SNOWY_MOUNTAINS_NIGHT_3:
            Mtx_Identity(mtx);
            period = 512;
            mdl = stage->matSets + 3;
            break;
        case AREA_TOWER_8:
            mdl = stage->matSets + drawSlot->part + 2;
            break;
        case AREA_DESERT_NIGHT_4:
        case AREA_DESERT_4:
            mdl = stage->matSets + LightShaft_ModelsDesert4[drawSlot->part];
            break;
        case AREA_SWAMP_NIGHT_3:
        case AREA_SWAMP_NIGHT_4:
        case AREA_SWAMP_3:
        case AREA_SWAMP_4:
            mdl = stage->matSets;
            break;
        }
        factor = (1.0f / period) * obj->timer;
        Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Mtx_Translation(uvMtx, factor, 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        if (mdl->id != -1) {
            Model_Draw(mdl->id, 0);
        }
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * Makes a SkyStreak object of type streakType (Area_StartObjects makes type 0
 * in the desert's night area 3).
 */
void SkyStreak_Create(u8 streakType)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 2;
        obj->type = streakType;
        obj->update = SkyStreak_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * SkyStreak_Setup, _Run, _End and _Free.
 */
void SkyStreak_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        SkyStreak_Setup(obj);
        break;
    case 1:
        SkyStreak_Run(obj);
        break;
    case 2:
        SkyStreak_End(obj);
        break;
    case 3:
        SkyStreak_Free(obj);
        break;
    }
}

/*
 * Sets up the type's parts (SkyStreak_Counts): each waits 100 to 163 frames
 * (SkyStreak_WaitBase plus random SkyStreak_WaitMask bits) with a random
 * model variant and a draw slot of its own (SkyStreak_Draw); then the first
 * frame runs.
 */
void SkyStreak_Setup(Obj *obj)
{
    SkyStreakPart *streak = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    obj->timer = 0;
    obj->fvar = 0.0f;
    count = SkyStreak_Counts[obj->type];
    for (i = 0; i < count; i++, streak++) {
        streak->variant = Rand_Next(1) % SkyStreak_RandRange[obj->type];
        streak->phase = 0;
        streak->unk2A = 0;
        streak->frame = 0;
        streak->wait = SkyStreak_WaitBase[obj->type] + (SkyStreak_WaitMask[obj->type] & Rand_Next(1));
        streak->pos[0] = 0.0f;
        streak->pos[1] = 0.0f;
        streak->pos[2] = 0.0f;
        streak->slot = DrawSlot_Alloc();
        if (streak->slot != -1) {
            streak->ds = DrawSlot_Get(streak->slot);
            if (streak->ds != NULL) {
                streak->ds->owner = obj;
                streak->ds->part = i;
                streak->ds->draw = SkyStreak_Draw;
            }
        } else {
            streak->ds = NULL;
        }
    }
    SkyStreak_Run(obj);
}

/*
 * Steps each part: after its wait it picks a random model variant (below
 * SkyStreak_RandRange) and shows it for SkyStreak_Frames frames, added to the
 * 8-entry draw list, then waits again.
 */
void SkyStreak_Run(Obj *obj)
{
    SkyStreakPart *part = obj->buf;
    s16 count = SkyStreak_Counts[obj->type];
    s16 i;

    for (i = 0; i < count; i++, part++) {
        switch (part->phase) {
        case 0:
            if (++part->frame >= part->wait) {
                part->phase++;
                part->frame = 0;
                part->variant = Rand_Next(1) % SkyStreak_RandRange[obj->type];
            }
            break;
        case 1:
            if (++part->frame >= SkyStreak_Frames[obj->type]) {
                part->phase = 0;
                part->frame = 0;
                part->wait = SkyStreak_WaitBase[obj->type] + (SkyStreak_WaitMask[obj->type] & Rand_Next(1));
            } else if (part->ds != NULL) {
                Vec_Copy((f32 *)part->ds->pos, (f32 *)part->pos);
                OT_AddByDepth(gDrawOt8, part->ds, 8, 1);
            }
            break;
        }
    }
}

/*
 * Frees each part's draw slot and hides the object; it is freed on the next update.
 */
void SkyStreak_End(Obj *obj)
{
    SkyStreakPart *part = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = SkyStreak_Counts[obj->type];
    for (i = 0; i < count; i++) {
        if (part->ds != NULL) {
            DrawSlot_Free(part->slot);
        }
        part++;
    }
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void SkyStreak_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws a part's model (the type's first model, SkyStreak_Models, plus its
 * variant) at the part's position, the origin, the model carrying its own
 * place; no alpha test, a white colour, its texture scrolled once along v
 * over the showing.
 */
void SkyStreak_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    SkyStreakPart *streak = (SkyStreakPart *) obj->buf + drawSlot->part;
    f32 mtx[16];
    f32 uvMtx[16];
    ModelMatSet *model;

    if (stage != NULL && stage->active != 0) {
        model = stage->matSets + SkyStreak_Models[obj->type] + streak->variant;
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Gs_SetState(RS_ALPHA_REF, 0);
        Gs_SetState(RS_COLOR2, -1);
        Mtx_Translation(uvMtx, 0.0f, (f32) streak->frame / (f32) SkyStreak_Frames[obj->type], 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        if (model != NULL && model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Makes the area's ambient AreaDrop (type 0); Area_StartObjects calls it in
 * the desert's areas 4 and 10, the swamp's 3, 4 and 8 and the tower's area 8.
 */
void AreaDrop_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 3;
        obj->type = 0;
        obj->update = AreaDrop_Update;
    }
}

/*
 * Makes a weapon burst (type 1) at spot; AreaWeapon_Run makes them at its
 * burst points when a great weapon fires.
 */
void AreaDrop_CreateWeaponBurst(void *spot)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 3;
        obj->type = 1;
        obj->update = AreaDrop_Update;
        Vec_Copy((f32 *)obj->pos, (f32 *)spot);
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AreaDrop_Setup, _Run, _End and _Free.
 */
void AreaDrop_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaDrop_Setup(obj);
        break;
    case 1:
        AreaDrop_Run(obj);
        break;
    case 2:
        AreaDrop_End(obj);
        break;
    case 3:
        AreaDrop_Free(obj);
        break;
    }
}

/*
 * Sets the object up: type 0 waits 10 to 41 frames, type 1 starts its scroll
 * at 0. It takes a draw slot (AreaDrop_Draw) and runs its first frame, or
 * frees itself without one.
 */
void AreaDrop_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    switch (obj->type) {
    case 0:
        obj->timer = (Rand_Next(1) & 0x1F) + 10;
        break;
    case 1:
        obj->timer = 0;
        obj->fvar = 0.0f;
        break;
    }
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = AreaDrop_Draw;
        AreaDrop_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Runs the object by area: drips (AreaDrop_RunDrip) in the desert's area 4,
 * the swamp's 3, 4 and 8 and the tower's 8; weapon bursts
 * (AreaDrop_RunWeaponBurst) in the fortress's area 5, the castle's 1 and the
 * battle town's 1; and here the desert's area 10, where after a random wait
 * one of four points (AreaDrop_FallPointsDesert10) shows its falling stream
 * for 100 frames, with a sound on frame 2 and an Effect0D puff at the floor
 * 15, 20, 25, 30 and 35 frames before the end.
 */
void AreaDrop_Run(Obj *obj)
{
    f32 (*points)[3];
    f32 pos[3];

    switch (gScene.area) {
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        points = (f32 (*)[3]) AreaDrop_FallPointsDesert10;
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_NIGHT_8:
    case AREA_TOWER_8:
    case AREA_DESERT_4:
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
    case AREA_SWAMP_8:
        AreaDrop_RunDrip(obj);
        return;
    case AREA_FORTRESS_5:
    case AREA_CASTLE_1:
    case AREA_FORTRESS_NIGHT_5:
    case AREA_BATTLE_TOWN_1:
    case AREA_BATTLE_TOWN_NIGHT_1:
        AreaDrop_RunWeaponBurst(obj);
        return;
    default:
        return;
    }
    switch (obj->var0) {
    case 0:
        if (--obj->timer <= 0) {
            obj->var0++;
            obj->var1 = Rand_Next(1) % 4;
            obj->timer = 0;
        }
        break;
    case 1:
        if (++obj->timer >= 100) {
            obj->var0 = 0;
            obj->timer = Rand_Next(1) & 0x3F;
        } else {
            obj->ds->pos[0] = points[obj->var1][0];
            obj->ds->pos[1] = points[obj->var1][1];
            obj->ds->pos[2] = points[obj->var1][2];
            if (obj->timer == 2) {
                SoundObj_Start(7, 32, 1, obj->ds->pos, 100, 0);
            }
            OT_AddByDepth(gDrawOtSorted, obj->ds, 64, 0);
        }
        switch (100 - obj->timer) {
        case 15:
        case 20:
        case 25:
        case 30:
        case 35:
            pos[0] = obj->ds->pos[0];
            pos[1] = -10.0f;
            pos[2] = obj->ds->pos[2];
            PartFx_SpawnAt(pos, 0, 1.75f);
            break;
        }
        break;
    }
}

/*
 * A drip: after a random wait it picks a spot on the ground 450 to 1473 units
 * from the camera, in a 90-degree fan around the camera's heading turned a
 * quarter, and keeps it only inside one of the area's drip zones (the
 * AreaDrop_Zones* centres and radii; else it tries again the next frame). The
 * drop then falls 1000 units in 30 frames and its splash spreads for 60
 * frames before a new wait of up to 63 frames; Obj.var2 is the model each
 * step shows.
 */
void AreaDrop_RunDrip(Obj *obj)
{
    f32 *radius;
    f32 *zoneCenter;
    u8 i;
    s16 zoneCount;
    u16 angle;
    f32 *spotZ;
    f32 spot[3];
    f32 camPos[3];
    f32 height;

    switch (obj->var0) {
    case 0:
        if (--obj->timer <= 0) {
            angle = Mtx_Heading(gCamMatrix) + ANGLE_TURN / 4;
            spot[0] = 0.0f;
            spot[1] = 0.0f;
            spotZ = &spot[2];
            *spotZ = -450 - (Rand_Next(1) & 0x3FF);
            angle += (u16) ((Rand_Next(1) & 0x3FFF) - 0x1FFF);
            Vec_RotateY(spot, 6.2831855f * angle / 65536.0f);
            Mtx_GetTrans(camPos, gCamMatrix);
            spot[0] += camPos[0];
            *spotZ += camPos[2];
            switch (gScene.area) {
            default:
            case AREA_TOWER_8:
                obj->var2 = 5;
                zoneCount = 1;
                zoneCenter = AreaDrop_ZonesTower8;
                radius = AreaDrop_ZoneRadiiTower8;
                break;
            case AREA_DESERT_NIGHT_4:
                obj->var2 = 8;
                zoneCount = 1;
                zoneCenter = AreaDrop_ZonesDesertNight4;
                radius = AreaDrop_ZoneRadiiDesertNight4;
                break;
            case AREA_SWAMP_NIGHT_3:
                zoneCount = 2;
                obj->var2 = 2;
                zoneCenter = AreaDrop_ZonesSwamp3;
                radius = AreaDrop_ZoneRadiiSwamp3;
                break;
            case AREA_SWAMP_NIGHT_8:
                obj->var2 = 1;
                zoneCount = 2;
                zoneCenter = AreaDrop_ZonesSwamp8;
                radius = AreaDrop_ZoneRadiiSwamp8;
                break;
            case AREA_DESERT_4:
                obj->var2 = 8;
                zoneCount = 1;
                zoneCenter = AreaDrop_ZonesDesert4;
                radius = AreaDrop_ZoneRadiiDesert4;
                break;
            case AREA_SWAMP_3:
                zoneCount = 2;
                obj->var2 = 2;
                zoneCenter = AreaDrop_ZonesSwamp3;
                radius = AreaDrop_ZoneRadiiSwamp3;
                break;
            case AREA_SWAMP_4:
                zoneCount = 2;
                obj->var2 = 2;
                zoneCenter = AreaDrop_ZonesSwamp4;
                radius = AreaDrop_ZoneRadiiSwamp4;
                break;
            case AREA_SWAMP_8:
                obj->var2 = 1;
                zoneCount = 2;
                zoneCenter = AreaDrop_ZonesSwamp8;
                radius = AreaDrop_ZoneRadiiSwamp8;
                break;
            }
            for (i = 0; i < zoneCount; i++) {
                if (!(Vec_Distance(spot, zoneCenter) < *radius) == 0) {
                    Vec_Copy(obj->pos, (f32 *)spot);
                    obj->var0++;
                    obj->timer = 0;
                    return;
                }
                zoneCenter += 3;
                radius++;
            }
        }
        break;
    case 1:
        if (++obj->timer >= 30) {
            switch (gScene.area) {
            default:
            case AREA_TOWER_8:
                obj->var2 = 4;
                break;
            case AREA_DESERT_NIGHT_4:
            case AREA_DESERT_4:
                obj->var2 = 5;
                break;
            case AREA_SWAMP_NIGHT_3:
            case AREA_SWAMP_3:
                obj->var2 = 1;
                break;
            case AREA_SWAMP_4:
                obj->var2 = 1;
                break;
            case AREA_SWAMP_NIGHT_8:
            case AREA_SWAMP_8:
                obj->var2 = 0;
                break;
            }
            obj->var0++;
            obj->timer = 0;
            return;
        }
        height = 1000.0f - 33.333332f * obj->timer;
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1] + height;
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOtSorted, obj->ds, 64, 0);
        break;
    case 2:
        if (++obj->timer >= 60) {
            obj->var0 = 0;
            obj->timer = Rand_Next(1) & 0x3F;
            return;
        }
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt8, obj->ds, 8, 0);
        break;
    }
}

/*
 * A weapon burst: for 20 frames moves its texture by 0.05 a frame (Obj.fvar,
 * up to 1.0) at its position, then ends.
 */
void AreaDrop_RunWeaponBurst(Obj *obj)
{

    if (++obj->timer >= 20) {
        obj->state++;
        return;
    }
    obj->fvar += 0.05f;
    if (obj->fvar > 1.0f) {
        obj->fvar = 1.0f;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOtSorted, obj->ds, 64, 0);
}

/*
 * Frees the draw slot, when there is one, and hides the object; it is freed
 * on the next update.
 */
void AreaDrop_End(Obj *obj)
{
    if (obj->ds != NULL) {
        DrawSlot_Free(obj->slot);
    }
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AreaDrop_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the object, facing the camera except a weapon burst: the desert's
 * area 10 stream with its texture scrolled down through its 100 frames; a
 * drip's drop fading in as it falls, then its splash widening from nothing
 * while it fades and scrolls; a weapon burst flat (turned 26 degrees in the
 * castle) without depth writes, fading out over its 20 frames, its texture
 * moved by Obj.fvar. No alpha test; depth writes are back on after.
 */
void AreaDrop_Draw(DrawSlot *drawSlot)
{
    Obj *obj;
    ModelMatSet *mdl;
    u8 flat;
    Model *stage;
    u8 alpha;
    f32 spread;
    f32 mtx[16];
    f32 uvMtx[16];

    stage = gAreaSubModels[0];
    obj = drawSlot->owner;
    flat = 0;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(RS_ALPHA_REF, 0);
        Gs_SetState(RS_COLOR2, -1);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_10:
        case AREA_DESERT_10:
            mdl = stage->matSets + 1;
            Mtx_Translation(uvMtx, 0.0f, 1.0f - obj->timer / 100.0f, 0.0f);
            break;
        case AREA_DESERT_NIGHT_4:
        case AREA_SWAMP_NIGHT_3:
        case AREA_SWAMP_NIGHT_8:
        case AREA_TOWER_8:
        case AREA_DESERT_4:
        case AREA_SWAMP_3:
        case AREA_SWAMP_4:
        case AREA_SWAMP_8:
            mdl = stage->matSets + obj->var2;
            if (obj->var0 == 1) {
                alpha = 8.5f * obj->timer;
            } else {
                spread = 0.016666668f * obj->timer;
                Mtx_SetScale(mtx, spread, 1.0f, spread);
                Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
                alpha = 255 - (u8) (4.25f * obj->timer);
                Mtx_Translation(uvMtx, 0.0f, 0.016666668f * obj->timer, 0.0f);
            }
            Gs_SetState(RS_COLOR2, (alpha << 24) | 0xFFFFFF);
            break;
        case AREA_BATTLE_TOWN_1:
        case AREA_BATTLE_TOWN_NIGHT_1:
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 5;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (255 - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_FORTRESS_5:
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 7;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (255 - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_FORTRESS_NIGHT_5:
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 8;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (255 - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_CASTLE_1:
            Mtx_RotateY(mtx, 0.45378563f);
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 15;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (255 - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        }
        if (flat == 0) {
            Mtx_Concat3x3(mtx, gCamMatrix);
        }
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * Makes the area's flag, except in the fortress's area 3 once
 * gScene.emEventFlags bit 0 is set (Shen Gaoren and Lao-Shan Lung set it).
 */
void AreaFlag_Create(void)
{
    Obj *obj;

    if ((gScene.area == AREA_FORTRESS_3 || gScene.area == AREA_FORTRESS_NIGHT_3) && (gScene.emEventFlags & 1)) {
        return;
    }
    obj = Obj_Alloc(0);
    if (obj != NULL) {
        obj->kind = 4;
        obj->update = AreaFlag_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AreaFlag_Setup, _Run, _End and _Free.
 */
void AreaFlag_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaFlag_Setup(obj);
        break;
    case 1:
        AreaFlag_Run(obj);
        break;
    case 2:
        AreaFlag_End(obj);
        break;
    case 3:
        AreaFlag_Free(obj);
        break;
    }
}

/*
 * Places the flag at its area's fixed position (the fortress's areas 1 and 3,
 * the arenas' area 0, the great arena's and the arena's area 1, the snowy
 * mountains' area 6); in another area, or without a draw slot, it frees
 * itself, else its first frame runs.
 */
void AreaFlag_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->var3 = 0;
    switch (gScene.area) {
    case AREA_FORTRESS_1:
    case AREA_FORTRESS_NIGHT_1:
        obj->pos[0] = 12680.0f;
        obj->pos[1] = 696.0f;
        obj->pos[2] = 12756.0f;
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        obj->pos[0] = 10400.0f;
        obj->pos[1] = 1300.0f;
        obj->pos[2] = 20570.0f;
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        obj->pos[0] = 10007.0f;
        obj->pos[1] = 200.0f;
        obj->pos[2] = 9377.0f;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        obj->pos[0] = 5129.0f;
        obj->pos[1] = 1513.0f;
        obj->pos[2] = 10935.0f;
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        obj->pos[0] = 9908.0f;
        obj->pos[1] = 2957.0f;
        obj->pos[2] = 15082.0f;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        obj->pos[0] = 11600.0f;
        obj->pos[1] = 4460.0f;
        obj->pos[2] = 7610.0f;
        break;
    default:
        Obj_Free(obj);
        return;
    }
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AreaFlag_Draw;
        Vec_Copy(obj->ds->pos, obj->pos);
        AreaFlag_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Steps the flag's 16-frame animation (Obj.timer), plays its sound every 55
 * frames (Obj.var3 counts frames) and adds it to the 32-entry draw list. In
 * the fortress's area 3 it stops once emEventFlags bit 0 is set; in the snowy
 * mountains' area 6 it runs only once a set-down delivery is done
 * (QuestGoal_DeliveryDone) and rises 1.95 units a frame up to height 4850.
 */
void AreaFlag_Run(Obj *obj)
{
    s16 firstFrame;
    s16 lastFrame;

    obj->var3++;
    switch (gScene.area) {
    case AREA_FORTRESS_1:
        firstFrame = 2;
        lastFrame = 17;
        break;
    case AREA_FORTRESS_NIGHT_1:
        firstFrame = 3;
        lastFrame = 18;
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        if (gScene.emEventFlags & 1) {
            return;
        }
        firstFrame = 4;
        lastFrame = 19;
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        firstFrame = 1;
        lastFrame = 16;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        firstFrame = 6;
        lastFrame = 21;
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        firstFrame = 1;
        lastFrame = 16;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        switch (QuestGoal_DeliveryDone()) {
        case 0:
            return;
        case 1:
            firstFrame = 13;
            lastFrame = 28;
            break;
        case 2:
            firstFrame = 29;
            lastFrame = 44;
            break;
        }
        obj->pos[1] += 1.95f;
        if (obj->pos[1] > 4850.0f) {
            obj->pos[1] = 4850.0f;
        }
        break;
    default:
        return;
    }
    if (++obj->timer >= lastFrame - firstFrame + 1) {
        obj->timer = 0;
    }
    if (obj->var3 % 55 == 0) {
        Sound_PlaySeAtPosChecked(7, 82, 0, obj->pos, 17, 0);
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/* Hides the object; it is freed on the next update. */
void AreaFlag_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AreaFlag_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the flag's current frame (the area's first frame model plus
 * Obj.timer), swaying up to 30 degrees either way about y over 256 frames,
 * except in the fortress's area 3.
 */
void AreaFlag_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *mdl;
    s16 firstModel;
    f32 mtx[16];

    if (stage != NULL && stage->active != 0) {
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        switch (gScene.area) {
        case AREA_FORTRESS_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 2;
            break;
        case AREA_FORTRESS_NIGHT_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 3;
            break;
        case AREA_FORTRESS_3:
        case AREA_FORTRESS_NIGHT_3:
            firstModel = 4;
            break;
        case AREA_ARENAS_NIGHT_0:
        case AREA_ARENAS_0:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 1;
            break;
        case AREA_GREAT_ARENA_NIGHT_1:
        case AREA_GREAT_ARENA_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 6;
            break;
        case AREA_ARENA_NIGHT_1:
        case AREA_ARENA_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 1;
            break;
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_NIGHT_6:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 13;
            break;
        default:
            return;
        }
        mdl = stage->matSets + firstModel + obj->timer;
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Makes the parts of the area's weapon set weaponSet (0 to 9, two to five
 * parts), one object a part with its index in Obj.var2.
 */
void AreaWeapon_Create(u8 weaponSet)
{
    u8 count;
    u8 i;
    Obj *obj;

    switch (weaponSet) {
    case 0:
    case 2:
    case 4:
    case 5:
    case 7:
    case 8:
        count = 3;
        break;
    case 1:
    case 6:
        count = 5;
        break;
    case 3:
        count = 2;
        break;
    case 9:
        count = 4;
        break;
    default:
        count = 0;
        break;
    }
    for (i = 0; i < count; i++) {
        obj = Obj_Alloc(0);
        if (obj != NULL) {
            obj->kind = 5;
            obj->type = weaponSet;
            obj->update = AreaWeapon_Update;
            obj->var2 = i;
        }
    }
}

/*
 * The part's update, run every frame: switches on Obj.state to
 * AreaWeapon_Setup, _Run, _End and _Free.
 */
void AreaWeapon_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaWeapon_Setup(obj);
        break;
    case 1:
        AreaWeapon_Run(obj);
        break;
    case 2:
        AreaWeapon_End(obj);
        break;
    case 3:
        AreaWeapon_Free(obj);
        break;
    }
}

/*
 * Sets a part up from its set's tables: its position (AreaWeapon_PartPos)
 * and, for a great weapon, its countdown state (waiting, or done for a
 * countdown already running). It takes a draw slot (AreaWeapon_Draw) and runs
 * its first frame, or frees itself without one.
 */
void AreaWeapon_Setup(Obj *obj)
{
    u8 partKind = AreaWeapon_PartKinds[obj->type][obj->var2];
    f32 *posSrc;

    switch (obj->type) {
    case 1:
    case 6:
        posSrc = AreaWeapon_PosFortress5[obj->var2];
        break;
    case 2:
        posSrc = AreaWeapon_PosCastle1[obj->var2];
        break;
    case 8:
        posSrc = AreaWeapon_PosBattleTown1[obj->var2];
        break;
    case 9:
        posSrc = AreaWeapon_PosBattleTown3[obj->var2];
        break;
    default:
        posSrc = AreaWeapon_PartPos[obj->type] + obj->var2 * 3;
        break;
    }
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->var3 = 0;
    if (partKind == 2) {
        if (gScene.countdownOn == 0) {
            obj->var0 = 0;
        } else {
            obj->timer = gQuest.countdown;
            obj->var0 = 5;
        }
    }
    obj->pos[0] = *posSrc++;
    obj->pos[1] = *posSrc++;
    obj->pos[2] = *posSrc;
    obj->slot = DrawSlot_Alloc();
    obj->ds = DrawSlot_Get(obj->slot);
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AreaWeapon_Draw;
        Vec_Copy(obj->ds->pos, obj->pos);
        AreaWeapon_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/* unit's own declaration: declared here with ang as u16 though the function takes an s32: AreaWeapon_Run compiles as the game's code only with this declaration. */
void StageShot_CreateAt(f32 *pos, u8 code, u8 area, u16 ang);
extern u16 AreaWeapon_GunTurnSounds[];
extern u16 AreaWeapon_GunRestYawBattleTown1[];
extern f32 AreaWeapon_BurstPosFortress5[];
extern f32 AreaWeapon_BurstPosCastle1[];
extern f32 AreaWeapon_BurstPosBattleTown1[];
extern s32 gQuestCountdown;
/*
 * Runs a part by its part kind. A mounted gun follows the player operating it
 * (PL_MOT_MOUNTED_GUN past frame 66, within 200 units; the local player first): his
 * heading goes to Obj.timer and his aim pitch to Obj.var3, its turning sound
 * plays at most every 10 frames, and with no one on it it goes back to its
 * rest heading. A great weapon waits for the quest countdown: 45 frames after
 * the countdown starts it makes the weapon bursts
 * (AreaDrop_CreateWeaponBurst), StageObj_K0E effects and its StageShot hit,
 * thrusts out 1300 units along its yaw in 4 frames (with its sound), holds 90
 * frames, pulls back over 150 and waits for the next countdown. The countdown
 * test is a switch with its default first: the compiler then leaves the
 * branch delay slots of the whole function empty, as in the game's code.
 */
void AreaWeapon_Run(Obj *obj)
{
    Player *player;
    u8 *unusedPtr;
    s16 i;
    u8 local;
    u16 ang;
    u8 code;
    u8 unusedIdx;
    Player *localPl;
    f32 *offsetY;
    f32 *offsetZ;
    f32 offset[3];
    f32 effectPos[3];
    f32 burstOfs[3];
    u8 flags[4];

    offsetY = &offset[1];
    local = 0;
    offsetZ = &offset[2];
    offset[0] = 0.0f;
    *offsetY = 0.0f;
    *offsetZ = 0.0f;
    player = gPlayers;
    switch (AreaWeapon_PartKinds[obj->type][obj->var2]) {
    case 2:
        obj->timer++;
        switch (obj->type) {
        case 1:
        case 6:
            ang = 0;
            code = 2;
            break;
        case 2:
            ang = AreaWeapon_YawCastle1[obj->var2];
            code = 3;
            break;
        case 8:
            code = 8;
            ang = ANGLE_TURN / 2;
            break;
        default:
            ang = 0;
            code = 255;
            break;
        }
        switch (obj->var0) {
        case 0:
            switch (gScene.countdownOn) {
            default:
                obj->var0++;
                obj->timer = 0;
                break;
            case 0:
                break;
            }
            break;
        case 1:
            if (obj->timer > 45) {
                obj->timer = 0;
                switch (obj->type) {
                case 8:
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosBattleTown1);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosBattleTown1 + 3);
                    Vec_Copy((f32 *)effectPos, (f32 *)AreaWeapon_BurstPosBattleTown1);
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 2.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosBattleTown1 + 3));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 2.0f);
                    break;
                case 1:
                case 6:
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5 + 3);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5 + 6);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5 + 9);
                    Vec_Copy((f32 *)effectPos, (f32 *)AreaWeapon_BurstPosFortress5);
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosFortress5 + 3));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosFortress5 + 6));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosFortress5 + 9));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    break;
                case 2:
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosCastle1);
                    Vec_Copy((f32 *)effectPos, (f32 *)AreaWeapon_BurstPosCastle1);
                    burstOfs[2] = 100.0f;
                    burstOfs[0] = 0.0f;
                    burstOfs[1] = 0.0f;
                    Vec_RotateY(burstOfs, 0.45378563f);
                    Vec_Add(effectPos, effectPos, burstOfs);
                    FireBlast_CreateScaled(effectPos, 8, 2.0f);
                    break;
                }
                obj->var0++;
                if (code != 255) {
                    switch (obj->type) {
                    case 8:
                        StageShot_CreateAt(obj->ds->pos, code, gScene.area, ang);
                        break;
                    default:
                        StageShot_CreateAt(obj->pos, code, gScene.area, ang);
                        break;
                    }
                }
            }
            break;
        case 2:
            obj->var1++;
            *offsetZ = 1300.0f * (obj->timer / 4.0f);
            if (obj->timer >= 4) {
                switch (obj->type) {
                case 8:
                    Sound_PlaySeAtPosChecked(7, 54, 0, obj->ds->pos, 7, 0);
                    break;
                default:
                    Sound_PlaySeAtPosChecked(7, 54, 0, obj->pos, 7, 0);
                    break;
                }
                obj->timer = 0;
                obj->var0++;
            }
            break;
        case 3:
            obj->var1++;
            *offsetZ = 1300.0f;
            if (obj->timer >= 90) {
                obj->timer = 0;
                obj->var0++;
            }
            break;
        case 4:
            obj->var1++;
            *offsetZ = 1300.0f - 1300.0f * (obj->timer / 150.0f);
            if (obj->timer >= 150) {
                obj->timer = gQuestCountdown;
                obj->var0 = 5;
            }
            break;
        case 5:
            if (obj->timer < gQuestCountdown) {
                obj->var0 = 1;
                obj->timer = 0;
            } else {
                if (gScene.countdownOn == 0) {
                    obj->var0 = 0;
                }
                obj->timer = gQuestCountdown;
            }
            break;
        }
        Vec_RotateY(offset, 6.2831855f * ang / 65536.0f);
        switch (obj->type) {
        case 8:
            Vec_Copy(obj->pos, (f32 *)offset);
            OT_AddByDepth(gDrawOt32, obj->ds, 32, 1);
            return;
        }
        break;
    case 1:
        if (obj->var1 > 0) {
            obj->var1--;
        }
        for (i = 0; i < 4; i++, player++) {
            flags[i] = 0;
            if (player->active && player->visible && (u8) Obj_InCurrentArea(player) && player->motionId[0] == PL_MOT_MOUNTED_GUN &&
                Motion_ReachedFrame(player, 0, 66.0f)) {
                flags[i] = 1;
                if (Player_IsLocal(player)) {
                    local = 1;
                }
            }
        }
        if (local) {
            offset[0] = obj->pos[0];
            localPl = &gPlayers[gScene.localPlayer];
            offset[1] = localPl->pos[1];
            offset[2] = obj->pos[2];
            if (Vec_Distance(localPl->pos, offset) <= 200.0f) {
                if (obj->var1 == 0 && (u16) obj->timer != localPl->rot[1]) {
                    Sound_PlaySeAtPosChecked(7, AreaWeapon_GunTurnSounds[(u8) AreaWeapon_GunIndex(obj)], 0, obj->pos, 1, 0);
                    obj->var1 = 10;
                }
                obj->var3 = localPl->aimPitchPct;
                obj->timer = localPl->rot[1];
                obj->var4 = gScene.localPlayer;
                goto done;
            }
        }
        player = gPlayers;
        for (i = 0; i < 4; i++, player++) {
            if (flags[i] && player->active && player->visible && (u8) Obj_InCurrentArea(player) && !Player_IsLocal(player)) {
                offset[0] = obj->pos[0];
                offset[1] = player->pos[1];
                offset[2] = obj->pos[2];
                if (Vec_Distance(player->pos, offset) <= 200.0f) {
                    if (obj->var1 == 0 && (u16) obj->timer != player->rot[1]) {
                        Sound_PlaySeAtPosChecked(7, AreaWeapon_GunTurnSounds[(u8) AreaWeapon_GunIndex(obj)], 0, obj->pos, 1, 0);
                        obj->var1 = 10;
                    }
                    obj->var3 = player->aimPitchPct;
                    obj->timer = player->rot[1];
                    obj->var4 = i;
                    goto done;
                }
            }
        }
        obj->var3 = 0;
        switch (obj->type) {
        case 1:
        case 6:
        case 9:
            obj->timer = 0;
            break;
        case 2:
            obj->timer = AreaWeapon_YawCastle1[obj->var2];
            break;
        case 8:
            obj->timer = AreaWeapon_GunRestYawBattleTown1[obj->var2];
            break;
        }
    done:
        offset[0] = 0.0f;
        *offsetY = 0.0f;
        *offsetZ = 0.0f;
        break;
    }
    obj->ds->pos[0] = obj->pos[0] + offset[0];
    obj->ds->pos[1] = obj->pos[1] + *offsetY;
    obj->ds->pos[2] = obj->pos[2] + *offsetZ;
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 1);
}

/* Hides the part and frees its draw slot; it is freed on the next update. */
void AreaWeapon_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Frees the part (Obj_Free). */
void AreaWeapon_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws a part, only in its set's day area or that area's night twin
 * (AreaWeapon_DayAreas): up to two models of its part kind
 * (AreaWeapon_*Models). A mounted gun turns by Obj.timer and tilts its barrel
 * (the second model, 100 up and 75 ahead) by the aim pitch, up to 45 degrees
 * up or 35 down; a great weapon scrolls its texture (fortress), spins about
 * its length (castle) or rolls while it thrusts (battle town, whose two
 * models sit at their own points moved by the thrust kept in Obj.pos); the
 * castle's parts turn by their yaw.
 */
void AreaWeapon_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    u8 weaponSet = obj->type;
    u16 day = AreaWeapon_DayAreas[weaponSet];
    u8 partKind;
    f32 *modelPosY;
    f32 *modelPosZ;
    ModelMatSet *mdl;
    s16 *modelId;
    s16 j;
    s32 modelOfs;
    s32 drawKind;
    f32 yaw;
    u16 pitchTurn;
    f32 turn;
    u8 *posSrc;
    f32 modelPos[3];
    f32 barrelOfs[3];
    f32 mtx[16];
    f32 uvMtx[16];

    if (day == Area_DayId(gScene.area) && stage != NULL && stage->active != 0) {
        partKind = AreaWeapon_PartKinds[weaponSet][obj->var2];
        modelId = AreaWeapon_Models[partKind] + obj->type * 2;
        switch (obj->type) {
        case 2:
            yaw = 6.2831855f * AreaWeapon_YawCastle1[obj->var2] / 65536.0f;
            break;
        default:
            yaw = 0.0f;
            break;
        }
        drawKind = partKind;
        for (j = 0, modelOfs = 0, modelPosY = &modelPos[1], modelPosZ = &modelPos[2]; j < 2; j++, modelOfs += 12, modelId++) {
            if (*modelId == -1) {
                break;
            }
            if (drawKind == 1) {
                Vec_Copy((f32 *)modelPos, (f32 *)drawSlot->pos);
                if (j == 1) {
                    barrelOfs[0] = 0.0f;
                    barrelOfs[1] = 100.0f;
                    barrelOfs[2] = 75.0f;
                    Vec_RotateY(barrelOfs, 6.2831855f * obj->timer / 65536.0f);
                    modelPos[0] += barrelOfs[0];
                    *modelPosY += barrelOfs[1];
                    *modelPosZ += barrelOfs[2];
                }
            } else if (obj->type == 8) {
                posSrc = ((u8 **) AreaWeapon_PartPos[obj->type])[obj->var2];
                posSrc += modelOfs;
                Vec_Copy((f32 *)modelPos, (f32 *)posSrc);
                *modelPosZ += obj->pos[2];
            } else {
                Vec_Copy((f32 *)modelPos, (f32 *)drawSlot->pos);
            }
            Mtx_Translation(mtx, modelPos[0], *modelPosY, *modelPosZ);
            switch (drawKind) {
            case 2:
                switch (obj->type) {
                case 1:
                case 6:
                    Mtx_Identity(uvMtx);
                    Mtx_SetTrans(uvMtx, 0.0f, 1.0f - (obj->var1 & 7) / 8.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    break;
                default:
                case 2:
                    Mtx_RotateZ(mtx, 6.2831855f * (obj->var1 << 13) / 65536.0f);
                    break;
                case 8:
                    if (obj->var0 == 2) {
                        Mtx_RotateZ(mtx, 6.2831855f * (360.0f * (obj->timer / 4.0f)) / 360.0f);
                    }
                    break;
                }
                Mtx_RotateY(mtx, yaw);
                break;
            case 1:
                if (j == 1) {
                    if (obj->var3 >= 0) {
                        turn = 65536.0f;
                        pitchTurn = (s16) (u16) (s32) (0.5f + turn * (0.01f * (45.0f * obj->var3)) / 360.0f);
                    } else {
                        turn = 65536.0f;
                        pitchTurn = (s16) (u16) (s32) (0.5f + turn * (0.01f * (35.0f * obj->var3)) / 360.0f);
                    }
                    Mtx_RotateX(mtx, 6.2831855f * -pitchTurn / turn);
                }
                Mtx_RotateY(mtx, 6.2831855f * obj->timer / 65536.0f);
                break;
            default:
                Mtx_RotateY(mtx, yaw);
                break;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            mdl = stage->matSets + *modelId;
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
    }
}

/*
 * Returns the part's index among its area's mounted guns, which picks its
 * turning sound in AreaWeapon_GunTurnSounds: the part index less 2 in the
 * fortress's area 5 (found by its day area), else the part index.
 */
s32 AreaWeapon_GunIndex(Obj *obj)
{
    s32 dayArea;
    u8 firstGun;

    dayArea = Area_DayId(gScene.area) & 0xFFFF;
    switch (dayArea) {
    case 12:
        firstGun = 2;
        break;
    case 25:
        firstGun = 0;
        break;
    default:
        firstGun = 0;
        break;
    }
    return (obj->var2 - firstGun) & 0xFF;
}

/*
 * Makes the area's AnimatedProps object; Area_StartObjects calls it in the
 * forest's area 11, the snowy mountains' areas 6 and night 5, 6 and 8, and
 * the battle town's area 0.
 */
void AnimatedProps_Create(void)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 7;
        obj->type = 0;
        obj->update = AnimatedProps_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AnimatedProps_Setup, _Run, _End and _Free.
 */
void AnimatedProps_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AnimatedProps_Setup(obj);
        break;
    case 1:
        AnimatedProps_Run(obj);
        break;
    case 2:
        AnimatedProps_End(obj);
        break;
    case 3:
        AnimatedProps_Free(obj);
        break;
    }
}

/*
 * Returns ifState0 when weather is 0 (or any other value), ifState1 when it
 * is 1 and ifState2 when it is 2 (Area_MonsterState: 2 while a Kushala Daora
 * is in the area, else 0 or 1 by the quest time).
 */
static s32 AnimatedProps_PickByWeather(s16 weather, s32 ifState0, s32 ifState1, s32 ifState2)
{
    switch (weather) {
    default:
        return ifState0;
    case 1:
        return ifState1;
    case 2:
        return ifState2;
    }
}

/*
 * Sets the props up for the current area from its position table: per prop a
 * state (how AnimatedProps_Run moves it) and a level (the alpha it draws
 * with: by the weather, by the set-down delivery, or full), and draw slots
 * for up to slotMax props, skipping those another prop draws with its own. In
 * another area, or without the first slot, it frees itself; else its first
 * frame runs. The object comes in as a void * and is viewed as an Obj, as the
 * game's code does.
 */
void AnimatedProps_Setup(void *objArg)
{
    Obj *obj = (Obj *) objArg;
    s16 weather;
    s16 slotCount;
    s16 propCount;
    s16 slotMax;
    f32 *pos;
    DrawSlot *drawSlot;
    s16 i;
    u8 skip;
    AnimatedPropsBuf *props = obj->buf;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    slotCount = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        propCount = slotMax = 8;
        pos = AnimatedProps_PosForestHills11;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
        propCount = 6;
        slotMax = 2;
        pos = AnimatedProps_PosSnowyMountains6;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
        slotMax = 1;
        propCount = 4;
        pos = AnimatedProps_PosSnowyNight5;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        propCount = 7;
        slotMax = 2;
        pos = AnimatedProps_PosSnowyNight6;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        slotMax = 1;
        propCount = 3;
        pos = AnimatedProps_PosSnowyNight8;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        propCount = 13;
        slotMax = propCount;
        pos = AnimatedProps_PosBattleTown0;
        weather = Area_MonsterState(gScene.area);
        break;
    default:
        AnimatedProps_Free(obj);
        return;
    }
    for (i = 0; i < 13; i++, pos += 3) {
        if (i < propCount) {
            skip = 0;
            switch (gScene.area) {
            case AREA_SNOWY_MOUNTAINS_6:
                switch (i) {
                case 0:
                    props->state[i] = 1;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0, 128, 255);
                    break;
                case 5:
                    props->state[i] = 2;
                    if (QuestGoal_DeliveryDone() == 0) {
                        props->level[i] = 0;
                    } else {
                        props->level[i] = 255;
                    }
                    break;
                default:
                    props->state[i] = 0;
                    skip = 1;
                    props->level[i] = 255;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_5:
                switch (i) {
                case 0:
                    props->state[i] = 0;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 255, 128, 0);
                    break;
                case 1:
                    skip = 1;
                    props->state[i] = 3;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 255, 128, 0);
                    break;
                default:
                    props->state[i] = 1;
                    skip = 1;
                    props->level[i] = 255;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_6:
                switch (i) {
                case 0:
                    props->state[i] = 0;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 255, 128, 0);
                    break;
                case 1:
                    skip = 1;
                    props->state[i] = 3;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 255, 128, 0);
                    break;
                case 6:
                    props->state[i] = 2;
                    props->level[i] = 0;
                    break;
                default:
                    props->state[i] = 1;
                    skip = 1;
                    props->level[i] = 255;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_8:
                switch (i) {
                case 0:
                    props->state[i] = 0;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 255, 128, 0);
                    break;
                case 1:
                    skip = 1;
                    props->state[i] = 3;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 255, 128, 0);
                    break;
                default:
                    props->state[i] = 1;
                    skip = 1;
                    props->level[i] = 255;
                    break;
                }
                break;
            case AREA_BATTLE_TOWN_0:
            case AREA_BATTLE_TOWN_NIGHT_0:
                switch (i) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    props->state[i] = 0;
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    props->state[i] = 1;
                    break;
                default:
                    props->state[i] = 2;
                    break;
                }
                break;
            }
            props->rot[i] = 0;
            if (slotCount < slotMax) {
                if (skip == 0) {
                props->slot[i] = DrawSlot_Alloc();
                if (props->slot[i] != -1) {
                    drawSlot = DrawSlot_Get(props->slot[i]);
                    props->ds[i] = drawSlot;
                    drawSlot->owner = obj;
                    drawSlot->part = i;
                    drawSlot->draw = AnimatedProps_Draw;
                    slotCount++;
                    drawSlot->pos[0] = pos[0];
                    drawSlot->pos[1] = pos[1];
                    drawSlot->pos[2] = pos[2];
                } else {
                    props->ds[i] = NULL;
                    if (slotCount == 0) {
                        AnimatedProps_Free(obj);
                        return;
                    }
                }
                } else {
                    props->ds[i] = NULL;
                }
            } else {
                props->ds[i] = NULL;
            }
        } else {
            props->ds[i] = NULL;
        }
    }
    AnimatedProps_Run(objArg);
}

/*
 * Every frame, by area: in the snowy mountains eases each weather-driven
 * prop's level by 8 toward its weather's level (AnimatedProps_PickByWeather),
 * turns the spinning props (AnimatedProps_Spin*) and brings the delivery prop
 * in by 8 once a set-down delivery is done; in the battle town's area 0
 * pulses the glows between 207 and 255. Then adds each prop that has a draw
 * slot to the 8-entry list or the sorted list.
 */
void AnimatedProps_Run(Obj *obj)
{
    AreaState *areaState = &gAreaState;
    s16 i;
    AnimatedPropsBuf *props = obj->buf;
    u16 mode;
    u8 goal;
    u8 *levelPtr;

    obj->timer++;
    for (i = 0; i < 13; i++) {
        mode = 0;
        switch (gScene.area) {
        case AREA_SNOWY_MOUNTAINS_6:
            switch (props->state[i]) {
            default:
                props->rot[i] += AnimatedProps_SpinSnowyMountains6[i];
                mode = 3;
                break;
            case 1:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 0, 128, 255);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
                mode = 3;
                break;
            case 2:
                if (QuestGoal_DeliveryDone() == 0) {
                    props->level[i] = 0;
                } else if (props->level[i] < 247) {
                    props->level[i] += 8;
                } else {
                    props->level[i] = 255;
                }
                mode = 0;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_5:
            switch (props->state[i]) {
            case 0:
            case 3:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 255, 128, 0);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
                mode = 3;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_6:
            switch (props->state[i]) {
            case 0:
            case 3:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 255, 128, 0);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
            default:
            case 1:
                props->rot[i] += AnimatedProps_SpinSnowyNight6[i];
                mode = 3;
                break;
            case 2:
                if (QuestGoal_DeliveryDone() == 0) {
                    props->level[i] = 0;
                } else if (props->level[i] < 247) {
                    props->level[i] += 8;
                } else {
                    props->level[i] = 255;
                }
                mode = 0;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_8:
            switch (props->state[i]) {
            case 0:
            case 3:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 255, 128, 0);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
                mode = 3;
                break;
            }
            break;
        case AREA_BATTLE_TOWN_0:
        case AREA_BATTLE_TOWN_NIGHT_0:
            switch (props->state[i]) {
            case 1:
                props->level[i] = (u8) (24.0f * (1.0f + Math_Sin(6.2831855f * (areaState->frame << 12) / 65536.0f))) + 207;
                mode = 0;
                break;
            }
            break;
        }
        if (props->ds[i] != NULL) {
            switch (mode) {
            default:
                OT_AddByDepth(gDrawOtSorted, props->ds[i], 64, 0);
                break;
            case 3:
                OT_AddByDepth(gDrawOt8, props->ds[i], 8, 1);
                break;
            }
        }
    }
}

/* Hides the object; it is freed on the next update. */
void AnimatedProps_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AnimatedProps_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* The props' buffer as AnimatedProps_Draw names it: the AnimatedPropsBuf layout, kind being a
 * prop's state, alpha its level and ang its turn. */
typedef struct AnimatedPropsDrawView {
    u8 kind[13];                /* 0x00 */
    u8 alpha[13];               /* 0x0D */
    u16 ang[8];                 /* 0x1A */
} AnimatedPropsDrawView;

extern f32 AnimatedProps_ScalesForestHills11[];
extern s16 AnimatedProps_ModelsSnowyMountains6[];
extern s16 AnimatedProps_ModelsSnowyNight5[];
extern s16 AnimatedProps_ModelsSnowyNight6[];
extern s16 AnimatedProps_ModelsSnowyNight8[];
extern s16 AnimatedProps_ModelsBattleTown0[];

/*
 * Draws a prop by area, with no alpha test: in the forest's area 11 a camera-
 * yaw billboard stepping a 4 by 4 texture flipbook; in the snowy areas all
 * the area's props together, turned by their spin or scrolled and faded by
 * their level, or the delivery prop as a camera-yaw billboard scrolling up
 * without depth writes; in the battle town's area 0 camera-facing flames on a
 * 4 by 4 flipbook and glows faded by their level, from the area model
 * (gAreaState.model) with its lights.
 */
void AnimatedProps_Draw(DrawSlot *drawSlot)
{
    Model *stage = gAreaSubModels[0];
    Obj *obj = drawSlot->owner;
    AnimatedPropsDrawView *props = obj->buf;
    ModelMatSet *mdl;
    AreaState *areaState = &gAreaState;
    s16 i;
    f32 scrollU;
    f32 mtx[16];
    f32 uvMtx[16];

    switch (gScene.area) {
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        stage = areaState->model;
        break;
    default:
        if (stage == NULL) {
            return;
        }
        if (stage->active == 0) {
            return;
        }
        break;
    }
    switch (gScene.area) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        mdl = stage->matSets + 4;
        Mtx_Translation(uvMtx, 0.25f * ((obj->timer >> 1) & 3), 0.25f * ((obj->timer >> 3) & 3), 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_Scale(mtx, AnimatedProps_ScalesForestHills11[drawSlot->part * 3], AnimatedProps_ScalesForestHills11[drawSlot->part * 3 + 1], AnimatedProps_ScalesForestHills11[drawSlot->part * 3 + 2]);
        Mtx_Concat3x3(mtx, gCamYawMtx);
        goto draw;
    case AREA_SNOWY_MOUNTAINS_6:
        Gs_SetState(RS_ALPHA_REF, 0);
        switch (props->kind[drawSlot->part]) {
        case 0:
        case 1:
            for (i = 0; i < 5; i++) {
                mdl = stage->matSets + AnimatedProps_ModelsSnowyMountains6[i];
                Mtx_Translation(mtx, AnimatedProps_PosSnowyMountains6[i * 3], AnimatedProps_PosSnowyMountains6[i * 3 + 1], AnimatedProps_PosSnowyMountains6[i * 3 + 2]);
                if (props->kind[i] != 1) {
                    Mtx_RotateY(mtx, 6.2831855f * props->ang[i] / 65536.0f);
                } else {
                    Mtx_Translation(uvMtx, (obj->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                }
                Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                if (mdl != NULL && mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Model_Draw(mdl->id, 0);
                }
            }
            Draw_SetDefaultState();
            return;
        case 2:
        default:
            Gs_SetState(RS_ZWRITE, 0);
            mdl = stage->matSets + AnimatedProps_ModelsSnowyMountains6[drawSlot->part];
            Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            Mtx_Concat3x3(mtx, gCamYawMtx);
            Gs_SetState(RS_COLOR2, (props->alpha[drawSlot->part] << 24) | 0xFFFFFF);
            Mtx_Translation(uvMtx, 0.0f, (obj->timer & 0xFF) / 256.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            goto draw;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
        Gs_SetState(RS_ALPHA_REF, 0);
        for (i = 0; i < 4; i++) {
            mdl = stage->matSets + AnimatedProps_ModelsSnowyNight5[i];
            Mtx_Translation(mtx, AnimatedProps_PosSnowyNight5[i * 3], AnimatedProps_PosSnowyNight5[i * 3 + 1], AnimatedProps_PosSnowyNight5[i * 3 + 2]);
            switch (props->kind[i]) {
            case 3:
                Mtx_Translation(uvMtx, (obj->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            case 0:
                Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                break;
            case 1:
            default:
                if (i == 2) {
                    scrollU = (obj->timer & 0xFF) / 256.0f;
                } else {
                    scrollU = (obj->timer & 0x3F) / 64.0f;
                }
                Mtx_Translation(uvMtx, scrollU, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
        return;
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        Gs_SetState(RS_ALPHA_REF, 0);
        switch (props->kind[drawSlot->part]) {
        case 0:
        default:
        case 1:
        case 3:
            for (i = 0; i < 6; i++) {
                mdl = stage->matSets + AnimatedProps_ModelsSnowyNight6[i];
                Mtx_Translation(mtx, AnimatedProps_PosSnowyNight6[i * 3], AnimatedProps_PosSnowyNight6[i * 3 + 1], AnimatedProps_PosSnowyNight6[i * 3 + 2]);
                switch (props->kind[i]) {
                case 3:
                    Mtx_Translation(uvMtx, (obj->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                case 0:
                    Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                    break;
                case 1:
                default:
                    Mtx_RotateY(mtx, 6.2831855f * props->ang[i] / 65536.0f);
                    break;
                }
                Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                if (mdl != NULL && mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Model_Draw(mdl->id, 0);
                }
            }
            Draw_SetDefaultState();
            return;
        case 2:
            Gs_SetState(RS_ZWRITE, 0);
            mdl = stage->matSets + AnimatedProps_ModelsSnowyNight6[drawSlot->part];
            Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            Mtx_Concat3x3(mtx, gCamYawMtx);
            Gs_SetState(RS_COLOR2, (props->alpha[drawSlot->part] << 24) | 0xFFFFFF);
            Mtx_Translation(uvMtx, 0.0f, (obj->timer & 0xFF) / 256.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            goto draw;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        Gs_SetState(RS_ALPHA_REF, 0);
        for (i = 0; i < 3; i++) {
            mdl = stage->matSets + AnimatedProps_ModelsSnowyNight8[i];
            Mtx_Translation(mtx, AnimatedProps_PosSnowyNight8[i * 3], AnimatedProps_PosSnowyNight8[i * 3 + 1], AnimatedProps_PosSnowyNight8[i * 3 + 2]);
            switch (props->kind[i]) {
            case 3:
                Mtx_Translation(uvMtx, (obj->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            case 0:
            default:
                Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                break;
            case 1:
                Mtx_Translation(uvMtx, (obj->timer & 0x7F) / 128.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
        return;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        mdl = stage->matSets + AnimatedProps_ModelsBattleTown0[drawSlot->part];
        Gs_SetLightList(stage->mats, mdl);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Gs_SetState(RS_ZWRITE, 0);
        switch (AnimatedProps_ModelsBattleTown0[drawSlot->part]) {
        case 13:
        case 15:
        default:
            Mtx_Translation(uvMtx, 0.25f * (areaState->frame & 3), 0.25f * ((areaState->frame >> 2) & 3), 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            Gs_SetState(RS_COLOR2, -1);
            break;
        case 14:
            Gs_SetState(RS_COLOR2, (props->alpha[drawSlot->part] << 24) | 0xFFFFFF);
            break;
        }
        goto draw;
    }
    return;
draw:
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
    if (mdl != NULL && mdl->id != -1) {
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(RS_ZWRITE, 1);
    return;
}

/*
 * Makes the area's AnimatedGroundTiles object; Area_StartObjects calls it in
 * the swamp's area 5, day and night.
 */
void AnimatedGroundTiles_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 8;
        obj->type = 0;
        obj->update = AnimatedGroundTiles_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AnimatedGroundTiles_Setup, _Run, _End and _Free.
 */
void AnimatedGroundTiles_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AnimatedGroundTiles_Setup(obj);
        break;
    case 1:
        AnimatedGroundTiles_Run(obj);
        break;
    case 2:
        AnimatedGroundTiles_End(obj);
        break;
    case 3:
        AnimatedGroundTiles_Free(obj);
        break;
    }
}

/*
 * Sets the tiles up at the origin with a draw slot
 * (AnimatedGroundTiles_Draw), or frees itself without one; then the first
 * frame runs.
 */
void AnimatedGroundTiles_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->var3 = 0;
    obj->pos[0] = 0.0f;
    obj->pos[1] = 0.0f;
    obj->pos[2] = 0.0f;
    obj->timer = 0;
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AnimatedGroundTiles_Draw;
        AnimatedGroundTiles_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Counts frames and follows the local player in x and z at height 0, adding
 * the tiles to the 32-entry draw list.
 */
void AnimatedGroundTiles_Run(Obj *obj)
{
    Player *player;

    player = &gPlayers[gScene.localPlayer];
    obj->timer++;
    obj->pos[0] = player->pos[0];
    obj->pos[1] = 0.0f;
    obj->pos[2] = player->pos[2];
    if (obj->draw == NULL) {
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt32, obj->ds, 32, 1);
    }
}

/* Hides the object; it is freed on the next update. */
void AnimatedGroundTiles_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AnimatedGroundTiles_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws two layers, the area's first two models, on a 3 by 3 grid of
 * 1000-unit cells around the object, with alpha test reference 128 and
 * Gfx_SetTexFilter(GFX_TEX_BILINEAR); each layer steps through a 4 by 4 texture atlas every
 * 2 frames (AnimatedGroundTiles_UvSteps, 16 steps).
 */
void AnimatedGroundTiles_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *mdl;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 uvU;
    f32 cellX;
    f32 firstCellZ;
    f32 uvV;
    f32 z;
    s32 layer;
    s32 i;
    s32 j;

    if (stage != NULL && stage->active != 0) {
        mdl = stage->matSets;
        Gs_SetState(RS_ALPHA_REF, 128);
        obj->fvar = 1000.0f;
        for (layer = 0; layer < 2; mdl++, layer++) {
            uvU = AnimatedGroundTiles_UvSteps[((obj->timer & 0x1E) >> 1) * 4 + layer * 2];
            uvV = AnimatedGroundTiles_UvSteps[((obj->timer & 0x1E) >> 1) * 4 + layer * 2 + 1];
            uvU -= Math_Floor(uvU);
            uvV -= Math_Floor(uvV);
            Mtx_Translation(uvMtx, uvU, uvV, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            cellX = 1000.0f * Math_Floor((500.0f + obj->pos[0]) / 1000.0f) - 1000.0f;
            firstCellZ = 1000.0f * Math_Floor((500.0f + obj->pos[2]) / 1000.0f) - 1000.0f;
            for (i = 0; i < 3; i++, cellX += obj->fvar) {
                for (j = 0, z = firstCellZ; j < 3; j++, z += obj->fvar) {
                    Mtx_Translation(mtx, cellX, 0.0f, z);
                    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                    if (mdl->id != -1) {
                        Draw_ApplyStates(mdl->flags);
                        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
                        Model_Draw(mdl->id, 0);
                    }
                }
            }
        }
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
        Gs_SetState(RS_ALPHA_REF, 0);
        Draw_SetDefaultState();
    }
}

/* The file's data. */
f32 LightShaft_PosForestHills7[3] = {
#include "game/008B00B8.inc"
};
s16 LightShaft_TiltsForestHills7[4] = {
#include "game/008B00C8.inc"
};
f32 LightShaft_ScalesForestHills7[2] = {
#include "game/008B00D0.inc"
};
f32 LightShaft_ScalesForestHills7Z[2] = {
#include "game/008B00D8.inc"
};
f32 LightShaft_PosForestHills11[6] = {
#include "game/008B00E0.inc"
};
s16 LightShaft_TiltsForestHills11[4] = {
#include "game/008B00F8.inc"
};
f32 LightShaft_WidthsForestHills11[2] = {
#include "game/008B0100.inc"
};
f32 LightShaft_PosForestHills12[3] = {
#include "game/008B0108.inc"
};
s16 LightShaft_TiltsForestHills12[4] = {
#include "game/008B0118.inc"
};
f32 LightShaft_PosSnowyMountains1[4] = {
#include "game/008B0120.inc"
};
f32 LightShaft_PosSnowyMountains3[4] = {
#include "game/008B0130.inc"
};
f32 LightShaft_PosTower8[8] = {
#include "game/008B0140.inc"
};
f32 LightShaft_PosDesert4[6] = {
#include "game/008B0160.inc"
};
s16 LightShaft_ModelsDesert4[4] = {
#include "game/008B0178.inc"
};
f32 LightShaft_PosSwamp3[8] = {
#include "game/008B0180.inc"
};
f32 LightShaft_PosSwamp4[6] = {
#include "game/008B01A0.inc"
};
s16 SkyStreak_Counts[4] = {
#include "game/008B01B8.inc"
};
s16 SkyStreak_RandRange[4] = {
#include "game/008B01C0.inc"
};
s16 SkyStreak_WaitBase[4] = {
#include "game/008B01C8.inc"
};
u16 SkyStreak_WaitMask[4] = {
#include "game/008B01D0.inc"
};
s16 SkyStreak_Frames[4] = {
#include "game/008B01D8.inc"
};
s16 SkyStreak_Models[4] = {
#include "game/008B01E0.inc"
};
f32 AreaDrop_ZonesDesertNight4[3] = {
#include "game/008B01E8.inc"
};
f32 AreaDrop_ZoneRadiiDesertNight4[2] = {
#include "game/008B01F8.inc"
};
f32 AreaDrop_ZonesTower8[4] = {
#include "game/008B0200.inc"
};
f32 AreaDrop_ZoneRadiiTower8[2] = {
#include "game/008B0210.inc"
};
f32 AreaDrop_ZonesDesert4[3] = {
#include "game/008B0218.inc"
};
f32 AreaDrop_ZoneRadiiDesert4[2] = {
#include "game/008B0228.inc"
};
f32 AreaDrop_FallPointsDesert10[12] = {
#include "game/008B0230.inc"
};
f32 AreaDrop_ZonesSwamp3[6] = {
#include "game/008B0260.inc"
};
f32 AreaDrop_ZoneRadiiSwamp3[2] = {
#include "game/008B0278.inc"
};
f32 AreaDrop_ZonesSwamp4[6] = {
#include "game/008B0280.inc"
};
f32 AreaDrop_ZoneRadiiSwamp4[2] = {
#include "game/008B0298.inc"
};
f32 AreaDrop_ZonesSwamp8[6] = {
#include "game/008B02A0.inc"
};
f32 AreaDrop_ZoneRadiiSwamp8[2] = {
#include "game/008B02B8.inc"
};
u16 AreaWeapon_GunTurnSounds[4] = {
#include "game/008B02C0.inc"
};
u8 AreaWeapon_PartKindsFortress4[8] = {
#include "game/008B02C8.inc"
};
f32 AreaWeapon_PosFortress4[10] = {
#include "game/008B02D0.inc"
};
u8 AreaWeapon_PartKindsFortress5[8] = {
#include "game/008B02F8.inc"
};
f32 AreaWeapon_PosFortress5_0[4] = {
#include "game/008B0300.inc"
};
f32 AreaWeapon_PosFortress5_1[4] = {
#include "game/008B0310.inc"
};
f32 AreaWeapon_PosFortress5_2[4] = {
#include "game/008B0320.inc"
};
f32 AreaWeapon_PosFortress5_3[4] = {
#include "game/008B0330.inc"
};
f32 AreaWeapon_PosFortress5_4[4] = {
#include "game/008B0340.inc"
};
f32 *AreaWeapon_PosFortress5[8] = {
    AreaWeapon_PosFortress5_0, AreaWeapon_PosFortress5_1, AreaWeapon_PosFortress5_2, AreaWeapon_PosFortress5_3,
    AreaWeapon_PosFortress5_4, NULL, NULL, NULL,
};
f32 AreaWeapon_BurstPosFortress5[12] = {
#include "game/008B0370.inc"
};
u8 AreaWeapon_PartKindsCastle1[8] = {
#include "game/008B03A0.inc"
};
f32 AreaWeapon_PosCastle1_0[3] = {
#include "game/008B03A8.inc"
};
f32 AreaWeapon_PosCastle1_1[3] = {
#include "game/008B03B8.inc"
};
f32 AreaWeapon_PosCastle1_2[3] = {
#include "game/008B03C8.inc"
};
f32 *AreaWeapon_PosCastle1[3] = {
    AreaWeapon_PosCastle1_0, AreaWeapon_PosCastle1_1, AreaWeapon_PosCastle1_2,
};
u16 AreaWeapon_YawCastle1[4] = {
#include "game/008B03E8.inc"
};
f32 AreaWeapon_BurstPosCastle1[4] = {
#include "game/008B03F0.inc"
};
u8 AreaWeapon_PartKindsFortress3[16] = {
#include "game/008B0400.inc"
};
f32 AreaWeapon_PosFortress3[6] = {
#include "game/008B0410.inc"
};
u8 AreaWeapon_PartKindsFortress2[8] = {
#include "game/008B0428.inc"
};
f32 AreaWeapon_PosFortress2[10] = {
#include "game/008B0430.inc"
};
u8 AreaWeapon_PartKindsBattleTown1[8] = {
#include "game/008B0458.inc"
};
f32 AreaWeapon_PosBattleTown1_0[4] = {
#include "game/008B0460.inc"
};
f32 AreaWeapon_PosBattleTown1_1[4] = {
#include "game/008B0470.inc"
};
f32 AreaWeapon_PosBattleTown1_2[6] = {
#include "game/008B0480.inc"
};
f32 *AreaWeapon_PosBattleTown1[3] = {
    AreaWeapon_PosBattleTown1_0, AreaWeapon_PosBattleTown1_1, AreaWeapon_PosBattleTown1_2,
};
u16 AreaWeapon_GunRestYawBattleTown1[4] = {
#include "game/008B04A8.inc"
};
f32 AreaWeapon_BurstPosBattleTown1[6] = {
#include "game/008B04B0.inc"
};
u8 AreaWeapon_PartKindsBattleTown3[8] = {
#include "game/008B04C8.inc"
};
f32 AreaWeapon_PosBattleTown3_0[4] = {
#include "game/008B04D0.inc"
};
f32 AreaWeapon_PosBattleTown3_1[4] = {
#include "game/008B04E0.inc"
};
f32 AreaWeapon_PosBattleTown3_2[4] = {
#include "game/008B04F0.inc"
};
f32 AreaWeapon_PosBattleTown3_3[4] = {
#include "game/008B0500.inc"
};
f32 *AreaWeapon_PosBattleTown3[4] = {
    AreaWeapon_PosBattleTown3_0, AreaWeapon_PosBattleTown3_1, AreaWeapon_PosBattleTown3_2, AreaWeapon_PosBattleTown3_3,
};
u16 AreaWeapon_DayAreas[16] = {
#include "game/008B0520.inc"
};
f32 *AreaWeapon_PartPos[12] = {
    AreaWeapon_PosFortress4, (f32 *)AreaWeapon_PosFortress5, (f32 *)AreaWeapon_PosCastle1, AreaWeapon_PosFortress3,
    AreaWeapon_PosFortress2, AreaWeapon_PosFortress4, (f32 *)AreaWeapon_PosFortress5, AreaWeapon_PosFortress2,
    (f32 *)AreaWeapon_PosBattleTown1, (f32 *)AreaWeapon_PosBattleTown3, NULL, NULL,
};
u8 *AreaWeapon_PartKinds[12] = {
    AreaWeapon_PartKindsFortress4, AreaWeapon_PartKindsFortress5, AreaWeapon_PartKindsCastle1, AreaWeapon_PartKindsFortress3,
    AreaWeapon_PartKindsFortress2, AreaWeapon_PartKindsFortress4, AreaWeapon_PartKindsFortress5, AreaWeapon_PartKindsFortress2,
    AreaWeapon_PartKindsBattleTown1, AreaWeapon_PartKindsBattleTown3, NULL, NULL,
};
s16 AreaWeapon_NoModels[24] = {
#include "game/008B05A0.inc"
};
s16 AreaWeapon_GunModels[24] = {
#include "game/008B05D0.inc"
};
s16 AreaWeapon_GreatWeaponModels[24] = {
#include "game/008B0600.inc"
};
s16 AreaWeapon_FixedModels[24] = {
#include "game/008B0630.inc"
};
s16 *AreaWeapon_Models[4] = {
    AreaWeapon_NoModels, AreaWeapon_GunModels, AreaWeapon_GreatWeaponModels, AreaWeapon_FixedModels,
};
f32 AnimatedProps_PosForestHills11[24] = {
#include "game/008B0670.inc"
};
f32 AnimatedProps_ScalesForestHills11[24] = {
#include "game/008B06D0.inc"
};
f32 AnimatedProps_PosSnowyMountains6[18] = {
#include "game/008B0730.inc"
};
s16 AnimatedProps_ModelsSnowyMountains6[6] = {
#include "game/008B0778.inc"
};
s16 AnimatedProps_SpinSnowyMountains6[6] = {
#include "game/008B0788.inc"
};
s16 AnimatedProps_ModelsSnowyNight5[4] = {
#include "game/008B0798.inc"
};
f32 AnimatedProps_PosSnowyNight6[26] = {
#include "game/008B07A0.inc"
};
s16 AnimatedProps_SpinSnowyNight6[6] = {
#include "game/008B0808.inc"
};
s16 AnimatedProps_ModelsSnowyNight8[4] = {
#include "game/008B0818.inc"
};
f32 AnimatedProps_PosBattleTown0[40] = {
#include "game/008B0820.inc"
};
s16 AnimatedProps_ModelsBattleTown0[16] = {
#include "game/008B08C0.inc"
};
f32 AnimatedGroundTiles_UvSteps[64] = {
#include "game/008B08E0.inc"
};
