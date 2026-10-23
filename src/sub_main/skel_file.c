/*
 * Skeleton files: counting and finding the
 * parts, chunks and nodes of a skeleton file (SkelFileHeader, SkelChunk
 * and SkelFileNode, include/model.h), and SkelFile_BuildPart, which builds
 * a part's skeleton from them.
 */
#include "common.h"
#include "model.h"

extern s32 gSkelTreeCount;

static SkelFileHeader *SkelFile_Header(void *file);

/* The file's bones: its chunks, less the parts chunk when it has one. */
s32 SkelFile_BoneCount(void *file)
{
    SkelFileHeader *header = SkelFile_Header(file);
    s32 n = header->count;

    if (header->flags & SKEL_FILE_HAS_PARTS) {
        n -= 1;
    }
    return n;
}

/* The file's parts (its parts chunk's count), 0 without a parts chunk. */
s32 SkelFile_PartCount(void *file)
{
    u8 *chunk = SkelFile_FindChunk(file, SKEL_CHUNK_PARTS);

    if (chunk != NULL) {
        return ((SkelChunk *)chunk)->count;
    }
    return 0;
}

/* Part i's root node, -1 when there is no such part. */
s32 SkelFile_PartRoot(void *file, u32 i)
{
    u8 *chunk = SkelFile_FindChunk(file, SKEL_CHUNK_PARTS);

    if (chunk != NULL && i < ((SkelChunk *)chunk)->count) {
        return ((s32 *)(chunk + sizeof(SkelChunk)))[i];
    }
    return -1;
}

/* The bones of part i: the nodes of its tree. */
s32 SkelFile_PartBoneCount(void *file, s32 i)
{
    return SkelFile_CountTree(file, SkelFile_PartRoot(file, i));
}

/* Counts the nodes of the tree below node. */
s32 SkelFile_CountTree(void *file, s32 node)
{
    gSkelTreeCount = 0;
    SkelFile_CountTreeRec(file, node);
    return gSkelTreeCount;
}

/* Counts node, then its child's tree and its sibling's (into
 * gSkelTreeCount). */
void SkelFile_CountTreeRec(void *file, s32 node)
{
    SkelFileNode *fileNode;

    gSkelTreeCount++;
    fileNode = (SkelFileNode *)SkelFile_GetNode(file, node);
    if (fileNode->child != -1) {
        SkelFile_CountTreeRec(file, fileNode->child);
    }
    if (fileNode->sibling != -1) {
        SkelFile_CountTreeRec(file, fileNode->sibling);
    }
}

static SkelFileHeader *SkelFile_Header(void *file)
{
    return file;
}

/* The first chunk of a type, NULL when there is none. The parameter is
 * reused as the chunk cursor. */
u8 *SkelFile_FindChunk(void *file, s32 type)
{
    u32 i;
    u32 n = SkelFile_Header(file)->count;

    file = (u8 *)file + sizeof(SkelFileHeader);
    for (i = 0; i < n; i++) {
        if (((SkelChunk *)file)->type == type) {
            return file;
        }
        file = (u8 *)file + ((SkelChunk *)file)->size;
    }
    return NULL;
}

/* The idx-th node chunk (a chunk whose type is not the parts chunk's),
 * NULL when there is none. */
u8 *SkelFile_NodeChunk(void *file, u32 idx)
{
    u32 k = 0;
    u32 i;
    u32 n = SkelFile_Header(file)->count;

    if (idx < n) {
        file = (u8 *)file + sizeof(SkelFileHeader);
        for (i = 0; i < n; i++) {
            if (((SkelChunk *)file)->type != SKEL_CHUNK_PARTS) {
                if (k == idx) {
                    return file;
                }
                k++;
            }
            file = (u8 *)file + ((SkelChunk *)file)->size;
        }
    }
    return NULL;
}

/* Node `node` of the file (a SkelFileNode, after its chunk's header),
 * NULL when there is none. */
s32 *SkelFile_GetNode(void *file, s32 node)
{
    u8 *chunk = SkelFile_NodeChunk(file, node);

    if (chunk != NULL) {
        return (s32 *)(chunk + sizeof(SkelChunk));
    }
    return NULL;
}

/* The bytes part i's skeleton takes when built: a 16-byte header and
 * 64 bytes a bone (SkelHeader and BoneDef); 0 when there
 * is no such part. */
s32 SkelFile_PartSize(void *file, s32 i)
{
    if (i >= SkelFile_PartCount(file)) {
        return 0;
    }
    i = SkelFile_PartBoneCount(file, i);
    return (i << 6) + 16;
}

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
