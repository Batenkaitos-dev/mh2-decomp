/*
 * Model files.
 *
 * The model file's readers: a model file is a tree of chunks (ModelChunk,
 * MODEL_CHUNK_* in model.h), found by type and index (ModelChunk_Get). The file
 * has parts, materials and textures; a mesh part has strip groups of strips,
 * vertex chunks (positions, normals, texture coordinates, colours, weights), its
 * materials, draw states and matrix list. ModelFile_LoadMesh builds a part into a
 * MeshDesc (Mesh_BuildRigid, Mesh_BuildSkinned), which Model_Create
 * copies and the packet builders (Model_BuildVifPackets) draw.
 */
#include "common.h"
#include "model.h"
#include "heap.h"

static ModelChunk *ModelFile_Header(u8 *file);
static s32 ModelChunk_Check(ModelChunk *parent, s32 type);
static ModelChunk *ModelChunk_Get(ModelChunk *parent, s32 type, s32 idx);

/* The list in MODEL_CHUNK_MTX_LIST of part partIdx of modelFile (a mesh part) into out: the
 * count, then the entries as s16. 0, or -1. */
s32 ModelFile_GetPartList(s16 *out, u8 *modelFile, s32 partIdx)
{
    ModelChunk *part;
    ModelChunk *listChunk;
    s32 *entry;
    s32 i;
    s32 entryCount;

    part = ModelFile_GetPart(modelFile, partIdx);
    if (part->type != MODEL_PART_MESH) {
        return -1;
    }
    listChunk = ModelChunk_Get(part, MODEL_CHUNK_MTX_LIST, 0);
    if (listChunk == NULL) {
        return -1;
    }
    entryCount = listChunk->count;
    entry = MODEL_CHUNK_DATA(listChunk);
    out[0] = entryCount;
    for (i = 0; i < entryCount; i++) {
        out[i + 1] = entry[0];
        entry = &entry[1];
    }
    return 0;
}

/* Copies the draw states (MODEL_CHUNK_DRAW_STATES) of part partIdx of modelFile into
 * states. 0; -1 when the part isn't a mesh or has none; -2 when their version isn't
 * DRAW_STATES_VERSION. */
s32 ModelFile_GetDrawStates(DrawStates *states, u8 *modelFile, s32 partIdx)
{
    ModelChunk *part;
    ModelChunk *statesChunk;
    DrawStates *src;

    part = ModelFile_GetPart(modelFile, partIdx);
    if (part->type != MODEL_PART_MESH) {
        return -1;
    }
    statesChunk = ModelChunk_Get(part, MODEL_CHUNK_DRAW_STATES, 0);
    if (statesChunk == NULL) {
        return -1;
    }
    src = MODEL_CHUNK_DATA(statesChunk);
    states->version = src->version;
    if ((states->version & DRAW_STATES_VERSION_MASK) != DRAW_STATES_VERSION) {
        return -2;
    }
    states->version = src->version;
    states->s15 = src->s15;
    states->s02 = src->s02;
    states->s00 = src->s00;
    states->s0C = src->s0C;
    states->s01 = src->s01;
    states->unk18 = src->unk18;
    states->s62 = src->s62;
    states->s12 = src->s12;
    states->s66 = src->s66;
    states->s5D = src->s5D;
    states->f2 = src->f2;
    states->f6 = src->f6;
    states->f10 = src->f10;
    states->unk38[0] = src->unk38[0];
    states->unk38[1] = src->unk38[1];
    states->f12 = src->f12;
    states->f13 = src->f13;
    return 0;
}

/* The number of parts in modelFile (MODEL_CHUNK_PARTS), 0 when it has none. */
s32 ModelFile_PartCount(u8 *modelFile)
{
    ModelChunk *root = ModelFile_Header(modelFile);
    ModelChunk *listChunk;

    if (ModelChunk_Check(root, MODEL_CHUNK_PARTS) != 0) {
        return 0;
    }
    listChunk = ModelChunk_Get(root, MODEL_CHUNK_PARTS, 0);
    return listChunk == NULL ? 0 : listChunk->count;
}

/* Part partIdx of modelFile, or NULL. The bound test lets partIdx equal the count. */
ModelChunk *ModelFile_GetPart(u8 *modelFile, u32 partIdx)
{
    ModelChunk *root = ModelFile_Header(modelFile);
    ModelChunk *chunk;

    if (ModelChunk_Check(root, MODEL_CHUNK_PARTS) != 0) {
        return NULL;
    }
    chunk = ModelChunk_Get(root, MODEL_CHUNK_PARTS, 0);
    if (chunk == NULL) {
        return NULL;
    }
    if (chunk->count < partIdx) {
        return NULL;
    }
    chunk = ModelChunk_Get(chunk, MODEL_CHUNK_ANY, partIdx);
    if (chunk == NULL) {
        return NULL;
    }
    return chunk;
}

/* The number of materials in modelFile (MODEL_CHUNK_MATERIALS), 0 when it has none. */
s32 ModelFile_MatCount(u8 *modelFile)
{
    ModelChunk *root = ModelFile_Header(modelFile);
    ModelChunk *listChunk;

    if (ModelChunk_Check(root, MODEL_CHUNK_MATERIALS) != 0) {
        return 0;
    }
    listChunk = ModelChunk_Get(root, MODEL_CHUNK_MATERIALS, 0);
    return listChunk == NULL ? 0 : listChunk->count;
}

/* Material matIdx's chunk in modelFile, or NULL (as ModelFile_GetPart). */
ModelChunk *ModelFile_GetMat(u8 *modelFile, u32 matIdx)
{
    ModelChunk *root = ModelFile_Header(modelFile);
    ModelChunk *chunk;

    if (ModelChunk_Check(root, MODEL_CHUNK_MATERIALS) != 0) {
        return NULL;
    }
    chunk = ModelChunk_Get(root, MODEL_CHUNK_MATERIALS, 0);
    if (chunk == NULL) {
        return NULL;
    }
    if (chunk->count < matIdx) {
        return NULL;
    }
    chunk = ModelChunk_Get(chunk, MODEL_CHUNK_ANY, matIdx);
    if (chunk == NULL) {
        return NULL;
    }
    return chunk;
}

/* Material matIdx's record in modelFile, or NULL. */
ModelFileMat *ModelFile_GetMatData(u8 *modelFile, u32 matIdx)
{
    ModelChunk *matChunk = ModelFile_GetMat(modelFile, matIdx);

    if (matChunk == NULL) {
        return NULL;
    }
    return MODEL_CHUNK_DATA(matChunk);
}

/* Fills outMat from material matIdx of modelFile: its colours, its size, and its
 * texture's id from the file's texture list (0 when it has none). Returns 1. */
s32 ModelFile_ReadMat(s32 modelFile, s32 matIdx, ModelMat *outMat)
{
    ModelFileMat *rec;

    rec = ModelFile_GetMatData((u8 *)modelFile, matIdx);
    Heap_Fill(outMat, 0, sizeof(ModelMat));
    if (rec->hasTexture != 0) {
        outMat->tex = *ModelFile_GetTexData((u8 *)modelFile, rec->texIdx);
    } else {
        outMat->tex = 0;
    }
    outMat->rgb[0] = rec->rgb[0];
    outMat->rgb[1] = rec->rgb[1];
    outMat->rgb[2] = rec->rgb[2];
    outMat->alpha = rec->alpha;
    outMat->unk24[0] = rec->unk00[0];
    outMat->unk24[1] = rec->unk00[1];
    outMat->unk24[2] = rec->unk00[2];
    outMat->unk24[3] = rec->unk00[3];
    outMat->unk14[0] = rec->unk20[0];
    outMat->unk14[1] = rec->unk20[1];
    outMat->unk14[2] = rec->unk20[2];
    outMat->unk14[3] = rec->unk20[3];
    outMat->unk34[0] = 0;
    outMat->unk34[1] = 0;
    outMat->unk34[2] = 0;
    outMat->unk34[3] = 0;
    outMat->size = rec->size;
    return 1;
}

/* Texture texIdx's chunk in modelFile, or NULL (as ModelFile_GetPart). */
ModelChunk *ModelFile_GetTex(u8 *modelFile, u32 texIdx)
{
    ModelChunk *root = ModelFile_Header(modelFile);
    ModelChunk *chunk;

    if (ModelChunk_Check(root, MODEL_CHUNK_TEXTURES) != 0) {
        return NULL;
    }
    chunk = ModelChunk_Get(root, MODEL_CHUNK_TEXTURES, 0);
    if (chunk == NULL) {
        return NULL;
    }
    if (chunk->count < texIdx) {
        return NULL;
    }
    chunk = ModelChunk_Get(chunk, MODEL_CHUNK_ANY, texIdx);
    if (chunk == NULL) {
        return NULL;
    }
    return chunk;
}

/* Texture texIdx's data in modelFile (its first word is the texture's id), or NULL. */
s32 *ModelFile_GetTexData(u8 *modelFile, u32 texIdx)
{
    ModelChunk *texChunk = ModelFile_GetTex(modelFile, texIdx);

    return texChunk == NULL ? NULL : MODEL_CHUNK_DATA(texChunk);
}

/* The number of materials part partIdx of modelFile uses (MODEL_CHUNK_PART_MATS), or 0. */
s32 ModelFile_PartMatCount(u8 *modelFile, u32 partIdx)
{
    ModelChunk *chunk = ModelFile_GetPart(modelFile, partIdx);

    if (chunk == NULL) {
        return 0;
    }
    chunk = ModelChunk_Get(chunk, MODEL_CHUNK_PART_MATS, 0);
    return chunk == NULL ? 0 : chunk->count;
}

/* The words of MODEL_CHUNK_PART_MATS of part partIdx of modelFile into outMatIds; 1, or 0. */
s32 ModelFile_GetPartMats(u8 *modelFile, u32 partIdx, s32 *outMatIds)
{
    ModelChunk *part;
    ModelChunk *matsChunk;
    u32 i;
    u32 matCount;
    s32 *matIds;

    part = ModelFile_GetPart(modelFile, partIdx);
    if (part == NULL) {
        return 0;
    }
    matsChunk = ModelChunk_Get(part, MODEL_CHUNK_PART_MATS, 0);
    if (matsChunk == NULL) {
        return 0;
    }
    matCount = matsChunk->count;
    matIds = MODEL_CHUNK_DATA(matsChunk);
    for (i = 0; i < matCount; i++) {
        outMatIds[i] = *matIds++;
    }
    return 1;
}

/* The file's root chunk: the file itself. */
static ModelChunk *ModelFile_Header(u8 *file)
{
    return (ModelChunk *)file;
}

/* 0 when parent has a child chunk of the type, else -1. */
static s32 ModelChunk_Check(ModelChunk *parent, s32 type)
{
    u32 childIdx = 0;
    u32 childCount = parent->count;

    for (parent = MODEL_CHUNK_FIRST(parent); childIdx < childCount; childIdx++) {
        if (parent->type == type) {
            return 0;
        }
        parent = MODEL_CHUNK_NEXT(parent);
    }
    return -1;
}

/* Parent's idx-th child chunk of the type, or its idx-th child of any type for
 * MODEL_CHUNK_ANY; NULL when there is none. */
static ModelChunk *ModelChunk_Get(ModelChunk *parent, s32 type, s32 idx)
{
    s32 childIdx;
    s32 childCount;

    if (type != MODEL_CHUNK_ANY) {
        childCount = parent->count;
        childIdx = 0;
        for (parent = MODEL_CHUNK_FIRST(parent); childIdx < childCount; childIdx++) {
            if (parent->type == type) {
                if (idx == 0) {
                    return parent;
                }
                idx--;
            }
            parent = MODEL_CHUNK_NEXT(parent);
        }
    } else if ((u32)idx < parent->count) {
        parent = MODEL_CHUNK_FIRST(parent);
        for (childIdx = 0; childIdx < idx; childIdx++) {
            parent = MODEL_CHUNK_NEXT(parent);
        }
        return parent;
    }
    return NULL;
}

/* The number of strip groups in part (MODEL_CHUNK_STRIPS), or -1. */
static s32 ModelPart_StripGroupCount(ModelChunk *part)
{
    ModelChunk *chunk = ModelChunk_Get(part, MODEL_CHUNK_STRIPS, 0);

    return chunk == NULL ? -1 : chunk->count;
}

/* Strip group groupIdx's kind (MODEL_CHUNK_STRIPS_RIGID or _SKINNED), or -1. */
static s32 ModelPart_StripGroupKind(ModelChunk *part, s32 groupIdx)
{
    u8 *group = (u8 *)ModelChunk_Get(part, MODEL_CHUNK_STRIPS, 0);
    s32 skip;

    if (group == NULL) {
        return -1;
    }
    if (((ModelChunk *)group)->count <= (u32)groupIdx) {
        return -1;
    }
    group += sizeof(ModelChunk);
    for (skip = 0; skip < groupIdx; skip++) {
        group += ((ModelChunk *)group)->size;
    }
    return ((ModelChunk *)group)->type;
}

/* The number of strips in strip group groupIdx of part, or -1. */
static s32 ModelPart_GroupStripCount(ModelChunk *part, s32 groupIdx)
{
    u8 *group = (u8 *)ModelChunk_Get(part, MODEL_CHUNK_STRIPS, 0);
    s32 skip;

    if (group == NULL) {
        return -1;
    }
    if (((ModelChunk *)group)->count <= (u32)groupIdx) {
        return -1;
    }
    group += sizeof(ModelChunk);
    for (skip = 0; skip < groupIdx; skip++) {
        group += ((ModelChunk *)group)->size;
    }
    return ((ModelChunk *)group)->count;
}

/* The number of strips in all of part's strip groups, or -1. */
s32 ModelPart_StripCount(ModelChunk *part)
{
    u32 groupCount;
    u32 groupIdx;
    s32 total;

    total = 0;
    if ((s32)(groupCount = ModelPart_StripGroupCount(part)) == -1) {
        return -1;
    }
    for (groupIdx = 0; groupIdx < groupCount; groupIdx++) {
        total += ModelPart_GroupStripCount(part, groupIdx);
    }
    return total;
}

/* The number of vertices in part (MODEL_CHUNK_POSITIONS), or -1. */
s32 ModelPart_VertCount(ModelChunk *part)
{
    ModelChunk *chunk = ModelChunk_Get(part, MODEL_CHUNK_POSITIONS, 0);

    return chunk == NULL ? -1 : chunk->count;
}

/*
 * The part readers from ModelPart_StripMat to ModelPart_StripMatsVary keep their
 * arguments in registers across the calls of ModelChunk_Get, ModelChunk_Check,
 * ModelFile_Header and ModelPart_StripGroupCount: those are static, so the
 * compiler knows which registers they use. The strip readers (and
 * ModelPart_StripGroupKind and ModelPart_GroupStripCount above) step through the
 * groups and strips with byte pointers, `group += size`: the game's code adds in
 * place, which a ModelChunk pointer (MODEL_CHUNK_NEXT) doesn't give there.
 */
static s32 ModelPart_StripMat(ModelChunk *part, s32 groupIdx, s32 stripIdx);
/* main's allocator hook gSysAllocHook (SysHeap_Alloc, set in display.c): not a symbol main exports. */
#define gSysAllocHook (*(void *(**)(s32 size))0x2491E0)

/* The material of strip stripIdx of strip group groupIdx of part: entry (the strips of
 * the groups before it, plus stripIdx) of MODEL_CHUNK_STRIP_MATS, or -1. */
static s32 ModelPart_StripMat(ModelChunk *part, s32 groupIdx, s32 stripIdx)
{
    s32 flatIdx = 0;
    ModelChunk *chunk;
    u8 *group;
    s32 skip;
    s32 matId;

    if (groupIdx > ModelPart_StripGroupCount(part)) {
        return -1;
    }
    chunk = ModelChunk_Get(part, MODEL_CHUNK_STRIPS, 0);
    if (chunk == NULL) {
        return -1;
    }
    group = (u8 *)MODEL_CHUNK_FIRST(chunk);
    for (skip = 0; skip < groupIdx; skip++) {
        flatIdx += ((ModelChunk *)group)->count;
        group += ((ModelChunk *)group)->size;
    }
    flatIdx += stripIdx;
    chunk = ModelChunk_Get(part, MODEL_CHUNK_STRIP_MATS, 0);
    if (chunk == NULL) {
        matId = -1;
    } else {
        matId = ((s32 *)MODEL_CHUNK_DATA(chunk))[flatIdx];
    }
    return matId;
}

/* The length (ModelStrip.length) of strip stripIdx of strip group groupIdx of part, or -1. */
static s32 ModelPart_StripLength(ModelChunk *part, s32 groupIdx, s32 stripIdx)
{
    u8 *strip;
    u8 *group;
    s32 skip;
    u32 stripLen;

    if (groupIdx > ModelPart_StripGroupCount(part)) {
        return -1;
    }
    strip = (u8 *)ModelChunk_Get(part, MODEL_CHUNK_STRIPS, 0);
    if (strip == NULL) {
        return -1;
    }
    group = strip + sizeof(ModelChunk);
    for (skip = 0; skip < groupIdx; skip++) {
        group += ((ModelChunk *)group)->size;
    }
    strip = group + sizeof(ModelChunk);
    for (skip = 0; skip <= stripIdx; skip++) {
        stripLen = ((ModelStrip *)strip)->length;
        strip += stripLen * sizeof(u32) + sizeof(ModelStrip);
    }
    return stripLen;
}

/* The flag (ModelStrip.flag) of strip stripIdx of strip group groupIdx of part, or -1. */
static s32 ModelPart_StripFlag(ModelChunk *part, s32 groupIdx, s32 stripIdx)
{
    u8 *strip;
    u8 *group;
    s32 skip;
    u32 flagBit;

    if (groupIdx > ModelPart_StripGroupCount(part)) {
        return -1;
    }
    strip = (u8 *)ModelChunk_Get(part, MODEL_CHUNK_STRIPS, 0);
    if (strip == NULL) {
        return -1;
    }
    group = strip + sizeof(ModelChunk);
    for (skip = 0; skip < groupIdx; skip++) {
        group += ((ModelChunk *)group)->size;
    }
    strip = group + sizeof(ModelChunk);
    for (skip = 0; skip <= stripIdx; skip++) {
        flagBit = *(u32 *)strip & MODEL_STRIP_FLAG;
        strip += ((ModelStrip *)strip)->length * sizeof(u32) + sizeof(ModelStrip);
    }
    return flagBit != 0;
}

/* Vertex index wordIdx of strip stripIdx of strip group groupIdx of part, or -1. */
static s32 ModelPart_StripIndex(ModelChunk *part, s32 groupIdx, s32 stripIdx, s32 wordIdx)
{
    u8 *strip;
    u8 *group;
    s32 skip;

    if (groupIdx > ModelPart_StripGroupCount(part)) {
        return -1;
    }
    strip = (u8 *)ModelChunk_Get(part, MODEL_CHUNK_STRIPS, 0);
    if (strip == NULL) {
        return -1;
    }
    group = strip + sizeof(ModelChunk);
    for (skip = 0; skip < groupIdx; skip++) {
        group += ((ModelChunk *)group)->size;
    }
    strip = group + sizeof(ModelChunk);
    for (skip = 0; skip < stripIdx; skip++) {
        strip += ((ModelStrip *)strip)->length * sizeof(u32) + sizeof(ModelStrip);
    }
    if (wordIdx >= (s32)((ModelStrip *)strip)->length) {
        return -1;
    }
    return ((s32 *)(strip + sizeof(ModelStrip)))[wordIdx];
}

/* Vertex vertIdx's position in part's MODEL_CHUNK_POSITIONS, or NULL. */
static ModelVec3 *ModelPart_VertPos(ModelChunk *part, s32 vertIdx)
{
    ModelChunk *chunk = ModelChunk_Get(part, MODEL_CHUNK_POSITIONS, 0);

    if (chunk == NULL) {
        return NULL;
    }
    return (ModelVec3 *)MODEL_CHUNK_DATA(chunk) + vertIdx;
}

/* Vertex vertIdx's normal in part's MODEL_CHUNK_NORMALS, or NULL. */
static ModelVec3 *ModelPart_VertNormal(ModelChunk *part, s32 vertIdx)
{
    ModelChunk *chunk = ModelChunk_Get(part, MODEL_CHUNK_NORMALS, 0);

    if (chunk == NULL) {
        return NULL;
    }
    return (ModelVec3 *)MODEL_CHUNK_DATA(chunk) + vertIdx;
}

/* Vertex vertIdx's texture coordinates in part's MODEL_CHUNK_UVS, or NULL. */
static ModelVec2 *ModelPart_VertUv(ModelChunk *part, s32 vertIdx)
{
    ModelChunk *chunk = ModelChunk_Get(part, MODEL_CHUNK_UVS, 0);

    if (chunk == NULL) {
        return NULL;
    }
    return (ModelVec2 *)MODEL_CHUNK_DATA(chunk) + vertIdx;
}

/* Vertex vertIdx's colour in part's MODEL_CHUNK_COLORS, or NULL. */
static ModelVec4 *ModelPart_VertColor(ModelChunk *part, s32 vertIdx)
{
    ModelChunk *chunk = ModelChunk_Get(part, MODEL_CHUNK_COLORS, 0);

    if (chunk == NULL) {
        return NULL;
    }
    return (ModelVec4 *)MODEL_CHUNK_DATA(chunk) + vertIdx;
}

/* The number of vertex vertIdx's weights in part's MODEL_CHUNK_WEIGHTS, or -1. Each
 * vertex's list is an s32 count, then that many ModelWeights. */
static s32 ModelPart_VertWeightCount(ModelChunk *part, s32 vertIdx)
{
    ModelChunk *chunk;
    u8 *list;
    s32 skip;
    s32 listLen;

    chunk = ModelChunk_Get(part, MODEL_CHUNK_WEIGHTS, 0);
    if (chunk == NULL) {
        return -1;
    }
    list = MODEL_CHUNK_DATA(chunk);
    for (skip = 0; skip < vertIdx; skip++) {
        listLen = *(s32 *)list;
        list += sizeof(s32);
        list += listLen * sizeof(ModelWeight);
    }
    return *(s32 *)list;
}

/* Weight weightIdx of vertex vertIdx in part's MODEL_CHUNK_WEIGHTS, or NULL. */
static ModelWeight *ModelPart_VertWeight(ModelChunk *part, s32 vertIdx, s32 weightIdx)
{
    u8 *cursor;
    s32 skip;
    s32 listLen;

    cursor = (u8 *)ModelChunk_Get(part, MODEL_CHUNK_WEIGHTS, 0);
    if (cursor == NULL) {
        return NULL;
    }
    cursor += sizeof(ModelChunk);
    for (skip = 0; skip < vertIdx; skip++) {
        listLen = *(s32 *)cursor;
        cursor += sizeof(s32);
        cursor += listLen * sizeof(ModelWeight);
    }
    if (weightIdx >= *(s32 *)cursor) {
        cursor = NULL;
    } else {
        cursor += sizeof(s32);
        cursor += weightIdx * sizeof(ModelWeight);
    }
    return (ModelWeight *)cursor;
}

/* Sets up mesh (mesh->matCount from ModelFile_PartMatCount) from part partIdx of modelFile when the part
 * is of kind MODEL_PART_MESH (Mesh_Build); 0 for the other kinds or a bad index. */
s32 ModelFile_LoadMesh(MeshDesc *mesh, u8 *modelFile, u32 partIdx)
{
    ModelChunk *partsChunk;
    ModelChunk *part;

    ModelFile_Header(modelFile);
    partsChunk = ModelChunk_Get((ModelChunk *)modelFile, MODEL_CHUNK_PARTS, 0);
    if (partsChunk == NULL) {
        return 0;
    }
    if (partsChunk->count <= partIdx) {
        return 0;
    }
    if ((part = ModelFile_GetPart(modelFile, partIdx)) == NULL) {
        return 0;
    }
    switch (part->type) {
    case 3:
    case 6:
    case 7:
    case 8:
        return 0;
    case MODEL_PART_MESH:
        mesh->matCount = ModelFile_PartMatCount(modelFile, partIdx);
        return Mesh_Build(mesh, part);
    }
    return 0;
}

/* Builds mesh from part: Mesh_BuildSkinned when the part has MODEL_CHUNK_WEIGHTS, else Mesh_BuildRigid. */
s32 Mesh_Build(MeshDesc *mesh, ModelChunk *part)
{
    if (ModelChunk_Check(part, MODEL_CHUNK_WEIGHTS) != 0) {
        return Mesh_BuildRigid(mesh, part);
    }
    return Mesh_BuildSkinned(mesh, part);
}

/* 1 when some strip of part has another material than the first strip's
 * (ModelPart_StripMat), else 0. */
s32 ModelPart_StripMatsVary(ModelChunk *part)
{
    s32 groupIdx;
    s32 stripIdx;
    s32 groupCount;
    s32 groupStrips;
    s32 firstMat;

    firstMat = ModelPart_StripMat(part, 0, 0);
    groupCount = ModelPart_StripGroupCount(part);
    for (groupIdx = 0; groupIdx < groupCount; groupIdx++) {
        groupStrips = ModelPart_GroupStripCount(part, groupIdx);
        for (stripIdx = 0; stripIdx < groupStrips; stripIdx++) {
            if (ModelPart_StripMat(part, groupIdx, stripIdx) != firstMat) {
                return 1;
            }
        }
    }
    return 0;
}

/* Builds mesh from a part without weights: the strips (each its length with
 * MESH_STRIP_FLAGGED or MESH_STRIP_UNFLAGGED, its material when the strips' materials
 * differ, then its vertex indexes) and the vertices (MeshVtxTex when the part has
 * MODEL_CHUNK_UVS, else MeshVtxCol), both allocated through gSysAllocHook.
 * 1, or 0 when an allocation fails. The part readers it calls are static: it keeps
 * t registers across them. */
s32 Mesh_BuildRigid(MeshDesc *mesh, ModelChunk *part)
{
    s32 vertIdx;
    s32 size;
    s32 groupCount;
    s32 groupIdx;
    s32 stripIdx;
    u16 *stripOut;
    s32 groupStrips;
    s32 stripLen;
    s32 indexTotal;
    s32 hasUv;
    s32 wordIdx;
    u8 *vertOut;
    f32 *rec;

    hasUv = 0;
    if (ModelChunk_Check(part, MODEL_CHUNK_UVS) == 0) {
        hasUv = 1;
    }
    mesh->flags = MESH_BUILT | MESH_INDEXED;
    groupCount = ModelPart_StripGroupCount(part);
    mesh->count = ModelPart_StripCount(part);
    if (ModelPart_StripMatsVary(part) == 1) {
        mesh->flags |= MESH_STRIP_MAT;
    }
    indexTotal = 0;
    for (groupIdx = 0; groupIdx < groupCount; groupIdx++) {
        groupStrips = ModelPart_GroupStripCount(part, groupIdx);
        for (stripIdx = 0; stripIdx < groupStrips; stripIdx++) {
            indexTotal += ModelPart_StripLength(part, groupIdx, stripIdx);
        }
    }
    size = (mesh->count + indexTotal) * 2;
    if (mesh->flags & MESH_STRIP_MAT) {
        size += mesh->count * 2;
    }
    if ((mesh->strips = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    mesh->stripsSize = size;
    stripOut = mesh->strips;
    for (groupIdx = 0; groupIdx < groupCount; groupIdx++) {
        groupStrips = ModelPart_GroupStripCount(part, groupIdx);
        for (stripIdx = 0; stripIdx < groupStrips; stripIdx++) {
            stripLen = ModelPart_StripLength(part, groupIdx, stripIdx);
            *stripOut = stripLen;
            if (ModelPart_StripFlag(part, groupIdx, stripIdx) != 0) {
                *stripOut |= MESH_STRIP_FLAGGED;
            } else {
                *stripOut |= MESH_STRIP_UNFLAGGED;
            }
            stripOut++;
            if (mesh->flags & MESH_STRIP_MAT) {
                *stripOut++ = ModelPart_StripMat(part, groupIdx, stripIdx);
            }
            if (mesh->flags & MESH_INDEXED) {
                for (wordIdx = 0; wordIdx < stripLen; wordIdx++) {
                    *stripOut++ = ModelPart_StripIndex(part, groupIdx, stripIdx, wordIdx);
                }
            }
        }
    }
    mesh->format = MESH_FORMAT_COLOR;
    if (hasUv) {
        mesh->format |= MESH_FORMAT_UV;
    }
    mesh->vertCount = ModelPart_VertCount(part);
    size = mesh->vertCount * sizeof(MeshVtxTex);
    if ((mesh->verts = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    mesh->vertsSize = size;
    vertOut = mesh->verts;
    for (vertIdx = 0; vertIdx < mesh->vertCount; vertIdx++) {
        if (hasUv) {
            rec = ModelPart_VertPos(part, vertIdx)->v;
            ((MeshVtxTex *)vertOut)->pos[0] = rec[0];
            ((MeshVtxTex *)vertOut)->pos[1] = rec[1];
            ((MeshVtxTex *)vertOut)->pos[2] = rec[2];
            rec = ModelPart_VertNormal(part, vertIdx)->v;
            ((MeshVtxTex *)vertOut)->nrm[0] = rec[0];
            ((MeshVtxTex *)vertOut)->nrm[1] = rec[1];
            ((MeshVtxTex *)vertOut)->nrm[2] = rec[2];
            rec = ModelPart_VertUv(part, vertIdx)->v;
            ((MeshVtxTex *)vertOut)->uv[0] = rec[0];
            ((MeshVtxTex *)vertOut)->uv[1] = rec[1];
            rec = ModelPart_VertColor(part, vertIdx)->v;
            ((MeshVtxTex *)vertOut)->col = (u32)rec[3] << 24 | (u32)rec[0] << 16 | (u32)rec[1] << 8 | (u32)rec[2];
            vertOut += sizeof(MeshVtxTex);
        } else {
            rec = ModelPart_VertPos(part, vertIdx)->v;
            ((MeshVtxCol *)vertOut)->pos[0] = rec[0];
            ((MeshVtxCol *)vertOut)->pos[1] = rec[1];
            ((MeshVtxCol *)vertOut)->pos[2] = rec[2];
            rec = ModelPart_VertNormal(part, vertIdx)->v;
            ((MeshVtxCol *)vertOut)->nrm[0] = rec[0];
            ((MeshVtxCol *)vertOut)->nrm[1] = rec[1];
            ((MeshVtxCol *)vertOut)->nrm[2] = rec[2];
            rec = ModelPart_VertColor(part, vertIdx)->v;
            ((MeshVtxCol *)vertOut)->col = (u32)rec[3] << 24 | (u32)rec[0] << 16 | (u32)rec[1] << 8 | (u32)rec[2];
            vertOut += sizeof(MeshVtxCol);
        }
    }
    return 1;
}

/* Mesh_BuildRigid for a part with weights: each strip also gets its skin kind
 * (MESH_STRIP_SKIN, from its group's kind), and the vertices are MeshVtxSkin. A
 * vertex's weights become bytes; the remainder of 255 goes to the last nonzero
 * one, and empty slots are moved down. One byte cursor writes the strips and then
 * the vertices, and innerIdx counts both the strips and the weights: the game's
 * register use needs those shared locals. */
s32 Mesh_BuildSkinned(MeshDesc *mesh, ModelChunk *part)
{
    s32 outerIdx;
    s32 innerIdx;
    s32 wordIdx;
    s32 groupStrips;
    s32 weightCount;
    u8 *cursor;
    s32 kind;
    s32 indexTotal;
    s32 size;
    s32 lastWeighted;
    s32 stripLen;
    s32 weights[4];
    f32 *rec;
    ModelWeight *weightRec;
    s32 boneIds[4];
    s32 groupCount;

    ModelChunk_Check(part, MODEL_CHUNK_UVS);
    mesh->flags = MESH_BUILT | MESH_STRIP_SKIN | MESH_INDEXED;
    groupCount = ModelPart_StripGroupCount(part);
    mesh->count = ModelPart_StripCount(part);
    if (ModelPart_StripMatsVary(part) == 1) {
        mesh->flags |= MESH_STRIP_MAT;
    }
    indexTotal = 0;
    for (outerIdx = 0; outerIdx < groupCount; outerIdx++) {
        groupStrips = ModelPart_GroupStripCount(part, outerIdx);
        for (innerIdx = 0; innerIdx < groupStrips; innerIdx++) {
            indexTotal += ModelPart_StripLength(part, outerIdx, innerIdx);
        }
    }
    size = (mesh->count + indexTotal) * 2;
    if (mesh->flags & MESH_STRIP_MAT) {
        size += mesh->count * 2;
    }
    if (mesh->flags & MESH_STRIP_SKIN) {
        size += mesh->count * 2;
    }
    if ((mesh->strips = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    mesh->stripsSize = size;
    cursor = (u8 *)mesh->strips;
    for (outerIdx = 0; outerIdx < groupCount; outerIdx++) {
        groupStrips = ModelPart_GroupStripCount(part, outerIdx);
        kind = ModelPart_StripGroupKind(part, outerIdx);
        for (innerIdx = 0; innerIdx < groupStrips; innerIdx++) {
            stripLen = ModelPart_StripLength(part, outerIdx, innerIdx);
            *(u16 *)cursor = stripLen;
            if (ModelPart_StripFlag(part, outerIdx, innerIdx) != 0) {
                *(u16 *)cursor |= MESH_STRIP_FLAGGED;
            } else {
                *(u16 *)cursor |= MESH_STRIP_UNFLAGGED;
            }
            cursor += 2;
            if (mesh->flags & MESH_STRIP_MAT) {
                *(u16 *)cursor = ModelPart_StripMat(part, outerIdx, innerIdx);
                cursor += 2;
            }
            if (mesh->flags & MESH_STRIP_SKIN) {
                switch (kind) {
                case MODEL_CHUNK_STRIPS_RIGID:
                    *(u16 *)cursor = MESH_SKIN_ONE_BONE;
                    break;
                case MODEL_CHUNK_STRIPS_SKINNED:
                    *(u16 *)cursor = MESH_SKIN_FOUR_BONES;
                    break;
                }
                cursor += 2;
            }
            if (mesh->flags & MESH_INDEXED) {
                for (wordIdx = 0; wordIdx < stripLen; wordIdx++) {
                    *(u16 *)cursor = ModelPart_StripIndex(part, outerIdx, innerIdx, wordIdx);
                    cursor += 2;
                }
            }
        }
    }
    mesh->format = MESH_FORMAT_COLOR | MESH_FORMAT_UV;
    mesh->format |= MESH_FORMAT_SKIN;
    mesh->vertCount = ModelPart_VertCount(part);
    size = mesh->vertCount * sizeof(MeshVtxSkin);
    if ((mesh->verts = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    mesh->vertsSize = size;
    cursor = mesh->verts;
    for (outerIdx = 0; outerIdx < mesh->vertCount; outerIdx++) {
        rec = ModelPart_VertPos(part, outerIdx)->v;
        ((MeshVtxSkin *)cursor)->pos[0] = rec[0];
        ((MeshVtxSkin *)cursor)->pos[1] = rec[1];
        ((MeshVtxSkin *)cursor)->pos[2] = rec[2];
        rec = ModelPart_VertNormal(part, outerIdx)->v;
        ((MeshVtxSkin *)cursor)->nrm[0] = rec[0];
        ((MeshVtxSkin *)cursor)->nrm[1] = rec[1];
        ((MeshVtxSkin *)cursor)->nrm[2] = rec[2];
        rec = ModelPart_VertUv(part, outerIdx)->v;
        ((MeshVtxSkin *)cursor)->uv[0] = rec[0];
        ((MeshVtxSkin *)cursor)->uv[1] = rec[1];
        rec = ModelPart_VertColor(part, outerIdx)->v;
        ((MeshVtxSkin *)cursor)->col = (u32)rec[3] << 24 | (u32)rec[0] << 16 | (u32)rec[1] << 8 | (u32)rec[2];
        ((MeshVtxSkin *)cursor)->bones = 0;
        ((MeshVtxSkin *)cursor)->weights = 0;
        weightCount = ModelPart_VertWeightCount(part, outerIdx);
        weights[0] = 0;
        weights[1] = 0;
        weights[2] = 0;
        weights[3] = 0;
        lastWeighted = 0;
        boneIds[0] = 0;
        boneIds[1] = 0;
        boneIds[2] = 0;
        boneIds[3] = 0;
        for (innerIdx = 0; innerIdx < weightCount; innerIdx++) {
            weightRec = ModelPart_VertWeight(part, outerIdx, innerIdx);
            weights[innerIdx] = (s32)(255.0f * weightRec->percent / 100.0f) & 0xFF;
            if (0 < weights[innerIdx]) {
                lastWeighted = innerIdx;
            }
            boneIds[innerIdx] = weightRec->bone;
        }
        weights[lastWeighted] += 255 - weights[0] - weights[1] - weights[2] - weights[3];
        for (innerIdx = 0; innerIdx < weightCount - 1; innerIdx++) {
            if (weights[innerIdx] == 0) {
                weights[innerIdx] = weights[innerIdx + 1];
                boneIds[innerIdx] = boneIds[innerIdx + 1];
                weights[innerIdx + 1] = 0;
            }
        }
        ((MeshVtxSkin *)cursor)->weights = weights[0] << 24 | weights[1] << 16 | weights[2] << 8 | weights[3];
        ((MeshVtxSkin *)cursor)->bones = boneIds[0] << 24 | boneIds[1] << 16 | boneIds[2] << 8 | boneIds[3];
        cursor += sizeof(MeshVtxSkin);
    }
    return 1;
}
