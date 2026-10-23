/*
 * gm_sub: the monsters' data files and their motion effects in gm_sub's
 * scenes.
 *
 * GmLoad_LoadEmData reads a monster kind's data file from the stage's
 * emcmd partition into the space behind the stage data and relocates its
 * pointers in place (GmLoad_Relocate); GmLoad_GetEmData returns it.
 * GmEm_UpdateSfx and GmEm_MotionEffects play the sounds and effects of a
 * monster's motions at their frames; gm_em.c's tables point to the small
 * callbacks here.
 */
#include "common.h"
#include "syssettings.h"
#include "m2c_macros.h"
#include "em.h"
#include "shadow.h"
#include "em_bulldrome.h"
#include "player.h"
#include "capsule.h"
#include "card.h"
#include "stream.h"
#include "gm_load.h"
#include "dust_flash_effects.h"
#include "gm_em.h"
#include "main.h"
#include "part_effects.h"
#include "libc.h"
#include "area.h"

/* Per-kind data files, loaded behind the stage data (gSaveBuf + 0x1000) and
 * relocated in place: gGmKindFileLists[kind] lists each kind's file ids, and
 * gGmLoadSlots[i] keeps the loaded data of slot i (gGmLoadEmIds[i] its id). */

extern s32 gGmKindFileLists_0[];
extern s32 gGmKindFileLists_2[];
extern s32 gGmKindFileLists_3[];
extern s32 gGmKindFileLists_4[];
extern s32 gGmKindFileLists_5[];
extern s32 gGmKindFileLists_6[];
extern s32 gGmKindFileLists_7[];
extern s32 gGmKindFileLists_8[];
extern s32 gGmKindFileLists_9[];
extern s32 gGmKindFileLists_10[];
extern s32 gGmKindFileLists_11[];
extern s32 gGmKindFileLists_12[];
extern s32 gGmKindFileLists_13[];
extern s32 gGmKindFileLists_14[];
extern s32 gGmKindFileLists_15[];

/* .data: per stage map (gScene.map), a list of data file ids by monster kind
 * (GmLoad_LoadEmData; -1: no file); the lists are gm_em.c's tables. */
s32 *gGmKindFileLists[36] = {
    gGmKindFileLists_0, gGmKindFileLists_0, gGmKindFileLists_2, gGmKindFileLists_3,
    gGmKindFileLists_4, gGmKindFileLists_5, gGmKindFileLists_6, gGmKindFileLists_7,
    gGmKindFileLists_8, gGmKindFileLists_9, gGmKindFileLists_10, gGmKindFileLists_11,
    gGmKindFileLists_12, gGmKindFileLists_13, gGmKindFileLists_14, gGmKindFileLists_15,
    gGmKindFileLists_2, gGmKindFileLists_3, gGmKindFileLists_4, gGmKindFileLists_5,
    gGmKindFileLists_6, gGmKindFileLists_11, gGmKindFileLists_12, gGmKindFileLists_0,
    gGmKindFileLists_9, gGmKindFileLists_10,
};

static void GmEm_NoMotionEffect(Em *em);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);          /* is the motion at this frame */
static void GmLoad_Relocate(u32, u32 *, u32 *);
/* .bss, from the highest address down. */
u8 gGmLoadEmIds[75];
void *gGmLoadSlots[75];
s32 gGmLoadEmDataUsed;

/* Static: MWCC then knows that this empty function changes no register,
 * so v0 stays live through GmEm_MotionEffects's default case and the switch's
 * compare chain keeps its constants in v1, as in the original (with a
 * global definition the last three go to v0). */
static void GmEm_NoMotionEffect(Em *em)
{

}

/*
 * The effects of a monster's motions in gm_sub's scenes: records the
 * current motion (at state + 6, in the monster's work area), runs
 * Bulldrome's quest effects (GmEm_SpawnBulldromeRageAura), then spawns each motion's
 * effects at its frames. Motions 14 to 16 are the Bulldrome's aim, charge
 * and charge stop (BULL_MOT_*); bones 6 and 9 are its feet
 * (BULL_BONE_FOOT_A and _B).
 */
void GmEm_MotionEffects(Em *em, u8 *state)
{
    f32 bonePos[3];

    if (em->motionId[0] != *(s16 *)(state + 6)) {
        *(s16 *)(state + 6) = em->motionId[0];
    }
    if (em->type == 68) {
        GmEm_SpawnBulldromeRageAura(em, *(s16 *)(state + 6));
    }
    switch (*(s16 *)(state + 6)) {
    case 1014:
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
            PartFx_SpawnForEmScaled(em, BULL_BONE_FOOT_A, 3, 0.7f);
        }
        break;
    case 1015:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0)) {
            PartFx_SpawnForEmScaled(em, 4, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            PartFx_SpawnForEmScaled(em, 17, 3, 3.5f);
        }
        break;
    case 1016:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) || EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
            PartFx_SpawnForEmScaled(em, BULL_BONE_FOOT_A, 3, 1.2f);
            PartFx_SpawnForEmScaled(em, BULL_BONE_FOOT_B, 3, 1.2f);
        }
        break;
    case 1018:
    case 1019:
        if (em->type == 68 && (EmMotion_PassedFrameScaled(em, 54.0f, 0) || EmMotion_PassedFrameScaled(em, 168.0f, 0)) && (Player_InCurrentArea(em) & 0xFF)) {
            Em_BonePos(em, 10, bonePos);
            bonePos[1] = em->floorY;
            PartFx_SpawnOnGround(bonePos, 7, 0.8f * em->scaleX);
        }
        break;
    case 1021:
        if (em->type == 68 && EmMotion_PassedFrameScaled(em, 34.0f, 0) && (Player_InCurrentArea(em) & 0xFF)) {
            Em_BonePos(em, BULL_BONE_FOOT_A, bonePos);
            bonePos[1] = em->floorY;
            PartFx_SpawnOnGround(bonePos, 4, 2.4f * em->scaleX);
        }
        break;
    case 1022:
        if (em->type == 68) {
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0) || EmMotion_PassedFrameScaled(em, 94.0f, 0) || EmMotion_PassedFrameScaled(em, 154.0f, 0)) {
                DustCloud_Create(em, 2, 46, 0.5f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 62.0f, 0) || EmMotion_PassedFrameScaled(em, 118.0f, 0) || EmMotion_PassedFrameScaled(em, 178.0f, 0)) {
                DustCloud_Create(em, 3, 45, 0.5f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 234.0f, 0) && (Player_InCurrentArea(em) & 0xFF)) {
                Em_BonePos(em, 3, bonePos);
                bonePos[1] = em->floorY;
                PartFx_SpawnOnGround(bonePos, 7, em->scaleX);
            }
        }
        break;
    case 1023:
        if (em->type == 68 && EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
            DustCloud_Create(em, 11, 0, 0.25f * em->scaleX);
        }
        break;
    case 1063:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0)) {
            PartFx_SpawnForEmScaled(em, 2, 7, 0.8f);
        }
        break;
    case 1071:
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0)) {
            PartFx_SpawnForEmScaled(em, 2, 6, 1.0f);
        }
        break;
    default:
        GmEm_NoMotionEffect(em);
        break;
    }
}

/* The monster's sound and effect state, each frame: sets its quest sounds,
 * and its motion effects while overlay 3 is loaded. */
void GmEm_UpdateSfx(Em *em)
{
    u8 *state = &em->work;

    switch (*state) {
    case 0:
        (*state)++;
        break;
    case 1:
        GmEm_MotionSounds(em, (EmWork *)state);
        if (Overlay_IsLoaded(3) == 1) {
            GmEm_MotionEffects(em, state);
        }
        break;
    }
}

/* Runs the monster's motion effects: its kind's motionFx handler. */
void GmEm_RunMotionFx(Em *em)
{
    em->handlers->motionFx(em);
}

/* Spawns the monster's shadow (Shadow_Spawn, SHADOW_MODEL): a callback of
 * the monsters' motion effect tables. */
void GmEm_SpawnShadow(Em *em)
{
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

void GmEm_Nop(void)
{

}

s32 GmEm_ReturnZero(void)
{
    return 0;
}

/*
 * Relocates a loaded data file at base. ptr walks its table of pointers
 * (up to `size`, where the table of sizes starts): each offset becomes an
 * address (-1 becomes NULL), and the block it points to, of the size given
 * beside it, has its own offsets relocated up to the first word that is
 * already an address.
 */
static void GmLoad_Relocate(u32 base, u32 *ptr, u32 *size)
{
    u32 *word;
    s32 i;
    s32 j;
    s32 count;
    s32 words;

    count = ((u32)size - (u32)ptr) >> 2;
    for (i = 0; i < count; i++, ptr++, size++) {
        if (*ptr == -1) {
            *ptr = 0;
            continue;
        }
        if (*ptr >= base) {
            continue;
        }
        if (*ptr == 0) {
            continue;
        }
        *ptr += base;
        words = *size >> 2;
        word = (u32 *)*ptr;
        for (j = 0; j < words; j++, word++) {
            if (*word == -1) {
                *word = 0;
                continue;
            }
            if (*word >= base) {
                break;
            }
            *word += base;
        }
    }
}

/* Forgets every loaded monster data file. */
void GmLoad_Reset(void)
{
    memset(gGmLoadEmIds, -1, sizeof(gGmLoadEmIds));
    memset(gGmLoadSlots, 0, sizeof(gGmLoadSlots));
    gGmLoadEmDataUsed = 0;
}

/* Opens the emcmd partition of the stage data (gSaveBuf). */
void GmLoad_LoadStagePartition(void)
{
    Emcmd_LoadPartition((void *) gSysSettings.saveBuf);
}

extern u8 *gSaveBuf;
/* Loads the data file of monster kind idx for stage kind into the next free space behind the stage data, then relocates it (once per kind). */
void GmLoad_LoadEmData(s32 kind, s32 idx)
{
    u8 *dst;
    u8 *buf;
    s32 used;
    void **slot;
    s32 n;
    s32 *id;
    u8 *base;
    u32 *tail;
    u8 *start;

    buf = gSysSettings.loadBuf;
    id = gGmKindFileLists[kind];
    id += idx;
    base = gSaveBuf + 4096;
    if (*id != -1) {
        used = gGmLoadEmDataUsed;
        slot = &gGmLoadSlots[idx];
        dst = base + used;
        if (*slot == NULL) {
            gGmLoadEmIds[idx] = *id;
            n = Emcmd_Read(*id, buf);
            memcpy(dst, buf, n);
            tail = (u32 *)(buf + n);
            used += n;
            do {
                tail--;
            } while (*tail == 0);
            start = (u8 *)(tail[-1] + (u32)dst);
            GmLoad_Relocate((u32)dst, (u32 *)start, (u32 *)(*tail + (u32)dst));
            *slot = start;
            gGmLoadEmDataUsed = used;
        }
    }
}

/* The loaded data of slot i (NULL if not loaded). */
void *GmLoad_GetEmData(s32 i)
{
    return gGmLoadSlots[i];
}
