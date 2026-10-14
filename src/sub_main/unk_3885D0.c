#include "common.h"
#include "player.h"
#include "em.h"
#include "scene.h"

/*
 * 0x3885D0-0x399F70: the hunter's two very large per-motion functions
 * (Player_MotionSounds, 0xD71C bytes, and Player_MotionEffects, 0x4068 bytes),
 * with their work area inside the player at +0x438, and helpers.
 */

/* The work area at +0x438. */
typedef struct Work438 {
    u8 unk0;
    u8 state;                   /* 0x01 */
    u8 pad2[0x6 - 0x2];
    s16 unk6;                   /* 0x06: the motion last seen (Player_MotionEffects) */
    u8 unk8;                    /* 0x08: cleared when it changes */
    u8 pad9[0x12 - 0x9];
    u16 unk12;                  /* 0x12: a countdown between team item effects */
} Work438;

void Player_MotionSounds(Player *p, Work438 *w);
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
void Player_MotionEffects(Player *p, Work438 *w);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
/* unit's own declaration: with the definition's void (Player *p, s16 arg, s32 type) (sub_main/unk_3CD9C0.c), Player_EffectAtFrame: 73 of 96 words differ */
void Effect0D_SpawnForPlayer(Player *p, s32 a, u8 b);
void PlayerRec_SelectInRange(u8 *p, u8 kind, u8 sel);

/*
 * The hunter's motion sounds: per motion (w->unk6) the voice, armour rattle
 * and weapon sounds at their frames, first the motions of every weapon, then
 * those of the weapon type in hand. Called after Player_MotionEffects.
 */
void Player_MotionSounds(Player *p, Work438 *w)
{
    switch (w->unk6) {
    case 0x2:
        PlayerSe_VoiceAtFrame(p, 20.0f, 0);
        PlayerSe_VoiceAtFrame(p, 54.0f, 0);
        PlayerSe_VoiceAtFrame(p, 88.0f, 0);
        PlayerSe_EquipAtFrame(p, 22.0f, 0);
        PlayerSe_EquipAtFrame(p, 56.0f, 0);
        PlayerSe_EquipAtFrame(p, 90.0f, 0);
        break;
    case 0x3:
        PlayerSe_WeaponAtFrame(p, 56, Sound_RandomSe(39, 1, 40, 1));
        PlayerSe_VoiceAtFrame(p, 8.0f, 2);
        PlayerSe_VoiceAtFrame(p, 30.0f, 2);
        PlayerSe_VoiceAtFrame(p, 54.0f, 2);
        PlayerSe_EquipAtFrame(p, 32.0f, 0);
        PlayerSe_EquipAtFrame(p, 54.0f, 0);
        break;
    case 0x4:
        PlayerSe_VoiceAtFrame(p, 6.0f, 2);
        PlayerSe_VoiceAtFrame(p, 26.0f, 2);
        PlayerSe_EquipAtFrame(p, 10.0f, 0);
        PlayerSe_EquipAtFrame(p, 30.0f, 0);
        PlayerSe_VoiceAtFrame(p, 46.0f, 2);
        PlayerSe_VoiceAtFrame(p, 66.0f, 2);
        PlayerSe_EquipAtFrame(p, 50.0f, 0);
        break;
    case 0x5:
        PlayerSe_VoiceAtFrame(p, 4.0f, 2);
        PlayerSe_VoiceAtFrame(p, 18.0f, 1);
        PlayerSe_VoiceAtFrame(p, 42.0f, 0);
        PlayerSe_EquipAtFrame(p, 42.0f, 0);
        PlayerSe_WeaponAtFrame(p, 80, Sound_RandomSe(41, 3, 41, 2));
        PlayerSe_WeaponAtFrame(p, 164, Sound_RandomSe(41, 3, 41, 2));
        break;
    case 0x6:
        PlayerSe_VoiceAtFrame(p, 34.0f, 0);
        PlayerSe_EquipAtFrame(p, 34.0f, 0);
        break;
    case 0x7:
    case 0x9:
        PlayerSe_AtFrame(p, 2, 74);
        PlayerSe_EquipAtFrame(p, 4.0f, 0);
        PlayerSe_EquipAtFrame(p, 4.0f, 4);
        break;
    case 0xA:
        PlayerSe_EquipAtFrame(p, 14.0f, 4);
        PlayerSe_VoiceAtFrame(p, 30.0f, 1);
        PlayerSe_AtFrame(p, 38, 68);
        PlayerSe_EquipAtFrame(p, 84.0f, 4);
        PlayerSe_VoiceAtFrame(p, 100.0f, 1);
        PlayerSe_AtFrame(p, 108, 68);
        break;
    case 0xC:
        PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
        PlayerSe_VoiceAtFrame(p, 2.0f, 3);
        PlayerSe_EquipAtFrame(p, 4.0f, 0);
        break;
    case 0xD:
        PlayerSe_VoiceAtFrame(p, 2.0f, 3);
        PlayerSe_EquipAtFrame(p, 4.0f, 0);
        PlayerSe_AtFrame(p, 4, 58);
        break;
    case 0xE:
        PlayerSe_VoiceAtFrame(p, 4.0f, 3);
        PlayerSe_EquipAtFrame(p, 4.0f, 0);
        PlayerSe_AtFrame(p, 4, 74);
        break;
    case 0xF:
        PlayerSe_VoiceAtFrame(p, 4.0f, 2);
        PlayerSe_VoiceAtFrame(p, 10.0f, 3);
        PlayerSe_EquipAtFrame(p, 10.0f, 0);
        break;
    case 0x10:
        PlayerSe_VoiceAtFrame(p, 4.0f, 3);
        PlayerSe_EquipAtFrame(p, 8.0f, 0);
        break;
    case 0x11:
        PlayerSe_EquipAtFrame(p, 2.0f, 4);
        PlayerSe_AtFrame(p, 2, 75);
        break;
    case 0x12:
        PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(38, 2, 38, 2));
        PlayerSe_EquipAtFrame(p, 12.0f, 4);
        PlayerSe_EquipAtFrame(p, 56.0f, 4);
        PlayerSe_VoiceAtFrame(p, 88.0f, 3);
        PlayerSe_EquipAtFrame(p, 88.0f, 0);
        PlayerSe_VoiceAtFrame(p, 112.0f, 0);
        PlayerSe_AtFrame(p, 94, 68);
        break;
    case 0x13:
        PlayerSe_AtFrame(p, 2, 64);
        PlayerSe_EquipAtFrame(p, 2.0f, 0);
        PlayerSe_EquipAtFrame(p, 20.0f, 4);
        PlayerSe_VoiceAtFrame(p, 100.0f, 2);
        PlayerSe_EquipAtFrame(p, 100.0f, 0);
        break;
    case 0x14:
        PlayerSe_AtFrame(p, 2, 66);
        PlayerSe_EquipAtFrame(p, 20.0f, 3);
        PlayerSe_VoiceAtFrame(p, 68.0f, 1);
        PlayerSe_EquipAtFrame(p, 74.0f, 4);
        PlayerSe_VoiceAtFrame(p, 96.0f, 2);
        PlayerSe_EquipAtFrame(p, 96.0f, 0);
        break;
    case 0x15:
        PlayerSe_VoiceAtFrame(p, 34.0f, 0);
        PlayerSe_EquipAtFrame(p, 40.0f, 0);
        PlayerSe_VoiceAtFrame(p, 76.0f, 0);
        PlayerSe_EquipAtFrame(p, 76.0f, 0);
        break;
    case 0x16:
        PlayerSe_VoiceAtFrame(p, 6.0f, 3);
        PlayerSe_VoiceAtFrame(p, 14.0f, 3);
        PlayerSe_EquipAtFrame(p, 14.0f, 4);
        break;
    case 0x1C:
        PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 3, 36, 1));
        PlayerSe_VoiceAtFrame(p, 2.0f, 3);
        PlayerSe_AtFrame(p, 2, 63);
        PlayerSe_EquipAtFrame(p, 20.0f, 3);
        PlayerSe_AtFrame(p, 28, 65);
        PlayerSe_VoiceAtFrame(p, 46.0f, 0);
        PlayerSe_EquipAtFrame(p, 52.0f, 0);
        break;
    case 0x1D:
        PlayerSe_VoiceAtFrame(p, 20.0f, 0);
        PlayerSe_EquipAtFrame(p, 20.0f, 0);
        PlayerSe_VoiceAtFrame(p, 48.0f, 0);
        PlayerSe_EquipAtFrame(p, 48.0f, 0);
        PlayerSe_VoiceAtFrame(p, 82.0f, 0);
        PlayerSe_EquipAtFrame(p, 82.0f, 0);
        PlayerSe_VoiceAtFrame(p, 116.0f, 0);
        PlayerSe_EquipAtFrame(p, 116.0f, 0);
        PlayerSe_VoiceAtFrame(p, 148.0f, 0);
        PlayerSe_EquipAtFrame(p, 148.0f, 0);
        PlayerSe_VoiceAtFrame(p, 184.0f, 0);
        PlayerSe_EquipAtFrame(p, 184.0f, 0);
        break;
    case 0x1E:
        PlayerSe_VoiceAtFrame(p, 6.0f, 3);
        PlayerSe_EquipAtFrame(p, 15.0f, 0);
        break;
    case 0x22:
        if (Player_IsState(p, 0, 39) == 0) {
            PlayerSe_VoiceAtFrame(p, 4.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            Motion_VoiceAtFrame(p, 50.0f, 0);
            PlayerSe_EquipAtFrame(p, 56.0f, 4);
            Motion_VoiceAtFrame(p, 98.0f, 0);
            PlayerSe_EquipAtFrame(p, 104.0f, 4);
        }
        break;
    case 0x23:
        if (Player_IsState(p, 0, 39) == 0) {
            PlayerSe_EquipAtFrame(p, 38.0f, 0);
            Motion_VoiceAtFrame(p, 38.0f, 1);
            PlayerSe_EquipAtFrame(p, 82.0f, 0);
            Motion_VoiceAtFrame(p, 82.0f, 1);
        }
        break;
    case 0x24:
        PlayerSe_VoiceAtFrame(p, 2.0f, 0);
        PlayerSe_EquipAtFrame(p, 2.0f, 0);
        break;
    case 0x25:
        PlayerSe_VoiceAtFrame(p, 16.0f, 1);
        PlayerSe_VoiceAtFrame(p, 36.0f, 1);
        break;
    case 0x26:
        PlayerSe_WeaponAtFrame(p, 16, Sound_RandomSe(39, 3, 40, 2));
        PlayerSe_VoiceAtFrame(p, 16.0f, 2);
        PlayerSe_VoiceAtFrame(p, 32.0f, 2);
        PlayerSe_EquipAtFrame(p, 16.0f, 0);
        PlayerSe_EquipAtFrame(p, 32.0f, 0);
        break;
    case 0x27:
        PlayerSe_WeaponAtFrame(p, 42, 41);
        PlayerSe_VoiceAtFrame(p, 18.0f, 2);
        PlayerSe_VoiceAtFrame(p, 48.0f, 2);
        PlayerSe_VoiceAtFrame(p, 72.0f, 2);
        PlayerSe_VoiceAtFrame(p, 100.0f, 2);
        PlayerSe_EquipAtFrame(p, 46.0f, 0);
        PlayerSe_EquipAtFrame(p, 98.0f, 0);
        break;
    case 0x29:
        PlayerSe_VoiceAtFrame(p, 10.0f, 0);
        PlayerSe_EquipAtFrame(p, 6.0f, 4);
        break;
    case 0x2A:
        PlayerSe_VoiceAtFrame(p, 20.0f, 0);
        PlayerSe_EquipAtFrame(p, 24.0f, 0);
        break;
    case 0x2B:
        PlayerSe_VoiceAtFrame(p, 20.0f, 0);
        break;
    case 0x2C:
    case 0x2D:
        PlayerSe_VoiceAtFrame(p, 22.0f, 0);
        PlayerSe_AtFrame(p, 30, 69);
        break;
    case 0x2E:
        PlayerSe_VoiceAtFrame(p, 4.0f, 2);
        PlayerSe_EquipAtFrame(p, 4.0f, 0);
        break;
    case 0x30:
        PlayerSe_VoiceAtFrame(p, 16.0f, 3);
        PlayerSe_AtFrame(p, 4, 66);
        PlayerSe_EquipAtFrame(p, 4.0f, 3);
        PlayerSe_AtFrame(p, 60, 75);
        PlayerSe_VoiceAtFrame(p, 80.0f, 1);
        PlayerSe_VoiceAtFrame(p, 100.0f, 0);
        PlayerSe_EquipAtFrame(p, 100.0f, 0);
        break;
    case 0x31:
        PlayerSe_VoiceAtFrame(p, 40.0f, 1);
        PlayerSe_VoiceAtFrame(p, 102.0f, 1);
        break;
    case 0x32:
        PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 2, 36, 2));
        PlayerSe_VoiceAtFrame(p, 20.0f, 3);
        PlayerSe_AtFrame(p, 20, 62);
        PlayerSe_EquipAtFrame(p, 32.0f, 0);
        break;
    case 0x34:
        PlayerSe_VoiceAtFrame(p, 18.0f, 0);
        PlayerSe_EquipAtFrame(p, 18.0f, 0);
        PlayerSe_VoiceAtFrame(p, 48.0f, 0);
        PlayerSe_EquipAtFrame(p, 48.0f, 0);
        break;
    case 0x35:
        PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(p, 20.0f, 2);
        PlayerSe_EquipAtFrame(p, 20.0f, 0);
        PlayerSe_VoiceAtFrame(p, 42.0f, 2);
        PlayerSe_EquipAtFrame(p, 42.0f, 0);
        break;
    case 0x36:
        PlayerSe_VoiceAtFrame(p, 32.0f, 0);
        PlayerSe_EquipAtFrame(p, 32.0f, 0);
        PlayerSe_VoiceAtFrame(p, 76.0f, 1);
        PlayerSe_VoiceAtFrame(p, 162.0f, 0);
        PlayerSe_VoiceAtFrame(p, 192.0f, 1);
        PlayerSe_WeaponAtFrame(p, 36, Sound_RandomSe(38, 2, 38, 2));
        break;
    case 0x37:
        PlayerSe_EquipAtFrame(p, 8.0f, 4);
        PlayerSe_VoiceAtFrame(p, 16.0f, 0);
        PlayerSe_EquipAtFrame(p, 16.0f, 0);
        PlayerSe_VoiceAtFrame(p, 32.0f, 1);
        break;
    case 0x39:
        PlayerSe_AtFrame(p, 4, 64);
        PlayerSe_VoiceAtFrame(p, 88.0f, 0);
        PlayerSe_EquipAtFrame(p, 88.0f, 0);
        break;
    case 0x3A:
        PlayerSe_VoiceAtFrame(p, 6.0f, 3);
        PlayerSe_EquipAtFrame(p, 10.0f, 0);
        break;
    case 0x3B:
        PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(p, 10.0f, 2);
        PlayerSe_EquipAtFrame(p, 10.0f, Sound_RandomSe(0, 2, 0, 2));
        PlayerSe_VoiceAtFrame(p, 24.0f, 2);
        PlayerSe_EquipAtFrame(p, 24.0f, Sound_RandomSe(0, 2, 0, 2));
        break;
    case 0x3C:
        PlayerSe_EquipAtFrame(p, 20.0f, 4);
        break;
    case 0x3E:
        PlayerSe_EquipAtFrame(p, 4.0f, 0);
        PlayerSe_VoiceAtFrame(p, 4.0f, 3);
        PlayerSe_VoiceAtFrame(p, 10.0f, 2);
        break;
    case 0x3F:
        PlayerSe_EquipAtFrame(p, 4.0f, 0);
        PlayerSe_VoiceAtFrame(p, 70.0f, 1);
        PlayerSe_VoiceAtFrame(p, 100.0f, 1);
        break;
    case 0x40:
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        PlayerSe_EquipAtFrame(p, 26.0f, 0);
        PlayerSe_VoiceAtFrame(p, 60.0f, 0);
        PlayerSe_EquipAtFrame(p, 60.0f, 0);
        break;
    case 0xCA:
        PlayerSe_VoiceAtFrame(p, 12.0f, 0);
        PlayerSe_EquipAtFrame(p, 12.0f, 0);
        PlayerSe_AtFrame(p, 26, 64);
        PlayerSe_EquipAtFrame(p, 12.0f, 3);
        break;
    case 0xCC:
        PlayerSe_AtFrame(p, 14, 64);
        PlayerSe_EquipAtFrame(p, 14.0f, 0);
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        PlayerSe_EquipAtFrame(p, 26.0f, 4);
        break;
    case 0xCE:
        PlayerSe_AtFrame(p, 4, 60);
        PlayerSe_EquipAtFrame(p, 4.0f, 3);
        PlayerSe_AtFrame(p, 32, 66);
        PlayerSe_AtFrame(p, 116, 58);
        PlayerSe_EquipAtFrame(p, 54.0f, 4);
        PlayerSe_EquipAtFrame(p, 84.0f, 0);
        PlayerSe_EquipAtFrame(p, 116.0f, 4);
        break;
    case 0xCF:
        PlayerSe_EquipAtFrame(p, 10.0f, 4);
        PlayerSe_VoiceAtFrame(p, 30.0f, 1);
        PlayerSe_EquipAtFrame(p, 30.0f, 0);
        PlayerSe_VoiceAtFrame(p, 74.0f, 2);
        PlayerSe_EquipAtFrame(p, 74.0f, 0);
        break;
    case 0xD1:
        PlayerSe_AtFrame(p, 4, 60);
        PlayerSe_EquipAtFrame(p, 4.0f, 3);
        PlayerSe_AtFrame(p, 32, 66);
        PlayerSe_AtFrame(p, 116, 58);
        PlayerSe_EquipAtFrame(p, 54.0f, 4);
        PlayerSe_EquipAtFrame(p, 84.0f, 0);
        PlayerSe_EquipAtFrame(p, 116.0f, 4);
        break;
    case 0xD2:
        PlayerSe_AtFrame(p, 66, 58);
        PlayerSe_EquipAtFrame(p, 66.0f, 0);
        PlayerSe_AtFrame(p, 146, 75);
        PlayerSe_EquipAtFrame(p, 146.0f, 4);
        PlayerSe_AtFrame(p, 152, 60);
        break;
    case 0xD3:
        PlayerSe_AtFrame(p, 12, 74);
        PlayerSe_AtFrame(p, 20, 75);
        PlayerSe_EquipAtFrame(p, 16.0f, 4);
        PlayerSe_EquipAtFrame(p, 28.0f, 0);
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        break;
    case 0xD4:
        PlayerSe_EquipAtFrame(p, 32.0f, 4);
        PlayerSe_EquipAtFrame(p, 92.0f, 0);
        PlayerSe_VoiceAtFrame(p, 94.0f, 1);
        PlayerSe_VoiceAtFrame(p, 114.0f, 1);
        break;
    case 0xD6:
        PlayerSe_EquipAtFrame(p, 66.0f, 0);
        PlayerSe_EquipAtFrame(p, 16.0f, 0);
        break;
    case 0xD7:
        PlayerSe_AtFrame(p, 4, 60);
        PlayerSe_EquipAtFrame(p, 4.0f, 3);
        PlayerSe_AtFrame(p, 106, 64);
        PlayerSe_EquipAtFrame(p, 110.0f, 4);
        break;
    case 0xD8:
        PlayerSe_AtFrame(p, 14, 68);
        PlayerSe_EquipAtFrame(p, 4.0f, 4);
        PlayerSe_VoiceAtFrame(p, 44.0f, 1);
        PlayerSe_VoiceAtFrame(p, 64.0f, 3);
        PlayerSe_EquipAtFrame(p, 74.0f, 0);
        break;
    case 0xDA:
        PlayerSe_AtFrame(p, 10, 74);
        PlayerSe_EquipAtFrame(p, 10.0f, 4);
        break;
    case 0xDC:
        PlayerSe_EquipAtFrame(p, 18.0f, 0);
        break;
    case 0xDD:
        PlayerSe_EquipAtFrame(p, 56.0f, 4);
        PlayerSe_EquipAtFrame(p, 22.0f, 0);
        break;
    case 0xDE:
        PlayerSe_AtFrame(p, 38, 66);
        PlayerSe_EquipAtFrame(p, 46.0f, 0);
        PlayerSe_EquipAtFrame(p, 52.0f, 4);
        break;
    case 0xE0:
        PlayerSe_VoiceAtFrame(p, 20.0f, 0);
        PlayerSe_EquipAtFrame(p, 20.0f, 0);
        PlayerSe_WeaponAtFrame(p, 64, 41);
        PlayerSe_AtFrame(p, 64, 59);
        break;
    case 0xE1:
        PlayerSe_EquipAtFrame(p, 54.0f, 4);
        PlayerSe_EquipAtFrame(p, 120.0f, 0);
        PlayerSe_WeaponAtFrame(p, 64, 41);
        PlayerSe_AtFrame(p, 118, 59);
        PlayerSe_AtFrame(p, 170, 65);
        break;
    case 0xE2:
        PlayerSe_EquipAtFrame(p, 74.0f, 0);
        PlayerSe_VoiceAtFrame(p, 72.0f, 0);
        break;
    case 0xE3:
        PlayerSe_VoiceAtFrame(p, 60.0f, 2);
        PlayerSe_EquipAtFrame(p, 62.0f, 0);
        PlayerSe_AtFrame(p, 22, 67);
        PlayerSe_AtFrame(p, 78, 65);
        PlayerSe_EquipAtFrame(p, 94.0f, 3);
        PlayerSe_EquipAtFrame(p, 120.0f, 0);
        PlayerSe_AtFrame(p, 128, 68);
        break;
    case 0xFA:
        PlayerSe_OnlineAtFrame(p, 168, 3);
        PlayerSe_VoiceAtFrame(p, 32.0f, 0);
        PlayerSe_EquipAtFrame(p, 32.0f, 0);
        PlayerSe_VoiceAtFrame(p, 46.0f, 0);
        PlayerSe_AtFrame(p, 46, 75);
        PlayerSe_EquipAtFrame(p, 84.0f, 4);
        PlayerSe_AtFrame(p, 88, 88);
        PlayerSe_VoiceAtFrame(p, 168.0f, 2);
        PlayerSe_EquipAtFrame(p, 168.0f, 0);
        PlayerSe_AtFrame(p, 168, 89);
        PlayerSe_VoiceAtFrame(p, 198.0f, 2);
        PlayerSe_EquipAtFrame(p, 198.0f, 0);
        PlayerSe_AtFrame(p, 220, 74);
        PlayerSe_EquipAtFrame(p, 220.0f, 4);
        PlayerSe_AtFrame(p, 366, 61);
        PlayerSe_AtFrame(p, 410, 90);
        PlayerSe_VoiceAtFrame(p, 454.0f, 0);
        break;
    case 0xFB:
        PlayerSe_OnlineAtFrame(p, 162, 2);
        PlayerSe_OnlineAtFrame(p, 270, 2);
        PlayerSe_OnlineAtFrame(p, 410, 3);
        PlayerSe_OnlineAtFrame(p, 540, 6);
        PlayerSe_OnlineAtFrame(p, 1320, 9);
        PlayerSe_VoiceAtFrame(p, 48.0f, 0);
        PlayerSe_EquipAtFrame(p, 48.0f, 0);
        PlayerSe_VoiceAtFrame(p, 66.0f, 1);
        PlayerSe_EquipAtFrame(p, 48.0f, 0);
        PlayerSe_VoiceAtFrame(p, 356.0f, 0);
        PlayerSe_EquipAtFrame(p, 368.0f, 0);
        PlayerSe_VoiceAtFrame(p, 434.0f, 2);
        PlayerSe_EquipAtFrame(p, 434.0f, 0);
        PlayerSe_AtFrame(p, 54, 75);
        PlayerSe_AtFrame(p, 112, 45);
        PlayerSe_EquipAtFrame(p, 110.0f, 4);
        PlayerSe_EquipAtFrame(p, 170.0f, 4);
        PlayerSe_AtFrame(p, 172, 88);
        PlayerSe_EquipAtFrame(p, 222.0f, 4);
        PlayerSe_AtFrame(p, 224, 88);
        PlayerSe_AtFrame(p, 212, 45);
        PlayerSe_EquipAtFrame(p, 268.0f, 4);
        PlayerSe_AtFrame(p, 268, 88);
        PlayerSe_AtFrame(p, 260, 45);
        PlayerSe_AtFrame(p, 398, 45);
        PlayerSe_AtFrame(p, 400, 88);
        PlayerSe_AtFrame(p, 416, 63);
        PlayerSe_AtFrame(p, 500, 63);
        PlayerSe_EquipAtFrame(p, 426.0f, 4);
        PlayerSe_EquipAtFrame(p, 456.0f, 4);
        PlayerSe_EquipAtFrame(p, 486.0f, 4);
        PlayerSe_AtFrame(p, 260, 69);
        PlayerSe_AtFrame(p, 538, 66);
        PlayerSe_AtFrame(p, 574, 58);
        PlayerSe_EquipAtFrame(p, 578.0f, 0);
        PlayerSe_AtFrame(p, 630, 64);
        PlayerSe_AtFrame(p, 698, 58);
        PlayerSe_EquipAtFrame(p, 672.0f, 0);
        PlayerSe_AtFrame(p, 848, 67);
        PlayerSe_AtFrame(p, 924, 92);
        PlayerSe_EquipAtFrame(p, 924.0f, 0);
        PlayerSe_AtFrame(p, 946, 75);
        PlayerSe_AtFrame(p, 962, 67);
        PlayerSe_AtFrame(p, 988, 92);
        PlayerSe_EquipAtFrame(p, 924.0f, 0);
        PlayerSe_AtFrame(p, 1048, 92);
        PlayerSe_EquipAtFrame(p, 924.0f, 0);
        PlayerSe_AtFrame(p, 1086, 91);
        PlayerSe_EquipAtFrame(p, 1086.0f, 4);
        PlayerSe_AtFrame(p, 1098, 75);
        PlayerSe_EquipAtFrame(p, 1172.0f, 4);
        PlayerSe_AtFrame(p, 1122, 75);
        PlayerSe_AtFrame(p, 1172, 75);
        PlayerSe_EquipAtFrame(p, 1172.0f, 4);
        PlayerSe_AtFrame(p, 1196, 75);
        PlayerSe_EquipAtFrame(p, 1196.0f, 4);
        PlayerSe_AtFrame(p, 1256, 91);
        PlayerSe_EquipAtFrame(p, 1256.0f, 0);
        PlayerSe_AtFrame(p, 1298, 91);
        PlayerSe_EquipAtFrame(p, 1298.0f, 0);
        break;
    case 0x101:
        PlayerSe_EquipAtFrame(p, 28.0f, 0);
        PlayerSe_VoiceAtFrame(p, 28.0f, 0);
        PlayerSe_AtFrame(p, 54, 69);
        PlayerSe_AtFrame(p, 74, 65);
        PlayerSe_AtFrame(p, 118, 59);
        PlayerSe_VoiceAtFrame(p, 308.0f, 1);
        PlayerSe_EquipAtFrame(p, 310.0f, 4);
        PlayerSe_AtFrame(p, 304, 67);
        PlayerSe_EquipAtFrame(p, 308.0f, 0);
        PlayerSe_VoiceAtFrame(p, 362.0f, 1);
        PlayerSe_VoiceAtFrame(p, 478.0f, 1);
        PlayerSe_AtFrame(p, 78, 65);
        PlayerSe_AtFrame(p, 580, 84);
        break;
    case 0x102:
        PlayerSe_AtFrame(p, 4, 67);
        PlayerSe_EquipAtFrame(p, 16.0f, 0);
        PlayerSe_EquipAtFrame(p, 36.0f, 0);
        PlayerSe_VoiceAtFrame(p, 36.0f, 0);
        break;
    case 0x191:
        PlayerSe_EquipAtFrame(p, 20.0f, 4);
        PlayerSe_VoiceAtFrame(p, 16.0f, 3);
        PlayerSe_AtFrame(p, 20, 62);
        PlayerSe_EquipAtFrame(p, 40.0f, 0);
        break;
    case 0x192:
        PlayerSe_EquipAtFrame(p, 48.0f, 4);
        PlayerSe_EquipAtFrame(p, 84.0f, 4);
        PlayerSe_VoiceAtFrame(p, 46.0f, 1);
        PlayerSe_AtFrame(p, 122, 75);
        PlayerSe_AtFrame(p, 142, 75);
        PlayerSe_EquipAtFrame(p, 142.0f, 4);
        break;
    case 0x193:
        PlayerSe_VoiceAtFrame(p, 22.0f, 1);
        PlayerSe_EquipAtFrame(p, 22.0f, 0);
        PlayerSe_AtFrame(p, 28, Sound_RandomSe(48, 3, 48, 2));
        PlayerSe_AtFrame(p, 226, Sound_RandomSe(48, 2, 48, 2));
        break;
    case 0x195:
        PlayerSe_VoiceAtFrame(p, 16.0f, 0);
        PlayerSe_EquipAtFrame(p, 40.0f, 4);
        PlayerSe_AtFrame(p, 70, 76);
        PlayerSe_EquipAtFrame(p, 72.0f, 4);
        PlayerSe_AtFrame(p, 116, 76);
        PlayerSe_EquipAtFrame(p, 120.0f, 4);
        PlayerSe_AtFrame(p, 120, 76);
        PlayerSe_AtFrame(p, 146, 76);
        PlayerSe_EquipAtFrame(p, 192.0f, 4);
        PlayerSe_VoiceAtFrame(p, 230.0f, 0);
        PlayerSe_EquipAtFrame(p, 230.0f, 0);
        break;
    case 0x196:
        PlayerSe_AtFrame(p, 36, 73);
        break;
    case 0x197:
        PlayerSe_EquipAtFrame(p, 52.0f, 4);
        PlayerSe_AtFrame(p, 38, 74);
        PlayerSe_EquipAtFrame(p, 50.0f, 0);
        break;
    case 0x198:
        PlayerSe_WeaponAtFrame(p, 18, Sound_RandomSe(42, 5, 43, 2));
        PlayerSe_WeaponAtFrame(p, 38, Sound_RandomSe(42, 2, 43, 4));
        PlayerSe_WeaponAtFrame(p, 66, Sound_RandomSe(42, 4, 43, 2));
        PlayerSe_WeaponAtFrame(p, 84, Sound_RandomSe(42, 2, 43, 5));
        break;
    case 0x199:
        PlayerSe_EquipAtFrame(p, 10.0f, 4);
        PlayerSe_AtFrame(p, 20, 75);
        break;
    case 0x19A:
        PlayerSe_AtFrame(p, 42, 75);
        PlayerSe_AtFrame(p, 78, 75);
        break;
    case 0x19D:
        PlayerSe_VoiceAtFrame(p, 50.0f, 0);
        PlayerSe_EquipAtFrame(p, 52.0f, 0);
        PlayerSe_WeaponAtFrame(p, 30, 10);
        PlayerSe_WeaponAtFrame(p, 324, 11);
        PlayerSe_AtFrame(p, 100, 72);
        PlayerSe_EquipAtFrame(p, 100.0f, 4);
        PlayerSe_AtFrame(p, 146, 72);
        PlayerSe_AtFrame(p, 192, 72);
        PlayerSe_EquipAtFrame(p, 192.0f, 4);
        PlayerSe_AtFrame(p, 240, 72);
        PlayerSe_VoiceAtFrame(p, 346.0f, 0);
        PlayerSe_AtFrame(p, 274, 54);
        if (p->weaponType == 9) {
            PlayerSe_WeaponAtFrame(p, 20, 1);
        }
        break;
    case 0x19E:
        PlayerSe_VoiceAtFrame(p, 22.0f, 0);
        PlayerSe_EquipAtFrame(p, 22.0f, 0);
        PlayerSe_AtFrame(p, 62, 74);
        PlayerSe_EquipAtFrame(p, 46.0f, 4);
        break;
    case 0x19F:
        PlayerSe_AtFrame(p, 8, 61);
        PlayerSe_AtFrame(p, 26, 63);
        PlayerSe_AtFrame(p, 42, 47);
        PlayerSe_VoiceAtFrame(p, 8.0f, 0);
        PlayerSe_EquipAtFrame(p, 22.0f, 0);
        PlayerSe_VoiceAtFrame(p, 38.0f, 3);
        PlayerSe_EquipAtFrame(p, 38.0f, 0);
        PlayerSe_VoiceAtFrame(p, 78.0f, 0);
        PlayerSe_EquipAtFrame(p, 78.0f, 0);
        PlayerSe_VoiceAtFrame(p, 92.0f, 0);
        PlayerSe_EquipAtFrame(p, 92.0f, 0);
        PlayerSe_VoiceAtFrame(p, 136.0f, 0);
        PlayerSe_EquipAtFrame(p, 138.0f, 0);
        PlayerSe_VoiceAtFrame(p, 162.0f, 1);
        break;
    case 0x1A0:
        PlayerSe_AtFrame(p, 2, 74);
        PlayerSe_VoiceAtFrame(p, 12.0f, 0);
        PlayerSe_AtFrame(p, 54, 55);
        PlayerSe_AtFrame(p, 68, 55);
        PlayerSe_AtFrame(p, 78, 55);
        PlayerSe_AtFrame(p, 94, 55);
        PlayerSe_AtFrame(p, 110, 55);
        PlayerSe_AtFrame(p, 132, 55);
        PlayerSe_AtFrame(p, 146, 55);
        PlayerSe_AtFrame(p, 152, 55);
        PlayerSe_AtFrame(p, 172, 73);
        break;
    case 0x1A1:
        PlayerSe_AtFrame(p, 12, 62);
        PlayerSe_EquipAtFrame(p, 16.0f, 4);
        PlayerSe_VoiceAtFrame(p, 12.0f, 0);
        PlayerSe_EquipAtFrame(p, 70.0f, 0);
        PlayerSe_VoiceAtFrame(p, 70.0f, 0);
        break;
    case 0x1A2:
        PlayerSe_AtFrame(p, 32, 42);
        PlayerSe_EquipAtFrame(p, 14.0f, 4);
        PlayerSe_AtFrame(p, 104, 76);
        PlayerSe_VoiceAtFrame(p, 28.0f, 0);
        PlayerSe_EquipAtFrame(p, 104.0f, 4);
        PlayerSe_EquipAtFrame(p, 194.0f, 0);
        PlayerSe_VoiceAtFrame(p, 194.0f, 0);
        break;
    case 0x1A3:
        PlayerSe_AtFrame(p, 30, 50);
        PlayerSe_AtFrame(p, 64, 51);
        PlayerSe_AtFrame(p, 98, 53);
        PlayerSe_AtFrame(p, 172, 53);
        PlayerSe_AtFrame(p, 228, 51);
        PlayerSe_AtFrame(p, 228, 50);
        PlayerSe_AtFrame(p, 286, 75);
        PlayerSe_VoiceAtFrame(p, 24.0f, 0);
        PlayerSe_VoiceAtFrame(p, 58.0f, 0);
        PlayerSe_EquipAtFrame(p, 58.0f, 0);
        PlayerSe_VoiceAtFrame(p, 316.0f, 0);
        PlayerSe_EquipAtFrame(p, 318.0f, 0);
        PlayerSe_VoiceAtFrame(p, 336.0f, 0);
        break;
    case 0x1A4:
        PlayerSe_AtFrame(p, 6, 74);
        Motion_SeAtFrame(p, 70, 119);
        PlayerSe_EquipAtFrame(p, 118.0f, 4);
        PlayerSe_VoiceAtFrame(p, 24.0f, 0);
        PlayerSe_EquipAtFrame(p, 24.0f, 0);
        break;
    case 0x1A5:
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        PlayerSe_EquipAtFrame(p, 26.0f, 0);
        break;
    case 0x1A6:
        PlayerSe_VoiceAtFrame(p, 14.0f, 0);
        PlayerSe_EquipAtFrame(p, 18.0f, 0);
        PlayerSe_VoiceAtFrame(p, 48.0f, 0);
        PlayerSe_EquipAtFrame(p, 48.0f, 0);
        PlayerSe_AtFrame(p, 28, 50);
        PlayerSe_AtFrame(p, 58, 51);
        break;
    case 0x1A7:
        PlayerSe_VoiceAtFrame(p, 18.0f, 0);
        PlayerSe_EquipAtFrame(p, 18.0f, 0);
        PlayerSe_VoiceAtFrame(p, 30.0f, 0);
        PlayerSe_EquipAtFrame(p, 30.0f, 0);
        break;
    case 0x1A8:
        PlayerSe_VoiceAtFrame(p, 24.0f, 1);
        PlayerSe_EquipAtFrame(p, 24.0f, 0);
        PlayerSe_EquipAtFrame(p, 30.0f, 4);
        Motion_SeAtFrame(p, 72, 120);
        break;
    case 0x1A9:
        PlayerSe_AtFrame(p, 58, 74);
        PlayerSe_VoiceAtFrame(p, 48.0f, 1);
        PlayerSe_EquipAtFrame(p, 48.0f, 0);
        PlayerSe_VoiceAtFrame(p, 78.0f, 0);
        PlayerSe_EquipAtFrame(p, 78.0f, 0);
        break;
    case 0x1AA:
        PlayerSe_AtFrame(p, 168, 61);
        PlayerSe_AtFrame(p, 204, 59);
        PlayerSe_AtFrame(p, 234, 58);
        PlayerSe_EquipAtFrame(p, 210.0f, 3);
        PlayerSe_EquipAtFrame(p, 234.0f, 0);
        PlayerSe_EquipAtFrame(p, 324.0f, 4);
        PlayerSe_VoiceAtFrame(p, 46.0f, 0);
        PlayerSe_VoiceAtFrame(p, 74.0f, 0);
        PlayerSe_VoiceAtFrame(p, 166.0f, 3);
        PlayerSe_VoiceAtFrame(p, 172.0f, 3);
        break;
    case 0x1AC:
        PlayerSe_EquipAtFrame(p, 30.0f, 4);
        break;
    case 0x1AD:
        PlayerSe_AtFrame(p, 30, 67);
        PlayerSe_EquipAtFrame(p, 56.0f, 4);
        PlayerSe_EquipAtFrame(p, 122.0f, 4);
        PlayerSe_AtFrame(p, 192, 75);
        PlayerSe_AtFrame(p, 240, 69);
        PlayerSe_EquipAtFrame(p, 244.0f, 0);
        PlayerSe_AtFrame(p, 304, 69);
        PlayerSe_EquipAtFrame(p, 306.0f, 0);
        PlayerSe_EquipAtFrame(p, 352.0f, 0);
        PlayerSe_VoiceAtFrame(p, 350.0f, 0);
        PlayerSe_VoiceAtFrame(p, 414.0f, 0);
        PlayerSe_EquipAtFrame(p, 416.0f, 0);
        break;
    case 0x1AF:
        PlayerSe_VoiceAtFrame(p, 26.0f, 1);
        PlayerSe_EquipAtFrame(p, 26.0f, 0);
        PlayerSe_WeaponAtFrame(p, 132, Sound_RandomSe(39, 3, 40, 1));
        PlayerSe_WeaponAtFrame(p, 242, Sound_RandomSe(39, 1, 40, 3));
        break;
    case 0x1B0:
        PlayerSe_VoiceAtFrame(p, 8.0f, 0);
        PlayerSe_EquipAtFrame(p, 8.0f, 0);
        PlayerSe_WeaponAtFrame(p, 30, 38);
        PlayerSe_VoiceAtFrame(p, 106.0f, 0);
        PlayerSe_EquipAtFrame(p, 46.0f, 4);
        break;
    case 0x258:
        PlayerSe_AtFrame(p, 4, 67);
        PlayerSe_AtFrame(p, 158, 67);
        PlayerSe_VoiceAtFrame(p, 16.0f, 0);
        PlayerSe_EquipAtFrame(p, 16.0f, 0);
        PlayerSe_VoiceAtFrame(p, 46.0f, 0);
        PlayerSe_EquipAtFrame(p, 46.0f, 0);
        PlayerSe_VoiceAtFrame(p, 74.0f, 0);
        PlayerSe_VoiceAtFrame(p, 98.0f, 1);
        PlayerSe_VoiceAtFrame(p, 144.0f, 0);
        PlayerSe_EquipAtFrame(p, 144.0f, 0);
        PlayerSe_VoiceAtFrame(p, 178.0f, 0);
        PlayerSe_EquipAtFrame(p, 178.0f, 0);
        break;
    case 0x25A:
        PlayerSe_AtFrame(p, 48, 80);
        PlayerSe_AtFrame(p, 62, 80);
        break;
    case 0x25B:
        PlayerSe_VoiceAtFrame(p, 16.0f, 1);
        PlayerSe_VoiceAtFrame(p, 48.0f, 1);
        PlayerSe_AtFrame(p, 34, 49);
        PlayerSe_AtFrame(p, 48, 49);
        PlayerSe_AtFrame(p, 62, 49);
        PlayerSe_EquipAtFrame(p, 120.0f, 0);
        break;
    case 0x25C:
        PlayerSe_AtFrame(p, 12, 68);
        break;
    case 0x25D:
        PlayerSe_VoiceAtFrame(p, 52.0f, 0);
        PlayerSe_EquipAtFrame(p, 52.0f, 0);
        PlayerSe_AtFrame(p, 22, 74);
        break;
    case 0x25F:
        PlayerSe_VoiceAtFrame(p, 34.0f, 0);
        PlayerSe_EquipAtFrame(p, 34.0f, 0);
        break;
    case 0x260:
        PlayerSe_VoiceAtFrame(p, 34.0f, 0);
        PlayerSe_EquipAtFrame(p, 34.0f, 0);
        PlayerSe_VoiceAtFrame(p, 54.0f, 1);
        PlayerSe_AtFrame(p, 68, 74);
        break;
    case 0x262:
        PlayerSe_VoiceAtFrame(p, 14.0f, 0);
        PlayerSe_VoiceAtFrame(p, 64.0f, 1);
        PlayerSe_EquipAtFrame(p, 64.0f, 0);
        PlayerSe_AtFrame(p, 12, 68);
        break;
    case 0x263:
        PlayerSe_AtFrame(p, 42, 81);
        PlayerSe_AtFrame(p, 64, 81);
        PlayerSe_AtFrame(p, 108, 83);
        PlayerSe_EquipAtFrame(p, 108.0f, 4);
        break;
    case 0x264:
        PlayerSe_AtFrame(p, 124, 75);
        PlayerSe_AtFrame(p, 42, 61);
        PlayerSe_AtFrame(p, 56, 82);
        PlayerSe_EquipAtFrame(p, 48.0f, 4);
        PlayerSe_AtFrame(p, 128, 83);
        break;
    case 0x265:
        PlayerSe_AtFrame(p, 72, 58);
        PlayerSe_EquipAtFrame(p, 76.0f, 4);
        PlayerSe_AtFrame(p, 132, 84);
        PlayerSe_AtFrame(p, 206, 85);
        break;
    case 0x266:
        PlayerSe_AtFrame(p, 38, 68);
        PlayerSe_EquipAtFrame(p, 62.0f, 4);
        break;
    case 0x267:
        PlayerSe_AtFrame(p, 42, 74);
        PlayerSe_EquipAtFrame(p, 70.0f, 4);
        PlayerSe_EquipAtFrame(p, 154.0f, 4);
        PlayerSe_EquipAtFrame(p, 220.0f, 4);
        break;
    case 0x268:
        PlayerSe_AtFrame(p, 32, 67);
        PlayerSe_EquipAtFrame(p, 82.0f, 4);
        PlayerSe_EquipAtFrame(p, 102.0f, 0);
        PlayerSe_AtFrame(p, 74, 64);
        PlayerSe_AtFrame(p, 126, 84);
        PlayerSe_AtFrame(p, 246, 85);
        break;
    case 0x269:
        PlayerSe_AtFrame(p, 46, 67);
        PlayerSe_VoiceAtFrame(p, 62.0f, 1);
        PlayerSe_VoiceAtFrame(p, 102.0f, 3);
        PlayerSe_EquipAtFrame(p, 104.0f, 0);
        PlayerSe_VoiceAtFrame(p, 140.0f, 3);
        PlayerSe_EquipAtFrame(p, 142.0f, 0);
        PlayerSe_VoiceAtFrame(p, 176.0f, 0);
        PlayerSe_EquipAtFrame(p, 176.0f, 0);
        break;
    case 0x26A:
        PlayerSe_AtFrame(p, 38, 83);
        PlayerSe_AtFrame(p, 126, 58);
        PlayerSe_EquipAtFrame(p, 42.0f, 4);
        PlayerSe_VoiceAtFrame(p, 68.0f, 0);
        PlayerSe_VoiceAtFrame(p, 94.0f, 0);
        PlayerSe_EquipAtFrame(p, 96.0f, 0);
        PlayerSe_AtFrame(p, 126, 61);
        PlayerSe_EquipAtFrame(p, 132.0f, 4);
        PlayerSe_AtFrame(p, 190, 61);
        PlayerSe_EquipAtFrame(p, 192.0f, 4);
        break;
    case 0x26B:
        PlayerSe_VoiceAtFrame(p, 40.0f, 0);
        PlayerSe_VoiceAtFrame(p, 54.0f, 1);
        PlayerSe_EquipAtFrame(p, 40.0f, 0);
        break;
    case 0x26C:
        PlayerSe_VoiceAtFrame(p, 16.0f, 1);
        PlayerSe_EquipAtFrame(p, 42.0f, 0);
        PlayerSe_VoiceAtFrame(p, 42.0f, 0);
        PlayerSe_EquipAtFrame(p, 62.0f, 4);
        break;
    case 0x26D:
        PlayerSe_VoiceAtFrame(p, 50.0f, 0);
        PlayerSe_EquipAtFrame(p, 50.0f, 0);
        PlayerSe_AtFrame(p, 10, 68);
        PlayerSe_VoiceAtFrame(p, 114.0f, 0);
        break;
    case 0x26E:
        PlayerSe_VoiceAtFrame(p, 22.0f, 3);
        PlayerSe_VoiceAtFrame(p, 10.0f, 3);
        PlayerSe_EquipAtFrame(p, 22.0f, 0);
        PlayerSe_EquipAtFrame(p, 48.0f, 4);
        PlayerSe_AtFrame(p, 54, 61);
        PlayerSe_EquipAtFrame(p, 70.0f, 0);
        PlayerSe_VoiceAtFrame(p, 66.0f, 3);
        PlayerSe_AtFrame(p, 66, 58);
        break;
    case 0x26F:
        PlayerSe_VoiceAtFrame(p, 30.0f, 0);
        PlayerSe_EquipAtFrame(p, 30.0f, 0);
        PlayerSe_WeaponAtFrame(p, 46, 41);
        PlayerSe_AtFrame(p, 66, 64);
        PlayerSe_AtFrame(p, 80, 58);
        PlayerSe_EquipAtFrame(p, 80.0f, 4);
        break;
    case 0x270:
        PlayerSe_AtFrame(p, 4, 68);
        break;
    case 0x271:
        PlayerSe_VoiceAtFrame(p, 28.0f, 0);
        PlayerSe_EquipAtFrame(p, 28.0f, 0);
        PlayerSe_AtFrame(p, 28, 58);
        PlayerSe_VoiceAtFrame(p, 56.0f, 0);
        PlayerSe_EquipAtFrame(p, 56.0f, 0);
        PlayerSe_AtFrame(p, 56, 58);
        break;
    case 0x272:
        PlayerSe_VoiceAtFrame(p, 34.0f, 0);
        PlayerSe_EquipAtFrame(p, 34.0f, 0);
        break;
    case 0x273:
        PlayerSe_VoiceAtFrame(p, 16.0f, 0);
        PlayerSe_EquipAtFrame(p, 16.0f, 0);
        PlayerSe_VoiceAtFrame(p, 32.0f, 1);
        PlayerSe_AtFrame(p, 30, 49);
        PlayerSe_AtFrame(p, 42, 49);
        break;
    case 0x274:
        PlayerSe_OnlineAtFrame(p, 38, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_OnlineAtFrame(p, 54, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_OnlineAtFrame(p, 176, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_OnlineAtFrame(p, 166, Sound_RandomSe(7, 2, 8, 2));
        PlayerSe_WeaponAtFrame(p, 38, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_WeaponAtFrame(p, 54, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_WeaponAtFrame(p, 176, Sound_RandomSe(39, 39, 40, 2));
        PlayerSe_WeaponAtFrame(p, 166, Sound_RandomSe(39, 2, 40, 2));
        PlayerSe_VoiceAtFrame(p, 10.0f, 0);
        PlayerSe_EquipAtFrame(p, 10.0f, 0);
        PlayerSe_VoiceAtFrame(p, 44.0f, 0);
        PlayerSe_EquipAtFrame(p, 44.0f, 0);
        PlayerSe_AtFrame(p, 38, 61);
        PlayerSe_EquipAtFrame(p, 176.0f, 0);
        PlayerSe_AtFrame(p, 176, 61);
        PlayerSe_EquipAtFrame(p, 292.0f, 0);
        PlayerSe_EquipAtFrame(p, 40.0f, 4);
        PlayerSe_AtFrame(p, 54, 61);
        PlayerSe_EquipAtFrame(p, 56.0f, 4);
        PlayerSe_EquipAtFrame(p, 88.0f, 4);
        PlayerSe_EquipAtFrame(p, 114.0f, 4);
        PlayerSe_EquipAtFrame(p, 230.0f, 4);
        PlayerSe_EquipAtFrame(p, 246.0f, 4);
        PlayerSe_EquipAtFrame(p, 168.0f, 4);
        PlayerSe_AtFrame(p, 166, 61);
        PlayerSe_EquipAtFrame(p, 176.0f, 4);
        break;
    case 0x275:
        PlayerSe_AtFrame(p, 316, 69);
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        PlayerSe_EquipAtFrame(p, 26.0f, 0);
        break;
    case 0x276:
        PlayerSe_VoiceAtFrame(p, 26.0f, 1);
        PlayerSe_VoiceAtFrame(p, 74.0f, 1);
        PlayerSe_VoiceAtFrame(p, 132.0f, 0);
        PlayerSe_VoiceAtFrame(p, 154.0f, 0);
        PlayerSe_VoiceAtFrame(p, 186.0f, 0);
        PlayerSe_VoiceAtFrame(p, 214.0f, 0);
        PlayerSe_VoiceAtFrame(p, 252.0f, 0);
        PlayerSe_VoiceAtFrame(p, 278.0f, 0);
        PlayerSe_VoiceAtFrame(p, 318.0f, 0);
        PlayerSe_EquipAtFrame(p, 86.0f, 0);
        PlayerSe_EquipAtFrame(p, 150.0f, 0);
        PlayerSe_EquipAtFrame(p, 216.0f, 0);
        PlayerSe_EquipAtFrame(p, 266.0f, 0);
        PlayerSe_EquipAtFrame(p, 138.0f, 4);
        PlayerSe_EquipAtFrame(p, 204.0f, 4);
        PlayerSe_EquipAtFrame(p, 248.0f, 4);
        PlayerSe_EquipAtFrame(p, 312.0f, 4);
        break;
    case 0x277:
        PlayerSe_VoiceAtFrame(p, 42.0f, 0);
        PlayerSe_EquipAtFrame(p, 42.0f, 0);
        PlayerSe_VoiceAtFrame(p, 198.0f, 0);
        PlayerSe_EquipAtFrame(p, 198.0f, 0);
        break;
    case 0x278:
        PlayerSe_VoiceAtFrame(p, 32.0f, 0);
        PlayerSe_EquipAtFrame(p, 34.0f, 0);
        PlayerSe_EquipAtFrame(p, 46.0f, 4);
        PlayerSe_EquipAtFrame(p, 70.0f, 4);
        PlayerSe_EquipAtFrame(p, 118.0f, 4);
        PlayerSe_EquipAtFrame(p, 134.0f, 4);
        break;
    case 0x279:
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        PlayerSe_EquipAtFrame(p, 26.0f, 0);
        PlayerSe_AtFrame(p, 42, 62);
        PlayerSe_EquipAtFrame(p, 42.0f, 4);
        break;
    case 0x27B:
        Motion_SeAtFrameLocal(p, 138, 86);
        Motion_SeAtFrameLocal(p, 222, 53);
        Motion_SeAtFrameLocal(p, 234, 52);
        Motion_SeAtFrameLocal(p, 256, 55);
        Motion_SeAtFrameLocal(p, 270, 50);
        Motion_SeAtFrameLocal(p, 286, 53);
        Motion_SeAtFrameLocal(p, 310, 51);
        Motion_SeAtFrameLocal(p, 328, 51);
        Motion_SeAtFrameLocal(p, 380, 52);
        Motion_SeAtFrameLocal(p, 388, 52);
        Motion_SeAtFrameLocal(p, 412, 55);
        Motion_SeAtFrameLocal(p, 434, 50);
        Motion_SeAtFrameLocal(p, 478, 53);
        Motion_SeAtFrameLocal(p, 504, 51);
        Motion_SeAtFrameLocal(p, 504, 52);
        Motion_SeAtFrameLocal(p, 230, 87);
        Motion_SeAtFrameLocal(p, 250, 87);
        Motion_SeAtFrameLocal(p, 290, 87);
        Motion_SeAtFrameLocal(p, 350, 87);
        Motion_SeAtFrameLocal(p, 380, 87);
        Motion_SeAtFrameLocal(p, 478, 87);
        Motion_SeAtFrameLocal(p, 412, 87);
        Motion_SeAtFrameLocal(p, 440, 87);
        Motion_SeAtFrameLocal(p, 560, 87);
        PlayerSe_EquipAtFrame(p, 26.0f, 0);
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        break;
    case 0x27C:
        PlayerSe_OnlineAtFrame(p, 26, Sound_RandomSe(4, 4, 5, 4));
        Motion_SeAtFrameLocal(p, 22, 74);
        Motion_SeAtFrameLocal(p, 22, 75);
        PlayerSe_VoiceAtFrame(p, 26.0f, 0);
        PlayerSe_EquipAtFrame(p, 26.0f, 0);
        PlayerSe_VoiceAtFrame(p, 56.0f, 2);
        PlayerSe_EquipAtFrame(p, 56.0f, 0);
        PlayerSe_VoiceAtFrame(p, 68.0f, 2);
        PlayerSe_EquipAtFrame(p, 68.0f, 0);
        PlayerSe_VoiceAtFrame(p, 80.0f, 2);
        PlayerSe_EquipAtFrame(p, 80.0f, 0);
        PlayerSe_VoiceAtFrame(p, 90.0f, 2);
        PlayerSe_EquipAtFrame(p, 90.0f, 0);
        PlayerSe_VoiceAtFrame(p, 100.0f, 2);
        PlayerSe_EquipAtFrame(p, 100.0f, 0);
        PlayerSe_VoiceAtFrame(p, 110.0f, 2);
        PlayerSe_EquipAtFrame(p, 110.0f, 0);
        PlayerSe_VoiceAtFrame(p, 126.0f, 2);
        PlayerSe_EquipAtFrame(p, 126.0f, 0);
        Motion_SeAtFrameLocal(p, 150, 58);
        break;
    case 0x27D:
        PlayerSe_VoiceAtFrame(p, 30.0f, 0);
        PlayerSe_EquipAtFrame(p, 30.0f, 0);
        Motion_SeAtFrameLocal(p, 44, 67);
        PlayerSe_EquipAtFrame(p, 44.0f, 4);
        Motion_SeAtFrameLocal(p, 60, 67);
        PlayerSe_EquipAtFrame(p, 60.0f, 4);
        Motion_SeAtFrameLocal(p, 80, 86);
        PlayerSe_EquipAtFrame(p, 80.0f, 4);
        PlayerSe_VoiceAtFrame(p, 146.0f, 0);
        PlayerSe_EquipAtFrame(p, 146.0f, 0);
        break;
    case 0x280:
        PlayerSe_EquipAtFrame(p, 14.0f, 4);
        PlayerSe_AtFrame(p, 14, 69);
        break;
    case 0x282:
        PlayerSe_EquipAtFrame(p, 14.0f, 4);
        PlayerSe_VoiceAtFrame(p, 86.0f, 0);
        PlayerSe_EquipAtFrame(p, 86.0f, 0);
        PlayerSe_VoiceAtFrame(p, 98.0f, 1);
        PlayerSe_EquipAtFrame(p, 98.0f, 0);
        PlayerSe_AtFrame(p, 116, 83);
        break;
    case 0x283:
        PlayerSe_AtFrame(p, 24, 83);
        PlayerSe_EquipAtFrame(p, 24.0f, 4);
        break;
    case 0x287:
        PlayerSe_VoiceAtFrame(p, 26.0f, 1);
        PlayerSe_AtFrame(p, 78, 91);
        PlayerSe_EquipAtFrame(p, 78.0f, 0);
        PlayerSe_AtFrame(p, 116, 92);
        PlayerSe_EquipAtFrame(p, 116.0f, 0);
        PlayerSe_AtFrame(p, 192, 75);
        PlayerSe_AtFrame(p, 230, 88);
        PlayerSe_EquipAtFrame(p, 244.0f, 4);
        PlayerSe_AtFrame(p, 286, 89);
        PlayerSe_EquipAtFrame(p, 300.0f, 0);
        PlayerSe_AtFrame(p, 300, 93);
        PlayerSe_AtFrame(p, 390, 94);
        PlayerSe_AtFrame(p, 416, 61);
        PlayerSe_AtFrame(p, 566, 90);
        PlayerSe_EquipAtFrame(p, 430.0f, 0);
        PlayerSe_VoiceAtFrame(p, 430.0f, 0);
        PlayerSe_EquipAtFrame(p, 430.0f, 0);
        PlayerSe_EquipAtFrame(p, 580.0f, 0);
        PlayerSe_VoiceAtFrame(p, 580.0f, 0);
        PlayerSe_EquipAtFrame(p, 608.0f, 0);
        PlayerSe_VoiceAtFrame(p, 608.0f, 0);
        PlayerSe_AtFrame(p, 282, 62);
        PlayerSe_EquipAtFrame(p, 42.0f, 4);
        break;
    case 0x288:
        PlayerSe_VoiceAtFrame(p, 12.0f, 1);
        PlayerSe_AtFrame(p, 46, 91);
        PlayerSe_EquipAtFrame(p, 46.0f, 0);
        PlayerSe_AtFrame(p, 86, 92);
        PlayerSe_EquipAtFrame(p, 86.0f, 0);
        PlayerSe_AtFrame(p, 102, 93);
        PlayerSe_AtFrame(p, 140, 75);
        PlayerSe_AtFrame(p, 158, 88);
        PlayerSe_EquipAtFrame(p, 168.0f, 4);
        PlayerSe_AtFrame(p, 254, 88);
        PlayerSe_AtFrame(p, 276, 88);
        PlayerSe_EquipAtFrame(p, 280.0f, 4);
        PlayerSe_AtFrame(p, 300, 88);
        PlayerSe_EquipAtFrame(p, 302.0f, 4);
        PlayerSe_AtFrame(p, 332, 88);
        PlayerSe_EquipAtFrame(p, 336.0f, 4);
        PlayerSe_AtFrame(p, 368, 88);
        PlayerSe_AtFrame(p, 398, 88);
        PlayerSe_EquipAtFrame(p, 406.0f, 4);
        PlayerSe_AtFrame(p, 250, 91);
        PlayerSe_EquipAtFrame(p, 468.0f, 4);
        PlayerSe_EquipAtFrame(p, 466.0f, 0);
        PlayerSe_AtFrame(p, 466, 92);
        PlayerSe_EquipAtFrame(p, 466.0f, 0);
        PlayerSe_AtFrame(p, 486, 91);
        PlayerSe_EquipAtFrame(p, 486.0f, 0);
        PlayerSe_AtFrame(p, 502, 92);
        PlayerSe_EquipAtFrame(p, 556.0f, 4);
        PlayerSe_EquipAtFrame(p, 556.0f, 4);
        PlayerSe_EquipAtFrame(p, 698.0f, 0);
        PlayerSe_AtFrame(p, 698, 92);
        PlayerSe_EquipAtFrame(p, 740.0f, 0);
        PlayerSe_AtFrame(p, 740, 91);
        PlayerSe_EquipAtFrame(p, 792.0f, 0);
        PlayerSe_VoiceAtFrame(p, 792.0f, 0);
        PlayerSe_EquipAtFrame(p, 820.0f, 0);
        PlayerSe_VoiceAtFrame(p, 820.0f, 0);
        break;
    case 0x2A5:
        PlayerSe_VoiceAtFrame(p, 40.0f, 1);
        PlayerSe_EquipAtFrame(p, 46.0f, 0);
        PlayerSe_AtFrame(p, 96, 75);
        PlayerSe_EquipAtFrame(p, 96.0f, 4);
        PlayerSe_AtFrame(p, 96, 67);
        PlayerSe_AtFrame(p, 140, 58);
        PlayerSe_AtFrame(p, 140, 74);
        break;
    case 0x2A7:
        PlayerSe_AtFrame(p, 38, 67);
        PlayerSe_VoiceAtFrame(p, 30.0f, 1);
        PlayerSe_VoiceAtFrame(p, 60.0f, 0);
        PlayerSe_EquipAtFrame(p, 60.0f, 0);
        PlayerSe_VoiceAtFrame(p, 110.0f, 1);
        break;
    case 0x320:
        PlayerSe_AtFrame(p, 24, 74);
        PlayerSe_AtFrame(p, 72, 58);
        PlayerSe_EquipAtFrame(p, 54.0f, 0);
        PlayerSe_VoiceAtFrame(p, 54.0f, 0);
        break;
    case 0x321:
        PlayerSe_WeaponAtFrame(p, 4, 37);
        PlayerSe_AtFrame(p, 4, 74);
        PlayerSe_VoiceAtFrame(p, 18.0f, 0);
        PlayerSe_EquipAtFrame(p, 18.0f, 0);
        break;
    case 0x322:
        PlayerSe_AtFrame(p, 4, 74);
        PlayerSe_WeaponAtFrame(p, 56, 41);
        PlayerSe_VoiceAtFrame(p, 118.0f, 0);
        PlayerSe_EquipAtFrame(p, 118.0f, 0);
        break;
    case 0x323:
        PlayerSe_AtFrame(p, 58, 40);
        PlayerSe_AtFrame(p, 58, 41);
        PlayerSe_AtFrame(p, 120, 41);
        PlayerSe_AtFrame(p, 180, 41);
        PlayerSe_AtFrame(p, 180, 40);
        PlayerSe_AtFrame(p, 242, 41);
        break;
    case 0x325:
        PlayerSe_VoiceAtFrame(p, 60.0f, 1);
        PlayerSe_EquipAtFrame(p, 94.0f, 0);
        PlayerSe_VoiceAtFrame(p, 94.0f, 1);
        PlayerSe_VoiceAtFrame(p, 142.0f, 1);
        PlayerSe_AtFrame(p, 100, 43);
        PlayerSe_EquipAtFrame(p, 106.0f, 4);
        break;
    case 0x327:
        PlayerSe_VoiceAtFrame(p, 32.0f, 3);
        PlayerSe_EquipAtFrame(p, 32.0f, 0);
        PlayerSe_VoiceAtFrame(p, 54.0f, 1);
        PlayerSe_EquipAtFrame(p, 54.0f, 0);
        PlayerSe_VoiceAtFrame(p, 84.0f, 1);
        PlayerSe_EquipAtFrame(p, 32.0f, 0);
        PlayerSe_EquipAtFrame(p, 34.0f, 4);
        PlayerSe_AtFrame(p, 52, 75);
        PlayerSe_AtFrame(p, 54, 45);
        PlayerSe_AtFrame(p, 96, 45);
        PlayerSe_AtFrame(p, 210, 45);
        PlayerSe_AtFrame(p, 160, 45);
        break;
    case 0x328:
        PlayerSe_OnlineAtFrame(p, 16, 2);
        PlayerSe_WeaponAtFrame(p, 16, 38);
        PlayerSe_AtFrame(p, 12, 43);
        PlayerSe_AtFrame(p, 22, 45);
        PlayerSe_AtFrame(p, 84, 75);
        PlayerSe_VoiceAtFrame(p, 24.0f, 0);
        PlayerSe_EquipAtFrame(p, 24.0f, 0);
        PlayerSe_VoiceAtFrame(p, 76.0f, 0);
        PlayerSe_VoiceAtFrame(p, 100.0f, 1);
        PlayerSe_VoiceAtFrame(p, 180.0f, 1);
        break;
    case 0x329:
        PlayerSe_AtFrame(p, 28, 43);
        PlayerSe_VoiceAtFrame(p, 66.0f, 0);
        PlayerSe_EquipAtFrame(p, 94.0f, 0);
        PlayerSe_VoiceAtFrame(p, 94.0f, 1);
        PlayerSe_VoiceAtFrame(p, 212.0f, 1);
        PlayerSe_VoiceAtFrame(p, 226.0f, 1);
        break;
    case 0x32A:
        PlayerSe_AtFrame(p, 10, 43);
        PlayerSe_AtFrame(p, 10, 45);
        PlayerSe_AtFrame(p, 54, 65);
        PlayerSe_EquipAtFrame(p, 118.0f, 0);
        PlayerSe_EquipAtFrame(p, 122.0f, 4);
        PlayerSe_VoiceAtFrame(p, 176.0f, 1);
        break;
    case 0x363:
        PlayerSe_AtFrame(p, 2, 61);
        PlayerSe_AtFrame(p, 14, 58);
        PlayerSe_AtFrame(p, 114, 82);
        PlayerSe_EquipAtFrame(p, 116.0f, 4);
        PlayerSe_AtFrame(p, 130, 82);
        PlayerSe_EquipAtFrame(p, 312.0f, 4);
        PlayerSe_AtFrame(p, 178, 83);
        break;
    case 0x37A:
        PlayerSe_OnlineAtFrame(p, 160, Sound_RandomSe(0, 4, 1, 4));
        PlayerSe_AtFrame(p, 28, 74);
        PlayerSe_EquipAtFrame(p, 36.0f, 4);
        PlayerSe_AtFrame(p, 80, 63);
        PlayerSe_AtFrame(p, 128, 63);
        PlayerSe_AtFrame(p, 162, 83);
        PlayerSe_EquipAtFrame(p, 38.0f, 0);
        PlayerSe_VoiceAtFrame(p, 42.0f, 1);
        break;
    case 0x37B:
        PlayerSe_OnlineAtFrame(p, 188, 2);
        PlayerSe_OnlineAtFrame(p, 250, Sound_RandomSe(7, 4, 8, 4));
        PlayerSe_OnlineAtFrame(p, 296, 2);
        PlayerSe_OnlineAtFrame(p, 562, 2);
        PlayerSe_OnlineAtFrame(p, 340, Sound_RandomSe(7, 4, 8, 4));
        PlayerSe_EquipAtFrame(p, 20.0f, 0);
        PlayerSe_VoiceAtFrame(p, 18.0f, 1);
        PlayerSe_EquipAtFrame(p, 180.0f, 0);
        PlayerSe_VoiceAtFrame(p, 176.0f, 3);
        PlayerSe_AtFrame(p, 186, 69);
        PlayerSe_AtFrame(p, 334, 69);
        PlayerSe_AtFrame(p, 490, 83);
        PlayerSe_AtFrame(p, 506, 83);
        PlayerSe_AtFrame(p, 518, 83);
        PlayerSe_EquipAtFrame(p, 206.0f, 4);
        PlayerSe_EquipAtFrame(p, 260.0f, 4);
        PlayerSe_EquipAtFrame(p, 300.0f, 4);
        PlayerSe_EquipAtFrame(p, 558.0f, 4);
        PlayerSe_AtFrame(p, 52, 45);
        PlayerSe_AtFrame(p, 192, 45);
        PlayerSe_AtFrame(p, 262, 45);
        PlayerSe_AtFrame(p, 304, 45);
        PlayerSe_AtFrame(p, 446, 45);
        PlayerSe_AtFrame(p, 562, 45);
        PlayerSe_AtFrame(p, 217, 45);
        PlayerSe_AtFrame(p, 280, 45);
        PlayerSe_AtFrame(p, 320, 45);
        PlayerSe_AtFrame(p, 600, 45);
        break;
    case 0x37C:
        PlayerSe_OnlineAtFrame(p, 4, 3);
        PlayerSe_AtFrame(p, 8, 83);
        PlayerSe_AtFrame(p, 8, 59);
        PlayerSe_EquipAtFrame(p, 12.0f, 4);
        PlayerSe_EquipAtFrame(p, 84.0f, 4);
        PlayerSe_AtFrame(p, 84, 74);
        break;
    case 0x37D:
        PlayerSe_OnlineAtFrame(p, 4, 6);
        PlayerSe_OnlineAtFrame(p, 134, 9);
        PlayerSe_AtFrame(p, 4, 63);
        PlayerSe_AtFrame(p, 46, 66);
        PlayerSe_AtFrame(p, 76, 60);
        PlayerSe_AtFrame(p, 230, 67);
        PlayerSe_EquipAtFrame(p, 76.0f, 0);
        PlayerSe_AtFrame(p, 272, 67);
        PlayerSe_EquipAtFrame(p, 276.0f, 0);
        PlayerSe_AtFrame(p, 334, 58);
        PlayerSe_EquipAtFrame(p, 336.0f, 4);
        PlayerSe_AtFrame(p, 366, 58);
        break;
    case 0x37E:
        PlayerSe_OnlineAtFrame(p, 160, Sound_RandomSe(0, 4, 1, 4));
        PlayerSe_AtFrame(p, 36, 74);
        PlayerSe_EquipAtFrame(p, 36.0f, 4);
        PlayerSe_AtFrame(p, 80, 63);
        PlayerSe_AtFrame(p, 114, 63);
        PlayerSe_AtFrame(p, 166, 83);
        PlayerSe_EquipAtFrame(p, 38.0f, 0);
        PlayerSe_VoiceAtFrame(p, 42.0f, 1);
        break;
    case 0x37F:
        PlayerSe_EquipAtFrame(p, 20.0f, 0);
        PlayerSe_VoiceAtFrame(p, 18.0f, 1);
        PlayerSe_EquipAtFrame(p, 324.0f, 0);
        PlayerSe_VoiceAtFrame(p, 322.0f, 3);
        PlayerSe_AtFrame(p, 170, 69);
        PlayerSe_AtFrame(p, 334, 69);
        PlayerSe_AtFrame(p, 304, 83);
        PlayerSe_AtFrame(p, 322, 83);
        PlayerSe_AtFrame(p, 626, 83);
        PlayerSe_AtFrame(p, 642, 83);
        PlayerSe_AtFrame(p, 664, 83);
        PlayerSe_EquipAtFrame(p, 24.0f, 4);
        PlayerSe_EquipAtFrame(p, 90.0f, 4);
        PlayerSe_EquipAtFrame(p, 146.0f, 4);
        PlayerSe_EquipAtFrame(p, 372.0f, 4);
        PlayerSe_EquipAtFrame(p, 468.0f, 4);
        PlayerSe_AtFrame(p, 22, 45);
        PlayerSe_AtFrame(p, 88, 45);
        PlayerSe_AtFrame(p, 144, 45);
        PlayerSe_AtFrame(p, 370, 45);
        PlayerSe_AtFrame(p, 468, 45);
        break;
    case 0x380:
        PlayerSe_OnlineAtFrame(p, 4, 3);
        PlayerSe_AtFrame(p, 8, 83);
        PlayerSe_AtFrame(p, 8, 59);
        PlayerSe_EquipAtFrame(p, 12.0f, 4);
        PlayerSe_EquipAtFrame(p, 84.0f, 4);
        PlayerSe_AtFrame(p, 84, 74);
        break;
    case 0x381:
        PlayerSe_OnlineAtFrame(p, 4, 6);
        PlayerSe_OnlineAtFrame(p, 134, 9);
        PlayerSe_AtFrame(p, 4, 63);
        PlayerSe_AtFrame(p, 46, 66);
        PlayerSe_AtFrame(p, 76, 60);
        PlayerSe_AtFrame(p, 230, 67);
        PlayerSe_EquipAtFrame(p, 76.0f, 0);
        PlayerSe_AtFrame(p, 272, 67);
        PlayerSe_EquipAtFrame(p, 276.0f, 0);
        PlayerSe_AtFrame(p, 348, 58);
        PlayerSe_EquipAtFrame(p, 350.0f, 4);
        PlayerSe_AtFrame(p, 380, 58);
        break;
    }
    switch (p->weaponType) {
    case 1:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 76, 2);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 86, 6);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 32.0f, 0);
            return;
        case 0x3F1:
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 24.0f, 0);
            PlayerSe_VoiceAtFrame(p, 50.0f, 0);
            PlayerSe_VoiceAtFrame(p, 78.0f, 0);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            PlayerSe_EquipAtFrame(p, 50.0f, 0);
            PlayerSe_EquipAtFrame(p, 78.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 46, 6);
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 92.0f, 0);
            PlayerSe_VoiceAtFrame(p, 108.0f, 0);
            PlayerSe_EquipAtFrame(p, 108.0f, 0);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 44, 13);
            PlayerSe_WeaponAtFrame(p, 66, 14);
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_WeaponAtFrame(p, 104, 4);
            return;
        case 0x57D:
        case 0x57E:
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            return;
        case 0x580:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 44.0f, 0);
            PlayerSe_EquipAtFrame(p, 64.0f, 0);
            PlayerSe_VoiceAtFrame(p, 64.0f, 2);
            PlayerSe_VoiceAtFrame(p, 130.0f, 2);
            PlayerSe_EquipAtFrame(p, 130.0f, 0);
            return;
        case 0x581:
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 108.0f, 0);
            PlayerSe_VoiceAtFrame(p, 136.0f, 0);
            PlayerSe_EquipAtFrame(p, 136.0f, 0);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(p, 2, 4);
            PlayerSe_WeaponAtFrame(p, 68, 9);
            PlayerSe_WeaponAtFrame(p, 180, 1);
            return;
        case 0x584:
        case 0x588:
        case 0x589:
            PlayerSe_WeaponAtFrame(p, 12, 8);
            return;
        case 0x585:
            PlayerSe_WeaponAtFrame(p, 24, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 4, 7);
            PlayerSe_WeaponAtFrame(p, 12, 1);
            PlayerSe_AtFrame(p, 8, 61);
            return;
        case 0x58A:
            PlayerSe_VoiceAtFrame(p, 60.0f, 0);
            PlayerSe_EquipAtFrame(p, 60.0f, 0);
            return;
        case 0x58B:
            PlayerSe_VoiceAtFrame(p, 116.0f, 0);
            PlayerSe_EquipAtFrame(p, 116.0f, 0);
            PlayerSe_AtFrame(p, 2, 67);
            return;
        case 0x58E:
            PlayerSe_VoiceAtFrame(p, 20.0f, 0);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            PlayerSe_VoiceAtFrame(p, 90.0f, 0);
            PlayerSe_EquipAtFrame(p, 90.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 72, 2);
            return;
        }
        break;
    case 2:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_EquipAtFrame(p, 24.0f, 4);
            PlayerSe_WeaponAtFrame(p, 66, 2);
            PlayerSe_VoiceAtFrame(p, 60.0f, 3);
            PlayerSe_EquipAtFrame(p, 60.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 2, 3);
            PlayerSe_EquipAtFrame(p, 24.0f, 4);
            PlayerSe_VoiceAtFrame(p, 72.0f, 0);
            PlayerSe_EquipAtFrame(p, 72.0f, 0);
            return;
        case 0x3EC:
        case 0x3ED:
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 2);
            PlayerSe_VoiceAtFrame(p, 56.0f, 2);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 56.0f, 0);
            return;
        case 0x3EE:
            PlayerSe_VoiceAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 3);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            PlayerSe_WeaponAtFrame(p, 20, 5);
            PlayerSe_WeaponAtFrame(p, 4, 2);
            return;
        case 0x3F1:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 2, 3);
            PlayerSe_EquipAtFrame(p, 24.0f, 4);
            PlayerSe_VoiceAtFrame(p, 36.0f, 0);
            PlayerSe_VoiceAtFrame(p, 64.0f, 0);
            PlayerSe_EquipAtFrame(p, 36.0f, 0);
            PlayerSe_EquipAtFrame(p, 64.0f, 0);
            return;
        case 0x3F2:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 12, 2);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            return;
        case 0x3F3:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_AtFrame(p, 2, 63);
            PlayerSe_EquipAtFrame(p, 20.0f, 3);
            PlayerSe_AtFrame(p, 28, 65);
            PlayerSe_VoiceAtFrame(p, 56.0f, 0);
            PlayerSe_EquipAtFrame(p, 52.0f, 0);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_AtFrame(p, 2, 63);
            PlayerSe_EquipAtFrame(p, 20.0f, 3);
            PlayerSe_AtFrame(p, 28, 65);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(p, 28, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 4, 2);
            PlayerSe_WeaponAtFrame(p, 26, 4);
            PlayerSe_EquipAtFrame(p, 4.0f, 4);
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(p, 48, Sound_RandomSe(36, 6, 35, 2));
            PlayerSe_WeaponAtFrame(p, 14, 5);
            PlayerSe_WeaponAtFrame(p, 40, 4);
            PlayerSe_WeaponAtFrame(p, 58, 7);
            PlayerSe_EquipAtFrame(p, 14.0f, 4);
            PlayerSe_VoiceAtFrame(p, 52.0f, 3);
            PlayerSe_EquipAtFrame(p, 54.0f, 0);
            PlayerSe_WeaponAtFrame(p, 102, 5);
            PlayerSe_EquipAtFrame(p, 128.0f, 4);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_VoiceAtFrame(p, 2.0f, 2);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 8.0f, 2);
            PlayerSe_EquipAtFrame(p, 8.0f, 0);
            PlayerSe_WeaponAtFrame(p, 10, 6);
            PlayerSe_WeaponAtFrame(p, 38, 6);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 192, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_WeaponAtFrame(p, 38, 7);
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_VoiceAtFrame(p, 36.0f, 2);
            PlayerSe_EquipAtFrame(p, 36.0f, 0);
            PlayerSe_VoiceAtFrame(p, 56.0f, 0);
            PlayerSe_WeaponAtFrame(p, 116, 5);
            PlayerSe_WeaponAtFrame(p, 170, 2);
            PlayerSe_VoiceAtFrame(p, 168.0f, 0);
            PlayerSe_EquipAtFrame(p, 168.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(p, 24, Sound_RandomSe(35, 4, 35, 4));
            PlayerSe_WeaponAtFrame(p, 66, Sound_RandomSe(36, 2, 36, 4));
            PlayerSe_WeaponAtFrame(p, 12, 4);
            PlayerSe_WeaponAtFrame(p, 64, 4);
            PlayerSe_WeaponAtFrame(p, 64, 5);
            PlayerSe_EquipAtFrame(p, 4.0f, 4);
            PlayerSe_EquipAtFrame(p, 68.0f, 4);
            PlayerSe_VoiceAtFrame(p, 10.0f, 0);
            PlayerSe_EquipAtFrame(p, 10.0f, 0);
            PlayerSe_VoiceAtFrame(p, 48.0f, 2);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            PlayerSe_VoiceAtFrame(p, 92.0f, 0);
            PlayerSe_VoiceAtFrame(p, 112.0f, 0);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            return;
        case 0x57E:
            PlayerSe_WeaponAtFrame(p, 20, 38);
            PlayerSe_WeaponAtFrame(p, 72, 37);
            PlayerSe_WeaponAtFrame(p, 74, 8);
            PlayerSe_WeaponAtFrame(p, 90, 15);
            PlayerSe_WeaponAtFrame(p, 2, 4);
            PlayerSe_WeaponAtFrame(p, 62, 4);
            PlayerSe_WeaponAtFrame(p, 182, 38);
            PlayerSe_WeaponAtFrame(p, 186, 5);
            PlayerSe_WeaponAtFrame(p, 238, 2);
            PlayerSe_EquipAtFrame(p, 30.0f, 4);
            PlayerSe_VoiceAtFrame(p, 70.0f, 3);
            PlayerSe_EquipAtFrame(p, 70.0f, 0);
            PlayerSe_VoiceAtFrame(p, 238.0f, 1);
            PlayerSe_EquipAtFrame(p, 190.0f, 4);
            return;
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 28, 5);
            PlayerSe_WeaponAtFrame(p, 58, 2);
            PlayerSe_EquipAtFrame(p, 4.0f, 4);
            PlayerSe_EquipAtFrame(p, 68.0f, 4);
            PlayerSe_VoiceAtFrame(p, 12.0f, 2);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 38.0f, 0);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            return;
        case 0x580:
            PlayerSe_VoiceAtFrame(p, 12.0f, 2);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_EquipAtFrame(p, 32.0f, 0);
            PlayerSe_WeaponAtFrame(p, 62, 1);
            PlayerSe_EquipAtFrame(p, 90.0f, 4);
            return;
        case 0x581:
            PlayerSe_WeaponAtFrame(p, 4, 5);
            PlayerSe_EquipAtFrame(p, 4.0f, 4);
            return;
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(p, 38, 37);
            PlayerSe_VoiceAtFrame(p, 154.0f, 2);
            PlayerSe_WeaponAtFrame(p, 14, 6);
            PlayerSe_WeaponAtFrame(p, 34, 4);
            PlayerSe_VoiceAtFrame(p, 28.0f, 2);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            PlayerSe_VoiceAtFrame(p, 130.0f, 0);
            PlayerSe_EquipAtFrame(p, 130.0f, 0);
            PlayerSe_WeaponAtFrame(p, 34, 15);
            PlayerSe_WeaponAtFrame(p, 156, 2);
            return;
        case 0x584:
            PlayerSe_WeaponAtFrame(p, 50, Sound_RandomSe(35, 1, 36, 2));
            PlayerSe_WeaponAtFrame(p, 4, 2);
            PlayerSe_WeaponAtFrame(p, 42, 4);
            PlayerSe_EquipAtFrame(p, 4.0f, 4);
            PlayerSe_VoiceAtFrame(p, 50.0f, 0);
            PlayerSe_EquipAtFrame(p, 50.0f, 0);
            PlayerSe_WeaponAtFrame(p, 56, 7);
            return;
        case 0x585:
            PlayerSe_WeaponAtFrame(p, 4, 2);
            PlayerSe_WeaponAtFrame(p, 12, 6);
            PlayerSe_VoiceAtFrame(p, 26.0f, 2);
            PlayerSe_WeaponAtFrame(p, 30, 4);
            return;
        }
        break;
    case 8:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_EquipAtFrame(p, 24.0f, 4);
            PlayerSe_WeaponAtFrame(p, 66, 2);
            PlayerSe_VoiceAtFrame(p, 58.0f, 3);
            PlayerSe_EquipAtFrame(p, 58.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 2, 5);
            PlayerSe_WeaponAtFrame(p, 48, 3);
            PlayerSe_EquipAtFrame(p, 24.0f, 4);
            PlayerSe_VoiceAtFrame(p, 58.0f, 0);
            PlayerSe_EquipAtFrame(p, 60.0f, 0);
            return;
        case 0x3EC:
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(p, 18.0f, 2);
            PlayerSe_VoiceAtFrame(p, 44.0f, 2);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            PlayerSe_EquipAtFrame(p, 44.0f, 0);
            return;
        case 0x3EE:
            PlayerSe_WeaponAtFrame(p, 20, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_VoiceAtFrame(p, 16.0f, 3);
            PlayerSe_EquipAtFrame(p, 52.0f, 0);
            PlayerSe_WeaponAtFrame(p, 20, 4);
            return;
        case 0x3F3:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_AtFrame(p, 2, 63);
            PlayerSe_EquipAtFrame(p, 20.0f, 3);
            PlayerSe_AtFrame(p, 28, 65);
            PlayerSe_VoiceAtFrame(p, 56.0f, 0);
            PlayerSe_EquipAtFrame(p, 52.0f, 0);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_AtFrame(p, 2, 63);
            PlayerSe_EquipAtFrame(p, 20.0f, 3);
            PlayerSe_AtFrame(p, 28, 65);
            return;
        case 0x579:
            PlayerSe_VoiceAtFrame(p, 12.0f, 3);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 70.0f, 0);
            PlayerSe_WeaponAtFrame(p, 20, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_WeaponAtFrame(p, 4, 5);
            PlayerSe_WeaponAtFrame(p, 20, 4);
            PlayerSe_WeaponAtFrame(p, 80, 2);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(p, 40, Sound_RandomSe(35, 3, 35, 3));
            PlayerSe_VoiceAtFrame(p, 20.0f, 3);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            PlayerSe_EquipAtFrame(p, 88.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 5);
            PlayerSe_WeaponAtFrame(p, 34, 4);
            PlayerSe_WeaponAtFrame(p, 70, 2);
            return;
        case 0x57B:
            PlayerSe_VoiceAtFrame(p, 14.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 4);
            PlayerSe_VoiceAtFrame(p, 74.0f, 3);
            PlayerSe_EquipAtFrame(p, 74.0f, 0);
            PlayerSe_VoiceAtFrame(p, 192.0f, 0);
            PlayerSe_EquipAtFrame(p, 194.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 5);
            PlayerSe_WeaponAtFrame(p, 22, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_WeaponAtFrame(p, 76, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(p, 24, 6);
            PlayerSe_WeaponAtFrame(p, 74, 4);
            PlayerSe_WeaponAtFrame(p, 84, 7);
            PlayerSe_WeaponAtFrame(p, 158, 5);
            PlayerSe_WeaponAtFrame(p, 192, 2);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 18, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_VoiceAtFrame(p, 12.0f, 3);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 62.0f, 0);
            PlayerSe_EquipAtFrame(p, 62.0f, 0);
            PlayerSe_WeaponAtFrame(p, 14, 6);
            return;
        case 0x580:
            PlayerSe_VoiceAtFrame(p, 10.0f, 0);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 62.0f, 3);
            PlayerSe_VoiceAtFrame(p, 94.0f, 2);
            PlayerSe_VoiceAtFrame(p, 110.0f, 2);
            PlayerSe_EquipAtFrame(p, 112.0f, 0);
            PlayerSe_VoiceAtFrame(p, 156.0f, 0);
            PlayerSe_EquipAtFrame(p, 158.0f, 0);
            PlayerSe_WeaponAtFrame(p, 68, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 4, 1);
            PlayerSe_WeaponAtFrame(p, 34, 4);
            PlayerSe_WeaponAtFrame(p, 48, 7);
            PlayerSe_WeaponAtFrame(p, 70, 2);
            PlayerSe_WeaponAtFrame(p, 116, 2);
            return;
        case 0x581:
            PlayerSe_VoiceAtFrame(p, 10.0f, 0);
            PlayerSe_VoiceAtFrame(p, 62.0f, 3);
            PlayerSe_VoiceAtFrame(p, 78.0f, 2);
            PlayerSe_EquipAtFrame(p, 80.0f, 0);
            PlayerSe_VoiceAtFrame(p, 102.0f, 0);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_WeaponAtFrame(p, 40, Sound_RandomSe(38, 3, 38, 3));
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 40, 6);
            PlayerSe_WeaponAtFrame(p, 52, 7);
            PlayerSe_WeaponAtFrame(p, 70, 4);
            PlayerSe_WeaponAtFrame(p, 132, 2);
            return;
        case 0x582:
            PlayerSe_VoiceAtFrame(p, 10.0f, 0);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 46.0f, 2);
            PlayerSe_VoiceAtFrame(p, 70.0f, 2);
            PlayerSe_EquipAtFrame(p, 72.0f, 0);
            PlayerSe_VoiceAtFrame(p, 86.0f, 2);
            PlayerSe_VoiceAtFrame(p, 102.0f, 0);
            PlayerSe_EquipAtFrame(p, 102.0f, 0);
            PlayerSe_WeaponAtFrame(p, 40, Sound_RandomSe(38, 2, 38, 2));
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 32, 4);
            PlayerSe_WeaponAtFrame(p, 50, 7);
            PlayerSe_WeaponAtFrame(p, 112, 2);
            return;
        case 0x583:
            PlayerSe_VoiceAtFrame(p, 48.0f, 3);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            PlayerSe_VoiceAtFrame(p, 74.0f, 2);
            PlayerSe_EquipAtFrame(p, 80.0f, 0);
            PlayerSe_VoiceAtFrame(p, 86.0f, 2);
            PlayerSe_VoiceAtFrame(p, 100.0f, 2);
            PlayerSe_EquipAtFrame(p, 108.0f, 0);
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(38, 2, 38, 2));
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 40, 6);
            PlayerSe_WeaponAtFrame(p, 50, 7);
            PlayerSe_WeaponAtFrame(p, 90, 5);
            PlayerSe_WeaponAtFrame(p, 120, 2);
            return;
        case 0x584:
            PlayerSe_VoiceAtFrame(p, 20.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 64.0f, 3);
            PlayerSe_EquipAtFrame(p, 64.0f, 0);
            PlayerSe_VoiceAtFrame(p, 98.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 5);
            PlayerSe_WeaponAtFrame(p, 30, 4);
            PlayerSe_WeaponAtFrame(p, 68, 2);
            return;
        case 0x585:
            PlayerSe_VoiceAtFrame(p, 54.0f, 3);
            PlayerSe_EquipAtFrame(p, 54.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 5);
            PlayerSe_WeaponAtFrame(p, 30, 4);
            PlayerSe_WeaponAtFrame(p, 60, 2);
            return;
        case 0x586:
            PlayerSe_WeaponAtFrame(p, 2, 5);
            return;
        case 0x587:
            PlayerSe_WeaponAtFrame(p, 2, 5);
            return;
        case 0x588:
            break;
        case 0x589:
            PlayerSe_VoiceAtFrame(p, 10.0f, 0);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_WeaponAtFrame(p, 4, 1);
            return;
        case 0x58A:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            return;
        case 0x58B:
            PlayerSe_VoiceAtFrame(p, 16.0f, 0);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 1);
            return;
        case 0x58C:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            return;
        case 0x58D:
            PlayerSe_WeaponAtFrame(p, 4, 1);
            return;
        case 0x58E:
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 1);
            return;
        case 0x58F:
            PlayerSe_VoiceAtFrame(p, 18.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 1);
            return;
        case 0x590:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            return;
        case 0x593:
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            PlayerSe_WeaponAtFrame(p, 34, 2);
            return;
        case 0x594:
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            PlayerSe_WeaponAtFrame(p, 34, 2);
            return;
        case 0x595:
            PlayerSe_VoiceAtFrame(p, 20.0f, 2);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            PlayerSe_VoiceAtFrame(p, 34.0f, 2);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_WeaponAtFrame(p, 34, 2);
            PlayerSe_WeaponAtFrame(p, 34, 8);
            return;
        case 0x596:
            PlayerSe_WeaponAtFrame(p, 4, 5);
            return;
        case 0x597:
            PlayerSe_VoiceAtFrame(p, 20.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_WeaponAtFrame(p, 12, 1);
            PlayerSe_WeaponAtFrame(p, 30, 4);
            return;
        case 0x598:
            break;
        case 0x57D:
            Player_SeAtFrame(p, 12, 24);
            PlayerSe_VoiceAtFrame(p, 2.0f, 1);
            PlayerSe_VoiceAtFrame(p, 20.0f, 1);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 1);
            PlayerSe_VoiceAtFrame(p, 54.0f, 1);
            PlayerSe_VoiceAtFrame(p, 74.0f, 1);
            PlayerSe_VoiceAtFrame(p, 92.0f, 1);
            return;
        case 0x57E:
            Player_SeAtFrame(p, 12, 25);
            PlayerSe_VoiceAtFrame(p, 2.0f, 1);
            PlayerSe_VoiceAtFrame(p, 10.0f, 1);
            PlayerSe_VoiceAtFrame(p, 28.0f, 1);
            PlayerSe_VoiceAtFrame(p, 44.0f, 1);
            PlayerSe_EquipAtFrame(p, 44.0f, 0);
            PlayerSe_VoiceAtFrame(p, 62.0f, 1);
            PlayerSe_VoiceAtFrame(p, 76.0f, 1);
            PlayerSe_VoiceAtFrame(p, 92.0f, 1);
            return;
        case 0x57F:
            Player_SeAtFrame(p, 12, 26);
            PlayerSe_VoiceAtFrame(p, 22.0f, 1);
            PlayerSe_VoiceAtFrame(p, 70.0f, 1);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            return;
            return;
        }
        break;
    case 5:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_VoiceAtFrame(p, 24.0f, 0);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 2, 6);
            PlayerSe_VoiceAtFrame(p, 60.0f, 0);
            PlayerSe_EquipAtFrame(p, 60.0f, 0);
            return;
        case 0x3EC:
            PlayerSe_WeaponAtFrame(p, 56, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 24.0f, 0);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            return;
        case 0x57D:
        case 0x57E:
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            return;
        case 0x3F1:
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 16.0f, 0);
            PlayerSe_VoiceAtFrame(p, 40.0f, 0);
            PlayerSe_VoiceAtFrame(p, 54.0f, 3);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_EquipAtFrame(p, 54.0f, 0);
            PlayerSe_WeaponAtFrame(p, 10, 6);
            return;
        case 0x3F2:
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 18.0f, 0);
            PlayerSe_VoiceAtFrame(p, 40.0f, 0);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            PlayerSe_WeaponAtFrame(p, 10, 1);
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 92.0f, 0);
            PlayerSe_VoiceAtFrame(p, 106.0f, 0);
            PlayerSe_EquipAtFrame(p, 78.0f, 0);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 44, 13);
            PlayerSe_WeaponAtFrame(p, 66, 14);
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_WeaponAtFrame(p, 104, 4);
            return;
        case 0x580:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 44.0f, 0);
            PlayerSe_EquipAtFrame(p, 64.0f, 0);
            PlayerSe_VoiceAtFrame(p, 64.0f, 0);
            PlayerSe_VoiceAtFrame(p, 130.0f, 0);
            PlayerSe_EquipAtFrame(p, 130.0f, 0);
            return;
        case 0x581:
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 108.0f, 0);
            PlayerSe_VoiceAtFrame(p, 136.0f, 0);
            PlayerSe_EquipAtFrame(p, 136.0f, 0);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(p, 2, 9);
            PlayerSe_WeaponAtFrame(p, 180, 7);
            return;
        case 0x584:
            PlayerSe_WeaponAtFrame(p, 12, 8);
            return;
        case 0x585:
            PlayerSe_WeaponAtFrame(p, 24, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 70, 7);
            PlayerSe_WeaponAtFrame(p, 20, 1);
            PlayerSe_AtFrame(p, 8, 61);
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_VoiceAtFrame(p, 72.0f, 0);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            return;
        case 0x586:
        case 0x587:
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 128.0f, 0);
            PlayerSe_WeaponAtFrame(p, 4, 3);
            PlayerSe_WeaponAtFrame(p, 100, 4);
            return;
        case 0x588:
        case 0x589:
            PlayerSe_WeaponAtFrame(p, 12, 8);
            return;
        case 0x58E:
            PlayerSe_WeaponAtFrame(p, 4, 1);
            PlayerSe_VoiceAtFrame(p, 40.0f, 0);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_WeaponAtFrame(p, 24, 3);
            return;
        case 0x58F:
            PlayerSe_VoiceAtFrame(p, 14.0f, 2);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 0);
            PlayerSe_VoiceAtFrame(p, 52.0f, 0);
            PlayerSe_EquipAtFrame(p, 52.0f, 0);
            PlayerSe_WeaponAtFrame(p, 4, 1);
            PlayerSe_WeaponAtFrame(p, 38, 3);
            return;
        }
        break;
    case 10:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_EquipAtFrame(p, 4.0f, 4);
            PlayerSe_WeaponAtFrame(p, 16, 1);
            PlayerSe_WeaponAtFrame(p, 32, 2);
            PlayerSe_VoiceAtFrame(p, 24.0f, 0);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_EquipAtFrame(p, 4.0f, 4);
            PlayerSe_WeaponAtFrame(p, 24, 3);
            PlayerSe_WeaponAtFrame(p, 36, 4);
            PlayerSe_VoiceAtFrame(p, 42.0f, 1);
            PlayerSe_VoiceAtFrame(p, 66.0f, 1);
            PlayerSe_EquipAtFrame(p, 66.0f, 0);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 6.0f, 0);
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            return;
        case 0x3ED:
            PlayerSe_WeaponAtFrame(p, 8, 3);
            PlayerSe_WeaponAtFrame(p, 40, 4);
            PlayerSe_VoiceAtFrame(p, 50.0f, 1);
            PlayerSe_EquipAtFrame(p, 52.0f, 0);
            PlayerSe_VoiceAtFrame(p, 86.0f, 1);
            PlayerSe_EquipAtFrame(p, 88.0f, 0);
            return;
        case 0x3EE:
            PlayerSe_AtFrame(p, 4, 67);
            PlayerSe_VoiceAtFrame(p, 16.0f, 3);
            PlayerSe_EquipAtFrame(p, 14.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_VoiceAtFrame(p, 42.0f, 0);
            PlayerSe_EquipAtFrame(p, 44.0f, 0);
            return;
        case 0x3F1:
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 6.0f, 0);
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_WeaponAtFrame(p, 2, 11);
            PlayerSe_WeaponAtFrame(p, 12, 11);
            PlayerSe_WeaponAtFrame(p, 32, 11);
            PlayerSe_WeaponAtFrame(p, 42, 11);
            return;
        case 0x3F2:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 32, 2);
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 6.0f, 0);
            PlayerSe_VoiceAtFrame(p, 16.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            return;
        case 0x3F3:
            PlayerSe_AtFrame(p, 4, 67);
            PlayerSe_VoiceAtFrame(p, 8.0f, 3);
            PlayerSe_EquipAtFrame(p, 10.0f, 0);
            return;
        case 0x3F4:
            PlayerSe_WeaponAtFrame(p, 16, 1);
            PlayerSe_VoiceAtFrame(p, 14.0f, 0);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            return;
        case 0x3F5:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_VoiceAtFrame(p, 12.0f, 0);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            return;
        case 0x3F6:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 8, 2);
            PlayerSe_VoiceAtFrame(p, 6.0f, 0);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_AtFrame(p, 26, 62);
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 2, 36, 2));
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            break;
        case 0x57C:
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            PlayerSe_VoiceAtFrame(p, 194.0f, 0);
            PlayerSe_EquipAtFrame(p, 194.0f, 0);
            PlayerSe_WeaponAtFrame(p, 46, 5);
            PlayerSe_AtFrame(p, 72, 74);
            PlayerSe_WeaponAtFrame(p, 100, 6);
            PlayerSe_WeaponAtFrame(p, 130, 4);
            return;
        case 0x57D:
        case 0x57E:
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 20, 7);
            PlayerSe_WeaponAtFrame(p, 40, 8);
            PlayerSe_WeaponAtFrame(p, 70, 9);
            PlayerSe_WeaponAtFrame(p, 100, 10);
            PlayerSe_WeaponAtFrame(p, 142, 11);
            return;
        case 0x580:
        case 0x581:
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 2, 17);
            PlayerSe_EquipAtFrame(p, 38.0f, 4);
            return;
        case 0x583:
            PlayerSe_VoiceAtFrame(p, 30.0f, 2);
            PlayerSe_EquipAtFrame(p, 32.0f, 0);
            PlayerSe_VoiceAtFrame(p, 42.0f, 2);
            PlayerSe_EquipAtFrame(p, 44.0f, 0);
            return;
        case 0x585:
            PlayerSe_AtFrame(p, 22, 62);
            PlayerSe_WeaponAtFrame(p, 24, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 28.0f, 3);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 92.0f, 0);
            return;
        case 0x586:
            PlayerSe_EquipAtFrame(p, 8.0f, 4);
            PlayerSe_AtFrame(p, 20, 63);
            PlayerSe_WeaponAtFrame(p, 24, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 32.0f, 3);
            PlayerSe_EquipAtFrame(p, 34.0f, 5);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_VoiceAtFrame(p, 96.0f, 0);
            PlayerSe_EquipAtFrame(p, 98.0f, 0);
            PlayerSe_WeaponAtFrame(p, 96, 7);
            return;
        case 0x587:
        case 0x588:
        case 0x589:
            PlayerSe_WeaponAtFrame(p, 2, 15);
            PlayerSe_VoiceAtFrame(p, 18.0f, 2);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            return;
        case 0x58A:
        case 0x58B:
        case 0x58C:
            PlayerSe_WeaponAtFrame(p, 2, 7);
            PlayerSe_WeaponAtFrame(p, 14, 8);
            return;
        }
        break;
    case 3:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_EquipAtFrame(p, 18.0f, 4);
            PlayerSe_VoiceAtFrame(p, 42.0f, 3);
            PlayerSe_EquipAtFrame(p, 42.0f, 0);
            PlayerSe_WeaponAtFrame(p, 56, 1);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 22, 4);
            PlayerSe_WeaponAtFrame(p, 32, 3);
            PlayerSe_WeaponAtFrame(p, 58, 2);
            PlayerSe_EquipAtFrame(p, 54.0f, 4);
            PlayerSe_VoiceAtFrame(p, 72.0f, 3);
            PlayerSe_EquipAtFrame(p, 72.0f, 0);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 4);
            PlayerSe_VoiceAtFrame(p, 56.0f, 3);
            PlayerSe_EquipAtFrame(p, 56.0f, 0);
            return;
        case 0x3ED:
        case 0x3EF:
            PlayerSe_WeaponAtFrame(p, 7, 5);
            return;
        case 0x3F1:
            PlayerSe_WeaponAtFrame(p, 22, 4);
            PlayerSe_WeaponAtFrame(p, 32, 3);
            PlayerSe_WeaponAtFrame(p, 58, 2);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 62.0f, 3);
            PlayerSe_EquipAtFrame(p, 62.0f, 0);
            PlayerSe_VoiceAtFrame(p, 94.0f, 3);
            PlayerSe_EquipAtFrame(p, 94.0f, 0);
            return;
        case 0x3F2:
            PlayerSe_WeaponAtFrame(p, 28, 37);
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_WeaponAtFrame(p, 28, 1);
            PlayerSe_WeaponAtFrame(p, 40, 6);
            PlayerSe_VoiceAtFrame(p, 18.0f, 3);
            PlayerSe_EquipAtFrame(p, 18.0f, 4);
            PlayerSe_VoiceAtFrame(p, 38.0f, 3);
            PlayerSe_EquipAtFrame(p, 38.0f, 0);
            return;
        case 0x3F3:
            PlayerSe_AtFrame(p, 12, 69);
            PlayerSe_VoiceAtFrame(p, 4.0f, 0);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 1);
            PlayerSe_EquipAtFrame(p, 32.0f, 0);
            return;
        case 0x3F6:
        case 0x3F7:
            PlayerSe_VoiceAtFrame(p, 4.0f, 3);
            PlayerSe_AtFrame(p, 12, 67);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 0);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            return;
        case 0x3F8:
            PlayerSe_VoiceAtFrame(p, 4.0f, 3);
            PlayerSe_AtFrame(p, 12, 67);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 0);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 6, 3);
            PlayerSe_WeaponAtFrame(p, 18, 1);
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 4);
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 12, 5);
            PlayerSe_WeaponAtFrame(p, 52, 7);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(p, 4, 7);
            PlayerSe_WeaponAtFrame(p, 22, 7);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_VoiceAtFrame(p, 16.0f, 2);
            PlayerSe_EquipAtFrame(p, 32.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 2);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 66, 5);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 4.0f, 3);
            PlayerSe_EquipAtFrame(p, 8.0f, 0);
            PlayerSe_VoiceAtFrame(p, 8.0f, 2);
            PlayerSe_EquipAtFrame(p, 64.0f, 0);
            PlayerSe_VoiceAtFrame(p, 64.0f, 1);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 70, 5);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            PlayerSe_VoiceAtFrame(p, 20.0f, 3);
            PlayerSe_EquipAtFrame(p, 50.0f, 0);
            PlayerSe_VoiceAtFrame(p, 42.0f, 0);
            PlayerSe_EquipAtFrame(p, 74.0f, 0);
            PlayerSe_VoiceAtFrame(p, 74.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 2, 36, 3));
            PlayerSe_WeaponAtFrame(p, 18, 6);
            PlayerSe_WeaponAtFrame(p, 92, 5);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 98.0f, 4);
            PlayerSe_VoiceAtFrame(p, 60.0f, 0);
            PlayerSe_VoiceAtFrame(p, 100.0f, 0);
            return;
        case 0x57E:
            PlayerSe_WeaponAtFrame(p, 18, Sound_RandomSe(35, 3, 36, 2));
            PlayerSe_WeaponAtFrame(p, 18, 6);
            PlayerSe_WeaponAtFrame(p, 92, 5);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_VoiceAtFrame(p, 16.0f, 3);
            PlayerSe_EquipAtFrame(p, 102.0f, 4);
            PlayerSe_VoiceAtFrame(p, 58.0f, 0);
            PlayerSe_VoiceAtFrame(p, 100.0f, 1);
            return;
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 28, 5);
            PlayerSe_AtFrame(p, 80, 69);
            PlayerSe_AtFrame(p, 80, 74);
            PlayerSe_VoiceAtFrame(p, 26.0f, 0);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            PlayerSe_VoiceAtFrame(p, 74.0f, 1);
            PlayerSe_VoiceAtFrame(p, 98.0f, 1);
            PlayerSe_EquipAtFrame(p, 98.0f, 0);
            return;
        case 0x581:
            PlayerSe_WeaponAtFrame(p, 52, 5);
            PlayerSe_AtFrame(p, 6, 69);
            PlayerSe_AtFrame(p, 6, 74);
            PlayerSe_VoiceAtFrame(p, 26.0f, 0);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            PlayerSe_VoiceAtFrame(p, 50.0f, 0);
            PlayerSe_EquipAtFrame(p, 50.0f, 0);
            PlayerSe_VoiceAtFrame(p, 66.0f, 1);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(36, 2, 37, 3));
            PlayerSe_WeaponAtFrame(p, 6, 6);
            PlayerSe_VoiceAtFrame(p, 8.0f, 3);
            PlayerSe_AtFrame(p, 10, 68);
            PlayerSe_EquipAtFrame(p, 10.0f, 0);
            return;
        case 0x584:
            PlayerSe_WeaponAtFrame(p, 6, 37);
            PlayerSe_WeaponAtFrame(p, 12, 6);
            PlayerSe_WeaponAtFrame(p, 116, 5);
            PlayerSe_AtFrame(p, 14, 68);
            PlayerSe_VoiceAtFrame(p, 14.0f, 3);
            PlayerSe_EquipAtFrame(p, 14.0f, 0);
            PlayerSe_VoiceAtFrame(p, 84.0f, 0);
            PlayerSe_EquipAtFrame(p, 84.0f, 0);
            PlayerSe_VoiceAtFrame(p, 118.0f, 0);
            PlayerSe_EquipAtFrame(p, 118.0f, 0);
            return;
        case 0x587:
            PlayerSe_VoiceAtFrame(p, 8.0f, 3);
            PlayerSe_EquipAtFrame(p, 10.0f, 0);
            PlayerSe_AtFrame(p, 26, 68);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 10, 6);
            return;
        case 0x589:
            PlayerSe_WeaponAtFrame(p, 88, 5);
            PlayerSe_VoiceAtFrame(p, 28.0f, 3);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 48.0f, 3);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            PlayerSe_VoiceAtFrame(p, 92.0f, 1);
            return;
        }
        break;
    case 9:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_EquipAtFrame(p, 18.0f, 4);
            PlayerSe_VoiceAtFrame(p, 42.0f, 3);
            PlayerSe_EquipAtFrame(p, 42.0f, 0);
            PlayerSe_WeaponAtFrame(p, 56, 1);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 22, 4);
            PlayerSe_WeaponAtFrame(p, 32, 3);
            PlayerSe_WeaponAtFrame(p, 58, 2);
            PlayerSe_EquipAtFrame(p, 54.0f, 4);
            PlayerSe_VoiceAtFrame(p, 72.0f, 0);
            PlayerSe_EquipAtFrame(p, 72.0f, 0);
            PlayerSe_VoiceAtFrame(p, 84.0f, 1);
            PlayerSe_EquipAtFrame(p, 84.0f, 0);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 56.0f, 3);
            PlayerSe_EquipAtFrame(p, 56.0f, 0);
            return;
        case 0x3ED:
            PlayerSe_WeaponAtFrame(p, 8, 5);
            return;
        case 0x3EF:
            PlayerSe_AtFrame(p, 2, 74);
            return;
        case 0x3F1:
            PlayerSe_WeaponAtFrame(p, 22, 4);
            PlayerSe_WeaponAtFrame(p, 32, 3);
            PlayerSe_WeaponAtFrame(p, 58, 2);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 62.0f, 3);
            PlayerSe_EquipAtFrame(p, 62.0f, 0);
            PlayerSe_VoiceAtFrame(p, 94.0f, 3);
            PlayerSe_EquipAtFrame(p, 94.0f, 0);
            return;
        case 0x3F2:
            PlayerSe_WeaponAtFrame(p, 28, 37);
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_WeaponAtFrame(p, 16, 1);
            PlayerSe_WeaponAtFrame(p, 40, 6);
            PlayerSe_VoiceAtFrame(p, 18.0f, 3);
            PlayerSe_EquipAtFrame(p, 18.0f, 4);
            PlayerSe_VoiceAtFrame(p, 38.0f, 3);
            PlayerSe_EquipAtFrame(p, 38.0f, 0);
            return;
        case 0x3F3:
            PlayerSe_AtFrame(p, 12, 69);
            PlayerSe_VoiceAtFrame(p, 4.0f, 0);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 1);
            PlayerSe_EquipAtFrame(p, 32.0f, 0);
            return;
        case 0x3F6:
        case 0x3F7:
            PlayerSe_VoiceAtFrame(p, 4.0f, 3);
            PlayerSe_AtFrame(p, 12, 67);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 0);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            return;
        case 0x3F8:
            PlayerSe_VoiceAtFrame(p, 4.0f, 3);
            PlayerSe_AtFrame(p, 12, 67);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 0);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(p, 2, 2);
            PlayerSe_WeaponAtFrame(p, 6, 3);
            PlayerSe_WeaponAtFrame(p, 18, 1);
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 4);
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 8, 13);
            PlayerSe_WeaponAtFrame(p, 72, 5);
            PlayerSe_VoiceAtFrame(p, 72.0f, 2);
            PlayerSe_EquipAtFrame(p, 74.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 8, 13);
            PlayerSe_WeaponAtFrame(p, 72, 5);
            PlayerSe_EquipAtFrame(p, 74.0f, 0);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 8, 13);
            PlayerSe_WeaponAtFrame(p, 78, 5);
            PlayerSe_VoiceAtFrame(p, 72.0f, 2);
            PlayerSe_EquipAtFrame(p, 74.0f, 0);
            return;
        case 0x57C:
            PlayerSe_EquipAtFrame(p, 12.0f, 4);
            PlayerSe_WeaponAtFrame(p, 8, 2);
            PlayerSe_WeaponAtFrame(p, 34, 4);
            PlayerSe_WeaponAtFrame(p, 52, 7);
            PlayerSe_WeaponAtFrame(p, 88, 1);
            PlayerSe_EquipAtFrame(p, 52.0f, 0);
            PlayerSe_VoiceAtFrame(p, 82.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 2, 36, 3));
            PlayerSe_WeaponAtFrame(p, 18, 6);
            PlayerSe_WeaponAtFrame(p, 92, 5);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 98.0f, 4);
            PlayerSe_VoiceAtFrame(p, 98.0f, 0);
            return;
        case 0x57E:
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(36, 3, 37, 3));
            PlayerSe_WeaponAtFrame(p, 12, 2);
            PlayerSe_WeaponAtFrame(p, 28, 3);
            PlayerSe_WeaponAtFrame(p, 38, 6);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_WeaponAtFrame(p, 80, 5);
            PlayerSe_VoiceAtFrame(p, 84.0f, 1);
            PlayerSe_EquipAtFrame(p, 86.0f, 0);
            return;
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 4, 2);
            PlayerSe_WeaponAtFrame(p, 28, 5);
            PlayerSe_EquipAtFrame(p, 14.0f, 0);
            PlayerSe_WeaponAtFrame(p, 42, 28);
            return;
        case 0x580:
            PlayerSe_WeaponAtFrame(p, 4, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 6, 14);
            PlayerSe_VoiceAtFrame(p, 28.0f, 2);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_VoiceAtFrame(p, 48.0f, 0);
            PlayerSe_AtFrame(p, 58, 69);
            PlayerSe_WeaponAtFrame(p, 60, 20);
            PlayerSe_WeaponAtFrame(p, 72, 9);
            PlayerSe_WeaponAtFrame(p, 144, 5);
            PlayerSe_VoiceAtFrame(p, 154.0f, 2);
            PlayerSe_EquipAtFrame(p, 156.0f, 0);
            return;
        case 0x581:
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 2.0f, 2);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 2);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_VoiceAtFrame(p, 38.0f, 3);
            PlayerSe_AtFrame(p, 38, 69);
            PlayerSe_WeaponAtFrame(p, 40, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_VoiceAtFrame(p, 110.0f, 1);
            PlayerSe_WeaponAtFrame(p, 44, 6);
            PlayerSe_WeaponAtFrame(p, 94, 5);
            return;
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 12, 8);
            return;
        case 0x587:
            PlayerSe_VoiceAtFrame(p, 8.0f, 3);
            PlayerSe_EquipAtFrame(p, 10.0f, 0);
            PlayerSe_AtFrame(p, 26, 68);
            PlayerSe_WeaponAtFrame(p, 78, 10);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 10, 6);
            return;
        case 0x589:
            PlayerSe_WeaponAtFrame(p, 88, 5);
            PlayerSe_VoiceAtFrame(p, 28.0f, 3);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 48.0f, 3);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            PlayerSe_VoiceAtFrame(p, 92.0f, 1);
            return;
        case 0x592:
            PlayerSe_WeaponAtFrame(p, 4, 5);
            return;
        }
        break;
    case 4:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_VoiceAtFrame(p, 12.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 28, 2);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_VoiceAtFrame(p, 40.0f, 1);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 40.0f, 0);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(p, 18.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            return;
        case 0x3ED:
        case 0x3EF:
            PlayerSe_AtFrame(p, 2, 74);
            PlayerSe_VoiceAtFrame(p, 10.0f, 1);
            return;
        case 0x3F1:
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            PlayerSe_VoiceAtFrame(p, 46.0f, 0);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            PlayerSe_WeaponAtFrame(p, 4, 3);
            PlayerSe_WeaponAtFrame(p, 20, 2);
            return;
        case 0x3F2:
            PlayerSe_WeaponAtFrame(p, 2, 1);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_AtFrame(p, 2, 63);
            PlayerSe_EquipAtFrame(p, 20.0f, 3);
            PlayerSe_AtFrame(p, 20, 65);
            return;
        case 0x3FA:
            PlayerSe_VoiceAtFrame(p, 12.0f, 3);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_WeaponAtFrame(p, 4, 1);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(p, 16, Sound_RandomSe(35, 2, 36, 1));
            PlayerSe_WeaponAtFrame(p, 14, 4);
            PlayerSe_VoiceAtFrame(p, 12.0f, 3);
            PlayerSe_EquipAtFrame(p, 12.0f, 4);
            PlayerSe_VoiceAtFrame(p, 58.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 2, 3);
            PlayerSe_WeaponAtFrame(p, 4, 4);
            PlayerSe_VoiceAtFrame(p, 14.0f, 1);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 18, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(p, 54, Sound_RandomSe(37, 2, 37, 2));
            PlayerSe_WeaponAtFrame(p, 4, 3);
            PlayerSe_WeaponAtFrame(p, 18, 6);
            PlayerSe_WeaponAtFrame(p, 40, 5);
            PlayerSe_VoiceAtFrame(p, 8.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 54.0f, 3);
            PlayerSe_EquipAtFrame(p, 60.0f, 4);
            PlayerSe_EquipAtFrame(p, 54.0f, 0);
            PlayerSe_VoiceAtFrame(p, 108.0f, 0);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 8, 6);
            PlayerSe_EquipAtFrame(p, 60.0f, 4);
            PlayerSe_WeaponAtFrame(p, 24, 5);
            PlayerSe_VoiceAtFrame(p, 6.0f, 3);
            PlayerSe_VoiceAtFrame(p, 10.0f, 3);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 28.0f, 3);
            PlayerSe_VoiceAtFrame(p, 32.0f, 3);
            PlayerSe_VoiceAtFrame(p, 76.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(p, 28, Sound_RandomSe(37, 4, 36, 2));
            PlayerSe_WeaponAtFrame(p, 4, 6);
            PlayerSe_WeaponAtFrame(p, 14, 5);
            PlayerSe_VoiceAtFrame(p, 8.0f, 0);
            PlayerSe_VoiceAtFrame(p, 28.0f, 3);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            PlayerSe_VoiceAtFrame(p, 84.0f, 0);
            PlayerSe_EquipAtFrame(p, 42.0f, 4);
            return;
        case 0x57E:
            PlayerSe_VoiceAtFrame(p, 4.0f, 3);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 68.0f, 0);
            PlayerSe_WeaponAtFrame(p, 16, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 10, 5);
            return;
        case 0x580:
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 36.0f, 0);
            PlayerSe_EquipAtFrame(p, 36.0f, 0);
            return;
        case 0x585:
            PlayerSe_VoiceAtFrame(p, 16.0f, 0);
            PlayerSe_VoiceAtFrame(p, 32.0f, 3);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_EquipAtFrame(p, 32.0f, 0);
            return;
        case 0x587:
            PlayerSe_VoiceAtFrame(p, 30.0f, 3);
            PlayerSe_VoiceAtFrame(p, 40.0f, 0);
            PlayerSe_VoiceAtFrame(p, 124.0f, 0);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_AtFrame(p, 46, 64);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(p, 20, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(p, 14, 4);
            PlayerSe_VoiceAtFrame(p, 10.0f, 3);
            PlayerSe_EquipAtFrame(p, 10.0f, 0);
            return;
        case 0x589:
            PlayerSe_WeaponAtFrame(p, 16, Sound_RandomSe(35, 3, 35, 3));
            PlayerSe_WeaponAtFrame(p, 16, 5);
            return;
        }
        break;
    case 6:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 2, 6);
            PlayerSe_WeaponAtFrame(p, 8, 1);
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 18, 2);
            PlayerSe_WeaponAtFrame(p, 26, 7);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_VoiceAtFrame(p, 40.0f, 1);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 2.0f, 0);
            PlayerSe_EquipAtFrame(p, 6.0f, 0);
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(39, 1, 40, 1));
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            return;
        case 0x3F1:
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_EquipAtFrame(p, 2.0f, 0);
            PlayerSe_VoiceAtFrame(p, 22.0f, 3);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            PlayerSe_VoiceAtFrame(p, 46.0f, 0);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            PlayerSe_WeaponAtFrame(p, 4, 3);
            PlayerSe_WeaponAtFrame(p, 20, 2);
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_WeaponAtFrame(p, 28, 7);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(p, 2, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 2.0f, 3);
            PlayerSe_AtFrame(p, 2, 63);
            PlayerSe_EquipAtFrame(p, 20.0f, 3);
            PlayerSe_AtFrame(p, 20, 65);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(p, 18, 1);
            PlayerSe_WeaponAtFrame(p, 22, 6);
            PlayerSe_WeaponAtFrame(p, 34, 12);
            PlayerSe_AtFrame(p, 34, 54);
            PlayerSe_AtFrame(p, 32, 63);
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            PlayerSe_VoiceAtFrame(p, 72.0f, 0);
            return;
        case 0x3FB:
            PlayerSe_WeaponAtFrame(p, 8, 5);
            PlayerSe_WeaponAtFrame(p, 8, 9);
            PlayerSe_AtFrame(p, 10, 63);
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 46.0f, 0);
            PlayerSe_VoiceAtFrame(p, 46.0f, 0);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(p, 16, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 2, 3);
            PlayerSe_WeaponAtFrame(p, 14, 8);
            PlayerSe_VoiceAtFrame(p, 12.0f, 3);
            PlayerSe_EquipAtFrame(p, 12.0f, 4);
            PlayerSe_VoiceAtFrame(p, 58.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(p, 14, 8);
            PlayerSe_WeaponAtFrame(p, 22, 4);
            PlayerSe_VoiceAtFrame(p, 36.0f, 3);
            PlayerSe_VoiceAtFrame(p, 54.0f, 3);
            PlayerSe_VoiceAtFrame(p, 64.0f, 0);
            PlayerSe_AtFrame(p, 30, 68);
            PlayerSe_EquipAtFrame(p, 36.0f, 0);
            PlayerSe_EquipAtFrame(p, 24.0f, 0);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 18, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 54, 37);
            PlayerSe_WeaponAtFrame(p, 4, 3);
            PlayerSe_WeaponAtFrame(p, 16, 6);
            PlayerSe_WeaponAtFrame(p, 44, 5);
            PlayerSe_WeaponAtFrame(p, 64, 9);
            PlayerSe_VoiceAtFrame(p, 60.0f, 3);
            PlayerSe_VoiceAtFrame(p, 124.0f, 0);
            PlayerSe_EquipAtFrame(p, 60.0f, 0);
            PlayerSe_EquipAtFrame(p, 88.0f, 4);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 6, 1);
            PlayerSe_WeaponAtFrame(p, 10, 6);
            PlayerSe_WeaponAtFrame(p, 30, 9);
            PlayerSe_WeaponAtFrame(p, 30, 4);
            PlayerSe_VoiceAtFrame(p, 24.0f, 3);
            PlayerSe_VoiceAtFrame(p, 42.0f, 3);
            PlayerSe_VoiceAtFrame(p, 98.0f, 1);
            PlayerSe_VoiceAtFrame(p, 106.0f, 0);
            PlayerSe_EquipAtFrame(p, 64.0f, 0);
            PlayerSe_EquipAtFrame(p, 98.0f, 0);
            return;
        case 0x57E:
            PlayerSe_VoiceAtFrame(p, 4.0f, 3);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 20.0f, 3);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            PlayerSe_VoiceAtFrame(p, 68.0f, 0);
            PlayerSe_WeaponAtFrame(p, 16, 36);
            PlayerSe_WeaponAtFrame(p, 10, 5);
            return;
        case 0x580:
            PlayerSe_VoiceAtFrame(p, 22.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 0);
            PlayerSe_VoiceAtFrame(p, 36.0f, 0);
            PlayerSe_EquipAtFrame(p, 36.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(p, 28, Sound_RandomSe(37, 4, 36, 2));
            PlayerSe_WeaponAtFrame(p, 6, 3);
            PlayerSe_WeaponAtFrame(p, 18, 5);
            PlayerSe_WeaponAtFrame(p, 30, 9);
            PlayerSe_VoiceAtFrame(p, 12.0f, 2);
            PlayerSe_VoiceAtFrame(p, 36.0f, 2);
            PlayerSe_VoiceAtFrame(p, 56.0f, 2);
            PlayerSe_AtFrame(p, 40, 61);
            PlayerSe_WeaponAtFrame(p, 66, 58);
            PlayerSe_VoiceAtFrame(p, 68.0f, 0);
            PlayerSe_EquipAtFrame(p, 56.0f, 0);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(p, 28, Sound_RandomSe(37, 4, 36, 2));
            PlayerSe_WeaponAtFrame(p, 6, 3);
            PlayerSe_WeaponAtFrame(p, 18, 9);
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 12.0f, 2);
            PlayerSe_VoiceAtFrame(p, 36.0f, 2);
            PlayerSe_VoiceAtFrame(p, 56.0f, 2);
            PlayerSe_AtFrame(p, 40, 61);
            PlayerSe_AtFrame(p, 66, 58);
            PlayerSe_VoiceAtFrame(p, 68.0f, 0);
            PlayerSe_EquipAtFrame(p, 56.0f, 0);
            return;
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(p, 54, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(p, 92, Sound_RandomSe(35, 2, 35, 2));
            PlayerSe_WeaponAtFrame(p, 126, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(p, 8, 8);
            PlayerSe_WeaponAtFrame(p, 26, 4);
            PlayerSe_WeaponAtFrame(p, 48, 4);
            PlayerSe_WeaponAtFrame(p, 54, 8);
            PlayerSe_WeaponAtFrame(p, 66, 8);
            PlayerSe_WeaponAtFrame(p, 72, 4);
            PlayerSe_WeaponAtFrame(p, 88, 8);
            PlayerSe_WeaponAtFrame(p, 100, 4);
            PlayerSe_WeaponAtFrame(p, 118, 9);
            PlayerSe_WeaponAtFrame(p, 114, 5);
            PlayerSe_AtFrame(p, 114, 63);
            PlayerSe_AtFrame(p, 122, 60);
            PlayerSe_EquipAtFrame(p, 38.0f, 0);
            PlayerSe_EquipAtFrame(p, 126.0f, 0);
            return;
        case 0x581:
            PlayerSe_WeaponAtFrame(p, 28, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(p, 24, 5);
            PlayerSe_WeaponAtFrame(p, 36, 9);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_EquipAtFrame(p, 72.0f, 0);
            PlayerSe_VoiceAtFrame(p, 12.0f, 2);
            PlayerSe_VoiceAtFrame(p, 30.0f, 2);
            PlayerSe_VoiceAtFrame(p, 60.0f, 2);
            PlayerSe_AtFrame(p, 44, 63);
            PlayerSe_AtFrame(p, 16, 58);
            PlayerSe_AtFrame(p, 70, 68);
            PlayerSe_VoiceAtFrame(p, 86.0f, 0);
            return;
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 28, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(p, 24, 9);
            PlayerSe_WeaponAtFrame(p, 36, 5);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_EquipAtFrame(p, 72.0f, 0);
            PlayerSe_VoiceAtFrame(p, 12.0f, 2);
            PlayerSe_VoiceAtFrame(p, 30.0f, 2);
            PlayerSe_VoiceAtFrame(p, 60.0f, 2);
            PlayerSe_AtFrame(p, 44, 63);
            PlayerSe_AtFrame(p, 16, 58);
            PlayerSe_AtFrame(p, 70, 68);
            PlayerSe_VoiceAtFrame(p, 86.0f, 0);
            return;
        case 0x584:
            PlayerSe_VoiceAtFrame(p, 16.0f, 2);
            PlayerSe_AtFrame(p, 18, 68);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            return;
        case 0x585:
            PlayerSe_WeaponAtFrame(p, 14, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_WeaponAtFrame(p, 2, 6);
            PlayerSe_WeaponAtFrame(p, 8, 1);
            PlayerSe_VoiceAtFrame(p, 24.0f, 0);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            return;
        }
        break;
    case 7:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 20, 6);
            PlayerSe_WeaponAtFrame(p, 20, 8);
            PlayerSe_VoiceAtFrame(p, 20.0f, 0);
            PlayerSe_WeaponAtFrame(p, 38, 1);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_VoiceAtFrame(p, 54.0f, 0);
            PlayerSe_VoiceAtFrame(p, 66.0f, 1);
            PlayerSe_EquipAtFrame(p, 68.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 12, 8);
            PlayerSe_WeaponAtFrame(p, 38, 3);
            PlayerSe_VoiceAtFrame(p, 44.0f, 0);
            PlayerSe_EquipAtFrame(p, 68.0f, 0);
            PlayerSe_VoiceAtFrame(p, 64.0f, 1);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 2.0f, 2);
            PlayerSe_EquipAtFrame(p, 4.0f, 0);
            PlayerSe_VoiceAtFrame(p, 24.0f, 2);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            return;
        case 0x3ED:
            PlayerSe_WeaponAtFrame(p, 8, 8);
            PlayerSe_WeaponAtFrame(p, 38, 3);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_VoiceAtFrame(p, 60.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_EquipAtFrame(p, 62.0f, 0);
            return;
        case 0x3EE:
        case 0x3EF:
            PlayerSe_WeaponAtFrame(p, 12, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_AtFrame(p, 20, 65);
            PlayerSe_EquipAtFrame(p, 40.0f, 3);
            PlayerSe_WeaponAtFrame(p, 60, 6);
            PlayerSe_VoiceAtFrame(p, 40.0f, 3);
            PlayerSe_VoiceAtFrame(p, 66.0f, 0);
            PlayerSe_VoiceAtFrame(p, 126.0f, 0);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_EquipAtFrame(p, 66.0f, 0);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(p, 12, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_AtFrame(p, 20, 65);
            PlayerSe_EquipAtFrame(p, 40.0f, 3);
            PlayerSe_WeaponAtFrame(p, 58, 6);
            PlayerSe_VoiceAtFrame(p, 40.0f, 3);
            PlayerSe_VoiceAtFrame(p, 66.0f, 0);
            PlayerSe_VoiceAtFrame(p, 126.0f, 0);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_EquipAtFrame(p, 66.0f, 0);
            return;
        case 0x3F9:
            PlayerSe_WeaponAtFrame(p, 8, 8);
            PlayerSe_WeaponAtFrame(p, 38, 3);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_VoiceAtFrame(p, 60.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_EquipAtFrame(p, 62.0f, 0);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(p, 6, 8);
            PlayerSe_WeaponAtFrame(p, 34, 4);
            PlayerSe_WeaponAtFrame(p, 50, Sound_RandomSe(35, 4, 36, 2));
            PlayerSe_WeaponAtFrame(p, 66, 7);
            PlayerSe_WeaponAtFrame(p, 132, 6);
            PlayerSe_VoiceAtFrame(p, 24.0f, 2);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            return;
        case 0x57A:
            PlayerSe_WeaponAtFrame(p, 12, 8);
            PlayerSe_WeaponAtFrame(p, 38, 5);
            PlayerSe_WeaponAtFrame(p, 148, 6);
            PlayerSe_WeaponAtFrame(p, 50, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 60, 7);
            PlayerSe_VoiceAtFrame(p, 48.0f, 3);
            PlayerSe_EquipAtFrame(p, 50.0f, 0);
            return;
        case 0x57B:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 18, 4);
            PlayerSe_WeaponAtFrame(p, 26, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_VoiceAtFrame(p, 38.0f, 3);
            PlayerSe_EquipAtFrame(p, 46.0f, 0);
            PlayerSe_VoiceAtFrame(p, 64.0f, 2);
            PlayerSe_VoiceAtFrame(p, 76.0f, 0);
            PlayerSe_EquipAtFrame(p, 78.0f, 0);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 10, 4);
            PlayerSe_WeaponAtFrame(p, 40, 2);
            PlayerSe_WeaponAtFrame(p, 46, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 128, 6);
            PlayerSe_VoiceAtFrame(p, 52.0f, 3);
            PlayerSe_EquipAtFrame(p, 54.0f, 0);
            PlayerSe_VoiceAtFrame(p, 112.0f, 0);
            PlayerSe_EquipAtFrame(p, 130.0f, 0);
            PlayerSe_VoiceAtFrame(p, 132.0f, 1);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(p, 2, 6);
            PlayerSe_WeaponAtFrame(p, 16, 2);
            PlayerSe_WeaponAtFrame(p, 102, 6);
            PlayerSe_WeaponAtFrame(p, 20, Sound_RandomSe(38, 4, 38, 4));
            PlayerSe_WeaponAtFrame(p, 40, 6);
            PlayerSe_EquipAtFrame(p, 42.0f, 4);
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_EquipAtFrame(p, 118.0f, 0);
            PlayerSe_VoiceAtFrame(p, 116.0f, 1);
            return;
        case 0x57E:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 8, 4);
            PlayerSe_WeaponAtFrame(p, 28, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 114, 6);
            PlayerSe_VoiceAtFrame(p, 40.0f, 2);
            PlayerSe_VoiceAtFrame(p, 50.0f, 0);
            PlayerSe_EquipAtFrame(p, 52.0f, 0);
            return;
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 4, 6);
            PlayerSe_WeaponAtFrame(p, 20, 2);
            PlayerSe_WeaponAtFrame(p, 20, Sound_RandomSe(35, 2, 36, 2));
            PlayerSe_WeaponAtFrame(p, 80, 8);
            PlayerSe_WeaponAtFrame(p, 118, 6);
            PlayerSe_VoiceAtFrame(p, 18.0f, 0);
            PlayerSe_VoiceAtFrame(p, 26.0f, 3);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 98.0f, 0);
            PlayerSe_EquipAtFrame(p, 100.0f, 0);
            PlayerSe_VoiceAtFrame(p, 120.0f, 1);
            return;
        case 0x580:
            PlayerSe_WeaponAtFrame(p, 8, 4);
            PlayerSe_WeaponAtFrame(p, 34, 2);
            PlayerSe_WeaponAtFrame(p, 44, 6);
            PlayerSe_WeaponAtFrame(p, 18, Sound_RandomSe(35, 4, 35, 4));
            PlayerSe_WeaponAtFrame(p, 40, Sound_RandomSe(36, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 90, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(p, 56, 8);
            PlayerSe_WeaponAtFrame(p, 70, 4);
            PlayerSe_WeaponAtFrame(p, 96, 9);
            PlayerSe_WeaponAtFrame(p, 198, 6);
            PlayerSe_VoiceAtFrame(p, 22.0f, 2);
            PlayerSe_VoiceAtFrame(p, 36.0f, 2);
            PlayerSe_EquipAtFrame(p, 38.0f, 0);
            PlayerSe_VoiceAtFrame(p, 90.0f, 3);
            PlayerSe_EquipAtFrame(p, 92.0f, 0);
            PlayerSe_VoiceAtFrame(p, 192.0f, 1);
            return;
        case 0x581:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 40, 4);
            PlayerSe_WeaponAtFrame(p, 72, 7);
            PlayerSe_WeaponAtFrame(p, 54, Sound_RandomSe(37, 4, 37, 4));
            PlayerSe_WeaponAtFrame(p, 158, 6);
            PlayerSe_VoiceAtFrame(p, 66.0f, 3);
            PlayerSe_EquipAtFrame(p, 68.0f, 0);
            return;
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 18, 4);
            return;
        case 0x585:
            PlayerSe_VoiceAtFrame(p, 26.0f, 2);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 42.0f, 2);
            PlayerSe_VoiceAtFrame(p, 56.0f, 0);
            PlayerSe_EquipAtFrame(p, 58.0f, 0);
            return;
        case 0x586:
            PlayerSe_WeaponAtFrame(p, 6, 8);
            PlayerSe_WeaponAtFrame(p, 34, 4);
            return;
        case 0x587:
            PlayerSe_WeaponAtFrame(p, 16, 8);
            PlayerSe_WeaponAtFrame(p, 38, 1);
            PlayerSe_WeaponAtFrame(p, 24, 4);
            PlayerSe_VoiceAtFrame(p, 12.0f, 3);
            PlayerSe_VoiceAtFrame(p, 32.0f, 3);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(p, 20, 2);
            PlayerSe_WeaponAtFrame(p, 20, Sound_RandomSe(35, 4, 36, 4));
            return;
        case 0x589:
        case 0x58A:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 16, 4);
            return;
        case 0x58B:
            PlayerSe_WeaponAtFrame(p, 8, 4);
            return;
        case 0x58C:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            return;
        case 0x58D:
        case 0x58E:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            return;
        case 0x58F:
            PlayerSe_WeaponAtFrame(p, 2, 6);
            PlayerSe_WeaponAtFrame(p, 12, 2);
            return;
        case 0x590:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 8, 4);
            return;
        case 0x591:
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 10, 4);
            return;
        case 0x592:
            PlayerSe_WeaponAtFrame(p, 4, 6);
            PlayerSe_WeaponAtFrame(p, 4, 8);
            PlayerSe_WeaponAtFrame(p, 16, 5);
            PlayerSe_WeaponAtFrame(p, 38, 2);
            PlayerSe_VoiceAtFrame(p, 10.0f, 0);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 54.0f, 0);
            PlayerSe_VoiceAtFrame(p, 66.0f, 1);
            PlayerSe_EquipAtFrame(p, 68.0f, 0);
            return;
        }
        break;
    default:
        switch (w->unk6) {
        case 0x3EA:
            PlayerSe_WeaponAtFrame(p, 32, 12);
            PlayerSe_WeaponAtFrame(p, 18, 1);
            PlayerSe_WeaponAtFrame(p, 20, 2);
            PlayerSe_VoiceAtFrame(p, 36.0f, 3);
            PlayerSe_EquipAtFrame(p, 36.0f, 0);
            return;
        case 0x3EB:
            PlayerSe_WeaponAtFrame(p, 10, 3);
            PlayerSe_WeaponAtFrame(p, 32, 1);
            PlayerSe_VoiceAtFrame(p, 56.0f, 0);
            PlayerSe_EquipAtFrame(p, 56.0f, 0);
            return;
        case 0x3EC:
            PlayerSe_VoiceAtFrame(p, 62.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 0);
            PlayerSe_EquipAtFrame(p, 64.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            return;
        case 0x3F4:
        case 0x3F5:
            PlayerSe_WeaponAtFrame(p, 12, Sound_RandomSe(36, 2, 35, 2));
            PlayerSe_AtFrame(p, 20, 65);
            PlayerSe_EquipAtFrame(p, 40.0f, 3);
            PlayerSe_WeaponAtFrame(p, 58, 1);
            PlayerSe_VoiceAtFrame(p, 40.0f, 3);
            PlayerSe_VoiceAtFrame(p, 66.0f, 0);
            PlayerSe_VoiceAtFrame(p, 126.0f, 0);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_EquipAtFrame(p, 66.0f, 0);
            return;
        case 0x3F8:
            PlayerSe_WeaponAtFrame(p, 12, 1);
            PlayerSe_WeaponAtFrame(p, 20, 2);
            PlayerSe_VoiceAtFrame(p, 12.0f, 0);
            PlayerSe_VoiceAtFrame(p, 30.0f, 3);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            PlayerSe_EquipAtFrame(p, 30.0f, 0);
            return;
        case 0x3F9:
            PlayerSe_WeaponAtFrame(p, 8, 3);
            PlayerSe_WeaponAtFrame(p, 42, 1);
            PlayerSe_VoiceAtFrame(p, 32.0f, 0);
            PlayerSe_VoiceAtFrame(p, 60.0f, 0);
            PlayerSe_EquipAtFrame(p, 34.0f, 0);
            PlayerSe_EquipAtFrame(p, 62.0f, 0);
            return;
        case 0x3FA:
            PlayerSe_WeaponAtFrame(p, 24, 1);
            PlayerSe_VoiceAtFrame(p, 14.0f, 0);
            PlayerSe_EquipAtFrame(p, 22.0f, 4);
            return;
        case 0x579:
            PlayerSe_WeaponAtFrame(p, 8, 4);
            PlayerSe_EquipAtFrame(p, 26.0f, 4);
            PlayerSe_VoiceAtFrame(p, 18.0f, 0);
            PlayerSe_EquipAtFrame(p, 18.0f, 0);
            return;
        case 0x594:
            PlayerSe_WeaponAtFrame(p, 4, 5);
            PlayerSe_WeaponAtFrame(p, 32, Sound_RandomSe(37, 3, 36, 2));
            PlayerSe_WeaponAtFrame(p, 40, 7);
            PlayerSe_WeaponAtFrame(p, 140, 1);
            PlayerSe_VoiceAtFrame(p, 40.0f, 3);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_VoiceAtFrame(p, 118.0f, 0);
            PlayerSe_VoiceAtFrame(p, 148.0f, 0);
            PlayerSe_EquipAtFrame(p, 132.0f, 4);
            return;
        case 0x58A:
            PlayerSe_WeaponAtFrame(p, 4, 1);
            PlayerSe_VoiceAtFrame(p, 16.0f, 0);
            return;
        case 0x595:
            PlayerSe_WeaponAtFrame(p, 4, 5);
            return;
        case 0x590:
            PlayerSe_WeaponAtFrame(p, 4, 1);
            PlayerSe_VoiceAtFrame(p, 16.0f, 0);
            return;
        case 0x596:
            PlayerSe_WeaponAtFrame(p, 4, 5);
            return;
        case 0x57C:
            PlayerSe_WeaponAtFrame(p, 84, 1);
            PlayerSe_VoiceAtFrame(p, 14.0f, 0);
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 100.0f, 0);
            PlayerSe_VoiceAtFrame(p, 230.0f, 0);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            return;
        case 0x57D:
            PlayerSe_WeaponAtFrame(p, 4, 5);
            return;
        case 0x57E:
            PlayerSe_WeaponAtFrame(p, 48, Sound_RandomSe(37, 3, 36, 2));
            PlayerSe_WeaponAtFrame(p, 16, 5);
            PlayerSe_WeaponAtFrame(p, 130, 1);
            PlayerSe_VoiceAtFrame(p, 16.0f, 3);
            PlayerSe_VoiceAtFrame(p, 40.0f, 0);
            PlayerSe_VoiceAtFrame(p, 76.0f, 3);
            PlayerSe_VoiceAtFrame(p, 158.0f, 1);
            PlayerSe_VoiceAtFrame(p, 184.0f, 1);
            PlayerSe_EquipAtFrame(p, 16.0f, 0);
            PlayerSe_EquipAtFrame(p, 76.0f, 0);
            PlayerSe_EquipAtFrame(p, 52.0f, 4);
            PlayerSe_EquipAtFrame(p, 178.0f, 4);
            return;
        case 0x57F:
            PlayerSe_WeaponAtFrame(p, 76, Sound_RandomSe(35, 3, 36, 3));
            PlayerSe_WeaponAtFrame(p, 228, 38);
            PlayerSe_WeaponAtFrame(p, 8, 4);
            PlayerSe_WeaponAtFrame(p, 50, 5);
            PlayerSe_WeaponAtFrame(p, 114, 8);
            PlayerSe_WeaponAtFrame(p, 190, 1);
            PlayerSe_WeaponAtFrame(p, 224, 2);
            PlayerSe_WeaponAtFrame(p, 272, 1);
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 110.0f, 3);
            PlayerSe_VoiceAtFrame(p, 264.0f, 0);
            PlayerSe_VoiceAtFrame(p, 282.0f, 0);
            PlayerSe_EquipAtFrame(p, 28.0f, 0);
            PlayerSe_EquipAtFrame(p, 110.0f, 0);
            PlayerSe_EquipAtFrame(p, 198.0f, 4);
            PlayerSe_EquipAtFrame(p, 274.0f, 4);
            return;
        case 0x582:
            PlayerSe_WeaponAtFrame(p, 34, Sound_RandomSe(35, 4, 36, 4));
            PlayerSe_WeaponAtFrame(p, 2, 1);
            PlayerSe_WeaponAtFrame(p, 26, 2);
            PlayerSe_VoiceAtFrame(p, 12.0f, 3);
            PlayerSe_EquipAtFrame(p, 12.0f, 0);
            return;
        case 0x583:
            PlayerSe_WeaponAtFrame(p, 6, Sound_RandomSe(35, 3, 36, 1));
            PlayerSe_AtFrame(p, 6, 61);
            PlayerSe_WeaponAtFrame(p, 48, 1);
            PlayerSe_VoiceAtFrame(p, 48.0f, 0);
            PlayerSe_EquipAtFrame(p, 48.0f, 0);
            return;
        case 0x584:
            PlayerSe_WeaponAtFrame(p, 4, 1);
            PlayerSe_WeaponAtFrame(p, 20, 6);
            PlayerSe_VoiceAtFrame(p, 20.0f, 1);
            return;
        case 0x586:
            PlayerSe_WeaponAtFrame(p, 20, 1);
            PlayerSe_VoiceAtFrame(p, 30.0f, 0);
            PlayerSe_EquipAtFrame(p, 284.0f, 4);
            return;
        case 0x587:
            PlayerSe_WeaponAtFrame(p, 56, 1);
            PlayerSe_WeaponAtFrame(p, 80, 2);
            PlayerSe_VoiceAtFrame(p, 28.0f, 0);
            PlayerSe_VoiceAtFrame(p, 36.0f, 0);
            PlayerSe_VoiceAtFrame(p, 52.0f, 0);
            PlayerSe_VoiceAtFrame(p, 72.0f, 0);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            PlayerSe_EquipAtFrame(p, 72.0f, 0);
            PlayerSe_EquipAtFrame(p, 84.0f, 4);
            return;
        case 0x588:
            PlayerSe_WeaponAtFrame(p, 8, 4);
            PlayerSe_WeaponAtFrame(p, 48, 5);
            PlayerSe_VoiceAtFrame(p, 46.0f, 3);
            PlayerSe_EquipAtFrame(p, 46.0f, 0);
            return;
        case 0x589:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 26.0f, 0);
            PlayerSe_EquipAtFrame(p, 26.0f, 0);
            return;
        case 0x58B:
            PlayerSe_WeaponAtFrame(p, 28, 5);
            PlayerSe_VoiceAtFrame(p, 12.0f, 0);
            return;
        case 0x58C:
            PlayerSe_WeaponAtFrame(p, 8, 4);
            PlayerSe_VoiceAtFrame(p, 8.0f, 0);
            PlayerSe_VoiceAtFrame(p, 40.0f, 3);
            PlayerSe_EquipAtFrame(p, 40.0f, 0);
            return;
        case 0x58D:
            PlayerSe_WeaponAtFrame(p, 16, 5);
            PlayerSe_VoiceAtFrame(p, 26.0f, 0);
            return;
        case 0x58E:
            PlayerSe_WeaponAtFrame(p, 8, 4);
            PlayerSe_WeaponAtFrame(p, 48, 5);
            PlayerSe_VoiceAtFrame(p, 46.0f, 3);
            PlayerSe_EquipAtFrame(p, 46.0f, 0);
            return;
        case 0x58F:
            PlayerSe_WeaponAtFrame(p, 30, 5);
            PlayerSe_VoiceAtFrame(p, 26.0f, 0);
            PlayerSe_EquipAtFrame(p, 46.0f, 0);
            return;
        case 0x591:
            PlayerSe_WeaponAtFrame(p, 28, 5);
            PlayerSe_VoiceAtFrame(p, 12.0f, 0);
            return;
        case 0x592:
            PlayerSe_VoiceAtFrame(p, 8.0f, 0);
            PlayerSe_VoiceAtFrame(p, 40.0f, 3);
            PlayerSe_EquipAtFrame(p, 8.0f, 0);
            return;
        case 0x593:
            PlayerSe_WeaponAtFrame(p, 16, 5);
            PlayerSe_VoiceAtFrame(p, 26.0f, 0);
            return;
        case 0x597:
            PlayerSe_WeaponAtFrame(p, 6, 1);
            PlayerSe_VoiceAtFrame(p, 18.0f, 0);
            PlayerSe_EquipAtFrame(p, 20.0f, 0);
            break;
        }
        break;
    }
}

void func_00395CF0(Player *p) /* p: unused, called through a (Player *) table */
{
}

void Player_MotionFxStep(Player *p)
{
    Work438 *w = (Work438 *)((u8 *)p + 0x438);

    switch (w->state) {
    case 0:
        w->state++;
        w->unk12 = 0;
        break;
    case 1:
        Player_MotionEffects(p, w);
        Player_MotionSounds(p, w);
        break;
    }
}

/* Spawns effect a (by kind) on obj once its motion passes frame. The frame is the second
 * parameter: EABI passes it in f12 either way, but the argument order sets the order of
 * the callers' set-up (Player_MotionEffects' loop for motion 0x587). */
void Player_EffectAtFrame(void *obj, f32 frame, s32 a, s16 kind)
{
    if (Motion_PassedFrame(obj, 0, frame) != 0) {
        switch (kind) {
        case 0:
            Effect0D_SpawnForPlayer(obj, a, 0);
            break;
        case 1:
            Effect0D_SpawnForPlayer(obj, a, 3);
            break;
        case 2:
            Effect0D_SpawnForPlayer(obj, 5, 4);
            break;
        case 3:
            Effect0D_SpawnForPlayer(obj, a, 4);
            break;
        case 4:
            Effect0D_SpawnForPlayer(obj, a, 5);
            break;
        case 5:
            Effect0D_SpawnForPlayer(obj, a, 6);
            break;
        case 6:
            Effect0D_SpawnForPlayer(obj, a, 7);
            break;
        case 7:
            Effect0D_SpawnForPlayer(obj, a, 8);
            break;
        case 8:
            Effect0D_SpawnForPlayer(obj, a, 0x14);
            break;
        }
    }
}

void func_00395EC0(void *obj, Work438 *w)
{
    PlayerRec_SelectInRange(obj, 0x12, 0);
    PlayerRec_SelectInRange(obj, 0xE, 0);
}

s32 Player_IsLocal(void *obj);
s32 Player_OnSameSide(Player *a, Player *b);
s32 Player_HasSkill(Player *p, u8 skill);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void Effect0D_SpawnForPlayerScaled(Player *p, s16 arg, s32 type, f32 scale);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
void Effect02_SpawnAt(f32 *pos, s32 a, u16 b);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 Motion_ReachedFrame(void *, s32, f32);
s32 Obj_InCurrentArea(void *obj);
void Player_SpawnGroundEffect(Player *p, s32 kind, f32 scale);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
u16 Rand_Next(s32 stream);
void ModelObj_SpawnOn(void *arg0, s32 arg1);
struct StageObj *StageObj_K05_Create(void *owner, s32 a, s32 b);
void StageObj_K05_CreateEx(Em *em, s32 a1, s32 a2, s16 v); /* game em_bulldrome.c; called here with a Player */
void StageObj_K0C_CreateAtPlayer(Player *p, s32 a);
void StageObj_K0E_CreateOnEm(Em *em, s16 a);
void StageObj_K14_CreateOnEm(Player *p, s16 a, s16 b, f32 c);
void StageObj_K15_Create(Em *em, u8 a);
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);

/* The effects and sounds of a hunter's motions (state 1 of the object at +0x438), by the
 * motion w->unk6 and the weapon type: for the other hunters on the same side, the team item
 * effects (unk86F), then Player_EffectAtFrame and friends at the motion's frames. */
void Player_MotionEffects(Player *p, Work438 *w)
{
    Player *q;
    s16 i;
    f32 fr;
    f32 v[3];
    f32 r[3];

    if ((s32)p->motionId[0] != w->unk6) {
        w->unk8 = 0;
        w->unk6 = p->motionId[0];
    }
    if (w->unk12 != 0) {
        w->unk12--;
    }
    if (Player_IsLocal(p) != 1 && p->stateGroup != 3) {
        for (i = 0; i < gScene.playerCount; i++) {
            q = &gPlayers[i];
            if (Player_OnSameSide(q, p) && i != p->index && q->area == p->area
                && (q->teamItemMask & (1 << p->index)) && q->stateGroup == 0) {
                q->teamItemMask &= ~(1 << p->index);
                switch (q->state) {
                case 0x71:
                    switch (q->useItem) {
                    case 0x22:
                        if (w->unk12 == 0) {
                            w->unk12 = 60;
                            Effect06_SpawnAtBone(p, 0, 0, 10, 4.0f);
                        }
                        break;
                    case 0x7:
                        if (Player_HasSkill(q, 0x5D) == 1 || Player_HasSkill(q, 0x5E) == 1) {
                            if (w->unk12 == 0) {
                                w->unk12 = 60;
                                Effect06_SpawnAtBone(p, 0, 0, 10, 4.0f);
                            }
                        }
                        break;
                    case 0xB7:
                        if (Player_HasSkill(q, 0x5D) == 1 || Player_HasSkill(q, 0x5E) == 1) {
                            if (w->unk12 == 0) {
                                w->unk12 = 60;
                                Effect06_SpawnAtBone(p, 0, 0, 10, 4.0f);
                            }
                        }
                        break;
                    case 0xB:
                        if (Player_HasSkill(q, 0x5D) == 1 || Player_HasSkill(q, 0x5E) == 1) {
                            if (w->unk12 == 0) {
                                w->unk12 = 60;
                                Effect06_SpawnAtBone(p, 0, 1, 10, 4.0f);
                            }
                        }
                        break;
                    case 0xCE:
                        if (Player_HasSkill(q, 0x5D) == 1 || Player_HasSkill(q, 0x5E) == 1) {
                            if (w->unk12 == 0) {
                                w->unk12 = 60;
                                Effect06_SpawnAtBone(p, 0, 2, 10, 4.0f);
                            }
                        }
                        break;
                    case 0xCF:
                        if (Player_HasSkill(q, 0x5D) == 1 || Player_HasSkill(q, 0x5E) == 1) {
                            if (w->unk12 == 0) {
                                w->unk12 = 60;
                                Effect06_SpawnAtBone(p, 0, 3, 10, 4.0f);
                            }
                        }
                        break;
                    }
                    break;
                case 0x63:
                    if (w->unk12 == 0) {
                        w->unk12 = 60;
                        switch (q->useItem) {
                        case 0xB0:
                            Effect06_SpawnAtBone(p, 0, 0, 10, 4.0f);
                            break;
                        case 0xB1:
                            Effect06_SpawnAtBone(p, 0, 1, 10, 4.0f);
                            break;
                        case 0xB2:
                            Effect06_SpawnAtBone(p, 0, 2, 10, 4.0f);
                            break;
                        case 0xB3:
                            Effect06_SpawnAtBone(p, 0, 3, 10, 4.0f);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    switch (w->unk6) {
    case 0x2:
        if (gScene.online == 0) {
            Player_EffectAtFrame(p, 20.0f, 8, 8);
            Player_EffectAtFrame(p, 54.0f, 5, 8);
            Player_EffectAtFrame(p, 88.0f, 8, 8);
        }
        break;
    case 0x3:
        if (gScene.online == 0) {
            Player_EffectAtFrame(p, 12.0f, 8, 0);
            Player_EffectAtFrame(p, 16.0f, 5, 0);
            Player_EffectAtFrame(p, 32.0f, 8, 0);
            Player_EffectAtFrame(p, 56.0f, 5, 0);
        }
        break;
    case 0x4:
        Player_EffectAtFrame(p, 12.0f, 5, 1);
        Player_EffectAtFrame(p, 30.0f, 8, 1);
        Player_EffectAtFrame(p, 50.0f, 5, 1);
        break;
    case 0x5:
        if (Motion_PassedFrame(p, 0, 2.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 8, 3, 0.7f);
        }
        if (Motion_PassedFrame(p, 0, 20.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 5, 3, 0.7f);
        }
        Player_EffectAtFrame(p, 30.0f, 8, 3);
        break;
    case 0xD:
        Player_EffectAtFrame(p, 2.0f, 0xA, 6);
        break;
    case 0xF:
        Player_EffectAtFrame(p, 2.0f, 8, 1);
        break;
    case 0x13:
        Player_EffectAtFrame(p, 2.0f, 8, 7);
        break;
    case 0x14:
        Player_EffectAtFrame(p, 2.0f, 8, 7);
        break;
    case 0x1C:
        if (Motion_PassedFrame(p, 0, 22.0f)) {
            Effect0D_SpawnForPlayer(p, 0xA, 0xB);
        }
        Player_EffectAtFrame(p, 4.0f, 0xA, 0);
        break;
    case 0x1E:
        if (gScene.online == 0) {
            Player_EffectAtFrame(p, 4.0f, 8, 2);
        }
        break;
    case 0x1F:
        Player_EffectAtFrame(p, 16.0f, 2, 4);
        Player_EffectAtFrame(p, 20.0f, 2, 4);
        break;
    case 0x20:
        if (((s32)p->motionTime % 3) == 0) {
            Effect0D_SpawnForPlayer(p, 2, 5);
        }
        break;
    case 0x25:
        Player_EffectAtFrame(p, 12.0f, 8, 8);
        Player_EffectAtFrame(p, 32.0f, 5, 8);
        break;
    case 0x26:
        if (Motion_PassedFrame(p, 0, 4.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 8, 0, 1.5f);
        }
        if (Motion_PassedFrame(p, 0, 20.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 5, 0, 1.5f);
        }
        break;
    case 0x27:
        if (Motion_PassedFrame(p, 0, 2.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 5, 3, 0.8f);
        }
        if (Motion_PassedFrame(p, 0, 26.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 8, 3, 0.8f);
        }
        if (Motion_PassedFrame(p, 0, 54.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 5, 3, 0.8f);
        }
        if (Motion_PassedFrame(p, 0, 78.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 8, 3, 0.8f);
        }
        break;
    case 0x2A:
        if (gScene.online == 0) {
            Player_EffectAtFrame(p, 26.0f, 8, 1);
            Player_EffectAtFrame(p, 18.0f, 8, 1);
            Player_EffectAtFrame(p, 46.0f, 8, 1);
        }
        break;
    case 0x2C:
    case 0x2D:
        if (w->unk6 == 0x2C) {
            Player_EffectAtFrame(p, 30.0f, 5, 3);
        } else {
            Player_EffectAtFrame(p, 30.0f, 8, 3);
        }
        if ((Motion_PassedFrame(p, 0, 30.0f)) && !(Rand_Next(1) & 0xFFFF & 3)) {
            StageObj_K15_Create((Em *) p, 0);
        }
        break;
    case 0x2E:
        Player_EffectAtFrame(p, 10.0f, 5, 1);
        break;
    case 0x30:
        if ((Motion_ReachedFrame(p, 0, 16.0f) == 0) && (gScene.frame & 1)) {
            Effect0D_SpawnForPlayer(p, 0xA, 0xA);
        }
        break;
    case 0x32:
        Player_EffectAtFrame(p, 30.0f, 8, 3);
        break;
    case 0x34:
        Player_EffectAtFrame(p, 16.0f, 8, 8);
        Player_EffectAtFrame(p, 46.0f, 5, 8);
        break;
    case 0x35:
        Player_EffectAtFrame(p, 28.0f, 8, 1);
        Player_EffectAtFrame(p, 8.0f, 5, 1);
        break;
    case 0x39:
        Player_EffectAtFrame(p, 4.0f, 2, 6);
        break;
    case 0x3A:
        if (Motion_PassedFrame(p, 0, 6.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 7, 0.7f);
        }
        break;
    case 0x3B:
        Player_EffectAtFrame(p, 16.0f, 8, 1);
        Player_EffectAtFrame(p, 2.0f, 5, 1);
        break;
    case 0x3C:
        if ((Motion_ReachedFrame(p, 0, 18.0f)) && !(gScene.frame & 0x1F)) {
            Effect0D_SpawnForPlayerScaled(p, 0xE, 0x1A, 0.4f);
        }
        break;
    case 0x3E:
        if (Motion_PassedFrame(p, 0, 2.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0xA, 7, 0.6f);
        }
        break;
    case 0xCA:
        if (Motion_PassedFrame(p, 0, 24.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 7, 0.5f);
        }
        break;
    case 0xCC:
        if (Motion_PassedFrame(p, 0, 16.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 7, 0.8f);
        }
        break;
    case 0xCE:
        if (Motion_PassedFrame(p, 0, 6.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0xA, 7, 1.5f);
        }
        if (Motion_PassedFrame(p, 0, 34.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0xA, 0x11, 1.5f);
        }
        if (Motion_PassedFrame(p, 0, 50.0f) || Motion_PassedFrame(p, 0, 54.0f) || Motion_PassedFrame(p, 0, 58.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 8, 0x12, 1.0f);
        }
        break;
    case 0xD1:
        if (Motion_PassedFrame(p, 0, 6.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0x14, 7, 1.5f);
        }
        if (Motion_PassedFrame(p, 0, 28.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0xA, 7, 1.5f);
        }
        if (Motion_PassedFrame(p, 0, 42.0f) || Motion_PassedFrame(p, 0, 46.0f) || Motion_PassedFrame(p, 0, 50.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 8, 0x12, 1.0f);
        }
        break;
    case 0xD2:
        if (Motion_PassedFrame(p, 0, 152.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 7, 1.0f);
        }
        break;
    case 0xD7:
        Player_EffectAtFrame(p, 2.0f, 0x13, 5);
        break;
    case 0xDE:
        if (Motion_PassedFrame(p, 0, 40.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 7, 1.0f);
        }
        if (Motion_PassedFrame(p, 0, 46.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 0x14, 1.2f);
        }
        break;
    case 0xFA:
        if (Motion_PassedFrame(p, 0, 250.0f)) {
            ModelObj_SpawnOn(p, 6);
            ModelObj_SpawnOn(p, 7);
        }
        break;
    case 0xFB:
        if ((Motion_PassedFrame(p, 0, 536.0f)) && (Obj_InCurrentArea(p) & 0xFF)) {
            Em_GetBonePos(p, 0xA, v);
            Effect0D_SpawnAt(v, 7, 0.8f);
        }
        if ((Motion_PassedFrame(p, 0, 620.0f)) && (Obj_InCurrentArea(p) & 0xFF)) {
            Em_GetBonePos(p, 0xA, v);
            Effect0D_SpawnAt(v, 7, 1.0f);
        }
        if ((Motion_PassedFrame(p, 0, 698.0f)) && (Obj_InCurrentArea(p) & 0xFF)) {
            Em_GetBonePos(p, 2, v);
            Effect0D_SpawnAt(v, 7, 0.6f);
        }
        break;
    case 0x19D:
        if (Motion_PassedFrame(p, 0, 274.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0xE, 0x15, 1.0f);
        }
        break;
    case 0x19F:
        if ((Motion_PassedFrame(p, 0, 50.0f)) && ((Obj_InCurrentArea(p) & 0xFF) == 1)) {
            Vec_Set(v, 0.0f, 0.0f, 120.0f);
            Vec_Rotate(r, v, (f32 *)p->worldMtx);
            v[0] = p->pos[0] + r[0];
            v[1] = p->pos[1] + r[1];
            v[2] = p->pos[2] + r[2];
            Effect02_SpawnAt(v, 7, p->rot[1]);
        }
        break;
    case 0x1A0:
        if ((p->useItem == 0x31) && ((Motion_PassedFrame(p, 0, 16.0f)) || (Motion_PassedFrame(p, 0, 22.0f)) || (Motion_PassedFrame(p, 0, 28.0f)))) {
            StageObj_K0C_CreateAtPlayer(p, 6);
        }
        break;
    case 0x1A2:
        if (Motion_PassedFrame(p, 0, 36.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0x12, 0x18, 0.3f);
        }
        break;
    case 0x1AA:
        if (Motion_PassedFrame(p, 0, 190.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 0x17, 0.6f);
        }
        break;
    case 0x1AF:
        if (!(gScene.frame & 0x1F)) {
            Effect0D_SpawnForPlayerScaled(p, 0x14, 0x16, 1.0f);
        }
        break;
    case 0x1B0:
        Player_EffectAtFrame(p, 78.0f, 1, 6);
        break;
    case 0x287:
        if (Motion_PassedFrame(p, 0, 236.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0, 0x21, 0.7f);
        }
        if (Motion_PassedFrame(p, 0, 314.0f)) {
            ModelObj_SpawnOn(p, 6);
            ModelObj_SpawnOn(p, 7);
        }
        break;
    case 0x320:
        if (Motion_PassedFrame(p, 0, 24.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 0, 0x19, 0.8f);
        }
        break;
    case 0x321:
        if ((Motion_PassedFrame(p, 0, 20.0f)) && (p->cookResult == 4)) {
            Effect06_SpawnAtBone(p, 0xA, 0, 0xE, 1.0f);
        }
        break;
    case 0x323:
        if ((w->unk6 == 0x323) && (((s32) gScene.frame % 20) == 0)) {
            StageObj_K0E_CreateOnEm((Em *) p, 6);
        }
        break;
    case 0x32A:
        if (Motion_PassedFrame(p, 0, 52.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 7, 0.5f);
        }
        break;
    case 0x37D:
        if (Motion_PassedFrame(p, 0, 44.0f)) {
            Effect0D_SpawnForPlayerScaled(p, 2, 7, 1.5f);
        }
        break;
    }
    switch (p->weaponType) {
    case 1:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 2.0f, 5, 8);
            Player_EffectAtFrame(p, 32.0f, 8, 8);
            return;
        case 0x3F1:
            Player_EffectAtFrame(p, 22.0f, 8, 8);
            Player_EffectAtFrame(p, 76.0f, 8, 8);
            Player_EffectAtFrame(p, 50.0f, 5, 8);
            Player_EffectAtFrame(p, 104.0f, 5, 8);
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            Player_EffectAtFrame(p, 12.0f, 8, 1);
            Player_EffectAtFrame(p, 12.0f, 5, 1);
            Player_EffectAtFrame(p, 16.0f, 5, 1);
            return;
        case 0x580:
            Player_EffectAtFrame(p, 64.0f, 8, 1);
            Player_EffectAtFrame(p, 50.0f, 5, 1);
            Player_EffectAtFrame(p, 54.0f, 5, 1);
            return;
        case 0x581:
        case 0x582:
            Player_EffectAtFrame(p, 8.0f, 8, 1);
            Player_EffectAtFrame(p, 8.0f, 5, 1);
            Player_EffectAtFrame(p, 12.0f, 5, 1);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 2:
        switch (w->unk6) {
        case 0x3EC:
        case 0x3ED:
            Player_EffectAtFrame(p, 4.0f, 5, 1);
            Player_EffectAtFrame(p, 28.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(p, 60.0f, 8, 8);
            Player_EffectAtFrame(p, 32.0f, 5, 8);
            Player_EffectAtFrame(p, 76.0f, 5, 8);
            return;
        case 0x3F3:
            Player_EffectAtFrame(p, 4.0f, 0xA, 0);
            if (Motion_PassedFrame(p, 0, 20.0f)) {
                Effect0D_SpawnForPlayer(p, 0xA, 0xB);
            }
            break;
        case 0x3F4:
        case 0x3F5:
            if (w->unk6 == 0x3F4) {
                Player_EffectAtFrame(p, 2.0f, 8, 1);
            } else {
                Player_EffectAtFrame(p, 2.0f, 5, 1);
            }
            Player_EffectAtFrame(p, 18.0f, 0xA, 6);
            return;
        case 0x57A:
            if (Motion_PassedFrame(p, 0, 68.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xE, 0xC, 1.8f);
                return;
            }
            break;
        case 0x57B:
            if ((Motion_ReachedFrame(p, 0, 1e+01f)) && (Motion_ReachedFrame(p, 0, 66.0f) == 0) && !(gScene.frame & 3)) {
                Effect0D_SpawnForPlayer(p, 2, 0xD);
                return;
            }
            break;
        case 0x57C:
            if (Motion_PassedFrame(p, 0, 38.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xE, 0xF, 1.5f);
                return;
            }
            break;
        case 0x57E:
            if (Motion_PassedFrame(p, 0, 76.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xE, 0x13, 0.6f);
                return;
            }
            break;
        case 0x583:
            if (Motion_PassedFrame(p, 0, 34.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xA, 0xC, 1.2f);
                Effect0D_SpawnForPlayer(p, 0xA, 0x1F);
                return;
            }
            break;
        case 0x584:
            if (Motion_PassedFrame(p, 0, 52.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xE, 0xC, 0.7f);
                return;
            }
            break;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 5:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 2.0f, 5, 1);
            Player_EffectAtFrame(p, 26.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(p, 22.0f, 5, 1);
            return;
        case 0x3F2:
            Player_EffectAtFrame(p, 18.0f, 8, 1);
            return;
        case 0x579:
        case 0x57A:
        case 0x57B:
            Player_EffectAtFrame(p, 12.0f, 8, 1);
            Player_EffectAtFrame(p, 12.0f, 5, 1);
            Player_EffectAtFrame(p, 16.0f, 5, 1);
            return;
        case 0x580:
            Player_EffectAtFrame(p, 64.0f, 8, 1);
            Player_EffectAtFrame(p, 50.0f, 5, 1);
            Player_EffectAtFrame(p, 54.0f, 5, 1);
            return;
        case 0x581:
        case 0x582:
            Player_EffectAtFrame(p, 8.0f, 8, 1);
            Player_EffectAtFrame(p, 8.0f, 5, 1);
            Player_EffectAtFrame(p, 12.0f, 5, 1);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 9:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 18.0f, 5, 8);
            Player_EffectAtFrame(p, 50.0f, 8, 8);
            return;
        case 0x3F3:
            Player_EffectAtFrame(p, 10.0f, 8, 8);
            Player_EffectAtFrame(p, 20.0f, 5, 8);
            return;
        case 0x3F6:
            Player_EffectAtFrame(p, 12.0f, 8, 1);
            Player_EffectAtFrame(p, 22.0f, 5, 1);
            Player_EffectAtFrame(p, 28.0f, 8, 8);
            return;
        case 0x3F7:
            Player_EffectAtFrame(p, 12.0f, 5, 1);
            Player_EffectAtFrame(p, 22.0f, 8, 1);
            Player_EffectAtFrame(p, 28.0f, 5, 8);
            return;
        case 0x3F8:
            Player_EffectAtFrame(p, 4.0f, 8, 1);
            Player_EffectAtFrame(p, 24.0f, 8, 8);
            Player_EffectAtFrame(p, 32.0f, 5, 3);
            return;
        case 0x579:
            Player_EffectAtFrame(p, 10.0f, 8, 8);
            Player_EffectAtFrame(p, 14.0f, 5, 3);
            return;
        case 0x57A:
            Player_EffectAtFrame(p, 10.0f, 8, 8);
            Player_EffectAtFrame(p, 10.0f, 5, 3);
            return;
        case 0x57B:
            Player_EffectAtFrame(p, 10.0f, 8, 8);
            Player_EffectAtFrame(p, 10.0f, 5, 3);
            return;
        case 0x57D:
            Player_EffectAtFrame(p, 20.0f, 5, 8);
            Player_EffectAtFrame(p, 24.0f, 8, 1);
            return;
        case 0x57E:
            Player_EffectAtFrame(p, 22.0f, 8, 1);
            return;
        case 0x580:
            Player_EffectAtFrame(p, 2.0f, 8, 3);
            Player_EffectAtFrame(p, 14.0f, 5, 8);
            Player_EffectAtFrame(p, 48.0f, 5, 3);
            Player_EffectAtFrame(p, 54.0f, 5, 8);
            Player_EffectAtFrame(p, 64.0f, 5, 8);
            Player_EffectAtFrame(p, 74.0f, 5, 3);
            return;
        case 0x581:
            Player_EffectAtFrame(p, 2.0f, 8, 1);
            Player_EffectAtFrame(p, 24.0f, 5, 3);
            Player_EffectAtFrame(p, 42.0f, 8, 3);
            Player_EffectAtFrame(p, 50.0f, 5, 8);
            return;
        case 0x589:
            Player_EffectAtFrame(p, 24.0f, 8, 8);
            Player_EffectAtFrame(p, 44.0f, 5, 3);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 3:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 18.0f, 8, 8);
            Player_EffectAtFrame(p, 52.0f, 5, 8);
            return;
        case 0x3F1:
            Player_EffectAtFrame(p, 18.0f, 5, 8);
            Player_EffectAtFrame(p, 96.0f, 5, 8);
            Player_EffectAtFrame(p, 60.0f, 8, 8);
            return;
        case 0x3F2:
            Player_EffectAtFrame(p, 2.0f, 8, 1);
            Player_EffectAtFrame(p, 44.0f, 5, 1);
            Player_EffectAtFrame(p, 46.0f, 5, 1);
            return;
        case 0x3F3:
            Player_EffectAtFrame(p, 8.0f, 8, 8);
            Player_EffectAtFrame(p, 36.0f, 5, 8);
            return;
        case 0x3F6:
        case 0x3F7:
            if (w->unk6 == 0x3F6) {
                Player_EffectAtFrame(p, 4.0f, 8, 1);
                return;
            }
            Player_EffectAtFrame(p, 2.0f, 5, 1);
            return;
        case 0x3F8:
            if (Motion_PassedFrame(p, 0, 4.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 8, 3, 0.8f);
            }
            if (Motion_PassedFrame(p, 0, 8.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 5, 3, 0.8f);
                return;
            }
            break;
        case 0x579:
            if (Motion_PassedFrame(p, 0, 46.0f)) {
                Effect0D_SpawnForPlayer(p, 0xA, 0x10);
            }
            if (Motion_PassedFrame(p, 0, 48.0f)) {
                StageObj_K14_CreateOnEm(p, 2, 3, 0.6f);
            }
            if (Motion_PassedFrame(p, 0, 52.0f)) {
                StageObj_K14_CreateOnEm(p, 3, 3, 0.6f);
                return;
            }
            break;
        case 0x57A:
            if (Motion_PassedFrame(p, 0, 4.0f)) {
                StageObj_K14_CreateOnEm(p, 2, 2, 0.4f);
            }
            if (Motion_PassedFrame(p, 0, 22.0f)) {
                StageObj_K14_CreateOnEm(p, 3, 2, 0.4f);
                return;
            }
            break;
        case 0x57B:
            Player_EffectAtFrame(p, 10.0f, 8, 1);
            Player_EffectAtFrame(p, 12.0f, 8, 1);
            Player_EffectAtFrame(p, 12.0f, 5, 1);
            Player_EffectAtFrame(p, 14.0f, 5, 1);
            return;
        case 0x57C:
            Player_EffectAtFrame(p, 8.0f, 8, 1);
            Player_EffectAtFrame(p, 10.0f, 8, 1);
            Player_EffectAtFrame(p, 16.0f, 5, 1);
            return;
        case 0x57D:
            Player_EffectAtFrame(p, 16.0f, 5, 1);
            Player_EffectAtFrame(p, 20.0f, 5, 1);
            Player_EffectAtFrame(p, 24.0f, 8, 1);
            return;
        case 0x57E:
            Player_EffectAtFrame(p, 22.0f, 5, 1);
            Player_EffectAtFrame(p, 26.0f, 5, 1);
            Player_EffectAtFrame(p, 18.0f, 8, 1);
            return;
        case 0x583:
            Player_EffectAtFrame(p, 16.0f, 8, 1);
            Player_EffectAtFrame(p, 18.0f, 8, 1);
            return;
        case 0x584:
            Player_EffectAtFrame(p, 10.0f, 8, 1);
            Player_EffectAtFrame(p, 14.0f, 8, 1);
            Player_EffectAtFrame(p, 12.0f, 5, 1);
            Player_EffectAtFrame(p, 16.0f, 5, 1);
            return;
        case 0x587:
            fr = 26.0f;
            i = 0;
            do {
                if (Motion_PassedFrame(p, 0, fr)) {
                    Effect0D_SpawnForPlayerScaled(p, 5, 3, 1.3f);
                    Effect0D_SpawnForPlayerScaled(p, 8, 3, 1.3f);
                }
                fr += 4.0f;
                i++;
            } while (i < 9);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 4:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 2.0f, 5, 1);
            Player_EffectAtFrame(p, 26.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(p, 6.0f, 5, 1);
            return;
        case 0x3F4:
        case 0x3F5:
            if (w->unk6 == 0x3F4) {
                Player_EffectAtFrame(p, 2.0f, 8, 1);
                Player_EffectAtFrame(p, 16.0f, 0xA, 6);
                return;
            } else {
                Player_EffectAtFrame(p, 2.0f, 5, 1);
                Player_EffectAtFrame(p, 18.0f, 0xA, 6);
            }
            return;
        case 0x579:
            if (Motion_PassedFrame(p, 0, 6.0f)) {
                StageObj_K05_Create(p, 0xB, 1);
                return;
            }
            break;
        case 0x57A:
            if (Motion_PassedFrame(p, 0, 2.0f)) {
                StageObj_K05_Create(p, 9, 1);
                return;
            }
            break;
        case 0x57B:
            if (Motion_PassedFrame(p, 0, 52.0f)) {
                StageObj_K05_Create(p, 9, 1);
                return;
            }
            break;
        case 0x57C:
            if (Motion_PassedFrame(p, 0, 24.0f)) {
                StageObj_K05_Create(p, 0xE, 1);
            }
            Player_EffectAtFrame(p, 6.0f, 8, 1);
            Player_EffectAtFrame(p, 26.0f, 5, 3);
            return;
        case 0x57D:
            if (Motion_PassedFrame(p, 0, 20.0f)) {
                StageObj_K05_Create(p, 0xD, 1);
                return;
            }
            break;
        case 0x57E:
            if (Motion_PassedFrame(p, 0, 10.0f)) {
                StageObj_K05_Create(p, 0xC, 1);
                return;
            }
            break;
        case 0x587:
            fr = 46.0f;
            i = 0;
            do {
                Player_EffectAtFrame(p, fr, 5, 1);
                Player_EffectAtFrame(p, fr, 8, 1);
                fr += 4.0f;
                i++;
            } while (i < 7);
            return;
        case 0x588:
            if (Motion_PassedFrame(p, 0, 16.0f)) {
                StageObj_K05_Create(p, 8, 1);
                return;
            }
            break;
        case 0x589:
            if (Motion_PassedFrame(p, 0, 10.0f)) {
                StageObj_K05_CreateEx((Em *)p, 0xC, 1, 0x57E);
                return;
            }
            break;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 6:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 2.0f, 5, 1);
            Player_EffectAtFrame(p, 26.0f, 8, 1);
            return;
        case 0x3F1:
            Player_EffectAtFrame(p, 6.0f, 5, 1);
            return;
        case 0x3F4:
            Player_EffectAtFrame(p, 2.0f, 8, 1);
            Player_EffectAtFrame(p, 16.0f, 0xA, 6);
            return;
        case 0x3F5:
            Player_EffectAtFrame(p, 2.0f, 5, 1);
            Player_EffectAtFrame(p, 18.0f, 0xA, 6);
            return;
        case 0x3FA:
            if (Motion_PassedFrame(p, 0, 32.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 2, 0x23, 1.0f);
                return;
            }
            break;
        case 0x3FB:
            if (Motion_PassedFrame(p, 0, 8.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 2, 0x27, 1.5f);
                Effect0D_SpawnForPlayerScaled(p, 0xA, 0x24, 1.0f);
                return;
            }
            break;
        case 0x579:
            Player_EffectAtFrame(p, 10.0f, 8, 1);
            if (Motion_PassedFrame(p, 0, 14.0f)) {
                StageObj_K05_Create(p, 5, 2);
                return;
            }
            break;
        case 0x57A:
            Player_EffectAtFrame(p, 12.0f, 5, 8);
            Player_EffectAtFrame(p, 34.0f, 5, 1);
            if (Motion_PassedFrame(p, 0, 2.0f)) {
                StageObj_K05_Create(p, 6, 2);
            }
            if (Motion_PassedFrame(p, 0, 20.0f)) {
                StageObj_K05_Create(p, 7, 1);
                return;
            }
            break;
        case 0x57B:
            Player_EffectAtFrame(p, 8.0f, 5, 8);
            Player_EffectAtFrame(p, 60.0f, 8, 3);
            if (Motion_PassedFrame(p, 0, 10.0f)) {
                StageObj_K05_Create(p, 7, 2);
            }
            if (Motion_PassedFrame(p, 0, 54.0f)) {
                StageObj_K05_Create(p, 8, 1);
            }
            if (Motion_PassedFrame(p, 0, 64.0f)) {
                StageObj_K05_Create(p, 0xC, 2);
                return;
            }
            break;
        case 0x57C:
            Player_EffectAtFrame(p, 14.0f, 5, 1);
            Player_EffectAtFrame(p, 28.0f, 8, 8);
            Player_EffectAtFrame(p, 38.0f, 5, 3);
            if (Motion_PassedFrame(p, 0, 36.0f)) {
                StageObj_K05_Create(p, 7, 1);
            }
            if (Motion_PassedFrame(p, 0, 36.0f)) {
                StageObj_K05_Create(p, 7, 2);
                return;
            }
            break;
        case 0x57D:
            Player_EffectAtFrame(p, 18.0f, 8, 8);
            Player_EffectAtFrame(p, 28.0f, 5, 3);
            if (Motion_PassedFrame(p, 0, 14.0f)) {
                StageObj_K05_Create(p, 0xD, 1);
            }
            if (Motion_PassedFrame(p, 0, 24.0f)) {
                StageObj_K05_Create(p, 0xA, 2);
                return;
            }
            break;
        case 0x57E:
            Player_EffectAtFrame(p, 18.0f, 8, 1);
            if (Motion_PassedFrame(p, 0, 10.0f)) {
                StageObj_K05_Create(p, 0xC, 1);
                return;
            }
            break;
        case 0x57F:
            if (Motion_PassedFrame(p, 0, 124.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xA, 0x26, 1.0f);
                Effect0D_SpawnForPlayer(p, 0xA, 0x25);
                return;
            }
            break;
        case 0x581:
            Player_EffectAtFrame(p, 34.0f, 5, 3);
            Player_EffectAtFrame(p, 60.0f, 5, 1);
            Player_EffectAtFrame(p, 72.0f, 8, 8);
            return;
        case 0x582:
            Player_EffectAtFrame(p, 34.0f, 8, 3);
            Player_EffectAtFrame(p, 60.0f, 8, 1);
            Player_EffectAtFrame(p, 72.0f, 5, 8);
            return;
        case 0x583:
            Player_EffectAtFrame(p, 18.0f, 5, 8);
            Player_EffectAtFrame(p, 28.0f, 8, 3);
            if (Motion_PassedFrame(p, 0, 14.0f)) {
                StageObj_K05_Create(p, 0xD, 2);
            }
            if (Motion_PassedFrame(p, 0, 24.0f)) {
                StageObj_K05_Create(p, 0xA, 1);
                return;
            }
            break;
        case 0x584:
            Player_EffectAtFrame(p, 18.0f, 5, 3);
            return;
        case 0x585:
            Player_EffectAtFrame(p, 16.0f, 5, 3);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 7:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 4.0f, 5, 1);
            Player_EffectAtFrame(p, 26.0f, 8, 1);
            return;
        case 0x3EE:
            Player_EffectAtFrame(p, 24.0f, 0xA, 6);
            return;
        case 0x3EF:
            Player_EffectAtFrame(p, 24.0f, 0xA, 6);
            return;
        case 0x579:
            if (Motion_PassedFrame(p, 0, 62.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xE, 0xC, 1.2f);
            }
            Player_EffectAtFrame(p, 28.0f, 5, 1);
            Player_EffectAtFrame(p, 34.0f, 8, 8);
            Player_EffectAtFrame(p, 58.0f, 8, 3);
            return;
        case 0x57A:
            if (Motion_PassedFrame(p, 0, 58.0f)) {
                Effect0D_SpawnForPlayerScaled(p, 0xE, 0xC, 1.2f);
            }
            Player_EffectAtFrame(p, 30.0f, 8, 8);
            Player_EffectAtFrame(p, 54.0f, 8, 3);
            return;
        case 0x57B:
            Player_EffectAtFrame(p, 42.0f, 8, 1);
            Player_EffectAtFrame(p, 68.0f, 8, 3);
            Player_EffectAtFrame(p, 80.0f, 5, 8);
            return;
        case 0x57C:
            Player_EffectAtFrame(p, 50.0f, 8, 3);
            return;
        case 0x57D:
            Player_EffectAtFrame(p, 22.0f, 5, 8);
            Player_EffectAtFrame(p, 28.0f, 8, 3);
            return;
        case 0x57E:
            Player_EffectAtFrame(p, 26.0f, 8, 8);
            Player_EffectAtFrame(p, 34.0f, 8, 3);
            Player_EffectAtFrame(p, 38.0f, 5, 1);
            return;
        case 0x57F:
            Player_EffectAtFrame(p, 22.0f, 5, 8);
            Player_EffectAtFrame(p, 26.0f, 8, 3);
            return;
        case 0x580:
            Player_EffectAtFrame(p, 14.0f, 8, 3);
            Player_EffectAtFrame(p, 28.0f, 5, 8);
            Player_EffectAtFrame(p, 36.0f, 8, 3);
            Player_EffectAtFrame(p, 68.0f, 5, 8);
            if (Motion_PassedFrame(p, 0, 94.0f)) {
                StageObj_K29_CreateOnPlayerDefault(p, 0xA, 1.0f);
                return;
            }
            break;
        case 0x585:
            Player_EffectAtFrame(p, 26.0f, 8, 1);
            Player_EffectAtFrame(p, 44.0f, 5, 3);
            Player_EffectAtFrame(p, 50.0f, 8, 8);
            return;
        case 0x586:
            Player_EffectAtFrame(p, 6.0f, 8, 3);
            Player_EffectAtFrame(p, 42.0f, 8, 3);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 8:
        switch (w->unk6) {
        case 0x3EA:
            Player_EffectAtFrame(p, 60.0f, 8, 1);
            return;
        case 0x3EB:
            Player_EffectAtFrame(p, 60.0f, 5, 8);
            return;
        case 0x3EC:
            Player_EffectAtFrame(p, 2.0f, 8, 1);
            Player_EffectAtFrame(p, 28.0f, 5, 1);
            return;
        case 0x3F3:
            Player_EffectAtFrame(p, 30.0f, 0xA, 6);
            return;
        case 0x3F4:
            Player_EffectAtFrame(p, 24.0f, 0xA, 6);
            return;
        case 0x3F5:
            Player_EffectAtFrame(p, 24.0f, 0xA, 6);
            return;
        case 0x579:
            Player_EffectAtFrame(p, 14.0f, 8, 8);
            return;
        case 0x57A:
            Player_EffectAtFrame(p, 14.0f, 8, 8);
            return;
        case 0x57B:
            if (Motion_PassedFrame(p, 0, 84.0f)) {
                Player_SpawnGroundEffect(p, 0x13, 0.5f);
                return;
            }
            break;
        case 0x57C:
            Player_EffectAtFrame(p, 14.0f, 8, 8);
            return;
        case 0x580:
            if (Motion_PassedFrame(p, 0, 48.0f)) {
                Player_SpawnGroundEffect(p, 0xC, 1.5f);
            }
            Player_EffectAtFrame(p, 70.0f, 8, 1);
            Player_EffectAtFrame(p, 92.0f, 8, 1);
            Player_EffectAtFrame(p, 110.0f, 5, 8);
            return;
        case 0x581:
            if (Motion_PassedFrame(p, 0, 50.0f)) {
                Player_SpawnGroundEffect(p, 0xC, 1.2f);
            }
            Player_EffectAtFrame(p, 50.0f, 5, 8);
            Player_EffectAtFrame(p, 64.0f, 5, 8);
            Player_EffectAtFrame(p, 80.0f, 8, 3);
            return;
        case 0x582:
            if (Motion_PassedFrame(p, 0, 50.0f)) {
                Player_SpawnGroundEffect(p, 0xC, 1.2f);
            }
            Player_EffectAtFrame(p, 46.0f, 5, 8);
            Player_EffectAtFrame(p, 68.0f, 8, 8);
            Player_EffectAtFrame(p, 88.0f, 5, 1);
            return;
        case 0x583:
            if (Motion_PassedFrame(p, 0, 48.0f)) {
                Player_SpawnGroundEffect(p, 0xC, 1.5f);
            }
            Player_EffectAtFrame(p, 46.0f, 5, 8);
            Player_EffectAtFrame(p, 72.0f, 8, 1);
            Player_EffectAtFrame(p, 88.0f, 5, 8);
            Player_EffectAtFrame(p, 102.0f, 8, 8);
            /* fallthrough */
        case 0x585:
            Player_EffectAtFrame(p, 52.0f, 8, 8);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    case 10:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 2.0f, 5, 1);
            Player_EffectAtFrame(p, 28.0f, 8, 1);
            return;
        case 0x3EE:
            Player_EffectAtFrame(p, 16.0f, 5, 1);
            Player_EffectAtFrame(p, 18.0f, 8, 8);
            Player_EffectAtFrame(p, 32.0f, 5, 8);
            Player_EffectAtFrame(p, 42.0f, 8, 3);
            return;
        case 0x3F1:
            Player_EffectAtFrame(p, 2.0f, 5, 1);
            Player_EffectAtFrame(p, 28.0f, 8, 1);
            return;
        case 0x3F2:
            Player_EffectAtFrame(p, 16.0f, 5, 8);
            return;
        case 0x583:
            Player_EffectAtFrame(p, 32.0f, 5, 1);
            Player_EffectAtFrame(p, 48.0f, 8, 3);
            return;
        case 0x585:
            if (Motion_PassedFrame(p, 0, 26.0f)) {
                StageObj_K05_Create(p, 6, 4);
            }
            Player_EffectAtFrame(p, 34.0f, 5, 8);
            return;
        case 0x586:
            if (Motion_PassedFrame(p, 0, 32.0f)) {
                StageObj_K05_Create(p, 6, 4);
            }
            Player_EffectAtFrame(p, 34.0f, 5, 8);
            return;
        default:
            func_00395EC0(p, w);
            return;
        }
        break;
    default:
        switch (w->unk6) {
        case 0x3EC:
            Player_EffectAtFrame(p, 62.0f, 8, 8);
            Player_EffectAtFrame(p, 30.0f, 5, 8);
            return;
        case 0x3F4:
        case 0x3F5:
            if (w->unk6 == 0x3F4) {
                Player_EffectAtFrame(p, 6.0f, 5, 0);
            } else {
                Player_EffectAtFrame(p, 6.0f, 8, 0);
            }
            Player_EffectAtFrame(p, 18.0f, 0xA, 6);
            return;
        case 0x3F8:
            Player_EffectAtFrame(p, 4.0f, 5, 1);
            return;
        case 0x3F9:
            Player_EffectAtFrame(p, 60.0f, 8, 8);
            Player_EffectAtFrame(p, 32.0f, 5, 8);
            Player_EffectAtFrame(p, 74.0f, 5, 8);
            return;
        case 0x583:
            Player_EffectAtFrame(p, 10.0f, 5, 1);
            return;
        case 0x587:
            fr = 64.0f;
            i = 0;
            do {
                if (Motion_PassedFrame(p, 0, fr)) {
                    Effect0D_SpawnForPlayerScaled(p, 5, 3, 1.3f);
                    Effect0D_SpawnForPlayerScaled(p, 8, 3, 1.3f);
                }
                fr += 4.0f;
                i++;
            } while (i < 6);
            return;
        default:
            func_00395EC0(p, w);
            break;
        }
        break;
    }
}
