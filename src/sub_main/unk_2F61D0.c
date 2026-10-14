/*
 * A chunked data block: a header {u32 flags, u32 count} and, from +0xC,
 * chunks {u8 type, ..., u32 size at +8}. Chunk type 0 holds a table of
 * node indices; the nodes form a binary tree (+8/+0xC children, -1 for
 * none).
 */
#include "common.h"

typedef struct ChunkHdr {
    u32 flags;                  /* 0x0: bit 31 */
    u32 count;                  /* 0x4 */
} ChunkHdr;

extern s32 gSkelTreeCount;

ChunkHdr *AnimFile_Header(void *f);
static ChunkHdr *SkelFile_Header(void *f);
u8 *SkelFile_FindChunk(void *f, s32 type);
s32 SkelFile_PartRoot(void *f, u32 i);
s32 SkelFile_CountTree(void *f, s32 node);
void SkelFile_CountTreeRec(void *f, s32 node);
u8 *SkelFile_NodeChunk(void *f, u32 idx);
s32 *SkelFile_GetNode(void *f, s32 node);
s32 SkelFile_PartCount(void *f);
s32 SkelFile_PartBoneCount(void *f, s32 i);

s32 AnimFile_Count(void *f)
{
    return AnimFile_Header(f)->count;
}

s32 SkelFile_BoneCount(void *f)
{
    ChunkHdr *h = SkelFile_Header(f);
    s32 n = h->count;

    if (h->flags & 0x80000000) {
        n -= 1;
    }
    return n;
}

s32 SkelFile_PartCount(void *f)
{
    u8 *c = SkelFile_FindChunk(f, 0);

    if (c != NULL) {
        return *(s32 *)(c + 4);
    }
    return 0;
}

s32 SkelFile_PartRoot(void *f, u32 i)
{
    u8 *c = SkelFile_FindChunk(f, 0);

    if (c != NULL && i < *(u32 *)(c + 4)) {
        return ((s32 *)(c + 0xC))[i];
    }
    return -1;
}

s32 SkelFile_PartBoneCount(void *f, s32 i)
{
    return SkelFile_CountTree(f, SkelFile_PartRoot(f, i));
}

/* Counts the nodes of the tree below node. */
s32 SkelFile_CountTree(void *f, s32 node)
{
    gSkelTreeCount = 0;
    SkelFile_CountTreeRec(f, node);
    return gSkelTreeCount;
}

void SkelFile_CountTreeRec(void *f, s32 node)
{
    s32 *n;

    gSkelTreeCount++;
    n = SkelFile_GetNode(f, node);
    if (n[2] != -1) {
        SkelFile_CountTreeRec(f, n[2]);
    }
    if (n[3] != -1) {
        SkelFile_CountTreeRec(f, n[3]);
    }
}

static ChunkHdr *SkelFile_Header(void *f)
{
    return f;
}

/* The first chunk of a type. */
u8 *SkelFile_FindChunk(void *f, s32 type)
{
    u32 i;
    u32 n = SkelFile_Header(f)->count;

    f = (u8 *)f + 0xC;
    for (i = 0; i < n; i++) {
        if (*(u8 *)f == type) {
            return f;
        }
        f = (u8 *)f + *(u32 *)((u8 *)f + 8);
    }
    return NULL;
}

/* The idx-th chunk whose type is not 0. */
u8 *SkelFile_NodeChunk(void *f, u32 idx)
{
    u32 k = 0;
    u32 i;
    u32 n = SkelFile_Header(f)->count;

    if (idx < n) {
        f = (u8 *)f + 0xC;
        for (i = 0; i < n; i++) {
            if (*(u8 *)f != 0) {
                if (k == idx) {
                    return f;
                }
                k++;
            }
            f = (u8 *)f + *(u32 *)((u8 *)f + 8);
        }
    }
    return NULL;
}

s32 *SkelFile_GetNode(void *f, s32 node)
{
    u8 *c = SkelFile_NodeChunk(f, node);

    if (c != NULL) {
        return (s32 *)(c + 0xC);
    }
    return NULL;
}

s32 SkelFile_PartSize(void *f, s32 i)
{
    if (i >= SkelFile_PartCount(f)) {
        return 0;
    }
    i = SkelFile_PartBoneCount(f, i);
    return (i << 6) + 0x10;
}
