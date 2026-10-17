#include "common.h"
#include "lobby_globals.h"
#include "em.h"
#include "sprite.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "player.h"

/* an entry of a motion script: at frame, start the motion (Actor_SetMotion)
 * and, when speed is positive, set the actor's speeds to twice it */
typedef struct ActorKey {
    /* 0x0 */ s16 frame;            /* -1 ends the script */
    /* 0x2 */ s16 motion;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ f32 speed;
} ActorKey;

/* the motion script of the actor LobbyActor_Update updates (gLobbyActorCurScript):
 * LobbyActor_GetScript(set, idx) finds it in gLobbyActorScripts, LobbyActor_RunScript starts the
 * entry of the current frame */
typedef struct ActorScript {
    /* 0x0 */ s16 set;
    /* 0x2 */ s16 idx;
    /* 0x4 */ s32 frame;
    /* 0x8 */ ActorKey *keys;
} ActorScript;

extern ActorScript gLobbyActorCurScript;





/* prototypes */
void Mtx_Concat3x3(f32 *, f32 *);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_Scale(f32 *, f32, f32, f32);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Copy(f32 *dst, f32 *src);
struct EmBone;
void Bone_UpdateWorldScaledUpToRoot(struct EmBone *b, f32 *m, u8 maxId);
s32 LobbyActor_RunScript(Em *o, s32 frame);
ActorKey *LobbyActor_GetScript(s32, s32);
extern ActorKey **gLobbyActorScripts[];
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Actor_StartMotions(Player *obj);
s32 Debug_IsOn(s32);
f32 AreaGrid2_GetFloorY(f32 *);
struct TriHdr;
s32 Em_GetFloorInfo(Em *em, f32 *pos, struct TriHdr *info, f32 *out, s32 *arg4);
void Player_TickTimers(Player *p);
void Player_UpdateHitStop(void *obj);
void LightSet_UpdateAll(void);
f32 *Em_BoneMatrix(void *obj, s16 bone);
struct EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
void Effect_RunFixedList(EquipFxSwing *a, EmBone *b1, EmBone *b2, EmBone *m);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void LobbyActor_SetMatrix(Em *o);
void LobbyActor_Draw(DrawSlot *ds);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_TurnY(f32 *m, f32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
s32 Gs_SetState(u8 reg, u32 v);
void Gfx_SetTexFilter(s32);
void LightSet_SetAmbient(void *obj, s32 n);
void LightSet_FadeAmbient(void *obj);
void Vec_AnglesToRad(s32 *in, f32 *out);
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
void Bone_StoreMatrices(struct EmBone *b);
void Tex_Prepare(s32 n, s32 tex);
Em *LobbyBgm_GetActor(void);
u8 func_007D9130(void);
/* end of prototypes */

void LobbyActor_DrawPart(DrawSlot *d)
{
    f32 m[16];
    LobbyPart *e;
    Model *p;
    ModelMatSet *part;

    e = &((LobbyFlock *)d->owner)->parts[d->part];
    p = gAreaSubModels[0];
    if (p != NULL && p->active != 0) {
        part = &p->matSets[7];
        Mtx_Identity(m);
        Mtx_RotateX(m, e->rot[0] * 6.2831855f / 65536.0f);
        Mtx_TurnY(m, e->rot[1] * 6.2831855f / 65536.0f);
        Mtx_RotateX(m, e->rot[0] * 6.2831855f / 65536.0f);
        Mtx_SetTrans(m, d->pos[0], d->pos[1], d->pos[2]);
        Gs_SetState(0x1A, (s32)m);
        if (part != NULL && part->id != -1) {
            Draw_ApplyStates(part->flags);
            Model_Draw(part->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* starts the actor's motion script (set, idx) at frame 0 */
void LobbyActor_StartScript(Em *o, s16 set, s16 idx)
{
    gLobbyActorCurScript.idx = idx;
    gLobbyActorCurScript.set = set;
    gLobbyActorCurScript.keys = LobbyActor_GetScript(gLobbyActorCurScript.set, gLobbyActorCurScript.idx);
    gLobbyActorCurScript.frame = 0;
    LobbyActor_RunScript(o, gLobbyActorCurScript.frame);
}

void LobbyActor_Update(Em *o)
{
    f32 m[16];
    Model *w;
    f32 *a;

    w = o->model;
    o->drawSlot->draw = LobbyActor_Draw;
    o->prevPosX = o->posX;
    o->prevPosY = o->posY;
    o->prevPosZ = o->posZ;
    gLobbyActorCurScript.frame++;
    LobbyActor_RunScript(o, gLobbyActorCurScript.frame);
    if (Debug_IsOn(0x10) == 0) {
        o->floorY = AreaGrid2_GetFloorY(&o->posX);
        Em_GetFloorInfo(o, &o->posX, (struct TriHdr *) &o->floorKind, &o->floorY, (s32 *) &o->surfaceY);
    }
    Player_TickTimers((Player *) o);
    Player_UpdateHitStop((Player *) o);
    LightSet_UpdateAll();
    LobbyActor_SetMatrix(o);
    a = Em_BoneMatrix(o, 0x1A);
    Mtx_Mul(m, a, (f32 *)(Em_GetBone(o, 0x17)));
    Effect_RunFixedList(((Player *) o)->equipFx, (EmBone *) m, (EmBone *) (Em_GetBone(o, 9)), &w->bones[27]);
    o->drawSlot->pos[0] = o->posX;
    o->drawSlot->pos[1] = o->posY;
    o->drawSlot->pos[2] = o->posZ;
    if (Debug_IsOn(0x24) == 0) {
        OT_AddByDepth(gDrawOt32, o->drawSlot, 0x20, 0);
    }
}

/* starts the script's entry for frame (if any), then the actor's motions */
s32 LobbyActor_RunScript(Em *o, s32 frame)
{
    ActorKey *k;

    k = gLobbyActorCurScript.keys;
    while (k->frame != -1) {
        if (k->frame == (s16)frame) {
            if (k->speed > 0.0f) {
                o->motion[0].step = 2.0f * k->speed;
                o->motion[1].step = 2.0f * k->speed;
            }
            Actor_SetMotion(o, k->motion, k->unk6, k->unk4);
            break;
        }
        k++;
    }

    Actor_StartMotions((Player *)o);
    return 0;
}

ActorKey *LobbyActor_GetScript(s32 set, s32 idx)
{
    return gLobbyActorScripts[(s16)set][(s16)idx];
}

/* the object's matrix from its angles (+0xA0), position (+0xAC) and scale
 * (+0xB8), set into its worldMtx and its model's bones */
void LobbyActor_SetMatrix(Em *o)
{
    f32 m[16];
    f32 s[16];
    Model *w;

    w = o->model;
    Mtx_Scale(s, o->scaleX, o->scaleY, o->scaleZ);
    Mtx_FromAnglesYXZ(&o->rotX, m);
    Mtx_SetTrans(m, o->posX, o->posY, o->posZ);
    Mtx_Concat3x3(m, s);
    Mtx_Copy((f32 *) o->worldMtx, m);
    Bone_UpdateWorldScaledUpToRoot(w->bones, m, 1);
}

/* the actor's draw callback: its matrix, then its parts' models */
void LobbyActor_Draw(DrawSlot *ds /* unused: the draw-slot walk passes it */)
{
    f32 rad[3];
    f32 m[16];
    f32 id[16];
    f32 s[16];
    Em *o;
    Model *w;
    ModelMatSet *p;
    ModelMat *mats;
    s32 i;
    s32 n;
    s32 j;


    o = LobbyBgm_GetActor();
    w = o->model;
    o->visible = func_007D9130();
    if (o->active == 0 || o->visible == 0) {
        return;
    }
    LightSet_SetAmbient(o, 1);
    LightSet_FadeAmbient(o);
    Vec_AnglesToRad(&o->rotX, rad);
    Mtx_Scale(s, o->scaleX, o->scaleY, o->scaleZ);
    Mtx_FromAnglesYXZ(&o->rotX, m);
    Mtx_SetTrans(m, o->posX, o->posY, o->posZ);
    Mtx_Concat3x3(m, s);
    Mtx_Copy((f32 *) o->worldMtx, m);
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    p = w->matSets;
    n = w->matSetCount;
    mats = w->mats;
    Bone_UpdateWorldScaledRoot(&w->bones[26], (f32 *) &w->bones[23]);
    Bone_StoreMatrices(w->bones);
    Mtx_Identity(id);
    Gs_SetState(0x19, (s32)id);
    Tex_Prepare(0xA, 0xA3);
    for (i = 0; i < n; i++, p++) {
        Gs_SetState(0x67, -1);
        if (p->id != -1) {
            for (j = 0; j < p->count; j++) {
                Gs_SetState((u8)(j + 0x3A), (s32)&mats[p->mat[j]]);
            }
            Draw_ApplyStates(p->flags);
            Model_Draw(p->id, 0);
        }
    }
    Draw_SetDefaultState();
    Gs_SetState(0x60, 0);
}

/* unk_7DB0A0's .data (0x81BCA0-0x825B10) */
u32 gLobbyActorScripts_0_0[16] = {
#include "lobby/0081BCA0.inc"
};
u32 gLobbyActorScripts_0_1[12] = {
#include "lobby/0081BCE0.inc"
};
u32 gLobbyActorScripts_0_2[12] = {
#include "lobby/0081BD10.inc"
};
u32 gLobbyActorScripts_0_3[16] = {
#include "lobby/0081BD40.inc"
};
u32 gLobbyActorScripts_0_4[16] = {
#include "lobby/0081BD80.inc"
};
u32 gLobbyActorScripts_0_5[12] = {
#include "lobby/0081BDC0.inc"
};
u32 gLobbyActorScripts_0_6[12] = {
#include "lobby/0081BDF0.inc"
};
u32 gLobbyActorScripts_0_7[20] = {
#include "lobby/0081BE20.inc"
};
ActorKey *gLobbyActorScripts_0[12] = {
    (ActorKey *)gLobbyActorScripts_0_0, (ActorKey *)gLobbyActorScripts_0_1, (ActorKey *)gLobbyActorScripts_0_2, (ActorKey *)gLobbyActorScripts_0_3,
    (ActorKey *)gLobbyActorScripts_0_4, (ActorKey *)gLobbyActorScripts_0_5, (ActorKey *)gLobbyActorScripts_0_6, (ActorKey *)gLobbyActorScripts_0_7,
    NULL, NULL, NULL, NULL,
};
u32 gLobbyActorScripts_1_0[16] = {
#include "lobby/0081BEA0.inc"
};
u32 gLobbyActorScripts_1_1[8] = {
#include "lobby/0081BEE0.inc"
};
u32 gLobbyActorScripts_1_2[20] = {
#include "lobby/0081BF00.inc"
};
u32 gLobbyActorScripts_1_3[16] = {
#include "lobby/0081BF50.inc"
};
u32 gLobbyActorScripts_1_4[16] = {
#include "lobby/0081BF90.inc"
};
u32 gLobbyActorScripts_1_5[8] = {
#include "lobby/0081BFD0.inc"
};
u32 gLobbyActorScripts_1_6[12] = {
#include "lobby/0081BFF0.inc"
};
u32 gLobbyActorScripts_1_7[12] = {
#include "lobby/0081C020.inc"
};
u32 gLobbyActorScripts_1_8[12] = {
#include "lobby/0081C050.inc"
};
ActorKey *gLobbyActorScripts_1[12] = {
    (ActorKey *)gLobbyActorScripts_1_0, (ActorKey *)gLobbyActorScripts_1_1, (ActorKey *)gLobbyActorScripts_1_2, (ActorKey *)gLobbyActorScripts_1_3,
    (ActorKey *)gLobbyActorScripts_1_4, (ActorKey *)gLobbyActorScripts_1_5, (ActorKey *)gLobbyActorScripts_1_6, (ActorKey *)gLobbyActorScripts_1_7,
    (ActorKey *)gLobbyActorScripts_1_8, NULL, NULL, NULL,
};
u32 gLobbyActorScripts_2_0[12] = {
#include "lobby/0081C0B0.inc"
};
u32 gLobbyActorScripts_2_1[8] = {
#include "lobby/0081C0E0.inc"
};
u32 gLobbyActorScripts_2_2[16] = {
#include "lobby/0081C100.inc"
};
u32 gLobbyActorScripts_2_3[12] = {
#include "lobby/0081C140.inc"
};
u32 gLobbyActorScripts_2_4[12] = {
#include "lobby/0081C170.inc"
};
u32 gLobbyActorScripts_2_5[12] = {
#include "lobby/0081C1A0.inc"
};
u32 gLobbyActorScripts_2_6[12] = {
#include "lobby/0081C1D0.inc"
};
u32 gLobbyActorScripts_2_7[20] = {
#include "lobby/0081C200.inc"
};
ActorKey *gLobbyActorScripts_2[12] = {
    (ActorKey *)gLobbyActorScripts_2_0, (ActorKey *)gLobbyActorScripts_2_1, (ActorKey *)gLobbyActorScripts_2_2, (ActorKey *)gLobbyActorScripts_2_3,
    (ActorKey *)gLobbyActorScripts_2_4, (ActorKey *)gLobbyActorScripts_2_5, (ActorKey *)gLobbyActorScripts_2_6, (ActorKey *)gLobbyActorScripts_2_7,
    NULL, NULL, NULL, NULL,
};
u32 gLobbyActorScripts_3_0[12] = {
#include "lobby/0081C280.inc"
};
u32 gLobbyActorScripts_3_1[12] = {
#include "lobby/0081C2B0.inc"
};
u32 gLobbyActorScripts_3_2[12] = {
#include "lobby/0081C2E0.inc"
};
u32 gLobbyActorScripts_3_3[12] = {
#include "lobby/0081C310.inc"
};
u32 gLobbyActorScripts_3_4[16] = {
#include "lobby/0081C340.inc"
};
u32 gLobbyActorScripts_3_5[12] = {
#include "lobby/0081C380.inc"
};
u32 gLobbyActorScripts_3_6[12] = {
#include "lobby/0081C3B0.inc"
};
u32 gLobbyActorScripts_3_7[8] = {
#include "lobby/0081C3E0.inc"
};
u32 gLobbyActorScripts_3_8[16] = {
#include "lobby/0081C400.inc"
};
u32 gLobbyActorScripts_3_9[12] = {
#include "lobby/0081C440.inc"
};
u32 gLobbyActorScripts_3_10[12] = {
#include "lobby/0081C470.inc"
};
u32 gLobbyActorScripts_3_11[12] = {
#include "lobby/0081C4A0.inc"
};
u32 gLobbyActorScripts_3_12[16] = {
#include "lobby/0081C4D0.inc"
};
u32 gLobbyActorScripts_3_13[12] = {
#include "lobby/0081C510.inc"
};
ActorKey *gLobbyActorScripts_3[16] = {
    (ActorKey *)gLobbyActorScripts_3_0, (ActorKey *)gLobbyActorScripts_3_1, (ActorKey *)gLobbyActorScripts_3_2, (ActorKey *)gLobbyActorScripts_3_3,
    (ActorKey *)gLobbyActorScripts_3_4, (ActorKey *)gLobbyActorScripts_3_5, (ActorKey *)gLobbyActorScripts_3_6, (ActorKey *)gLobbyActorScripts_3_7,
    (ActorKey *)gLobbyActorScripts_3_8, (ActorKey *)gLobbyActorScripts_3_9, (ActorKey *)gLobbyActorScripts_3_10, (ActorKey *)gLobbyActorScripts_3_11,
    (ActorKey *)gLobbyActorScripts_3_12, (ActorKey *)gLobbyActorScripts_3_13, NULL, NULL,
};
ActorKey* *gLobbyActorScripts[32] = {
    gLobbyActorScripts_0, gLobbyActorScripts_1, gLobbyActorScripts_2, gLobbyActorScripts_3,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u32 D_0081C600[68] = {
#include "lobby/0081C600.inc"
};
u32 D_0081C710[52] = {
#include "lobby/0081C710.inc"
};
u32 D_0081C7E0[96] = {
#include "lobby/0081C7E0.inc"
};
u32 D_0081C960[68] = {
#include "lobby/0081C960.inc"
};
u32 D_0081CA70[116] = {
#include "lobby/0081CA70.inc"
};
u32 D_0081CC40[52] = {
#include "lobby/0081CC40.inc"
};
u32 D_0081CD10[60] = {
#include "lobby/0081CD10.inc"
};
u32 D_0081CE00[92] = {
#include "lobby/0081CE00.inc"
};
u32 D_0081CF70[136] = {
#include "lobby/0081CF70.inc"
};
u32 gRecordCats_0_1_0[32] = {
#include "lobby/0081D190.inc"
};
u32 gRecordCats_0_1_1[60] = {
#include "lobby/0081D210.inc"
};
u32 gRecordCats_0_1_2[52] = {
#include "lobby/0081D300.inc"
};
u32 gRecordCats_0_4_0[48] = {
#include "lobby/0081D3D0.inc"
};
u32 gRecordCats_0_4_1[44] = {
#include "lobby/0081D490.inc"
};
u32 gRecordCats_0_7_0[52] = {
#include "lobby/0081D540.inc"
};
u32 gRecordCats_0_7_1[56] = {
#include "lobby/0081D610.inc"
};
u32 gRecordCats_0_10_0[60] = {
#include "lobby/0081D6F0.inc"
};
u32 gRecordCats_0_10_1[48] = {
#include "lobby/0081D7E0.inc"
};
u32 gRecordCats_0_13_0[56] = {
#include "lobby/0081D8A0.inc"
};
u32 gRecordCats_0_13_1[60] = {
#include "lobby/0081D980.inc"
};
u32 gRecordCats_0_13_2[44] = {
#include "lobby/0081DA70.inc"
};
u32 gRecordCats_0_16_0[48] = {
#include "lobby/0081DB20.inc"
};
u32 gRecordCats_0_16_1[48] = {
#include "lobby/0081DBE0.inc"
};
u32 gRecordCats_0_19_0[52] = {
#include "lobby/0081DCA0.inc"
};
u32 gRecordCats_0_19_1[40] = {
#include "lobby/0081DD70.inc"
};
u32 gRecordCats_0_22_0[56] = {
#include "lobby/0081DE10.inc"
};
u32 gRecordCats_0_22_1[52] = {
#include "lobby/0081DEF0.inc"
};
u32 gRecordCats_0_22_2[52] = {
#include "lobby/0081DFC0.inc"
};
u32 gRecordCats_0_22_3[56] = {
#include "lobby/0081E090.inc"
};
u32 gRecordCats_0_25_0[60] = {
#include "lobby/0081E170.inc"
};
u32 gRecordCats_0_25_1[44] = {
#include "lobby/0081E260.inc"
};
u32 gRecordCats_0_28_0[36] = {
#include "lobby/0081E310.inc"
};
u32 gRecordCats_0_28_1[48] = {
#include "lobby/0081E3A0.inc"
};
u32 gRecordCats_0_28_2[40] = {
#include "lobby/0081E460.inc"
};
u32 gRecordCats_0_28_3[52] = {
#include "lobby/0081E500.inc"
};
u32 gRecordCats_0_31_0[52] = {
#include "lobby/0081E5D0.inc"
};
u32 gRecordCats_0_31_1[48] = {
#include "lobby/0081E6A0.inc"
};
u32 gRecordCats_0_34_0[64] = {
#include "lobby/0081E760.inc"
};
u32 gRecordCats_0_34_1[52] = {
#include "lobby/0081E860.inc"
};
u32 gRecordCats_0_34_2[48] = {
#include "lobby/0081E930.inc"
};
u32 gRecordCats_0_37_0[60] = {
#include "lobby/0081E9F0.inc"
};
u32 gRecordCats_0_37_1[64] = {
#include "lobby/0081EAE0.inc"
};
u32 gRecordCats_0_37_2[48] = {
#include "lobby/0081EBE0.inc"
};
u32 gRecordCats_0_40_0[56] = {
#include "lobby/0081ECA0.inc"
};
u32 gRecordCats_0_40_1[40] = {
#include "lobby/0081ED80.inc"
};
u32 gRecordCats_0_43_0[56] = {
#include "lobby/0081EE20.inc"
};
u32 gRecordCats_0_43_1[60] = {
#include "lobby/0081EF00.inc"
};
u32 gRecordCats_1_1_0[40] = {
#include "lobby/0081EFF0.inc"
};
u32 gRecordCats_1_1_1[32] = {
#include "lobby/0081F090.inc"
};
u32 gRecordCats_1_4_0[44] = {
#include "lobby/0081F110.inc"
};
u32 gRecordCats_1_4_1[56] = {
#include "lobby/0081F1C0.inc"
};
u32 gRecordCats_1_4_2[44] = {
#include "lobby/0081F2A0.inc"
};
u32 gRecordCats_1_4_3[40] = {
#include "lobby/0081F350.inc"
};
u32 gRecordCats_1_7_0[48] = {
#include "lobby/0081F3F0.inc"
};
u32 gRecordCats_1_7_1[40] = {
#include "lobby/0081F4B0.inc"
};
u32 gRecordCats_1_10_0[56] = {
#include "lobby/0081F550.inc"
};
u32 gRecordCats_1_10_1[48] = {
#include "lobby/0081F630.inc"
};
u32 gRecordCats_1_13_0[60] = {
#include "lobby/0081F6F0.inc"
};
u32 gRecordCats_1_13_1[48] = {
#include "lobby/0081F7E0.inc"
};
u32 gRecordCats_1_13_2[44] = {
#include "lobby/0081F8A0.inc"
};
u32 gRecordCats_1_16_0[68] = {
#include "lobby/0081F950.inc"
};
u32 gRecordCats_1_16_1[64] = {
#include "lobby/0081FA60.inc"
};
u32 gRecordCats_1_19_0[64] = {
#include "lobby/0081FB60.inc"
};
u32 gRecordCats_1_19_1[56] = {
#include "lobby/0081FC60.inc"
};
u32 gRecordCats_1_22_0[60] = {
#include "lobby/0081FD40.inc"
};
u32 gRecordCats_1_22_1[40] = {
#include "lobby/0081FE30.inc"
};
u32 gRecordCats_1_25_0[60] = {
#include "lobby/0081FED0.inc"
};
u32 gRecordCats_1_28_0[16] = {
#include "lobby/0081FFC0.inc"
};
u32 gLobbyFadeTables_0_0_6_0[40] = {
#include "lobby/00820000.inc"
};
u32 gRecordCats_1_28_1[56] = {
#include "lobby/008200A0.inc"
};
u32 gRecordCats_1_28_2[52] = {
#include "lobby/00820180.inc"
};
u32 gRecordCats_1_28_3[56] = {
#include "lobby/00820250.inc"
};
u32 gRecordCats_2_1_0[64] = {
#include "lobby/00820330.inc"
};
u32 gRecordCats_2_4_0[60] = {
#include "lobby/00820430.inc"
};
u32 gRecordCats_2_4_1[40] = {
#include "lobby/00820520.inc"
};
u32 gRecordCats_2_7_0[48] = {
#include "lobby/008205C0.inc"
};
u32 gRecordCats_2_7_1[56] = {
#include "lobby/00820680.inc"
};
u32 gRecordCats_2_7_2[68] = {
#include "lobby/00820760.inc"
};
u32 gRecordCats_2_10_0[56] = {
#include "lobby/00820870.inc"
};
u32 gRecordCats_2_10_1[44] = {
#include "lobby/00820950.inc"
};
u32 gRecordCats_2_10_2[52] = {
#include "lobby/00820A00.inc"
};
u32 gRecordCats_2_10_3[60] = {
#include "lobby/00820AD0.inc"
};
u32 gRecordCats_2_10_4[64] = {
#include "lobby/00820BC0.inc"
};
u32 gRecordCats_2_13_0[60] = {
#include "lobby/00820CC0.inc"
};
u32 gRecordCats_2_13_1[48] = {
#include "lobby/00820DB0.inc"
};
u32 gRecordCats_2_13_2[60] = {
#include "lobby/00820E70.inc"
};
u32 gRecordCats_2_16_0[56] = {
#include "lobby/00820F60.inc"
};
u32 gRecordCats_2_16_1[64] = {
#include "lobby/00821040.inc"
};
u32 gRecordCats_2_19_0[52] = {
#include "lobby/00821140.inc"
};
u32 gRecordCats_2_19_1[52] = {
#include "lobby/00821210.inc"
};
u32 gRecordCats_2_19_2[52] = {
#include "lobby/008212E0.inc"
};
u32 gRecordCats_2_22_0[60] = {
#include "lobby/008213B0.inc"
};
u32 gRecordCats_2_22_1[44] = {
#include "lobby/008214A0.inc"
};
u32 gRecordCats_2_25_0[48] = {
#include "lobby/00821550.inc"
};
u32 gRecordCats_2_25_1[56] = {
#include "lobby/00821610.inc"
};
u32 gRecordCats_2_28_0[44] = {
#include "lobby/008216F0.inc"
};
u32 gRecordCats_2_28_1[40] = {
#include "lobby/008217A0.inc"
};
u32 gRecordCats_3_1_0[48] = {
#include "lobby/00821840.inc"
};
u32 gRecordCats_3_1_1[44] = {
#include "lobby/00821900.inc"
};
u32 gRecordCats_3_1_2[48] = {
#include "lobby/008219B0.inc"
};
u32 gRecordCats_3_1_3[44] = {
#include "lobby/00821A70.inc"
};
u32 gRecordCats_3_4_0[44] = {
#include "lobby/00821B20.inc"
};
u32 gRecordCats_3_4_1[52] = {
#include "lobby/00821BD0.inc"
};
u32 gRecordCats_3_4_2[48] = {
#include "lobby/00821CA0.inc"
};
u32 gRecordCats_3_7_0[44] = {
#include "lobby/00821D60.inc"
};
u32 gRecordCats_3_7_1[48] = {
#include "lobby/00821E10.inc"
};
u32 gRecordCats_3_7_2[44] = {
#include "lobby/00821ED0.inc"
};
u32 gRecordCats_3_7_3[40] = {
#include "lobby/00821F80.inc"
};
u32 gRecordCats_3_10_0[40] = {
#include "lobby/00822020.inc"
};
u32 gRecordCats_3_10_1[36] = {
#include "lobby/008220C0.inc"
};
u32 gRecordCats_3_10_2[56] = {
#include "lobby/00822150.inc"
};
u32 gRecordCats_3_13_0[48] = {
#include "lobby/00822230.inc"
};
u32 gRecordCats_3_13_1[60] = {
#include "lobby/008222F0.inc"
};
u32 gRecordCats_3_13_2[56] = {
#include "lobby/008223E0.inc"
};
u32 gRecordCats_3_16_0[44] = {
#include "lobby/008224C0.inc"
};
u32 gRecordCats_3_16_1[52] = {
#include "lobby/00822570.inc"
};
u32 gRecordCats_3_16_2[52] = {
#include "lobby/00822640.inc"
};
u32 gRecordCats_3_16_3[48] = {
#include "lobby/00822710.inc"
};
u32 gRecordCats_3_16_4[56] = {
#include "lobby/008227D0.inc"
};
u32 gRecordCats_3_19_0[44] = {
#include "lobby/008228B0.inc"
};
u32 gRecordCats_3_19_1[48] = {
#include "lobby/00822960.inc"
};
u32 gRecordCats_3_22_0[52] = {
#include "lobby/00822A20.inc"
};
u32 gRecordCats_3_22_1[40] = {
#include "lobby/00822AF0.inc"
};
u32 gRecordCats_3_22_2[44] = {
#include "lobby/00822B90.inc"
};
u32 gRecordCats_3_22_3[44] = {
#include "lobby/00822C40.inc"
};
u32 gRecordCats_3_25_0[40] = {
#include "lobby/00822CF0.inc"
};
u32 gRecordCats_3_25_1[48] = {
#include "lobby/00822D90.inc"
};
u32 gRecordCats_3_25_2[48] = {
#include "lobby/00822E50.inc"
};
u32 gRecordCats_3_25_3[48] = {
#include "lobby/00822F10.inc"
};
u32 gRecordCats_3_28_0[36] = {
#include "lobby/00822FD0.inc"
};
u32 gRecordCats_3_28_1[60] = {
#include "lobby/00823060.inc"
};
u32 gRecordCats_3_28_2[52] = {
#include "lobby/00823150.inc"
};
u32 gRecordCats_3_31_0[40] = {
#include "lobby/00823220.inc"
};
u32 gRecordCats_3_31_1[44] = {
#include "lobby/008232C0.inc"
};
u32 gRecordCats_3_31_2[44] = {
#include "lobby/00823370.inc"
};
u32 gRecordCats_3_34_0[44] = {
#include "lobby/00823420.inc"
};
u32 gRecordCats_3_34_1[52] = {
#include "lobby/008234D0.inc"
};
u32 gRecordCats_3_34_2[56] = {
#include "lobby/008235A0.inc"
};
u32 gRecordCats_3_37_0[52] = {
#include "lobby/00823680.inc"
};
u32 gRecordCats_3_37_1[48] = {
#include "lobby/00823750.inc"
};
u32 gRecordCats_3_37_2[52] = {
#include "lobby/00823810.inc"
};
u32 gRecordCats_3_40_0[44] = {
#include "lobby/008238E0.inc"
};
u32 gRecordCats_3_40_1[52] = {
#include "lobby/00823990.inc"
};
u32 gRecordCats_3_40_2[40] = {
#include "lobby/00823A60.inc"
};
u32 gRecordCats_3_40_3[40] = {
#include "lobby/00823B00.inc"
};
u32 gRecordCats_3_43_0[48] = {
#include "lobby/00823BA0.inc"
};
u32 gRecordCats_3_43_1[48] = {
#include "lobby/00823C60.inc"
};
u32 gRecordCats_3_43_2[52] = {
#include "lobby/00823D20.inc"
};
u32 gRecordCats_3_43_3[52] = {
#include "lobby/00823DF0.inc"
};
u32 gRecordCats_3_46_0[40] = {
#include "lobby/00823EC0.inc"
};
u32 gRecordCats_3_46_1[56] = {
#include "lobby/00823F60.inc"
};
u32 gRecordCats_3_46_2[40] = {
#include "lobby/00824040.inc"
};
u32 gRecordCats_3_46_3[52] = {
#include "lobby/008240E0.inc"
};
u32 gRecordCats_3_46_4[60] = {
#include "lobby/008241B0.inc"
};
u32 gRecordCats_3_49_0[72] = {
#include "lobby/008242A0.inc"
};
u32 gRecordCats_3_49_1[36] = {
#include "lobby/008243C0.inc"
};
u32 gRecordCats_3_49_2[52] = {
#include "lobby/00824450.inc"
};
u32 gRecordCats_3_49_3[56] = {
#include "lobby/00824520.inc"
};
u32 gRecordCats_3_52_0[44] = {
#include "lobby/00824600.inc"
};
u32 gRecordCats_3_52_1[40] = {
#include "lobby/008246B0.inc"
};
u32 gRecordCats_3_52_2[56] = {
#include "lobby/00824750.inc"
};
u32 gRecordCats_3_52_3[40] = {
#include "lobby/00824830.inc"
};
u32 gRecordCats_3_55_0[72] = {
#include "lobby/008248D0.inc"
};
u32 gRecordCats_3_55_1[32] = {
#include "lobby/008249F0.inc"
};
u32 gRecordCats_3_55_2[56] = {
#include "lobby/00824A70.inc"
};
u32 gRecordCats_3_55_3[56] = {
#include "lobby/00824B50.inc"
};
u32 gRecordCats_3_58_0[44] = {
#include "lobby/00824C30.inc"
};
u32 gRecordCats_3_58_1[48] = {
#include "lobby/00824CE0.inc"
};
u32 gRecordCats_3_58_2[48] = {
#include "lobby/00824DA0.inc"
};
u32 gRecordCats_3_58_3[40] = {
#include "lobby/00824E60.inc"
};
u32 gRecordCats_3_61_0[52] = {
#include "lobby/00824F00.inc"
};
u32 gRecordCats_3_61_1[56] = {
#include "lobby/00824FD0.inc"
};
u32 gRecordCats_3_61_2[32] = {
#include "lobby/008250B0.inc"
};
u32 gRecordCats_3_64_0[52] = {
#include "lobby/00825130.inc"
};
u32 gRecordCats_3_64_1[40] = {
#include "lobby/00825200.inc"
};
u32 gRecordCats_3_67_0[48] = {
#include "lobby/008252A0.inc"
};
u32 gRecordCats_3_67_1[48] = {
#include "lobby/00825360.inc"
};
u32 gRecordCats_0_0[8] = {
#include "lobby/00825420.inc"
};
u32 gRecordCats_0_3[8] = {
#include "lobby/00825440.inc"
};
u32 gRecordCats_0_6[8] = {
#include "lobby/00825460.inc"
};
u32 gRecordCats_0_9[8] = {
#include "lobby/00825480.inc"
};
u32 gRecordCats_0_12[8] = {
#include "lobby/008254A0.inc"
};
u32 gRecordCats_0_15[8] = {
#include "lobby/008254C0.inc"
};
u32 gRecordCats_0_18[8] = {
#include "lobby/008254E0.inc"
};
u32 gRecordCats_0_21[8] = {
#include "lobby/00825500.inc"
};
u32 gRecordCats_0_24[8] = {
#include "lobby/00825520.inc"
};
u32 gRecordCats_0_27[8] = {
#include "lobby/00825540.inc"
};
u32 gRecordCats_0_30[8] = {
#include "lobby/00825560.inc"
};
u32 gRecordCats_0_33[8] = {
#include "lobby/00825580.inc"
};
u32 gRecordCats_0_36[8] = {
#include "lobby/008255A0.inc"
};
u32 gRecordCats_0_39[8] = {
#include "lobby/008255C0.inc"
};
u32 gRecordCats_0_42[8] = {
#include "lobby/008255E0.inc"
};
u32 gRecordCats_1_0[8] = {
#include "lobby/00825600.inc"
};
u32 gRecordCats_1_3[8] = {
#include "lobby/00825620.inc"
};
u32 gRecordCats_1_6[8] = {
#include "lobby/00825640.inc"
};
u32 gRecordCats_1_9[8] = {
#include "lobby/00825660.inc"
};
u32 gRecordCats_1_12[8] = {
#include "lobby/00825680.inc"
};
u32 gRecordCats_1_15[8] = {
#include "lobby/008256A0.inc"
};
u32 gRecordCats_1_18[8] = {
#include "lobby/008256C0.inc"
};
u32 gRecordCats_1_21[8] = {
#include "lobby/008256E0.inc"
};
u32 gRecordCats_1_24[8] = {
#include "lobby/00825700.inc"
};
u32 gRecordCats_1_27[8] = {
#include "lobby/00825720.inc"
};
u32 gRecordCats_2_0[8] = {
#include "lobby/00825740.inc"
};
u32 gRecordCats_2_3[8] = {
#include "lobby/00825760.inc"
};
u32 gRecordCats_2_6[8] = {
#include "lobby/00825780.inc"
};
u32 gRecordCats_2_9[8] = {
#include "lobby/008257A0.inc"
};
u32 gRecordCats_2_12[8] = {
#include "lobby/008257C0.inc"
};
u32 gRecordCats_2_15[8] = {
#include "lobby/008257E0.inc"
};
u32 gRecordCats_2_18[8] = {
#include "lobby/00825800.inc"
};
u32 gRecordCats_2_21[8] = {
#include "lobby/00825820.inc"
};
u32 gRecordCats_2_24[8] = {
#include "lobby/00825840.inc"
};
u32 gRecordCats_2_27[8] = {
#include "lobby/00825860.inc"
};
u32 gRecordCats_3_0[8] = {
#include "lobby/00825880.inc"
};
u32 gRecordCats_3_3[8] = {
#include "lobby/008258A0.inc"
};
u32 gRecordCats_3_6[8] = {
#include "lobby/008258C0.inc"
};
u32 gRecordCats_3_12[8] = {
#include "lobby/008258E0.inc"
};
u32 gRecordCats_3_18[8] = {
#include "lobby/00825900.inc"
};
u32 gRecordCats_3_24[8] = {
#include "lobby/00825920.inc"
};
u32 gRecordCats_3_30[8] = {
#include "lobby/00825940.inc"
};
u32 gRecordCats_3_36[8] = {
#include "lobby/00825960.inc"
};
u32 gRecordCats_3_42[8] = {
#include "lobby/00825980.inc"
};
u32 gRecordCats_3_48[8] = {
#include "lobby/008259A0.inc"
};
u32 gRecordCats_3_54[8] = {
#include "lobby/008259C0.inc"
};
u32 gRecordCats_3_60[8] = {
#include "lobby/008259E0.inc"
};
u32 gRecordCats_3_66[8] = {
#include "lobby/00825A00.inc"
};
u32 gRecordCats_0_a[16] = {
#include "lobby/00825A20.inc"
};
u32 gRecordCats_1_a[16] = {
#include "lobby/00825A60.inc"
};
u32 gRecordCats_2_a[16] = {
#include "lobby/00825AA0.inc"
};
u32 gRecordCats_3_a[12] = {
#include "lobby/00825AE0.inc"
};
