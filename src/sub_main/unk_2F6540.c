/*
 * Model files and small helpers (sub_main 0x2F6540-0x2F9D80).
 *
 * First the skeleton builder SkelFile_BuildPart (skeleton files: unk_2F61D0.c).
 * Then the model file's readers: a model file is a tree of chunks (ModelChunk,
 * MODEL_CHUNK_* in model.h), found by type and index (ModelChunk_Get). The file
 * has parts, materials and textures; a mesh part has strip groups of strips,
 * vertex chunks (positions, normals, texture coordinates, colours, weights), its
 * materials, draw states and matrix list. ModelFile_LoadMesh builds a part into a
 * MeshDesc (Mesh_BuildRigid, Mesh_BuildSkinned), which Model_Create
 * (unk_335040.c) copies and unk_2E3940.c's packet builders draw.
 *
 * Then a BMP header check, the animation curves' start and end times, 3-vector
 * helpers, a camera frustum's planes, a free list and a tagged list, and the USB
 * keyboard's set-up and poll. ACCESS's kana-kanji engine starts after it, at
 * 0x2F9D80.
 */
#include "common.h"
#include "em.h"
#include "model.h"
#include "motion.h"
#include "image.h"
#include "kbd.h"

static ModelChunk *ModelFile_Header(u8 *file);
static s32 ModelChunk_Check(ModelChunk *parent, s32 type);
static ModelChunk *ModelChunk_Get(ModelChunk *parent, s32 type, s32 idx);
ModelChunk *ModelFile_GetPart(u8 *modelFile, u32 partIdx);
ModelChunk *ModelFile_GetMat(u8 *modelFile, u32 matIdx);
ModelChunk *ModelFile_GetTex(u8 *modelFile, u32 texIdx);

/* prototypes */
f32 AnimCurve_EndTime(AnimCurve *c);
f32 Vec_Length2(f32 *);
s32 Debug_Sprintf(const char *fmt, ...);
extern s32 gAnimKeyBase;
extern s16 gUsbKbReady;
/* end of prototypes */

s32 SkelFile_PartCount(void *f);
s32 SkelFile_PartRoot(void *f, u32 i);
s32 SkelFile_PartBoneCount(void *f, s32 i);
s32 *SkelFile_GetNode(void *f, s32 node);
s32 SkelFile_PartSize(void *f, s32 i);

/* Builds a part of skeleton file skelFile in built (the bones follow the header), copies the
 * header to dst and links it; 0 for a bad part. */
s32 SkelFile_BuildPart(SkelHeader *dst, SkelHeader *built, void *skelFile, s32 part)
{
    s32 boneCount;
    s32 boneIdx;
    s32 comp;
    s32 rootNode;
    BoneDef *def;
    BoneDef *clearDef;
    SkelFileNode *srcNode;

    if (part >= SkelFile_PartCount(skelFile)) {
        return 0;
    }
    built->size = SkelFile_PartSize(skelFile, part);
    built->count = boneCount = SkelFile_PartBoneCount(skelFile, part);
    built->headerSize = sizeof(SkelHeader);
    def = (BoneDef *)(built + 1);
    built->block = NULL;
    for (boneIdx = 0; boneIdx < boneCount; boneIdx++) {
        clearDef = &def[boneIdx];
        for (comp = 0; comp < 4; comp++) {
            clearDef->scale[comp] = 0.0f;
            clearDef->rot[comp] = 0.0f;
            clearDef->trans[comp] = 0.0f;
        }
    }
    rootNode = SkelFile_PartRoot(skelFile, part);
    for (boneIdx = 0; boneIdx < boneCount; boneIdx++, def++) {
        srcNode = (SkelFileNode *)SkelFile_GetNode(skelFile, boneIdx + rootNode);
        def->index = srcNode->index;
        def->id = srcNode->id;
        def->child = srcNode->child;
        def->sibling = srcNode->sibling;
        if (def->child != -1) {
            def->child -= (s16)rootNode;
        }
        if (def->sibling != -1) {
            def->sibling -= (s16)rootNode;
        }
        def->unkA = srcNode->unkA;
        for (comp = 0; comp < 4; comp++) {
            def->scale[comp] = srcNode->scale[comp];
            def->rot[comp] = srcNode->rot[comp];
            def->trans[comp] = srcNode->trans[comp];
        }
    }
    *dst = *built;
    dst->block = built;
    return 1;
}

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

s32 *ModelFile_GetTexData(u8 *modelFile, u32 texIdx);
void Heap_Fill(void *dst, u8 c, s32 n);

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

s32 ModelPart_StripCount(ModelChunk *part);

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
s32 Mesh_Build(MeshDesc *mesh, ModelChunk *part);
/* main's allocator hook gSysAllocHook (SysHeap_Alloc, set in display.c): not a symbol main exports. */
#define gSysAllocHook (*(void *(**)(s32 size))0x2491E0)
s32 Mesh_BuildRigid(MeshDesc *mesh, ModelChunk *part);
s32 Mesh_BuildSkinned(MeshDesc *mesh, ModelChunk *part);

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
 * vertex's weights become bytes; the remainder of 0xFF goes to the last nonzero
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
        weights[lastWeighted] += 0xFF - weights[0] - weights[1] - weights[2] - weights[3];
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

/* The pixels of BMP file bmp (its header's offBits on from its start), or NULL with a
 * debug message when bmp has no BMP_SIGNATURE. The lobby calls it. */
u8 *Bmp_GetPixels(u8 *bmp)
{
    BmpFileHeader *header = (BmpFileHeader *)bmp;

    if (header->type != BMP_SIGNATURE) {
        Debug_Sprintf("This is not BMP image. @plCreateContextFromBMPImage");
        return NULL;
    }
    return bmp + (header->offBits[0] | (header->offBits[1] << 16));
}

/* Sets gAnimKeyBase, the address the curves' key offsets (AnimCurve.keys) count from. */
void Anim_SetKeyBase(void *buf)
{
    gAnimKeyBase = (s32) buf;
}

/* The cubic Hermite curve through (startTime, startValue) and (endTime, endValue), with the
 * tangents startTangent and endTangent, at time. */
f32 Math_Hermite(f32 time, f32 startValue, f32 startTime, f32 startTangent, f32 endValue, f32 endTime, f32 endTangent)
{
    f32 elapsed;
    f32 invSpan;
    f32 cubeTerm;
    f32 invSpanSq;
    f32 sqTerm;
    f32 elapsedSq;
    f32 twoCube;
    f32 threeSq;
    f32 sqTermB;
    f32 basisStart;
    f32 basisStartTangent;
    f32 basisEnd;
    f32 basisEndTangent;

    elapsed = time - startTime;
    invSpan = 1.0f / (endTime - startTime);
    elapsedSq = elapsed * elapsed;
    invSpanSq = invSpan * invSpan;
    sqTerm = elapsedSq * invSpan;
    cubeTerm = invSpanSq * (elapsedSq * elapsed);
    twoCube = 2.0f * cubeTerm * invSpan;
    threeSq = 3.0f * elapsedSq * invSpanSq;
    sqTermB = sqTerm;
    basisStart = 1.0f + (twoCube - threeSq);
    basisStartTangent = elapsed + (cubeTerm - sqTermB - sqTermB);
    basisEnd = -twoCube + threeSq;
    basisEndTangent = cubeTerm - sqTermB;
    return startValue * basisStart + endValue * basisEnd + startTangent * basisStartTangent + endTangent * basisEndTangent;
}

/* The time of curve's first key (0 for ANIM_KEY_NONE and ANIM_KEY_S16_NONE). */
f32 AnimCurve_StartTime(AnimCurve *curve)
{
    u8 *keys = (u8 *)(curve->keys + gAnimKeyBase);
    f32 time;

    switch (curve->type) {
    case ANIM_KEY_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_F32_LINEAR:
        time = ((KeyF2 *)keys)->t;
        break;
    case ANIM_KEY_F32_CUBIC:
        time = ((KeyF4 *)keys)->t;
        break;
    case ANIM_KEY_F32_MIXED:
        time = ((KeyFM *)keys)->t;
        break;
    case ANIM_KEY_S16_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_S16_LINEAR:
        time = ((KeyS2 *)keys)->t;
        break;
    case ANIM_KEY_S16_CUBIC:
        time = ((KeyS4 *)keys)->t;
        break;
    case ANIM_KEY_S16_MIXED:
        time = ((KeySM *)keys)->t;
        break;
    }
    return time;
}

/* The time of curve's last key. */
f32 AnimCurve_GetEndTime(AnimCurve *curve)
{
    return AnimCurve_EndTime(curve);
}

/* The time of curve's last key (0 for ANIM_KEY_NONE and ANIM_KEY_S16_NONE). */
f32 AnimCurve_EndTime(AnimCurve *curve)
{
    u8 *keys = (u8 *)(curve->keys + gAnimKeyBase);
    f32 time;

    switch (curve->type) {
    case ANIM_KEY_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_F32_LINEAR:
        time = ((KeyF2 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_F32_CUBIC:
        time = ((KeyF4 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_F32_MIXED:
        time = ((KeyFM *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_S16_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_S16_LINEAR:
        time = ((KeyS2 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_S16_CUBIC:
        time = ((KeyS4 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_S16_MIXED:
        time = ((KeySM *)keys)[curve->count - 1].t;
        break;
    }
    return time;
}

/* Copies the 3x3 rotation part of the 4x4 matrix src (rows 0-2, columns 0-2) into dst,
 * leaving the rest of dst as it is. */
void Mtx_Copy3x3(f32 *dst, f32 *src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[4] = src[4];
    dst[5] = src[5];
    dst[6] = src[6];
    dst[8] = src[8];
    dst[9] = src[9];
    dst[10] = src[10];
}

f32 sqrtf(f32 x);

/* |vec| (the length of a 3-vector). */
f32 Vec_Length2(f32 *vec)
{
    return sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

/* Scales the 3-vector vec to unit length (as Vec_Normalize); a zero vector stays zero. */
void Vec_Normalize2(f32 *vec)
{
    f32 len;

    len = Vec_Length2(vec);
    if (len == 0.0f) {
        vec[2] = 0.0f;
        vec[1] = 0.0f;
        vec[0] = 0.0f;
        return;
    }
    vec[0] /= len;
    vec[1] = vec[1] / len;
    vec[2] = vec[2] / len;
}

/* lhs . rhs (as Vec_Dot). */
f32 Vec_Dot2(f32 *lhs, f32 *rhs)
{
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

/* dst = lhs x rhs. */
void Vec_Cross2(f32 *dst, f32 *lhs, f32 *rhs)
{
    dst[0] = lhs[1] * rhs[2] - rhs[1] * lhs[2];
    dst[1] = lhs[2] * rhs[0] - rhs[2] * lhs[0];
    dst[2] = lhs[0] * rhs[1] - rhs[0] * lhs[1];
}

/* The plane through pointA, pointB and pointC: the unit normal of edgeAB x edgeAC, and in
 * [3] minus its dot product with edgeAB (as the game computes it: that is 0, where a
 * plane's distance would use pointA). */
void Plane_FromPoints(f32 *plane, f32 *pointA, f32 *pointB, f32 *pointC)
{
    f32 edgeAB[3];
    f32 edgeAC[3];
    f32 normal[3];

    edgeAB[0] = pointB[0] - pointA[0];
    edgeAB[1] = pointB[1] - pointA[1];
    edgeAB[2] = pointB[2] - pointA[2];
    edgeAC[0] = pointC[0] - pointA[0];
    edgeAC[1] = pointC[1] - pointA[1];
    edgeAC[2] = pointC[2] - pointA[2];
    Vec_Cross2(normal, edgeAB, edgeAC);
    Vec_Normalize2(normal);
    plane[0] = normal[0];
    plane[1] = normal[1];
    plane[2] = normal[2];
    plane[3] = -Vec_Dot2(normal, edgeAB);
}

f64 fptodp(f32 x);           /* float to double (main) */
f64 tan(f64 x);             /* the C library's tangent (main) */
f32 dptofp(f64 x);           /* double to float (main) */

/* (f32)tan(x), with main's double helpers named as they are. */
#define TAN(x) dptofp(tan(fptodp(x)))

/* The four side planes of the view frustum (fov, aspect) through the eye, and in [16], [17]
 * minus near and minus far. */
void Frustum_BuildPlanes(f32 *planes, f32 fov, f32 aspect, f32 near, f32 far)
{
    f32 eye[4];
    f32 cornerA[4];
    f32 cornerB[4];
    f32 halfFov;
    f32 negFar;

    halfFov = 0.5f * fov;
    negFar = -far;
    eye[0] = 0.0f;
    eye[1] = 0.0f;
    eye[2] = 0.0f;
    cornerB[2] = negFar;
    cornerA[2] = negFar;
    cornerA[0] = far * (TAN(halfFov) / aspect);
    cornerA[1] = negFar * TAN(halfFov);
    cornerB[0] = cornerA[0];
    cornerB[1] = -cornerA[1];
    Plane_FromPoints(planes, eye, cornerA, cornerB);
    cornerA[0] = negFar * (TAN(halfFov) / aspect);
    cornerA[1] = far * TAN(halfFov);
    cornerB[0] = cornerA[0];
    cornerB[1] = -cornerA[1];
    Plane_FromPoints(planes + 4, eye, cornerA, cornerB);
    cornerA[0] = far * (TAN(halfFov) / aspect);
    cornerA[1] = far * TAN(halfFov);
    cornerB[0] = -cornerA[0];
    cornerB[1] = cornerA[1];
    Plane_FromPoints(planes + 8, eye, cornerA, cornerB);
    cornerA[0] = negFar * (TAN(halfFov) / aspect);
    cornerA[1] = negFar * TAN(halfFov);
    cornerB[0] = -cornerA[0];
    cornerB[1] = cornerA[1];
    Plane_FromPoints(planes + 12, eye, cornerA, cornerB);
    planes[17] = negFar;
    planes[16] = -near;
}

/* Links the nodeCount words at nodes into a free list: each holds the address of the next, the last 0. */
void FreeList_Init(s32 nodeCount, u32 *nodes)
{
    s32 nodeIdx;

    for (nodeIdx = 0; nodeIdx < nodeCount - 1; nodeIdx++) {
        nodes[nodeIdx] = (u32)&nodes[nodeIdx + 1];
    }
    nodes[nodeIdx] = 0;
}

/* Pushes node onto the list at *link: node takes the old head, and *link the node's
 * address with bit 0 set (the tag TagList_Pop looks for). */
void TagList_Push(u32 *node, u32 *link)
{
    u32 next;

    next = *link;
    *link = (u32)node | 1;
    *node = next;
}

/* Pops the head of a list whose link word carries a flag in bit 0. */
void *TagList_Pop(u32 *head)
{
    u32 nextWord;

    for (;;) {
        if (*head == 0) {
            return NULL;
        }
        nextWord = *(u32 *)*head;
        if (nextWord & 1) {
            return (void *)(*head = nextWord & ~1);
        }
        *head = nextWord;
    }
}

extern s8 gUsbKbStep;
extern s32 (*jtbl_00403140[])(s8 *step);

/*
 * The USB keyboard (libusbkb, linked in main). Its set-up is a run of steps, each
 * given the step counter gUsbKbStep: a step advances it on success, resets it to 0
 * on failure, and returns 1 to go on (0 when it reset it, or at the end).
 * jtbl_00403140 holds the whole run, UsbKb_StepNext to UsbKb_StepDone (it keeps
 * splat's jump-table name: the port's recompiler finds tables by that name);
 * gUsbKbSetupSteps the same steps from UsbKb_StepWaitConnect on, which UsbKb_Poll
 * runs until gUsbKbReady is set.
 */

/* Runs step gUsbKbStep of jtbl_00403140. */
s32 UsbKb_RunInitStep(void)
{
    return jtbl_00403140[gUsbKbStep](&gUsbKbStep);
}

/* The first step: only advances. */
s32 UsbKb_StepNext(s8 *step)
{
    (*step)++;
    return 1;
}

s32 sceUsbKbInit(s32 *st);
s32 sceUsbKbGetInfo(void *p);
s32 sceUsbKbSync(s32 a, s32 *st);
extern s8 gUsbKbLibInit;
extern s32 gUsbKbSyncResult;
extern UsbKbInfo gUsbKbInfo;

/* Initialises libusbkb (sceUsbKbInit) once (gUsbKbLibInit), advancing when it
 * reports a nonzero status. */
s32 UsbKb_StepInit(s8 *step)
{
    s32 status;

    if (gUsbKbLibInit == 0) {
        if (sceUsbKbInit(&status) != 0) {
            *step = 0;
            return 0;
        }
        if (status == 0) {
            *step = 0;
            return 0;
        }
        gUsbKbSyncResult = 0;
        gUsbKbLibInit = 1;
        (*step)++;
        return 1;
    } else {
        *step = 0;
        return 0;
    }
}

/* Reads the keyboards' state into gUsbKbInfo (sceUsbKbGetInfo, then sceUsbKbSync);
 * advances when a keyboard is connected, resets when none is. */
s32 UsbKb_StepWaitConnect(s8 *step)
{
    s32 status;

    status = sceUsbKbGetInfo(&gUsbKbInfo);
    if (status != 0) {
        *step = 0;
        return 0;
    }
    gUsbKbSyncResult = sceUsbKbSync(0, &status);
    if (gUsbKbSyncResult == 0 && status == 0) {
        if (gUsbKbInfo.connectedCount == 0) {
            *step = 0;
            return 0;
        }
        (*step)++;
    }
    return 1;
}

s32 sceUsbKbSetRepeat(s32, s32, s32);
s32 sceUsbKbSetCodeType(s32 port, u32 mode);
s32 sceUsbKbSetArrangement(s32, u32);
s32 sceUsbKbSetReadMode(s32, u32);
extern s32 D_005819C0;
extern s32 D_005819C8;

/* Sets keyboard 0 up (no key repeat, code type 0, arrangement 1, read mode 0) when it
 * is connected, else resets; advances when every call succeeds. D_005819C8 and
 * D_005819C0 are only written. */
s32 UsbKb_StepSetup(s8 *step)
{
    s32 repeatErr;
    s32 codeTypeErr;
    s32 arrangeErr;
    s32 readModeErr;

    if (gUsbKbInfo.connected[0] == 0) {
        *step = 0;
        return 0;
    }
    repeatErr = sceUsbKbSetRepeat(0, 0, 0);
    codeTypeErr = sceUsbKbSetCodeType(0, 0U);
    D_005819C8 = 1;
    arrangeErr = sceUsbKbSetArrangement(0, 1U);
    D_005819C0 = 0;
    readModeErr = sceUsbKbSetReadMode(0, 0U);
    if ((repeatErr == 0) && (codeTypeErr == 0) && (arrangeErr == 0) && (readModeErr == 0)) {
        *step += 1;
    }
    return 1;
}

s32 sceUsbKbSetLEDMode(s32 a, s32 b);

/* Sets keyboard 0's LED mode to 0 (sceUsbKbSetLEDMode) and waits for it (sceUsbKbSync);
 * advances when both succeed. */
s32 UsbKb_StepSetLed(s8 *step)
{
    s32 status;

    status = sceUsbKbSetLEDMode(0, 0);
    if (status == 0) {
        gUsbKbSyncResult = sceUsbKbSync(0, &status);
        if (gUsbKbSyncResult == 0 && status == 0) {
            (*step)++;
        }
    }
    return 1;
}

/* The last step: sets gUsbKbReady and starts the run over. Returns 0. */
s32 UsbKb_StepDone(s8 *step)
{
    *step = 0;
    gUsbKbReady = 1;
    return 0;
}

void *memset(void *d, s32 c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
s32 sceUsbKbRead(s32 port, void *buf);
s32 UsbKb_IsConnected(s32 i);
extern s8 gUsbKbStep;
extern s32 (*gUsbKbSetupSteps[])(s8 *step);
extern u8 gKbdSettings[0x88];
extern UsbKbData gUsbKbData;
extern u8 D_005819A0[];
extern u8 gUsbKbReport[9];
extern u8 *D_005819B0;          /* its first byte is gUsbKbReport[8] */

/*
 * The USB keyboard poll: runs step gUsbKbStep of the set-up table at gUsbKbSetupSteps until
 * gUsbKbReady is set; then reads keyboard 0 (sceUsbKbRead) into gUsbKbData, copies
 * sizeof(gKbdSettings) bytes from there into gKbdSettings (and then gKbdSettings onto
 * itself, as the game does) and, when it has key codes, returns them as a 9-byte report
 * in gUsbKbReport (else NULL). D_005819B0 is written with D_005819A0's address, and
 * then its low byte with the last key code (gUsbKbReport[8] is the same byte).
 */
u8 *UsbKb_Poll(void)
{
    s32 status;

    memset(gKbdSettings, 0, sizeof(gKbdSettings));
    if (gUsbKbReady == 0) {
        gUsbKbSetupSteps[gUsbKbStep](&gUsbKbStep);
        return NULL;
    }
    if (UsbKb_IsConnected(0) == -1) {
        return NULL;
    }
    sceUsbKbRead(0, &gUsbKbData);
    memcpy(gKbdSettings, &gUsbKbData, sizeof(gKbdSettings));
    sceUsbKbSync(0, &status);
    memcpy(gKbdSettings, gKbdSettings, sizeof(gKbdSettings));
    if (gUsbKbData.keyCount == 0) {
        return NULL;
    }
    gUsbKbReport[0] = gUsbKbData.modifiers;
    D_005819B0 = D_005819A0;
    gUsbKbReport[1] = gUsbKbData.leds;
    gUsbKbReport[2] = gUsbKbData.keys[0];
    gUsbKbReport[3] = gUsbKbData.keys[1];
    gUsbKbReport[4] = gUsbKbData.keys[2];
    gUsbKbReport[5] = gUsbKbData.keys[3];
    gUsbKbReport[6] = gUsbKbData.keys[4];
    gUsbKbReport[7] = gUsbKbData.keys[5];
    gUsbKbReport[8] = gUsbKbData.keys[6];
    return gUsbKbReport;
}

/* 0 when keyboard port is connected (gUsbKbInfo, read again), else -1. */
s32 UsbKb_IsConnected(s32 port)
{
    s32 status;

    if (sceUsbKbGetInfo(&gUsbKbInfo) != 0) {
        return -1;
    }
    sceUsbKbSync(0, &status);
    if (status != 0) {
        return -1;
    }
    if (gUsbKbInfo.connected[port] == 0) {
        return -1;
    }
    return 0;
}

/* unk_2F6540's .data (0x403140-0x403170) */
s32 (*jtbl_00403140[8])(s8 *step) = {
    UsbKb_StepNext, UsbKb_StepInit, UsbKb_StepWaitConnect, UsbKb_StepSetLed,
    UsbKb_StepSetup, UsbKb_StepDone, NULL, NULL,
};
s32 (*gUsbKbSetupSteps[4])(s8 *step) = {
    UsbKb_StepWaitConnect, UsbKb_StepSetLed, UsbKb_StepSetup, UsbKb_StepDone,
};
