/*
 * The free townspeople's motion sounds (body 0, whose handler set is
 * gNpcCommonHandlers). Npc_MotionSounds plays a townsperson's sounds and
 * effects at frames of its motion (Npc_PlayTownSeAt for the town's sound
 * bank, Npc_PlayCommonSeAt for the shared bank, Npc_PlayVoice for its
 * voice), run each frame from its handler table (Npc_RunMotionSounds). The
 * townspeople with models of their own (the pets, the service and the
 * stationed townspeople) play theirs from their own MotionFx functions.
 * The file has no data of its own.
 */

#include "common.h"
#include "lobby_npc.h"
#include "em.h"
#include "player.h"
#include "lobby_menu.h"
#include "capsule.h"
#include "card.h"
#include "sound.h"
#include "engine_math.h"
#include "part_effects.h"
#include "rand.h"
#include "song_menu.h"
#include "town_players.h"
#include "hit_spark.h"
#include "area.h"
#include "vec_math.h"

s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);
extern s32 gTownMode;
void Npc_MotionSounds(Em *, NpcWork *);

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

/* A townsperson's sounds and effects by its layer-0 motion (Em.motionId[0]):
 * sounds at motion frames (Npc_PlayTownSeAt, Npc_PlayCommonSeAt, Npc_PlayVoice; some
 * picked at random by Sound_RandomSe), sparks in front of it (HitSpark_SpawnAt:
 * the blows of motion 415 and of 850-855) and an effect on the floor
 * (PartFx_SpawnAt), these two only while it is in the player's area.
 * Townsperson 1 (NpcWork.id) adds random sounds to its run and to motion 658. */
void Npc_MotionSounds(Em *npc, NpcWork *npcWork)
{
    f32 pos[3];

    switch (npc->motionId[0]) {
    case 1:
        break;
    case 2:
        Npc_PlayVoice(npc, 20.0f);
        Npc_PlayVoice(npc, 54.0f);
        Npc_PlayVoice(npc, 88.0f);
        break;
    case 3:
        Npc_PlayTownSeAt(npc, 24, 34);
        Npc_PlayTownSeAt(npc, 48, 34);
        Npc_PlayTownSeAt(npc, 72, 34);
        if (npcWork->id == 1) {
            Npc_PlayTownSeAt(npc, 70, Sound_RandomSe(184, 1, 184, 1));
            break;
        }
        break;
    case 64:
        Npc_PlayVoice(npc, 26.0f);
        Npc_PlayVoice(npc, 60.0f);
        break;
    case 415:
        Npc_PlayCommonSeAt(npc, 8, 61);
        Npc_PlayCommonSeAt(npc, 26, 63);
        Npc_PlayCommonSeAt(npc, 42, 47);
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
    case 611:
        Npc_PlayCommonSeAt(npc, 42, 81);
        Npc_PlayCommonSeAt(npc, 108, 83);
        break;
    case 612:
        Npc_PlayCommonSeAt(npc, 124, 75);
        Npc_PlayCommonSeAt(npc, 42, 61);
        Npc_PlayCommonSeAt(npc, 56, 82);
        Npc_PlayCommonSeAt(npc, 128, 83);
        break;
    case 627:
        Npc_PlayVoice(npc, 16.0f);
        Npc_PlayVoice(npc, 32.0f);
        Npc_PlayCommonSeAt(npc, 30, 49);
        Npc_PlayCommonSeAt(npc, 42, 49);
        break;
    case 658:
        if (npcWork->id == 1) {
            Npc_PlayTownSeAt(npc, 180, Sound_RandomSe(180, 1, 181, 1));
            break;
        }
        break;
    case 676:
        Npc_PlayTownSeAt(npc, 24, 20);
        Npc_PlayTownSeAt(npc, 60, 21);
        break;
    case 679:
        Npc_PlayTownSeAt(npc, 40, 169);
        Npc_PlayCommonSeAt(npc, 38, 67);
        Npc_PlayTownSeAt(npc, 30, 30);
        Npc_PlayTownSeAt(npc, 60, 31);
        Npc_PlayTownSeAt(npc, 110, 30);
        break;
    case 685:
        Npc_PlayCommonSeAt(npc, 4, 74);
        Npc_PlayTownSeAt(npc, 36, 61);
        break;
    case 691:
        Npc_PlayCommonSeAt(npc, 42, 49);
        Npc_PlayCommonSeAt(npc, 64, 49);
        Npc_PlayCommonSeAt(npc, 244, 49);
        Npc_PlayCommonSeAt(npc, 264, 49);
        Npc_PlayCommonSeAt(npc, 306, 49);
        Npc_PlayCommonSeAt(npc, 322, 49);
        Npc_PlayTownSeAt(npc, 80, Sound_RandomSe(62, 2, 62, 2));
        Npc_PlayTownSeAt(npc, 400, Sound_RandomSe(63, 2, 63, 1));
        break;
    case 694:
        Npc_PlayCommonSeAt(npc, 34, 75);
        Npc_PlayCommonSeAt(npc, 56, 75);
        Npc_PlayCommonSeAt(npc, 82, 75);
        break;
    case 254:
        Npc_PlayTownSeAt(npc, 66, 182);
        break;
    case 255:
        Npc_PlayTownSeAt(npc, 16, 179);
        Npc_PlayTownSeAt(npc, 110, 180);
        Npc_PlayTownSeAt(npc, 180, 181);
        break;
    case 256:
        Npc_PlayTownSeAt(npc, 16, Sound_RandomSe(183, 4, 183, 4));
        break;
    case 850:
        Npc_PlayTownSeAt(npc, 6, 167);
        Npc_PlayTownSeAt(npc, 10, 103);
        Npc_PlayTownSeAt(npc, 36, 102);
        break;
    case 851:
        Npc_PlayTownSeAt(npc, 8, 101);
        break;
    case 852:
        Npc_PlayTownSeAt(npc, 6, 167);
        Npc_PlayTownSeAt(npc, 10, 103);
        Npc_PlayTownSeAt(npc, 36, 102);
        break;
    case 853:
        Npc_PlayTownSeAt(npc, 8, 101);
        break;
    case 855:
        Npc_PlayTownSeAt(npc, 110, 99);
        Npc_PlayTownSeAt(npc, 198, 100);
        Npc_PlayTownSeAt(npc, 218, 92);
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
    case 861:
        Npc_PlayCommonSeAt(npc, 20, 61);
        Npc_PlayTownSeAt(npc, 24, 206);
        Npc_PlayCommonSeAt(npc, 80, 62);
        Npc_PlayTownSeAt(npc, 212, 224);
        break;
    case 862:
        Npc_PlayCommonSeAt(npc, 56, 75);
        Npc_PlayCommonSeAt(npc, 110, 81);
        Npc_PlayTownSeAt(npc, 232, 224);
        break;
    case 863:
        Npc_PlayCommonSeAt(npc, 44, 74);
        Npc_PlayTownSeAt(npc, 100, 205);
        break;
    case 864:
        Npc_PlayTownSeAt(npc, 4, 224);
        Npc_PlayCommonSeAt(npc, 52, 61);
        Npc_PlayTownSeAt(npc, 134, 204);
        Npc_PlayTownSeAt(npc, 268, 207);
        break;
    case 865:
        Npc_PlayTownSeAt(npc, 20, 20);
        Npc_PlayTownSeAt(npc, 60, 21);
        break;
    case 869:
        Npc_PlayCommonSeAt(npc, 14, 67);
        Npc_PlayTownSeAt(npc, 16, 132);
        Npc_PlayCommonSeAt(npc, 22, 7);
        break;
    case 870:
        Npc_PlayCommonSeAt(npc, 54, 7);
        Npc_PlayTownSeAt(npc, 92, 35);
        break;
    case 871:
        Npc_PlayTownSeAt(npc, 18, 132);
        Npc_PlayTownSeAt(npc, 20, 35);
        Npc_PlayCommonSeAt(npc, 24, 7);
        break;
    case 872:
        Npc_PlayTownSeAt(npc, 20, 34);
        Npc_PlayTownSeAt(npc, 58, 35);
        break;
    case 873:
        Npc_PlayTownSeAt(npc, 16, 34);
        Npc_PlayTownSeAt(npc, 44, 35);
        break;
    case 874:
        Npc_PlayTownSeAt(npc, 8, 34);
        Npc_PlayTownSeAt(npc, 36, 35);
        Npc_PlayTownSeAt(npc, 68, 35);
        break;
    case 875:
        Npc_PlayTownSeAt(npc, 210, Sound_RandomSe(185, 2, 186, 1));
        break;
    case 879:
        Npc_PlayTownSeAt(npc, 16, 32);
        Npc_PlayTownSeAt(npc, 50, 33);
        Npc_PlayTownSeAt(npc, 90, 32);
        break;
    case 882:
        Npc_PlayTownSeAt(npc, 20, 115);
        break;
    case 883:
        Npc_PlayTownSeAt(npc, 12, 112);
        Npc_PlayCommonSeAt(npc, 108, 83);
        Npc_PlayCommonSeAt(npc, 110, 83);
        Npc_PlayCommonSeAt(npc, 126, 83);
        Npc_PlayCommonSeAt(npc, 230, 99);
        Npc_PlayCommonSeAt(npc, 280, 99);
        break;
    case 884:
        Npc_PlayTownSeAt(npc, 30, 113);
        Npc_PlayCommonSeAt(npc, 94, 83);
        Npc_PlayCommonSeAt(npc, 180, 67);
        break;
    case 885:
        Npc_PlayTownSeAt(npc, 26, 114);
        Npc_PlayCommonSeAt(npc, 32, 49);
        Npc_PlayTownSeAt(npc, 54, 112);
        Npc_PlayCommonSeAt(npc, 60, 63);
        Npc_PlayCommonSeAt(npc, 108, 62);
        Npc_PlayCommonSeAt(npc, 130, 63);
        Npc_PlayTownSeAt(npc, 172, 116);
        Npc_PlayCommonSeAt(npc, 172, 83);
        Npc_PlayCommonSeAt(npc, 190, 99);
        Npc_PlayCommonSeAt(npc, 234, 99);
        Npc_PlayCommonSeAt(npc, 222, 55);
        break;
    case 886:
        Npc_PlayCommonSeAt(npc, 18, 45);
        Npc_PlayTownSeAt(npc, 78, 115);
        Npc_PlayTownSeAt(npc, 98, 116);
        Npc_PlayCommonSeAt(npc, 142, 83);
        Npc_PlayCommonSeAt(npc, 168, 83);
        Npc_PlayCommonSeAt(npc, 194, 45);
        Npc_PlayTownSeAt(npc, 230, 117);
        Npc_PlayCommonSeAt(npc, 320, 45);
        Npc_PlayTownSeAt(npc, 422, 118);
        Npc_PlayTownSeAt(npc, 458, 114);
        Npc_PlayTownSeAt(npc, 586, 116);
        Npc_PlayTownSeAt(npc, 636, 117);
        break;
    case 887:
        Npc_PlayCommonSeAt(npc, 2, 61);
        Npc_PlayCommonSeAt(npc, 4, 58);
        Npc_PlayCommonSeAt(npc, 86, 74);
        Npc_PlayTownSeAt(npc, 100, 114);
        Npc_PlayCommonSeAt(npc, 200, 100);
        Npc_PlayCommonSeAt(npc, 226, 49);
        Npc_PlayCommonSeAt(npc, 266, 49);
        Npc_PlayCommonSeAt(npc, 306, 49);
        Npc_PlayTownSeAt(npc, 370, 119);
        break;
    case 888:
        Npc_PlayCommonSeAt(npc, 2, 45);
        Npc_PlayTownSeAt(npc, 82, 121);
        Npc_PlayCommonSeAt(npc, 210, 63);
        Npc_PlayCommonSeAt(npc, 286, 66);
        Npc_PlayTownSeAt(npc, 206, 120);
        Npc_PlayCommonSeAt(npc, 588, 58);
        Npc_PlayCommonSeAt(npc, 620, 58);
        Npc_PlayCommonSeAt(npc, 702, 58);
        Npc_PlayCommonSeAt(npc, 722, 58);
        Npc_PlayCommonSeAt(npc, 738, 58);
        Npc_PlayCommonSeAt(npc, 770, 58);
        Npc_PlayCommonSeAt(npc, 834, 45);
        if ((EmMotion_PassedFrameScaled((Player *) npc, 290.0f, 0) != 0) && (Player_InCurrentArea((Player *) npc) & 0xFF)) {
            Em_BonePos(npc, 2, pos);
            pos[1] = npc->floorY;
            PartFx_SpawnAt(pos, 7, 1.0f);
        }
        break;
    }
}

/* The motion-effects handler of gNpcCommonHandlers (body 0, and the looks 60-62,
 * 75 and 76 on any body: Npc_SetupDraw), called each frame by Npc_RunMotionFx:
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
