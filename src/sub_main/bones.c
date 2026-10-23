/*
 * The bones, and the memory handles that hold their data. First the
 * handles: Anim_SetData makes a motion's animation data current,
 * Skeleton_CopyData and AnimSet_CopyToHandle copy a built skeleton or an
 * animation set into a new handle, and Model_FreeBlock and Mem_FreeRet1
 * free one. Then the bones: a model's skeleton is a tree of EmBones
 * (model.h), built from a skeleton (Bone_InitTree), given a motion's bone
 * animations part by part (Bone_SetPartAnim; the AnimSet of motion.h),
 * posed (Bone_PoseTree), blended (Bone_BlendTree), and turned into world
 * and skin matrices for the matrix slots (Bone_UpdateWorld*,
 * Bone_StoreMatrices). The motion code (Player_UpdateMotion and its
 * layers) calls these. Where the unit starts is not known: the file
 * starts after the maths, at the first function that handles animation
 * data.
 */
#include "common.h"
#include "model.h"
#include "motion.h"
#include "anim_curve.h"
#include "engine_math.h"
#include "mem_util.h"

/* Optimization level 4, like the unit before it (these functions build the same at either level). */
#pragma optimization_level 4

extern s32 gAnimData;
s32 Bone_SetPartAnim(EmBone *b, s32 h, s32 id);
s32 Bone_PosePart(f32 t, EmBone *b, s32 id);
s32 Bone_BlendPart(f32 wa, f32 wb, EmBone *a, EmBone *b, s32 id);
extern MtxSlot gMtxSlots[];
extern u8 gMtxIdentity[];

s32 Anim_EvalPose(AnimHeader *hdr, f32 *base, f32 *pose, s16 *keys, f32 t);
s32 Anim_EvalPoseMatrix(AnimHeader *hdr, f32 *base, f32 *pose, f32 *m, s16 *keys, f32 t);

/* Makes the animation data in memory handle `handle` current (its keys too). */
void Anim_SetData(s32 handle)
{
    gAnimData = (s32) (Mem_GetAddr((void *) handle));
    Anim_SetCurveKeys((u8 *) gAnimData);
}

/* A copy of a built skeleton (the block skel's header points at, its size
 * bytes; model.h's SkelHeader) in a new memory handle, which it returns
 * (Model_InitSkeleton). The header's block (+8) and size (+0xC) are read
 * as bytes: read through SkelHeader, the code differs. */
void *Skeleton_CopyData(u8 *skel)
{
    u8 *src = *(u8 **)(skel + 8);
    void *handle = Mem_Alloc(*(s32 *)(src + 12), 4);
    u8 *dst = Mem_GetAddr(handle);
    u32 i;
    u8 *from = src;

    for (i = 0; i < *(u32 *)(src + 12); i++) {
        *dst++ = *from++;
    }
    return handle;
}

/* A copy of a converted animation set (set->base, whose first object is the set itself,
 * with the block's size; copied by words) in a new memory handle, which it returns. The
 * set is read as bytes, like Skeleton_CopyData's. */
void *AnimSet_CopyToHandle(u8 *set)
{
    u8 *src = *(u8 **)(set + 20);
    void *handle = Mem_Alloc(*(s32 *)(src + 24), 4);
    u32 *dst = Mem_GetAddr(handle);
    u32 i;
    u32 *from = (u32 *)src;

    for (i = 0; i < *(u32 *)(src + 24) / 4; i++) {
        *dst++ = *from++;
    }
    return handle;
}

/* Frees a memory handle; returns 1. Mem_FreeRet1 is the same. */
s32 Model_FreeBlock(void *block)
{
    Mem_Free(block);
    return 1;
}

/* Frees a memory handle (a motion of gMotions); returns 1. */
s32 Mem_FreeRet1(void *block)
{
    Mem_Free(block);
    return 1;
}

/* out = how far bone's animation moves it from fromTime to toTime (the difference of the two
 * poses' translations). Returns 1. */
s32 Bone_GetAnimMove(f32 *out, EmBone *bone, f32 fromTime, f32 toTime)
{
    f32 fromMtx[16];
    f32 toMtx[16];
    f32 pose[12];
    AnimHeader *boneAnim;

    Anim_SetData(bone->anim);
    boneAnim = (AnimHeader *)(bone->animOfs + gAnimData);
    Anim_EvalPoseMatrix(boneAnim, bone->base, pose, fromMtx, bone->keyHint, fromTime);
    Anim_EvalPoseMatrix(boneAnim, bone->base, pose, toMtx, bone->keyHint, toTime);
    out[0] = toMtx[12] - fromMtx[12];
    out[1] = toMtx[13] - fromMtx[13];
    out[2] = toMtx[14] - fromMtx[14];
    return 1;
}

/* Blends part id of treeA with treeB's (Bone_BlendTree, from each tree's first node of the part). */
s32 Bone_BlendPart(f32 weightA, f32 weightB, EmBone *treeA, EmBone *treeB, s32 id)
{
    EmBone *partA = Bone_FindPart(treeA, id);

    Bone_BlendTree(partA, Bone_FindPart(treeB, id), weightA, weightB, id);
    return 1;
}

/*
 * nodeA = nodeA * weightA + nodeB * weightB (the pose's scale and the local matrix) for
 * the nodes of part id connected to nodeA, walking the two trees (of the same shape)
 * together. This file's tree walks use gotos where the game's code branches back.
 */
s32 Bone_BlendTree(EmBone *nodeA, EmBone *nodeB, f32 weightA, f32 weightB, s32 id)
{
    EmBone *rootA = nodeA;

top:
    if (nodeA->id == (u16)id) {
        nodeA->pose[0] = nodeA->pose[0] * weightA + nodeB->pose[0] * weightB;
        nodeA->pose[1] = nodeA->pose[1] * weightA + nodeB->pose[1] * weightB;
        nodeA->pose[2] = nodeA->pose[2] * weightA + nodeB->pose[2] * weightB;
        Mtx_Blend(nodeA->local, nodeA->local, nodeB->local, weightA, weightB);
        if (nodeA->child != NULL) {
            nodeB = nodeB->child;
            nodeA = nodeA->child;
            goto top;
        }
    }
    if (nodeA->sibling != NULL && nodeA != rootA) {
        nodeB = nodeB->sibling;
        nodeA = nodeA->sibling;
        goto top;
    }
    while (nodeA != rootA) {
        while (nodeA->sibling != NULL) {
            nodeA = nodeA->sibling;
            nodeB = nodeB->sibling;
            if (nodeA->id == (u16)id) {
                goto top;
            }
        }
        nodeA = nodeA->parent;
        nodeB = nodeB->parent;
    }
    return 1;
}

/* world = local times the parent's world (parentMtx for bone) for every node of the tree,
 * depth first. */
void Bone_UpdateWorld(EmBone *bone, f32 *parentMtx)
{
    EmBone *root = bone;

loop:
    Mtx_Mul(bone->world, bone->local, parentMtx);
    if (bone->child != NULL) {
        parentMtx = bone->world;
        bone = bone->child;
        goto loop;
    }
    if (bone->sibling != NULL) {
        bone = bone->sibling;
        goto loop;
    }
    while (bone != root) {
        if (bone->sibling != NULL) {
            bone = bone->sibling;
            parentMtx = bone->parent->world;
            goto loop;
        }
        bone = bone->parent;
    }
}

/* gMtxSlots[index] of every node of the tree = its skin times its world matrix. */
void Bone_StoreMatrices(EmBone *bone)
{
    EmBone *root = bone;

loop:
    Mtx_Mul(gMtxSlots[bone->index].m[0], bone->skin, bone->world);
    if (bone->child != NULL) {
        bone = bone->child;
        goto loop;
    }
    if (bone->sibling != NULL) {
        bone = bone->sibling;
        goto loop;
    }
    while (bone != root) {
        if (bone->sibling != NULL) {
            bone = bone->sibling;
            goto loop;
        }
        bone = bone->parent;
    }
}

/* As Bone_StoreMatrices, into table. */
void Bone_StoreMatricesTo(MtxSlot *table, EmBone *bone)
{
    EmBone *root = bone;

loop:
    Mtx_Mul(table[bone->index].m[0], bone->skin, bone->world);
    if (bone->child != NULL) {
        bone = bone->child;
        goto loop;
    }
    if (bone->sibling != NULL) {
        bone = bone->sibling;
        goto loop;
    }
    while (bone != root) {
        if (bone->sibling != NULL) {
            bone = bone->sibling;
            goto loop;
        }
        bone = bone->parent;
    }
}

void Mem_Copy64ByQwords(void *src, void *dst, s32 n);

/* gMtxSlots[0..count-1] = table[list[1..count]] (count = list[0]). */
void MtxSlot_LoadList(s16 *list, MtxSlot *table)
{
    s32 i;
    s32 count;
    s16 *entry;

    count = list[0];
    entry = &list[1];
    for (i = 0; i < count; i++) {
        Mem_Copy64ByQwords(&table[*entry], &gMtxSlots[i], 1);
        entry++;
    }
}

extern s32 gBonePartAnim;

/*
 * Gives part id of tree the motion (a memory handle holding an AnimSet): the part's
 * nodes take, in tree order, the offsets of the set's bone animations (AnimSet.anims).
 * The set's address becomes gAnimData, which the offsets count from.
 */
s32 Bone_SetPartAnim(EmBone *tree, s32 motion, s32 id)
{
    s32 *boneAnims;
    AnimSet *set;
    EmBone *partRoot;

    set = Mem_GetAddr((void *)motion);
    gAnimData = (s32)set;
    boneAnims = (s32 *)(set->anims + gAnimData);
    gBonePartAnim = motion;
    partRoot = Bone_FindPart(tree, id);
    if (partRoot != NULL) {
        Bone_SetAnimTree(partRoot, boneAnims, id);
    }
    return 1;
}

/* The nodes of part id connected to bone take the motion gBonePartAnim and, in tree order,
 * the offsets at boneAnims. */
s32 Bone_SetAnimTree(EmBone *bone, s32 *boneAnims, s32 id)
{
    EmBone *root = bone;

top:
    if (bone->id == (u16)id) {
        bone->anim = gBonePartAnim;
        bone->animOfs = *boneAnims++;
        if (bone->child != NULL) {
            bone = bone->child;
            goto top;
        }
    }
    if (bone->sibling != NULL && bone != root) {
        bone = bone->sibling;
        goto top;
    }
    while (bone != root) {
        while (bone->sibling != NULL) {
            bone = bone->sibling;
            if (bone->id == (u16)id) {
                goto top;
            }
        }
        bone = bone->parent;
    }
    return 1;
}

/* The first node of part id in the tree at bone (depth first), or NULL. */
EmBone *Bone_FindPart(EmBone *bone, s32 id)
{
    EmBone *root = bone;

loop:
    if (bone->id == (u16)id) {
        return bone;
    }
    if (bone->child != NULL) {
        bone = bone->child;
        goto loop;
    }
    if (bone->sibling != NULL) {
        bone = bone->sibling;
        goto loop;
    }
    while (bone != root) {
        if (bone->sibling != NULL) {
            bone = bone->sibling;
            goto loop;
        }
        bone = bone->parent;
    }
    return NULL;
}

/* The length of motion (a memory handle holding an AnimSet): its latest key's time. */
f32 Motion_GetLength(s32 motion)
{
    return ((AnimSet *)Mem_GetAddr((void *)motion))->end;
}

/* *out = motion's loop time when it loops (ANIM_SET_LOOPS): 1; else 0, and *out = 0. */
s32 Anim_GetLoopTime(s32 motion, f32 *out)
{
    AnimSet *set = Mem_GetAddr((void *)motion);

    if (set->flags & ANIM_SET_LOOPS) {
        *out = set->loop;
        return 1;
    }
    *out = 0.0f;
    return 0;
}

/*
 * pose = base with the curves of boneAnim at time (ANIM_TYPE_FIXED stores fixed point:
 * angles in 0x4000ths of a turn, the rest in 16ths), then mtx = its scale, rotation and
 * translation. keyHints: per pose value, the key found last (EmBone.keyHint).
 */
s32 Anim_EvalPoseMatrix(AnimHeader *boneAnim, f32 *base, f32 *pose, f32 *mtx, s16 *keyHints, f32 time)
{
    s32 i;
    AnimCurve *curve;

    for (i = 0; i < 10; i++) {
        pose[i] = base[i];
    }
    if (boneAnim != NULL) {
        switch (boneAnim->flags & ANIM_TYPE_MASK) {
        case ANIM_TYPE_FLOAT:
            curve = (AnimCurve *)(boneAnim->curves + gAnimData);
            for (i = 0; i < boneAnim->count; i++, curve++) {
                u16 valueIdx = curve->idx;

                pose[valueIdx] = Anim_EvalCurve(time, curve, &keyHints[valueIdx]);
            }
            break;
        case ANIM_TYPE_FIXED:
            curve = (AnimCurve *)(boneAnim->curves + gAnimData);
            for (i = 0; i < boneAnim->count; i++, curve++) {
                u16 valueIdx = curve->idx;
                f32 *value = &pose[valueIdx];

                *value = Anim_EvalCurve(time, curve, &keyHints[valueIdx]);
                switch (valueIdx) {
                case 3:
                case 4:
                case 5:
                    *value *= 3.1415927f / 8192.0f;
                    break;
                default:
                    *value /= 16.0f;
                    break;
                }
            }
            break;
        }
    }
    Mtx_Scale(mtx, pose[0], pose[1], pose[2]);
    Mtx_RotateXYZ(mtx, pose[3], pose[4], pose[5]);
    mtx[12] = pose[6];
    mtx[13] = pose[7];
    mtx[14] = pose[8];
    return 1;
}

/* Poses part id of tree at time (Bone_PoseTree from the part's first node). */
s32 Bone_PosePart(f32 time, EmBone *tree, s32 id)
{
    Bone_PoseTree(time, Bone_FindPart(tree, id), id);
    return 1;
}

/*
 * The nodes of part id from bone on: pose = the rest pose, plus the node's animation at
 * time when it has one, then local from the pose.
 */
s32 Bone_PoseTree(f32 time, EmBone *bone, s32 id)
{
    s32 i;

    Anim_SetData(bone->anim);
    if (bone->id == (u16)id) {
        for (i = 0; i < 10; i++) {
            bone->pose[i] = bone->base[i];
        }
        if (bone->anim != 0) {
            Anim_EvalPose((AnimHeader *)(bone->animOfs + gAnimData), bone->base, bone->pose, bone->keyHint, time);
        }
        Mtx_FromTransform(bone->local, bone->pose);
        if (bone->child != NULL) {
            Bone_PoseTree(time, bone->child, id);
        }
    }
    if (bone->sibling != NULL) {
        Bone_PoseTree(time, bone->sibling, id);
    }
    return 1;
}

/* Bone_UpdateWorldScaled from a scale of 1 (parentMtx: the root's parent matrix). */
void Bone_UpdateWorldScaledRoot(EmBone *bone, f32 *parentMtx)
{
    Bone_UpdateWorldScaled(bone, parentMtx, 1.0f, 1.0f, 1.0f);
}

/*
 * world for bone and the nodes after it: the pose's scale times the scale passed down
 * (scaleX, scaleY, scaleZ: the children inherit the product), local's rotation, the
 * inverse of the inherited scale with local's translation, then parentMtx.
 */
void Bone_UpdateWorldScaled(EmBone *bone, f32 *parentMtx, f32 scaleX, f32 scaleY, f32 scaleZ)
{
    f32 unscale[16];
    f32 x = bone->pose[0] * scaleX;
    f32 y = bone->pose[1] * scaleY;
    f32 z = bone->pose[2] * scaleZ;

    Mtx_Scale(bone->world, x, y, z);
    Mtx_Identity(unscale);
    unscale[0] = 1.0f / scaleX;
    unscale[5] = 1.0f / scaleY;
    unscale[10] = 1.0f / scaleZ;
    unscale[12] = bone->local[12];
    unscale[13] = bone->local[13];
    unscale[14] = bone->local[14];
    Mtx_Concat3x3(bone->world, bone->local);
    Mtx_Concat(bone->world, unscale);
    Mtx_Concat(bone->world, parentMtx);
    if (bone->child != NULL) {
        Bone_UpdateWorldScaled(bone->child, bone->world, x, y, z);
    }
    if (bone->sibling != NULL) {
        Bone_UpdateWorldScaled(bone->sibling, parentMtx, scaleX, scaleY, scaleZ);
    }
}

/* Bone_UpdateWorldScaledUpTo from the identity scale. */
void Bone_UpdateWorldScaledUpToRoot(EmBone *bone, f32 *parentMtx, u8 maxId)
{
    Bone_UpdateWorldScaledUpTo(bone, parentMtx, maxId, 1.0f, 1.0f, 1.0f);
}

/* As Bone_UpdateWorldScaled, for the nodes whose id is at most maxId (a node above it stops its branch). */
void Bone_UpdateWorldScaledUpTo(EmBone *bone, f32 *parentMtx, u8 maxId, f32 scaleX, f32 scaleY, f32 scaleZ)
{
    f32 unscale[16];
    f32 x;
    f32 y;
    f32 z;

    if (bone->id <= maxId) {
        x = bone->pose[0] * scaleX;
        y = bone->pose[1] * scaleY;
        z = bone->pose[2] * scaleZ;
        Mtx_Scale(bone->world, x, y, z);
        Mtx_Identity(unscale);
        unscale[0] = 1.0f / scaleX;
        unscale[5] = 1.0f / scaleY;
        unscale[10] = 1.0f / scaleZ;
        unscale[12] = bone->local[12];
        unscale[13] = bone->local[13];
        unscale[14] = bone->local[14];
        Mtx_Concat3x3(bone->world, bone->local);
        Mtx_Concat(bone->world, unscale);
        Mtx_Concat(bone->world, parentMtx);
        if (bone->child != NULL) {
            Bone_UpdateWorldScaledUpTo(bone->child, bone->world, maxId, x, y, z);
        }
        if (bone->sibling != NULL) {
            Bone_UpdateWorldScaledUpTo(bone->sibling, parentMtx, maxId, scaleX, scaleY, scaleZ);
        }
    }
}

/* Bone_InitTree with the rest pose of mode 0. */
void Bone_InitTreeRoot(EmBone *bones, s32 skelHandle)
{
    Bone_InitTree(bones, skelHandle, 0, 0);
}

/*
 * Sets up the bone tree bones from the built skeleton at skelHandle (a SkelHeader and
 * its BoneDefs, SkelFile_BuildPart): clears the nodes, reads each definition, links the
 * tree, then the rest matrices (mode 0: Bone_InitRestPose, 1: Bone_InitRestPoseAlt).
 * unused is not read. Returns 1.
 */
s32 Bone_InitTree(EmBone *bones, s32 skelHandle, s32 unused, s32 mode)
{
    s32 boneIdx;
    s32 boneCount;
    SkelHeader *skel;
    EmBone *bone;

    skel = Mem_GetAddr((void *)skelHandle);
    boneCount = skel->count;
    for (boneIdx = 0; boneIdx < boneCount; boneIdx++) {
        bone = &bones[boneIdx];
        bone->count = boneCount;
        bone->siblingIdx = -1;
        bone->childIdx = -1;
        bone->sibling = NULL;
        bone->child = NULL;
        bone->anim = 0;
        bone->animOfs = 0;
        bone->unk178 = 0;
        bone->unk17C = 0;
        Mtx_Identity(bone->local);
    }
    for (boneIdx = 0; boneIdx < boneCount; boneIdx++) {
        Bone_ReadDef(&bones[boneIdx], skel, boneIdx);
    }
    Bone_LinkTree(bones, bones, NULL);
    switch (mode) {
    case 0:
        Bone_InitRestPoseUnit(bones);
        break;
    case 1:
        Bone_InitRestPoseAltUnit(bones);
        break;
    }
    return 1;
}

/* Links bone and the bones after it in the tree (bones: the model's bone array): parent,
 * and child and sibling from the indexes childIdx and siblingIdx (-1 none). */
void Bone_LinkTree(EmBone *bones, EmBone *bone, EmBone *parent)
{
    s16 siblingSlot;
    s16 childSlot;

    if (parent != NULL) {
        bone->parent = parent;
    }
    childSlot = bone->childIdx;
    if (childSlot != -1) {
        bone->child = &bones[childSlot];
        Bone_LinkTree(bones, bone->child, bone);
    }
    siblingSlot = bone->siblingIdx;
    if (siblingSlot != -1) {
        bone->sibling = &bones[siblingSlot];
        Bone_LinkTree(bones, bone->sibling, parent);
    }
}


/* Bone_InitRestPose from a scale of 1 and the identity matrix. */
void Bone_InitRestPoseUnit(EmBone *bone)
{
    Bone_InitRestPose(bone, 1.0f, 1.0f, 1.0f, (f32 *)gMtxIdentity);
}

/*
 * The rest pose of bone and the nodes after it: pose = base, local from it
 * (Bone_PoseMatrixScaled), and skin = the inverse of restWorld, local times parentMtx
 * (which the children get as theirs).
 */
s32 Bone_InitRestPose(EmBone *bone, f32 scaleX, f32 scaleY, f32 scaleZ, f32 *parentMtx)
{
    f32 restWorld[16];
    s32 i;

    for (i = 0; i < 10; i++) {
        bone->pose[i] = bone->base[i];
    }
    Bone_PoseMatrixScaled(bone->local, bone->pose, scaleX, scaleY, scaleZ);
    Mtx_Mul(restWorld, bone->local, parentMtx);
    Mtx_Inverse(bone->skin, restWorld);
    if (bone->child != NULL) {
        Bone_InitRestPose(bone->child, scaleX * bone->pose[0], scaleY * bone->pose[1], scaleZ * bone->pose[2], restWorld);
    }
    if (bone->sibling != NULL) {
        Bone_InitRestPose(bone->sibling, scaleX, scaleY, scaleZ, parentMtx);
    }
    return 1;
}

/* Bone_InitRestPoseAlt from a scale of 1 and the identity matrix. */
void Bone_InitRestPoseAltUnit(EmBone *bone)
{
    Bone_InitRestPoseAlt(bone, 1.0f, 1.0f, 1.0f, (f32 *)gMtxIdentity);
}

/* As Bone_InitRestPose, with local from Mtx_FromTransform and the inverse from
 * Bone_PoseMatrix's poseMtx. */
s32 Bone_InitRestPoseAlt(EmBone *bone, f32 scaleX, f32 scaleY, f32 scaleZ, f32 *parentMtx)
{
    f32 restWorld[16];
    f32 poseMtx[16];
    s32 i;

    for (i = 0; i < 10; i++) {
        bone->pose[i] = bone->base[i];
    }
    Mtx_FromTransform(bone->local, bone->pose);
    Bone_PoseMatrix(poseMtx, bone->pose, scaleX, scaleY, scaleZ);
    Mtx_Mul(restWorld, poseMtx, parentMtx);
    Mtx_Inverse(bone->skin, restWorld);
    if (bone->child != NULL) {
        Bone_InitRestPoseAlt(bone->child, scaleX * bone->pose[0], scaleY * bone->pose[1], scaleZ * bone->pose[2], restWorld);
    }
    if (bone->sibling != NULL) {
        Bone_InitRestPoseAlt(bone->sibling, scaleX, scaleY, scaleZ, parentMtx);
    }
    return 1;
}

/* mtx = the rotation (transform[3..5], X then Y then Z) and the translation
 * (transform[6..8]) of transform, a pose. */
s32 Mtx_FromTransform(f32 *mtx, f32 *transform)
{
    Mtx_Identity(mtx);
    Mtx_RotateXYZ(mtx, transform[3], transform[4], transform[5]);
    mtx[12] = transform[6];
    mtx[13] = transform[7];
    mtx[14] = transform[8];
    return 1;
}

/*
 * mtx = the pose's scale times (scaleX, scaleY, scaleZ), its rotation, then the inverse
 * of that extra scale with the pose's translation.
 */
s32 Bone_PoseMatrixScaled(f32 *mtx, f32 *pose, f32 scaleX, f32 scaleY, f32 scaleZ)
{
    f32 rot[16];
    f32 unscale[16];

    Mtx_Scale(mtx, pose[0] * scaleX, pose[1] * scaleY, pose[2] * scaleZ);
    Mtx_Identity(rot);
    Mtx_RotateXYZ(rot, pose[3], pose[4], pose[5]);
    Mtx_Identity(unscale);
    unscale[0] = 1.0f / scaleX;
    unscale[5] = 1.0f / scaleY;
    unscale[10] = 1.0f / scaleZ;
    unscale[12] = pose[6];
    unscale[13] = pose[7];
    unscale[14] = pose[8];
    Mtx_Concat(mtx, rot);
    Mtx_Concat(mtx, unscale);
    return 1;
}

/* As Bone_PoseMatrixScaled, with the pose's own scale. */
s32 Bone_PoseMatrix(f32 *mtx, f32 *pose, f32 scaleX, f32 scaleY, f32 scaleZ)
{
    f32 rot[16];
    f32 unscale[16];

    Mtx_Scale(mtx, pose[0], pose[1], pose[2]);
    Mtx_Identity(rot);
    Mtx_RotateXYZ(rot, pose[3], pose[4], pose[5]);
    Mtx_Identity(unscale);
    unscale[0] = 1.0f / scaleX;
    unscale[5] = 1.0f / scaleY;
    unscale[10] = 1.0f / scaleZ;
    unscale[12] = pose[6];
    unscale[13] = pose[7];
    unscale[14] = pose[8];
    Mtx_Concat(mtx, rot);
    Mtx_Concat(mtx, unscale);
    return 1;
}

/* Bone boneIdx of the built skeleton skel (a SkelHeader, then its BoneDefs) into bone:
 * its ids, links and rest pose. */
s32 Bone_ReadDef(EmBone *bone, SkelHeader *skel, s32 boneIdx)
{
    BoneDef *def = (BoneDef *)(skel + 1);

    def += boneIdx;
    bone->unk174 = def->unkA;
    bone->index = def->index;
    bone->id = def->id;
    bone->childIdx = def->child;
    bone->siblingIdx = def->sibling;
    bone->pose[0] = bone->base[0] = def->scale[0];
    bone->pose[1] = bone->base[1] = def->scale[1];
    bone->pose[2] = bone->base[2] = def->scale[2];
    bone->pose[3] = bone->base[3] = def->rot[0];
    bone->pose[4] = bone->base[4] = def->rot[1];
    bone->pose[5] = bone->base[5] = def->rot[2];
    bone->pose[6] = bone->base[6] = def->trans[0];
    bone->pose[7] = bone->base[7] = def->trans[1];
    bone->pose[8] = bone->base[8] = def->trans[2];
    bone->animOfs = 0;
    bone->anim = 0;
    return 1;
}

/* The curves of boneAnim at time into pose (as Anim_EvalPoseMatrix, without the matrix). */
s32 Anim_EvalPose(AnimHeader *boneAnim, f32 *base, f32 *pose, s16 *keyHints, f32 time)
{
    s32 i;
    AnimCurve *curve;

    if (boneAnim != NULL) {
        switch (boneAnim->flags & ANIM_TYPE_MASK) {
        case ANIM_TYPE_FLOAT:
            curve = (AnimCurve *)(boneAnim->curves + gAnimData);
            for (i = 0; i < boneAnim->count; i++, curve++) {
                u16 valueIdx = curve->idx;

                pose[valueIdx] = Anim_EvalCurve(time, curve, &keyHints[valueIdx]);
            }
            break;
        case ANIM_TYPE_FIXED:
            curve = (AnimCurve *)(boneAnim->curves + gAnimData);
            for (i = 0; i < boneAnim->count; i++, curve++) {
                u16 valueIdx = curve->idx;
                f32 *value = &pose[valueIdx];

                *value = Anim_EvalCurve(time, curve, &keyHints[valueIdx]);
                switch (valueIdx) {
                case 3:
                case 4:
                case 5:
                    *value *= 3.1415927f / 8192.0f;
                    break;
                default:
                    *value /= 16.0f;
                    break;
                }
            }
            break;
        }
    }
    return 1;
}

/* This file's initialised data. */
s32 gBonePartAnim = {
#include "sub_main/00402570.inc"
};
