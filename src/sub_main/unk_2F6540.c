/*
 * Readers for the chunked model data (see unk_2F61D0.c): chunks with an
 * s32 type at +0 and the size at +8, found by type and index; the chunk
 * readers after them keep registers across the static finders (one unit
 * with the old unk_2F7420.c). Then a BMP header check, the animation curve
 * and 2D vector helpers, frustum planes, a free list, and the USB keyboard
 * steps. ACCESS's kana-kanji engine starts after it, at 0x2F9D80.
 */
#include "common.h"
#include "em.h"

static u8 *ModelFile_Header(u8 *h);
static s32 ModelChunk_Check(u8 *h, s32 type);
static u8 *ModelChunk_Get(u8 *h, s32 type, s32 idx);
u8 *ModelFile_GetPart(u8 *h, u32 idx);
u8 *ModelFile_GetMat(u8 *h, u32 idx);
u8 *ModelFile_GetTex(u8 *h, u32 idx);

/* An animation curve (8 bytes) and its key formats, as in unk_2F17C0.c (the keys at
 * gAnimKeyBase + keys; the 16-bit formats hold signed values). */
typedef struct AnimCurve {
    u8 type;                    /* 0x0: the key format */
    u8 idx;                     /* 0x1: the pose value it drives */
    u16 count;                  /* 0x2: keys */
    s32 keys;                   /* 0x4: offset of the keys from gAnimKeyBase */
} AnimCurve;

typedef struct KeyF2 { f32 v, t; } KeyF2;                       /* 0x21: linear */
typedef struct KeyF4 { f32 v, t, in, out; } KeyF4;              /* 0x22: cubic */
typedef struct KeyFM { u32 mode; f32 v, t, in, out; } KeyFM;    /* 0x23: either, per key */
typedef struct KeyS2 { s16 v, t; } KeyS2;                       /* 0x11 */
typedef struct KeyS4 { s16 v, t, in, out; } KeyS4;              /* 0x12 */
typedef struct KeySM { u32 mode; s16 v, t, in, out; } KeySM;    /* 0x13 */

/* prototypes */
f32 AnimCurve_EndTime(AnimCurve *c);
f32 Vec_Length2(f32 *);
s32 Debug_Sprintf(const char *fmt, ...);
extern s32 gAnimKeyBase;
extern s16 gUsbKbReady;
/* end of prototypes */

/* A skeleton as loaded: a 16-byte header, then count BoneDefs of 0x40 bytes. */
typedef struct SkelHeader {
    s16 unk0;                   /* 0x0 */
    s16 count;                  /* 0x2 */
    s32 size;                   /* 0x4: of the header (0x10) */
    struct SkelHeader *next;    /* 0x8 */
    s32 unkC;                   /* 0xC */
} SkelHeader;

typedef struct BoneDef {
    s16 index;                  /* 0x00 */
    u16 id;                     /* 0x02 */
    u8 pad04[2];
    s16 child;                  /* 0x06 */
    s16 sibling;                /* 0x08 */
    s16 unkA;                   /* 0x0A */
    u8 pad0C[4];
    f32 scale[4];               /* 0x10 */
    f32 rot[4];                 /* 0x20 */
    f32 trans[4];               /* 0x30 */
} BoneDef;

/* A node of the model file (SkelFile_GetNode). */
typedef struct FileNode {
    s32 index;                  /* 0x00 */
    s32 unk4;
    s32 child;                  /* 0x08 */
    s32 sibling;                /* 0x0C */
    f32 scale[4];               /* 0x10 */
    f32 rot[4];                 /* 0x20 */
    f32 trans[4];               /* 0x30 */
    s32 unkA;                   /* 0x40 */
    s32 id;                     /* 0x44 */
} FileNode;

s32 SkelFile_PartCount(void *f);
s32 SkelFile_PartRoot(void *f, u32 i);
s32 SkelFile_PartBoneCount(void *f, s32 i);
s32 *SkelFile_GetNode(void *f, s32 node);
s32 SkelFile_PartSize(void *f, s32 i);

/* Builds skeleton part of file f in h (the bones follow the header), copies the header
 * to dst and links it; 0 for a bad part. */
s32 SkelFile_BuildPart(SkelHeader *dst, SkelHeader *h, void *f, s32 part)
{
    s32 n;
    s32 i;
    s32 k;
    s32 base;
    BoneDef *d;
    BoneDef *b;
    FileNode *src;

    if (part >= SkelFile_PartCount(f)) {
        return 0;
    }
    h->unkC = SkelFile_PartSize(f, part);
    h->count = n = SkelFile_PartBoneCount(f, part);
    h->size = 0x10;
    d = (BoneDef *)(h + 1);
    h->next = NULL;
    for (i = 0; i < n; i++) {
        b = &d[i];
        for (k = 0; k < 4; k++) {
            b->scale[k] = 0.0f;
            b->rot[k] = 0.0f;
            b->trans[k] = 0.0f;
        }
    }
    base = SkelFile_PartRoot(f, part);
    for (i = 0; i < n; i++, d++) {
        src = (FileNode *)SkelFile_GetNode(f, i + base);
        d->index = src->index;
        d->id = src->id;
        d->child = src->child;
        d->sibling = src->sibling;
        if (d->child != -1) {
            d->child -= (s16)base;
        }
        if (d->sibling != -1) {
            d->sibling -= (s16)base;
        }
        d->unkA = src->unkA;
        for (k = 0; k < 4; k++) {
            d->scale[k] = src->scale[k];
            d->rot[k] = src->rot[k];
            d->trans[k] = src->trans[k];
        }
    }
    *dst = *h;
    dst->next = h;
    return 1;
}

/* The list in chunk 0x100000 of part idx of file f (a part of kind 4) into out: the
 * count, then the entries as s16. 0, or -1. */
s32 ModelFile_GetPartList(s16 *out, u8 *f, s32 idx)
{
    u8 *p;
    u8 *c;
    s32 *src;
    s32 i;
    s32 n;

    p = ModelFile_GetPart(f, idx);
    if (*(s32 *)p != 4) {
        return -1;
    }
    c = ModelChunk_Get(p, 0x100000, 0);
    if (c == NULL) {
        return -1;
    }
    n = *(s32 *)(c + 4);
    src = (s32 *)(c + 0xC);
    out[0] = n;
    for (i = 0; i < n; i++) {
        out[i + 1] = src[0];
        src = &src[1];
    }
    return 0;
}



s32 ModelFile_GetDrawStates(void *ds, u8 *f, s32 idx)
{
    u8 *h;
    u8 *chunk;
    u8 *src;

    h = ModelFile_GetPart((u8 *) f, (u32) idx);
    if (*(s32 *)h != 4) {
        return -1;
    }
    chunk = ModelChunk_Get((u8 *) h, 0xF0000, 0);
    if (chunk == NULL) {
        return -1;
    }
    src = chunk + 0xC;
    *(s32 *)((u8 *)ds + 0) = (s32) *(s32 *)((u8 *)chunk + 0xC);
    if ((*(s32 *)((u8 *)ds + 0) & 0xFFFF0000) != 0x10000) {
        return -2;
    }
    *(s32 *)((u8 *)ds + 0) = (s32) *(s32 *)((u8 *)chunk + 0xC);
    *(s32 *)((u8 *)ds + 4) = (s32) *(s32 *)((u8 *)src + 4);
    *(s32 *)((u8 *)ds + 8) = (s32) *(s32 *)((u8 *)src + 8);
    *(s32 *)((u8 *)ds + 0xC) = (s32) *(s32 *)((u8 *)src + 0xC);
    *(s32 *)((u8 *)ds + 0x10) = (s32) *(s32 *)((u8 *)src + 0x10);
    *(s32 *)((u8 *)ds + 0x14) = (s32) *(s32 *)((u8 *)src + 0x14);
    *(s32 *)((u8 *)ds + 0x18) = (s32) *(s32 *)((u8 *)src + 0x18);
    *(s32 *)((u8 *)ds + 0x1C) = (s32) *(s32 *)((u8 *)src + 0x1C);
    *(s32 *)((u8 *)ds + 0x20) = (s32) *(s32 *)((u8 *)src + 0x20);
    *(s32 *)((u8 *)ds + 0x24) = (s32) *(s32 *)((u8 *)src + 0x24);
    *(s32 *)((u8 *)ds + 0x28) = (s32) *(s32 *)((u8 *)src + 0x28);
    *(s32 *)((u8 *)ds + 0x2C) = (s32) *(s32 *)((u8 *)src + 0x2C);
    *(s32 *)((u8 *)ds + 0x30) = (s32) *(s32 *)((u8 *)src + 0x30);
    *(s32 *)((u8 *)ds + 0x34) = (s32) *(s32 *)((u8 *)src + 0x34);
    *(s32 *)((u8 *)ds + 0x38) = (s32) *(s32 *)((u8 *)src + 0x38);
    *(s32 *)((u8 *)ds + 0x3C) = (s32) *(s32 *)((u8 *)src + 0x3C);
    *(s32 *)((u8 *)ds + 0x40) = (s32) *(s32 *)((u8 *)src + 0x40);
    *(s32 *)((u8 *)ds + 0x44) = (s32) *(s32 *)((u8 *)src + 0x44);
    return 0;
}

s32 ModelFile_PartCount(u8 *f)
{
    u8 *h = ModelFile_Header(f);
    u8 *c;

    if (ModelChunk_Check(h, 2) != 0) {
        return 0;
    }
    c = ModelChunk_Get(h, 2, 0);
    return c == NULL ? 0 : *(s32 *)(c + 4);
}

u8 *ModelFile_GetPart(u8 *f, u32 idx)
{
    u8 *h = ModelFile_Header(f);
    u8 *c;

    if (ModelChunk_Check(h, 2) != 0) {
        return NULL;
    }
    c = ModelChunk_Get(h, 2, 0);
    if (c == NULL) {
        return NULL;
    }
    if (*(u32 *)(c + 4) < idx) {
        return NULL;
    }
    c = ModelChunk_Get(c, -1, idx);
    if (c == NULL) {
        return NULL;
    }
    return c;
}

s32 ModelFile_MatCount(u8 *f)
{
    u8 *h = ModelFile_Header(f);
    u8 *c;

    if (ModelChunk_Check(h, 9) != 0) {
        return 0;
    }
    c = ModelChunk_Get(h, 9, 0);
    return c == NULL ? 0 : *(s32 *)(c + 4);
}

u8 *ModelFile_GetMat(u8 *f, u32 idx)
{
    u8 *h = ModelFile_Header(f);
    u8 *c;

    if (ModelChunk_Check(h, 9) != 0) {
        return NULL;
    }
    c = ModelChunk_Get(h, 9, 0);
    if (c == NULL) {
        return NULL;
    }
    if (*(u32 *)(c + 4) < idx) {
        return NULL;
    }
    c = ModelChunk_Get(c, -1, idx);
    if (c == NULL) {
        return NULL;
    }
    return c;
}

u8 *ModelFile_GetMatData(u8 *f, u32 idx)
{
    u8 *c = ModelFile_GetMat(f, idx);

    if (c == NULL) {
        return NULL;
    }
    return c + 0xC;
}

u8 * ModelFile_GetMatData(u8 *f, u32 idx);
u8 * ModelFile_GetTexData(u8 *f, u32 idx);
void Heap_Fill(void *dst, u8 c, s32 n);

s32 ModelFile_ReadMat(s32 f, s32 idx, ModelMat *dst)
{
    u8 *mat;

    mat = ModelFile_GetMatData((u8 *) f, (u32) idx);
    Heap_Fill(dst, 0, 0x4C);
    if (*(s32 *)((u8 *)mat + 0x34) != 0) {
        dst->tex = *(s32 *)ModelFile_GetTexData((u8 *) f, *(u32 *)((u8 *)mat + 0x100));
    } else {
        dst->tex = 0;
    }
    dst->rgb[0] = (f32) *(f32 *)((u8 *)mat + 0x10);
    dst->rgb[1] = (f32) *(f32 *)((u8 *)mat + 0x14);
    dst->rgb[2] = (f32) *(f32 *)((u8 *)mat + 0x18);
    dst->alpha = (f32) *(f32 *)((u8 *)mat + 0x1C);
    dst->unk24[0] = (f32) *(f32 *)((u8 *)mat + 0);
    dst->unk24[1] = (f32) *(f32 *)((u8 *)mat + 4);
    dst->unk24[2] = (f32) *(f32 *)((u8 *)mat + 8);
    dst->unk24[3] = (f32) *(f32 *)((u8 *)mat + 0xC);
    dst->unk14[0] = (f32) *(f32 *)((u8 *)mat + 0x20);
    dst->unk14[1] = (f32) *(f32 *)((u8 *)mat + 0x24);
    dst->unk14[2] = (f32) *(f32 *)((u8 *)mat + 0x28);
    dst->unk14[3] = (f32) *(f32 *)((u8 *)mat + 0x2C);
    dst->unk34[0] = 0;
    dst->unk34[1] = 0;
    dst->unk34[2] = 0;
    dst->unk34[3] = 0;
    dst->size = (s32) *(f32 *)((u8 *)mat + 0x30);
    return 1;
}

u8 *ModelFile_GetTex(u8 *f, u32 idx)
{
    u8 *h = ModelFile_Header(f);
    u8 *c;

    if (ModelChunk_Check(h, 0xA) != 0) {
        return NULL;
    }
    c = ModelChunk_Get(h, 0xA, 0);
    if (c == NULL) {
        return NULL;
    }
    if (*(u32 *)(c + 4) < idx) {
        return NULL;
    }
    c = ModelChunk_Get(c, -1, idx);
    if (c == NULL) {
        return NULL;
    }
    return c;
}

u8 *ModelFile_GetTexData(u8 *f, u32 idx)
{
    u8 *c = ModelFile_GetTex(f, idx);

    return c == NULL ? NULL : c + 0xC;
}

s32 ModelFile_PartMatCount(u8 *f, u32 idx)
{
    u8 *c = ModelFile_GetPart(f, idx);

    if (c == NULL) {
        return 0;
    }
    c = ModelChunk_Get(c, 0x50000, 0);
    return c == NULL ? 0 : *(s32 *)(c + 4);
}

/* The words of chunk 0x50000 of part idx of file f into out; 1, or 0. */
s32 ModelFile_GetPartMats(u8 *f, u32 idx, s32 *out)
{
    u8 *p;
    u8 *c;
    u32 i;
    u32 n;
    s32 *src;

    p = ModelFile_GetPart(f, idx);
    if (p == NULL) {
        return 0;
    }
    c = ModelChunk_Get(p, 0x50000, 0);
    if (c == NULL) {
        return 0;
    }
    n = *(u32 *)(c + 4);
    src = (s32 *)(c + 0xC);
    for (i = 0; i < n; i++) {
        out[i] = *src++;
    }
    return 1;
}

static u8 *ModelFile_Header(u8 *h)
{
    return h;
}

static s32 ModelChunk_Check(u8 *h, s32 type)
{
    u32 i = 0;
    u32 n = *(u32 *)(h + 4);

    for (h += 0xC; i < n; i++) {
        if (*(s32 *)h == type) {
            return 0;
        }
        h += *(u32 *)(h + 8);
    }
    return -1;
}

/* The idx-th chunk of a type, or the idx-th chunk of any type for -1. */
static u8 *ModelChunk_Get(u8 *h, s32 type, s32 idx)
{
    s32 i;
    s32 n;

    if (type != -1) {
        n = *(s32 *)(h + 4);
        i = 0;
        for (h += 0xC; i < n; i++) {
            if (*(s32 *)h == type) {
                if (idx == 0) {
                    return h;
                }
                idx--;
            }
            h += *(u32 *)(h + 8);
        }
    } else if ((u32)idx < *(u32 *)(h + 4)) {
        h += 0xC;
        for (i = 0; i < idx; i++) {
            h += *(u32 *)(h + 8);
        }
        return h;
    }
    return NULL;
}

static s32 ModelChunk5_Count(u8 *h)
{
    u8 *c = ModelChunk_Get(h, 5, 0);

    return c == NULL ? -1 : *(s32 *)(c + 4);
}

/* The word at +0 of the i-th entry inside h's chunk 5, or -1. */
static s32 ModelChunk5_Word0(u8 *h, s32 i)
{
    u8 *c = ModelChunk_Get(h, 5, 0);
    s32 k;

    if (c == NULL) {
        return -1;
    }
    if (*(u32 *)(c + 4) <= (u32)i) {
        return -1;
    }
    c += 0xC;
    for (k = 0; k < i; k++) {
        c += *(u32 *)(c + 8);
    }
    return *(s32 *)c;
}

static s32 ModelChunk5_ItemCount(u8 *h, s32 i)
{
    u8 *c = ModelChunk_Get(h, 5, 0);
    s32 k;

    if (c == NULL) {
        return -1;
    }
    if (*(u32 *)(c + 4) <= (u32)i) {
        return -1;
    }
    c += 0xC;
    for (k = 0; k < i; k++) {
        c += *(u32 *)(c + 8);
    }
    return *(s32 *)(c + 4);
}

s32 ModelChunk5_TotalItems(u8 *h);

/* The sum of the words at +4 of the entries of h chunk 5 (ModelChunk5_ItemCount), or -1. */
s32 ModelChunk5_TotalItems(u8 *h)
{
    u32 n;
    u32 i;
    s32 sum;

    sum = 0;
    if ((s32)(n = ModelChunk5_Count(h)) == -1) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        sum += ModelChunk5_ItemCount(h, i);
    }
    return sum;
}

s32 ModelChunk7_Count(u8 *h)
{
    u8 *c = ModelChunk_Get(h, 0x70000, 0);

    return c == NULL ? -1 : *(s32 *)(c + 4);
}

/*
 * The chunk readers (ModelChunk6_EntryFor to ModelChunk6_Varies) keep argument registers
 * across calls to ModelChunk_Get (and ModelChunk_Check, ModelFile_Header,
 * ModelChunk5_Count): the chunk finders above are static.
 */
typedef struct ChunkItem {
    u32 n : 31;                 /* the words that follow */
    u32 flag : 1;
} ChunkItem;
typedef struct ChunkRec8 { s32 a, b; } ChunkRec8;
typedef struct ChunkRec12 { s32 a, b, c; } ChunkRec12;
typedef struct ChunkRec16 { s32 a, b, c, d; } ChunkRec16;

static s32 ModelChunk6_EntryFor(u8 *h, s32 i, s32 j);
s32 Mesh_Build(MeshDesc *a, u8 *h);
/* The mesh built by Mesh_BuildRigid from the chunks of h. */
/* main's allocator hook gSysAllocHook (SysHeap_Alloc, set in display.c): not a symbol main exports. */
#define gSysAllocHook (*(void *(**)(s32 size))0x2491E0)
s32 Mesh_BuildRigid(MeshDesc *a, u8 *h);
s32 Mesh_BuildSkinned(MeshDesc *a, u8 *h);

/* Entry j of record i of h's chunk 5: word (the counts of records 0..i-1, plus j) of
 * chunk 0x60000, or -1. */
static s32 ModelChunk6_EntryFor(u8 *h, s32 i, s32 j)
{
    s32 sum = 0;
    u8 *c;
    u8 *p;
    s32 k;
    s32 r;

    if (i > ModelChunk5_Count(h)) {
        return -1;
    }
    c = ModelChunk_Get(h, 5, 0);
    if (c == NULL) {
        return -1;
    }
    p = c + 0xC;
    for (k = 0; k < i; k++) {
        sum += *(s32 *)(p + 4);
        p += *(s32 *)(p + 8);
    }
    sum += j;
    c = ModelChunk_Get(h, 0x60000, 0);
    if (c == NULL) {
        r = -1;
    } else {
        r = ((s32 *)(c + 0xC))[sum];
    }
    return r;
}

/* The size (31 bits) of item j of record i of h's chunk 5 (items: a header word, then
 * that many words), or -1. */
static s32 ModelChunk5_ItemSize(u8 *h, s32 i, s32 j)
{
    u8 *c;
    u8 *p;
    s32 k;
    u32 n;

    if (i > ModelChunk5_Count(h)) {
        return -1;
    }
    c = ModelChunk_Get(h, 5, 0);
    if (c == NULL) {
        return -1;
    }
    p = c + 0xC;
    for (k = 0; k < i; k++) {
        p += *(s32 *)(p + 8);
    }
    c = p + 0xC;
    for (k = 0; k <= j; k++) {
        n = ((ChunkItem *)c)->n;
        c += n * 4 + 4;
    }
    return n;
}

/* The flag (bit 31) of item j of record i of h's chunk 5, or -1. */
static s32 ModelChunk5_ItemFlag(u8 *h, s32 i, s32 j)
{
    u8 *c;
    u8 *p;
    s32 k;
    u32 f;

    if (i > ModelChunk5_Count(h)) {
        return -1;
    }
    c = ModelChunk_Get(h, 5, 0);
    if (c == NULL) {
        return -1;
    }
    p = c + 0xC;
    for (k = 0; k < i; k++) {
        p += *(s32 *)(p + 8);
    }
    c = p + 0xC;
    for (k = 0; k <= j; k++) {
        f = *(u32 *)c & 0x80000000;
        c += ((ChunkItem *)c)->n * 4 + 4;
    }
    return f != 0;
}

/* Word k of item j of record i of h's chunk 5, or -1. */
static s32 ModelChunk5_ItemWord(u8 *h, s32 i, s32 j, s32 k)
{
    u8 *c;
    u8 *p;
    s32 m;

    if (i > ModelChunk5_Count(h)) {
        return -1;
    }
    c = ModelChunk_Get(h, 5, 0);
    if (c == NULL) {
        return -1;
    }
    p = c + 0xC;
    for (m = 0; m < i; m++) {
        p += *(s32 *)(p + 8);
    }
    c = p + 0xC;
    for (m = 0; m < j; m++) {
        c += ((ChunkItem *)c)->n * 4 + 4;
    }
    if (k >= (s32)((ChunkItem *)c)->n) {
        return -1;
    }
    return ((s32 *)(c + 4))[k];
}

/* Record idx (12 bytes) of h's chunk 0x70000, or NULL. */
static ChunkRec12 *ModelChunk7_Rec(u8 *h, s32 idx)
{
    u8 *c = ModelChunk_Get(h, 0x70000, 0);

    if (c == NULL) {
        return NULL;
    }
    return (ChunkRec12 *)(c + 0xC) + idx;
}

/* Record idx (12 bytes) of h's chunk 0x80000, or NULL. */
static ChunkRec12 *ModelChunk8_Rec(u8 *h, s32 idx)
{
    u8 *c = ModelChunk_Get(h, 0x80000, 0);

    if (c == NULL) {
        return NULL;
    }
    return (ChunkRec12 *)(c + 0xC) + idx;
}

/* Record idx (8 bytes) of h's chunk 0xA0000, or NULL. */
static ChunkRec8 *ModelChunkA_Rec(u8 *h, s32 idx)
{
    u8 *c = ModelChunk_Get(h, 0xA0000, 0);

    if (c == NULL) {
        return NULL;
    }
    return (ChunkRec8 *)(c + 0xC) + idx;
}

/* Record idx (16 bytes) of h's chunk 0xB0000, or NULL. */
static ChunkRec16 *ModelChunkB_Rec(u8 *h, s32 idx)
{
    u8 *c = ModelChunk_Get(h, 0xB0000, 0);

    if (c == NULL) {
        return NULL;
    }
    return (ChunkRec16 *)(c + 0xC) + idx;
}

/* The count of list idx of h's chunk 0xC0000 (lists of 8-byte records after their
 * count), or -1. */
static s32 ModelChunkC_ListCount(u8 *h, s32 idx)
{
    u8 *c;
    u8 *p;
    s32 i;
    s32 n;

    c = ModelChunk_Get(h, 0xC0000, 0);
    if (c == NULL) {
        return -1;
    }
    p = c + 0xC;
    for (i = 0; i < idx; i++) {
        n = *(s32 *)p;
        p += 4;
        p += n * 8;
    }
    return *(s32 *)p;
}

/* Record j of list i of h's chunk 0xC0000, or NULL. */
static ChunkRec8 *ModelChunkC_Rec(u8 *h, s32 i, s32 j)
{
    u8 *p;
    s32 k;
    s32 n;

    p = ModelChunk_Get(h, 0xC0000, 0);
    if (p == NULL) {
        return NULL;
    }
    p += 0xC;
    for (k = 0; k < i; k++) {
        n = *(s32 *)p;
        p += 4;
        p += n * 8;
    }
    if (j >= *(s32 *)p) {
        p = NULL;
    } else {
        p += 4;
        p += j * 8;
    }
    return (ChunkRec8 *)p;
}

/* Sets up a (its s16 at +0x20 from ModelFile_PartMatCount) from part idx of file h when the part
 * is of kind 4 (Mesh_Build); 0 for the other kinds or a bad index. */
s32 ModelFile_LoadMesh(MeshDesc *a, u8 *h, u32 idx)
{
    u8 *c;
    u8 *rec;

    ModelFile_Header(h);
    c = ModelChunk_Get(h, 2, 0);
    if (c == NULL) {
        return 0;
    }
    if (*(u32 *)(c + 4) <= idx) {
        return 0;
    }
    if ((rec = ModelFile_GetPart(h, idx)) == NULL) {
        return 0;
    }
    switch (*(s32 *)rec) {
    case 3:
    case 6:
    case 7:
    case 8:
        return 0;
    case 4:
        a->matCount = ModelFile_PartMatCount(h, idx);
        return Mesh_Build(a, rec);
    }
    return 0;
}

/* Mesh_BuildSkinned when h has a chunk 0xC0000, else Mesh_BuildRigid. */
s32 Mesh_Build(MeshDesc *a, u8 *h)
{
    if (ModelChunk_Check(h, 0xC0000) != 0) {
        return Mesh_BuildRigid(a, h);
    }
    return Mesh_BuildSkinned(a, h);
}

/* 1 when some entry (ModelChunk6_EntryFor) differs from entry (0, 0), else 0. */
s32 ModelChunk6_Varies(u8 *h)
{
    s32 i;
    s32 j;
    s32 n;
    s32 m;
    s32 first;

    first = ModelChunk6_EntryFor(h, 0, 0);
    n = ModelChunk5_Count(h);
    for (i = 0; i < n; i++) {
        m = ModelChunk5_ItemCount(h, i);
        for (j = 0; j < m; j++) {
            if (ModelChunk6_EntryFor(h, i, j) != first) {
                return 1;
            }
        }
    }
    return 0;
}

/* Builds the mesh a from h: the index lists (each entry count | 0x8000 or 0x4000, the entry of
 * ModelChunk6_EntryFor when they differ, then the entries) and the vertices (position, normal, colour
 * from the four floats of chunk 0xB0000, texture coordinates when h has chunk 0xA0000). 0 when
 * an allocation fails. The chunk readers it calls are static: it keeps t registers across them. */
s32 Mesh_BuildRigid(MeshDesc *a, u8 *h)
{
    s32 t;
    s32 size;
    s32 n;
    s32 i;
    s32 j;
    u16 *p;
    s32 m;
    s32 cnt;
    s32 sum;
    s32 uv;
    s32 k;
    u8 *v;
    f32 *r;

    uv = 0;
    if (ModelChunk_Check(h, 0xA0000) == 0) {
        uv = 1;
    }
    a->flags = 0x21000;
    n = ModelChunk5_Count(h);
    a->count = ModelChunk5_TotalItems(h);
    if (ModelChunk6_Varies(h) == 1) {
        a->flags |= 0x100000;
    }
    sum = 0;
    for (i = 0; i < n; i++) {
        m = ModelChunk5_ItemCount(h, i);
        for (j = 0; j < m; j++) {
            sum += ModelChunk5_ItemSize(h, i, j);
        }
    }
    size = (a->count + sum) * 2;
    if (a->flags & 0x100000) {
        size += a->count * 2;
    }
    if ((a->strips = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    a->stripsSize = size;
    p = a->strips;
    for (i = 0; i < n; i++) {
        m = ModelChunk5_ItemCount(h, i);
        for (j = 0; j < m; j++) {
            cnt = ModelChunk5_ItemSize(h, i, j);
            *p = cnt;
            if (ModelChunk5_ItemFlag(h, i, j) != 0) {
                *p |= 0x8000;
            } else {
                *p |= 0x4000;
            }
            p++;
            if (a->flags & 0x100000) {
                *p++ = ModelChunk6_EntryFor(h, i, j);
            }
            if (a->flags & 0x1000) {
                for (k = 0; k < cnt; k++) {
                    *p++ = ModelChunk5_ItemWord(h, i, j, k);
                }
            }
        }
    }
    a->format = 0x25;
    if (uv) {
        a->format |= 0x10;
    }
    a->vertCount = ModelChunk7_Count(h);
    size = a->vertCount * 0x24;
    if ((a->verts = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    a->vertsSize = size;
    v = a->verts;
    for (t = 0; t < a->vertCount; t++) {
        if (uv) {
            r = (f32 *)ModelChunk7_Rec(h, t);
            ((f32 *)v)[0] = r[0];
            ((f32 *)v)[1] = r[1];
            ((f32 *)v)[2] = r[2];
            r = (f32 *)ModelChunk8_Rec(h, t);
            ((f32 *)v)[3] = r[0];
            ((f32 *)v)[4] = r[1];
            ((f32 *)v)[5] = r[2];
            r = (f32 *)ModelChunkA_Rec(h, t);
            ((f32 *)v)[7] = r[0];
            ((f32 *)v)[8] = r[1];
            r = (f32 *)ModelChunkB_Rec(h, t);
            ((u32 *)v)[6] = (u32)r[3] << 24 | (u32)r[0] << 16 | (u32)r[1] << 8 | (u32)r[2];
            v += 0x24;
        } else {
            r = (f32 *)ModelChunk7_Rec(h, t);
            ((f32 *)v)[0] = r[0];
            ((f32 *)v)[1] = r[1];
            ((f32 *)v)[2] = r[2];
            r = (f32 *)ModelChunk8_Rec(h, t);
            ((f32 *)v)[3] = r[0];
            ((f32 *)v)[4] = r[1];
            ((f32 *)v)[5] = r[2];
            r = (f32 *)ModelChunkB_Rec(h, t);
            ((u32 *)v)[6] = (u32)r[3] << 24 | (u32)r[0] << 16 | (u32)r[1] << 8 | (u32)r[2];
            v += 0x1C;
        }
    }
    return 1;
}

/* Mesh_BuildRigid for skinned meshes (flags 0x200000: a word per list from ModelChunk5_Word0's kind;
 * vertices of 0x2C bytes with four bone indices at +0x24 and four weights at +0x28, the largest
 * weight taking the remainder of 0xFF, empty slots moved down). One byte cursor p writes the index
 * list and then the vertices, and j counts the weight records too: their vertex-loop webs are
 * later webs, numbered after the loop's created values, so p takes t3 and j t4 as in the game;
 * the record count nw is a local of its own (a first web, numbered before best: s5). */
s32 Mesh_BuildSkinned(MeshDesc *a, u8 *h)
{
    s32 i;
    s32 j;
    s32 k;
    s32 m;
    s32 nw;
    u8 *p;
    s32 kind;
    s32 sum;
    s32 size;
    s32 best;
    s32 cnt;
    s32 w[4];
    f32 *r;
    u8 *e;
    s32 b[4];
    s32 n;

    ModelChunk_Check(h, 0xA0000);
    a->flags = 0x221000;
    n = ModelChunk5_Count(h);
    a->count = ModelChunk5_TotalItems(h);
    if (ModelChunk6_Varies(h) == 1) {
        a->flags |= 0x100000;
    }
    sum = 0;
    for (i = 0; i < n; i++) {
        m = ModelChunk5_ItemCount(h, i);
        for (j = 0; j < m; j++) {
            sum += ModelChunk5_ItemSize(h, i, j);
        }
    }
    size = (a->count + sum) * 2;
    if (a->flags & 0x100000) {
        size += a->count * 2;
    }
    if (a->flags & 0x200000) {
        size += a->count * 2;
    }
    if ((a->strips = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    a->stripsSize = size;
    p = (u8 *)a->strips;
    for (i = 0; i < n; i++) {
        m = ModelChunk5_ItemCount(h, i);
        kind = ModelChunk5_Word0(h, i);
        for (j = 0; j < m; j++) {
            cnt = ModelChunk5_ItemSize(h, i, j);
            *(u16 *)p = cnt;
            if (ModelChunk5_ItemFlag(h, i, j) != 0) {
                *(u16 *)p |= 0x8000;
            } else {
                *(u16 *)p |= 0x4000;
            }
            p += 2;
            if (a->flags & 0x100000) {
                *(u16 *)p = ModelChunk6_EntryFor(h, i, j);
                p += 2;
            }
            if (a->flags & 0x200000) {
                switch (kind) {
                case 0x30000:
                    *(u16 *)p = 0;
                    break;
                case 0x40000:
                    *(u16 *)p = 1;
                    break;
                }
                p += 2;
            }
            if (a->flags & 0x1000) {
                for (k = 0; k < cnt; k++) {
                    *(u16 *)p = ModelChunk5_ItemWord(h, i, j, k);
                    p += 2;
                }
            }
        }
    }
    a->format = 0x35;
    a->format |= 0x100;
    a->vertCount = ModelChunk7_Count(h);
    size = a->vertCount * 0x2C;
    if ((a->verts = gSysAllocHook(size)) == NULL) {
        return 0;
    }
    a->vertsSize = size;
    p = a->verts;
    for (i = 0; i < a->vertCount; i++) {
        r = (f32 *)ModelChunk7_Rec(h, i);
        ((f32 *)p)[0] = r[0];
        ((f32 *)p)[1] = r[1];
        ((f32 *)p)[2] = r[2];
        r = (f32 *)ModelChunk8_Rec(h, i);
        ((f32 *)p)[3] = r[0];
        ((f32 *)p)[4] = r[1];
        ((f32 *)p)[5] = r[2];
        r = (f32 *)ModelChunkA_Rec(h, i);
        ((f32 *)p)[7] = r[0];
        ((f32 *)p)[8] = r[1];
        r = (f32 *)ModelChunkB_Rec(h, i);
        ((u32 *)p)[6] = (u32)r[3] << 24 | (u32)r[0] << 16 | (u32)r[1] << 8 | (u32)r[2];
        ((u32 *)p)[9] = 0;
        ((u32 *)p)[10] = 0;
        nw = ModelChunkC_ListCount(h, i);
        w[0] = 0;
        w[1] = 0;
        w[2] = 0;
        w[3] = 0;
        best = 0;
        b[0] = 0;
        b[1] = 0;
        b[2] = 0;
        b[3] = 0;
        for (j = 0; j < nw; j++) {
            e = (u8 *)ModelChunkC_Rec(h, i, j);
            w[j] = (s32)(255.0f * *(f32 *)(e + 4) / 100.0f) & 0xFF;
            if (0 < w[j]) {
                best = j;
            }
            b[j] = *e;
        }
        w[best] += 0xFF - w[0] - w[1] - w[2] - w[3];
        for (j = 0; j < nw - 1; j++) {
            if (w[j] == 0) {
                w[j] = w[j + 1];
                b[j] = b[j + 1];
                w[j + 1] = 0;
            }
        }
        ((u32 *)p)[10] = w[0] << 24 | w[1] << 16 | w[2] << 8 | w[3];
        ((u32 *)p)[9] = b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3];
        p += 0x2C;
    }
    return 1;
}

u8 * Bmp_GetPixels(u8 *bmp)
{
    if (*(u16 *)((u8 *)bmp + 0) != 0x4D42) {
        Debug_Sprintf("This is not BMP image. @plCreateContextFromBMPImage");
        return NULL;
    }
    return bmp + (*(u16 *)((u8 *)bmp + 0xA) | (*(u16 *)((u8 *)bmp + 0xC) << 0x10));
}

void Anim_SetKeyBase(void *buf)
{
    gAnimKeyBase = (s32) buf;
}

/* The cubic Hermite curve through (t0, p0) and (t1, p1) with the tangents m0 and m1, at t. */
f32 Math_Hermite(f32 t, f32 p0, f32 t0, f32 m0, f32 p1, f32 t1, f32 m1)
{
    f32 u, r, c, r2, ur, u2, d, a, b;
    f32 h00, h10, h01, h11;

    u = t - t0;
    r = 1.0f / (t1 - t0);
    u2 = u * u;
    r2 = r * r;
    ur = u2 * r;
    c = r2 * (u2 * u);
    d = 2.0f * c * r;
    a = 3.0f * u2 * r2;
    b = ur;
    h00 = 1.0f + (d - a);
    h10 = u + (c - b - b);
    h01 = -d + a;
    h11 = c - b;
    return p0 * h00 + p1 * h01 + m0 * h10 + m1 * h11;
}

/* The time of curve c's first key (0 for the constant formats 0x10 and 0x20). */
f32 AnimCurve_StartTime(AnimCurve *c)
{
    u8 *keys = (u8 *)(c->keys + gAnimKeyBase);
    f32 t;

    switch (c->type) {
    case 0x20:
        t = 0.0f;
        break;
    case 0x21:
        t = ((KeyF2 *)keys)->t;
        break;
    case 0x22:
        t = ((KeyF4 *)keys)->t;
        break;
    case 0x23:
        t = ((KeyFM *)keys)->t;
        break;
    case 0x10:
        t = 0.0f;
        break;
    case 0x11:
        t = ((KeyS2 *)keys)->t;
        break;
    case 0x12:
        t = ((KeyS4 *)keys)->t;
        break;
    case 0x13:
        t = ((KeySM *)keys)->t;
        break;
    }
    return t;
}

/* The time of curve c's last key. */
f32 AnimCurve_GetEndTime(AnimCurve *c)
{
    return AnimCurve_EndTime(c);
}

/* The time of curve c's last key (0 for the constant formats 0x10 and 0x20). */
f32 AnimCurve_EndTime(AnimCurve *c)
{
    u8 *keys = (u8 *)(c->keys + gAnimKeyBase);
    f32 t;

    switch (c->type) {
    case 0x20:
        t = 0.0f;
        break;
    case 0x21:
        t = ((KeyF2 *)keys)[c->count - 1].t;
        break;
    case 0x22:
        t = ((KeyF4 *)keys)[c->count - 1].t;
        break;
    case 0x23:
        t = ((KeyFM *)keys)[c->count - 1].t;
        break;
    case 0x10:
        t = 0.0f;
        break;
    case 0x11:
        t = ((KeyS2 *)keys)[c->count - 1].t;
        break;
    case 0x12:
        t = ((KeyS4 *)keys)[c->count - 1].t;
        break;
    case 0x13:
        t = ((KeySM *)keys)[c->count - 1].t;
        break;
    }
    return t;
}

void Mtx_Copy3x3(f32 *m, f32 *a)
{
    m[0] = a[0];
    m[1] = a[1];
    m[2] = a[2];
    m[4] = a[4];
    m[5] = a[5];
    m[6] = a[6];
    m[8] = a[8];
    m[9] = a[9];
    m[10] = a[10];
}

f32 sqrtf(f32 x);

/* |v| (the length of a 3-vector). */
f32 Vec_Length2(f32 *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void Vec_Normalize2(f32 *v)
{
    f32 len;

    len = Vec_Length2(v);
    if (len == 0.0f) {
        v[2] = 0.0f;
        v[1] = 0.0f;
        v[0] = 0.0f;
        return;
    }
    v[0] /= len;
    v[1] = v[1] / len;
    v[2] = v[2] / len;
}

/* a . b (as Vec_Dot). */
f32 Vec_Dot2(f32 *a, f32 *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/* dst = a x b. */
void Vec_Cross2(f32 *dst, f32 *a, f32 *b)
{
    dst[0] = a[1] * b[2] - b[1] * a[2];
    dst[1] = a[2] * b[0] - b[2] * a[0];
    dst[2] = a[0] * b[1] - b[0] * a[1];
}

/* The plane through a, b and c: the unit normal of (b - a) x (c - a), and in [3] minus its
 * dot product with b - a (sic: 0 for a true normal; a plane would use a). */
void Plane_FromPoints(f32 *plane, f32 *a, f32 *b, f32 *c)
{
    f32 ab[3];
    f32 ac[3];
    f32 n[3];

    ab[0] = b[0] - a[0];
    ab[1] = b[1] - a[1];
    ab[2] = b[2] - a[2];
    ac[0] = c[0] - a[0];
    ac[1] = c[1] - a[1];
    ac[2] = c[2] - a[2];
    Vec_Cross2(n, ab, ac);
    Vec_Normalize2(n);
    plane[0] = n[0];
    plane[1] = n[1];
    plane[2] = n[2];
    plane[3] = -Vec_Dot2(n, ab);
}

f64 fptodp(f32 x);          /* float to double (main) */
f64 tan(f64 x);          /* tan, probably (main) */
f32 dptofp(f64 x);

/* (f32)tan(x), with main's double helpers named as they are. */
#define TAN(x) dptofp(tan(fptodp(x)))

/* The four side planes of the view frustum (fov, aspect) through the eye, and in [16], [17]
 * minus near and minus far. */
void Frustum_BuildPlanes(f32 *planes, f32 fov, f32 aspect, f32 near, f32 far)
{
    f32 o[4];
    f32 a[4];
    f32 b[4];
    f32 half;
    f32 nf;

    half = 0.5f * fov;
    nf = -far;
    o[0] = 0.0f;
    o[1] = 0.0f;
    o[2] = 0.0f;
    b[2] = nf;
    a[2] = nf;
    a[0] = far * (TAN(half) / aspect);
    a[1] = nf * TAN(half);
    b[0] = a[0];
    b[1] = -a[1];
    Plane_FromPoints(planes, o, a, b);
    a[0] = nf * (TAN(half) / aspect);
    a[1] = far * TAN(half);
    b[0] = a[0];
    b[1] = -a[1];
    Plane_FromPoints(planes + 4, o, a, b);
    a[0] = far * (TAN(half) / aspect);
    a[1] = far * TAN(half);
    b[0] = -a[0];
    b[1] = a[1];
    Plane_FromPoints(planes + 8, o, a, b);
    a[0] = nf * (TAN(half) / aspect);
    a[1] = nf * TAN(half);
    b[0] = -a[0];
    b[1] = a[1];
    Plane_FromPoints(planes + 12, o, a, b);
    planes[17] = nf;
    planes[16] = -near;
}

/* Links the n words at p into a free list: each holds the address of the next, the last 0. */
void FreeList_Init(s32 n, u32 *p)
{
    s32 i;

    for (i = 0; i < n - 1; i++) {
        p[i] = (u32)&p[i + 1];
    }
    p[i] = 0;
}

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
    u32 v;

    for (;;) {
        if (*head == 0) {
            return NULL;
        }
        v = *(u32 *)*head;
        if (v & 1) {
            return (void *)(*head = v & ~1);
        }
        *head = v;
    }
}

extern s8 gUsbKbStep;
extern s32 (*jtbl_00403140[])(s8 *step);

/* Runs step gUsbKbStep of the step table jtbl_00403140 (function pointers, each given the
 * step counter to advance, as UsbKb_StepNext does). */
s32 UsbKb_RunInitStep(void)
{
    return jtbl_00403140[gUsbKbStep](&gUsbKbStep);
}

/* `(*step)++; return 1;`, a function of its own: jtbl_00403140 is a table of function
 * pointers that UsbKb_RunInitStep tail-calls through, and splat had merged this, its
 * first entry, into it (lead, after round 9). */
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
extern u8 gUsbKbInfo[];
extern s32 D_00581A64;

/* The USB keyboard set-up steps (libusbkb in main; step: the caller's state counter,
 * advanced on success, reset on failure). Once (gUsbKbLibInit): sceUsbKbInit until its status is set. */
s32 UsbKb_StepInit(s8 *step)
{
    s32 st;

    if (gUsbKbLibInit == 0) {
        if (sceUsbKbInit(&st) != 0) {
            *step = 0;
            return 0;
        }
        if (st == 0) {
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

/* sceUsbKbGetInfo on gUsbKbInfo, then sceUsbKbSync; advances when D_00581A64 is set. */
s32 UsbKb_StepWaitConnect(s8 *step)
{
    s32 st;

    st = sceUsbKbGetInfo(gUsbKbInfo);
    if (st != 0) {
        *step = 0;
        return 0;
    }
    gUsbKbSyncResult = sceUsbKbSync(0, &st);
    if (gUsbKbSyncResult == 0 && st == 0) {
        if (D_00581A64 == 0) {
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
extern u8 D_00581A68[];

s32 UsbKb_StepSetup(s8 *step)
{
    s32 temp_s2;
    s32 temp_s1;
    s32 temp_s0;
    s32 temp_v0;

    if (*D_00581A68 == 0) {
        *step = 0;
        return 0;
    }
    temp_s2 = sceUsbKbSetRepeat(0, 0, 0);
    temp_s1 = sceUsbKbSetCodeType(0, 0U);
    D_005819C8 = 1;
    temp_s0 = sceUsbKbSetArrangement(0, 1U);
    D_005819C0 = 0;
    temp_v0 = sceUsbKbSetReadMode(0, 0U);
    if ((temp_s2 == 0) && (temp_s1 == 0) && (temp_s0 == 0) && (temp_v0 == 0)) {
        *step += 1;
    }
    return 1;
}

s32 sceUsbKbSetLEDMode(s32 a, s32 b);

s32 UsbKb_StepSync(s8 *p)
{
    s32 st;

    st = sceUsbKbSetLEDMode(0, 0);
    if (st == 0) {
        gUsbKbSyncResult = sceUsbKbSync(0, &st);
        if (gUsbKbSyncResult == 0 && st == 0) {
            (*p)++;
        }
    }
    return 1;
}

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
/* The keyboard data libusbkb returns (sceUsbKbRead), as its led, mkey, len, keycode[]. */
typedef struct UsbKbData {
    s32 led;                    /* 0x00 */
    s32 mkey;                   /* 0x04: the modifier keys */
    s32 len;                    /* 0x08: key codes */
    u16 keycode[7];             /* 0x0C */
} UsbKbData;
extern UsbKbData gUsbKbData;
extern u8 D_005819A0[];
extern u8 gUsbKbReport[9];
extern u8 *D_005819B0;          /* the game stores a pointer here, then byte 8 of gUsbKbReport */

/*
 * The USB keyboard poll: runs step gUsbKbStep of the set-up table at gUsbKbSetupSteps until
 * gUsbKbReady is set; then reads keyboard 0 (sceUsbKbRead) into gUsbKbData and, when it
 * has key codes, returns them as a 9-byte report in gUsbKbReport (else NULL).
 */
u8 *UsbKb_Poll(void)
{
    s32 st;

    memset(gKbdSettings, 0, 0x88);
    if (gUsbKbReady == 0) {
        gUsbKbSetupSteps[gUsbKbStep](&gUsbKbStep);
        return NULL;
    }
    if (UsbKb_IsConnected(0) == -1) {
        return NULL;
    }
    sceUsbKbRead(0, &gUsbKbData);
    memcpy(gKbdSettings, &gUsbKbData, 0x88);
    sceUsbKbSync(0, &st);
    memcpy(gKbdSettings, gKbdSettings, 0x88);
    if (gUsbKbData.len == 0) {
        return NULL;
    }
    gUsbKbReport[0] = gUsbKbData.mkey;
    D_005819B0 = D_005819A0;
    gUsbKbReport[1] = gUsbKbData.led;
    gUsbKbReport[2] = gUsbKbData.keycode[0];
    gUsbKbReport[3] = gUsbKbData.keycode[1];
    gUsbKbReport[4] = gUsbKbData.keycode[2];
    gUsbKbReport[5] = gUsbKbData.keycode[3];
    gUsbKbReport[6] = gUsbKbData.keycode[4];
    gUsbKbReport[7] = gUsbKbData.keycode[5];
    gUsbKbReport[8] = gUsbKbData.keycode[6];
    return gUsbKbReport;
}

/* 0 when the query (sceUsbKbGetInfo, sceUsbKbSync) succeeds and flag i of D_00581A68 is set, else -1. */
s32 UsbKb_IsConnected(s32 i)
{
    s32 st;

    if (sceUsbKbGetInfo(gUsbKbInfo) != 0) {
        return -1;
    }
    sceUsbKbSync(0, &st);
    if (st != 0) {
        return -1;
    }
    if (D_00581A68[i] == 0) {
        return -1;
    }
    return 0;
}

/* unk_2F6540's .data (0x403140-0x403170) */
s32 (*jtbl_00403140[8])(s8 *step) = {
    UsbKb_StepNext, UsbKb_StepInit, UsbKb_StepWaitConnect, UsbKb_StepSync,
    UsbKb_StepSetup, UsbKb_StepDone, NULL, NULL,
};
s32 (*gUsbKbSetupSteps[4])(s8 *step) = {
    UsbKb_StepWaitConnect, UsbKb_StepSync, UsbKb_StepSetup, UsbKb_StepDone,
};
