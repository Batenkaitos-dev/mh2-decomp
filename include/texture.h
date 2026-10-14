#ifndef TEXTURE_H
#define TEXTURE_H

/*
 * The core's texture and palette handles (src/main/texture.c, texconv.c,
 * unk_17EB00.c): Capcom's fl layer (flCreatePaletteHandle and
 * flPS2ConvertTextureFromContext are named in its error strings).
 *
 * A texture handle is index + 1 of gTexRecs, a palette handle
 * (index + 1) << 16 of gPalRecs; the two can be or-ed into one value.
 * The pixels live in an EE heap block (mem) and are sent to GS memory
 * (vram, in 256-byte blocks) when the handle is created or unlocked.
 */
#include "common.h"
#include "image.h"

/* GS pixel storage modes */
#define GS_PSMCT32 0x00
#define GS_PSMCT24 0x01
#define GS_PSMCT16 0x02
#define GS_PSMT8 0x13
#define GS_PSMT4 0x14

#define TEX_MAX 256

typedef struct TexRec {
    /* 0x00 */ s32 mode;        /* 1..5 (1, 4: not sent to GS memory) */
    /* 0x04 */ s32 flags;       /* Image.flags of the source */
    /* 0x08 */ u32 size;        /* bytes, all mip levels */
    /* 0x0C */ s32 fmt;         /* Image.bpp code: 0 4-bit ... 4 32-bit */
    /* 0x10 */ s16 blocks;      /* GS memory needed, 256-byte blocks */
    /* 0x12 */ s16 align;       /* of the GS address: 0x20 (a page) textures, 1 palettes */
    /* 0x14 */ s16 vram;        /* GS address, 256-byte blocks */
    /* 0x16 */ s16 tw;          /* log2 of the width */
    /* 0x18 */ s16 th;          /* log2 of the height */
    /* 0x1A */ s16 width;
    /* 0x1C */ s16 height;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ u32 psm;         /* GS_PSM* */
    /* 0x24 */ void *mem;       /* heap handle of the pixels (Mem_Alloc), NULL: none */
    /* 0x28 */ s16 xferW;           /* the GS upload width (Dma_BuildLoadImage: buffer width xferW / 64) */
    /* 0x2A */ s16 xferH;           /* the GS upload height */
    /* 0x2C */ u8 *locked;      /* pixels handed out by the lock */
    /* 0x30 */ u32 levels;      /* mip levels */
    /* 0x34 */ s8 used;
    /* 0x35 */ s8 resident;     /* has a VramSlot */
    /* 0x36 */ s8 unk36;
    /* 0x37 */ u8 lockFlag;
} TexRec; /* size 0x38 */
typedef char TexRec_size_check[(sizeof(TexRec) == 0x38) ? 1 : -1];

/* A block of GS memory for textures (4 at most). */
typedef struct VramBlock {
    s16 used;
    s16 addr;                   /* 256-byte blocks */
    s32 size;
} VramBlock;

/* A texture's place in GS memory: 512 of them at 0x2151A0, in a list
 * ordered by address (gVramSlotHead). */
typedef struct VramSlot {
    /* 0x00 */ struct VramSlot *prev;
    /* 0x04 */ struct VramSlot *next;
    /* 0x08 */ TexRec *tex;
    /* 0x0C */ s32 flags;       /* of tex */
    /* 0x10 */ s32 mode;        /* of tex */
    /* 0x14 */ s16 addr;        /* 256-byte blocks; 0: free slot */
    /* 0x16 */ s16 blocks;
    /* 0x18 */ s16 align;
} VramSlot; /* size 0x1C */

#define VRAM_SLOT_MAX 512

extern TexRec gTexRecs[TEX_MAX];
extern TexRec gPalRecs[TEX_MAX];
extern s32 gTexCount;
extern s32 gPalCount;
extern s32 gVramBlockCount;
extern VramBlock gVramBlocks[4];
extern VramSlot gVramSlots[VRAM_SLOT_MAX];
extern s32 gVramSlotCount;
extern VramSlot *gVramSlotHead;

u16 Vram_AllocBlock(u32 size);
void Vram_FreeBlock(u32 addr);
u32 flCreateTextureHandle(Image *img, s32 mode);
s32 Tex_SetupRec(Image *img, u32 levels, u32 handle, s32 mode);
s32 Tex_Upload(u32 handle, s32 mode);
void Tex_SendLevels(TexRec *t);
s32 Tex_LevelVram(TexRec *t, u32 level);
u32 Tex_FindFree(void);
u32 flCreatePaletteHandle(Image *img, s32 mode);
s32 Pal_SetupRec(Image *img, u32 handle, s32 mode);
s32 Pal_Upload(u32 handle, s32 mode);
u32 Pal_FindFree(void);
s32 flReleaseTextureHandle(u32 handle);
s32 Tex_ReleaseKeep(u32 handle);
s32 flReleasePaletteHandle(u32 handle);
s32 Pal_ReleaseKeep(u32 handle);
s32 flLockTexture(void *rect, u32 handle, Image *img, s32 flag);
s32 flLockPalette(void *rect, u32 handle, Image *img, s32 flag);
s32 Tex_Lock(void *rect, TexRec *t, Image *img, s32 flag, s32 pal);
s32 flUnlockTexture(u32 handle);
s32 flUnlockPalette(u32 handle);
s32 Tex_Unlock(TexRec *t);

/* texconv.c */
s32 Tex_MakeResident(s32 n, u32 *handles);
s16 Tex_Log2(s16 size);
s32 Tex_Size(u32 psm, s32 w, s32 h, s32 levels);
s16 Tex_VramBlocks(TexRec *t);
s16 Pal_VramBlocks(TexRec *t);
s32 flPS2ConvertTextureFromContext(Image *img, TexRec *t, s32 pal);
void Vram_Init(void);
VramSlot *VramSlot_Alloc(VramSlot *table, s32 n);
void VramSlot_Free(VramSlot *s);
void VramSlot_Insert(VramSlot *pos, VramSlot *s);

/* unk_17EB00.c */
VramSlot *VramSlot_Find(TexRec *t);
s32 VramSlot_Assign(VramSlot *after, TexRec *t);
s32 VramSlot_Replace(VramSlot *s, TexRec *t);
s32 VramSlot_FreeAll(void);
s32 Tex_Evict(TexRec *t);
s32 Tex_Reserve(u32 *handles, s32 n);
VramSlot *VramSlot_FindGap(u32 blocks, s32 align);
VramSlot *VramSlot_FindVictim(TexRec *t, u32 *handles, s32 n);
void ScreenTex_Init(void *buf);
s32 ScreenTex_Capture(s32 fromTex);
s32 ScreenTex_Upload(void);
void ScreenTex_Release(void);

/* unk_17F660.c */
void ScreenTex_Send(TexRec *t);
void ScreenTex_SetRegs(void);
s32 ScreenTex_Reserve(u32 *pixels);

#endif /* TEXTURE_H */
