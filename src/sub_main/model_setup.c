/*
 * Model set-up: a model (Model) is set up from a model file and its
 * texture pack (Model_SetupWithPack and its variants, which differ in how
 * the pack is loaded): its materials (Model_LoadMaterials), its skeleton
 * (Model_InitSkeleton, from the skeletons SkelFile_BuildPart builds) and
 * its material sets, each with its draw states and draw model
 * (Model_InitMatSets). Model_ResetAll empties every model pool, and
 * DrawStates_Apply and Draw_ApplyStates set a material set's render states
 * when it is drawn. The data: the material sets' state tables and the
 * values each draw state selects.
 */

#include "common.h"
#include "model.h"
#include "render_state.h"
#include "text.h"
#include "mem_util.h"
#include "rand.h"
#include "libc.h"

extern s32 gModels;
extern u32 gTexHandles[];
extern u8 *gLoadBuf;

/* Clears every model of the pool at gModels and frees every slot of the
 * material, bone, material set and draw model pools. */
void Model_ResetAll(void)
{
    memset((void *) gModels, 0, MODEL_POOL_COUNT * sizeof(Model));
    Material_Release(0, MATERIAL_POOL_COUNT);
    Bone_Release(0, BONE_POOL_COUNT);
    MatSet_Release(0, MATSET_POOL_COUNT);
    Model_Release(0, MODEL_POOL_COUNT);
}

extern s32 gDrawState15Values[2];
extern s32 gDrawState12Values[4];
extern s32 gDrawState01Values[12];
extern s32 gDrawState02Values[4];
extern s32 gDrawState0CValues[2];
extern s32 gDrawState66Values[2];
extern s32 gDrawState62Values[4];
extern s32 gDrawState00Values[4];

/* Sets the render states of ds and packs its other settings into *flags
 * (bit 0 set; for Draw_ApplyStates). */
void DrawStates_Apply(DrawStates *states, s32 *flags)
{
    Gs_SetState(21, gDrawState15Values[states->s15]);
    Gs_SetState(18, gDrawState12Values[states->s12]);
    Gs_SetState(1, gDrawState01Values[states->s01]);
    Gs_SetState(2, gDrawState02Values[states->s02]);
    Gs_SetState(RS_SCISSOR, gDrawState0CValues[states->s0C]);
    Gs_SetState(102, gDrawState66Values[states->s66]);
    Gs_SetState(98, gDrawState62Values[states->s62]);
    Gs_SetState(0, gDrawState00Values[states->s00]);
    if (gDrawState66Values[states->s66] == 1) {
        Gs_SetState(RS_COLOR2, -1);
    }
    switch (states->s5D) {
    case 6:
        Gs_SetState(93, 0x06000000);
        break;
    case 5:
        Gs_SetState(93, 0x05000000);
        break;
    default:
    case 0:
        Gs_SetState(93, 0);
        break;
    }
    *flags = 1;
    *flags |= (states->s66 & 1) << 1;
    *flags |= (states->f2 & 0xF) << 2;
    *flags |= (states->f6 & 0xF) << 6;
    *flags |= (states->f10 & 3) << 10;
    *flags |= (states->f12 & 1) << 12;
    *flags |= (states->f13 & 3) << 13;
    *flags |= (states->s0C & 1) << 15;
}

extern s32 gDrawBlendFactors[];
extern s32 gDrawBlendOps[];
extern s32 gDrawState63Values[2];
extern s32 gDrawState64Values[4];

/* Applies a material set's packed draw states (DrawStates_Apply) when bit 0
 * is set: bits 2-5 and 6-9 Gfx_SetBlend, 10-11 Gfx_SetBlendOp, 12
 * Gfx_SetTexFilter, 13-14 RS_TEX_CLAMP, 15 RS_SCISSOR. */
void Draw_ApplyStates(u32 packed)
{
    if (packed & 1) {
        Gfx_SetBlend(gDrawBlendFactors[(packed >> 2) & 0xF], gDrawBlendFactors[(packed >> 6) & 0xF]);
        Gfx_SetBlendOp(gDrawBlendOps[(packed >> 10) & 3]);
        Gfx_SetTexFilter(gDrawState63Values[(packed >> 12) & 1]);
        Gs_SetState(RS_TEX_CLAMP, gDrawState64Values[(packed >> 13) & 3]);
        Gs_SetState(RS_SCISSOR, gDrawState0CValues[(packed >> 15) & 1]);
    }
}

/* The default blending (the usual alpha blend, operation 0) with filtered
 * textures. */
void Draw_SetDefaultState(void)
{
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gfx_SetBlendOp(GFX_BLEND_OP_ADD);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
}

/*
 * Sets the model up with kind 0 (none), 1 (one bone set) or 2 (two sets)
 * of the skeleton file at base + MODEL_LOAD_SKEL_OFFSET
 * (MODEL_LOAD_ALT_SKEL_OFFSET with alt): its bones, and for each part its
 * skeleton copy and first bones. `off` holds the part count for kind 2 and `first` is reused as the
 * part counter, which the original's register use needs.
 */
void Model_InitSkeleton(u8 *base, Model *model, s32 kind, s32 alt)
{
    EmBone *bones;
    s32 boneIdx;
    s32 off;
    SkelHeader skel;
    EmBone *bone;
    s32 first;
    EmBone *bones2;
    s32 n;

    if (alt != 0) {
        base = (u8 *)base + MODEL_LOAD_ALT_SKEL_OFFSET;
    } else {
        base = (u8 *)base + MODEL_LOAD_SKEL_OFFSET;
    }
    model->boneKind = kind;
    switch (kind) {
    case 0:
        model->boneFirst = 0;
        model->boneCount = 0;
        model->bones = NULL;
        model->bones2 = NULL;
        model->partCount = 0;
        return;
    case 1:
        n = SkelFile_BoneCount(base);
        break;
    case 2:
        off = SkelFile_BoneCount(base);
        n = off * 2;
        break;
    }
    first = Bone_Alloc(n);
    if (first < 0) {
        return;
    }
    model->boneFirst = first;
    model->boneCount = n;
    model->bones = Bone_Get(first);
    if (kind == 2) {
        model->bones2 = Bone_Get(first + n / 2);
    } else {
        model->bones2 = NULL;
    }
    Bone_Reserve(first, n);
    model->partCount = SkelFile_PartCount(base);
    boneIdx = 0;
    bones = model->bones;
    bones2 = model->bones2;
    for (first = 0; first < model->partCount; first++) {
        SkelFile_BuildPart(&skel, (SkelHeader *)gLoadBuf, base, first);
        model->partSkels[first] = (s32)Skeleton_CopyData((u8 *)&skel);
        off = boneIdx * sizeof(EmBone);
        Bone_InitTreeRoot((EmBone *)((u8 *)bones + off), model->partSkels[first]);
        model->partBones[first] = (EmBone *)((u8 *)bones + off);
        if (kind == 2) {
            Bone_InitTreeRoot((EmBone *)((u8 *)bones2 + off), model->partSkels[first]);
            model->partBones2[first] = (EmBone *)((u8 *)bones2 + off);
        }
        boneIdx += SkelFile_PartBoneCount(base, first);
    }
}

extern char D_0046CEA0[];
extern char D_0046CEC0[];
extern char D_0046CEE0[];
extern char D_0046CF00[];
extern s32 gMatSetState15[];
extern s32 D_0042FB04[];
extern s32 D_0042FB14[];
extern s32 gMatSetState12[];
extern s32 D_0042FB34;
extern s32 gMatSetModelKinds[];

/*
 * Sets up the model's material sets (ModelMatSet), one per part of its
 * file: each set's render states (from the file's draw states, or
 * defaults), its materials (set as the RS_MATERIAL states) and its draw
 * model (Model_Create); then its skeleton (kind, alt: Model_InitSkeleton).
 * setupFlags: 0x100 picks gMatSetModelKinds[1] for the draw models; 0x200,
 * 0x800, 0x1000 and 0x2000 give render states 1, 18, 102 and 0 other
 * values.
 */
void Model_InitMatSets(Model *model, s32 setupFlags, s32 kind, s32 alt)
{
    MeshDesc desc;
    DrawStates states;
    u8 mark[8];
    s32 j;
    ModelMatSet *set;
    ModelMat *mats;
    u8 *base;
    s32 i;
    s32 sel;
    u16 flags;

    flags = setupFlags;
    sel = (flags & 0x100) / 256;
    base = model->file;
    model->matSetCount = ModelFile_PartCount(base);
    if (model->matSetCount == 0) {
        Debug_Message(D_0046CEA0, 0, 0, 0);
    }
    i = MatSet_FindFree(model->matSetCount);
    if (i < 0) {
        return;
    }
    model->matSetFirst = i;
    set = model->matSets = MatSet_Get(i);
    MatSet_Reserve(i, model->matSetCount);
    Gs_SetState(RS_SCISSOR, 0);
    mats = model->mats;
    for (i = 0; i < model->matSetCount; i++, set++) {
        if (ModelFile_GetDrawStates(&states, base, i) >= 0) {
            DrawStates_Apply(&states, &set->flags);
            if (flags & 0x800) {
                Gs_SetState(18, D_0042FB34);
            }
            if (flags & 0x1000) {
                Gs_SetState(102, 1);
            }
            if (flags & 0x2000) {
                Gs_SetState(0, 32);
            }
        } else {
            set->flags = 0;
            Gs_SetState(102, 0);
            Gs_SetState(RS_SCISSOR, 0);
            Gs_SetState(98, 0);
            Gs_SetState(0, 0);
            Gs_SetState(2, 0);
            Gs_SetState(21, gMatSetState15[0]);
            if (flags & 0x800) {
                Gs_SetState(18, D_0042FB34);
            } else {
                Gs_SetState(18, gMatSetState12[0]);
            }
            if (flags & 0x200) {
                Gs_SetState(1, D_0042FB14[0]);
            } else {
                Gs_SetState(1, D_0042FB04[0]);
            }
            if (flags & 0x2000) {
                Gs_SetState(0, 32);
            } else {
                Gs_SetState(0, 0);
            }
        }
        set->count = ModelFile_PartMatCount(base, i);
        if (set->count == 0) {
            Debug_Message(D_0046CEC0, (s16)i, model->matSetCount, 0);
        }
        if (ModelFile_GetPartMats(base, i, set->mat) == 0) {
            Debug_Message(D_0046CEE0, (s16)i, model->matSetCount, 0);
        }
        for (j = 0; j < set->count; j++) {
            Gs_SetState((u8)RS_MATERIAL(j), (s32)&mats[set->mat[j]]);
        }
        SysHeap_GetMark(mark);
        if (ModelFile_LoadMesh(&desc, base, i) != 0) {
            set->id = Model_Create(&desc, gMatSetModelKinds[sel]);
        } else {
            Debug_Message(D_0046CF00, (s16)i, model->matSetCount, 0);
            set->id = -1;
        }
        SysHeap_Release(mark);
    }
    Model_InitSkeleton(base, model, kind, alt);
    Gs_SetState(18, 0);
    Gs_SetState(102, 0);
    Gs_SetState(RS_SCISSOR, 0);
}

extern s16 gModelPartLists[][16][33];

/* Model_InitMatSets for a model with matrix slot `slot`: also its material
 * sets' matrix lists (gModelPartLists[slot], filled with -1, then read by
 * ModelFile_GetPartList), with one heap mark for all the draw models. */
void Model_InitMatSetsSlot(Model *model, s32 setupFlags, u8 slot, s32 kind)
{
    MeshDesc desc;
    DrawStates states;
    u8 mark[8];
    s32 j;
    ModelMatSet *set;
    ModelMat *mats;
    u8 *base;
    s32 i;
    s32 sel;
    u16 flags;

    flags = setupFlags;
    sel = (flags & 0x100) / 256;
    base = model->file;
    model->matSetCount = ModelFile_PartCount(base);
    if (model->matSetCount == 0) {
        Debug_Message(D_0046CEA0, 0, 0, 0);
    }
    i = MatSet_FindFree(model->matSetCount);
    if (i < 0) {
        return;
    }
    model->matSetFirst = i;
    set = model->matSets = MatSet_Get(i);
    MatSet_Reserve(i, model->matSetCount);
    mats = model->mats;
    Gs_SetState(RS_SCISSOR, 0);
    SysHeap_GetMark(mark);
    for (i = 0; i < model->matSetCount; i++, set++) {
        if (ModelFile_GetDrawStates(&states, base, i) >= 0) {
            DrawStates_Apply(&states, &set->flags);
        } else {
            set->flags = 0;
            Gs_SetState(98, 0);
            Gs_SetState(0, 0);
            Gs_SetState(21, gMatSetState15[0]);
            Gs_SetState(102, 0);
            if (flags & 0x800) {
                Gs_SetState(18, D_0042FB34);
            } else {
                Gs_SetState(18, gMatSetState12[0]);
            }
            if (flags & 0x200) {
                Gs_SetState(1, D_0042FB14[0]);
            } else {
                Gs_SetState(1, D_0042FB04[0]);
            }
            Gs_SetState(2, 0);
        }
        set->count = ModelFile_PartMatCount(base, i);
        if (set->count == 0) {
            Debug_Message(D_0046CEC0, (s16)i, model->matSetCount, 0);
        }
        if (ModelFile_GetPartMats(base, i, set->mat) == 0) {
            Debug_Message(D_0046CEE0, (s16)i, model->matSetCount, 0);
        }
        for (j = 0; j < set->count; j++) {
            Gs_SetState((u8)RS_MATERIAL(j), (s32)&mats[set->mat[j]]);
        }
        if (ModelFile_LoadMesh(&desc, base, i) != 0) {
            set->id = Model_Create(&desc, gMatSetModelKinds[sel]);
            if (set->id == 0) {
                Debug_Message(D_0046CF00, (s16)i, model->matSetCount, 0);
            }
        } else {
            Debug_Message(D_0046CF00, (s16)i, model->matSetCount, 0);
            set->id = -1;
        }
    }
    for (i = 0; i < model->matSetCount; i++) {
        for (j = 0; j < 64; j++) {
            gModelPartLists[slot][i][j] = -1;
        }
    }
    for (i = 0; i < model->matSetCount; i++) {
        ModelFile_GetPartList(gModelPartLists[slot][i], base, i);
    }
    SysHeap_Release(mark);
    Model_InitSkeleton(base, model, kind, 0);
}

#pragma rodata on
char D_0046CEA0[21] = {
#include "sub_main/0046CEA0.inc"
};
char D_0046CEC0[27] = {
#include "sub_main/0046CEC0.inc"
};
char D_0046CEE0[23] = {
#include "sub_main/0046CEE0.inc"
};
char D_0046CF00[20] = {
#include "sub_main/0046CF00.inc"
};
#pragma rodata off

/* the calls cast to s16: this unit's code takes the value as s16 */
struct MaterialSlot *Material_Get(s32 i);

/* Allocates the materials of model data f (ModelFile_MatCount of them) and reads them, their texture ids made handles; -1 when the pool is full. */
s32 Model_LoadMaterials(Model *model, s32 file)
{
    s32 i;
    s32 first;
    s32 n;
    ModelMat *mat;

    n = (s16)ModelFile_MatCount((u8 *)file);
    if (n == 0) {
        Debug_Message("MATERIAL NUM IS ZERO !!", 0, 0, 0);
    }
    first = Material_FindFree(n);
    if (first < 0) {
        return -1;
    }
    model->matFirst = first;
    model->matCount = n;
    model->mats = (ModelMat *)Material_Get(first);
    Material_Reserve(first, n);
    mat = model->mats;
    for (i = 0; i < model->matCount; i++, mat++) {
        ModelFile_ReadMat(file, i, mat);
        mat->tex = gTexHandles[model->texSlot + mat->tex];
    }
    return 0;
}

/* unit's own declaration: s16 slot (the definitions, take s32); with s32 Model_SetupWithPack differs by 15 words, Model_SetupWithPackEx by 32 */
s32 Tex_LoadPack(s32 id, s16 slot, s32 kind);

/* Sets up model idx from model file modelFile with the texture pack file
 * packId in texture slot `slot`: its materials, material sets (setupFlags,
 * Model_InitMatSets) and skeleton (skelKind). Returns the model, or NULL
 * when the material pool is full. */
Model *Model_SetupWithPack(s16 idx, s32 modelFile, s32 slot, s32 packId, s32 setupFlags, s32 skelKind)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = slot;
    model->texCount = Tex_LoadPack(packId, slot, 0);
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSets(model, setupFlags, skelKind, 0);
    return model;
}

/* unit's own declaration: as Tex_LoadPack above; with s32 Model_SetupWithPackRaw differs by 15 words */
s32 Tex_LoadPackRaw(s32 id, s16 slot, s32 kind);

/* As Model_SetupWithPack with Tex_LoadPackRaw, and Model_InitMatSets mode 1. */
Model *Model_SetupWithPackRaw(s16 idx, s32 modelFile, s32 slot, s32 packId, s32 setupFlags, s32 skelKind)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = slot;
    model->texCount = Tex_LoadPackRaw(packId, slot, 0);
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSets(model, setupFlags, skelKind, 1);
    return model;
}

/* unit's own declaration: s16 slot, as Tex_LoadPack above */
s32 Tex_LoadPackLz(s32 id, s16 slot, s32 kind, s32 d);

/* As Model_SetupWithPack with a texture pack packed in memory at src
 * (Tex_LoadPackLz; packId is not used). */
Model *Model_SetupWithPackEx(s16 idx, s32 modelFile, s32 slot, s32 packId, s32 setupFlags, s32 skelKind, s32 src)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = slot;
    model->texCount = Tex_LoadPackLz(packId, slot, 0, src);
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSets(model, setupFlags, skelKind, 0);
    return model;
}
void Tex_LoadPackFile(s32 *pack, s16 slot, s32 kind, u16 d); /* unit's own declaration: declared as this file's callers use it; with the definition's void (TexPackHead *p, s32 slot, s32 kind, s32 d), Model_SetupLoadedPack compiles differently */

/* As Model_SetupWithPack with a model's texture pack (Tex_LoadPackFile,
 * secondId its second texture), or no texture when pack is NULL. */
Model *Model_SetupLoadedPack(s16 idx, s32 modelFile, s32 slot, s32 *pack, s32 setupFlags, s32 secondId, s32 skelKind)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = (u16)slot;
    if (pack != NULL) {
        model->texCount = *pack;
        Tex_LoadPackFile(pack, slot, 0, secondId);
    } else {
        model->texCount = 0;
    }
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSets(model, setupFlags, skelKind, 0);
    return model;
}
void Tex_LoadPackLzHigh(s32 *pack, s16 slot, s32 kind, u16 d, s32 e); /* unit's own declaration: declared as this file's callers use it; with the definition's void (TexPackHead *p, s32 slot, s32 kind, s32 d, s32 src), Model_SetupLoadedPackEx compiles differently */

/* As Model_SetupLoadedPack with the pack packed in memory at src
 * (Tex_LoadPackLzHigh). */
Model *Model_SetupLoadedPackEx(s16 idx, s32 modelFile, s32 slot, s32 *pack, s32 setupFlags, s32 secondId, s32 skelKind, s32 src)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = (u16)slot;
    if (pack != NULL) {
        model->texCount = *pack;
        Tex_LoadPackLzHigh(pack, slot, 0, secondId, src);
    } else {
        model->texCount = 0;
    }
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSets(model, setupFlags, skelKind, 0);
    return model;
}

/* This file's initialised data. */
s32 gMatSetState15[12] = {
#include "sub_main/0042FB00.inc"
};
s32 gMatSetState12[4] = {
#include "sub_main/0042FB30.inc"
};
s32 gMatSetModelKinds[4] = {
#include "sub_main/0042FB40.inc"
};
s16 gAreaSubModelKinds[4] = {
#include "sub_main/0042FB50.inc"
};
s32 gDrawState15Values[2] = {
#include "sub_main/0042FB58.inc"
};
s32 gDrawState12Values[4] = {
#include "sub_main/0042FB60.inc"
};
s32 gDrawState01Values[12] = {
#include "sub_main/0042FB70.inc"
};
s32 gDrawState02Values[4] = {
#include "sub_main/0042FBA0.inc"
};
s32 gDrawState0CValues[2] = {
#include "sub_main/0042FBB0.inc"
};
s32 gDrawState66Values[2] = {
#include "sub_main/0042FBB8.inc"
};
s32 gDrawState62Values[4] = {
#include "sub_main/0042FBC0.inc"
};
s32 gDrawState00Values[4] = {
#include "sub_main/0042FBD0.inc"
};
s32 gDrawBlendFactors[10] = {
#include "sub_main/0042FBE0.inc"
};
s32 gDrawBlendOps[3] = {
#include "sub_main/0042FC08.inc"
};
s32 gDrawState63Values[2] = {
#include "sub_main/0042FC18.inc"
};
s32 gDrawState64Values[4] = {
#include "sub_main/0042FC20.inc"
};
