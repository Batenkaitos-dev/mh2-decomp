/*
 * Skeleton files: counting and finding the
 * parts, chunks and nodes of a skeleton file (SkelFileHeader, SkelChunk
 * and SkelFileNode, include/model.h), for SkelFile_BuildPart, which builds
 * the skeletons from them. Also an animation file's entry count.
 */
#include "common.h"
#include "model.h"

extern s32 gSkelTreeCount;

void *AnimFile_Header(void *file);
static SkelFileHeader *SkelFile_Header(void *file);
u8 *SkelFile_FindChunk(void *file, s32 type);
s32 SkelFile_PartRoot(void *file, u32 i);
s32 SkelFile_CountTree(void *file, s32 node);
void SkelFile_CountTreeRec(void *file, s32 node);
u8 *SkelFile_NodeChunk(void *file, u32 idx);
s32 *SkelFile_GetNode(void *file, s32 node);
s32 SkelFile_PartCount(void *file);
s32 SkelFile_PartBoneCount(void *file, s32 i);

/* The entries of an animation file: its header starts like a skeleton
 * file's, flags then a count. */
s32 AnimFile_Count(void *file)
{
    return ((SkelFileHeader *)AnimFile_Header(file))->count;
}

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

/* The bytes part i's skeleton takes when built: a 0x10-byte header and
 * 0x40 bytes a bone (SkelHeader and BoneDef); 0 when there
 * is no such part. */
s32 SkelFile_PartSize(void *file, s32 i)
{
    if (i >= SkelFile_PartCount(file)) {
        return 0;
    }
    i = SkelFile_PartBoneCount(file, i);
    return (i << 6) + 0x10;
}
