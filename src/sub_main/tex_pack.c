/*
 * The texture slots (gTexHandles) and the texture packs.
 *
 * Tex_ReleaseAll and Tex_ReleaseRange release the slots' texture and
 * palette handles.
 *
 * Texture packs: a pack file holds TIM2
 * pictures (Pack_Count, Pack_GetPart), and registering it makes a texture
 * handle of each, in consecutive slots of gTexHandles from a given slot,
 * with one of gTexCreateKinds' creation modes. The packs are loaded from a
 * file (LoadCompressedFile, LoadFile) or decompressed from memory. A
 * model's pack (TexPackHead, include/texture.h) registers its first
 * picture and, when it has two, a second texture by id (Tex_LoadById or
 * Tex_FindById). The common pack fills the fixed slots of
 * gCommonPackTexSlots.
 *
 * The game compiled this unit at optimization level 4 (Tex_RegisterPack
 * matches only there); the next unit, at level 3.
 */
#include "common.h"
#include "syssettings.h"
#include "texture.h"
#include "model.h"
#include "file_load.h"
#include "tex_files.h"
#include "packs.h"

extern u8 *gLoadBuf;
extern u8 *gReadBuf;

s32 Tex_FindById(s32 a, u8 *buf, s32 kind); /* unit's own declaration: declared as this file's callers use it; with the definition's s32 (s8 id, u8 *buf, s32 kind), Tex_RegisterTim2Pair compiles differently */
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);

extern s32 gTexHandles[];
extern s8 gCommonPackTexSlots[];

#pragma optimization_level 4

/* Releases the texture and palette handles of all the texture slots but 0, 328 and
 * TEX_SLOT_ICONS, which Tex_LoadFile loads once and keeps (a slot
 * whose handle Tex_IsIdSlot frees is just cleared). */
void Tex_ReleaseAll(void)
{
    s32 i;
    u32 *handle;
    u32 palHandle;

    for (i = 0, handle = (u32 *)gTexHandles; i < TEX_HANDLE_COUNT; i++, handle++) {
        if (*handle == 0) {
            continue;
        }
        if (Tex_IsIdSlot(*handle) == 1) {
            *handle = 0;
            continue;
        }
        switch (i) {
        case 0:
        case 328:
        case TEX_SLOT_ICONS:
            continue;
        }
        if ((palHandle = *handle & 0xFFFF0000) != 0 && flReleasePaletteHandle(palHandle >> 16) == 1) {
            *handle = *(u16 *)handle;
        }
        if (*(u16 *)handle != 0 && flReleaseTextureHandle(*(u16 *)handle) == 1) {
            *handle &= 0xFFFF0000;
        }
    }
    Tex_ReleaseIdSlots();
}

/* Releases the texture and palette handles of the n texture slots from start (as Tex_ReleaseAll). */
void Tex_ReleaseRange(s32 start, s32 n)
{
    s32 i;
    u32 *handle;
    u32 palHandle;

    i = 0;
    if (i < n) {
        handle = (u32 *)&gTexHandles[start];
        do {
            if (*handle != 0) {
                if (Tex_IsIdSlot(*handle) == 1) {
                    *handle = 0;
                } else {
                    if ((palHandle = *handle & 0xFFFF0000) != 0 && flReleasePaletteHandle(palHandle >> 16) == 1) {
                        *handle = *(u16 *)handle;
                    }
                    if (*(u16 *)handle != 0 && flReleaseTextureHandle(*(u16 *)handle) == 1) {
                        *handle &= 0xFFFF0000;
                    }
                }
            }
            i++;
            handle++;
        } while (i < n);
    }
}

/* Releases n texture handles from slot start. */
void Tex_FreeRange(s32 start, s32 n)
{
    Tex_ReleaseRange(start, n);
}

/* Makes a texture of each of the pack's pictures (TEX_PACK_MAX at most)
 * in the slots from `slot`, with creation mode gTexCreateKinds[kind].
 * Returns the number made. */
s32 Tex_RegisterPack(s32 slot, s32 kind, u8 *buf)
{
    s32 i;
    s32 n;

    n = Pack_Count((s32 *) buf);
    if (n > TEX_PACK_MAX) {
        n = TEX_PACK_MAX;
    }
    for (i = 0; i < n; i++) {
        (&gTexHandles[slot])[i] = Tex_CreateFromTim2(Pack_GetPart(buf, i), gTexCreateKinds[kind]);
    }
    return n;
}

/* Loads compressed pack file `id` into gLoadBuf and registers it. */
s32 Tex_LoadPack(s32 id, s32 slot, s32 kind)
{
    u8 *buf = gLoadBuf;

    LoadCompressedFile(buf, id);
    return Tex_RegisterPack(slot, kind, buf);
}

/* Loads uncompressed pack file `id` into the data buffer (2.625 MB into it)
 * and registers it. */
s32 Tex_LoadPackRaw(s32 id, s32 slot, s32 kind)
{
    u8 *buf = (*(u8 * *)&gSysSettings.dataBuf) + 0x2A0000;

    LoadFile(buf, id);
    return Tex_RegisterPack(slot, kind, buf);
}

/* Decompresses a pack from memory (src, up to 32768 bytes) into
 * gLoadBuf and registers it. */
s32 Tex_LoadPackLz(s32 unused, s32 slot, s32 kind, s32 src)
{
    u8 *buf = gLoadBuf;

    LzDecompressSized((u8 *)src, buf, 32768);
    return Tex_RegisterPack(slot, kind, buf);
}

/* A model's textures: the picture in buf in slot `slot`, and when the
 * pack counts two, texture secondId in the next slot, found among the
 * loaded ones (find) or loaded. */
void Tex_RegisterTim2Pair(TexPackHead *pack, s32 slot, s32 kind, s32 secondId, u8 *buf, s32 find)
{
    gTexHandles[slot] = Tex_CreateFromTim2(buf, gTexCreateKinds[kind]);
    if (pack->count >= 2) {
        if (find != 0) {
            gTexHandles[slot + 1] = Tex_FindById(secondId, buf, kind);
        } else {
            gTexHandles[slot + 1] = Tex_LoadById(secondId, (s32)buf, kind);
        }
    }
}

/* Loads a model's pack file (gTexPackFiles[*pack->file]) into gLoadBuf
 * and registers its textures (Model_SetupLoadedPack). */
void Tex_LoadPackFile(TexPackHead *pack, s32 slot, s32 kind, s32 secondId)
{
    u8 *buf = gLoadBuf;

    LoadCompressedFile(buf, gTexPackFiles[*pack->file]);
    Tex_RegisterTim2Pair(pack, slot, kind, secondId, buf, 0);
}

/* Decompresses a model's pack from memory (src, up to 32768 bytes) into
 * the read buffer (at +0xE0000) and registers its textures, the second
 * found among the loaded ones (Model_SetupLoadedPackEx). */
void Tex_LoadPackLzHigh(TexPackHead *pack, s32 slot, s32 kind, s32 secondId, s32 src)
{
    u8 *buf = gReadBuf + 0xE0000;

    LzDecompressSized((u8 *)src, buf, 32768);
    Tex_RegisterTim2Pair(pack, slot, kind, secondId, buf, 1);
}

/* Loads the common pack (system texture file 5) and puts each picture's
 * texture in its slot of gCommonPackTexSlots. */
void Tex_LoadCommonPack(void)
{
    u8 *buf = gLoadBuf;
    s32 i;
    s32 n;

    LoadCompressedFile(buf, gSystemTexFiles[5]);
    n = Pack_Count((s32 *) buf);
    for (i = 0; i < n; i++) {
        s32 id = gCommonPackTexSlots[i];
        gTexHandles[id] = Tex_CreateFromTim2(Pack_GetPart(buf, i), gTexCreateKinds[0]);
    }
}

/* Releases the common pack's slots, 2 to 11. */
void Tex_ReleaseCommon(void)
{
    Tex_ReleaseRange(2, 10);
}
