/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_npc.h"

void EmSlot_Free(struct Em *em);

void Npc_Free(Em *npc) {
    EmSlot_Free(npc);
}
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
s32 Player_UpdateMotion(Player *p);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Debug_IsOn(s32 arg0);

void Npc_ClearAll43B(void)
{
    s32 i;

    for (i = 0; i < 20; i++) {
        ((u8 *)&gEms[i])[0x43B] = 0;
    }
}

s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Npc_Init(Em *a);
s32 Npc_Run(Em *arg0);
void Npc_RunHook3(Em *npc);
void Npc_UpdateAreaPos(Em *npc);
s32 Game_IsOnline(void);

s32 Npc_Update(Em *npc)
{
    switch (npc->taskPhase) {
    case 0:
        Npc_Init(npc);
        Npc_RunHook3(npc);
        Npc_UpdateAreaPos(npc);
        return 0;
    case 1:
        if (Npc_Run(npc) == 0) {
            return 1;
        }
        break;
    case 2:
        Npc_Free(npc);
        return 1;
    }
    Npc_RunHook3(npc);
    if (npc->visible != 0) {
        if (npc->area == gScene.area || Game_IsOnline() == 0) {
            npc->drawSlot->pos[0] = npc->posX;
            npc->drawSlot->pos[1] = npc->posY;
            npc->drawSlot->pos[2] = npc->posZ;
            OT_AddByDepth(gDrawOt32, npc->drawSlot, 0x20, 0);
        }
    }
    Npc_UpdateAreaPos(npc);
    return 0;
}

void Npc_UpdateMotion(Em *p)
{
    if (Debug_IsOn(0x28) == 0) {
        Mtx_FromAnglesXYZ(&p->rotX, (f32 *) p->rotMtx);
        if (p->hitStop == 0) {
            if (p->motionStarted[0] == 0) {
                p->motionStarted[0]++;
                Player_BeginLayerMotion((Player *) p, p->motionFrame[0], p->motionBlend[0], 0);
            }
            if (p->motionStarted[1] == 0 && p->motionLayerCount > 1) {
                p->motionStarted[1]++;
                Player_BeginLayerMotion((Player *) p, p->motionFrame[1], p->motionBlend[1], 1);
            }
            Player_UpdateMotion((Player *) p);
        }
    }
}

void Mtx_Scale(f32 *, f32, f32, f32);
void Mtx_Concat3x3(f32 *m, f32 *n);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Copy(f32 *dst, f32 *src);
f32 Math_Angle16ToRad(u16 a);
struct EmBone;
void Bone_UpdateWorldScaledRoot(struct EmBone *sk, f32 *m);
void Mtx_RotateY(f32 *, f32);
f32 *Em_BoneMatrixB(void *obj, s16 bone);
s32 Actor_HeadingDiff(void *obj, void *pos);
s32 Npc_IsBusyNotAct3C4B(Em *arg0);
s32 Npc_IsAct51to55(Em *arg0);
s32 Npc_IsAct4For50(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s32 Npc_IsType4E(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);

/* The townsperson's model matrix (scale, angles, position); unless it is busy,
 * its head bone turns toward the local player (at most 10 or 20 degrees). */
void Npc_UpdateMatrix(Em *npc)
{
    f32 m[16];
    f32 scale[16];
    Model *model;
    NpcWork *st;
    u16 still;
    f32 *bone;
    f32 limit;
    f32 a;

    st = (NpcWork *)&npc->work;
    model = (npc)->model;
    Mtx_Scale(scale, npc->scaleX, npc->scaleY, npc->scaleZ);
    Mtx_FromAnglesYXZ(&npc->rotX, m);
    Mtx_SetTrans(m, npc->posX, npc->posY, npc->posZ);
    Mtx_Concat3x3(m, scale);
    Mtx_Copy(&npc->worldMtx[0][0], m);
    switch (st->id) {
    case 1:
    case 5:
    case 0x21:
    case 0x55:
    case 0x59:
    case 0x5A:
    case 0x5B:
        still = 1;
        break;
    default:
        still = 0;
        break;
    }
    if (Npc_IsType4D(npc) == 1) {
        still = 1;
    }
    if (Npc_IsType3Cto3E(npc) == 1) {
        still = 1;
    }
    if (Npc_IsType4E(npc) == 1) {
        still = 1;
    }
    switch (npc->motionId[0]) {
    case 0x284:
    case 0x286:
        still = 1;
        break;
    }
    if (Npc_IsAct51to55(npc) == 1 || Npc_IsAct4For50(npc) == 1) {
        still = 1;
    }
    if (!still) {
        if (Npc_IsBusyNotAct3C4B(npc) != 0) {
            bone = Em_BoneMatrixB(npc, 0xC);
            limit = 10.0f;
        } else {
            bone = Em_BoneMatrixB(npc, 0x13);
            limit = 20.0f;
        }
        a = 180.0f * Math_Angle16ToRad(Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos)) / 3.1415927f;
        if (!(a <= 180.0f)) {
            a = -(a - 180.0f);
        }
        if (!(a <= -90.0f) || a < 90.0f) {
            if (a < -limit) {
                a = -limit;
            } else if (!(a <= limit)) {
                a = limit;
            }
            a = st->headYaw + 0.2f * (a - st->headYaw);
            Mtx_RotateY(bone, 3.1415927f * a / 180.0f);
            st->headYaw = a;
        }
    }
    Bone_UpdateWorldScaledRoot(model->bones, m);
}

/* A townsperson: +0x3D0 points to the handler table of its kind. */
void Npc_RunHook3(Em *npc)
{
    (npc)->handlers->unk0C(npc);
}

/* An empty function that the table at gNpcHandlers_4 points to (round 5, lobby village 2). */
void func_0077E520(void)
{
}

void func_0077E530(void) {

}
