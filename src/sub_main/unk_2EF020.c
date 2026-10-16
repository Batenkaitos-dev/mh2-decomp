/*
 * The bones: blending, posing and world matrices of a bone tree (Bone_*), 0x2EF020-0x2F17C0.
 * Another unit than unk_2E3940.c's Player_UpdateMotion, which declares the Bone_* calls otherwise.
 * Where the unit starts (after 0x2EEA70, Math_SinCos's last caller, by 0x2EF0F0) is not known.
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

/* Built at optimization level 4 like unk_2E3940.c (every function matches either way). */
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
extern Mtx44Copy gMtxSlots[];
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
s32 Bone_ReadDef(EmBone *b, u8 *data, s32 i);

/* out = how far b's animation moves it from time t0 to time t1 (the translations' difference). */
s32 Bone_GetAnimMove(f32 *out, EmBone *b, f32 t0, f32 t1)
{
    f32 m0[16];
    f32 m1[16];
    f32 pose[12];
    AnimHeader *hdr;

    Anim_SetData(b->anim);
    hdr = (AnimHeader *)(b->animOfs + gAnimData);
    Anim_EvalPoseMatrix(hdr, b->base, pose, m0, b->keyHint, t0);
    Anim_EvalPoseMatrix(hdr, b->base, pose, m1, b->keyHint, t1);
    out[0] = m1[12] - m0[12];
    out[1] = m1[13] - m0[13];
    out[2] = m1[14] - m0[14];
    return 1;
}

/* Blends the part id of tree a with tree b (Bone_BlendTree), from each tree's first node of it. */
s32 Bone_BlendPart(f32 wa, f32 wb, EmBone *a, EmBone *b, s32 id)
{
    EmBone *na = Bone_FindPart(a, id);

    Bone_BlendTree(na, Bone_FindPart(b, id), wa, wb, id);
    return 1;
}

/*
 * a = a * wa + b * wb (the first three pose values and m1) for the nodes of part id
 * connected to a, walking the two trees (of the same shape) together.
 */
s32 Bone_BlendTree(EmBone *a, EmBone *b, f32 wa, f32 wb, s32 id)
{
    EmBone *root = a;

top:
    if (a->id == (u16)id) {
        a->pose[0] = a->pose[0] * wa + b->pose[0] * wb;
        a->pose[1] = a->pose[1] * wa + b->pose[1] * wb;
        a->pose[2] = a->pose[2] * wa + b->pose[2] * wb;
        Mtx_Blend(a->local, a->local, b->local, wa, wb);
        if (a->child != NULL) {
            b = b->child;
            a = a->child;
            goto top;
        }
    }
    if (a->sibling != NULL && a != root) {
        b = b->sibling;
        a = a->sibling;
        goto top;
    }
    while (a != root) {
        while (a->sibling != NULL) {
            a = a->sibling;
            b = b->sibling;
            if (a->id == (u16)id) {
                goto top;
            }
        }
        a = a->parent;
        b = b->parent;
    }
    return 1;
}

/* m0 = m1 times the parent's m0 (m for the root) for every node of the tree (depth first). */
void Bone_UpdateWorld(EmBone *b, f32 *m)
{
    EmBone *root = b;

loop:
    Mtx_Mul(b->world, b->local, m);
    if (b->child != NULL) {
        m = b->world;
        b = b->child;
        goto loop;
    }
    if (b->sibling != NULL) {
        b = b->sibling;
        goto loop;
    }
    while (b != root) {
        if (b->sibling != NULL) {
            b = b->sibling;
            m = b->parent->world;
            goto loop;
        }
        b = b->parent;
    }
}

/* The matrix slot of every node of the tree = its m2 times its m0. */
void Bone_StoreMatrices(EmBone *b)
{
    EmBone *root = b;

loop:
    Mtx_Mul(gMtxSlots[b->index].m[0], b->skin, b->world);
    if (b->child != NULL) {
        b = b->child;
        goto loop;
    }
    if (b->sibling != NULL) {
        b = b->sibling;
        goto loop;
    }
    while (b != root) {
        if (b->sibling != NULL) {
            b = b->sibling;
            goto loop;
        }
        b = b->parent;
    }
}

/* As Bone_StoreMatrices, into the matrix table tbl. */
void Bone_StoreMatricesTo(Mtx44Copy *tbl, EmBone *b)
{
    EmBone *root = b;

loop:
    Mtx_Mul(tbl[b->index].m[0], b->skin, b->world);
    if (b->child != NULL) {
        b = b->child;
        goto loop;
    }
    if (b->sibling != NULL) {
        b = b->sibling;
        goto loop;
    }
    while (b != root) {
        if (b->sibling != NULL) {
            b = b->sibling;
            goto loop;
        }
        b = b->parent;
    }
}

void Mem_Copy64ByQwords(void *src, void *dst, s32 n);

/* Matrix slots 0..n-1 (gMtxSlots) = tbl[list[1..n]] (n = list[0]). */
void MtxSlot_LoadList(s16 *list, Mtx44Copy *tbl)
{
    s32 i;
    s32 n;
    s16 *p;

    n = list[0];
    p = &list[1];
    for (i = 0; i < n; i++) {
        Mem_Copy64ByQwords(&tbl[*p], &gMtxSlots[i], 1);
        p++;
    }
}

extern s32 gBonePartAnim;
s32 Bone_SetAnimTree(EmBone *b, s32 *ofs, s32 id);

/*
 * Gives part id of the tree at b the animation h: each node's offset comes from the
 * table at +0x10 of the animation's data.
 */
s32 Bone_SetPartAnim(EmBone *b, s32 h, s32 id)
{
    s32 *ofs;
    u8 *p;
    EmBone *n;

    p = Mem_GetAddr((void *)h);
    gAnimData = (s32)p;
    ofs = (s32 *)(*(s32 *)(p + 0x10) + gAnimData);
    gBonePartAnim = h;
    n = Bone_FindPart(b, id);
    if (n != NULL) {
        Bone_SetAnimTree(n, ofs, id);
    }
    return 1;
}

/* The nodes of part id connected to b get the animation gBonePartAnim and the next offsets. */
s32 Bone_SetAnimTree(EmBone *b, s32 *ofs, s32 id)
{
    EmBone *root = b;

top:
    if (b->id == (u16)id) {
        b->anim = gBonePartAnim;
        b->animOfs = *ofs++;
        if (b->child != NULL) {
            b = b->child;
            goto top;
        }
    }
    if (b->sibling != NULL && b != root) {
        b = b->sibling;
        goto top;
    }
    while (b != root) {
        while (b->sibling != NULL) {
            b = b->sibling;
            if (b->id == (u16)id) {
                goto top;
            }
        }
        b = b->parent;
    }
    return 1;
}

/* The node of the tree at b whose id is id (depth first), or NULL. */
EmBone *Bone_FindPart(EmBone *b, s32 id)
{
    EmBone *root = b;

loop:
    if (b->id == (u16)id) {
        return b;
    }
    if (b->child != NULL) {
        b = b->child;
        goto loop;
    }
    if (b->sibling != NULL) {
        b = b->sibling;
        goto loop;
    }
    while (b != root) {
        if (b->sibling != NULL) {
            b = b->sibling;
            goto loop;
        }
        b = b->parent;
    }
    return NULL;
}

f32 Motion_GetLength(s32 handle)
{
    return *(f32 *)((u8 *)(Mem_GetAddr((void *) handle)) + 8);
}

/* *out = the float at +0xC of animation h when its flag 0x8000 is set (1), else 0 (0). */
s32 Anim_GetLoopTime(s32 h, f32 *out)
{
    u8 *p = Mem_GetAddr((void *)h);

    if (*(u16 *)p & 0x8000) {
        *out = *(f32 *)(p + 0xC);
        return 1;
    }
    *out = 0.0f;
    return 0;
}

/*
 * pose = base with the curves of animation hdr at time t (format 0x2000 stores fixed
 * point: angles in 0x4000ths of a turn, the rest in 16ths), then m = its scale, rotation
 * and translation.
 */
s32 Anim_EvalPoseMatrix(AnimHeader *hdr, f32 *base, f32 *pose, f32 *m, s16 *keys, f32 t)
{
    s32 i;
    AnimCurve *c;

    for (i = 0; i < 10; i++) {
        pose[i] = base[i];
    }
    if (hdr != NULL) {
        switch (hdr->flags & ANIM_TYPE_MASK) {
        case 0x1000:
            c = (AnimCurve *)(hdr->curves + gAnimData);
            for (i = 0; i < hdr->count; i++, c++) {
                u16 k = c->idx;

                pose[k] = Anim_EvalCurve(t, c, &keys[k]);
            }
            break;
        case 0x2000:
            c = (AnimCurve *)(hdr->curves + gAnimData);
            for (i = 0; i < hdr->count; i++, c++) {
                u16 k = c->idx;
                f32 *p = &pose[k];

                *p = Anim_EvalCurve(t, c, &keys[k]);
                switch (k) {
                case 3:
                case 4:
                case 5:
                    *p *= 3.1415927f / 8192.0f;
                    break;
                default:
                    *p /= 16.0f;
                    break;
                }
            }
            break;
        }
    }
    Mtx_Scale(m, pose[0], pose[1], pose[2]);
    Mtx_RotateXYZ(m, pose[3], pose[4], pose[5]);
    m[12] = pose[6];
    m[13] = pose[7];
    m[14] = pose[8];
    return 1;
}

/* Poses part id of the tree at b at time t (Bone_PoseTree from its first node of it). */
s32 Bone_PosePart(f32 t, EmBone *b, s32 id)
{
    Bone_PoseTree(t, Bone_FindPart(b, id), id);
    return 1;
}

/*
 * The nodes of part id from b on: pose = the rest pose, plus the node's animation at
 * time t when it has one, then m1 from the pose.
 */
s32 Bone_PoseTree(f32 t, EmBone *b, s32 id)
{
    s32 i;

    Anim_SetData(b->anim);
    if (b->id == (u16)id) {
        for (i = 0; i < 10; i++) {
            b->pose[i] = b->base[i];
        }
        if (b->anim != 0) {
            Anim_EvalPose((AnimHeader *)(b->animOfs + gAnimData), b->base, b->pose, b->keyHint, t);
        }
        Mtx_FromTransform(b->local, b->pose);
        if (b->child != NULL) {
            Bone_PoseTree(t, b->child, id);
        }
    }
    if (b->sibling != NULL) {
        Bone_PoseTree(t, b->sibling, id);
    }
    return 1;
}

/* Bone_UpdateWorldScaled over the whole tree from the identity (with m, the parent matrix). */
void Bone_UpdateWorldScaledRoot(EmBone *b, f32 *m)
{
    Bone_UpdateWorldScaled(b, m, 1.0f, 1.0f, 1.0f);
}

/*
 * m0 for b and the nodes after it: its m1 with the pose's scale times the scale passed
 * down (sx, sy, sz, which the children inherit), times m.
 */
void Bone_UpdateWorldScaled(EmBone *b, f32 *m, f32 sx, f32 sy, f32 sz)
{
    f32 inv[16];
    f32 x = b->pose[0] * sx;
    f32 y = b->pose[1] * sy;
    f32 z = b->pose[2] * sz;

    Mtx_Scale(b->world, x, y, z);
    Mtx_Identity(inv);
    inv[0] = 1.0f / sx;
    inv[5] = 1.0f / sy;
    inv[10] = 1.0f / sz;
    inv[12] = b->local[12];
    inv[13] = b->local[13];
    inv[14] = b->local[14];
    Mtx_Concat3x3(b->world, b->local);
    Mtx_Concat(b->world, inv);
    Mtx_Concat(b->world, m);
    if (b->child != NULL) {
        Bone_UpdateWorldScaled(b->child, b->world, x, y, z);
    }
    if (b->sibling != NULL) {
        Bone_UpdateWorldScaled(b->sibling, m, sx, sy, sz);
    }
}

/* Bone_UpdateWorldScaledUpTo from the identity scale. */
void Bone_UpdateWorldScaledUpToRoot(EmBone *b, f32 *m, u8 maxId)
{
    Bone_UpdateWorldScaledUpTo(b, m, maxId, 1.0f, 1.0f, 1.0f);
}

/* As Bone_UpdateWorldScaled, for the nodes whose id is at most maxId (a node above it stops its branch). */
void Bone_UpdateWorldScaledUpTo(EmBone *b, f32 *m, u8 maxId, f32 sx, f32 sy, f32 sz)
{
    f32 inv[16];
    f32 x;
    f32 y;
    f32 z;

    if (b->id <= maxId) {
        x = b->pose[0] * sx;
        y = b->pose[1] * sy;
        z = b->pose[2] * sz;
        Mtx_Scale(b->world, x, y, z);
        Mtx_Identity(inv);
        inv[0] = 1.0f / sx;
        inv[5] = 1.0f / sy;
        inv[10] = 1.0f / sz;
        inv[12] = b->local[12];
        inv[13] = b->local[13];
        inv[14] = b->local[14];
        Mtx_Concat3x3(b->world, b->local);
        Mtx_Concat(b->world, inv);
        Mtx_Concat(b->world, m);
        if (b->child != NULL) {
            Bone_UpdateWorldScaledUpTo(b->child, b->world, maxId, x, y, z);
        }
        if (b->sibling != NULL) {
            Bone_UpdateWorldScaledUpTo(b->sibling, m, maxId, sx, sy, sz);
        }
    }
}

void Bone_InitTreeRoot(EmBone *bones, s32 h)
{
    Bone_InitTree(bones, h, 0, 0);
}

/*
 * Sets up the bone tree bones from the skeleton file h: clears the nodes, reads each
 * definition, links the tree, then the rest matrices (mode 0 or 1).
 */
s32 Bone_InitTree(EmBone *bones, s32 h, s32 unused, s32 mode)
{
    s32 i;
    s32 n;
    u8 *data;
    EmBone *b;

    data = Mem_GetAddr((void *)h);
    n = ((SkelHeader *)data)->count;
    for (i = 0; i < n; i++) {
        b = &bones[i];
        b->count = n;
        b->siblingIdx = -1;
        b->childIdx = -1;
        b->sibling = NULL;
        b->child = NULL;
        b->anim = 0;
        b->animOfs = 0;
        b->unk178 = 0;
        b->unk17C = 0;
        Mtx_Identity(b->local);
    }
    for (i = 0; i < n; i++) {
        Bone_ReadDef(&bones[i], data, i);
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

/* links b and the bones after it in the tree (parts, the model's bone array): parent, and child and sibling
 * from the indices childIdx and siblingIdx (-1 none) */
void Bone_LinkTree(EmBone *parts, EmBone *b, EmBone *parent)
{
    s16 sibling;
    s16 child;

    if (parent != NULL) {
        b->parent = parent;
    }
    child = b->childIdx;
    if (child != -1) {
        b->child = &parts[child];
        Bone_LinkTree(parts, b->child, b);
    }
    sibling = b->siblingIdx;
    if (sibling != -1) {
        b->sibling = &parts[sibling];
        Bone_LinkTree(parts, b->sibling, parent);
    }
}


void Bone_InitRestPoseUnit(EmBone *b)
{
    Bone_InitRestPose(b, 1.0f, 1.0f, 1.0f, (f32 *)gMtxIdentity);
}

/*
 * The rest pose of b and the nodes after it: pose = base, m1 from it (Bone_PoseMatrixScaled), and
 * m2 = the inverse of m1 times m (the parent's, the children get it).
 */
s32 Bone_InitRestPose(EmBone *b, f32 sx, f32 sy, f32 sz, f32 *m)
{
    f32 w[16];
    s32 i;

    for (i = 0; i < 10; i++) {
        b->pose[i] = b->base[i];
    }
    Bone_PoseMatrixScaled(b->local, b->pose, sx, sy, sz);
    Mtx_Mul(w, b->local, m);
    Mtx_Inverse(b->skin, w);
    if (b->child != NULL) {
        Bone_InitRestPose(b->child, sx * b->pose[0], sy * b->pose[1], sz * b->pose[2], w);
    }
    if (b->sibling != NULL) {
        Bone_InitRestPose(b->sibling, sx, sy, sz, m);
    }
    return 1;
}

void Bone_InitRestPoseAltUnit(EmBone *b)
{
    Bone_InitRestPoseAlt(b, 1.0f, 1.0f, 1.0f, (f32 *)gMtxIdentity);
}

/* As Bone_InitRestPose, with m1 from Mtx_FromTransform and the inverse from Bone_PoseMatrix's matrix. */
s32 Bone_InitRestPoseAlt(EmBone *b, f32 sx, f32 sy, f32 sz, f32 *m)
{
    f32 w[16];
    f32 t[16];
    s32 i;

    for (i = 0; i < 10; i++) {
        b->pose[i] = b->base[i];
    }
    Mtx_FromTransform(b->local, b->pose);
    Bone_PoseMatrix(t, b->pose, sx, sy, sz);
    Mtx_Mul(w, t, m);
    Mtx_Inverse(b->skin, w);
    if (b->child != NULL) {
        Bone_InitRestPoseAlt(b->child, sx * b->pose[0], sy * b->pose[1], sz * b->pose[2], w);
    }
    if (b->sibling != NULL) {
        Bone_InitRestPoseAlt(b->sibling, sx, sy, sz, m);
    }
    return 1;
}

/* m = the rotation (t[3..5], X then Y then Z) and the translation (t[6..8]) of t. */
s32 Mtx_FromTransform(f32 *m, f32 *t)
{
    Mtx_Identity(m);
    Mtx_RotateXYZ(m, t[3], t[4], t[5]);
    m[12] = t[6];
    m[13] = t[7];
    m[14] = t[8];
    return 1;
}

/*
 * m = the pose's scale times (sx, sy, sz), its rotation, then the inverse of that extra
 * scale with the pose's translation.
 */
s32 Bone_PoseMatrixScaled(f32 *m, f32 *pose, f32 sx, f32 sy, f32 sz)
{
    f32 rot[16];
    f32 inv[16];

    Mtx_Scale(m, pose[0] * sx, pose[1] * sy, pose[2] * sz);
    Mtx_Identity(rot);
    Mtx_RotateXYZ(rot, pose[3], pose[4], pose[5]);
    Mtx_Identity(inv);
    inv[0] = 1.0f / sx;
    inv[5] = 1.0f / sy;
    inv[10] = 1.0f / sz;
    inv[12] = pose[6];
    inv[13] = pose[7];
    inv[14] = pose[8];
    Mtx_Concat(m, rot);
    Mtx_Concat(m, inv);
    return 1;
}

/* As Bone_PoseMatrixScaled, with the pose's own scale. */
s32 Bone_PoseMatrix(f32 *m, f32 *pose, f32 sx, f32 sy, f32 sz)
{
    f32 rot[16];
    f32 inv[16];

    Mtx_Scale(m, pose[0], pose[1], pose[2]);
    Mtx_Identity(rot);
    Mtx_RotateXYZ(rot, pose[3], pose[4], pose[5]);
    Mtx_Identity(inv);
    inv[0] = 1.0f / sx;
    inv[5] = 1.0f / sy;
    inv[10] = 1.0f / sz;
    inv[12] = pose[6];
    inv[13] = pose[7];
    inv[14] = pose[8];
    Mtx_Concat(m, rot);
    Mtx_Concat(m, inv);
    return 1;
}

/* Bone i of a built skeleton (data: a SkelHeader, then its BoneDefs) into
 * b: its ids, links and rest pose. */
s32 Bone_ReadDef(EmBone *b, u8 *data, s32 i)
{
    BoneDef *d = (BoneDef *)((SkelHeader *)data + 1);

    d += i;
    b->unk174 = d->unkA;
    b->index = d->index;
    b->id = d->id;
    b->childIdx = d->child;
    b->siblingIdx = d->sibling;
    b->pose[0] = b->base[0] = d->scale[0];
    b->pose[1] = b->base[1] = d->scale[1];
    b->pose[2] = b->base[2] = d->scale[2];
    b->pose[3] = b->base[3] = d->rot[0];
    b->pose[4] = b->base[4] = d->rot[1];
    b->pose[5] = b->base[5] = d->rot[2];
    b->pose[6] = b->base[6] = d->trans[0];
    b->pose[7] = b->base[7] = d->trans[1];
    b->pose[8] = b->base[8] = d->trans[2];
    b->animOfs = 0;
    b->anim = 0;
    return 1;
}

/* The curves of animation hdr at time t into pose (as Anim_EvalPoseMatrix, without the matrix). */
s32 Anim_EvalPose(AnimHeader *hdr, f32 *base, f32 *pose, s16 *keys, f32 t)
{
    s32 i;
    AnimCurve *c;

    if (hdr != NULL) {
        switch (hdr->flags & ANIM_TYPE_MASK) {
        case 0x1000:
            c = (AnimCurve *)(hdr->curves + gAnimData);
            for (i = 0; i < hdr->count; i++, c++) {
                u16 k = c->idx;

                pose[k] = Anim_EvalCurve(t, c, &keys[k]);
            }
            break;
        case 0x2000:
            c = (AnimCurve *)(hdr->curves + gAnimData);
            for (i = 0; i < hdr->count; i++, c++) {
                u16 k = c->idx;
                f32 *p = &pose[k];

                *p = Anim_EvalCurve(t, c, &keys[k]);
                switch (k) {
                case 3:
                case 4:
                case 5:
                    *p *= 3.1415927f / 8192.0f;
                    break;
                default:
                    *p /= 16.0f;
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
 * Picks model e's VU1 program: e->prog (0..0x3B) and its descriptor e->progInfo, from the
 * low 24 bits of the word at 0x0C (its top byte must be 0) and the bits of the words at 0x04
 * and 0x08; -1 and NULL when no program fits. The top byte is tested by a one-case switch
 * (`beqz` over a `b` to the end). hdr is unused.
 */
void Vu1_SelectProgram(ModelEntry *e, MeshDesc *hdr)
{
    u32 key;
    u32 lo;
    s32 fmt;

    e->prog = -1;
    e->progInfo = NULL;
    key = e->frame;
    fmt = e->used | e->unk08;
    lo = key & 0xFFFFFF;
    switch (key & 0xFF000000) {
    case 0:
            switch (lo) {
            case 0x202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 0;
                    e->progInfo = gVu1ProgInfo00;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 1;
                    e->progInfo = gVu1ProgInfo01;
                    break;
                }
                break;
            case 0x20202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 2;
                    e->progInfo = gVu1ProgInfo02;
                    break;
                }
                break;
            case 0x222:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 3;
                    e->progInfo = gVu1ProgInfo03;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 4;
                    e->progInfo = gVu1ProgInfo04;
                    break;
                }
                break;
            case 0x20020:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 5;
                    e->progInfo = gVu1ProgInfo05;
                    break;
                }
                break;
            case 0x80:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 6;
                    e->progInfo = gVu1ProgInfo06;
                    break;
                }
                break;
            case 0x20080:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 7;
                    e->progInfo = gVu1ProgInfo07;
                    break;
                }
                break;
            case 0x200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 8;
                    e->progInfo = gVu1ProgInfo08;
                    break;
                }
                break;
            case 0x20200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 9;
                    e->progInfo = gVu1ProgInfo09;
                    break;
                }
                break;
            case 0x0:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 10;
                    e->progInfo = gVu1ProgInfo10;
                    break;
                }
                break;
            case 0x2:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 11;
                    e->progInfo = gVu1ProgInfo11;
                    break;
                }
                break;
            case 0x20000:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 12;
                    e->progInfo = gVu1ProgInfo12;
                    break;
                }
                break;
            case 0x20002:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 13;
                    e->progInfo = gVu1ProgInfo13;
                    break;
                }
                break;
            case 0x8222:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 14;
                    e->progInfo = gVu1ProgInfo14;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 15;
                    e->progInfo = gVu1ProgInfo15;
                    break;
                }
                break;
            case 0x2A2:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 16;
                    e->progInfo = gVu1ProgInfo16;
                    break;
                }
                break;
            case 0x282:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 17;
                    e->progInfo = gVu1ProgInfo17;
                    break;
                }
                break;
            case 0x8202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 18;
                    e->progInfo = gVu1ProgInfo18;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 19;
                    e->progInfo = gVu1ProgInfo19;
                    break;
                }
                break;
            case 0x28202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 20;
                    e->progInfo = gVu1ProgInfo20;
                    break;
                }
                break;
            case 0x8200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 21;
                    e->progInfo = gVu1ProgInfo21;
                    break;
                }
                break;
            case 0x8000:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 22;
                    e->progInfo = gVu1ProgInfo22;
                    break;
                }
                break;
            case 0x8002:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 23;
                    e->progInfo = gVu1ProgInfo23;
                    break;
                }
                break;
            case 0x28200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 24;
                    e->progInfo = gVu1ProgInfo24;
                    break;
                }
                break;
            case 0x28000:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 25;
                    e->progInfo = gVu1ProgInfo25;
                    break;
                }
                break;
            case 0x28002:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 26;
                    e->progInfo = gVu1ProgInfo26;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 27;
                    e->progInfo = gVu1ProgInfo27;
                    break;
                }
                break;
            case 0x4202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 28;
                    e->progInfo = gVu1ProgInfo28;
                    break;
                }
                break;
            case 0x24202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 29;
                    e->progInfo = gVu1ProgInfo29;
                    break;
                }
                break;
            case 0x4200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 30;
                    e->progInfo = gVu1ProgInfo30;
                    break;
                }
                break;
            case 0x24200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 31;
                    e->progInfo = gVu1ProgInfo31;
                    break;
                }
                break;
            case 0x4002:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 32;
                    e->progInfo = gVu1ProgInfo32;
                    break;
                }
                break;
            case 0x24002:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 33;
                    e->progInfo = gVu1ProgInfo33;
                    break;
                }
                break;
            case 0x4000:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 34;
                    e->progInfo = gVu1ProgInfo34;
                    break;
                }
                break;
            case 0x24000:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 35;
                    e->progInfo = gVu1ProgInfo35;
                    break;
                }
                break;
            case 0xC202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 36;
                    e->progInfo = gVu1ProgInfo36;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 37;
                    e->progInfo = gVu1ProgInfo37;
                    break;
                }
                break;
            case 0x2C202:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 38;
                    e->progInfo = gVu1ProgInfo38;
                    break;
                }
                break;
            case 0xC200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 39;
                    e->progInfo = gVu1ProgInfo39;
                    break;
                }
                break;
            case 0x2C200:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 40;
                    e->progInfo = gVu1ProgInfo40;
                    break;
                }
                break;
            case 0xC002:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 41;
                    e->progInfo = gVu1ProgInfo41;
                    break;
                }
                break;
            case 0x2C002:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 42;
                    e->progInfo = gVu1ProgInfo42;
                    break;
                }
                break;
            case 0xC000:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 43;
                    e->progInfo = gVu1ProgInfo43;
                    break;
                }
                break;
            case 0x2C000:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 44;
                    e->progInfo = gVu1ProgInfo44;
                    break;
                }
                break;
            case 0x28080:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 45;
                    e->progInfo = gVu1ProgInfo45;
                    break;
                }
                break;
            case 0x2C080:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 46;
                    e->progInfo = gVu1ProgInfo46;
                    break;
                }
                break;
            case 0x2C280:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 47;
                    e->progInfo = gVu1ProgInfo47;
                    break;
                }
                break;
            case 0x28022:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 48;
                    e->progInfo = gVu1ProgInfo48;
                    break;
                }
                break;
            case 0x4222:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 49;
                    e->progInfo = gVu1ProgInfo49;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 50;
                    e->progInfo = gVu1ProgInfo50;
                    break;
                }
                break;
            case 0x24020:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 51;
                    e->progInfo = gVu1ProgInfo51;
                    break;
                case 0x321135:
                case 0x221135:
                    e->prog = 52;
                    e->progInfo = gVu1ProgInfo52;
                    break;
                }
                break;
            case 0x200A0:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 53;
                    e->progInfo = gVu1ProgInfo53;
                    break;
                }
                break;
            case 0x28020:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 54;
                    e->progInfo = gVu1ProgInfo54;
                    break;
                }
                break;
            case 0x8080:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 55;
                    e->progInfo = gVu1ProgInfo55;
                    break;
                }
                break;
            case 0x4080:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 56;
                    e->progInfo = gVu1ProgInfo56;
                    break;
                }
                break;
            case 0xC080:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 57;
                    e->progInfo = gVu1ProgInfo57;
                    break;
                }
                break;
            case 0x24080:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 58;
                    e->progInfo = gVu1ProgInfo58;
                    break;
                }
                break;
            case 0x280A0:
                switch (fmt) {
                case 0x21035:
                case 0x121035:
                    e->prog = 59;
                    e->progInfo = gVu1ProgInfo59;
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
