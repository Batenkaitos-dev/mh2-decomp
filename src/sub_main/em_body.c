/*
 * The monsters' bodies: their target, parts, skeletons and riding.
 *
 * - Helpers every family calls: the nearest player in range (Em_FindTargetPlayer), the breakable
 *   parts' hit points (Em_InitParts, Em_ResetPartHp, and Em_SetPartScaled with Em_ScalePartHp for
 *   the levels that raise them) and the bone accessors (Em_GetBonePos, Em_GetBone, Em_BoneMatrix
 *   and their second names).
 * - Bones: Em_UpdateMatrix builds a monster's matrix (scale, angles, position) and its bones'
 *   (Bone_UpdateTree, after Em_AdjustKindBones' per-kind poses: the Rathalos family's neck bend,
 *   the Kut-Ku's ears, the Plesioth's fins ...); Em_CopyBoneMatrices keeps a copy of the matrices
 *   (gEmBoneMtxBuf) that Em_PosInFrame uses.
 * - Riding: the large monsters a hunter can stand on (the Lao-Shan Lung's, the Shen Gaoren's and the
 *   Yama Tsukami's kinds) list quads on their bones (gEmRideQuads). Em_CarryRiders carries the hunters
 *   standing on one and lands the others that fall onto it; Em_HitQuadsVertical finds the quad under a
 *   point. Tri_SegmentHit and Tri_ContainsPointAngles test the quads' triangles.
 * - Em_RunAll runs every active monster's update and gives it a draw slot; after it, two small
 *   lookups by monster kind (EmKind_MotionGroupCount, EmSlot_FindKind).
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "em_hermitaur.h"
#include "fxobj.h"
#include "objhead.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "gm_em.h"
#include "stage_data.h"
#include "vibration.h"
#include "area.h"
#include "vec_math.h"
#include "em_yamatsukami.h"
#include "equip_preview.h"
#include "fishing.h"

/* Built at optimization level 4: Tri_SegmentHit and Em_ResetPartHp compile as the game's code only
 * at that level. */
#pragma optimization_level 4

struct SparkEmitter;
struct FxObj;
extern Em gEms[];
struct SparkObj;
/* end of prototypes */

/* the calls cast to u16: this unit's code takes the value as u16 */

static void Em_ScalePartHp(Em *em, s32 i, s16 hp);

/* Picks the nearest of the players in the monster's area, within maxHeight and maxDist (0: any) and
 * within range of its facing. Sets target, targetNo, targetDist and targetAngle; returns the
 * player's index, or -1. */
s8 Em_FindTargetPlayer(Em *em, u16 range, f32 maxDist, f32 maxHeight)
{
    f32 dy;
    f32 bestDist;
    f32 x;
    f32 y;
    f32 z;
    f32 flatDist;
    s32 i;
    s32 best;
    Player *player;
    u16 ang;

    best = -1;
    player = gPlayers;
    em->target = NULL;
    bestDist = 0.0f;
    em->targetNo = -1;
    em->targetDist = 0.0f;
    x = em->posX;
    y = em->posY;
    z = em->posZ;
    for (i = 0; i < SCENE_PLAYERS; i++, player++) {
        if (em->area == player->area) {
            dy = player->pos[1] - y;
            if (maxHeight == 0.0f || Math_Absf(dy) <= maxHeight) {
                f32 dx = player->pos[0] - x;
                f32 dz = player->pos[2] - z;

                flatDist = Math_Sqrtf(dx * dx + dz * dz);
                if (maxDist == 0.0f || flatDist <= maxDist) {
                    dy = Math_Sqrtf(flatDist * flatDist + dy * dy);
                    ang = Math_Heading(x, z, player->pos[0], player->pos[2]) - ANGLE_TURN / 4;
                    if ((u16)(ang + range - em->rotY) <= range * 2) {
                        if (bestDist == 0.0f) {
                            bestDist = dy;
                        } else if (dy <= bestDist) {
                            bestDist = dy;
                        } else {
                            continue;
                        }
                        best = i;
                    }
                }
            }
        }
    }
    if (best != -1) {
        em->target = &gPlayers[best];
        em->targetNo = best;
        em->targetDist = bestDist;
        em->targetAngle = (u16)(Math_Heading(x, z, ((Player *)em->target)->pos[0], ((Player *)em->target)->pos[2]) - ANGLE_TURN / 4);
    }
    return (u8)best;
}

/* Sets bit `bit` of partMask, which lets Em_ScalePartHp scale that part's hp. */
void Em_SetPartScaled(Em *em, s32 bit)
{
    em->partMask |= 1 << bit;
}

/* Adds to part i's hp its table value baseHp times 0.33 + 0.08 * (level - 11) / 3, when the
 * part's partMask bit is set and the monster's level is 11 to 17. Static (declared so above): the
 * callers keep their values in the registers the game's code uses only for a static callee. */
void Em_ScalePartHp(Em *em, s32 i, s16 baseHp)
{
    if (em->partMask & (1 << i)) {
        if (em->level >= 11 && em->level < 18) {
            baseHp = baseHp * (0.33f + 0.08f * ((em->level - 11) / 3.0f));
            em->parts[i].hp += baseHp;
        }
    }
}

/* Sets up the 9 breakable parts from the hp table of the monster's type (gEmPartHpTables; 0 or
 * less: no part), and partDamageMode from the value after them. */
void Em_InitParts(Em *em)
{
    s32 i;
    s16 *tbl = gEmPartHpTables[em->type];

    for (i = 0; i < 9; tbl++, i++) {
        if (*tbl > 0) {
            em->parts[i].active = 1;
            em->parts[i].index = i;
            em->parts[i].unk2 = 0;
            em->parts[i].unk3 = 0;
            em->parts[i].hp = *tbl;
            Em_ScalePartHp(em, i, *tbl);
            em->parts[i].state = 0;
        } else {
            em->parts[i].active = 0;
        }
    }
    em->partDamageMode = *tbl;
}

/* Resets part i's hp to its table value, scaled as Em_InitParts does, when i is a part in use.
 * The table pointer is stepped (tbl += i) rather than indexed: only so does it compile as the
 * game's code. */
void Em_ResetPartHp(Em *em, u32 i)
{
    s16 *tbl;

    if (i >= 9 || em->parts[i].active == 0) {
        return;
    }
    tbl = gEmPartHpTables[em->type];
    tbl += i;
    em->parts[i].hp = *tbl;
    Em_ScalePartHp(em, i, *tbl);
}

/* Copies the position of bone `bone`'s world matrix to out. */
void Em_GetBonePos(void *obj, s16 bone, f32 *out)
{
    ObjHead *head = obj;

    Mtx_GetTrans(out, head->model->bones[bone].world);
}

/* Em_GetBonePos again, under a second name. */
void Em_BonePos(void *obj, s16 bone, f32 *out)
{
    Em_GetBonePos(obj, bone, out);
}

/* Bone `bone` of the object's model. */
EmBone *Em_GetBone(void *obj, s16 bone)
{
    ObjHead *head = obj;

    return &head->model->bones[bone];
}

/* Em_GetBone again, under a second name. */
EmBone *Em_Bone(void *obj, s16 bone)
{
    return Em_GetBone(obj, bone);
}

/* The local matrix of bone `bone`. */
f32 *Em_BoneMatrix(void *obj, s16 bone)
{
    ObjHead *head = obj;

    return head->model->bones[bone].local;
}

/* Em_BoneMatrix again, under a second name. */
f32 *Em_BoneMatrixB(void *obj, s16 bone)
{
    return Em_BoneMatrix(obj, bone);
}

/* 1 when p is inside triangle tri (three points): the angles the sides subtend at p add up to
 * a full turn (at least ANGLE_TURN * 15 / 16). */
s32 Tri_ContainsPointAngles(f32 *tri, f32 *point)
{
    f32 toA[3];
    f32 toB[3];
    f32 toC[3];
    f32 angleAB;
    f32 angleBC;
    f32 angleCA;

    toA[0] = tri[0] - point[0];
    toA[1] = tri[1] - point[1];
    toA[2] = tri[2] - point[2];
    toB[0] = tri[3] - point[0];
    toB[1] = tri[4] - point[1];
    toB[2] = tri[5] - point[2];
    toC[0] = tri[6] - point[0];
    toC[1] = tri[7] - point[1];
    toC[2] = tri[8] - point[2];
    Vec_Normalize(toA);
    Vec_Normalize(toB);
    Vec_Normalize(toC);
    angleAB = Vec_Dot(toA, toB);
    angleBC = Vec_Dot(toB, toC);
    angleCA = Vec_Dot(toC, toA);
    if (angleAB > 1.0f) {
        angleAB = 1.0f;
    }
    if (angleBC > 1.0f) {
        angleBC = 1.0f;
    }
    if (angleCA > 1.0f) {
        angleCA = 1.0f;
    }
    if (angleAB < -1.0f) {
        angleAB = -1.0f;
    }
    if (angleBC < -1.0f) {
        angleBC = -1.0f;
    }
    if (angleCA < -1.0f) {
        angleCA = -1.0f;
    }
    angleAB = Math_Acos(angleAB);
    angleBC = Math_Acos(angleBC);
    angleCA = Math_Acos(angleCA);
    if ((u16)Math_RadToAngle(angleAB) + (u16)Math_RadToAngle(angleBC) + (u16)Math_RadToAngle(angleCA) >= ANGLE_TURN * 15 / 16) {
        return 1;
    }
    return 0;
}

/* Where segment start-end crosses the plane of triangle tri (three points): out, and returns 1 when
 * that point is inside the triangle and between start and end; returns 2 when start lowered by 30
 * lies on the plane inside the triangle (out is that point); else 0. */
s32 Tri_SegmentHit(f32 *tri, f32 *start, f32 *end, f32 *out)
{
    f32 toStart[3];
    f32 toEnd[3];
    f32 normal[3];
    f32 edgeA[3];
    f32 edgeB[3];
    s32 result = 0;
    f32 *normalY;
    f32 *normalZ;
    f32 planeDist;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 along;

    edgeA[0] = tri[0] - tri[3];
    edgeA[1] = tri[1] - tri[4];
    edgeA[2] = tri[2] - tri[5];
    edgeB[0] = tri[6] - tri[3];
    edgeB[1] = tri[7] - tri[4];
    edgeB[2] = tri[8] - tri[5];
    Vec_Cross(normal, edgeB, edgeA);
    Vec_Normalize(normal);
    normalY = &normal[1];
    normalZ = &normal[2];
    planeDist = -(normal[0] * tri[0]) - *normalY * tri[1] - *normalZ * tri[2];
    dx = end[0] - start[0];
    dy = end[1] - start[1];
    dz = end[2] - start[2];
    if (planeDist + (normal[0] * start[0] + *normalY * (start[1] - 30.0f) + *normalZ * start[2]) == 0.0f) {
        out[0] = start[0];
        out[1] = start[1] - 30.0f;
        out[2] = start[2];
        if (Tri_ContainsPointAngles(tri, out)) {
            return 2;
        }
    }
    along = -(planeDist + (normal[0] * start[0] + *normalY * start[1] + *normalZ * start[2]));
    along /= normal[0] * dx + *normalY * dy + *normalZ * dz;
    out[0] = start[0] + dx * along;
    out[1] = start[1] + dy * along;
    out[2] = start[2] + dz * along;
    toStart[0] = start[0] - out[0];
    toStart[1] = start[1] - out[1];
    toStart[2] = start[2] - out[2];
    toEnd[0] = end[0] - out[0];
    toEnd[1] = end[1] - out[1];
    toEnd[2] = end[2] - out[2];
    Vec_Normalize(toStart);
    Vec_Normalize(toEnd);
    if (Vec_Dot(toEnd, toStart) < 0.0f && Tri_ContainsPointAngles(tri, out)) {
        result = 1;
    }
    return result;
}

extern f32 gEmBoneMtxBuf[][16];
extern f32 gEmBonePosBuf[][3];

/* Copies each bone matrix of an active monster to gEmBoneMtxBuf[i] (Mtx_Copy) and its
 * translation to gEmBonePosBuf[i]. */
void Em_CopyBoneMatrices(Em *em)
{
    s16 i;
    EmBone *bone;
    s16 n;

    if (em->active == 0 || em->visible == 0) {
        return;
    }
    bone = em->model->bones;
    n = bone->count;
    for (i = 0; i < n; i++, bone++) {
        gEmBonePosBuf[i][0] = bone->world[12];
        gEmBonePosBuf[i][1] = bone->world[13];
        gEmBonePosBuf[i][2] = bone->world[14];
        Mtx_Copy(gEmBoneMtxBuf[i], bone->world);
    }
}

/* The per-kind poses set on the bones before the skeleton is updated (Em_UpdateMatrix):
 * - the Lao-Shan Lungs, the Rathians and the Rathaloses: the four bend bones (gEmBendOrigins' row)
 *   turn by bendAngle;
 * - the Kut-Kus and the Garuga: the ears (bones 37-39) open over 30 frames while the monster is
 *   in combat with at least a fifth of its health, and close otherwise (EmFlyerWork.earOpen);
 * - the Khezus: bone 37 takes KHEZU_BONE_HEAD's rotation, transposed;
 * - the Kirin: bones 24 and 25 spin about X (a turn every 16 frames, the second 5 frames behind),
 *   and stop in state group 5;
 * - the Plesioths and the Cephadrome family: the fin bones fold and wave (EmPlesiothWork.finRot);
 * - the Shen Gaoren: bones 11-14 stretch by its work's ease values;
 * - kind 56: bone 2 spins on all three axes;
 * - the Yama Tsukamis: EmYamaTsukami_ScaleBones;
 * - the Shogun Ceanataur: its claws (HERM_BONE_CLAW_A, HERM_BONE_CLAW_B) scale and turn by
 *   EmHermitaur_RageFxRatio, one way calm and the other enraged;
 * - the Rajang: bone 15 swells with its rage timer. */
void Em_AdjustKindBones(Em *em, EmBone *boneList)
{
    EmBone *bones = boneList;
    f32 mtx[16];
    f32 transposed[16];
    EmBendRow *bend;
    EmFlyerWork *flyer;
    EmPlesiothWork *finWork;
    f32 *boneMtx;
    f32 phase;
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;

    bend = gEmBendOrigins[em->type];
    switch (em->type) {
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
        Mtx_RotateY(bones[bend->bendBone[3]].local, Math_Angle16ToRad((u16)em->bendAngle[3]));
        Mtx_RotateY(bones[bend->bendBone[2]].local, Math_Angle16ToRad((u16)em->bendAngle[2]));
        Mtx_RotateY(bones[bend->bendBone[1]].local, Math_Angle16ToRad((u16)em->bendAngle[1]));
        Mtx_RotateY(bones[bend->bendBone[0]].local, Math_Angle16ToRad((u16)em->bendAngle[0]));
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_YIAN_GARUGA:
        flyer = (EmFlyerWork *)&em->work;
        if (em->combat != 1 || em->hp < (s16)(0.2f * em->hpMax)) {
            if (flyer->earOpen > 0) {
                flyer->earOpen--;
            }
        } else {
            if (flyer->earOpen < 30) {
                flyer->earOpen++;
            }
        }
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 37));
        Mtx_ClearRotation(mtx);
        Mtx_RotateX(mtx, -1.308997f + 0.043633234f * (u32)flyer->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 37), mtx);
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 38));
        Mtx_ClearRotation(mtx);
        Mtx_RotateY(mtx, 0.5235988f - 0.017453292f * (u32)flyer->earOpen);
        Mtx_RotateZ(mtx, 0.87266463f - 0.072722055f * (u32)flyer->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 38), mtx);
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 39));
        Mtx_ClearRotation(mtx);
        Mtx_RotateY(mtx, -0.5235988f + 0.017453292f * (u32)flyer->earOpen);
        Mtx_RotateZ(mtx, -0.87266463f + 0.072722055f * (u32)flyer->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 39), mtx);
        break;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        Mtx_Copy(transposed, Em_BoneMatrixB(em, 35));
        Mtx_Transpose3x3(transposed);
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 37));
        Mtx_CopyRot(mtx, transposed);
        Mtx_Copy(Em_BoneMatrixB(em, 37), mtx);
        break;
    case EM_KIND_KIRIN:
        if (em->stateGroup != 5) {
            Mtx_Copy(mtx, Em_BoneMatrixB(em, 24));
            Mtx_ClearRotation(mtx);
            Mtx_RotateX(mtx, (6.2831855f * (f32)-((gScene.frame & 0xF) << 12)) / 65536.0f);
            Mtx_CopyRot(Em_BoneMatrixB(em, 24), mtx);
            Mtx_Copy(mtx, Em_BoneMatrixB(em, 25));
            Mtx_ClearRotation(mtx);
            Mtx_RotateX(mtx, (6.2831855f * (f32)-(((gScene.frame & 0xF) - 5) << 12)) / 65536.0f);
            Mtx_CopyRot(Em_BoneMatrixB(em, 25), mtx);
        } else {
            Mtx_ClearRotation(Em_BoneMatrixB(em, 24));
            Mtx_ClearRotation(Em_BoneMatrixB(em, 25));
        }
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        bones[37].pose[0] = 1.0f;
        bones[37].pose[1] = 1.0f;
        bones[37].pose[2] = 1.0f;
        bones[38].pose[0] = 1.0f;
        bones[38].pose[1] = 1.0f;
        bones[38].pose[2] = 1.0f;
        bones[39].pose[0] = 1.0f;
        bones[39].pose[1] = 1.0f;
        bones[39].pose[2] = 1.0f;
        bones[16].pose[0] = 1.0f;
        bones[16].pose[1] = 1.0f;
        bones[16].pose[2] = 1.0f;
        finWork = (EmPlesiothWork *)&em->work;
        Mtx_RotateX(bones[37].local, Math_Angle16ToRad(finWork->finRot[0].fold));
        Mtx_RotateX(bones[38].local, Math_Angle16ToRad(finWork->finRot[1].fold));
        Mtx_RotateX(bones[39].local, Math_Angle16ToRad(finWork->finRot[2].fold));
        Mtx_RotateX(bones[16].local, Math_Angle16ToRad(finWork->finRot[3].fold));
        Mtx_RotateY(bones[37].local, Math_Angle16ToRad(finWork->finRot[0].wave));
        Mtx_RotateY(bones[38].local, Math_Angle16ToRad(finWork->finRot[1].wave));
        Mtx_RotateY(bones[39].local, Math_Angle16ToRad(finWork->finRot[2].wave));
        Mtx_RotateY(bones[16].local, Math_Angle16ToRad(finWork->finRot[3].wave));
        break;
    case EM_KIND_SHEN_GAOREN:
        bones[11].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[0];
        bones[12].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[1];
        bones[13].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[2];
        bones[14].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[3];
        break;
    case 56:
        boneMtx = Em_BoneMatrixB(em, 2);
        Mtx_ClearRotation(boneMtx);
        phase = (6.2831855f * (f32)(gScene.frame << 13)) / 65536.0f;
        Mtx_RotateXYZ(boneMtx, phase, phase, phase);
        break;
    case EM_KIND_YAMA_TSUKAMI:
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_ScaleBones(em, bones);
        break;
    case EM_KIND_SHOGUN_CEANATAUR:
        phase = EmHermitaur_RageFxRatio(em);
        if (em->rage == 0) {
            scaleX = 0.9f + 0.1f * phase;
            scaleY = 0.85f + 0.15f * phase;
            scaleZ = 0.75f + 0.25f * phase;
            bones[HERM_BONE_CLAW_A].pose[0] = scaleX;
            bones[HERM_BONE_CLAW_A].pose[1] = scaleY;
            bones[HERM_BONE_CLAW_A].pose[2] = scaleZ;
            bones[HERM_BONE_CLAW_B].pose[0] = scaleX;
            bones[HERM_BONE_CLAW_B].pose[1] = scaleY;
            bones[HERM_BONE_CLAW_B].pose[2] = scaleZ;
            Mtx_ClearRotation(bones[HERM_BONE_CLAW_A].local);
            Mtx_ClearRotation(bones[HERM_BONE_CLAW_B].local);
            phase = (6.2831855f * (170.0f - 170.0f * phase)) / 360.0f;
            Mtx_RotateX(bones[HERM_BONE_CLAW_A].local, phase);
            Mtx_RotateX(bones[HERM_BONE_CLAW_B].local, phase);
            Mtx_RotateY(bones[HERM_BONE_CLAW_A].local, 0.0f);
            Mtx_RotateY(bones[HERM_BONE_CLAW_B].local, 0.0f);
            Mtx_RotateZ(bones[HERM_BONE_CLAW_A].local, 0.0f);
            Mtx_RotateZ(bones[HERM_BONE_CLAW_B].local, 0.0f);
        } else {
            scaleX = 1.0f - 0.1f * phase;
            scaleY = 1.0f - 0.15f * phase;
            scaleZ = 1.0f - 0.25f * phase;
            bones[HERM_BONE_CLAW_A].pose[0] = scaleX;
            bones[HERM_BONE_CLAW_A].pose[1] = scaleY;
            bones[HERM_BONE_CLAW_A].pose[2] = scaleZ;
            bones[HERM_BONE_CLAW_B].pose[0] = scaleX;
            bones[HERM_BONE_CLAW_B].pose[1] = scaleY;
            bones[HERM_BONE_CLAW_B].pose[2] = scaleZ;
            Mtx_ClearRotation(bones[HERM_BONE_CLAW_A].local);
            Mtx_ClearRotation(bones[HERM_BONE_CLAW_B].local);
            phase = (6.2831855f * (170.0f * phase)) / 360.0f;
            Mtx_RotateX(bones[HERM_BONE_CLAW_A].local, phase);
            Mtx_RotateX(bones[HERM_BONE_CLAW_B].local, phase);
            Mtx_RotateY(bones[HERM_BONE_CLAW_A].local, 0.0f);
            Mtx_RotateY(bones[HERM_BONE_CLAW_B].local, 0.0f);
            Mtx_RotateZ(bones[HERM_BONE_CLAW_A].local, 0.0f);
            Mtx_RotateZ(bones[HERM_BONE_CLAW_B].local, 0.0f);
        }
        break;
    case EM_KIND_RAJANG:
        phase = (f32)((EmBlangongaWork *)&em->work)->rageTimer / 15.0f;
        bones[15].pose[0] = 1.0f + 6.0f * phase;
        bones[15].pose[1] = 1.0f + 10.0f * phase;
        bones[15].pose[2] = 1.0f;
        break;
    }
}

/* The world matrices of bone and its subtree: the bone's pose scale times the parent's scale (which the
 * children inherit), its local matrix, then the parent's matrix; bones 33-36 then turn by
 * bendAngle[3] down to bendAngle[0]. */
void Bone_UpdateTree(Em *em, EmBone *bone, f32 *parent, f32 parentScaleX, f32 parentScaleY, f32 parentScaleZ)
{
    f32 unscale[16];
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;

    scaleX = bone->pose[0] * parentScaleX;
    scaleY = bone->pose[1] * parentScaleY;
    scaleZ = bone->pose[2] * parentScaleZ;
    Mtx_Scale(bone->world, scaleX, scaleY, scaleZ);
    Mtx_Identity(unscale);
    unscale[0] = 1.0f / parentScaleX;
    unscale[5] = 1.0f / parentScaleY;
    unscale[10] = 1.0f / parentScaleZ;
    unscale[12] = bone->local[12];
    unscale[13] = bone->local[13];
    unscale[14] = bone->local[14];
    Mtx_Concat3x3(bone->world, bone->local);
    Mtx_Concat(bone->world, unscale);
    Mtx_Concat(bone->world, parent);
    switch (bone->index) {
    case 33:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[3]));
        break;
    case 34:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[2]));
        break;
    case 35:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[1]));
        break;
    case 36:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[0]));
        break;
    }
    if (bone->child != NULL) {
        Bone_UpdateTree(em, bone->child, bone->world, scaleX, scaleY, scaleZ);
    }
    if (bone->sibling != NULL) {
        Bone_UpdateTree(em, bone->sibling, parent, parentScaleX, parentScaleY, parentScaleZ);
    }
}

/* Bone_UpdateTree from the root bones under matrix parent, with the scale 1. */
void Bone_UpdateTreeUnscaled(void *em, void *bones, f32 *parent)
{
    Bone_UpdateTree(em, bones, parent, 1.0f, 1.0f, 1.0f);
}


/* Builds the monster's matrix from its scale, angles and position into worldMtx[0], then its bones'
 * (Em_AdjustKindBones, then Bone_UpdateWorldScaledRoot; the Chameleos: Bone_UpdateTreeUnscaled). */
void Em_UpdateMatrix(Em *em)
{
    Model *model = em->model;
    f32 mtx[16];
    f32 scaleMtx[16];

    Mtx_Scale(scaleMtx, em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, mtx);
    Mtx_SetTrans(mtx, em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(mtx, scaleMtx);
    Mtx_Copy(em->worldMtx[0], mtx);
    Em_AdjustKindBones(em, model->bones);
    if (em->type == EM_KIND_CHAMELEOS) {
        Bone_UpdateTreeUnscaled(em, model->bones, mtx);
        return;
    }
    Bone_UpdateWorldScaledRoot(model->bones, mtx);
}

/* out = obj's position (a monster's, or a player's: the same offset) in the frame of matrix i of
 * gEmBoneMtxBuf. em (the monster carrying obj) is unused. */
void Em_PosInFrame(Em *obj, Em *em, s16 i, f32 *out)
{
    f32 delta[3];
    f32 origin[3];
    f32 inv[16];
    f32 *boneMtx = gEmBoneMtxBuf[i];

    Mtx_Inverse(inv, boneMtx);
    Mtx_GetTrans(origin, boneMtx);
    delta[0] = obj->posX - origin[0];
    delta[1] = obj->posY - origin[1];
    delta[2] = obj->posZ - origin[2];
    Vec_Rotate(out, delta, inv);
}

/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
extern f32 *gEmRideQuads[];

/* Carries the hunters standing on a monster that can be ridden (the Lao-Shan Lungs, the Shen Gaoren and
 * the Yama Tsukamis): a rider (rideState 1 or 3) on this monster moves with its bone (rideDelta: how
 * far). Then each hunter in the area who isn't reacting, fainted or in an event is dropped onto the
 * monster's quads (gEmRideQuads) from just above: on a hit it stands on the quad's bone (rideState 1,
 * or 3 and a ledge hang when it jumped up onto it); without a hit it stops riding. A Yama Tsukami
 * lands no one while EmYamaTsukami_IsQuakeFreeMotion is 0. */
void Em_CarryRiders(Em *em)
{
    f32 corner[3];
    f32 origin[3];
    f32 oldPos[3];
    f32 rotated[3];
    f32 top[3];
    f32 bottom[3];
    f32 tri[3][3];
    f32 quad[4][3];
    Player *pl;
    EmBone *bones;
    s32 i;
    s32 hits;
    s32 skip;
    s32 k;
    f32 *list;
    s32 j;
    f32 *quadCorner;
    u8 yamaHolds;

    pl = gPlayers;
    yamaHolds = 0;
    bones = em->model->bones;
    if (em->active == 0 || em->visible == 0) {
        return;
    }
    if (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG || em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
        if ((u8)(s32)Player_InCurrentArea(em)) {
            if ((em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) && EmYamaTsukami_IsQuakeFreeMotion(em) == 0) {
                yamaHolds = 1;
            }
            skip = yamaHolds;
            for (i = 0; i < SCENE_PLAYERS; i++, pl++) {
                if (!(u8)Obj_SameArea(pl, (Player *)em) || !(u8)Obj_InCurrentArea(pl)) {
                    continue;
                }
                if (pl->rideState == 1 || pl->rideState == 3) {
                    if (pl->rideEm == em->slotNo) {
                        Vec_Copy(oldPos, pl->pos);
                        k = pl->rideBone;
                        Em_PosInFrame((Em *)pl, em, k, origin);
                        Vec_Rotate(corner, origin, bones[k].world);
                        Mtx_GetTrans(origin, bones[k].world);
                        pl->pos[0] = corner[0] + origin[0];
                        pl->pos[1] = corner[1] + origin[1];
                        pl->pos[2] = corner[2] + origin[2];
                        pl->rideDelta[0] = pl->pos[0] - oldPos[0];
                        pl->rideDelta[1] = pl->pos[1] - oldPos[1];
                        pl->rideDelta[2] = pl->pos[2] - oldPos[2];
                    }
                }
                if (pl->stateGroup != PL_GROUP_REACT && pl->stateGroup != PL_GROUP_FAINT && pl->stateGroup != PL_GROUP_EVENT) {
                    if (skip) {
                        continue;
                    }
                    if (pl->vel[1] > 0.0f && pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_LEAP_OFF_LEDGE && (s32)pl->motionTime < 21 && pl->stateVar0 == 0) {
                        continue;
                    }
                    if (pl->vel[1] > 0.0f) {
                        top[1] = 230.0f + pl->pos[1];
                        bottom[1] = top[1] - 60.0f;
                    } else {
                        top[1] = 30.0f + pl->pos[1];
                        bottom[1] = top[1] - 60.0f;
                    }
                    top[0] = pl->pos[0];
                    hits = 0;
                    top[2] = pl->pos[2];
                    bottom[0] = top[0];
                    bottom[2] = top[2];
                    for (list = gEmRideQuads[em->type]; *list != -1.0f; ) {
                        k = *list;
                        list++;
                        for (j = 0, quadCorner = quad[0]; j < 4; j++, quadCorner += 3) {
                            corner[0] = *list++;
                            corner[1] = *list++;
                            corner[2] = *list++;
                            Vec_Rotate(rotated, corner, bones[k].world);
                            quadCorner[0] = rotated[0] + bones[k].world[12];
                            quadCorner[1] = rotated[1] + bones[k].world[13];
                            quadCorner[2] = rotated[2] + bones[k].world[14];
                        }
                        Vec_Copy(tri[0], quad[0]);
                        Vec_Copy(tri[1], quad[1]);
                        Vec_Copy(tri[2], quad[2]);
                        if (Tri_SegmentHit(tri[0], top, bottom, corner) == 0) {
                            Vec_Copy(tri[0], quad[1]);
                            Vec_Copy(tri[1], quad[2]);
                            Vec_Copy(tri[2], quad[3]);
                            if (Tri_SegmentHit(tri[0], top, bottom, corner) == 0) {
                                continue;
                            }
                        }
                        hits++;
                        if ((pl->rideState == 1 || pl->rideState == 3) && pl->rideBone == (u8)k) {
                            pl->pos[0] = corner[0];
                            pl->pos[1] = corner[1];
                            pl->pos[2] = corner[2];
                            break;
                        }
                        pl->pos[0] = corner[0];
                        pl->pos[1] = corner[1];
                        pl->pos[2] = corner[2];
                        pl->rideEm = em->slotNo;
                        pl->rideBone = k;
                        if (pl->rideState != 1 && pl->rideState != 3) {
                            pl->action = PL_POSTURE_STANDING;
                            if (pl->vel[1] > 0.0f && pl->stateGroup == PL_GROUP_ACTION && (pl->state == PL_ACT_LEAP_OFF_LEDGE || pl->state == PL_ACT_JUMP_TO_LEDGE)) {
                                pl->rideState = 3;
                                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_HANG, 0);
                            } else {
                                pl->rideState = 1;
                                Player_ReturnToIdle(pl, 0, 0, 0);
                            }
                            pl->weaponOut = 0;
                            Player_StopMove(pl);
                        }
                        break;
                    }
                    if (hits == 0 && pl->rideState != 2) {
                        pl->rideState = 0;
                    }
                } else {
                    pl->rideState = 0;
                    hits++;
                }
            }
        }
    }
}

/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
EmBone *Em_Bone(void *obj, s16 bone);

/* The first hit of the vertical segment from 200 above to 200 below point on the ridden monsters'
 * quads (gEmRideQuads[kind]: a bone, four corners, ..., -1): returns 1 with the monster, the bone and
 * the point in the bone's frame; 0 when none. */
s32 Em_HitQuadsVertical(f32 *point, Em **hitEm, s16 *hitBone, f32 *hit)
{
    f32 top[3];
    f32 bottom[3];
    f32 corner[3];
    f32 quad[4][3];
    f32 boneMtx[16];
    f32 inv[16];
    f32 tri[3][3];
    Em *em;
    s16 i;
    f32 *list;
    s16 j;

    for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
        if (em->active == 0 || em->visible == 0) {
            continue;
        }
        if (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG || em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
            if ((u8)(s32)Player_InCurrentArea(em)) {
                top[1] = 200.0f + point[1];
                bottom[1] = point[1] - 200.0f;
                top[0] = bottom[0] = point[0];
                top[2] = bottom[2] = point[2];
                for (list = gEmRideQuads[em->type]; *list != -1.0f; ) {
                    *hitBone = *list;
                    list++;
                    Mtx_Copy(boneMtx, (f32 *)Em_Bone(em, *hitBone));
                    for (j = 0; j < 4; j++) {
                        corner[0] = *list++;
                        corner[1] = *list++;
                        corner[2] = *list++;
                        Vec_RotateSelf(corner, boneMtx);
                        quad[j][0] = corner[0] + boneMtx[12];
                        quad[j][1] = corner[1] + boneMtx[13];
                        quad[j][2] = corner[2] + boneMtx[14];
                    }
                    Vec_Copy(tri[0], quad[0]);
                    Vec_Copy(tri[1], quad[1]);
                    Vec_Copy(tri[2], quad[2]);
                    if (Tri_SegmentHit(tri[0], top, bottom, hit)) {
                        hit[0] -= boneMtx[12];
                        hit[1] -= boneMtx[13];
                        hit[2] -= boneMtx[14];
                        Mtx_Inverse(inv, boneMtx);
                        Vec_RotateSelf(hit, inv);
                        *hitEm = em;
                        return 1;
                    }
                    Vec_Copy(tri[0], quad[1]);
                    Vec_Copy(tri[1], quad[2]);
                    Vec_Copy(tri[2], quad[3]);
                    if (Tri_SegmentHit(tri[0], top, bottom, hit)) {
                        hit[0] -= boneMtx[12];
                        hit[1] -= boneMtx[13];
                        hit[2] -= boneMtx[14];
                        Mtx_Inverse(inv, boneMtx);
                        Vec_RotateSelf(hit, inv);
                        *hitEm = em;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* Runs each active monster's update (its handlers), then gives it a draw slot that Em_DrawParts
 * draws. */
void Em_RunAll(void)
{
    s16 i;
    Em *em;

    for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
        if (em->active != 0) {
            em->handlers->update(em);
            em->drawSlotId = DrawSlot_Alloc();
            if (em->drawSlotId != -1) {
                em->drawSlot = DrawSlot_Get(em->drawSlotId);
                em->drawSlot->owner = (void *)em;
                em->drawSlot->draw = Em_DrawParts;
            }
        }
    }
}

/* The number of motion groups of monster kind i (gEmMotionGroupCounts). */
u8 EmKind_MotionGroupCount(s16 i)
{
    return gEmMotionGroupCounts[i];
}

/* The monster slot (0-3) whose kind (gScene.emKind) is k; -1 when none. The loop steps a byte
 * pointer through the scene and reads emKind[0] from it, which is emKind[i]: indexing emKind
 * compiles to other instructions than the game's. */
s16 EmSlot_FindKind(s16 k)
{
    s16 i;
    u8 *scene;

    for (i = 0, scene = (u8 *)&gScene; i < 4; i++, scene++) {
        if (((SceneState *)scene)->emKind[0] == k) {
            return i;
        }
    }
    return -1;
}

/* The file's data: the ridden monsters' quads by kind and the spark effect's tables. */
f32 gEmRideQuadsNone[2] = {
#include "sub_main/00468BA8.inc"
};
f32 gEmRideQuadsLaoShanLung[56] = {
#include "sub_main/00468BB0.inc"
};
f32 gEmRideQuadsShenGaoren[16] = {
#include "sub_main/00468C90.inc"
};
f32 gEmRideQuadsYamaTsukami[484] = {
#include "sub_main/00468CD0.inc"
};
f32 *gEmRideQuads[76] = {
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsLaoShanLung,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsLaoShanLung, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsShenGaoren,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsYamaTsukami, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, NULL, NULL,
};
