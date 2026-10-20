/*
 * Two small pieces of sub_main's effects.
 *
 * - The ground effects: the puffs a footfall or a landing raises on the
 *   ground below (Effect_SpawnGround, by the ground kind the STAGEOBJ_KIND_PART_FX
 *   effects look up for their type), with their sounds; and
 *   Player_SpawnGroundEffect, a STAGEOBJ_KIND_PART_FX effect on the ground at a player's
 *   weapon (the motion effects' slams).
 * - The lobby's lock-on mark: a stage object of kind STAGEOBJ_KIND_LOCK_ON_MARK that the town
 *   spawns for the local player (TownPc_InitAll). While the player deals
 *   with a townsperson (Player.target), a mark turns and bobs above it, at
 *   a height by what the townsperson is doing (gLockOnMarkHeights) and in a
 *   colour of its own (gLockOnMarkColors); the lobby hides it for a frame
 *   through gLockOnMarkSkip while a menu or the keyboard is open.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"

void StageObj_Free(StageObj *o);
void DrawSlot_Free(s32 id);
void LockOnMark_Setup(StageObj *o);
void LockOnMark_Run(StageObj *o);
void LockOnMark_End(StageObj *o);
void LockOnMark_Delete(StageObj *o);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Obj_InCurrentArea(void *obj);
StageObj *StageObj_Alloc(s32 nblk);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Mtx_GetTrans(f32 *out, f32 *m);
f32 Math_Sin(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void Mtx_RotateY(f32 *m, f32 a);
s16 Effect_MapTexId(s16 id);
void AttackBurst_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 groundKind, f32 scale);

void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale);
void Effect08_SpawnAtEm(Em *em, s32 k, s32 b, f32 scale, f32 y);
/*
 * Spawns the ground effects of ground kind `groundKind` at pos (kind 8
 * effects: dust, splashes and the like, two of them a frame apart in
 * kinds 2 and 8) and raises pos by 5. Kind 1 (and any unknown kind) plays
 * the landing playSound 0x2E; the others play step playSound 0x76 when the puffs'
 * scale is above 4, else 0x70, except kind 4 and, seven frames in eight,
 * kind 5. Kind 5 follows its owner (an Em). Returns 0 for kind 0 (no
 * ground effect) and for kind 5 without an owner, else 1.
 */
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 groundKind, f32 scale)
{
    s32 playSound = 1;
    f32 puffScale;

    switch (groundKind) {
    case 0:
        return 0;
    case 1:
    default:
        puffScale = 1.2f * scale;
        Effect08_Spawn(pos, 3, 0, puffScale);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 0, 0, puffScale);
        Sound_PlaySeAtPosChecked(1, 0x2E, 0, pos, 1, 0);
        return 1;
    case 2:
        if (gScene.frame & 2) {
            Effect08_Spawn(pos, 4, 0, 1.5f * scale);
        }
        pos[1] += 5.0f;
        puffScale = 2.0f * scale;
        Effect08_Spawn(pos, 2, 0, puffScale);
        break;
    case 3:
        puffScale = 1.5f * scale;
        Effect08_Spawn(pos, 4, 0, puffScale);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, puffScale);
        break;
    case 4:
        playSound = 0;
    case 9:
        puffScale = 2.7f * scale;
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 1, 0, puffScale);
        break;
    case 5:
        if (owner == NULL) {
            return 0;
        }
        puffScale = 2.0f * scale;
        pos[1] += 5.0f;
        Effect08_SpawnAtEm(owner, 6, 0, puffScale, pos[1]);
        if (!(gScene.frame & 7)) {
            playSound = 0;
        }
        break;
    case 6:
        puffScale = 2.5f * scale;
        Effect08_Spawn(pos, 4, 0, puffScale);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, puffScale);
        break;
    case 7:
        puffScale = 3.0f * scale;
        Effect08_Spawn(pos, 4, 0, puffScale);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, puffScale);
        break;
    case 8:
        if (gScene.frame & 4) {
            Effect08_Spawn(pos, 4, 0, 1.5f * scale);
        }
        puffScale = 2.0f * scale;
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 2, 0, puffScale);
        break;
    }
    if (playSound != 0) {
        if (puffScale > 4.0f) {
            Sound_PlaySeAtPosChecked(1, 0x76, 0, pos, 1, 0);
        } else {
            Sound_PlaySeAtPosChecked(1, 0x70, 0, pos, 1, 0);
        }
    }
    return 1;
}

void PartFx_SpawnOnGround(f32 *pos, s32 type, f32 scale);

/*
 * When the player is in the current area: a STAGEOBJ_KIND_PART_FX effect of type `kind`
 * on the ground (at the player's floor height) below its weapon model's
 * bone 3. Type 0x13 adds an attack burst along the player's heading and a
 * type 0xE effect. The calls take only the low byte of Obj_InCurrentArea's
 * result and of `kind` (the casts), as the game's code does.
 */
void Player_SpawnGroundEffect(Player *pl, s32 kind, f32 scale)
{
    f32 pos[3];

    if ((u8)Obj_InCurrentArea(pl) != 0) {
        Mtx_GetTrans(pos, pl->weaponModel->partBones[0][3].world);
        pos[1] = pl->floorY;
        PartFx_SpawnOnGround(pos, (u8)kind, scale);
        if ((u8)kind == 0x13) {
            AttackBurst_CreateAtScaled(pos, *(u16 *)&pl->rot[1], 10, 0.3f);
            PartFx_SpawnOnGround(pos, 0xE, 0.9f);
        }
    }
}

void LockOnMark_Update(StageObj *o);

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

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void LockOnMark_Draw(DrawSlot *s);

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

s32 Npc_HasSecondHead(Em *em);
s32 Npc_IsFree(Em *em);
s32 Npc_IsAct51to55(Em *em);
s32 Npc_IsPet(Em *em);
s32 Npc_IsType4D(Em *em);
s32 Npc_IsType49(Em *em);
s32 LobbyTime_IsAm(void);
f32 Math_Sin(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
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
        if (Npc_IsType4D(npc) == 1) {
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
            OT_AddByDepth(gDrawOt32, mark->node, 0x20, 0);
            return;
        }
        OT_AddByDepth(gDrawOt32, mark->node, 0x20, 0);
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

s16 Effect_MapTexId(s16 id);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
extern s32 gLockOnMarkColors[];
extern s8 gLockOnMarkSkip;
/*
 * Draws the lock-on mark: the common models' entry for model 0x86, seven
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
    matSet = Effect_MapTexId(0x86);
    if (matSet == -1) {
        return;
    }
    model = &set->matSets[matSet];
    mats = set->mats;
    Mtx_Scale(m[0], 7.0f, 7.0f, 7.0f);
    Mtx_RotateY(m[0], 6.2831855f * (mark->timer << 8) / 65536.0f);
    Mtx_SetTrans(m[0], drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
    Gs_SetState(0x67, gLockOnMarkColors[npc->bowgunReady]);
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
