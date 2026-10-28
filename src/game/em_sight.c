/*
 * A monster's sight, then the per-kind monster data that comes after it.
 *
 * GmEm_Update runs Em_SeePlayers after Em_SensePlayers.
 * Em_SeePlayers uses Em_CheckFlash (flash objects) and Em_SightBlocked
 * (sight-blocking objects). Em_SelectSightRow picks the sight row.
 * Em_SetMotion is how every monster family starts a motion.
 *
 * The data is indexed by Em.type: push priorities, body scales, shadow
 * sizes, Effect06 scales, the bone hit lists (gEmBoneHits), the body and push
 * sphere lists, the sight rows (gEmSightRows), the bend origins and the
 * motion tables (gEmMotionRows, gEmMotionTransRows, gEmMotionAltRows).
 */
#include "common.h"
#include "em.h"
#include "em_laoshan.h"
#include "em_blangonga.h"
#include "em_chameleos.h"
#include "em_fatalis.h"
#include "em_kushala.h"
#include "obj.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"
#include "capsule.h"
#include "render_state.h"
#include "area.h"
#include "bgm.h"
#include "camera.h"
#include "card.h"
#include "em_bulldrome.h"
#include "em_diablos.h"
#include "em_drome.h"
#include "em_remobra.h"
#include "em_vespoid.h"
#include "quest.h"
#include "sound.h"
#include "text.h"
#include "ambient_life.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "game_util.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "vec_math.h"
#include "em_anteka.h"
#include "em_apceros.h"
#include "em_cephadrome.h"
#include "em_garuga.h"
#include "em_gravios.h"
#include "em_hermitaur.h"
#include "em_khezu.h"
#include "em_kind38.h"
#include "em_kirin.h"
#include "em_kutku.h"
#include "em_melynx.h"
#include "em_mosswine.h"
#include "em_plesioth.h"
#include "em_rathalos.h"
#include "em_shakalaka.h"
#include "em_shengaoren.h"
#include "em_velociprey.h"
#include "em_yamatsukami.h"
/* Declarations of the functions called below. */
#pragma optimization_level 4

extern EmObj *gSightBlockObjs[32];


extern EmRate *gEmRateTbl[];

/* Em +0x6C4: four s16 values, one per player, and +0x6CC their last seen copies (Em_UpdateReactions runs
 * the reaction of a player whose value changed). Em has no fields there yet: a view of it. */
typedef struct EmReactView {
    u8 pad[1732];
    s16 cur[4];
    s16 last[4];
} EmReactView;

/* Em +0x818: what Em_SensePlayers measures of each player each frame (em.h has bytes there: other
 * code uses +0x846 as an s16). */
typedef struct EmSenseView {
    u8 pad[2020];
    s16 noticeTimer[4];              /* 0x7E4: Em unk7E4 */
    u8 pad7EC[2072 - 2028];
    f32 dist[4];                /* 0x818: from the monster to the player */
    f32 distXZ[4];              /* 0x828: the same, in the ground plane */
    f32 range[4];               /* 0x838: the four reaction ranges (Em_GetReaction kinds 3 to 6) */
    s32 alertBuildup[4];              /* 0x848: Em unk848, cleared when the player is out of reach */
    u16 dir[4];                 /* 0x858: the direction to the player (Math_AngleXZ) */
} EmSenseView;

/* prototypes */
EmBone *Em_Bone(void *obj, s16 bone);
s32 Em_PartBroken(Em *em, s32 arg1);
extern f32 D_008B0B8C[];
extern f32 BreakableStructure_EffectPos[][3];
extern EmObj *gFlashObjs[32];
typedef struct EmSight {
    f32 *pos;
    s32 target;
    s32 dir;
    u16 half;
} EmSight;
void Em_CheckFlash(Em *em, u8 player, EmSight *s);
u8 AreaHit_Raycast(f32 *a, f32 *b, f32 *out, u16 mask);
extern f32 D_008B0A94[];
extern f32 D_008B0A98[];
extern f32 D_008B0AC4[];
extern f32 D_008B0AC8[];
extern f32 CastleGate_PosOpen[][3];
extern f32 CastleGate_PosShut[][3];
typedef struct BreakableStructurePiece {
    s16 slot;
    s16 idx;
    f32 pos[3];
    u16 ang;
    u8 pad12[2];
    DrawSlot *ds;
} BreakableStructurePiece;
extern f32 BreakableStructure_Pos[][3];
extern f32 D_008B0A14[];
extern f32 D_008B0A18[];
extern f32 D_008B0AAC;
extern f32 TurningProp_Pos[][3];
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
extern f32 gCamMatrix[];
struct Segment;
EmReaction *Em_GetReaction(Em *em, u8 kind, u8 player);
struct TexRect;
extern EmObj *gAreaObjList[32];
extern f32 gViewMatrix[];
extern f32 gFrustumPlanes[];
typedef struct EmBendView {
    u8 pad[1596];
    u32 bend[4];
} EmBendView;
/* end of prototypes */

/*
 * Finds the players the monster sees this frame and clears Em.targetDist. The
 * eye is the kind's head bone, facing that bone's heading; for the Felyne
 * kinds and any kind not listed, it is 100 above the position, facing rotY. A
 * player is seen when all of these hold: active, not fainted, in the
 * monster's area; within the height and ground-distance limits of the sight
 * row (Em.sightTable; a zero limit means none) and within its half angle of
 * the eye's facing; no area triangle (except for kind 56) and no sight-
 * blocking object (Em_SightBlocked) between the eye and the player's head (50
 * above the feet when crouched, else 170). Em.seen gets one bit per seen
 * player; a newly seen player outside combat adds reaction 0's notice
 * (Em_AddNotice), and Em_CheckFlash runs for every player in the area.
 * Returns the farthest player seen, or -1; with Em.blind set it sees nobody.
 */
s8 Em_SeePlayers(Em *em)
{
    EmSenseView *sense;
    EmSight sight;
    f32 eyePos[3];
    f32 hit[3];
    f32 headPos[3];
    f32 targetPos[3];
    EmSightRange *row;
    u16 half;
    s32 i;
    s32 eyeDir;
    s32 farthest = -1;
    s32 blocked;
    f32 farthestDist;
    f32 maxXZ;
    Player *pl;
    f32 minXZ;
    f32 above;
    f32 below;
    f32 y;
    f32 heightDiff;
    f32 distToPlayer;
    f32 groundDist;
    f32 *headPtr;
    s32 coneWidth;
    EmReaction *reaction;

    if (em->blind != 0) {
        em->seen = 0;
        return (u8) farthest;
    }
    em->targetDist = 0.0f;
    farthestDist = 0.0f;
    pl = gPlayers;
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_CEPHADROME:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_CEPHALOS:
    case EM_KIND_YIAN_GARUGA:
        Em_BonePos(em, 34, eyePos);
        eyeDir = (u16) (Mtx_Heading((f32 *)Em_Bone(em, 34)) + 0x4000);
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        Em_BonePos(em, FAT_BONE_BREATH, eyePos);
        eyeDir = (u16) (Mtx_Heading((f32 *)Em_Bone(em, FAT_BONE_BREATH)) + 0x4000);
        break;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        Em_BonePos(em, LAOSHAN_BONE_EYE, eyePos);
        eyeDir = (u16) (Mtx_Heading((f32 *)Em_Bone(em, LAOSHAN_BONE_EYE)) + 0x4000);
        break;
    case EM_KIND_FELYNE:
    case EM_KIND_MELYNX:
    case EM_KIND_SHAKALAKA:
        eyePos[0] = em->posX;
        eyePos[1] = 100.0f + em->posY;
        eyePos[2] = em->posZ;
        eyeDir = em->rotY;
        break;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        Em_BonePos(em, BLANG_BONE_BREATH, eyePos);
        eyeDir = (u16) (Mtx_Heading((f32 *)Em_Bone(em, BLANG_BONE_BREATH)) + 0x4000);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        Em_BonePos(em, KUSH_BONE_BREATH, eyePos);
        eyeDir = (u16) (Mtx_Heading((f32 *)Em_Bone(em, KUSH_BONE_BREATH)) + 0x4000);
        break;
    case EM_KIND_CHAMELEOS:
        Em_BonePos(em, CHAM_BONE_BREATH, eyePos);
        eyeDir = (u16) (Mtx_Heading((f32 *)Em_Bone(em, CHAM_BONE_BREATH)) + 0x4000);
        break;
    default:
        eyePos[0] = em->posX;
        eyePos[1] = 100.0f + em->posY;
        eyePos[2] = em->posZ;
        eyeDir = em->rotY;
        break;
    }
    sense = (EmSenseView *) em;
    row = *(EmSightRange **) &em->sightTable;
    maxXZ = row->maxXZ;
    y = em->posY;
    minXZ = row->minXZ;
    above = row->above;
    below = row->below;
    half = row->half;
    sight.dir = eyeDir;
    coneWidth = half * 2;
    sight.half = half;
    sight.pos = eyePos;
    for (i = 0; i < 4; i++, pl++) {
        if (pl->stateGroup != PL_GROUP_FAINT && Obj_SameArea(em, pl) && pl->active) {
            Em_GetTargetPos(em, i, targetPos);
            Em_CheckFlash(em, i, &sight);
            heightDiff = targetPos[1] - y;
            if ((above == 0.0f || heightDiff <= 0.0f || heightDiff <= above)
                && (below == 0.0f || !(heightDiff < 0.0f) || -heightDiff <= below)) {
                groundDist = sense->distXZ[i];
                if ((maxXZ == 0.0f || groundDist <= maxXZ)
                    && (minXZ == 0.0f || !(groundDist < minXZ))
                    && (distToPlayer = sense->dist[i], (u16) (sense->dir[i] + half - eyeDir) <= coneWidth)) {
                    if (pl->action == PL_POSTURE_CROUCHED) {
                        Vec_Set(headPos, targetPos[0], 50.0f + targetPos[1], targetPos[2]);
                    } else {
                        Vec_Set(headPos, targetPos[0], 170.0f + targetPos[1], targetPos[2]);
                    }
                    sight.target = (s32) (headPtr = headPos);
                    if (em->type != EM_KIND_THUNDERBUG) {
                        if (gScene.skipFlaggedTris != 0) {
                            blocked = AreaGrid_TestSegment(em, eyePos, headPtr, hit, 16640);
                        } else {
                            blocked = AreaGrid_TestSegment(em, eyePos, headPtr, hit, 256);
                        }
                        if (blocked == 1) {
                            goto out;
                        }
                    }
                    if ((u8) Em_SightBlocked(em, (void **) &sight) != 1) {
                        if (!(em->seen & (1 << i)) && em->combat == 0) {
                            reaction = Em_GetReaction(em, 0, i);
                            Em_AddNotice(em, reaction->noticeAdd, reaction->noticeMax, i);
                        }
                        em->seen |= 1 << i;
                        if (farthestDist == 0.0f) {
                            farthestDist = distToPlayer;
                            farthest = i;
                        } else if (!(distToPlayer <= farthestDist)) {
                            farthestDist = distToPlayer;
                            farthest = i;
                        }
                        continue;
                    }
                }
            }
        }
out:
        em->seen &= ~(1 << i);
    }
    return (u8) farthest;
}

extern s8 gFlashObjCount;
/*
 * Sets em->flashed when a flash object (gFlashObjs) in the monster's area has
 * the eye (sight->pos) within its radius and lies within sight->half of the
 * eye's facing (sight->dir). It skips a monster that noHitTimer or guardTimer
 * protects, one another console runs, the kinds that flashes never affect,
 * and the Diablos and Monoblos kinds under the sand. `player` is not used.
 */
void Em_CheckFlash(Em *em, u8 player, EmSight *sight)
{
    s32 emKind;
    s32 i;
    EmObj **slot;
    EmObj *flash;
    u16 flashDir;

    if (*(u16 *) &em->noHitTimer > 0 || *(s8 *) &em->guardTimer > 0 || em->remote != 0) {
        return;
    }
    emKind = em->type;
    if (emKind == 2 || emKind == 36 || emKind == 5 || emKind == 50 || emKind == 7 || emKind == 8
        || emKind == 15 || emKind == 39 || emKind == 20 || emKind == 21 || emKind == 45
        || emKind == 46 || emKind == 24 || emKind == EM_KIND_ROCK || emKind == 33 || emKind == 34
        || emKind == 48 || emKind == 55 || emKind == 58 || emKind == 71 || emKind == 72
        || emKind == 73 || emKind == 66 || emKind == 67 || emKind == 68) {
        return;
    }
    if (emKind == 14 || emKind == 43 || emKind == 26 || emKind == 44) {
        if (em->stance == 4) {
            return;
        }
    }
    if (gFlashObjCount == 0) {
        return;
    }
    for (i = 0, slot = gFlashObjs; i < 32; i++, slot++) {
        flash = *slot;
        if (flash != NULL && flash->area == em->area && Vec_Distance(flash->pos, sight->pos) <= flash->radius) {
            flashDir = Math_AngleXZ(sight->pos, flash->pos);
            if ((u16) (flashDir + sight->half - sight->dir) <= sight->half * 2) {
                em->flashed = 1;
            }
        }
    }
}

/*
 * Returns 1 when a sight-blocking object (gSightBlockObjs, kind bit 0 set) in
 * the monster's area lies across the line from the player's head (ends[1]) to
 * the monster's eye (ends[0]); the test is Capsule_Classify against the
 * object's sphere. Otherwise returns 0.
 */
s32 Em_SightBlocked(Em *em, void **ends)
{
    f32 segment[16];
    s32 i;
    u8 *blocker;
    u8 **slot;

    for (i = 0, slot = (u8 **) gSightBlockObjs; i < 32; i++, slot++) {
        blocker = *slot;
        if (blocker != NULL && *(u16 *) (blocker + 24) == em->area && (blocker[18] & 1)) {
            Segment_Init(ends[1], ends[0], (struct Segment *) segment);
            if (Capsule_Classify((struct Segment *) segment, (f32 *) blocker, *(f32 *) (blocker + 12)) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 * Points Em.sightTable at row `idx` of the kind's rows in gEmSightRows. Each
 * row (EmSightRow, 32 bytes) holds the sight ranges, the action reach and the
 * sense reach. GmEm_Setup picks row 0; states such as Rathalos' head-down or
 * Kushala's eating pick row 1, then row 0 again.
 */
void Em_SelectSightRow(Em *em, u8 idx)
{
    em->sightTable = &gEmSightRows[em->type][idx];
}

/*
 * Starts row `motion` of the kind's motion table (gEmMotionRows) on every
 * layer the row names (EmMotionRow.layers), after rebuilding rotMtx. With
 * motionRemap set, it clears it and uses the row's remap row of
 * gEmMotionAltRows instead. When `frame` is 0 the row's start frame is used;
 * when `blend` is 0 the row's blend is used, or rowBlend when the row has
 * none. A layer blends over blend / 2 frames; it gets no blend when it has no
 * motion yet, or when it is already on this motion with motion[i].loop set
 * and motion[i].playing clear. When the row of the layer's current motion
 * names a transition row (EmMotionRow.trans) whose motion is the current one,
 * the layer keeps its current motion, restarted from the transition row's
 * frame and blend. Afterwards rowBlend holds the row's nextBlend. The (s8)
 * (s32) and (s16) (s32) casts keep the sign-extensions that the game's code
 * does.
 */
void Em_SetMotion(Em *em, s32 motion, s32 blend, s32 frame)
{
    EmMotionRow *tbl = gEmMotionRows[em->type];
    s32 i;
    u16 id;
    u16 curId;
    s32 curRow;
    EmMotionRow *transRow;
    u8 transIdx;
    u16 transId;

    Mtx_FromAnglesYXZ(&em->rotX, em->rotMtx[0]);
    if (em->motionRemap) {
        em->motionRemap = 0;
        if (tbl[motion].remap) {
            motion = tbl[motion].remap;
            tbl = gEmMotionAltRows[em->type];
        }
    }
    for (i = 0; i < em->motionLayerCount; i++) {
        switch (tbl[motion].layers) {
        case -1:
            break;
        case 0:
        case 1:
        case 2:
            if (i != tbl[motion].layers) {
                continue;
            }
            break;
        case 16:
            if (i == 2) {
                continue;
            }
            break;
        case 17:
            if (i == 1) {
                continue;
            }
            break;
        case 18:
            if (i == 0) {
                continue;
            }
            break;
        }
        if (frame == 0) {
            frame = tbl[motion].frame;
        }
        id = tbl[motion].motion + 1000 + i * 200;
        if (blend == 0) {
            if (em->rowBlend != 0) {
                if (tbl[motion].blend != 0) {
                    blend = (s8) (s32) tbl[motion].blend;
                } else {
                    blend = (s16) (s32) em->rowBlend;
                }
            } else {
                blend = tbl[motion].blend;
            }
        }
        curId = em->motionId[i];
        if (curId == id && em->motion[i].loop != 0 && em->motion[i].playing == 0) {
            blend = 0;
        }
        curRow = curId - 1000 - i * 200;
        if ((transIdx = tbl[curRow].trans) != 0 && curRow > 0) {
            transRow = &gEmMotionTransRows[em->type][transIdx];
            transId = transRow->motion + 1000 + i * 200;
            if (transId == curId) {
                frame = transRow->frame;
                id = transId;
                if (em->rowBlend != 0) {
                    if (transRow->blend != 0) {
                        blend = (s8) (s32) transRow->blend;
                    } else {
                        blend = (s16) (s32) em->rowBlend;
                    }
                } else {
                    blend = transRow->blend;
                }
                if (curId == transId && em->motion[i].loop != 0) {
                    blend = 0;
                }
            }
        }
        if (curId == 0) {
            blend = 0;
        }
        em->motionId[i] = id;
        em->motionBlend[i] = blend / 2;
        em->motionFrame[i] = frame;
        Player_BeginLayerMotion((Player *)em, em->motionFrame[i], em->motionBlend[i], i);
    }
    em->rowBlend = tbl[motion].nextBlend;
}

/* The file's data. */
extern f32 gEmWallSpheres_Remobra[];
extern f32 gEmPushSpheres_Plesioth[];
/* Per monster type (gm_sub gm_em.c: Em +0x596). */
u8 gEmPushPriorities[74] = {
#include "game/008B15D0.inc"
};
/* Per monster type: its body scale (gm_sub gm_em.c: Em bodyScale and scaleX/Y/Z). */
f32 gEmBodyScales[74] = {
#include "game/008B1620.inc"
};
/* Per monster kind: two floats, a minimum and a maximum, that Shadow_Run and
 * Shadow_Draw read for the effect drawn under the monster. */
f32 gShadowSizes[74][2] = {
#include "game/008B1750.inc"
};
f32 gEffect06Scales[76] = {
#include "game/008B19A0.inc"
};
HitShape D_008B1AD0[1] = {
#include "game/008B1AD0.inc"
};
HitShape D_008B1B00[45] = {
#include "game/008B1B00.inc"
};
HitShape D_008B2210[3] = {
#include "game/008B2210.inc"
};
HitShape D_008B2290[2] = {
#include "game/008B2290.inc"
};
HitShape D_008B22E0[2] = {
#include "game/008B22E0.inc"
};
HitShape D_008B2330[26] = {
#include "game/008B2330.inc"
};
HitShape D_008B2740[41] = {
#include "game/008B2740.inc"
};
HitShape D_008B2DB0[20] = {
#include "game/008B2DB0.inc"
};
HitShape D_008B30D0[2] = {
#include "game/008B30D0.inc"
};
HitShape D_008B3120[32] = {
#include "game/008B3120.inc"
};
HitShape D_008B3620[3] = {
#include "game/008B3620.inc"
};
HitShape D_008B36A0[31] = {
#include "game/008B36A0.inc"
};
HitShape D_008B3B80[25] = {
#include "game/008B3B80.inc"
};
HitShape D_008B3F70[4] = {
#include "game/008B3F70.inc"
};
HitShape D_008B4010[32] = {
#include "game/008B4010.inc"
};
HitShape D_008B4510[2] = {
#include "game/008B4510.inc"
};
HitShape D_008B4560[24] = {
#include "game/008B4560.inc"
};
HitShape D_008B4920[32] = {
#include "game/008B4920.inc"
};
HitShape D_008B4E20[30] = {
#include "game/008B4E20.inc"
};
HitShape D_008B52D0[3] = {
#include "game/008B52D0.inc"
};
HitShape D_008B5350[32] = {
#include "game/008B5350.inc"
};
HitShape D_008B5850[4] = {
#include "game/008B5850.inc"
};
HitShape D_008B58F0[3] = {
#include "game/008B58F0.inc"
};
HitShape D_008B5970[4] = {
#include "game/008B5970.inc"
};
HitShape D_008B5A10[20] = {
#include "game/008B5A10.inc"
};
HitShape D_008B5D30[32] = {
#include "game/008B5D30.inc"
};
HitShape D_008B6230[21] = {
#include "game/008B6230.inc"
};
HitShape D_008B6580[13] = {
#include "game/008B6580.inc"
};
HitShape D_008B6790[21] = {
#include "game/008B6790.inc"
};
HitShape D_008B6AE0[36] = {
#include "game/008B6AE0.inc"
};
HitShape D_008B7080[43] = {
#include "game/008B7080.inc"
};
HitShape D_008B7740[2] = {
#include "game/008B7740.inc"
};
HitShape D_008B7790[2] = {
#include "game/008B7790.inc"
};
HitShape D_008B77E0[37] = {
#include "game/008B77E0.inc"
};
HitShape D_008B7DB0[40] = {
#include "game/008B7DB0.inc"
};
HitShape D_008B83F0[13] = {
#include "game/008B83F0.inc"
};
HitShape D_008B8600[14] = {
#include "game/008B8600.inc"
};
HitShape D_008B8830[25] = {
#include "game/008B8830.inc"
};
HitShape D_008B8C20[36] = {
#include "game/008B8C20.inc"
};
HitShape D_008B91C0[36] = {
#include "game/008B91C0.inc"
};
HitShape D_008B9760[2] = {
#include "game/008B9760.inc"
};
HitShape D_008B97B0[26] = {
#include "game/008B97B0.inc"
};
HitShape D_008B9BC0[3] = {
#include "game/008B9BC0.inc"
};
HitShape *gEmBoneHits[74] = {
    D_008B1AD0, D_008B3120, D_008B1B00, D_008B2210,
    D_008B2290, D_008B2290, D_008B2330, D_008B2740,
    D_008B2DB0, D_008B30D0, D_008B22E0, D_008B3120,
    D_008B3620, D_008B3F70, D_008B36A0, D_008B3B80,
    D_008B3F70, D_008B4010, D_008B1AD0, D_008B4510,
    D_008B4560, D_008B4920, D_008B4E20, D_008B30D0,
    D_008B22E0, D_008B52D0, D_008B5350, D_008B5850,
    D_008B5850, D_008B58F0, D_008B3F70, D_008B5850,
    D_008B2290, D_008B5970, D_008B5A10, D_008B3F70,
    D_008B1B00, D_008B3120, D_008B2330, D_008B4560,
    D_008B5D30, D_008B3120, D_008B3120, D_008B36A0,
    D_008B5350, D_008B3B80, D_008B4920, D_008B4010,
    D_008B6230, D_008B3120, D_008B2740, D_008B6580,
    D_008B6790, D_008B6580, D_008B6AE0, D_008B7080,
    D_008B7740, D_008B7790, D_008B77E0, D_008B7DB0,
    D_008B6AE0, D_008B83F0, D_008B8600, D_008B8830,
    D_008B8C20, D_008B91C0, D_008B9760, D_008B97B0,
    D_008B9BC0, D_008B2210, D_008B3620, D_008B1B00,
    D_008B77E0, D_008B9760,
};
f32 gEmWallSpheres_0[8] = {
#include "game/008B9D70.inc"
};
f32 gEmWallSpheres_Fatalis[8] = {
#include "game/008B9D90.inc"
};
f32 gEmWallSpheres_Kelbi[8] = {
#include "game/008B9DB0.inc"
};
f32 gEmWallSpheres_Mosswine[8] = {
#include "game/008B9DD0.inc"
};
f32 gEmWallSpheres_LaoShanLung[8] = {
#include "game/008B9DF0.inc"
};
f32 gEmWallSpheres_12[8] = {
#include "game/008B9E10.inc"
};
f32 gEmWallSpheres_Diablos[8] = {
#include "game/008B9E30.inc"
};
f32 gEmWallSpheres_Khezu[8] = {
#include "game/008B9E50.inc"
};
f32 gEmWallSpheres_Genprey[8] = {
#include "game/008B9E70.inc"
};
f32 gEmWallSpheres_Gravios[8] = {
#include "game/008B9E90.inc"
};
f32 gEmWallSpheres_FelyneCart[4] = {
#include "game/008B9EB0.inc"
};
f32 gEmWallSpheres_Vespoid[8] = {
#include "game/008B9EC0.inc"
};
f32 gEmWallSpheres_Gypceros[8] = {
#include "game/008B9EE0.inc"
};
f32 gEmWallSpheres_Plesioth[8] = {
#include "game/008B9F00.inc"
};
f32 gEmWallSpheres_29[8] = {
#include "game/008B9F20.inc"
};
f32 gEmWallSpheres_Blangonga[8] = {
#include "game/008B9F40.inc"
};
f32 gEmWallSpheres_56[8] = {
#include "game/008B9F60.inc"
};
f32 gEmWallSpheres_YamaTsukami[8] = {
#include "game/008B9F80.inc"
};
f32 gEmWallSpheres_Remobra[8] = {
#include "game/008B9FA0.inc"
};
f32 *gEmWallSpheres[76] = {
#include "game/008B9FC0.inc"
};
f32 gEmPushSpheres_Rathian[8] = {
#include "game/008BA0F0.inc"
};
f32 gEmPushSpheres_Plesioth[8] = {
#include "game/008BA110.inc"
};
f32 *gEmPushSpheres[76] = {
#include "game/008BA130.inc"
};
EmSightRow gEmSightRows_0[2] = {
#include "game/008BA260.inc"
};
EmSightRow gEmSightRows_2[2] = {
#include "game/008BA2A0.inc"
};
EmSightRow gEmSightRows_3[1] = {
#include "game/008BA2E0.inc"
};
EmSightRow gEmSightRows_4[1] = {
#include "game/008BA300.inc"
};
EmSightRow gEmSightRows_7[1] = {
#include "game/008BA320.inc"
};
EmSightRow gEmSightRows_8[2] = {
#include "game/008BA340.inc"
};
EmSightRow gEmSightRows_9[1] = {
#include "game/008BA380.inc"
};
EmSightRow gEmSightRows_12[1] = {
#include "game/008BA3A0.inc"
};
EmSightRow gEmSightRows_15[2] = {
#include "game/008BA3C0.inc"
};
EmSightRow gEmSightRows_13[2] = {
#include "game/008BA400.inc"
};
EmSightRow gEmSightRows_51[2] = {
#include "game/008BA440.inc"
};
EmSightRow gEmSightRows_54[2] = {
#include "game/008BA480.inc"
};
EmSightRow gEmSightRows_56[2] = {
#include "game/008BA4C0.inc"
};
EmSightRow gEmSightRows_58[2] = {
#include "game/008BA500.inc"
};
EmSightRow gEmSightRows_66[2] = {
#include "game/008BA540.inc"
};
EmSightRow gEmSightRows_73[2] = {
#include "game/008BA580.inc"
};
EmSightRow *gEmSightRows[76] = {
    gEmSightRows_0, gEmSightRows_0, gEmSightRows_2, gEmSightRows_3,
    gEmSightRows_4, gEmSightRows_4, gEmSightRows_0, gEmSightRows_7,
    gEmSightRows_8, gEmSightRows_9, gEmSightRows_0, gEmSightRows_0,
    gEmSightRows_12, gEmSightRows_13, gEmSightRows_0, gEmSightRows_15,
    gEmSightRows_13, gEmSightRows_0, gEmSightRows_0, gEmSightRows_0,
    gEmSightRows_0, gEmSightRows_0, gEmSightRows_0, gEmSightRows_9,
    gEmSightRows_0, gEmSightRows_12, gEmSightRows_0, gEmSightRows_13,
    gEmSightRows_13, gEmSightRows_0, gEmSightRows_13, gEmSightRows_13,
    gEmSightRows_4, gEmSightRows_3, gEmSightRows_8, gEmSightRows_13,
    gEmSightRows_2, gEmSightRows_0, gEmSightRows_0, gEmSightRows_0,
    gEmSightRows_0, gEmSightRows_0, gEmSightRows_0, gEmSightRows_0,
    gEmSightRows_0, gEmSightRows_15, gEmSightRows_0, gEmSightRows_0,
    gEmSightRows_0, gEmSightRows_0, gEmSightRows_7, gEmSightRows_51,
    gEmSightRows_51, gEmSightRows_51, gEmSightRows_54, gEmSightRows_0,
    gEmSightRows_56, gEmSightRows_9, gEmSightRows_58, gEmSightRows_0,
    gEmSightRows_54, gEmSightRows_51, gEmSightRows_51, gEmSightRows_0,
    gEmSightRows_54, gEmSightRows_54, gEmSightRows_66, gEmSightRows_0,
    gEmSightRows_4, gEmSightRows_3, gEmSightRows_12, gEmSightRows_2,
    gEmSightRows_0, gEmSightRows_73, NULL, NULL,
};
EmBendRow gEmBendOrigins_0 = {
#include "game/008BA6F0.inc"
};
EmBendRow gEmBendOrigins_7 = {
#include "game/008BA710.inc"
};
EmBendRow gEmBendOrigins_59 = {
#include "game/008BA730.inc"
};
EmBendRow *gEmBendOrigins[76] = {
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_7,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_7, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_59,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, NULL, NULL,
};
extern EmMotionRow D_0090F990[];
extern EmMotionRow D_00910010[];
extern EmMotionRow D_00910A00[];
extern EmMotionRow D_00911450[];
extern EmMotionRow D_00911910[];
extern EmMotionRow D_00912190[];
extern EmMotionRow D_00912650[];
extern EmMotionRow D_00912670[];
extern EmMotionRow D_00912C30[];
extern EmMotionRow D_009136D0[];
extern EmMotionRow D_00913BB0[];
extern EmMotionRow D_009142B0[];
extern EmMotionRow D_00914E90[];
extern EmMotionRow D_00915CA0[];
extern EmMotionRow D_00916340[];
extern EmMotionRow D_00917140[];
extern EmMotionRow D_009177E0[];
extern EmMotionRow D_00917CA0[];
extern EmMotionRow D_00918D50[];
extern EmMotionRow D_009197B0[];
extern EmMotionRow D_0091C480[];
extern EmMotionRow D_0091D120[];
extern EmMotionRow D_0091E1C0[];
extern EmMotionRow D_0091E700[];
extern EmMotionRow D_00920630[];
extern EmMotionRow D_00921E20[];
extern EmMotionRow D_009222E0[];
extern EmMotionRow D_009239A0[];
extern EmMotionRow D_009242E0[];
extern EmMotionRow D_009247A0[];
extern EmMotionRow D_00925150[];
extern EmMotionRow D_00926210[];
EmMotionRow *gEmMotionRows[76] = {
    D_0090F990, D_0090F990, D_00910010, D_00910A00,
    D_00911450, D_00911450, D_00911910, D_00912190,
    D_00912C30, D_009136D0, D_00913BB0, D_0090F990,
    D_009142B0, D_00916340, D_00914E90, D_00915CA0,
    D_00916340, D_00917140, D_009177E0, D_00917CA0,
    D_00918D50, D_009197B0, D_00917140, D_009136D0,
    D_00917CA0, D_009142B0, D_00914E90, D_00916340,
    D_00916340, D_0091C480, D_00916340, D_00916340,
    D_00911450, D_00910A00, D_00912C30, D_00916340,
    D_0091D120, D_0090F990, D_00911910, D_00918D50,
    D_0091E1C0, D_0090F990, D_0090F990, D_00914E90,
    D_00914E90, D_0091E700, D_009197B0, D_00917140,
    D_00920630, D_0090F990, D_00912190, D_00921E20,
    D_009222E0, D_00921E20, D_009239A0, D_009242E0,
    D_0090F990, D_009247A0, D_00925150, D_00926210,
    D_009239A0, D_00921E20, D_009222E0, (EmMotionRow *)gEmMotionRows_Remobra,
    D_009239A0, D_009239A0, D_00920630, D_00920630,
    (EmMotionRow *)gEmMotionRows_68, D_00910A00, D_009142B0, D_0091D120,
    D_0090F990, D_00920630, NULL, NULL,
};
EmMotionRow *gEmMotionTransRows[76] = {
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, NULL, NULL,
};
EmMotionRow *gEmMotionAltRows[76] = {
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, NULL, NULL,
};
