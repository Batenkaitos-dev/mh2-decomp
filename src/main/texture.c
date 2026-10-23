/*
 * Texture and palette handles: GS memory blocks, creating a
 * handle from an Image (the pixels are converted to the GS layout and sent,
 * mip level by mip level), releasing it, locking it for the CPU, and
 * making a set of handles resident in GS memory for a draw (Tex_Use,
 * Tex_MakeResident).
 */
#include "common.h"
#include "dma.h"
#include "texture.h"
#include "heap.h"
#include "gs_setup.h"
#include "mem_util.h"
#include "packet_tim2.h"

/* img: the channels of the texture pixels in the GS order. */
#define TEX_GS_DESC(t, img)                                         \
    switch ((t)->psm) {                                             \
    case GS_PSMT4:                                                  \
        (img)->bpp = 0;                                             \
        (img)->pitch = (img)->width / 2;                            \
        break;                                                      \
    case GS_PSMT8:                                                  \
        (img)->bpp = 1;                                             \
        IMAGE_SET_PITCH(img);                                          \
        break;                                                      \
    case GS_PSMCT16:                                                \
        (img)->bpp = 2;                                             \
        IMAGE_RGBA5551(img);                                        \
        IMAGE_GS_ORDER_16(img);                                          \
        IMAGE_SET_PITCH(img);                                          \
        break;                                                      \
    case GS_PSMCT24:                                                \
        (img)->bpp = 3;                                             \
        IMAGE_RGB888(img);                                          \
        IMAGE_GS_ORDER_32(img);                                          \
        IMAGE_SET_PITCH(img);                                          \
        break;                                                      \
    case GS_PSMCT32:                                                \
        (img)->bpp = 4;                                             \
        IMAGE_RGBA8888(img);                                        \
        IMAGE_GS_ORDER_32(img);                                          \
        IMAGE_SET_PITCH(img);                                          \
        break;                                                      \
    }

extern s32 gSysTexBlock;          /* gSysWork: next free GS block */

u128 *Dma_BuildLoadImage(u128 *pkt, u32 irq, u32 src, u32 size, s16 dbp, s16 dbw, s16 psm, s16 dx,
                         s16 dy, s16 w, s16 h);
extern DmaQueue gDmaQueue;

/* A GS memory block of size bytes (rounded to 8 KB pages); returns its
 * address in 256-byte blocks, 0 when all four are in use. */
u16 Vram_AllocBlock(u32 size)
{
    s32 i;
    u32 blockCount;
    VramBlock *block;

    if (gVramBlockCount >= 4) {
        return 0;
    }
    VramSlot_FreeAll();
    blockCount = (((size + 255) >> 8) + 31) & ~0x1F;
    for (block = gVramBlocks, i = 0; i < 4; i++, block++) {
        if (block->used == 0) {
            break;
        }
    }
    block->used = 1;
    block->addr = gSysTexBlock;
    block->size = blockCount;
    gVramBlockCount++;
    gSysTexBlock += blockCount;
    return block->addr;
}

/* Frees the block at addr (which becomes the next free address). */
void Vram_FreeBlock(u32 addr)
{
    VramBlock *block;
    s32 i;

    for (i = 0, block = gVramBlocks; i < 512; i++, block++) {
        if (block->addr == (u16)addr) {
            break;
        }
    }
    if (i != 4) {
        VramSlot_FreeAll();
        Mem_Fill(block, 0, sizeof(VramBlock));
        gVramBlockCount--;
        gSysTexBlock = (u16)addr;
    }
}

/*
 * Creates a texture handle from img (one mip level) with mode (the
 * TexRec mode: 1 and 4 keep it out of GS memory). Without pixels, GS
 * memory is reserved and an empty pixel block allocated; otherwise the
 * pixels are converted to the GS layout and sent (Tex_Upload). Returns the
 * handle (index + 1 of gTexRecs), 0 when none is free.
 */
u32 flCreateTextureHandle(Image *img, s32 mode)
{
    u32 handle;
    TexRec *texRec;

    handle = Tex_FindFree();
    if (handle == 0) {
        return 0;
    }
    texRec = &gTexRecs[(handle & 0xFFFF) - 1];
    Tex_SetupRec(img, 1, handle, mode);
    if (img->pixels == NULL) {
        Tex_Reserve(&handle, 1);
        texRec->mem = Mem_Alloc(texRec->size, 2);
        gTexCount++;
    } else {
        flPS2ConvertTextureFromContext(img, texRec, 0);
        Tex_Upload(handle, mode);
    }
    return handle;
}

/*
 * Fills the texture record of handle from img: size, format, mode,
 * levels mip levels, log2 sizes, GS memory blocks (page-aligned) and
 * bytes. Returns 1.
 */
s32 Tex_SetupRec(Image *img, u32 levels, u32 handle, s32 mode)
{
    TexRec *texRec = &gTexRecs[(handle & 0xFFFF) - 1];

    texRec->used = 1;
    texRec->mode = mode;
    texRec->flags = img->flags;
    texRec->width = img->width;
    texRec->height = img->height;
    texRec->swizzled = 0;
    texRec->mem = 0;
    texRec->locked = NULL;
    texRec->lockFlag = 0;
    texRec->levels = levels;
    switch (img->bpp) {
    case 0:
        texRec->psm = GS_PSMT4;
        texRec->fmt = 0;
        break;
    case 1:
        texRec->psm = GS_PSMT8;
        texRec->fmt = 1;
        break;
    case 2:
        texRec->psm = GS_PSMCT16;
        texRec->fmt = 2;
        break;
    case 3:
        texRec->psm = GS_PSMCT24;
        texRec->fmt = 3;
        break;
    case 4:
        texRec->psm = GS_PSMCT32;
        texRec->fmt = 4;
        break;
    }
    texRec->tw = Tex_Log2(texRec->width);
    texRec->th = Tex_Log2(texRec->height);
    texRec->blocks = Tex_VramBlocks(texRec);
    texRec->align = TEX_ALIGN_PAGE;
    texRec->size = Tex_Size(texRec->psm, texRec->width, texRec->height, texRec->levels);
    return 1;
}

/* Sends the texture to GS memory unless mode is TEX_MODE_DEFERRED or
 * TEX_MODE_ON_DEMAND (it becomes TEX_MODE_ON_DEMAND also when no GS memory
 * is free). */
s32 Tex_Upload(u32 handle, s32 mode)
{
    TexRec *texRec = &gTexRecs[(u16)handle - 1];
    u128 *endPkt;

retry:
    switch (mode) {
    case TEX_MODE_DEFERRED:
    case TEX_MODE_ON_DEMAND:
        break;
    case 5:
    case 2:
    case 3:
    default:
        if (Tex_Reserve(&handle, 1) == 0) {
            mode = TEX_MODE_ON_DEMAND;
            texRec->mode = TEX_MODE_ON_DEMAND;
            goto retry;
        }
        Tex_SendLevels(texRec);
        endPkt = Packet_Alloc(Dma_EndPacketSize(texRec->size), 16);
        Dma_BuildEndPacket(endPkt, 1);
        DmaQueue_AddChain(0, (u32)endPkt & 0x0FFFFFFF, (u32 *)endPkt, &gDmaQueue);
        break;
    }
    gTexCount++;
    return 1;
}

/*
 * Queues the upload of every mip level of rec from its heap pixels (mem)
 * to its GS address (Tex_LevelVram), one load-image chain per level. When
 * rec->swizzled is set (swizzled pixels) each level is sent as 32-bit
 * xferW x xferH.
 */
void Tex_SendLevels(TexRec *rec)
{
    u32 src;
    u32 i;
    s32 levelWidth;
    s32 levelHeight;

    s32 size;
    u128 *pkt;
    u128 *last;
    s32 vram;

    src = (u32)Mem_GetAddr(rec->mem);
    levelWidth = rec->width;
    levelHeight = rec->height;
    for (i = 0; i < rec->levels; i++) {
        switch (rec->psm) {
        case GS_PSMT4:
            size = (levelWidth * levelHeight) >> 1;
            break;
        case GS_PSMT8:
            size = levelWidth * levelHeight;
            break;
        case GS_PSMCT16:
            size = levelWidth * levelHeight * 2;
            break;
        case GS_PSMCT24:
            size = levelWidth * levelHeight * 4;
            break;
        case GS_PSMCT32:
            size = levelWidth * levelHeight * 4;
            break;
        }
        pkt = Packet_Alloc(Dma_ImagePacketSize(size), 16);
        vram = Tex_LevelVram(rec, i);
        if (rec->swizzled == 0) {
            last = Dma_BuildLoadImage(pkt, 1, src, size, vram, levelWidth / 64, rec->psm, 0, 0, levelWidth, levelHeight);
        } else {
            last = Dma_BuildLoadImage(pkt, 1, src, size, vram, rec->xferW / 64, 0, 0, 0, rec->xferW, rec->xferH);
        }
        DmaQueue_AddChain(0, ((u32)pkt & 0x0FFFFFFF) | 0x10000000, (u32 *)last, &gDmaQueue);
        levelWidth >>= 1;
        levelHeight >>= 1;
        src += size;
    }
}

/*
 * GS address (256-byte blocks) of mip level level of rec: its vram plus
 * the sizes of the levels before it. Returns -1 when level is past the
 * last level or the psm is unknown.
 */
s32 Tex_LevelVram(TexRec *rec, u32 level)
{
    s32 i;
    s32 levelWidth;
    s32 levelHeight;
    s32 vram;
    s32 size;

    if (rec->levels <= level) {
        return -1;
    }
    levelWidth = rec->width;
    levelHeight = rec->height;
    vram = rec->vram;
    for (i = 0; i < (s32)level; i++) {
        switch (rec->psm) {
        default:
            return -1;
        case GS_PSMT4:
            size = (levelWidth * levelHeight) >> 1;
            break;
        case GS_PSMT8:
            size = levelWidth * levelHeight;
            break;
        case GS_PSMCT16:
            size = levelWidth * levelHeight * 2;
            break;
        case GS_PSMCT24:
            size = levelWidth * levelHeight * 4;
            break;
        case GS_PSMCT32:
            size = levelWidth * levelHeight * 4;
            break;
        }
        vram += (size + 255) / 256;
        levelWidth >>= 1;
        levelHeight >>= 1;
    }
    return vram;
}

/*
 * Handle of the first unused texture record (index + 1 of gTexRecs);
 * TEX_MAX + 1 when all are in use.
 */
u32 Tex_FindFree(void)
{
    s32 i;
    TexRec *texRec = gTexRecs;

    for (i = 0; i < TEX_MAX; i++, texRec++) {
        if (texRec->used == 0) {
            break;
        }
    }
    return i + 1;
}

/*
 * Creates a palette handle from img (width the colour count, 16 or 256;
 * height 1; 16, 24 or 32 bits) with mode, as flCreateTextureHandle does
 * for textures; a 256-colour palette is reordered for the GS. Returns the
 * index + 1 of the palette in gPalRecs (the handle shifted down 16 bits),
 * 0 when none is free.
 */
u32 flCreatePaletteHandle(Image *img, s32 mode)
{
    u32 handle;
    TexRec *palRec;

    handle = Pal_FindFree();
    if (handle == 0) {
        return 0;
    }
    palRec = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
    Pal_SetupRec(img, handle, mode);
    if (img->pixels == NULL) {
        Tex_Reserve(&handle, 1);
        palRec->mem = Mem_Alloc(palRec->size, 2);
        gPalCount++;
    } else {
        if (img->width == 256) {
            flPS2ConvertTextureFromContext(img, palRec, 1);
        } else {
            flPS2ConvertTextureFromContext(img, palRec, 0);
        }
        Pal_Upload(handle, mode);
    }
    return handle >> 16;
}

/*
 * Fills the palette record of handle from img: format (16, 24 or 32 bits),
 * mode, size as a texture (16 x 16 for 256 colours, 8 x 2 for 16), one
 * level, GS memory blocks and bytes. Returns 1, 0 (with a debug message)
 * when img is more than one row or of another depth.
 */
s32 Pal_SetupRec(Image *img, u32 handle, s32 mode)
{
    TexRec *palRec = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];

    if (img->height != 1) {
        Debug_Printf("Supported only 1 palette. Unallocatable. @flCreatePaletteHandle");
        return 0;
    }
    switch (img->bpp) {
    default:
        Debug_Printf("Not supported texture bit depth @flCreatePaletteHandle");
        return 0;
    case 2:
        palRec->psm = GS_PSMCT16;
        palRec->fmt = 2;
        break;
    case 3:
        palRec->psm = GS_PSMCT24;
        palRec->fmt = 3;
        break;
    case 4:
        palRec->psm = GS_PSMCT32;
        palRec->fmt = 4;
        break;
    }
    if (img->width == 256) {
        palRec->width = 16;
        palRec->height = 16;
    } else {
        palRec->width = 8;
        palRec->height = 2;
    }
    palRec->flags = img->flags;
    palRec->mode = mode;
    palRec->used = 1;
    palRec->tw = Tex_Log2(palRec->width);
    palRec->th = Tex_Log2(palRec->height);
    palRec->swizzled = 0;
    palRec->mem = 0;
    palRec->locked = NULL;
    palRec->lockFlag = 0;
    palRec->levels = 1;
    palRec->blocks = Pal_VramBlocks(palRec);
    palRec->align = 1;
    palRec->size = Tex_Size(palRec->psm, palRec->width, palRec->height, palRec->levels);
    return 1;
}

/*
 * Sends the palette of handle to GS memory unless mode is 1 or 4 (mode 4
 * also when no GS memory is free), as Tex_Upload does for textures.
 * Returns 1.
 */
s32 Pal_Upload(u32 handle, s32 mode)
{
    TexRec *palRec = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
    u128 *endPkt;

retry:
    switch (mode) {
    case TEX_MODE_DEFERRED:
    case TEX_MODE_ON_DEMAND:
        break;
    case 5:
    case 2:
    case 3:
    default:
        if (Tex_Reserve(&handle, 1) == 0) {
            mode = TEX_MODE_ON_DEMAND;
            palRec->mode = TEX_MODE_ON_DEMAND;
            goto retry;
        }
        Tex_SendLevels(palRec);
        endPkt = Packet_Alloc(Dma_EndPacketSize(palRec->size), 16);
        Dma_BuildEndPacket(endPkt, 1);
        DmaQueue_AddChain(0, (u32)endPkt & 0x0FFFFFFF, (u32 *)endPkt, &gDmaQueue);
        break;
    }
    gPalCount++;
    return 1;
}

/*
 * Handle of the first unused palette record (index + 1 of gPalRecs,
 * shifted up 16 bits); TEX_MAX + 1 shifted up 16 bits when all are in use.
 */
u32 Pal_FindFree(void)
{
    s32 i;
    TexRec *palRec = gPalRecs;

    for (i = 0; i < TEX_MAX; i++, palRec++) {
        if (palRec->used == 0) {
            break;
        }
    }
    return (i + 1) << 16;
}

/*
 * Releases texture handle handle: waits for the DMA queue (Dma_FlushQueue),
 * frees its GS memory slot and heap pixels and clears the record. Returns
 * 1, 0 when handle is out of range or unused.
 */
s32 flReleaseTextureHandle(u32 handle)
{
    TexRec *texRec = &gTexRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (texRec->used == 0) {
        return 0;
    }
    Dma_FlushQueue();
    Tex_Evict(texRec);
    if (texRec->mem != 0) {
        Mem_Free(texRec->mem);
    }
    Mem_Fill(texRec, 0, sizeof(TexRec));
    gTexCount--;
    return 1;
}

/* As flReleaseTextureHandle, without flushing the DMA queue first. */
s32 Tex_ReleaseKeep(u32 handle)
{
    TexRec *texRec = &gTexRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (texRec->used == 0) {
        return 0;
    }
    Tex_Evict(texRec);
    if (texRec->mem != 0) {
        Mem_Free(texRec->mem);
    }
    Mem_Fill(texRec, 0, sizeof(TexRec));
    gTexCount--;
    return 1;
}

/*
 * Releases palette handle handle (index + 1 of gPalRecs) as
 * flReleaseTextureHandle does textures. Returns 1, 0 when handle is out of
 * range or unused.
 */
s32 flReleasePaletteHandle(u32 handle)
{
    TexRec *palRec = &gPalRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (palRec->used == 0) {
        return 0;
    }
    Dma_FlushQueue();
    Tex_Evict(palRec);
    if (palRec->mem != 0) {
        Mem_Free(palRec->mem);
    }
    Mem_Fill(palRec, 0, sizeof(TexRec));
    gPalCount--;
    return 1;
}

/* As flReleasePaletteHandle, without flushing the DMA queue first. */
s32 Pal_ReleaseKeep(u32 handle)
{
    TexRec *palRec = &gPalRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (palRec->used == 0) {
        return 0;
    }
    Tex_Evict(palRec);
    if (palRec->mem != 0) {
        Mem_Free(palRec->mem);
    }
    Mem_Fill(palRec, 0, sizeof(TexRec));
    gPalCount--;
    return 1;
}

/*
 * Locks texture handle handle for the CPU and describes its pixels in img
 * (Tex_Lock, flag as there). rect is not used. Returns 1, 0 when handle is
 * out of range or unused.
 */
s32 flLockTexture(void *rect, u32 handle, Image *img, s32 flag)
{
    TexRec *texRec = &gTexRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (texRec->used != 0) {
        return Tex_Lock(rect, texRec, img, flag, 0);
    }
    return 0;
}

/*
 * Locks palette handle handle for the CPU (Tex_Lock, flag as there) and
 * describes it in img as one row of 256 or 16 colours. rect is not used.
 * Returns 1, 0 when handle is out of range or unused.
 */
s32 flLockPalette(void *rect, u32 handle, Image *img, s32 flag)
{
    TexRec *palRec = &gPalRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (palRec->used == 0) {
        return 0;
    }
    if (Tex_Lock(rect, palRec, img, flag, 1) == 0) {
        return 0;
    }
    if (palRec->width == 16 && palRec->height == 16) {
        img->width = 256;
        img->height = 1;
    } else {
        img->width = 16;
        img->height = 1;
    }
    return 1;
}

/*
 * Hands the pixels of rec to the CPU in img and keeps them in rec->locked;
 * flag (bits 0-1) picks how. 0: a linear copy converted from the GS
 * layout (read back from GS memory by Dma_StoreImage when rec has no heap
 * pixels). 1: a copy in the GS layout. 2 and 3: the heap pixels
 * themselves (a new buffer when there are none). Sets 2 in img->flags.
 * rect and isPalette are not used. Returns 1.
 */
s32 Tex_Lock(void *rect, TexRec *rec, Image *img, s32 flag, s32 isPalette)
{
    u8 *buf;
    u8 *gsPixels;
    Image src;

    rec->lockFlag = flag;
    img->flags = rec->flags;
    img->width = rec->width;
    img->height = rec->height;
    switch (flag & 3) {
    case 0:
        if (rec->mem == 0) {
            gsPixels = MainHeap_MaxAlloc(rec->size * 2);
            buf = gsPixels + rec->size;
            Dma_StoreImage((u32)gsPixels, rec->size, rec->vram, rec->width / 64, rec->psm, 0, 0, rec->width, rec->height);
        } else {
            buf = MainHeap_MaxAlloc(rec->size);
            gsPixels = Mem_GetAddr(rec->mem);
        }
        rec->locked = buf;
        img->pixels = buf;
        src.flags = img->flags;
        src.width = img->width;
        src.height = img->height;
        src.pixels = gsPixels;
        switch (rec->psm) {
        case GS_PSMT4:
            img->bpp = 0;
            img->pitch = img->width / 2;
            Mem_Copy(buf, gsPixels, rec->size);
            break;
        case GS_PSMT8:
            img->bpp = 1;
            img->pitch = img->width * img->bpp;
            Mem_Copy(buf, gsPixels, rec->size);
            break;
        case GS_PSMCT16:
            img->bpp = 2;
            IMAGE_RGBA5551(img);
            IMAGE_SET_PITCH(img);
            src.bpp = 2;
            IMAGE_RGBA5551(&src);
            IMAGE_GS_ORDER_16(&src);
            IMAGE_SET_PITCH(&src);
            Image_Convert(img, &src);
            break;
        case GS_PSMCT24:
            img->bpp = 3;
            IMAGE_RGB888(img);
            IMAGE_SET_PITCH(img);
            src.bpp = 3;
            IMAGE_RGB888(&src);
            IMAGE_GS_ORDER_32(&src);
            IMAGE_SET_PITCH(&src);
            Image_Convert(img, &src);
            break;
        case GS_PSMCT32:
            img->bpp = 4;
            IMAGE_RGBA8888(img);
            IMAGE_SET_PITCH(img);
            src.bpp = 4;
            IMAGE_RGBA8888(&src);
            IMAGE_GS_ORDER_32(&src);
            IMAGE_SET_PITCH(&src);
            Image_Convert(img, &src);
            break;
        }
        break;
    case 1:
        buf = MainHeap_MaxAlloc(rec->size);
        if (rec->mem == 0) {
            Dma_StoreImage((u32)buf, rec->size, rec->vram, rec->width / 64, rec->psm, 0, 0, rec->width, rec->height);
        } else {
            Mem_Copy(buf, Mem_GetAddr(rec->mem), rec->size);
        }
        rec->locked = buf;
        img->pixels = buf;
        TEX_GS_DESC(rec, img);
        break;
    case 2:
        img->pixels = rec->mem == 0 ? (rec->locked = MainHeap_MaxAlloc(rec->size)) : (rec->locked = Mem_GetAddr(rec->mem));
        TEX_GS_DESC(rec, img);
        break;
    case 3:
        img->pixels = rec->mem == 0 ? (rec->locked = MainHeap_MaxAlloc(rec->size)) : (rec->locked = Mem_GetAddr(rec->mem));
        TEX_GS_DESC(rec, img);
        break;
    }
    img->flags |= 2;
    return 1;
}

/*
 * Unlocks texture handle handle (Tex_Unlock). Returns its result, 0 when
 * handle is out of range or unused.
 */
s32 flUnlockTexture(u32 handle)
{
    TexRec *texRec = &gTexRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (texRec->used != 0) {
        return Tex_Unlock(texRec);
    }
    return 0;
}

/*
 * Unlocks palette handle handle (Tex_Unlock). Returns its result, 0 when
 * handle is out of range or unused.
 */
s32 flUnlockPalette(u32 handle)
{
    TexRec *palRec = &gPalRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (palRec->used != 0) {
        return Tex_Unlock(palRec);
    }
    return 0;
}

/*
 * Takes back the pixels Tex_Lock handed out. By the lock flag: converts
 * the linear copy back to the GS layout (0), copies it into the heap
 * pixels (1) or uses them as they are (2, 3). Clears 2 in rec->flags,
 * then queues their upload to GS memory and an end packet. A record of
 * mode 1 or 4 is evicted from GS memory instead. Returns 1.
 */
s32 Tex_Unlock(TexRec *rec)
{
    u8 *buf;
    u8 *gsPixels;
    u8 *lockedPixels;
    Image src;
    u128 *last;
    Image dst;
    u128 *pkt;

    gsPixels = NULL;
    switch (rec->lockFlag & 3) {
    case 0:
        if (rec->mem != 0) {
            gsPixels = buf = Mem_GetAddr(rec->mem);
            lockedPixels = rec->locked;
        } else {
            gsPixels = buf = MainHeap_MaxAlloc(rec->size);
            lockedPixels = rec->locked;
        }
        src.flags = rec->flags;
        src.width = rec->width;
        src.height = rec->height;
        src.pixels = lockedPixels;
        dst.flags = rec->flags;
        dst.width = rec->width;
        dst.height = rec->height;
        dst.pixels = buf;
        switch (rec->psm) {
        case GS_PSMT4:
        case GS_PSMT8:
            Mem_Copy(buf, lockedPixels, rec->size);
            break;
        case GS_PSMCT16:
            src.bpp = 2;
            IMAGE_RGBA5551(&src);
            IMAGE_SET_PITCH(&src);
            dst.bpp = 2;
            IMAGE_RGBA5551(&dst);
            IMAGE_GS_ORDER_16(&dst);
            IMAGE_SET_PITCH(&dst);
            Image_Convert(&dst, &src);
            break;
        case GS_PSMCT24:
            src.bpp = 3;
            IMAGE_RGB888(&src);
            IMAGE_SET_PITCH(&src);
            dst.bpp = 3;
            IMAGE_RGB888(&dst);
            IMAGE_GS_ORDER_32(&dst);
            IMAGE_SET_PITCH(&dst);
            Image_Convert(&dst, &src);
            break;
        case GS_PSMCT32:
            src.bpp = 4;
            IMAGE_RGBA8888(&src);
            IMAGE_SET_PITCH(&src);
            dst.bpp = 4;
            IMAGE_RGBA8888(&dst);
            IMAGE_GS_ORDER_32(&dst);
            IMAGE_SET_PITCH(&dst);
            Image_Convert(&dst, &src);
            break;
        }
        break;
    case 1:
        if (rec->mem != 0) {
            gsPixels = Mem_GetAddr(rec->mem);
            Mem_Copy(gsPixels, rec->locked, rec->size);
        } else {
            gsPixels = rec->locked;
        }
        break;
    case 2:
        gsPixels = rec->locked;
        break;
    case 3:
        gsPixels = rec->locked;
        break;
    }
    rec->flags &= ~2;
    if (gsPixels != NULL) {
        if (rec->mode == TEX_MODE_DEFERRED || rec->mode == TEX_MODE_ON_DEMAND) {
            Tex_Evict(rec);
            return 1;
        }
        pkt = Packet_Alloc(Dma_ImagePacketSize(rec->size), 16);
        if (rec->swizzled == 0) {
            last = Dma_BuildLoadImage(pkt, 1, (u32)gsPixels, rec->size, rec->vram, rec->width / 64, rec->psm, 0, 0, rec->width,
                                      rec->height);
        } else {
            last = Dma_BuildLoadImage(pkt, 1, (u32)gsPixels, rec->size, rec->vram, rec->xferW / 64, 0, 0, 0, rec->xferW,
                                      rec->xferH);
        }
        DmaQueue_AddChain(0, ((u32)pkt & 0x0FFFFFFF) | 0x10000000, (u32 *)last, &gDmaQueue);
        pkt = Packet_Alloc(Dma_EndPacketSize(rec->size), 16);
        Dma_BuildEndPacket(pkt, 1);
        DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    }
    return 1;
}

/* Makes n texture handles resident in GS memory; returns what
 * Tex_MakeResident returns. */
s32 Tex_Use(s32 n, u32 *handles)
{
    return Tex_MakeResident(n, handles);
}

/*
 * Makes the textures and palettes of handleCount handles resident in GS
 * memory: each record not yet resident gets a gap (VramSlot_FindGap), or
 * else the slot of a texture not among handles (VramSlot_FindVictim,
 * asked again until one is found), and its pixels are queued
 * (Tex_SendLevels), then an end packet. Each handle holds a texture handle
 * in its low 16 bits and a palette handle in its high 16 bits (0: none).
 * Returns how many records were sent.
 */
s32 Tex_MakeResident(s32 handleCount, u32 *handles)
{
    TexRec *last = NULL;
    s32 i;
    TexRec *texRec;
    TexRec *palRec;
    VramSlot *slot;

    TexRec *rec;
    s32 k;
    s32 count = 0;
    u32 texHandle;
    u32 palHandle;
    u128 *endPkt;

    for (i = 0; i < handleCount; i++) {
        texHandle = handles[i] & 0xFFFF;
        palHandle = (handles[i] & 0xFFFF0000) >> 16;
        texRec = &gTexRecs[texHandle - 1];
        palRec = &gPalRecs[palHandle - 1];
        for (k = 0; k < 2; k++) {
            if (k == 0) {
                if (texHandle == 0 || texHandle >= 256) {
                    continue;
                }
                rec = texRec;
            } else {
                if (palHandle == 0 || palHandle >= 256) {
                    continue;
                }
                rec = palRec;
            }
            if (rec->resident != 0) {
                continue;
            }
            slot = VramSlot_FindGap(rec->blocks, rec->align);
            if (slot != (VramSlot *)-1) {
                VramSlot_Assign(slot, rec);
            } else {
                do {
                    slot = VramSlot_FindVictim(rec, handles, handleCount);
                } while (slot == (VramSlot *)-1);
                VramSlot_Replace(slot, rec);
            }
            Tex_SendLevels(rec);
            last = rec;
            count++;
        }
    }
    if (last != NULL) {
        endPkt = Packet_Alloc(Dma_EndPacketSize(0), 16);
        Dma_BuildEndPacket(endPkt, 1);
        DmaQueue_AddChain(0, (u32)endPkt & 0x0FFFFFFF, (u32 *)endPkt, &gDmaQueue);
    }
    return count;
}
