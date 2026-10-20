/* The townspeople's motion effects, in three sets (NpcSetA, NpcSetB,
 * NpcSetC): each set's MotionFx plays sounds and effects on given frames of
 * the townsperson's motions. Then small tests of the lobby's state, and
 * the village's conditions: Cond_Check and its two halves, by the event
 * flags and by the hunter rank (enum VillageCond, village_cond.h). */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "scene.h"
#include "lobby_globals.h"
#include "save.h"
#include "player.h"
#include "em.h"
#include "lobby_npc.h"
#include "quest.h"
#include "village_cond.h"

void NpcSetC_MotionFx(u8 *task, NpcWork *m);
void NpcSetB_MotionFx(u8 *task, NpcWork *m);
void NpcSetA_MotionFx(u8 *task, NpcWork *m);
void Shadow_Spawn(void *pl, s16 kind);
s32 Game_IsOnline(void);
s32 Save_TestEventFlag(s32);
s32 Cond_CheckEvent(u16 id);
s32 Cond_CheckRank(u16 id);
s32 Cond_Check(u16 id);
s32 Save_IsQuestCleared(u16 id);
s32 Monster_AllHunted(void);
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);
u8 Player_InCurrentArea(void *obj);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void HitSpark_SpawnAt(f32 *pos, s32 a, u16 b);

/* The motion effects for a motion that has none. */
static void NpcSetA_NoFx(u8 *task)
{
}

s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void Npc_PlayTownSeAt(Em *arg0, s32 arg1, s32 arg2);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale);

/* A townsperson's sounds by its motion (+0x29C): Npc_PlayTownSeAt(npc, frame,
 * sound) plays a sound on a frame of the motion. */
void NpcSetA_MotionFx(u8 *task, NpcWork *work)
{
    f32 ofs[3];

    switch (*(u16 *)(task + 0x29C)) {
    case 0x3E9:
        Npc_PlayTownSeAt((Em *)task, 0x10, Sound_RandomSe(3, 1, 0, 1));
        Npc_PlayTownSeAt((Em *)task, 0x4C, Sound_RandomSe(3, 1, 0, 1));
        break;
    case 0x3EA:
        Npc_PlayTownSeAt((Em *)task, 0xC, Sound_RandomSe(3, 2, 3, 2));
        Npc_PlayTownSeAt((Em *)task, 6, 1);
        Npc_PlayTownSeAt((Em *)task, 0x10, 1);
        Npc_PlayTownSeAt((Em *)task, 0x24, 1);
        break;
    case 0x3EB:
        Npc_PlayTownSeAt((Em *)task, 0xC, Sound_RandomSe(3, 2, 3, 2));
        Npc_PlayTownSeAt((Em *)task, 0x26, Sound_RandomSe(3, 2, 3, 2));
        Npc_PlayTownSeAt((Em *)task, 6, 2);
        Npc_PlayTownSeAt((Em *)task, 0x1E, 2);
        Npc_PlayTownSeAt((Em *)task, 0x2C, 2);
        break;
    case 0x3F2:
        Npc_PlayTownSeAt((Em *)task, 2, Sound_RandomSe(4, 6, 3, 2));
        Npc_PlayTownSeAt((Em *)task, 0x40, Sound_RandomSe(4, 6, 3, 2));
        Npc_PlayTownSeAt((Em *)task, 0x60, Sound_RandomSe(3, 6, 4, 2));
        Npc_PlayTownSeAt((Em *)task, 0x80, Sound_RandomSe(4, 6, 3, 2));
        Npc_PlayTownSeAt((Em *)task, 0xA0, Sound_RandomSe(3, 6, 4, 2));
        Npc_PlayTownSeAt((Em *)task, 0x16, 1);
        Npc_PlayTownSeAt((Em *)task, 0x74, 1);
        break;
    case 0x3F3:
        Npc_PlayTownSeAt((Em *)task, 0x16, 4);
        Npc_PlayTownSeAt((Em *)task, 0xA, 1);
        Npc_PlayTownSeAt((Em *)task, 0x52, 1);
        break;
    case 0x3F6:
        Npc_PlayTownSeAt((Em *)task, 0x1E, 6);
        Npc_PlayTownSeAt((Em *)task, 0x16, 6);
        if (EmMotion_PassedFrameScaled((Player *) task, 66.0f, 0) != 0) {
            PartFx_SpawnForEmScaled((Em *) task, 6, 3, 0.7f);
        }
        break;
    case 0x3F7:
        Npc_PlayTownSeAt((Em *)task, 0xA, 6);
        Npc_PlayTownSeAt((Em *)task, 0x18, 2);
        Npc_PlayTownSeAt((Em *)task, 0x1C, 2);
        Npc_PlayTownSeAt((Em *)task, 0x24, 2);
        if (EmMotion_PassedFrameScaled((Player *) task, 16.0f, 0) != 0) {
            PartFx_SpawnForEmScaled((Em *) task, 4, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled((Player *) task, 30.0f, 0) != 0) {
            PartFx_SpawnForEmScaled((Em *) task, 0x11, 3, 3.5f);
        }
        break;
    case 0x3FA:
        Npc_PlayTownSeAt((Em *)task, 0x14, 6);
        Npc_PlayTownSeAt((Em *)task, 0x18, 5);
        Npc_PlayTownSeAt((Em *)task, 0x2A, 2);
        Npc_PlayTownSeAt((Em *)task, 0x2E, 2);
        break;
    case 0x432:
        if (EmMotion_ReachedFrame(task, 0, 60.0f) != 0) {
            switch (gScene.frame % 90) {
            case 0:
            case 10:
            case 20:
                ofs[2] = 15.0f;
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                Effect06_SpawnAt((Player *) task, 4, 0xB, ofs, 0.4f);
                break;
            }
        }
    default:
        NpcSetA_NoFx(task);
        break;
    }
}

/* Set A's update: the first frame only steps fxStep, after that the
 * motion effects run each frame (NpcSetA_MotionFx). */
void NpcSetA_Run(u8 *task)
{
    NpcWork *work = (NpcWork *)(task + 0x438);

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        NpcSetA_MotionFx(task, work);
        break;
    }
}

/* Set A's shadow. */
void NpcSetA_Effect(u8 *task)
{
    Shadow_Spawn(task, 0);
}

/* The motion effects for a motion that has none. */
static void NpcSetB_NoFx(u8 *task)
{
}

extern s32 gTownMode;
void Npc_PlayCommonSeAt(Em *arg0, s32 arg1, s32 arg2);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void PartFx_SpawnAt(f32 *pos, s32 id, f32 scale);

/* A townsperson's sounds and effects by its motion (+0x29C), like
 * NpcSetA_MotionFx; in event 0x3C some sounds depend on work->id (0x6F). */
void NpcSetB_MotionFx(u8 *task, NpcWork *work)
{
    f32 ofs[3];

    switch (*(u16 *)(task + 0x29C)) {
    case 0x3E9:
        break;
    case 0x3EA:
        Npc_PlayTownSeAt((Em *)task, 0x12, 0xB);
        Npc_PlayTownSeAt((Em *)task, 0x26, 0xB);
        Npc_PlayTownSeAt((Em *)task, 0x72, 0xB);
        Npc_PlayTownSeAt((Em *)task, 0x7E, 0xB);
        Npc_PlayTownSeAt((Em *)task, 0x98, 0xB);
        Npc_PlayTownSeAt((Em *)task, 8, 8);
        Npc_PlayTownSeAt((Em *)task, 0x58, 8);
        Npc_PlayTownSeAt((Em *)task, 0xD0, 8);
        break;
    case 0x3EB:
        Npc_PlayTownSeAt((Em *)task, 0x26, 0xA);
        Npc_PlayTownSeAt((Em *)task, 6, 8);
        Npc_PlayTownSeAt((Em *)task, 0x16, 8);
        Npc_PlayTownSeAt((Em *)task, 0x9A, 8);
        Npc_PlayTownSeAt((Em *)task, 0xA4, 8);
        break;
    case 0x3EC:
        break;
    case 0x3ED:
        Npc_PlayTownSeAt((Em *)task, 6, 0x10);
        break;
    case 0x3F3:
        Npc_PlayTownSeAt((Em *)task, 0x50, Sound_RandomSe(0xA, 2, 0xA, 2));
        Npc_PlayTownSeAt((Em *)task, 0xC, 8);
        Npc_PlayTownSeAt((Em *)task, 0x1E, 8);
        Npc_PlayTownSeAt((Em *)task, 0x2E, 8);
        Npc_PlayTownSeAt((Em *)task, 0x42, 8);
        Npc_PlayTownSeAt((Em *)task, 0x58, 8);
        Npc_PlayTownSeAt((Em *)task, 0x76, 8);
        Npc_PlayTownSeAt((Em *)task, 0x8C, 8);
        Npc_PlayTownSeAt((Em *)task, 0x98, 8);
        Npc_PlayTownSeAt((Em *)task, 0xA2, 8);
        Npc_PlayTownSeAt((Em *)task, 0xAE, 8);
        Npc_PlayTownSeAt((Em *)task, 0xB8, 8);
        Npc_PlayTownSeAt((Em *)task, 0xC2, 8);
        Npc_PlayTownSeAt((Em *)task, 0xAE, 8);
        break;
    case 0x3F4:
        if (gTownMode == 0x3C) {
            if (work->id == 0x6F) {
                Npc_PlayTownSeAt((Em *)task, 8, 0x12);
                Npc_PlayTownSeAt((Em *)task, 0xA, 8);
                Npc_PlayTownSeAt((Em *)task, 0x16, 8);
                Npc_PlayTownSeAt((Em *)task, 0x20, 8);
                Npc_PlayTownSeAt((Em *)task, 0x2E, 8);
                Npc_PlayTownSeAt((Em *)task, 0x38, 8);
                Npc_PlayTownSeAt((Em *)task, 0x44, 8);
                Npc_PlayTownSeAt((Em *)task, 0x4E, 8);
                Npc_PlayTownSeAt((Em *)task, 0x58, 8);
                Npc_PlayTownSeAt((Em *)task, 0x64, 8);
                Npc_PlayTownSeAt((Em *)task, 0x70, 8);
                break;
            }
            Npc_PlayCommonSeAt((Em *)task, 8, 0xB2);
            Npc_PlayTownSeAt((Em *)task, 0xA, 8);
            Npc_PlayTownSeAt((Em *)task, 0x16, 8);
            Npc_PlayTownSeAt((Em *)task, 0x20, 8);
            Npc_PlayTownSeAt((Em *)task, 0x2E, 8);
            Npc_PlayTownSeAt((Em *)task, 0x38, 8);
            Npc_PlayTownSeAt((Em *)task, 0x44, 8);
            Npc_PlayTownSeAt((Em *)task, 0x4E, 8);
            Npc_PlayTownSeAt((Em *)task, 0x58, 8);
            Npc_PlayTownSeAt((Em *)task, 0x64, 8);
            Npc_PlayTownSeAt((Em *)task, 0x70, 8);
            break;
        }
        Npc_PlayTownSeAt((Em *)task, 8, 0x12);
        Npc_PlayTownSeAt((Em *)task, 0xA, 8);
        Npc_PlayTownSeAt((Em *)task, 0x16, 8);
        Npc_PlayTownSeAt((Em *)task, 0x20, 8);
        Npc_PlayTownSeAt((Em *)task, 0x2E, 8);
        Npc_PlayTownSeAt((Em *)task, 0x38, 8);
        Npc_PlayTownSeAt((Em *)task, 0x44, 8);
        Npc_PlayTownSeAt((Em *)task, 0x4E, 8);
        Npc_PlayTownSeAt((Em *)task, 0x58, 8);
        Npc_PlayTownSeAt((Em *)task, 0x64, 8);
        Npc_PlayTownSeAt((Em *)task, 0x70, 8);
        break;
    case 0x3F8:
        Npc_PlayTownSeAt((Em *)task, 0xA, 8);
        Npc_PlayTownSeAt((Em *)task, 0xA, 0x13);
        Npc_PlayTownSeAt((Em *)task, 0x18, 8);
        Npc_PlayTownSeAt((Em *)task, 0x26, 8);
        Npc_PlayTownSeAt((Em *)task, 0x32, 8);
        break;
    case 0x40C:
        if (gTownMode == 0x3C) {
            if (work->id == 0x6F) {
                Npc_PlayTownSeAt((Em *)task, 2, 0xC);
                break;
            }
            Npc_PlayCommonSeAt((Em *)task, 2, 0xB1);
            Npc_PlayCommonSeAt((Em *)task, 0xA, 0xB3);
            break;
        }
        Npc_PlayTownSeAt((Em *)task, 2, 0xC);
        break;
    case 0x40E:
        if (gTownMode == 0x3C) {
            if (work->id == 0x6F) {
                Npc_PlayTownSeAt((Em *)task, 0x12, Sound_RandomSe(0xC, 4, 0xD, 4));
                Npc_PlayTownSeAt((Em *)task, 0x3C, Sound_RandomSe(0xE, 4, 0xE, 4));
                break;
            }
            Npc_PlayCommonSeAt((Em *)task, 0x12, Sound_RandomSe(0xB2, 4, 0xB0, 4));
            Npc_PlayCommonSeAt((Em *)task, 0x3C, Sound_RandomSe(0xB1, 4, 0xB1, 4));
            break;
        }
        Npc_PlayTownSeAt((Em *)task, 0x12, Sound_RandomSe(0xC, 4, 0xD, 4));
        Npc_PlayTownSeAt((Em *)task, 0x3C, Sound_RandomSe(0xE, 4, 0xE, 4));
        break;
    case 0x42E:
        Npc_PlayTownSeAt((Em *)task, 4, 0x10);
        Npc_PlayTownSeAt((Em *)task, 0x100, 0xB);
        break;
    case 0x42F:
        Npc_PlayTownSeAt((Em *)task, 0x10, 8);
        Npc_PlayTownSeAt((Em *)task, 0x12, 8);
        break;
    case 0x430:
        Npc_PlayTownSeAt((Em *)task, 0x30, 0xC);
        break;
    case 0x431:
        Npc_PlayTownSeAt((Em *)task, 0xA, 0xA);
        break;
    case 0x432:
        if (gTownMode == 0x3C) {
            if (work->id == 0x6F) {
                Npc_PlayTownSeAt((Em *)task, 0x34, 9);
                Npc_PlayCommonSeAt((Em *)task, 0x44, 0xA8);
                Npc_PlayTownSeAt((Em *)task, 0x16, 8);
                Npc_PlayTownSeAt((Em *)task, 0x1E, 8);
                Npc_PlayTownSeAt((Em *)task, 0x80, 8);
                Npc_PlayTownSeAt((Em *)task, 0x94, 8);
                break;
            }
            Npc_PlayCommonSeAt((Em *)task, 0x34, 0xAF);
            Npc_PlayCommonSeAt((Em *)task, 0x44, 0xA8);
            Npc_PlayTownSeAt((Em *)task, 0x16, 8);
            Npc_PlayTownSeAt((Em *)task, 0x1E, 8);
            Npc_PlayTownSeAt((Em *)task, 0x80, 8);
            Npc_PlayTownSeAt((Em *)task, 0x94, 8);
            break;
        }
        Npc_PlayTownSeAt((Em *)task, 8, 0xE);
        Npc_PlayTownSeAt((Em *)task, 0x34, 9);
        Npc_PlayCommonSeAt((Em *)task, 0x44, 0xAC);
        Npc_PlayTownSeAt((Em *)task, 0x16, 8);
        Npc_PlayTownSeAt((Em *)task, 0x1E, 8);
        Npc_PlayTownSeAt((Em *)task, 0x80, 8);
        Npc_PlayTownSeAt((Em *)task, 0x94, 8);
        break;
    case 0x433:
        Npc_PlayTownSeAt((Em *)task, 0xE, 0xF);
        break;
    case 0x43C:
        if (gTownMode == 0x3C) {
            if (work->id == 0x6F) {
                Npc_PlayTownSeAt((Em *)task, 0x1A, 9);
                Npc_PlayTownSeAt((Em *)task, 0x50, 8);
                Npc_PlayCommonSeAt((Em *)task, 0x7A, 0xA9);
                Npc_PlayTownSeAt((Em *)task, 0xB4, 0xC);
                Npc_PlayCommonSeAt((Em *)task, 0xBA, 0x3D);
                Npc_PlayCommonSeAt((Em *)task, 0xE0, 0xAA);
                Npc_PlayCommonSeAt((Em *)task, 0xE6, 0xAB);
                break;
            }
            Npc_PlayCommonSeAt((Em *)task, 0x1A, 0xB1);
            Npc_PlayCommonSeAt((Em *)task, 0x50, 0xB0);
            Npc_PlayCommonSeAt((Em *)task, 0x7A, 0xA9);
            Npc_PlayCommonSeAt((Em *)task, 0xB4, 0xAF);
            Npc_PlayCommonSeAt((Em *)task, 0xBA, 0x3D);
            Npc_PlayCommonSeAt((Em *)task, 0xE0, 0xAA);
            Npc_PlayCommonSeAt((Em *)task, 0xE6, 0xAB);
            break;
        }
        Npc_PlayCommonSeAt((Em *)task, 0x1A, 0xB1);
        Npc_PlayCommonSeAt((Em *)task, 0x50, 0xB0);
        Npc_PlayCommonSeAt((Em *)task, 0x7A, 0xA9);
        Npc_PlayCommonSeAt((Em *)task, 0xB4, 0xAF);
        Npc_PlayCommonSeAt((Em *)task, 0xBA, 0x3D);
        Npc_PlayCommonSeAt((Em *)task, 0xE0, 0xAA);
        Npc_PlayCommonSeAt((Em *)task, 0xE6, 0xAB);
        break;
    case 0x43D:
        if (gTownMode == 0x3C) {
            if (work->id == 0x6F) {
                Npc_PlayCommonSeAt((Em *)task, 4, 0xAA);
                Npc_PlayCommonSeAt((Em *)task, 0xA, 0xAB);
                Npc_PlayCommonSeAt((Em *)task, 0x14, 0xAA);
                Npc_PlayCommonSeAt((Em *)task, 0x1C, 0xAB);
                Npc_PlayCommonSeAt((Em *)task, 0x26, 0xAA);
                Npc_PlayCommonSeAt((Em *)task, 0x36, 0xAA);
                Npc_PlayTownSeAt((Em *)task, 0x3E, 0xC);
                Npc_PlayCommonSeAt((Em *)task, 0x42, 0x3D);
                Npc_PlayTownSeAt((Em *)task, 0x54, 8);
                break;
            }
            Npc_PlayCommonSeAt((Em *)task, 4, 0xAA);
            Npc_PlayCommonSeAt((Em *)task, 0xA, 0xAB);
            Npc_PlayCommonSeAt((Em *)task, 0x14, 0xAA);
            Npc_PlayCommonSeAt((Em *)task, 0x1C, 0xAB);
            Npc_PlayCommonSeAt((Em *)task, 0x26, 0xAA);
            Npc_PlayCommonSeAt((Em *)task, 0x36, 0xAA);
            Npc_PlayCommonSeAt((Em *)task, 0x3E, 0xAF);
            Npc_PlayCommonSeAt((Em *)task, 0x42, 0x3D);
            Npc_PlayTownSeAt((Em *)task, 0x54, 8);
            break;
        }
        Npc_PlayCommonSeAt((Em *)task, 4, 0xAA);
        Npc_PlayCommonSeAt((Em *)task, 0xA, 0xAB);
        Npc_PlayCommonSeAt((Em *)task, 0x14, 0xAA);
        Npc_PlayCommonSeAt((Em *)task, 0x1C, 0xAB);
        Npc_PlayCommonSeAt((Em *)task, 0x26, 0xAA);
        Npc_PlayCommonSeAt((Em *)task, 0x36, 0xAA);
        Npc_PlayTownSeAt((Em *)task, 0x3E, 0xC);
        Npc_PlayCommonSeAt((Em *)task, 0x42, 0x3D);
        Npc_PlayTownSeAt((Em *)task, 0x54, 8);
        break;
    case 0x43E:
        if (gTownMode == 0x3C) {
            if (work->id == 0x6F) {
                Npc_PlayTownSeAt((Em *)task, 0x10, 8);
                Npc_PlayTownSeAt((Em *)task, 0x14, 0x10);
                break;
            }
            Npc_PlayTownSeAt((Em *)task, 0x10, 8);
            Npc_PlayCommonSeAt((Em *)task, 0x14, 0xAE);
            break;
        }
        Npc_PlayTownSeAt((Em *)task, 0x10, 8);
        Npc_PlayTownSeAt((Em *)task, 0x14, 0x10);
        break;
    case 0x435:
        Npc_PlayTownSeAt((Em *)task, 8, 8);
        Npc_PlayTownSeAt((Em *)task, 0x16, 8);
        Npc_PlayTownSeAt((Em *)task, 0x22, 8);
        break;
    case 0x436:
        Npc_PlayTownSeAt((Em *)task, 0x1A, 0x52);
        Npc_PlayTownSeAt((Em *)task, 0x52, 0x52);
        Npc_PlayTownSeAt((Em *)task, 0x8C, 0x53);
        Npc_PlayTownSeAt((Em *)task, 0xAA, 0x52);
        Npc_PlayTownSeAt((Em *)task, 0xCA, 0x53);
        if (EmMotion_PassedFrameScaled((Player *) task, 32.0f, 0) != 0 || EmMotion_PassedFrameScaled((Player *) task, 92.0f, 0) != 0) {
            Em_BonePos(task, 9, ofs);
            ofs[1] = *(f32 *)(task + 0x574);
            if (Player_InCurrentArea((Player *) task) != 0) {
                PartFx_SpawnAt(ofs, 0x14, 1.5f);
            }
        }
        if (EmMotion_PassedFrameScaled((Player *) task, 146.0f, 0) != 0 || EmMotion_PassedFrameScaled((Player *) task, 174.0f, 0) != 0 || EmMotion_PassedFrameScaled((Player *) task, 208.0f, 0) != 0) {
            Em_BonePos(task, 9, ofs);
            ofs[1] = *(f32 *)(task + 0x574);
            if (Player_InCurrentArea((Player *) task) != 0) {
                PartFx_SpawnAt(ofs, 4, 1.0f);
            }
        }
        break;
    case 0x437:
        Npc_PlayTownSeAt((Em *)task, 0x28, 0x10);
        break;
    default:
        NpcSetB_NoFx(task);
        break;
    }
}

/* Set B's update: the first frame only steps fxStep, after that the
 * motion effects run each frame (NpcSetB_MotionFx). */
void NpcSetB_Run(u8 *task)
{
    NpcWork *work = (NpcWork *)(task + 0x438);

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        NpcSetB_MotionFx(task, work);
        break;
    }
}

/* Set B's shadow. */
void NpcSetB_Effect(u8 *task)
{
    Shadow_Spawn(task, 0);
}

/* An empty handler (set B's slots 2 and 4). */
void NpcSetB_Nop(void)
{

}

/* Once the motion passes the frame, and the townsperson is in the current
 * area: a hit spark (kind 7) at its position plus ofs turned by its
 * rotation. */
void NpcSetC_SpawnModelAt(u8 *obj, f32 frame, f32 *ofs)
{
    f32 pos[3];
    f32 rotated[3];

    if (EmMotion_PassedFrameScaled((Player *) obj, frame, 0) != 0 && Player_InCurrentArea((Player *) obj) == 1) {
        Vec_Rotate((f32 *) rotated, (f32 *) ofs, (f32 *) (obj + 0x60));
        pos[0] = *(f32 *)(obj + 0xAC) + rotated[0];
        pos[1] = *(f32 *)(obj + 0xB0) + rotated[1];
        pos[2] = *(f32 *)(obj + 0xB4) + rotated[2];
        HitSpark_SpawnAt(pos, 7, *(u16 *)(obj + 0xA4));
    }
}

s32 EmMotion_InFrameRange(void *em, s32 a, f32 frame, f32 frame2);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void ModelObj_SpawnOn(void *obj, s32 a);

/* Another townsperson's sounds and effects by its motion (+0x29C); then more
 * by the townsperson work->id (0xE, 0xF, 0x58). */
void NpcSetC_MotionFx(u8 *task, NpcWork *work)
{
    f32 ofs[3];

    switch (*(u16 *)(task + 0x29C)) {
    case 0x3EF:
        Npc_PlayTownSeAt((Em *)task, 0xA, 0x26);
        break;
    case 0x3F0:
        Npc_PlayTownSeAt((Em *)task, 0x2A, Sound_RandomSe(0x4F, 2, 0x4F, 1));
        Npc_PlayTownSeAt((Em *)task, 0x6A, Sound_RandomSe(0x51, 1, 0x50, 2));
        Npc_PlayTownSeAt((Em *)task, 0xB4, Sound_RandomSe(0x51, 2, 0x50, 1));
        Npc_PlayTownSeAt((Em *)task, 0xEE, Sound_RandomSe(0x28, 3, 0x29, 1));
        Npc_PlayTownSeAt((Em *)task, 0x15E, Sound_RandomSe(0x28, 1, 0x29, 3));
        break;
    case 0x3F5:
        Npc_PlayTownSeAt((Em *)task, 0x26, Sound_RandomSe(0x2A, 1, 0x2B, 1));
        Npc_PlayTownSeAt((Em *)task, 0x6C, Sound_RandomSe(0x2C, 1, 0x2A, 1));
        Npc_PlayTownSeAt((Em *)task, 0xC, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x2C, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x4C, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x6A, 0x25);
        break;
    case 0x3F7:
        Npc_PlayTownSeAt((Em *)task, 0x42, 0x2D);
        Npc_PlayTownSeAt((Em *)task, 0xC2, 0x2E);
        break;
    case 0x3F8:
        Npc_PlayTownSeAt((Em *)task, 0xE, 0x30);
        break;
    case 0x3FA:
        Npc_PlayTownSeAt((Em *)task, 0xE, Sound_RandomSe(0x2F, 2, 0x50, 2));
        Npc_PlayTownSeAt((Em *)task, 0x8E, Sound_RandomSe(0x4F, 2, 0x4F, 2));
        Npc_PlayTownSeAt((Em *)task, 0xCE, Sound_RandomSe(0x51, 1, 0x50, 3));
        Npc_PlayTownSeAt((Em *)task, 0x118, Sound_RandomSe(0x51, 3, 0x50, 1));
        break;
    case 0x3FB:
        Npc_PlayTownSeAt((Em *)task, 0x34, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x52, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x94, 0x31);
        break;
    case 0x3FC:
        Npc_PlayTownSeAt((Em *)task, 4, 0x26);
        Npc_PlayTownSeAt((Em *)task, 0x3A, 0x2F);
        Npc_PlayTownSeAt((Em *)task, 0x5E, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x78, 0x30);
        break;
    case 0x3FD:
        Npc_PlayTownSeAt((Em *)task, 8, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x10, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x1A, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x26, 0x25);
        break;
    case 0x3FE:
        Npc_PlayTownSeAt((Em *)task, 8, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x12, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x1C, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x26, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x32, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x3C, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x38, 0x2B);
        Npc_PlayTownSeAt((Em *)task, 0x82, 0x2C);
        Npc_PlayTownSeAt((Em *)task, 0xE6, 0x4D);
        break;
    case 0x401:
        Npc_PlayTownSeAt((Em *)task, 0x32, 0x2F);
        Npc_PlayTownSeAt((Em *)task, 0xAA, 0x2B);
        Npc_PlayTownSeAt((Em *)task, 0xE2, 0x28);
        break;
    case 0x403:
        Npc_PlayTownSeAt((Em *)task, 0xE, Sound_RandomSe(0x2F, 2, 0x50, 2));
        Npc_PlayTownSeAt((Em *)task, 0x8C, Sound_RandomSe(0x51, 2, 0x4F, 2));
        Npc_PlayTownSeAt((Em *)task, 0x162, Sound_RandomSe(0x2A, 1, 0x2C, 3));
        break;
    case 0x400:
        if (EmMotion_PassedFrameScaled((Player *) task, 84.0f, 0) != 0) {
            PartFx_SpawnForEmScaled((Em *) task, 0xD, 0x1C, 1.0f);
        }
        if (EmMotion_InFrameRange(task, 0, 38.0f, 198.0f) != 0 && gScene.frame % 10 == 0) {
            ModelObj_SpawnOn(task, 2);
        }
        break;
    case 0x40C:
        Vec_Set(ofs, 0.0f, 20.0f, 125.0f);
        NpcSetC_SpawnModelAt(task, 96.0f, ofs);
        break;
    case 0x40D:
        Vec_Set(ofs, 0.0f, 20.0f, 125.0f);
        NpcSetC_SpawnModelAt(task, 28.0f, ofs);
        break;
    case 0x41F:
        Npc_PlayTownSeAt((Em *)task, 0xC, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x2A, 0x25);
        Npc_PlayTownSeAt((Em *)task, 0x4A, 0x24);
        Npc_PlayTownSeAt((Em *)task, 0x62, 0x25);
        break;
    case 0x420:
        Npc_PlayTownSeAt((Em *)task, 0x10, 0x94);
        break;
    case 0x429:
        Npc_PlayTownSeAt((Em *)task, 0x30, Sound_RandomSe(0x93, 2, 0x93, 2));
        Npc_PlayTownSeAt((Em *)task, 0x13A, Sound_RandomSe(0x94, 3, 0x94, 1));
        break;
    case 0x421:
        Npc_PlayTownSeAt((Em *)task, 0x22, 0x2E);
        break;
    case 0x422:
        Npc_PlayTownSeAt((Em *)task, 0xE, 0x26);
        Npc_PlayTownSeAt((Em *)task, 0x78, 0xC0);
        Npc_PlayTownSeAt((Em *)task, 0xDC, 0xC0);
        Npc_PlayTownSeAt((Em *)task, 0x140, 0xC0);
        Npc_PlayTownSeAt((Em *)task, 0x182, 0xC1);
        Npc_PlayTownSeAt((Em *)task, 0x1D6, 0x2A);
        break;
    case 0x423:
        Npc_PlayTownSeAt((Em *)task, 4, 0x28);
        Npc_PlayTownSeAt((Em *)task, 0x42, 0x26);
        break;
    case 0x425:
        Npc_PlayTownSeAt((Em *)task, 4, 0xC2);
        break;
    case 0x426:
        Npc_PlayTownSeAt((Em *)task, 0xA, 0xC3);
        break;
    case 0x3EA:
        Npc_PlayTownSeAt((Em *)task, 0x10, 0xB8);
        Npc_PlayCommonSeAt((Em *)task, 0x12, 2);
        Npc_PlayTownSeAt((Em *)task, 0x36, 0xBD);
        break;
    case 0x3EB:
        Npc_PlayTownSeAt((Em *)task, 4, 0xB8);
        Npc_PlayTownSeAt((Em *)task, 0xCC, Sound_RandomSe(0xB9, 3, 0xB9, 2));
        Npc_PlayTownSeAt((Em *)task, 0x196, Sound_RandomSe(0xBB, 2, 0xBB, 3));
        break;
    case 0x3EC:
        Npc_PlayTownSeAt((Em *)task, 0x2C, 0xBC);
        break;
    }
    if (work->id == 0xE) {
        switch (*(u16 *)(task + 0x29C)) {
    case 0x407:
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x7D);
        Npc_PlayTownSeAt((Em *)task, 0x84, 0x7E);
        Npc_PlayTownSeAt((Em *)task, 0x14A, 0x7C);
        Npc_PlayTownSeAt((Em *)task, 0x14A, 0x5D);
        Npc_PlayTownSeAt((Em *)task, 0x15A, 0x5E);
        Npc_PlayTownSeAt((Em *)task, 0x17C, 0x5D);
        Npc_PlayTownSeAt((Em *)task, 0x18C, 0x5E);
        Npc_PlayTownSeAt((Em *)task, 0x230, 0x7D);
        Npc_PlayTownSeAt((Em *)task, 0x28A, 0x7E);
        Npc_PlayTownSeAt((Em *)task, 0x30A, 0x68);
        Npc_PlayTownSeAt((Em *)task, 0x356, 0x89);
        Npc_PlayTownSeAt((Em *)task, 0x37E, 0x89);
        break;
    case 0x408:
        Npc_PlayTownSeAt((Em *)task, 6, 0x7F);
        Npc_PlayTownSeAt((Em *)task, 0x42, 0x80);
        Npc_PlayTownSeAt((Em *)task, 0xA8, 0x81);
        break;
    case 0x409:
        Npc_PlayTownSeAt((Em *)task, 0x5E, 0x83);
        Npc_PlayTownSeAt((Em *)task, 0x82, 0x85);
        Npc_PlayTownSeAt((Em *)task, 0xCE, Sound_RandomSe(0x87, 3, 0x88, 3));
        Npc_PlayTownSeAt((Em *)task, 0x112, 0x83);
        Npc_PlayTownSeAt((Em *)task, 0x132, 0x85);
        Npc_PlayTownSeAt((Em *)task, 0x152, Sound_RandomSe(0x88, 3, 0x87, 3));
        Npc_PlayTownSeAt((Em *)task, 0x192, 0x86);
        Npc_PlayTownSeAt((Em *)task, 0x244, 0x84);
        break;
    case 0x40A:
        Npc_PlayTownSeAt((Em *)task, 0x14, Sound_RandomSe(0x88, 4, 0x87, 4));
        Npc_PlayTownSeAt((Em *)task, 0x3A, Sound_RandomSe(0x87, 3, 0x90, 3));
        Npc_PlayTownSeAt((Em *)task, 0xE2, Sound_RandomSe(0x89, 3, 0x87, 3));
        Npc_PlayTownSeAt((Em *)task, 0x168, Sound_RandomSe(0x87, 3, 0x88, 3));
        break;
    case 0x40B:
        Npc_PlayTownSeAt((Em *)task, 0x14, 0x88);
        Npc_PlayTownSeAt((Em *)task, 0x36, 0x8A);
        break;
    case 0x40C:
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x8C);
        Npc_PlayTownSeAt((Em *)task, 0x58, 0x60);
        Npc_PlayTownSeAt((Em *)task, 0x62, 0x5C);
        break;
    case 0x40D:
        Npc_PlayTownSeAt((Em *)task, 0x12, 0x60);
        Npc_PlayTownSeAt((Em *)task, 0x8A, 0x8D);
        Npc_PlayTownSeAt((Em *)task, 0x10, 0x60);
        Npc_PlayTownSeAt((Em *)task, 0x1C, 0x5C);
        break;
    case 0x40E:
        Npc_PlayCommonSeAt((Em *)task, 8, 0x3F);
        Npc_PlayTownSeAt((Em *)task, 0x24, 0x92);
        break;
    case 0x40F:
        Npc_PlayTownSeAt((Em *)task, 6, 0x8F);
        Npc_PlayTownSeAt((Em *)task, 0x14, 0x61);
        break;
    case 0x410:
        Npc_PlayTownSeAt((Em *)task, 0x32, 0x90);
        Npc_PlayTownSeAt((Em *)task, 0x18, 0x62);
        break;
    case 0x411:
        Npc_PlayTownSeAt((Em *)task, 0x28, 0x90);
        Npc_PlayTownSeAt((Em *)task, 0xA, 0x62);
        break;
        }
    } else if (work->id == 0xF) {
        switch (*(u16 *)(task + 0x29C)) {
    case 0x40B:
        Npc_PlayTownSeAt((Em *)task, 0x36, 0x2F);
        break;
    case 0x40C:
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x26);
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x26);
        Npc_PlayTownSeAt((Em *)task, 0x58, 0x60);
        Npc_PlayTownSeAt((Em *)task, 0x62, 0x5C);
        break;
    case 0x40D:
        Npc_PlayTownSeAt((Em *)task, 0x10, 0x60);
        Npc_PlayTownSeAt((Em *)task, 0x1C, 0x5C);
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x4D);
        break;
    case 0x41A:
        Npc_PlayTownSeAt((Em *)task, 4, 0x26);
        break;
        }
    } else if (work->id == 0x58) {
        switch (*(u16 *)(task + 0x29C)) {
    case 0x40B:
        Npc_PlayTownSeAt((Em *)task, 0x36, 0x2F);
        break;
    case 0x40C:
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x26);
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x26);
        Npc_PlayTownSeAt((Em *)task, 0x58, 0x60);
        Npc_PlayTownSeAt((Em *)task, 0x62, 0x5C);
        break;
    case 0x40D:
        Npc_PlayTownSeAt((Em *)task, 0x10, 0x60);
        Npc_PlayTownSeAt((Em *)task, 0x1C, 0x5C);
        Npc_PlayTownSeAt((Em *)task, 0x1E, 0x4D);
        break;
    case 0x41A:
        Npc_PlayTownSeAt((Em *)task, 4, 0x26);
        break;
        }
    }
}

/* Set C's update: the first frame only steps fxStep, after that the
 * motion effects run each frame (NpcSetC_MotionFx). */
void NpcSetC_Run(u8 *task)
{
    NpcWork *work = (NpcWork *)(task + 0x438);

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        NpcSetC_MotionFx(task, work);
        break;
    }
}

/* Set C's shadow. */
void NpcSetC_Effect(u8 *task)
{
    Shadow_Spawn(task, 0);
}

/* An empty handler of set C. */
void NpcSetC_Nop(void)
{

}

/* 1 while gLobby->elderRaidState is 1. */
s32 ElderRaid_InState1(void)
{
    return gLobby->elderRaidState == 1;
}

/* 1 while gLobby->elderRaidState is 2. */
s32 ElderRaid_InState2(void)
{
    return gLobby->elderRaidState == 2;
}

/* 1 offline while event flag 0x186 is set and 0x187 isn't (the event the
 * first one starts hasn't ended). */
s32 Village_Event186Pending(void)
{
    if ((Game_IsOnline() == 0) && (Save_TestEventFlag(0x186) == 1) && (Save_TestEventFlag(0x187) == 0)) {
        return 1;
    }
    return 0;
}

/* 1 while gLobby->townEvent is 4. */
s32 Village_Is3924Kind4(void)
{
    return gLobby->townEvent == 4;
}

/* The village's conditions below COND_RANK_FIRST (enum VillageCond): each
 * tests the save's event flags (one flag, or two), another condition, or
 * Monster_AllHunted; 1 when it holds. The cases are in the order the
 * game's jump table gives their blocks. */
s32 Cond_CheckEvent(u16 id)
{
    switch (id) {
    case 27:
        if (Save_TestEventFlag(0x59) == 1) {
            return 1;
        }
        break;
    case 26:
        if (Save_TestEventFlag(0x58) == 1) {
            return 1;
        }
        break;
    case 25:
        if (Save_TestEventFlag(0x57) == 1) {
            return 1;
        }
        break;
    case 24:
        if (Save_TestEventFlag(0x55) == 1) {
            return 1;
        }
        break;
    case 23:
        if (Save_TestEventFlag(0x54) == 1 && Save_TestEventFlag(0x53) == 1) {
            return 1;
        }
        break;
    case 22:
        if (Save_TestEventFlag(0x53) == 1) {
            return 1;
        }
        break;
    case 21:
        if (Save_TestEventFlag(0x52) == 1) {
            return 1;
        }
        break;
    case 20:
        if (Save_TestEventFlag(0x51) == 1) {
            return 1;
        }
        break;
    case 19:
        if (Save_TestEventFlag(0x50) == 1) {
            return 1;
        }
        break;
    case 18:
        if (Save_TestEventFlag(0x4F) == 1 && Save_TestEventFlag(0x4E) == 1) {
            return 1;
        }
        break;
    case 17:
        if (Save_TestEventFlag(0x4E) == 1) {
            return 1;
        }
        break;
    case 16:
        if (Save_TestEventFlag(0x4D) == 1) {
            return 1;
        }
        break;
    case 15:
        if (Save_TestEventFlag(0x4C) == 1) {
            return 1;
        }
        break;
    case 14:
        if (Save_TestEventFlag(0x4B) == 1) {
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(0x48) == 1) {
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(0x47) == 1) {
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(0x46) == 1) {
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(0x12) == 1) {
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(0x11) == 1) {
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(0xC) == 1) {
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(0xB) == 1) {
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x9) == 1) {
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x8) == 1) {
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x7) == 1) {
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(0x6) == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x5) == 1) {
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(0x0) == 1) {
            return 1;
        }
        break;
    case 62:
        if (Save_TestEventFlag(0x3B)) {
            return 1;
        }
        break;
    case 60:
        if (Save_TestEventFlag(0x3A)) {
            return 1;
        }
        break;
    case 59:
        if (Save_TestEventFlag(0x39)) {
            return 1;
        }
        break;
    case 58:
        if (Save_TestEventFlag(0x38)) {
            return 1;
        }
        break;
    case 57:
        if (Save_TestEventFlag(0x37)) {
            return 1;
        }
        break;
    case 56:
        if (Save_TestEventFlag(0x36)) {
            return 1;
        }
        break;
    case 55:
        if (Save_TestEventFlag(0x35)) {
            return 1;
        }
        break;
    case 53:
        if (Save_TestEventFlag(0x34)) {
            return 1;
        }
        break;
    case 52:
        if (Save_TestEventFlag(0x33)) {
            return 1;
        }
        break;
    case 51:
        if (Cond_Check(0xA)) {
            return 1;
        }
        break;
    case 50:
        if (Cond_Check(0x7)) {
            return 1;
        }
        break;
    case 49:
        if (Cond_Check(0x3)) {
            return 1;
        }
        break;
    case 48:
        if (Cond_Check(0x2)) {
            return 1;
        }
        break;
    case 47:
        if (Cond_Check(0x1)) {
            return 1;
        }
        break;
    case 33:
        if (Cond_Check(0x6)) {
            return 1;
        }
        break;
    case 32:
        if (Cond_Check(0x3)) {
            return 1;
        }
        break;
    case 31:
        if (Cond_Check(0x1)) {
            return 1;
        }
        break;
    case 78:
        if (Save_TestEventFlag(0x45)) {
            return 1;
        }
        break;
    case 76:
        if (Save_TestEventFlag(0x44)) {
            return 1;
        }
        break;
    case 75:
        if (Save_TestEventFlag(0x43)) {
            return 1;
        }
        break;
    case 74:
        if (Save_TestEventFlag(0x42)) {
            return 1;
        }
        break;
    case 73:
        if (Save_TestEventFlag(0x41)) {
            return 1;
        }
        break;
    case 72:
        if (Save_TestEventFlag(0x40)) {
            return 1;
        }
        break;
    case 71:
        if (Save_TestEventFlag(0x3F)) {
            return 1;
        }
        break;
    case 69:
        if (Save_TestEventFlag(0x3E)) {
            return 1;
        }
        break;
    case 68:
        if (Save_TestEventFlag(0x3D)) {
            return 1;
        }
        break;
    case 67:
        if (Cond_Check(0xA)) {
            return 1;
        }
        break;
    case 66:
        if (Cond_Check(0x7)) {
            return 1;
        }
        break;
    case 65:
        if (Cond_Check(0x3)) {
            return 1;
        }
        break;
    case 64:
        if (Cond_Check(0x2)) {
            return 1;
        }
        break;
    case 63:
        if (Cond_Check(0x1)) {
            return 1;
        }
        break;
    case COND_ALL_MONSTERS_HUNTED:
        if (Monster_AllHunted() == 1) {
            return 1;
        }
        break;
    case 83:
        if (Save_TestEventFlag(0x13A) == 1) {
            return 1;
        }
        break;
    case COND_HOUSE_SLEEP_EVENT:
        if (Save_TestEventFlag(EVENT_FLAG_HOUSE_SLEEP_EVENT) == 1) {
            return 1;
        }
        break;
    case 81:
        if (Save_TestEventFlag(0x138) == 1) {
            return 1;
        }
        break;
    case 80:
        if (Save_TestEventFlag(0x139) == 1) {
            return 1;
        }
        break;
    case 79:
        if (Save_TestEventFlag(0x136) == 1) {
            return 1;
        }
        break;
    case COND_ALWAYS:
        return 1;
    }
    return 0;
}


/* The hunter rank's conditions (enum VillageCond, from COND_RANK_FIRST):
 * an HR reached, an HR gate cleared, or both; 1 when it holds. */
s32 Cond_CheckRank(u16 id)
{
    switch (id) {
    case COND_HR_GATE_4:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_4) == 1) {
            return 1;
        }
        break;
    case 0x69:
        if (Save_IsQuestCleared(0x3AAE) && Save_IsQuestCleared(0x3AAF) && Save_IsQuestCleared(0x3AB0)) {
            return 1;
        }
        break;
    case COND_HR_50:
        if (gSave.hr >= 50) {
            return 1;
        }
        break;
    case COND_HR_GATE_3_HR_41:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_3) == 1 && gSave.hr >= 41) {
            return 1;
        }
        break;
    case COND_HR_GATE_3:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_3) == 1) {
            return 1;
        }
        break;
    case COND_HR_40:
        if (gSave.hr >= 40) {
            return 1;
        }
        break;
    case COND_HR_GATE_2_HR_31:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_2) == 1 && gSave.hr >= 31) {
            return 1;
        }
        break;
    case COND_HR_GATE_2:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_2) == 1) {
            return 1;
        }
        break;
    case COND_HR_30:
        if (gSave.hr >= 30) {
            return 1;
        }
        break;
    case 0x61:
        if (Save_TestEventFlag(0x85) == 1) {
            return 1;
        }
        break;
    case 0x60:
        if (gSave.hr >= 22 && Save_TestEventFlag(0x6E) == 1) {
            return 1;
        }
        break;
    case COND_HR_GATE_1_HR_17:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_1) == 1 && gSave.hr >= 17) {
            return 1;
        }
        break;
    case COND_HR_GATE_1:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_1) == 1) {
            return 1;
        }
        break;
    case COND_HR_16:
        if (gSave.hr >= 16) {
            return 1;
        }
        break;
    case COND_HR_GATE_0_HR_11:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_0) == 1 && gSave.hr >= 11) {
            return 1;
        }
        break;
    case COND_HR_GATE_0:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_0) == 1) {
            return 1;
        }
        break;
    case COND_HR_10:
        if (gSave.hr >= 10) {
            return 1;
        }
        break;
    case COND_HR_ANY:
        if (gSave.hr > 0) {
            return 1;
        }
        break;
    case COND_ALWAYS:
        return 1;
    }
    return 0;
}

/* 1 when the village's condition id (enum VillageCond) holds: COND_ALWAYS
 * always, the rank's conditions by Cond_CheckRank, the others by the event
 * flags (Cond_CheckEvent). */
s32 Cond_Check(u16 id)
{
    if (id == COND_ALWAYS) {
        return 1;
    }
    if (id >= COND_RANK_FIRST) {
        return Cond_CheckRank(id);
    }
    return Cond_CheckEvent(id);
}

/* 1 when village condition `id` holds and condition id + 1 does not: the
 * story stands at that condition's step (Cond_Check). 0 for id 0. */
s32 Cond_IsLatest(u16 id)
{
    if (id != COND_ALWAYS) {
        if (Cond_Check(id) == 1 && Cond_Check(id + 1) == 0) {
            return 1;
        }
    } else {
        return 0;
    }
    return 0;
}
