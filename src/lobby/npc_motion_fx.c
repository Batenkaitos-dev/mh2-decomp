/* The motion effects of the townspeople with models of their own: the pets
 * (body 3), the service townspeople (body 2) and the stationed townspeople
 * (body 1). Each body's handler table runs its MotionFx every frame, which
 * plays sounds and effects on given frames of the townsperson's current
 * motion (npc_motion.h names the motions; the free townspeople's are
 * Npc_MotionSounds'). Sounds of the town's own bank go through
 * Npc_PlayTownSeAt, the shared bank's through Npc_PlayCommonSeAt; their
 * numbers stay numbers, since the code doesn't show what they are. */
#include "common.h"
#include "lobby_state.h"
#include "scene.h"
#include "lobby_globals.h"
#include "save.h"
#include "player.h"
#include "em.h"
#include "lobby_npc.h"
#include "quest.h"
#include "npc_motion.h"
#include "lobby_menu.h"
#include "shadow.h"
#include "capsule.h"
#include "card.h"
#include "sound.h"
#include "engine_math.h"
#include "mine_model_objects.h"
#include "part_effects.h"
#include "hit_spark.h"
#include "area.h"
#include "vec_math.h"

void StationedNpc_MotionFx(Em *npc, NpcWork *work);
void ServiceNpc_MotionFx(Em *npc, NpcWork *work);
void Pet_MotionFx(Em *npc, NpcWork *work);
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);

/* The pets' motion effects for a motion that has none. */
static void Pet_NoMotionFx(Em *npc)
{
}

void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale);

/* A pet's sounds and effects by its motion: sounds on given frames, part
 * effects as its charge and its run pass given frames, and while it sits,
 * from frame 60, three small effects at its bone 11 every 90 frames.
 * Npc_PlayTownSeAt(npc, frame, sound) plays a sound as the motion passes
 * the frame; Sound_RandomSe draws one of two sounds or none. */
void Pet_MotionFx(Em *npc, NpcWork *work)
{
    f32 ofs[3];

    switch (npc->motionId[0]) {
    case EM_MOTION_ID(0, PET_MOT_IDLE):
        Npc_PlayTownSeAt(npc, 16, Sound_RandomSe(3, 1, 0, 1));
        Npc_PlayTownSeAt(npc, 76, Sound_RandomSe(3, 1, 0, 1));
        break;
    case EM_MOTION_ID(0, PET_MOT_TURN_TO_PLAYER):
        Npc_PlayTownSeAt(npc, 12, Sound_RandomSe(3, 2, 3, 2));
        Npc_PlayTownSeAt(npc, 6, 1);
        Npc_PlayTownSeAt(npc, 16, 1);
        Npc_PlayTownSeAt(npc, 36, 1);
        break;
    case EM_MOTION_ID(0, PET_MOT_WALK):
        Npc_PlayTownSeAt(npc, 12, Sound_RandomSe(3, 2, 3, 2));
        Npc_PlayTownSeAt(npc, 38, Sound_RandomSe(3, 2, 3, 2));
        Npc_PlayTownSeAt(npc, 6, 2);
        Npc_PlayTownSeAt(npc, 30, 2);
        Npc_PlayTownSeAt(npc, 44, 2);
        break;
    case EM_MOTION_ID(0, PET_MOT_STEP_BACK):
        Npc_PlayTownSeAt(npc, 2, Sound_RandomSe(4, 6, 3, 2));
        Npc_PlayTownSeAt(npc, 64, Sound_RandomSe(4, 6, 3, 2));
        Npc_PlayTownSeAt(npc, 96, Sound_RandomSe(3, 6, 4, 2));
        Npc_PlayTownSeAt(npc, 128, Sound_RandomSe(4, 6, 3, 2));
        Npc_PlayTownSeAt(npc, 160, Sound_RandomSe(3, 6, 4, 2));
        Npc_PlayTownSeAt(npc, 22, 1);
        Npc_PlayTownSeAt(npc, 116, 1);
        break;
    case EM_MOTION_ID(0, PET_MOT_JOY):
        Npc_PlayTownSeAt(npc, 22, 4);
        Npc_PlayTownSeAt(npc, 10, 1);
        Npc_PlayTownSeAt(npc, 82, 1);
        break;
    case EM_MOTION_ID(0, PET_MOT_CHARGE):
        Npc_PlayTownSeAt(npc, 30, 6);
        Npc_PlayTownSeAt(npc, 22, 6);
        if (EmMotion_PassedFrameScaled(npc, 66.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(npc, 6, 3, 0.7f);
        }
        break;
    case EM_MOTION_ID(0, PET_MOT_RUN):
        Npc_PlayTownSeAt(npc, 10, 6);
        Npc_PlayTownSeAt(npc, 24, 2);
        Npc_PlayTownSeAt(npc, 28, 2);
        Npc_PlayTownSeAt(npc, 36, 2);
        if (EmMotion_PassedFrameScaled(npc, 16.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(npc, 4, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(npc, 30.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(npc, 17, 3, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, PET_MOT_TACKLE):
        Npc_PlayTownSeAt(npc, 20, 6);
        Npc_PlayTownSeAt(npc, 24, 5);
        Npc_PlayTownSeAt(npc, 42, 2);
        Npc_PlayTownSeAt(npc, 46, 2);
        break;
    case EM_MOTION_ID(0, PET_MOT_SIT):
        if (EmMotion_ReachedFrame(npc, 0, 60.0f) != 0) {
            switch (gScene.frame % 90) {
            case 0:
            case 10:
            case 20:
                ofs[2] = 15.0f;
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                Effect06_SpawnAt(npc, 4, 11, ofs, 0.4f);
                break;
            }
        }
    default:
        Pet_NoMotionFx(npc);
        break;
    }
}

/* The pets' handler table entry motionFx: the first frame only steps
 * fxStep, after that Pet_MotionFx runs each frame. */
void Pet_RunMotionFx(Em *npc)
{
    NpcWork *work = (NpcWork *)&npc->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        Pet_MotionFx(npc, work);
        break;
    }
}

/* The pets' handler table entries init, update, unk08 and appear: the shadow. */
void Pet_SpawnShadow(Em *npc)
{
    Shadow_Spawn(npc, SHADOW_MODEL);
}

/* The service townspeople's motion effects for a motion that has none. */
static void ServiceNpc_NoMotionFx(Em *npc)
{
}

extern s32 gTownMode;

/* A service townsperson's sounds and effects by its motion. In the hair
 * menu (TOWN_MODE_HAIR_MENU) every one but townsperson 111 plays other
 * sounds, of the shared bank, for its walk, the hair menu's preparing, its
 * finish and the haircut. Look 82's idle spawns part effects at its bone 9,
 * at the floor's height, on given frames. */
void ServiceNpc_MotionFx(Em *npc, NpcWork *work)
{
    f32 ofs[3];

    switch (npc->motionId[0]) {
    case EM_MOTION_ID(0, SERVICE_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_WAIT):
        Npc_PlayTownSeAt(npc, 18, 11);
        Npc_PlayTownSeAt(npc, 38, 11);
        Npc_PlayTownSeAt(npc, 114, 11);
        Npc_PlayTownSeAt(npc, 126, 11);
        Npc_PlayTownSeAt(npc, 152, 11);
        Npc_PlayTownSeAt(npc, 8, 8);
        Npc_PlayTownSeAt(npc, 88, 8);
        Npc_PlayTownSeAt(npc, 208, 8);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_STEP_IN_PLACE):
        Npc_PlayTownSeAt(npc, 38, 10);
        Npc_PlayTownSeAt(npc, 6, 8);
        Npc_PlayTownSeAt(npc, 22, 8);
        Npc_PlayTownSeAt(npc, 154, 8);
        Npc_PlayTownSeAt(npc, 164, 8);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_FIDGET):
        break;
    case EM_MOTION_ID(0, 5):
        Npc_PlayTownSeAt(npc, 6, 16);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_WALK):
        Npc_PlayTownSeAt(npc, 80, Sound_RandomSe(10, 2, 10, 2));
        Npc_PlayTownSeAt(npc, 12, 8);
        Npc_PlayTownSeAt(npc, 30, 8);
        Npc_PlayTownSeAt(npc, 46, 8);
        Npc_PlayTownSeAt(npc, 66, 8);
        Npc_PlayTownSeAt(npc, 88, 8);
        Npc_PlayTownSeAt(npc, 118, 8);
        Npc_PlayTownSeAt(npc, 140, 8);
        Npc_PlayTownSeAt(npc, 152, 8);
        Npc_PlayTownSeAt(npc, 162, 8);
        Npc_PlayTownSeAt(npc, 174, 8);
        Npc_PlayTownSeAt(npc, 184, 8);
        Npc_PlayTownSeAt(npc, 194, 8);
        Npc_PlayTownSeAt(npc, 174, 8);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_WALK_ALT):
        if (gTownMode == TOWN_MODE_HAIR_MENU) {
            if (work->id == 111) {
                Npc_PlayTownSeAt(npc, 8, 18);
                Npc_PlayTownSeAt(npc, 10, 8);
                Npc_PlayTownSeAt(npc, 22, 8);
                Npc_PlayTownSeAt(npc, 32, 8);
                Npc_PlayTownSeAt(npc, 46, 8);
                Npc_PlayTownSeAt(npc, 56, 8);
                Npc_PlayTownSeAt(npc, 68, 8);
                Npc_PlayTownSeAt(npc, 78, 8);
                Npc_PlayTownSeAt(npc, 88, 8);
                Npc_PlayTownSeAt(npc, 100, 8);
                Npc_PlayTownSeAt(npc, 112, 8);
                break;
            }
            Npc_PlayCommonSeAt(npc, 8, 178);
            Npc_PlayTownSeAt(npc, 10, 8);
            Npc_PlayTownSeAt(npc, 22, 8);
            Npc_PlayTownSeAt(npc, 32, 8);
            Npc_PlayTownSeAt(npc, 46, 8);
            Npc_PlayTownSeAt(npc, 56, 8);
            Npc_PlayTownSeAt(npc, 68, 8);
            Npc_PlayTownSeAt(npc, 78, 8);
            Npc_PlayTownSeAt(npc, 88, 8);
            Npc_PlayTownSeAt(npc, 100, 8);
            Npc_PlayTownSeAt(npc, 112, 8);
            break;
        }
        Npc_PlayTownSeAt(npc, 8, 18);
        Npc_PlayTownSeAt(npc, 10, 8);
        Npc_PlayTownSeAt(npc, 22, 8);
        Npc_PlayTownSeAt(npc, 32, 8);
        Npc_PlayTownSeAt(npc, 46, 8);
        Npc_PlayTownSeAt(npc, 56, 8);
        Npc_PlayTownSeAt(npc, 68, 8);
        Npc_PlayTownSeAt(npc, 78, 8);
        Npc_PlayTownSeAt(npc, 88, 8);
        Npc_PlayTownSeAt(npc, 100, 8);
        Npc_PlayTownSeAt(npc, 112, 8);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_WALK_CARRY):
        Npc_PlayTownSeAt(npc, 10, 8);
        Npc_PlayTownSeAt(npc, 10, 19);
        Npc_PlayTownSeAt(npc, 24, 8);
        Npc_PlayTownSeAt(npc, 38, 8);
        Npc_PlayTownSeAt(npc, 50, 8);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_HAIR_PREPARE):
        if (gTownMode == TOWN_MODE_HAIR_MENU) {
            if (work->id == 111) {
                Npc_PlayTownSeAt(npc, 2, 12);
                break;
            }
            Npc_PlayCommonSeAt(npc, 2, 177);
            Npc_PlayCommonSeAt(npc, 10, 179);
            break;
        }
        Npc_PlayTownSeAt(npc, 2, 12);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_FINISH):
        if (gTownMode == TOWN_MODE_HAIR_MENU) {
            if (work->id == 111) {
                Npc_PlayTownSeAt(npc, 18, Sound_RandomSe(12, 4, 13, 4));
                Npc_PlayTownSeAt(npc, 60, Sound_RandomSe(14, 4, 14, 4));
                break;
            }
            Npc_PlayCommonSeAt(npc, 18, Sound_RandomSe(178, 4, 176, 4));
            Npc_PlayCommonSeAt(npc, 60, Sound_RandomSe(177, 4, 177, 4));
            break;
        }
        Npc_PlayTownSeAt(npc, 18, Sound_RandomSe(12, 4, 13, 4));
        Npc_PlayTownSeAt(npc, 60, Sound_RandomSe(14, 4, 14, 4));
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_TALK):
        Npc_PlayTownSeAt(npc, 4, 16);
        Npc_PlayTownSeAt(npc, 256, 11);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_GREET):
        Npc_PlayTownSeAt(npc, 16, 8);
        Npc_PlayTownSeAt(npc, 18, 8);
        break;
    case EM_MOTION_ID(0, 72):
        Npc_PlayTownSeAt(npc, 48, 12);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_BUSY):
        Npc_PlayTownSeAt(npc, 10, 10);
        break;
    case EM_MOTION_ID(0, 74):
        if (gTownMode == TOWN_MODE_HAIR_MENU) {
            if (work->id == 111) {
                Npc_PlayTownSeAt(npc, 52, 9);
                Npc_PlayCommonSeAt(npc, 68, 168);
                Npc_PlayTownSeAt(npc, 22, 8);
                Npc_PlayTownSeAt(npc, 30, 8);
                Npc_PlayTownSeAt(npc, 128, 8);
                Npc_PlayTownSeAt(npc, 148, 8);
                break;
            }
            Npc_PlayCommonSeAt(npc, 52, 175);
            Npc_PlayCommonSeAt(npc, 68, 168);
            Npc_PlayTownSeAt(npc, 22, 8);
            Npc_PlayTownSeAt(npc, 30, 8);
            Npc_PlayTownSeAt(npc, 128, 8);
            Npc_PlayTownSeAt(npc, 148, 8);
            break;
        }
        Npc_PlayTownSeAt(npc, 8, 14);
        Npc_PlayTownSeAt(npc, 52, 9);
        Npc_PlayCommonSeAt(npc, 68, 172);
        Npc_PlayTownSeAt(npc, 22, 8);
        Npc_PlayTownSeAt(npc, 30, 8);
        Npc_PlayTownSeAt(npc, 128, 8);
        Npc_PlayTownSeAt(npc, 148, 8);
        break;
    case EM_MOTION_ID(0, 75):
        Npc_PlayTownSeAt(npc, 14, 15);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT):
        if (gTownMode == TOWN_MODE_HAIR_MENU) {
            if (work->id == 111) {
                Npc_PlayTownSeAt(npc, 26, 9);
                Npc_PlayTownSeAt(npc, 80, 8);
                Npc_PlayCommonSeAt(npc, 122, 169);
                Npc_PlayTownSeAt(npc, 180, 12);
                Npc_PlayCommonSeAt(npc, 186, 61);
                Npc_PlayCommonSeAt(npc, 224, 170);
                Npc_PlayCommonSeAt(npc, 230, 171);
                break;
            }
            Npc_PlayCommonSeAt(npc, 26, 177);
            Npc_PlayCommonSeAt(npc, 80, 176);
            Npc_PlayCommonSeAt(npc, 122, 169);
            Npc_PlayCommonSeAt(npc, 180, 175);
            Npc_PlayCommonSeAt(npc, 186, 61);
            Npc_PlayCommonSeAt(npc, 224, 170);
            Npc_PlayCommonSeAt(npc, 230, 171);
            break;
        }
        Npc_PlayCommonSeAt(npc, 26, 177);
        Npc_PlayCommonSeAt(npc, 80, 176);
        Npc_PlayCommonSeAt(npc, 122, 169);
        Npc_PlayCommonSeAt(npc, 180, 175);
        Npc_PlayCommonSeAt(npc, 186, 61);
        Npc_PlayCommonSeAt(npc, 224, 170);
        Npc_PlayCommonSeAt(npc, 230, 171);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT_NEXT):
        if (gTownMode == TOWN_MODE_HAIR_MENU) {
            if (work->id == 111) {
                Npc_PlayCommonSeAt(npc, 4, 170);
                Npc_PlayCommonSeAt(npc, 10, 171);
                Npc_PlayCommonSeAt(npc, 20, 170);
                Npc_PlayCommonSeAt(npc, 28, 171);
                Npc_PlayCommonSeAt(npc, 38, 170);
                Npc_PlayCommonSeAt(npc, 54, 170);
                Npc_PlayTownSeAt(npc, 62, 12);
                Npc_PlayCommonSeAt(npc, 66, 61);
                Npc_PlayTownSeAt(npc, 84, 8);
                break;
            }
            Npc_PlayCommonSeAt(npc, 4, 170);
            Npc_PlayCommonSeAt(npc, 10, 171);
            Npc_PlayCommonSeAt(npc, 20, 170);
            Npc_PlayCommonSeAt(npc, 28, 171);
            Npc_PlayCommonSeAt(npc, 38, 170);
            Npc_PlayCommonSeAt(npc, 54, 170);
            Npc_PlayCommonSeAt(npc, 62, 175);
            Npc_PlayCommonSeAt(npc, 66, 61);
            Npc_PlayTownSeAt(npc, 84, 8);
            break;
        }
        Npc_PlayCommonSeAt(npc, 4, 170);
        Npc_PlayCommonSeAt(npc, 10, 171);
        Npc_PlayCommonSeAt(npc, 20, 170);
        Npc_PlayCommonSeAt(npc, 28, 171);
        Npc_PlayCommonSeAt(npc, 38, 170);
        Npc_PlayCommonSeAt(npc, 54, 170);
        Npc_PlayTownSeAt(npc, 62, 12);
        Npc_PlayCommonSeAt(npc, 66, 61);
        Npc_PlayTownSeAt(npc, 84, 8);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_HAIR_DONE):
        if (gTownMode == TOWN_MODE_HAIR_MENU) {
            if (work->id == 111) {
                Npc_PlayTownSeAt(npc, 16, 8);
                Npc_PlayTownSeAt(npc, 20, 16);
                break;
            }
            Npc_PlayTownSeAt(npc, 16, 8);
            Npc_PlayCommonSeAt(npc, 20, 174);
            break;
        }
        Npc_PlayTownSeAt(npc, 16, 8);
        Npc_PlayTownSeAt(npc, 20, 16);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_ALT_WALK):
        Npc_PlayTownSeAt(npc, 8, 8);
        Npc_PlayTownSeAt(npc, 22, 8);
        Npc_PlayTownSeAt(npc, 34, 8);
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_ALT_IDLE):
        Npc_PlayTownSeAt(npc, 26, 82);
        Npc_PlayTownSeAt(npc, 82, 82);
        Npc_PlayTownSeAt(npc, 140, 83);
        Npc_PlayTownSeAt(npc, 170, 82);
        Npc_PlayTownSeAt(npc, 202, 83);
        if (EmMotion_PassedFrameScaled(npc, 32.0f, 0) != 0 || EmMotion_PassedFrameScaled(npc, 92.0f, 0) != 0) {
            Em_BonePos(npc, 9, ofs);
            ofs[1] = npc->floorY;
            if (Player_InCurrentArea(npc) != 0) {
                PartFx_SpawnAt(ofs, 20, 1.5f);
            }
        }
        if (EmMotion_PassedFrameScaled(npc, 146.0f, 0) != 0 || EmMotion_PassedFrameScaled(npc, 174.0f, 0) != 0 || EmMotion_PassedFrameScaled(npc, 208.0f, 0) != 0) {
            Em_BonePos(npc, 9, ofs);
            ofs[1] = npc->floorY;
            if (Player_InCurrentArea(npc) != 0) {
                PartFx_SpawnAt(ofs, 4, 1.0f);
            }
        }
        break;
    case EM_MOTION_ID(0, SERVICE_MOT_GREET_EFFECT):
        Npc_PlayTownSeAt(npc, 40, 16);
        break;
    default:
        ServiceNpc_NoMotionFx(npc);
        break;
    }
}

/* The service townspeople's handler table entry motionFx: the first frame
 * only steps fxStep, after that ServiceNpc_MotionFx runs each frame. */
void ServiceNpc_RunMotionFx(Em *npc)
{
    NpcWork *work = (NpcWork *)&npc->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        ServiceNpc_MotionFx(npc, work);
        break;
    }
}

/* The service townspeople's handler table entries init and update: the shadow. */
void ServiceNpc_SpawnShadow(Em *npc)
{
    Shadow_Spawn(npc, SHADOW_MODEL);
}

/* An empty handler: the service townspeople's entries unk08 and appear. */
void ServiceNpc_NopHandler(void)
{

}

/* Once the motion passes the frame, and the townsperson is in the current
 * area: a hit spark (kind 7) at its position plus ofs turned by its
 * rotation. The smith's hammer strikes it on the anvil. */
void StationedNpc_SparkAtFrame(Em *npc, f32 frame, f32 *ofs)
{
    f32 pos[3];
    f32 rotated[3];

    if (EmMotion_PassedFrameScaled(npc, frame, 0) != 0 && Player_InCurrentArea(npc) == 1) {
        Vec_Rotate(rotated, ofs, &npc->worldMtx[0][0]);
        pos[0] = npc->posX + rotated[0];
        pos[1] = npc->posY + rotated[1];
        pos[2] = npc->posZ + rotated[2];
        HitSpark_SpawnAt(pos, 7, (u16)npc->rotY);
    }
}

/* A stationed townsperson's sounds and effects by its motion; then the
 * smith's own sounds (NPC_WORKSHOP_SMITH). The forging's strikes spark on
 * the anvil, the busy fidget spawns an effect and model objects. The
 * workshop helpers' switches have empty cases only. Rows 2-4 here come
 * from look 77, a model of its own that Npc_SetupDraw gives this handler
 * table: its idle's fidget, its talk's start and its talk's end. */
void StationedNpc_MotionFx(Em *npc, NpcWork *work)
{
    f32 ofs[3];

    switch (npc->motionId[0]) {
    case EM_MOTION_ID(0, STATIONED_MOT_TALK_START):
        Npc_PlayTownSeAt(npc, 10, 38);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_TALK):
        Npc_PlayTownSeAt(npc, 42, Sound_RandomSe(79, 2, 79, 1));
        Npc_PlayTownSeAt(npc, 106, Sound_RandomSe(81, 1, 80, 2));
        Npc_PlayTownSeAt(npc, 180, Sound_RandomSe(81, 2, 80, 1));
        Npc_PlayTownSeAt(npc, 238, Sound_RandomSe(40, 3, 41, 1));
        Npc_PlayTownSeAt(npc, 350, Sound_RandomSe(40, 1, 41, 3));
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_WALK):
        Npc_PlayTownSeAt(npc, 38, Sound_RandomSe(42, 1, 43, 1));
        Npc_PlayTownSeAt(npc, 108, Sound_RandomSe(44, 1, 42, 1));
        Npc_PlayTownSeAt(npc, 12, 36);
        Npc_PlayTownSeAt(npc, 44, 37);
        Npc_PlayTownSeAt(npc, 76, 36);
        Npc_PlayTownSeAt(npc, 106, 37);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE):
        Npc_PlayTownSeAt(npc, 66, 45);
        Npc_PlayTownSeAt(npc, 194, 46);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK_START):
        Npc_PlayTownSeAt(npc, 14, 48);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK):
        Npc_PlayTownSeAt(npc, 14, Sound_RandomSe(47, 2, 80, 2));
        Npc_PlayTownSeAt(npc, 142, Sound_RandomSe(79, 2, 79, 2));
        Npc_PlayTownSeAt(npc, 206, Sound_RandomSe(81, 1, 80, 3));
        Npc_PlayTownSeAt(npc, 280, Sound_RandomSe(81, 3, 80, 1));
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP):
        Npc_PlayTownSeAt(npc, 52, 36);
        Npc_PlayTownSeAt(npc, 82, 36);
        Npc_PlayTownSeAt(npc, 148, 49);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP_TALK_START):
        Npc_PlayTownSeAt(npc, 4, 38);
        Npc_PlayTownSeAt(npc, 58, 47);
        Npc_PlayTownSeAt(npc, 94, 36);
        Npc_PlayTownSeAt(npc, 120, 48);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_RUN):
        Npc_PlayTownSeAt(npc, 8, 37);
        Npc_PlayTownSeAt(npc, 16, 37);
        Npc_PlayTownSeAt(npc, 26, 37);
        Npc_PlayTownSeAt(npc, 38, 37);
        break;
    case EM_MOTION_ID(0, 22):
        Npc_PlayTownSeAt(npc, 8, 37);
        Npc_PlayTownSeAt(npc, 18, 37);
        Npc_PlayTownSeAt(npc, 28, 37);
        Npc_PlayTownSeAt(npc, 38, 37);
        Npc_PlayTownSeAt(npc, 50, 37);
        Npc_PlayTownSeAt(npc, 60, 36);
        Npc_PlayTownSeAt(npc, 56, 43);
        Npc_PlayTownSeAt(npc, 130, 44);
        Npc_PlayTownSeAt(npc, 230, 77);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_START):
        Npc_PlayTownSeAt(npc, 50, 47);
        Npc_PlayTownSeAt(npc, 170, 43);
        Npc_PlayTownSeAt(npc, 226, 40);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK):
        Npc_PlayTownSeAt(npc, 14, Sound_RandomSe(47, 2, 80, 2));
        Npc_PlayTownSeAt(npc, 140, Sound_RandomSe(81, 2, 79, 2));
        Npc_PlayTownSeAt(npc, 354, Sound_RandomSe(42, 1, 44, 3));
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_BUSY_IDLE_FIDGET):
        if (EmMotion_PassedFrameScaled(npc, 84.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(npc, 13, 28, 1.0f);
        }
        if (EmMotion_InFrameRange(npc, 0, 38.0f, 198.0f) != 0 && gScene.frame % 10 == 0) {
            ModelObj_SpawnOn(npc, 2);
        }
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER):
        Vec_Set(ofs, 0.0f, 20.0f, 125.0f);
        StationedNpc_SparkAtFrame(npc, 96.0f, ofs);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_FORGE_LAST):
        Vec_Set(ofs, 0.0f, 20.0f, 125.0f);
        StationedNpc_SparkAtFrame(npc, 28.0f, ofs);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_WALK):
        Npc_PlayTownSeAt(npc, 12, 36);
        Npc_PlayTownSeAt(npc, 42, 37);
        Npc_PlayTownSeAt(npc, 74, 36);
        Npc_PlayTownSeAt(npc, 98, 37);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_TALK_START):
        Npc_PlayTownSeAt(npc, 16, 148);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_TALK):
        Npc_PlayTownSeAt(npc, 48, Sound_RandomSe(147, 2, 147, 2));
        Npc_PlayTownSeAt(npc, 314, Sound_RandomSe(148, 3, 148, 1));
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_IDLE_B):
        Npc_PlayTownSeAt(npc, 34, 46);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_IDLE_B_BREAK):
        Npc_PlayTownSeAt(npc, 14, 38);
        Npc_PlayTownSeAt(npc, 120, 192);
        Npc_PlayTownSeAt(npc, 220, 192);
        Npc_PlayTownSeAt(npc, 320, 192);
        Npc_PlayTownSeAt(npc, 386, 193);
        Npc_PlayTownSeAt(npc, 470, 42);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_IDLE_B_TALK_START):
        Npc_PlayTownSeAt(npc, 4, 40);
        Npc_PlayTownSeAt(npc, 66, 38);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM):
        Npc_PlayTownSeAt(npc, 4, 194);
        break;
    case EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM_BREAK):
        Npc_PlayTownSeAt(npc, 10, 195);
        break;
    case EM_MOTION_ID(0, 2):
        Npc_PlayTownSeAt(npc, 16, 184);
        Npc_PlayCommonSeAt(npc, 18, 2);
        Npc_PlayTownSeAt(npc, 54, 189);
        break;
    case EM_MOTION_ID(0, 3):
        Npc_PlayTownSeAt(npc, 4, 184);
        Npc_PlayTownSeAt(npc, 204, Sound_RandomSe(185, 3, 185, 2));
        Npc_PlayTownSeAt(npc, 406, Sound_RandomSe(187, 2, 187, 3));
        break;
    case EM_MOTION_ID(0, 4):
        Npc_PlayTownSeAt(npc, 44, 188);
        break;
    }
    if (work->id == NPC_WORKSHOP_SMITH) {
        switch (npc->motionId[0]) {
        case EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE_ALT):
            Npc_PlayTownSeAt(npc, 30, 125);
            Npc_PlayTownSeAt(npc, 132, 126);
            Npc_PlayTownSeAt(npc, 330, 124);
            Npc_PlayTownSeAt(npc, 330, 93);
            Npc_PlayTownSeAt(npc, 346, 94);
            Npc_PlayTownSeAt(npc, 380, 93);
            Npc_PlayTownSeAt(npc, 396, 94);
            Npc_PlayTownSeAt(npc, 560, 125);
            Npc_PlayTownSeAt(npc, 650, 126);
            Npc_PlayTownSeAt(npc, 778, 104);
            Npc_PlayTownSeAt(npc, 854, 137);
            Npc_PlayTownSeAt(npc, 894, 137);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_SMITH_PARTNER_CUE):
            Npc_PlayTownSeAt(npc, 6, 127);
            Npc_PlayTownSeAt(npc, 66, 128);
            Npc_PlayTownSeAt(npc, 168, 129);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_SMITH_WAIT_ABOVE):
            Npc_PlayTownSeAt(npc, 94, 131);
            Npc_PlayTownSeAt(npc, 130, 133);
            Npc_PlayTownSeAt(npc, 206, Sound_RandomSe(135, 3, 136, 3));
            Npc_PlayTownSeAt(npc, 274, 131);
            Npc_PlayTownSeAt(npc, 306, 133);
            Npc_PlayTownSeAt(npc, 338, Sound_RandomSe(136, 3, 135, 3));
            Npc_PlayTownSeAt(npc, 402, 134);
            Npc_PlayTownSeAt(npc, 580, 132);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_SMITH_TALK):
            Npc_PlayTownSeAt(npc, 20, Sound_RandomSe(136, 4, 135, 4));
            Npc_PlayTownSeAt(npc, 58, Sound_RandomSe(135, 3, 144, 3));
            Npc_PlayTownSeAt(npc, 226, Sound_RandomSe(137, 3, 135, 3));
            Npc_PlayTownSeAt(npc, 360, Sound_RandomSe(135, 3, 136, 3));
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_START):
            Npc_PlayTownSeAt(npc, 20, 136);
            Npc_PlayTownSeAt(npc, 54, 138);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER):
            Npc_PlayTownSeAt(npc, 30, 140);
            Npc_PlayTownSeAt(npc, 88, 96);
            Npc_PlayTownSeAt(npc, 98, 92);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_LAST):
            Npc_PlayTownSeAt(npc, 18, 96);
            Npc_PlayTownSeAt(npc, 138, 141);
            Npc_PlayTownSeAt(npc, 16, 96);
            Npc_PlayTownSeAt(npc, 28, 92);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_END):
            Npc_PlayCommonSeAt(npc, 8, 63);
            Npc_PlayTownSeAt(npc, 36, 146);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_SMITH_LEAVE):
            Npc_PlayTownSeAt(npc, 6, 143);
            Npc_PlayTownSeAt(npc, 20, 97);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_SMITH_ARRIVE_ABOVE):
            Npc_PlayTownSeAt(npc, 50, 144);
            Npc_PlayTownSeAt(npc, 24, 98);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_SMITH_ARRIVE_BELOW):
            Npc_PlayTownSeAt(npc, 40, 144);
            Npc_PlayTownSeAt(npc, 10, 98);
            break;
        }
    } else if (work->id == NPC_WORKSHOP_HELPER) {
        switch (npc->motionId[0]) {
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_START):
            Npc_PlayTownSeAt(npc, 54, 47);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER):
            Npc_PlayTownSeAt(npc, 30, 38);
            Npc_PlayTownSeAt(npc, 30, 38);
            Npc_PlayTownSeAt(npc, 88, 96);
            Npc_PlayTownSeAt(npc, 98, 92);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_LAST):
            Npc_PlayTownSeAt(npc, 16, 96);
            Npc_PlayTownSeAt(npc, 28, 92);
            Npc_PlayTownSeAt(npc, 30, 77);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_HELPER_TURN):
            Npc_PlayTownSeAt(npc, 4, 38);
            break;
        }
    } else if (work->id == NPC_WORKSHOP_HELPER_ONLINE) {
        switch (npc->motionId[0]) {
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_START):
            Npc_PlayTownSeAt(npc, 54, 47);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER):
            Npc_PlayTownSeAt(npc, 30, 38);
            Npc_PlayTownSeAt(npc, 30, 38);
            Npc_PlayTownSeAt(npc, 88, 96);
            Npc_PlayTownSeAt(npc, 98, 92);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_LAST):
            Npc_PlayTownSeAt(npc, 16, 96);
            Npc_PlayTownSeAt(npc, 28, 92);
            Npc_PlayTownSeAt(npc, 30, 77);
            break;
        case EM_MOTION_ID(0, STATIONED_MOT_FORGE_HELPER_TURN):
            Npc_PlayTownSeAt(npc, 4, 38);
            break;
        }
    }
}

/* The stationed townspeople's handler table entry motionFx: the first
 * frame only steps fxStep, after that StationedNpc_MotionFx runs each
 * frame. */
void StationedNpc_RunMotionFx(Em *npc)
{
    NpcWork *work = (NpcWork *)&npc->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        StationedNpc_MotionFx(npc, work);
        break;
    }
}

/* The stationed townspeople's handler table entries init and update: the shadow. */
void StationedNpc_SpawnShadow(Em *npc)
{
    Shadow_Spawn(npc, SHADOW_MODEL);
}

/* An empty handler: the stationed townspeople's entries unk08 and appear. */
void StationedNpc_NopHandler(void)
{

}
