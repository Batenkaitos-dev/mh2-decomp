/*
 * Five lists of 32 EmObj pointers, each with a count of the entries used:
 * gAreaObjList (the monsters' goal search: baits and the like), gSightBlockObjs
 * (zones that block a monster's sight), gFlashObjs (flashes), gEmSenseObjs (the
 * objects monsters sense, Em_SenseObjects) and gUnusedObjs (only cleared). Add
 * takes the first empty entry and returns 1, or 0 when the list is full; Remove
 * empties every entry holding the object; the two Tick functions count each
 * object's timer down once a frame (Field_UpdateAll) and remove it at 0.
 */
#include "common.h"
#include "em.h"
extern EmObj *gEmSenseObjs[32];
extern EmObj *gUnusedObjs[32];
extern EmObj *gFlashObjs[32];
extern EmObj *gSightBlockObjs[32];
extern EmObj *gAreaObjList[32];
extern s8 gEmSenseObjCount;           /* entries used in gEmSenseObjs */
extern s8 gUnusedObjCount;           /* ... in gUnusedObjs (only UnusedObjs_Clear touches the two) */
extern s8 gFlashObjCount;           /* ... in gFlashObjs */
extern s8 gSightBlockObjCount;           /* ... in gSightBlockObjs */
extern s8 gAreaObjCount;           /* ... in gAreaObjList */
/* end of prototypes */

/* Empties gAreaObjList (despite its name, it only clears). */
void ObjList_Update(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gAreaObjList[i] = NULL;
    }
    gAreaObjCount = 0;
}

/* Adds obj to gAreaObjList; 0 when the list is full. */
s32 ObjList_Add(EmObj *obj)
{
    s32 i;

    if (gAreaObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gAreaObjList[i] == NULL) {
            gAreaObjList[i] = obj;
            gAreaObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gAreaObjList. */
void ObjList_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gAreaObjList[i] == obj) {
            gAreaObjList[i] = NULL;
            gAreaObjCount--;
        }
    }
}

/* Empties gSightBlockObjs. */
void SightBlock_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gSightBlockObjs[i] = NULL;
    }
    gSightBlockObjCount = 0;
}

/* Adds obj to gSightBlockObjs; 0 when the list is full. */
s32 SightBlock_Add(EmObj *obj)
{
    s32 i;

    if (gSightBlockObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gSightBlockObjs[i] == NULL) {
            gSightBlockObjs[i] = obj;
            gSightBlockObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gSightBlockObjs. */
void SightBlock_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gSightBlockObjs[i] == obj) {
            gSightBlockObjs[i] = NULL;
            gSightBlockObjCount--;
        }
    }
}

/* Counts down the timer of each object of gSightBlockObjs and removes those that reach 0. */
void SightBlock_Tick(void)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gSightBlockObjs[i];
        if (obj != NULL && --obj->timer <= 0) {
            SightBlock_Remove(obj);
        }
    }
}

/* Empties gFlashObjs. */
void FlashObj_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gFlashObjs[i] = NULL;
    }
    gFlashObjCount = 0;
}

/* Adds obj to gFlashObjs, first alerting the monsters to it (EmObj_AlertMonsters); 0 when the list is full. */
s32 FlashObj_Add(EmObj *obj)
{
    s32 i;

    if (gFlashObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gFlashObjs[i] == NULL) {
            EmObj_AlertMonsters((EmObj *)obj);
            gFlashObjs[i] = obj;
            gFlashObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gFlashObjs. */
void FlashObj_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gFlashObjs[i] == obj) {
            gFlashObjs[i] = NULL;
            gFlashObjCount--;
        }
    }
}

/* Counts down the timer of each object of gFlashObjs and removes those that reach 0. */
void FlashObj_Tick(void)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gFlashObjs[i];
        if (obj != NULL && --obj->timer <= 0) {
            FlashObj_Remove(obj);
        }
    }
}

/* Empties gUnusedObjs (seen: no other code reads or writes that list). */
void UnusedObjs_Clear(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gUnusedObjs[i] = NULL;
    }
    gUnusedObjCount = 0;
}

/* Empties gEmSenseObjs. */
void EmSenseObjs_Clear(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gEmSenseObjs[i] = NULL;
    }
    gEmSenseObjCount = 0;
}

/* Adds obj to gEmSenseObjs; 0 when the list is full. */
s32 EmSenseObjs_Add(EmObj *obj)
{
    s32 i;

    if (gEmSenseObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gEmSenseObjs[i] == NULL) {
            gEmSenseObjs[i] = obj;
            gEmSenseObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gEmSenseObjs. */
void EmSenseObjs_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gEmSenseObjs[i] == obj) {
            gEmSenseObjs[i] = NULL;
            gEmSenseObjCount--;
        }
    }
}

