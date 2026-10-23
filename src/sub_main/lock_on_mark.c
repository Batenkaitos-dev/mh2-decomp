/*
 * The lobby's lock-on mark (sub_main): a stage object of kind
 * STAGEOBJ_KIND_LOCK_ON_MARK that the town spawns for the local player
 * (TownPc_InitAll). While the player deals with a townsperson
 * (Player.target), a mark turns and bobs above it, at a height by what the
 * townsperson is doing (gLockOnMarkHeights) and in a colour of its own
 * (gLockOnMarkColors); the lobby hides it for a frame through
 * gLockOnMarkSkip while a menu or the keyboard is open.
 */
#include "common.h"
#include "em.h"
#include "stageobj.h"
#include "player.h"
#include "sprite.h"
#include "render_state.h"
#include "lock_on_mark.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "lobby_state.h"
#include "area.h"
#include "lobby_npc.h"

/* Creates pl's lock-on mark, a stage object of kind STAGEOBJ_KIND_LOCK_ON_MARK that
 * LockOnMark_Update runs, when pl is in the current area. */
void LockOnMark_Spawn(Player *pl)
{
    StageObj *mark;

    if ((u8)Obj_InCurrentArea(pl) == 0) {
        return;
    }
    mark = StageObj_Alloc(0);
    if (mark == NULL) {
        return;
    }
    mark->kind = STAGEOBJ_KIND_LOCK_ON_MARK;
    mark->update = LockOnMark_Update;
    mark->draw = NULL;
    mark->owner = pl;
    mark->ownerIndex = pl->index;
}

/* The lock-on mark's update, by its state: set up, run, end, free. */
void LockOnMark_Update(StageObj *mark)
{
    switch (mark->state) {
    case 0:
        LockOnMark_Setup(mark);
        break;
    case 1:
        LockOnMark_Run(mark);
        break;
    case 2:
        LockOnMark_End(mark);
        break;
    case 3:
        LockOnMark_Delete(mark);
        break;
    }
}

/* Sets up the lock-on mark: a draw slot that LockOnMark_Draw draws; with
 * no slot free, the mark is freed at once. */
void LockOnMark_Setup(StageObj *mark)
{
    mark->state++;
    mark->var0 = 0;
    mark->var1 = 0;
    mark->visible = 1;
    mark->draw = NULL;
    mark->timer = 0;
    mark->slot = DrawSlot_Alloc();
    if (mark->slot != -1) {
        mark->node = DrawSlot_Get(mark->slot);
        mark->node->owner = mark;
        mark->node->draw = LockOnMark_Draw;
        return;
    }
    mark->node = NULL;
    LockOnMark_Delete(mark);
}

extern f32 gLockOnMarkHeights[];
/*
 * The lock-on mark's frame: while its player is active, visible, in the
 * current area and dealing with a townsperson (Player.target), the mark's
 * draw slot is placed above the townsperson, at the height
 * gLockOnMarkHeights gives for what it is doing (the lobby's Npc_* tests),
 * bobbing 4 up and down, and joins an ordering table: in the lobby the
 * sky's list in the afternoon (LobbyTime_IsAm 0), else gDrawOt32.
 */
void LockOnMark_Run(StageObj *mark)
{
    Player *pl = mark->owner;
    Em *npc;
    s16 heightIdx;
    f32 bob;

    if (pl->target != NULL) {
        if (pl->active == 0 || pl->visible == 0 || (u8)Obj_InCurrentArea(pl) == 0) {
            return;
        }
        npc = pl->target;
        mark->timer++;
        if (Npc_IsType77(npc) == 1) {
            heightIdx = 4;
        } else if (Npc_IsType49(npc) == 1) {
            heightIdx = 5;
        } else if (Npc_IsFree(npc) == 1) {
            heightIdx = 0;
        } else if (Npc_HasSecondHead(npc) == 1) {
            heightIdx = 1;
        } else if (Npc_IsAct51to55(npc) == 1) {
            heightIdx = 2;
        } else if (Npc_IsPet(npc) == 1) {
            heightIdx = 3;
        } else {
            heightIdx = 0;
        }
        bob = 4.0f * Math_Sin(6.2831855f * (u16)(mark->timer << 10) / 65536.0f);
        mark->node->pos[0] = npc->posX;
        mark->node->pos[1] = bob + (npc->posY + gLockOnMarkHeights[heightIdx]);
        mark->node->pos[2] = npc->posZ;
        if (gScene.inLobby != 0) {
            if (LobbyTime_IsAm() == 0) {
                OT_AddByDepth(gDrawListSky, mark->node, 1, 0);
                return;
            }
            OT_AddByDepth(gDrawOt32, mark->node, 32, 0);
            return;
        }
        OT_AddByDepth(gDrawOt32, mark->node, 32, 0);
    }
}

/* Ends the lock-on mark: frees its draw slot, if it has one. */
void LockOnMark_End(StageObj *mark)
{
    mark->state++;
    if (mark->node != NULL) {
        DrawSlot_Free(mark->slot);
    }
}

void LockOnMark_Delete(StageObj *mark)
{
    StageObj_Free(mark);
}

extern s32 gLockOnMarkColors[];
extern s8 gLockOnMarkSkip;
/*
 * Draws the lock-on mark: the common models' entry for model 134, seven
 * times its size, turning with the mark's timer, at the draw slot, in the
 * townsperson's colour (gLockOnMarkColors, by its byte at
 * Player.bowgunReady's offset). In the lobby it isn't drawn while its
 * player is in a weapon state, on a frame gLockOnMarkSkip is set (which
 * clears it), or while the local player crouches.
 */
void LockOnMark_Draw(DrawSlot *drawSlot)
{
    StageObj *mark = drawSlot->owner;
    Player *pl = mark->owner;
    Player *npc = ((Player *)pl->target); /* an Em, read as a Player for its colour byte */
    Model *set = gCommonModels[0];
    s16 matSet;
    ModelMat *mats;
    ModelMatSet *model;
    f32 m[4][4];

    if (npc == NULL) {
        return;
    }
    if (gScene.inLobby != 0) {
        if (pl->stateGroup == PL_GROUP_WEAPON) {
            return;
        }
        if (gLockOnMarkSkip != 0) {
            gLockOnMarkSkip = 0;
            return;
        }
        if (gPlayers[gScene.localPlayer].action == PL_POSTURE_CROUCHED) {
            return;
        }
    }
    matSet = Effect_MapTexId(134);
    if (matSet == -1) {
        return;
    }
    model = &set->matSets[matSet];
    mats = set->mats;
    Mtx_Scale(m[0], 7.0f, 7.0f, 7.0f);
    Mtx_RotateY(m[0], 6.2831855f * (mark->timer << 8) / 65536.0f);
    Mtx_SetTrans(m[0], drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
    Gs_SetState(RS_COLOR2, gLockOnMarkColors[npc->bowgunReady]);
    Effect_DrawModelAdd(model, m[0], mats);
}

/* The lock-on mark's heights above a townsperson by what it does, and its
 * colours. */
f32 gLockOnMarkHeights[8] = {
#include "sub_main/00467C40.inc"
};
s32 gLockOnMarkColors[6] = {
#include "sub_main/00467C60.inc"
};
