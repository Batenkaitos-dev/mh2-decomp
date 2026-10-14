/*
 * main 0x17eb00-0x17f660: placing textures in GS memory (the VramSlot
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

/* The slot holding t. */
VramSlot *VramSlot_Find(TexRec *t)
{
    VramSlot *s;

    for (s = gVramSlotHead; s != NULL; s = s->next) {
        if (s->tex == t) {
            return s;
        }
    }
    return NULL;
}

/* Gives t a new slot after `after` (at the start of texture memory when
 * NULL), at the next address aligned for t. */
s32 VramSlot_Assign(VramSlot *after, TexRec *t)
{
    VramSlot *s;
    s16 addr;

    if (after == NULL) {
        if ((s = VramSlot_Alloc(gVramSlots, VRAM_SLOT_MAX)) == NULL) {
            return 0;
        }
        VramSlot_Insert(after, s);
        addr = gSysTexBlock;
    } else {
        if ((s = VramSlot_Alloc(gVramSlots, VRAM_SLOT_MAX)) == NULL) {
            return 0;
        }
        VramSlot_Insert(after, s);
        addr = ~(t->align - 1) & (t->align - 1 + (after->addr + after->blocks));
    }
    t->vram = addr;
    t->resident = 1;
    s->tex = t;
    s->flags = t->flags;
    s->mode = t->mode;
    s->addr = addr;
    s->blocks = t->blocks;
    s->align = t->align;
    return 1;
}

/* Gives slot s (a new one at the start when NULL) to t: the slots after it
 * that overlap t's blocks are freed, and its texture is evicted. */
s32 VramSlot_Replace(VramSlot *s, TexRec *t)
{
    if (s == NULL) {
        if ((s = VramSlot_Alloc(gVramSlots, VRAM_SLOT_MAX)) == NULL) {
            return 0;
        }
        VramSlot_Insert(NULL, s);
        s->addr = gSysTexBlock;
    } else if (s->next != NULL && s->next->addr - s->addr < t->blocks) {
        do {
            VramSlot_Free(s->next);
        } while (s->next != NULL && s->next->addr - s->addr < t->blocks);
    }
    if (s->tex != NULL) {
        s->tex->resident = 0;
        s->tex->mode = 4;
    }
    t->vram = s->addr;
    t->resident = 1;
    s->tex = t;
    s->flags = t->flags;
    s->mode = t->mode;
    s->blocks = t->blocks;
    s->align = t->align;
    return 1;
}

s32 VramSlot_FreeAll(void)
{
    while (gVramSlotHead != NULL) {
        VramSlot_Free(gVramSlotHead);
    }
    return 1;
}

s32 Tex_Evict(TexRec *t)
{
    VramSlot *s = VramSlot_Find(t);

    if (s != NULL) {
        VramSlot_Free(s);
    }
    return 1;
}

/* Places the textures and palettes of n handles in GS memory (gaps only,
 * nothing is evicted); 0 when one does not fit. */
s32 Tex_Reserve(u32 *handles, s32 n)
{
    u32 h;
    s32 i;
    TexRec *t;
    VramSlot *s;

    for (i = 0; i < n; i++) {
        h = *handles++;
        if ((h & 0xFFFF) != 0) {
            t = &gTexRecs[(h & 0xFFFF) - 1];
            if (t->resident == 0) {
                s = VramSlot_FindGap(t->blocks, t->align);
                if (s == (VramSlot *)-1) {
                    return 0;
                }
                if (VramSlot_Assign(s, t) == 0) {
                    return 0;
                }
            }
        }
        if ((h & 0xFFFF0000) >> 16 != 0) {
            t = &gPalRecs[((h & 0xFFFF0000) >> 16) - 1];
            if (t->resident == 0) {
                s = VramSlot_FindGap(t->blocks, t->align);
                if (s == (VramSlot *)-1) {
                    return 0;
                }
                if (VramSlot_Assign(s, t) == 0) {
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
    VramSlot *s = gVramSlotHead;
    VramSlot *next;
    u32 start;
    s32 end;

    if (s == NULL) {
        return NULL;
    }
    if (s->addr != gSysTexBlock && (u32)(s->addr - gSysTexBlock) >= blocks) {
        return NULL;
    }
    for (;;) {
        end = s->addr + s->blocks;
        next = s->next;
        start = ~(align - 1) & (end + (align - 1));
        if (next == NULL) {
            if (end >= VRAM_END) {
                return (VramSlot *)-1;
            }
            if (start + blocks >= VRAM_END) {
                return (VramSlot *)-1;
            }
            return s;
        }
        if (start < (u32)next->addr && (u32)(next->addr - start) >= blocks) {
            return s;
        }
        s = next;
    }
}

/* The slot from which t can take GS memory by evicting the textures there
 * (pass 0: only textures of mode 1 or 4, pass 1: any) that are not in
 * handles; NULL when there are no slots, 1 when t fits after the last one,
 * -1 when nothing can be evicted. */
VramSlot *VramSlot_FindVictim(TexRec *t, u32 *handles, s32 n)
{
    VramSlot *s2;
    VramSlot *first;
    s32 pass = 0;
    s32 i;
    VramSlot *s;
    VramSlot *last;
    u32 at;
    u32 h;

    for (;;) {
        s = gVramSlotHead;
        if (s == NULL) {
            return NULL;
        }
        do {
            if (pass == 0 && s->mode != 1 && s->mode != 4) {
                goto next;
            }
            for (i = 0; i < n; i++) {
                h = handles[i];
                if ((h & 0xFFFF) != 0 && &gTexRecs[(h & 0xFFFF) - 1] == s->tex) {
                    goto next;
                }
                if ((h & 0xFFFF0000) >> 16 != 0 && &gPalRecs[((h & 0xFFFF0000) >> 16) - 1] == s->tex) {
                    goto next;
                }
            }
            at = ~(t->align - 1) & (s->addr + (t->align - 1));
            if (s->addr != at) {
                goto next;
            }
            s2 = s->next;
            if (s2 == NULL) {
                if (VRAM_END - at < t->blocks) {
                    goto next;
                }
                return s;
            }
            first = s;
            while (s2->addr - at < t->blocks) {
                s = s2;
                if (pass == 0 && s2->mode != 1 && s2->mode != 4) {
                    goto next;
                }
                for (i = 0; i < n; i++) {
                    h = handles[i];
                    if ((h & 0xFFFF) != 0 && &gTexRecs[(h & 0xFFFF) - 1] == s2->tex) {
                        goto next;
                    }
                    if ((h & 0xFFFF0000) >> 16 != 0 && &gPalRecs[((h & 0xFFFF0000) >> 16) - 1] == s2->tex) {
                        goto next;
                    }
                }
                s2 = s2->next;
                if (s2 == NULL) {
                    if (VRAM_END - at < t->blocks) {
                        goto next;
                    }
                    return first;
                }
            }
            return first;
        next:
            last = s;
            s = s->next;
        } while (s != NULL);
        if (VRAM_END - (last->addr + last->blocks) >= t->blocks) {
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
    TexRec *t;
    s16 src;

    if (gSysNoDraw != 0) {
        return 0;
    }
    if (gFlipCountdown != 0) {
        return 0;
    }
    Dma_WaitIdle();
    t = &gScreenTex;
    if (fromTex == 0) {
        if (gSysCurBuf == 0) {
            src = gSysFbBlock0;
        } else {
            src = gSysFbBlock1;
        }
    } else {
        src = t->vram;
    }
    Dma_QueueStoreImage(t->mem, t->size, src, t->width / 64, t->psm, 0, 0, t->width, t->height, 1);
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
    TexRec *t = &gScreenTex;
    VramSlot *s;
    u128 *p;

    if (gScreenTexState == 2) {
        ScreenTex_SetRegs();
        return 1;
    }
    if (gScreenTexState == 1) {
        t->vram = gScreenTexVram;
        t->resident = 1;
    } else {
        s = VramSlot_FindGap(t->blocks, t->align);
        if ((s32)s < 0) {
            return 0;
        }
        if (VramSlot_Assign(s, t) == 0) {
            return 0;
        }
    }
    ScreenTex_Send(t);
    p = Packet_Alloc(Tex_UploadPacketSize(0), 16);
    Dma_BuildEndPacket(p, 1);
    DmaQueue_AddChain(0, (u32)p & 0x0FFFFFFF, (u32 *)p, &gDmaQueue);
    ScreenTex_SetRegs();
    return 1;
}

void ScreenTex_Release(void)
{
    if (gScreenTexVram != 0) {
        Vram_FreeBlock(gScreenTexVram & 0xFFFF);
    }
    gScreenTexState = 0;
}
