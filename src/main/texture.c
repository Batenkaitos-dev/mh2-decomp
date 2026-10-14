/*
 * 0x17B1B0: texture and palette handles: GS memory blocks, creating a
 * handle from an Image (the pixels are converted to the GS layout and sent,
 * mip level by mip level), releasing it, and locking it for the CPU.
 */
#include "common.h"
#include "dma.h"
#include "texture.h"

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
u8 *MainHeap_MaxAlloc(u32 size);
void Mem_Copy(void *dst, void *src, s32 n);
s32 Image_Convert(Image *dst, Image *src);
void Dma_StoreImage(u32 dst, u32 size, s16 sbp, s16 sbw, s16 psm, s16 sx, s16 sy, s16 w, s16 h);

extern s32 gSysTexBlock;          /* gSysWork: next free GS block */

void Mem_Fill(void *dst, u8 c, s32 n);
s32 Dma_FlushQueue(void);
s32 Tex_UploadPacketSize(u32 unused0);
void *Packet_Alloc(u32 size, s32 align);
void *Mem_Alloc(u32 size, s32 flag);
void Mem_Free(void *handle);
void *Mem_GetAddr(void *handle);
s32 Debug_Printf(const char *fmt, ...);
void Dma_BuildEndPacket(u128 *pkt, s32 irq);
u128 *Dma_BuildLoadImage(u128 *pkt, u32 irq, u32 src, u32 size, s16 dbp, s16 dbw, s16 psm, s16 dx,
                         s16 dy, s16 w, s16 h);
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q);
extern DmaQueue gDmaQueue;

/* A GS memory block of size bytes (rounded to 8 KB pages); returns its
 * address in 256-byte blocks, 0 when all four are in use. */
u16 Vram_AllocBlock(u32 size)
{
    s32 i;
    u32 n;
    VramBlock *b;

    if (gVramBlockCount >= 4) {
        return 0;
    }
    VramSlot_FreeAll();
    n = (((size + 0xFF) >> 8) + 0x1F) & ~0x1F;
    for (b = gVramBlocks, i = 0; i < 4; i++, b++) {
        if (b->used == 0) {
            break;
        }
    }
    b->used = 1;
    b->addr = gSysTexBlock;
    b->size = n;
    gVramBlockCount++;
    gSysTexBlock += n;
    return b->addr;
}

/* Frees the block at addr (which becomes the next free address). */
void Vram_FreeBlock(u32 addr)
{
    VramBlock *b;
    s32 i;

    for (i = 0, b = gVramBlocks; i < 0x200; i++, b++) {
        if (b->addr == (u16)addr) {
            break;
        }
    }
    if (i != 4) {
        VramSlot_FreeAll();
        Mem_Fill(b, 0, sizeof(VramBlock));
        gVramBlockCount--;
        gSysTexBlock = (u16)addr;
    }
}

u32 flCreateTextureHandle(Image *img, s32 mode)
{
    u32 handle;
    TexRec *t;

    handle = Tex_FindFree();
    if (handle == 0) {
        return 0;
    }
    t = &gTexRecs[(handle & 0xFFFF) - 1];
    Tex_SetupRec(img, 1, handle, mode);
    if (img->pixels == NULL) {
        Tex_Reserve(&handle, 1);
        t->mem = Mem_Alloc(t->size, 2);
        gTexCount++;
    } else {
        flPS2ConvertTextureFromContext(img, t, 0);
        Tex_Upload(handle, mode);
    }
    return handle;
}

s32 Tex_SetupRec(Image *img, u32 levels, u32 handle, s32 mode)
{
    TexRec *t = &gTexRecs[(handle & 0xFFFF) - 1];

    t->used = 1;
    t->mode = mode;
    t->flags = img->flags;
    t->width = img->width;
    t->height = img->height;
    t->unk36 = 0;
    t->mem = 0;
    t->locked = NULL;
    t->lockFlag = 0;
    t->levels = levels;
    switch (img->bpp) {
    case 0:
        t->psm = GS_PSMT4;
        t->fmt = 0;
        break;
    case 1:
        t->psm = GS_PSMT8;
        t->fmt = 1;
        break;
    case 2:
        t->psm = GS_PSMCT16;
        t->fmt = 2;
        break;
    case 3:
        t->psm = GS_PSMCT24;
        t->fmt = 3;
        break;
    case 4:
        t->psm = GS_PSMCT32;
        t->fmt = 4;
        break;
    }
    t->tw = Tex_Log2(t->width);
    t->th = Tex_Log2(t->height);
    t->blocks = Tex_VramBlocks(t);
    t->align = 0x20;
    t->size = Tex_Size(t->psm, t->width, t->height, t->levels);
    return 1;
}

/* Sends the texture to GS memory unless mode is 1 or 4 (mode 4 also when
 * no GS memory is free). */
s32 Tex_Upload(u32 handle, s32 mode)
{
    TexRec *t = &gTexRecs[(u16)handle - 1];
    u128 *p;

retry:
    switch (mode) {
    case 1:
    case 4:
        break;
    case 5:
    case 2:
    case 3:
    default:
        if (Tex_Reserve(&handle, 1) == 0) {
            mode = 4;
            t->mode = 4;
            goto retry;
        }
        Tex_SendLevels(t);
        p = Packet_Alloc(Tex_UploadPacketSize(t->size), 16);
        Dma_BuildEndPacket(p, 1);
        DmaQueue_AddChain(0, (u32)p & 0x0FFFFFFF, (u32 *)p, &gDmaQueue);
        break;
    }
    gTexCount++;
    return 1;
}

void Tex_SendLevels(TexRec *t)
{
    u32 src;
    u32 i;
    s32 w;
    s32 h;

    s32 size;
    u128 *p;
    u128 *last;
    s32 vram;

    src = (u32)Mem_GetAddr(t->mem);
    w = t->width;
    h = t->height;
    for (i = 0; i < t->levels; i++) {
        switch (t->psm) {
        case GS_PSMT4:
            size = (w * h) >> 1;
            break;
        case GS_PSMT8:
            size = w * h;
            break;
        case GS_PSMCT16:
            size = w * h * 2;
            break;
        case GS_PSMCT24:
            size = w * h * 4;
            break;
        case GS_PSMCT32:
            size = w * h * 4;
            break;
        }
        p = Packet_Alloc(Dma_ImagePacketSize(size), 16);
        vram = Tex_LevelVram(t, i);
        if (t->unk36 == 0) {
            last = Dma_BuildLoadImage(p, 1, src, size, vram, w / 64, t->psm, 0, 0, w, h);
        } else {
            last = Dma_BuildLoadImage(p, 1, src, size, vram, t->xferW / 64, 0, 0, 0, t->xferW, t->xferH);
        }
        DmaQueue_AddChain(0, ((u32)p & 0x0FFFFFFF) | 0x10000000, (u32 *)last, &gDmaQueue);
        w >>= 1;
        h >>= 1;
        src += size;
    }
}

s32 Tex_LevelVram(TexRec *t, u32 level)
{
    s32 i;
    s32 w;
    s32 h;
    s32 vram;
    s32 size;

    if (t->levels <= level) {
        return -1;
    }
    w = t->width;
    h = t->height;
    vram = t->vram;
    for (i = 0; i < (s32)level; i++) {
        switch (t->psm) {
        default:
            return -1;
        case GS_PSMT4:
            size = (w * h) >> 1;
            break;
        case GS_PSMT8:
            size = w * h;
            break;
        case GS_PSMCT16:
            size = w * h * 2;
            break;
        case GS_PSMCT24:
            size = w * h * 4;
            break;
        case GS_PSMCT32:
            size = w * h * 4;
            break;
        }
        vram += (size + 0xFF) / 256;
        w >>= 1;
        h >>= 1;
    }
    return vram;
}

u32 Tex_FindFree(void)
{
    s32 i;
    TexRec *t = gTexRecs;

    for (i = 0; i < TEX_MAX; i++, t++) {
        if (t->used == 0) {
            break;
        }
    }
    return i + 1;
}

/* img: width = the colour count (16 or 256), height 1; 16, 24 or 32 bits. */
u32 flCreatePaletteHandle(Image *img, s32 mode)
{
    u32 handle;
    TexRec *t;

    handle = Pal_FindFree();
    if (handle == 0) {
        return 0;
    }
    t = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
    Pal_SetupRec(img, handle, mode);
    if (img->pixels == NULL) {
        Tex_Reserve(&handle, 1);
        t->mem = Mem_Alloc(t->size, 2);
        gPalCount++;
    } else {
        if (img->width == 0x100) {
            flPS2ConvertTextureFromContext(img, t, 1);
        } else {
            flPS2ConvertTextureFromContext(img, t, 0);
        }
        Pal_Upload(handle, mode);
    }
    return handle >> 16;
}

s32 Pal_SetupRec(Image *img, u32 handle, s32 mode)
{
    TexRec *t = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];

    if (img->height != 1) {
        Debug_Printf("Supported only 1 palette. Unallocatable. @flCreatePaletteHandle");
        return 0;
    }
    switch (img->bpp) {
    default:
        Debug_Printf("Not supported texture bit depth @flCreatePaletteHandle");
        return 0;
    case 2:
        t->psm = GS_PSMCT16;
        t->fmt = 2;
        break;
    case 3:
        t->psm = GS_PSMCT24;
        t->fmt = 3;
        break;
    case 4:
        t->psm = GS_PSMCT32;
        t->fmt = 4;
        break;
    }
    if (img->width == 0x100) {
        t->width = 16;
        t->height = 16;
    } else {
        t->width = 8;
        t->height = 2;
    }
    t->flags = img->flags;
    t->mode = mode;
    t->used = 1;
    t->tw = Tex_Log2(t->width);
    t->th = Tex_Log2(t->height);
    t->unk36 = 0;
    t->mem = 0;
    t->locked = NULL;
    t->lockFlag = 0;
    t->levels = 1;
    t->blocks = Pal_VramBlocks(t);
    t->align = 1;
    t->size = Tex_Size(t->psm, t->width, t->height, t->levels);
    return 1;
}

s32 Pal_Upload(u32 handle, s32 mode)
{
    TexRec *t = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
    u128 *p;

retry:
    switch (mode) {
    case 1:
    case 4:
        break;
    case 5:
    case 2:
    case 3:
    default:
        if (Tex_Reserve(&handle, 1) == 0) {
            mode = 4;
            t->mode = 4;
            goto retry;
        }
        Tex_SendLevels(t);
        p = Packet_Alloc(Tex_UploadPacketSize(t->size), 16);
        Dma_BuildEndPacket(p, 1);
        DmaQueue_AddChain(0, (u32)p & 0x0FFFFFFF, (u32 *)p, &gDmaQueue);
        break;
    }
    gPalCount++;
    return 1;
}

u32 Pal_FindFree(void)
{
    s32 i;
    TexRec *t = gPalRecs;

    for (i = 0; i < TEX_MAX; i++, t++) {
        if (t->used == 0) {
            break;
        }
    }
    return (i + 1) << 16;
}

s32 flReleaseTextureHandle(u32 handle)
{
    TexRec *t = &gTexRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used == 0) {
        return 0;
    }
    Dma_FlushQueue();
    Tex_Evict(t);
    if (t->mem != 0) {
        Mem_Free(t->mem);
    }
    Mem_Fill(t, 0, sizeof(TexRec));
    gTexCount--;
    return 1;
}

/* As flReleaseTextureHandle, without flushing the DMA queue first. */
s32 Tex_ReleaseKeep(u32 handle)
{
    TexRec *t = &gTexRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used == 0) {
        return 0;
    }
    Tex_Evict(t);
    if (t->mem != 0) {
        Mem_Free(t->mem);
    }
    Mem_Fill(t, 0, sizeof(TexRec));
    gTexCount--;
    return 1;
}

s32 flReleasePaletteHandle(u32 handle)
{
    TexRec *t = &gPalRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used == 0) {
        return 0;
    }
    Dma_FlushQueue();
    Tex_Evict(t);
    if (t->mem != 0) {
        Mem_Free(t->mem);
    }
    Mem_Fill(t, 0, sizeof(TexRec));
    gPalCount--;
    return 1;
}

s32 Pal_ReleaseKeep(u32 handle)
{
    TexRec *t = &gPalRecs[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used == 0) {
        return 0;
    }
    Tex_Evict(t);
    if (t->mem != 0) {
        Mem_Free(t->mem);
    }
    Mem_Fill(t, 0, sizeof(TexRec));
    gPalCount--;
    return 1;
}

s32 flLockTexture(void *rect, u32 handle, Image *img, s32 flag)
{
    TexRec *t = &gTexRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used != 0) {
        return Tex_Lock(rect, t, img, flag, 0);
    }
    return 0;
}

s32 flLockPalette(void *rect, u32 handle, Image *img, s32 flag)
{
    TexRec *t = &gPalRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used == 0) {
        return 0;
    }
    if (Tex_Lock(rect, t, img, flag, 1) == 0) {
        return 0;
    }
    if (t->width == 16 && t->height == 16) {
        img->width = 0x100;
        img->height = 1;
    } else {
        img->width = 16;
        img->height = 1;
    }
    return 1;
}

s32 Tex_Lock(void *rect, TexRec *t, Image *img, s32 flag, s32 pal)
{
    u8 *buf;
    u8 *gs;
    Image src;

    t->lockFlag = flag;
    img->flags = t->flags;
    img->width = t->width;
    img->height = t->height;
    switch (flag & 3) {
    case 0:
        if (t->mem == 0) {
            gs = MainHeap_MaxAlloc(t->size * 2);
            buf = gs + t->size;
            Dma_StoreImage((u32)gs, t->size, t->vram, t->width / 64, t->psm, 0, 0, t->width, t->height);
        } else {
            buf = MainHeap_MaxAlloc(t->size);
            gs = Mem_GetAddr(t->mem);
        }
        t->locked = buf;
        img->pixels = buf;
        src.flags = img->flags;
        src.width = img->width;
        src.height = img->height;
        src.pixels = gs;
        switch (t->psm) {
        case GS_PSMT4:
            img->bpp = 0;
            img->pitch = img->width / 2;
            Mem_Copy(buf, gs, t->size);
            break;
        case GS_PSMT8:
            img->bpp = 1;
            img->pitch = img->width * img->bpp;
            Mem_Copy(buf, gs, t->size);
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
        buf = MainHeap_MaxAlloc(t->size);
        if (t->mem == 0) {
            Dma_StoreImage((u32)buf, t->size, t->vram, t->width / 64, t->psm, 0, 0, t->width, t->height);
        } else {
            Mem_Copy(buf, Mem_GetAddr(t->mem), t->size);
        }
        t->locked = buf;
        img->pixels = buf;
        TEX_GS_DESC(t, img);
        break;
    case 2:
        img->pixels = t->mem == 0 ? (t->locked = MainHeap_MaxAlloc(t->size)) : (t->locked = Mem_GetAddr(t->mem));
        TEX_GS_DESC(t, img);
        break;
    case 3:
        img->pixels = t->mem == 0 ? (t->locked = MainHeap_MaxAlloc(t->size)) : (t->locked = Mem_GetAddr(t->mem));
        TEX_GS_DESC(t, img);
        break;
    }
    img->flags |= 2;
    return 1;
}

s32 flUnlockTexture(u32 handle)
{
    TexRec *t = &gTexRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used != 0) {
        return Tex_Unlock(t);
    }
    return 0;
}

s32 flUnlockPalette(u32 handle)
{
    TexRec *t = &gPalRecs[handle - 1];

    if (handle > TEX_MAX) {
        return 0;
    }
    if (t->used != 0) {
        return Tex_Unlock(t);
    }
    return 0;
}

s32 Tex_Unlock(TexRec *t)
{
    u8 *buf;
    u8 *gs;
    u8 *pix;
    Image src;
    u128 *last;
    Image dst;
    u128 *p;

    gs = NULL;
    switch (t->lockFlag & 3) {
    case 0:
        if (t->mem != 0) {
            gs = buf = Mem_GetAddr(t->mem);
            pix = t->locked;
        } else {
            gs = buf = MainHeap_MaxAlloc(t->size);
            pix = t->locked;
        }
        src.flags = t->flags;
        src.width = t->width;
        src.height = t->height;
        src.pixels = pix;
        dst.flags = t->flags;
        dst.width = t->width;
        dst.height = t->height;
        dst.pixels = buf;
        switch (t->psm) {
        case GS_PSMT4:
        case GS_PSMT8:
            Mem_Copy(buf, pix, t->size);
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
        if (t->mem != 0) {
            gs = Mem_GetAddr(t->mem);
            Mem_Copy(gs, t->locked, t->size);
        } else {
            gs = t->locked;
        }
        break;
    case 2:
        gs = t->locked;
        break;
    case 3:
        gs = t->locked;
        break;
    }
    t->flags &= ~2;
    if (gs != NULL) {
        if (t->mode == 1 || t->mode == 4) {
            Tex_Evict(t);
            return 1;
        }
        p = Packet_Alloc(Dma_ImagePacketSize(t->size), 16);
        if (t->unk36 == 0) {
            last = Dma_BuildLoadImage(p, 1, (u32)gs, t->size, t->vram, t->width / 64, t->psm, 0, 0, t->width,
                                      t->height);
        } else {
            last = Dma_BuildLoadImage(p, 1, (u32)gs, t->size, t->vram, t->xferW / 64, 0, 0, 0, t->xferW,
                                      t->xferH);
        }
        DmaQueue_AddChain(0, ((u32)p & 0x0FFFFFFF) | 0x10000000, (u32 *)last, &gDmaQueue);
        p = Packet_Alloc(Tex_UploadPacketSize(t->size), 16);
        Dma_BuildEndPacket(p, 1);
        DmaQueue_AddChain(0, (u32)p & 0x0FFFFFFF, (u32 *)p, &gDmaQueue);
    }
    return 1;
}
