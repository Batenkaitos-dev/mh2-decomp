/*
 * The shooting gallery's target board (GalleryBoard, a lobby stage object of
 * kind LOBBY_OBJ_GALLERY_BOARD): its update and its four states. The board
 * lays its 15 targets out in 3 rows of 5, flips them up, holds them standing
 * for the course's frames and flips them down, then waits to be refilled by
 * the gallery's round. Types 1 to 9 of the same kind are the bursts a hit
 * leaves, each a single drawn effect with a short life.
 *
 * The board's draw, its spawn and the queries the gallery's shots make
 * follow in the next file; the targets' picture draw after them. The data
 * at the end: the targets' sizes and kinds and the rows' first spots.
 */
#include "common.h"
#include "sprite.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
/* Built at optimisation level 4: the compiler gives these functions the
 * game's code only at this level. */
#pragma optimization_level 4
void DrawSlot_Free(s32);

void GalleryBoard_Init(StageObj *o);
void GalleryBoard_Run(StageObj *o);
void GalleryBoard_End(StageObj *o);
void GalleryBoard_Free(StageObj *o);

/* The update of the board and its bursts: runs the object's state. */
void GalleryBoard_Update(StageObj *obj)
{
    u8 state;

    state = obj->state;
    switch (state) {
    case LOBBY_OBJ_INIT:
        GalleryBoard_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        GalleryBoard_Run(obj);
        break;
    case LOBBY_OBJ_END:
        GalleryBoard_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        GalleryBoard_Free(obj);
        break;
    }
}

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
u16 Rand_Next(s32 stream);
f32 Rand_Unit(void);
u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
void GalleryBoard_Draw(DrawSlot *);
extern f32 gCamMatrix[];
extern Vec3f gGalleryRowBases[];
extern s8 gGalleryBurstDelaysA[];
extern s8 gGalleryBurstDelaysB[];

/* The init state. The board lays its 15 targets out in 3 rows of 5, the
 * rows' first spots from gGalleryRowBases and a step of 100 along the
 * camera's x axis between targets; each target stands flipped down, facing
 * away from the board's direction, with its kind's hits and a draw slot.
 * A burst sets its life, its start delay (bursts 4 to 9 start one after
 * another) and its roll, and takes one draw slot; without one it frees
 * itself. */
void GalleryBoard_Init(StageObj *obj)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;
    u8 *kinds;
    Vec3f rowStep;
    u8 i;
    u8 col;
    s8 sign;

    obj->var0 = GALLERY_BOARD_FLIP;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->state = LOBBY_OBJ_RUN;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    if (obj->type == GALLERY_BOARD_TARGETS) {
        kinds = board->kinds;
        board->turnStep = GALLERY_FLIP_STEP;
        board->flipTimer = GALLERY_FLIP_FRAMES;
        board->flipDown = 0;
        board->cleared = 0;
        board->facing[15] = 0;
        rowStep.x = -500.0f;
        rowStep.y = 0.0f;
        rowStep.z = 0.0f;
        Vec_Rotate(&rowStep.x, &rowStep.x, gCamMatrix);
        rowStep.x *= 0.2f;
        rowStep.y *= 0.2f;
        rowStep.z *= 0.2f;
        for (i = 0; i < GALLERY_TARGETS; i++) {
            board->downed[i] = 0;
            board->swing[i] = 0;
            board->hitsLeft[i] = gGalleryTargetKinds[kinds[i]].hits;
            board->tilt[i] = GALLERY_TILT_DOWN;
            board->facing[i] = Math_Heading(board->dir.x, board->dir.z, 0.0f, 0.0f) + 0x8000;   /* half a turn */
            board->fallFrames[i] = 0;
            col = i % GALLERY_ROW_TARGETS;
            board->spots[i].x = gGalleryRowBases[(u8)(i / GALLERY_ROW_TARGETS)].x + rowStep.x * col;
            board->spots[i].y = gGalleryRowBases[(u8)(i / GALLERY_ROW_TARGETS)].y + rowStep.y * col;
            board->spots[i].z = gGalleryRowBases[(u8)(i / GALLERY_ROW_TARGETS)].z + rowStep.z * col;
            board->slot[i] = DrawSlot_Alloc();
            if (board->slot[i] != -1) {
                board->parts[i] = DrawSlot_Get(board->slot[i]);
                board->parts[i]->owner = obj;
                board->parts[i]->part = i;
                board->parts[i]->draw = GalleryBoard_Draw;
            } else {
                board->parts[i] = NULL;
            }
        }
    } else {
        switch (obj->type) {
        case GALLERY_BURST_DRIFT1:
            board->facing[15] = 0;
            board->life = 6;     /* frames */
            break;
        case GALLERY_BURST_DRIFT2:
            board->facing[15] = 0;
            board->life = 16;     /* frames */
            break;
        case GALLERY_BURST_DRIFT3:
            board->facing[15] = Rand_Next(1);
            board->life = 12;     /* frames */
            break;
        case GALLERY_BURST_STILL1:
        case GALLERY_BURST_STILL2:
        case GALLERY_BURST_STILL3:
            obj->timer = gGalleryBurstDelaysA[obj->type - GALLERY_BURST_STILL1];
            board->facing[15] = Rand_Next(1);
            board->life = 6;     /* frames */
            break;
        case GALLERY_BURST_SPIN1:
        case GALLERY_BURST_SPIN2:
        case GALLERY_BURST_SPIN3:
            obj->timer = gGalleryBurstDelaysB[obj->type - GALLERY_BURST_SPIN1];
            if (Rand_Next(1) % 2 == 1) {
                sign = -1;
            } else {
                sign = 1;
            }
            /* a roll of 20 to 30 degrees over 8 frames, either way */
            board->turnStep = (u16)(s32)(0.5f + 65536.0f * (sign * ((20.0f + 10.0f * Rand_Unit()) / 8.0f)) / 360.0f);
            board->facing[15] = Rand_Next(1);
            board->life = 8;     /* frames */
            break;
        }
        obj->slot = DrawSlot_Alloc();
        if (obj->slot != -1) {
            obj->node = DrawSlot_Get(obj->slot);
            obj->node->owner = obj;
            obj->node->draw = GalleryBoard_Draw;
        } else {
            StageObj_Free(obj);
        }
    }
}

u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
/* Starts the board's next round: its targets flipped down and facing away
 * again, none downed, to flip up and stand for the course's frames. */
void GalleryBoard_Reset(StageObj *obj)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;
    u8 i;

    board->flipTimer = GALLERY_FLIP_FRAMES;
    board->flipDown = 0;
    board->cleared = 0;
    board->holdLeft = board->hold;
    for (i = 0; i < GALLERY_TARGETS; i++) {
        board->downed[i] = 0;
        board->tilt[i] = GALLERY_TILT_DOWN;
        board->facing[i] = Math_Heading(board->dir.x, board->dir.z, 0.0f, 0.0f) + 0x8000;
        board->fallFrames[i] = 0;
    }
}

void Vec_Copy(f32 *dst, f32 *src);
void Vec_Normalize(f32 *v);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Gallery_BeforeShooting(void);
extern f32 gGalleryTargetSizes[];

/* The run state. The board ends once the gallery leaves its shooting steps
 * (Gallery_BeforeShooting) and starts its next round once cleared. Each
 * frame a downed target falls, then leaves its spot (downing a kind that
 * ends the round flips the whole board down); the others turn while the
 * board flips. Every target is drawn at its spot, raised by half its size.
 * The phases: a flip up, the hold, a flip down, then the clear wait.
 * Bursts 1 to 3 drift along the camera's direction for their life; bursts
 * 4 to 9 show for 6 or 8 frames once their start delay has run, 7 to 9
 * rolling. */
void GalleryBoard_Run(StageObj *obj)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;
    u8 *kinds = board->kinds;
    Vec3f camDir;
    u8 i;

    switch (obj->type) {
    case GALLERY_BOARD_TARGETS:
        if (Gallery_BeforeShooting() == 0) {
            obj->state = LOBBY_OBJ_END;
            GalleryBoard_End(obj);
            break;
        }
        if (board->cleared == 1) {
            GalleryBoard_Reset(obj);
            break;
        }
        for (i = 0; i < GALLERY_TARGETS; i++) {
            if (board->parts[i] != NULL) {
                if (kinds[i] > 0) {
                    if (board->downed[i] == 1) {
                        if (++board->fallFrames[i] < GALLERY_FALL_FRAMES) {
                            board->tilt[i] -= GALLERY_FALL_STEP;
                        } else {
                            if (gGalleryTargetKinds[kinds[i]].endsRound == 1) {
                                board->cleared = 1;
                                board->flipTimer = GALLERY_FLIP_FRAMES;
                                obj->var0 = GALLERY_BOARD_FLIP;
                                board->flipDown = 1;
                            }
                            kinds[i] = 0;
                            board->swing[i] = 0;
                        }
                    } else if (obj->var0 <= GALLERY_BOARD_FLIP) {
                        if (board->flipDown == 1) {
                            board->tilt[i] -= board->turnStep;
                        } else {
                            board->tilt[i] += board->turnStep;
                        }
                    }
                }
                Vec_Copy(board->parts[i]->pos, &board->spots[board->spotOrder[i]].x);
                board->parts[i]->pos[1] += gGalleryTargetSizes[gGalleryTargetKinds[kinds[i]].size] / 2.0f;
                OT_AddByDepth(gDrawOt32, board->parts[i], 0x40, 0);
            }
        }
        switch (obj->var0) {
        case GALLERY_BOARD_FLIP:
            board->cleared = 0;
            if (board->flipTimer-- <= 0) {
                if (board->flipDown == 0) {
                    obj->var0 = GALLERY_BOARD_STAND;
                } else {
                    board->clearWait = GALLERY_CLEAR_WAIT;
                    obj->var0 = GALLERY_BOARD_CLEARING;
                }
            }
            break;
        case GALLERY_BOARD_STAND:
            if (board->holdLeft-- <= 0) {
                board->flipDown = 1;
                board->flipTimer = GALLERY_FLIP_FRAMES;
                obj->var0 = GALLERY_BOARD_FLIP;
            }
            break;
        case GALLERY_BOARD_CLEARING:
            if (--board->clearWait < 0) {
                board->cleared = 1;
                obj->var0 = GALLERY_BOARD_FLIP;
            }
            break;
        }
        break;
    case GALLERY_BURST_DRIFT1:
    case GALLERY_BURST_DRIFT2:
    case GALLERY_BURST_DRIFT3:
        if (++obj->timer > board->life) {
            obj->timer = 0;
            obj->state = LOBBY_OBJ_END;
            break;
        }
        camDir.x = gCamDir[0];
        camDir.y = gCamDir[1];
        camDir.z = gCamDir[2];
        Vec_Normalize(&camDir.x);
        obj->pos[0] += 5.0f * camDir.x;
        obj->pos[1] += 5.0f * camDir.y;
        obj->pos[2] += 5.0f * camDir.z;
        Vec_Copy(obj->node->pos, &obj->pos[0]);
        OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
        break;
    case GALLERY_BURST_STILL1:
    case GALLERY_BURST_STILL2:
    case GALLERY_BURST_STILL3:
        if (++obj->timer >= 0) {
            if (obj->timer > 6) {     /* frames */
                obj->state = LOBBY_OBJ_END;
            } else {
                Vec_Copy(obj->node->pos, &obj->pos[0]);
                OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
            }
        }
        break;
    case GALLERY_BURST_SPIN1:
    case GALLERY_BURST_SPIN2:
    case GALLERY_BURST_SPIN3:
        if (++obj->timer >= 0) {
            if (obj->timer > 8) {     /* frames */
                obj->state = LOBBY_OBJ_END;
            } else {
                board->facing[15] += board->turnStep;
                Vec_Copy(obj->node->pos, &obj->pos[0]);
                OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
            }
        }
        break;
    }
}

/* The end state: frees the board's draw slots, or a burst's one. */
void GalleryBoard_End(StageObj *obj)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;
    u8 i;

    switch (obj->type) {
    case GALLERY_BOARD_TARGETS:
        obj->state = LOBBY_OBJ_FREE;
        for (i = 0; i < GALLERY_TARGETS; i++) {
            if (board->parts[i] != NULL) {
                DrawSlot_Free(board->slot[i]);
            }
        }
        break;
    default:
        obj->state = LOBBY_OBJ_FREE;
        if (obj->node != NULL) {
            DrawSlot_Free(obj->slot);
        }
        break;
    }
}

/* The free state: frees the object. */
void GalleryBoard_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/* The targets' sizes (by GalleryTargetKind.size: a picture's side) and
 * kinds, and the first spot of each row (the fourth is unused). */
f32 gGalleryTargetSizes[4] = {
#include "lobby/007DCAC0.inc"
};
GalleryTargetKind gGalleryTargetKinds[22] = {
#include "lobby/007DCAD0.inc"
};
Vec3f gGalleryRowBases[4] = {
#include "lobby/007DCB80.inc"
};
