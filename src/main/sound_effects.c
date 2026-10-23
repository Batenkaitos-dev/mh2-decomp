/*
 * The sound effects' wrappers, which the game
 * calls to play a sound effect. A sound is a bank (SE_SLOT_* in sound.h)
 * and an id in it. The flat ones play at the volume the options set
 * (gSysSettings); the placed ones check that the object is in the current
 * area and hand the sound to Sound_PlaySeAtPos (sound.c), which sets its
 * volume and pan from the distance to the listener by the range's curve
 * (gSePosRanges). Each player plays in its own bank and the monsters in
 * the monsters' bank, where Sound_SetSeIds tags the playing sound with the
 * monster's type and slot. Sound_PlayArmorRattle plays a player's
 * armour rattle, picked by the pieces worn.
 */
#include "common.h"
#include "syssettings.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "item.h"
#include "sound.h"
#include "bgm.h"
#include "rand.h"
#include "area.h"

/* Plays a sound unplaced and unpanned at the options' sound-effect volume
 * (gSndVolumeScale's step for gSysSettings.seVolume, of 127); nothing for
 * SE_ID_NONE or a volume of 0. */
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog)
{
    s32 vol;

    if (id != SE_ID_NONE) {
        vol = 127.0f * gSndVolumeScale[gSysSettings.seVolume];
        if (vol != 0) {
            Sound_PlaySeVolPan(slot, id, vol, 0, 0, prog);
        }
    }
}

/* Sound_PlaySeFlat with a pan. */
void Sound_PlaySeFlatPan(u32 slot, u32 id, s32 prog, s32 pan)
{
    s32 vol;

    if (id != SE_ID_NONE) {
        vol = 127.0f * gSndVolumeScale[gSysSettings.seVolume];
        if (vol != 0) {
            Sound_PlaySeVolPan(slot, id, vol, pan, 0, prog);
        }
    }
}

/* Sound_PlaySeFlat at the options' music volume (gSysSettings.musicVolume). */
void Sound_PlaySeMusicVol(u32 slot, u32 id, s32 prog)
{
    s32 vol;

    if (id != SE_ID_NONE) {
        vol = 127.0f * gSndVolumeScale[gSysSettings.musicVolume];
        if (vol != 0) {
            Sound_PlaySeVolPan(slot, id, vol, 0, 0, prog);
        }
    }
}

/*
 * Plays a sound placed at pos (Sound_PlaySeAtPos): range picks the volume's
 * curve by distance (gSePosRanges), and update != 0 only moves the pan of
 * the sound already playing. Returns -1 for SE_ID_NONE, else
 * Sound_PlaySeAtPos's result.
 */
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 prog, void *pos, s32 range, s32 update)
{
    if (id == SE_ID_NONE) {
        return -1;
    }
    return Sound_PlaySeAtPos(slot, id, prog, pos, range, update);
}

/* Picks a sound by a roll of 0-7: id below idWeight, alt below
 * idWeight + altWeight, SE_ID_NONE above (no sound). */
s32 Sound_RandomSe(s32 id, s16 idWeight, s32 alt, s16 altWeight)
{
    s16 roll = Rand_Next(1) & 7;

    if (roll < idWeight) {
        return id;
    }
    if (roll < idWeight + altWeight) {
        return alt;
    }
    alt = SE_ID_NONE;
    return alt;
}

/* Plays a sound of the shared bank at pos when the object is in the
 * current area. */
void Sound_PlayPlayerSe(void *obj, s32 id, s32 prog, void *pos, s32 range, s32 update)
{
    ObjHead *head = obj;
    if (Obj_InCurrentArea(head) & 0xFF) {
        Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, id, prog, pos, range, update);
    }
}

/*
 * Plays the rattle of a player's armour, at the player, in the shared
 * bank. Each armour piece has a rattle per gender and piece kind
 * (gArmorRattleKinds[gender + type * 2][Player.armourKind[type]]; 0 is
 * silent), played as one of its two variants (rattle * 2 + 0 or 1).
 * kind is EQUIP_RATTLE_ALL (the legs', body's and waist's, each distinct
 * rattle once) or one piece's type: EQUIP_BODY and EQUIP_WAIST play only
 * when the piece sounds unlike the ones before it, EQUIP_ARMS always.
 */
void Sound_PlayArmorRattle(Player *pl, s32 kind)
{
    s8 legs = gArmorRattleKinds[pl->gender + EQUIP_LEGS * 2][pl->armourKind[EQUIP_LEGS]];
    s8 waist = gArmorRattleKinds[pl->gender + EQUIP_WAIST * 2][pl->armourKind[EQUIP_WAIST]];
    s8 *rattles;
    s8 body = gArmorRattleKinds[pl->gender + EQUIP_BODY * 2][pl->armourKind[EQUIP_BODY]];

    if (Obj_InCurrentArea(pl) & 0xFF) {
        switch (kind) {
        case EQUIP_RATTLE_ALL:
            if (legs != body && body != 0) {
                Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, body * 2 + (Rand_Next(1) & 1), 0, pl->pos, 14, 0);
            }
            if (legs != waist && body != waist && waist != 0) {
                Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, waist * 2 + (Rand_Next(1) & 1), 0, pl->pos, 14, 0);
            }
            if (legs != 0) {
                Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, legs * 2 + (Rand_Next(1) & 1), 0, pl->pos, 14, 0);
            }
            break;
        case EQUIP_BODY:
            if (body != 0 && body != legs) {
                Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, body * 2 + (Rand_Next(1) & 1), 0, pl->pos, 14, 0);
            }
            break;
        case EQUIP_WAIST:
            if (waist != 0 && waist != body && waist != legs) {
                Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, waist * 2 + (Rand_Next(1) & 1), 0, pl->pos, 14, 0);
            }
            break;
        case EQUIP_ARMS:
            rattles = gArmorRattleKinds[pl->gender + kind * 2];
            Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, rattles[pl->armourKind[kind]] * 2 + (Rand_Next(1) & 1), 0, pl->pos, 14, 0);
            break;
        }
    }
}

/* Plays a sound at pos in the player's own bank (in the monsters' bank
 * for a monster run as a player, isEm) when it is in the current area. */
void Player_PlaySe(Player *pl, s32 id, s32 prog, void *pos, s32 range, s32 update)
{
    if (Obj_InCurrentArea(pl) & 0xFF) {
        Sound_PlaySeAtPosChecked(pl->isEm != 0 ? SE_SLOT_EM : SE_SLOT_PLAYER(pl->index), id, prog, pos, range, update);
    }
}

/* Plays a sound of the shared bank at pos for a monster, tagged with its
 * type and slot (Sound_SetSeIds). Returns -1 for SE_ID_NONE or when the
 * monster is out of the current area. */
s32 Sound_PlayEmSe(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update)
{
    if (id == SE_ID_NONE) {
        return -1;
    }
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return -1;
    }
    Sound_SetSeIds(SE_SLOT_COMMON, id, em->type, em->slotNo, range);
    return Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, id, prog, pos, range, update);
}

/* Em_PlaySeAtPos when the monster is in the current area; -1 otherwise. */
s32 Em_PlaySeChecked(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update)
{
    if ((Player_InCurrentArea(em) & 0xFF) == 0) {
        return -1;
    }
    return Em_PlaySeAtPos(em, id, prog, pos, range, update);
}

/*
 * Plays a sound at pos for a monster: in the monsters' bank, tagged with
 * its type and slot, or in the player's own bank for a player (isEm 0).
 * The program is the type's (gSeBankByType); prog is unused. Returns -1
 * for SE_ID_NONE.
 */
s32 Em_PlaySeAtPos(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update)
{
    s32 slot;

    if (id == SE_ID_NONE) {
        return -1;
    }
    if (em->isEm != 0) {
        slot = SE_SLOT_EM;
        Sound_SetSeIds(slot, id, em->type, em->slotNo, range);
    } else {
        slot = SE_SLOT_PLAYER(em->index);
    }
    return Sound_PlaySeAtPosChecked(slot, id, gSeBankByType[em->type], pos, range, update);
}

/* Plays a sound of the monsters' bank at pos, with the type's program
 * (gSeBankByType), tagged with the monster's type + 100 and its slot
 * (other wrappers tag the type itself). Returns -1 for SE_ID_NONE or when
 * the monster is out of the current area; prog is unused. */
s32 Em_PlaySeType(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update)
{
    if (id == SE_ID_NONE) {
        return -1;
    }
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return -1;
    }
    Sound_SetSeIds(SE_SLOT_EM, id, em->type + 100, em->slotNo, range);
    return Sound_PlaySeAtPosChecked(SE_SLOT_EM, id, gSeBankByType[em->type], pos, range, update);
}

/* Plays a sound of the monsters' bank at pos, untagged, when the monster
 * is in the current area. */
void Em_PlayEmBankSe(Em *em, s32 id, void *pos, s32 range)
{
    if (Player_InCurrentArea(em) & 0xFF) {
        Sound_PlaySeAtPosChecked(SE_SLOT_EM, id, 0, pos, range, 0);
    }
}

/* Plays a sound of the shared bank at pos, untagged, when the monster is
 * in the current area. */
void Em_PlayCommonSe(Em *em, s32 id, void *pos, s32 range)
{
    if (Player_InCurrentArea(em) & 0xFF) {
        Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, id, 0, pos, range, 0);
    }
}

/* This file's initialised data. */
f32 gSndVolumeScale[8] = {
#include "main/001BCE20.inc"
};
/* Zeros after the table: nothing reads them. */
#pragma explicit_zero_data on
u32 D_001BCE40[16] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};
#pragma explicit_zero_data reset
