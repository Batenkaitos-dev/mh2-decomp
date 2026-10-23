/*
 * The monsters sub_main runs itself, without the game overlay (Em.ownTask
 * set): Field_UpdateAll calls Em_RunByState for them each frame, by
 * taskPhase. Phase 0 sets one up (Em_SetupKind: its model, scale, start
 * position, the meshes its variant shows, the do-nothing handler table
 * gEmNopHandlers, a draw slot drawn by Em_Draw, its first motions); phase 1
 * runs Em_UpdateState (idle and turn in place, the variant stepped by a
 * button for kind 1); phases 2 and 3 end and free it. Em_UpdateWorldMatrix
 * gives it its world matrix after the frame. Player_PrepareMotions, the
 * player's motion start and advance, sits among them.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "sprite.h"
#include "render_state.h"
#include "pad.h"
#include "light.h"
#include "text.h"
#include "quest.h"
#include "bgm.h"
#include "place_rules.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "rand.h"
#include "vec_math.h"
#include "area.h"

/* prototypes */
/* end of prototypes */

/* Draws a monster sub_main runs itself (the slot's owner): its world matrix, then each material set
 * of its model with its materials' alpha set to em->alpha. Kind 0 draws only the sets meshShown
 * marks; kind 1 draws them all, its materials' alpha 0 where meshShown[j] is 0 (j: the material's
 * place in the set, as the game indexes it). */
void Em_Draw(DrawSlot *slot)
{
    f32 rad[4];
    f32 mtx[4][4];
    f32 ident[4][4];
    f32 scale[4][4];
    s32 i;
    Em *em;
    s32 partCount;
    s32 j;
    ModelMatSet *part;
    ModelMat *mats;
    Model *model;

    em = slot->owner;
    model = em->model;
    if (em->active == 0 || em->visible == 0) {
        return;
    }
    LightSet_SetColors(em, 1);
    LightSet_FadeColors(em);
    Vec_AnglesToRad(&em->rotX, rad);
    Mtx_Scale(scale[0], em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, mtx[0]);
    Mtx_SetTrans(mtx[0], em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(mtx[0], scale[0]);
    Mtx_Copy((f32 *)em->worldMtx, mtx[0]);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    part = model->matSets;
    partCount = model->matSetCount;
    mats = model->mats;
    Bone_StoreMatrices(model->bones);
    Mtx_Identity(ident[0]);
    Gs_SetState(RS_UV_MTX, (s32)ident);
    Tex_Prepare(10, em->type * 20 + 163);
    for (i = 0; i < partCount; i++, part++) {
        if (em->type != 0 || (s8)em->meshShown[i] != 0) {
            Gs_SetState(RS_COLOR2, -1);
            if (part->id != -1) {
                for (j = 0; j < part->count; j++) {
                    ModelMat *mat = &mats[part->mat[j]];

                    switch (em->type) {
                    case 1:
                        if ((s8)em->meshShown[j] == 0) {
                            mat->alpha = 0.0f;
                        } else {
                            mat->alpha = em->alpha;
                        }
                        break;
                    default:
                        mat->alpha = em->alpha;
                        break;
                    }
                    Gs_SetState(RS_MATERIAL(j), (s32)mat);
                }
                Draw_ApplyStates(part->flags);
                Model_Draw(part->id, 0);
            }
        }
    }
    Draw_SetDefaultState();
    Gs_SetState(RS_ALPHA_REF, 0);
}

extern void *gEmNopHandlers[];
/* Sets up a monster: its handler table, the table's first handler, and a draw slot drawn by Em_Draw. */
void Em_InitHandlers(Em *em)
{
    em->handlers = (EmVt *)gEmNopHandlers;
    em->handlers->init(em);
    if (em->visible != 0) {
        em->drawSlotId = DrawSlot_Alloc();
        if (em->drawSlotId != -1) {
            em->drawSlot = DrawSlot_Get(em->drawSlotId);
            em->drawSlot->owner = em;
            em->drawSlot->draw = Em_Draw;
        }
    }
}

void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_SetMotion(Player *p, s32 motion, s32 a, s16 b, s32 layer);
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer); /* unit's own declaration: u16 and s16, as Player_PrepareMotions' calls need (the definition takes s32) */
void Player_UpdateAreaPos(void *obj);
extern f32 gStartPos[][3];
extern s16 gEmVariantShownMeshes[][2];
extern u16 *gEmVariantMeshes[];
/* Sets up a monster sub_main runs itself (taskPhase 0 of Em_RunByState; ownTask = 1): its model,
 * scale, the area's start position (kind 1: moved by 200 * variant), the meshes it shows
 * (meshShown, from gEmVariantShownMeshes or gEmVariantMeshes by variant), its handlers and draw
 * slot (Em_InitHandlers) and first motions. Kind 0 has two motion layers and is not flagged isEm. */
void Em_SetupKind(Em *em)
{
    u16 *list;
    u16 mesh;
    s32 i;

    EmSlot_SetupModel(em);
    em->taskPhase++;
    em->isEm = 0;
    em->ownTask = 1;
    em->hitPhase = 0;
    em->visible = 1;
    em->motion[0].step = 2.0f;
    em->motion[1].step = 2.0f;
    em->motion[2].step = 2.0f;
    em->motion[3].step = 2.0f;
    em->motionSpeed = 1.0f;
    if (em->type == 0) {
        em->motionLayerCount = 2;
    } else {
        em->isEm = 1;
        em->motionLayerCount = 1;
    }
    em->scaleX = 1.0f;
    em->scaleY = 1.0f;
    em->scaleZ = 1.0f;
    em->rotX = 0;
    em->rotY = 0;
    em->rotZ = 0;
    em->motionTime = 0;
    em->area = gScene.area;
    em->posX = gStartPos[em->area][0];
    em->posY = gStartPos[em->area][1];
    em->posZ = gStartPos[em->area][2];
    if (em->type == 1) {
        em->posX += 200.0f * em->variant;
        em->posZ += 200.0f * em->variant;
    }
    em->alpha = 1.0f;
    em->hideShadow = 0;
    em->fieldCollide = 1;
    em->motionId[0] = 0;
    em->motionId[1] = 0;
    em->motionId[2] = 0;
    em->motionId[3] = 0;
    Em_InitCollMask(em);
    for (i = 0; i < (s32)sizeof(em->meshShown); i++) {
        em->meshShown[i] = 0;
    }
    switch (em->type) {
    case 0:
        em->meshShown[gEmVariantShownMeshes[em->variant][0]] = 1;
        if (em->variant * 2 + 1 != 255) {
            em->meshShown[gEmVariantShownMeshes[em->variant][1]] = 1;
        }
        break;
    case 1:
        list = gEmVariantMeshes[em->variant];
        mesh = *list;
        while (mesh != 0xFFFF) {
            em->meshShown[mesh] = 1;
            list++;
            mesh = *list;
        }
        break;
    }
    Em_InitHandlers(em);
    if (em->type == 0) {
        Player_SetMotionPair((Player *)em, 1, 0, 0);
        Player_BeginLayerMotion((Player *)em, em->motionFrame[0], em->motionBlend[0], 0);
        Player_BeginLayerMotion((Player *)em, em->motionFrame[1], em->motionBlend[1], 1);
    } else {
        Player_SetMotion((Player *)em, 1001, 0, 0, 0);
        Player_BeginLayerMotion((Player *)em, em->motionFrame[0], em->motionBlend[0], 0);
    }
    Player_UpdateAreaPos(em);
}

struct TriHdr;
void Player_PrepareMotions(Player *p);
/* The state update of a monster sub_main runs itself (states 0 and 1 of group 0: idle and turn
 * in place by random times; kind 1 in state 0 steps its variant, of 6, when square is pressed and
 * shows that variant's meshes), then its matrix, motion speed and floor. */
void Em_UpdateState(Em *em)
{
    s32 type;
    u16 i;
    u16 *list;
    u16 mesh;

    em->noPushTimer = 10;
    Player_TickTimers((Player *) em);
    Player_UpdateHitStop(em);
    em->prevPosX = em->posX;
    em->prevPosY = em->posY;
    em->prevPosZ = em->posZ;
    type = em->type;
    switch (em->stateGroup) {
    case 0:
        switch (em->state) {
        case 0:
            if (type == 0) {
                if (em->step == 0) {
                    em->step++;
                    em->timer = (u8)(Rand_Next(1) + 60);
                    if (em->motionId[0] != 1) {
                        Player_SetMotionPair((Player *)em, 1, 4, 0);
                    }
                } else if (--em->timer <= 0) {
                    Player_ForceState(em, 0, 1);
                }
            } else {
                em->timer++;
                if (gPadState[0].pressed & PAD_SQUARE) {
                    em->variant++;
                    em->variant = em->variant % 6;
                    for (i = 0; i < 32; i++) {
                        em->meshShown[i] = 0;
                    }
                    list = gEmVariantMeshes[em->variant];
                    while ((mesh = *list) != 0xFFFF) {
                        list++;
                        em->meshShown[mesh] = 1;
                    }
                }
            }
            break;
        case 1:
            if (em->step == 0) {
                em->step++;
                em->stateVar0 = Rand_Next(1) & 1;
                em->timer = (u8)(Rand_Next(1) + 60);
                Player_SetMotionPair((Player *)em, 3, 4, 0);
            } else if (--em->timer <= 0) {
                Player_ForceState(em, 0, 0);
            } else if (em->stateVar0 == 0) {
                em->rotY += Rand_Next(1) & 0x1FF;
            } else {
                em->rotY -= Rand_Next(1) & 0x1FF;
            }
            break;
        }
        break;
    default:
        em->stateGroup = 0;
        em->state = 0;
        em->step = 0;
        break;
    }
    em->rotX = (u16)em->rotX;
    em->rotY = (u16)em->rotY;
    em->rotZ = (u16)em->rotZ;
    Mtx_FromAnglesYXZ(&em->rotX, em->rotMtx[0]);
    em->motion[0].step = 2.0f * em->motionSpeed;
    em->motion[1].step = 2.0f * em->motionSpeed;
    em->motion[2].step = 2.0f * em->motionSpeed;
    em->motion[3].step = 2.0f * em->motionSpeed;
    Player_PrepareMotions((Player *)em);
    Em_CollideField(em, 0);
    Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *)&em->floorKind, &em->floorY, &em->surfaceY);
    em->posY = em->floorY;
}

/* Steps the monster's taskPhase (Em_RunByState: from the end to the free). */
void Em_NextTaskPhase(Em *em)
{
    em->taskPhase++;
}

/* Frees the monster's slot (Em_RunByState's last phase). */
void Em_StateFree(Em *em)
{
    EmSlot_Free(em);
}

/* The frame of a monster sub_main runs itself, by taskPhase (0 set up, 1 run, 2 end, 3 free): runs
 * the handlers' hook, puts its draw slot at its position in the ordering table when it is visible
 * in the current area, and updates its area position. Returns 1 once freed. */
s32 Em_RunByState(Em *em)
{
    switch (em->taskPhase) {
    case 0:
        Em_SetupKind(em);
        Em_RunMotionFx(em);
        Player_UpdateAreaPos(em);
        return 0;
    case 1:
        Em_UpdateState(em);
        break;
    case 2:
        Em_NextTaskPhase(em);
        break;
    case 3:
        Em_StateFree(em);
        return 1;
    }
    Em_RunMotionFx(em);
    if (em->visible != 0 && em->area == gScene.area) {
        em->drawSlot->pos[0] = em->posX;
        em->drawSlot->pos[1] = em->posY;
        em->drawSlot->pos[2] = em->posZ;
        OT_AddByDepth(gDrawOt32, em->drawSlot, 32, 0);
    }
    Player_UpdateAreaPos(em);
    return 0;
}

/* unit's own declaration: the definition takes (Player *p, s32 frame, s32 n, s32 part); the game's
 * Player_PrepareMotions passes the frame as a u16 and the second value as an s16, so its calls
 * truncate them first */
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer); /* unit's own declaration: u16 and s16, as the calls below need (the definition takes s32) */
s32 Player_UpdateMotion(Player *p);
/* Unless debug switch 40 is on: the player's rotation matrix, then (while not in hit stop)
 * starts the motions pending on layers 0 and 1 and advances them. */
void Player_PrepareMotions(Player *pl)
{
    if (Debug_IsOn(40) == 0) {
        Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
        if (pl->hitStop == 0) {
            if (pl->motionStarted[0] == 0) {
                pl->motionStarted[0]++;
                Player_BeginLayerMotion(pl, pl->motionFrame[0], pl->motionBlend[0], 0);
            }
            if (pl->motionStarted[1] == 0 && pl->motionLayerCount > 1) {
                pl->motionStarted[1]++;
                Player_BeginLayerMotion(pl, pl->motionFrame[1], pl->motionBlend[1], 1);
            }
            Player_UpdateMotion(pl);
        }
    }
}

/* The monster's world matrix (scale, rotation, position) into worldMtx, and its skeleton's. */
void Em_UpdateWorldMatrix(Em *em)
{
    Model *model = em->model;
    f32 mtx[4][4];
    f32 scale[4][4];

    Mtx_Scale(scale[0], em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, mtx[0]);
    Mtx_SetTrans(mtx[0], em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(mtx[0], scale[0]);
    Mtx_Copy((f32 *)em->worldMtx, mtx[0]);
    Bone_UpdateWorldScaledRoot(model->bones, mtx[0]);
}

/* Calls the monster's motion-effects handler (EmVt.motionFx); for these monsters
 * gEmNopHandlers' Em_NopHandlerB. */
void Em_RunMotionFx(Em *em)
{
    em->handlers->motionFx(em);
}

void Em_NopHandlerA(void)
{
}

void Em_NopHandlerB(void)
{
}

/* The file's initialised data, in the game's order */
u16 gEmVariantMeshes_0[8] = {
#include "sub_main/004655E0.inc"
};
u16 gEmVariantMeshes_1[8] = {
#include "sub_main/004655F0.inc"
};
u16 gEmVariantMeshes_2[8] = {
#include "sub_main/00465600.inc"
};
u16 gEmVariantMeshes_3[8] = {
#include "sub_main/00465610.inc"
};
u16 gEmVariantMeshes_4[8] = {
#include "sub_main/00465620.inc"
};
u16 gEmVariantMeshes_5[8] = {
#include "sub_main/00465630.inc"
};
u16 *gEmVariantMeshes[8] = {
    gEmVariantMeshes_0, gEmVariantMeshes_1, gEmVariantMeshes_2, gEmVariantMeshes_3,
    gEmVariantMeshes_4, gEmVariantMeshes_5, NULL, NULL,
};
s16 gEmVariantShownMeshes[16][2] = {
#include "sub_main/00465660.inc"
};
void *gEmNopHandlers[8] = {
    (void *)Em_NopHandlerA, (void *)Em_NopHandlerA, (void *)Em_NopHandlerB, (void *)Em_NopHandlerB,
    NULL, NULL, NULL, NULL,
};
