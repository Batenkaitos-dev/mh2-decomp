/*
 * The bones (sub_main 0x2EF020-0x2F17C0). A model's skeleton is a tree of EmBones
 * (model.h): built from a skeleton (Bone_InitTree), given a motion's bone animations
 * part by part (Bone_SetPartAnim; the AnimSet of motion.h), posed (Bone_PoseTree),
 * blended (Bone_BlendTree), and turned into world and skin matrices for the matrix slots
 * (Bone_UpdateWorld*, Bone_StoreMatrices). unk_2E3940.c's motion layers call these.
 * Then Vu1_SelectProgram, which picks a model's VU1 program, and the programs'
 * descriptors (data). Where the unit starts exactly is not known: after 0x2EEA70
 * (Math_SinCos's last caller), by 0x2EF0F0.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "image.h"
#include "dma.h"
#include "texture.h"
#include "padio.h"
#include "model.h"
#include "motion.h"

/* Optimization level 4, like unk_2E3940.c (these functions build the same at either level). */
#pragma optimization_level 4

s32 Bone_GetAnimMove(f32 *out, EmBone *b, f32 t0, f32 t1);
void * Mem_GetAddr(void *handle);
extern s32 gAnimData;
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
s32 Bone_SetPartAnim(EmBone *b, s32 h, s32 id);
f32 Motion_GetLength(s32 h);
s32 Anim_GetLoopTime(s32 h, f32 *out);
s32 Bone_PosePart(f32 t, EmBone *b, s32 id);
s32 Bone_BlendPart(f32 wa, f32 wb, EmBone *a, EmBone *b, s32 id);
void Vu1_SelectProgram(ModelEntry *e, MeshDesc *hdr);
extern MtxSlot gMtxSlots[];
asm void Mtx_Identity(f32 *m);
asm void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
extern u8 gMtxIdentity[];
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Concat(f32 *m, f32 *a);
f32 Anim_EvalCurve(f32 t, AnimCurve *c, s16 *cur);
asm void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
asm void Mtx_Inverse(f32 *dst, f32 *src);
asm void Mtx_Blend(f32 *dst, f32 *a, f32 *b, f32 wa, f32 wb);
void Anim_SetData(s32 arg0);


EmBone *Bone_FindPart(EmBone *b, s32 id);
s32 Bone_BlendTree(EmBone *a, EmBone *b, f32 wa, f32 wb, s32 id);
s32 Bone_PoseTree(f32 t, EmBone *b, s32 id);
s32 Mtx_FromTransform(f32 *m, f32 *t);
s32 Anim_EvalPose(AnimHeader *hdr, f32 *base, f32 *pose, s16 *keys, f32 t);
s32 Anim_EvalPoseMatrix(AnimHeader *hdr, f32 *base, f32 *pose, f32 *m, s16 *keys, f32 t);
s32 Bone_InitTree(EmBone *bones, s32 h, s32 unused, s32 mode);
void Bone_UpdateWorldScaled(EmBone *b, f32 *m, f32 sx, f32 sy, f32 sz);
void Bone_UpdateWorldScaledUpTo(EmBone *b, f32 *m, u8 maxId, f32 sx, f32 sy, f32 sz);
void Bone_LinkTree(EmBone *parts, EmBone *b, EmBone *parent);
void Bone_InitRestPoseUnit(EmBone *b);
s32 Bone_InitRestPose(EmBone *b, f32 sx, f32 sy, f32 sz, f32 *m);
void Bone_InitRestPoseAltUnit(EmBone *b);
s32 Bone_InitRestPoseAlt(EmBone *b, f32 sx, f32 sy, f32 sz, f32 *m);
s32 Bone_PoseMatrixScaled(f32 *m, f32 *pose, f32 sx, f32 sy, f32 sz);
s32 Bone_PoseMatrix(f32 *m, f32 *pose, f32 sx, f32 sy, f32 sz);
s32 Bone_ReadDef(EmBone *bone, SkelHeader *skel, s32 boneIdx);

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
s32 Bone_SetAnimTree(EmBone *b, s32 *ofs, s32 id);

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

/* The VU1 programs' descriptors (0x402580..0x403110, 60 of them). */
extern s32 gVu1ProgInfo00[];
extern s32 gVu1ProgInfo01[];
extern s32 gVu1ProgInfo02[];
extern s32 gVu1ProgInfo03[];
extern s32 gVu1ProgInfo04[];
extern s32 gVu1ProgInfo05[];
extern s32 gVu1ProgInfo06[];
extern s32 gVu1ProgInfo07[];
extern s32 gVu1ProgInfo08[];
extern s32 gVu1ProgInfo09[];
extern s32 gVu1ProgInfo10[];
extern s32 gVu1ProgInfo11[];
extern s32 gVu1ProgInfo12[];
extern s32 gVu1ProgInfo13[];
extern s32 gVu1ProgInfo14[];
extern s32 gVu1ProgInfo15[];
extern s32 gVu1ProgInfo16[];
extern s32 gVu1ProgInfo17[];
extern s32 gVu1ProgInfo18[];
extern s32 gVu1ProgInfo19[];
extern s32 gVu1ProgInfo20[];
extern s32 gVu1ProgInfo21[];
extern s32 gVu1ProgInfo22[];
extern s32 gVu1ProgInfo23[];
extern s32 gVu1ProgInfo24[];
extern s32 gVu1ProgInfo25[];
extern s32 gVu1ProgInfo26[];
extern s32 gVu1ProgInfo27[];
extern s32 gVu1ProgInfo28[];
extern s32 gVu1ProgInfo29[];
extern s32 gVu1ProgInfo30[];
extern s32 gVu1ProgInfo31[];
extern s32 gVu1ProgInfo32[];
extern s32 gVu1ProgInfo33[];
extern s32 gVu1ProgInfo34[];
extern s32 gVu1ProgInfo35[];
extern s32 gVu1ProgInfo36[];
extern s32 gVu1ProgInfo37[];
extern s32 gVu1ProgInfo38[];
extern s32 gVu1ProgInfo39[];
extern s32 gVu1ProgInfo40[];
extern s32 gVu1ProgInfo41[];
extern s32 gVu1ProgInfo42[];
extern s32 gVu1ProgInfo43[];
extern s32 gVu1ProgInfo44[];
extern s32 gVu1ProgInfo45[];
extern s32 gVu1ProgInfo46[];
extern s32 gVu1ProgInfo47[];
extern s32 gVu1ProgInfo48[];
extern s32 gVu1ProgInfo49[];
extern s32 gVu1ProgInfo50[];
extern s32 gVu1ProgInfo51[];
extern s32 gVu1ProgInfo52[];
extern s32 gVu1ProgInfo53[];
extern s32 gVu1ProgInfo54[];
extern s32 gVu1ProgInfo55[];
extern s32 gVu1ProgInfo56[];
extern s32 gVu1ProgInfo57[];
extern s32 gVu1ProgInfo58[];
extern s32 gVu1ProgInfo59[];
/*
 * Picks entry's VU1 program: entry->prog (0..59) and its descriptor entry->progInfo
 * (gVu1ProgInfo00 ...), from the render flags the data was built with (entry->renderFlags,
 * whose top byte, render state 0x5D's, must be 0) and the mesh key (MESH_KEY_*); -1 and
 * NULL when no program fits. mesh is not read. The render keys are bits of gRenderFlags2,
 * set by render states without names yet (render_state.h): 0x2 by state 0x15, 0x20 and 0x40
 * by state 0, 0x80 by RS_SCISSOR, 0x200 by state 1, 0x4000 by state 0x62, 0x8000 by state
 * 0x12, 0x20000 by state 0x66. The top byte is tested by a one-case switch, as the game's
 * code does (a branch over a jump to the end).
 */
void Vu1_SelectProgram(ModelEntry *entry, MeshDesc *mesh)
{
    u32 flags;
    u32 renderKey;
    s32 meshKey;

    entry->prog = -1;
    entry->progInfo = NULL;
    flags = entry->renderFlags;
    meshKey = entry->used | entry->vtxFormat;
    renderKey = flags & 0xFFFFFF;
    switch (flags & 0xFF000000) {
    case 0:
            switch (renderKey) {
            case 0x202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 0;
                    entry->progInfo = gVu1ProgInfo00;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 1;
                    entry->progInfo = gVu1ProgInfo01;
                    break;
                }
                break;
            case 0x20202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 2;
                    entry->progInfo = gVu1ProgInfo02;
                    break;
                }
                break;
            case 0x222:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 3;
                    entry->progInfo = gVu1ProgInfo03;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 4;
                    entry->progInfo = gVu1ProgInfo04;
                    break;
                }
                break;
            case 0x20020:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 5;
                    entry->progInfo = gVu1ProgInfo05;
                    break;
                }
                break;
            case 0x80:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 6;
                    entry->progInfo = gVu1ProgInfo06;
                    break;
                }
                break;
            case 0x20080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 7;
                    entry->progInfo = gVu1ProgInfo07;
                    break;
                }
                break;
            case 0x200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 8;
                    entry->progInfo = gVu1ProgInfo08;
                    break;
                }
                break;
            case 0x20200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 9;
                    entry->progInfo = gVu1ProgInfo09;
                    break;
                }
                break;
            case 0x0:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 10;
                    entry->progInfo = gVu1ProgInfo10;
                    break;
                }
                break;
            case 0x2:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 11;
                    entry->progInfo = gVu1ProgInfo11;
                    break;
                }
                break;
            case 0x20000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 12;
                    entry->progInfo = gVu1ProgInfo12;
                    break;
                }
                break;
            case 0x20002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 13;
                    entry->progInfo = gVu1ProgInfo13;
                    break;
                }
                break;
            case 0x8222:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 14;
                    entry->progInfo = gVu1ProgInfo14;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 15;
                    entry->progInfo = gVu1ProgInfo15;
                    break;
                }
                break;
            case 0x2A2:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 16;
                    entry->progInfo = gVu1ProgInfo16;
                    break;
                }
                break;
            case 0x282:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 17;
                    entry->progInfo = gVu1ProgInfo17;
                    break;
                }
                break;
            case 0x8202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 18;
                    entry->progInfo = gVu1ProgInfo18;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 19;
                    entry->progInfo = gVu1ProgInfo19;
                    break;
                }
                break;
            case 0x28202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 20;
                    entry->progInfo = gVu1ProgInfo20;
                    break;
                }
                break;
            case 0x8200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 21;
                    entry->progInfo = gVu1ProgInfo21;
                    break;
                }
                break;
            case 0x8000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 22;
                    entry->progInfo = gVu1ProgInfo22;
                    break;
                }
                break;
            case 0x8002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 23;
                    entry->progInfo = gVu1ProgInfo23;
                    break;
                }
                break;
            case 0x28200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 24;
                    entry->progInfo = gVu1ProgInfo24;
                    break;
                }
                break;
            case 0x28000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 25;
                    entry->progInfo = gVu1ProgInfo25;
                    break;
                }
                break;
            case 0x28002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 26;
                    entry->progInfo = gVu1ProgInfo26;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 27;
                    entry->progInfo = gVu1ProgInfo27;
                    break;
                }
                break;
            case 0x4202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 28;
                    entry->progInfo = gVu1ProgInfo28;
                    break;
                }
                break;
            case 0x24202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 29;
                    entry->progInfo = gVu1ProgInfo29;
                    break;
                }
                break;
            case 0x4200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 30;
                    entry->progInfo = gVu1ProgInfo30;
                    break;
                }
                break;
            case 0x24200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 31;
                    entry->progInfo = gVu1ProgInfo31;
                    break;
                }
                break;
            case 0x4002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 32;
                    entry->progInfo = gVu1ProgInfo32;
                    break;
                }
                break;
            case 0x24002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 33;
                    entry->progInfo = gVu1ProgInfo33;
                    break;
                }
                break;
            case 0x4000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 34;
                    entry->progInfo = gVu1ProgInfo34;
                    break;
                }
                break;
            case 0x24000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 35;
                    entry->progInfo = gVu1ProgInfo35;
                    break;
                }
                break;
            case 0xC202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 36;
                    entry->progInfo = gVu1ProgInfo36;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 37;
                    entry->progInfo = gVu1ProgInfo37;
                    break;
                }
                break;
            case 0x2C202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 38;
                    entry->progInfo = gVu1ProgInfo38;
                    break;
                }
                break;
            case 0xC200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 39;
                    entry->progInfo = gVu1ProgInfo39;
                    break;
                }
                break;
            case 0x2C200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 40;
                    entry->progInfo = gVu1ProgInfo40;
                    break;
                }
                break;
            case 0xC002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 41;
                    entry->progInfo = gVu1ProgInfo41;
                    break;
                }
                break;
            case 0x2C002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 42;
                    entry->progInfo = gVu1ProgInfo42;
                    break;
                }
                break;
            case 0xC000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 43;
                    entry->progInfo = gVu1ProgInfo43;
                    break;
                }
                break;
            case 0x2C000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 44;
                    entry->progInfo = gVu1ProgInfo44;
                    break;
                }
                break;
            case 0x28080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 45;
                    entry->progInfo = gVu1ProgInfo45;
                    break;
                }
                break;
            case 0x2C080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 46;
                    entry->progInfo = gVu1ProgInfo46;
                    break;
                }
                break;
            case 0x2C280:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 47;
                    entry->progInfo = gVu1ProgInfo47;
                    break;
                }
                break;
            case 0x28022:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 48;
                    entry->progInfo = gVu1ProgInfo48;
                    break;
                }
                break;
            case 0x4222:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 49;
                    entry->progInfo = gVu1ProgInfo49;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 50;
                    entry->progInfo = gVu1ProgInfo50;
                    break;
                }
                break;
            case 0x24020:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 51;
                    entry->progInfo = gVu1ProgInfo51;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 52;
                    entry->progInfo = gVu1ProgInfo52;
                    break;
                }
                break;
            case 0x200A0:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 53;
                    entry->progInfo = gVu1ProgInfo53;
                    break;
                }
                break;
            case 0x28020:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 54;
                    entry->progInfo = gVu1ProgInfo54;
                    break;
                }
                break;
            case 0x8080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 55;
                    entry->progInfo = gVu1ProgInfo55;
                    break;
                }
                break;
            case 0x4080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 56;
                    entry->progInfo = gVu1ProgInfo56;
                    break;
                }
                break;
            case 0xC080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 57;
                    entry->progInfo = gVu1ProgInfo57;
                    break;
                }
                break;
            case 0x24080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 58;
                    entry->progInfo = gVu1ProgInfo58;
                    break;
                }
                break;
            case 0x280A0:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 59;
                    entry->progInfo = gVu1ProgInfo59;
                    break;
                }
                break;
            }
        break;
    }
}

/* unk_2EF020's .data (0x402570-0x403140) */
void Vu1Pkt_D(u8 *e, u32 prog, u32 next);
void Vu1Pkt_DE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_DEP(u8 *e, u32 prog, u32 next);
void Vu1Pkt_DP(u8 *e, u32 prog, u32 next);
void Vu1Pkt_DV(u8 *e, u32 prog, u32 next);
void Vu1Pkt_DVE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_DVEP(u8 *e, u32 prog, u32 next);
void Vu1Pkt_DVP(u8 *e, u32 prog, u32 next);
void Vu1Pkt_E(u8 *e, u32 prog, u32 next);
void Vu1Pkt_L(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LD(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LDE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LDV(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LDVE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LDVEP(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LP(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LV(u8 *e, u32 prog, u32 next);
void Vu1Pkt_LVE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_P(u8 *e, u32 prog, u32 next);
void Vu1Pkt_Plain(u8 *e, u32 prog, u32 next);
void Vu1Pkt_SDV(u8 *e, u32 prog, u32 next);
void Vu1Pkt_SL(u8 *e, u32 prog, u32 next);
void Vu1Pkt_SLD(u8 *e, u32 prog, u32 next);
void Vu1Pkt_SLDE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_SVE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_V(u8 *e, u32 prog, u32 next);
void Vu1Pkt_VE(u8 *e, u32 prog, u32 next);
void Vu1Pkt_VEP(u8 *e, u32 prog, u32 next);
void Vu1Pkt_VP(u8 *e, u32 prog, u32 next);
s32 gBonePartAnim = {
#include "sub_main/00402570.inc"
};
/* The VU1 programs (src/sub_main/vu1_microcode.vsm) the descriptors load. */
extern u32 Vu1Prog00[];
extern u32 Vu1Prog01[];
extern u32 Vu1Prog02[];
extern u32 Vu1Prog03[];
extern u32 Vu1Prog04[];
extern u32 Vu1Prog05[];
extern u32 Vu1Prog06[];
extern u32 Vu1Prog07[];
extern u32 Vu1Prog08[];
extern u32 Vu1Prog09[];
extern u32 Vu1Prog10[];
extern u32 Vu1Prog11[];
extern u32 Vu1Prog12[];
extern u32 Vu1Prog13[];
extern u32 Vu1Prog14[];
extern u32 Vu1Prog15[];
extern u32 Vu1Prog16[];
extern u32 Vu1Prog17[];
extern u32 Vu1Prog18[];
extern u32 Vu1Prog19[];
extern u32 Vu1Prog20[];
extern u32 Vu1Prog21[];
extern u32 Vu1Prog22[];
extern u32 Vu1Prog23[];
extern u32 Vu1Prog24[];
extern u32 Vu1Prog25[];
extern u32 Vu1Prog26[];
extern u32 Vu1Prog27[];
extern u32 Vu1Prog28[];
extern u32 Vu1Prog29[];
extern u32 Vu1Prog30[];
extern u32 Vu1Prog31[];
extern u32 Vu1Prog32[];
extern u32 Vu1Prog33[];
extern u32 Vu1Prog34[];
extern u32 Vu1Prog35[];
extern u32 Vu1Prog36[];
extern u32 Vu1Prog37[];
extern u32 Vu1Prog38[];
extern u32 Vu1Prog39[];
extern u32 Vu1Prog40[];
extern u32 Vu1Prog41[];
extern u32 Vu1Prog42[];
extern u32 Vu1Prog43[];
extern u32 Vu1Prog44[];
extern u32 Vu1Prog45[];
extern u32 Vu1Prog46[];
extern u32 Vu1Prog47[];
extern u32 Vu1Prog48[];
extern u32 Vu1Prog49[];
extern u32 Vu1Prog50[];
extern u32 Vu1Prog51[];
extern u32 Vu1Prog52[];
extern u32 Vu1Prog53[];
extern u32 Vu1Prog54[];
extern u32 Vu1Prog55[];
extern u32 Vu1Prog56[];
extern u32 Vu1Prog57[];
extern u32 Vu1Prog58[];
extern u32 Vu1Prog59[];
s32 gVu1ProgInfo00[12] = {
#include "sub_main/00402580.inc"
};
u32 D_004025B0[4] = {
#include "sub_main/004025B0.inc"
};
s32 gVu1ProgInfo01[12] = {
#include "sub_main/004025C0.inc"
};
s32 gVu1ProgInfo02[12] = {
#include "sub_main/004025F0.inc"
};
s32 gVu1ProgInfo03[12] = {
#include "sub_main/00402620.inc"
};
u32 D_00402650[4] = {
#include "sub_main/00402650.inc"
};
s32 gVu1ProgInfo04[12] = {
#include "sub_main/00402660.inc"
};
s32 gVu1ProgInfo05[12] = {
#include "sub_main/00402690.inc"
};
s32 gVu1ProgInfo06[12] = {
#include "sub_main/004026C0.inc"
};
s32 gVu1ProgInfo07[12] = {
#include "sub_main/004026F0.inc"
};
s32 gVu1ProgInfo08[12] = {
#include "sub_main/00402720.inc"
};
s32 gVu1ProgInfo09[12] = {
#include "sub_main/00402750.inc"
};
s32 gVu1ProgInfo10[12] = {
#include "sub_main/00402780.inc"
};
s32 gVu1ProgInfo11[12] = {
#include "sub_main/004027B0.inc"
};
s32 gVu1ProgInfo12[12] = {
#include "sub_main/004027E0.inc"
};
s32 gVu1ProgInfo13[12] = {
#include "sub_main/00402810.inc"
};
s32 gVu1ProgInfo14[12] = {
#include "sub_main/00402840.inc"
};
u32 D_00402870[4] = {
#include "sub_main/00402870.inc"
};
s32 gVu1ProgInfo15[12] = {
#include "sub_main/00402880.inc"
};
s32 gVu1ProgInfo16[12] = {
#include "sub_main/004028B0.inc"
};
s32 gVu1ProgInfo17[12] = {
#include "sub_main/004028E0.inc"
};
s32 gVu1ProgInfo18[12] = {
#include "sub_main/00402910.inc"
};
u32 D_00402940[4] = {
#include "sub_main/00402940.inc"
};
s32 gVu1ProgInfo19[12] = {
#include "sub_main/00402950.inc"
};
s32 gVu1ProgInfo20[12] = {
#include "sub_main/00402980.inc"
};
s32 gVu1ProgInfo21[12] = {
#include "sub_main/004029B0.inc"
};
s32 gVu1ProgInfo22[12] = {
#include "sub_main/004029E0.inc"
};
s32 gVu1ProgInfo23[12] = {
#include "sub_main/00402A10.inc"
};
s32 gVu1ProgInfo24[12] = {
#include "sub_main/00402A40.inc"
};
s32 gVu1ProgInfo25[12] = {
#include "sub_main/00402A70.inc"
};
s32 gVu1ProgInfo26[12] = {
#include "sub_main/00402AA0.inc"
};
u32 D_00402AD0[4] = {
#include "sub_main/00402AD0.inc"
};
s32 gVu1ProgInfo27[12] = {
#include "sub_main/00402AE0.inc"
};
s32 gVu1ProgInfo28[12] = {
#include "sub_main/00402B10.inc"
};
s32 gVu1ProgInfo29[12] = {
#include "sub_main/00402B40.inc"
};
s32 gVu1ProgInfo30[12] = {
#include "sub_main/00402B70.inc"
};
s32 gVu1ProgInfo31[12] = {
#include "sub_main/00402BA0.inc"
};
s32 gVu1ProgInfo32[12] = {
#include "sub_main/00402BD0.inc"
};
s32 gVu1ProgInfo33[12] = {
#include "sub_main/00402C00.inc"
};
s32 gVu1ProgInfo34[12] = {
#include "sub_main/00402C30.inc"
};
s32 gVu1ProgInfo35[12] = {
#include "sub_main/00402C60.inc"
};
s32 gVu1ProgInfo36[12] = {
#include "sub_main/00402C90.inc"
};
u32 D_00402CC0[4] = {
#include "sub_main/00402CC0.inc"
};
s32 gVu1ProgInfo37[12] = {
#include "sub_main/00402CD0.inc"
};
s32 gVu1ProgInfo38[12] = {
#include "sub_main/00402D00.inc"
};
s32 gVu1ProgInfo39[12] = {
#include "sub_main/00402D30.inc"
};
s32 gVu1ProgInfo40[12] = {
#include "sub_main/00402D60.inc"
};
s32 gVu1ProgInfo41[12] = {
#include "sub_main/00402D90.inc"
};
s32 gVu1ProgInfo42[12] = {
#include "sub_main/00402DC0.inc"
};
s32 gVu1ProgInfo43[12] = {
#include "sub_main/00402DF0.inc"
};
s32 gVu1ProgInfo44[12] = {
#include "sub_main/00402E20.inc"
};
s32 gVu1ProgInfo45[12] = {
#include "sub_main/00402E50.inc"
};
s32 gVu1ProgInfo46[12] = {
#include "sub_main/00402E80.inc"
};
s32 gVu1ProgInfo47[12] = {
#include "sub_main/00402EB0.inc"
};
s32 gVu1ProgInfo48[12] = {
#include "sub_main/00402EE0.inc"
};
s32 gVu1ProgInfo49[12] = {
#include "sub_main/00402F10.inc"
};
u32 D_00402F40[4] = {
#include "sub_main/00402F40.inc"
};
s32 gVu1ProgInfo50[12] = {
#include "sub_main/00402F50.inc"
};
s32 gVu1ProgInfo51[12] = {
#include "sub_main/00402F80.inc"
};
u32 D_00402FB0[4] = {
#include "sub_main/00402FB0.inc"
};
s32 gVu1ProgInfo52[12] = {
#include "sub_main/00402FC0.inc"
};
s32 gVu1ProgInfo53[12] = {
#include "sub_main/00402FF0.inc"
};
s32 gVu1ProgInfo54[12] = {
#include "sub_main/00403020.inc"
};
s32 gVu1ProgInfo55[12] = {
#include "sub_main/00403050.inc"
};
s32 gVu1ProgInfo56[12] = {
#include "sub_main/00403080.inc"
};
s32 gVu1ProgInfo57[12] = {
#include "sub_main/004030B0.inc"
};
s32 gVu1ProgInfo58[12] = {
#include "sub_main/004030E0.inc"
};
s32 gVu1ProgInfo59[12] = {
#include "sub_main/00403110.inc"
};
