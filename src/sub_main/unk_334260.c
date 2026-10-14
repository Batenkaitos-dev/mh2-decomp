/*
 * Texture and file loading, split from unk_333D70.c at Tex_LoadSystemPack: the
 * game compiled unk_333D70.c's unit at optimization level 4 and this one at
 * level 3 (Tex_LoadSystemPack matches only at level 3).
 */
#include "common.h"
#include "syssettings.h"
#include "scene.h"

/* This unit was built without lifetime analysis (opt_lifetimes): Tex_LoadFile's unrolled walk
 * keeps one pointer register stepped by 8 eight times, the load not folded into an offset;
 * every other function of the file compiles the same either way. */
#pragma opt_lifetimes off

s32 LoadFile(u8 *dst, s32 id);
s32 LoadCompressedFile(u8 *dst, s32 id);
void Pack_CopyParts01(u8 *pack, u8 *dst0, u8 *dst1);
s32 Afs_ReadId(u32 id, void *dst);
s32 LzDecompress(u8 *src, u8 *dst);

extern u8 *gLoadBuf;
extern u8 *gReadBuf;

s32 Pack_Count(s32 *buf);
void *Pack_GetPart(u8 *buf, s32 a);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);

extern s32 gTexHandles[];
extern s32 gTexCreateKinds[];
extern s32 gSystemTexFiles;
extern s32 D_0042F1B8;
extern s8 gSystemPackTexSlots[];
extern s8 gSystemPackTexSlotsB[];

typedef struct {
    s32 unk0[4];
    s32 unk10;
    s32 unk14[15];
} Unk334550;

s32 Debug_IsOn(s32 unused0);
void FlushCache(s32 op);
s32 Dma_WaitIdle(void);
struct Image;
s32 flLockTexture(void *rect, u32 handle, struct Image *info, s32 flag);
s32 flLockPalette(void *rect, u32 handle, struct Image *info, s32 flag);
s32 flUnlockTexture(u32 handle);
s32 flUnlockPalette(u32 handle);
s32 VramSlot_FreeAll(void);
void Mem_Copy64ByQwords(u8 *src, s32 dst, s32 n);

typedef struct {
    s32 off;
    s32 size;
} Unk334680;

s32 Quest_IsVersus(void);
s32 ElderRaid_InState1(void);
s32 LobbyWorld_GetSeason(void);
u8 Village_FacilityLevel(s32 k);
void AreaPack_ReadPart2(s32 *pack);

extern s16 gTexFileIds[];
extern s16 gStageTexFileIds[];
extern u8 gMapPackTexIndex[];
extern s32 gAreaPackFiles[];
extern s32 gAreaSubModelFiles[];
extern s32 gArea83PackFiles;
extern s32 D_0042F374;
extern s32 gArea83SubModelFiles;
extern s32 D_0042F37C;
extern s32 gArea83SubModelFileS1;
extern s32 gArea84PackFiles;
extern s32 D_0042F38C;
extern s32 gFarmFacilityModelFiles[];

extern s32 gPlayerAnimFiles[];
extern s32 gCommonAnimFiles[];
extern s32 gEmAnimFiles[];
extern s32 *gEmSlotAnimFiles[];
extern s32 gCommonModelPackFiles[];
extern s32 gWeaponModelPackFiles[];
extern s32 *gPartModelFiles0[];
extern s32 *gPartModelFiles1[];
extern s32 gEmModelPackFilesA[];
extern s32 gEmModelPackFiles[];
extern s32 gEmSlotModelPackFiles[];
extern s32 gEquipModelPackFilesA[];
extern s32 gEquipModelPackFilesB[];

void Tex_LoadSystemPack(s32 mode)
{
    u8 *buf = gLoadBuf;
    s32 i;
    s32 n;

    LoadCompressedFile(buf, gSystemTexFiles);
    n = Pack_Count((s32 *) buf);
    for (i = 0; i < n; i++) {
        s32 id = gSystemPackTexSlots[i];

        if (mode != 0) {
            if (id == 2 || id == 4 || id == 5) {
                continue;
            }
            if (mode != 1) {
                if (id == 10 || id == 11) {
                    continue;
                }
            }
        }
        gTexHandles[id] = Tex_CreateFromTim2(Pack_GetPart(buf, i), gTexCreateKinds[0]);
    }
    if (mode == 0) {
        LoadCompressedFile(buf, D_0042F1B8);
        gTexHandles[3] = Tex_CreateFromTim2(Pack_GetPart(buf, 0), gTexCreateKinds[0]);
    }
}

void Tex_LoadSystemPackB(void)
{
    u8 *buf = gLoadBuf;
    s32 i;
    s32 n;

    LoadCompressedFile(buf, gSystemTexFiles);
    n = Pack_Count((s32 *) buf);
    for (i = 0; i < n; i++) {
        s32 id = gSystemPackTexSlotsB[i];

        if (id == 8 || id == 11) {
            gTexHandles[id] = Tex_CreateFromTim2(Pack_GetPart(buf, i), gTexCreateKinds[0]);
        }
    }
}

void Tex_PatchClut_Unk(void)
{
    u8 *buf = gLoadBuf;
    s32 j;
    s32 i;
    u32 *clut;
    u32 a;
    u32 c;

    if (Debug_IsOn(0x9C) == 0) {
        LoadCompressedFile(buf, 0x28);
        FlushCache(0);
        Dma_WaitIdle();
        buf += 8;
        for (j = 0; j < 2; j++) {
            clut = (u32 *)((s32)gLoadBuf + *(s32 *)buf + 0x10040);
            for (i = 0; i < 0x100; i++) {
                c = *clut;
                a = c & 0xFF000000;
                if (a == 0xFF000000) {
                    a = 0x80000000;
                } else if (a != 0) {
                    a = (a >> 1) & 0xFF000000;
                }
                *clut = (c & 0xFFFFFF) | a;
                clut++;
            }
            buf += 8;
        }
    }
}

void Tex_LoadResultTex_Unk(void)
{
    u8 *buf = gLoadBuf;
    s32 tex;
    s32 i;
    s32 lo;
    s32 hi;
    s32 v;
    Unk334550 info;

    if (Debug_IsOn(0x9C) == 0) {
        buf += 8;
        for (i = 0; i < 2; i++) {
            tex = (s32)gLoadBuf + *(s32 *)buf;
            v = gTexHandles[i == 0 ? 0x154 : 0x146];
            lo = v & 0xFFFF;
            hi = (u32)(v & 0xFFFF0000) >> 16;
            flLockTexture(0, lo, (struct Image *)&info, 2);
            Mem_Copy64ByQwords((u8 *)(tex + 0x40), info.unk10, 0x400);
            flUnlockTexture(lo);
            flLockPalette(0, hi, (struct Image *)&info, 2);
            Mem_Copy64ByQwords((u8 *)(tex + 0x10040), info.unk10, 0x10);
            flUnlockPalette(hi);
            buf += 8;
        }
        VramSlot_FreeAll();
    }
}

void Tex_LoadFile(s32 file, s32 slot, s32 kind)
{
    u8 *buf = gLoadBuf;
    s32 off;
    Unk334680 *e;
    s32 i;
    s32 n;

    switch (slot) {
    case 0:
    case 0x148:
    case 0x155:
    case 0x154:
        if (gTexHandles[slot] != 0) {
            return;
        }
    }
    LoadCompressedFile(buf, gTexFileIds[file]);
    if (slot == 0x154) {
        n = 0;
        if (gScene.map < 0x1A) {
            n = gMapPackTexIndex[gScene.map];
        }
        e = (Unk334680 *)(buf + 8);
        for (i = 0; i < n; i++) {
            off = e->off;
            e = (Unk334680 *)((u8 *)e + 8);
        }
        off += (s32)buf;
        gTexHandles[slot] = Tex_CreateFromTim2((void *)off, gTexCreateKinds[0]);
    } else {
        gTexHandles[slot] = Tex_CreateFromTim2(buf, gTexCreateKinds[kind]);
    }
}

void Tex_LoadStageFile(s32 file, s32 unused, s32 slot, s32 kind)
{
    s32 id;
    u8 *buf;

    buf = gLoadBuf;
    id = gStageTexFileIds[file];
    if (file == 9 || file == 0x18) {
        if (Quest_IsVersus() == 1) {
            id = 0x10;
        }
    } else {
        switch (gScene.questId) {
        case 1:
        case 2:
            id = 0xB;
            break;
        case 3:
        case 4:
        case 5:
            id = (file == 6) ? 0xC : 0x1A;
            break;
        }
    }
    LoadCompressedFile(buf, id);
    gTexHandles[slot] = Tex_CreateFromTim2(buf, gTexCreateKinds[kind]);
}

s32 LoadCompressedFile(u8 *dst, s32 id)
{
    s32 ret;

    if (id < 0) {
        return -1;
    }
    ret = Afs_ReadId(id | 0x30000, gReadBuf);
    if (ret != 0) {
        ret = LzDecompress(gReadBuf, dst);
    }
    return ret;
}

s32 LoadFile(u8 *dst, s32 id)
{
    if (id < 0) {
        return -1;
    }
    return Afs_ReadId(id | 0x30000, dst);
}

void Load_PlayerAnim(s32 unused, s32 i)
{
    LoadCompressedFile(gLoadBuf, gPlayerAnimFiles[i]);
}

void Load_CommonAnim(s32 i)
{
    LoadFile(gLoadBuf, gCommonAnimFiles[i]);
}

void Load_EmAnim(s32 unused, s32 i)
{
    LoadCompressedFile(gLoadBuf, gEmAnimFiles[i]);
}

void Load_EmSlotAnim(s32 i, s32 j)
{
    s32 *tbl = gEmSlotAnimFiles[i];
    u8 *buf = gLoadBuf;

    if (tbl != NULL) {
        LoadCompressedFile(buf, tbl[j]);
    }
}

void Load_AreaPack(s32 id)
{
    u8 *base;
    u8 *end;
    s32 file;

    end = (base = (*(u8 * *)&gSysSettings.modelPack)) + 0x128000;
    switch (id) {
    case 0x83:
        if (gScene.online != 0 && ElderRaid_InState1() == 1) {
            file = D_0042F374;
        } else {
            file = gArea83PackFiles;
        }
        break;
    case 0x84:
        if (gScene.online != 0 && ElderRaid_InState1() == 1) {
            file = D_0042F38C;
        } else {
            file = gArea84PackFiles;
        }
        break;
    default:
        file = gAreaPackFiles[id];
        break;
    }
    LoadCompressedFile(gLoadBuf, file);
    Pack_CopyParts01(gLoadBuf, base, end);
    AreaPack_ReadPart2((s32 *) gLoadBuf);
}

void Load_CommonModelPack(s32 i)
{
    u8 *base = (*(u8 * *)&gSysSettings.modelPack);

    LoadCompressedFile(gLoadBuf, gCommonModelPackFiles[i]);
    Pack_CopyParts01(gLoadBuf, base, base + 0x128000);
}

void Load_WeaponModelPack(s32 i)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gWeaponModelPackFiles[i]);
    Pack_CopyParts01(gLoadBuf, base, base + 0x128000);
}

void Load_PartModelPack(s32 i, s32 j, s32 k)
{
    s32 *tbl;
    u8 *base;

    if (k == 0) {
        tbl = gPartModelFiles0[j];
    } else {
        tbl = gPartModelFiles1[j];
    }
    base = (*(u8 * *)&gSysSettings.dataBuf);
    tbl += i;
    LoadCompressedFile(gLoadBuf, *tbl);
    Pack_CopyParts01(gLoadBuf, base, base + 0x128000);
}

void Load_EmModelPack(s32 i)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gEmModelPackFilesA[i]);
    Pack_CopyParts01(gLoadBuf, base, base + 0x128000);
}

void Load_EmModelPack2(s32 i)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gEmModelPackFiles[i]);
    Pack_CopyParts01(gLoadBuf, base, base + 0x128000);
}

void Load_EmSlotModelPack(s32 i)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gEmSlotModelPackFiles[i]);
    Pack_CopyParts01(gLoadBuf, base, base + 0x128000);
}

void Load_EquipModelPack(s32 i, s32 k)
{
    u8 *base;
    u8 *end;

    end = (base = (*(u8 * *)&gSysSettings.dataBuf)) + 0x128000;
    if (k == 0) {
        LoadCompressedFile(gLoadBuf, gEquipModelPackFilesA[i]);
    } else {
        LoadCompressedFile(gLoadBuf, gEquipModelPackFilesB[i]);
    }
    Pack_CopyParts01(gLoadBuf, base, end);
}

void Load_AreaSubModelPack(s32 id, s32 sub)
{
    s32 file;
    u8 *base;

    if (sub == 0) {
        switch (id) {
        case 0x83:
            if (gScene.online != 0) {
                switch (LobbyWorld_GetSeason()) {
                case 2:
                default:
                    file = gArea83SubModelFiles;
                    break;
                case 0:
                    file = D_0042F37C;
                    break;
                case 1:
                    file = gArea83SubModelFileS1;
                    break;
                }
            } else {
                file = gArea83SubModelFiles;
            }
            break;
        default:
            file = gAreaSubModelFiles[id];
            break;
        }
    } else {
        switch (id) {
        case 0x89:
            file = gFarmFacilityModelFiles[(sub - 1) * 3 + (u8)Village_FacilityLevel(sub)];
            break;
        default:
            return;
        }
    }
    base = (*(u8 * *)&gSysSettings.modelPack);
    LoadCompressedFile(gLoadBuf, file);
    Pack_CopyParts01(gLoadBuf, base, base + 0x128000);
}

/* unk_334260's .data (0x41ED50-0x4224E0) */
s32 gPlayerAnimFiles[12] = {
#include "sub_main/0041ED50.inc"
};
s32 gCommonAnimFiles[8] = {
#include "sub_main/0041ED80.inc"
};
s32 gPartModelFiles0_0[200] = {
#include "sub_main/0041EDA0.inc"
};
s32 gPartModelFiles0_1[28] = {
#include "sub_main/0041F0C0.inc"
};
s32 gPartModelFiles0_2[220] = {
#include "sub_main/0041F130.inc"
};
s32 gPartModelFiles0_3[236] = {
#include "sub_main/0041F4A0.inc"
};
s32 gPartModelFiles0_4[228] = {
#include "sub_main/0041F850.inc"
};
s32 gPartModelFiles0_5[208] = {
#include "sub_main/0041FBE0.inc"
};
s32 gPartModelFiles0_6[24] = {
#include "sub_main/0041FF20.inc"
};
s32 *gPartModelFiles0[8] = {
    gPartModelFiles0_0, gPartModelFiles0_1, gPartModelFiles0_2, gPartModelFiles0_3,
    gPartModelFiles0_4, gPartModelFiles0_5, gPartModelFiles0_6, NULL,
};
s32 gPartModelFiles1_0[24] = {
#include "sub_main/0041FFA0.inc"
};
u32 D_00420000[64] = {
#include "sub_main/00420000.inc"
};
u32 D_00420100[64] = {
#include "sub_main/00420100.inc"
};
u32 D_00420200[48] = {
#include "sub_main/00420200.inc"
};
s32 gPartModelFiles1_1[28] = {
#include "sub_main/004202C0.inc"
};
s32 gPartModelFiles1_2[220] = {
#include "sub_main/00420330.inc"
};
s32 gPartModelFiles1_3[236] = {
#include "sub_main/004206A0.inc"
};
s32 gPartModelFiles1_4[228] = {
#include "sub_main/00420A50.inc"
};
s32 gPartModelFiles1_5[208] = {
#include "sub_main/00420DE0.inc"
};
s32 gPartModelFiles1_6[24] = {
#include "sub_main/00421120.inc"
};
s32 *gPartModelFiles1[8] = {
    gPartModelFiles1_0, gPartModelFiles1_1, gPartModelFiles1_2, gPartModelFiles1_3,
    gPartModelFiles1_4, gPartModelFiles1_5, gPartModelFiles1_6, NULL,
};
s32 gWeaponModelPackFiles[340] = {
#include "sub_main/004211A0.inc"
};
s32 gEmModelPackFilesA[76] = {
#include "sub_main/004216F0.inc"
};
s32 gEmAnimFiles[76] = {
#include "sub_main/00421820.inc"
};
s32 gEmModelPackFiles[76] = {
#include "sub_main/00421950.inc"
};
s32 gAreaPackFiles[200] = {
#include "sub_main/00421A80.inc"
};
u32 gAreaTableFiles[200] = {
#include "sub_main/00421DA0.inc"
};
s32 gAreaSubModelFiles[200] = {
#include "sub_main/004220C0.inc"
};
s32 gEmSlotAnimFiles_1[4] = {
#include "sub_main/004223E0.inc"
};
s32 gEmSlotAnimFiles_7[2] = {
#include "sub_main/004223F0.inc"
};
s32 gEmSlotAnimFiles_8[2] = {
#include "sub_main/004223F8.inc"
};
s32 gEmSlotAnimFiles_10[2] = {
#include "sub_main/00422400.inc"
};
s32 gEmSlotAnimFiles_11[2] = {
#include "sub_main/00422408.inc"
};
s32 *gEmSlotAnimFiles[20] = {
    NULL, gEmSlotAnimFiles_1, gEmSlotAnimFiles_1, NULL,
    NULL, gEmSlotAnimFiles_1, NULL, gEmSlotAnimFiles_7,
    gEmSlotAnimFiles_8, NULL, gEmSlotAnimFiles_10, gEmSlotAnimFiles_11,
    gEmSlotAnimFiles_10, gEmSlotAnimFiles_10, gEmSlotAnimFiles_10, gEmSlotAnimFiles_10,
    gEmSlotAnimFiles_10, gEmSlotAnimFiles_10, NULL, NULL,
};
s32 gEmSlotModelPackFiles[20] = {
#include "sub_main/00422460.inc"
};
u32 gStageFilePairs[4] = {
#include "sub_main/004224B0.inc"
};
s32 gCommonModelPackFiles[8] = {
#include "sub_main/004224C0.inc"
};
