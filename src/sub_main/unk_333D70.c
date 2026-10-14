#include "common.h"
#include "syssettings.h"

void Tex_ReleaseRange(s32 a, s32 b);
s32 Tex_RegisterPack(s32 slot, s32 kind, u8 *buf);
s32 LoadFile(u8 *dst, s32 id);
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 LzDecompressSized(u8 *src, u8 *dst, s32 size);

extern u8 *gLoadBuf;
extern u8 *gReadBuf;

/* A texture pack as Model_SetupLoadedPack passes it: its count, then its file. */
typedef struct TexPackHead {
    s32 count;          /* 0x0: the textures (Model.texCount); a second is registered from 2 */
    s32 *file;          /* 0x4: the pack file, an index of gTexPackFiles */
} TexPackHead;

s32 Pack_Count(s32 *buf);
void *Pack_GetPart(u8 *buf, s32 a);
s32 Tex_LoadById(s32 id, s32 buf, s32 kind);
s32 Tex_FindById(s32 a, u8 *buf, s32 kind); /* unit's own declaration: with the definition's s32 (s8 id, u8 *buf, s32 kind) (sub_main/unk_335040.c), Tex_RegisterTim2Pair 9 of 57 words differ */
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
void Tex_RegisterTim2Pair(TexPackHead *p, s32 slot, s32 kind, s32 arg3, u8 *buf, s32 flag);

extern s32 gTexHandles[];
extern s32 gTexCreateKinds[];
extern s32 gTexPackFiles[];
extern s32 gSystemTexFiles[];
extern s8 gCommonPackTexSlots[];

/*
 * Texture packs. The game compiled this unit at optimization level 4
 * (Tex_RegisterPack matches only there); the next unit, unk_334260.c,
 * at level 3.
 */
#pragma optimization_level 4

void Tex_FreeRange(s32 a, s32 b)
{
    Tex_ReleaseRange(a, b);
}

s32 Tex_RegisterPack(s32 slot, s32 kind, u8 *buf)
{
    s32 i;
    s32 n;

    n = Pack_Count((s32 *) buf);
    if (n > 100) {
        n = 100;
    }
    for (i = 0; i < n; i++) {
        (&gTexHandles[slot])[i] = Tex_CreateFromTim2(Pack_GetPart(buf, i), gTexCreateKinds[kind]);
    }
    return n;
}

s32 Tex_LoadPack(s32 id, s32 slot, s32 kind)
{
    u8 *buf = gLoadBuf;

    LoadCompressedFile(buf, id);
    return Tex_RegisterPack(slot, kind, buf);
}

s32 Tex_LoadPackRaw(s32 id, s32 slot, s32 kind)
{
    u8 *buf = (*(u8 * *)&gSysSettings.dataBuf) + 0x2A0000;

    LoadFile(buf, id);
    return Tex_RegisterPack(slot, kind, buf);
}

s32 Tex_LoadPackLz(s32 unused, s32 slot, s32 kind, s32 src)
{
    u8 *buf = gLoadBuf;

    LzDecompressSized((u8 *)src, buf, 0x8000);
    return Tex_RegisterPack(slot, kind, buf);
}

void Tex_RegisterTim2Pair(TexPackHead *p, s32 slot, s32 kind, s32 id, u8 *buf, s32 flag)
{
    gTexHandles[slot] = Tex_CreateFromTim2(buf, gTexCreateKinds[kind]);
    if (p->count >= 2) {
        if (flag != 0) {
            gTexHandles[slot + 1] = Tex_FindById(id, buf, kind);
        } else {
            gTexHandles[slot + 1] = Tex_LoadById(id, (s32)buf, kind);
        }
    }
}

void Tex_LoadPackFile(TexPackHead *p, s32 slot, s32 kind, s32 d)
{
    u8 *buf = gLoadBuf;

    LoadCompressedFile(buf, gTexPackFiles[*p->file]);
    Tex_RegisterTim2Pair(p, slot, kind, d, buf, 0);
}

void Tex_LoadPackLzHigh(TexPackHead *p, s32 slot, s32 kind, s32 d, s32 src)
{
    u8 *buf = gReadBuf + 0xE0000;

    LzDecompressSized((u8 *)src, buf, 0x8000);
    Tex_RegisterTim2Pair(p, slot, kind, d, buf, 1);
}

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

void Tex_ReleaseCommon(void)
{
    Tex_ReleaseRange(2, 10);
}
