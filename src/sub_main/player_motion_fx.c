/*
 * The sounds and effects of the hunter's motions.
 *
 * Every frame the player's last handler, Player_MotionFxStep, runs
 * Player_MotionEffects and then Player_MotionSounds on the player's work
 * area (Player.work, a PlayerWork). Both switch on the motion of layer 0
 * (PlayerWork.motion; the player's motion ids have no names yet): first the
 * motions every weapon shares, then those of the weapon type in hand
 * (Player.weaponType). For each motion they list what starts at which of its
 * frames; the helpers fire once the motion passes the frame
 * (Motion_PassedFrame):
 * - the sounds: the voice (PlayerSe_VoiceAtFrame: voice n plays one of the
 *   pair 2n, 2n + 1 of the voice bank, at random), the armour's rattle
 *   (PlayerSe_EquipAtFrame: EQUIP_RATTLE_ALL or one armour type), the
 *   weapon's sounds (PlayerSe_WeaponAtFrame, only offline) and the player's
 *   other sounds (PlayerSe_AtFrame and the rest), some picked at random by
 *   Sound_RandomSe;
 * - the effects: the ground effects at a bone (Player_EffectAtFrame,
 *   PartFx_*), sparks and stage objects; and first the glow of an item
 *   another hunter of the same side shared with this one.
 * Numbers left as numbers: the motion ids, the frames, the bones, and the
 * sound ids of the player's sound bank.
 *
 * The file begins with the sound helpers (PlayerSe_AtFrame to
 * PlayerSe_EquipAtFrame, which the player's actions call at hundreds of
 * places as well): when the motion of layer 0 passes a frame, a sound is
 * played at the player's position, from the shared bank, the player's own
 * bank, the voice bank (two takes per voice, picked at random) or the
 * armour's rattle. The effects go through PartFx_SpawnForPlayer and the
 * ground effects.
 */
#include "common.h"
#include "stageobj.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "capsule.h"
#include "card.h"
#include "sound.h"
#include "bone_effects.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "hit_effects.h"
#include "mine_model_objects.h"
#include "rand.h"
#include "hit_spark.h"
#include "area.h"
#include "vec_math.h"
#include "save.h"

s16 Player_IsState(void *obj, u8 state, u8 sub);
/* Player_EffectAtFrame's calls compile as the game's only with these s32 and
 * u8 parameters; the definition takes (Player *, s16 bone, s32 type). */
/* unit's own declaration: kept for the match */
void PartFx_SpawnForPlayer(Player *p, s32 a, u8 b);

/* The walls a player touches (gEmWallContacts, by Player.index), as
 * Motion_VoiceAtFrame reads them. */
typedef struct WallContactRow {
    WallContact e[WALL_CONTACTS];
} WallContactRow;

extern WallContactRow gEmWallContacts[];

/* Plays sound se of the shared bank at the player when its motion passes
 * frame. */
void PlayerSe_AtFrame(Player *pl, s32 frame, s32 seId)
{
    if (Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayPlayerSe(pl, seId, 0, pl->pos, 1, 0);
    }
}

/* The same with the distance curve 23. */
void Motion_SeAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayPlayerSe(pl, seId, 0, pl->pos, 23, 0);
    }
}

/* As PlayerSe_AtFrame, but only for the local player and the remote
 * players of even index. */
void Motion_SeAtFrameLocal(Player *pl, s32 frame, s32 seId)
{
    if (Player_IsLocal(pl) == 0 && (pl->index & 1)) {
        return;
    }
    if (Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayPlayerSe(pl, seId, 0, pl->pos, 1, 0);
    }
}

/* Outside the town: plays sound se of the player's own bank (distance curve 26)
 * when its motion passes frame, unless its weapon sounds are muted. */
void Player_SeAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (pl->weaponSeMute == 0 && gScene.inLobby == 0 && Motion_PassedFrame(pl, 0, frame)) {
        Player_PlaySe(pl, seId, 0, pl->pos, 26, 0);
    }
}

/* The same with the distance curve 1. */
void PlayerSe_WeaponAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (pl->weaponSeMute == 0 && gScene.inLobby == 0 && Motion_PassedFrame(pl, 0, frame)) {
        Player_PlaySe(pl, seId, 0, pl->pos, 1, 0);
    }
}

/*
 * Plays voice voice (one of its two takes, at random) from the voice bank
 * when the player, in the current area, passes frame. Against a wall
 * (Player.wallHitDirs), the sound argument of the first wall contact that
 * has one goes with it.
 */
void Motion_VoiceAtFrame(Player *pl, f32 frame, s32 voice)
{
    s16 wallArg = 0;
    s32 i;

    if ((u8)Obj_InCurrentArea(pl) && Motion_PassedFrame(pl, 0, frame)) {
        if (pl->wallHitDirs & 0xE0000007) {
            for (i = 0; i < 10; i++) {
                if (gEmWallContacts[pl->index].e[i].seArg != 0 && wallArg == 0) {
                    wallArg = gEmWallContacts[pl->index].e[i].seArg;
                }
            }
        }
        Sound_PlaySeAtPosChecked(7, voice * 2 + (Rand_Next(1) & 1), wallArg, pl->pos, 1, 0);
    }
}

/* Plays voice voice as Motion_VoiceAtFrame does, with the floor's sound
 * argument and the distance curve 14. */
void PlayerSe_VoiceAtFrame(Player *pl, f32 frame, s32 voice)
{
    if ((u8)Obj_InCurrentArea(pl) && Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlaySeAtPosChecked(7, voice * 2 + (Rand_Next(1) & 1), pl->floorArg, pl->pos, 14, 0);
    }
}

/* In the town: plays sound se of bank 2 when the local player's motion passes
 * frame, unless its weapon sounds are muted. */
void PlayerSe_OnlineAtFrame(Player *pl, s32 frame, s32 seId)
{
    if (pl->weaponSeMute == 0 && gScene.inLobby != 0 && gScene.localPlayer == pl->index
        && Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlaySeAtPosChecked(2, seId, 0, pl->pos, 1, 0);
    }
}

/* Plays the rattle of the player's armour piece kind when the player, in
 * the current area, passes frame. */
void PlayerSe_EquipAtFrame(Player *pl, f32 frame, s32 kind)
{
    if ((u8)Obj_InCurrentArea(pl) && Motion_PassedFrame(pl, 0, frame)) {
        Sound_PlayArmorRattle(pl, kind);
    }
}

/*
 * The hunter's motion sounds: per motion (work->motion) the voice, armour
 * rattle and weapon sounds at their frames, first the motions of every
 * weapon, then those of the weapon type in hand. Called after
 * Player_MotionEffects.
 */
void Player_MotionSounds(Player *pl, PlayerWork *work)
{
    switch (work->motion) {
    case 2:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 88.0f, 0);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 90.0f, EQUIP_RATTLE_ALL);
        break;
    case 3:
        PlayerSe_WeaponAtFrame(pl, 56, Sound_RandomSe(39, 1, 40, 1));
        PlayerSe_VoiceAtFrame(pl, 8.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 2);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
        break;
    case 4:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 2);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 66.0f, 2);
        PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
        break;
    case 5:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 80, Sound_RandomSe(41, 3, 41, 2));
        PlayerSe_WeaponAtFrame(pl, 164, Sound_RandomSe(41, 3, 41, 2));
        break;
    case 6:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        break;
    case 7:
    case 9:
        PlayerSe_AtFrame(pl, 2, 74);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
        break;
    case 10:
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 1);
        PlayerSe_AtFrame(pl, 38, 68);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 1);
        PlayerSe_AtFrame(pl, 108, 68);
        break;
    case 12:
        PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
        PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        break;
    case 13:
        PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 4, 58);
        break;
    case 14:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 4, 74);
        break;
    case 15:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 10.0f, 3);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
        break;
    case 16:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
        PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
        break;
    case 17:
        PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 2, 75);
        break;
    case 18:
        PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(38, 2, 38, 2));
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 88.0f, 3);
        PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 112.0f, 0);
        PlayerSe_AtFrame(pl, 94, 68);
        break;
    case 19:
        PlayerSe_AtFrame(pl, 2, 64);
        PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 2);
        PlayerSe_EquipAtFrame(pl, 100.0f, EQUIP_RATTLE_ALL);
        break;
    case 20:
        PlayerSe_AtFrame(pl, 2, 66);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
        PlayerSe_VoiceAtFrame(pl, 68.0f, 1);
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 96.0f, 2);
        PlayerSe_EquipAtFrame(pl, 96.0f, EQUIP_RATTLE_ALL);
        break;
    case 21:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 76.0f, 0);
        PlayerSe_EquipAtFrame(pl, 76.0f, EQUIP_RATTLE_ALL);
        break;
    case 22:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 3);
        PlayerSe_VoiceAtFrame(pl, 14.0f, 3);
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        break;
    case 28:
        PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 3, 36, 1));
        PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
        PlayerSe_AtFrame(pl, 2, 63);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 28, 65);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
        break;
    case 29:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 82.0f, 0);
        PlayerSe_EquipAtFrame(pl, 82.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 116.0f, 0);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 148.0f, 0);
        PlayerSe_EquipAtFrame(pl, 148.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 184.0f, 0);
        PlayerSe_EquipAtFrame(pl, 184.0f, EQUIP_RATTLE_ALL);
        break;
    case 30:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 3);
        PlayerSe_EquipAtFrame(pl, 15.0f, EQUIP_RATTLE_ALL);
        break;
    case 34:
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) == 0) {
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            Motion_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
            Motion_VoiceAtFrame(pl, 98.0f, 0);
            PlayerSe_EquipAtFrame(pl, 104.0f, EQUIP_ARMS);
        }
        break;
    case 35:
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) == 0) {
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
            Motion_VoiceAtFrame(pl, 38.0f, 1);
            PlayerSe_EquipAtFrame(pl, 82.0f, EQUIP_RATTLE_ALL);
            Motion_VoiceAtFrame(pl, 82.0f, 1);
        }
        break;
    case 36:
        PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
        PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
        break;
    case 37:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 36.0f, 1);
        break;
    case 38:
        PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(39, 3, 40, 2));
        PlayerSe_VoiceAtFrame(pl, 16.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 32.0f, 2);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        break;
    case 39:
        PlayerSe_WeaponAtFrame(pl, 42, 41);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 72.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 2);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
        break;
    case 41:
        PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
        PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_ARMS);
        break;
    case 42:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
        break;
    case 43:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        break;
    case 44:
    case 45:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
        PlayerSe_AtFrame(pl, 30, 69);
        break;
    case 46:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 2);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        break;
    case 48:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
        PlayerSe_AtFrame(pl, 4, 66);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 60, 75);
        PlayerSe_VoiceAtFrame(pl, 80.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 0);
        PlayerSe_EquipAtFrame(pl, 100.0f, EQUIP_RATTLE_ALL);
        break;
    case 49:
        PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 102.0f, 1);
        break;
    case 50:
        PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 2));
        PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
        PlayerSe_AtFrame(pl, 20, 62);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        break;
    case 52:
        PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        break;
    case 53:
        PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(pl, 20.0f, 2);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 2);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        break;
    case 54:
        PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 76.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 162.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 192.0f, 1);
        PlayerSe_WeaponAtFrame(pl, 36, Sound_RandomSe(38, 2, 38, 2));
        break;
    case 55:
        PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
        break;
    case 57:
        PlayerSe_AtFrame(pl, 4, 64);
        PlayerSe_VoiceAtFrame(pl, 88.0f, 0);
        PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
        break;
    case 58:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 3);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
        break;
    case 59:
        PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(pl, 10.0f, 2);
        PlayerSe_EquipAtFrame(pl, 10.0f, Sound_RandomSe(0, 2, 0, 2));
        PlayerSe_VoiceAtFrame(pl, 24.0f, 2);
        PlayerSe_EquipAtFrame(pl, 24.0f, Sound_RandomSe(0, 2, 0, 2));
        break;
    case 60:
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_ARMS);
        break;
    case 62:
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
        PlayerSe_VoiceAtFrame(pl, 10.0f, 2);
        break;
    case 63:
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 70.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 1);
        break;
    case 64:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
        PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
        break;
    case 202:
        PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 26, 64);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_BODY);
        break;
    case 204:
        PlayerSe_AtFrame(pl, 14, 64);
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_ARMS);
        break;
    case 206:
        PlayerSe_AtFrame(pl, 4, 60);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 32, 66);
        PlayerSe_AtFrame(pl, 116, 58);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_ARMS);
        break;
    case 207:
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 1);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 74.0f, 2);
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
        break;
    case 209:
        PlayerSe_AtFrame(pl, 4, 60);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 32, 66);
        PlayerSe_AtFrame(pl, 116, 58);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_ARMS);
        break;
    case 210:
        PlayerSe_AtFrame(pl, 66, 58);
        PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 146, 75);
        PlayerSe_EquipAtFrame(pl, 146.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 152, 60);
        break;
    case 211:
        PlayerSe_AtFrame(pl, 12, 74);
        PlayerSe_AtFrame(pl, 20, 75);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        break;
    case 212:
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 92.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 94.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 114.0f, 1);
        break;
    case 214:
        PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        break;
    case 215:
        PlayerSe_AtFrame(pl, 4, 60);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 106, 64);
        PlayerSe_EquipAtFrame(pl, 110.0f, EQUIP_ARMS);
        break;
    case 216:
        PlayerSe_AtFrame(pl, 14, 68);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 44.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 64.0f, 3);
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
        break;
    case 218:
        PlayerSe_AtFrame(pl, 10, 74);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_ARMS);
        break;
    case 220:
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        break;
    case 221:
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        break;
    case 222:
        PlayerSe_AtFrame(pl, 38, 66);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_ARMS);
        break;
    case 224:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 64, 41);
        PlayerSe_AtFrame(pl, 64, 59);
        break;
    case 225:
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 120.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 64, 41);
        PlayerSe_AtFrame(pl, 118, 59);
        PlayerSe_AtFrame(pl, 170, 65);
        break;
    case 226:
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
        break;
    case 227:
        PlayerSe_VoiceAtFrame(pl, 60.0f, 2);
        PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 22, 67);
        PlayerSe_AtFrame(pl, 78, 65);
        PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_BODY);
        PlayerSe_EquipAtFrame(pl, 120.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 128, 68);
        break;
    case 250:
        PlayerSe_OnlineAtFrame(pl, 168, 3);
        PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
        PlayerSe_AtFrame(pl, 46, 75);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 88, 88);
        PlayerSe_VoiceAtFrame(pl, 168.0f, 2);
        PlayerSe_EquipAtFrame(pl, 168.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 168, 89);
        PlayerSe_VoiceAtFrame(pl, 198.0f, 2);
        PlayerSe_EquipAtFrame(pl, 198.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 220, 74);
        PlayerSe_EquipAtFrame(pl, 220.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 366, 61);
        PlayerSe_AtFrame(pl, 410, 90);
        PlayerSe_VoiceAtFrame(pl, 454.0f, 0);
        break;
    case 251:
        PlayerSe_OnlineAtFrame(pl, 162, 2);
        PlayerSe_OnlineAtFrame(pl, 270, 2);
        PlayerSe_OnlineAtFrame(pl, 410, 3);
        PlayerSe_OnlineAtFrame(pl, 540, 6);
        PlayerSe_OnlineAtFrame(pl, 1320, 9);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 356.0f, 0);
        PlayerSe_EquipAtFrame(pl, 368.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 434.0f, 2);
        PlayerSe_EquipAtFrame(pl, 434.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 54, 75);
        PlayerSe_AtFrame(pl, 112, 45);
        PlayerSe_EquipAtFrame(pl, 110.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 170.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 172, 88);
        PlayerSe_EquipAtFrame(pl, 222.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 224, 88);
        PlayerSe_AtFrame(pl, 212, 45);
        PlayerSe_EquipAtFrame(pl, 268.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 268, 88);
        PlayerSe_AtFrame(pl, 260, 45);
        PlayerSe_AtFrame(pl, 398, 45);
        PlayerSe_AtFrame(pl, 400, 88);
        PlayerSe_AtFrame(pl, 416, 63);
        PlayerSe_AtFrame(pl, 500, 63);
        PlayerSe_EquipAtFrame(pl, 426.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 456.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 486.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 260, 69);
        PlayerSe_AtFrame(pl, 538, 66);
        PlayerSe_AtFrame(pl, 574, 58);
        PlayerSe_EquipAtFrame(pl, 578.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 630, 64);
        PlayerSe_AtFrame(pl, 698, 58);
        PlayerSe_EquipAtFrame(pl, 672.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 848, 67);
        PlayerSe_AtFrame(pl, 924, 92);
        PlayerSe_EquipAtFrame(pl, 924.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 946, 75);
        PlayerSe_AtFrame(pl, 962, 67);
        PlayerSe_AtFrame(pl, 988, 92);
        PlayerSe_EquipAtFrame(pl, 924.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 1048, 92);
        PlayerSe_EquipAtFrame(pl, 924.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 1086, 91);
        PlayerSe_EquipAtFrame(pl, 1086.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 1098, 75);
        PlayerSe_EquipAtFrame(pl, 1172.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 1122, 75);
        PlayerSe_AtFrame(pl, 1172, 75);
        PlayerSe_EquipAtFrame(pl, 1172.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 1196, 75);
        PlayerSe_EquipAtFrame(pl, 1196.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 1256, 91);
        PlayerSe_EquipAtFrame(pl, 1256.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 1298, 91);
        PlayerSe_EquipAtFrame(pl, 1298.0f, EQUIP_RATTLE_ALL);
        break;
    case 257:
        PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
        PlayerSe_AtFrame(pl, 54, 69);
        PlayerSe_AtFrame(pl, 74, 65);
        PlayerSe_AtFrame(pl, 118, 59);
        PlayerSe_VoiceAtFrame(pl, 308.0f, 1);
        PlayerSe_EquipAtFrame(pl, 310.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 304, 67);
        PlayerSe_EquipAtFrame(pl, 308.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 362.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 478.0f, 1);
        PlayerSe_AtFrame(pl, 78, 65);
        PlayerSe_AtFrame(pl, 580, 84);
        break;
    case 258:
        PlayerSe_AtFrame(pl, 4, 67);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
        break;
    case 401:
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
        PlayerSe_AtFrame(pl, 20, 62);
        PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
        break;
    case PL_MOT_EAT:
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 1);
        PlayerSe_AtFrame(pl, 122, 75);
        PlayerSe_AtFrame(pl, 142, 75);
        PlayerSe_EquipAtFrame(pl, 142.0f, EQUIP_ARMS);
        break;
    case 403:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 1);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 28, Sound_RandomSe(48, 3, 48, 2));
        PlayerSe_AtFrame(pl, 226, Sound_RandomSe(48, 2, 48, 2));
        break;
    case 405:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
        PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 70, 76);
        PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 116, 76);
        PlayerSe_EquipAtFrame(pl, 120.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 120, 76);
        PlayerSe_AtFrame(pl, 146, 76);
        PlayerSe_EquipAtFrame(pl, 192.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 230.0f, 0);
        PlayerSe_EquipAtFrame(pl, 230.0f, EQUIP_RATTLE_ALL);
        break;
    case 406:
        PlayerSe_AtFrame(pl, 36, 73);
        break;
    case PL_MOT_ITEM_POSE:
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 38, 74);
        PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
        break;
    case PL_MOT_ITEM_NO_GAIN:
        PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(42, 5, 43, 2));
        PlayerSe_WeaponAtFrame(pl, 38, Sound_RandomSe(42, 2, 43, 4));
        PlayerSe_WeaponAtFrame(pl, 66, Sound_RandomSe(42, 4, 43, 2));
        PlayerSe_WeaponAtFrame(pl, 84, Sound_RandomSe(42, 2, 43, 5));
        break;
    case 409:
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 20, 75);
        break;
    case 410:
        PlayerSe_AtFrame(pl, 42, 75);
        PlayerSe_AtFrame(pl, 78, 75);
        break;
    case 413:
        PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 30, 10);
        PlayerSe_WeaponAtFrame(pl, 324, 11);
        PlayerSe_AtFrame(pl, 100, 72);
        PlayerSe_EquipAtFrame(pl, 100.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 146, 72);
        PlayerSe_AtFrame(pl, 192, 72);
        PlayerSe_EquipAtFrame(pl, 192.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 240, 72);
        PlayerSe_VoiceAtFrame(pl, 346.0f, 0);
        PlayerSe_AtFrame(pl, 274, 54);
        if (pl->weaponType == 9) {
            PlayerSe_WeaponAtFrame(pl, 20, 1);
        }
        break;
    case 414:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 62, 74);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_ARMS);
        break;
    case 415:
        PlayerSe_AtFrame(pl, 8, 61);
        PlayerSe_AtFrame(pl, 26, 63);
        PlayerSe_AtFrame(pl, 42, 47);
        PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 38.0f, 3);
        PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 78.0f, 0);
        PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
        PlayerSe_EquipAtFrame(pl, 92.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 136.0f, 0);
        PlayerSe_EquipAtFrame(pl, 138.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 162.0f, 1);
        break;
    case 416:
        PlayerSe_AtFrame(pl, 2, 74);
        PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
        PlayerSe_AtFrame(pl, 54, 55);
        PlayerSe_AtFrame(pl, 68, 55);
        PlayerSe_AtFrame(pl, 78, 55);
        PlayerSe_AtFrame(pl, 94, 55);
        PlayerSe_AtFrame(pl, 110, 55);
        PlayerSe_AtFrame(pl, 132, 55);
        PlayerSe_AtFrame(pl, 146, 55);
        PlayerSe_AtFrame(pl, 152, 55);
        PlayerSe_AtFrame(pl, 172, 73);
        break;
    case 417:
        PlayerSe_AtFrame(pl, 12, 62);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 70.0f, 0);
        break;
    case 418:
        PlayerSe_AtFrame(pl, 32, 42);
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 104, 76);
        PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
        PlayerSe_EquipAtFrame(pl, 104.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 194.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 194.0f, 0);
        break;
    case 419:
        PlayerSe_AtFrame(pl, 30, 50);
        PlayerSe_AtFrame(pl, 64, 51);
        PlayerSe_AtFrame(pl, 98, 53);
        PlayerSe_AtFrame(pl, 172, 53);
        PlayerSe_AtFrame(pl, 228, 51);
        PlayerSe_AtFrame(pl, 228, 50);
        PlayerSe_AtFrame(pl, 286, 75);
        PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
        PlayerSe_EquipAtFrame(pl, 58.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 316.0f, 0);
        PlayerSe_EquipAtFrame(pl, 318.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 336.0f, 0);
        break;
    case 420:
        PlayerSe_AtFrame(pl, 6, 74);
        Motion_SeAtFrame(pl, 70, 119);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
        break;
    case 421:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        break;
    case 422:
        PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 28, 50);
        PlayerSe_AtFrame(pl, 58, 51);
        break;
    case 423:
        PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        break;
    case 424:
        PlayerSe_VoiceAtFrame(pl, 24.0f, 1);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_ARMS);
        Motion_SeAtFrame(pl, 72, 120);
        break;
    case 425:
        PlayerSe_AtFrame(pl, 58, 74);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 1);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 78.0f, 0);
        PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
        break;
    case 426:
        PlayerSe_AtFrame(pl, 168, 61);
        PlayerSe_AtFrame(pl, 204, 59);
        PlayerSe_AtFrame(pl, 234, 58);
        PlayerSe_EquipAtFrame(pl, 210.0f, EQUIP_BODY);
        PlayerSe_EquipAtFrame(pl, 234.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 324.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 74.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 166.0f, 3);
        PlayerSe_VoiceAtFrame(pl, 172.0f, 3);
        break;
    case 428:
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_ARMS);
        break;
    case 429:
        PlayerSe_AtFrame(pl, 30, 67);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 122.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 192, 75);
        PlayerSe_AtFrame(pl, 240, 69);
        PlayerSe_EquipAtFrame(pl, 244.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 304, 69);
        PlayerSe_EquipAtFrame(pl, 306.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 352.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 350.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 414.0f, 0);
        PlayerSe_EquipAtFrame(pl, 416.0f, EQUIP_RATTLE_ALL);
        break;
    case 431:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 1);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 132, Sound_RandomSe(39, 3, 40, 1));
        PlayerSe_WeaponAtFrame(pl, 242, Sound_RandomSe(39, 1, 40, 3));
        break;
    case 432:
        PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
        PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 30, 38);
        PlayerSe_VoiceAtFrame(pl, 106.0f, 0);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_ARMS);
        break;
    case 600:
        PlayerSe_AtFrame(pl, 4, 67);
        PlayerSe_AtFrame(pl, 158, 67);
        PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 74.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 98.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 144.0f, 0);
        PlayerSe_EquipAtFrame(pl, 144.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 178.0f, 0);
        PlayerSe_EquipAtFrame(pl, 178.0f, EQUIP_RATTLE_ALL);
        break;
    case 602:
        PlayerSe_AtFrame(pl, 48, 80);
        PlayerSe_AtFrame(pl, 62, 80);
        break;
    case 603:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 1);
        PlayerSe_AtFrame(pl, 34, 49);
        PlayerSe_AtFrame(pl, 48, 49);
        PlayerSe_AtFrame(pl, 62, 49);
        PlayerSe_EquipAtFrame(pl, 120.0f, EQUIP_RATTLE_ALL);
        break;
    case 604:
        PlayerSe_AtFrame(pl, 12, 68);
        break;
    case 605:
        PlayerSe_VoiceAtFrame(pl, 52.0f, 0);
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 22, 74);
        break;
    case 607:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        break;
    case 608:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 1);
        PlayerSe_AtFrame(pl, 68, 74);
        break;
    case 610:
        PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 64.0f, 1);
        PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 12, 68);
        break;
    case 611:
        PlayerSe_AtFrame(pl, 42, 81);
        PlayerSe_AtFrame(pl, 64, 81);
        PlayerSe_AtFrame(pl, 108, 83);
        PlayerSe_EquipAtFrame(pl, 108.0f, EQUIP_ARMS);
        break;
    case 612:
        PlayerSe_AtFrame(pl, 124, 75);
        PlayerSe_AtFrame(pl, 42, 61);
        PlayerSe_AtFrame(pl, 56, 82);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 128, 83);
        break;
    case 613:
        PlayerSe_AtFrame(pl, 72, 58);
        PlayerSe_EquipAtFrame(pl, 76.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 132, 84);
        PlayerSe_AtFrame(pl, 206, 85);
        break;
    case 614:
        PlayerSe_AtFrame(pl, 38, 68);
        PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_ARMS);
        break;
    case 615:
        PlayerSe_AtFrame(pl, 42, 74);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 154.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 220.0f, EQUIP_ARMS);
        break;
    case 616:
        PlayerSe_AtFrame(pl, 32, 67);
        PlayerSe_EquipAtFrame(pl, 82.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 102.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 74, 64);
        PlayerSe_AtFrame(pl, 126, 84);
        PlayerSe_AtFrame(pl, 246, 85);
        break;
    case 617:
        PlayerSe_AtFrame(pl, 46, 67);
        PlayerSe_VoiceAtFrame(pl, 62.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 102.0f, 3);
        PlayerSe_EquipAtFrame(pl, 104.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 140.0f, 3);
        PlayerSe_EquipAtFrame(pl, 142.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 176.0f, 0);
        PlayerSe_EquipAtFrame(pl, 176.0f, EQUIP_RATTLE_ALL);
        break;
    case 618:
        PlayerSe_AtFrame(pl, 38, 83);
        PlayerSe_AtFrame(pl, 126, 58);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 68.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 94.0f, 0);
        PlayerSe_EquipAtFrame(pl, 96.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 126, 61);
        PlayerSe_EquipAtFrame(pl, 132.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 190, 61);
        PlayerSe_EquipAtFrame(pl, 192.0f, EQUIP_ARMS);
        break;
    case 619:
        PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 1);
        PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
        break;
    case 620:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 1);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
        PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_ARMS);
        break;
    case 621:
        PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
        PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 10, 68);
        PlayerSe_VoiceAtFrame(pl, 114.0f, 0);
        break;
    case 622:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
        PlayerSe_VoiceAtFrame(pl, 10.0f, 3);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 54, 61);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 66.0f, 3);
        PlayerSe_AtFrame(pl, 66, 58);
        break;
    case 623:
        PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 46, 41);
        PlayerSe_AtFrame(pl, 66, 64);
        PlayerSe_AtFrame(pl, 80, 58);
        PlayerSe_EquipAtFrame(pl, 80.0f, EQUIP_ARMS);
        break;
    case 624:
        PlayerSe_AtFrame(pl, 4, 68);
        break;
    case 625:
        PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
        PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 28, 58);
        PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 56, 58);
        break;
    case 626:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        break;
    case 627:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
        PlayerSe_AtFrame(pl, 30, 49);
        PlayerSe_AtFrame(pl, 42, 49);
        break;
    case 628:
        PlayerSe_OnlineAtFrame(pl, 38, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_OnlineAtFrame(pl, 54, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_OnlineAtFrame(pl, 176, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_OnlineAtFrame(pl, 166, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_WeaponAtFrame(pl, 38, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_WeaponAtFrame(pl, 54, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_WeaponAtFrame(pl, 176, Sound_RandomSe(39, 39, 40, 2));
        PlayerSe_WeaponAtFrame(pl, 166, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 44.0f, 0);
        PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 38, 61);
        PlayerSe_EquipAtFrame(pl, 176.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 176, 61);
        PlayerSe_EquipAtFrame(pl, 292.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 54, 61);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 114.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 230.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 246.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 168.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 166, 61);
        PlayerSe_EquipAtFrame(pl, 176.0f, EQUIP_ARMS);
        break;
    case 629:
        PlayerSe_AtFrame(pl, 316, 69);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        break;
    case 630:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 74.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 132.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 154.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 186.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 214.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 252.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 278.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 318.0f, 0);
        PlayerSe_EquipAtFrame(pl, 86.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 150.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 216.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 266.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 138.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 204.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 248.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 312.0f, EQUIP_ARMS);
        break;
    case 631:
        PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 198.0f, 0);
        PlayerSe_EquipAtFrame(pl, 198.0f, EQUIP_RATTLE_ALL);
        break;
    case 632:
        PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 134.0f, EQUIP_ARMS);
        break;
    case 633:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 42, 62);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_ARMS);
        break;
    case 635:
        Motion_SeAtFrameLocal(pl, 138, 86);
        Motion_SeAtFrameLocal(pl, 222, 53);
        Motion_SeAtFrameLocal(pl, 234, 52);
        Motion_SeAtFrameLocal(pl, 256, 55);
        Motion_SeAtFrameLocal(pl, 270, 50);
        Motion_SeAtFrameLocal(pl, 286, 53);
        Motion_SeAtFrameLocal(pl, 310, 51);
        Motion_SeAtFrameLocal(pl, 328, 51);
        Motion_SeAtFrameLocal(pl, 380, 52);
        Motion_SeAtFrameLocal(pl, 388, 52);
        Motion_SeAtFrameLocal(pl, 412, 55);
        Motion_SeAtFrameLocal(pl, 434, 50);
        Motion_SeAtFrameLocal(pl, 478, 53);
        Motion_SeAtFrameLocal(pl, 504, 51);
        Motion_SeAtFrameLocal(pl, 504, 52);
        Motion_SeAtFrameLocal(pl, 230, 87);
        Motion_SeAtFrameLocal(pl, 250, 87);
        Motion_SeAtFrameLocal(pl, 290, 87);
        Motion_SeAtFrameLocal(pl, 350, 87);
        Motion_SeAtFrameLocal(pl, 380, 87);
        Motion_SeAtFrameLocal(pl, 478, 87);
        Motion_SeAtFrameLocal(pl, 412, 87);
        Motion_SeAtFrameLocal(pl, 440, 87);
        Motion_SeAtFrameLocal(pl, 560, 87);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        break;
    case 636:
        PlayerSe_OnlineAtFrame(pl, 26, Sound_RandomSe(4, 4, 5, 4));
        Motion_SeAtFrameLocal(pl, 22, 74);
        Motion_SeAtFrameLocal(pl, 22, 75);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 56.0f, 2);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 68.0f, 2);
        PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 80.0f, 2);
        PlayerSe_EquipAtFrame(pl, 80.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 90.0f, 2);
        PlayerSe_EquipAtFrame(pl, 90.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 2);
        PlayerSe_EquipAtFrame(pl, 100.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 110.0f, 2);
        PlayerSe_EquipAtFrame(pl, 110.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 126.0f, 2);
        PlayerSe_EquipAtFrame(pl, 126.0f, EQUIP_RATTLE_ALL);
        Motion_SeAtFrameLocal(pl, 150, 58);
        break;
    case 637:
        PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        Motion_SeAtFrameLocal(pl, 44, 67);
        PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_ARMS);
        Motion_SeAtFrameLocal(pl, 60, 67);
        PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_ARMS);
        Motion_SeAtFrameLocal(pl, 80, 86);
        PlayerSe_EquipAtFrame(pl, 80.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 146.0f, 0);
        PlayerSe_EquipAtFrame(pl, 146.0f, EQUIP_RATTLE_ALL);
        break;
    case 640:
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 14, 69);
        break;
    case 642:
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 86.0f, 0);
        PlayerSe_EquipAtFrame(pl, 86.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 98.0f, 1);
        PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 116, 83);
        break;
    case 643:
        PlayerSe_AtFrame(pl, 24, 83);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
        break;
    case 647:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 1);
        PlayerSe_AtFrame(pl, 78, 91);
        PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 116, 92);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 192, 75);
        PlayerSe_AtFrame(pl, 230, 88);
        PlayerSe_EquipAtFrame(pl, 244.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 286, 89);
        PlayerSe_EquipAtFrame(pl, 300.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 300, 93);
        PlayerSe_AtFrame(pl, 390, 94);
        PlayerSe_AtFrame(pl, 416, 61);
        PlayerSe_AtFrame(pl, 566, 90);
        PlayerSe_EquipAtFrame(pl, 430.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 430.0f, 0);
        PlayerSe_EquipAtFrame(pl, 430.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 580.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 580.0f, 0);
        PlayerSe_EquipAtFrame(pl, 608.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 608.0f, 0);
        PlayerSe_AtFrame(pl, 282, 62);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_ARMS);
        break;
    case 648:
        PlayerSe_VoiceAtFrame(pl, 12.0f, 1);
        PlayerSe_AtFrame(pl, 46, 91);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 86, 92);
        PlayerSe_EquipAtFrame(pl, 86.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 102, 93);
        PlayerSe_AtFrame(pl, 140, 75);
        PlayerSe_AtFrame(pl, 158, 88);
        PlayerSe_EquipAtFrame(pl, 168.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 254, 88);
        PlayerSe_AtFrame(pl, 276, 88);
        PlayerSe_EquipAtFrame(pl, 280.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 300, 88);
        PlayerSe_EquipAtFrame(pl, 302.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 332, 88);
        PlayerSe_EquipAtFrame(pl, 336.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 368, 88);
        PlayerSe_AtFrame(pl, 398, 88);
        PlayerSe_EquipAtFrame(pl, 406.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 250, 91);
        PlayerSe_EquipAtFrame(pl, 468.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 466.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 466, 92);
        PlayerSe_EquipAtFrame(pl, 466.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 486, 91);
        PlayerSe_EquipAtFrame(pl, 486.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 502, 92);
        PlayerSe_EquipAtFrame(pl, 556.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 556.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 698.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 698, 92);
        PlayerSe_EquipAtFrame(pl, 740.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 740, 91);
        PlayerSe_EquipAtFrame(pl, 792.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 792.0f, 0);
        PlayerSe_EquipAtFrame(pl, 820.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 820.0f, 0);
        break;
    case 677:
        PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 96, 75);
        PlayerSe_EquipAtFrame(pl, 96.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 96, 67);
        PlayerSe_AtFrame(pl, 140, 58);
        PlayerSe_AtFrame(pl, 140, 74);
        break;
    case 679:
        PlayerSe_AtFrame(pl, 38, 67);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
        PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 110.0f, 1);
        break;
    case 800:
        PlayerSe_AtFrame(pl, 24, 74);
        PlayerSe_AtFrame(pl, 72, 58);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 0);
        break;
    case 801:
        PlayerSe_WeaponAtFrame(pl, 4, 37);
        PlayerSe_AtFrame(pl, 4, 74);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        break;
    case 802:
        PlayerSe_AtFrame(pl, 4, 74);
        PlayerSe_WeaponAtFrame(pl, 56, 41);
        PlayerSe_VoiceAtFrame(pl, 118.0f, 0);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_RATTLE_ALL);
        break;
    case 803:
        PlayerSe_AtFrame(pl, 58, 40);
        PlayerSe_AtFrame(pl, 58, 41);
        PlayerSe_AtFrame(pl, 120, 41);
        PlayerSe_AtFrame(pl, 180, 41);
        PlayerSe_AtFrame(pl, 180, 40);
        PlayerSe_AtFrame(pl, 242, 41);
        break;
    case 805:
        PlayerSe_VoiceAtFrame(pl, 60.0f, 1);
        PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 94.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 142.0f, 1);
        PlayerSe_AtFrame(pl, 100, 43);
        PlayerSe_EquipAtFrame(pl, 106.0f, EQUIP_ARMS);
        break;
    case 807:
        PlayerSe_VoiceAtFrame(pl, 32.0f, 3);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 1);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 84.0f, 1);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 52, 75);
        PlayerSe_AtFrame(pl, 54, 45);
        PlayerSe_AtFrame(pl, 96, 45);
        PlayerSe_AtFrame(pl, 210, 45);
        PlayerSe_AtFrame(pl, 160, 45);
        break;
    case 808:
        PlayerSe_OnlineAtFrame(pl, 16, 2);
        PlayerSe_WeaponAtFrame(pl, 16, 38);
        PlayerSe_AtFrame(pl, 12, 43);
        PlayerSe_AtFrame(pl, 22, 45);
        PlayerSe_AtFrame(pl, 84, 75);
        PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 76.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 180.0f, 1);
        break;
    case 809:
        PlayerSe_AtFrame(pl, 28, 43);
        PlayerSe_VoiceAtFrame(pl, 66.0f, 0);
        PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 94.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 212.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 226.0f, 1);
        break;
    case 810:
        PlayerSe_AtFrame(pl, 10, 43);
        PlayerSe_AtFrame(pl, 10, 45);
        PlayerSe_AtFrame(pl, 54, 65);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 122.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 176.0f, 1);
        break;
    case 867:
        PlayerSe_AtFrame(pl, 2, 61);
        PlayerSe_AtFrame(pl, 14, 58);
        PlayerSe_AtFrame(pl, 114, 82);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 130, 82);
        PlayerSe_EquipAtFrame(pl, 312.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 178, 83);
        break;
    case 890:
        PlayerSe_OnlineAtFrame(pl, 160, Sound_RandomSe(0, 4, 1, 4));
        PlayerSe_AtFrame(pl, 28, 74);
        PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 80, 63);
        PlayerSe_AtFrame(pl, 128, 63);
        PlayerSe_AtFrame(pl, 162, 83);
        PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 1);
        break;
    case 891:
        PlayerSe_OnlineAtFrame(pl, 188, 2);
        PlayerSe_OnlineAtFrame(pl, 250, Sound_RandomSe(7, 4, 8, 4));
        PlayerSe_OnlineAtFrame(pl, 296, 2);
        PlayerSe_OnlineAtFrame(pl, 562, 2);
        PlayerSe_OnlineAtFrame(pl, 340, Sound_RandomSe(7, 4, 8, 4));
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 1);
        PlayerSe_EquipAtFrame(pl, 180.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 176.0f, 3);
        PlayerSe_AtFrame(pl, 186, 69);
        PlayerSe_AtFrame(pl, 334, 69);
        PlayerSe_AtFrame(pl, 490, 83);
        PlayerSe_AtFrame(pl, 506, 83);
        PlayerSe_AtFrame(pl, 518, 83);
        PlayerSe_EquipAtFrame(pl, 206.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 260.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 300.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 558.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 52, 45);
        PlayerSe_AtFrame(pl, 192, 45);
        PlayerSe_AtFrame(pl, 262, 45);
        PlayerSe_AtFrame(pl, 304, 45);
        PlayerSe_AtFrame(pl, 446, 45);
        PlayerSe_AtFrame(pl, 562, 45);
        PlayerSe_AtFrame(pl, 217, 45);
        PlayerSe_AtFrame(pl, 280, 45);
        PlayerSe_AtFrame(pl, 320, 45);
        PlayerSe_AtFrame(pl, 600, 45);
        break;
    case 892:
        PlayerSe_OnlineAtFrame(pl, 4, 3);
        PlayerSe_AtFrame(pl, 8, 83);
        PlayerSe_AtFrame(pl, 8, 59);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 84, 74);
        break;
    case 893:
        PlayerSe_OnlineAtFrame(pl, 4, 6);
        PlayerSe_OnlineAtFrame(pl, 134, 9);
        PlayerSe_AtFrame(pl, 4, 63);
        PlayerSe_AtFrame(pl, 46, 66);
        PlayerSe_AtFrame(pl, 76, 60);
        PlayerSe_AtFrame(pl, 230, 67);
        PlayerSe_EquipAtFrame(pl, 76.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 272, 67);
        PlayerSe_EquipAtFrame(pl, 276.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 334, 58);
        PlayerSe_EquipAtFrame(pl, 336.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 366, 58);
        break;
    case 894:
        PlayerSe_OnlineAtFrame(pl, 160, Sound_RandomSe(0, 4, 1, 4));
        PlayerSe_AtFrame(pl, 36, 74);
        PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 80, 63);
        PlayerSe_AtFrame(pl, 114, 63);
        PlayerSe_AtFrame(pl, 166, 83);
        PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 1);
        break;
    case 895:
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 1);
        PlayerSe_EquipAtFrame(pl, 324.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 322.0f, 3);
        PlayerSe_AtFrame(pl, 170, 69);
        PlayerSe_AtFrame(pl, 334, 69);
        PlayerSe_AtFrame(pl, 304, 83);
        PlayerSe_AtFrame(pl, 322, 83);
        PlayerSe_AtFrame(pl, 626, 83);
        PlayerSe_AtFrame(pl, 642, 83);
        PlayerSe_AtFrame(pl, 664, 83);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 90.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 146.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 372.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 468.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 22, 45);
        PlayerSe_AtFrame(pl, 88, 45);
        PlayerSe_AtFrame(pl, 144, 45);
        PlayerSe_AtFrame(pl, 370, 45);
        PlayerSe_AtFrame(pl, 468, 45);
        break;
    case 896:
        PlayerSe_OnlineAtFrame(pl, 4, 3);
        PlayerSe_AtFrame(pl, 8, 83);
        PlayerSe_AtFrame(pl, 8, 59);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 84, 74);
        break;
    case 897:
        PlayerSe_OnlineAtFrame(pl, 4, 6);
        PlayerSe_OnlineAtFrame(pl, 134, 9);
        PlayerSe_AtFrame(pl, 4, 63);
        PlayerSe_AtFrame(pl, 46, 66);
        PlayerSe_AtFrame(pl, 76, 60);
        PlayerSe_AtFrame(pl, 230, 67);
        PlayerSe_EquipAtFrame(pl, 76.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 272, 67);
        PlayerSe_EquipAtFrame(pl, 276.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 348, 58);
        PlayerSe_EquipAtFrame(pl, 350.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 380, 58);
        break;
    }
    switch (pl->weaponType) {
    case WEAPON_HEAVY_BOWGUN:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 76, 2);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 86, 6);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 1009:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 78.0f, 0);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 46, 6);
            return;
        case 1401:
        case 1402:
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 108.0f, 0);
            PlayerSe_EquipAtFrame(pl, 108.0f, EQUIP_RATTLE_ALL);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 44, 13);
            PlayerSe_WeaponAtFrame(pl, 66, 14);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 104, 4);
            return;
        case 1405:
        case 1406:
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 1408:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 0);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 130.0f, 2);
            PlayerSe_EquipAtFrame(pl, 130.0f, EQUIP_RATTLE_ALL);
            return;
        case 1409:
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 108.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 136.0f, 0);
            PlayerSe_EquipAtFrame(pl, 136.0f, EQUIP_RATTLE_ALL);
            return;
        case 1411:
            PlayerSe_WeaponAtFrame(pl, 2, 4);
            PlayerSe_WeaponAtFrame(pl, 68, 9);
            PlayerSe_WeaponAtFrame(pl, 180, 1);
            return;
        case 1412:
        case 1416:
        case 1417:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 1413:
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 4, 7);
            PlayerSe_WeaponAtFrame(pl, 12, 1);
            PlayerSe_AtFrame(pl, 8, 61);
            return;
        case 1418:
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 1419:
            PlayerSe_VoiceAtFrame(pl, 116.0f, 0);
            PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 2, 67);
            return;
        case 1422:
            PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 90.0f, 0);
            PlayerSe_EquipAtFrame(pl, 90.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 72, 2);
            return;
        }
        break;
    case WEAPON_HAMMER:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 66, 2);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 3);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            return;
        case 1004:
        case 1005:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 2);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 1006:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 3);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 20, 5);
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            return;
        case 1009:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 0);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            return;
        case 1010:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 12, 2);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            return;
        case 1011:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            return;
        case 1012:
        case 1013:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            return;
        case 1401:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 26, 4);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            return;
        case 1402:
            PlayerSe_WeaponAtFrame(pl, 48, Sound_RandomSe(36, 6, 35, 2));
            PlayerSe_WeaponAtFrame(pl, 14, 5);
            PlayerSe_WeaponAtFrame(pl, 40, 4);
            PlayerSe_WeaponAtFrame(pl, 58, 7);
            PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 52.0f, 3);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 102, 5);
            PlayerSe_EquipAtFrame(pl, 128.0f, EQUIP_ARMS);
            return;
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 2);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 2);
            PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            PlayerSe_WeaponAtFrame(pl, 38, 6);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 192, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_WeaponAtFrame(pl, 38, 7);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 2);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 116, 5);
            PlayerSe_WeaponAtFrame(pl, 170, 2);
            PlayerSe_VoiceAtFrame(pl, 168.0f, 0);
            PlayerSe_EquipAtFrame(pl, 168.0f, EQUIP_RATTLE_ALL);
            return;
        case 1405:
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 4, 35, 4));
            PlayerSe_WeaponAtFrame(pl, 66, Sound_RandomSe(36, 2, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 12, 4);
            PlayerSe_WeaponAtFrame(pl, 64, 4);
            PlayerSe_WeaponAtFrame(pl, 64, 5);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 2);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 112.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            return;
        case 1406:
            PlayerSe_WeaponAtFrame(pl, 20, 38);
            PlayerSe_WeaponAtFrame(pl, 72, 37);
            PlayerSe_WeaponAtFrame(pl, 74, 8);
            PlayerSe_WeaponAtFrame(pl, 90, 15);
            PlayerSe_WeaponAtFrame(pl, 2, 4);
            PlayerSe_WeaponAtFrame(pl, 62, 4);
            PlayerSe_WeaponAtFrame(pl, 182, 38);
            PlayerSe_WeaponAtFrame(pl, 186, 5);
            PlayerSe_WeaponAtFrame(pl, 238, 2);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 70.0f, 3);
            PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 238.0f, 1);
            PlayerSe_EquipAtFrame(pl, 190.0f, EQUIP_ARMS);
            return;
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 38.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            return;
        case 1408:
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 62, 1);
            PlayerSe_EquipAtFrame(pl, 90.0f, EQUIP_ARMS);
            return;
        case 1409:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            return;
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            return;
        case 1411:
            PlayerSe_WeaponAtFrame(pl, 38, 37);
            PlayerSe_VoiceAtFrame(pl, 154.0f, 2);
            PlayerSe_WeaponAtFrame(pl, 14, 6);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 2);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 130.0f, 0);
            PlayerSe_EquipAtFrame(pl, 130.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 34, 15);
            PlayerSe_WeaponAtFrame(pl, 156, 2);
            return;
        case 1412:
            PlayerSe_WeaponAtFrame(pl, 50, Sound_RandomSe(35, 1, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 42, 4);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 56, 7);
            return;
        case 1413:
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 12, 6);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 2);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            return;
        }
        break;
    case WEAPON_HUNTING_HORN:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 66, 2);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 3);
            PlayerSe_EquipAtFrame(pl, 58.0f, EQUIP_RATTLE_ALL);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 48, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 1004:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 18.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 2);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
            return;
        case 1006:
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 20, 4);
            return;
        case 1011:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            return;
        case 1012:
        case 1013:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            return;
        case 1401:
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 70.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            PlayerSe_WeaponAtFrame(pl, 20, 4);
            PlayerSe_WeaponAtFrame(pl, 80, 2);
            return;
        case 1402:
            PlayerSe_WeaponAtFrame(pl, 40, Sound_RandomSe(35, 3, 35, 3));
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_WeaponAtFrame(pl, 70, 2);
            return;
        case 1403:
            PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 3);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 192.0f, 0);
            PlayerSe_EquipAtFrame(pl, 194.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 22, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_WeaponAtFrame(pl, 76, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(pl, 24, 6);
            PlayerSe_WeaponAtFrame(pl, 74, 4);
            PlayerSe_WeaponAtFrame(pl, 84, 7);
            PlayerSe_WeaponAtFrame(pl, 158, 5);
            PlayerSe_WeaponAtFrame(pl, 192, 2);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 62.0f, 0);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 14, 6);
            return;
        case 1408:
            PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 62.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 94.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 110.0f, 2);
            PlayerSe_EquipAtFrame(pl, 112.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 156.0f, 0);
            PlayerSe_EquipAtFrame(pl, 158.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 68, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_WeaponAtFrame(pl, 48, 7);
            PlayerSe_WeaponAtFrame(pl, 70, 2);
            PlayerSe_WeaponAtFrame(pl, 116, 2);
            return;
        case 1409:
            PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 62.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 78.0f, 2);
            PlayerSe_EquipAtFrame(pl, 80.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 102.0f, 0);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 40, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 40, 6);
            PlayerSe_WeaponAtFrame(pl, 52, 7);
            PlayerSe_WeaponAtFrame(pl, 70, 4);
            PlayerSe_WeaponAtFrame(pl, 132, 2);
            return;
        case 1410:
            PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 70.0f, 2);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 86.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 102.0f, 0);
            PlayerSe_EquipAtFrame(pl, 102.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 40, Sound_RandomSe(38, 2, 38, 2));
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 32, 4);
            PlayerSe_WeaponAtFrame(pl, 50, 7);
            PlayerSe_WeaponAtFrame(pl, 112, 2);
            return;
        case 1411:
            PlayerSe_VoiceAtFrame(pl, 48.0f, 3);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 2);
            PlayerSe_EquipAtFrame(pl, 80.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 86.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 100.0f, 2);
            PlayerSe_EquipAtFrame(pl, 108.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(38, 2, 38, 2));
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 40, 6);
            PlayerSe_WeaponAtFrame(pl, 50, 7);
            PlayerSe_WeaponAtFrame(pl, 90, 5);
            PlayerSe_WeaponAtFrame(pl, 120, 2);
            return;
        case 1412:
            PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 3);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            PlayerSe_WeaponAtFrame(pl, 68, 2);
            return;
        case 1413:
            PlayerSe_VoiceAtFrame(pl, 54.0f, 3);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            PlayerSe_WeaponAtFrame(pl, 60, 2);
            return;
        case 1414:
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            return;
        case 1415:
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            return;
        case 1416:
            break;
        case 1417:
            PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            return;
        case 1418:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 1419:
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 1420:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 1421:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            return;
        case 1422:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 1423:
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 1424:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 1427:
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 34, 2);
            return;
        case 1428:
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 34, 2);
            return;
        case 1429:
            PlayerSe_VoiceAtFrame(pl, 20.0f, 2);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 34.0f, 2);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 34, 2);
            PlayerSe_WeaponAtFrame(pl, 34, 8);
            return;
        case 1430:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 1431:
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 12, 1);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            return;
        case 1432:
            break;
        case 1405:
            Player_SeAtFrame(pl, 12, 24);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 1);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 1);
            return;
        case 1406:
            Player_SeAtFrame(pl, 12, 25);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 1);
            PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 62.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 76.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 1);
            return;
        case 1407:
            Player_SeAtFrame(pl, 12, 26);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 70.0f, 1);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
            return;
        }
        break;
    case WEAPON_LIGHT_BOWGUN:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 1004:
            PlayerSe_WeaponAtFrame(pl, 56, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 1405:
        case 1406:
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 1009:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 3);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            return;
        case 1010:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 1);
            return;
        case 1401:
        case 1402:
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 106.0f, 0);
            PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 44, 13);
            PlayerSe_WeaponAtFrame(pl, 66, 14);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 104, 4);
            return;
        case 1408:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 0);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 130.0f, 0);
            PlayerSe_EquipAtFrame(pl, 130.0f, EQUIP_RATTLE_ALL);
            return;
        case 1409:
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 108.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 136.0f, 0);
            PlayerSe_EquipAtFrame(pl, 136.0f, EQUIP_RATTLE_ALL);
            return;
        case 1411:
            PlayerSe_WeaponAtFrame(pl, 2, 9);
            PlayerSe_WeaponAtFrame(pl, 180, 7);
            return;
        case 1412:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 1413:
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 70, 7);
            PlayerSe_WeaponAtFrame(pl, 20, 1);
            PlayerSe_AtFrame(pl, 8, 61);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 1414:
        case 1415:
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 128.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 4, 3);
            PlayerSe_WeaponAtFrame(pl, 100, 4);
            return;
        case 1416:
        case 1417:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 1422:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 24, 3);
            return;
        case 1423:
            PlayerSe_VoiceAtFrame(pl, 14.0f, 2);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 52.0f, 0);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_WeaponAtFrame(pl, 38, 3);
            return;
        }
        break;
    case WEAPON_BOW:
        switch (work->motion) {
        case 1002:
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 16, 1);
            PlayerSe_WeaponAtFrame(pl, 32, 2);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 1003:
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 24, 3);
            PlayerSe_WeaponAtFrame(pl, 36, 4);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
            PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 1005:
            PlayerSe_WeaponAtFrame(pl, 8, 3);
            PlayerSe_WeaponAtFrame(pl, 40, 4);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 1);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 86.0f, 1);
            PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
            return;
        case 1006:
            PlayerSe_AtFrame(pl, 4, 67);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
            PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
            PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
            return;
        case 1009:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 11);
            PlayerSe_WeaponAtFrame(pl, 12, 11);
            PlayerSe_WeaponAtFrame(pl, 32, 11);
            PlayerSe_WeaponAtFrame(pl, 42, 11);
            return;
        case 1010:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 32, 2);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 1011:
            PlayerSe_AtFrame(pl, 4, 67);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            return;
        case 1012:
            PlayerSe_WeaponAtFrame(pl, 16, 1);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            return;
        case 1013:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            return;
        case 1014:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 8, 2);
            PlayerSe_VoiceAtFrame(pl, 6.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 26, 62);
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 2));
            return;
        case 1401:
        case 1402:
        case 1403:
            break;
        case 1404:
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 194.0f, 0);
            PlayerSe_EquipAtFrame(pl, 194.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 46, 5);
            PlayerSe_AtFrame(pl, 72, 74);
            PlayerSe_WeaponAtFrame(pl, 100, 6);
            PlayerSe_WeaponAtFrame(pl, 130, 4);
            return;
        case 1405:
        case 1406:
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 20, 7);
            PlayerSe_WeaponAtFrame(pl, 40, 8);
            PlayerSe_WeaponAtFrame(pl, 70, 9);
            PlayerSe_WeaponAtFrame(pl, 100, 10);
            PlayerSe_WeaponAtFrame(pl, 142, 11);
            return;
        case 1408:
        case 1409:
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 2, 17);
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_ARMS);
            return;
        case 1411:
            PlayerSe_VoiceAtFrame(pl, 30.0f, 2);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 2);
            PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
            return;
        case 1413:
            PlayerSe_AtFrame(pl, 22, 62);
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
            return;
        case 1414:
            PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_ARMS);
            PlayerSe_AtFrame(pl, 20, 63);
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 32.0f, 3);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_WAIST);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 96.0f, 0);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 96, 7);
            return;
        case 1415:
        case 1416:
        case 1417:
            PlayerSe_WeaponAtFrame(pl, 2, 15);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 2);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 1418:
        case 1419:
        case 1420:
            PlayerSe_WeaponAtFrame(pl, 2, 7);
            PlayerSe_WeaponAtFrame(pl, 14, 8);
            return;
        }
        break;
    case WEAPON_LANCE:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 3);
            PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 56, 1);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_WeaponAtFrame(pl, 32, 3);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 3);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 3);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 1005:
        case 1007:
            PlayerSe_WeaponAtFrame(pl, 7, 5);
            return;
        case 1009:
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_WeaponAtFrame(pl, 32, 3);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 62.0f, 3);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 94.0f, 3);
            PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_RATTLE_ALL);
            return;
        case 1010:
            PlayerSe_WeaponAtFrame(pl, 28, 37);
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 28, 1);
            PlayerSe_WeaponAtFrame(pl, 40, 6);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 3);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 38.0f, 3);
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
            return;
        case 1011:
            PlayerSe_AtFrame(pl, 12, 69);
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 1014:
        case 1015:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 1016:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 1018:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 6, 3);
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 1401:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 12, 5);
            PlayerSe_WeaponAtFrame(pl, 52, 7);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 1402:
            PlayerSe_WeaponAtFrame(pl, 4, 7);
            PlayerSe_WeaponAtFrame(pl, 22, 7);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 2);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 2);
            return;
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 66, 5);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 2);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 1);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 70, 5);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 0);
            return;
        case 1405:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 18, 6);
            PlayerSe_WeaponAtFrame(pl, 92, 5);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 100.0f, 0);
            return;
        case 1406:
            PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(35, 3, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 18, 6);
            PlayerSe_WeaponAtFrame(pl, 92, 5);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
            PlayerSe_EquipAtFrame(pl, 102.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 100.0f, 1);
            return;
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_AtFrame(pl, 80, 69);
            PlayerSe_AtFrame(pl, 80, 74);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 1);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
            return;
        case 1409:
            PlayerSe_WeaponAtFrame(pl, 52, 5);
            PlayerSe_AtFrame(pl, 6, 69);
            PlayerSe_AtFrame(pl, 6, 74);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
            return;
        case 1411:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(36, 2, 37, 3));
            PlayerSe_WeaponAtFrame(pl, 6, 6);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_AtFrame(pl, 10, 68);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            return;
        case 1412:
            PlayerSe_WeaponAtFrame(pl, 6, 37);
            PlayerSe_WeaponAtFrame(pl, 12, 6);
            PlayerSe_WeaponAtFrame(pl, 116, 5);
            PlayerSe_AtFrame(pl, 14, 68);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 3);
            PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 84.0f, 0);
            PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 118.0f, 0);
            PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_RATTLE_ALL);
            return;
        case 1415:
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 26, 68);
            return;
        case 1416:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            return;
        case 1417:
            PlayerSe_WeaponAtFrame(pl, 88, 5);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 3);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 1);
            return;
        }
        break;
    case WEAPON_GUNLANCE:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 3);
            PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 56, 1);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_WeaponAtFrame(pl, 32, 3);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 84.0f, 1);
            PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_RATTLE_ALL);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 3);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 1005:
            PlayerSe_WeaponAtFrame(pl, 8, 5);
            return;
        case 1007:
            PlayerSe_AtFrame(pl, 2, 74);
            return;
        case 1009:
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_WeaponAtFrame(pl, 32, 3);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 62.0f, 3);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 94.0f, 3);
            PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_RATTLE_ALL);
            return;
        case 1010:
            PlayerSe_WeaponAtFrame(pl, 28, 37);
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 16, 1);
            PlayerSe_WeaponAtFrame(pl, 40, 6);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 3);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 38.0f, 3);
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
            return;
        case 1011:
            PlayerSe_AtFrame(pl, 12, 69);
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 1014:
        case 1015:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 1016:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 1018:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 6, 3);
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 1401:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 8, 13);
            PlayerSe_WeaponAtFrame(pl, 72, 5);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 2);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            return;
        case 1402:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 8, 13);
            PlayerSe_WeaponAtFrame(pl, 72, 5);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            return;
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 8, 13);
            PlayerSe_WeaponAtFrame(pl, 78, 5);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 2);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            return;
        case 1404:
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 8, 2);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_WeaponAtFrame(pl, 52, 7);
            PlayerSe_WeaponAtFrame(pl, 88, 1);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 82.0f, 0);
            return;
        case 1405:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 18, 6);
            PlayerSe_WeaponAtFrame(pl, 92, 5);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 0);
            return;
        case 1406:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(36, 3, 37, 3));
            PlayerSe_WeaponAtFrame(pl, 12, 2);
            PlayerSe_WeaponAtFrame(pl, 28, 3);
            PlayerSe_WeaponAtFrame(pl, 38, 6);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_WeaponAtFrame(pl, 80, 5);
            PlayerSe_VoiceAtFrame(pl, 84.0f, 1);
            PlayerSe_EquipAtFrame(pl, 86.0f, EQUIP_RATTLE_ALL);
            return;
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 42, 28);
            return;
        case 1408:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 6, 14);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 2);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
            PlayerSe_AtFrame(pl, 58, 69);
            PlayerSe_WeaponAtFrame(pl, 60, 20);
            PlayerSe_WeaponAtFrame(pl, 72, 9);
            PlayerSe_WeaponAtFrame(pl, 144, 5);
            PlayerSe_VoiceAtFrame(pl, 154.0f, 2);
            PlayerSe_EquipAtFrame(pl, 156.0f, EQUIP_RATTLE_ALL);
            return;
        case 1409:
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 2);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 2);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 38.0f, 3);
            PlayerSe_AtFrame(pl, 38, 69);
            PlayerSe_WeaponAtFrame(pl, 40, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_VoiceAtFrame(pl, 110.0f, 1);
            PlayerSe_WeaponAtFrame(pl, 44, 6);
            PlayerSe_WeaponAtFrame(pl, 94, 5);
            return;
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 1415:
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 26, 68);
            PlayerSe_WeaponAtFrame(pl, 78, 10);
            return;
        case 1416:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            return;
        case 1417:
            PlayerSe_WeaponAtFrame(pl, 88, 5);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 3);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 1);
            return;
        case 1426:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        }
        break;
    case WEAPON_SWORD_SHIELD:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 28, 2);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 1005:
        case 1007:
            PlayerSe_AtFrame(pl, 2, 74);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 1);
            return;
        case 1009:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 3);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            return;
        case 1010:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 1012:
        case 1013:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 20, 65);
            return;
        case 1018:
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            return;
        case 1401:
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 2, 36, 1));
            PlayerSe_WeaponAtFrame(pl, 14, 4);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            return;
        case 1402:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_WeaponAtFrame(pl, 4, 4);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 1);
            return;
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(pl, 54, Sound_RandomSe(37, 2, 37, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 3);
            PlayerSe_WeaponAtFrame(pl, 18, 6);
            PlayerSe_WeaponAtFrame(pl, 40, 5);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 3);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_ARMS);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 108.0f, 0);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 8, 6);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 24, 5);
            PlayerSe_VoiceAtFrame(pl, 6.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 76.0f, 0);
            return;
        case 1405:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(37, 4, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 6);
            PlayerSe_WeaponAtFrame(pl, 14, 5);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 84.0f, 0);
            PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_ARMS);
            return;
        case 1406:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 68.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 10, 5);
            return;
        case 1408:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            return;
        case 1413:
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 3);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 1415:
            PlayerSe_VoiceAtFrame(pl, 30.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 124.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 46, 64);
            return;
        case 1416:
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(pl, 14, 4);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            return;
        case 1417:
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 3, 35, 3));
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            return;
        }
        break;
    case WEAPON_DUAL_SWORDS:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_WeaponAtFrame(pl, 8, 1);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 18, 2);
            PlayerSe_WeaponAtFrame(pl, 26, 7);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 1009:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 3);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 28, 7);
            return;
        case 1012:
        case 1013:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 20, 65);
            return;
        case 1018:
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_WeaponAtFrame(pl, 22, 6);
            PlayerSe_WeaponAtFrame(pl, 34, 12);
            PlayerSe_AtFrame(pl, 34, 54);
            PlayerSe_AtFrame(pl, 32, 63);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            return;
        case 1019:
            PlayerSe_WeaponAtFrame(pl, 8, 5);
            PlayerSe_WeaponAtFrame(pl, 8, 9);
            PlayerSe_AtFrame(pl, 10, 63);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
            return;
        case 1401:
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_WeaponAtFrame(pl, 14, 8);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            return;
        case 1402:
            PlayerSe_WeaponAtFrame(pl, 14, 8);
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 0);
            PlayerSe_AtFrame(pl, 30, 68);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 54, 37);
            PlayerSe_WeaponAtFrame(pl, 4, 3);
            PlayerSe_WeaponAtFrame(pl, 16, 6);
            PlayerSe_WeaponAtFrame(pl, 44, 5);
            PlayerSe_WeaponAtFrame(pl, 64, 9);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 124.0f, 0);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_ARMS);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 6, 1);
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            PlayerSe_WeaponAtFrame(pl, 30, 9);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 106.0f, 0);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
            return;
        case 1406:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 68.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 16, 36);
            PlayerSe_WeaponAtFrame(pl, 10, 5);
            return;
        case 1408:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            return;
        case 1405:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(37, 4, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 6, 3);
            PlayerSe_WeaponAtFrame(pl, 18, 5);
            PlayerSe_WeaponAtFrame(pl, 30, 9);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 2);
            PlayerSe_AtFrame(pl, 40, 61);
            PlayerSe_WeaponAtFrame(pl, 66, 58);
            PlayerSe_VoiceAtFrame(pl, 68.0f, 0);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 1411:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(37, 4, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 6, 3);
            PlayerSe_WeaponAtFrame(pl, 18, 9);
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 2);
            PlayerSe_AtFrame(pl, 40, 61);
            PlayerSe_AtFrame(pl, 66, 58);
            PlayerSe_VoiceAtFrame(pl, 68.0f, 0);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(pl, 54, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(pl, 92, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(pl, 126, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(pl, 8, 8);
            PlayerSe_WeaponAtFrame(pl, 26, 4);
            PlayerSe_WeaponAtFrame(pl, 48, 4);
            PlayerSe_WeaponAtFrame(pl, 54, 8);
            PlayerSe_WeaponAtFrame(pl, 66, 8);
            PlayerSe_WeaponAtFrame(pl, 72, 4);
            PlayerSe_WeaponAtFrame(pl, 88, 8);
            PlayerSe_WeaponAtFrame(pl, 100, 4);
            PlayerSe_WeaponAtFrame(pl, 118, 9);
            PlayerSe_WeaponAtFrame(pl, 114, 5);
            PlayerSe_AtFrame(pl, 114, 63);
            PlayerSe_AtFrame(pl, 122, 60);
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 126.0f, EQUIP_RATTLE_ALL);
            return;
        case 1409:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(pl, 24, 5);
            PlayerSe_WeaponAtFrame(pl, 36, 9);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 2);
            PlayerSe_AtFrame(pl, 44, 63);
            PlayerSe_AtFrame(pl, 16, 58);
            PlayerSe_AtFrame(pl, 70, 68);
            PlayerSe_VoiceAtFrame(pl, 86.0f, 0);
            return;
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(pl, 24, 9);
            PlayerSe_WeaponAtFrame(pl, 36, 5);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 2);
            PlayerSe_AtFrame(pl, 44, 63);
            PlayerSe_AtFrame(pl, 16, 58);
            PlayerSe_AtFrame(pl, 70, 68);
            PlayerSe_VoiceAtFrame(pl, 86.0f, 0);
            return;
        case 1412:
            PlayerSe_VoiceAtFrame(pl, 16.0f, 2);
            PlayerSe_AtFrame(pl, 18, 68);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            return;
        case 1413:
            PlayerSe_WeaponAtFrame(pl, 14, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_WeaponAtFrame(pl, 8, 1);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            return;
        }
        break;
    case WEAPON_LONG_SWORD:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 20, 6);
            PlayerSe_WeaponAtFrame(pl, 20, 8);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 38, 1);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 3);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 0);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 1);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 2);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 2);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 1005:
            PlayerSe_WeaponAtFrame(pl, 8, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 3);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            return;
        case 1006:
        case 1007:
            PlayerSe_WeaponAtFrame(pl, 12, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_AtFrame(pl, 20, 65);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_BODY);
            PlayerSe_WeaponAtFrame(pl, 60, 6);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 126.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
            return;
        case 1012:
        case 1013:
            PlayerSe_WeaponAtFrame(pl, 12, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_AtFrame(pl, 20, 65);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_BODY);
            PlayerSe_WeaponAtFrame(pl, 58, 6);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 126.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
            return;
        case 1017:
            PlayerSe_WeaponAtFrame(pl, 8, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 3);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            return;
        case 1401:
            PlayerSe_WeaponAtFrame(pl, 6, 8);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_WeaponAtFrame(pl, 50, Sound_RandomSe(35, 4, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 66, 7);
            PlayerSe_WeaponAtFrame(pl, 132, 6);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 2);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 1402:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 5);
            PlayerSe_WeaponAtFrame(pl, 148, 6);
            PlayerSe_WeaponAtFrame(pl, 50, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 60, 7);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 3);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            return;
        case 1403:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 18, 4);
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 38.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 76.0f, 0);
            PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 10, 4);
            PlayerSe_WeaponAtFrame(pl, 40, 2);
            PlayerSe_WeaponAtFrame(pl, 46, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 128, 6);
            PlayerSe_VoiceAtFrame(pl, 52.0f, 3);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 112.0f, 0);
            PlayerSe_EquipAtFrame(pl, 130.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 132.0f, 1);
            return;
        case 1405:
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_WeaponAtFrame(pl, 16, 2);
            PlayerSe_WeaponAtFrame(pl, 102, 6);
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(pl, 40, 6);
            PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 116.0f, 1);
            return;
        case 1406:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 114, 6);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            return;
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 4, 6);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 80, 8);
            PlayerSe_WeaponAtFrame(pl, 118, 6);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 0);
            PlayerSe_EquipAtFrame(pl, 100.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 120.0f, 1);
            return;
        case 1408:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 34, 2);
            PlayerSe_WeaponAtFrame(pl, 44, 6);
            PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(35, 4, 35, 4));
            PlayerSe_WeaponAtFrame(pl, 40, Sound_RandomSe(36, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 90, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(pl, 56, 8);
            PlayerSe_WeaponAtFrame(pl, 70, 4);
            PlayerSe_WeaponAtFrame(pl, 96, 9);
            PlayerSe_WeaponAtFrame(pl, 198, 6);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 2);
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 90.0f, 3);
            PlayerSe_EquipAtFrame(pl, 92.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 192.0f, 1);
            return;
        case 1409:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 40, 4);
            PlayerSe_WeaponAtFrame(pl, 72, 7);
            PlayerSe_WeaponAtFrame(pl, 54, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(pl, 158, 6);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 3);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
            return;
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 18, 4);
            return;
        case 1413:
            PlayerSe_VoiceAtFrame(pl, 26.0f, 2);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 58.0f, EQUIP_RATTLE_ALL);
            return;
        case 1414:
            PlayerSe_WeaponAtFrame(pl, 6, 8);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            return;
        case 1415:
            PlayerSe_WeaponAtFrame(pl, 16, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 1);
            PlayerSe_WeaponAtFrame(pl, 24, 4);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 3);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 1416:
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 4, 36, 4));
            return;
        case 1417:
        case 1418:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 16, 4);
            return;
        case 1419:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            return;
        case 1420:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            return;
        case 1421:
        case 1422:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            return;
        case 1423:
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_WeaponAtFrame(pl, 12, 2);
            return;
        case 1424:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            return;
        case 1425:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 10, 4);
            return;
        case 1426:
            PlayerSe_WeaponAtFrame(pl, 4, 6);
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            PlayerSe_WeaponAtFrame(pl, 38, 2);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
            return;
        }
        break;
    default:
        switch (work->motion) {
        case 1002:
            PlayerSe_WeaponAtFrame(pl, 32, 12);
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 3);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            return;
        case 1003:
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 32, 1);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 1004:
            PlayerSe_VoiceAtFrame(pl, 62.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 1012:
        case 1013:
            PlayerSe_WeaponAtFrame(pl, 12, Sound_RandomSe(36, 2, 35, 2));
            PlayerSe_AtFrame(pl, 20, 65);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_BODY);
            PlayerSe_WeaponAtFrame(pl, 58, 1);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 126.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
            return;
        case 1016:
            PlayerSe_WeaponAtFrame(pl, 12, 1);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 1017:
            PlayerSe_WeaponAtFrame(pl, 8, 3);
            PlayerSe_WeaponAtFrame(pl, 42, 1);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            return;
        case 1018:
            PlayerSe_WeaponAtFrame(pl, 24, 1);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_ARMS);
            return;
        case 1401:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 1428:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            PlayerSe_WeaponAtFrame(pl, 32, Sound_RandomSe(37, 3, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 40, 7);
            PlayerSe_WeaponAtFrame(pl, 140, 1);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 118.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 148.0f, 0);
            PlayerSe_EquipAtFrame(pl, 132.0f, EQUIP_ARMS);
            return;
        case 1418:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            return;
        case 1429:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 1424:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            return;
        case 1430:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 1404:
            PlayerSe_WeaponAtFrame(pl, 84, 1);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 100.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 230.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            return;
        case 1405:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 1406:
            PlayerSe_WeaponAtFrame(pl, 48, Sound_RandomSe(37, 3, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            PlayerSe_WeaponAtFrame(pl, 130, 1);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 76.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 158.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 184.0f, 1);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 76.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_ARMS);
            PlayerSe_EquipAtFrame(pl, 178.0f, EQUIP_ARMS);
            return;
        case 1407:
            PlayerSe_WeaponAtFrame(pl, 76, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 228, 38);
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 50, 5);
            PlayerSe_WeaponAtFrame(pl, 114, 8);
            PlayerSe_WeaponAtFrame(pl, 190, 1);
            PlayerSe_WeaponAtFrame(pl, 224, 2);
            PlayerSe_WeaponAtFrame(pl, 272, 1);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 110.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 264.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 282.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 110.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 198.0f, EQUIP_ARMS);
            PlayerSe_EquipAtFrame(pl, 274.0f, EQUIP_ARMS);
            return;
        case 1410:
            PlayerSe_WeaponAtFrame(pl, 34, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 26, 2);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            return;
        case 1411:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(35, 3, 36, 1));
            PlayerSe_AtFrame(pl, 6, 61);
            PlayerSe_WeaponAtFrame(pl, 48, 1);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            return;
        case 1412:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_WeaponAtFrame(pl, 20, 6);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 1);
            return;
        case 1414:
            PlayerSe_WeaponAtFrame(pl, 20, 1);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 284.0f, EQUIP_ARMS);
            return;
        case 1415:
            PlayerSe_WeaponAtFrame(pl, 56, 1);
            PlayerSe_WeaponAtFrame(pl, 80, 2);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 52.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
            return;
        case 1416:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 48, 5);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            return;
        case 1417:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 1419:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            return;
        case 1420:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            return;
        case 1421:
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            return;
        case 1422:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 48, 5);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            return;
        case 1423:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            return;
        case 1425:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            return;
        case 1426:
            PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
            return;
        case 1427:
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            return;
        case 1431:
            PlayerSe_WeaponAtFrame(pl, 6, 1);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            break;
        }
        break;
    }
}

/* An empty handler: the first three entries of the player handler table (pl unused). */
void Player_NoHandler(Player *pl)
{
}

/* The last of the player's handlers: the first call clears the team item timer, then each
 * frame plays the effects and the sounds of the motion. */
void Player_MotionFxStep(Player *pl)
{
    PlayerWork *work = &pl->work;

    switch (work->state) {
    case 0:
        work->state++;
        work->teamFxTimer = 0;
        break;
    case 1:
        Player_MotionEffects(pl, work);
        Player_MotionSounds(pl, work);
        break;
    }
}

/* Spawns a ground effect at a bone of the player obj once its motion passes frame: kind 0-8
 * picks the effect's type (PartFx_SpawnForPlayer's 0, 3, 4, 4, 5, 6, 7, 8, 20; kind 2 always
 * at bone 5). The frame is the second parameter for the match: it goes in f12 in either place,
 * but this order gives the order of the callers' set-up (Player_MotionEffects' loop for motion
 * 1415). */
void Player_EffectAtFrame(void *obj, f32 frame, s32 bone, s16 kind)
{
    if (Motion_PassedFrame(obj, 0, frame) != 0) {
        switch (kind) {
        case 0:
            PartFx_SpawnForPlayer(obj, bone, 0);
            break;
        case 1:
            PartFx_SpawnForPlayer(obj, bone, 3);
            break;
        case 2:
            PartFx_SpawnForPlayer(obj, 5, 4);
            break;
        case 3:
            PartFx_SpawnForPlayer(obj, bone, 4);
            break;
        case 4:
            PartFx_SpawnForPlayer(obj, bone, 5);
            break;
        case 5:
            PartFx_SpawnForPlayer(obj, bone, 6);
            break;
        case 6:
            PartFx_SpawnForPlayer(obj, bone, 7);
            break;
        case 7:
            PartFx_SpawnForPlayer(obj, bone, 8);
            break;
        case 8:
            PartFx_SpawnForPlayer(obj, bone, 20);
            break;
        }
    }
}

/* Shows the first mesh of the player's two mesh pairs (Obj_ShowMeshInRange 18 and 14: meshes 2
 * and 4). No caller seen; work is unused. */
void Player_ResetMeshPairs(void *obj, PlayerWork *work)
{
    Obj_ShowMeshInRange(obj, 18, 0);
    Obj_ShowMeshInRange(obj, 14, 0);
}

void PartFx_SpawnForPlayerScaled(Player *p, s16 arg, s32 type, f32 scale);
void PartFx_SpawnAt(f32 *pos, s32 kind, f32 scale);
void Player_SpawnGroundEffect(Player *p, s32 kind, f32 scale);
void WeaponTrail_CreateTwoMotions(Em *em, s32 a1, s32 a2, s16 v); /* game em_bulldrome.c; called here with a Player */

/*
 * The effects of a hunter's motions, by the motion (work->motion) and the weapon type. First,
 * for a hunter of another console that isn't fainted: when a hunter of the same side in the
 * same area has used an item that reaches this one (its teamItemMask holds this one's bit,
 * which is cleared here), the item's glow shows on this one: the shared items (with the
 * sharing skills) and the team items, at most one glow every 60 frames. Then
 * the motion's effects at their frames (Player_EffectAtFrame and the others).
 */
void Player_MotionEffects(Player *pl, PlayerWork *work)
{
    Player *mate;
    s16 i;
    f32 atFrame;
    f32 point[3];
    f32 offset[3];

    if ((s32)pl->motionId[0] != work->motion) {
        work->unk08 = 0;
        work->motion = pl->motionId[0];
    }
    if (work->teamFxTimer != 0) {
        work->teamFxTimer--;
    }
    if (Player_IsLocal(pl) != 1 && pl->stateGroup != PL_GROUP_FAINT) {
        for (i = 0; i < gScene.playerCount; i++) {
            mate = &gPlayers[i];
            if (Player_OnSameSide(mate, pl) && i != pl->index && mate->area == pl->area
                && (mate->teamItemMask & (1 << pl->index)) && mate->stateGroup == PL_GROUP_ACTION) {
                mate->teamItemMask &= ~(1 << pl->index);
                switch (mate->state) {
                case PL_ACT_APPLY_ITEM:
                    switch (mate->useItem) {
                    case ITEM_ID_HEAL_50_TEAM:
                        if (work->teamFxTimer == 0) {
                            work->teamFxTimer = 60;
                            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
                        }
                        break;
                    case ITEM_ID_HEAL_30:
                        if (Player_HasSkill(mate, SKILL_ITEM_SHARE_HALF) == 1 || Player_HasSkill(mate, SKILL_ITEM_SHARE_FULL) == 1) {
                            if (work->teamFxTimer == 0) {
                                work->teamFxTimer = 60;
                                Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
                            }
                        }
                        break;
                    case ITEM_ID_HEAL_20:
                        if (Player_HasSkill(mate, SKILL_ITEM_SHARE_HALF) == 1 || Player_HasSkill(mate, SKILL_ITEM_SHARE_FULL) == 1) {
                            if (work->teamFxTimer == 0) {
                                work->teamFxTimer = 60;
                                Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
                            }
                        }
                        break;
                    case ITEM_ID_CURE_POISON:
                        if (Player_HasSkill(mate, SKILL_ITEM_SHARE_HALF) == 1 || Player_HasSkill(mate, SKILL_ITEM_SHARE_FULL) == 1) {
                            if (work->teamFxTimer == 0) {
                                work->teamFxTimer = 60;
                                Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_CURE, 10, 4.0f);
                            }
                        }
                        break;
                    case ITEM_ID_ATTACK_SHARED:
                        if (Player_HasSkill(mate, SKILL_ITEM_SHARE_HALF) == 1 || Player_HasSkill(mate, SKILL_ITEM_SHARE_FULL) == 1) {
                            if (work->teamFxTimer == 0) {
                                work->teamFxTimer = 60;
                                Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
                            }
                        }
                        break;
                    case ITEM_ID_DEFENSE_SHARED:
                        if (Player_HasSkill(mate, SKILL_ITEM_SHARE_HALF) == 1 || Player_HasSkill(mate, SKILL_ITEM_SHARE_FULL) == 1) {
                            if (work->teamFxTimer == 0) {
                                work->teamFxTimer = 60;
                                Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
                            }
                        }
                        break;
                    }
                    break;
                case PL_ACT_TEAM_ITEM_END:
                    if (work->teamFxTimer == 0) {
                        work->teamFxTimer = 60;
                        switch (mate->useItem) {
                        case ITEM_ID_TEAM_HEAL:
                            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
                            break;
                        case ITEM_ID_TEAM_CURE:
                            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_CURE, 10, 4.0f);
                            break;
                        case ITEM_ID_TEAM_ATTACK:
                            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
                            break;
                        case ITEM_ID_TEAM_DEFENSE:
                            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    switch (work->motion) {
    case 2:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 20.0f, 8, 8);
            Player_EffectAtFrame(pl, 54.0f, 5, 8);
            Player_EffectAtFrame(pl, 88.0f, 8, 8);
        }
        break;
    case 3:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 12.0f, 8, 0);
            Player_EffectAtFrame(pl, 16.0f, 5, 0);
            Player_EffectAtFrame(pl, 32.0f, 8, 0);
            Player_EffectAtFrame(pl, 56.0f, 5, 0);
        }
        break;
    case 4:
        Player_EffectAtFrame(pl, 12.0f, 5, 1);
        Player_EffectAtFrame(pl, 30.0f, 8, 1);
        Player_EffectAtFrame(pl, 50.0f, 5, 1);
        break;
    case 5:
        if (Motion_PassedFrame(pl, 0, 2.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 3, 0.7f);
        }
        if (Motion_PassedFrame(pl, 0, 20.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 5, 3, 0.7f);
        }
        Player_EffectAtFrame(pl, 30.0f, 8, 3);
        break;
    case 13:
        Player_EffectAtFrame(pl, 2.0f, 10, 6);
        break;
    case 15:
        Player_EffectAtFrame(pl, 2.0f, 8, 1);
        break;
    case 19:
        Player_EffectAtFrame(pl, 2.0f, 8, 7);
        break;
    case 20:
        Player_EffectAtFrame(pl, 2.0f, 8, 7);
        break;
    case 28:
        if (Motion_PassedFrame(pl, 0, 22.0f)) {
            PartFx_SpawnForPlayer(pl, 10, 11);
        }
        Player_EffectAtFrame(pl, 4.0f, 10, 0);
        break;
    case 30:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 4.0f, 8, 2);
        }
        break;
    case 31:
        Player_EffectAtFrame(pl, 16.0f, 2, 4);
        Player_EffectAtFrame(pl, 20.0f, 2, 4);
        break;
    case 32:
        if (((s32)pl->motionTime % 3) == 0) {
            PartFx_SpawnForPlayer(pl, 2, 5);
        }
        break;
    case 37:
        Player_EffectAtFrame(pl, 12.0f, 8, 8);
        Player_EffectAtFrame(pl, 32.0f, 5, 8);
        break;
    case 38:
        if (Motion_PassedFrame(pl, 0, 4.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 0, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 20.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 5, 0, 1.5f);
        }
        break;
    case 39:
        if (Motion_PassedFrame(pl, 0, 2.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 5, 3, 0.8f);
        }
        if (Motion_PassedFrame(pl, 0, 26.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 3, 0.8f);
        }
        if (Motion_PassedFrame(pl, 0, 54.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 5, 3, 0.8f);
        }
        if (Motion_PassedFrame(pl, 0, 78.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 3, 0.8f);
        }
        break;
    case 42:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            Player_EffectAtFrame(pl, 46.0f, 8, 1);
        }
        break;
    case 44:
    case 45:
        if (work->motion == 44) {
            Player_EffectAtFrame(pl, 30.0f, 5, 3);
        } else {
            Player_EffectAtFrame(pl, 30.0f, 8, 3);
        }
        if ((Motion_PassedFrame(pl, 0, 30.0f)) && !(Rand_Next(1) & 0xFFFF & 3)) {
            FallingDebris_Create((Em *) pl, 0);
        }
        break;
    case 46:
        Player_EffectAtFrame(pl, 10.0f, 5, 1);
        break;
    case 48:
        if ((Motion_ReachedFrame(pl, 0, 16.0f) == 0) && (gScene.frame & 1)) {
            PartFx_SpawnForPlayer(pl, 10, 10);
        }
        break;
    case 50:
        Player_EffectAtFrame(pl, 30.0f, 8, 3);
        break;
    case 52:
        Player_EffectAtFrame(pl, 16.0f, 8, 8);
        Player_EffectAtFrame(pl, 46.0f, 5, 8);
        break;
    case 53:
        Player_EffectAtFrame(pl, 28.0f, 8, 1);
        Player_EffectAtFrame(pl, 8.0f, 5, 1);
        break;
    case 57:
        Player_EffectAtFrame(pl, 4.0f, 2, 6);
        break;
    case 58:
        if (Motion_PassedFrame(pl, 0, 6.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.7f);
        }
        break;
    case 59:
        Player_EffectAtFrame(pl, 16.0f, 8, 1);
        Player_EffectAtFrame(pl, 2.0f, 5, 1);
        break;
    case 60:
        if ((Motion_ReachedFrame(pl, 0, 18.0f)) && !(gScene.frame & 0x1F)) {
            PartFx_SpawnForPlayerScaled(pl, 14, 26, 0.4f);
        }
        break;
    case 62:
        if (Motion_PassedFrame(pl, 0, 2.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 10, 7, 0.6f);
        }
        break;
    case 202:
        if (Motion_PassedFrame(pl, 0, 24.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.5f);
        }
        break;
    case 204:
        if (Motion_PassedFrame(pl, 0, 16.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.8f);
        }
        break;
    case 206:
        if (Motion_PassedFrame(pl, 0, 6.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 10, 7, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 34.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 10, 17, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 50.0f) || Motion_PassedFrame(pl, 0, 54.0f) || Motion_PassedFrame(pl, 0, 58.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 18, 1.0f);
        }
        break;
    case 209:
        if (Motion_PassedFrame(pl, 0, 6.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 20, 7, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 28.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 10, 7, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 42.0f) || Motion_PassedFrame(pl, 0, 46.0f) || Motion_PassedFrame(pl, 0, 50.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 18, 1.0f);
        }
        break;
    case 210:
        if (Motion_PassedFrame(pl, 0, 152.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 1.0f);
        }
        break;
    case 215:
        Player_EffectAtFrame(pl, 2.0f, 19, 5);
        break;
    case 222:
        if (Motion_PassedFrame(pl, 0, 40.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 1.0f);
        }
        if (Motion_PassedFrame(pl, 0, 46.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 20, 1.2f);
        }
        break;
    case 250:
        if (Motion_PassedFrame(pl, 0, 250.0f)) {
            ModelObj_SpawnOn(pl, 6);
            ModelObj_SpawnOn(pl, 7);
        }
        break;
    case 251:
        if ((Motion_PassedFrame(pl, 0, 536.0f)) && (Obj_InCurrentArea(pl) & 0xFF)) {
            Em_GetBonePos(pl, 10, point);
            PartFx_SpawnAt(point, 7, 0.8f);
        }
        if ((Motion_PassedFrame(pl, 0, 620.0f)) && (Obj_InCurrentArea(pl) & 0xFF)) {
            Em_GetBonePos(pl, 10, point);
            PartFx_SpawnAt(point, 7, 1.0f);
        }
        if ((Motion_PassedFrame(pl, 0, 698.0f)) && (Obj_InCurrentArea(pl) & 0xFF)) {
            Em_GetBonePos(pl, 2, point);
            PartFx_SpawnAt(point, 7, 0.6f);
        }
        break;
    case 413:
        if (Motion_PassedFrame(pl, 0, 274.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 14, 21, 1.0f);
        }
        break;
    case 415:
        if ((Motion_PassedFrame(pl, 0, 50.0f)) && ((Obj_InCurrentArea(pl) & 0xFF) == 1)) {
            Vec_Set(point, 0.0f, 0.0f, 120.0f);
            Vec_Rotate(offset, point, (f32 *)pl->worldMtx);
            point[0] = pl->pos[0] + offset[0];
            point[1] = pl->pos[1] + offset[1];
            point[2] = pl->pos[2] + offset[2];
            HitSpark_SpawnAt(point, 7, pl->rot[1]);
        }
        break;
    case 416:
        if ((pl->useItem == ITEM_ID_FOOD_STAMINA_FULL) && ((Motion_PassedFrame(pl, 0, 16.0f)) || (Motion_PassedFrame(pl, 0, 22.0f)) || (Motion_PassedFrame(pl, 0, 28.0f)))) {
            ItemProp_CreateAtPlayer(pl, ITEMPROP_EAT_PUFF);
        }
        break;
    case 418:
        if (Motion_PassedFrame(pl, 0, 36.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 18, 24, 0.3f);
        }
        break;
    case 426:
        if (Motion_PassedFrame(pl, 0, 190.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 23, 0.6f);
        }
        break;
    case 431:
        if (!(gScene.frame & 0x1F)) {
            PartFx_SpawnForPlayerScaled(pl, 20, 22, 1.0f);
        }
        break;
    case 432:
        Player_EffectAtFrame(pl, 78.0f, 1, 6);
        break;
    case 647:
        if (Motion_PassedFrame(pl, 0, 236.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0, 33, 0.7f);
        }
        if (Motion_PassedFrame(pl, 0, 314.0f)) {
            ModelObj_SpawnOn(pl, 6);
            ModelObj_SpawnOn(pl, 7);
        }
        break;
    case 800:
        if (Motion_PassedFrame(pl, 0, 24.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0, 25, 0.8f);
        }
        break;
    case 801:
        if ((Motion_PassedFrame(pl, 0, 20.0f)) && (pl->cookResult == 4)) {
            Effect06_SpawnAtBone(pl, 10, 0, 14, 1.0f);
        }
        break;
    case 803:
        if ((work->motion == 803) && (((s32) gScene.frame % 20) == 0)) {
            FireBlast_CreateOnEm((Em *) pl, 6);
        }
        break;
    case 810:
        if (Motion_PassedFrame(pl, 0, 52.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.5f);
        }
        break;
    case 893:
        if (Motion_PassedFrame(pl, 0, 44.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 1.5f);
        }
        break;
    }
    switch (pl->weaponType) {
    case WEAPON_HEAVY_BOWGUN:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 2.0f, 5, 8);
            Player_EffectAtFrame(pl, 32.0f, 8, 8);
            return;
        case 1009:
            Player_EffectAtFrame(pl, 22.0f, 8, 8);
            Player_EffectAtFrame(pl, 76.0f, 8, 8);
            Player_EffectAtFrame(pl, 50.0f, 5, 8);
            Player_EffectAtFrame(pl, 104.0f, 5, 8);
            return;
        case 1401:
        case 1402:
        case 1403:
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 1408:
            Player_EffectAtFrame(pl, 64.0f, 8, 1);
            Player_EffectAtFrame(pl, 50.0f, 5, 1);
            Player_EffectAtFrame(pl, 54.0f, 5, 1);
            return;
        case 1409:
        case 1410:
            Player_EffectAtFrame(pl, 8.0f, 8, 1);
            Player_EffectAtFrame(pl, 8.0f, 5, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_HAMMER:
        switch (work->motion) {
        case 1004:
        case 1005:
            Player_EffectAtFrame(pl, 4.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 1);
            return;
        case 1009:
            Player_EffectAtFrame(pl, 60.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 8);
            Player_EffectAtFrame(pl, 76.0f, 5, 8);
            return;
        case 1011:
            Player_EffectAtFrame(pl, 4.0f, 10, 0);
            if (Motion_PassedFrame(pl, 0, 20.0f)) {
                PartFx_SpawnForPlayer(pl, 10, 11);
            }
            break;
        case 1012:
        case 1013:
            if (work->motion == 1012) {
                Player_EffectAtFrame(pl, 2.0f, 8, 1);
            } else {
                Player_EffectAtFrame(pl, 2.0f, 5, 1);
            }
            Player_EffectAtFrame(pl, 18.0f, 10, 6);
            return;
        case 1402:
            if (Motion_PassedFrame(pl, 0, 68.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 14, 12, 1.8f);
                return;
            }
            break;
        case 1403:
            if ((Motion_ReachedFrame(pl, 0, 1e+01f)) && (Motion_ReachedFrame(pl, 0, 66.0f) == 0) && !(gScene.frame & 3)) {
                PartFx_SpawnForPlayer(pl, 2, 13);
                return;
            }
            break;
        case 1404:
            if (Motion_PassedFrame(pl, 0, 38.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 14, 15, 1.5f);
                return;
            }
            break;
        case 1406:
            if (Motion_PassedFrame(pl, 0, 76.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 14, 19, 0.6f);
                return;
            }
            break;
        case 1411:
            if (Motion_PassedFrame(pl, 0, 34.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 10, 12, 1.2f);
                PartFx_SpawnForPlayer(pl, 10, 31);
                return;
            }
            break;
        case 1412:
            if (Motion_PassedFrame(pl, 0, 52.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 14, 12, 0.7f);
                return;
            }
            break;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_LIGHT_BOWGUN:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 1009:
            Player_EffectAtFrame(pl, 22.0f, 5, 1);
            return;
        case 1010:
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            return;
        case 1401:
        case 1402:
        case 1403:
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 1408:
            Player_EffectAtFrame(pl, 64.0f, 8, 1);
            Player_EffectAtFrame(pl, 50.0f, 5, 1);
            Player_EffectAtFrame(pl, 54.0f, 5, 1);
            return;
        case 1409:
        case 1410:
            Player_EffectAtFrame(pl, 8.0f, 8, 1);
            Player_EffectAtFrame(pl, 8.0f, 5, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_GUNLANCE:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 18.0f, 5, 8);
            Player_EffectAtFrame(pl, 50.0f, 8, 8);
            return;
        case 1011:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 20.0f, 5, 8);
            return;
        case 1014:
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 22.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 8);
            return;
        case 1015:
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 22.0f, 8, 1);
            Player_EffectAtFrame(pl, 28.0f, 5, 8);
            return;
        case 1016:
            Player_EffectAtFrame(pl, 4.0f, 8, 1);
            Player_EffectAtFrame(pl, 24.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 3);
            return;
        case 1401:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 14.0f, 5, 3);
            return;
        case 1402:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 10.0f, 5, 3);
            return;
        case 1403:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 10.0f, 5, 3);
            return;
        case 1405:
            Player_EffectAtFrame(pl, 20.0f, 5, 8);
            Player_EffectAtFrame(pl, 24.0f, 8, 1);
            return;
        case 1406:
            Player_EffectAtFrame(pl, 22.0f, 8, 1);
            return;
        case 1408:
            Player_EffectAtFrame(pl, 2.0f, 8, 3);
            Player_EffectAtFrame(pl, 14.0f, 5, 8);
            Player_EffectAtFrame(pl, 48.0f, 5, 3);
            Player_EffectAtFrame(pl, 54.0f, 5, 8);
            Player_EffectAtFrame(pl, 64.0f, 5, 8);
            Player_EffectAtFrame(pl, 74.0f, 5, 3);
            return;
        case 1409:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 24.0f, 5, 3);
            Player_EffectAtFrame(pl, 42.0f, 8, 3);
            Player_EffectAtFrame(pl, 50.0f, 5, 8);
            return;
        case 1417:
            Player_EffectAtFrame(pl, 24.0f, 8, 8);
            Player_EffectAtFrame(pl, 44.0f, 5, 3);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_LANCE:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 18.0f, 8, 8);
            Player_EffectAtFrame(pl, 52.0f, 5, 8);
            return;
        case 1009:
            Player_EffectAtFrame(pl, 18.0f, 5, 8);
            Player_EffectAtFrame(pl, 96.0f, 5, 8);
            Player_EffectAtFrame(pl, 60.0f, 8, 8);
            return;
        case 1010:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 44.0f, 5, 1);
            Player_EffectAtFrame(pl, 46.0f, 5, 1);
            return;
        case 1011:
            Player_EffectAtFrame(pl, 8.0f, 8, 8);
            Player_EffectAtFrame(pl, 36.0f, 5, 8);
            return;
        case 1014:
        case 1015:
            if (work->motion == 1014) {
                Player_EffectAtFrame(pl, 4.0f, 8, 1);
                return;
            }
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            return;
        case 1016:
            if (Motion_PassedFrame(pl, 0, 4.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 8, 3, 0.8f);
            }
            if (Motion_PassedFrame(pl, 0, 8.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 5, 3, 0.8f);
                return;
            }
            break;
        case 1401:
            if (Motion_PassedFrame(pl, 0, 46.0f)) {
                PartFx_SpawnForPlayer(pl, 10, 16);
            }
            if (Motion_PassedFrame(pl, 0, 48.0f)) {
                DustCloud_CreateOnPlayer(pl, 2, 3, 0.6f);
            }
            if (Motion_PassedFrame(pl, 0, 52.0f)) {
                DustCloud_CreateOnPlayer(pl, 3, 3, 0.6f);
                return;
            }
            break;
        case 1402:
            if (Motion_PassedFrame(pl, 0, 4.0f)) {
                DustCloud_CreateOnPlayer(pl, 2, 2, 0.4f);
            }
            if (Motion_PassedFrame(pl, 0, 22.0f)) {
                DustCloud_CreateOnPlayer(pl, 3, 2, 0.4f);
                return;
            }
            break;
        case 1403:
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 14.0f, 5, 1);
            return;
        case 1404:
            Player_EffectAtFrame(pl, 8.0f, 8, 1);
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 1405:
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            Player_EffectAtFrame(pl, 20.0f, 5, 1);
            Player_EffectAtFrame(pl, 24.0f, 8, 1);
            return;
        case 1406:
            Player_EffectAtFrame(pl, 22.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 5, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            return;
        case 1411:
            Player_EffectAtFrame(pl, 16.0f, 8, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            return;
        case 1412:
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            Player_EffectAtFrame(pl, 14.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 1415:
            atFrame = 26.0f;
            i = 0;
            do {
                if (Motion_PassedFrame(pl, 0, atFrame)) {
                    PartFx_SpawnForPlayerScaled(pl, 5, 3, 1.3f);
                    PartFx_SpawnForPlayerScaled(pl, 8, 3, 1.3f);
                }
                atFrame += 4.0f;
                i++;
            } while (i < 9);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_SWORD_SHIELD:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 1009:
            Player_EffectAtFrame(pl, 6.0f, 5, 1);
            return;
        case 1012:
        case 1013:
            if (work->motion == 1012) {
                Player_EffectAtFrame(pl, 2.0f, 8, 1);
                Player_EffectAtFrame(pl, 16.0f, 10, 6);
                return;
            } else {
                Player_EffectAtFrame(pl, 2.0f, 5, 1);
                Player_EffectAtFrame(pl, 18.0f, 10, 6);
            }
            return;
        case 1401:
            if (Motion_PassedFrame(pl, 0, 6.0f)) {
                WeaponTrail_Create(pl, 11, WTRAIL_BLADE);
                return;
            }
            break;
        case 1402:
            if (Motion_PassedFrame(pl, 0, 2.0f)) {
                WeaponTrail_Create(pl, 9, WTRAIL_BLADE);
                return;
            }
            break;
        case 1403:
            if (Motion_PassedFrame(pl, 0, 52.0f)) {
                WeaponTrail_Create(pl, 9, WTRAIL_BLADE);
                return;
            }
            break;
        case 1404:
            if (Motion_PassedFrame(pl, 0, 24.0f)) {
                WeaponTrail_Create(pl, 14, WTRAIL_BLADE);
            }
            Player_EffectAtFrame(pl, 6.0f, 8, 1);
            Player_EffectAtFrame(pl, 26.0f, 5, 3);
            return;
        case 1405:
            if (Motion_PassedFrame(pl, 0, 20.0f)) {
                WeaponTrail_Create(pl, 13, WTRAIL_BLADE);
                return;
            }
            break;
        case 1406:
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_Create(pl, 12, WTRAIL_BLADE);
                return;
            }
            break;
        case 1415:
            atFrame = 46.0f;
            i = 0;
            do {
                Player_EffectAtFrame(pl, atFrame, 5, 1);
                Player_EffectAtFrame(pl, atFrame, 8, 1);
                atFrame += 4.0f;
                i++;
            } while (i < 7);
            return;
        case 1416:
            if (Motion_PassedFrame(pl, 0, 16.0f)) {
                WeaponTrail_Create(pl, 8, WTRAIL_BLADE);
                return;
            }
            break;
        case 1417:
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_CreateTwoMotions((Em *)pl, 12, WTRAIL_BLADE, 1406);
                return;
            }
            break;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_DUAL_SWORDS:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 1009:
            Player_EffectAtFrame(pl, 6.0f, 5, 1);
            return;
        case 1012:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 16.0f, 10, 6);
            return;
        case 1013:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 18.0f, 10, 6);
            return;
        case 1018:
            if (Motion_PassedFrame(pl, 0, 32.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 2, 35, 1.0f);
                return;
            }
            break;
        case 1019:
            if (Motion_PassedFrame(pl, 0, 8.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 2, 39, 1.5f);
                PartFx_SpawnForPlayerScaled(pl, 10, 36, 1.0f);
                return;
            }
            break;
        case 1401:
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            if (Motion_PassedFrame(pl, 0, 14.0f)) {
                WeaponTrail_Create(pl, 5, WTRAIL_BLADE_B);
                return;
            }
            break;
        case 1402:
            Player_EffectAtFrame(pl, 12.0f, 5, 8);
            Player_EffectAtFrame(pl, 34.0f, 5, 1);
            if (Motion_PassedFrame(pl, 0, 2.0f)) {
                WeaponTrail_Create(pl, 6, WTRAIL_BLADE_B);
            }
            if (Motion_PassedFrame(pl, 0, 20.0f)) {
                WeaponTrail_Create(pl, 7, WTRAIL_BLADE);
                return;
            }
            break;
        case 1403:
            Player_EffectAtFrame(pl, 8.0f, 5, 8);
            Player_EffectAtFrame(pl, 60.0f, 8, 3);
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_Create(pl, 7, WTRAIL_BLADE_B);
            }
            if (Motion_PassedFrame(pl, 0, 54.0f)) {
                WeaponTrail_Create(pl, 8, WTRAIL_BLADE);
            }
            if (Motion_PassedFrame(pl, 0, 64.0f)) {
                WeaponTrail_Create(pl, 12, WTRAIL_BLADE_B);
                return;
            }
            break;
        case 1404:
            Player_EffectAtFrame(pl, 14.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 8);
            Player_EffectAtFrame(pl, 38.0f, 5, 3);
            if (Motion_PassedFrame(pl, 0, 36.0f)) {
                WeaponTrail_Create(pl, 7, WTRAIL_BLADE);
            }
            if (Motion_PassedFrame(pl, 0, 36.0f)) {
                WeaponTrail_Create(pl, 7, WTRAIL_BLADE_B);
                return;
            }
            break;
        case 1405:
            Player_EffectAtFrame(pl, 18.0f, 8, 8);
            Player_EffectAtFrame(pl, 28.0f, 5, 3);
            if (Motion_PassedFrame(pl, 0, 14.0f)) {
                WeaponTrail_Create(pl, 13, WTRAIL_BLADE);
            }
            if (Motion_PassedFrame(pl, 0, 24.0f)) {
                WeaponTrail_Create(pl, 10, WTRAIL_BLADE_B);
                return;
            }
            break;
        case 1406:
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_Create(pl, 12, WTRAIL_BLADE);
                return;
            }
            break;
        case 1407:
            if (Motion_PassedFrame(pl, 0, 124.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 10, 38, 1.0f);
                PartFx_SpawnForPlayer(pl, 10, 37);
                return;
            }
            break;
        case 1409:
            Player_EffectAtFrame(pl, 34.0f, 5, 3);
            Player_EffectAtFrame(pl, 60.0f, 5, 1);
            Player_EffectAtFrame(pl, 72.0f, 8, 8);
            return;
        case 1410:
            Player_EffectAtFrame(pl, 34.0f, 8, 3);
            Player_EffectAtFrame(pl, 60.0f, 8, 1);
            Player_EffectAtFrame(pl, 72.0f, 5, 8);
            return;
        case 1411:
            Player_EffectAtFrame(pl, 18.0f, 5, 8);
            Player_EffectAtFrame(pl, 28.0f, 8, 3);
            if (Motion_PassedFrame(pl, 0, 14.0f)) {
                WeaponTrail_Create(pl, 13, WTRAIL_BLADE_B);
            }
            if (Motion_PassedFrame(pl, 0, 24.0f)) {
                WeaponTrail_Create(pl, 10, WTRAIL_BLADE);
                return;
            }
            break;
        case 1412:
            Player_EffectAtFrame(pl, 18.0f, 5, 3);
            return;
        case 1413:
            Player_EffectAtFrame(pl, 16.0f, 5, 3);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_LONG_SWORD:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 4.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 1006:
            Player_EffectAtFrame(pl, 24.0f, 10, 6);
            return;
        case 1007:
            Player_EffectAtFrame(pl, 24.0f, 10, 6);
            return;
        case 1401:
            if (Motion_PassedFrame(pl, 0, 62.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 14, 12, 1.2f);
            }
            Player_EffectAtFrame(pl, 28.0f, 5, 1);
            Player_EffectAtFrame(pl, 34.0f, 8, 8);
            Player_EffectAtFrame(pl, 58.0f, 8, 3);
            return;
        case 1402:
            if (Motion_PassedFrame(pl, 0, 58.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 14, 12, 1.2f);
            }
            Player_EffectAtFrame(pl, 30.0f, 8, 8);
            Player_EffectAtFrame(pl, 54.0f, 8, 3);
            return;
        case 1403:
            Player_EffectAtFrame(pl, 42.0f, 8, 1);
            Player_EffectAtFrame(pl, 68.0f, 8, 3);
            Player_EffectAtFrame(pl, 80.0f, 5, 8);
            return;
        case 1404:
            Player_EffectAtFrame(pl, 50.0f, 8, 3);
            return;
        case 1405:
            Player_EffectAtFrame(pl, 22.0f, 5, 8);
            Player_EffectAtFrame(pl, 28.0f, 8, 3);
            return;
        case 1406:
            Player_EffectAtFrame(pl, 26.0f, 8, 8);
            Player_EffectAtFrame(pl, 34.0f, 8, 3);
            Player_EffectAtFrame(pl, 38.0f, 5, 1);
            return;
        case 1407:
            Player_EffectAtFrame(pl, 22.0f, 5, 8);
            Player_EffectAtFrame(pl, 26.0f, 8, 3);
            return;
        case 1408:
            Player_EffectAtFrame(pl, 14.0f, 8, 3);
            Player_EffectAtFrame(pl, 28.0f, 5, 8);
            Player_EffectAtFrame(pl, 36.0f, 8, 3);
            Player_EffectAtFrame(pl, 68.0f, 5, 8);
            if (Motion_PassedFrame(pl, 0, 94.0f)) {
                PlayerFx_CreateOnPlayerDefault(pl, 10, 1.0f);
                return;
            }
            break;
        case 1413:
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            Player_EffectAtFrame(pl, 44.0f, 5, 3);
            Player_EffectAtFrame(pl, 50.0f, 8, 8);
            return;
        case 1414:
            Player_EffectAtFrame(pl, 6.0f, 8, 3);
            Player_EffectAtFrame(pl, 42.0f, 8, 3);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_HUNTING_HORN:
        switch (work->motion) {
        case 1002:
            Player_EffectAtFrame(pl, 60.0f, 8, 1);
            return;
        case 1003:
            Player_EffectAtFrame(pl, 60.0f, 5, 8);
            return;
        case 1004:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 28.0f, 5, 1);
            return;
        case 1011:
            Player_EffectAtFrame(pl, 30.0f, 10, 6);
            return;
        case 1012:
            Player_EffectAtFrame(pl, 24.0f, 10, 6);
            return;
        case 1013:
            Player_EffectAtFrame(pl, 24.0f, 10, 6);
            return;
        case 1401:
            Player_EffectAtFrame(pl, 14.0f, 8, 8);
            return;
        case 1402:
            Player_EffectAtFrame(pl, 14.0f, 8, 8);
            return;
        case 1403:
            if (Motion_PassedFrame(pl, 0, 84.0f)) {
                Player_SpawnGroundEffect(pl, 19, 0.5f);
                return;
            }
            break;
        case 1404:
            Player_EffectAtFrame(pl, 14.0f, 8, 8);
            return;
        case 1408:
            if (Motion_PassedFrame(pl, 0, 48.0f)) {
                Player_SpawnGroundEffect(pl, 12, 1.5f);
            }
            Player_EffectAtFrame(pl, 70.0f, 8, 1);
            Player_EffectAtFrame(pl, 92.0f, 8, 1);
            Player_EffectAtFrame(pl, 110.0f, 5, 8);
            return;
        case 1409:
            if (Motion_PassedFrame(pl, 0, 50.0f)) {
                Player_SpawnGroundEffect(pl, 12, 1.2f);
            }
            Player_EffectAtFrame(pl, 50.0f, 5, 8);
            Player_EffectAtFrame(pl, 64.0f, 5, 8);
            Player_EffectAtFrame(pl, 80.0f, 8, 3);
            return;
        case 1410:
            if (Motion_PassedFrame(pl, 0, 50.0f)) {
                Player_SpawnGroundEffect(pl, 12, 1.2f);
            }
            Player_EffectAtFrame(pl, 46.0f, 5, 8);
            Player_EffectAtFrame(pl, 68.0f, 8, 8);
            Player_EffectAtFrame(pl, 88.0f, 5, 1);
            return;
        case 1411:
            if (Motion_PassedFrame(pl, 0, 48.0f)) {
                Player_SpawnGroundEffect(pl, 12, 1.5f);
            }
            Player_EffectAtFrame(pl, 46.0f, 5, 8);
            Player_EffectAtFrame(pl, 72.0f, 8, 1);
            Player_EffectAtFrame(pl, 88.0f, 5, 8);
            Player_EffectAtFrame(pl, 102.0f, 8, 8);
            /* fallthrough */
        case 1413:
            Player_EffectAtFrame(pl, 52.0f, 8, 8);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_BOW:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 1);
            return;
        case 1006:
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 8);
            Player_EffectAtFrame(pl, 42.0f, 8, 3);
            return;
        case 1009:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 1);
            return;
        case 1010:
            Player_EffectAtFrame(pl, 16.0f, 5, 8);
            return;
        case 1411:
            Player_EffectAtFrame(pl, 32.0f, 5, 1);
            Player_EffectAtFrame(pl, 48.0f, 8, 3);
            return;
        case 1413:
            if (Motion_PassedFrame(pl, 0, 26.0f)) {
                WeaponTrail_Create(pl, 6, WTRAIL_BOW);
            }
            Player_EffectAtFrame(pl, 34.0f, 5, 8);
            return;
        case 1414:
            if (Motion_PassedFrame(pl, 0, 32.0f)) {
                WeaponTrail_Create(pl, 6, WTRAIL_BOW);
            }
            Player_EffectAtFrame(pl, 34.0f, 5, 8);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    default:
        switch (work->motion) {
        case 1004:
            Player_EffectAtFrame(pl, 62.0f, 8, 8);
            Player_EffectAtFrame(pl, 30.0f, 5, 8);
            return;
        case 1012:
        case 1013:
            if (work->motion == 1012) {
                Player_EffectAtFrame(pl, 6.0f, 5, 0);
            } else {
                Player_EffectAtFrame(pl, 6.0f, 8, 0);
            }
            Player_EffectAtFrame(pl, 18.0f, 10, 6);
            return;
        case 1016:
            Player_EffectAtFrame(pl, 4.0f, 5, 1);
            return;
        case 1017:
            Player_EffectAtFrame(pl, 60.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 8);
            Player_EffectAtFrame(pl, 74.0f, 5, 8);
            return;
        case 1411:
            Player_EffectAtFrame(pl, 10.0f, 5, 1);
            return;
        case 1415:
            atFrame = 64.0f;
            i = 0;
            do {
                if (Motion_PassedFrame(pl, 0, atFrame)) {
                    PartFx_SpawnForPlayerScaled(pl, 5, 3, 1.3f);
                    PartFx_SpawnForPlayerScaled(pl, 8, 3, 1.3f);
                }
                atFrame += 4.0f;
                i++;
            } while (i < 6);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            break;
        }
        break;
    }
}
