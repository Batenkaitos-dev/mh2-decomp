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
 * The sounds are played through the PlayerSe_* helpers, the effects
 * through PartFx_SpawnForPlayer and the ground effects.
 */
#include "common.h"
#include "stageobj.h"
#include "player.h"
#include "em.h"
#include "scene.h"

void Player_MotionSounds(Player *p, PlayerWork *w);
void PlayerSe_AtFrame(Player *p, s32 frame, s32 se);
void Motion_SeAtFrame(Player *p, s32 frame, s32 se);
void Motion_SeAtFrameLocal(Player *p, s32 frame, s32 se);
void Player_SeAtFrame(Player *p, s32 frame, s32 se);
void PlayerSe_WeaponAtFrame(Player *p, s32 frame, s32 se);
void Motion_VoiceAtFrame(Player *p, f32 frame, s32 voice);
void PlayerSe_VoiceAtFrame(Player *p, f32 frame, s32 voice);
void PlayerSe_OnlineAtFrame(Player *p, s32 frame, s32 se);
void PlayerSe_EquipAtFrame(Player *p, f32 frame, s32 kind);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_MotionEffects(Player *p, PlayerWork *w);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
/* Player_EffectAtFrame's calls compile as the game's only with these s32 and
 * u8 parameters; the definition takes (Player *, s16 bone, s32 type). */
/* unit's own declaration: kept for the match */
void PartFx_SpawnForPlayer(Player *p, s32 a, u8 b);
void Obj_ShowMeshInRange(u8 *p, u8 kind, u8 sel);

/*
 * The hunter's motion sounds: per motion (work->motion) the voice, armour
 * rattle and weapon sounds at their frames, first the motions of every
 * weapon, then those of the weapon type in hand. Called after
 * Player_MotionEffects.
 */
void Player_MotionSounds(Player *pl, PlayerWork *work)
{
    switch (work->motion) {
    case 0x2:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 88.0f, 0);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 90.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x3:
        PlayerSe_WeaponAtFrame(pl, 56, Sound_RandomSe(39, 1, 40, 1));
        PlayerSe_VoiceAtFrame(pl, 8.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 2);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x4:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 2);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 66.0f, 2);
        PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x5:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 80, Sound_RandomSe(41, 3, 41, 2));
        PlayerSe_WeaponAtFrame(pl, 164, Sound_RandomSe(41, 3, 41, 2));
        break;
    case 0x6:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x7:
    case 0x9:
        PlayerSe_AtFrame(pl, 2, 74);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
        break;
    case 0xA:
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 1);
        PlayerSe_AtFrame(pl, 38, 68);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 1);
        PlayerSe_AtFrame(pl, 108, 68);
        break;
    case 0xC:
        PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
        PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        break;
    case 0xD:
        PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 4, 58);
        break;
    case 0xE:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 4, 74);
        break;
    case 0xF:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 10.0f, 3);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x10:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
        PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x11:
        PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 2, 75);
        break;
    case 0x12:
        PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(38, 2, 38, 2));
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 88.0f, 3);
        PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 112.0f, 0);
        PlayerSe_AtFrame(pl, 94, 68);
        break;
    case 0x13:
        PlayerSe_AtFrame(pl, 2, 64);
        PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 2);
        PlayerSe_EquipAtFrame(pl, 100.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x14:
        PlayerSe_AtFrame(pl, 2, 66);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
        PlayerSe_VoiceAtFrame(pl, 68.0f, 1);
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 96.0f, 2);
        PlayerSe_EquipAtFrame(pl, 96.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x15:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 76.0f, 0);
        PlayerSe_EquipAtFrame(pl, 76.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x16:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 3);
        PlayerSe_VoiceAtFrame(pl, 14.0f, 3);
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        break;
    case 0x1C:
        PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 3, 36, 1));
        PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
        PlayerSe_AtFrame(pl, 2, 63);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 28, 65);
        PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x1D:
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
    case 0x1E:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 3);
        PlayerSe_EquipAtFrame(pl, 15.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x22:
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) == 0) {
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            Motion_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
            Motion_VoiceAtFrame(pl, 98.0f, 0);
            PlayerSe_EquipAtFrame(pl, 104.0f, EQUIP_ARMS);
        }
        break;
    case 0x23:
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) == 0) {
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
            Motion_VoiceAtFrame(pl, 38.0f, 1);
            PlayerSe_EquipAtFrame(pl, 82.0f, EQUIP_RATTLE_ALL);
            Motion_VoiceAtFrame(pl, 82.0f, 1);
        }
        break;
    case 0x24:
        PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
        PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x25:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 36.0f, 1);
        break;
    case 0x26:
        PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(39, 3, 40, 2));
        PlayerSe_VoiceAtFrame(pl, 16.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 32.0f, 2);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x27:
        PlayerSe_WeaponAtFrame(pl, 42, 41);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 72.0f, 2);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 2);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x29:
        PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
        PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_ARMS);
        break;
    case 0x2A:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x2B:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        break;
    case 0x2C:
    case 0x2D:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
        PlayerSe_AtFrame(pl, 30, 69);
        break;
    case 0x2E:
        PlayerSe_VoiceAtFrame(pl, 4.0f, 2);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x30:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
        PlayerSe_AtFrame(pl, 4, 66);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 60, 75);
        PlayerSe_VoiceAtFrame(pl, 80.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 0);
        PlayerSe_EquipAtFrame(pl, 100.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x31:
        PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 102.0f, 1);
        break;
    case 0x32:
        PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 2));
        PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
        PlayerSe_AtFrame(pl, 20, 62);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x34:
        PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x35:
        PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(pl, 20.0f, 2);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 2);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x36:
        PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 76.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 162.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 192.0f, 1);
        PlayerSe_WeaponAtFrame(pl, 36, Sound_RandomSe(38, 2, 38, 2));
        break;
    case 0x37:
        PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
        break;
    case 0x39:
        PlayerSe_AtFrame(pl, 4, 64);
        PlayerSe_VoiceAtFrame(pl, 88.0f, 0);
        PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x3A:
        PlayerSe_VoiceAtFrame(pl, 6.0f, 3);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x3B:
        PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(pl, 10.0f, 2);
        PlayerSe_EquipAtFrame(pl, 10.0f, Sound_RandomSe(0, 2, 0, 2));
        PlayerSe_VoiceAtFrame(pl, 24.0f, 2);
        PlayerSe_EquipAtFrame(pl, 24.0f, Sound_RandomSe(0, 2, 0, 2));
        break;
    case 0x3C:
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_ARMS);
        break;
    case 0x3E:
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
        PlayerSe_VoiceAtFrame(pl, 10.0f, 2);
        break;
    case 0x3F:
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 70.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 100.0f, 1);
        break;
    case 0x40:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
        PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
        break;
    case 0xCA:
        PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 26, 64);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_BODY);
        break;
    case 0xCC:
        PlayerSe_AtFrame(pl, 14, 64);
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_ARMS);
        break;
    case 0xCE:
        PlayerSe_AtFrame(pl, 4, 60);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 32, 66);
        PlayerSe_AtFrame(pl, 116, 58);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_ARMS);
        break;
    case 0xCF:
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 1);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 74.0f, 2);
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
        break;
    case 0xD1:
        PlayerSe_AtFrame(pl, 4, 60);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 32, 66);
        PlayerSe_AtFrame(pl, 116, 58);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_ARMS);
        break;
    case 0xD2:
        PlayerSe_AtFrame(pl, 66, 58);
        PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 146, 75);
        PlayerSe_EquipAtFrame(pl, 146.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 152, 60);
        break;
    case 0xD3:
        PlayerSe_AtFrame(pl, 12, 74);
        PlayerSe_AtFrame(pl, 20, 75);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        break;
    case 0xD4:
        PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 92.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 94.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 114.0f, 1);
        break;
    case 0xD6:
        PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        break;
    case 0xD7:
        PlayerSe_AtFrame(pl, 4, 60);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_BODY);
        PlayerSe_AtFrame(pl, 106, 64);
        PlayerSe_EquipAtFrame(pl, 110.0f, EQUIP_ARMS);
        break;
    case 0xD8:
        PlayerSe_AtFrame(pl, 14, 68);
        PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 44.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 64.0f, 3);
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
        break;
    case 0xDA:
        PlayerSe_AtFrame(pl, 10, 74);
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_ARMS);
        break;
    case 0xDC:
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        break;
    case 0xDD:
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        break;
    case 0xDE:
        PlayerSe_AtFrame(pl, 38, 66);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_ARMS);
        break;
    case 0xE0:
        PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
        PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 64, 41);
        PlayerSe_AtFrame(pl, 64, 59);
        break;
    case 0xE1:
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 120.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 64, 41);
        PlayerSe_AtFrame(pl, 118, 59);
        PlayerSe_AtFrame(pl, 170, 65);
        break;
    case 0xE2:
        PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
        break;
    case 0xE3:
        PlayerSe_VoiceAtFrame(pl, 60.0f, 2);
        PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 22, 67);
        PlayerSe_AtFrame(pl, 78, 65);
        PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_BODY);
        PlayerSe_EquipAtFrame(pl, 120.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 128, 68);
        break;
    case 0xFA:
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
    case 0xFB:
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
    case 0x101:
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
    case 0x102:
        PlayerSe_AtFrame(pl, 4, 67);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
        break;
    case 0x191:
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
    case 0x193:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 1);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 28, Sound_RandomSe(48, 3, 48, 2));
        PlayerSe_AtFrame(pl, 226, Sound_RandomSe(48, 2, 48, 2));
        break;
    case 0x195:
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
    case 0x196:
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
    case 0x199:
        PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 20, 75);
        break;
    case 0x19A:
        PlayerSe_AtFrame(pl, 42, 75);
        PlayerSe_AtFrame(pl, 78, 75);
        break;
    case 0x19D:
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
    case 0x19E:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 62, 74);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_ARMS);
        break;
    case 0x19F:
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
    case 0x1A0:
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
    case 0x1A1:
        PlayerSe_AtFrame(pl, 12, 62);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 70.0f, 0);
        break;
    case 0x1A2:
        PlayerSe_AtFrame(pl, 32, 42);
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 104, 76);
        PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
        PlayerSe_EquipAtFrame(pl, 104.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 194.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 194.0f, 0);
        break;
    case 0x1A3:
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
    case 0x1A4:
        PlayerSe_AtFrame(pl, 6, 74);
        Motion_SeAtFrame(pl, 70, 119);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x1A5:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x1A6:
        PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 28, 50);
        PlayerSe_AtFrame(pl, 58, 51);
        break;
    case 0x1A7:
        PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x1A8:
        PlayerSe_VoiceAtFrame(pl, 24.0f, 1);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_ARMS);
        Motion_SeAtFrame(pl, 72, 120);
        break;
    case 0x1A9:
        PlayerSe_AtFrame(pl, 58, 74);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 1);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 78.0f, 0);
        PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x1AA:
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
    case 0x1AC:
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_ARMS);
        break;
    case 0x1AD:
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
    case 0x1AF:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 1);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 132, Sound_RandomSe(39, 3, 40, 1));
        PlayerSe_WeaponAtFrame(pl, 242, Sound_RandomSe(39, 1, 40, 3));
        break;
    case 0x1B0:
        PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
        PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 30, 38);
        PlayerSe_VoiceAtFrame(pl, 106.0f, 0);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_ARMS);
        break;
    case 0x258:
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
    case 0x25A:
        PlayerSe_AtFrame(pl, 48, 80);
        PlayerSe_AtFrame(pl, 62, 80);
        break;
    case 0x25B:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 48.0f, 1);
        PlayerSe_AtFrame(pl, 34, 49);
        PlayerSe_AtFrame(pl, 48, 49);
        PlayerSe_AtFrame(pl, 62, 49);
        PlayerSe_EquipAtFrame(pl, 120.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x25C:
        PlayerSe_AtFrame(pl, 12, 68);
        break;
    case 0x25D:
        PlayerSe_VoiceAtFrame(pl, 52.0f, 0);
        PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 22, 74);
        break;
    case 0x25F:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x260:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 1);
        PlayerSe_AtFrame(pl, 68, 74);
        break;
    case 0x262:
        PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 64.0f, 1);
        PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 12, 68);
        break;
    case 0x263:
        PlayerSe_AtFrame(pl, 42, 81);
        PlayerSe_AtFrame(pl, 64, 81);
        PlayerSe_AtFrame(pl, 108, 83);
        PlayerSe_EquipAtFrame(pl, 108.0f, EQUIP_ARMS);
        break;
    case 0x264:
        PlayerSe_AtFrame(pl, 124, 75);
        PlayerSe_AtFrame(pl, 42, 61);
        PlayerSe_AtFrame(pl, 56, 82);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 128, 83);
        break;
    case 0x265:
        PlayerSe_AtFrame(pl, 72, 58);
        PlayerSe_EquipAtFrame(pl, 76.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 132, 84);
        PlayerSe_AtFrame(pl, 206, 85);
        break;
    case 0x266:
        PlayerSe_AtFrame(pl, 38, 68);
        PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_ARMS);
        break;
    case 0x267:
        PlayerSe_AtFrame(pl, 42, 74);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 154.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 220.0f, EQUIP_ARMS);
        break;
    case 0x268:
        PlayerSe_AtFrame(pl, 32, 67);
        PlayerSe_EquipAtFrame(pl, 82.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 102.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 74, 64);
        PlayerSe_AtFrame(pl, 126, 84);
        PlayerSe_AtFrame(pl, 246, 85);
        break;
    case 0x269:
        PlayerSe_AtFrame(pl, 46, 67);
        PlayerSe_VoiceAtFrame(pl, 62.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 102.0f, 3);
        PlayerSe_EquipAtFrame(pl, 104.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 140.0f, 3);
        PlayerSe_EquipAtFrame(pl, 142.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 176.0f, 0);
        PlayerSe_EquipAtFrame(pl, 176.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x26A:
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
    case 0x26B:
        PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 1);
        PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x26C:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 1);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
        PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_ARMS);
        break;
    case 0x26D:
        PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
        PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 10, 68);
        PlayerSe_VoiceAtFrame(pl, 114.0f, 0);
        break;
    case 0x26E:
        PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
        PlayerSe_VoiceAtFrame(pl, 10.0f, 3);
        PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 54, 61);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 66.0f, 3);
        PlayerSe_AtFrame(pl, 66, 58);
        break;
    case 0x26F:
        PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
        PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
        PlayerSe_WeaponAtFrame(pl, 46, 41);
        PlayerSe_AtFrame(pl, 66, 64);
        PlayerSe_AtFrame(pl, 80, 58);
        PlayerSe_EquipAtFrame(pl, 80.0f, EQUIP_ARMS);
        break;
    case 0x270:
        PlayerSe_AtFrame(pl, 4, 68);
        break;
    case 0x271:
        PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
        PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 28, 58);
        PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
        PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 56, 58);
        break;
    case 0x272:
        PlayerSe_VoiceAtFrame(pl, 34.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x273:
        PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
        PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
        PlayerSe_AtFrame(pl, 30, 49);
        PlayerSe_AtFrame(pl, 42, 49);
        break;
    case 0x274:
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
    case 0x275:
        PlayerSe_AtFrame(pl, 316, 69);
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x276:
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
    case 0x277:
        PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 198.0f, 0);
        PlayerSe_EquipAtFrame(pl, 198.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x278:
        PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
        PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 70.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 134.0f, EQUIP_ARMS);
        break;
    case 0x279:
        PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
        PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 42, 62);
        PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_ARMS);
        break;
    case 0x27B:
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
    case 0x27C:
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
    case 0x27D:
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
    case 0x280:
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 14, 69);
        break;
    case 0x282:
        PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 86.0f, 0);
        PlayerSe_EquipAtFrame(pl, 86.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 98.0f, 1);
        PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 116, 83);
        break;
    case 0x283:
        PlayerSe_AtFrame(pl, 24, 83);
        PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
        break;
    case 0x287:
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
    case 0x288:
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
    case 0x2A5:
        PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
        PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
        PlayerSe_AtFrame(pl, 96, 75);
        PlayerSe_EquipAtFrame(pl, 96.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 96, 67);
        PlayerSe_AtFrame(pl, 140, 58);
        PlayerSe_AtFrame(pl, 140, 74);
        break;
    case 0x2A7:
        PlayerSe_AtFrame(pl, 38, 67);
        PlayerSe_VoiceAtFrame(pl, 30.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
        PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 110.0f, 1);
        break;
    case 0x320:
        PlayerSe_AtFrame(pl, 24, 74);
        PlayerSe_AtFrame(pl, 72, 58);
        PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 54.0f, 0);
        break;
    case 0x321:
        PlayerSe_WeaponAtFrame(pl, 4, 37);
        PlayerSe_AtFrame(pl, 4, 74);
        PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
        PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x322:
        PlayerSe_AtFrame(pl, 4, 74);
        PlayerSe_WeaponAtFrame(pl, 56, 41);
        PlayerSe_VoiceAtFrame(pl, 118.0f, 0);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_RATTLE_ALL);
        break;
    case 0x323:
        PlayerSe_AtFrame(pl, 58, 40);
        PlayerSe_AtFrame(pl, 58, 41);
        PlayerSe_AtFrame(pl, 120, 41);
        PlayerSe_AtFrame(pl, 180, 41);
        PlayerSe_AtFrame(pl, 180, 40);
        PlayerSe_AtFrame(pl, 242, 41);
        break;
    case 0x325:
        PlayerSe_VoiceAtFrame(pl, 60.0f, 1);
        PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 94.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 142.0f, 1);
        PlayerSe_AtFrame(pl, 100, 43);
        PlayerSe_EquipAtFrame(pl, 106.0f, EQUIP_ARMS);
        break;
    case 0x327:
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
    case 0x328:
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
    case 0x329:
        PlayerSe_AtFrame(pl, 28, 43);
        PlayerSe_VoiceAtFrame(pl, 66.0f, 0);
        PlayerSe_EquipAtFrame(pl, 94.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 94.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 212.0f, 1);
        PlayerSe_VoiceAtFrame(pl, 226.0f, 1);
        break;
    case 0x32A:
        PlayerSe_AtFrame(pl, 10, 43);
        PlayerSe_AtFrame(pl, 10, 45);
        PlayerSe_AtFrame(pl, 54, 65);
        PlayerSe_EquipAtFrame(pl, 118.0f, EQUIP_RATTLE_ALL);
        PlayerSe_EquipAtFrame(pl, 122.0f, EQUIP_ARMS);
        PlayerSe_VoiceAtFrame(pl, 176.0f, 1);
        break;
    case 0x363:
        PlayerSe_AtFrame(pl, 2, 61);
        PlayerSe_AtFrame(pl, 14, 58);
        PlayerSe_AtFrame(pl, 114, 82);
        PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 130, 82);
        PlayerSe_EquipAtFrame(pl, 312.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 178, 83);
        break;
    case 0x37A:
        PlayerSe_OnlineAtFrame(pl, 160, Sound_RandomSe(0, 4, 1, 4));
        PlayerSe_AtFrame(pl, 28, 74);
        PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 80, 63);
        PlayerSe_AtFrame(pl, 128, 63);
        PlayerSe_AtFrame(pl, 162, 83);
        PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 1);
        break;
    case 0x37B:
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
    case 0x37C:
        PlayerSe_OnlineAtFrame(pl, 4, 3);
        PlayerSe_AtFrame(pl, 8, 83);
        PlayerSe_AtFrame(pl, 8, 59);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 84, 74);
        break;
    case 0x37D:
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
    case 0x37E:
        PlayerSe_OnlineAtFrame(pl, 160, Sound_RandomSe(0, 4, 1, 4));
        PlayerSe_AtFrame(pl, 36, 74);
        PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 80, 63);
        PlayerSe_AtFrame(pl, 114, 63);
        PlayerSe_AtFrame(pl, 166, 83);
        PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_RATTLE_ALL);
        PlayerSe_VoiceAtFrame(pl, 42.0f, 1);
        break;
    case 0x37F:
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
    case 0x380:
        PlayerSe_OnlineAtFrame(pl, 4, 3);
        PlayerSe_AtFrame(pl, 8, 83);
        PlayerSe_AtFrame(pl, 8, 59);
        PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
        PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_ARMS);
        PlayerSe_AtFrame(pl, 84, 74);
        break;
    case 0x381:
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
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 76, 2);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 86, 6);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F1:
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
        case 0x579:
        case 0x57A:
        case 0x57B:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 108.0f, 0);
            PlayerSe_EquipAtFrame(pl, 108.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(pl, 44, 13);
            PlayerSe_WeaponAtFrame(pl, 66, 14);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 104, 4);
            return;
        case 0x57D:
        case 0x57E:
        case 0x57F:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x580:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 0);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 130.0f, 2);
            PlayerSe_EquipAtFrame(pl, 130.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x581:
        case 0x582:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 108.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 136.0f, 0);
            PlayerSe_EquipAtFrame(pl, 136.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(pl, 2, 4);
            PlayerSe_WeaponAtFrame(pl, 68, 9);
            PlayerSe_WeaponAtFrame(pl, 180, 1);
            return;
        case 0x584:
        case 0x588:
        case 0x589:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 0x585:
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 4, 7);
            PlayerSe_WeaponAtFrame(pl, 12, 1);
            PlayerSe_AtFrame(pl, 8, 61);
            return;
        case 0x58A:
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x58B:
            PlayerSe_VoiceAtFrame(pl, 116.0f, 0);
            PlayerSe_EquipAtFrame(pl, 116.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 2, 67);
            return;
        case 0x58E:
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
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 66, 2);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 3);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EC:
        case 0x3ED:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 2);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EE:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 3);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 20, 5);
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            return;
        case 0x3F1:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 0);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F2:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 12, 2);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F3:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 26, 4);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57A:
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
        case 0x57B:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 2);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 2);
            PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            PlayerSe_WeaponAtFrame(pl, 38, 6);
            return;
        case 0x57C:
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
        case 0x57D:
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
        case 0x57E:
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
        case 0x57F:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 38.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x580:
            PlayerSe_VoiceAtFrame(pl, 12.0f, 2);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 62, 1);
            PlayerSe_EquipAtFrame(pl, 90.0f, EQUIP_ARMS);
            return;
        case 0x581:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            return;
        case 0x582:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            return;
        case 0x583:
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
        case 0x584:
            PlayerSe_WeaponAtFrame(pl, 50, Sound_RandomSe(35, 1, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 42, 4);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 56, 7);
            return;
        case 0x585:
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 12, 6);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 2);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            return;
        }
        break;
    case WEAPON_HUNTING_HORN:
        switch (work->motion) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 66, 2);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 3);
            PlayerSe_EquipAtFrame(pl, 58.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 48, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EC:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 18.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 2);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EE:
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 20, 4);
            return;
        case 0x3F3:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 28, 65);
            return;
        case 0x579:
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 70.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            PlayerSe_WeaponAtFrame(pl, 20, 4);
            PlayerSe_WeaponAtFrame(pl, 80, 2);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(pl, 40, Sound_RandomSe(35, 3, 35, 3));
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_WeaponAtFrame(pl, 70, 2);
            return;
        case 0x57B:
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
        case 0x57C:
            PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 62.0f, 0);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 14, 6);
            return;
        case 0x580:
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
        case 0x581:
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
        case 0x582:
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
        case 0x583:
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
        case 0x584:
            PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 3);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            PlayerSe_WeaponAtFrame(pl, 68, 2);
            return;
        case 0x585:
            PlayerSe_VoiceAtFrame(pl, 54.0f, 3);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            PlayerSe_WeaponAtFrame(pl, 60, 2);
            return;
        case 0x586:
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            return;
        case 0x587:
            PlayerSe_WeaponAtFrame(pl, 2, 5);
            return;
        case 0x588:
            break;
        case 0x589:
            PlayerSe_VoiceAtFrame(pl, 10.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            return;
        case 0x58A:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 0x58B:
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 0x58C:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 0x58D:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            return;
        case 0x58E:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 0x58F:
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 0x590:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 0x593:
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 34, 2);
            return;
        case 0x594:
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 34, 2);
            return;
        case 0x595:
            PlayerSe_VoiceAtFrame(pl, 20.0f, 2);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 34.0f, 2);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 34, 2);
            PlayerSe_WeaponAtFrame(pl, 34, 8);
            return;
        case 0x596:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 0x597:
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 12, 1);
            PlayerSe_WeaponAtFrame(pl, 30, 4);
            return;
        case 0x598:
            break;
        case 0x57D:
            Player_SeAtFrame(pl, 12, 24);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 1);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 1);
            return;
        case 0x57E:
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
        case 0x57F:
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
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 60.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EC:
            PlayerSe_WeaponAtFrame(pl, 56, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57D:
        case 0x57E:
        case 0x57F:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F1:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 3);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            return;
        case 0x3F2:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 1);
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 106.0f, 0);
            PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(pl, 44, 13);
            PlayerSe_WeaponAtFrame(pl, 66, 14);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 104, 4);
            return;
        case 0x580:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 0);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 130.0f, 0);
            PlayerSe_EquipAtFrame(pl, 130.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x581:
        case 0x582:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 108.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 136.0f, 0);
            PlayerSe_EquipAtFrame(pl, 136.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(pl, 2, 9);
            PlayerSe_WeaponAtFrame(pl, 180, 7);
            return;
        case 0x584:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 0x585:
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 70, 7);
            PlayerSe_WeaponAtFrame(pl, 20, 1);
            PlayerSe_AtFrame(pl, 8, 61);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x586:
        case 0x587:
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 128.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 4, 3);
            PlayerSe_WeaponAtFrame(pl, 100, 4);
            return;
        case 0x588:
        case 0x589:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 0x58E:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 24, 3);
            return;
        case 0x58F:
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
        case 0x3EA:
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 16, 1);
            PlayerSe_WeaponAtFrame(pl, 32, 2);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 0);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EB:
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 24, 3);
            PlayerSe_WeaponAtFrame(pl, 36, 4);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
            PlayerSe_EquipAtFrame(pl, 66.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3ED:
            PlayerSe_WeaponAtFrame(pl, 8, 3);
            PlayerSe_WeaponAtFrame(pl, 40, 4);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 1);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 86.0f, 1);
            PlayerSe_EquipAtFrame(pl, 88.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EE:
            PlayerSe_AtFrame(pl, 4, 67);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
            PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
            PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F1:
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
        case 0x3F2:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 32, 2);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F3:
            PlayerSe_AtFrame(pl, 4, 67);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F4:
            PlayerSe_WeaponAtFrame(pl, 16, 1);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F5:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F6:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 8, 2);
            PlayerSe_VoiceAtFrame(pl, 6.0f, 0);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 26, 62);
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 2));
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            break;
        case 0x57C:
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 194.0f, 0);
            PlayerSe_EquipAtFrame(pl, 194.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 46, 5);
            PlayerSe_AtFrame(pl, 72, 74);
            PlayerSe_WeaponAtFrame(pl, 100, 6);
            PlayerSe_WeaponAtFrame(pl, 130, 4);
            return;
        case 0x57D:
        case 0x57E:
        case 0x57F:
            PlayerSe_WeaponAtFrame(pl, 20, 7);
            PlayerSe_WeaponAtFrame(pl, 40, 8);
            PlayerSe_WeaponAtFrame(pl, 70, 9);
            PlayerSe_WeaponAtFrame(pl, 100, 10);
            PlayerSe_WeaponAtFrame(pl, 142, 11);
            return;
        case 0x580:
        case 0x581:
        case 0x582:
            PlayerSe_WeaponAtFrame(pl, 2, 17);
            PlayerSe_EquipAtFrame(pl, 38.0f, EQUIP_ARMS);
            return;
        case 0x583:
            PlayerSe_VoiceAtFrame(pl, 30.0f, 2);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 2);
            PlayerSe_EquipAtFrame(pl, 44.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x585:
            PlayerSe_AtFrame(pl, 22, 62);
            PlayerSe_WeaponAtFrame(pl, 24, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 0);
            return;
        case 0x586:
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
        case 0x587:
        case 0x588:
        case 0x589:
            PlayerSe_WeaponAtFrame(pl, 2, 15);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 2);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x58A:
        case 0x58B:
        case 0x58C:
            PlayerSe_WeaponAtFrame(pl, 2, 7);
            PlayerSe_WeaponAtFrame(pl, 14, 8);
            return;
        }
        break;
    case WEAPON_LANCE:
        switch (work->motion) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 3);
            PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 56, 1);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_WeaponAtFrame(pl, 32, 3);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 3);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 3);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3ED:
        case 0x3EF:
            PlayerSe_WeaponAtFrame(pl, 7, 5);
            return;
        case 0x3F1:
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
        case 0x3F2:
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
        case 0x3F3:
            PlayerSe_AtFrame(pl, 12, 69);
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F6:
        case 0x3F7:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F8:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 6, 3);
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 12, 5);
            PlayerSe_WeaponAtFrame(pl, 52, 7);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(pl, 4, 7);
            PlayerSe_WeaponAtFrame(pl, 22, 7);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 2);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 2);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(pl, 66, 5);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 2);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 1);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(pl, 70, 5);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 0);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 18, 6);
            PlayerSe_WeaponAtFrame(pl, 92, 5);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 100.0f, 0);
            return;
        case 0x57E:
            PlayerSe_WeaponAtFrame(pl, 18, Sound_RandomSe(35, 3, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 18, 6);
            PlayerSe_WeaponAtFrame(pl, 92, 5);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 3);
            PlayerSe_EquipAtFrame(pl, 102.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 100.0f, 1);
            return;
        case 0x57F:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_AtFrame(pl, 80, 69);
            PlayerSe_AtFrame(pl, 80, 74);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 74.0f, 1);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 1);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x581:
            PlayerSe_WeaponAtFrame(pl, 52, 5);
            PlayerSe_AtFrame(pl, 6, 69);
            PlayerSe_AtFrame(pl, 6, 74);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(36, 2, 37, 3));
            PlayerSe_WeaponAtFrame(pl, 6, 6);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_AtFrame(pl, 10, 68);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x584:
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
        case 0x587:
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 26, 68);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            return;
        case 0x589:
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
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 3);
            PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 56, 1);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_WeaponAtFrame(pl, 32, 3);
            PlayerSe_WeaponAtFrame(pl, 58, 2);
            PlayerSe_EquipAtFrame(pl, 54.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            PlayerSe_EquipAtFrame(pl, 72.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 84.0f, 1);
            PlayerSe_EquipAtFrame(pl, 84.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 3);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3ED:
            PlayerSe_WeaponAtFrame(pl, 8, 5);
            return;
        case 0x3EF:
            PlayerSe_AtFrame(pl, 2, 74);
            return;
        case 0x3F1:
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
        case 0x3F2:
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
        case 0x3F3:
            PlayerSe_AtFrame(pl, 12, 69);
            PlayerSe_VoiceAtFrame(pl, 4.0f, 0);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 1);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F6:
        case 0x3F7:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F8:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_AtFrame(pl, 12, 67);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(pl, 2, 2);
            PlayerSe_WeaponAtFrame(pl, 6, 3);
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 8, 13);
            PlayerSe_WeaponAtFrame(pl, 72, 5);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 2);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 8, 13);
            PlayerSe_WeaponAtFrame(pl, 72, 5);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(pl, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 8, 13);
            PlayerSe_WeaponAtFrame(pl, 78, 5);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 2);
            PlayerSe_EquipAtFrame(pl, 74.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57C:
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
            PlayerSe_WeaponAtFrame(pl, 8, 2);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_WeaponAtFrame(pl, 52, 7);
            PlayerSe_WeaponAtFrame(pl, 88, 1);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 82.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 3));
            PlayerSe_WeaponAtFrame(pl, 18, 6);
            PlayerSe_WeaponAtFrame(pl, 92, 5);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 98.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 98.0f, 0);
            return;
        case 0x57E:
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
        case 0x57F:
            PlayerSe_WeaponAtFrame(pl, 4, 2);
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_EquipAtFrame(pl, 14.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 42, 28);
            return;
        case 0x580:
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
        case 0x581:
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
        case 0x582:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            return;
        case 0x587:
            PlayerSe_VoiceAtFrame(pl, 8.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 26, 68);
            PlayerSe_WeaponAtFrame(pl, 78, 10);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 10, 6);
            return;
        case 0x589:
            PlayerSe_WeaponAtFrame(pl, 88, 5);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 3);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 92.0f, 1);
            return;
        case 0x592:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        }
        break;
    case WEAPON_SWORD_SHIELD:
        switch (work->motion) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 28, 2);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3ED:
        case 0x3EF:
            PlayerSe_AtFrame(pl, 2, 74);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 1);
            return;
        case 0x3F1:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_EquipAtFrame(pl, 2.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 3);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            return;
        case 0x3F2:
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 20, 65);
            return;
        case 0x3FA:
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 2, 36, 1));
            PlayerSe_WeaponAtFrame(pl, 14, 4);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_WeaponAtFrame(pl, 4, 4);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 1);
            return;
        case 0x57B:
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
        case 0x57C:
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
        case 0x57D:
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(37, 4, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 4, 6);
            PlayerSe_WeaponAtFrame(pl, 14, 5);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 3);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 84.0f, 0);
            PlayerSe_EquipAtFrame(pl, 42.0f, EQUIP_ARMS);
            return;
        case 0x57E:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 3);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 68.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 10, 5);
            return;
        case 0x580:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x585:
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 3);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 32.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x587:
            PlayerSe_VoiceAtFrame(pl, 30.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 124.0f, 0);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            PlayerSe_AtFrame(pl, 46, 64);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(pl, 14, 4);
            PlayerSe_VoiceAtFrame(pl, 10.0f, 3);
            PlayerSe_EquipAtFrame(pl, 10.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x589:
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 3, 35, 3));
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            return;
        }
        break;
    case WEAPON_DUAL_SWORDS:
        switch (work->motion) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_WeaponAtFrame(pl, 8, 1);
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 18, 2);
            PlayerSe_WeaponAtFrame(pl, 26, 7);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 1);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 0);
            PlayerSe_EquipAtFrame(pl, 6.0f, EQUIP_RATTLE_ALL);
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F1:
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
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(pl, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 2.0f, 3);
            PlayerSe_AtFrame(pl, 2, 63);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_BODY);
            PlayerSe_AtFrame(pl, 20, 65);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_WeaponAtFrame(pl, 22, 6);
            PlayerSe_WeaponAtFrame(pl, 34, 12);
            PlayerSe_AtFrame(pl, 34, 54);
            PlayerSe_AtFrame(pl, 32, 63);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 72.0f, 0);
            return;
        case 0x3FB:
            PlayerSe_WeaponAtFrame(pl, 8, 5);
            PlayerSe_WeaponAtFrame(pl, 8, 9);
            PlayerSe_AtFrame(pl, 10, 63);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 0);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(pl, 16, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 2, 3);
            PlayerSe_WeaponAtFrame(pl, 14, 8);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 58.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(pl, 14, 8);
            PlayerSe_WeaponAtFrame(pl, 22, 4);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 0);
            PlayerSe_AtFrame(pl, 30, 68);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 24.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57B:
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
        case 0x57C:
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
        case 0x57E:
            PlayerSe_VoiceAtFrame(pl, 4.0f, 3);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 3);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 68.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 16, 36);
            PlayerSe_WeaponAtFrame(pl, 10, 5);
            return;
        case 0x580:
            PlayerSe_VoiceAtFrame(pl, 22.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 0);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57D:
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
        case 0x583:
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
        case 0x57F:
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
        case 0x581:
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
        case 0x582:
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
        case 0x584:
            PlayerSe_VoiceAtFrame(pl, 16.0f, 2);
            PlayerSe_AtFrame(pl, 18, 68);
            PlayerSe_EquipAtFrame(pl, 20.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x585:
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
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 20, 6);
            PlayerSe_WeaponAtFrame(pl, 20, 8);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 0);
            PlayerSe_WeaponAtFrame(pl, 38, 1);
            PlayerSe_EquipAtFrame(pl, 16.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 54.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 1);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 3);
            PlayerSe_VoiceAtFrame(pl, 44.0f, 0);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 1);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 2.0f, 2);
            PlayerSe_EquipAtFrame(pl, 4.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 2);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3ED:
            PlayerSe_WeaponAtFrame(pl, 8, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 3);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EE:
        case 0x3EF:
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
        case 0x3F4:
        case 0x3F5:
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
        case 0x3F9:
            PlayerSe_WeaponAtFrame(pl, 8, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 3);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(pl, 6, 8);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            PlayerSe_WeaponAtFrame(pl, 50, Sound_RandomSe(35, 4, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 66, 7);
            PlayerSe_WeaponAtFrame(pl, 132, 6);
            PlayerSe_VoiceAtFrame(pl, 24.0f, 2);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(pl, 12, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 5);
            PlayerSe_WeaponAtFrame(pl, 148, 6);
            PlayerSe_WeaponAtFrame(pl, 50, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(pl, 60, 7);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 3);
            PlayerSe_EquipAtFrame(pl, 50.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 18, 4);
            PlayerSe_WeaponAtFrame(pl, 26, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(pl, 38.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 64.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 76.0f, 0);
            PlayerSe_EquipAtFrame(pl, 78.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57C:
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
        case 0x57D:
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
        case 0x57E:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 28, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 114, 6);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 50.0f, 0);
            PlayerSe_EquipAtFrame(pl, 52.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57F:
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
        case 0x580:
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
        case 0x581:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 40, 4);
            PlayerSe_WeaponAtFrame(pl, 72, 7);
            PlayerSe_WeaponAtFrame(pl, 54, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(pl, 158, 6);
            PlayerSe_VoiceAtFrame(pl, 66.0f, 3);
            PlayerSe_EquipAtFrame(pl, 68.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x582:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 18, 4);
            return;
        case 0x585:
            PlayerSe_VoiceAtFrame(pl, 26.0f, 2);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            PlayerSe_VoiceAtFrame(pl, 42.0f, 2);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 58.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x586:
            PlayerSe_WeaponAtFrame(pl, 6, 8);
            PlayerSe_WeaponAtFrame(pl, 34, 4);
            return;
        case 0x587:
            PlayerSe_WeaponAtFrame(pl, 16, 8);
            PlayerSe_WeaponAtFrame(pl, 38, 1);
            PlayerSe_WeaponAtFrame(pl, 24, 4);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 3);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_WeaponAtFrame(pl, 20, Sound_RandomSe(35, 4, 36, 4));
            return;
        case 0x589:
        case 0x58A:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 16, 4);
            return;
        case 0x58B:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            return;
        case 0x58C:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            return;
        case 0x58D:
        case 0x58E:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            return;
        case 0x58F:
            PlayerSe_WeaponAtFrame(pl, 2, 6);
            PlayerSe_WeaponAtFrame(pl, 12, 2);
            return;
        case 0x590:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            return;
        case 0x591:
            PlayerSe_WeaponAtFrame(pl, 4, 8);
            PlayerSe_WeaponAtFrame(pl, 10, 4);
            return;
        case 0x592:
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
        case 0x3EA:
            PlayerSe_WeaponAtFrame(pl, 32, 12);
            PlayerSe_WeaponAtFrame(pl, 18, 1);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_VoiceAtFrame(pl, 36.0f, 3);
            PlayerSe_EquipAtFrame(pl, 36.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(pl, 10, 3);
            PlayerSe_WeaponAtFrame(pl, 32, 1);
            PlayerSe_VoiceAtFrame(pl, 56.0f, 0);
            PlayerSe_EquipAtFrame(pl, 56.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(pl, 62.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 64.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F4:
        case 0x3F5:
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
        case 0x3F8:
            PlayerSe_WeaponAtFrame(pl, 12, 1);
            PlayerSe_WeaponAtFrame(pl, 20, 2);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 30.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3F9:
            PlayerSe_WeaponAtFrame(pl, 8, 3);
            PlayerSe_WeaponAtFrame(pl, 42, 1);
            PlayerSe_VoiceAtFrame(pl, 32.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 60.0f, 0);
            PlayerSe_EquipAtFrame(pl, 34.0f, EQUIP_RATTLE_ALL);
            PlayerSe_EquipAtFrame(pl, 62.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(pl, 24, 1);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
            PlayerSe_EquipAtFrame(pl, 22.0f, EQUIP_ARMS);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_ARMS);
            PlayerSe_VoiceAtFrame(pl, 18.0f, 0);
            PlayerSe_EquipAtFrame(pl, 18.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x594:
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
        case 0x58A:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            return;
        case 0x595:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 0x590:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_VoiceAtFrame(pl, 16.0f, 0);
            return;
        case 0x596:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(pl, 84, 1);
            PlayerSe_VoiceAtFrame(pl, 14.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 28.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 100.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 230.0f, 0);
            PlayerSe_EquipAtFrame(pl, 28.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(pl, 4, 5);
            return;
        case 0x57E:
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
        case 0x57F:
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
        case 0x582:
            PlayerSe_WeaponAtFrame(pl, 34, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(pl, 2, 1);
            PlayerSe_WeaponAtFrame(pl, 26, 2);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 3);
            PlayerSe_EquipAtFrame(pl, 12.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(pl, 6, Sound_RandomSe(35, 3, 36, 1));
            PlayerSe_AtFrame(pl, 6, 61);
            PlayerSe_WeaponAtFrame(pl, 48, 1);
            PlayerSe_VoiceAtFrame(pl, 48.0f, 0);
            PlayerSe_EquipAtFrame(pl, 48.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x584:
            PlayerSe_WeaponAtFrame(pl, 4, 1);
            PlayerSe_WeaponAtFrame(pl, 20, 6);
            PlayerSe_VoiceAtFrame(pl, 20.0f, 1);
            return;
        case 0x586:
            PlayerSe_WeaponAtFrame(pl, 20, 1);
            PlayerSe_VoiceAtFrame(pl, 30.0f, 0);
            PlayerSe_EquipAtFrame(pl, 284.0f, EQUIP_ARMS);
            return;
        case 0x587:
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
        case 0x588:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 48, 5);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x589:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 26.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x58B:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            return;
        case 0x58C:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_EquipAtFrame(pl, 40.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x58D:
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            return;
        case 0x58E:
            PlayerSe_WeaponAtFrame(pl, 8, 4);
            PlayerSe_WeaponAtFrame(pl, 48, 5);
            PlayerSe_VoiceAtFrame(pl, 46.0f, 3);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x58F:
            PlayerSe_WeaponAtFrame(pl, 30, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            PlayerSe_EquipAtFrame(pl, 46.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x591:
            PlayerSe_WeaponAtFrame(pl, 28, 5);
            PlayerSe_VoiceAtFrame(pl, 12.0f, 0);
            return;
        case 0x592:
            PlayerSe_VoiceAtFrame(pl, 8.0f, 0);
            PlayerSe_VoiceAtFrame(pl, 40.0f, 3);
            PlayerSe_EquipAtFrame(pl, 8.0f, EQUIP_RATTLE_ALL);
            return;
        case 0x593:
            PlayerSe_WeaponAtFrame(pl, 16, 5);
            PlayerSe_VoiceAtFrame(pl, 26.0f, 0);
            return;
        case 0x597:
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
 * picks the effect's type (PartFx_SpawnForPlayer's 0, 3, 4, 4, 5, 6, 7, 8, 0x14; kind 2 always
 * at bone 5). The frame is the second parameter for the match: it goes in f12 in either place,
 * but this order gives the order of the callers' set-up (Player_MotionEffects' loop for motion
 * 0x587). */
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
            PartFx_SpawnForPlayer(obj, bone, 0x14);
            break;
        }
    }
}

/* Shows the first mesh of the player's two mesh pairs (Obj_ShowMeshInRange 0x12 and 0xE: meshes 2
 * and 4). No caller seen; work is unused. */
void Player_ResetMeshPairs(void *obj, PlayerWork *work)
{
    Obj_ShowMeshInRange(obj, 0x12, 0);
    Obj_ShowMeshInRange(obj, 0xE, 0);
}

s32 Player_IsLocal(void *obj);
s32 Player_OnSameSide(Player *a, Player *b);
s32 Player_HasSkill(Player *p, u8 skill);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void PartFx_SpawnForPlayerScaled(Player *p, s16 arg, s32 type, f32 scale);
void PartFx_SpawnAt(f32 *pos, s32 kind, f32 scale);
void HitSpark_SpawnAt(f32 *pos, s32 a, u16 b);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 Motion_ReachedFrame(void *, s32, f32);
s32 Obj_InCurrentArea(void *obj);
void Player_SpawnGroundEffect(Player *p, s32 kind, f32 scale);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
u16 Rand_Next(s32 stream);
void ModelObj_SpawnOn(void *arg0, s32 arg1);
struct StageObj *WeaponTrail_Create(void *owner, s32 a, s32 b);
void WeaponTrail_CreateTwoMotions(Em *em, s32 a1, s32 a2, s16 v); /* game em_bulldrome.c; called here with a Player */
void ItemProp_CreateAtPlayer(Player *p, s32 a);
void FireBlast_CreateOnEm(Em *em, s16 a);
void DustCloud_CreateOnPlayer(Player *p, s16 a, s16 b, f32 c);
void FallingDebris_Create(Em *em, u8 a);
void PlayerFx_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);

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
    case 0x2:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 20.0f, 8, 8);
            Player_EffectAtFrame(pl, 54.0f, 5, 8);
            Player_EffectAtFrame(pl, 88.0f, 8, 8);
        }
        break;
    case 0x3:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 12.0f, 8, 0);
            Player_EffectAtFrame(pl, 16.0f, 5, 0);
            Player_EffectAtFrame(pl, 32.0f, 8, 0);
            Player_EffectAtFrame(pl, 56.0f, 5, 0);
        }
        break;
    case 0x4:
        Player_EffectAtFrame(pl, 12.0f, 5, 1);
        Player_EffectAtFrame(pl, 30.0f, 8, 1);
        Player_EffectAtFrame(pl, 50.0f, 5, 1);
        break;
    case 0x5:
        if (Motion_PassedFrame(pl, 0, 2.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 3, 0.7f);
        }
        if (Motion_PassedFrame(pl, 0, 20.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 5, 3, 0.7f);
        }
        Player_EffectAtFrame(pl, 30.0f, 8, 3);
        break;
    case 0xD:
        Player_EffectAtFrame(pl, 2.0f, 0xA, 6);
        break;
    case 0xF:
        Player_EffectAtFrame(pl, 2.0f, 8, 1);
        break;
    case 0x13:
        Player_EffectAtFrame(pl, 2.0f, 8, 7);
        break;
    case 0x14:
        Player_EffectAtFrame(pl, 2.0f, 8, 7);
        break;
    case 0x1C:
        if (Motion_PassedFrame(pl, 0, 22.0f)) {
            PartFx_SpawnForPlayer(pl, 0xA, 0xB);
        }
        Player_EffectAtFrame(pl, 4.0f, 0xA, 0);
        break;
    case 0x1E:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 4.0f, 8, 2);
        }
        break;
    case 0x1F:
        Player_EffectAtFrame(pl, 16.0f, 2, 4);
        Player_EffectAtFrame(pl, 20.0f, 2, 4);
        break;
    case 0x20:
        if (((s32)pl->motionTime % 3) == 0) {
            PartFx_SpawnForPlayer(pl, 2, 5);
        }
        break;
    case 0x25:
        Player_EffectAtFrame(pl, 12.0f, 8, 8);
        Player_EffectAtFrame(pl, 32.0f, 5, 8);
        break;
    case 0x26:
        if (Motion_PassedFrame(pl, 0, 4.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 0, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 20.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 5, 0, 1.5f);
        }
        break;
    case 0x27:
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
    case 0x2A:
        if (gScene.inLobby == 0) {
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            Player_EffectAtFrame(pl, 46.0f, 8, 1);
        }
        break;
    case 0x2C:
    case 0x2D:
        if (work->motion == 0x2C) {
            Player_EffectAtFrame(pl, 30.0f, 5, 3);
        } else {
            Player_EffectAtFrame(pl, 30.0f, 8, 3);
        }
        if ((Motion_PassedFrame(pl, 0, 30.0f)) && !(Rand_Next(1) & 0xFFFF & 3)) {
            FallingDebris_Create((Em *) pl, 0);
        }
        break;
    case 0x2E:
        Player_EffectAtFrame(pl, 10.0f, 5, 1);
        break;
    case 0x30:
        if ((Motion_ReachedFrame(pl, 0, 16.0f) == 0) && (gScene.frame & 1)) {
            PartFx_SpawnForPlayer(pl, 0xA, 0xA);
        }
        break;
    case 0x32:
        Player_EffectAtFrame(pl, 30.0f, 8, 3);
        break;
    case 0x34:
        Player_EffectAtFrame(pl, 16.0f, 8, 8);
        Player_EffectAtFrame(pl, 46.0f, 5, 8);
        break;
    case 0x35:
        Player_EffectAtFrame(pl, 28.0f, 8, 1);
        Player_EffectAtFrame(pl, 8.0f, 5, 1);
        break;
    case 0x39:
        Player_EffectAtFrame(pl, 4.0f, 2, 6);
        break;
    case 0x3A:
        if (Motion_PassedFrame(pl, 0, 6.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.7f);
        }
        break;
    case 0x3B:
        Player_EffectAtFrame(pl, 16.0f, 8, 1);
        Player_EffectAtFrame(pl, 2.0f, 5, 1);
        break;
    case 0x3C:
        if ((Motion_ReachedFrame(pl, 0, 18.0f)) && !(gScene.frame & 0x1F)) {
            PartFx_SpawnForPlayerScaled(pl, 0xE, 0x1A, 0.4f);
        }
        break;
    case 0x3E:
        if (Motion_PassedFrame(pl, 0, 2.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0xA, 7, 0.6f);
        }
        break;
    case 0xCA:
        if (Motion_PassedFrame(pl, 0, 24.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.5f);
        }
        break;
    case 0xCC:
        if (Motion_PassedFrame(pl, 0, 16.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.8f);
        }
        break;
    case 0xCE:
        if (Motion_PassedFrame(pl, 0, 6.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0xA, 7, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 34.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0xA, 0x11, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 50.0f) || Motion_PassedFrame(pl, 0, 54.0f) || Motion_PassedFrame(pl, 0, 58.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 0x12, 1.0f);
        }
        break;
    case 0xD1:
        if (Motion_PassedFrame(pl, 0, 6.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0x14, 7, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 28.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0xA, 7, 1.5f);
        }
        if (Motion_PassedFrame(pl, 0, 42.0f) || Motion_PassedFrame(pl, 0, 46.0f) || Motion_PassedFrame(pl, 0, 50.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 8, 0x12, 1.0f);
        }
        break;
    case 0xD2:
        if (Motion_PassedFrame(pl, 0, 152.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 1.0f);
        }
        break;
    case 0xD7:
        Player_EffectAtFrame(pl, 2.0f, 0x13, 5);
        break;
    case 0xDE:
        if (Motion_PassedFrame(pl, 0, 40.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 1.0f);
        }
        if (Motion_PassedFrame(pl, 0, 46.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 0x14, 1.2f);
        }
        break;
    case 0xFA:
        if (Motion_PassedFrame(pl, 0, 250.0f)) {
            ModelObj_SpawnOn(pl, 6);
            ModelObj_SpawnOn(pl, 7);
        }
        break;
    case 0xFB:
        if ((Motion_PassedFrame(pl, 0, 536.0f)) && (Obj_InCurrentArea(pl) & 0xFF)) {
            Em_GetBonePos(pl, 0xA, point);
            PartFx_SpawnAt(point, 7, 0.8f);
        }
        if ((Motion_PassedFrame(pl, 0, 620.0f)) && (Obj_InCurrentArea(pl) & 0xFF)) {
            Em_GetBonePos(pl, 0xA, point);
            PartFx_SpawnAt(point, 7, 1.0f);
        }
        if ((Motion_PassedFrame(pl, 0, 698.0f)) && (Obj_InCurrentArea(pl) & 0xFF)) {
            Em_GetBonePos(pl, 2, point);
            PartFx_SpawnAt(point, 7, 0.6f);
        }
        break;
    case 0x19D:
        if (Motion_PassedFrame(pl, 0, 274.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0xE, 0x15, 1.0f);
        }
        break;
    case 0x19F:
        if ((Motion_PassedFrame(pl, 0, 50.0f)) && ((Obj_InCurrentArea(pl) & 0xFF) == 1)) {
            Vec_Set(point, 0.0f, 0.0f, 120.0f);
            Vec_Rotate(offset, point, (f32 *)pl->worldMtx);
            point[0] = pl->pos[0] + offset[0];
            point[1] = pl->pos[1] + offset[1];
            point[2] = pl->pos[2] + offset[2];
            HitSpark_SpawnAt(point, 7, pl->rot[1]);
        }
        break;
    case 0x1A0:
        if ((pl->useItem == ITEM_ID_FOOD_STAMINA_FULL) && ((Motion_PassedFrame(pl, 0, 16.0f)) || (Motion_PassedFrame(pl, 0, 22.0f)) || (Motion_PassedFrame(pl, 0, 28.0f)))) {
            ItemProp_CreateAtPlayer(pl, ITEMPROP_EAT_PUFF);
        }
        break;
    case 0x1A2:
        if (Motion_PassedFrame(pl, 0, 36.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0x12, 0x18, 0.3f);
        }
        break;
    case 0x1AA:
        if (Motion_PassedFrame(pl, 0, 190.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 0x17, 0.6f);
        }
        break;
    case 0x1AF:
        if (!(gScene.frame & 0x1F)) {
            PartFx_SpawnForPlayerScaled(pl, 0x14, 0x16, 1.0f);
        }
        break;
    case 0x1B0:
        Player_EffectAtFrame(pl, 78.0f, 1, 6);
        break;
    case 0x287:
        if (Motion_PassedFrame(pl, 0, 236.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0, 0x21, 0.7f);
        }
        if (Motion_PassedFrame(pl, 0, 314.0f)) {
            ModelObj_SpawnOn(pl, 6);
            ModelObj_SpawnOn(pl, 7);
        }
        break;
    case 0x320:
        if (Motion_PassedFrame(pl, 0, 24.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 0, 0x19, 0.8f);
        }
        break;
    case 0x321:
        if ((Motion_PassedFrame(pl, 0, 20.0f)) && (pl->cookResult == 4)) {
            Effect06_SpawnAtBone(pl, 0xA, 0, 0xE, 1.0f);
        }
        break;
    case 0x323:
        if ((work->motion == 0x323) && (((s32) gScene.frame % 20) == 0)) {
            FireBlast_CreateOnEm((Em *) pl, 6);
        }
        break;
    case 0x32A:
        if (Motion_PassedFrame(pl, 0, 52.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 0.5f);
        }
        break;
    case 0x37D:
        if (Motion_PassedFrame(pl, 0, 44.0f)) {
            PartFx_SpawnForPlayerScaled(pl, 2, 7, 1.5f);
        }
        break;
    }
    switch (pl->weaponType) {
    case WEAPON_HEAVY_BOWGUN:
        switch (work->motion) {
        case 0x3EC:
            Player_EffectAtFrame(pl, 2.0f, 5, 8);
            Player_EffectAtFrame(pl, 32.0f, 8, 8);
            return;
        case 0x3F1:
            Player_EffectAtFrame(pl, 22.0f, 8, 8);
            Player_EffectAtFrame(pl, 76.0f, 8, 8);
            Player_EffectAtFrame(pl, 50.0f, 5, 8);
            Player_EffectAtFrame(pl, 104.0f, 5, 8);
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 0x580:
            Player_EffectAtFrame(pl, 64.0f, 8, 1);
            Player_EffectAtFrame(pl, 50.0f, 5, 1);
            Player_EffectAtFrame(pl, 54.0f, 5, 1);
            return;
        case 0x581:
        case 0x582:
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
        case 0x3EC:
        case 0x3ED:
            Player_EffectAtFrame(pl, 4.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(pl, 60.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 8);
            Player_EffectAtFrame(pl, 76.0f, 5, 8);
            return;
        case 0x3F3:
            Player_EffectAtFrame(pl, 4.0f, 0xA, 0);
            if (Motion_PassedFrame(pl, 0, 20.0f)) {
                PartFx_SpawnForPlayer(pl, 0xA, 0xB);
            }
            break;
        case 0x3F4:
        case 0x3F5:
            if (work->motion == 0x3F4) {
                Player_EffectAtFrame(pl, 2.0f, 8, 1);
            } else {
                Player_EffectAtFrame(pl, 2.0f, 5, 1);
            }
            Player_EffectAtFrame(pl, 18.0f, 0xA, 6);
            return;
        case 0x57A:
            if (Motion_PassedFrame(pl, 0, 68.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xE, 0xC, 1.8f);
                return;
            }
            break;
        case 0x57B:
            if ((Motion_ReachedFrame(pl, 0, 1e+01f)) && (Motion_ReachedFrame(pl, 0, 66.0f) == 0) && !(gScene.frame & 3)) {
                PartFx_SpawnForPlayer(pl, 2, 0xD);
                return;
            }
            break;
        case 0x57C:
            if (Motion_PassedFrame(pl, 0, 38.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xE, 0xF, 1.5f);
                return;
            }
            break;
        case 0x57E:
            if (Motion_PassedFrame(pl, 0, 76.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xE, 0x13, 0.6f);
                return;
            }
            break;
        case 0x583:
            if (Motion_PassedFrame(pl, 0, 34.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xA, 0xC, 1.2f);
                PartFx_SpawnForPlayer(pl, 0xA, 0x1F);
                return;
            }
            break;
        case 0x584:
            if (Motion_PassedFrame(pl, 0, 52.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xE, 0xC, 0.7f);
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
        case 0x3EC:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(pl, 22.0f, 5, 1);
            return;
        case 0x3F2:
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 0x580:
            Player_EffectAtFrame(pl, 64.0f, 8, 1);
            Player_EffectAtFrame(pl, 50.0f, 5, 1);
            Player_EffectAtFrame(pl, 54.0f, 5, 1);
            return;
        case 0x581:
        case 0x582:
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
        case 0x3EC:
            Player_EffectAtFrame(pl, 18.0f, 5, 8);
            Player_EffectAtFrame(pl, 50.0f, 8, 8);
            return;
        case 0x3F3:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 20.0f, 5, 8);
            return;
        case 0x3F6:
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 22.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 8);
            return;
        case 0x3F7:
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 22.0f, 8, 1);
            Player_EffectAtFrame(pl, 28.0f, 5, 8);
            return;
        case 0x3F8:
            Player_EffectAtFrame(pl, 4.0f, 8, 1);
            Player_EffectAtFrame(pl, 24.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 3);
            return;
        case 0x579:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 14.0f, 5, 3);
            return;
        case 0x57A:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 10.0f, 5, 3);
            return;
        case 0x57B:
            Player_EffectAtFrame(pl, 10.0f, 8, 8);
            Player_EffectAtFrame(pl, 10.0f, 5, 3);
            return;
        case 0x57D:
            Player_EffectAtFrame(pl, 20.0f, 5, 8);
            Player_EffectAtFrame(pl, 24.0f, 8, 1);
            return;
        case 0x57E:
            Player_EffectAtFrame(pl, 22.0f, 8, 1);
            return;
        case 0x580:
            Player_EffectAtFrame(pl, 2.0f, 8, 3);
            Player_EffectAtFrame(pl, 14.0f, 5, 8);
            Player_EffectAtFrame(pl, 48.0f, 5, 3);
            Player_EffectAtFrame(pl, 54.0f, 5, 8);
            Player_EffectAtFrame(pl, 64.0f, 5, 8);
            Player_EffectAtFrame(pl, 74.0f, 5, 3);
            return;
        case 0x581:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 24.0f, 5, 3);
            Player_EffectAtFrame(pl, 42.0f, 8, 3);
            Player_EffectAtFrame(pl, 50.0f, 5, 8);
            return;
        case 0x589:
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
        case 0x3EC:
            Player_EffectAtFrame(pl, 18.0f, 8, 8);
            Player_EffectAtFrame(pl, 52.0f, 5, 8);
            return;
        case 0x3F1:
            Player_EffectAtFrame(pl, 18.0f, 5, 8);
            Player_EffectAtFrame(pl, 96.0f, 5, 8);
            Player_EffectAtFrame(pl, 60.0f, 8, 8);
            return;
        case 0x3F2:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 44.0f, 5, 1);
            Player_EffectAtFrame(pl, 46.0f, 5, 1);
            return;
        case 0x3F3:
            Player_EffectAtFrame(pl, 8.0f, 8, 8);
            Player_EffectAtFrame(pl, 36.0f, 5, 8);
            return;
        case 0x3F6:
        case 0x3F7:
            if (work->motion == 0x3F6) {
                Player_EffectAtFrame(pl, 4.0f, 8, 1);
                return;
            }
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            return;
        case 0x3F8:
            if (Motion_PassedFrame(pl, 0, 4.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 8, 3, 0.8f);
            }
            if (Motion_PassedFrame(pl, 0, 8.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 5, 3, 0.8f);
                return;
            }
            break;
        case 0x579:
            if (Motion_PassedFrame(pl, 0, 46.0f)) {
                PartFx_SpawnForPlayer(pl, 0xA, 0x10);
            }
            if (Motion_PassedFrame(pl, 0, 48.0f)) {
                DustCloud_CreateOnPlayer(pl, 2, 3, 0.6f);
            }
            if (Motion_PassedFrame(pl, 0, 52.0f)) {
                DustCloud_CreateOnPlayer(pl, 3, 3, 0.6f);
                return;
            }
            break;
        case 0x57A:
            if (Motion_PassedFrame(pl, 0, 4.0f)) {
                DustCloud_CreateOnPlayer(pl, 2, 2, 0.4f);
            }
            if (Motion_PassedFrame(pl, 0, 22.0f)) {
                DustCloud_CreateOnPlayer(pl, 3, 2, 0.4f);
                return;
            }
            break;
        case 0x57B:
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 14.0f, 5, 1);
            return;
        case 0x57C:
            Player_EffectAtFrame(pl, 8.0f, 8, 1);
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 0x57D:
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            Player_EffectAtFrame(pl, 20.0f, 5, 1);
            Player_EffectAtFrame(pl, 24.0f, 8, 1);
            return;
        case 0x57E:
            Player_EffectAtFrame(pl, 22.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 5, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            return;
        case 0x583:
            Player_EffectAtFrame(pl, 16.0f, 8, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            return;
        case 0x584:
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            Player_EffectAtFrame(pl, 14.0f, 8, 1);
            Player_EffectAtFrame(pl, 12.0f, 5, 1);
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            return;
        case 0x587:
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
        case 0x3EC:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(pl, 6.0f, 5, 1);
            return;
        case 0x3F4:
        case 0x3F5:
            if (work->motion == 0x3F4) {
                Player_EffectAtFrame(pl, 2.0f, 8, 1);
                Player_EffectAtFrame(pl, 16.0f, 0xA, 6);
                return;
            } else {
                Player_EffectAtFrame(pl, 2.0f, 5, 1);
                Player_EffectAtFrame(pl, 18.0f, 0xA, 6);
            }
            return;
        case 0x579:
            if (Motion_PassedFrame(pl, 0, 6.0f)) {
                WeaponTrail_Create(pl, 0xB, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x57A:
            if (Motion_PassedFrame(pl, 0, 2.0f)) {
                WeaponTrail_Create(pl, 9, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x57B:
            if (Motion_PassedFrame(pl, 0, 52.0f)) {
                WeaponTrail_Create(pl, 9, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x57C:
            if (Motion_PassedFrame(pl, 0, 24.0f)) {
                WeaponTrail_Create(pl, 0xE, WTRAIL_BLADE);
            }
            Player_EffectAtFrame(pl, 6.0f, 8, 1);
            Player_EffectAtFrame(pl, 26.0f, 5, 3);
            return;
        case 0x57D:
            if (Motion_PassedFrame(pl, 0, 20.0f)) {
                WeaponTrail_Create(pl, 0xD, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x57E:
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_Create(pl, 0xC, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x587:
            atFrame = 46.0f;
            i = 0;
            do {
                Player_EffectAtFrame(pl, atFrame, 5, 1);
                Player_EffectAtFrame(pl, atFrame, 8, 1);
                atFrame += 4.0f;
                i++;
            } while (i < 7);
            return;
        case 0x588:
            if (Motion_PassedFrame(pl, 0, 16.0f)) {
                WeaponTrail_Create(pl, 8, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x589:
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_CreateTwoMotions((Em *)pl, 0xC, WTRAIL_BLADE, 0x57E);
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
        case 0x3EC:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(pl, 6.0f, 5, 1);
            return;
        case 0x3F4:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 16.0f, 0xA, 6);
            return;
        case 0x3F5:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 18.0f, 0xA, 6);
            return;
        case 0x3FA:
            if (Motion_PassedFrame(pl, 0, 32.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 2, 0x23, 1.0f);
                return;
            }
            break;
        case 0x3FB:
            if (Motion_PassedFrame(pl, 0, 8.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 2, 0x27, 1.5f);
                PartFx_SpawnForPlayerScaled(pl, 0xA, 0x24, 1.0f);
                return;
            }
            break;
        case 0x579:
            Player_EffectAtFrame(pl, 10.0f, 8, 1);
            if (Motion_PassedFrame(pl, 0, 14.0f)) {
                WeaponTrail_Create(pl, 5, WTRAIL_BLADE_B);
                return;
            }
            break;
        case 0x57A:
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
        case 0x57B:
            Player_EffectAtFrame(pl, 8.0f, 5, 8);
            Player_EffectAtFrame(pl, 60.0f, 8, 3);
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_Create(pl, 7, WTRAIL_BLADE_B);
            }
            if (Motion_PassedFrame(pl, 0, 54.0f)) {
                WeaponTrail_Create(pl, 8, WTRAIL_BLADE);
            }
            if (Motion_PassedFrame(pl, 0, 64.0f)) {
                WeaponTrail_Create(pl, 0xC, WTRAIL_BLADE_B);
                return;
            }
            break;
        case 0x57C:
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
        case 0x57D:
            Player_EffectAtFrame(pl, 18.0f, 8, 8);
            Player_EffectAtFrame(pl, 28.0f, 5, 3);
            if (Motion_PassedFrame(pl, 0, 14.0f)) {
                WeaponTrail_Create(pl, 0xD, WTRAIL_BLADE);
            }
            if (Motion_PassedFrame(pl, 0, 24.0f)) {
                WeaponTrail_Create(pl, 0xA, WTRAIL_BLADE_B);
                return;
            }
            break;
        case 0x57E:
            Player_EffectAtFrame(pl, 18.0f, 8, 1);
            if (Motion_PassedFrame(pl, 0, 10.0f)) {
                WeaponTrail_Create(pl, 0xC, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x57F:
            if (Motion_PassedFrame(pl, 0, 124.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xA, 0x26, 1.0f);
                PartFx_SpawnForPlayer(pl, 0xA, 0x25);
                return;
            }
            break;
        case 0x581:
            Player_EffectAtFrame(pl, 34.0f, 5, 3);
            Player_EffectAtFrame(pl, 60.0f, 5, 1);
            Player_EffectAtFrame(pl, 72.0f, 8, 8);
            return;
        case 0x582:
            Player_EffectAtFrame(pl, 34.0f, 8, 3);
            Player_EffectAtFrame(pl, 60.0f, 8, 1);
            Player_EffectAtFrame(pl, 72.0f, 5, 8);
            return;
        case 0x583:
            Player_EffectAtFrame(pl, 18.0f, 5, 8);
            Player_EffectAtFrame(pl, 28.0f, 8, 3);
            if (Motion_PassedFrame(pl, 0, 14.0f)) {
                WeaponTrail_Create(pl, 0xD, WTRAIL_BLADE_B);
            }
            if (Motion_PassedFrame(pl, 0, 24.0f)) {
                WeaponTrail_Create(pl, 0xA, WTRAIL_BLADE);
                return;
            }
            break;
        case 0x584:
            Player_EffectAtFrame(pl, 18.0f, 5, 3);
            return;
        case 0x585:
            Player_EffectAtFrame(pl, 16.0f, 5, 3);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_LONG_SWORD:
        switch (work->motion) {
        case 0x3EC:
            Player_EffectAtFrame(pl, 4.0f, 5, 1);
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            return;
        case 0x3EE:
            Player_EffectAtFrame(pl, 24.0f, 0xA, 6);
            return;
        case 0x3EF:
            Player_EffectAtFrame(pl, 24.0f, 0xA, 6);
            return;
        case 0x579:
            if (Motion_PassedFrame(pl, 0, 62.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xE, 0xC, 1.2f);
            }
            Player_EffectAtFrame(pl, 28.0f, 5, 1);
            Player_EffectAtFrame(pl, 34.0f, 8, 8);
            Player_EffectAtFrame(pl, 58.0f, 8, 3);
            return;
        case 0x57A:
            if (Motion_PassedFrame(pl, 0, 58.0f)) {
                PartFx_SpawnForPlayerScaled(pl, 0xE, 0xC, 1.2f);
            }
            Player_EffectAtFrame(pl, 30.0f, 8, 8);
            Player_EffectAtFrame(pl, 54.0f, 8, 3);
            return;
        case 0x57B:
            Player_EffectAtFrame(pl, 42.0f, 8, 1);
            Player_EffectAtFrame(pl, 68.0f, 8, 3);
            Player_EffectAtFrame(pl, 80.0f, 5, 8);
            return;
        case 0x57C:
            Player_EffectAtFrame(pl, 50.0f, 8, 3);
            return;
        case 0x57D:
            Player_EffectAtFrame(pl, 22.0f, 5, 8);
            Player_EffectAtFrame(pl, 28.0f, 8, 3);
            return;
        case 0x57E:
            Player_EffectAtFrame(pl, 26.0f, 8, 8);
            Player_EffectAtFrame(pl, 34.0f, 8, 3);
            Player_EffectAtFrame(pl, 38.0f, 5, 1);
            return;
        case 0x57F:
            Player_EffectAtFrame(pl, 22.0f, 5, 8);
            Player_EffectAtFrame(pl, 26.0f, 8, 3);
            return;
        case 0x580:
            Player_EffectAtFrame(pl, 14.0f, 8, 3);
            Player_EffectAtFrame(pl, 28.0f, 5, 8);
            Player_EffectAtFrame(pl, 36.0f, 8, 3);
            Player_EffectAtFrame(pl, 68.0f, 5, 8);
            if (Motion_PassedFrame(pl, 0, 94.0f)) {
                PlayerFx_CreateOnPlayerDefault(pl, 0xA, 1.0f);
                return;
            }
            break;
        case 0x585:
            Player_EffectAtFrame(pl, 26.0f, 8, 1);
            Player_EffectAtFrame(pl, 44.0f, 5, 3);
            Player_EffectAtFrame(pl, 50.0f, 8, 8);
            return;
        case 0x586:
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
        case 0x3EA:
            Player_EffectAtFrame(pl, 60.0f, 8, 1);
            return;
        case 0x3EB:
            Player_EffectAtFrame(pl, 60.0f, 5, 8);
            return;
        case 0x3EC:
            Player_EffectAtFrame(pl, 2.0f, 8, 1);
            Player_EffectAtFrame(pl, 28.0f, 5, 1);
            return;
        case 0x3F3:
            Player_EffectAtFrame(pl, 30.0f, 0xA, 6);
            return;
        case 0x3F4:
            Player_EffectAtFrame(pl, 24.0f, 0xA, 6);
            return;
        case 0x3F5:
            Player_EffectAtFrame(pl, 24.0f, 0xA, 6);
            return;
        case 0x579:
            Player_EffectAtFrame(pl, 14.0f, 8, 8);
            return;
        case 0x57A:
            Player_EffectAtFrame(pl, 14.0f, 8, 8);
            return;
        case 0x57B:
            if (Motion_PassedFrame(pl, 0, 84.0f)) {
                Player_SpawnGroundEffect(pl, 0x13, 0.5f);
                return;
            }
            break;
        case 0x57C:
            Player_EffectAtFrame(pl, 14.0f, 8, 8);
            return;
        case 0x580:
            if (Motion_PassedFrame(pl, 0, 48.0f)) {
                Player_SpawnGroundEffect(pl, 0xC, 1.5f);
            }
            Player_EffectAtFrame(pl, 70.0f, 8, 1);
            Player_EffectAtFrame(pl, 92.0f, 8, 1);
            Player_EffectAtFrame(pl, 110.0f, 5, 8);
            return;
        case 0x581:
            if (Motion_PassedFrame(pl, 0, 50.0f)) {
                Player_SpawnGroundEffect(pl, 0xC, 1.2f);
            }
            Player_EffectAtFrame(pl, 50.0f, 5, 8);
            Player_EffectAtFrame(pl, 64.0f, 5, 8);
            Player_EffectAtFrame(pl, 80.0f, 8, 3);
            return;
        case 0x582:
            if (Motion_PassedFrame(pl, 0, 50.0f)) {
                Player_SpawnGroundEffect(pl, 0xC, 1.2f);
            }
            Player_EffectAtFrame(pl, 46.0f, 5, 8);
            Player_EffectAtFrame(pl, 68.0f, 8, 8);
            Player_EffectAtFrame(pl, 88.0f, 5, 1);
            return;
        case 0x583:
            if (Motion_PassedFrame(pl, 0, 48.0f)) {
                Player_SpawnGroundEffect(pl, 0xC, 1.5f);
            }
            Player_EffectAtFrame(pl, 46.0f, 5, 8);
            Player_EffectAtFrame(pl, 72.0f, 8, 1);
            Player_EffectAtFrame(pl, 88.0f, 5, 8);
            Player_EffectAtFrame(pl, 102.0f, 8, 8);
            /* fallthrough */
        case 0x585:
            Player_EffectAtFrame(pl, 52.0f, 8, 8);
            return;
        default:
            Player_ResetMeshPairs(pl, work);
            return;
        }
        break;
    case WEAPON_BOW:
        switch (work->motion) {
        case 0x3EC:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 1);
            return;
        case 0x3EE:
            Player_EffectAtFrame(pl, 16.0f, 5, 1);
            Player_EffectAtFrame(pl, 18.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 8);
            Player_EffectAtFrame(pl, 42.0f, 8, 3);
            return;
        case 0x3F1:
            Player_EffectAtFrame(pl, 2.0f, 5, 1);
            Player_EffectAtFrame(pl, 28.0f, 8, 1);
            return;
        case 0x3F2:
            Player_EffectAtFrame(pl, 16.0f, 5, 8);
            return;
        case 0x583:
            Player_EffectAtFrame(pl, 32.0f, 5, 1);
            Player_EffectAtFrame(pl, 48.0f, 8, 3);
            return;
        case 0x585:
            if (Motion_PassedFrame(pl, 0, 26.0f)) {
                WeaponTrail_Create(pl, 6, WTRAIL_BOW);
            }
            Player_EffectAtFrame(pl, 34.0f, 5, 8);
            return;
        case 0x586:
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
        case 0x3EC:
            Player_EffectAtFrame(pl, 62.0f, 8, 8);
            Player_EffectAtFrame(pl, 30.0f, 5, 8);
            return;
        case 0x3F4:
        case 0x3F5:
            if (work->motion == 0x3F4) {
                Player_EffectAtFrame(pl, 6.0f, 5, 0);
            } else {
                Player_EffectAtFrame(pl, 6.0f, 8, 0);
            }
            Player_EffectAtFrame(pl, 18.0f, 0xA, 6);
            return;
        case 0x3F8:
            Player_EffectAtFrame(pl, 4.0f, 5, 1);
            return;
        case 0x3F9:
            Player_EffectAtFrame(pl, 60.0f, 8, 8);
            Player_EffectAtFrame(pl, 32.0f, 5, 8);
            Player_EffectAtFrame(pl, 74.0f, 5, 8);
            return;
        case 0x583:
            Player_EffectAtFrame(pl, 10.0f, 5, 1);
            return;
        case 0x587:
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
