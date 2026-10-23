/*
 * The object pools: Obj (128, gObjPool) and ObjAttach (64, gObjAttachPool), each a free list
 * (a stack) and a doubly linked list of the objects in use, and the 256 blocks of 512 bytes
 * (gObjBufBlocks) an object takes for its work. Obj_InitAll and Obj_FreeAll run with the
 * scene; the update and draw walks run every frame.
 */
#include "common.h"
#include "sprite.h"
#include "chat_log.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "syssettings.h"
#include "objhead.h"
#include "objattach.h"
#include "obj.h"
#include "window.h"
#include "render_state.h"
#include "area.h"
#include "hud.h"
#include "adx.h"
#include "sound.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "lobby_state.h"
#include "area_draw.h"
#include "kbd.h"
#include "save.h"
#include "text.h"
#include "village_cond.h"
#include "libc.h"

#define OBJ_POOL_SIZE       128     /* gObjPool's objects */
#define OBJ_ATTACH_POOL_SIZE 64     /* gObjAttachPool's */
#define OBJ_BUF_BLOCKS      256     /* gObjBufBlocks' blocks */
#define OBJ_BUF_BLOCK_SHIFT 9       /* of 512 bytes */
extern u8 gObjBufUsed[OBJ_BUF_BLOCKS];  /* one per block of gObjBufBlocks: in use */
extern u8 *gObjBufBlocks;          /* OBJ_BUF_BLOCKS blocks of 512 bytes */
extern Obj **gObjFreeList;        /* free list (a stack) of the first pool */
extern s32 gObjFreeCount;          /* its number of free objects */
extern Obj gObjPool[OBJ_POOL_SIZE];
extern Obj *gObjList;         /* the objects in use */
extern ObjAttach **gObjAttachFreeList;
extern s32 gObjAttachFreeCount;
extern ObjAttach gObjAttachPool[OBJ_ATTACH_POOL_SIZE];
/* gObjAttachPool[63], declared as an object of its own: the linker pairs
 * the two halves of the address gObjAttachPool + 0x342C wrongly when they
 * fall between the halves of gObjAttachPool's own address. */
extern ObjAttach gObjAttachPoolLast;
extern ObjAttach *gObjAttachList;
extern Em gEms[];

/* Empties the three object pools (Obj, StageObj, ObjAttach) and frees every work block. */
void Obj_InitAll(void)
{
    Obj_InitPool();
    StageObj_InitPool();
    ObjAttach_InitPool();
    ObjBuf_MarkFree(0, OBJ_BUF_BLOCKS);
}

/* Frees the objects of the three pools that are not marked to keep. */
void Obj_FreeAll(void)
{
    Obj_FreeUnkept();
    StageObj_FreeUnkept();
    ObjAttach_FreeUnkept();
}

/*
 * Finds n free work blocks in a row: returns the first one's index, 0 for
 * n 0, or -1. The search starts at a free block and, when the n blocks from
 * there are not all free, goes on n blocks further.
 * Kept for the match: j, the sum loop's counter, also holds the end index
 * (i + n) for the bound test, as the game's code reuses the one register;
 * with a variable of its own the compiler swaps two registers. The loop is
 * a goto, so that gObjBufUsed's address is loaded on each pass, and the
 * search is an if around a do-while, which gives its test at the top.
 */
s32 ObjBuf_FindFree(s32 n)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (n == 0) {
        return 0;
    }
loop:
        if (i < OBJ_BUF_BLOCKS) {
            do {
                if (gObjBufUsed[i] == 0) {
                    break;
                }
                i++;
            } while (i < OBJ_BUF_BLOCKS);
        }
        if (i >= OBJ_BUF_BLOCKS) {
            return -1;
        }
        j = i + n;
        if (j > OBJ_BUF_BLOCKS) {
            return -1;
        }
        for (j = sum = 0; j < n; j++) {
            sum += gObjBufUsed[i + j];
        }
        if (sum != 0) {
            i += n;
            goto loop;
        }
        return i;
}

/* The address of work block blk. */
u8 *ObjBuf_GetBlock(s32 blk)
{
    return (u8 *)((s32)gObjBufBlocks + (blk << OBJ_BUF_BLOCK_SHIFT));
}

/* Marks the n work blocks from start as in use. */
void ObjBuf_MarkUsed(s32 start, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gObjBufUsed[start + i] = 1;
    }
}

/* Marks the n work blocks from start as free. */
void ObjBuf_MarkFree(s32 start, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gObjBufUsed[start + i] = 0;
    }
}

/* Empties the Obj pool: every object goes on the free list, in order. */
void Obj_InitPool(void)
{
    s32 i;
    Obj *obj;

    memset(gObjPool, 0, sizeof(gObjPool));
    gObjList = NULL;
    obj = &gObjPool[OBJ_POOL_SIZE - 1];
    gObjFreeList = (Obj **)gObjPool;
    for (i = 0; i < OBJ_POOL_SIZE; i++, obj--) {
        *--gObjFreeList = obj;
    }
    gObjFreeCount = OBJ_POOL_SIZE;
}

/* Frees the active objects not marked to keep. */
void Obj_FreeUnkept(void)
{
    Obj *obj = gObjList;
    Obj *dead;

    while (obj != NULL) {
        if (obj->active != 0 && obj->keep == 0) {
            dead = obj;
            obj = obj->next;
            Obj_Free(dead);
        } else {
            obj = obj->next;
        }
    }
}

/*
 * Takes an object from the pool with n work blocks (none for 0: its buf is
 * then -1), puts it first in the list in use and marks it active. Returns
 * NULL when the pool or the blocks run out.
 */
Obj *Obj_Alloc(s32 n)
{
    s32 blk;
    Obj *obj;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gObjFreeCount == 0) {
        return NULL;
    }
    gObjFreeCount--;
    obj = *gObjFreeList++;
    obj->prev = NULL;
    obj->next = gObjList;
    gObjList = obj;
    if (obj->next != NULL) {
        obj->next->prev = obj;
    }
    obj->active = 1;
    obj->buf = (n > 0) ? ObjBuf_GetBlock(blk) : (u8 *)-1;
    obj->blk = blk;
    obj->nblk = n;
    obj->keep = 0;
    ObjBuf_MarkUsed(blk, n);
    return obj;
}

/* Returns an object to the pool and frees its work blocks; its first 8
 * bytes (the flags and the list links) are cleared. */
void Obj_Free(Obj *obj)
{
    if (obj->prev == NULL) {
        gObjList = obj->next;
    } else {
        obj->prev->next = obj->next;
    }
    if (obj->next != NULL) {
        obj->next->prev = obj->prev;
    }
    gObjFreeCount++;
    *--gObjFreeList = obj;
    if (obj->nblk > 0) {
        ObjBuf_MarkFree(obj->blk, obj->nblk);
    }
    memset(obj, 0, 8);
}

/* Runs every active object's update. */
void Obj_UpdateAll(void)
{
    Obj *obj;

    for (obj = gObjList; obj != NULL; obj = obj->next) {
        if (obj->active != 0) {
            obj->update(obj);
        }
    }
}

/* Runs the draw function of every active, visible object that has one. */
void Obj_DrawAll(void)
{
    Obj *obj;

    for (obj = gObjList; obj != NULL; obj = obj->next) {
        if (obj->active != 0 && obj->visible != 0) {
            if (obj->draw != NULL) {
                obj->draw(obj);
            }
        }
    }
}

/* Empties the ObjAttach pool, as Obj_InitPool does, and zeroes the
 * scene's counts of placed items (placed1, placed2). */
void ObjAttach_InitPool(void)
{
    s32 i;
    ObjAttach *att;

    memset(gObjAttachPool, 0, sizeof(gObjAttachPool));
    gObjAttachList = NULL;
    att = &gObjAttachPoolLast;
    gObjAttachFreeList = (ObjAttach **)gObjAttachPool;
    for (i = 0; i < OBJ_ATTACH_POOL_SIZE; i++, att--) {
        *--gObjAttachFreeList = att;
    }
    gScene.placed1 = 0;
    gObjAttachFreeCount = OBJ_ATTACH_POOL_SIZE;
    gScene.placed2 = 0;
}

/* Frees the active attachments not marked to keep. */
void ObjAttach_FreeUnkept(void)
{
    ObjAttach *att = gObjAttachList;
    ObjAttach *dead;

    while (att != NULL) {
        if (att->active != 0 && att->keep == 0) {
            dead = att;
            att = att->next;
            ObjAttach_Free(dead);
        } else {
            att = att->next;
        }
    }
}

/* Clears an attachment's hit state, keep flag and side, and sets its hit
 * scale to 1 and its power to 0. */
void ObjAttach_Reset(ObjAttach *att)
{
    att->wasHit = 0;
    att->hitBy = 0;
    att->keep = 0;
    att->side = 0;
    att->hitScale = 1.0f;
    att->powerPct = 0;
}

/* Takes an attachment from the pool with n work blocks, as Obj_Alloc does,
 * and resets it (ObjAttach_Reset). Returns NULL when the pool or the blocks
 * run out. */
ObjAttach *ObjAttach_Alloc(s32 n)
{
    s32 blk;
    ObjAttach *att;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gObjAttachFreeCount == 0) {
        return NULL;
    }
    gObjAttachFreeCount--;
    att = *gObjAttachFreeList++;
    att->prev = NULL;
    att->next = gObjAttachList;
    gObjAttachList = att;
    if (att->next != NULL) {
        att->next->prev = att;
    }
    att->active = 1;
    att->buf = ObjBuf_GetBlock(blk);
    att->blk = blk;
    att->nblk = n;
    ObjBuf_MarkUsed(blk, n);
    ObjAttach_Reset(att);
    return att;
}

/* Returns an attachment to the pool and frees its work blocks; clears its
 * first 12 bytes, its attack flags, hit and hurt lists and draw slots, and
 * the low bits of hitFx. */
void ObjAttach_Free(ObjAttach *att)
{
    if (att->prev == NULL) {
        gObjAttachList = att->next;
    } else {
        att->prev->next = att->next;
    }
    if (att->next != NULL) {
        att->next->prev = att->prev;
    }
    gObjAttachFreeCount++;
    *--gObjAttachFreeList = att;
    ObjBuf_MarkFree(att->blk, att->nblk);
    memset(att, 0, 12);
    att->attackFlags = 0;
    att->hitList = 0;
    att->hurtList = 0;
    att->drawSlot = 0;
    att->drawSlotBuf = 0;
    att->hitFx &= 0x3F;
}

/* Runs every active attachment's update, or counts down its delay first. */
void ObjAttach_UpdateAll(void)
{
    ObjAttach *att;

    for (att = gObjAttachList; att != NULL; att = att->next) {
        if (att->active != 0) {
            if (att->delay != 0) {
                att->delay--;
            } else {
                att->update(att);
            }
        }
    }
}

/* Runs the draw function of every active, visible attachment whose owner
 * (the player or the monster it follows) is visible. */
void ObjAttach_DrawAll(void)
{
    ObjAttach *att;

    for (att = gObjAttachList; att != NULL; att = att->next) {
        if (att->isEm == 0) {
            Player *pl = &gPlayers[att->idx];

            if (att->active != 0 && att->visible != 0 && pl->visible != 0) {
                if (att->draw != NULL) {
                    att->draw(att);
                }
            }
        } else {
            Em *em = &gEms[att->idx];

            if (att->active != 0 && att->visible != 0 && em->visible != 0) {
                if (att->draw != NULL) {
                    att->draw(att);
                }
            }
        }
    }
}

