/*
 * Texture packs (sub_main 0x333D70-0x334260): a pack file holds TIM2
 * pictures (Pack_Count, Pack_GetPart), and registering it makes a texture
 * handle of each, in consecutive slots of gTexHandles from a given slot,
 * with one of gTexCreateKinds' creation modes. The packs are loaded from a
 * file (LoadCompressedFile, LoadFile) or decompressed from memory. A
 * model's pack (TexPackHead, include/texture.h) registers its first
 * picture and, when it has two, a second texture by id (Tex_LoadById or
 * Tex_FindById in unk_335040.c). The common pack fills the fixed slots of
 * gCommonPackTexSlots.
 *
 * The game compiled this unit at optimization level 4 (Tex_RegisterPack
 * matches only there); the next unit, unk_334260.c, at level 3.
 */
#include "common.h"
#include "syssettings.h"
#include "texture.h"

void Tex_ReleaseRange(s32 start, s32 n);
s32 Tex_RegisterPack(s32 slot, s32 kind, u8 *buf);
s32 LoadFile(u8 *dst, s32 id);
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 LzDecompressSized(u8 *src, u8 *dst, s32 size);

extern u8 *gLoadBuf;
extern u8 *gReadBuf;


s32 Pack_Count(s32 *buf);
void *Pack_GetPart(u8 *buf, s32 a);
s32 Tex_LoadById(s32 id, s32 buf, s32 kind);
s32 Tex_FindById(s32 a, u8 *buf, s32 kind); /* unit's own declaration: with the definition's s32 (s8 id, u8 *buf, s32 kind) (sub_main/unk_335040.c), Tex_RegisterTim2Pair 9 of 57 words differ */
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
void Tex_RegisterTim2Pair(TexPackHead *pack, s32 slot, s32 kind, s32 secondId, u8 *buf, s32 find);

extern s32 gTexHandles[];
extern s32 gTexCreateKinds[];
extern s32 gTexPackFiles[];
extern s32 gSystemTexFiles[];
extern s8 gCommonPackTexSlots[];

#pragma optimization_level 4

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

/* Loads uncompressed pack file `id` into the data buffer (at +0x2A0000)
 * and registers it. */
s32 Tex_LoadPackRaw(s32 id, s32 slot, s32 kind)
{
    u8 *buf = (*(u8 * *)&gSysSettings.dataBuf) + 0x2A0000;

    LoadFile(buf, id);
    return Tex_RegisterPack(slot, kind, buf);
}

/* Decompresses a pack from memory (src, up to 0x8000 bytes) into
 * gLoadBuf and registers it. */
s32 Tex_LoadPackLz(s32 unused, s32 slot, s32 kind, s32 src)
{
    u8 *buf = gLoadBuf;

    LzDecompressSized((u8 *)src, buf, 0x8000);
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

/* Decompresses a model's pack from memory (src, up to 0x8000 bytes) into
 * the read buffer (at +0xE0000) and registers its textures, the second
 * found among the loaded ones (Model_SetupLoadedPackEx). */
void Tex_LoadPackLzHigh(TexPackHead *pack, s32 slot, s32 kind, s32 secondId, s32 src)
{
    u8 *buf = gReadBuf + 0xE0000;

    LzDecompressSized((u8 *)src, buf, 0x8000);
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
