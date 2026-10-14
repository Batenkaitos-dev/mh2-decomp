/*
 * main 0x194830-0x195230
 */
#include "common.h"
#include "syssettings.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
extern s8 *gArmorRattleKinds[];

extern f32 gSndVolumeScale[];
extern s8 gSeBankByType[];
s32 Sound_PlaySeVolPan(u32 slot, u32 id, s32 vol, s32 pan, s32 unused, s32 prog);
void Sound_SetSeIds(u32 slot, u32 id, u8 id2, u8 id3, s32 table);
s32 Sound_PlaySeAtPos(u32 slot, u32 id, s32 prog, void *pos, s32 kind, s32 update);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Em_PlaySeAtPos(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
u16 Rand_Next(s32 stream);
s32 Obj_InCurrentArea(void *obj);
u8 Player_InCurrentArea(void *obj); /* the definition (sub_main) has (Player *p): the Em is passed */

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog)
{
    s32 vol;

    if (id != 0xFFFF) {
        vol = 127.0f * gSndVolumeScale[gSysSettings.seVolume];
        if (vol != 0) {
            Sound_PlaySeVolPan(slot, id, vol, 0, 0, prog);
        }
    }
}

void Sound_PlaySeFlatPan(u32 slot, u32 id, s32 prog, s32 pan)
{
    s32 vol;

    if (id != 0xFFFF) {
        vol = 127.0f * gSndVolumeScale[gSysSettings.seVolume];
        if (vol != 0) {
            Sound_PlaySeVolPan(slot, id, vol, pan, 0, prog);
        }
    }
}

void Sound_PlaySeMusicVol(u32 slot, u32 id, s32 prog)
{
    s32 vol;

    if (id != 0xFFFF) {
        vol = 127.0f * gSndVolumeScale[gSysSettings.musicVolume];
        if (vol != 0) {
            Sound_PlaySeVolPan(slot, id, vol, 0, 0, prog);
        }
    }
}

s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c)
{
    if (id == 0xFFFF) {
        return -1;
    }
    return Sound_PlaySeAtPos(slot, id, a, pos, b, c);
}

s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n)
{
    s16 r = Rand_Next(1) & 7;

    if (r < lo) {
        return id;
    }
    if (r < lo + n) {
        return alt;
    }
    alt = 0xFFFF;
    return alt;
}

void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c)
{
    ObjHead *o = obj;
    if (Obj_InCurrentArea(o) & 0xFF) {
        Sound_PlaySeAtPosChecked(1, id, a, pos, b, c);
    }
}

/*
 * The rattle of a player's equipment (pl: the game's Player): each armor
 * part has a sound kind per gender (gArmorRattleKinds[gender + part * 2], indexed by
 * the part's type at +0x366 + part), played at the player (+0xAC) as one
 * of two variations. kind 0 plays the parts 3, 5 and 0 (each distinct kind
 * once), 3 and 5 one part unless it sounds like another, 4 part 4.
 */
void Sound_PlayArmorRattle(Player *pl, s32 kind)
{
    s8 a = gArmorRattleKinds[pl->gender][pl->armourKind[0]];
    s8 c = gArmorRattleKinds[pl->gender + 10][pl->armourKind[5]];
    s8 *t;
    s8 b = gArmorRattleKinds[pl->gender + 6][pl->armourKind[3]];

    if (Obj_InCurrentArea(pl) & 0xFF) {
        switch (kind) {
        case 0:
            if (a != b && b != 0) {
                Sound_PlaySeAtPosChecked(1, b * 2 + (Rand_Next(1) & 1), 0, pl->pos, 0xE, 0);
            }
            if (a != c && b != c && c != 0) {
                Sound_PlaySeAtPosChecked(1, c * 2 + (Rand_Next(1) & 1), 0, pl->pos, 0xE, 0);
            }
            if (a != 0) {
                Sound_PlaySeAtPosChecked(1, a * 2 + (Rand_Next(1) & 1), 0, pl->pos, 0xE, 0);
            }
            break;
        case 3:
            if (b != 0 && b != a) {
                Sound_PlaySeAtPosChecked(1, b * 2 + (Rand_Next(1) & 1), 0, pl->pos, 0xE, 0);
            }
            break;
        case 5:
            if (c != 0 && c != b && c != a) {
                Sound_PlaySeAtPosChecked(1, c * 2 + (Rand_Next(1) & 1), 0, pl->pos, 0xE, 0);
            }
            break;
        case 4:
            t = gArmorRattleKinds[pl->gender + kind * 2];
            Sound_PlaySeAtPosChecked(1, t[pl->armourKind[kind]] * 2 + (Rand_Next(1) & 1), 0, pl->pos, 0xE, 0);
            break;
        }
    }
}

void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c)
{
    if (Obj_InCurrentArea(pl) & 0xFF) {
        Sound_PlaySeAtPosChecked(pl->isEm != 0 ? 6 : pl->index + 2, id, a, pos, b, c);
    }
}

s32 Sound_PlayEmSe(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c)
{
    if (id == 0xFFFF) {
        return -1;
    }
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return -1;
    }
    Sound_SetSeIds(1, id, em->type, em->slotNo, b);
    return Sound_PlaySeAtPosChecked(1, id, a, pos, b, c);
}

s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c)
{
    if ((Player_InCurrentArea(em) & 0xFF) == 0) {
        return -1;
    }
    return Em_PlaySeAtPos(em, id, a, pos, b, c);
}

s32 Em_PlaySeAtPos(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c)
{
    s32 slot;

    if (id == 0xFFFF) {
        return -1;
    }
    if (em->isEm != 0) {
        slot = 6;
        Sound_SetSeIds(slot, id, em->type, em->slotNo, b);
    } else {
        slot = em->index + 2;
    }
    return Sound_PlaySeAtPosChecked(slot, id, gSeBankByType[em->type], pos, b, c);
}

s32 Em_PlaySeType(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c)
{
    if (id == 0xFFFF) {
        return -1;
    }
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return -1;
    }
    Sound_SetSeIds(6, id, em->type + 100, em->slotNo, b);
    return Sound_PlaySeAtPosChecked(6, id, gSeBankByType[em->type], pos, b, c);
}

void Em_PlaySeSlot6(Em *em, s32 id, void *pos, s32 b)
{
    if (Player_InCurrentArea(em) & 0xFF) {
        Sound_PlaySeAtPosChecked(6, id, 0, pos, b, 0);
    }
}

void Em_PlaySeSlot1(Em *em, s32 id, void *pos, s32 b)
{
    if (Player_InCurrentArea(em) & 0xFF) {
        Sound_PlaySeAtPosChecked(1, id, 0, pos, b, 0);
    }
}

/* unk_194830's .data (0x1BCE20-0x1BCE80) */
f32 gSndVolumeScale[8] = {
#include "main/001BCE20.inc"
};
#pragma explicit_zero_data on
u32 D_001BCE40[16] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};
#pragma explicit_zero_data reset
