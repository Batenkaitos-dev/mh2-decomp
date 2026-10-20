/*
 * Placing textures in GS memory (the VramSlot
 * list: finding a gap, evicting textures that are not in use), and the
 * screen texture (gScreenTex, a copy of the frame buffer).
 */
#include "common.h"
#include "dma.h"
#include "texture.h"

#define VRAM_END 0x4000     /* GS memory, 256-byte blocks */

extern s32 gSysTexBlock;      /* gSysWork: first GS block for textures */
extern s32 gSysFbDepth;
extern s32 gSysFbPsm;      /* display pixel format */
extern s32 gSysFbBlock0;      /* frame buffers (GS blocks) */
extern s32 gSysFbBlock1;
extern s32 gSysCurBuf;      /* frame buffer shown */
extern s32 gSysNoDraw;
extern s32 gDispHeight;      /* display height */
extern s32 gDispWidth;      /* display width */
extern s32 gFlipCountdown;
extern DmaQueue gDmaQueue;

/* The screen texture and its state (0: none, 1: GS block reserved,
 * 2: resident). */
extern TexRec gScreenTex;
extern s32 gScreenTexState;
extern s32 gScreenTexVram;

s32 Dma_WaitIdle(void);
s32 Tex_UploadPacketSize(u32 unused0);
void *Packet_Alloc(u32 size, s32 align);
void Dma_BuildEndPacket(u128 *pkt, s32 irq);
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q);
void Dma_QueueStoreImage(void *dst, u32 size, s16 sbp, s16 sbw, s16 psm, s16 sx, s16 sy, s16 w, s16 h,
                         u32 wait);

/*
 * The VramSlot holding texRec, searched from gVramSlotHead; NULL when
 * texRec has no GS memory.
 */
VramSlot *VramSlot_Find(TexRec *texRec)
{
    VramSlot *slot;

    for (slot = gVramSlotHead; slot != NULL; slot = slot->next) {
        if (slot->tex == texRec) {
            return slot;
        }
    }
    return NULL;
}

/*
 * Gives texRec a new VramSlot inserted after `after` (at the start of
 * texture memory, gSysTexBlock, when after is NULL), at the first address
 * past after's blocks aligned to texRec->align. Sets texRec->vram, marks
 * it resident and copies its flags, mode, block count and alignment into
 * the slot. Returns 1, or 0 when no free VramSlot is left.
 */
s32 VramSlot_Assign(VramSlot *after, TexRec *texRec)
{
    VramSlot *slot;
    s16 addr;

    if (after == NULL) {
        if ((slot = VramSlot_Alloc(gVramSlots, VRAM_SLOT_MAX)) == NULL) {
            return 0;
        }
        VramSlot_Insert(after, slot);
        addr = gSysTexBlock;
    } else {
        if ((slot = VramSlot_Alloc(gVramSlots, VRAM_SLOT_MAX)) == NULL) {
            return 0;
        }
        VramSlot_Insert(after, slot);
        addr = ~(texRec->align - 1) & (texRec->align - 1 + (after->addr + after->blocks));
    }
    texRec->vram = addr;
    texRec->resident = 1;
    slot->tex = texRec;
    slot->flags = texRec->flags;
    slot->mode = texRec->mode;
    slot->addr = addr;
    slot->blocks = texRec->blocks;
    slot->align = texRec->align;
    return 1;
}

/*
 * Gives slot (a new one at the start of texture memory when slot is NULL)
 * to texRec: the slots after it that overlap texRec's blocks are freed,
 * and the texture the slot held is evicted (resident 0, mode 4). Returns
 * 1, or 0 when slot is NULL and no free VramSlot is left. seen: called
 * with the slot VramSlot_FindVictim found.
 */
s32 VramSlot_Replace(VramSlot *slot, TexRec *texRec)
{
    if (slot == NULL) {
        if ((slot = VramSlot_Alloc(gVramSlots, VRAM_SLOT_MAX)) == NULL) {
            return 0;
        }
        VramSlot_Insert(NULL, slot);
        slot->addr = gSysTexBlock;
    } else if (slot->next != NULL && slot->next->addr - slot->addr < texRec->blocks) {
        do {
            VramSlot_Free(slot->next);
        } while (slot->next != NULL && slot->next->addr - slot->addr < texRec->blocks);
    }
    if (slot->tex != NULL) {
        slot->tex->resident = 0;
        slot->tex->mode = 4;
    }
    texRec->vram = slot->addr;
    texRec->resident = 1;
    slot->tex = texRec;
    slot->flags = texRec->flags;
    slot->mode = texRec->mode;
    slot->blocks = texRec->blocks;
    slot->align = texRec->align;
    return 1;
}

/* Frees every VramSlot in the list. Returns 1. */
s32 VramSlot_FreeAll(void)
{
    while (gVramSlotHead != NULL) {
        VramSlot_Free(gVramSlotHead);
    }
    return 1;
}

/* Frees the VramSlot holding texRec, if it has one. Returns 1. */
s32 Tex_Evict(TexRec *texRec)
{
    VramSlot *slot = VramSlot_Find(texRec);

    if (slot != NULL) {
        VramSlot_Free(slot);
    }
    return 1;
}

/*
 * Places the textures and palettes of handleCount handles in GS memory
 * (gaps only, nothing is evicted). A handle holds a texture number in its
 * low 16 bits and a palette number in its high 16 bits (1-based into
 * gTexRecs and gPalRecs, 0: none). Returns 1, or 0 when one does not fit.
 */
s32 Tex_Reserve(u32 *handles, s32 handleCount)
{
    u32 handle;
    s32 i;
    TexRec *texRec;
    VramSlot *gapSlot;

    for (i = 0; i < handleCount; i++) {
        handle = *handles++;
        if ((handle & 0xFFFF) != 0) {
            texRec = &gTexRecs[(handle & 0xFFFF) - 1];
            if (texRec->resident == 0) {
                gapSlot = VramSlot_FindGap(texRec->blocks, texRec->align);
                if (gapSlot == (VramSlot *)-1) {
                    return 0;
                }
                if (VramSlot_Assign(gapSlot, texRec) == 0) {
                    return 0;
                }
            }
        }
        if ((handle & 0xFFFF0000) >> 16 != 0) {
            texRec = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
            if (texRec->resident == 0) {
                gapSlot = VramSlot_FindGap(texRec->blocks, texRec->align);
                if (gapSlot == (VramSlot *)-1) {
                    return 0;
                }
                if (VramSlot_Assign(gapSlot, texRec) == 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/* The slot after which `blocks` (aligned) fit, NULL for the start of
 * texture memory, -1 when there is no room. */
VramSlot *VramSlot_FindGap(u32 blocks, s32 align)
{
    VramSlot *slot = gVramSlotHead;
    VramSlot *next;
    u32 start;
    s32 end;

    if (slot == NULL) {
        return NULL;
    }
    if (slot->addr != gSysTexBlock && (u32)(slot->addr - gSysTexBlock) >= blocks) {
        return NULL;
    }
    for (;;) {
        end = slot->addr + slot->blocks;
        next = slot->next;
        start = ~(align - 1) & (end + (align - 1));
        if (next == NULL) {
            if (end >= VRAM_END) {
                return (VramSlot *)-1;
            }
            if (start + blocks >= VRAM_END) {
                return (VramSlot *)-1;
            }
            return slot;
        }
        if (start < (u32)next->addr && (u32)(next->addr - start) >= blocks) {
            return slot;
        }
        slot = next;
    }
}

/*
 * The slot from which texRec can take GS memory by evicting the textures
 * there (pass 0: only textures of mode 1 or 4, pass 1: any) that are not
 * among the handleCount handles; the slot must start at an address
 * aligned for texRec. Returns NULL when there are no slots, 1 when texRec
 * fits after the last one, -1 when nothing can be evicted.
 */
VramSlot *VramSlot_FindVictim(TexRec *texRec, u32 *handles, s32 handleCount)
{
    VramSlot *runSlot;
    VramSlot *first;
    s32 pass = 0;
    s32 i;
    VramSlot *slot;
    VramSlot *last;
    u32 alignedAddr;
    u32 handle;

    for (;;) {
        slot = gVramSlotHead;
        if (slot == NULL) {
            return NULL;
        }
        do {
            if (pass == 0 && slot->mode != 1 && slot->mode != 4) {
                goto next;
            }
            for (i = 0; i < handleCount; i++) {
                handle = handles[i];
                if ((handle & 0xFFFF) != 0 && &gTexRecs[(handle & 0xFFFF) - 1] == slot->tex) {
                    goto next;
                }
                if ((handle & 0xFFFF0000) >> 16 != 0 && &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1] == slot->tex) {
                    goto next;
                }
            }
            alignedAddr = ~(texRec->align - 1) & (slot->addr + (texRec->align - 1));
            if (slot->addr != alignedAddr) {
                goto next;
            }
            runSlot = slot->next;
            if (runSlot == NULL) {
                if (VRAM_END - alignedAddr < texRec->blocks) {
                    goto next;
                }
                return slot;
            }
            first = slot;
            while (runSlot->addr - alignedAddr < texRec->blocks) {
                slot = runSlot;
                if (pass == 0 && runSlot->mode != 1 && runSlot->mode != 4) {
                    goto next;
                }
                for (i = 0; i < handleCount; i++) {
                    handle = handles[i];
                    if ((handle & 0xFFFF) != 0 && &gTexRecs[(handle & 0xFFFF) - 1] == runSlot->tex) {
                        goto next;
                    }
                    if ((handle & 0xFFFF0000) >> 16 != 0 && &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1] == runSlot->tex) {
                        goto next;
                    }
                }
                runSlot = runSlot->next;
                if (runSlot == NULL) {
                    if (VRAM_END - alignedAddr < texRec->blocks) {
                        goto next;
                    }
                    return first;
                }
            }
            return first;
        next:
            last = slot;
            slot = slot->next;
        } while (slot != NULL);
        if (VRAM_END - (last->addr + last->blocks) >= texRec->blocks) {
            return (VramSlot *)1;
        }
        if (pass != 0) {
            break;
        }
        pass++;
    }
    return (VramSlot *)-1;
}

/* Describes the frame buffer as a texture whose pixels go to buf. */
void ScreenTex_Init(void *buf)
{
    gScreenTex.used = 0;
    gScreenTex.resident = 0;
    gScreenTex.unk36 = 0;
    gScreenTex.mode = 4;
    gScreenTex.levels = 1;
    gScreenTex.lockFlag = 0;
    gScreenTex.flags = 0;
    gScreenTex.locked = NULL;
    gScreenTex.psm = gSysFbPsm;
    gScreenTex.fmt = gSysFbDepth;
    gScreenTex.width = gDispWidth;
    gScreenTex.height = gDispHeight;
    gScreenTex.tw = Tex_Log2(gScreenTex.width);
    gScreenTex.th = Tex_Log2(gScreenTex.height);
    gScreenTex.blocks = Tex_VramBlocks(&gScreenTex);
    gScreenTex.align = 0x20;
    gScreenTex.size = Tex_Size(gScreenTex.psm, gScreenTex.width, gScreenTex.height, gScreenTex.levels);
    gScreenTex.mem = buf;
}

/* Reads the frame buffer shown (fromTex: the screen texture's GS copy) back
 * into the screen texture's buffer; fromTex 1 then drops the GS copy. */
s32 ScreenTex_Capture(s32 fromTex)
{
    TexRec *screenTex;
    s16 srcBlock;

    if (gSysNoDraw != 0) {
        return 0;
    }
    if (gFlipCountdown != 0) {
        return 0;
    }
    Dma_WaitIdle();
    screenTex = &gScreenTex;
    if (fromTex == 0) {
        if (gSysCurBuf == 0) {
            srcBlock = gSysFbBlock0;
        } else {
            srcBlock = gSysFbBlock1;
        }
    } else {
        srcBlock = screenTex->vram;
    }
    Dma_QueueStoreImage(screenTex->mem, screenTex->size, srcBlock, screenTex->width / 64, screenTex->psm, 0, 0, screenTex->width, screenTex->height, 1);
    Dma_WaitIdle();
    if (fromTex == 1) {
        ScreenTex_Release();
    }
    return 1;
}

/* Sends the screen texture to GS memory (into its reserved block, or a
 * gap); 0 when there is no room. */
s32 ScreenTex_Upload(void)
{
    TexRec *screenTex = &gScreenTex;
    VramSlot *gapSlot;
    u128 *endPacket;

    if (gScreenTexState == 2) {
        ScreenTex_SetRegs();
        return 1;
    }
    if (gScreenTexState == 1) {
        screenTex->vram = gScreenTexVram;
        screenTex->resident = 1;
    } else {
        gapSlot = VramSlot_FindGap(screenTex->blocks, screenTex->align);
        if ((s32)gapSlot < 0) {
            return 0;
        }
        if (VramSlot_Assign(gapSlot, screenTex) == 0) {
            return 0;
        }
    }
    ScreenTex_Send(screenTex);
    endPacket = Packet_Alloc(Tex_UploadPacketSize(0), 16);
    Dma_BuildEndPacket(endPacket, 1);
    DmaQueue_AddChain(0, (u32)endPacket & 0x0FFFFFFF, (u32 *)endPacket, &gDmaQueue);
    ScreenTex_SetRegs();
    return 1;
}

/*
 * Frees the GS block reserved for the screen texture (gScreenTexVram, when
 * set) and sets gScreenTexState to 0 (none).
 */
void ScreenTex_Release(void)
{
    if (gScreenTexVram != 0) {
        Vram_FreeBlock(gScreenTexVram & 0xFFFF);
    }
    gScreenTexState = 0;
}
