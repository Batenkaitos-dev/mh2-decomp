/*
 * The motions: the motion packs loaded into gMotions for the common set,
 * the players' sets and the monsters' sets (Anim_Setup*, MotionPack_*);
 * the player's motion layers, started, blended and advanced with their
 * root motion (Player_BeginLayerMotion, Player_SetPartAnim,
 * Player_UpdateMotion, Player_MoveLocal); and the frame tests of a layer's
 * motion (Motion_PassedFrame, Motion_ReachedFrame, Motion_InFrameRange,
 * with their monster forms).
 *
 * The bones are another unit: Player_UpdateMotion declares
 * Bone_PosePart and Bone_BlendPart with the float first and u16 ids, which
 * their definitions don't match.
 */

#include "common.h"
#include "syssettings.h"
#include "em.h"
#include "player.h"
#include "objhead.h"
#include "quest.h"
#include "anim_file.h"
#include "engine_math.h"
#include "mem_util.h"
#include "rand.h"
#include "vec_math.h"
#include "engine_util.h"

#pragma optimization_level 4

struct EmBone;

/* This file's read-only data. */
#pragma rodata on
char gMotionIdBadFmt[32] = {
#include "sub_main/0046A1E0.inc"
};
#pragma rodata off

extern s32 gCommonMotionGroupFirst[];
extern void *gMotions[];

/*
 * Loads the common motion pack (in gSysSettings.loadBuf; MOTIONS_COMMON_GROUPS groups)
 * into gMotions from 0, and the first index of each group into gCommonMotionGroupFirst.
 * Each animation file is converted in gSysSettings.dataBuf, under a system-heap mark
 * released after it, and copied into a memory handle of its own (NULL for no entry).
 */
void Anim_SetupCommon(void)
{
    s32 mark[2];
    AnimSet set;
    s32 groupBase;
    u8 *buf;
    s32 count;
    s32 group;
    s32 j;
    s32 motionIndex;
    u8 *entry;
    s32 *groupFirst;
    void **motions;

    motions = gMotions;
    groupFirst = gCommonMotionGroupFirst;
    buf = gSysSettings.loadBuf;
    motionIndex = 0;
    for (group = 0, groupBase = 0; group < MOTIONS_COMMON_GROUPS; group++, groupBase += MOTION_GROUP_SIZE) {
        groupFirst[group] = motionIndex;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                entry = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                motions[motionIndex] = entry;
            } else {
                motions[motionIndex] = NULL;
            }
        }
    }
}

extern s32 gPlayerMotionGroupFirst[][8]; /* the first set index of each group, per set */

/*
 * As Anim_SetupCommon for the six groups of set slot: loads each animation set of the table
 * in gLoadBuf into gMotions[MOTIONS_PLAYER_FIRST + slot * MOTIONS_PER_PLAYER ...], the first index of each group into
 * gPlayerMotionGroupFirst[slot].
 */
void Anim_SetupPlayer(s32 slot)
{
    s32 groupBase;
    s32 group;
    s32 j;
    s32 motionIndex;
    s32 *groupFirst;
    s32 count;
    s32 mark[2];
    u8 *entry;
    AnimSet set;
    void **out;
    u8 *buf;

    out = &gMotions[slot * MOTIONS_PER_PLAYER + MOTIONS_PLAYER_FIRST];
    buf = gSysSettings.loadBuf;
    motionIndex = 0;
    group = 0;
    groupFirst = gPlayerMotionGroupFirst[slot];
    for (groupBase = 0; group < MOTIONS_PLAYER_GROUPS; group++, groupFirst++, groupBase += MOTION_GROUP_SIZE) {
        *groupFirst = motionIndex;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, out++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                entry = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                *out = entry;
            } else {
                *out = NULL;
            }
        }
    }
}

extern s32 gEmMotionGroupFirst[][8]; /* as gPlayerMotionGroupFirst, for these sets */

/*
 * As Anim_SetupPlayer for the 2 * EmKind_MotionGroupCount(kind) groups of set slot, into
 * gMotions[MOTIONS_EM_FIRST + slot * MOTIONS_PER_EM ...] and gEmMotionGroupFirst[slot]. The copy has its own
 * variable, and groupBase is set after the store, for the match (MWCC strength-reduces it).
 */
void Anim_SetupEm(s32 slot, s32 kind)
{
    s32 mark[2];
    AnimSet set;
    u8 *buf;
    s32 motionIndex;
    s32 count;
    u8 *entry;
    void **out;
    void *copy;
    s32 groupBase;
    s32 group;
    s32 j;
    s32 groups;

    out = &gMotions[slot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
    buf = gSysSettings.loadBuf;
    groups = EmKind_MotionGroupCount(kind) * 2;
    motionIndex = 0;
    for (group = 0; group < groups; group++) {
        gEmMotionGroupFirst[slot][group] = motionIndex;
        groupBase = group * MOTION_GROUP_SIZE;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, out++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                copy = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                *out = copy;
            } else {
                *out = NULL;
            }
        }
    }
}

/* As Anim_SetupPlayer for the first two groups of the sets Anim_SetupEm loads (into
 * gMotions[MOTIONS_EM_FIRST + slot * MOTIONS_PER_EM ...] and gEmMotionGroupFirst[slot]). */
void Anim_SetupEmSlot(s32 slot)
{
    s32 groupBase;
    s32 group;
    s32 j;
    s32 motionIndex;
    s32 *groupFirst;
    s32 count;
    s32 mark[2];
    u8 *entry;
    AnimSet set;
    void **out;
    u8 *buf;

    out = &gMotions[slot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
    buf = gSysSettings.loadBuf;
    motionIndex = 0;
    group = 0;
    groupFirst = gEmMotionGroupFirst[slot];
    for (groupBase = 0; group < 2; group++, groupFirst++, groupBase += MOTION_GROUP_SIZE) {
        *groupFirst = motionIndex;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, out++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                entry = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                *out = entry;
            } else {
                *out = NULL;
            }
        }
    }
}

/*
 * Entry index of a motion pack (a loaded file of animation files): the entries come in
 * groups of MOTION_GROUP_SIZE (group index / 100: its count at +0, the offset of its
 * offsets at +4), each an offset from the pack (-1: none); NULL when there is none.
 * The copy `start` of the pack, made first, gives the result (for the match: v0);
 * pack itself walks to the offsets.
 */
u8 *MotionPack_GetEntry(u8 *pack, s32 index)
{
    u8 *start = pack;
    s32 *group;
    s32 inGroup;
    s32 offset;

    group = (s32 *)(pack + (index / MOTION_GROUP_SIZE) * 8);
    inGroup = index % MOTION_GROUP_SIZE;
    if (inGroup >= group[0]) {
        return NULL;
    }
    pack = (u8 *)((s32 *)pack + group[1] / 4);
    offset = ((s32 *)pack)[inGroup];
    if (offset == -1) {
        return NULL;
    }
    start += offset;
    return start;
}

/* The number of entries of a group of a motion pack. */
s32 MotionPack_GroupCount(u8 *pack, s32 group)
{
    return *(s32 *)(pack + group * 8);
}

/* The root motion of bone's animation from fromFrame to toFrame (Bone_GetAnimMove);
 * a negative fromFrame counts as 0, keeping the length. */
void Bone_GetAnimMoveFrom(f32 fromFrame, f32 toFrame, f32 *out, EmBone *bone)
{
    if (fromFrame < 0.0f) {
        Bone_GetAnimMove(out, bone, 0.0f, toFrame - fromFrame);
        return;
    }
    Bone_GetAnimMove(out, bone, fromFrame, toFrame);
}

/* dst = first * weightFirst + second * weightSecond (3-vectors). */
void Vec_Blend(f32 *dst, f32 *first, f32 *second, f32 weightFirst, f32 weightSecond)
{
    dst[0] = first[0] * weightFirst + second[0] * weightSecond;
    dst[1] = first[1] * weightFirst + second[1] * weightSecond;
    dst[2] = first[2] * weightFirst + second[2] * weightSecond;
}

/* Moves the player by move, turned and scaled by its rotation and scale (not while
 * PLF_NO_ROOT_MOVE is set); with flag 0x4000000 set, it only clears that flag (a move
 * skipped once). */
void Player_MoveLocal(Player *pl, f32 *move)
{
    f32 delta[4];
    f32 mtx[16];
    f32 scaleMtx[16];

    if (Player_TestFlag(pl, 0x4000000)) {
        Player_ClearFlag(pl, 0x4000000);
        return;
    }
    Mtx_FromAnglesXYZ(pl->rot, mtx);
    Mtx_Scale(scaleMtx, pl->scale[0], pl->scale[1], pl->scale[2]);
    Mtx_Mul(mtx, mtx, scaleMtx);
    Mtx_RotateVec(mtx, move, delta);
    if (!Player_TestFlag(pl, PLF_NO_ROOT_MOVE)) {
        pl->pos[0] += delta[0];
        pl->pos[1] += delta[1];
        pl->pos[2] += delta[2];
    }
}

/* unit's own declaration: declared as this file's callers use it; with the definition's s32 (EmBone *b, s32 h, s32 id), Player_UpdateMotion compiles differently */
s32 Bone_SetPartAnim(struct EmBone *b, s32 h, u16 id);

/* Starts motion motionId[part] on the player's layer `part` at frame `frame`, directly
 * (blendLen == 0) or blended over |blendLen| + 1 frames (blendLen < 0: without the root
 * motion). The id is read as in Player_SetPartAnim. */
void Player_BeginLayerMotion(Player *pl, s32 frame, s32 blendLen, s32 part)
{
    u16 id;
    Model *plModel;
    void **slot;
    s32 inSet;

    id = pl->motionId[part];
    plModel = pl->model;
    inSet = id % MOTION_ID_SET;
    if (pl->isEm == 0 || (pl->ownTask != 0 && pl->playerMotions != 0)) {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->index * MOTIONS_PER_PLAYER + MOTIONS_PLAYER_FIRST];
            slot += gPlayerMotionGroupFirst[pl->index][inSet / MOTION_GROUP_SIZE];
        } else {
            slot = gMotions;
            slot += gCommonMotionGroupFirst[inSet / MOTION_GROUP_SIZE];
        }
    } else {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->emModelSlot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
            slot += gEmMotionGroupFirst[pl->emModelSlot][inSet / MOTION_GROUP_SIZE];
        } else {
            Debug_Message(gMotionIdBadFmt, (s16)id, 0, 0);
        }
    }
    if (pl->isEm == 0 || pl->ownTask != 0) {
        slot += inSet % MOTION_GROUP_SIZE;
    } else {
        slot = (void **)((s32 *)slot + inSet % MOTION_GROUP_SIZE);
    }
    if (blendLen != 0) {
        if (blendLen < 0) {
            pl->motion[part].blend = -1;
            blendLen = -blendLen;
        } else {
            pl->motion[part].blend = 1;
        }
        Bone_SetPartAnim(plModel->partBones2[0], *(s32 *)slot, (u16)part);
        pl->motion[part].blendFrameCount = Motion_GetLength(*(s32 *)slot);
        pl->motion[part].blendLoop = Anim_GetLoopTime(*(s32 *)slot, &pl->motion[part].blendLoopFrame);
        pl->motion[part].blendFrame = frame;
        pl->motion[part].blendFrames = blendLen + 1;
        pl->motion[part].blendT = pl->motion[part].blendStep = 1.0f / (blendLen + 1);
        pl->motion[part].blendMotion = (s32 *)slot;
    } else {
        Bone_SetPartAnim(plModel->partBones[0], *(s32 *)slot, (u16)part);
        pl->motion[part].frameCount = Motion_GetLength(*(s32 *)slot);
        pl->motion[part].loop = Anim_GetLoopTime(*(s32 *)slot, &pl->motion[part].loopFrame);
        pl->motion[part].frame = frame;
        pl->motion[part].blendFrames = 0;
        pl->motion[part].blend = 0;
        pl->motion[part].blendMotion = (s32 *)slot;
    }
    pl->motion[part].playing = 1;
    if (part == 0) {
        pl->motion[0].move[0] = 0.0f;
        pl->motion[0].move[1] = 0.0f;
        pl->motion[0].move[2] = 0.0f;
    }
}

/* Sets the animation of part `part` of the player's model from its id in partAnimMotion
 * (MOTION_ID_SET: the set: 0 the common set (gCommonMotionGroupFirst), else the player's
 * own (gPlayerMotionGroupFirst) or, for an object that isn't a player, the monster set of
 * its unk30F (gEmMotionGroupFirst); then MOTION_GROUP_SIZE: the group; the rest: the
 * entry). */
void Player_SetPartAnim(Player *pl, s32 part)
{
    u16 id;
    Model *plModel;
    void **slot;
    s32 inSet;

    id = pl->partAnimMotion[part];
    plModel = pl->model;
    inSet = id % MOTION_ID_SET;
    if (pl->isEm == 0 || (pl->ownTask != 0 && pl->playerMotions != 0)) {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->index * MOTIONS_PER_PLAYER + MOTIONS_PLAYER_FIRST];
            slot += gPlayerMotionGroupFirst[pl->index][inSet / MOTION_GROUP_SIZE];
        } else {
            slot = gMotions;
            slot += gCommonMotionGroupFirst[inSet / MOTION_GROUP_SIZE];
        }
    } else {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->emModelSlot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
            slot += gEmMotionGroupFirst[pl->emModelSlot][inSet / MOTION_GROUP_SIZE];
        } else {
            Debug_Message(gMotionIdBadFmt, (s16)id, 0, 0);
        }
    }
    if (pl->isEm == 0 || pl->ownTask != 0) {
        slot += inSet % MOTION_GROUP_SIZE;
    } else {
        slot = (void **)((s32 *)slot + inSet % MOTION_GROUP_SIZE);
    }
    Bone_SetPartAnim(plModel->partBones2[0], *(s32 *)slot, (u16)part);
}

/* The definitions take the float first too (the same registers either way). */
s32 Bone_PosePart(f32 t, struct EmBone *b, u16 id); /* unit's own declaration: the definition takes s32 id; Player_UpdateMotion is 22 words off with it */
s32 Bone_BlendPart(f32 wa, f32 wb, struct EmBone *a, struct EmBone *b, u16 id); /* unit's own declaration: as Bone_PosePart */
f32 Math_Hermite(f32 t, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);

/* Advances the player's motion layers: poses each active layer, blends a layer into its
 * next motion (blend != 0) with the root motion of layer 0, moves the player by layer 0's
 * root motion (Player_MoveLocal), loops or stops a layer at its end, and blends the part
 * overrides (partAnimOn, weight partAnimWeight %). Returns 1 when a layer ended. */
s32 Player_UpdateMotion(Player *pl)
{
    Model *plModel = pl->model;
    EmBone *bones = plModel->bones;
    u16 i;
    s32 done = 0;
    f32 weightFrom;
    f32 weightTo;
    f32 loopStart;
    f32 rootMove[4];
    f32 moveFrom[4];
    f32 moveTo[4];

    for (i = 0; i < 4; i++) {
        if (i < pl->motionLayerCount) {
            if (pl->motion[i].blend != 0) {
                Bone_PosePart(pl->motion[i].frame, plModel->partBones[0], i);
                Bone_PosePart(pl->motion[i].blendFrame, plModel->partBones2[0], i);
                Bone_GetAnimMoveFrom(pl->motion[0].frame, pl->motion[0].frame + pl->motion[0].step, moveFrom, plModel->partBones[0]->child);
                weightTo = Math_Hermite(pl->motion[i].blendT, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
                weightFrom = 1.0f - weightTo;
                Bone_BlendPart(weightFrom, weightTo, plModel->partBones[0], plModel->partBones2[0], i);
                if (i == 0) {
                    Bone_GetAnimMoveFrom(pl->motion[0].blendFrame, pl->motion[0].blendFrame + pl->motion[0].step, moveTo, plModel->partBones2[0]->child);
                    Vec_Blend(rootMove, moveFrom, moveTo, weightFrom, weightTo);
                    if (pl->motion[i].blend > 0) {
                        Player_MoveLocal(pl, rootMove);
                    }
                    bones[1].local[12] = 0.0f;
                    bones[1].local[13] = 0.0f;
                    bones[1].local[14] = 0.0f;
                }
                pl->motion[i].blendT += pl->motion[i].blendStep;
                if (--pl->motion[i].blendFrames <= 0.0f) {
                    pl->motion[i].blend = 0;
                    Bone_SetPartAnim(plModel->partBones[0], *pl->motion[i].blendMotion, i);
                    pl->motion[i].frameCount = pl->motion[i].blendFrameCount;
                    pl->motion[i].loop = pl->motion[i].blendLoop;
                    pl->motion[i].loopFrame = pl->motion[i].blendLoopFrame;
                    pl->motion[i].prevFrame = pl->motion[i].blendFrame;
                    pl->motion[i].frame = pl->motion[i].blendFrame + pl->motion[i].step;
                    pl->motion[i].blendFrames = 0;
                }
            } else {
                Bone_PosePart(pl->motion[i].frame, plModel->partBones[0], i);
                if (pl->partAnimOn[i] != 0) {
                    Player_SetPartAnim(pl, i);
                    Bone_PosePart(pl->motion[i].frame, plModel->partBones2[0], i);
                    weightTo = (f32)(u32)pl->partAnimWeight[i] / 100.0f;
                    Bone_BlendPart(1.0f - weightTo, weightTo, plModel->partBones[0], plModel->partBones2[0], i);
                }
                if (i == 0) {
                    Bone_GetAnimMoveFrom(pl->motion[i].frame - pl->motion[i].step, pl->motion[i].frame, rootMove, plModel->partBones[0]->child);
                    Player_MoveLocal(pl, rootMove);
                    rootMove[0] = bones[1].local[12] - pl->motion[0].move[0];
                    rootMove[1] = bones[1].local[13] - pl->motion[0].move[1];
                    rootMove[2] = bones[1].local[14] - pl->motion[0].move[2];
                    pl->motion[0].move[0] = bones[1].local[12];
                    pl->motion[0].move[1] = bones[1].local[13];
                    pl->motion[0].move[2] = bones[1].local[14];
                    bones[1].local[12] = 0.0f;
                    bones[1].local[13] = 0.0f;
                    bones[1].local[14] = 0.0f;
                }
                pl->motion[i].prevFrame = pl->motion[i].frame;
                pl->motion[i].frame += pl->motion[i].step;
                if (!(pl->motion[i].frame <= pl->motion[i].frameCount)) {
                    if (pl->motion[i].loop != 0) {
                        loopStart = pl->motion[i].loopFrame;
                        if (!(loopStart < 1.0f)) {
                            loopStart -= 1.0f;
                        }
                        Bone_PosePart(loopStart, plModel->partBones[0], i);
                        if (i == 0) {
                            pl->motion[i].move[0] = bones[1].local[12];
                            pl->motion[i].move[1] = bones[1].local[13];
                            pl->motion[i].move[2] = bones[1].local[14];
                        }
                        Bone_PosePart(pl->motion[i].frameCount, plModel->partBones[0], i);
                        pl->motion[i].frame = pl->motion[i].loopFrame;
                        if (i == 0) {
                            bones[1].local[12] = 0.0f;
                            bones[1].local[13] = 0.0f;
                            bones[1].local[14] = 0.0f;
                        }
                        pl->motion[i].playing = 0;
                        done = 1;
                    } else {
                        done = 1;
                        pl->motion[i].frame = pl->motion[i].frameCount;
                        pl->motion[i].playing = 0;
                    }
                    if (pl->partAnimOn[i] != 0) {
                        Bone_PosePart(pl->motion[i].frame, plModel->partBones[0], i);
                        Player_SetPartAnim(pl, i);
                        Bone_PosePart(pl->motion[i].frame, plModel->partBones2[0], i);
                        weightTo = (f32)(u32)pl->partAnimWeight[i] / 100.0f;
                        Bone_BlendPart(1.0f - weightTo, weightTo, plModel->partBones[0], plModel->partBones2[0], i);
                    }
                } else {
                    pl->motion[i].playing = 1;
                }
            }
        }
    }
    return done;
}

/* 1 when the motion of layer passes frame during this step (frame to frame + speed,
 * wrapping to the loop frame at the end of a looping motion); 0 while motions are held. */
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame)
{
    Player *pl = obj;
    ObjHead *head = (ObjHead *)pl;
    s32 wrapped = 0;
    f32 next;
    f32 cur;
    f32 end;

    if (head->motion[0].blend != 0) {
        return 0;
    }
    cur = head->motion[layer].frame;
    end = head->motion[layer].frameCount;
    next = cur + head->motion[layer].step;
    if (!(next <= end)) {
        if (head->motion[layer].loop) {
            wrapped = 1;
            next = head->motion[layer].loopFrame + (next - end);
        } else {
            next = end;
        }
    }
    if (wrapped == 0) {
        if (!(frame < cur) && frame < next) {
            return 1;
        }
    } else {
        if (!(frame < cur) && next < end) {
            return 1;
        }
    }
    return 0;
}

/* Motion_PassedFrame for a monster; frame 2 is scaled by the monster's motion speed
 * when that isn't 1. */
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer)
{
    Em *em = obj;
    f32 speed;

    speed = em->motionSpeed;
    if (speed != 1.0f && frame == 2.0f) {
        frame *= speed;
    }
    return Motion_PassedFrame(em, layer, frame);
}

/* 1 once the motion of layer has reached frame (0 while motions are held). */
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame)
{
    ObjHead *head = obj;

    if (head->motion[0].blend != 0) {
        return 0;
    }
    if (frame <= head->motion[layer].frame) {
        return 1;
    }
    return 0;
}

/* Motion_ReachedFrame for the monsters. */
s32 EmMotion_ReachedFrame(void *em, s32 layer, f32 frame)
{
    return Motion_ReachedFrame(em, layer, frame);
}

/* 1 while the motion of layer is at or past `from` but before `until`. */
s32 Motion_InFrameRange(void *obj, s32 layer, f32 from, f32 until)
{
    if (Motion_ReachedFrame(obj, layer, from) != 0 && Motion_ReachedFrame(obj, layer, until) == 0) {
        return 1;
    }
    return 0;
}

/* Motion_InFrameRange for the monsters. */
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 until)
{
    return Motion_InFrameRange(em, layer, from, until);
}
