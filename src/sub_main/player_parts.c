/*
 * The players' parts: a player's 32 part records (Player_InitRecords),
 * the mesh shown in a range of meshes (Obj_ShowMeshInRange), and the
 * swing chains of the equipment, by which hair, cloth and the like follow
 * the bones (EquipFx_ChainUpdate): for each visible player
 * (Players_RunEquipEffects, the chains listed for its armour) and for the
 * lobby's actors (Effect_RunFixedList, a fixed list). The data: the swing
 * chains' parameters and their effect points.
 */

#include "common.h"
#include "player.h"
#include "model.h"
#include "lobby_state.h"
#include "scene.h"
#include "capsule.h"
#include "em.h"
#include "engine_math.h"
#include "vec_math.h"

extern PlRec gPlayerRecs[][32];

/* Gives the player its 32 part records (gPlayerRecs; both matrices
 * identity), shows all 32 meshes and sets the scale of its model's first
 * 21 bones (in both bone sets) and of their records to 1. The last loop's
 * chained stores and stepped bone pointers keep its body small enough for
 * MWCC to unroll it by 7, as the original does. */
void Player_InitRecords(Player *pl)
{
    s16 i;
    PlRec *rec;
    EmBone *bone;
    EmBone *altBone;
    Model *body;

    rec = gPlayerRecs[pl->index];
    for (i = 0; i < 32; i++, rec++) {
        pl->partRecs[i] = rec;
        Mtx_Identity(rec->m0);
        Mtx_Identity(rec->m1);
    }
    for (i = 0; i < 32; i++) {
        pl->meshShown[i] = 1;
    }
    body = pl->model;
    bone = body->partBones[0];
    altBone = body->partBones2[0];
    for (i = 0; i < 21; i++) {
        pl->partRecs[i]->scale[0] = bone->base[0] = altBone->base[0] = 1.0f;
        pl->partRecs[i]->scale[1] = bone->base[1] = altBone->base[1] = 1.0f;
        pl->partRecs[i]->scale[2] = bone->base[2] = altBone->base[2] = 1.0f;
        altBone++;
        bone++;
    }
}

/* Shows mesh sel of the range of kind (0x12: meshes 2 and 3, 0xE: 4 and 5)
 * of a player or monster p and hides the range's other mesh: p's
 * meshShown (+0x4C8, at the same place in Player and Em). */
void Obj_ShowMeshInRange(u8 *obj, u8 kind, u8 sel)
{
    u8 first;
    u8 count;
    u8 i;

    switch (kind) {
    case 18:
        count = 2;
        first = count;
        break;
    case 14:
        count = 2;
        first = 4;
        break;
    default:
        return;
    }
    for (i = first; i < first + count; i++) {
        if (i == sel + first) {
            obj[1224 + i] = 1;
        } else {
            obj[1224 + i] = 0;
        }
    }
}

/* Clears the four swing chains of an equipment's effects. */
void FxBlock_Clear(EquipFxSwing *swing)
{
    s16 i;

    i = 0;
    do {
        swing->vel[0][0] = 0.0f;
        swing->vel[0][1] = 0.0f;
        swing->vel[0][2] = 0.0f;
        swing->vel[1][0] = 0.0f;
        swing->vel[1][1] = 0.0f;
        swing->vel[1][2] = 0.0f;
        swing->calls = 0;
        swing->unk01[0] = 0;
        swing->unk01[1] = 0;
        swing->unk01[2] = 0;
        i++;
        swing++;
    } while (i < 4);
}

/* Clears the player's equipment swing chains (FxBlock_Clear). */
void Player_ClearFxBlock(Player *pl)
{
    FxBlock_Clear(pl->equipFx);
}

/* Player_RunEquipEffects for each active, visible player (online, only
 * those whose model the lobby has loaded: gLobby's pcLoaded). */
void Players_RunEquipEffects(void)
{
    s16 i;
    Player *pl;

    for (i = 0, pl = gPlayers; i < 10; i++, pl++) {
        if (pl->active != 0 && pl->visible != 0) {
            if (gScene.inLobby == 0 || (s8)gLobby->pcLoaded[i] != 0) {
                Player_RunEquipEffects(pl);
            }
        }
    }
}

extern s16 gLobbyActorFxIds[];
/* declared without parameters, defined K&R: its caller passes key and alt
 * unconverted, the definition masks them */
void Effect_RunList(EquipFxSwing *a, EmBone *b1, EmBone *b2, EmBone *m, s32 key, s32 alt, s32 kind); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it) */

/* A chain of effect bones (the rows of gEquipFxChains that Effect_RunList lists). */
typedef struct FxChain {
    u8 range;                   /* 0x0: gEquipFxRanges, the bones */
    u8 flags;                   /* 0x1: 1: clamp every bone, not only the first */
    s16 up;                     /* 0x2: gEquipFxUps */
    s16 base;                   /* 0x4: gEquipFxBases */
    s16 limit;                  /* 0x6: gEquipFxLimits, a pair per chain */
    s16 target;                 /* 0x8: gEquipFxTargets, -1: none */
    s16 clamp;                  /* 0xA: gEquipFxClamps, -1: none */
    s16 damp;                   /* 0xC: gEquipFxDamps */
    s16 pull;                   /* 0xE: gEquipFxPulls */
} FxChain;
extern FxChain gEquipFxChains[];
typedef struct FxRange {
    s16 start;
    u16 end;
} FxRange;
extern FxRange gEquipFxRanges[];
extern f32 gEquipFxUps[];
extern f32 gEquipFxBases[][3];
extern f32 gEquipFxLimits[];
extern f32 gEquipFxTargets[][3];
extern f32 gEquipFxClamps[][6];
extern f32 gEquipFxDamps[];
extern f32 gEquipFxPulls[];

#pragma optimization_level 4
/*
 * Moves the count bone chains of list (FxChain rows) on the part's bones:
 * from the third call on (swing->calls; one EquipFxSwing per chain), each
 * bone's direction follows its parent (baseBone first), pulled by
 * gEquipFxPulls, swung by the chain's up vector with its velocity
 * (swing->vel, damped by gEquipFxDamps), pushed off pushBone's target by
 * Vec_ProjectOnDir, and clamped by gEquipFxClamps. Returns 0.
 * FAKEMATCH: two forms only mirror the game's allocation. (1) The copies
 * baseBone, pushBone and bones are made again in the outer loop's step: a
 * plain copy of a parameter is propagated away and spills as the parameter
 * (a 4-byte slot), where the game spills them as locals (16-byte slots
 * 0x100..0xE0); two definitions keep them variables, and the step copies
 * give no code. (2) The clamp reads dir[1] and dir[2] through dy and dz,
 * set before the loop: the game numbers its two hoisted addresses before
 * bone and swingRec (s4, s3 over s2, s1), which MWCC's own hoisting
 * doesn't (docs/notes/nm/EquipFx_ChainUpdate.md).
 */
s32 EquipFx_ChainUpdate(EquipFxSwing *swingsIn, EmBone *baseBoneIn, EmBone *pushBoneIn, EmBone *bonesIn, f32 **list, s16 countIn)
{
    EmBone *baseBone = baseBoneIn;
    EmBone *pushBone = pushBoneIn;
    EmBone *bones = bonesIn;
    EquipFxSwing *swing;
    s32 count = countIn;
    s16 i;
    s16 j;
    s16 k;
    s32 end;
    FxChain **chain;
    f32 *dy;
    f32 *dz;
    EmBone *bone;
    u8 *swingRec;
    s32 clampIdx;
    f32 *clampRow;
    f32 angleX;
    f32 angleZ;
    f32 vec[4];
    f32 wpos[4];
    f32 tip[4];
    f32 upv[4];
    f32 dir[4];
    f32 tgt[4];
    f32 hit[4];
    f32 side[4];
    f32 swingDir[4];
    f32 baseUp[4];
    f32 off[4];
    f32 pos[4];
    f32 cur[16];
    f32 inv[16];
    f32 pushMtx[16];

    i = 0;
    if (i < count) {
    chain = (FxChain **)list;
    swing = swingsIn;
    dy = &dir[1];
    dz = &dir[2];
    do {
        baseUp[0] = gEquipFxBases[(*chain)->base][0];
        baseUp[1] = gEquipFxBases[(*chain)->base][1];
        baseUp[2] = gEquipFxBases[(*chain)->base][2];
        Vec_RotateSelf(baseUp, (f32 *)baseBone);
        if (swing->calls < 2) {
            swing->calls++;
            continue;
        }
        if ((*chain)->target != -1) {
            Vec_Copy(tgt, gEquipFxTargets[(*chain)->target]);
            Mtx_Copy(pushMtx, (f32 *)pushBone);
            Vec_RotateSelf(tgt, pushMtx);
        }
        upv[0] = 0.0f;
        upv[1] = gEquipFxUps[(*chain)->up];
        upv[2] = 0.0f;
        Vec_Add(upv, upv, baseUp);
        k = gEquipFxRanges[(*chain)->range].start;
        end = gEquipFxRanges[(*chain)->range].end;
        j = 0;
        if (k < end) {
            swingRec = (u8 *) swing; /* walked per bone: vel[j] at swingRec + 4 (the game keeps the record's base) */
            bone = &bones[k];
            do {
            Mtx_GetTrans(wpos, bone->world);
            Mtx_GetTrans(off, bone->local);
            if (j == 0) {
                Mtx_Copy(cur, (f32 *)baseBone);
            } else {
                Mtx_Mul(inv, bone->parent->local, cur);
                Mtx_Copy(cur, inv);
            }
            Vec_RotateSelf(off, cur);
            Vec_Add(pos, &cur[12], off);
            Mtx_GetTrans(off, bone[1].local);
            Vec_Rotate(vec, off, (f32 *)bone);
            Vec_Sub(dir, wpos, pos);
            Vec_Scale(dir, dir, gEquipFxPulls[(*chain)->pull]);
            Vec_Add(vec, vec, dir);
            Vec_Cross(side, vec, upv);
            Vec_Cross(swingDir, side, vec);
            Vec_Normalize(swingDir);
            Vec_Scale(swingDir, swingDir, Vec_Dot(swingDir, upv));
            Vec_Add((f32 *)(swingRec + 4), (f32 *)(swingRec + 4), swingDir);
            Vec_Add(vec, vec, (f32 *)(swingRec + 4));
            Vec_Add(tip, pos, vec);
            Mtx_Inverse(inv, cur);
            if ((*chain)->target == -1) {
                dir[0] = -vec[0];
                dir[1] = -vec[1];
                dir[2] = -vec[2];
                Vec_RotateSelf(dir, inv);
                Vec_Normalize(dir);
            } else {
                Mtx_Copy(pushMtx, (f32 *)pushBone);
                if (Vec_ProjectOnDir(tip, &pushMtx[12], tgt, hit, gEquipFxLimits[j + (*chain)->limit * 2]) != 0) {
                    Vec_Add(tip, tip, hit);
                }
                Vec_Sub(dir, pos, tip);
                Vec_RotateSelf(dir, inv);
                Vec_Normalize(dir);
            }
            if (j == 0 || ((*chain)->flags & 1)) {
                clampIdx = (*chain)->clamp;
                if (clampIdx != -1) {
                    clampRow = gEquipFxClamps[(s16)clampIdx];
                    if (dir[0] < clampRow[0]) {
                        dir[0] = clampRow[0];
                    } else if (dir[0] > clampRow[1]) {
                        dir[0] = clampRow[1];
                    }
                    if (*dy < clampRow[2]) {
                        *dy = clampRow[2];
                    } else if (*dy > clampRow[3]) {
                        *dy = clampRow[3];
                    }
                    if (*dz < clampRow[4]) {
                        *dz = clampRow[4];
                    } else if (*dz > clampRow[5]) {
                        *dz = clampRow[5];
                    }
                }
            }
            angleX = Math_Atan2(dir[2], dir[1]);
            angleZ = Math_Atan2(-dir[0], Math_Sqrtf(dir[1] * dir[1] + dir[2] * dir[2]));
            Mtx_Identity(inv);
            Mtx_RotateZ(inv, angleZ);
            Mtx_RotateX(inv, angleX);
            Vec_Copy(&bone->local[0], &inv[0]);
            Vec_Copy(&bone->local[4], &inv[4]);
            Vec_Copy(&bone->local[8], &inv[8]);
            Vec_Scale((f32 *)(swingRec + 4), (f32 *)(swingRec + 4), gEquipFxDamps[(*chain)->damp]);
                k++;
                bone++;
                swingRec += 12;
                j++;
            } while (k < end);
        }
    } while (swing++, chain++, baseBone = baseBoneIn, pushBone = pushBoneIn, bones = bonesIn, ++i < count);
    }
    return 0;
}
#pragma optimization_level reset

/*
 * Runs the swing chains listed for key (the head armour's kind) under kind
 * (the gender) on the part's bones, when it has at least 5 bones: from
 * gEquipFxListsAltHead when the head uses the alternative model (alt 0),
 * else from gEquipFxListsHead. Each list entry names up to four rows of
 * gEquipFxChains (-1 ends it).
 */
void Effect_RunList(swing, baseBone, pushBone, bones, key, alt, kind)
    EquipFxSwing *swing;
    EmBone *baseBone;
    EmBone *pushBone;
    EmBone *bones;
    u16 key;
    u8 alt;
    u8 kind;
{
    f32 *list[4];
    EquipFxList *entry;
    s16 i;
    s16 *ids;
    s16 id;
    s16 listLen;

    if (bones->count > 4) {
        if (alt == 0) {
            entry = gEquipFxListsAltHead[kind];
        } else {
            entry = gEquipFxListsHead[kind];
        }
        ids = NULL;
        for (; entry->key != -1; entry++) {
            if (entry->key == key) {
                ids = entry->ids;
                break;
            }
        }
        if (ids != NULL) {
            i = 0;
            listLen = 0;
            for (; i < 4; i++) {
                if ((id = *ids) == -1) {
                    break;
                }
                list[listLen++] = (f32 *)&gEquipFxChains[id];
                ids++;
            }
            if (listLen != 0) {
                EquipFx_ChainUpdate(swing, baseBone, pushBone, bones, list, listLen);
            }
        }
    }
}

/* Moves the player's swinging equipment parts (equipFx) on the bones of its
 * head armour's model (part EQUIP_HEAD), when it has one: the chains start
 * from bone 20 and are pushed off bone 10. */
void Player_RunEquipEffects(Player *pl)
{
    if (pl->partModels[EQUIP_HEAD] != NULL) {
        s32 headKind = pl->armourKind[EQUIP_HEAD];
        s32 ownModel = Armor_LacksSexModel(pl->equip[EQUIP_HEAD].id, pl->gender) != 1;
        EmBone *baseBone = Em_GetBone(pl, 20);

        Effect_RunList(pl->equipFx, baseBone, Em_GetBone(pl, 10), pl->partModels[EQUIP_HEAD]->bones, headKind, ownModel, pl->gender);
    }
}

/* Runs the swing chains of the fixed list gLobbyActorFxIds on the bones,
 * when there are at least 27 of them. */
void Effect_RunFixedList(EquipFxSwing *swing, EmBone *baseBone, EmBone *pushBone, EmBone *bones)
{
    f32 *list[4];
    s16 i;
    s16 *ids;
    s16 id;
    s16 listLen;

    if (bones->count > 26) {
        ids = gLobbyActorFxIds;
        if (ids != NULL) {
            i = 0;
            listLen = 0;
            for (; i < 4; i++) {
                if ((id = *ids) == -1) {
                    break;
                }
                list[listLen++] = (f32 *)&gEquipFxChains[id];
                ids++;
            }
            if (listLen != 0) {
                EquipFx_ChainUpdate(swing, baseBone, pushBone, bones, list, listLen);
            }
        }
    }
}

/* This file's initialised data. */
FxRange gEquipFxRanges[8] = {
#include "sub_main/0042F4A0.inc"
};
f32 gEquipFxUps[4] = {
#include "sub_main/0042F4C0.inc"
};
f32 gEquipFxBases[14][3] = {
#include "sub_main/0042F4D0.inc"
};
f32 gEquipFxLimits[12] = {
#include "sub_main/0042F580.inc"
};
f32 gEquipFxTargets[4][3] = {
#include "sub_main/0042F5B0.inc"
};
f32 gEquipFxClamps[16][6] = {
#include "sub_main/0042F5E0.inc"
};
f32 gEquipFxDamps[4] = {
#include "sub_main/0042F760.inc"
};
f32 gEquipFxPulls[4] = {
#include "sub_main/0042F770.inc"
};
FxChain gEquipFxChains[56] = {
#include "sub_main/0042F780.inc"
};
