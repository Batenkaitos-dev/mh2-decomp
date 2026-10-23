/*
 * The textures loaded by id: TEX_ID_SLOTS slots (gTexIds, gTexIdHandles)
 * that Tex_LoadById fills from a texture file and Tex_FindById looks up,
 * each marked in use for the frame; Tex_ReleaseIdSlots frees them. Then
 * the common texture (Tex_LoadCommon: its texture and CLUT handles go in
 * gLoadLettersTex). The data: the texture file tables (the system's, the
 * monsters' slots', the equipment models', the stages', the ids' files)
 * and the common texture's image headers.
 */

#include "common.h"
#include "model.h"
#include "texture.h"
#include "tex_files.h"
#include "file_load.h"
#include "packet_tim2.h"
#include "rand.h"
#include "packs.h"

void * memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, const void *src, unsigned long n);

extern s8 gTexIds[TEX_ID_SLOTS];
extern s32 gTexIdHandles[TEX_ID_SLOTS];

/* Empties the TEX_ID_SLOTS slots of the textures loaded by id, without
 * freeing them (Tex_ReleaseIdSlots frees them). */
void Tex_ClearIdSlots(void)
{
    memset(gTexIdHandles, 0, sizeof(gTexIdHandles));
    memset(gTexIds, -1, sizeof(gTexIds));
}

/* Frees the TEX_ID_SLOTS texture handles of gTexIdHandles (a texture's
 * handle | its CLUT's << 16) and empties their slots. */
void Tex_ReleaseIdSlots(void)
{
    s32 i;
    s32 *handle;
    s8 *id;

    i = 0;
    handle = gTexIdHandles;
    id = gTexIds;
    do {
        s32 value = *handle;

        if (value != 0) {
            u32 tex = value & 0xFFFF;
            u32 clut = (u32)(value & 0xFFFF0000) >> 16;

            if (tex != 0) {
                flReleaseTextureHandle(tex);
            }
            if (clut != 0) {
                flReleasePaletteHandle(clut);
            }
        }
        *handle = 0;
        i++;
        *id = -1;
        handle++;
        id++;
    } while (i < TEX_ID_SLOTS);
}

extern s16 gTexByIdFiles[];

/* The texture handle loaded with id, marked in use this frame; loads it
 * first (file gTexByIdFiles[id] through buf, kind gTexCreateKinds[kind])
 * into the first free slot when it is not there. */
s32 Tex_LoadById(s32 id, s32 buf, s32 kind)
{
    s32 i;
    s8 *slotId;
    u32 *handle;

    for (i = 0, slotId = gTexIds; i < TEX_ID_SLOTS; i++, slotId++) {
        if (*slotId == -1) {
            break;
        }
        if (*slotId == (s8)id) {
            handle = (u32 *)&gTexIdHandles[i];
            if (*handle != 0) {
                Tex_Use(1, handle);
            }
            return *handle;
        }
    }
    LoadCompressedFile((u8 *)buf, gTexByIdFiles[id]);
    handle = (u32 *)&gTexIdHandles[i];
    *handle = Tex_CreateFromTim2((u8 *)buf, gTexCreateKinds[kind]);
    gTexIds[i] = id;
    return *handle;
}

/* The texture handle loaded with id (gTexIds, -1 ends the list), marked in
 * use this frame; 0 when there is none. buf and kind are not used. */
s32 Tex_FindById(s8 id, u8 *buf, s32 kind)
{
    s32 i;
    s8 *slotId;

    for (i = 0, slotId = gTexIds; i < TEX_ID_SLOTS; i++, slotId++) {
        if (*slotId == -1) {
            break;
        }
        if (*slotId == id) {
            u32 *handle = (u32 *)&gTexIdHandles[i];

            if (*handle != 0) {
                Tex_Use(1, handle);
            }
            return *handle;
        }
    }
    return 0;
}

/* 1 when texture handle h is one of the textures loaded by id. */
s32 Tex_IsIdSlot(s32 handle)
{
    s32 i;
    s32 *slot;

    for (i = 0, slot = gTexIdHandles; i < TEX_ID_SLOTS; i++, slot++) {
        if (handle == *slot) {
            return 1;
        }
    }
    return 0;
}

extern u8 *gLoadBuf;
extern s32 gLoadLettersTex;
extern u8 gCommonTexImage[];
extern u8 *gCommonTexData[];
extern u8 gCommonClutImage[];
extern u8 *gCommonClutData[];

/* Loads the common texture file (gSystemTexFiles[0]) through gLoadBuf and sends its texture and CLUT; gLoadLettersTex = clut << 16 | texture. */
void Tex_LoadCommon(void)
{
    u8 *buf;
    u32 tex;
    u32 clut;

    buf = gLoadBuf;

    if (gLoadLettersTex != 0) {
        Debug_Message("TEXTURE OVERWRITE", 8, 0, 0);
    }
    LoadCompressedFile(buf, gSystemTexFiles[0]);
    Pack_Count((s32 *)buf);
    memcpy(buf, Pack_GetPart(buf, 2), 0x10440);
    gCommonTexData[0] = buf + 64;
    clut = 0;
    tex = flCreateTextureHandle((Image *) gCommonTexImage, TEX_MODE_ON_DEMAND);
    if (tex != 0) {
        gCommonClutData[0] = buf + 0x10040;
        clut = flCreatePaletteHandle((Image *) gCommonClutImage, TEX_MODE_ON_DEMAND);
    }
    gLoadLettersTex = clut << 16 | tex;
}

/* This file's initialised data. */
s32 gSystemTexFiles[6] = {
#include "sub_main/0042F1A0.inc"
};
u32 gSystemExtraTexFiles[2] = {
#include "sub_main/0042F1B8.inc"
};
s32 gCommonModelTexFiles[2] = {
#include "sub_main/0042F1C0.inc"
};
u32 D_0042F1C8[2] = {
#include "sub_main/0042F1C8.inc"
};
u32 D_0042F1D0[4] = {
#include "sub_main/0042F1D0.inc"
};
u32 gEmSlotTexFiles[20] = {
#include "sub_main/0042F1E0.inc"
};
u32 gSubModelPairTexFiles[4] = {
#include "sub_main/0042F230.inc"
};
u32 gEquipModelTexFilesM[24] = {
#include "sub_main/0042F240.inc"
};
u32 gEquipModelTexFilesF[24] = {
#include "sub_main/0042F2A0.inc"
};
s16 gTexFileIds[8] = {
#include "sub_main/0042F300.inc"
};
s16 gTexFileIds8[8] = {
#include "sub_main/0042F310.inc"
};
s16 gStageTexFileIds[32] = {
#include "sub_main/0042F320.inc"
};
s16 gTexByIdFiles[8] = {
#include "sub_main/0042F360.inc"
};
u32 gOnlineVillagePackFiles[2] = {
#include "sub_main/0042F370.inc"
};
u32 gOnlineVillageSubModelFiles[2] = {
#include "sub_main/0042F378.inc"
};
u32 gOnlineVillageSubModelFileSeason1[2] = {
#include "sub_main/0042F380.inc"
};
u32 gArea84PackFiles[2] = {
#include "sub_main/0042F388.inc"
};
u32 gFarmFacilityModelFiles[10] = {
#include "sub_main/0042F390.inc"
};
s32 gTexCreateKinds[2] = {
#include "sub_main/0042F3B8.inc"
};
u8 gMapPackTexIndex[32] = {
#include "sub_main/0042F3C0.inc"
};
s8 gCommonPackTexSlots[8] = {
#include "sub_main/0042F3E0.inc"
};
s8 gSystemPackTexSlots[8] = {
#include "sub_main/0042F3E8.inc"
};
s8 gSystemPackTexSlotsB[16] = {
#include "sub_main/0042F3F0.inc"
};
u8 gCommonTexImage[16] = {
#include "sub_main/0042F400.inc"
};
u8 *gCommonTexData[16] = {
#include "sub_main/0042F410.inc"
};
u8 gCommonClutImage[16] = {
#include "sub_main/0042F450.inc"
};
u8 *gCommonClutData[16] = {
#include "sub_main/0042F460.inc"
};
