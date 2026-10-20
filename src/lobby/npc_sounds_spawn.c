/*
 * The townspeople's motion sounds and their placing. Npc_MotionSounds plays a
 * townsperson's sounds and effects at frames of its motion (Npc_PlayTownSeAt for the
 * town's sound bank, Npc_PlayCommonSeAt for the shared bank, Npc_PlayVoice for its
 * voice), run each frame from its handler table (Npc_RunMotionSounds). Npc_Spawn
 * and Npc_SpawnArea place the current area's townspeople from the per-area tables
 * (gAreaNpcTables, gAreaNpcCounts, gAreaNpcSpawns), Npc_Exists and Npc_Find look
 * one up by id. The data holds the area tables, the route lines, the town's area
 * sets and the townspeople's handler tables (gNpcHandlers). The townsperson's
 * task and set-up come just before.
 */
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
#include "lobby_menu.h"
Em *EmSlot_Alloc(void);

s32 SongMenu_IsPlaying(void);
void Em_PlayCommonSe(Em *em, s32 id, void *pos, s32 b);
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);
extern s32 gTownMode;
void Em_PlayEmBankSe(Em *em, s32 id, void *pos, s32 b);
void Npc_MotionSounds(Em *, NpcWork *);
u16 Rand_Next(s32 stream);
u8 Area_GetSetIndex(u16 id);
void Npc_SetupFromArea(Em *npc, u8 keep, s32 id);
/* unit's own declaration: the definition takes (u8 kind, s32 area, u8 force); with those types the compiler converts Npc_SpawnArea's arguments differently from the game's code */
s32 Npc_IsPresent(u8 kind, u16 area, s32 force);
extern u8 *gAreaNpcSpawns[];

/* Plays sound id of the town's own bank (loaded into the monsters' slot,
 * SE_SLOT_EM) at the townsperson when its layer-0 motion passes frame; silent
 * while the song menu plays or the shooting gallery runs. */
void Npc_PlayTownSeAt(Em *npc, s32 frame, s32 id)
{
    if ((SongMenu_IsPlaying() != 1) && (gTownMode != TOWN_MODE_GALLERY) && (EmMotion_PassedFrameScaled((Player *) npc, (f32) frame, 0) != 0)) {
        Em_PlayEmBankSe(npc, id, &npc->posX, 2);
    }
}

/* Npc_PlayTownSeAt with a sound of the shared bank (SE_SLOT_COMMON). */
void Npc_PlayCommonSeAt(Em *npc, s32 frame, s32 id)
{
    if ((SongMenu_IsPlaying() != 1) && (gTownMode != TOWN_MODE_GALLERY) && (EmMotion_PassedFrameScaled((Player *) npc, (f32) frame, 0) != 0)) {
        Em_PlayCommonSe(npc, id, &npc->posX, 2);
    }
}

/* Plays the townsperson's voice sound (NpcWork.voice, or the one after it, at
 * random) from the town's bank when its layer-0 motion passes frame dist; not
 * while the shooting gallery runs, nor while the song menu plays with the local
 * player seated at a table. */
void Npc_PlayVoice(Em *npc, f32 frame)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (SongMenu_IsPlaying() == 1 && pl->stateGroup == TOWN_PC_GROUP_ACTION && pl->state == TOWN_PC_SIT_DOWN_SEATED) {
        return;
    }
    if (gTownMode != TOWN_MODE_GALLERY && EmMotion_PassedFrameScaled((Player *) npc, frame, 0) != 0) {
        Em_PlayEmBankSe(npc, (s8)npcWork->voice + ((u16)Rand_Next(1) & 1), &npc->posX, 2);
    }
}

s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
u8 Player_InCurrentArea(void *obj);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Vec_RotateSelf(f32 *v, f32 *m);
void HitSpark_SpawnAt(f32 *pos, s32 a, u16 b);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void PartFx_SpawnAt(f32 *pos, s32 a, f32 scale);
void Actor_SetState(void *obj, s32 group, s32 state);

/* A townsperson's sounds and effects by its layer-0 motion (Em.motionId[0]):
 * sounds at motion frames (Npc_PlayTownSeAt, Npc_PlayCommonSeAt, Npc_PlayVoice; some
 * picked at random by Sound_RandomSe), sparks in front of it (HitSpark_SpawnAt:
 * the blows of motion 0x19F and of 0x352-0x357) and an effect on the floor
 * (PartFx_SpawnAt), these two only while it is in the player's area.
 * Townsperson 1 (NpcWork.id) adds random sounds to its run and to motion 0x292. */
void Npc_MotionSounds(Em *npc, NpcWork *npcWork)
{
    f32 pos[3];

    switch (npc->motionId[0]) {
    case 1:
        break;
    case 0x2:
        Npc_PlayVoice(npc, 20.0f);
        Npc_PlayVoice(npc, 54.0f);
        Npc_PlayVoice(npc, 88.0f);
        break;
    case 0x3:
        Npc_PlayTownSeAt(npc, 0x18, 0x22);
        Npc_PlayTownSeAt(npc, 0x30, 0x22);
        Npc_PlayTownSeAt(npc, 0x48, 0x22);
        if (npcWork->id == 1) {
            Npc_PlayTownSeAt(npc, 0x46, Sound_RandomSe(0xB8, 1, 0xB8, 1));
            break;
        }
        break;
    case 0x40:
        Npc_PlayVoice(npc, 26.0f);
        Npc_PlayVoice(npc, 60.0f);
        break;
    case 0x19F:
        Npc_PlayCommonSeAt(npc, 8, 0x3D);
        Npc_PlayCommonSeAt(npc, 0x1A, 0x3F);
        Npc_PlayCommonSeAt(npc, 0x2A, 0x2F);
        Npc_PlayVoice(npc, 8.0f);
        Npc_PlayVoice(npc, 38.0f);
        Npc_PlayVoice(npc, 78.0f);
        Npc_PlayVoice(npc, 92.0f);
        Npc_PlayVoice(npc, 136.0f);
        Npc_PlayVoice(npc, 162.0f);
        if ((EmMotion_PassedFrameScaled((Player *) npc, 50.0f, 0) != 0) && ((Player_InCurrentArea((Player *) npc) & 0xFF) == 1)) {
            Vec_Set(pos, 0.0f, 0.0f, 120.0f);
            Vec_RotateSelf((f32 *) pos, &npc->worldMtx[0][0]);
            pos[0] += npc->posX;
            pos[1] += npc->posY;
            pos[2] += npc->posZ;
            HitSpark_SpawnAt(pos, 7, (u16)npc->rotY);
            break;
        }
        break;
    case 0x263:
        Npc_PlayCommonSeAt(npc, 0x2A, 0x51);
        Npc_PlayCommonSeAt(npc, 0x6C, 0x53);
        break;
    case 0x264:
        Npc_PlayCommonSeAt(npc, 0x7C, 0x4B);
        Npc_PlayCommonSeAt(npc, 0x2A, 0x3D);
        Npc_PlayCommonSeAt(npc, 0x38, 0x52);
        Npc_PlayCommonSeAt(npc, 0x80, 0x53);
        break;
    case 0x273:
        Npc_PlayVoice(npc, 16.0f);
        Npc_PlayVoice(npc, 32.0f);
        Npc_PlayCommonSeAt(npc, 0x1E, 0x31);
        Npc_PlayCommonSeAt(npc, 0x2A, 0x31);
        break;
    case 0x292:
        if (npcWork->id == 1) {
            Npc_PlayTownSeAt(npc, 0xB4, Sound_RandomSe(0xB4, 1, 0xB5, 1));
            break;
        }
        break;
    case 0x2A4:
        Npc_PlayTownSeAt(npc, 0x18, 0x14);
        Npc_PlayTownSeAt(npc, 0x3C, 0x15);
        break;
    case 0x2A7:
        Npc_PlayTownSeAt(npc, 0x28, 0xA9);
        Npc_PlayCommonSeAt(npc, 0x26, 0x43);
        Npc_PlayTownSeAt(npc, 0x1E, 0x1E);
        Npc_PlayTownSeAt(npc, 0x3C, 0x1F);
        Npc_PlayTownSeAt(npc, 0x6E, 0x1E);
        break;
    case 0x2AD:
        Npc_PlayCommonSeAt(npc, 4, 0x4A);
        Npc_PlayTownSeAt(npc, 0x24, 0x3D);
        break;
    case 0x2B3:
        Npc_PlayCommonSeAt(npc, 0x2A, 0x31);
        Npc_PlayCommonSeAt(npc, 0x40, 0x31);
        Npc_PlayCommonSeAt(npc, 0xF4, 0x31);
        Npc_PlayCommonSeAt(npc, 0x108, 0x31);
        Npc_PlayCommonSeAt(npc, 0x132, 0x31);
        Npc_PlayCommonSeAt(npc, 0x142, 0x31);
        Npc_PlayTownSeAt(npc, 0x50, Sound_RandomSe(0x3E, 2, 0x3E, 2));
        Npc_PlayTownSeAt(npc, 0x190, Sound_RandomSe(0x3F, 2, 0x3F, 1));
        break;
    case 0x2B6:
        Npc_PlayCommonSeAt(npc, 0x22, 0x4B);
        Npc_PlayCommonSeAt(npc, 0x38, 0x4B);
        Npc_PlayCommonSeAt(npc, 0x52, 0x4B);
        break;
    case 0xFE:
        Npc_PlayTownSeAt(npc, 0x42, 0xB6);
        break;
    case 0xFF:
        Npc_PlayTownSeAt(npc, 0x10, 0xB3);
        Npc_PlayTownSeAt(npc, 0x6E, 0xB4);
        Npc_PlayTownSeAt(npc, 0xB4, 0xB5);
        break;
    case 0x100:
        Npc_PlayTownSeAt(npc, 0x10, Sound_RandomSe(0xB7, 4, 0xB7, 4));
        break;
    case 0x352:
        Npc_PlayTownSeAt(npc, 6, 0xA7);
        Npc_PlayTownSeAt(npc, 0xA, 0x67);
        Npc_PlayTownSeAt(npc, 0x24, 0x66);
        break;
    case 0x353:
        Npc_PlayTownSeAt(npc, 8, 0x65);
        break;
    case 0x354:
        Npc_PlayTownSeAt(npc, 6, 0xA7);
        Npc_PlayTownSeAt(npc, 0xA, 0x67);
        Npc_PlayTownSeAt(npc, 0x24, 0x66);
        break;
    case 0x355:
        Npc_PlayTownSeAt(npc, 8, 0x65);
        break;
    case 0x357:
        Npc_PlayTownSeAt(npc, 0x6E, 0x63);
        Npc_PlayTownSeAt(npc, 0xC6, 0x64);
        Npc_PlayTownSeAt(npc, 0xDA, 0x5C);
        if ((EmMotion_PassedFrameScaled((Player *) npc, 220.0f, 0) != 0) && ((Player_InCurrentArea((Player *) npc) & 0xFF) == 1)) {
            Vec_Set(pos, 5.0f, 20.0f, 166.0f);
            Vec_RotateSelf((f32 *) pos, &npc->worldMtx[0][0]);
            pos[0] += npc->posX;
            pos[1] += npc->posY;
            pos[2] += npc->posZ;
            HitSpark_SpawnAt(pos, 7, (u16)npc->rotY);
            break;
        }
        break;
    case 0x35D:
        Npc_PlayCommonSeAt(npc, 0x14, 0x3D);
        Npc_PlayTownSeAt(npc, 0x18, 0xCE);
        Npc_PlayCommonSeAt(npc, 0x50, 0x3E);
        Npc_PlayTownSeAt(npc, 0xD4, 0xE0);
        break;
    case 0x35E:
        Npc_PlayCommonSeAt(npc, 0x38, 0x4B);
        Npc_PlayCommonSeAt(npc, 0x6E, 0x51);
        Npc_PlayTownSeAt(npc, 0xE8, 0xE0);
        break;
    case 0x35F:
        Npc_PlayCommonSeAt(npc, 0x2C, 0x4A);
        Npc_PlayTownSeAt(npc, 0x64, 0xCD);
        break;
    case 0x360:
        Npc_PlayTownSeAt(npc, 4, 0xE0);
        Npc_PlayCommonSeAt(npc, 0x34, 0x3D);
        Npc_PlayTownSeAt(npc, 0x86, 0xCC);
        Npc_PlayTownSeAt(npc, 0x10C, 0xCF);
        break;
    case 0x361:
        Npc_PlayTownSeAt(npc, 0x14, 0x14);
        Npc_PlayTownSeAt(npc, 0x3C, 0x15);
        break;
    case 0x365:
        Npc_PlayCommonSeAt(npc, 0xE, 0x43);
        Npc_PlayTownSeAt(npc, 0x10, 0x84);
        Npc_PlayCommonSeAt(npc, 0x16, 7);
        break;
    case 0x366:
        Npc_PlayCommonSeAt(npc, 0x36, 7);
        Npc_PlayTownSeAt(npc, 0x5C, 0x23);
        break;
    case 0x367:
        Npc_PlayTownSeAt(npc, 0x12, 0x84);
        Npc_PlayTownSeAt(npc, 0x14, 0x23);
        Npc_PlayCommonSeAt(npc, 0x18, 7);
        break;
    case 0x368:
        Npc_PlayTownSeAt(npc, 0x14, 0x22);
        Npc_PlayTownSeAt(npc, 0x3A, 0x23);
        break;
    case 0x369:
        Npc_PlayTownSeAt(npc, 0x10, 0x22);
        Npc_PlayTownSeAt(npc, 0x2C, 0x23);
        break;
    case 0x36A:
        Npc_PlayTownSeAt(npc, 8, 0x22);
        Npc_PlayTownSeAt(npc, 0x24, 0x23);
        Npc_PlayTownSeAt(npc, 0x44, 0x23);
        break;
    case 0x36B:
        Npc_PlayTownSeAt(npc, 0xD2, Sound_RandomSe(0xB9, 2, 0xBA, 1));
        break;
    case 0x36F:
        Npc_PlayTownSeAt(npc, 0x10, 0x20);
        Npc_PlayTownSeAt(npc, 0x32, 0x21);
        Npc_PlayTownSeAt(npc, 0x5A, 0x20);
        break;
    case 0x372:
        Npc_PlayTownSeAt(npc, 0x14, 0x73);
        break;
    case 0x373:
        Npc_PlayTownSeAt(npc, 0xC, 0x70);
        Npc_PlayCommonSeAt(npc, 0x6C, 0x53);
        Npc_PlayCommonSeAt(npc, 0x6E, 0x53);
        Npc_PlayCommonSeAt(npc, 0x7E, 0x53);
        Npc_PlayCommonSeAt(npc, 0xE6, 0x63);
        Npc_PlayCommonSeAt(npc, 0x118, 0x63);
        break;
    case 0x374:
        Npc_PlayTownSeAt(npc, 0x1E, 0x71);
        Npc_PlayCommonSeAt(npc, 0x5E, 0x53);
        Npc_PlayCommonSeAt(npc, 0xB4, 0x43);
        break;
    case 0x375:
        Npc_PlayTownSeAt(npc, 0x1A, 0x72);
        Npc_PlayCommonSeAt(npc, 0x20, 0x31);
        Npc_PlayTownSeAt(npc, 0x36, 0x70);
        Npc_PlayCommonSeAt(npc, 0x3C, 0x3F);
        Npc_PlayCommonSeAt(npc, 0x6C, 0x3E);
        Npc_PlayCommonSeAt(npc, 0x82, 0x3F);
        Npc_PlayTownSeAt(npc, 0xAC, 0x74);
        Npc_PlayCommonSeAt(npc, 0xAC, 0x53);
        Npc_PlayCommonSeAt(npc, 0xBE, 0x63);
        Npc_PlayCommonSeAt(npc, 0xEA, 0x63);
        Npc_PlayCommonSeAt(npc, 0xDE, 0x37);
        break;
    case 0x376:
        Npc_PlayCommonSeAt(npc, 0x12, 0x2D);
        Npc_PlayTownSeAt(npc, 0x4E, 0x73);
        Npc_PlayTownSeAt(npc, 0x62, 0x74);
        Npc_PlayCommonSeAt(npc, 0x8E, 0x53);
        Npc_PlayCommonSeAt(npc, 0xA8, 0x53);
        Npc_PlayCommonSeAt(npc, 0xC2, 0x2D);
        Npc_PlayTownSeAt(npc, 0xE6, 0x75);
        Npc_PlayCommonSeAt(npc, 0x140, 0x2D);
        Npc_PlayTownSeAt(npc, 0x1A6, 0x76);
        Npc_PlayTownSeAt(npc, 0x1CA, 0x72);
        Npc_PlayTownSeAt(npc, 0x24A, 0x74);
        Npc_PlayTownSeAt(npc, 0x27C, 0x75);
        break;
    case 0x377:
        Npc_PlayCommonSeAt(npc, 2, 0x3D);
        Npc_PlayCommonSeAt(npc, 4, 0x3A);
        Npc_PlayCommonSeAt(npc, 0x56, 0x4A);
        Npc_PlayTownSeAt(npc, 0x64, 0x72);
        Npc_PlayCommonSeAt(npc, 0xC8, 0x64);
        Npc_PlayCommonSeAt(npc, 0xE2, 0x31);
        Npc_PlayCommonSeAt(npc, 0x10A, 0x31);
        Npc_PlayCommonSeAt(npc, 0x132, 0x31);
        Npc_PlayTownSeAt(npc, 0x172, 0x77);
        break;
    case 0x378:
        Npc_PlayCommonSeAt(npc, 2, 0x2D);
        Npc_PlayTownSeAt(npc, 0x52, 0x79);
        Npc_PlayCommonSeAt(npc, 0xD2, 0x3F);
        Npc_PlayCommonSeAt(npc, 0x11E, 0x42);
        Npc_PlayTownSeAt(npc, 0xCE, 0x78);
        Npc_PlayCommonSeAt(npc, 0x24C, 0x3A);
        Npc_PlayCommonSeAt(npc, 0x26C, 0x3A);
        Npc_PlayCommonSeAt(npc, 0x2BE, 0x3A);
        Npc_PlayCommonSeAt(npc, 0x2D2, 0x3A);
        Npc_PlayCommonSeAt(npc, 0x2E2, 0x3A);
        Npc_PlayCommonSeAt(npc, 0x302, 0x3A);
        Npc_PlayCommonSeAt(npc, 0x342, 0x2D);
        if ((EmMotion_PassedFrameScaled((Player *) npc, 290.0f, 0) != 0) && (Player_InCurrentArea((Player *) npc) & 0xFF)) {
            Em_BonePos(npc, 2, pos);
            pos[1] = npc->floorY;
            PartFx_SpawnAt(pos, 7, 1.0f);
        }
        break;
    }
}

/* The motion-effects handler of gNpcCommonHandlers (body 0, and the looks 0x3C-0x3E,
 * 0x4B and 0x4C on any body: Npc_SetupDraw), called each frame by Npc_RunMotionFx:
 * skips the first frame (NpcWork.fxStep 0, then 1), then runs Npc_MotionSounds. */
void Npc_RunMotionSounds(Em *npc)
{
    s8 step;
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    step = npcWork->fxStep;
    switch (step) {
    case 0:
        npcWork->fxStep = (s8) (step + 1);
        break;
    case 1:
        Npc_MotionSounds(npc, npcWork);
        break;
    }
}

/* An entry of the per-area townspeople tables (gAreaNpcTables[area], 0x48 bytes;
 * gAreaNpcCounts[area] of them), as this file's data holds them: the
 * townsperson id, then the rest of lobby_npc.h's NpcAreaEntry as bytes. Only
 * the id is read here; Npc_SetupFromArea reads the rest. */
typedef struct NpcAreaEntryData {
    u8 f0;              /* the townsperson id (NpcWork.id) */
    u8 pad1[0x47];
} NpcAreaEntryData;
extern NpcAreaEntryData *gAreaNpcTables[];
extern u8 gAreaNpcCounts[];

/* Places townsperson kind (an NpcWork.id) from the current area's table: for the
 * first entry with that id it takes a free Em slot, marks it a townsperson task
 * (ownTask) of the current area with the entry's index, and sets it up from the
 * entry (Npc_SetupFromArea). The set-up is called even when no slot was free, as
 * in the game. */
void Npc_Spawn(s32 npcId)
{
    s32 area = Area_GetSetIndex(gScene.area);
    NpcAreaEntryData *entry = gAreaNpcTables[area];
    s32 i;
    Em *em;

    for (i = 0; i < gAreaNpcCounts[area]; i++) {
        if (npcId == entry->f0) {
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
        entry++;
    }
}

/* Returns 1 when townsperson kind (an NpcWork.id; 0xFF: none) has an active Em
 * slot, else 0. */
s32 Npc_Exists(u8 npcId)
{
    Em *em;
    s32 i;

    if (npcId == 0xFF) {
        return 0;
    }
    em = gEms;
    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && ((NpcWork *)&em->work)->id == npcId) {
            return 1;
        }
    }
    return 0;
}

/* Returns the active Em slot of townsperson kind (an NpcWork.id), or NULL. */
Em *Npc_Find(u8 npcId)
{
    Em *em = gEms;
    s32 i;

    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && ((NpcWork *)&em->work)->id == npcId) {
            return em;
        }
    }
    return NULL;
}

/* Places the current area's alternating townspeople (gAreaNpcSpawns[area]: pairs
 * of ids, 0xFF ends the list): when neither of a pair is placed, the first one
 * if it is present now (Npc_IsPresent), else the second if it is. */
void Npc_SpawnArea(void)
{
    NpcPair *pair = (NpcPair *)gAreaNpcSpawns[Area_GetSetIndex(gScene.area)];

    if (pair == NULL) {
        return;
    }
    while (pair->first != 0xFF) {
        if (Npc_Exists(pair->first) == 0 && Npc_Exists(pair->second) == 0) {
            if (Npc_IsPresent(pair->first, gScene.area, 0) == 1) {
                Npc_Spawn(pair->first);
            } else if (Npc_IsPresent(pair->second, gScene.area, 0) == 1) {
                Npc_Spawn(pair->second);
            }
        }
        pair++;
    }
}

/* The file's data: the per-area townspeople tables and spawn pairs, the route
 * lines (gNpcSidePairs, gNpcSideLines), the partner pairs, the town's area
 * files and sets, and the townspeople's handler tables (gNpcHandlers by body). */
extern u32 gNpcHandlers_3[];
extern u32 gNpcHandlers_2[];
extern u32 gNpcHandlers_1[];
extern u32 gAreaNpcTables_0[];
void Npc_NopHandlerA(void);
void Npc_NopHandlerB(void);
extern void *gNpcCommonHandlers[];
extern void *gNpcNopHandlers[];
u8 gAreaNpcSpawns_0[16] = {
#include "lobby/007FBEA0.inc"
};
NpcAreaEntryData gAreaNpcTables_1[11] = {
#include "lobby/007FBEB0.inc"
};
u8 gAreaNpcSpawns_1[8] = {
#include "lobby/007FC1C8.inc"
};
NpcAreaEntryData gAreaNpcTables_11[6] = {
#include "lobby/007FC1D0.inc"
};
NpcAreaEntryData gAreaNpcTables_12[8] = {
#include "lobby/007FC380.inc"
};
NpcAreaEntryData gAreaNpcTables_13[14] = {
#include "lobby/007FC5C0.inc"
};
NpcAreaEntryData gAreaNpcTables_2[8] = {
#include "lobby/007FC9B0.inc"
};
NpcAreaEntryData gAreaNpcTables_3[9] = {
#include "lobby/007FCBF0.inc"
};
u32 D_007FCE80[12] = {
#include "lobby/007FCE80.inc"
};
NpcAreaEntryData gAreaNpcTables_4[11] = {
#include "lobby/007FCEB0.inc"
};
u8 gAreaNpcSpawns_4[12] = {
#include "lobby/007FD1C8.inc"
};
NpcAreaEntryData gAreaNpcTables_5[8] = {
#include "lobby/007FD1E0.inc"
};
NpcAreaEntryData gAreaNpcTables_6[25] = {
#include "lobby/007FD420.inc"
};
u32 gNpcSidePairs[8] = {
#include "lobby/007FDB30.inc"
};
u8 gAreaNpcSpawns_6[16] = {
#include "lobby/007FDB50.inc"
};
u32 gNpcSideLines[16] = {
#include "lobby/007FDB60.inc"
};
u32 gNpcPartnerPairs[4] = {
#include "lobby/007FDBA0.inc"
};
NpcAreaEntryData gAreaNpcTables_7 = {
#include "lobby/007FDBB0.inc"
};
NpcAreaEntryData gAreaNpcTables_14[2] = {
#include "lobby/007FDC00.inc"
};
NpcAreaEntryData gAreaNpcTables_10[19] = {
#include "lobby/007FDC90.inc"
};
u8 *gAreaNpcSpawns[16] = {
    gAreaNpcSpawns_0, gAreaNpcSpawns_1, NULL, NULL,
    gAreaNpcSpawns_4, NULL, gAreaNpcSpawns_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
NpcAreaEntryData *gAreaNpcTables[16] = {
    (NpcAreaEntryData *)gAreaNpcTables_0, gAreaNpcTables_1, gAreaNpcTables_2, gAreaNpcTables_3,
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
    (u32)gNpcCommonHandlers, (u32)gNpcHandlers_1, (u32)gNpcHandlers_2, (u32)gNpcHandlers_3,
    (u32)gNpcNopHandlers, (u32)gNpcNopHandlers, (u32)gNpcNopHandlers, (u32)gNpcNopHandlers,
    (u32)gNpcNopHandlers, (u32)gNpcNopHandlers, 0, 0,
};
void *gNpcNopHandlers[8] = {
    Npc_NopHandlerA, Npc_NopHandlerA, Npc_NopHandlerB, Npc_NopHandlerB,
    Npc_NopHandlerB, NULL, NULL, NULL,
};
void *gNpcCommonHandlers[8] = {
    Npc_NopHandlerA, Npc_NopHandlerA, Npc_NopHandlerB, Npc_RunMotionSounds,
    Npc_NopHandlerB, NULL, NULL, NULL,
};
