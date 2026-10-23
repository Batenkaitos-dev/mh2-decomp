/*
 * The village's falling leaves (LobbyFlock), a sub_main object (Obj_Alloc)
 * with draw slots of its own: in AREA_ONLINE_VILLAGE outside the first
 * season, 16 pieces tumble down from above a patch of the village and start
 * again at the top when they reach the ground; each is drawn through its
 * own draw slot by LobbyFlock_DrawPart. The file has no data of its own.
 */

#include "common.h"
#include "lobby_vobj.h"
#include "obj.h"
#include "render_state.h"
#include "falling_leaves.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "rand.h"
#include "vec_math.h"

/* spawns the falling leaves (an Obj of kind 24; LobbyFlock_Run) */
void LobbyFlock_Spawn(void)
{
    Obj *flock;

    flock = Obj_Alloc(1);
    if (flock != NULL) {
        flock->kind = 24;
        flock->type = 0;
        flock->update = (void *)LobbyFlock_Run;
    }
}

/* the leaves' update, by state */
void LobbyFlock_Run(LobbyFlock *flock)
{
    switch (flock->state) {
    case LOBBY_OBJ_INIT:
        LobbyFlock_Init(flock);
        break;
    case LOBBY_OBJ_RUN:
        LobbyFlock_Update(flock);
        break;
    case LOBBY_OBJ_END:
        LobbyFlock_End(flock);
        break;
    case LOBBY_OBJ_FREE:
        LobbyFlock_Free(flock);
        break;
    }
}

/* puts a leaf back at the top: at rest, at random angles, at a random
 * place over the patch the leaves fall on */
void LobbyFlock_ResetPart(LobbyPart *part)
{
    part->vel[0] = 0.0f;
    part->vel[1] = 0.0f;
    part->vel[2] = 0.0f;
    part->rot[0] = Rand_Next(1);
    part->rot[1] = Rand_Next(1);
    part->rot[2] = Rand_Next(1);
    part->spin = 0;
    part->unk16 = 0;
    if (part->draw != NULL) {
        part->draw->pos[0] = 5500.0f + ((u16)Rand_Next(1) & 0x7FF);
        part->draw->pos[1] = 2500.0f;
        part->draw->pos[2] = 10300.0f + ((u16)Rand_Next(1) & 0x3FF);
    }
}

/* sets the leaves up: a draw slot for each of the 16 (drawn by
 * LobbyFlock_DrawPart), each put at the top */
void LobbyFlock_Init(LobbyFlock *flock)
{
    LobbyPart *piece;
    s16 i;

    piece = flock->parts;
    flock->state++;
    flock->visible = 1;
    flock->draw = NULL;
    flock->unk5 = (u16)Rand_Next(1) & 3;
    flock->pos[0] = 5500.0f + ((u16)Rand_Next(1) & 0x7FF);
    flock->pos[1] = 3000.0f;
    flock->pos[2] = 10300.0f + ((u16)Rand_Next(1) & 0x3FF);
    flock->timer = 0;
    for (i = 0; i < 16; i++, piece++) {
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->draw = DrawSlot_Get(piece->slot);
            piece->draw->owner = flock;
            piece->draw->part = i;
            piece->draw->draw = LobbyFlock_DrawPart;
        } else {
            piece->draw = NULL;
        }
        LobbyFlock_ResetPart(piece);
    }
}

/* each leaf tumbles (random changes to its spin and roll, both damped), is
 * pushed along its own down axis, slowed (most of all vertically) and
 * moved; below the ground it starts again at the top */
void LobbyFlock_Update(LobbyFlock *flock)
{
    f32 move[3];
    f32 mtx[16];
    LobbyPart *part;
    s16 i;

    i = 0;
    part = flock->parts;
    do {
        part->spin = 0.965f * part->spin;
        part->unk16 = 0.975f * part->unk16;
        part->spin += (s16)((Rand_Next(1) & 0xFF) - 128);
        part->unk16 += (s16)(((u16)Rand_Next(1) & 0x7F) - 64);
        part->rot[0] += part->spin;
        part->rot[1] += (u16)(((u16)Rand_Next(1) & 0x1F) - 15);
        part->rot[2] += part->unk16;
        Mtx_Identity(mtx);
        Mtx_RotateX(mtx, 6.2831855f * part->rot[0] / 65536.0f);
        Mtx_TurnY(mtx, 6.2831855f * part->rot[1] / 65536.0f);
        Mtx_TurnZ(mtx, 6.2831855f * part->rot[2] / 65536.0f);
        part->vel[0] += -4.0f * mtx[1];
        part->vel[1] += -4.0f * mtx[5];
        part->vel[2] += -4.0f * mtx[9];
        part->vel[0] *= 0.64f;
        part->vel[1] *= 0.25f;
        part->vel[2] *= 0.74f;
        Vec_Rotate(move, part->vel, mtx);
        if (part->draw != NULL) {
            part->draw->pos[0] += move[0];
            part->draw->pos[1] += move[1];
            part->draw->pos[2] += move[2];
            if (part->draw->pos[1] < -30.0f) {
                LobbyFlock_ResetPart(part);
            } else {
                OT_AddByDepth(gDrawOt32, part->draw, 32, 0);
            }
        }
        i++;
        part++;
    } while (i < 16);
}

/* an object's end state: its 16 parts' draw slots freed */
void LobbyFlock_End(LobbyFlock *flock)
{
    LobbyPart *part;
    s16 i;

    i = 0;
    part = flock->parts;
    flock->state++;
    flock->visible = 0;
    for (; i < 16; i++, part++) {
        if (part->draw != NULL) {
            DrawSlot_Free(part->slot);
        }
    }
}

void LobbyFlock_Free(void *flock)
{
    Obj_Free((Obj *)flock);
}

/* The draw callback of one of a LobbyFlock's 16 falling pieces (the slot's
 * owner is the flock, its part the piece's index; LobbyFlock_Init sets it):
 * when the area's first sub-model is loaded, draws its material set 7 at
 * the slot's position, turned by the piece's angles rot[0], rot[1], then
 * rot[0] again (rot[2] is not used). */
void LobbyFlock_DrawPart(DrawSlot *slot)
{
    f32 mtx[16];
    LobbyPart *piece;
    Model *subModel;
    ModelMatSet *matSet;

    piece = &((LobbyFlock *)slot->owner)->parts[slot->part];
    subModel = gAreaSubModels[0];
    if (subModel != NULL && subModel->active != 0) {
        matSet = &subModel->matSets[7];
        Mtx_Identity(mtx);
        Mtx_RotateX(mtx, piece->rot[0] * 6.2831855f / 65536.0f);
        Mtx_TurnY(mtx, piece->rot[1] * 6.2831855f / 65536.0f);
        Mtx_RotateX(mtx, piece->rot[0] * 6.2831855f / 65536.0f);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        if (matSet != NULL && matSet->id != -1) {
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
        Draw_SetDefaultState();
    }
}
