/*
 * The pool of stage objects (StageObj_*, include/stageobj.h): allocating,
 * freeing, updating and drawing them every frame.
 */
#include "common.h"
#include "em.h"
#include "stageobj.h"
#include "bgm.h"
#include "obj.h"
#include "libc.h"
/* The StageObj pool: 128 objects of 64 bytes, a free stack and a list of
 * the objects in use (include/stageobj.h). */
extern StageObj **gStageObjFreeTop;   /* the free stack */
extern s32 gStageObjFreeCount;          /* its number of free objects */
#define STAGEOBJ_COUNT 0x80             /* the pool's objects */
extern StageObj gStageObjs[STAGEOBJ_COUNT];
extern StageObj *gStageObjList;    /* the objects in use */
/* end of prototypes */

/* Clears the pool and stacks every object as free. */
void StageObj_InitPool(void)
{
    s32 i;
    StageObj *obj;

    memset(gStageObjs, 0, sizeof(gStageObjs));
    gStageObjList = NULL;
    obj = &gStageObjs[STAGEOBJ_COUNT - 1];
    gStageObjFreeTop = (StageObj **)gStageObjs;
    for (i = 0; i < STAGEOBJ_COUNT; i++, obj--) {
        *--gStageObjFreeTop = obj;
    }
    gScene.placed3 = 0;
    gStageObjFreeCount = STAGEOBJ_COUNT;
}

/* Frees every object in use but those marked keep. */
void StageObj_FreeUnkept(void)
{
    StageObj *obj = gStageObjList;
    StageObj *dead;

    while (obj != NULL) {
        if (obj->active != 0 && obj->keep == 0) {
            dead = obj;
            obj = obj->next;
            StageObj_Free(dead);
        } else {
            obj = obj->next;
        }
    }
}

/* Takes a free object with n blocks of the object buffer, at the head of the
 * list. */
StageObj *StageObj_Alloc(s32 blocks)
{
    s32 block;
    StageObj *obj;

    if ((block = ObjBuf_FindFree(blocks)) < 0) {
        return NULL;
    }
    if (gStageObjFreeCount == 0) {
        return NULL;
    }
    gStageObjFreeCount--;
    obj = *gStageObjFreeTop++;
    obj->prev = NULL;
    obj->next = gStageObjList;
    gStageObjList = obj;
    if (obj->next != NULL) {
        obj->next->prev = obj;
    }
    obj->active = 1;
    obj->keep = 0;
    obj->buf = ObjBuf_GetBlock(block);
    obj->blk = block;
    obj->nblk = blocks;
    ObjBuf_MarkUsed(block, blocks);
    return obj;
}

/* The same, at the tail of the list. */
StageObj *StageObj_AllocTail(s32 blocks)
{
    s32 block;
    StageObj *obj;
    StageObj *last;

    if ((block = ObjBuf_FindFree(blocks)) < 0) {
        return NULL;
    }
    if (gStageObjFreeCount == 0) {
        return NULL;
    }
    gStageObjFreeCount--;
    obj = *gStageObjFreeTop++;
    if (gStageObjList == NULL) {
        gStageObjList = obj;
        obj->prev = NULL;
    } else {
        last = gStageObjList;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = obj;
        obj->prev = last;
    }
    obj->next = NULL;
    obj->active = 1;
    obj->keep = 0;
    obj->buf = ObjBuf_GetBlock(block);
    obj->blk = block;
    obj->nblk = blocks;
    ObjBuf_MarkUsed(block, blocks);
    return obj;
}

/* Unlinks an object and gives it and its blocks back. */
void StageObj_Free(StageObj *obj)
{
    if (obj->prev == NULL) {
        gStageObjList = obj->next;
    } else {
        obj->prev->next = obj->next;
    }
    if (obj->next != NULL) {
        obj->next->prev = obj->prev;
    }
    gStageObjFreeCount++;
    *--gStageObjFreeTop = obj;
    ObjBuf_MarkFree(obj->blk, obj->nblk);
    memset(obj, 0, 8);
    obj->slot = 0;
    *(s32 *)&obj->node = 0;
}

/* Runs the update of every object in use. */
void StageObj_UpdateAll(void)
{
    StageObj *obj;

    for (obj = gStageObjList; obj != NULL; obj = obj->next) {
        if (obj->active != 0) {
            ((void (*)(StageObj *))obj->update)(obj);
        }
    }
}

/* Draws the visible objects, but kind 4 type 0 (drawn by StageObj_DrawKind4). */
void StageObj_DrawAll(void)
{
    StageObj *obj;

    for (obj = gStageObjList; obj != NULL; obj = obj->next) {
        if (obj->draw != NULL && obj->active != 0 && obj->visible != 0) {
            if (obj->kind != 4 || obj->type != 0) {
                ((void (*)(StageObj *))obj->draw)(obj);
            }
        }
    }
}

/* Draws the visible objects of kind 4 type 0. */
void StageObj_DrawKind4(void)
{
    StageObj *obj;

    for (obj = gStageObjList; obj != NULL; obj = obj->next) {
        if (obj->draw != NULL && obj->active != 0 && obj->visible != 0 && obj->kind == 4 && obj->type == 0) {
            ((void (*)(StageObj *))obj->draw)(obj);
        }
    }
}
