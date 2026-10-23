/*
 * The area objects of the later kinds: objects of the area's own list (Obj,
 * from Obj_Alloc) that Area_StartObjects starts when an area is entered, by
 * the area's id or by gAreaInfo's set-up ids. Each kind has a Create
 * function, an Update that runs the state Obj.state names (0 set up, 1 run,
 * 2 end, 3 free) and a Draw that its
 * draw slot calls (DrawSlot_Alloc; the run state adds the slot to a
 * depth-sorted list with OT_AddByDepth every frame). The models come from
 * the area's sub-model (gAreaSubModels[0]) or the area model
 * (gAreaState.model).
 *
 * The kinds: FortressTremor, BreakableStructure, ScrollingOverlay,
 * TurningProp, GroundTileGrid, AnimatedScenery, CastleGate and
 * DistantLightning, then two more:
 * - the quest-end banner (kind 23, AreaObj_K17_*): Scene_ResultFade creates
 *   it at the end of a quest with the quest phase. It draws a full-screen
 *   picture and the phase's mark over the darkened screen, starts the
 *   phase's event tune and ends after 300 frames;
 * - the volcano's hazard spawner (kind 25, AreaObj_K19_*): Area_StartObjects
 *   creates it in AREA_VOLCANO_1, 2, 5 and 6. It starts a VolcanoHazard
 *   attack and its stage object at one of the area's points every 300
 *   frames.
 * The file ends with their tables of positions, models, texture steps and
 * points. The Em views and prototypes declared at its top (EmReaction,
 * EmReactView, EmSenseView, EmSight, EmBendView) are used by no function
 * here.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "obj.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"
#include "area.h"
#include "bgm.h"
#include "objattach.h"
#include "render_state.h"
#include "camera.h"
#include "capsule.h"
#include "card.h"
#include "em_bulldrome.h"
#include "em_diablos.h"
#include "em_drome.h"
#include "em_vespoid.h"
#include "sound.h"
#include "text.h"
#include "ambient_life.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "game_util.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "volcano_hazard.h"
#include "cam_shake.h"
#include "em_anteka.h"
#include "em_apceros.h"
#include "em_blangonga.h"
#include "em_cephadrome.h"
#include "em_chameleos.h"
#include "em_fatalis.h"
#include "em_garuga.h"
#include "em_gravios.h"
#include "em_hermitaur.h"
#include "em_khezu.h"
#include "em_kind38.h"
#include "em_kirin.h"
#include "em_kushala.h"
#include "em_kutku.h"
#include "em_laoshan.h"
#include "em_melynx.h"
#include "em_mosswine.h"
#include "em_plesioth.h"
#include "em_rathalos.h"
#include "em_remobra.h"
#include "em_shakalaka.h"
#include "em_shengaoren.h"
#include "em_velociprey.h"
#include "em_yamatsukami.h"
#include "prim.h"
#include "sprite_queue.h"

/* Built at optimization level 4, as the game's code was; DistantLightning_Draw alone is built
 * at level 3. */
#pragma optimization_level 4

extern EmObj *gSightBlockObjs[32];


extern EmRate *gEmRateTbl[];

/* A view of Em at its per-player reaction values: four s16, one per player, then their last-seen
 * copies (Em_UpdateReactions runs the reaction of a player whose value changed). */
typedef struct EmReactView {
    u8 pad[1732];
    s16 cur[4];
    s16 last[4];
} EmReactView;

/* A view of Em at what Em_SensePlayers measures of each player every frame. */
typedef struct EmSenseView {
    u8 pad[2020];
    s16 noticeTimer[4];              /* 0x7E4: Em unk7E4 */
    u8 pad7EC[2072 - 2028];
    f32 dist[4];                /* 0x818: from the monster to the player */
    f32 distXZ[4];              /* 0x828: the same, in the ground plane */
    f32 range[4];               /* 0x838: the four reaction ranges (Em_GetReaction kinds 3 to 6) */
    s32 alertBuildup[4];              /* 0x848: Em unk848, cleared when the player is out of reach */
    u16 dir[4];                 /* 0x858: the direction to the player (Math_AngleXZ) */
} EmSenseView;

/* prototypes */
extern f32 CastleGate_SoundPos[];
extern f32 D_008B149C[3];
void StageShot_CreateAt(f32 *pos, u8 kind, u8 area, s32 ang);
s32 Em_PartBroken(Em *em, s32 arg1);
extern f32 D_008B0B8C[];
extern f32 BreakableStructure_EffectPos[][3];
extern EmObj *gFlashObjs[32];
typedef struct EmSight {
    f32 *pos;
    s32 target;
    s32 dir;
    u16 half;
} EmSight;
u8 AreaHit_Raycast(f32 *a, f32 *b, f32 *out, u16 mask);
extern f32 D_008B0A94[];
extern f32 D_008B0A98[];
extern f32 D_008B0AC4[];
extern f32 D_008B0AC8[];
extern f32 CastleGate_PosOpen[][3];
extern f32 CastleGate_PosShut[][3];
typedef struct BreakableStructurePiece {
    s16 slot;
    s16 idx;
    f32 pos[3];
    u16 ang;
    u8 pad12[2];
    DrawSlot *ds;
} BreakableStructurePiece;
extern f32 BreakableStructure_Pos[][3];
extern f32 D_008B0A14[];
extern f32 D_008B0A18[];
extern f32 D_008B0AAC;
extern f32 TurningProp_Pos[][3];
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
extern f32 gCamMatrix[];
struct Segment;
EmReaction *Em_GetReaction(Em *em, u8 kind, u8 player);
struct TexRect;
extern EmObj *gAreaObjList[32];
extern f32 gViewMatrix[];
extern f32 gFrustumPlanes[];
typedef struct EmBendView {
    u8 pad[1596];
    u32 bend[4];
} EmBendView;
/* end of prototypes */

/*
 * Creates the FortressTremor object (kind 10, no buffer); Area_StartObjects
 * calls it for AREA_FORTRESS_0 and its night.
 */
void FortressTremor_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 10;
        obj->type = 0;
        obj->update = FortressTremor_Update;
    }
}

/*
 * FortressTremor's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void FortressTremor_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        FortressTremor_Setup(obj);
        break;
    case 1:
        FortressTremor_Run(obj);
        break;
    case 2:
        FortressTremor_End(obj);
        break;
    case 3:
        FortressTremor_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the object at FortressTremor_PosX,
 * FortressTremor_StartY and FortressTremor_PosZ, takes a draw slot
 * (FortressTremor_Draw) and runs the first frame, which starts a tremor at
 * once; frees the object when no slot is free.
 */
void FortressTremor_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    obj->pos[0] = FortressTremor_PosX;
    obj->pos[1] = D_008B0AAC;
    obj->pos[2] = FortressTremor_PosZ;
    obj->timer = 0;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = FortressTremor_Draw;
        FortressTremor_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: when its wait (Obj.timer) runs out, a tremor: shakes the camera
 * for everyone, plays the rumble (area sound 96) at the local player and
 * drops its model 62.5 a frame for 15 frames (drawn through gDrawOt32),
 * playing an impact (area sound 97) at one of four
 * FortressTremor_ImpactOffsets around the player on the third; then waits 60
 * to 91 frames back at its start height.
 */
void FortressTremor_Run(Obj *obj)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 impactPos[4];
    s16 k;

    switch (obj->var0) {
    case 0:
        if (--obj->timer <= 0) {
            obj->var0++;
            obj->timer = 16;
            CamShake_SetAEverywhere(0);
            Sound_PlaySeAtPosChecked(7, 96, 0, pl->pos, 19, 0);
        }
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var0 = 0;
            obj->timer = (Rand_Next(1) & 0x1F) + 60;
            obj->pos[1] = D_008B0AAC;
            break;
        }
        if (obj->timer == 13) {
            k = Rand_Next(1) & 3;
            impactPos[0] = pl->pos[0] + FortressTremor_ImpactOffsets[k * 3];
            impactPos[1] = pl->pos[1] + D_008B0AC4[k * 3];
            impactPos[2] = pl->pos[2] + D_008B0AC8[k * 3];
            Sound_PlaySeAtPosChecked(7, 97, 0, impactPos, 19, 0);
        }
        obj->pos[1] -= 62.5f;
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt32, obj->ds, 32, 1);
        break;
    }
}

/* End state: hides the object; its draw slot is not freed. */
void FortressTremor_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void FortressTremor_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the first model of the area's sub-model at the draw slot, alpha test
 * reference 128.
 */
void FortressTremor_Draw(DrawSlot *drawSlot)
{
    f32 mtx[16];
    Model *subs;
    ModelMatSet *model;

    subs = gAreaSubModels[0];
    if ((subs != NULL) && (subs->active != 0)) {
        model = subs->matSets;
        Gs_SetState(RS_ALPHA_REF, 128);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the BreakableStructure object (kind 11, one buffer block for its
 * five BreakableStructurePiece entries); Area_StartObjects calls it for
 * AREA_FORTRESS_3 and its night.
 */
void BreakableStructure_Create(void)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 11;
        obj->type = 0;
        obj->update = BreakableStructure_Update;
    }
}

/*
 * BreakableStructure's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void BreakableStructure_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        BreakableStructure_Setup(obj);
        break;
    case 1:
        BreakableStructure_Run(obj);
        break;
    case 2:
        BreakableStructure_End(obj);
        break;
    case 3:
        BreakableStructure_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the five pieces (BreakableStructure_Pos); when bit 0
 * of gScene.emEventFlags is already set it starts broken (Obj.var0 2), each
 * piece raised by BreakableStructure_Rise and tilted by
 * BreakableStructure_Tilt. Gives each piece a draw slot
 * (BreakableStructure_Draw, DrawSlot.part its index; none when the pool is
 * full) and runs the first frame.
 */
void BreakableStructure_Setup(Obj *obj)
{
    BreakableStructurePiece *piece = obj->buf;
    s16 i;

    obj->state++;
    if (gScene.emEventFlags & 1) {
        obj->var0 = 2;
    } else {
        obj->var0 = 0;
    }
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    obj->timer = 0;
    for (i = 0; i < 5; i++, piece++) {
        piece->idx = i;
        piece->ang = 0;
        Vec_Copy(piece->pos, BreakableStructure_Pos[i]);
        if (obj->var0 == 2) {
            piece->pos[1] += BreakableStructure_Rise[i];
            piece->ang += (s32) (65536.0f * BreakableStructure_Tilt[i] / 360.0f + 0.5f);
        }
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->ds = DrawSlot_Get(piece->slot);
            piece->ds->owner = obj;
            piece->ds->part = i;
            piece->ds->draw = BreakableStructure_Draw;
        } else {
            piece->ds = NULL;
        }
    }
    BreakableStructure_Run(obj);
}

/*
 * Run state: whole (Obj.var0 0), it waits for bit 0 of gScene.emEventFlags (a
 * fortress strike of Lao-Shan Lung or Shen Gaoren), then plays the break
 * sounds (26, 27) of the first such monster shown in the area and spawns
 * three effects (Effect0D kind 0, scale 25); for the next 5 frames each piece
 * rises and tilts by a fifth of its amounts; then it stays broken. Draws
 * pieces 0-3 while whole, all five while breaking, piece 4 alone once broken
 * (by the step at the frame's start).
 */
void BreakableStructure_Run(Obj *obj)
{
    BreakableStructurePiece *piece = obj->buf;
    Em *em = gEms;
    s16 stepAtStart = obj->var0;
    s16 i;

    switch (obj->var0) {
    case 0:
        if (gScene.emEventFlags & 1) {
            for (i = 0; i < 20; i++, em++) {
                if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0
                    && (em->type == 7 || em->type == 50 || em->type == 55)) {
                    Em_PlaySeChecked(em, 26, 0, BreakableStructure_SoundPos, 8, 0);
                    Em_PlaySeChecked(em, 27, 0, D_008B0B8C, 8, 0);
                    break;
                }
            }
            for (i = 0; i < 3; i++) {
                PartFx_SpawnAt(BreakableStructure_EffectPos[i], 0, 25.0f);
            }
            obj->var0++;
        }
        break;
    case 1:
        if (++obj->timer >= 5) {
            obj->var0++;
        }
        break;
    case 2:
        break;
    }
    for (i = 0; i < 5; i++, piece++) {
        switch (stepAtStart) {
        case 0:
            if (piece->idx >= 4) {
                continue;
            }
            break;
        case 1:
            piece->pos[1] += BreakableStructure_Rise[piece->idx] / 5.0f;
            piece->ang += (s32) (65536.0f * (BreakableStructure_Tilt[piece->idx] / 5) / 360.0f + 0.5f);
            break;
        case 2:
            if (piece->idx < 4) {
                continue;
            }
            break;
        }
        if (piece->ds != NULL) {
            piece->ds->pos[0] = piece->pos[0];
            piece->ds->pos[1] = piece->pos[1];
            piece->ds->pos[2] = piece->pos[2];
            OT_AddByDepth(gDrawOt32, piece->ds, 32, 0);
        }
    }
}

/*
 * End state: hides the object and frees the draw slots its pieces hold (the
 * piece entries read by byte offset).
 */
void BreakableStructure_End(Obj *obj)
{
    u8 *pieceBytes = obj->buf;
    s16 i;

    obj->state++;
    obj->visible = 0;
    for (i = 0; i < 5; i++) {
        if (*(s32 *) (pieceBytes + 20) != 0) {
            DrawSlot_Free(*(s16 *) pieceBytes);
        }
        pieceBytes += 24;
    }
}

/* Free state: Obj_Free. */
void BreakableStructure_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws piece DrawSlot.part: its model (BreakableStructure_ModelsDay in
 * AREA_FORTRESS_3, BreakableStructure_ModelsNight in AREA_FORTRESS_NIGHT_3,
 * nothing elsewhere) at the slot, tilted about x by the piece's angle and
 * turned by BreakableStructure_Yaw, alpha test reference 128; skips a model
 * whose id is 0.
 */
void BreakableStructure_Draw(DrawSlot *drawSlot)
{
    BreakableStructurePiece *piece = &((BreakableStructurePiece *) ((Obj *) drawSlot->owner)->buf)[drawSlot->part];
    Model *stage = gAreaSubModels[0];
    f32 mtx[16];
    ModelMatSet *mdl;

    if (stage != NULL && stage->active != 0) {
        switch (gScene.area) {
        case AREA_FORTRESS_3:
            mdl = &stage->matSets[BreakableStructure_ModelsDay[piece->idx]];
            break;
        case AREA_FORTRESS_NIGHT_3:
            mdl = &stage->matSets[BreakableStructure_ModelsNight[piece->idx]];
            break;
        default:
            return;
        }
        Gs_SetState(RS_ALPHA_REF, 128);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_RotateXYZ(mtx, (6.2831855f * piece->ang) / 65536.0f, (6.2831855f * BreakableStructure_Yaw[piece->idx]) / 65536.0f, 0.0f);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (mdl != NULL && mdl->id != 0) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(RS_ALPHA_REF, 0);
            Draw_SetDefaultState();
        }
    }
}

/*
 * Creates the ScrollingOverlay object (kind 14, no buffer);
 * Area_StartObjects calls it in the areas its set-up knows.
 */
void ScrollingOverlay_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 14;
        obj->type = 0;
        obj->update = ScrollingOverlay_Update;
    }
}

/*
 * ScrollingOverlay's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void ScrollingOverlay_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        ScrollingOverlay_Setup(obj);
        break;
    case 1:
        ScrollingOverlay_Run(obj);
        break;
    case 2:
        ScrollingOverlay_End(obj);
        break;
    case 3:
        ScrollingOverlay_Free(obj);
        break;
    }
}

/*
 * Set-up state: picks the type by Area_DayId of the area (0
 * AREA_FOREST_HILLS_11, 1 AREA_DESERT_NIGHT_7, 2 AREA_GREAT_ARENA_1, 3
 * AREA_DESERT_2, 4 AREA_DESERT_3, 5 AREA_DESERT_9, 6 AREA_DESERT_10, with
 * most nights) and its position. Types 1-5 pick the rows their two sliding
 * overlays start on (Obj.var0, Obj.var1, never the same), the overlays'
 * durations (Obj.var2, Obj.var4: 55 to 64 frames) and starting frames
 * (Obj.timer, Obj.var3); type 6 a 100-frame cycle (Obj.var3). Frees the
 * object in other areas; else takes a draw slot (ScrollingOverlay_Draw) and
 * runs the first frame, or frees it when none is free.
 */
void ScrollingOverlay_Setup(Obj *obj)
{
    DrawSlot *drawSlot;

    obj->state++;
    obj->visible = 1;
    obj->var3 = 0;
    obj->timer = 0;
    switch (Area_DayId(gScene.area)) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        obj->type = 0;
        obj->pos[0] = 7227.0f;
        obj->pos[1] = -34.0f;
        obj->pos[2] = 5795.0f;
        break;
    case AREA_DESERT_NIGHT_7:
        obj->type = 1;
        obj->pos[0] = 8800.0f;
        obj->pos[1] = 13.0f;
        obj->pos[2] = 10400.0f;
        obj->var0 = Rand_Next(1) % 5;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 4 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 5) {
            obj->var1 -= 5;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        obj->type = 2;
        obj->pos[0] = 9400.0f;
        obj->pos[1] = 18.0f;
        obj->pos[2] = 9780.0f;
        obj->var0 = Rand_Next(1) % 5;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 4 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 5) {
            obj->var1 -= 5;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_2:
    case AREA_DESERT_2:
        obj->type = 3;
        obj->pos[0] = 6200.0f;
        obj->pos[1] = 54.0f;
        obj->pos[2] = 5500.0f;
        obj->var0 = Rand_Next(1) % 4;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 3 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 4) {
            obj->var1 -= 4;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_3:
        obj->type = 4;
        obj->pos[0] = 10000.0f;
        obj->pos[1] = 13.0f;
        obj->pos[2] = 10300.0f;
        obj->var0 = Rand_Next(1) % 4;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 3 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 4) {
            obj->var1 -= 4;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_9:
    case AREA_DESERT_9:
        obj->type = 5;
        obj->pos[0] = 10000.0f;
        obj->pos[1] = 13.0f;
        obj->pos[2] = 10300.0f;
        obj->var0 = Rand_Next(1) % 4;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 3 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 4) {
            obj->var1 -= 4;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        obj->type = 6;
        obj->var3 = 100;
        break;
    default:
        Obj_Free(obj);
        return;
    }
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = ScrollingOverlay_Draw;
        ScrollingOverlay_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: in types 1-5, when an overlay's duration runs out, restarts it
 * on a random row other than the other overlay's, with a new duration
 * (overlay 0: Obj.timer, Obj.var2, row Obj.var0; overlay 1: Obj.var3,
 * Obj.var4, row Obj.var1); type 6 counts Obj.timer through 100 frames, the
 * others count it up. Adds the draw slot to gDrawOt8 while Obj.draw is NULL
 * (always: the set-up clears it).
 */
void ScrollingOverlay_Run(Obj *obj)
{

    switch (obj->type) {
    case 1:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 4 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 5) {
                obj->var0 -= 5;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 4 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 5) {
                obj->var1 -= 5;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 2:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 4 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 5) {
                obj->var0 -= 5;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 4 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 5) {
                obj->var1 -= 5;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 3:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 3 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 4) {
                obj->var0 -= 4;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 3 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 4) {
                obj->var1 -= 4;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 4:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 3 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 4) {
                obj->var0 -= 4;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 3 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 4) {
                obj->var1 -= 4;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 5:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 3 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 4) {
                obj->var0 -= 4;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 3 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 4) {
                obj->var1 -= 4;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 6:
        if (++obj->timer >= obj->var3) {
            obj->timer = 0;
            obj->var3 = 100;
        } else if (obj->timer >= 100) {
            return;
        }
        break;
    default:
        obj->timer++;
        break;
    }
    if (obj->draw == NULL) {
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt8, obj->ds, 8, 1);
    }
}

/* End state: hides the object; its draw slot is not freed. */
void ScrollingOverlay_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void ScrollingOverlay_Free(Obj *obj)
{
    Obj_Free(obj);
}

extern f32 ScrollingOverlay_FlipbookUV[16][2];
extern f32 ScrollingOverlay_ForestHillsPos[4][3];
extern f32 ScrollingOverlay_RowsDesertNight7[5][3];
extern f32 ScrollingOverlay_RowsGreatArena[5][3];
extern f32 ScrollingOverlay_RowsDesert2[4][3];
extern f32 ScrollingOverlay_RowsDesert3[4][3];
extern f32 ScrollingOverlay_RowsDesert9[4][3];
extern s16 ScrollingOverlay_SlotsForestHills[4];
extern s16 ScrollingOverlay_SlotsDesertNight7[4];
extern s16 ScrollingOverlay_SlotsGreatArena[4];
extern s16 ScrollingOverlay_SlotsDesert2[4];
extern s16 ScrollingOverlay_SlotsDesert3[4];
extern s16 ScrollingOverlay_SlotsDesert9[4];
extern s16 ScrollingOverlay_SlotsDesert10[8];

/* One sliding overlay of types 1-5: overlay 0 runs on Obj.var0 (its row), Obj.timer and Obj.var2
 * (its duration), overlay 1 on Obj.var1, Obj.var3 and Obj.var4; it moves 1000 along x from its row
 * over the duration, fading in over the first 20 frames and out over the last 20. */
#define OVERLAY_SLIDE_ROW(tbl, off)                                    \
    if (i == 0) {                                            \
        duration = obj->var2;                                       \
        elapsed = obj->timer;                                       \
        row = tbl[obj->var0];                                \
        w = elapsed * (1000.0f / duration);                               \
        x = w + row[0];                                      \
        y = row[1];                                          \
        z = row[2];                                          \
    } else {                                                 \
        duration = obj->var4;                                    \
        elapsed = obj->var3;                                       \
        row = tbl[obj->var1];                                \
        w = elapsed * (1000.0f / duration);                               \
        x = w + row[0];                                      \
        y = row[1];                                          \
        z = row[2];                                          \
    }                                                        \
    if (elapsed < 20) {                                            \
        alpha = (u8) (255.0f * (0.05f * elapsed));                     \
    } else if (duration - 20 < elapsed) {                                 \
        alpha = (u8) (255.0f * ((f32) (duration - elapsed) / 20.0f));         \
    } else {                                                 \
        alpha = 255;                                            \
    }                                                        \
    mdl = stage->matSets + off;                              \
    Gs_SetState(RS_COLOR2, alpha << 24 | 0xFFFFFF);                   \
    break;

/*
 * Draws the type's overlays without depth writes: each enabled slot (its
 * ScrollingOverlay_Slots table, up to four) draws a model of the area's sub-
 * model with its texture scrolled: slots 0 and 1 by the area's frame count
 * (slower in AREA_DESERT_10 and AREA_JUNGLE_NIGHT_3), slot 2 through the 16
 * steps of ScrollingOverlay_FlipbookUV, slot 3 along u over 300 frames. Type
 * 0 draws models 1-3 at ScrollingOverlay_ForestHillsPos, type 6 model 2 at
 * the origin, types 1-5 their one model sliding along its row as
 * OVERLAY_SLIDE_ROW says.
 */
void ScrollingOverlay_Draw(DrawSlot *drawSlot)
{
    f32 posMtx[16];
    f32 uvMtx[16];
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    AreaState *areaState = &gAreaState;
    ModelMatSet *mdl;
    s16 *slotOn;
    f32 *pos;
    s32 off;
    s16 i;
    f32 texU;
    f32 texV;
    f32 z;
    f32 x;
    f32 y;
    f32 *row;
    f32 w;
    s16 elapsed;
    s16 duration;
    u8 alpha;

    if (stage != NULL && stage->active != 0) {
        mdl = stage->matSets;
        switch (obj->type) {
        case 0: slotOn = ScrollingOverlay_SlotsForestHills; break;
        case 1: slotOn = ScrollingOverlay_SlotsDesertNight7; break;
        case 2: slotOn = ScrollingOverlay_SlotsGreatArena; break;
        case 3: slotOn = ScrollingOverlay_SlotsDesert2; break;
        case 4: slotOn = ScrollingOverlay_SlotsDesert3; break;
        case 5: slotOn = ScrollingOverlay_SlotsDesert9; break;
        case 6: slotOn = ScrollingOverlay_SlotsDesert10; break;
        default: return;
        }
        Gs_SetState(RS_ZWRITE, 0);
        Gs_SetState(RS_ALPHA_REF, 0);
        pos = ScrollingOverlay_ForestHillsPos[0];
        for (i = 0, off = 0; i < 4; i++) {
            if (*slotOn++ != 0) {
                texU = 0.0f;
                texV = texU;
                switch (i) {
                case 0:
                    if (gScene.area == AREA_DESERT_10) {
                        texV = 1.0f - areaState->frame / 100.0f;
                    } else {
                        texV = 1.0f - 0.011111111f * areaState->frame;
                    }
                    break;
                case 1:
                    if (gScene.area == AREA_JUNGLE_NIGHT_3) {
                        texV = 1.0f - 0.015625f * areaState->frame;
                    } else {
                        texV = 1.0f - 0.03125f * areaState->frame;
                    }
                    break;
                case 2:
                    texU = ScrollingOverlay_FlipbookUV[(obj->timer & 0x1E) >> 1][0];
                    texV = ScrollingOverlay_FlipbookUV[(obj->timer & 0x1E) >> 1][1];
                    break;
                case 3:
                    texU = 0.0033333334f * ((u16) obj->timer % 300);
                    break;
                }
                texU -= Math_Floor(texU);
                texV -= Math_Floor(texV);
                Mtx_Translation(uvMtx, texU, texV, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                x = 0.0f;
                y = x;
                z = x;
                switch (obj->type) {
                case 0:
                    x = pos[0];
                    y = pos[1];
                    z = pos[2];
                    mdl = (ModelMatSet *) ((u8 *) stage->matSets + off + 140);
                    break;
                case 1:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesertNight7, 0)
                case 2:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsGreatArena, 5)
                case 3:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesert2, 3)
                case 4:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesert3, 7)
                case 5:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesert9, 3)
                case 6:
                    mdl = stage->matSets + 2;
                    break;
                }
                Mtx_Translation(posMtx, x, y, z);
                Gs_SetState(RS_MTX_SLOT(0), (s32) posMtx);
                if (mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
                    Model_Draw(mdl->id, 0);
                }
            }
            pos += 3;
            off += 140;
        }
        Draw_SetDefaultState();
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * Creates the five TurningProp objects (kind 16, types 0-4, no buffer) in
 * AREA_BATTLE_TOWN_0 and its night, where Area_StartObjects calls it; creates
 * nothing elsewhere.
 */
void TurningProp_Create(void)
{
    s16 i;
    Obj *obj;

    switch (gScene.area) {
    default:
        return;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        for (i = 0; i < 5; i++) {
            obj = Obj_Alloc(0);
            if (obj != NULL) {
                obj->kind = 16;
                obj->type = i;
                obj->update = TurningProp_Update;
            }
        }
        break;
    }
}

/*
 * TurningProp's update, run every frame: calls the set-up, run, end or free
 * function by Obj.state (0-3).
 */
void TurningProp_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        TurningProp_Setup(obj);
        break;
    case 1:
        TurningProp_Run(obj);
        break;
    case 2:
        TurningProp_End(obj);
        break;
    case 3:
        TurningProp_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the prop at TurningProp_Pos[type], picks its turn
 * period of 200 to 327 frames (Obj.var3), takes a draw slot
 * (TurningProp_Draw) and runs the first frame, or frees the object when none
 * is free.
 */
void TurningProp_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->var3 = 0;
    obj->pos[0] = 0.0f;
    obj->pos[1] = 0.0f;
    obj->pos[2] = 0.0f;
    obj->timer = 0;
    Vec_Copy(obj->pos, TurningProp_Pos[obj->type]);
    obj->var3 = (Rand_Next(1) & 0x7F) + 200;
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = TurningProp_Draw;
        TurningProp_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: counts Obj.timer through the turn period and adds the draw slot
 * to gDrawOt32.
 */
void TurningProp_Run(Obj *obj)
{
    obj->timer++;
    if (obj->timer >= obj->var3) {
        obj->timer = 0;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/* End state: hides the prop; its draw slot is not freed. */
void TurningProp_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void TurningProp_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the prop: model TurningProp_Models[type] of the area model with its
 * lights, scaled by TurningProp_Scales[type] and turned about y once per
 * period (the angle falls as Obj.timer grows) at the prop's position, alpha
 * test reference 128.
 */
void TurningProp_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaState.model;
    f32 mtx[16];
    ModelMatSet *model;

    if (stage != NULL && stage->active != 0) {
        model = &stage->matSets[TurningProp_Models[obj->type]];
        Gs_SetLightList(stage->mats, model);
        Mtx_Scale(mtx, TurningProp_Scales[obj->type], TurningProp_Scales[obj->type], TurningProp_Scales[obj->type]);
        Mtx_RotateY(mtx, (6.2831855f * (360.0f * (1.0f - (f32) obj->timer / (f32) obj->var3))) / 360.0f);
        Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Gs_SetState(RS_ALPHA_REF, 128);
        if (model != NULL && model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the GroundTileGrid object (kind 17, no buffer); Area_StartObjects
 * calls it for AREA_JUNGLE_1, AREA_JUNGLE_6 and their nights.
 */
void GroundTileGrid_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 17;
        obj->type = 0;
        obj->update = GroundTileGrid_Update;
    }
}

/*
 * GroundTileGrid's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void GroundTileGrid_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        GroundTileGrid_Setup(obj);
        break;
    case 1:
        GroundTileGrid_Run(obj);
        break;
    case 2:
        GroundTileGrid_End(obj);
        break;
    case 3:
        GroundTileGrid_Free(obj);
        break;
    }
}

/*
 * Set-up state: zeroes its position and counters, takes a draw slot
 * (GroundTileGrid_Draw) and runs the first frame, or frees the object when
 * none is free.
 */
void GroundTileGrid_Setup(Obj *obj)
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
        obj->ds->draw = GroundTileGrid_Draw;
        GroundTileGrid_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: follows the local player on x and z at height 0 (the slot's
 * depth) and adds the draw slot to gDrawOt8; returns OT_AddByDepth's result.
 */
u32 GroundTileGrid_Run(Obj *obj)
{
    Player *pl;

    pl = &gPlayers[gScene.localPlayer];
    obj->timer++;
    obj->pos[0] = pl->pos[0];
    obj->pos[1] = 0.0f;
    obj->pos[2] = pl->pos[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    return OT_AddByDepth(gDrawOt8, obj->ds, 8, 1);
}

/* End state: hides the object; its draw slot is not freed. */
void GroundTileGrid_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void GroundTileGrid_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the area's tile grid (by Area_DayId): AREA_JUNGLE_1 a 5 by 5 grid of
 * 2000-unit tiles (GroundTileGrid_Jungle1Tiles), AREA_JUNGLE_6 18 rows of 7
 * tiles of 500 by 1000 (GroundTileGrid_Jungle6Tiles), each turned by
 * GroundTileGrid_Jungle6Turns. A tile (its low 4 bits: its model of the
 * area's sub-model; 255: none) is drawn at height 0 when its bounding sphere
 * is in the view frustum (Camera_BuildFrustum out to 4000). In the first case
 * the assignments stay in this order: the compiler then loads the table's
 * address after the radius is stored, as the game's code does.
 */
void GroundTileGrid_Draw(DrawSlot *drawSlot)
{
    f32 out[4];
    f32 mtx[16];
    f32 sph[8];
    s32 k;
    ModelMatSet *mdl;
    u16 day;
    s16 i;
    s16 j;
    s16 rows;
    Model *stage = gAreaSubModels[0];
    u8 tileId;
    s16 cols;
    u8 *tbl;
    f32 startX;
    f32 xstep;
    f32 zstep;
    f32 z;
    f32 x;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(RS_ALPHA_REF, 128);
        Gs_SetState(RS_COLOR2, -1);
        day = Area_DayId(gScene.area);
        switch (day) {
        case 2:
        case 111:
            z = 4000.0f;
            startX = 8000.0f;
            zstep = 2000.0f;
            sph[4] = 2828.0f;
            xstep = 2000.0f;
            rows = 5;
            cols = 5;
            tbl = GroundTileGrid_Jungle1Tiles;
            Camera_BuildFrustum(4000.0f);
            break;
        case 116:
            z = 5800.0f;
            startX = 10100.0f;
            rows = 18;
            cols = 7;
            xstep = 500.0f;
            zstep = 1000.0f;
            sph[4] = 1414.0f;
            tbl = GroundTileGrid_Jungle6Tiles;
            Camera_BuildFrustum(4000.0f);
            break;
        }
        sph[2] = 0.0f;
        Tex_Prepare(16, 296);
        for (i = 0; i < rows; i++) {
            x = startX;
            for (j = 0; j < cols; j++) {
                k = i * cols + j;
                tileId = tbl[(s16) k];
                if (tileId != 255) {
                    sph[1] = x;
                    sph[3] = z;
                    if (Frustum_TestSphere(&sph[1], out, gViewMatrix, gFrustumPlanes, sph[4]) != 0) {
                        mdl = stage->matSets + (tileId & 0xF);
                        Mtx_Translation(mtx, x, 0.0f, z);
                        if (day == 116) {
                            Mtx_RotateY(mtx, (6.2831855f * GroundTileGrid_Jungle6Turns[k]) / 65536.0f);
                        }
                        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                        Draw_ApplyStates(mdl->flags);
                        Model_Draw(mdl->id, 0);
                    }
                }
                x += xstep;
            }
            z += zstep;
        }
    }
}

/*
 * Creates the AnimatedScenery object (kind 19, no buffer) of scenery set
 * setId (its Obj.type); Area_StartObjects passes the area's third set-up id
 * of gAreaInfo.
 */
void AnimatedScenery_Create(u8 setId)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 19;
        obj->type = setId;
        obj->update = AnimatedScenery_Update;
    }
}

/*
 * AnimatedScenery's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void AnimatedScenery_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AnimatedScenery_Setup(obj);
        break;
    case 1:
        AnimatedScenery_Run(obj);
        break;
    case 2:
        AnimatedScenery_End(obj);
        break;
    case 3:
        AnimatedScenery_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the object at its set's first position
 * (AnimatedScenery_Positions[type]), clears its counters, takes a draw slot
 * (AnimatedScenery_Draw) at that position and runs the first frame, or frees
 * the object when none is free.
 */
void AnimatedScenery_Setup(Obj *obj)
{
    f32 *posRow;

    posRow = AnimatedScenery_Positions[obj->type];
    obj->state++;
    obj->visible = 1;
    obj->pos[0] = *posRow++;
    obj->pos[1] = *posRow++;
    obj->pos[2] = *posRow;
    obj->timer = 0;
    obj->var3 = 0;
    obj->var5 = 0;
    obj->fvar = 0.0f;
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AnimatedScenery_Draw;
        Vec_Copy(obj->ds->pos, obj->pos);
        AnimatedScenery_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: counts Obj.timer up and the set's own counter Obj.var3: wrapping
 * at 896 (set 25), 600 (set 36, whose Obj.var5 wraps at 1200) or 800
 * (sets 54, 55), or stepping as an angle by 46 (set 37) or 91 (the
 * others) a frame. Adds the draw slot to gDrawOt8 and returns OT_AddByDepth's
 * result.
 */
u32 AnimatedScenery_Run(Obj *obj)
{

    obj->timer++;
    switch (obj->type) {
    case 25:
        obj->var3++;
        if (obj->var3 >= 896) {
            obj->var3 = 0;
        }
        break;
    case 36:
        obj->var5++;
        if (obj->var5 >= 1200) {
            obj->var5 = 0;
        }
        obj->var3++;
        if (obj->var3 >= 600) {
            obj->var3 = 0;
        }
        break;
    case 37:
        obj->var3 += 46;
        break;
    case 54:
        obj->var3++;
        if (obj->var3 >= 800) {
            obj->var3 = 0;
        }
        break;
    case 55:
        obj->var3++;
        if (obj->var3 >= 800) {
            obj->var3 = 0;
        }
        break;
    default:
        obj->var3 += 91;
        break;
    }
    return OT_AddByDepth(gDrawOt8, obj->ds, 8, 1);
}

/* End state: hides the object; its draw slot is not freed. */
void AnimatedScenery_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void AnimatedScenery_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Returns 1 for the scenery sets drawn from the area model with its lights
 * (32-34, 54, 55), else 0. obj is the AnimatedScenery object, passed
 * as an Em (Em.subType lies where Obj.type does); type is unused.
 */
s32 AnimatedScenery_UsesAreaModel(Em *obj, u8 type)
{
    switch (obj->subType) {
    case 32:
    case 33:
    case 34:
    case 54:
    case 55:
        return 1;
    }
    return 0;
}

extern s16 AnimatedScenery_FirstModel[56];
extern s16 *AnimatedScenery_Hidden[56];
extern f32 AnimatedScenery_FlipbookUV[16][2];

/*
 * Draws the set's models: up to four slots, each drawn when its
 * AnimatedScenery_Hidden[type] entry is 0, with the next model from
 * AnimatedScenery_FirstModel[type] on and the next position of
 * AnimatedScenery_Positions[type], its texture scrolled by the set
 * (Obj.timer, Obj.var3 or Obj.var5; AnimatedScenery_FlipbookUV for the
 * 16-step sets); the first slot of the sets not listed turns about y by
 * Obj.var3. Set 17 first draws model 8 of the area model; sets 32-34, 54
 * and 55 draw from the area model with its lights.
 */
void AnimatedScenery_Draw(DrawSlot *drawSlot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    AreaState *areaState = &gAreaState;
    ModelMat *lights;
    ModelMatSet *mdl;
    s16 k;
    s16 *flags;
    f32 *pos;
    f32 x;
    f32 y;
    f32 texU;
    f32 texV;

    if (obj->type == 17) {
        stage = areaState->model;
        lights = stage->mats;
        if (stage == NULL) {
            return;
        }
        if (stage->active == 0) {
            return;
        }
        Mtx_Identity(mtx);
        mdl = stage->matSets + 8;
        Gs_SetLightList(lights, mdl);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
        stage = gAreaSubModels[0];
    }
    if (AnimatedScenery_UsesAreaModel((Em *) obj, obj->type)) {
        stage = areaState->model;
        lights = stage->mats;
    }
    if (stage != NULL && stage->active != 0) {
        Gs_SetState(RS_COLOR2, -1);
        pos = AnimatedScenery_Positions[obj->type];
        flags = AnimatedScenery_Hidden[obj->type];
        k = AnimatedScenery_FirstModel[obj->type];
        if (*flags++ == 0) {
            Mtx_Identity(mtx);
            mdl = stage->matSets + k++;
            if (AnimatedScenery_UsesAreaModel((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            x = 0.0f;
            y = x;
            switch (obj->type) {
            case 20:
            case 44:
                Gs_SetState(RS_ALPHA_REF, 0);
                x = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 27:
            case 31:
            case 52:
                Gs_SetState(RS_ALPHA_REF, 0);
                x = (obj->timer & 0xFF) / 256.0f;
                break;
            case 16:
                Gs_SetState(RS_ALPHA_REF, 0);
                break;
            case 12:
                Gs_SetState(RS_ALPHA_REF, 0);
                x = 0.125f * (obj->timer & 7);
                y = 0.125f * ((obj->timer >> 3) & 3);
                break;
            case 0:
            case 22:
            case 23:
            case 24:
            case 29:
            case 53:
                Gs_SetState(RS_ALPHA_REF, 0);
                break;
            case 30:
                Gs_SetState(RS_ALPHA_REF, 0);
                x = 1.0f - (obj->timer & 0x1FF) / 512.0f;
                break;
            case 11:
                Gs_SetState(RS_ALPHA_REF, 0);
                y = 1.0f - (obj->timer & 0x1FF) / 512.0f;
                break;
            default:
                if (obj->type == 37 || obj->type == 41) {
                    Gs_SetState(RS_ALPHA_REF, 0);
                } else {
                    Gs_SetState(RS_ALPHA_REF, 128);
                }
                x = 0.033333335f * ((360.0f * obj->var3) / 65536.0f);
                x -= Math_Floor(x);
                Mtx_RotateY(mtx, (6.2831855f * obj->var3) / 65536.0f);
                break;
            }
            Mtx_Translation(uvMtx, x, y, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Gs_SetState(RS_ALPHA_REF, 0);
        if (*flags++ == 0) {
            Mtx_Identity(mtx);
            mdl = stage->matSets + k++;
            if (AnimatedScenery_UsesAreaModel((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            switch (obj->type) {
            case 0:
            case 1:
            case 20:
            case 22:
            case 23:
            case 24:
            case 29:
            case 43:
            case 44:
                Mtx_Translation(uvMtx, (obj->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 49:
                Gs_SetState(RS_ZWRITE, 0);
                Mtx_Translation(uvMtx, (obj->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 12:
            case 27:
                Mtx_Translation(uvMtx, 0.0f, (obj->timer & 0x7F) / 128.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 8:
            case 40:
                Mtx_Translation(uvMtx, 0.0f, 1.0f - (obj->timer & 0x1FF) / 512.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 11:
                Mtx_Translation(uvMtx, 1.0f - (obj->timer & 0xF) / 16.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 30:
                Mtx_Translation(uvMtx, 0.125f * (obj->timer & 7), 0.125f * ((obj->timer >> 3) & 3), 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 31:
            case 52:
                Mtx_Translation(uvMtx, (obj->timer & 0xFF) / 256.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 36:
                Mtx_Translation(uvMtx, (obj->timer & 0x7FF) / 2048.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            case 16:
            case 38:
            case 41:
                break;
            }
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(RS_ZWRITE, 1);
        }
        if (*flags++ == 0) {
            Mtx_Identity(mtx);
            texU = 0.0f;
            texV = texU;
            switch (obj->type) {
            case 4:
            case 7:
                texV = (obj->timer & 0xFF) / 256.0f;
                break;
            case 12:
            case 20:
            case 42:
            case 43:
            case 46:
                texU = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 19:
                texU = (obj->timer & 0xFF) / 256.0f;
                break;
            case 16:
            case 27:
                break;
            case 25:
                texV = obj->var3 / 896.0f;
                texU = texV;
                break;
            case 28:
                texV = (obj->timer & 0x7F) / 128.0f;
                break;
            case 1:
            case 13:
            case 31:
            case 44:
                texU = 0.125f * (obj->timer & 7);
                texV = 0.125f * ((obj->timer >> 3) & 3);
                break;
            case 30:
                texU = (obj->timer & 0x3F) / 64.0f;
                break;
            case 37:
                texU = 0.033333335f * ((360.0f * obj->var3) / 65536.0f);
                texU -= Math_Floor(texU);
                Mtx_RotateY(mtx, (6.2831855f * obj->var3) / 65536.0f);
                texV = (obj->timer & 0xFF) / 256.0f;
                break;
            case 18:
            case 26:
            case 35:
            case 39:
            case 48:
                texU = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 8:
                texU = (obj->timer & 0x1FF) / 512.0f;
                texV = 1.0f - (obj->timer & 0x7F) / 128.0f;
                break;
            case 40:
                Gs_SetState(RS_ZWRITE, 0);
                texU = (obj->timer & 0x1FF) / 512.0f;
                texV = 1.0f - (obj->timer & 0x7F) / 128.0f;
                break;
            case 36:
                texU = obj->var5 / 1200.0f;
                break;
            case 9:
            case 38:
            case 41:
                texV = (obj->timer & 0x7FF) / 2048.0f;
                break;
            case 33:
            case 34:
                texU = 1.0f - (obj->timer & 0x3FF) / 1024.0f;
                break;
            case 45:
                texV = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 51:
                texV = 1.0f - (obj->timer & 0x1FF) / 512.0f;
                break;
            case 11:
            case 52:
                break;
            case 54:
            case 55:
                texU = (obj->timer & 0x3FF) / 1024.0f;
                break;
            case 32:
                texV = (obj->timer & 0x3FF) / 1024.0f;
                break;
            default:
                texU = AnimatedScenery_FlipbookUV[obj->timer & 0xF][0];
                texV = AnimatedScenery_FlipbookUV[obj->timer & 0xF][1];
                break;
            }
            Mtx_Translation(uvMtx, texU, texV, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            mdl = stage->matSets + k++;
            if (AnimatedScenery_UsesAreaModel((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Gs_SetState(RS_ZWRITE, 1);
        if (*flags++ == 0) {
            mdl = stage->matSets + k++;
            if (AnimatedScenery_UsesAreaModel((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            Mtx_Identity(mtx);
            switch (obj->type) {
            case 2:
            case 3:
            case 5:
            case 6:
            case 15:
            case 32:
            case 33:
                y = 0.0f;
                x = (obj->timer & 0x3FF) / 1024.0f;
                break;
            case 8:
            case 21:
            case 40:
                y = 0.0f;
                x = (obj->timer & 0x7F) / 128.0f;
                break;
            case 9:
            case 12:
            case 20:
            case 35:
            case 38:
            case 39:
            case 41:
            case 42:
            case 46:
            case 47:
                x = 0.125f * (obj->timer & 7);
                y = 0.125f * ((obj->timer >> 3) & 3);
                break;
            case 11:
                Gs_SetState(RS_ZWRITE, 0);
                y = 0.0f;
                x = (obj->timer & 0x7F) / 128.0f;
                break;
            case 31:
                y = 0.0f;
                x = (obj->timer & 0x3F) / 64.0f;
                break;
            case 30:
                y = 0.0f;
                x = (obj->timer & 0xFF) / 256.0f;
                break;
            case 51:
                Gs_SetState(RS_ZWRITE, 0);
                y = 0.0f;
                x = (obj->timer & 0xFF) / 256.0f;
                break;
            case 17:
            case 26:
            case 27:
                y = 0.0f;
                x = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 1:
            case 13:
            case 44:
                x = 0.0f;
                y = (obj->timer & 0x7F) / 128.0f;
                break;
            case 34:
                x = 0.0f;
                y = (obj->timer & 0x7FF) / 2048.0f;
                break;
            case 0:
            case 22:
            case 23:
            case 24:
            case 29:
            case 48:
                x = 0.0f;
                y = (obj->timer & 0xFF) / 256.0f;
                break;
            case 36:
                y = 0.0f;
                x = obj->var3 / 600.0f;
                break;
            case 54:
                y = 0.0f;
                x = obj->var3 / 800.0f;
                break;
            case 55:
                y = 0.0f;
                x = obj->var3 / 800.0f;
                Gs_SetState(RS_COLOR2, 0xFF303030);
                break;
            case 10:
            case 50:
                x = 0.0f;
                y = x;
                break;
            }
            Mtx_Translation(uvMtx, x, y, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(RS_ZWRITE, 1);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the CastleGate object of gate gateType (0: AREA_CASTLE_0's, 1:
 * AREA_CASTLE_1's dropping gate). Unless gQuest.stage is 0 it first sets
 * gScene.skipFlaggedTris, the gate open (the collisions skip the triangles of
 * attribute 0x4000), in every quest but 3004, which clears it.
 */
void CastleGate_Create(u8 gateType)
{
    Obj *obj;

    switch (gQuest.stage) {
    default:
        switch (gQuest.info->id) {
        case 3004:
            gScene.skipFlaggedTris = 0;
            break;
        case 3005:
        case 3006:
        case 3007:
        case 0xEA68:
        default:
            gScene.skipFlaggedTris = 1;
            break;
        }
        break;
    case 0:
        break;
    }
    obj = Obj_Alloc(0);
    if (obj != NULL) {
        obj->kind = 20;
        obj->arg = NULL;
        obj->type = gateType;
        obj->update = CastleGate_Update;
    }
}

/*
 * CastleGate's update, run every frame: calls the set-up, run, end or free
 * function by Obj.state (0-3).
 */
void CastleGate_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        CastleGate_Setup(obj);
        break;
    case 1:
        CastleGate_Run(obj);
        break;
    case 2:
        CastleGate_End(obj);
        break;
    case 3:
        CastleGate_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the gate at its open (CastleGate_PosOpen) or shut
 * (CastleGate_PosShut) position by gScene.skipFlaggedTris. When bit 1 of
 * gScene.emEventFlags is set (a Fatalis was knocked down or killed), it drops
 * at once (Obj.var0 2) if a Fatalis of the area lies knocked down, else never
 * drops (Obj.var0 4). Takes a draw slot (CastleGate_Draw) and runs the first
 * frame, or frees the object when none is free.
 */
void CastleGate_Setup(Obj *obj)
{
    s16 i;
    Em *em;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->fvar = 0.0f;
    if (gScene.skipFlaggedTris != 0) {
        Vec_Copy(obj->pos, CastleGate_PosOpen[obj->type]);
    } else {
        Vec_Copy(obj->pos, CastleGate_PosShut[obj->type]);
    }
    if (gScene.emEventFlags & 2) {
        em = gEms;
        for (i = 0; i < 20; i++, em++) {
            if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                if (em->type == 2 || em->type == 36 || em->type == 71) {
                    break;
                }
            }
        }
        if (i == 20 || em->motionId[0] != 1027) {
            obj->var0 = 4;
        } else {
            obj->var0 = 2;
        }
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = CastleGate_Draw;
        CastleGate_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state of gate 1 while gScene.skipFlaggedTris is set, once: waits for a
 * Fatalis of the area standing on the ground under the gate (x 10900-13000, z
 * 19700-20200), lying knocked down, or after bit 1 of gScene.emEventFlags;
 * then plays its drop sounds, makes the hit under the gate
 * (StageShot_CreateAt, STAGESHOT_AREA_DROP, at the monster's floor) and drops
 * 1750 below its open height under gravity, shaking the camera and bouncing
 * up to three times. After 180 frames, once no Fatalis lies knocked down, it
 * plays the rise sounds and rises back at 29.2 a frame. Every frame adds the
 * draw slot to gDrawOt32.
 */
void CastleGate_Run(Obj *obj)
{
    Em *em = gEms;
    s16 i;
    f32 hitPos[3];

    switch (obj->type) {
    case 1:
        if (gScene.skipFlaggedTris == 0) {
            break;
        }
        switch (obj->var0) {
        case 0:
            for (i = 0; i < 20; i++, em++) {
                if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                    if (em->type == 2 || em->type == 36 || em->type == 71) {
                        break;
                    }
                }
            }
            if (i == 20) {
                break;
            }
            if (!(gScene.emEventFlags & 2) && em->motionId[0] != 1027) {
                if (em->stance != 0) {
                    break;
                }
                if (!(em->posX > 10900.0f && em->posX < 13000.0f && em->posZ > 19700.0f && em->posZ < 20200.0f)) {
                    break;
                }
            }
            Em_PlaySeChecked(em, 54, 0, CastleGate_SoundPos, 9, 0);
            Em_PlaySeChecked(em, 55, 0, D_008B149C, 9, 0);
            obj->var0++;
            obj->fvar = -100.0f;
            hitPos[0] = obj->pos[0];
            hitPos[1] = em->floorY;
            hitPos[2] = obj->pos[2];
            StageShot_CreateAt(hitPos, STAGESHOT_AREA_DROP, em->area, 0);
            break;
        case 1:
            obj->pos[1] += obj->fvar;
            obj->fvar += -20.0f;
            if (obj->pos[1] <= CastleGate_PosOpen[obj->type][1] - 1750.0f) {
                obj->pos[1] = CastleGate_PosOpen[obj->type][1] - 1750.0f;
                obj->fvar *= -0.2f;
                obj->var0++;
                CamShake_SetA(5, obj->pos);
                obj->timer = 0;
            }
            break;
        case 2:
            if (obj->var1 < 3) {
                obj->pos[1] += obj->fvar;
                obj->fvar += -20.0f;
                if (obj->pos[1] <= CastleGate_PosOpen[obj->type][1] - 1750.0f) {
                    obj->pos[1] = CastleGate_PosOpen[obj->type][1] - 1750.0f;
                    obj->fvar *= -0.2f;
                    obj->var1++;
                }
            }
            for (i = 0; i < 20; i++, em++) {
                if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                    if (em->type == 2 || em->type == 36 || em->type == 71) {
                        break;
                    }
                }
            }
            if (++obj->timer <= 180) {
                break;
            }
            if (i == 20 || em->motionId[0] != 1027) {
                obj->var0++;
                obj->timer = 0;
                for (i = 0; i < 20; i++, em++) {
                    if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                        if (em->type == 2 || em->type == 36 || em->type == 71) {
                            Em_PlaySeChecked(em, 56, 0, CastleGate_SoundPos, 9, 0);
                            Em_PlaySeChecked(em, 57, 0, D_008B149C, 9, 0);
                            break;
                        }
                    }
                }
            }
            break;
        case 3:
            obj->pos[1] += 29.166666f;
            if (obj->pos[1] > CastleGate_PosOpen[obj->type][1]) {
                obj->var0++;
                obj->pos[1] = CastleGate_PosOpen[obj->type][1];
                obj->timer = 0;
            }
            break;
        case 4:
            break;
        }
        break;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
}

/* End state: hides the gate and frees its draw slot. */
void CastleGate_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Free state: Obj_Free. */
void CastleGate_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws model CastleGate_Models[type] of the area's sub-model at the gate's
 * position; the owner's Obj.pos and Obj.type are read through byte offsets.
 */
void CastleGate_Draw(DrawSlot *drawSlot)
{
    f32 mtx[16];
    s32 gate;
    Model *subs;
    ModelMatSet *model;

    subs = gAreaSubModels[0];
    gate = (s32) drawSlot->owner;
    if ((subs != NULL) && (subs->active != 0)) {
        Mtx_Translation((void *) mtx, *(f32 *)((u8 *)gate + 32), *(f32 *)((u8 *)gate + 36), *(f32 *)((u8 *)gate + 40));
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        model = &subs->matSets[CastleGate_Models[*(u8 *)((u8 *)gate + 3)]];
        if ((model != NULL) && (model->id != -1)) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the DistantLightning object (kind 22, no buffer);
 * Area_StartObjects calls it for AREA_CASTLE_1.
 */
void DistantLightning_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 22;
        obj->update = DistantLightning_Update;
    }
}

/*
 * DistantLightning's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void DistantLightning_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        DistantLightning_Setup(obj);
        break;
    case 1:
        DistantLightning_Run(obj);
        break;
    case 2:
        DistantLightning_End(obj);
        break;
    case 3:
        DistantLightning_Free(obj);
        break;
    }
}

/*
 * Set-up state: clears its counters, waits 0 to 15 frames (Obj.timer), takes
 * a draw slot (DistantLightning_Draw) and runs the first frame, or frees the
 * object when none is free.
 */
void DistantLightning_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = Rand_Next(1) & 0xF;
    obj->var3 = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = DistantLightning_Draw;
        DistantLightning_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: after its wait, a flash: picks a kind (Obj.var1, 0-4) and places
 * it 21700 from the point (10000, 10000) in a random direction, at height 0
 * (kinds 0, 1) or 6000 to 9300 up, with a random roll (Obj.var3) and, for
 * kind 4, a scale of 0.7 to 1.0 (Obj.fvar); shows it 6 frames through
 * gDrawOt8, then waits 1 to 64 frames. Kind 5, which Rand_Next(1) % 5 never
 * gives, would also spawn four StageObj_K11 effects and draw through
 * gDrawOtSorted. y stays a variable set to 0.0f: the game's code adds it to
 * the height.
 */
void DistantLightning_Run(Obj *obj)
{
    f32 offset[3];
    f32 y = 0.0f;

    switch (obj->var0) {
    case 0:
        if (--obj->timer >= 0) {
            break;
        }
        obj->var0++;
        obj->var1 = Rand_Next(1) % 5;
        obj->timer = 7;
        offset[0] = 0.0f;
        offset[2] = 21700.0f;
        switch (obj->var1) {
        case 0:
        case 1:
        case 5:
            offset[1] = 0.0f;
            obj->var3 = (Rand_Next(1) & 0x7FF) - 1024;
            obj->fvar = 1.0f;
            break;
        case 2:
            offset[1] = 7300.0f + (Rand_Next(1) & 0x3FF);
            obj->var3 = (Rand_Next(1) & 0xFFF) - 2048;
            obj->fvar = 1.0f;
            break;
        case 3:
            offset[1] = 6000.0f + (Rand_Next(1) & 0x3FF) * 3;
            obj->var3 = (Rand_Next(1) & 0xFFF) - 2048;
            obj->fvar = 1.0f;
            break;
        case 4:
            offset[1] = 7000.0f + (Rand_Next(1) & 0x7FF);
            obj->var3 = 0;
            obj->fvar = 0.7f + 0.0003f * (Rand_Next(1) & 0x3FF);
            break;
        }
        Vec_RotateY(offset, (6.2831855f * Rand_Next(1)) / 65536.0f);
        obj->pos[0] = 10000.0f + offset[0];
        obj->pos[1] = y + offset[1];
        obj->pos[2] = 10000.0f + offset[2];
        if (obj->var1 == 5) {
            AttackBurst_CreateAtScaled(obj->pos, 0, 4, 0.15f);
            AttackBurst_CreateAtScaled(obj->pos, 0, 9, 1.0f);
            AttackBurst_CreateAtScaled(obj->pos, 0, 8, 1.0f);
            AttackBurst_CreateAtScaled(obj->pos, 0, 11, 1.0f);
        }
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var0 = 0;
            obj->timer = (Rand_Next(1) & 0x3F) + 1;
            break;
        }
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        if (obj->var1 == 5) {
            OT_AddByDepth(gDrawOtSorted, obj->ds, 64, 0);
        } else {
            OT_AddByDepth(gDrawOt8, obj->ds, 8, 0);
        }
        break;
    }
}

/* End state: hides the object and frees its draw slot. */
void DistantLightning_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Free state: Obj_Free. */
void DistantLightning_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* Built at optimization level 3: at level 4 the compiler computes the address of the model pair
 * Obj.var1 picks in another order of registers and additions than the game's code. */
#pragma optimization_level 3
/*
 * Draws the flash facing the camera: model pair Obj.var1 of the area's sub-
 * model (from model 3 on; the first for its first frame, the second after),
 * scaled by Obj.fvar, rolled about z by Obj.var3 and fading out over its last
 * 3 frames (alpha from Obj.timer), alpha test reference 0. The owner's fields
 * are read through byte offsets.
 */
void DistantLightning_Draw(DrawSlot *drawSlot)
{
    u8 *flash = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    f32 mtx[16];
    ModelMatSet *mdl;
    s32 alpha;
    s32 off;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(RS_ALPHA_REF, 0);
        Mtx_Scale(mtx, *(f32 *) (flash + 48), *(f32 *) (flash + 48), *(f32 *) (flash + 48));
        Mtx_RotateZ(mtx, (6.2831855f * *(s16 *) (flash + 10)) / 65536.0f);
        Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_Concat3x3(mtx, gCamMatrix);
        if (*(s16 *) (flash + 8) < 4) {
            alpha = (u32) (255.0f * (*(s16 *) (flash + 8) / 4.0f));
        } else {
            alpha = 255;
        }
        Gs_SetState(RS_COLOR2, ((u8) alpha << 24) | 0xFFFFFF);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        /* The offset is computed in bytes and added before the table's address, as the game's code
         * does. */
        mdl = stage->matSets;
        off = flash[6] * 280;
        mdl = (ModelMatSet *) (off + (s32) mdl + 420);
        if (*(s16 *) (flash + 8) < 6) {
            mdl++;
        }
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}
#pragma optimization_level reset

extern s16 AreaObj_K17_RectX[];
extern s16 AreaObj_K17_RectY[];
extern s16 AreaObj_K17_RectW[];
extern s16 AreaObj_K17_RectH[];
extern f32 AreaObj_K17_Spread1[];
extern s16 D_008F58C0[];
extern s16 D_008F58F0[];
extern u8 AreaObj_K19_PointsA2[];
extern u8 AreaObj_K19_PointsA3[];
extern u8 AreaObj_K19_PointsA6[];
extern u8 AreaObj_K19_PointsA7[];

/*
 * Creates the quest-end banner, an area object of kind 23. Its type is
 * `phase`, the quest phase: QUEST_PHASE_CLEARED, QUEST_PHASE_FAILED or
 * QUEST_PHASE_ABORTED. Scene_ResultFade calls it once the screen has
 * darkened. Nothing happens when no object is free.
 */
void AreaObj_K17_Create(u8 phase)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 23;
        obj->update = AreaObj_K17_Update;
        obj->type = phase;
    }
}

/*
 * The banner's update. By the object's state (0 to 3) it runs
 * QuestEndBanner_Setup, _Run, _End or _Free.
 */
void AreaObj_K17_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaObj_K17_Setup(obj);
        break;
    case 1:
        AreaObj_K17_Run(obj);
        break;
    case 2:
        AreaObj_K17_End(obj);
        break;
    case 3:
        AreaObj_K17_Free(obj);
        break;
    }
}

/*
 * Goes to the next state, clears var0 to var3 and timer, and makes the banner
 * visible and kept. It takes a draw slot whose callback is
 * QuestEndBanner_Draw, and frees the object when no slot is free. Then it
 * runs QuestEndBanner_Run once.
 */
void AreaObj_K17_Setup(Obj *obj)
{

    obj->state++;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    obj->keep = 1;
    obj->timer = 0;
    obj->var3 = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = AreaObj_K17_Draw;
    } else {
        Obj_Free(obj);
        return;
    }
    AreaObj_K17_Run(obj);
}

/*
 * Counts the banner's frames in var3 and goes to the next state at 300. Until
 * then it counts timer up to 100 (cleared, abandoned) or 113 (failed), with
 * var0 counting the frames at the cap (only once for a cleared quest). Each
 * frame it puts the draw slot into gDrawListText0b.
 */
void AreaObj_K17_Run(Obj *obj)
{
    obj->var3++;
    if (obj->var3 >= 300) {
        obj->state++;
        return;
    }
    switch (obj->type) {
    case 4:
        if (obj->var0 == 0) {
            if (++obj->timer > 100) {
                obj->timer = 100;
                obj->var0++;
            }
        }
        break;
    case 7:
        if (++obj->timer >= 100) {
            obj->timer = 100;
            obj->var0++;
        }
        break;
    case 6:
        if (++obj->timer >= 113) {
            obj->timer = 113;
            obj->var0++;
        }
        break;
    }
    OT_AddByDepth(gDrawListText0b, obj->ds, 1, 1);
}

/* Goes to the next state, hides the banner and frees its draw slot. */
void AreaObj_K17_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Frees the banner's object. */
void AreaObj_K17_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws `rect` (a TexRect) rotated by `angle` (ANGLE_TURN is a full turn) with
 * texture 340, unfiltered and blended.
 */
void AreaObj_K17_DrawTexRect(s16 *rect, s32 angle)
{
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 340);
    Tex_Bind(340);
    TexRect_DrawRotated((struct TexRect *) rect, angle);
}

/*
 * Draws the failed quest's mark from the box at `box` (x, y, w, h, colour) as
 * two rotated rects of texture 340. The first is 12/17 of the box, turned a
 * quarter turn past `angle`. The second is 5/17 of the box, turned three
 * quarters past `angle`, set near the box's lower end and moved sideways by
 * QuestEndBanner_PieceShift. The shift is read in 5-degree steps of the angle
 * plus 10 degrees and interpolated between steps; past 25 degrees it is entry
 * 2.
 */
void AreaObj_K17_DrawPart(s16 *box, u32 angle)
{
    Sprite piece;
    u16 rot;
    f32 degrees;
    u8 i;
    s32 shift;

    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 340);
    Tex_Bind(340);
    piece.color = *(s32 *) (box + 4);
    piece.w = box[3] * 12 / 17;
    piece.h = box[2];
    piece.x = box[0] + (piece.h - piece.w) / 2;
    piece.y = box[1] + (piece.w - piece.h) / 2;
    rot = angle;
    piece.u0 = 160;
    piece.v0 = 112;
    piece.u1 = piece.u0 + 95;
    piece.v1 = piece.v0 + 143;
    TexRect_DrawRotated((struct TexRect *) &piece, (u16) (rot + ANGLE_TURN / 4));
    degrees = 10.0f + (360.0f * angle) / 65536.0f;
    if (!(degrees < 360.0f)) {
        degrees -= 360.0f;
    }
    i = degrees / 5.0f;
    switch (i) {
    case 0:
    case 1:
    case 2:
    case 3:
        shift = (s16) (AreaObj_K17_AngleTbl[i] + ((degrees - 5.0f * i) * (AreaObj_K17_AngleTbl[i + 1] - AreaObj_K17_AngleTbl[i])) / 5.0f);
        break;
    case 4:
        shift = AreaObj_K17_AngleTbl[i];
        break;
    default:
        shift = AreaObj_K17_AngleTbl[2];
        break;
    }
    piece.w = box[3] * 5 / 17;
    piece.h = box[2];
    piece.x = box[0] + (piece.h - piece.w) / 2;
    piece.y = (box[1] + box[3]) - (piece.w + piece.h) / 2;
    piece.x = piece.x + shift;
    piece.y -= 6;
    piece.u0 = 160;
    piece.v0 = 112;
    piece.u1 = piece.u0 + 39;
    piece.v1 = piece.v0 + 143;
    TexRect_DrawRotated((struct TexRect *) &piece, (u16) (rot + 0x7FFF + 0x4001));
}

/*
 * Draws `sprite` with the whole 256 x 256 of texture 326, filtered and
 * blended. This is the banner's full-screen picture.
 */
void AreaObj_K17_DrawSprite(Sprite *sprite)
{
    sprite->u0 = 0;
    sprite->v0 = 0;
    sprite->u1 = sprite->u0 + 255;
    sprite->v1 = sprite->v0 + 255;
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, 326);
    Tex_Bind(326);
    Sprite_Draw(sprite);
}

/*
 * Draw callback of the quest-end banner (slot->owner is its Obj). It draws
 * the picture over the whole screen (QuestEndBanner_DrawPicture): fading in
 * over the first 15 frames of timer (var1 phase 0), at 0.95 size in phase 3.
 * From phase 2 it adds the type's mark at QuestEndBanner_RectX/Y, sized
 * RectW/H times 1.15 at half alpha in phase 2, 0.9 in phase 3 and 1.0 after.
 * The mark is tilted by pos[0] degrees, moved by the type's first
 * QuestEndBanner_ShakeX/ShakeY entry (pos[1] is 0 or 8, and 8 reads as 0),
 * and drawn in two pieces for a failed quest. Phase 1 ends at frame 60: it
 * picks the tilt at random within the type's QuestEndBanner_TiltRange and
 * starts the type's event tune; phases 2 and 3 last one frame each.
 */
void AreaObj_K17_Draw(DrawSlot *slot)
{
    Obj *obj = slot->owner;
    Sprite sprite;
    f32 scale;
    f32 alpha;
    u8 typeIdx;
    u8 type;
    u8 shakeIdx;

    sprite.x = 0;
    sprite.y = 0;
    type = obj->type;
    switch (type) {
    case 4:
        typeIdx = 0;
        break;
    case 7:
        typeIdx = 1;
        break;
    case 6:
        typeIdx = 2;
        break;
    }
    switch (type) {
    case 4:
    case 7:
    case 6:
        sprite.w = 640;
        sprite.h = 448;
        switch (obj->var1) {
        case 0:
            scale = 1.0f;
            alpha = obj->timer / 15.0f;
            break;
        case 1:
        case 2:
            scale = 1.0f;
            alpha = 1.0f;
            break;
        case 3:
            scale = 0.95f;
            alpha = 1.0f;
            break;
        case 4:
            scale = 1.0f;
            alpha = 1.0f;
            break;
        }
        sprite.x = 0;
        sprite.y = 0;
        sprite.w = 640.0f * scale;
        sprite.h = 448.0f * scale;
        sprite.x -= (s16) ((sprite.w - 640) / 2);
        sprite.y -= (s16) ((sprite.h - 448) / 2);
        sprite.color = ((u8) (255.0f * alpha) << 24) | 0xFFFFFF;
        AreaObj_K17_DrawSprite(&sprite);
        switch (obj->var1) {
        case 0:
        case 1:
            break;
        case 2:
            scale = 1.15f;
            alpha = 0.5f;
            break;
        case 3:
            scale = 0.9f;
            alpha = 1.0f;
            break;
        case 4:
            scale = 1.0f;
            alpha = 1.0f;
            break;
        }
        switch (obj->var1) {
        case 0:
        case 1:
            break;
        case 2:
        case 3:
        case 4:
            sprite.w = scale * AreaObj_K17_RectW[typeIdx];
            sprite.h = scale * AreaObj_K17_RectH[typeIdx];
            sprite.x = AreaObj_K17_RectX[typeIdx] - sprite.w / 2;
            sprite.y = AreaObj_K17_RectY[typeIdx] - sprite.h / 2;
            shakeIdx = obj->pos[1];
            if (shakeIdx >= 8) {
                shakeIdx = 0;
            }
            sprite.x += D_008F58C0[shakeIdx + typeIdx * 8];
            sprite.y += D_008F58F0[shakeIdx + typeIdx * 8];
            sprite.color = ((u8) (255.0f * alpha) << 24) | 0xFFFFFF;
            switch (obj->type) {
            case 4:
            case 6:
                sprite.u0 = 0;
                sprite.v0 = 0;
                sprite.u1 = sprite.u0 + 255;
                sprite.v1 = sprite.v0 + 111;
                break;
            case 7:
                sprite.u0 = 0;
                sprite.v0 = 112;
                sprite.u1 = sprite.u0 + 159;
                sprite.v1 = sprite.v0 + 143;
                break;
            }
            switch (obj->type) {
            case 4:
                AreaObj_K17_DrawTexRect(&sprite.x, (u16) (s32) (0.5f + (65536.0f * obj->pos[0]) / 360.0f));
                break;
            case 7:
                AreaObj_K17_DrawTexRect(&sprite.x, (u16) (s32) (0.5f + (65536.0f * obj->pos[0]) / 360.0f));
                break;
            case 6:
                AreaObj_K17_DrawPart(&sprite.x, (u16) (s32) (0.5f + (65536.0f * obj->pos[0]) / 360.0f));
                break;
            }
            break;
        }
        switch (obj->var1) {
        case 0:
            if (obj->timer >= 15) {
                obj->var1++;
            }
            break;
        case 1:
            if (obj->timer >= 60) {
                obj->var1++;
                obj->pos[0] = AreaObj_K17_Spread1[typeIdx] * (-1.0f + (2.0f * Rand_Next(1)) / 65536.0f);
                obj->pos[1] = (u16) Rand_Next(1) & 8;
                switch (obj->type) {
                case 4:
                    Bgm_PlayEvent(42);
                    break;
                case 7:
                    Bgm_PlayEvent(44);
                    break;
                case 6:
                    Bgm_PlayEvent(43);
                    break;
                }
            }
            break;
        case 2:
        case 3:
            obj->var1++;
            break;
        }
    }
}

/*
 * Creates the volcano hazard spawner, an area object of kind 25 that keeps
 * the current area in var3. Area_StartObjects calls it in AREA_VOLCANO_1, 2,
 * 5 and 6.
 */
void AreaObj_K19_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 25;
        obj->update = AreaObj_K19_Update;
        *(u16 *) &obj->var3 = gScene.area;
    }
}

/*
 * The spawner's update. By the object's state (0 to 3) it runs
 * VolcanoHazardSpawner_Setup, _Run, _End or _Free.
 */
void AreaObj_K19_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaObj_K19_Setup(obj);
        break;
    case 1:
        AreaObj_K19_Run(obj);
        break;
    case 2:
        AreaObj_K19_End(obj);
        break;
    case 3:
        AreaObj_K19_Free(obj);
        break;
    }
}

/*
 * Goes to the next state and makes the spawner visible, with no draw slot and
 * its timer at 0.
 */
void AreaObj_K19_Setup(Obj *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var0 = 0;
    obj->timer = 0;
    obj->ds = NULL;
}

/*
 * When the timer runs out and no scripted event (gScene.eventRunning) is
 * running, it does three things at one of the area's points: creates a
 * PlayerFx (STAGEOBJ_KIND_PLAYER_FX) of type 15 (scale 0.7), starts a VolcanoHazard of
 * type 1 lasting 130 frames, and sets the timer to 300. It does this only on
 * a frame whose quest time (time left minus the limit) has bits 4 to 6 clear;
 * on other frames it waits for the next such frame. The point is the quest
 * time in 300-frame steps plus the quest's first seed byte, modulo the point
 * count. An area without points creates nothing.
 */
void AreaObj_K19_Run(Obj *obj)
{
    u8 *point;
    s32 questTime;
    u16 pick;
    s16 count;

    questTime = Quest_GetTime(0) - Quest_GetTime(1);
    pick = questTime / 300;
    pick += Quest_GetSeedByte(0);
    if (--obj->timer <= 0) {
        if ((questTime & 0x40) != 64) {
            if ((questTime & 0x30) == 0) {
                if (gScene.eventRunning == 0) {
                    point = (u8 *) AreaObj_K19_GetPoints(obj->var3);
                    count = AreaObj_K19_PointCount(obj->var3);
                    if (point == NULL || count == 0) {
                        return;
                    }
                    point += (pick % count) * 12;
                    PlayerFx_CreateAt((f32 *) point, 15, 150, 0.7f);
                    VolcanoHazard_Create((f32 *) point, 1, 130);
                    obj->timer = 300;
                }
            } else {
                obj->timer = 0;
            }
        } else {
            obj->timer = 0;
        }
    }
}

/*
 * Frees the draw slot if the spawner has one, goes to the next state and
 * hides it.
 */
void AreaObj_K19_End(Obj *obj)
{
    if (obj->ds != NULL) {
        DrawSlot_Free(obj->slot);
    }
    obj->state++;
    obj->visible = 0;
}

/* Frees the spawner's object. */
void AreaObj_K19_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Returns, as an address, the hazard points ({x, y, z} floats) of `area`:
 * VolcanoHazardSpawner_PointsVolcano1, 2, 5 or 6. Returns 0 for any other
 * area.
 */
s32 AreaObj_K19_GetPoints(u16 area)
{
    s32 areaId;

    areaId = area & 0xFFFF;
    switch (areaId) {
    case 162:
        return (s32) AreaObj_K19_PointsA2;
    case 163:
        return (s32) AreaObj_K19_PointsA3;
    case 166:
        return (s32) AreaObj_K19_PointsA6;
    case 167:
        return (s32) AreaObj_K19_PointsA7;
    default:
        return 0;
    }
}

/*
 * Returns how many hazard points `area` has (6, 3, 2 and 5 for
 * AREA_VOLCANO_1, 2, 5 and 6), or 0 for any other area.
 */
s16 AreaObj_K19_PointCount(u16 area)
{
    s32 areaId;

    areaId = area & 0xFFFF;
    switch (areaId) {
    case 162:
        return 6;
    case 163:
        return 3;
    case 166:
        return 2;
    case 167:
        return 5;
    default:
        return 0;
    }
}

/* The file's data. */
extern u32 D_008F5290[];
extern u32 D_008F52C0[];
extern u32 D_008F52D8[];
extern u32 D_008F52F0[];
extern u32 D_008F5320[];
extern u32 D_008F5330[];
extern u32 D_008F5360[];
extern u32 D_008F5370[];
extern u32 D_008F5390[];
extern u32 D_008F53B8[];
extern u32 D_008F53C8[];
extern u32 D_008F53D8[];
extern u32 D_008F53F0[];
extern u32 D_008F5420[];
extern u32 D_008F5428[];
extern u32 D_008F5440[];
extern u32 D_008F5470[];
extern u32 D_008F54A0[];
extern u32 D_008F54B0[];
extern u32 D_008F54E0[];
extern u32 D_008F54F0[];
extern u32 D_008F5510[];
extern u32 D_008F5540[];
extern u32 D_008F5548[];
extern u32 D_008F5560[];
extern u32 D_008F5590[];
extern u32 D_008F55A0[];
extern u32 D_008F55D0[];
extern u32 D_008F55E0[];
extern u32 D_008F5610[];
extern u32 D_008F5620[];
extern u32 D_008F5640[];
extern u32 D_008F5660[];
extern u32 D_008F5680[];
extern u32 D_008F56A0[];
extern u32 D_008F56D0[];
extern u32 D_008F5700[];
extern u32 D_008F5720[];
extern u32 D_008F5748[];
extern u32 D_008F5750[];
extern u32 D_008F5768[];
extern u32 D_008F5780[];
extern u32 D_008F57B0[];
extern u32 D_008F57C0[];
extern u32 D_008F57D8[];
extern u32 D_008F57F0[];
extern u32 D_008F5818[];
extern u32 D_008F5830[];
extern u32 D_008F5848[];
extern u32 D_008F5860[];
extern u32 D_008F5888[];
extern u32 D_008F58A0[];
f32 FortressTremor_PosX = {
#include "game/008B0AA8.inc"
};
f32 FortressTremor_PosZ = {
#include "game/008B0AB0.inc"
};
f32 FortressTremor_ImpactOffsets[12] = {
#include "game/008B0AC0.inc"
};
f32 BreakableStructure_Pos[5][3] = {
#include "game/008B0AF0.inc"
};
s16 BreakableStructure_ModelsDay[8] = {
#include "game/008B0B30.inc"
};
u16 BreakableStructure_Yaw[8] = {
#include "game/008B0B40.inc"
};
f32 BreakableStructure_Rise[6] = {
#include "game/008B0B50.inc"
};
s16 BreakableStructure_Tilt[6] = {
#include "game/008B0B68.inc"
};
f32 BreakableStructure_SoundPos[8] = {
#include "game/008B0B80.inc"
};
f32 BreakableStructure_EffectPos[3][3] = {
#include "game/008B0BA0.inc"
};
s16 BreakableStructure_ModelsNight[6] = {
#include "game/008B0BC8.inc"
};
f32 ScrollingOverlay_FlipbookUV[16][2] = {
#include "game/008B0BE0.inc"
};
f32 ScrollingOverlay_ForestHillsPos[4][3] = {
#include "game/008B0C60.inc"
};
f32 ScrollingOverlay_RowsDesertNight7[5][3] = {
#include "game/008B0C90.inc"
};
f32 ScrollingOverlay_RowsGreatArena[5][3] = {
#include "game/008B0CD0.inc"
};
f32 ScrollingOverlay_RowsDesert2[4][3] = {
#include "game/008B0D10.inc"
};
f32 ScrollingOverlay_RowsDesert3[4][3] = {
#include "game/008B0D40.inc"
};
f32 ScrollingOverlay_RowsDesert9[4][3] = {
#include "game/008B0D70.inc"
};
s16 ScrollingOverlay_SlotsForestHills[4] = {
#include "game/008B0DA0.inc"
};
s16 ScrollingOverlay_SlotsDesertNight7[4] = {
#include "game/008B0DA8.inc"
};
s16 ScrollingOverlay_SlotsGreatArena[4] = {
#include "game/008B0DB0.inc"
};
s16 ScrollingOverlay_SlotsDesert2[4] = {
#include "game/008B0DB8.inc"
};
s16 ScrollingOverlay_SlotsDesert3[4] = {
#include "game/008B0DC0.inc"
};
s16 ScrollingOverlay_SlotsDesert9[4] = {
#include "game/008B0DC8.inc"
};
s16 ScrollingOverlay_SlotsDesert10[8] = {
#include "game/008B0DD0.inc"
};
f32 TurningProp_Pos[5][3] = {
#include "game/008B0DE0.inc"
};
f32 TurningProp_Scales[6] = {
#include "game/008B0E20.inc"
};
s16 TurningProp_Models[6] = {
#include "game/008B0E38.inc"
};
u8 GroundTileGrid_Jungle1Tiles[32] = {
#include "game/008B0E50.inc"
};
u8 GroundTileGrid_Jungle6Tiles[80] = {
#include "game/008B0E70.inc"
};
u16 GroundTileGrid_Jungle6Turns[72] = {
#include "game/008B0EC0.inc"
};
s16 AnimatedScenery_Hidden_1[4] = {
#include "game/008B0F50.inc"
};
f32 AnimatedScenery_Positions_2[3] = {
#include "game/008B0F58.inc"
};
s16 AnimatedScenery_Hidden_2[4] = {
#include "game/008B0F68.inc"
};
f32 AnimatedScenery_Positions_3[4] = {
#include "game/008B0F70.inc"
};
s16 AnimatedScenery_Hidden_3[8] = {
#include "game/008B0F80.inc"
};
f32 AnimatedScenery_Positions_4[10] = {
#include "game/008B0F90.inc"
};
s16 AnimatedScenery_Hidden_4[4] = {
#include "game/008B0FB8.inc"
};
f32 AnimatedScenery_Positions_5[4] = {
#include "game/008B0FC0.inc"
};
s16 AnimatedScenery_Hidden_5[4] = {
#include "game/008B0FD0.inc"
};
f32 AnimatedScenery_Positions_6[3] = {
#include "game/008B0FD8.inc"
};
s16 AnimatedScenery_Hidden_6[4] = {
#include "game/008B0FE8.inc"
};
f32 AnimatedScenery_Positions_7[6] = {
#include "game/008B0FF0.inc"
};
s16 AnimatedScenery_Hidden_7[4] = {
#include "game/008B1008.inc"
};
s16 AnimatedScenery_Hidden_9[4] = {
#include "game/008B1010.inc"
};
s16 AnimatedScenery_Hidden_10[4] = {
#include "game/008B1018.inc"
};
s16 AnimatedScenery_Hidden_13[4] = {
#include "game/008B1020.inc"
};
s16 AnimatedScenery_Hidden_16[4] = {
#include "game/008B1028.inc"
};
s16 AnimatedScenery_Hidden_17[4] = {
#include "game/008B1030.inc"
};
s16 AnimatedScenery_Hidden_18[4] = {
#include "game/008B1038.inc"
};
s16 AnimatedScenery_Hidden_19[4] = {
#include "game/008B1040.inc"
};
s16 AnimatedScenery_Hidden_21[4] = {
#include "game/008B1048.inc"
};
s16 AnimatedScenery_Hidden_0[4] = {
#include "game/008B1050.inc"
};
f32 AnimatedScenery_Positions_25[3] = {
#include "game/008B1058.inc"
};
s16 AnimatedScenery_Hidden_25[4] = {
#include "game/008B1068.inc"
};
s16 AnimatedScenery_Hidden_26[4] = {
#include "game/008B1070.inc"
};
s16 AnimatedScenery_Hidden_28[4] = {
#include "game/008B1078.inc"
};
s16 AnimatedScenery_Hidden_32[4] = {
#include "game/008B1080.inc"
};
s16 AnimatedScenery_Hidden_33[4] = {
#include "game/008B1088.inc"
};
s16 AnimatedScenery_Hidden_34[4] = {
#include "game/008B1090.inc"
};
s16 AnimatedScenery_Hidden_35[4] = {
#include "game/008B1098.inc"
};
s16 AnimatedScenery_Hidden_36[8] = {
#include "game/008B10A0.inc"
};
f32 AnimatedScenery_Positions_37[10] = {
#include "game/008B10B0.inc"
};
s16 AnimatedScenery_Hidden_37[4] = {
#include "game/008B10D8.inc"
};
s16 AnimatedScenery_Hidden_38[4] = {
#include "game/008B10E0.inc"
};
s16 AnimatedScenery_Hidden_39[4] = {
#include "game/008B10E8.inc"
};
s16 AnimatedScenery_Hidden_8[8] = {
#include "game/008B10F0.inc"
};
f32 AnimatedScenery_Positions_41[12] = {
#include "game/008B1100.inc"
};
s16 AnimatedScenery_Hidden_42[4] = {
#include "game/008B1130.inc"
};
s16 AnimatedScenery_Hidden_43[4] = {
#include "game/008B1138.inc"
};
f32 AnimatedScenery_Positions_45[4] = {
#include "game/008B1140.inc"
};
s16 AnimatedScenery_Hidden_45[4] = {
#include "game/008B1150.inc"
};
s16 AnimatedScenery_Hidden_46[4] = {
#include "game/008B1158.inc"
};
s16 AnimatedScenery_Hidden_47[4] = {
#include "game/008B1160.inc"
};
s16 AnimatedScenery_Hidden_48[4] = {
#include "game/008B1168.inc"
};
s16 AnimatedScenery_Hidden_49[4] = {
#include "game/008B1170.inc"
};
s16 AnimatedScenery_Hidden_51[4] = {
#include "game/008B1178.inc"
};
s16 AnimatedScenery_Hidden_50[4] = {
#include "game/008B1180.inc"
};
s16 AnimatedScenery_Hidden_52[4] = {
#include "game/008B1188.inc"
};
s16 AnimatedScenery_Hidden_53[4] = {
#include "game/008B1190.inc"
};
s16 AnimatedScenery_Hidden_54[4] = {
#include "game/008B1198.inc"
};
f32 *AnimatedScenery_Positions[56] = {
    (f32 *)D_008F5440, (f32 *)D_008F5290, AnimatedScenery_Positions_2, AnimatedScenery_Positions_3,
    AnimatedScenery_Positions_4, AnimatedScenery_Positions_5, AnimatedScenery_Positions_6, AnimatedScenery_Positions_7,
    (f32 *)D_008F5720, (f32 *)D_008F52C0, (f32 *)D_008F52D8, (f32 *)D_008F52F0,
    (f32 *)D_008F5330, (f32 *)D_008F5370, AnimatedScenery_Positions_2, AnimatedScenery_Positions_6,
    (f32 *)D_008F5390, (f32 *)D_008F53B8, (f32 *)D_008F53C8, (f32 *)D_008F53D8,
    (f32 *)D_008F53F0, (f32 *)D_008F5428, (f32 *)D_008F5440, (f32 *)D_008F5470,
    (f32 *)D_008F54B0, AnimatedScenery_Positions_25, (f32 *)D_008F54F0, (f32 *)D_008F5510,
    (f32 *)D_008F5548, (f32 *)D_008F5560, (f32 *)D_008F55A0, (f32 *)D_008F55E0,
    (f32 *)D_008F5620, (f32 *)D_008F5640, (f32 *)D_008F5660, (f32 *)D_008F5680,
    (f32 *)D_008F56A0, AnimatedScenery_Positions_37, (f32 *)D_008F56D0, (f32 *)D_008F5700,
    (f32 *)D_008F5720, AnimatedScenery_Positions_41, (f32 *)D_008F5750, (f32 *)D_008F5768,
    (f32 *)D_008F5780, AnimatedScenery_Positions_45, (f32 *)D_008F57C0, (f32 *)D_008F57D8,
    (f32 *)D_008F57F0, (f32 *)D_008F5818, (f32 *)D_008F5848, (f32 *)D_008F5830,
    (f32 *)D_008F5860, (f32 *)D_008F5888, (f32 *)D_008F58A0, (f32 *)D_008F58A0,
};
s16 *AnimatedScenery_Hidden[56] = {
    AnimatedScenery_Hidden_0, AnimatedScenery_Hidden_1, AnimatedScenery_Hidden_2, AnimatedScenery_Hidden_3,
    AnimatedScenery_Hidden_4, AnimatedScenery_Hidden_5, AnimatedScenery_Hidden_6, AnimatedScenery_Hidden_7,
    AnimatedScenery_Hidden_8, AnimatedScenery_Hidden_9, AnimatedScenery_Hidden_10, (s16 *)D_008F5320,
    (s16 *)D_008F5360, AnimatedScenery_Hidden_13, AnimatedScenery_Hidden_2, AnimatedScenery_Hidden_6,
    AnimatedScenery_Hidden_16, AnimatedScenery_Hidden_17, AnimatedScenery_Hidden_18, AnimatedScenery_Hidden_19,
    (s16 *)D_008F5420, AnimatedScenery_Hidden_21, AnimatedScenery_Hidden_0, (s16 *)D_008F54A0,
    (s16 *)D_008F54E0, AnimatedScenery_Hidden_25, AnimatedScenery_Hidden_26, (s16 *)D_008F5540,
    AnimatedScenery_Hidden_28, (s16 *)D_008F5590, (s16 *)D_008F55D0, (s16 *)D_008F5610,
    AnimatedScenery_Hidden_32, AnimatedScenery_Hidden_33, AnimatedScenery_Hidden_34, AnimatedScenery_Hidden_35,
    AnimatedScenery_Hidden_36, AnimatedScenery_Hidden_37, AnimatedScenery_Hidden_38, AnimatedScenery_Hidden_39,
    AnimatedScenery_Hidden_8, (s16 *)D_008F5748, AnimatedScenery_Hidden_42, AnimatedScenery_Hidden_43,
    (s16 *)D_008F57B0, AnimatedScenery_Hidden_45, AnimatedScenery_Hidden_46, AnimatedScenery_Hidden_47,
    AnimatedScenery_Hidden_48, AnimatedScenery_Hidden_49, AnimatedScenery_Hidden_50, AnimatedScenery_Hidden_51,
    AnimatedScenery_Hidden_52, AnimatedScenery_Hidden_53, AnimatedScenery_Hidden_54, AnimatedScenery_Hidden_54,
};
s16 AnimatedScenery_FirstModel[56] = {
#include "game/008B1360.inc"
};
f32 AnimatedScenery_FlipbookUV[16][2] = {
#include "game/008B13D0.inc"
};
f32 CastleGate_PosOpen[2][3] = {
#include "game/008B1450.inc"
};
f32 CastleGate_PosShut[2][3] = {
#include "game/008B1470.inc"
};
s16 CastleGate_Models[4] = {
#include "game/008B1488.inc"
};
f32 CastleGate_SoundPos[6] = {
#include "game/008B1490.inc"
};
s16 AreaObj_K17_RectX[4] = {
#include "game/008B14A8.inc"
};
s16 AreaObj_K17_RectY[4] = {
#include "game/008B14B0.inc"
};
s16 AreaObj_K17_RectW[4] = {
#include "game/008B14B8.inc"
};
s16 AreaObj_K17_RectH[4] = {
#include "game/008B14C0.inc"
};
f32 AreaObj_K17_Spread1[3] = {
#include "game/008B14C8.inc"
};
s16 AreaObj_K17_AngleTbl[6] = {
#include "game/008B14D8.inc"
};
u8 AreaObj_K19_PointsA2[80] = {
#include "game/008B14F0.inc"
};
u8 AreaObj_K19_PointsA3[48] = {
#include "game/008B1540.inc"
};
u8 AreaObj_K19_PointsA6[32] = {
#include "game/008B1570.inc"
};
/* Area object kind 0xA7: five points (x, y, z). */
u8 AreaObj_K19_PointsA7[60] = {
#include "game/008B1590.inc"
};
