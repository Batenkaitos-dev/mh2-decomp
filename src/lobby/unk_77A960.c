#include "common.h"
#include "lobby_npc.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "scene.h"
#include "player.h"
#include "item.h"
#include "em.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_globals.h"
s32 Game_IsOnline(void);
/* unit's own declaration: the definition (sub_main unk_3B4C80.c) returns u8; with it Npc_IsPresent and Npc_SetRouteByKind differ (477 of 511, 67 of 335 words) */
s32 Village_FacilityLevel(s32);
s32 Npc_IsType4E(Em *);

/* prototypes */
extern NpcRouteEnt gNpcRoute1D[];
extern NpcRouteEnt gNpcRoute1E[];
extern NpcRouteEnt gNpcRoute1F[];
extern NpcRouteEnt gNpcRoute21[];
extern NpcRouteEnt gNpcRoute23[];
extern NpcRouteEnt gNpcRoute24[];
extern NpcRouteEnt gNpcRoute25[];
extern NpcRouteEnt gNpcRoute3B[];
extern NpcRouteEnt gNpcRoute3C[];
extern NpcRouteEnt gNpcRoute3D[];
extern NpcRouteEnt gNpcRoute40[];
extern NpcRouteEnt gNpcRoute4E[];
/* end of prototypes */

void ActSpot_Take(u8);
void *memcpy(void *dst, const void *src, u32 n);
u8 Lobby_FindMember(u8 *arg0);
u8 Area_GetSetIndex(s32 id); /* unit's own declaration: with the definition's u8 (u16 id) (lobby/unk_79EA30.c), NpcTable_Find 1 of 44 words, AreaNpc_IsPresent 1 of 27 words, Npc_SetupFromArea 1 of 219 words differ */
s32 Npc_IsPresent(u8 kind, s32 id, u8 force);

/* An entry of the per-area tables gAreaNpcTables[area] (0x48 bytes; gAreaNpcCounts[area] of them). */
typedef struct V0077C490_0 {
    /* 0x00 */ u8 f0;               /* the kind */
    /* 0x01 */ u8 f1;               /* the colour set (gNpcColorSets) */
    /* 0x02 */ u8 pad2[2];
    /* 0x04 */ s32 f4;
    /* 0x08 */ f32 scale;
    /* 0x0C */ f32 f0C;
    /* 0x10 */ void *route;
    /* 0x14 */ u8 f14;
    /* 0x15 */ u8 f15;
    /* 0x16 */ char name[0x32];
} V0077C490_0;

extern V0077C490_0 *gAreaNpcTables[];
extern u8 gAreaNpcCounts[];
s32 Area_IsOnlineHouse(s32 arg0);
void TownPc_TakeSpot(void);
void TownMsg_Send17(void *pl);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
s32 LobbyTime_IsAm(void);
void Actor_SetState(void *, s32, u16); /* unit's own declaration: state u16 (the definition has s32); with the definition's type Npc_Init by 1 of 375 words differ */
s32 Area_Is86to88(s32 arg0);
void TownMsg_Send07(Player *pl);

void TownMsg_Send16(char *name, u8 a, u8 b);
extern u8 gHouseAccess;

/* A town message claiming slot msg[0] for the member id: taken when the
 * slot is free (bit of LobbyState +0x1562; its id goes to +0x1564) and,
 * unless gHouseAccess, the member is the local player; TownMsg_Send16 gets
 * 1 or 0. */
void TownMsg_On15(char *id, u8 *msg)
{
    u8 slot = msg[0];
    s32 bit = 1 << slot;

    if ((gLobby->readyBits & bit) || gTownMode == 0x48
        || (gHouseAccess == 0 && gScene.localPlayer != (u8)Lobby_FindMember((u8 *)id))) {
        TownMsg_Send16(id, 0, slot);
    } else {
        gLobby->readyBits = gLobby->readyBits | bit;
        memcpy(gLobby->slotIds + (u8)slot * 8, id, 8);
        TownMsg_Send16(id, 1, slot);
    }
}

void TownMsg_On05(s8 unused, u8 *data)
{
    if (data[0] == 0) {
        if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2) && gTownMode == 0x19) {
            gTownMode = 0;
            gTownMenuOpen = 0;
        }
        return;
    }
    ActSpot_Take(data[1]);
    if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2)) {
        if (Area_Is86to88(gScene.area) == 1 && gTownMode == 0x19) {
            TownPc_TakeSpot();
            return;
        }
        TownMsg_Send07(&gPlayers[gScene.localPlayer]);
        return;
    }
}

void TownMsg_On16(s8 unused, u8 *data)
{
    if (data[0] == 0) {
        if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2) && gTownMode == 0x19) {
            gTownMode = 0;
            gTownMenuOpen = 0;
        }
        return;
    }
    if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2)) {
        if (Area_IsOnlineHouse(gScene.area) == 1 && gTownMode == 0x19) {
            TownPc_TakeSpot();
            return;
        }
        TownMsg_Send17(&gPlayers[gScene.localPlayer]);
        return;
    }
}

/* Inline bit helpers: written as plain expressions, the `or`/`and` below get
 * their operands the other way round (found by decomp-permuter). */
static inline s32 Bit(s32 n)
{
    return 1 << n;
}

static inline s32 BitMask(s32 n)
{
    return ~(1 << n);
}

/* msg: [0] a bit of the ready flags at +0x1562 of the online state, then the
 * sender's name (8 bytes). */
void Lobby_ClearReadyBit(s8 *msg)
{
    u8 *p = (u8 *)gLobby;

    if ((p + 0x1538)[(u8)Lobby_FindMember((u8 *)msg + 1)] == (s8)(gScene.localPlayer + 0x30)) {
        gLobby->readyBits = BitMask(msg[0]) & gLobby->readyBits;
    }
}

void TownMsg_On0F(u8 *a0)
{
    ActSpot_Take(*(u8 *)a0);
}

void Lobby_SetReadyBit(u8 *msg)
{
    u8 *p = (u8 *)gLobby;

    if ((p + 0x1538)[(u8)Lobby_FindMember(msg + 1)] == (s8)(gScene.localPlayer + 0x30)) {
        gLobby->readyBits = Bit(msg[0]) | gLobby->readyBits;
    }
}

s32 Area_HasTownPcs(u16 area)
{
    s32 temp_v1;

    temp_v1 = area & 0xFFFF;
    switch (temp_v1) {
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x84:
        return 1;
    case 0xAD:
    case 0xAE:
    case 0xAF:
        return 1;
    default:
        return 0;
    }
}

s32 TownPc_IsInContest(Player *p);
void TownMsg_SendBgmTo(s32 unused, void *to);
void TownMsg_SendActor(Player *a, void *to);
void TownMsg_SendAreaTo(void *to, u16 id);
void TownMsg_Send1E(void);
void TownMsg_Send0A(Player *p, void *x);
s32 LobbyBgm_GetTrack(void);
extern s8 gLobbyScreenMode;

void TownMsg_On0D(u8 *from)
{
    s32 track;
    u8 localPlayer;

    track = LobbyBgm_GetTrack();
    if (gLobbyScreenMode == 2) {
        TownMsg_SendAreaTo(from, (s32) gScene.area);
        if (Area_HasTownPcs(gScene.area) == 1) {
            TownMsg_SendActor(&gPlayers[gScene.localPlayer], from);
            localPlayer = gScene.localPlayer;
            if ((gPlayers[localPlayer].offerStarted != 0) && (gPlayers[localPlayer].state == 0xA) && (gPlayers[localPlayer].stateGroup == 0)) {
                TownMsg_Send0A(&gPlayers[localPlayer], from);
            }
        }
        if (TownPc_IsInContest(&gPlayers[gScene.localPlayer]) == 1) {
            TownMsg_Send1E();
        }
        if (track != -1) {
            TownMsg_SendBgmTo(track, from);
        }
    }
}

void Lobby_SetMemberSlot(s8 idx, u8 *msg)
{
    u8 buf[9];

    memcpy(buf, msg, 8);
    buf[8] = 0;
    gLobby->memberSlot[idx] = (s8)Lobby_FindMember(buf) + 0x30;
}

void TownMsg_On06(s8 idx)
{
    Player *p;

    p = &gPlayers[idx];
    p->unk844 = 0;
    p->unk867 = 0;
}


s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 a, f32 frame, f32 frame2);
s16 Effect_MapTexId(s16 id);
s32 Npc_IsFreeId0Aor2E(Em *arg0);
s32 Npc_IsFree(Em *arg0);
s32 Npc_IsType40(Em *arg0);
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8, u32);
void Draw_ApplyStates(u32);
s32 Model_Draw(u32, s32);
void Draw_SetDefaultState(void);
void Player_DrawHeldItem(void *obj);

/* Draws what a townsperson holds: props (model parts of the set at 0x5DA030,
 * or of the one at 0x5DA070 for kind 0x10), each at a bone with an offset and a
 * rotation that depend on the motion and its frame. */
void Npc_DrawProps(Em *npc)
{
    s16 bone[2];
    f32 pos[3];
    f32 r[16];
    f32 m[16];
    f32 ofs[2][3];
    f32 rot[2][3];
    ModelMat *mats;
    s32 cnt;
    Model *set;
    s16 n;
    NpcWork *w;
    s16 idx;
    s16 i;
    ModelMatSet *part;
    s32 j;
    f32 *mat;

    set = gCommonModels[0];
    w = (NpcWork *)&npc->work;
    if (set != NULL && set->active != 0) {
        mats = set->mats;
        Gs_SetState(0x67, -1);
        n = 0;
        if (Npc_IsFreeId0Aor2E((Em *)npc) == 1) {
            switch (npc->motionId[0]) {
            case 0x2AD:
                if (EmMotion_ReachedFrame(npc, 0, 80.0f) != 0) {
                    return;
                }
            case 0x2AB:
            case 0x2AC:
                idx = Effect_MapTexId(0x3E);
                if (idx == -1) {
                    return;
                }
                part = set->matSets + idx;
                bone[0] = 0x12;
                rot[0][0] = 2.51327419f;
                rot[0][1] = -0.488692224f;
                rot[0][2] = 0.0174532924f;
                ofs[0][0] = -9.39999962f;
                ofs[0][1] = -7.4000001f;
                ofs[0][2] = -3.0f;
                bone[1] = 0xE;
                rot[1][0] = 1.18682396f;
                rot[1][1] = 0.558505356f;
                rot[1][2] = -2.02458191f;
                ofs[1][0] = 11.0f;
                ofs[1][1] = -6.4000001f;
                ofs[1][2] = -1.60000002f;
                n += 2;
                break;
            }
        } else if (Npc_IsFree((Em *)npc) == 1) {
            switch (w->id) {
            case 0x10:
                set = gAreaSubModels[0];
                part = set->matSets + 8;
                switch (npc->motionId[0]) {
                case 0x35D:
                    if (EmMotion_InFrameRange(npc, 0, 172.0f, 274.0f) != 0) {
                        bone[0] = 0x12;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        n++;
                    } else if (EmMotion_ReachedFrame(npc, 0, 274.0f) != 0) {
                        n = 1;
                        rot[0][0] = 0.0f;
                        ofs[0][0] = -0.340000004f;
                        bone[0] = 1;
                        ofs[0][1] = 75.0f;
                        rot[0][1] = 0.0f;
                        ofs[0][2] = 92.0f;
                        rot[0][2] = 0.0f;
                    }
                    break;
                case 0x35E:
                    if (EmMotion_ReachedFrame(npc, 0, 54.0f) != 0) {
                        bone[0] = 0x12;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        n++;
                    } else {
                        n = 1;
                        rot[0][0] = 0.0f;
                        ofs[0][0] = -0.340000004f;
                        bone[0] = 1;
                        ofs[0][1] = 75.0f;
                        rot[0][1] = 0.0f;
                        ofs[0][2] = 92.0f;
                        rot[0][2] = 0.0f;
                    }
                    break;
                case 0x35F:
                    if (EmMotion_ReachedFrame(npc, 0, 206.0f) == 0) {
                        bone[0] = 0x12;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        n++;
                    }
                    break;
                case 0x360:
                    if (EmMotion_ReachedFrame(npc, 0, 486.0f) == 0) {
                        bone[0] = 0x12;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        n++;
                    }
                    break;
                }
                break;
            }
        }
        if (Npc_IsType40(npc) != 0) {
            part = set->matSets + 38;
            bone[n] = 0xE;
            rot[n][0] = 0.0f;
            rot[n][1] = 0.34906587f;
            rot[n][2] = 0.0f;
            ofs[n][0] = 2.4000001f;
            ofs[n][1] = -3.0f;
            ofs[n][2] = -10.0f;
            n++;
        }
        cnt = n;
        for (i = 0; i < cnt; i++) {
            if (part != NULL && part->id != -1) {
                Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(npc, bone[i])));
                Mtx_GetTrans(pos, m);
                Vec_RotateSelf(ofs[i], m);
                pos[0] += ofs[i][0];
                pos[1] += ofs[i][1];
                pos[2] += ofs[i][2];
                Mtx_Identity(r);
                Mtx_RotateXYZ(r, rot[i][0], rot[i][1], rot[i][2]);
                Mtx_Concat3x3(r, m);
                Mtx_SetTrans(r, pos[0], pos[1], pos[2]);
                Gs_SetState(0x1A, (s32)r);
                Gs_SetState(0x67, -1);
                for (j = 0; j < part->count; j++) {
                    mat = (f32 *)&mats[part->mat[j]];
                    mat[4] = npc->alpha;
                    Gs_SetState((u8)(j + 0x3A), (s32)mat);
                }
                Draw_ApplyStates(part->flags);
                Model_Draw(part->id, 0);
            }
        }
        Draw_SetDefaultState();
        Player_DrawHeldItem(npc);
    }
}

s32 Npc_IsBusyId0E(Em *arg0);
s32 Npc_IsAct51to55(Em *arg0);
s32 Motion_ReachedFrame(void *, s32, f32);


/* Hides (alpha 0) the parts of a townsperson's model that its kind, its
 * state or the area say it doesn't show. */
void Npc_HidePart(Em *npc, ModelMat *p, s32 part)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (Npc_IsBusyId0E(npc) == 1 || w->id == 0xF || w->id == 0x58) {
        switch (w->motionPhase) {
        case 0:
            if (part == 0xD) {
                p->alpha = 0.0f;
            }
            break;
        case 1:
            if (part == 6) {
                p->alpha = 0.0f;
            }
            break;
        default:
            if (w->id != 0x58 && (part == 0xD || part == 6)) {
                p->alpha = 0.0f;
            }
            break;
        }
    }
    if (w->id == 0x1A && gScene.area != 0x89 && part == 2) {
        p->alpha = 0.0f;
    }
    if (w->id == 6 &&
        ((u8)Village_FacilityLevel(3) == 2 || npc->state == 4 || npc->motionId[0] == 0x292 ||
         npc->motionId[0] == 1) &&
        part == 5) {
        p->alpha = 0.0f;
    }
    if ((npc->variant == 0x53 || npc->variant == 0x55) && part == 6) {
        p->alpha = 0.0f;
        if (npc->motionId[0] == 0x43C) {
            if (Motion_ReachedFrame(npc, 0, 120.0f)) {
                p->alpha = 1.0f;
            }
        } else if (npc->motionId[0] == 0x43D) {
            if (!Motion_ReachedFrame(npc, 0, 54.0f)) {
                p->alpha = 1.0f;
            }
        }
    }
    if (Npc_IsAct51to55(npc) == 1) {
        switch (w->unk42) {
        case 0:
            switch (part) {
            case 2:
            case 3:
            case 4:
                p->alpha = 0.0f;
                break;
            }
            break;
        case 1:
            switch (part) {
            case 1:
            case 3:
            case 4:
                p->alpha = 0.0f;
                break;
            }
            break;
        case 2:
            switch (part) {
            case 1:
            case 2:
            case 4:
                p->alpha = 0.0f;
                break;
            }
            break;
        }
    }
}

void LightSet_SetAmbient(void *obj, s32 n);
void LightSet_FadeAmbient(void *obj);
void Vec_AnglesToRad(s32 *in, f32 *out);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Gfx_SetState63(s32);
s32 Gs_SetState(u8, u32);
struct EmBone;
void Bone_StoreMatrices(struct EmBone *b);
void Mtx_Identity(f32 *m);
void Tex_Prepare(s32 n, s32 tex);
s32 Area_IsHouse(u16 area);
void Color_ArgbToFloat(f32 *rgba, u32 c);
void Draw_ApplyStates(u32);
s32 Model_Draw(u32, s32);
void Draw_SetDefaultState(void);
void Npc_DrawProps(Em *npc);

/* A colour for a part's materials (a list per kind, ended by part -1):
 * sub -1 colours all of the part's materials with color, then entries for
 * single materials follow. */
typedef struct NpcColor {
    /* 0x0 */ s16 part;
    /* 0x2 */ s16 sub;
    /* 0x4 */ u32 color;
} NpcColor;

extern NpcColor *gNpcColorSets[];

/* The draw callback of a townsperson (DrawSlot +0x14; the slot's +0x18 is
 * the townsperson): the model with each part's colours. */
void Npc_Draw(DrawSlot *ds)
{
    Em *o;
    f32 m[16];
    f32 id[16];
    f32 s[16];
    ModelMat *mats;
    ModelMatSet *parts;
    s32 n;
    s32 j;
    ModelMat *mat;
    NpcColor *c;
    Model *set;
    f32 rad[3];
    NpcWork *w;

    o = ds->owner; /* the townsperson (Npc_SetupDraw) */
    set = o->model;
    w = (NpcWork *)&o->work;
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
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    parts = set->matSets;
    mats = set->mats;
    Bone_StoreMatrices(set->bones);
    Mtx_Identity(id);
    Gs_SetState(0x19, (s32)id);
    Tex_Prepare(0xA, o->type * 20 + 0xA3);
    c = gNpcColorSets[w->activity];
    if (w->id == 0x14 && Area_IsHouse(gScene.area) == 1 && Game_IsOnline() == 0) {
        c->part = gSave.poogieOutfit;
    }
    while (c->part != -1) {
        Gs_SetState(0x67, -1);
        if (c->sub == -1) {
            n = 1;
        } else {
            n = 0;
        }
        for (j = 0; j < parts[c->part].count; j++) {
            mat = &mats[parts[c->part].mat[j]];
            if (j < 0 || j >= 0x20) {} /* compiled-out check */
            if (c->part == c[n].part && j == c[n].sub) {
                Color_ArgbToFloat(mat->rgb, c[n].color);
                n++;
            } else if (c->sub == -1) {
                Color_ArgbToFloat(mat->rgb, c->color);
            } else {
                Color_ArgbToFloat(mat->rgb, 0);
            }
            Npc_HidePart(o, mat, j);
            Gs_SetState((u8)(j + 0x3A), (s32)mat);
        }
        Draw_ApplyStates(parts[c->part].flags);
        Model_Draw(parts[c->part].id, 0);
        if (n == 0) {
            c++;
        } else {
            c += n;
        }
    }
    Draw_SetDefaultState();
    Npc_DrawProps(o);
    Gs_SetState(0x60, 0);
}

typedef void (*NpcFunc)(void *);
extern NpcFunc *gNpcHandlers[];
s32 Npc_IsType4B(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Npc_Draw(DrawSlot *ds);

/* A townsperson's start: its function table gNpcHandlers[kind] (4, 0 or 1 by
 * the checks, else the kind at +2), whose first entry is called, and its
 * draw slot (Npc_Draw). */
void Npc_SetupDraw(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    s8 kind = a->type;

    if (Npc_IsType4E(a) == 1) {
        kind = 4;
    } else if (Npc_IsType4B(a) == 1 || m->activity == 0x4C || Npc_IsType3Cto3E(a) == 1) {
        kind = 0;
    } else if (Npc_IsType4D(a) != 0) {
        kind = 1;
    }
    (a)->handlers = (EmVt *)gNpcHandlers[kind];
    (a)->handlers->init(a);
    a->drawSlotId = DrawSlot_Alloc();
    if (a->drawSlotId != -1) {
        a->drawSlot = DrawSlot_Get(a->drawSlotId);
        a->drawSlot->owner = a;
        a->drawSlot->draw = Npc_Draw;
    }
}

void EmSlot_SetupModel(Em *e);
s32 Npc_IsType4E(Em *);
s32 Npc_IsFree(Em *arg0);
s32 Npc_IsFreeAct15to19(Em *arg0);
s32 Npc_IsType40(Em *);
s32 Npc_IsBusyNotAct3C4B(Em *arg0);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
void LobbyBgm_SetActor(Em *actor);
void Actor_SetMotionOnLayer(void *, s32, s32, s32, s32);
s32 Player_UpdateMotion(Player *p);
void Npc_UpdateMotion(Em *);
void Npc_FollowFloor(Em *);
void Npc_UpdateAreaPos(Em *);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
extern s8 gNpc14ByPlayer;
extern s8 gPoogiePetStreak;
extern u8 gNpc14Follows;

/* Sets an NPC up: its model, then the motion for its route state. One s32
 * local holds both the route table's address (at each read of the route
 * entry's state) and the motion id, as the game's registers show. */
void Npc_Init(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;
    s32 v;
    s32 k;

    EmSlot_SetupModel(npc);
    npc->taskPhase++;
    npc->isEm = 0;
    npc->unk01E = 1;
    npc->unk414 = 0;
    npc->visible = 1;
    npc->motion[0].step = 2.0f;
    npc->motion[1].step = 2.0f;
    npc->motion[2].step = 2.0f;
    npc->motion[3].step = 2.0f;
    npc->motionTime = 0;
    npc->area = gScene.area;
    if (Npc_IsType4E(npc) == 1) {
        npc->motionLayerCount = 2;
        npc->isEm = 1;
    } else if (Npc_IsFree((Em *)npc) == 1) {
        npc->isEm = 0;
        npc->motionLayerCount = 2;
    } else {
        npc->isEm = 1;
        npc->motionLayerCount = 1;
    }
    npc->alpha = 1.0f;
    npc->hideShadow = 0;
    npc->fieldCollide = 1;
    npc->motionId[0] = 0;
    npc->motionId[1] = 0;
    npc->motionId[2] = 0;
    npc->motionId[3] = 0;
    Npc_SetupDraw(npc);
    npc->state = 1;
    if (Npc_IsType4E(npc) == 1) {
        Actor_SetMotion(npc, 0x3E9, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[1], npc->motionBlend[1], 1);
        LobbyBgm_SetActor(npc);
    } else if (Npc_IsFree((Em *)npc) == 1) {
        k = m->routeIdx * 0x14;
        v = (u32) m->route;
        npc->state = *(s32 *)(k + v + 0xC);
        k = m->routeIdx * 0x14;
        v = (u32) m->route;
        switch (*(s32 *)(k + v + 0xC)) {
        default:
            if (Npc_IsFreeAct15to19((Em *)npc) == 1) {
                v = 0x2A4;
                break;
            }
        case 0x1:
            if (Npc_IsType40(npc) != 0) {
                v = 0x364;
            } else {
                v = 1;
            }
            break;
        case 0x43:
            v = 0x291;
            break;
        case 0x35:
            v = 0x29E;
            break;
        case 0x36:
            v = 0x2A0;
            break;
        case 0x38:
            v = 0x29C;
            break;
        case 0x46:
        case 0x47:
            v = 0x295;
            break;
        case 0x44:
            v = 0x290;
            break;
        case 0x3A:
            v = 0x296;
            break;
        case 0x3B:
            v = 0x298;
            break;
        case 0x41:
            v = 0x261;
            break;
        case 0x48:
            v = 0x2A6;
            break;
        case 0x4B:
            v = 0x27E;
            break;
        case 0x4C:
            v = 0x285;
            break;
        case 0x4D:
            v = 0x284;
            break;
        }
        Actor_SetMotion(npc, v, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[1], npc->motionBlend[1], 1);
    } else {
        if (gHouseAccess != 0 && m->id == 0x14) {
            Actor_SetState(npc, 0, 0xB6);
        }
        if (((NpcRouteEnt *)m->route) == NULL) {
            v = 0x3E9;
        } else {
            switch (*(s32 *)&((NpcRouteEnt *)m->route)->state) {
            case 0x4E:
                v = 0x426;
                break;
            case 0x98:
                v = 0x43A;
                break;
            case 0x52:
            case 0xB6:
                v = 0x432;
                break;
            case 0x57:
                if (gNpc14Follows == 1) {
                    Actor_SetState(npc, 0, 0x58);
                    v = 0x3E9;
                } else {
                    v = 0x3E9;
                }
                break;
            default:
                v = 0x3E9;
                break;
            }
        }
        Actor_SetMotionOnLayer(npc, v, 0, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
    }
    k = m->routeIdx * 0x14;
    v = (u32) m->route;
    Actor_SetState(npc, 0, *(s32 *)(k + v + 0xC));
    Player_UpdateMotion((Player *) npc);
    Npc_UpdateMotion(npc);
    Npc_UpdateMotion(npc);
    Npc_FollowFloor(npc);
    Npc_UpdateAreaPos(npc);
    if (Npc_IsFree((Em *)npc) != 0) {
        Em_GetBonePos(npc, 0x14, m->unk50);
    } else if (Npc_IsAct51to55(npc) != 0) {
        Em_GetBonePos(npc, 0xA, m->unk50);
    } else if (Npc_IsBusyNotAct3C4B((Em *)npc) != 0) {
        Em_GetBonePos(npc, 0xC, m->unk50);
    } else {
        m->unk50[0] = npc->posX;
        m->unk50[1] = npc->posY + 200.0f * npc->scaleY;
        m->unk50[2] = npc->posZ;
    }
    if (m->id == 0x14 || m->id == 0x22 || m->id == 0x6D) {
        if (Area_IsOnlineHouse(gScene.area) == 1) {
            gNpc14Follows = 0;
            if (gOnlineBonusState == 2) {
                m->unk4 = 1;
            }
        }
        m->unk44 = 0;
        gPoogiePetStreak = 0;
    }
    if (Npc_IsType4E(npc) == 1) {
        npc->visible = 0;
    }
}

V0077C490_0 *NpcTable_Find(u8 kind, s32 id)
{
    u8 i;
    V0077C490_0 *e;

    if (kind == 0xFF) {
        return NULL;
    }
    id = Area_GetSetIndex(id);
    e = gAreaNpcTables[id];
    for (i = 0; i < gAreaNpcCounts[id]; i++) {
        if (e->f0 == kind) {
            return e;
        }
        e++;
    }
    return NULL;
}


s32 AreaNpc_IsPresent(s32 idx, s32 id)
{
    V0077C490_0 *tbl = gAreaNpcTables[Area_GetSetIndex(id)];

    tbl += idx;
    return Npc_IsPresent(tbl->f0, id, 0);
}

s32 Cond_Check(u16 id);
s32 PspLink_HasFlag(s32);
s32 Save_TestEventFlag(s32 flag);
/* unit's own declaration: the definition (sub_main unk_3B4C80.c) returns u8; with it Npc_IsPresent and Npc_SetRouteByKind differ (477 of 511, 67 of 335 words) */
s32 Village_FacilityLevel(s32);
u8 Village_TotalLevel(void);
s32 Npc_RollPresence(void);
/* unit's own declaration: the definition (unk_6CB690.c) returns s16; with it Npc_IsPresent differs (384 of 521 words) */
s32 Village_GetTimerState(s32);
s32 ElderRaid_InState1(void);
u8 Npc_RouteKindBySide(u8 kind);
s32 MyHouse_IsOwner(void);
s8 MyHouse_LocalSlot(void);
s32 Game_IsOnline(void);
extern u8 gNpc14Left;
extern u8 gNpc22Left;
extern u8 gNpc6DLeft;
extern u8 gNetUsers[];
extern u8 gNpc14Follows;

/* Whether the townsperson of the kind (its entry NpcTable_Find(kind, id))
 * is in the village now: the kind's own rule (event flags, conditions,
 * facility levels, the online state), then, unless force, the entry's
 * +0x15 (1 and 2: LobbyTime_IsAm; 4: Npc_RouteKindBySide of the kind). */
s32 Npc_IsPresent(u8 kind, s32 id, u8 force)
{
    V0077C490_0 *e;
    s32 total;
    u8 i;
    s8 m;
    u8 *p;

    if (kind == 0xFF) {
        return 0;
    }
    total = 0;
    for (i = 1; i <= 3; i++) {
        total = (u8)(total + Village_FacilityLevel(i));
    }
    e = NpcTable_Find(kind, id);
    if (e == NULL) {
        return 0;
    }
    switch (kind) {
    case 0x1:
        if (Save_TestEventFlag(0x186) == 1 && Save_TestEventFlag(0x187) == 0) {
            return 1;
        }
        if (Cond_Check(0x1A) == 1 && Village_GetTimerState(5) == 0) {
            return 0;
        }
        break;
    case 0x1B:
        if (Save_TestEventFlag(0x186) == 1 && Save_TestEventFlag(0x187) == 0) {
            return 0;
        }
        if (Cond_Check(0x1A) == 0 || Village_GetTimerState(5) != 0) {
            return 0;
        }
        break;
    case 0x4:
    case 0x8:
        if (Cond_Check(3) == 0) {
            return 0;
        }
        break;
    case 0x10:
        if (Cond_Check(0xD) == 0) {
            return 0;
        }
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x3E:
        if (ElderRaid_InState1() == 1) {
            return 0;
        }
        break;
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x3F:
        if (ElderRaid_InState1() == 0) {
            return 0;
        }
        break;
    case 0x6:
        if ((u8)Village_FacilityLevel(3) == 0) {
            return 0;
        }
        break;
    case 0xA:
        if ((u8)Village_FacilityLevel(3) < 2) {
            return 0;
        }
        break;
    case 0xB:
        if ((u8)Village_FacilityLevel(3) == 0) {
            return 0;
        }
        break;
    case 0xC:
    case 0x1A:
        if ((u8)Village_FacilityLevel(2) == 0) {
            return 0;
        }
        break;
    case 0xD:
        if ((u8)Village_FacilityLevel(2) < 2) {
            return 0;
        }
        break;
    case 0xF:
        if (Village_GetTimerState(4) == 0) {
            return 0;
        }
        break;
    case 0x12:
        if (Save_TestEventFlag(0x49) == 0) {
            return 0;
        }
        if (Save_TestEventFlag(0x4A) == 0) {
            return 1;
        }
        if (PspLink_HasFlag(0xB) != 0) {
            return 1;
        }
        if (Village_GetTimerState(2) == 0) {
            return 0;
        }
        break;
    case 0x13:
        if (Save_TestEventFlag(0x13F) == 0 || Save_TestEventFlag(0x12A) == 0) {
            return 0;
        }
        break;
    case 0x14:
        if (Game_IsOnline() == 1) {
            if (Area_IsOnlineHouse(gScene.area) == 1) {
                if (MyHouse_IsOwner() == 0) {
                    m = MyHouse_LocalSlot();
                    if (m == -1) {
                        return 0;
                    }
                    p = gNetUsers + m * 0x11C;
                    return (p[0xE6] != 0xFF) ? 1 : 0;
                }
                if (*(s8 *)&gSave.poogieName[0] != 0) {
                    if (gOnlineBonusState == 1 || gNpc14Left == 1) {
                        return 0;
                    }
                    return 1;
                }
            }
            return 0;
        }
        if (*(s8 *)&gSave.poogieName[0] != 0) {
            if (Area_IsOnlineHouse(gScene.area) == 0 && gNpc14Follows == 0) {
                return 0;
            }
            if (gOnlineBonusState == 1 || gNpc14Left == 1) {
                return 0;
            }
            break;
        }
        if (gScene.area != 0x89) {
            return 0;
        }
        if (Npc_IsPresent(0x12, gScene.area, 0) == 0) {
            return 0;
        }
        break;
    case 0x22:
        if (Npc_RollPresence() == 0 || gNpc22Left == 1) {
            return 0;
        }
        break;
    case 0x6B:
        if (Cond_Check(0x53) == 0) {
            return 0;
        }
        break;
    case 0x6D:
        if (gNpc6DLeft == 1) {
            return 0;
        }
        break;
    case 0x15:
        if (Save_TestEventFlag(0x13F) == 0 || Save_TestEventFlag(0x12A) == 0) {
            return 0;
        }
        break;
    case 0x16:
    case 0x17:
        if ((u8)total < 4) {
            return 0;
        }
        break;
    case 0x18:
        if ((u8)Village_TotalLevel() <= 0) {
            return 0;
        }
        break;
    case 0x19:
        if (Cond_Check(3) == 0) {
            return 0;
        }
        break;
    case 0x58:
        if (ElderRaid_InState1() == 0) {
            return 0;
        }
        break;
    case 0x6F:
        if (Game_IsOnline() == 1 || Village_GetTimerState(1) != 1) {
            return 0;
        }
        break;
    case 0x70:
        if (Game_IsOnline() == 1 || Village_GetTimerState(1) != 2) {
            return 0;
        }
        break;
    }
    if (force == 1) {
        return 1;
    }
    switch (e->f15) {
    case 1:
        if (LobbyTime_IsAm() == 0) {
            return 0;
        }
        break;
    case 2:
        if (LobbyTime_IsAm() == 1 && (kind != 9 || (u8)Village_FacilityLevel(3) != 0)) {
            return 0;
        }
        break;
    case 4:
        if (e->f0 != Npc_RouteKindBySide(e->f0)) {
            return 0;
        }
        break;
    }
    return 1;
}

void Npc_SelectRoute(Em *npc, NpcRouteEnt *a, NpcRouteEnt *b)
{
    NpcWork *m = (NpcWork *)&npc->work;
    NpcRouteEnt *cur = m->route;
    s32 f;

    f = LobbyTime_IsAm() == 0;
    if (f == 1) {
        if (cur != b) {
            m->route = b;
            m->routeIdx = 0;
            Actor_SetState(npc, 0, (u16)m->route[m->routeIdx].state);
        }
    } else if (cur != a) {
        m->route = a;
        m->routeIdx = 0;
        Actor_SetState(npc, 0, (u16)m->route[m->routeIdx].state);
    }
}

void Npc_SetRouteByTime(Em *npc, NpcRouteEnt **t)
{
    NpcWork *m = (NpcWork *)&npc->work;
    NpcRouteEnt *cur = m->route;
    s32 f;

    f = LobbyTime_IsAm() == 0;
    if (f == 1) {
        if (cur != t[3] && cur != t[2]) {
            if (t[2] == NULL) {
                m->route = t[3];
            } else {
                m->route = t[2];
            }
            m->routeIdx = 0;
            Actor_SetState(npc, 0, (u16)m->route[m->routeIdx].state);
        }
    } else if (cur != t[1] && cur != t[0]) {
        if (t[0] == NULL) {
            m->route = t[1];
        } else {
            m->route = t[0];
        }
        m->routeIdx = 0;
        Actor_SetState(npc, 0, (u16)m->route[m->routeIdx].state);
    }
}

s32 Cond_Check(u16 id);
/* unit's own declaration: the definition (sub_main unk_3B4C80.c) returns u8; with it Npc_IsPresent and Npc_SetRouteByKind differ (477 of 511, 67 of 335 words) */
s32 Village_FacilityLevel(s32);
void Npc_SetRouteByTime(Em *arg0, NpcRouteEnt **t);
s32 Area_IsHouse(u16 arg0);
s32 Game_IsOnline(void);
extern NpcRouteEnt *gNpc01Routes0[];
extern NpcRouteEnt *gNpc01Routes1[];
extern NpcRouteEnt *gNpc01Routes2[];
extern NpcRouteEnt gNpcRoute03Am[];
extern NpcRouteEnt gNpcRoute03Pm[];
extern NpcRouteEnt gNpcRoute04Am[];
extern NpcRouteEnt gNpcRoute04AmLv[];
extern NpcRouteEnt gNpcRoute04Pm[];
extern NpcRouteEnt gNpcRoute05Am[];
extern NpcRouteEnt gNpcRoute05Pm[];
extern NpcRouteEnt *gNpc06Routes[];
extern NpcRouteEnt gNpc09Routes1_3[];
extern NpcRouteEnt *gNpc09Routes1[];
extern NpcRouteEnt *gNpc09Routes2[];
extern NpcRouteEnt *gNpc10Routes0[];
extern NpcRouteEnt *gNpc10Routes1[];
extern NpcRouteEnt D_007F3E00[];
extern NpcRouteEnt *gNpc30Routes[];
extern NpcRouteEnt *gNpc31Routes[];
extern NpcRouteEnt *gNpc32Routes[];
extern NpcRouteEnt *gNpc45Routes[];
extern NpcRouteEnt *gNpc46Routes[];
extern NpcRouteEnt *gNpc47Routes[];
extern NpcRouteEnt *gNpc49Routes[];
extern NpcRouteEnt *gNpc4ARoutes[];
extern NpcRouteEnt *gNpc4BRoutes[];
extern NpcRouteEnt *gNpc4CRoutes[];
extern NpcRouteEnt *gNpc5FRoutes[];
extern NpcRouteEnt *gNpc60Routes[];
extern NpcRouteEnt *gNpc61Routes[];
extern NpcRouteEnt *gNpc62Routes[];
extern NpcRouteEnt *gNpc67Routes[];
extern NpcRouteEnt *gNpc69Routes[];
extern NpcRouteEnt *gNpc69RoutesCond53[];
extern s8 gNpc14ByPlayer;
extern u8 gNpc14Follows;

void Npc_SetRouteByKind(Em *npc)
{
    u8 i;
    NpcWork *w;
    s32 var_s0;
    s32 temp_a0_2;
    u8 id;

    w = (NpcWork *)&npc->work;
    if ((Game_IsOnline() != 0) || (w->id == 0x67) || (Area_IsHouse(gScene.area) != 1)) {
        var_s0 = 0;
        for (i = 1; i <= 3; i++) {
            var_s0 = (u8)(var_s0 + Village_FacilityLevel(i));
        }
        id = w->id;
        switch (id) {
        case 0x1:
            if (Cond_Check(1U) == 0) {
                Npc_SetRouteByTime(npc, gNpc01Routes0);
                break;
            }
            if ((Village_FacilityLevel(3) & 0xFF) == 2) {
                Npc_SetRouteByTime(npc, gNpc01Routes2);
                break;
            }
            Npc_SetRouteByTime(npc, gNpc01Routes1);
            break;
        case 0x3:
            Npc_SelectRoute(npc, gNpcRoute03Am, gNpcRoute03Pm);
            break;
        case 0x4:
            if (!(Village_FacilityLevel(2) & 0xFF)) {
                Npc_SelectRoute(npc, gNpcRoute04Am, gNpcRoute04Pm);
                break;
            }
            Npc_SelectRoute(npc, gNpcRoute04AmLv, gNpcRoute04Pm);
            break;
        case 0x5:
            Npc_SelectRoute(npc, gNpcRoute05Am, gNpcRoute05Pm);
            break;
        case 0x6:
            Npc_SetRouteByTime(npc, gNpc06Routes);
            break;
        case 0x9:
            temp_a0_2 = Village_FacilityLevel(3) & 0xFF;
            switch (temp_a0_2) {
            case 0:
                w->route = gNpc09Routes1_3;
                break;
            case 1:
                Npc_SetRouteByTime(npc, gNpc09Routes1);
                break;
            default:
                Npc_SetRouteByTime(npc, gNpc09Routes2);
            }
            break;
        case 0x10:
            if ((u8)var_s0 < 2) {
                Npc_SetRouteByTime(npc, gNpc10Routes1);
                break;
            }
            Npc_SetRouteByTime(npc, gNpc10Routes0);
            break;
        case 0x14:
            if ((npc->state != 0xAC) && (gNpc14Follows == 1) && (gScene.area == 0x89)) {
                w->route = D_007F3E00;
                gNpc14ByPlayer = 1;
            }
            break;
        case 0x30:
            Npc_SetRouteByTime(npc, gNpc30Routes);
            break;
        case 0x31:
            Npc_SetRouteByTime(npc, gNpc31Routes);
            break;
        case 0x32:
            Npc_SetRouteByTime(npc, gNpc32Routes);
            break;
        case 0x45:
            Npc_SetRouteByTime(npc, gNpc45Routes);
            break;
        case 0x46:
            Npc_SetRouteByTime(npc, gNpc46Routes);
            break;
        case 0x47:
            Npc_SetRouteByTime(npc, gNpc47Routes);
            break;
        case 0x49:
            Npc_SetRouteByTime(npc, gNpc49Routes);
            break;
        case 0x4A:
            Npc_SetRouteByTime(npc, gNpc4ARoutes);
            break;
        case 0x4B:
            Npc_SetRouteByTime(npc, gNpc4BRoutes);
            break;
        case 0x4C:
            Npc_SetRouteByTime(npc, gNpc4CRoutes);
            break;
        case 0x5F:
            Npc_SetRouteByTime(npc, gNpc5FRoutes);
            break;
        case 0x60:
            Npc_SetRouteByTime(npc, gNpc60Routes);
            break;
        case 0x61:
            Npc_SetRouteByTime(npc, gNpc61Routes);
            break;
        case 0x62:
            Npc_SetRouteByTime(npc, gNpc62Routes);
            break;
        case 0x67:
            Npc_SetRouteByTime(npc, gNpc67Routes);
            break;
        case 0x69:
            if (Cond_Check(0x53U) == 1) {
                Npc_SetRouteByTime(npc, gNpc69RoutesCond53);
                break;
            }
            Npc_SetRouteByTime(npc, gNpc69Routes);
            break;
        }
    }
}

void Npc_SetRouteFixed(Em *npc)
{
    u8 kind;
    NpcWork *w;

    kind = npc->work;
    w = (NpcWork *)&npc->work;
    switch (kind) {
    case 0x1D:
        w->route = gNpcRoute1D;
        break;
    case 0x1E:
        w->route = gNpcRoute1E;
        break;
    case 0x1F:
        w->route = gNpcRoute1F;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x21:
        w->route = gNpcRoute21;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x1C:
        npc->motionSpeed *= 1.1f;
        break;
    case 0x23:
        w->route = gNpcRoute23;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x24:
        w->route = gNpcRoute24;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3A:
        w->route = gNpcRoute25;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3B:
        w->route = gNpcRoute3B;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3C:
        w->route = gNpcRoute3C;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3D:
        w->route = gNpcRoute3D;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x40:
        w->route = gNpcRoute40;
        break;
    case 0x4E:
        w->route = gNpcRoute4E;
        npc->motionSpeed = 1.3f;
        break;
    }
}

s32 Village_NpcRecB(u16 id);
s32 MyHouse_InOwnHouse(void);
extern NpcRouteEnt gNpcRouteHouse[];
extern NpcRouteEnt gNpcRouteHouse0A[];
extern NpcRouteEnt gNpcRoute07Lv2[];
extern NpcRouteEnt gNpcRoute08Lv2[];
extern NpcRouteEnt gNpcRoute0CLv2[];
extern NpcRouteEnt gNpcRoute0E[];
extern NpcRouteEnt gMineRowsHigh[];
extern NpcRouteEnt gMineRowsLow[];
extern NpcRouteEnt gMineRowsMid[];
extern NpcRouteEnt gNpcRoute13High[];
extern NpcRouteEnt gNpcRoute13Low[];
extern NpcRouteEnt gNpcRoute13Mid[];
extern NpcRouteEnt gNpcRoute14Guest[];
extern NpcRouteEnt gNpcRoute1A[];

/* A townsperson's route table (+0x438 +0xC) by its kind, the village's
 * facility levels and the area. */
void Npc_SetRouteByVillage(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1 && Village_NpcRecB(w->id) == 1) {
        if (w->id != 0xA) {
            w->route = gNpcRouteHouse;
        } else {
            w->route = gNpcRouteHouse0A;
        }
        return;
    }
    switch (w->id) {
    case 7:
        if ((u8)Village_FacilityLevel(3) == 2) {
            w->route = gNpcRoute07Lv2;
        }
        break;
    case 8:
        if ((u8)Village_FacilityLevel(3) == 2) {
            w->route = gNpcRoute08Lv2;
        }
        break;
    case 0xC:
        if ((u8)Village_FacilityLevel(2) == 2) {
            w->route = gNpcRoute0CLv2;
            npc->rotY = 0x149F;
        }
        break;
    case 0xE:
        if (0 < (u8)Village_FacilityLevel(2)) {
            w->route = gNpcRoute0E;
            npc->rotY = 0x6E39;
        }
        break;
    case 0x13:
        if (*(u8 *)((u8 *)&gSave + 0x37F3) == 1) {
            if ((u8)Village_FacilityLevel(0) >= 6) {
                w->route = gMineRowsHigh;
            } else if ((u8)Village_FacilityLevel(0) <= 2) {
                w->route = gMineRowsLow;
            } else {
                w->route = gMineRowsMid;
            }
        } else {
            if ((u8)Village_FacilityLevel(0) >= 6) {
                w->route = gNpcRoute13High;
            } else if ((u8)Village_FacilityLevel(0) <= 2) {
                w->route = gNpcRoute13Low;
            } else {
                w->route = gNpcRoute13Mid;
            }
        }
        break;
    case 0x14:
        if (Game_IsOnline() == 1 && MyHouse_InOwnHouse() == 0) {
            w->route = gNpcRoute14Guest;
        }
        break;
    case 0x1A:
        if (Npc_IsPresent(0xF, gScene.area, 0) == 1) {
            w->route = gNpcRoute1A;
            npc->rotY = 0x6E39;
        }
        break;
    }
}

struct OfferTask;
s16 OfferMenu_BuildList(struct OfferTask *t, s32 b);
s32 Village_MealPointsReady(void);
s32 ElderRaid_InState1(void);
s32 Npc14_PlaceByPlayer(void);
s32 Npc_IsId10or1E(Em *npc);
void DrinkMug_Spawn(s32 npc);
s32 MyHouse_IsOwner(void);

/* Sets up townsperson npc (+0xC its index) from the area's table entry:
 * kind, model, colours, scale, route and its start point. */
/* the route the npc work (+0x438) points at: read through w + 0xC at each
 * use, so the loops' base is a value MWCC makes (numbered after their
 * walkers) */

void Npc_SetupFromArea(Em *npc, u8 keep, s32 id)
{
    V0077C490_0 *e;
    NpcWork *w;
    u8 area;
    s32 i;
    u16 v;
    u8 *save;

    w = (NpcWork *)&npc->work;
    area = Area_GetSetIndex(id);
    save = (u8 *)&gSave;
    e = gAreaNpcTables[area];
    if (e == NULL) {
        return;
    }
    e += npc->index;
    npc->rotZ = 0;
    npc->rotX = 0;
    npc->rotY = e->f4;
    npc->variant = e->f1;
    npc->modelSlot = npc->type = e->f14;
    w->unk49 = 0;
    w->name = e->name;
    if (e->f0 == 0x14) {
        w->name = (char *) (save + 0x2C98);
    }
    w->id = e->f0;
    w->route = e->route;
    Npc_SetRouteByKind(npc);
    w->unk48 = e->f15;
    w->busy = e->f14;
    w->activity = e->f1;
    npc->motionSpeed = e->f0C;
    npc->scaleX = npc->scaleY = npc->scaleZ = e->scale;
    w->holdRoute = 0;
    switch (OfferMenu_BuildList((struct OfferTask *) npc, 1)) {
    default:
        w->unk4 = 1;
        break;
    case 0:
        w->unk4 = 0;
        break;
    case 4:
        w->unk4 = 2;
        break;
    }
    if (w->id == 0x71 && MyHouse_IsOwner() != 0 && Village_MealPointsReady() == 1) {
        w->unk4 = 2;
    }
    if (ElderRaid_InState1() == 1) {
        Npc_SetRouteFixed(npc);
    }
    Npc_SetRouteByVillage(npc);
    if (keep == 0 || e->f15 == 4) {
        for (i = 0; w->route[i].state != 0; i++) {
            if (w->route[i].state == 0x83 || w->route[i].state == 0xB3) {
                w->route = (NpcRouteEnt *) w->route[i].arg;
                break;
            }
        }
        v = e->f4;
        for (i = 0; w->route[i].state != 0; i++) {
            if (w->route[i].state == 0x81) {
                v = w->route[i].arg;
            }
            if (w->route[i].arg == -1) {
                npc->rotY = v;
                w->routeIdx = i;
                break;
            }
        }
    }
    *(NpcPos *)&npc->posX = w->route[w->routeIdx].pos;
    if (w->id == 0x14 && Npc14_PlaceByPlayer() == 1 && Area_IsOnlineHouse(gScene.area) == 1) {
        gNpc14ByPlayer = 0;
    }
    if (Npc_IsId10or1E(npc)) {
        DrinkMug_Spawn((s32)npc);
    }
}

s32 Npc_IsFreeAct2Bto30(Em *arg0);
struct TriHdr;
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, struct TriHdr *info, f32 *out, f32 *depth);

void Npc_FollowFloor(Em *npc)
{
    switch (npc->work) {
    case 0x7:
    case 0x8:
    case 0xE:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x20:
    case 0x2C:
    case 0x2E:
    case 0x40:
    case 0x48:
    case 0x4D:
    case 0x15:
        return;
    case 0x1A:
    case 0xA:
        if (gScene.area == 0x89) {
            break;
        }
    default:
        if (Npc_IsFreeAct2Bto30((Em *)npc) != 1) {
            Em_GetFloorInfoEx(npc, &npc->posX, (struct TriHdr *) &npc->floorKind, &npc->floorY, &npc->surfaceY);
            if (npc->stance != 2) {
                npc->posY = npc->floorY;
            }
        }
        break;
    }
}

f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Npc_UpdateMotion(Em *arg0);
s32 Npc_IsType4E(Em *arg0);
s32 Npc_UpdateState(Em *a);
void TownPc_TickTimers(Player *arg0);
void Actor_StepMove(Em *obj);
void Npc_UpdateHitStop(Em *arg0);
s32 LobbyBgm_IsActive(void);

s32 Npc_Run(Em *npc)
{
    if (Npc_IsType4E(npc) == 1 && LobbyBgm_IsActive()) {
        return 0;
    }
    npc->noPushTimer = 0xA;
    TownPc_TickTimers((Player *) npc);
    Npc_UpdateHitStop(npc);
    npc->prevPosX = npc->posX;
    npc->prevPosY = npc->posY;
    npc->prevPosZ = npc->posZ;
    Actor_StepMove(npc);
    Npc_UpdateState(npc);
    npc->rotX = (u16)npc->rotX;
    npc->rotY = (u16)npc->rotY;
    npc->rotZ = (u16)npc->rotZ;
    Mtx_FromAnglesYXZ(&npc->rotX, &npc->rotMtx[0][0]);
    npc->motion[0].step = 2.0f * npc->motionSpeed;
    npc->motion[1].step = 2.0f * npc->motionSpeed;
    npc->motion[2].step = 2.0f * npc->motionSpeed;
    npc->motion[3].step = 2.0f * npc->motionSpeed;
    Npc_UpdateMotion(npc);
    Npc_FollowFloor(npc);
    if (npc->state == 0x60) {
        npc->taskPhase++;
    }
    return 1;
}

/* unk_77A960's .data (0x7F20B0-0x7FBEA0) */
extern u32 D_007FA690[];
extern u32 D_007FA780[];
NpcRouteEnt gNpc01Routes1_1[31] = {
#include "lobby/007F20B0.inc"
};
NpcRouteEnt gNpc01Routes2_1[31] = {
#include "lobby/007F2320.inc"
};
NpcRouteEnt gNpc01Routes1_3[7] = {
#include "lobby/007F2590.inc"
};
NpcRouteEnt gNpc01Routes1_0[6] = {
#include "lobby/007F2620.inc"
};
NpcRouteEnt gNpc01Routes2_0[6] = {
#include "lobby/007F26A0.inc"
};
NpcRouteEnt gNpc01Routes0_0[3] = {
#include "lobby/007F2720.inc"
};
NpcRouteEnt *gNpc01Routes0[4] = {
    gNpc01Routes0_0, gNpc01Routes0_0, NULL, gNpc01Routes0_0,
};
NpcRouteEnt *gNpc01Routes1[4] = {
    gNpc01Routes1_0, gNpc01Routes1_1, NULL, gNpc01Routes1_3,
};
NpcRouteEnt *gNpc01Routes2[4] = {
    gNpc01Routes2_0, gNpc01Routes2_1, NULL, gNpc01Routes1_3,
};
u32 D_007F2790[12] = {
#include "lobby/007F2790.inc"
};
NpcRouteEnt gNpcRoute03Am[6] = {
#include "lobby/007F27C0.inc"
};
NpcRouteEnt gNpcRoute03Pm[4] = {
#include "lobby/007F2840.inc"
};
NpcRouteEnt gNpcRoute04Am[13] = {
#include "lobby/007F2890.inc"
};
NpcRouteEnt gNpcRoute04AmLv[14] = {
#include "lobby/007F29A0.inc"
};
NpcRouteEnt gNpcRoute04Pm[5] = {
#include "lobby/007F2AC0.inc"
};
NpcRouteEnt gNpcRoute05Am[7] = {
#include "lobby/007F2B30.inc"
};
NpcRouteEnt gNpcRoute05Pm[7] = {
#include "lobby/007F2BC0.inc"
};
NpcRouteEnt gNpc06Routes_1[20] = {
#include "lobby/007F2C50.inc"
};
NpcRouteEnt gNpc06Routes_2[2] = {
#include "lobby/007F2DE0.inc"
};
NpcRouteEnt *gNpc06Routes[4] = {
    NULL, gNpc06Routes_1, gNpc06Routes_2, NULL,
};
u32 D_007F2E20[12] = {
#include "lobby/007F2E20.inc"
};
NpcRouteEnt gNpcRoute07Lv2[2] = {
#include "lobby/007F2E50.inc"
};
u32 D_007F2E80[12] = {
#include "lobby/007F2E80.inc"
};
NpcRouteEnt gNpcRoute08Lv2[3] = {
#include "lobby/007F2EB0.inc"
};
NpcRouteEnt gNpc09Routes1_3[18] = {
#include "lobby/007F2EF0.inc"
};
NpcRouteEnt gNpc09Routes2_3[23] = {
#include "lobby/007F3060.inc"
};
NpcRouteEnt gNpc09Routes1_0[6] = {
#include "lobby/007F3230.inc"
};
NpcRouteEnt gNpc09Routes2_0[2] = {
#include "lobby/007F32B0.inc"
};
NpcRouteEnt gNpc09Routes1_2[6] = {
#include "lobby/007F32E0.inc"
};
NpcRouteEnt *gNpc09Routes1[4] = {
    gNpc09Routes1_0, NULL, gNpc09Routes1_2, gNpc09Routes1_3,
};
NpcRouteEnt *gNpc09Routes2[4] = {
    gNpc09Routes2_0, NULL, NULL, gNpc09Routes2_3,
};
u32 D_007F3380[16] = {
#include "lobby/007F3380.inc"
};
u32 D_007F33C0[12] = {
#include "lobby/007F33C0.inc"
};
u32 D_007F33F0[20] = {
#include "lobby/007F33F0.inc"
};
NpcRouteEnt gNpcRoute0CLv2[3] = {
#include "lobby/007F3440.inc"
};
u32 D_007F3480[16] = {
#include "lobby/007F3480.inc"
};
u32 D_007F34C0[16] = {
#include "lobby/007F34C0.inc"
};
NpcRouteEnt gNpcRoute0E[3] = {
#include "lobby/007F3500.inc"
};
u32 D_007F3540[20] = {
#include "lobby/007F3540.inc"
};
NpcRouteEnt gNpc10Routes1_1[15] = {
#include "lobby/007F3590.inc"
};
NpcRouteEnt gNpc10Routes0_1[15] = {
#include "lobby/007F36C0.inc"
};
NpcRouteEnt gNpc10Routes0_3[11] = {
#include "lobby/007F37F0.inc"
};
NpcRouteEnt gNpc10Routes0_0[16] = {
#include "lobby/007F38D0.inc"
};
NpcRouteEnt gNpc10Routes1_0[16] = {
#include "lobby/007F3A10.inc"
};
NpcRouteEnt *gNpc10Routes0[4] = {
    gNpc10Routes0_0, gNpc10Routes0_1, NULL, gNpc10Routes0_3,
};
NpcRouteEnt *gNpc10Routes1[4] = {
    gNpc10Routes1_0, gNpc10Routes1_1, NULL, gNpc10Routes0_3,
};
u32 D_007F3B70[56] = {
#include "lobby/007F3B70.inc"
};
NpcRouteEnt gNpcRoute13Mid[2] = {
#include "lobby/007F3C50.inc"
};
NpcRouteEnt gMineRowsMid[4] = {
#include "lobby/007F3C80.inc"
};
NpcRouteEnt gNpcRoute13Low[2] = {
#include "lobby/007F3CD0.inc"
};
NpcRouteEnt gMineRowsLow[4] = {
#include "lobby/007F3D00.inc"
};
NpcRouteEnt gNpcRoute13High[2] = {
#include "lobby/007F3D50.inc"
};
NpcRouteEnt gMineRowsHigh[4] = {
#include "lobby/007F3D80.inc"
};
u32 D_007F3DD0[12] = {
#include "lobby/007F3DD0.inc"
};
NpcRouteEnt D_007F3E00[2] = {
#include "lobby/007F3E00.inc"
};
NpcRouteEnt gNpcRoute14Guest[2] = {
#include "lobby/007F3E30.inc"
};
u32 D_007F3E60[28] = {
#include "lobby/007F3E60.inc"
};
u32 D_007F3ED0[20] = {
#include "lobby/007F3ED0.inc"
};
u32 D_007F3F20[36] = {
#include "lobby/007F3F20.inc"
};
u32 D_007F3FB0[20] = {
#include "lobby/007F3FB0.inc"
};
u32 D_007F4000[12] = {
#include "lobby/007F4000.inc"
};
u32 D_007F4030[12] = {
#include "lobby/007F4030.inc"
};
u32 D_007F4060[120] = {
#include "lobby/007F4060.inc"
};
u32 D_007F4240[32] = {
#include "lobby/007F4240.inc"
};
u32 D_007F42C0[20] = {
#include "lobby/007F42C0.inc"
};
NpcRouteEnt gNpcRoute1A[4] = {
#include "lobby/007F4310.inc"
};
u32 D_007F4360[72] = {
#include "lobby/007F4360.inc"
};
u32 gAreaNpcTables_0_4[12] = {
#include "lobby/007F4480.inc"
};
u32 gAreaNpcTables_0_22[16] = {
#include "lobby/007F44B0.inc"
};
NpcRouteEnt gNpcRoute1D[3] = {
#include "lobby/007F44F0.inc"
};
u32 gAreaNpcTables_0_40[12] = {
#include "lobby/007F4530.inc"
};
NpcRouteEnt gNpcRoute1E[2] = {
#include "lobby/007F4560.inc"
};
u32 gAreaNpcTables_0_58[12] = {
#include "lobby/007F4590.inc"
};
NpcRouteEnt gNpcRoute1F[3] = {
#include "lobby/007F45C0.inc"
};
u32 gAreaNpcTables_0_76[12] = {
#include "lobby/007F4600.inc"
};
u32 gAreaNpcTables_0_94[12] = {
#include "lobby/007F4630.inc"
};
NpcRouteEnt gNpcRoute21[3] = {
#include "lobby/007F4660.inc"
};
u32 gAreaNpcTables_0_112[20] = {
#include "lobby/007F46A0.inc"
};
u32 gAreaNpcTables_0_130[12] = {
#include "lobby/007F46F0.inc"
};
NpcRouteEnt gNpcRoute23[3] = {
#include "lobby/007F4720.inc"
};
u32 gAreaNpcTables_0_148[108] = {
#include "lobby/007F4760.inc"
};
NpcRouteEnt gNpcRoute24[3] = {
#include "lobby/007F4910.inc"
};
u32 gAreaNpcTables_0_166[56] = {
#include "lobby/007F4950.inc"
};
u32 gAreaNpcTables_0_184[40] = {
#include "lobby/007F4A30.inc"
};
u32 gAreaNpcTables_0_202[32] = {
#include "lobby/007F4AD0.inc"
};
u32 gAreaNpcTables_0_220[12] = {
#include "lobby/007F4B50.inc"
};
u32 gAreaNpcTables_0_238[36] = {
#include "lobby/007F4B80.inc"
};
u32 gAreaNpcTables_0_256[48] = {
#include "lobby/007F4C10.inc"
};
u32 gAreaNpcTables_0_274[48] = {
#include "lobby/007F4CD0.inc"
};
u32 D_007F4D90[12] = {
#include "lobby/007F4D90.inc"
};
u32 D_007F4DC0[12] = {
#include "lobby/007F4DC0.inc"
};
u32 D_007F4DF0[12] = {
#include "lobby/007F4DF0.inc"
};
u32 D_007F4E20[12] = {
#include "lobby/007F4E20.inc"
};
NpcRouteEnt gNpc30Routes_1[20] = {
#include "lobby/007F4E50.inc"
};
NpcRouteEnt gNpc30Routes_3[22] = {
#include "lobby/007F4FE0.inc"
};
NpcRouteEnt *gNpc30Routes[4] = {
    NULL, gNpc30Routes_1, NULL, gNpc30Routes_3,
};
NpcRouteEnt gNpc31Routes_1[8] = {
#include "lobby/007F51B0.inc"
};
NpcRouteEnt gNpc31Routes_3[6] = {
#include "lobby/007F5250.inc"
};
NpcRouteEnt *gNpc31Routes[4] = {
    NULL, gNpc31Routes_1, NULL, gNpc31Routes_3,
};
NpcRouteEnt gNpc32Routes_1[7] = {
#include "lobby/007F52E0.inc"
};
NpcRouteEnt gNpc32Routes_3[8] = {
#include "lobby/007F5370.inc"
};
NpcRouteEnt *gNpc32Routes[4] = {
    NULL, gNpc32Routes_1, NULL, gNpc32Routes_3,
};
u32 D_007F5420[16] = {
#include "lobby/007F5420.inc"
};
u32 D_007F5460[32] = {
#include "lobby/007F5460.inc"
};
u32 D_007F54E0[36] = {
#include "lobby/007F54E0.inc"
};
u32 D_007F5570[60] = {
#include "lobby/007F5570.inc"
};
u32 D_007F5660[12] = {
#include "lobby/007F5660.inc"
};
u32 D_007F5690[12] = {
#include "lobby/007F5690.inc"
};
u32 D_007F56C0[12] = {
#include "lobby/007F56C0.inc"
};
u32 D_007F56F0[52] = {
#include "lobby/007F56F0.inc"
};
NpcRouteEnt gNpcRoute25[7] = {
#include "lobby/007F57C0.inc"
};
u32 D_007F5850[12] = {
#include "lobby/007F5850.inc"
};
NpcRouteEnt gNpcRoute3B[3] = {
#include "lobby/007F5880.inc"
};
u32 D_007F58C0[12] = {
#include "lobby/007F58C0.inc"
};
NpcRouteEnt gNpcRoute3C[3] = {
#include "lobby/007F58F0.inc"
};
u32 D_007F5930[12] = {
#include "lobby/007F5930.inc"
};
NpcRouteEnt gNpcRoute3D[3] = {
#include "lobby/007F5960.inc"
};
u32 D_007F59A0[88] = {
#include "lobby/007F59A0.inc"
};
u32 D_007F5B00[88] = {
#include "lobby/007F5B00.inc"
};
u32 D_007F5C60[12] = {
#include "lobby/007F5C60.inc"
};
NpcRouteEnt gNpcRoute40[2] = {
#include "lobby/007F5C90.inc"
};
u32 D_007F5CC0[12] = {
#include "lobby/007F5CC0.inc"
};
u32 D_007F5CF0[12] = {
#include "lobby/007F5CF0.inc"
};
NpcRouteEnt gNpc45Routes_1[5] = {
#include "lobby/007F5D20.inc"
};
NpcRouteEnt gNpc45Routes_0[7] = {
#include "lobby/007F5D90.inc"
};
NpcRouteEnt gNpc45Routes_2[5] = {
#include "lobby/007F5E20.inc"
};
NpcRouteEnt *gNpc45Routes[4] = {
    gNpc45Routes_0, gNpc45Routes_1, gNpc45Routes_2, NULL,
};
NpcRouteEnt gNpc46Routes_1[5] = {
#include "lobby/007F5EA0.inc"
};
NpcRouteEnt gNpc46Routes_0[8] = {
#include "lobby/007F5F10.inc"
};
NpcRouteEnt gNpc46Routes_2[7] = {
#include "lobby/007F5FB0.inc"
};
NpcRouteEnt *gNpc46Routes[4] = {
    gNpc46Routes_0, gNpc46Routes_1, gNpc46Routes_2, NULL,
};
NpcRouteEnt gNpc47Routes_1[5] = {
#include "lobby/007F6050.inc"
};
NpcRouteEnt gNpc47Routes_0[11] = {
#include "lobby/007F60C0.inc"
};
NpcRouteEnt gNpc47Routes_2[10] = {
#include "lobby/007F61A0.inc"
};
NpcRouteEnt *gNpc47Routes[4] = {
    gNpc47Routes_0, gNpc47Routes_1, gNpc47Routes_2, NULL,
};
NpcRouteEnt gNpc49Routes_0[9] = {
#include "lobby/007F6280.inc"
};
NpcRouteEnt gNpc49Routes_3[3] = {
#include "lobby/007F6340.inc"
};
NpcRouteEnt gNpc49Routes_2[11] = {
#include "lobby/007F6380.inc"
};
NpcRouteEnt *gNpc49Routes[4] = {
    gNpc49Routes_0, NULL, gNpc49Routes_2, gNpc49Routes_3,
};
NpcRouteEnt gNpc4ARoutes_3[3] = {
#include "lobby/007F6470.inc"
};
NpcRouteEnt gNpc4ARoutes_2[11] = {
#include "lobby/007F64B0.inc"
};
NpcRouteEnt gNpc4ARoutes_0[7] = {
#include "lobby/007F6590.inc"
};
NpcRouteEnt *gNpc4ARoutes[4] = {
    gNpc4ARoutes_0, NULL, gNpc4ARoutes_2, gNpc4ARoutes_3,
};
NpcRouteEnt gNpc4BRoutes_3[3] = {
#include "lobby/007F6630.inc"
};
NpcRouteEnt gNpc4BRoutes_2[11] = {
#include "lobby/007F6670.inc"
};
NpcRouteEnt gNpc4BRoutes_0[9] = {
#include "lobby/007F6750.inc"
};
NpcRouteEnt *gNpc4BRoutes[4] = {
    gNpc4BRoutes_0, NULL, gNpc4BRoutes_2, gNpc4BRoutes_3,
};
NpcRouteEnt gNpc4CRoutes_3[3] = {
#include "lobby/007F6820.inc"
};
NpcRouteEnt gNpc4CRoutes_2[8] = {
#include "lobby/007F6860.inc"
};
NpcRouteEnt gNpc4CRoutes_0[6] = {
#include "lobby/007F6900.inc"
};
NpcRouteEnt *gNpc4CRoutes[4] = {
    gNpc4CRoutes_0, NULL, gNpc4CRoutes_2, gNpc4CRoutes_3,
};
u32 D_007F6990[12] = {
#include "lobby/007F6990.inc"
};
u32 D_007F69C0[12] = {
#include "lobby/007F69C0.inc"
};
NpcRouteEnt gNpcRoute4E[7] = {
#include "lobby/007F69F0.inc"
};
u32 D_007F6A80[12] = {
#include "lobby/007F6A80.inc"
};
u32 D_007F6AB0[12] = {
#include "lobby/007F6AB0.inc"
};
u32 D_007F6AE0[148] = {
#include "lobby/007F6AE0.inc"
};
u32 D_007F6D30[12] = {
#include "lobby/007F6D30.inc"
};
u32 D_007F6D60[12] = {
#include "lobby/007F6D60.inc"
};
u32 D_007F6D90[12] = {
#include "lobby/007F6D90.inc"
};
u32 D_007F6DC0[12] = {
#include "lobby/007F6DC0.inc"
};
u32 D_007F6DF0[12] = {
#include "lobby/007F6DF0.inc"
};
u32 D_007F6E20[96] = {
#include "lobby/007F6E20.inc"
};
u32 gNpcRouteSel3[16] = {
#include "lobby/007F6FA0.inc"
};
u32 gNpcRouteSel4[28] = {
#include "lobby/007F6FE0.inc"
};
u32 gNpcRouteSel5[32] = {
#include "lobby/007F7050.inc"
};
u32 D_007F70D0[12] = {
#include "lobby/007F70D0.inc"
};
u32 D_007F7100[12] = {
#include "lobby/007F7100.inc"
};
u32 D_007F7130[12] = {
#include "lobby/007F7130.inc"
};
u32 gNpcRouteSel0[16] = {
#include "lobby/007F7160.inc"
};
u32 gNpcRouteSel1[20] = {
#include "lobby/007F71A0.inc"
};
u32 gNpcRouteSel2[28] = {
#include "lobby/007F71F0.inc"
};
u32 D_007F7260[12] = {
#include "lobby/007F7260.inc"
};
u32 D_007F7290[12] = {
#include "lobby/007F7290.inc"
};
NpcRouteEnt gNpc5FRoutes_1[2] = {
#include "lobby/007F72C0.inc"
};
NpcRouteEnt gNpc5FRoutes_3[12] = {
#include "lobby/007F72F0.inc"
};
NpcRouteEnt gNpc5FRoutes_0[7] = {
#include "lobby/007F73E0.inc"
};
NpcRouteEnt gNpc5FRoutes_2[8] = {
#include "lobby/007F7470.inc"
};
NpcRouteEnt *gNpc5FRoutes[4] = {
    gNpc5FRoutes_0, gNpc5FRoutes_1, gNpc5FRoutes_2, gNpc5FRoutes_3,
};
NpcRouteEnt gNpc60Routes_1[2] = {
#include "lobby/007F7520.inc"
};
NpcRouteEnt gNpc60Routes_3[2] = {
#include "lobby/007F7550.inc"
};
NpcRouteEnt gNpc60Routes_0[4] = {
#include "lobby/007F7580.inc"
};
NpcRouteEnt gNpc60Routes_2[3] = {
#include "lobby/007F75D0.inc"
};
NpcRouteEnt *gNpc60Routes[4] = {
    gNpc60Routes_0, gNpc60Routes_1, gNpc60Routes_2, gNpc60Routes_3,
};
NpcRouteEnt gNpc61Routes_1[2] = {
#include "lobby/007F7620.inc"
};
NpcRouteEnt gNpc61Routes_3[2] = {
#include "lobby/007F7650.inc"
};
NpcRouteEnt gNpc61Routes_0[5] = {
#include "lobby/007F7680.inc"
};
NpcRouteEnt gNpc61Routes_2[4] = {
#include "lobby/007F76F0.inc"
};
NpcRouteEnt *gNpc61Routes[4] = {
    gNpc61Routes_0, gNpc61Routes_1, gNpc61Routes_2, gNpc61Routes_3,
};
NpcRouteEnt gNpc62Routes_1[22] = {
#include "lobby/007F7750.inc"
};
NpcRouteEnt gNpc62Routes_3[3] = {
#include "lobby/007F7910.inc"
};
NpcRouteEnt gNpc62Routes_0[10] = {
#include "lobby/007F7950.inc"
};
NpcRouteEnt gNpc62Routes_2[13] = {
#include "lobby/007F7A20.inc"
};
NpcRouteEnt *gNpc62Routes[4] = {
    gNpc62Routes_0, gNpc62Routes_1, gNpc62Routes_2, gNpc62Routes_3,
};
u32 D_007F7B40[12] = {
#include "lobby/007F7B40.inc"
};
u32 D_007F7B70[12] = {
#include "lobby/007F7B70.inc"
};
u32 D_007F7BA0[40] = {
#include "lobby/007F7BA0.inc"
};
NpcRouteEnt gNpc67Routes_1[12] = {
#include "lobby/007F7C40.inc"
};
NpcRouteEnt gNpc67Routes_3[3] = {
#include "lobby/007F7D30.inc"
};
NpcRouteEnt gNpc67Routes_2[11] = {
#include "lobby/007F7D70.inc"
};
NpcRouteEnt gNpc67Routes_0[21] = {
#include "lobby/007F7E50.inc"
};
NpcRouteEnt *gNpc67Routes[4] = {
    gNpc67Routes_0, gNpc67Routes_1, gNpc67Routes_2, gNpc67Routes_3,
};
u32 D_007F8010[12] = {
#include "lobby/007F8010.inc"
};
NpcRouteEnt gNpc69Routes_1[3] = {
#include "lobby/007F8040.inc"
};
NpcRouteEnt gNpc69RoutesCond53_1[3] = {
#include "lobby/007F8080.inc"
};
NpcRouteEnt gNpc69Routes_3[3] = {
#include "lobby/007F80C0.inc"
};
NpcRouteEnt gNpc69Routes_0[11] = {
#include "lobby/007F8100.inc"
};
NpcRouteEnt gNpc69RoutesCond53_0[11] = {
#include "lobby/007F81E0.inc"
};
NpcRouteEnt gNpc69Routes_2[11] = {
#include "lobby/007F82C0.inc"
};
NpcRouteEnt *gNpc69Routes[4] = {
    gNpc69Routes_0, gNpc69Routes_1, gNpc69Routes_2, gNpc69Routes_3,
};
NpcRouteEnt *gNpc69RoutesCond53[4] = {
    gNpc69RoutesCond53_0, gNpc69RoutesCond53_1, gNpc69Routes_2, gNpc69Routes_3,
};
u32 D_007F83C0[40] = {
#include "lobby/007F83C0.inc"
};
u32 D_007F8460[12] = {
#include "lobby/007F8460.inc"
};
u32 D_007F8490[12] = {
#include "lobby/007F8490.inc"
};
u32 D_007F84C0[20] = {
#include "lobby/007F84C0.inc"
};
u32 D_007F8510[20] = {
#include "lobby/007F8510.inc"
};
u32 D_007F8560[28] = {
#include "lobby/007F8560.inc"
};
u32 gNpcRouteToHairChair[20] = {
#include "lobby/007F85D0.inc"
};
u32 gNpcRouteFromHairChair[28] = {
#include "lobby/007F8620.inc"
};
u32 gNpcRouteKitchen_34[16] = {
#include "lobby/007F8690.inc"
};
u32 gNpcRouteKitchen[40] = {
#include "lobby/007F86D0.inc"
};
u32 gKitchenRoute0_44[16] = {
#include "lobby/007F8770.inc"
};
u32 gKitchenRoute0[52] = {
#include "lobby/007F87B0.inc"
};
u32 gKitchenRoute1[68] = {
#include "lobby/007F8880.inc"
};
u32 gKitchenRoute2[48] = {
#include "lobby/007F8990.inc"
};
u32 gNpcPlaceRoutes0_0[76] = {
#include "lobby/007F8A50.inc"
};
u32 gNpcPlaceRoutes0_1[68] = {
#include "lobby/007F8B80.inc"
};
u32 gNpcPlaceRoutes1_0[72] = {
#include "lobby/007F8C90.inc"
};
u32 gNpcPlaceRoutes1_1[68] = {
#include "lobby/007F8DB0.inc"
};
u32 gNpcPlaceRoutes1_2[68] = {
#include "lobby/007F8EC0.inc"
};
u32 gNpcPlaceRoutes1_3[60] = {
#include "lobby/007F8FD0.inc"
};
u32 gNpcPlaceRoutes1_4[76] = {
#include "lobby/007F90C0.inc"
};
u32 gNpcPlaceRoutes1_5[68] = {
#include "lobby/007F91F0.inc"
};
u32 gNpcPlaceRoutes2_0[68] = {
#include "lobby/007F9300.inc"
};
u32 gNpcPlaceRoutes2_1[68] = {
#include "lobby/007F9410.inc"
};
u32 gNpcPlaceRoutes2_6[88] = {
#include "lobby/007F9520.inc"
};
u32 gNpcPlaceRoutes2_7[96] = {
#include "lobby/007F9680.inc"
};
u32 gNpcPlaceRoutes2_8[96] = {
#include "lobby/007F9800.inc"
};
u32 gNpcPlaceRoutes2_9[96] = {
#include "lobby/007F9980.inc"
};
u32 gNpcPlaceRoutes2_10[108] = {
#include "lobby/007F9B00.inc"
};
u32 gNpcPlaceRoutes2_11[96] = {
#include "lobby/007F9CB0.inc"
};
u32 gNpcPlaceRoutes2_12[96] = {
#include "lobby/007F9E30.inc"
};
u32 gNpcPlaceRoutes2_13[88] = {
#include "lobby/007F9FB0.inc"
};
u32 gNpcPlaceRoutes2_14[88] = {
#include "lobby/007FA110.inc"
};
u32 gNpcPlaceRoutes2_15[88] = {
#include "lobby/007FA270.inc"
};
u32 gNpcPlaceRoutes3_0[56] = {
#include "lobby/007FA3D0.inc"
};
u32 gNpcPlaceRoutes0[4] = {
    (u32)gNpcPlaceRoutes0_0, (u32)gNpcPlaceRoutes0_1, 0, 0,
};
u32 gNpcPlaceRoutes1[8] = {
    (u32)gNpcPlaceRoutes1_0, (u32)gNpcPlaceRoutes1_1, (u32)gNpcPlaceRoutes1_2, (u32)gNpcPlaceRoutes1_3,
    (u32)gNpcPlaceRoutes1_4, (u32)gNpcPlaceRoutes1_5, 0, 0,
};
u32 gNpcPlaceRoutes2[16] = {
    (u32)gNpcPlaceRoutes2_0, (u32)gNpcPlaceRoutes2_1, (u32)gNpcPlaceRoutes1_2, (u32)gNpcPlaceRoutes1_3,
    (u32)gNpcPlaceRoutes1_4, (u32)gNpcPlaceRoutes1_5, (u32)gNpcPlaceRoutes2_6, (u32)gNpcPlaceRoutes2_7,
    (u32)gNpcPlaceRoutes2_8, (u32)gNpcPlaceRoutes2_9, (u32)gNpcPlaceRoutes2_10, (u32)gNpcPlaceRoutes2_11,
    (u32)gNpcPlaceRoutes2_12, (u32)gNpcPlaceRoutes2_13, (u32)gNpcPlaceRoutes2_14, (u32)gNpcPlaceRoutes2_15,
};
u32 gNpcPlaceRoutes3[4] = {
    (u32)gNpcPlaceRoutes3_0, 0, 0, 0,
};
u32 D_007FA530[48] = {
#include "lobby/007FA530.inc"
};
u32 D_007FA5F0[40] = {
#include "lobby/007FA5F0.inc"
};
u32 D_007FA690[60] = {
#include "lobby/007FA690.inc"
};
u32 D_007FA780[20] = {
#include "lobby/007FA780.inc"
};
NpcRouteEnt gNpcRouteHouse[3] = {
#include "lobby/007FA7D0.inc"
};
NpcRouteEnt gNpcRouteHouse0A[3] = {
#include "lobby/007FA810.inc"
};
NpcColor gNpcColorSets_0[6] = {
#include "lobby/007FA850.inc"
};
NpcColor gNpcColorSets_1[6] = {
#include "lobby/007FA880.inc"
};
NpcColor gNpcColorSets_2[6] = {
#include "lobby/007FA8B0.inc"
};
NpcColor gNpcColorSets_3[6] = {
#include "lobby/007FA8E0.inc"
};
NpcColor gNpcColorSets_4[6] = {
#include "lobby/007FA910.inc"
};
NpcColor gNpcColorSets_5[6] = {
#include "lobby/007FA940.inc"
};
NpcColor gNpcColorSets_6[6] = {
#include "lobby/007FA970.inc"
};
NpcColor gNpcColorSets_7[6] = {
#include "lobby/007FA9A0.inc"
};
NpcColor gNpcColorSets_8[6] = {
#include "lobby/007FA9D0.inc"
};
NpcColor gNpcColorSets_9[6] = {
#include "lobby/007FAA00.inc"
};
NpcColor gNpcColorSets_10[4] = {
#include "lobby/007FAA30.inc"
};
NpcColor gNpcColorSets_11[6] = {
#include "lobby/007FAA50.inc"
};
NpcColor gNpcColorSets_65[8] = {
#include "lobby/007FAA80.inc"
};
NpcColor gNpcColorSets_66[6] = {
#include "lobby/007FAAC0.inc"
};
NpcColor gNpcColorSets_67[6] = {
#include "lobby/007FAAF0.inc"
};
NpcColor gNpcColorSets_68[8] = {
#include "lobby/007FAB20.inc"
};
NpcColor gNpcColorSets_69[8] = {
#include "lobby/007FAB60.inc"
};
NpcColor gNpcColorSets_70[6] = {
#include "lobby/007FABA0.inc"
};
NpcColor gNpcColorSets_71[10] = {
#include "lobby/007FABD0.inc"
};
NpcColor gNpcColorSets_72[12] = {
#include "lobby/007FAC20.inc"
};
NpcColor gNpcColorSets_73[8] = {
#include "lobby/007FAC80.inc"
};
NpcColor gNpcColorSets_74[8] = {
#include "lobby/007FACC0.inc"
};
NpcColor gNpcColorSets_79[2] = {
#include "lobby/007FAD00.inc"
};
NpcColor gNpcColorSets_80[2] = {
#include "lobby/007FAD10.inc"
};
NpcColor gNpcColorSets_81[8] = {
#include "lobby/007FAD20.inc"
};
NpcColor gNpcColorSets_82[8] = {
#include "lobby/007FAD60.inc"
};
NpcColor gNpcColorSets_83[10] = {
#include "lobby/007FADA0.inc"
};
NpcColor gNpcColorSets_84[8] = {
#include "lobby/007FADF0.inc"
};
NpcColor gNpcColorSets_85[8] = {
#include "lobby/007FAE30.inc"
};
NpcColor gNpcColorSets_12[4] = {
#include "lobby/007FAE70.inc"
};
NpcColor gNpcColorSets_13[6] = {
#include "lobby/007FAE90.inc"
};
NpcColor gNpcColorSets_14[4] = {
#include "lobby/007FAEC0.inc"
};
NpcColor gNpcColorSets_15[6] = {
#include "lobby/007FAEE0.inc"
};
NpcColor gNpcColorSets_16[6] = {
#include "lobby/007FAF10.inc"
};
NpcColor gNpcColorSets_17[6] = {
#include "lobby/007FAF40.inc"
};
NpcColor gNpcColorSets_18[6] = {
#include "lobby/007FAF70.inc"
};
NpcColor gNpcColorSets_19[6] = {
#include "lobby/007FAFA0.inc"
};
NpcColor gNpcColorSets_20[6] = {
#include "lobby/007FAFD0.inc"
};
NpcColor gNpcColorSets_21[4] = {
#include "lobby/007FB000.inc"
};
NpcColor gNpcColorSets_22[6] = {
#include "lobby/007FB020.inc"
};
NpcColor gNpcColorSets_23[6] = {
#include "lobby/007FB050.inc"
};
NpcColor gNpcColorSets_24[6] = {
#include "lobby/007FB080.inc"
};
NpcColor gNpcColorSets_25[4] = {
#include "lobby/007FB0B0.inc"
};
NpcColor gNpcColorSets_26[6] = {
#include "lobby/007FB0D0.inc"
};
NpcColor gNpcColorSets_27[6] = {
#include "lobby/007FB100.inc"
};
NpcColor gNpcColorSets_28[6] = {
#include "lobby/007FB130.inc"
};
NpcColor gNpcColorSets_29[6] = {
#include "lobby/007FB160.inc"
};
NpcColor gNpcColorSets_30[6] = {
#include "lobby/007FB190.inc"
};
NpcColor gNpcColorSets_31[6] = {
#include "lobby/007FB1C0.inc"
};
NpcColor gNpcColorSets_32[6] = {
#include "lobby/007FB1F0.inc"
};
NpcColor gNpcColorSets_33[6] = {
#include "lobby/007FB220.inc"
};
NpcColor gNpcColorSets_34[4] = {
#include "lobby/007FB250.inc"
};
NpcColor gNpcColorSets_35[4] = {
#include "lobby/007FB270.inc"
};
NpcColor gNpcColorSets_36[4] = {
#include "lobby/007FB290.inc"
};
NpcColor gNpcColorSets_37[4] = {
#include "lobby/007FB2B0.inc"
};
NpcColor gNpcColorSets_38[4] = {
#include "lobby/007FB2D0.inc"
};
NpcColor gNpcColorSets_39[4] = {
#include "lobby/007FB2F0.inc"
};
NpcColor gNpcColorSets_40[6] = {
#include "lobby/007FB310.inc"
};
NpcColor gNpcColorSets_41[2] = {
#include "lobby/007FB340.inc"
};
NpcColor gNpcColorSets_42[4] = {
#include "lobby/007FB350.inc"
};
NpcColor gNpcColorSets_43[4] = {
#include "lobby/007FB370.inc"
};
NpcColor gNpcColorSets_44[4] = {
#include "lobby/007FB390.inc"
};
NpcColor gNpcColorSets_45[4] = {
#include "lobby/007FB3B0.inc"
};
NpcColor gNpcColorSets_46[4] = {
#include "lobby/007FB3D0.inc"
};
NpcColor gNpcColorSets_47[4] = {
#include "lobby/007FB3F0.inc"
};
NpcColor gNpcColorSets_48[4] = {
#include "lobby/007FB410.inc"
};
NpcColor gNpcColorSets_49[4] = {
#include "lobby/007FB430.inc"
};
NpcColor gNpcColorSets_50[4] = {
#include "lobby/007FB450.inc"
};
NpcColor gNpcColorSets_51[4] = {
#include "lobby/007FB470.inc"
};
NpcColor gNpcColorSets_52[4] = {
#include "lobby/007FB490.inc"
};
NpcColor gNpcColorSets_53[4] = {
#include "lobby/007FB4B0.inc"
};
NpcColor gNpcColorSets_54[4] = {
#include "lobby/007FB4D0.inc"
};
NpcColor gNpcColorSets_55[4] = {
#include "lobby/007FB4F0.inc"
};
NpcColor gNpcColorSets_56[8] = {
#include "lobby/007FB510.inc"
};
NpcColor gNpcColorSets_57[6] = {
#include "lobby/007FB550.inc"
};
NpcColor gNpcColorSets_58[6] = {
#include "lobby/007FB580.inc"
};
NpcColor gNpcColorSets_59[6] = {
#include "lobby/007FB5B0.inc"
};
NpcColor gNpcColorSets_60[4] = {
#include "lobby/007FB5E0.inc"
};
NpcColor gNpcColorSets_61[4] = {
#include "lobby/007FB600.inc"
};
NpcColor gNpcColorSets_62[4] = {
#include "lobby/007FB620.inc"
};
NpcColor gNpcColorSets_63[2] = {
#include "lobby/007FB640.inc"
};
NpcColor gNpcColorSets_64[6] = {
#include "lobby/007FB650.inc"
};
NpcColor gNpcColorSets_75[4] = {
#include "lobby/007FB680.inc"
};
NpcColor gNpcColorSets_76[4] = {
#include "lobby/007FB6A0.inc"
};
NpcColor gNpcColorSets_77[2] = {
#include "lobby/007FB6C0.inc"
};
NpcColor gNpcColorSets_78[4] = {
#include "lobby/007FB6D0.inc"
};
NpcColor *gNpcColorSets[88] = {
    gNpcColorSets_0, gNpcColorSets_1, gNpcColorSets_2, gNpcColorSets_3,
    gNpcColorSets_4, gNpcColorSets_5, gNpcColorSets_6, gNpcColorSets_7,
    gNpcColorSets_8, gNpcColorSets_9, gNpcColorSets_10, gNpcColorSets_11,
    gNpcColorSets_12, gNpcColorSets_13, gNpcColorSets_14, gNpcColorSets_15,
    gNpcColorSets_16, gNpcColorSets_17, gNpcColorSets_18, gNpcColorSets_19,
    gNpcColorSets_20, gNpcColorSets_21, gNpcColorSets_22, gNpcColorSets_23,
    gNpcColorSets_24, gNpcColorSets_25, gNpcColorSets_26, gNpcColorSets_27,
    gNpcColorSets_28, gNpcColorSets_29, gNpcColorSets_30, gNpcColorSets_31,
    gNpcColorSets_32, gNpcColorSets_33, gNpcColorSets_34, gNpcColorSets_35,
    gNpcColorSets_36, gNpcColorSets_37, gNpcColorSets_38, gNpcColorSets_39,
    gNpcColorSets_40, gNpcColorSets_41, gNpcColorSets_42, gNpcColorSets_43,
    gNpcColorSets_44, gNpcColorSets_45, gNpcColorSets_46, gNpcColorSets_47,
    gNpcColorSets_48, gNpcColorSets_49, gNpcColorSets_50, gNpcColorSets_51,
    gNpcColorSets_52, gNpcColorSets_53, gNpcColorSets_54, gNpcColorSets_55,
    gNpcColorSets_56, gNpcColorSets_57, gNpcColorSets_58, gNpcColorSets_59,
    gNpcColorSets_60, gNpcColorSets_61, gNpcColorSets_62, gNpcColorSets_63,
    gNpcColorSets_64, gNpcColorSets_65, gNpcColorSets_66, gNpcColorSets_67,
    gNpcColorSets_68, gNpcColorSets_69, gNpcColorSets_70, gNpcColorSets_71,
    gNpcColorSets_72, gNpcColorSets_73, gNpcColorSets_74, gNpcColorSets_75,
    gNpcColorSets_76, gNpcColorSets_77, gNpcColorSets_78, gNpcColorSets_79,
    gNpcColorSets_80, gNpcColorSets_81, gNpcColorSets_82, gNpcColorSets_83,
    gNpcColorSets_84, gNpcColorSets_85, NULL, NULL,
};
u32 gNpcKindSe[116] = {
#include "lobby/007FB850.inc"
};
u32 gAreaNpcTables_0[288] = {
#include "lobby/007FBA20.inc"
};
