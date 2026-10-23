/*
 * Placing textures in GS memory.
 *
 * GS memory past gSysTexBlock is shared out through the VramSlot list
 * (gVramSlotHead, ordered by address; the slots themselves are allocated
 * and linked by the VramSlot_Alloc/Insert/Free helpers, and Vram_Init
 * clears them at the display's set-up). A texture or
 * palette record (TexRec) goes into a gap when one fits (VramSlot_FindGap,
 * VramSlot_Assign), or else takes the place of textures the frame does not
 * use (VramSlot_FindVictim, VramSlot_Replace). Tex_Upload places a record
 * when it is created, Tex_MakeResident when a draw needs it.
 */
#include "common.h"
#include "dma.h"
#include "texture.h"
#include "mem_util.h"

#define VRAM_END 0x4000     /* GS memory, 256-byte blocks */

extern s32 gSysTexBlock;      /* gSysWork: first GS block for textures */

extern s32 gGsBoundTexture;

/*
 * Clears the four GS memory blocks (gVramBlocks) and the VRAM slots
 * (gVramSlots, gVramSlotHead, gVramSlotCount), and sets the texture handle
 * the render state last bound to 1. The display set-up calls it.
 */
void Vram_Init(void)
{
    s32 i;
    VramBlock *block;
    VramSlot *slot;

    gVramBlockCount = 0;
    gGsBoundTexture = 1;
    for (i = 0, block = gVramBlocks; i < 4; i++, block++) {
        Mem_Fill(block, 0, sizeof(VramBlock));
    }
    gVramSlotCount = 0;
    gVramSlotHead = NULL;
    for (i = 0, slot = gVramSlots; i < VRAM_SLOT_MAX; i++, slot++) {
        Mem_Fill(slot, 0, sizeof(VramSlot));
    }
}

/*
 * Takes the first free slot of gVramSlots (addr 0) and counts it in
 * gVramSlotCount; returns it, NULL when all VRAM_SLOT_MAX are in use.
 * table and count are not used (callers pass gVramSlots, VRAM_SLOT_MAX).
 */
VramSlot *VramSlot_Alloc(VramSlot *table, s32 count)
{
    s32 i;
    VramSlot *slot;

    for (i = 0, slot = gVramSlots; i < VRAM_SLOT_MAX; i++, slot++) {
        if (slot->addr == 0) {
            gVramSlotCount++;
            return &gVramSlots[i];
        }
    }
    return NULL;
}

/* Unlinks and clears a slot; its texture loses its GS memory. */
void VramSlot_Free(VramSlot *slot)
{
    VramSlot *prev = slot->prev;
    VramSlot *next = slot->next;

    if (prev != NULL) {
        prev->next = next;
    } else if (next != NULL) {
        gVramSlotHead = next;
    } else {
        gVramSlotHead = NULL;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    if (slot->tex != NULL) {
        slot->tex->resident = 0;
        slot->tex->mode = TEX_MODE_ON_DEMAND;
    }
    Mem_Fill(slot, 0, sizeof(VramSlot));
    gVramSlotCount--;
}

/* Links slot into the slot list after pos (at the head when pos is NULL). */
void VramSlot_Insert(VramSlot *pos, VramSlot *slot)
{
    VramSlot *next = NULL;

    if (pos != NULL) {
        next = pos->next;
        pos->next = slot;
    } else if (gVramSlotHead == NULL) {
        gVramSlotHead = slot;
    } else {
        next = gVramSlotHead;
        gVramSlotHead = slot;
    }
    slot->prev = pos;
    slot->next = next;
    if (next != NULL) {
        next->prev = slot;
    }
}

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
 * and the texture the slot held is evicted (resident 0, TEX_MODE_ON_DEMAND). Returns
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
        slot->tex->mode = TEX_MODE_ON_DEMAND;
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
        if (TEX_HANDLE_TEX(handle) != 0) {
            texRec = &gTexRecs[TEX_HANDLE_TEX(handle) - 1];
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
        if (TEX_HANDLE_PAL(handle) != 0) {
            texRec = &gPalRecs[TEX_HANDLE_PAL(handle) - 1];
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
 * there that are not among the handleCount handles (pass 0: only textures
 * drawn on demand, TEX_MODE_DEFERRED or TEX_MODE_ON_DEMAND; pass 1: any).
 * The slot must start at an address aligned for texRec. Returns NULL when
 * there are no slots, 1 when texRec fits after the last one, -1 when
 * nothing can be evicted. Each goto leaves the checks, loops included,
 * for the next slot.
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
            if (pass == 0 && slot->mode != TEX_MODE_DEFERRED && slot->mode != TEX_MODE_ON_DEMAND) {
                goto next;
            }
            for (i = 0; i < handleCount; i++) {
                handle = handles[i];
                if (TEX_HANDLE_TEX(handle) != 0 && &gTexRecs[TEX_HANDLE_TEX(handle) - 1] == slot->tex) {
                    goto next;
                }
                if (TEX_HANDLE_PAL(handle) != 0 && &gPalRecs[TEX_HANDLE_PAL(handle) - 1] == slot->tex) {
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
                if (pass == 0 && runSlot->mode != TEX_MODE_DEFERRED && runSlot->mode != TEX_MODE_ON_DEMAND) {
                    goto next;
                }
                for (i = 0; i < handleCount; i++) {
                    handle = handles[i];
                    if (TEX_HANDLE_TEX(handle) != 0 && &gTexRecs[TEX_HANDLE_TEX(handle) - 1] == runSlot->tex) {
                        goto next;
                    }
                    if (TEX_HANDLE_PAL(handle) != 0 && &gPalRecs[TEX_HANDLE_PAL(handle) - 1] == runSlot->tex) {
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

