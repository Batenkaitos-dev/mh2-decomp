/*
 * The shooting gallery's target board (GalleryBoard), continued: its draw,
 * its spawn, the bursts a hit leaves, and the queries the gallery's shots
 * make of a target (its position, facing, tilt, size, picture and points;
 * counting a hit off it, downing it).
 *
 * A target is drawn as its kind's picture (GalleryBoard_DrawTarget); a
 * burst as one of the common effect models, scaled and coloured by key
 * tables by its frame. The data at the end: the bursts' scale and colour
 * keys by type, their start delays, and the bursts of a hit.
 */
#include "common.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "render_state.h"
#include "scene.h"
#include "gallery_targets.h"
#include "engine_math.h"
#include "gallery_board.h"
#include "gallery_objects.h"
#include "shooting_gallery.h"
/* Built at optimisation level 4: the compiler gives these functions the
 * game's code only at this level. */
#pragma optimization_level 4

extern u8 gGalleryHitBursts[];

extern Vec3Key gGalleryBurstScaleKeys0[];
extern ColorKey gGalleryBurstColorKeys0[];
extern Vec3Key gGalleryBurstScaleKeys1[];
extern ColorKey gGalleryBurstColorKeys1[];
extern Vec3Key gGalleryBurstScaleKeys2[];
extern ColorKey gGalleryBurstColorKeys2[];
extern Vec3Key gGalleryBurstScaleKeys3[];
extern ColorKey gGalleryBurstColorKeys3[];
extern Vec3Key gGalleryBurstScaleKeys4[];
extern ColorKey gGalleryBurstColorKeys4[];
extern f32 gCamMatrix[];

/* The draw slot callback of the board and its bursts. For the board, the
 * slot's target is drawn while it holds a kind and the board stands (a
 * swing left on an empty or hidden target is stopped). A burst draws its
 * type's effect model at the slot's position, facing the camera, rolled by
 * facing[15], its scale and colour from the type's keys at its frame, over
 * everything (the depth test off while it draws). */
void GalleryBoard_Draw(DrawSlot *slot)
{
    StageObj *obj = slot->owner;
    Model *mdl = gCommonModels[0];
    GalleryBoard *board = (GalleryBoard *)obj->buf;
    ModelMat *mats;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 scale[3];
    f32 rot[3];
    s32 color;
    s32 i;
    u8 kind;

    if (mdl != NULL && mdl->active != 0 && obj->visible != 0) {
        ModelMatSet *mesh;
        Vec3Key *scaleKeys;
        ColorKey *colKeys;
        mats = mdl->mats;
        color = -1;
        switch (obj->type) {
        case GALLERY_BOARD_TARGETS:
            i = slot->part;
            kind = board->kinds[i];
            if (kind > 0 && obj->var0 < GALLERY_BOARD_CLEARING && board->cleared == 0) {
                GalleryBoard_DrawTarget((Vec3f *)slot->pos, kind, board->tilt[i], board->facing[i], &board->swing[i]);
            } else if (board->swing[i] != 0) {
                board->swing[i] = 0;
            }
            break;
        case GALLERY_BURST_DRIFT1:
            scaleKeys = gGalleryBurstScaleKeys0;
            colKeys = gGalleryBurstColorKeys0;
            mesh = &mdl->matSets[20];
            break;
        case GALLERY_BURST_DRIFT2:
            scaleKeys = gGalleryBurstScaleKeys1;
            colKeys = gGalleryBurstColorKeys1;
            mesh = &mdl->matSets[18];
            break;
        case GALLERY_BURST_DRIFT3:
            scaleKeys = gGalleryBurstScaleKeys2;
            colKeys = gGalleryBurstColorKeys2;
            mesh = &mdl->matSets[31];
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            break;
        case GALLERY_BURST_STILL1:
        case GALLERY_BURST_STILL2:
        case GALLERY_BURST_STILL3:
            scaleKeys = gGalleryBurstScaleKeys3;
            colKeys = gGalleryBurstColorKeys3;
            mesh = &mdl->matSets[13];
            break;
        case GALLERY_BURST_SPIN1:
        case GALLERY_BURST_SPIN2:
        case GALLERY_BURST_SPIN3:
            scaleKeys = gGalleryBurstScaleKeys4;
            colKeys = gGalleryBurstColorKeys4;
            mesh = &mdl->matSets[14];
            break;
        }
        if (obj->type > GALLERY_BOARD_TARGETS) {
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            rot[2] = 6.2831855f * (f32)(u32)board->facing[15] / 65536.0f;
            Key_LerpVec3((f32 *) scaleKeys, (f32 *) scale, (f32)obj->timer);
            Mtx_FromScaleRotTrans((f32 *) scale, (f32 *) rot, slot->pos, 6, (f32 *) mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Color_EvalKeys((s32 *) colKeys, (f32)obj->timer, (u32 *) &color);
            Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
            Effect_DrawModel(mesh, mtx, color, 2, mats);
            Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
        }
    }
}

/* Creates the board: the targets' kinds and the spots they stand on (the
 * round's PrizeMenu.slots and .order), the board's direction, and the
 * frames the targets stand; registers it as the gallery's targets. */
void GalleryBoard_Spawn(u8 *kinds, u8 *order, Vec3f *boardDir, f32 holdFrames)
{
    Vec3f dirCopy = *boardDir;
    StageObj *obj;
    GalleryBoard *board;

    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = LOBBY_OBJ_GALLERY_BOARD;
        obj->type = GALLERY_BOARD_TARGETS;
        obj->update = GalleryBoard_Update;
        board = (GalleryBoard *)obj->buf;
        board->kinds = kinds;
        board->spotOrder = order;
        board->hold = board->holdLeft = (s32)holdFrames;
        board->dir = dirCopy;
        Gallery_AddObj(obj, GALLERY_OBJ_TARGETS);
    }
}

/* Writes target i's position (its draw slot's) to out. */
void GalleryBoard_GetTargetPos(Vec3f *out, StageObj *obj, u8 i)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    *out = *(Vec3f *)board->parts[i]->pos;
}

/* Marks target i downed: it falls from the next frame. */
void GalleryBoard_DownTarget(StageObj *obj, u8 i)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    board->downed[i] = 1;
}

/* Returns 1 when target i is downed. */
u8 GalleryBoard_IsTargetDown(StageObj *obj, u8 i)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    return board->downed[i];
}

/* Returns 1 once the board has flipped down (its clear wait, or cleared):
 * its targets can't be hit; else 0. */
s32 GalleryBoard_IsFlippedDown(StageObj *obj)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    if (obj->var0 < GALLERY_BOARD_CLEARING && board->cleared == 0) {
        return 0;
    }
    return 1;
}

/* Spawns the six bursts of a downed target (types 1 to 6) at pos. */
void GalleryBurst_SpawnDowned(Vec3f *pos)
{
    u8 i;
    StageObj *obj;

    for (i = GALLERY_BURST_DRIFT1; i < GALLERY_BURST_SPIN1; i++) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = LOBBY_OBJ_GALLERY_BOARD;
            obj->type = i;
            obj->update = GalleryBoard_Update;
            Vec_Copy((f32 *) &obj->pos, (f32 *) pos);
        }
    }
}

/* Spawns the five bursts of a hit that leaves its target standing (the
 * types of gGalleryHitBursts) at pos. */
void GalleryBurst_SpawnHit(Vec3f *pos)
{
    u8 i;
    u8 *burstType = gGalleryHitBursts;
    StageObj *obj;

    for (i = 0; i < GALLERY_HIT_BURSTS; i++, burstType++) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = LOBBY_OBJ_GALLERY_BOARD;
            obj->type = *burstType;
            obj->update = GalleryBoard_Update;
            Vec_Copy((f32 *) &obj->pos, (f32 *) pos);
        }
    }
}

/* Returns target i's size class (gGalleryTargetSizes' index). */
u8 GalleryBoard_GetTargetSize(StageObj *obj, u8 i)
{
    return gGalleryTargetKinds[((GalleryBoard *)obj->buf)->kinds[i]].size;
}

/* Returns target i's yaw (ANGLE_TURN to a turn). */
u16 GalleryBoard_GetTargetFacing(StageObj *obj, u8 i)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    return board->facing[i];
}

/* Returns target i's tilt about x, in radians. */
f32 GalleryBoard_GetTargetTilt(StageObj *obj, u8 i)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    return 6.2831855f * (u32)board->tilt[i] / 65536.0f;
}

/* Returns target i's kind's picture (the gallery picks the hit's cry by
 * it). */
u16 GalleryBoard_GetTargetPicture(StageObj *obj, u8 i)
{
    return gGalleryTargetKinds[((GalleryBoard *)obj->buf)->kinds[i]].picture;
}

/* Counts a hit off target i: returns 1 when that was its last (it goes
 * down), else 0. */
s32 GalleryBoard_HitTarget(StageObj *obj, u8 i)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    return --board->hitsLeft[i] <= 0;
}

/* Returns the points target i's kind gives. */
s16 GalleryBoard_GetTargetPoints(StageObj *obj, u8 i)
{
    u8 *kind = ((GalleryBoard *)obj->buf)->kinds;
    GalleryTargetKind *kindTable = gGalleryTargetKinds;

    kind = &kind[i];
    return kindTable[*kind].points;
}

/* The bursts' scale and colour keys: 0 to 2 for bursts 1 to 3, 3 for
 * bursts 4 to 6, 4 for bursts 7 to 9. Then the start delays of bursts 4
 * to 6 and 7 to 9 (by type, from their first), and the types of a hit's
 * bursts (five used). */
Vec3Key gGalleryBurstScaleKeys0[4] = {
#include "lobby/007DCBB0.inc"
};
ColorKey gGalleryBurstColorKeys0[4] = {
#include "lobby/007DCBF0.inc"
};
Vec3Key gGalleryBurstScaleKeys1[6] = {
#include "lobby/007DCC10.inc"
};
ColorKey gGalleryBurstColorKeys1[6] = {
#include "lobby/007DCC70.inc"
};
Vec3Key gGalleryBurstScaleKeys2[6] = {
#include "lobby/007DCCA0.inc"
};
ColorKey gGalleryBurstColorKeys2[6] = {
#include "lobby/007DCD00.inc"
};
Vec3Key gGalleryBurstScaleKeys3[4] = {
#include "lobby/007DCD30.inc"
};
ColorKey gGalleryBurstColorKeys3[4] = {
#include "lobby/007DCD70.inc"
};
Vec3Key gGalleryBurstScaleKeys4[3] = {
#include "lobby/007DCD90.inc"
};
ColorKey gGalleryBurstColorKeys4[3] = {
#include "lobby/007DCDC0.inc"
};
s8 gGalleryBurstDelaysA[8] = {
#include "lobby/007DCDD8.inc"
};
s8 gGalleryBurstDelaysB[8] = {
#include "lobby/007DCDE0.inc"
};
u8 gGalleryHitBursts[8] = {
#include "lobby/007DCDE8.inc"
};
