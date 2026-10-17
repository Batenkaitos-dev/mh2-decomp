#include "common.h"
#include "lobby_npc.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
Em *EmSlot_Alloc(void);

s32 SongMenu_IsPlaying(void);
void Em_PlaySeSlot1(Em *em, s32 id, void *pos, s32 b);
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);
extern s32 gTownMode;
void Em_PlaySeSlot6(Em *em, s32 id, void *pos, s32 b);
void Npc_MotionSounds(Em *, NpcWork *);
s32 Npc_IsFree(Em *arg0);
u16 Rand_Next(s32 stream);
u8 Area_GetSetIndex(u16 id);
void Npc_SetupFromArea(Em *npc, u8 keep, s32 id);
s32 Npc_IsPresent(u8 arg0, u16 arg1, s32 arg2); /* unit's own declaration: with the definition's s32 (u8 kind, s32 id, u8 force) (lobby/unk_77A960.c), Npc_SpawnArea 2 of 56 words differ */
extern u8 *gAreaNpcSpawns[];

void Npc_SeOnFrame(Em *npc, s32 frame, s32 id)
{
    if ((SongMenu_IsPlaying() != 1) && (gTownMode != 0x2F) && (EmMotion_PassedFrameScaled((Player *) npc, (f32) frame, 0) != 0)) {
        Em_PlaySeSlot6(npc, id, &npc->posX, 2);
    }
}

void Npc_SeOnFrameB(Em *npc, s32 frame, s32 id)
{
    if ((SongMenu_IsPlaying() != 1) && (gTownMode != 0x2F) && (EmMotion_PassedFrameScaled((Player *) npc, (f32) frame, 0) != 0)) {
        Em_PlaySeSlot1(npc, id, &npc->posX, 2);
    }
}

void Npc_PlayVoice(Em *npc, f32 dist)
{
    NpcWork *m = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (SongMenu_IsPlaying() == 1 && pl->stateGroup == 0 && pl->state == 0x25) {
        return;
    }
    if (gTownMode != 0x2F && EmMotion_PassedFrameScaled((Player *) npc, dist, 0) != 0) {
        Em_PlaySeSlot6(npc, (s8)m->voice + ((u16)Rand_Next(1) & 1), &npc->posX, 2);
    }
}

s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
u8 Player_InCurrentArea(void *obj);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Vec_RotateSelf(f32 *v, f32 *m);
void Effect02_SpawnAt(f32 *pos, s32 a, u16 b);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Effect0D_SpawnAt(f32 *pos, s32 a, f32 scale);
void Actor_SetState(void *obj, s32 group, s32 state);

/* A townsperson's sounds and effects by its motion (+0x29C): sounds at
 * motion frames (Npc_SeOnFrame, Npc_SeOnFrameB, Npc_PlayVoice), dust
 * (Effect02_SpawnAt) and splashes (Effect0D_SpawnAt) when near and
 * visible; townsperson 1 (m->id) adds the random extra sounds. */
void Npc_MotionSounds(Em *npc, NpcWork *m)
{
    f32 v[3];

    switch (npc->motionId[0]) {
    case 1:
        break;
    case 0x2:
        Npc_PlayVoice(npc, 20.0f);
        Npc_PlayVoice(npc, 54.0f);
        Npc_PlayVoice(npc, 88.0f);
        break;
    case 0x3:
        Npc_SeOnFrame(npc, 0x18, 0x22);
        Npc_SeOnFrame(npc, 0x30, 0x22);
        Npc_SeOnFrame(npc, 0x48, 0x22);
        if (m->id == 1) {
            Npc_SeOnFrame(npc, 0x46, Sound_RandomSe(0xB8, 1, 0xB8, 1));
            break;
        }
        break;
    case 0x40:
        Npc_PlayVoice(npc, 26.0f);
        Npc_PlayVoice(npc, 60.0f);
        break;
    case 0x19F:
        Npc_SeOnFrameB(npc, 8, 0x3D);
        Npc_SeOnFrameB(npc, 0x1A, 0x3F);
        Npc_SeOnFrameB(npc, 0x2A, 0x2F);
        Npc_PlayVoice(npc, 8.0f);
        Npc_PlayVoice(npc, 38.0f);
        Npc_PlayVoice(npc, 78.0f);
        Npc_PlayVoice(npc, 92.0f);
        Npc_PlayVoice(npc, 136.0f);
        Npc_PlayVoice(npc, 162.0f);
        if ((EmMotion_PassedFrameScaled((Player *) npc, 50.0f, 0) != 0) && ((Player_InCurrentArea((Player *) npc) & 0xFF) == 1)) {
            Vec_Set(v, 0.0f, 0.0f, 120.0f);
            Vec_RotateSelf((f32 *) v, &npc->worldMtx[0][0]);
            v[0] += npc->posX;
            v[1] += npc->posY;
            v[2] += npc->posZ;
            Effect02_SpawnAt(v, 7, (u16)npc->rotY);
            break;
        }
        break;
    case 0x263:
        Npc_SeOnFrameB(npc, 0x2A, 0x51);
        Npc_SeOnFrameB(npc, 0x6C, 0x53);
        break;
    case 0x264:
        Npc_SeOnFrameB(npc, 0x7C, 0x4B);
        Npc_SeOnFrameB(npc, 0x2A, 0x3D);
        Npc_SeOnFrameB(npc, 0x38, 0x52);
        Npc_SeOnFrameB(npc, 0x80, 0x53);
        break;
    case 0x273:
        Npc_PlayVoice(npc, 16.0f);
        Npc_PlayVoice(npc, 32.0f);
        Npc_SeOnFrameB(npc, 0x1E, 0x31);
        Npc_SeOnFrameB(npc, 0x2A, 0x31);
        break;
    case 0x292:
        if (m->id == 1) {
            Npc_SeOnFrame(npc, 0xB4, Sound_RandomSe(0xB4, 1, 0xB5, 1));
            break;
        }
        break;
    case 0x2A4:
        Npc_SeOnFrame(npc, 0x18, 0x14);
        Npc_SeOnFrame(npc, 0x3C, 0x15);
        break;
    case 0x2A7:
        Npc_SeOnFrame(npc, 0x28, 0xA9);
        Npc_SeOnFrameB(npc, 0x26, 0x43);
        Npc_SeOnFrame(npc, 0x1E, 0x1E);
        Npc_SeOnFrame(npc, 0x3C, 0x1F);
        Npc_SeOnFrame(npc, 0x6E, 0x1E);
        break;
    case 0x2AD:
        Npc_SeOnFrameB(npc, 4, 0x4A);
        Npc_SeOnFrame(npc, 0x24, 0x3D);
        break;
    case 0x2B3:
        Npc_SeOnFrameB(npc, 0x2A, 0x31);
        Npc_SeOnFrameB(npc, 0x40, 0x31);
        Npc_SeOnFrameB(npc, 0xF4, 0x31);
        Npc_SeOnFrameB(npc, 0x108, 0x31);
        Npc_SeOnFrameB(npc, 0x132, 0x31);
        Npc_SeOnFrameB(npc, 0x142, 0x31);
        Npc_SeOnFrame(npc, 0x50, Sound_RandomSe(0x3E, 2, 0x3E, 2));
        Npc_SeOnFrame(npc, 0x190, Sound_RandomSe(0x3F, 2, 0x3F, 1));
        break;
    case 0x2B6:
        Npc_SeOnFrameB(npc, 0x22, 0x4B);
        Npc_SeOnFrameB(npc, 0x38, 0x4B);
        Npc_SeOnFrameB(npc, 0x52, 0x4B);
        break;
    case 0xFE:
        Npc_SeOnFrame(npc, 0x42, 0xB6);
        break;
    case 0xFF:
        Npc_SeOnFrame(npc, 0x10, 0xB3);
        Npc_SeOnFrame(npc, 0x6E, 0xB4);
        Npc_SeOnFrame(npc, 0xB4, 0xB5);
        break;
    case 0x100:
        Npc_SeOnFrame(npc, 0x10, Sound_RandomSe(0xB7, 4, 0xB7, 4));
        break;
    case 0x352:
        Npc_SeOnFrame(npc, 6, 0xA7);
        Npc_SeOnFrame(npc, 0xA, 0x67);
        Npc_SeOnFrame(npc, 0x24, 0x66);
        break;
    case 0x353:
        Npc_SeOnFrame(npc, 8, 0x65);
        break;
    case 0x354:
        Npc_SeOnFrame(npc, 6, 0xA7);
        Npc_SeOnFrame(npc, 0xA, 0x67);
        Npc_SeOnFrame(npc, 0x24, 0x66);
        break;
    case 0x355:
        Npc_SeOnFrame(npc, 8, 0x65);
        break;
    case 0x357:
        Npc_SeOnFrame(npc, 0x6E, 0x63);
        Npc_SeOnFrame(npc, 0xC6, 0x64);
        Npc_SeOnFrame(npc, 0xDA, 0x5C);
        if ((EmMotion_PassedFrameScaled((Player *) npc, 220.0f, 0) != 0) && ((Player_InCurrentArea((Player *) npc) & 0xFF) == 1)) {
            Vec_Set(v, 5.0f, 20.0f, 166.0f);
            Vec_RotateSelf((f32 *) v, &npc->worldMtx[0][0]);
            v[0] += npc->posX;
            v[1] += npc->posY;
            v[2] += npc->posZ;
            Effect02_SpawnAt(v, 7, (u16)npc->rotY);
            break;
        }
        break;
    case 0x35D:
        Npc_SeOnFrameB(npc, 0x14, 0x3D);
        Npc_SeOnFrame(npc, 0x18, 0xCE);
        Npc_SeOnFrameB(npc, 0x50, 0x3E);
        Npc_SeOnFrame(npc, 0xD4, 0xE0);
        break;
    case 0x35E:
        Npc_SeOnFrameB(npc, 0x38, 0x4B);
        Npc_SeOnFrameB(npc, 0x6E, 0x51);
        Npc_SeOnFrame(npc, 0xE8, 0xE0);
        break;
    case 0x35F:
        Npc_SeOnFrameB(npc, 0x2C, 0x4A);
        Npc_SeOnFrame(npc, 0x64, 0xCD);
        break;
    case 0x360:
        Npc_SeOnFrame(npc, 4, 0xE0);
        Npc_SeOnFrameB(npc, 0x34, 0x3D);
        Npc_SeOnFrame(npc, 0x86, 0xCC);
        Npc_SeOnFrame(npc, 0x10C, 0xCF);
        break;
    case 0x361:
        Npc_SeOnFrame(npc, 0x14, 0x14);
        Npc_SeOnFrame(npc, 0x3C, 0x15);
        break;
    case 0x365:
        Npc_SeOnFrameB(npc, 0xE, 0x43);
        Npc_SeOnFrame(npc, 0x10, 0x84);
        Npc_SeOnFrameB(npc, 0x16, 7);
        break;
    case 0x366:
        Npc_SeOnFrameB(npc, 0x36, 7);
        Npc_SeOnFrame(npc, 0x5C, 0x23);
        break;
    case 0x367:
        Npc_SeOnFrame(npc, 0x12, 0x84);
        Npc_SeOnFrame(npc, 0x14, 0x23);
        Npc_SeOnFrameB(npc, 0x18, 7);
        break;
    case 0x368:
        Npc_SeOnFrame(npc, 0x14, 0x22);
        Npc_SeOnFrame(npc, 0x3A, 0x23);
        break;
    case 0x369:
        Npc_SeOnFrame(npc, 0x10, 0x22);
        Npc_SeOnFrame(npc, 0x2C, 0x23);
        break;
    case 0x36A:
        Npc_SeOnFrame(npc, 8, 0x22);
        Npc_SeOnFrame(npc, 0x24, 0x23);
        Npc_SeOnFrame(npc, 0x44, 0x23);
        break;
    case 0x36B:
        Npc_SeOnFrame(npc, 0xD2, Sound_RandomSe(0xB9, 2, 0xBA, 1));
        break;
    case 0x36F:
        Npc_SeOnFrame(npc, 0x10, 0x20);
        Npc_SeOnFrame(npc, 0x32, 0x21);
        Npc_SeOnFrame(npc, 0x5A, 0x20);
        break;
    case 0x372:
        Npc_SeOnFrame(npc, 0x14, 0x73);
        break;
    case 0x373:
        Npc_SeOnFrame(npc, 0xC, 0x70);
        Npc_SeOnFrameB(npc, 0x6C, 0x53);
        Npc_SeOnFrameB(npc, 0x6E, 0x53);
        Npc_SeOnFrameB(npc, 0x7E, 0x53);
        Npc_SeOnFrameB(npc, 0xE6, 0x63);
        Npc_SeOnFrameB(npc, 0x118, 0x63);
        break;
    case 0x374:
        Npc_SeOnFrame(npc, 0x1E, 0x71);
        Npc_SeOnFrameB(npc, 0x5E, 0x53);
        Npc_SeOnFrameB(npc, 0xB4, 0x43);
        break;
    case 0x375:
        Npc_SeOnFrame(npc, 0x1A, 0x72);
        Npc_SeOnFrameB(npc, 0x20, 0x31);
        Npc_SeOnFrame(npc, 0x36, 0x70);
        Npc_SeOnFrameB(npc, 0x3C, 0x3F);
        Npc_SeOnFrameB(npc, 0x6C, 0x3E);
        Npc_SeOnFrameB(npc, 0x82, 0x3F);
        Npc_SeOnFrame(npc, 0xAC, 0x74);
        Npc_SeOnFrameB(npc, 0xAC, 0x53);
        Npc_SeOnFrameB(npc, 0xBE, 0x63);
        Npc_SeOnFrameB(npc, 0xEA, 0x63);
        Npc_SeOnFrameB(npc, 0xDE, 0x37);
        break;
    case 0x376:
        Npc_SeOnFrameB(npc, 0x12, 0x2D);
        Npc_SeOnFrame(npc, 0x4E, 0x73);
        Npc_SeOnFrame(npc, 0x62, 0x74);
        Npc_SeOnFrameB(npc, 0x8E, 0x53);
        Npc_SeOnFrameB(npc, 0xA8, 0x53);
        Npc_SeOnFrameB(npc, 0xC2, 0x2D);
        Npc_SeOnFrame(npc, 0xE6, 0x75);
        Npc_SeOnFrameB(npc, 0x140, 0x2D);
        Npc_SeOnFrame(npc, 0x1A6, 0x76);
        Npc_SeOnFrame(npc, 0x1CA, 0x72);
        Npc_SeOnFrame(npc, 0x24A, 0x74);
        Npc_SeOnFrame(npc, 0x27C, 0x75);
        break;
    case 0x377:
        Npc_SeOnFrameB(npc, 2, 0x3D);
        Npc_SeOnFrameB(npc, 4, 0x3A);
        Npc_SeOnFrameB(npc, 0x56, 0x4A);
        Npc_SeOnFrame(npc, 0x64, 0x72);
        Npc_SeOnFrameB(npc, 0xC8, 0x64);
        Npc_SeOnFrameB(npc, 0xE2, 0x31);
        Npc_SeOnFrameB(npc, 0x10A, 0x31);
        Npc_SeOnFrameB(npc, 0x132, 0x31);
        Npc_SeOnFrame(npc, 0x172, 0x77);
        break;
    case 0x378:
        Npc_SeOnFrameB(npc, 2, 0x2D);
        Npc_SeOnFrame(npc, 0x52, 0x79);
        Npc_SeOnFrameB(npc, 0xD2, 0x3F);
        Npc_SeOnFrameB(npc, 0x11E, 0x42);
        Npc_SeOnFrame(npc, 0xCE, 0x78);
        Npc_SeOnFrameB(npc, 0x24C, 0x3A);
        Npc_SeOnFrameB(npc, 0x26C, 0x3A);
        Npc_SeOnFrameB(npc, 0x2BE, 0x3A);
        Npc_SeOnFrameB(npc, 0x2D2, 0x3A);
        Npc_SeOnFrameB(npc, 0x2E2, 0x3A);
        Npc_SeOnFrameB(npc, 0x302, 0x3A);
        Npc_SeOnFrameB(npc, 0x342, 0x2D);
        if ((EmMotion_PassedFrameScaled((Player *) npc, 290.0f, 0) != 0) && (Player_InCurrentArea((Player *) npc) & 0xFF)) {
            Em_BonePos(npc, 2, v);
            v[1] = npc->floorY;
            Effect0D_SpawnAt(v, 7, 1.0f);
        }
        break;
    }
}

void Npc_RunMotionSounds(Em *npc)
{
    s8 step;
    NpcWork *m;

    m = (NpcWork *)&npc->work;
    step = m->fxStep;
    switch (step) {
    case 0:
        m->fxStep = (s8) (step + 1);
        break;
    case 1:
        Npc_MotionSounds(npc, m);
        break;
    }
}

typedef struct V0077F660_0 {
    u8 f0;
    u8 pad1[0x47];
} V0077F660_0;
extern V0077F660_0 *gAreaNpcTables[];
extern u8 gAreaNpcCounts[];

void Npc_Spawn(s32 kind)
{
    s32 area = Area_GetSetIndex(gScene.area);
    V0077F660_0 *e = gAreaNpcTables[area];
    s32 i;
    Em *em;

    for (i = 0; i < gAreaNpcCounts[area]; i++) {
        if (kind == e->f0) {
            em = EmSlot_Alloc();
            if (em != NULL) {
                em->ownTask = 1;
                em->modelSlot = 0;
                em->index = i;
                em->area = gScene.area;
            }
            Npc_SetupFromArea(em, 1, gScene.area);
            return;
        }
        e++;
    }
}

s32 Npc_Exists(u8 kind)
{
    Em *em;
    s32 i;

    if (kind == 0xFF) {
        return 0;
    }
    em = gEms;
    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && em->work == kind) {
            return 1;
        }
    }
    return 0;
}

Em *Npc_Find(u8 kind)
{
    Em *em = gEms;
    s32 i;

    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && em->work == kind) {
            return em;
        }
    }
    return NULL;
}

void Npc_SpawnArea(void)
{
    u8 *p = gAreaNpcSpawns[Area_GetSetIndex(gScene.area)];

    if (p == NULL) {
        return;
    }
    while (p[0] != 0xFF) {
        if (Npc_Exists(p[0]) == 0 && Npc_Exists(p[1]) == 0) {
            if (Npc_IsPresent(p[0], gScene.area, 0) == 1) {
                Npc_Spawn(p[0]);
            } else if (Npc_IsPresent(p[1], gScene.area, 0) == 1) {
                Npc_Spawn(p[1]);
            }
        }
        p += 2;
    }
}

extern NpcRouteEnt gNpcRouteToHairChair[];
extern NpcRouteEnt gNpcRouteFromHairChair[];
typedef struct V0077F8E0_0 {
    u8 pad0[0xC];
    u16 fC;
    u8 padE[6];
} V0077F8E0_0;

void Npc_SetRouteToHairChair(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    m->route = gNpcRouteToHairChair;
    m->routeIdx = 0;
    Actor_SetState(npc, 0, (u16)m->route[m->routeIdx].state);
}

void Npc_SetRouteFromHairChair(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    m->route = gNpcRouteFromHairChair;
    m->routeIdx = 0;
    Actor_SetState(npc, 0, (u16)m->route[m->routeIdx].state);
}

s32 Npc_IsFreeAct2Bto30(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (m->activity) {
        case 0:
        case 0x2B:
        case 0x2E:
        case 0x30:
        case 0x2C:
        case 0x2D:
        case 0x2F:
            return 1;
        }
    }
    return 0;
}

s32 Npc_IsFreeAct15to19(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (m->activity) {
        case 4:
        case 0x15:
        case 0x17:
        case 0x19:
        case 0x16:
            return 1;
        }
    }
    return 0;
}

s32 Npc_IsFreeAct18Set(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (m->activity) {
        case 0x15:
        case 0x18:
        case 0x1A:
        case 0x22:
        case 0x25:
        case 0:
        case 1:
        case 2:
            return 1;
        }
    }
    return 0;
}

s32 Npc_IsBusyNotAct3C4B(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    switch (m->busy) {
    case 1:
        switch (m->activity) {
        case 0x4B:
        case 0x4F:
        case 0x50:
        case 0x3C:
        case 0x3D:
        case 0x4C:
        case 0x4E:
        case 0x4D:
            return 0;
        }
        return 1;
    }
    return 0;
}

s32 Npc_IsFreeId0Aor2E(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (m->id) {
        case 0x2E:
        case 0xA:
            return 1;
        }
    }
    return 0;
}

s32 Npc_IsBusyId0E(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    if (m->busy == 1 && m->id == 0xE) {
        return 1;
    }
    return 0;
}

s32 Npc_IsFreeAct29or2A(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (m->activity) {
        case 0x29:
        case 0x2A:
            return 1;
        }
    }
    return 0;
}

s32 Npc_IsFree(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

    switch (npc->npcAct) {
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
        return 0;
    }
    if (m->busy == 1 && npc->variant != 0x3D) {
        return 0;
    }
    return 1;
}

s32 Npc_IsAct51to55(Em *npc)
{
    switch (npc->npcAct) {
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
        return 1;
    }
    return 0;
}

/* unk_77E540's .data (0x7FBEA0-0x7FE490) */
extern u32 gNpcHandlers_3[];
extern u32 gNpcHandlers_2[];
extern u32 gNpcHandlers_1[];
extern u32 gAreaNpcTables_0[];
void func_0077E520(void);
void func_0077E530(void);
extern void *gNpcHandlers_0[];
extern void *gNpcHandlers_4[];
u8 gAreaNpcSpawns_0[16] = {
#include "lobby/007FBEA0.inc"
};
V0077F660_0 gAreaNpcTables_1[11] = {
#include "lobby/007FBEB0.inc"
};
u8 gAreaNpcSpawns_1[8] = {
#include "lobby/007FC1C8.inc"
};
V0077F660_0 gAreaNpcTables_11[6] = {
#include "lobby/007FC1D0.inc"
};
V0077F660_0 gAreaNpcTables_12[8] = {
#include "lobby/007FC380.inc"
};
V0077F660_0 gAreaNpcTables_13[14] = {
#include "lobby/007FC5C0.inc"
};
V0077F660_0 gAreaNpcTables_2[8] = {
#include "lobby/007FC9B0.inc"
};
V0077F660_0 gAreaNpcTables_3[9] = {
#include "lobby/007FCBF0.inc"
};
u32 D_007FCE80[12] = {
#include "lobby/007FCE80.inc"
};
V0077F660_0 gAreaNpcTables_4[11] = {
#include "lobby/007FCEB0.inc"
};
u8 gAreaNpcSpawns_4[12] = {
#include "lobby/007FD1C8.inc"
};
V0077F660_0 gAreaNpcTables_5[8] = {
#include "lobby/007FD1E0.inc"
};
V0077F660_0 gAreaNpcTables_6[25] = {
#include "lobby/007FD420.inc"
};
u32 gNpcSideRouteLines[8] = {
#include "lobby/007FDB30.inc"
};
u8 gAreaNpcSpawns_6[16] = {
#include "lobby/007FDB50.inc"
};
u32 gNpcAboveLines[16] = {
#include "lobby/007FDB60.inc"
};
u32 gNpcPartnerPairs[4] = {
#include "lobby/007FDBA0.inc"
};
V0077F660_0 gAreaNpcTables_7 = {
#include "lobby/007FDBB0.inc"
};
V0077F660_0 gAreaNpcTables_14[2] = {
#include "lobby/007FDC00.inc"
};
V0077F660_0 gAreaNpcTables_10[19] = {
#include "lobby/007FDC90.inc"
};
u8 *gAreaNpcSpawns[16] = {
    gAreaNpcSpawns_0, gAreaNpcSpawns_1, NULL, NULL,
    gAreaNpcSpawns_4, NULL, gAreaNpcSpawns_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
V0077F660_0 *gAreaNpcTables[16] = {
    (V0077F660_0 *)gAreaNpcTables_0, gAreaNpcTables_1, gAreaNpcTables_2, gAreaNpcTables_3,
    gAreaNpcTables_4, gAreaNpcTables_5, gAreaNpcTables_6, &gAreaNpcTables_7,
    &gAreaNpcTables_7, &gAreaNpcTables_7, gAreaNpcTables_10, gAreaNpcTables_11,
    gAreaNpcTables_12, gAreaNpcTables_13, gAreaNpcTables_14, NULL,
};
u8 gAreaNpcCounts[16] = {
#include "lobby/007FE270.inc"
};
u32 gAreaFiles[16] = {
#include "lobby/007FE280.inc"
};
u32 gTownAreaSets_0[8] = {
#include "lobby/007FE2C0.inc"
};
u32 gTownAreaSets_1[8] = {
#include "lobby/007FE2E0.inc"
};
u32 gTownAreaSets_2[8] = {
#include "lobby/007FE300.inc"
};
u32 gTownAreaSets_3[8] = {
#include "lobby/007FE320.inc"
};
u32 gTownAreaSets_4[8] = {
#include "lobby/007FE340.inc"
};
u32 gTownAreaSets_5[8] = {
#include "lobby/007FE360.inc"
};
u32 gTownAreaSets_6[8] = {
#include "lobby/007FE380.inc"
};
u32 gTownAreaSets_10[8] = {
#include "lobby/007FE3A0.inc"
};
u32 gTownAreaSets_11[8] = {
#include "lobby/007FE3C0.inc"
};
u32 gTownAreaSets[16] = {
    (u32)gTownAreaSets_0, (u32)gTownAreaSets_1, (u32)gTownAreaSets_2, (u32)gTownAreaSets_3,
    (u32)gTownAreaSets_4, (u32)gTownAreaSets_5, (u32)gTownAreaSets_6, (u32)gTownAreaSets_6,
    (u32)gTownAreaSets_6, (u32)gTownAreaSets_6, (u32)gTownAreaSets_10, (u32)gTownAreaSets_11,
    (u32)gTownAreaSets_11, (u32)gTownAreaSets_11, (u32)gTownAreaSets_11, 0,
};
u32 gNpcHandlers[12] = {
    (u32)gNpcHandlers_0, (u32)gNpcHandlers_1, (u32)gNpcHandlers_2, (u32)gNpcHandlers_3,
    (u32)gNpcHandlers_4, (u32)gNpcHandlers_4, (u32)gNpcHandlers_4, (u32)gNpcHandlers_4,
    (u32)gNpcHandlers_4, (u32)gNpcHandlers_4, 0, 0,
};
void *gNpcHandlers_4[8] = {
    func_0077E520, func_0077E520, func_0077E530, func_0077E530,
    func_0077E530, NULL, NULL, NULL,
};
void *gNpcHandlers_0[8] = {
    func_0077E520, func_0077E520, func_0077E530, Npc_RunMotionSounds,
    func_0077E530, NULL, NULL, NULL,
};
