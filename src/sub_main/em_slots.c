/*
 * The monster slots (gEms, EM_COUNT Ems): clearing them, taking a free one
 * and initialising it, giving a monster its model (a model slot, a copy of
 * the scene's model header and two sets of bone slots), and freeing them.
 */
#include "common.h"
#include "player.h"
#include "em.h"
#include "draw_slots.h"
extern u8 gModelUsed[];
void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, void *src, u32 n);
/* end of prototypes */

/* Clears the monster slots (only their first 2416 bytes, a Player's size). */
void EmSlot_ClearAll(void)
{
    s16 i;
    Em *em = gEms;

    for (i = 0; i < EM_COUNT; i++, em++) {
        memset(em, 0, sizeof(Player));
    }
}

/* Frees a monster slot: its pool entry, its model slots, and its first 18
 * bytes. */
void EmSlot_Free(Em *em)
{
    s16 i;

    if (em->drawSlot != 0) {
        DrawSlot_Free(em->drawSlotId);
    }
    if (em->model != NULL) {
        for (i = em->modelFirst; i < em->modelFirst + em->modelCount; i++) {
            Model_FreeCopy(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
            em->model = NULL;
            em->modelFirst = 0;
            em->modelCount = 0;
        }
    }
    memset(em, 0, 18);
}

/* Frees every monster slot in use (EmSlot_Free). */
void EmSlot_FreeAll(void)
{
    s16 i;
    Em *em = gEms;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0) {
            EmSlot_Free(em);
        }
    }
}

/* Gives a monster a model: a model slot, a copy of the scene's model header
 * e->modelId, and two sets of bone slots. */
void EmSlot_SetupModel(Em *em)
{
    s32 j;
    s32 bone;
    Model *emModel;
    s32 modelIndex;
    SceneState *scene = &gScene;
    s32 firstBone;

    modelIndex = Model_Alloc(1);
    if (modelIndex >= 0) {
        em->modelFirst = modelIndex;
        em->modelCount = 1;
        em->model = Model_Get(modelIndex);
        Model_Reserve(modelIndex, 1);
        emModel = Model_Get(modelIndex);
        memcpy(emModel, scene->models[em->modelSlot], sizeof(Model));
        emModel->active = 1;
        emModel->boneKind = 2;
        emModel->isCopy = 1;
        firstBone = Bone_Alloc(emModel->boneCount);
        emModel->boneFirst = firstBone;
        emModel->bones = Bone_Get(firstBone);
        emModel->bones2 = Bone_Get(firstBone + emModel->boneCount / 2);
        Bone_Reserve(firstBone, emModel->boneCount);
        for (bone = 0, j = 0; j < emModel->partCount; j++) {
            Bone_InitTreeRoot(&emModel->bones[bone], emModel->partSkels[j]);
            Bone_InitTreeRoot(&emModel->bones2[bone], emModel->partSkels[j]);
            emModel->partBones[j] = &emModel->bones[bone];
            emModel->partBones2[j] = &emModel->bones2[bone];
            bone += emModel->bones->count;
        }
    }
}

/*
 * Takes a free monster slot and initialises it. The original places the
 * last two stores after the loop and jumps back to the return; the gotos
 * reproduce that block order.
 */
Em *EmSlot_Alloc(void)
{
    u32 i;
    Em *em = gEms;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active == 0) {
            memset(em, 0, sizeof(Em));
            em->slotNo = i;
            em->active = 1;
            em->isEm = 1;
            em->mineSlot = -1;
            em->index = i;
            em->model = NULL;
            em->remote = gScene.localPlayer;
            em->alpha = 1.0f;
            em->partMask = 0;
            goto init;
        done:
            return em;
        }
    }
    return NULL;
init:
    em->hitIds[i] = 0;
    em->hitIdNext = 0;
    goto done;
}

