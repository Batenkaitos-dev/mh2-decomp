#include "common.h"
#include "em.h"
#include "player.h"
#include "model.h"
#include "lobby_state.h"
#include "image.h"
#include "scene.h"
#include "syssettings.h"
#include "area.h"

/* prototypes */
void Bone_Release(s32 first, s32 n);
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 LoadFile(u8 *dst, s32 id);
void Model_Release(s32 i, s32 a);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Vec_Normalize(f32 *);
void Vec_Copy(f32 *dst, f32 *src);
void Quat_ToMatrix(f32 *q, f32 (*m)[4]);
void Quat_Set(f32 *axis, f32 *q, f32 angle);
void CamData_Relocate(struct CamDataHdr *h);
void Material_Release(s32 first, s32 n);
void MatSet_Release(s32 first, s32 n);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetState63(s32 a);
void Gfx_SetBlendOp(s32 a);
void * memset(void *p, s32 c, u32 n);
extern s32 gAreaCamDataFiles[];
extern s8 gTexIds;
extern s32 gTexIdHandles[];
extern s32 gModels;
/* end of prototypes */

/* Two files per entry, loaded one after the other into the buffer at gModelLoadBuf. */
typedef struct FilePair {
    s32 first;
    s32 second;
} FilePair;

extern FilePair gStageFilePairs[];
extern u8 *gModelLoadBuf;

void Stage_LoadFilePair(s32 i)
{
    u8 *buf = gModelLoadBuf;

    LoadFile(buf, gStageFilePairs[i].first);
    LoadFile(buf + 0x298000, gStageFilePairs[i].second);
}

void func_003350B0(void)
{
}

extern u32 gTexHandles[];
extern s16 gTexPrepFrameCount;
extern s16 gTexPrepLastCount;
extern s16 gTexPrepLast;
s32 Tex_Use(s32 n, u32 *handles);

void Tex_Prepare(s32 n, s32 tex)
{
    if (n != 0) {
        Tex_Use(n, &gTexHandles[tex]);
        gTexPrepLast = tex;
        gTexPrepLastCount = n;
        gTexPrepFrameCount += n;
    }
}

void Area_LoadCamData(s32 area)
{
    s32 id;
    s32 h;

    id = gAreaCamDataFiles[area];
    h = 0;
    if (id != 0) {
        LoadCompressedFile(gSysSettings.camData, id);
        h = (s32)gSysSettings.camData;
    }
    CamData_Relocate((void *) h);
}

s32 LzDecompress(u8 *src, u8 *dst);

s32 LzDecompressSized(u8 *src, u8 *dst, s32 size)
{
    return LzDecompress(src, dst);
}

extern u8 gLookDelay[];

void PlayerLook_ResetLoaded(void)
{
    s16 i;

    for (i = 0; i < 10; i++) {
        gLookDelay[i] = 0;
    }
}

/* A player's equipment and colours as sent between players (0x5E bytes; gLookPending has
 * one per player). */
typedef struct PlayerLook {
    EquipItem weapon;   /* 0x00 */
    u16 kind;           /* 0x0C: the weapon kind (Player.unk30C) */
    EquipItem armour[6];/* 0x0E: Player.equip[0-5] */
    u8 colour[6];       /* 0x56: Player.armourKind */
    u8 unk5C[2];
} PlayerLook;
u8 WeaponClass_IsGunner(u8 cls);
u8 WeaponType_Flags(u8 cls);
u8 Equip_GetBladeSpecial(EquipItem *e);
extern u8 gWeaponKindClass[];

/* Puts look l on player p: the equipment, colours and weapon, and what depends on the weapon. */
void Player_SetLook(Player *p, PlayerLook *l)
{
    s32 i;

    p->weaponKind = l->kind;
    for (i = 0; i < 6; i++) {
        p->equip[i] = l->armour[i];
        p->armourKind[i] = l->colour[i];
    }
    p->equip[6] = l->weapon;
    p->weaponType = gWeaponKindClass[p->weaponKind];
    p->isGunner = WeaponClass_IsGunner(p->weaponType);
    p->weaponFlags = WeaponType_Flags(p->weaponType);
    p->bladeSpecial = Equip_GetBladeSpecial(&p->equip[6]);
}

typedef struct EquipSet {
    EquipItem e[6];
} EquipSet;

typedef struct LookFile {
    s32 unk0;
    s32 *unk4;
} LookFile;

extern u8 *gReadBuf;
extern LookFile *gPlayerLookFiles[];
void Player_BuildPartIds(Player *p, u8 *ids, EquipItem *a, u8 *b);
s16 Look_MapPartId(Player *p, s16 kind, s16 id, u8 k);
s32 Armor_LacksSexModel(s32 id, u8 a);

/* A player's model slots (as in unk_33B080.c): the body's range, then parts 1-5 each with a
 * range of slots, and the part ids at +0x8A8. */
typedef struct PlModels {
    u8 pad0[0x4E8];
    s16 count;                  /* 0x4E8 */
    s16 first;                  /* 0x4EA */
    Model *data;                /* 0x4EC */
    u8 pad4F0[0x4FC - 0x4F0];
    Model *partData[6];         /* 0x4FC */
    s16 partCount[6];           /* 0x514 */
    s16 partFirst[6];           /* 0x520 */
    u8 pad52C[0x8A8 - 0x52C];
    u8 partId[6];               /* 0x8A8: 0xFF: none */
} PlModels;

extern u8 *gLoadBuf;
void Pack_CopyParts01(u8 *pack, u8 *dst0, u8 *dst1);
void Player_AllocWeaponModel(s32 kind, s32 pl, u8 *arg2, u8 *arg3);
s32 Model_Alloc(s32 n);
void Model_Reserve(s32 first, s32 n);
Model *Model_SetupLoadedPackEx(s16 idx, s32 a, s32 slot, s32 *pack, s32 b, s32 d, s32 c, s32 e);

/* Loads the player's six look models (body and five parts) from the packed look files. */
void Player_LoadLookModels(Player *p)
{
    s16 k;
    u8 *end;
    s16 i;
    s32 part;
    s16 m;
    PlModels *pm;
    s16 id;
    LookFile *f;
    u8 *buf;

    buf = gModelLoadBuf;
    end = buf + 0x128000;
    pm = (PlModels *)p;

    LzDecompressSized(gReadBuf, gLoadBuf, 0x18000);
    Pack_CopyParts01(gLoadBuf, buf, end);
    Player_AllocWeaponModel(p->weaponKind, p->index, buf, gReadBuf + 0x18000);
    Player_BuildPartIds(p, p->armourKind, p->equip, &p->lookVariant);
    for (i = 0; i < 6; i++) {
        part = i;
        m = Look_MapPartId(p, i, p->armourKind[part], p->lookVariant);
        if (part == 2) {
            if (Armor_LacksSexModel(p->equip[2].id, p->gender) == 1) {
                p->torsoAltModel = 1;
                k = 6;
            } else {
                p->torsoAltModel = 0;
                k = i;
            }
        } else {
            k = i;
        }
        id = Model_Alloc(1);
        if (id < 0) {
            return;
        }
        if (part == 0) {
            pm->first = id;
            pm->count = 1;
            pm->data = Model_Get(id);
        } else {
            pm->partFirst[i] = id;
            pm->partCount[i] = 1;
            pm->partData[i] = Model_Get(id);
        }
        Model_Reserve(id, 1);
        f = gPlayerLookFiles[k + p->gender * 7];
        f += m;
        p->lookPartId[part] = m;
        switch (part) {
        case 0:
            LzDecompressSized(gReadBuf + 0x20000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(p->index * 12 + 0x2B + i * 2), (s32 *)f, 0x900, (u16)(p->lookVariant + p->gender * 4), 2, (s32)(gReadBuf + 0x38000));
            break;
        case 1:
            LzDecompressSized(gReadBuf + 0x40000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(p->index * 12 + 0x2B + i * 2), (s32 *)f, 0x900, (u16)(p->lookVariant + p->gender * 4), 1, (s32)(gReadBuf + 0x58000));
            break;
        case 2:
            LzDecompressSized(gReadBuf + 0x60000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(p->index * 12 + 0x2B + i * 2), (s32 *)f, 0x900, (u16)(p->lookVariant + p->gender * 4), 1, (s32)(gReadBuf + 0x78000));
            break;
        case 3:
            LzDecompressSized(gReadBuf + 0x80000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(p->index * 12 + 0x2B + i * 2), (s32 *)f, 0x900, (u16)(p->lookVariant + p->gender * 4), 1, (s32)(gReadBuf + 0x98000));
            break;
        case 4:
            LzDecompressSized(gReadBuf + 0xA0000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(p->index * 12 + 0x2B + i * 2), (s32 *)f, 0x900, (u16)(p->lookVariant + p->gender * 4), 1, (s32)(gReadBuf + 0xB8000));
            break;
        case 5:
            LzDecompressSized(gReadBuf + 0xC0000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(p->index * 12 + 0x2B + i * 2), (s32 *)f, 0x900, (u16)(p->lookVariant + p->gender * 4), 1, (s32)(gReadBuf + 0xD8000));
            break;
        }
    }
}

extern PlayerLook gLookPending[];
extern u8 gLookDelay[];
extern LobbyState gLobbyState;
s32 Overlay_IsLoaded(s32 id);
void Lobby_ReceiveWhileWaiting(void);
void Player_FreeWeaponModel(Player *p);
void Player_FreeModels(Player *p);
void Mem_Compact(void);
void Player_InitPartTable(s32 idx);
void Player_LoadLookModels(Player *p);
void Player_ClearFxBlock(Player *p);
void Player_StartMotionLayer(Player *p, u16 motion, s32 a, s16 b, s32 layer);
void Player_InitRecords(Player *p);

/* Applies player p's pending look (gLookPending) once its delay (gLookDelay) runs out: reloads
 * its models and motions and, in the lobby, marks it loaded; in the lobby a player that left
 * (or with its slot state 2 or more) is only cleared. */
void Player_ApplyLook(Player *p)
{
    PlayerLook *look = &gLookPending[p->index];

    if (Overlay_IsLoaded(4) == 1) {
        Lobby_ReceiveWhileWaiting();
    }
    if (gLookDelay[p->index] > 0) {
        gLookDelay[p->index]--;
        if (gLookDelay[p->index] > 0) {
            return;
        }
    }
    if (Overlay_IsLoaded(4) == 1 && ((s8)gLobbyState.lookPending[p->index] >= 2 || p->active == 0)) {
        gLobbyState.modelLoading[p->index] = 0;
        gLobbyState.modelLoaded[p->index] = 0;
        gLobbyState.pcLoaded[p->index] = 0;
        gLobbyState.lookPending[p->index] = 0;
        return;
    }
    Player_SetLook(p, look);
    Player_FreeWeaponModel(p);
    Player_FreeModels(p);
    Mem_Compact();
    Player_InitPartTable(p->index);
    Player_LoadLookModels(p);
    Player_ClearFxBlock(p);
    Player_StartMotionLayer(p, p->motionId[0], 0, 0, 0);
    Player_StartMotionLayer(p, p->motionId[1], 0, 0, 1);
    Player_InitRecords(p);
    p->visible = 1;
    if (Overlay_IsLoaded(4) == 1) {
        gLobbyState.modelLoading[p->index] = 0;
        gLobbyState.modelLoaded[p->index] = 1;
        gLobbyState.pcLoaded[p->index] = 1;
        gLobbyState.lookPending[p->index] = 0;
    }
}

void FileQueue_AddFile(u32 id, void *dst);

void FileQueue_AddPlayerFile(s32 id, s32 a)
{
    FileQueue_AddFile(id | 0x30000, (void *)a);
}

extern s32 *gPartModelFiles0[];
extern s32 *gPartModelFiles1[];
extern s32 gTexPackFiles[];
extern s32 gWeaponModelPackFiles[];
extern s32 gModelPackIds[];
u16 Equip_GetKind(EquipItem *e);
s32 Tex_LoadById(s32 id, s32 buf, s32 kind);
void FileQueue_AddCall(void (*func)(void), s32 nargs, s32 a0, s32 a1, s32 a2, s32 a3);

s32 Player_QueueLookFiles(Player *p)
{
    s16 part;
    s16 m;
    s16 k;
    s32 *tbl;
    LookFile *f;
    s16 i;
    PlayerLook *look = &gLookPending[p->index];

    if (Overlay_IsLoaded(4) == 1) {
        if ((s8)gLobbyState.pcLoaded[p->index] == 1) {
            return 0;
        }
        if ((s8)gLobbyState.lookPending[p->index] == 1) {
            return 0;
        }
    }
    if (Overlay_IsLoaded(4) == 1 && (s8)gLobbyState.lookPending[p->index] == 0) {
        gLobbyState.lookPending[p->index] = 1;
    }
    p->weaponKind = Equip_GetKind(&p->equip[6]);
    look->kind = p->weaponKind;
    look->weapon = p->equip[6];
    *(EquipSet *)look->armour = *(EquipSet *)p->equip;
    Player_BuildPartIds(p, look->colour, look->armour, look->unk5C);
    Tex_LoadById(look->unk5C[0] + p->gender * 4, (s32)(gReadBuf + 0xE0000), 0);
    gLookDelay[p->index]++;
    FileQueue_AddPlayerFile(gWeaponModelPackFiles[look->kind], (s32)gReadBuf);
    FileQueue_AddPlayerFile(gModelPackIds[look->kind], (s32)(gReadBuf + 0x18000));
    for (i = 0; i < 6; i++) {
        part = i;
        m = Look_MapPartId(p, i, look->colour[part], look->unk5C[0]);
        if (part == 2) {
            if (Armor_LacksSexModel(look->armour[2].id, p->gender) == 1) {
                p->torsoAltModel = 1;
                k = 6;
            } else {
                p->torsoAltModel = 0;
                k = part;
            }
        } else {
            k = part;
        }
        if (p->gender == 0) {
            tbl = gPartModelFiles0[k];
        } else {
            tbl = gPartModelFiles1[k];
        }
        f = gPlayerLookFiles[k + p->gender * 7];
        tbl += m;
        f += m;
        switch (part) {
        case 0:
            FileQueue_AddPlayerFile(*tbl, (s32)(gReadBuf + 0x20000));
            FileQueue_AddPlayerFile(gTexPackFiles[*f->unk4], (s32)(gReadBuf + 0x38000));
            break;
        case 3:
            FileQueue_AddPlayerFile(*tbl, (s32)(gReadBuf + 0x80000));
            FileQueue_AddPlayerFile(gTexPackFiles[*f->unk4], (s32)(gReadBuf + 0x98000));
            break;
        case 4:
            FileQueue_AddPlayerFile(*tbl, (s32)(gReadBuf + 0xA0000));
            FileQueue_AddPlayerFile(gTexPackFiles[*f->unk4], (s32)(gReadBuf + 0xB8000));
            break;
        case 1:
            FileQueue_AddPlayerFile(*tbl, (s32)(gReadBuf + 0x40000));
            FileQueue_AddPlayerFile(gTexPackFiles[*f->unk4], (s32)(gReadBuf + 0x58000));
            break;
        case 2:
            FileQueue_AddPlayerFile(*tbl, (s32)(gReadBuf + 0x60000));
            FileQueue_AddPlayerFile(gTexPackFiles[*f->unk4], (s32)(gReadBuf + 0x78000));
            break;
        case 5:
            FileQueue_AddPlayerFile(*tbl, (s32)(gReadBuf + 0xC0000));
            FileQueue_AddPlayerFile(gTexPackFiles[*f->unk4], (s32)(gReadBuf + 0xD8000));
            break;
        }
    }
    FileQueue_AddCall((void (*)(void))Player_ApplyLook, 1, (s32)p, 0, 0, 0);
    return 1;
}
/*
 * Unpacks src into dst (16-bit units); returns the bytes written. A flag word
 * (read when its 16 bits run out, high bit first) marks each unit: 0 copies a
 * literal, 1 a back-reference {count: 5 bits, distance: 11 bits} (count 0: the
 * count follows; distance 0: that many zeros, or the end when the count is 0).
 */
s32 LzDecompress(u8 *src, u8 *dst)
{
    s32 flags;
    s32 bit;
    u8 *start;
    u32 dist;
    u32 n;
    u8 *from;

    start = dst;
    bit = 0;
    flags = 0;

    for (;;) {
        if (bit == 0) {
            flags = *(s16 *)src;
            bit = 0x8000;
            src += 2;
        }
        if (flags & bit) {
            dist = *(u16 *)src;
            n = dist >> 11;
            src += 2;
            if (n != 0) {
                dist &= 0x7FF;
            } else {
                n = *(u16 *)src;
                src += 2;
            }
            if (dist == 0) {
                if (n == 0) {
                    break;
                }
                do {
                    *(u16 *)dst = 0;
                    n--;
                    dst += 2;
                } while (n != 0);
            } else {
                from = dst - dist * 2;
                do {
                    n--;
                    *(u16 *)dst = *(u16 *)from;
                    from += 2;
                    dst += 2;
                } while (n != 0);
            }
        } else {
            *(u16 *)dst = *(u16 *)src;
            src += 2;
            dst += 2;
        }
        bit >>= 1;
    }
    return dst - start;
}

s32 Pack_Count(s32 *buf)
{
    return *buf;
}

void *Pack_GetPart(u8 *buf, s32 a)
{
    return buf + *(s32 *)(buf + a * 8 + 4);
}

s32 Pack_PartSize(u8 *base, s32 i)
{
    return *(s32 *)(base + i * 8 + 8);
}

void *memcpy(void *dst, const void *src, unsigned long n);

/* Copies parts 0 and 1 of the pack at pack to dst0 and dst1. */
void Pack_CopyParts01(u8 *pack, u8 *dst0, u8 *dst1)
{
    void *part;

    part = Pack_GetPart(pack, 0);
    memcpy(dst0, part, Pack_PartSize(pack, 0));
    part = Pack_GetPart(pack, 1);
    memcpy(dst1, part, Pack_PartSize(pack, 1));
}

/* Reads part 0 of the area's environment (AreaPack_ReadPart2): the fog's colour, then its two
 * distances. */
u8 * AreaPack_ReadPart0(u8 *src)
{
    memcpy((void *) &gAreaState.fog.r, (const void *) src, 4U);
    memcpy((void *) &gAreaState.fog, (const void *) (src + 4), 8U);
    return src + 0xC;
}

/* Reads part 1: the area's three lights (directions, ambient colours, colours). */
u8 * AreaPack_ReadPart1(u8 *src)
{
    memcpy((void *) gAreaState.lightDir, (const void *) src, 0x24U);
    memcpy((void *) gAreaState.lightAmb, (const void *) (src + 0x24), 0x24U);
    memcpy((void *) gAreaState.lightCol, (const void *) (src + 0x48), 0x24U);
    return src + 0x6C;
}

/* Reads block 3 of the area pack: the sky's far point and its band (the rectangle, then the
 * eight colours stored in a mixed byte order). */
u8 *AreaPack_ReadBlock3(u8 *p)
{
    u8 b[4];

    memcpy(gAreaState.skyPos, p, 0xC);
    memcpy(&gAreaState.band, p + 0xC, 4);
    memcpy(b, p + 0x10, 4);
    gAreaState.band.c[0] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    memcpy(b, p + 0x14, 4);
    gAreaState.band.c[1] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    memcpy(b, p + 0x18, 4);
    gAreaState.band.c[2] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    memcpy(b, p + 0x1C, 4);
    gAreaState.band.c[3] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    memcpy(b, p + 0x20, 4);
    gAreaState.band.c[4] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    memcpy(b, p + 0x24, 4);
    gAreaState.band.c[5] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    memcpy(b, p + 0x28, 4);
    gAreaState.band.c[6] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    memcpy(b, p + 0x2C, 4);
    gAreaState.band.c[7] = (b[3] << 24) | (b[0] << 16) | (b[1] << 8) | b[2];
    return p + 0x30;
}

u8 *AreaPack_ReadBlock3(u8 *p);

/* Reads part 2 of the area pack: its first halfword (gAreaState.envBlocks) says which blocks follow. */
void AreaPack_ReadPart2(s32 *pack)
{
    AreaState *as = &gAreaState;
    u8 *part;

    if (Pack_Count(pack) < 3) {
        as->envBlocks = 0;
        return;
    }
    part = Pack_GetPart((u8 *)pack, 2);
    as->envBlocks = *(s16 *)part;
    switch (as->envBlocks) {
    case 1:
        AreaPack_ReadPart1(AreaPack_ReadPart0(part + 2));
        break;
    case 2:
        AreaPack_ReadBlock3(AreaPack_ReadPart1(AreaPack_ReadPart0(part + 2)));
        break;
    default:
        as->envBlocks = 0;
        break;
    }
}

void Tex_ClearIdSlots(void)
{
    memset(gTexIdHandles, 0, 0x20U);
    memset(&gTexIds, -1, 8U);
}

s32 flReleaseTextureHandle(u32);
s32 flReleasePaletteHandle(u32);

/* Frees the 8 texture handles of gTexIdHandles (texture id | CLUT id << 16) and clears gTexIds. */
void Tex_ReleaseIdSlots(void)
{
    s32 i;
    s32 *h;
    s8 *f;

    i = 0;
    h = gTexIdHandles;
    f = &gTexIds;
    do {
        s32 v = *h;

        if (v != 0) {
            u32 tex = v & 0xFFFF;
            u32 clut = (u32)(v & 0xFFFF0000) >> 16;

            if (tex != 0) {
                flReleaseTextureHandle(tex);
            }
            if (clut != 0) {
                flReleasePaletteHandle(clut);
            }
        }
        *h = 0;
        i++;
        *f = -1;
        h++;
        f++;
    } while (i < 8);
}

extern s16 gTexByIdFiles[];
extern s32 gTexCreateKinds[];
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);

/* The texture handle loaded with id; loads it (file gTexByIdFiles[id] into buf, kind gTexCreateKinds[kind]) into the first free slot when it is not there. */
s32 Tex_LoadById(s32 id, s32 buf, s32 kind)
{
    s32 i;
    s8 *p;
    u32 *h;

    for (i = 0, p = &gTexIds; i < 8; i++, p++) {
        if (*p == -1) {
            break;
        }
        if (*p == (s8)id) {
            h = (u32 *)&gTexIdHandles[i];
            if (*h != 0) {
                Tex_Use(1, h);
            }
            return *h;
        }
    }
    LoadCompressedFile((u8 *)buf, gTexByIdFiles[id]);
    h = (u32 *)&gTexIdHandles[i];
    *h = Tex_CreateFromTim2((u8 *)buf, gTexCreateKinds[kind]);
    (&gTexIds)[i] = id;
    return *h;
}

/* The texture handle loaded with id (gTexIds, -1 ends the list), after Tex_Use(1, handle); 0 when there is none. */
s32 Tex_FindById(s8 id, u8 *buf, s32 kind)
{
    s32 i;
    s8 *p;

    for (i = 0, p = &gTexIds; i < 8; i++, p++) {
        if (*p == -1) {
            break;
        }
        if (*p == id) {
            u32 *h = (u32 *)&gTexIdHandles[i];

            if (*h != 0) {
                Tex_Use(1, h);
            }
            return *h;
        }
    }
    return 0;
}

/* 1 when texture handle h is one of the 8 in gTexIdHandles. */
s32 Tex_IsIdSlot(s32 h)
{
    s32 i;
    s32 *p;

    for (i = 0, p = gTexIdHandles; i < 8; i++, p++) {
        if (h == *p) {
            return 1;
        }
    }
    return 0;
}

extern u8 *gLoadBuf;
extern s32 gLoadLettersTex;
extern s32 gSystemTexFiles[];
extern u8 gCommonTexImage[];
extern u8 *gCommonTexData[];
extern u8 gCommonClutImage[];
extern u8 *gCommonClutData[];
void Debug_Message(char *unused0, s32 unused1, s32 unused2, s32 unused3);
u32 flCreateTextureHandle(Image *img, s32 mode);
u32 flCreatePaletteHandle(Image *img, s32 mode);

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
    gCommonTexData[0] = buf + 0x40;
    clut = 0;
    tex = flCreateTextureHandle((Image *) gCommonTexImage, 4);
    if (tex != 0) {
        gCommonClutData[0] = buf + 0x10040;
        clut = flCreatePaletteHandle((Image *) gCommonClutImage, 4);
    }
    gLoadLettersTex = clut << 16 | tex;
}

f32 Math_AngleToRad(s32 angle)
{
    return 0.0000958738f * (f32) (angle & 0xFFFF);
}

f32 Math_AngleToRad(s32 a);

/* Three integer angles to floats (Player.rotX). */
void Vec_AnglesToRad(s32 *in, f32 *out)
{
    out[0] = Math_AngleToRad(in[0]);
    out[1] = Math_AngleToRad(in[1]);
    out[2] = Math_AngleToRad(in[2]);
}

void Mtx_Identity(f32 *m);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);

/* A rotation matrix from three integer angles (X, Y, Z order). */
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m)
{
    f32 r[3];

    Vec_AnglesToRad(angles, r);
    Mtx_Identity(m);
    Mtx_SetRotXYZ(m, r[0], r[1], r[2]);
    return m;
}

void Mtx_Identity(f32 *m);
void Mtx_TurnY(f32 *m, f32 a);
void Mtx_TurnX(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);

/* A rotation matrix from three integer angles (Y, X, Z order). */
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m)
{
    f32 r[3];

    Vec_AnglesToRad(angles, r);
    Mtx_Identity(m);
    Mtx_TurnY(m, r[1]);
    Mtx_TurnX(m, r[0]);
    Mtx_TurnZ(m, r[2]);
    return m;
}

/* out = v * m (the rotation part); returns out. */
f32 *Mtx_RotateVec(f32 *m, f32 *v, f32 *out)
{
    Vec_Rotate(out, v, m);
    return out;
}

f32 Math_Atan2(f32 y, f32 x);

/* The 16-bit angle of the direction from (x2, z2) to (x1, z1). */
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2)
{
    f32 v[3];

    v[1] = 0.0f;
    v[0] = x1 - x2;
    v[2] = z1 - z2;
    Vec_Normalize(v);
    return (s32)(0.5f + 65536.0f * Math_Atan2(-v[2], v[0]) / 6.2831855f);
}

/* The 16-bit angle of the direction from b to a (x, z). */
u16 Vec_Heading(f32 *a, f32 *b)
{
    f32 v[3];

    v[0] = a[0] - b[0];
    v[1] = 0.0f;
    v[2] = a[2] - b[2];
    Vec_Normalize(v);
    return (s32)(0.5f + 65536.0f * Math_Atan2(-v[2], v[0]) / 6.2831855f);
}

f32 Math_Sqrtf(f32 x);

/* The 16-bit angles of a direction: *yaw about y, *pitch about x. */
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw)
{
    *yaw = (s32)(0.5f + 65536.0f * Math_Atan2(dir[0], dir[2]) / 6.2831855f);
    *pitch = (s32)(0.5f + 65536.0f * -Math_Atan2(dir[1], Math_Sqrtf(dir[0] * dir[0] + dir[2] * dir[2])) / 6.2831855f);
}

void Vec4_Transform(f32 *out, f32 *v, f32 *m);

/* The 16-bit heading of matrix m (its z axis in the x-z plane). */
u16 Mtx_Heading(f32 *m)
{
    f32 v[4];
    f32 out[4];

    v[2] = 1.0f;
    v[3] = 1.0f;
    v[0] = 0.0f;
    v[1] = 0.0f;
    Vec4_Transform(out, v, m);
    return (s32)(0.5f + 65536.0f * Math_Atan2(-(out[2] - m[14]), out[0] - m[12]) / 6.2831855f);
}

void Vec_Sub(f32 *out, f32 *a, f32 *b);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
f32 Vec_Dot(f32 *a, f32 *b);
extern f32 gCamPos[];

/* Turns the x and y axes of m about its z axis so that its x axis is square to the direction from pos to the point gCamPos (a billboard). */
void Mtx_Billboard(f32 *m, f32 *pos)
{
    f32 d[3];
    f32 z[3];
    f32 x[3];
    f32 y[3];

    Vec_Sub(d, gCamPos, pos);
    Vec_Copy(z, m + 8);
    Vec_Cross(x, d, z);
    Vec_Normalize(x);
    Vec_Cross(y, z, x);
    if (!(Vec_Dot(y, y) <= 0.0001f)) {
        Vec_Cross(x, y, z);
        Vec_Copy(m, x);
        Vec_Copy(m + 4, y);
    }
}

f32 Vec_Length(f32 *v);
void Mtx_Billboard(f32 *m, f32 *pos);

/* Mtx_Billboard keeping the scale of each axis of m. */
void Mtx_BillboardScaled(f32 *m, f32 *pos)
{
    f32 sx = Vec_Length(m);
    f32 sy = Vec_Length(m + 4);
    f32 sz = Vec_Length(m + 8);

    Vec_Normalize(m + 8);
    Mtx_Billboard(m, pos);
    m[0] *= sx;
    m[1] *= sx;
    m[2] *= sx;
    m[4] *= sy;
    m[5] *= sy;
    m[6] *= sy;
    m[8] *= sz;
    m[9] *= sz;
    m[10] *= sz;
}

/* out = v normalized, times len. */
void Vec_Resize(f32 *out, f32 len, f32 *v)
{
    Vec_Copy(out, v);
    Vec_Normalize(out);
    out[0] *= len;
    out[1] *= len;
    out[2] *= len;
}

void Vec_Cross(f32 *out, f32 *a, f32 *b);
f32 Vec_Dot(f32 *a, f32 *b);

/* A rotation matrix whose axis `axis` (0 x, 1 y, 2 z) is the direction dir
 * (normalized in place); the other two axes are made from world z (or x/y,
 * when dir is too close to it). */
void Mtx_FromDirection(f32 *dir, f32 *m, u8 axis)
{
    f32 x[4];
    f32 y[4];
    f32 z[4];
    f32 up[3];

    Vec_Normalize(dir);
    switch (axis) {
    case 0:
        up[0] = 0.0f;
        up[1] = 0.0f;
        up[2] = 1.0f;
        Vec_Cross(y, up, dir);
        if (Vec_Dot(y, y) < 0.001f) {
            up[2] = 0.0f;
            up[0] = 1.0f;
            Vec_Cross(y, up, dir);
        }
        Vec_Normalize(y);
        Vec_Copy(x, dir);
        Vec_Cross(z, x, y);
        break;
    case 1:
        up[0] = 0.0f;
        up[1] = 0.0f;
        up[2] = 1.0f;
        Vec_Cross(x, dir, up);
        if (Vec_Dot(x, x) < 0.001f) {
            up[2] = 0.0f;
            up[1] = 1.0f;
            Vec_Cross(x, up, dir);
        }
        Vec_Normalize(x);
        Vec_Copy(y, dir);
        Vec_Cross(z, x, y);
        break;
    case 2:
        up[0] = 0.0f;
        up[1] = 1.0f;
        up[2] = 0.0f;
        Vec_Cross(x, up, dir);
        if (Vec_Dot(x, x) < 0.001f) {
            up[2] = 1.0f;
            up[1] = 0.0f;
            Vec_Cross(x, dir, up);
        }
        Vec_Normalize(x);
        Vec_Copy(z, dir);
        Vec_Cross(y, z, x);
        break;
    }
    Mtx_Identity(m);
    Vec_Copy(m, x);
    Vec_Copy(m + 4, y);
    Vec_Copy(m + 8, z);
}

void Vec_Set(f32 *v, f32 x, f32 y, f32 z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

/* out = a - b. */
void Vec_Sub(f32 *out, f32 *a, f32 *b)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

/* out = a + b. */
void Vec_Add(f32 *out, f32 *a, f32 *b)
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

/* out = a - b (as Vec_Sub). */
void Vec_Diff(f32 *out, f32 *a, f32 *b)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

/* out = v * s. */
void Vec_Scale(f32 *out, f32 *v, f32 s)
{
    out[0] = v[0] * s;
    out[1] = v[1] * s;
    out[2] = v[2] * s;
}

f32 Vec_Length(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);

/* The unit normal of the triangle a, b, c; 0 when it is degenerate. */
f32 Vec_Length(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);

/* The unit normal of the triangle a, b, c; 0 when it is degenerate. */
s32 Tri_Normal(f32 *a, f32 *b, f32 *c, f32 *out)
{
    f32 ab[3];
    f32 cb[3];

    Vec_Sub(ab, a, b);
    Vec_Sub(cb, c, b);
    Vec_Cross(out, cb, ab);
    Vec_Normalize(out);
    if (Vec_Length(out) == 0.0f) {
        return 0;
    }
    return 1;
}

/* The 2D cross product (b - a) x (c - a) in x-y; products one float step apart count as equal. */
f32 Tri_Cross2D(f32 *a, f32 *b, f32 *c)
{
    f32 v[2];
    s32 pi;
    s32 qi;

    v[0] = (b[0] - a[0]) * (c[1] - a[1]);
    v[1] = (c[0] - a[0]) * (b[1] - a[1]);
    qi = *(s32 *)&v[1];
    pi = *(s32 *)&v[0];
    if (pi == qi + 1) {
        v[0] = v[1];
    }
    if (pi == qi - 1) {
        v[0] = v[1];
    }
    return v[0] - v[1];
}

/* Whether p is inside the triangle a, b, c (by Tri_Cross2D): 1 when the
 * triangle's winding is negative, 2 when positive, 0 outside or when the
 * triangle is degenerate. */
u8 Tri_Side2D(f32 *a, f32 *b, f32 *c, f32 *p)
{
    f32 w = Tri_Cross2D(a, b, c);

    if (w == 0.0f) {
        return 0;
    }
    if (w < 0.0f) {
        if (!(Tri_Cross2D(a, b, p) <= 0.0f)) {
            return 0;
        }
        if (!(Tri_Cross2D(b, c, p) <= 0.0f)) {
            return 0;
        }
        return !(Tri_Cross2D(c, a, p) > 0.0f) ? 1 : 0;
    }
    if (Tri_Cross2D(a, b, p) < 0.0f) {
        return 0;
    }
    if (Tri_Cross2D(b, c, p) < 0.0f) {
        return 0;
    }
    if (Tri_Cross2D(c, a, p) < 0.0f) {
        return 0;
    }
    return 2;
}

/* Whether p lies inside the triangle tri (three f32[3] corners), by the
 * side Tri_Side2D reports and the winding (Tri_Cross2D). */
s32 Tri_Contains2D(f32 *tri, f32 *p)
{
    u8 side;
    s8 sum;
    s32 cw;

    cw = (s32)Tri_Cross2D(tri, tri + 3, tri + 6) <= 0;
    sum = 0;
    side = Tri_Side2D(tri, tri + 3, tri + 6, p);
    if (side != 0) {
        if (cw) {
            if (side == 1) {
                sum++;
            } else {
                sum--;
            }
        } else if (side == 2) {
            sum++;
        } else {
            sum--;
        }
    }
    return sum > 0;
}

f32 Math_Absf(f32 x);

/* dst = src with the components below 0.001 in size made 0. */
void Vec_SnapSmall(f32 *dst, f32 *src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    if (Math_Absf(src[0]) < 0.001f) {
        dst[0] = 0.0f;
    }
    if (Math_Absf(src[1]) < 0.001f) {
        dst[1] = 0.0f;
    }
    if (Math_Absf(src[2]) < 0.001f) {
        dst[2] = 0.0f;
    }
}

void Mtx_RotateX(f32 *m, f32 a);
void Mtx_Identity(f32 *m);
void Mtx_Mul3x3(f32 *m, f32 *a, f32 *b);

/* m = the rotation by a about x times m (3x3). */
void Mtx_TurnX(f32 *m, f32 a)
{
    f32 r[16];

    Mtx_Identity(r);
    Mtx_RotateX(r, a);
    Mtx_Mul3x3(m, r, m);
}

void Mtx_RotateY(f32 *m, f32 angle);
void Mtx_Identity(f32 *m);
void Mtx_Mul3x3(f32 *m, f32 *a, f32 *b);

/* m = the rotation by a about y times m (3x3). */
void Mtx_TurnY(f32 *m, f32 a)
{
    f32 r[16];

    Mtx_Identity(r);
    Mtx_RotateY(r, a);
    Mtx_Mul3x3(m, r, m);
}

void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_Identity(f32 *m);
void Mtx_Mul3x3(f32 *m, f32 *a, f32 *b);

/* m = the rotation by a about z times m (3x3). */
void Mtx_TurnZ(f32 *m, f32 a)
{
    f32 r[16];

    Mtx_Identity(r);
    Mtx_RotateZ(r, a);
    Mtx_Mul3x3(m, r, m);
}

f32 Math_Sqrtf(f32 x);

/* The 16-bit elevation angle of the direction from a to b. */
u16 Vec_Elevation(f32 *a, f32 *b)
{
    f32 dx = b[0] - a[0];
    f32 dy = b[1] - a[1];
    f32 dz = b[2] - a[2];

    return (s32)(0.5f + 65536.0f * Math_Atan2(dy, Math_Sqrtf(dx * dx + dz * dz)) / 6.2831855f);
}

/* out = a * t + b * (1 - t). */
void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t)
{
    f32 s = 1.0f - t;

    out[0] = a[0] * t + b[0] * s;
    out[1] = a[1] * t + b[1] * s;
    out[2] = a[2] * t + b[2] * s;
}

f32 Math_Cos(f32 x);

f32 Math_CosAngle(s32 a)
{
    return Math_Cos(3.1415927f * a / 32768.0f);
}

/* atan2 as a 16-bit angle (0x10000 a turn). */
s32 Math_Atan2Angle(f32 y, f32 x)
{
    return 10430.378f * Math_Atan2(y, x);
}

/* out = v * m plus the translation of m. */
void Vec_Transform(f32 *out, f32 *v, f32 *m)
{
    Vec_Rotate(out, v, m);
    out[0] = out[0] + m[12];
    out[1] = out[1] + m[13];
    out[2] = out[2] + m[14];
}

void Mtx_FromAxisAngle(f32 *m, f32 *axis, f32 fparg0)
{
    f32 q[3];

    Quat_Set((f32 *) axis, q, fparg0);
    Quat_ToMatrix(q, (void *) m);
}

f32 Vec_Length(f32 *v);

/* The distance between a and b in the x-z plane. */
f32 Vec_DistanceXZ(f32 *a, f32 *b)
{
    f32 d[3];

    d[0] = a[0] - b[0];
    d[1] = 0.0f;
    d[2] = a[2] - b[2];
    return Vec_Length(d);
}

/* Turns the 16-bit angle *cur toward *target by at most step (the shorter way); returns the step taken. */
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step)
{
    s32 d;
    s32 dist;

    *target = (u16)*target;
    d = (u16)(*target - *cur);
    dist = (d >= 0x8000) ? (u16)-d : d;
    if (dist < step) {
        step = dist;
    }
    if (d >= 0x8000) {
        *cur -= step;
    } else {
        *cur += step;
    }
    return step;
}

/* A bone (0x190 bytes) as its scale. */
typedef struct BoneScale {
    u8 pad00[0xDC];
    f32 scale[3];               /* 0xDC */
    u8 padE8[0x190 - 0xE8];
} BoneScale;

extern PlRec gPlayerRecs[][32];

/* Gives the player its 32 records (both matrices identity), marks them all and sets the
 * scale of its model's first 21 bones (in both bone sets) and records to 1. MWCC unrolls
 * the last loop by 7 only while its body is this small: the chained stores, the bone
 * pointers stepped and the records typed (no cast) in Player. */
void Player_InitRecords(Player *p)
{
    s16 i;
    PlRec *r;
    BoneScale *b;
    BoneScale *a;
    Model *m;

    r = gPlayerRecs[p->index];
    for (i = 0; i < 32; i++, r++) {
        p->partRecs[i] = r;
        Mtx_Identity(r->m0);
        Mtx_Identity(r->m1);
    }
    for (i = 0; i < 32; i++) {
        p->meshShown[i] = 1;
    }
    m = p->model;
    b = (BoneScale *)m->partBones[0];
    a = (BoneScale *)m->partBones2[0];
    for (i = 0; i < 21; i++) {
        p->partRecs[i]->scale[0] = b->scale[0] = a->scale[0] = 1.0f;
        p->partRecs[i]->scale[1] = b->scale[1] = a->scale[1] = 1.0f;
        p->partRecs[i]->scale[2] = b->scale[2] = a->scale[2] = 1.0f;
        a++;
        b++;
    }
}

/* Sets the flag at +0x4C8 of entry sel of the range of kind (0x12: entries 2 and 3, 0xE: 4 and 5) and clears the others. */
void PlayerRec_SelectInRange(u8 *p, u8 kind, u8 sel)
{
    u8 first;
    u8 n;
    u8 i;

    switch (kind) {
    case 0x12:
        n = 2;
        first = n;
        break;
    case 0xE:
        n = 2;
        first = 4;
        break;
    default:
        return;
    }
    for (i = first; i < first + n; i++) {
        if (i == sel + first) {
            p[0x4C8 + i] = 1;
        } else {
            p[0x4C8 + i] = 0;
        }
    }
}

void FxBlock_Clear(EquipFxSwing *p)
{
    s16 i;

    i = 0;
    do {
        p->vel[0][0] = 0.0f;
        p->vel[0][1] = 0.0f;
        p->vel[0][2] = 0.0f;
        p->vel[1][0] = 0.0f;
        p->vel[1][1] = 0.0f;
        p->vel[1][2] = 0.0f;
        p->calls = 0;
        p->unk01[0] = 0;
        p->unk01[1] = 0;
        p->unk01[2] = 0;
        i++;
        p++;
    } while (i < 4);
}

void FxBlock_Clear(EquipFxSwing *p);

void Player_ClearFxBlock(Player *p)
{
    FxBlock_Clear(p->equipFx);
}

void Player_RunEquipEffects(Player *p);

/* Player_RunEquipEffects for each player in use (online, only those flagged at +0x152E of the lobby state at *0x8EA908). */
void Players_RunEquipEffects(void)
{
    s16 i;
    Player *p;

    for (i = 0, p = gPlayers; i < 10; i++, p++) {
        if (p->active != 0 && p->visible != 0) {
            if (gScene.online == 0 || *(s8 *)(*(s32 *)&gLobby + i + 0x152E) != 0) {
                Player_RunEquipEffects(p);
            }
        }
    }
}

/* The effect lists of Effect_RunList: per kind, a table of {key, ids}
 * ended by key -1; ids are indices into ((f32 (*)[4])gEffectListPoints), ended by -1. */
typedef struct FxIdList {
    s32 key;
    s16 *ids;
} FxIdList;
extern FxIdList *D_0044A858[];
extern FxIdList *D_0044A860[];
extern s16 gLobbyActorFxIds[];
extern u32 gEffectListPoints[];
s32 EquipFx_ChainUpdate(EquipFxSwing *a, EmBone *b1, EmBone *b2, EmBone *m, f32 **list, s16 n);
/* declared without parameters, defined K&R: its caller passes key and alt
 * unconverted, the definition masks them */
void Effect_RunList(EquipFxSwing *a, EmBone *b1, EmBone *b2, EmBone *m, s32 key, s32 alt, s32 kind); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it) */

/* A chain of effect bones (the rows of ((f32 (*)[4])gEffectListPoints) that Effect_RunList lists). */
typedef struct FxChain {
    u8 range;                   /* 0x0: gEquipFxRanges, the bones */
    u8 flags;                   /* 0x1: 1: clamp every bone, not only the first */
    s16 up;                     /* 0x2: gEquipFxUps */
    s16 base;                   /* 0x4: gEquipFxBases */
    s16 limit;                  /* 0x6: gEquipFxLimits, a pair per chain */
    s16 target;                 /* 0x8: gEquipFxTargets, -1: none */
    s16 clamp;                  /* 0xA: gEquipFxClamps, -1: none */
    s16 damp;                   /* 0xC: gEquipFxDamps */
    s16 pull;                   /* 0xE: gEquipFxPulls */
} FxChain;
typedef struct FxRange {
    s16 start;
    u16 end;
} FxRange;
extern FxRange gEquipFxRanges[];
extern f32 gEquipFxUps[];
extern f32 gEquipFxBases[][3];
extern f32 gEquipFxLimits[];
extern f32 gEquipFxTargets[][3];
extern f32 gEquipFxClamps[][6];
extern f32 gEquipFxDamps[];
extern f32 gEquipFxPulls[];
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
void Mtx_Inverse(f32 *dst, f32 *src);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
s32 Vec_ProjectOnDir(f32 *p, f32 *o, f32 *t, f32 *out, f32 r);

/* Moves the bone chains of list (FxChain rows) on m's bones: from the third call on (a[0], one
 * 0x20-byte record per chain), each bone's direction follows its parent (b1 first), pulled by
 * gEquipFxPulls, swung by the chain's up vector with its velocity at a + 4 (damped by
 * gEquipFxDamps), pushed off b2's target by Vec_ProjectOnDir, and clamped by gEquipFxClamps.
 * FAKEMATCH: (round 30) two forms only mirror the game's allocation. (1) The copies b1, b2,
 * m are made again in the outer loop's step: a plain copy of a parameter is propagated away
 * and spills as the parameter (a 4-byte slot), where the game spills b1, b2 and m as locals
 * (16-byte slots 0x100..0xE0); two definitions keep them variables, and the step copies give
 * no code. (2) The clamp reads dir[1] and dir[2] through dy and dz, set before the loop: the
 * game numbers its two hoisted addresses before bone and st (s4, s3 over s2, s1), which
 * MWCC's own hoisting doesn't (docs/notes/nm/EquipFx_ChainUpdate.md). */
#pragma optimization_level 4
s32 EquipFx_ChainUpdate(EquipFxSwing *pa, EmBone *pb1, EmBone *pb2, EmBone *pm, f32 **list, s16 pn)
{
    EmBone *b1 = pb1;
    EmBone *b2 = pb2;
    EmBone *m = pm;
    EquipFxSwing *a;
    s32 n = pn;
    s16 i;
    s16 j;
    s16 k;
    s32 end;
    FxChain **l;
    f32 *dy;
    f32 *dz;
    EmBone *bone;
    u8 *st;
    s32 cl;
    f32 *c;
    f32 rx;
    f32 rz;
    f32 vec[4];
    f32 wpos[4];
    f32 tip[4];
    f32 upv[4];
    f32 dir[4];
    f32 tgt[4];
    f32 hit[4];
    f32 side[4];
    f32 side2[4];
    f32 up0[4];
    f32 off[4];
    f32 pos[4];
    f32 cur[16];
    f32 inv[16];
    f32 mtx2[16];

    i = 0;
    if (i < n) {
    l = (FxChain **)list;
    a = pa;
    dy = &dir[1];
    dz = &dir[2];
    do {
        up0[0] = gEquipFxBases[(*l)->base][0];
        up0[1] = gEquipFxBases[(*l)->base][1];
        up0[2] = gEquipFxBases[(*l)->base][2];
        Vec_RotateSelf(up0, (f32 *)b1);
        if (a->calls < 2) {
            a->calls++;
            continue;
        }
        if ((*l)->target != -1) {
            Vec_Copy(tgt, gEquipFxTargets[(*l)->target]);
            Mtx_Copy(mtx2, (f32 *)b2);
            Vec_RotateSelf(tgt, mtx2);
        }
        upv[0] = 0.0f;
        upv[1] = gEquipFxUps[(*l)->up];
        upv[2] = 0.0f;
        Vec_Add(upv, upv, up0);
        k = gEquipFxRanges[(*l)->range].start;
        end = gEquipFxRanges[(*l)->range].end;
        j = 0;
        if (k < end) {
            st = (u8 *) a; /* the record walked by bone: vel[j] at st + 4 (the game keeps the record base) */
            bone = &m[k];
            do {
            Mtx_GetTrans(wpos, bone->world);
            Mtx_GetTrans(off, bone->local);
            if (j == 0) {
                Mtx_Copy(cur, (f32 *)b1);
            } else {
                Mtx_Mul(inv, bone->parent->local, cur);
                Mtx_Copy(cur, inv);
            }
            Vec_RotateSelf(off, cur);
            Vec_Add(pos, &cur[12], off);
            Mtx_GetTrans(off, bone[1].local);
            Vec_Rotate(vec, off, (f32 *)bone);
            Vec_Sub(dir, wpos, pos);
            Vec_Scale(dir, dir, gEquipFxPulls[(*l)->pull]);
            Vec_Add(vec, vec, dir);
            Vec_Cross(side, vec, upv);
            Vec_Cross(side2, side, vec);
            Vec_Normalize(side2);
            Vec_Scale(side2, side2, Vec_Dot(side2, upv));
            Vec_Add((f32 *)(st + 4), (f32 *)(st + 4), side2);
            Vec_Add(vec, vec, (f32 *)(st + 4));
            Vec_Add(tip, pos, vec);
            Mtx_Inverse(inv, cur);
            if ((*l)->target == -1) {
                dir[0] = -vec[0];
                dir[1] = -vec[1];
                dir[2] = -vec[2];
                Vec_RotateSelf(dir, inv);
                Vec_Normalize(dir);
            } else {
                Mtx_Copy(mtx2, (f32 *)b2);
                if (Vec_ProjectOnDir(tip, &mtx2[12], tgt, hit, gEquipFxLimits[j + (*l)->limit * 2]) != 0) {
                    Vec_Add(tip, tip, hit);
                }
                Vec_Sub(dir, pos, tip);
                Vec_RotateSelf(dir, inv);
                Vec_Normalize(dir);
            }
            if (j == 0 || ((*l)->flags & 1)) {
                cl = (*l)->clamp;
                if (cl != -1) {
                    c = gEquipFxClamps[(s16)cl];
                    if (dir[0] < c[0]) {
                        dir[0] = c[0];
                    } else if (dir[0] > c[1]) {
                        dir[0] = c[1];
                    }
                    if (*dy < c[2]) {
                        *dy = c[2];
                    } else if (*dy > c[3]) {
                        *dy = c[3];
                    }
                    if (*dz < c[4]) {
                        *dz = c[4];
                    } else if (*dz > c[5]) {
                        *dz = c[5];
                    }
                }
            }
            rx = Math_Atan2(dir[2], dir[1]);
            rz = Math_Atan2(-dir[0], Math_Sqrtf(dir[1] * dir[1] + dir[2] * dir[2]));
            Mtx_Identity(inv);
            Mtx_RotateZ(inv, rz);
            Mtx_RotateX(inv, rx);
            Vec_Copy(&bone->local[0], &inv[0]);
            Vec_Copy(&bone->local[4], &inv[4]);
            Vec_Copy(&bone->local[8], &inv[8]);
            Vec_Scale((f32 *)(st + 4), (f32 *)(st + 4), gEquipFxDamps[(*l)->damp]);
                k++;
                bone++;
                st += 0xC;
                j++;
            } while (k < end);
        }
    } while (a++, l++, b1 = pb1, b2 = pb2, m = pm, ++i < n);
    }
    return 0;
}
#pragma optimization_level reset

/* Runs the effects listed for key under kind (the second table when alt) on m
 * (when its +0xC2 is at least 5). */
void Effect_RunList(a, b1, b2, m, key, alt, kind)
    EquipFxSwing *a;
    EmBone *b1;
    EmBone *b2;
    EmBone *m;
    u16 key;
    u8 alt;
    u8 kind;
{
    f32 *list[4];
    FxIdList *l;
    s16 i;
    s16 *ids;
    s16 id;
    s16 n;

    if (m->count > 4) {
        if (alt == 0) {
            l = D_0044A858[kind];
        } else {
            l = D_0044A860[kind];
        }
        ids = NULL;
        for (; l->key != -1; l++) {
            if (l->key == key) {
                ids = l->ids;
                break;
            }
        }
        if (ids != NULL) {
            i = 0;
            n = 0;
            for (; i < 4; i++) {
                if ((id = *ids) == -1) {
                    break;
                }
                list[n++] = ((f32 (*)[4])gEffectListPoints)[id];
                ids++;
            }
            if (n != 0) {
                EquipFx_ChainUpdate(a, b1, b2, m, list, n);
            }
        }
    }
}

EmBone *Em_GetBone(void *obj, s16 bone);
s32 Armor_LacksSexModel(s32 id, u8 a);

/* A player's swinging equipment parts (equipFx), from bones 0x14 and 0xA, on the bones of its part 2's
 * model when it has one. */
void Player_RunEquipEffects(Player *p)
{
    if (p->partModels[2] != NULL) {
        s32 k = p->armourKind[2];
        s32 flag = Armor_LacksSexModel(p->equip[2].id, p->gender) != 1;
        EmBone *b1 = Em_GetBone(p, 0x14);

        Effect_RunList(p->equipFx, b1, Em_GetBone(p, 0xA), p->partModels[2]->bones, k, flag, p->gender);
    }
}

/* Runs the fixed effect list gLobbyActorFxIds on m (when its +0xC2 is at least 27). */
void Effect_RunFixedList(EquipFxSwing *a, EmBone *b1, EmBone *b2, EmBone *m)
{
    f32 *list[4];
    s16 i;
    s16 *ids;
    s16 id;
    s16 n;

    if (m->count > 0x1A) {
        ids = gLobbyActorFxIds;
        if (ids != NULL) {
            i = 0;
            n = 0;
            for (; i < 4; i++) {
                if ((id = *ids) == -1) {
                    break;
                }
                list[n++] = ((f32 (*)[4])gEffectListPoints)[id];
                ids++;
            }
            if (n != 0) {
                EquipFx_ChainUpdate(a, b1, b2, m, list, n);
            }
        }
    }
}

void Model_ResetAll(void)
{
    memset((void *) gModels, 0, 0x4000U);
    Material_Release(0, 0x400);
    Bone_Release(0, 0x800);
    MatSet_Release(0, 0x180);
    Model_Release(0, 0x80);
}

/* Draw state settings: each field indexes one of the D_0042FBxx tables. */
typedef struct DrawStates {
    s32 unk0;
    s32 s15;        /* 0x04: gDrawState15Values, state 0x15 */
    s32 s02;        /* 0x08: gDrawState02Values, state 2 */
    s32 s00;        /* 0x0C: gDrawState00Values, state 0 */
    s32 s0C;        /* 0x10: gDrawState0CValues, state 0xC; flag bit 15 */
    s32 s01;        /* 0x14: gDrawState01Values, state 1 */
    s32 unk18;
    s32 s62;        /* 0x1C: gDrawState62Values, state 0x62 */
    s32 s12;        /* 0x20: gDrawState12Values, state 0x12 */
    s32 s66;        /* 0x24: gDrawState66Values, state 0x66 (1: also 0x67); flag bit 1 */
    s32 s5D;        /* 0x28: 5 or 6 sets state 0x5D to that << 24, else 0 */
    s32 f2;         /* 0x2C: flag bits 2-5 */
    s32 f6;         /* 0x30: flag bits 6-9 */
    s32 f10;        /* 0x34: flag bits 10-11 */
    s32 unk38[2];
    s32 f12;        /* 0x40: flag bit 12 */
    s32 f13;        /* 0x44: flag bits 13-14 */
} DrawStates;

extern s32 gDrawState15Values[2];
extern s32 gDrawState12Values[4];
extern s32 gDrawState01Values[12];
extern s32 gDrawState02Values[4];
extern s32 gDrawState0CValues[2];
extern s32 gDrawState66Values[2];
extern s32 gDrawState62Values[4];
extern s32 gDrawState00Values[4];
s32 Gs_SetState(u8 reg, u32 v);

/* Sets the GS states of ds and packs its other settings into *flags (for Draw_ApplyStates). */
void DrawStates_Apply(DrawStates *ds, s32 *flags)
{
    Gs_SetState(0x15, gDrawState15Values[ds->s15]);
    Gs_SetState(0x12, gDrawState12Values[ds->s12]);
    Gs_SetState(1, gDrawState01Values[ds->s01]);
    Gs_SetState(2, gDrawState02Values[ds->s02]);
    Gs_SetState(0xC, gDrawState0CValues[ds->s0C]);
    Gs_SetState(0x66, gDrawState66Values[ds->s66]);
    Gs_SetState(0x62, gDrawState62Values[ds->s62]);
    Gs_SetState(0, gDrawState00Values[ds->s00]);
    if (gDrawState66Values[ds->s66] == 1) {
        Gs_SetState(0x67, -1);
    }
    switch (ds->s5D) {
    case 6:
        Gs_SetState(0x5D, 0x06000000);
        break;
    case 5:
        Gs_SetState(0x5D, 0x05000000);
        break;
    default:
    case 0:
        Gs_SetState(0x5D, 0);
        break;
    }
    *flags = 1;
    *flags |= (ds->s66 & 1) << 1;
    *flags |= (ds->f2 & 0xF) << 2;
    *flags |= (ds->f6 & 0xF) << 6;
    *flags |= (ds->f10 & 3) << 10;
    *flags |= (ds->f12 & 1) << 12;
    *flags |= (ds->f13 & 3) << 13;
    *flags |= (ds->s0C & 1) << 15;
}

extern s32 gDrawBlendFactors[];
extern s32 gDrawBlendOps[];
extern s32 gDrawState63Values[2];
extern s32 gDrawState64Values[4];
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetBlendOp(s32 a);
void Gfx_SetState63(s32 a);

/* Applies packed draw states when bit 0 is set: bits 2-5 and 6-9 Gfx_SetBlend, 10-11 Gfx_SetBlendOp, 12 Gfx_SetState63, 13-14 mode 0x64, 15 mode 0xC. */
void Draw_ApplyStates(u32 f)
{
    if (f & 1) {
        Gfx_SetBlend(gDrawBlendFactors[(f >> 2) & 0xF], gDrawBlendFactors[(f >> 6) & 0xF]);
        Gfx_SetBlendOp(gDrawBlendOps[(f >> 10) & 3]);
        Gfx_SetState63(gDrawState63Values[(f >> 12) & 1]);
        Gs_SetState(0x64, gDrawState64Values[(f >> 13) & 3]);
        Gs_SetState(0xC, gDrawState0CValues[(f >> 15) & 1]);
    }
}

void Draw_SetDefaultState(void)
{
    Gfx_SetBlend(4, 5);
    Gfx_SetBlendOp(0);
    Gfx_SetState63(1);
}

typedef struct SkelHeader {
    s16 unk0;
    s16 count;
    s32 size;
    struct SkelHeader *next;
    s32 unkC;
} SkelHeader;
/* A bone slot of the 0x190-byte pool (Bone_Get). */
s32 SkelFile_BoneCount(void *f);
s32 SkelFile_PartCount(void *f);
s32 SkelFile_PartBoneCount(void *f, s32 i);
s32 SkelFile_BuildPart(SkelHeader *dst, SkelHeader *h, void *f, s32 part);
void *Skeleton_CopyData(u8 *obj);
void Bone_InitTreeRoot(EmBone *bones, s32 h);
s32 Bone_Alloc(s32 n);
EmBone *Bone_Get(s32 i);
void Bone_Reserve(s32 first, s32 n);

/*
 * Sets model m up with kind 0 (none), 1 (one bone set) or 2 (two sets) of the skeleton
 * file at base + 0x128000 (+ 0x298000 with alt): its bones, and for each part its skeleton
 * copy and first bones. `off` holds the part count for kind 2 and `first` is reused as the
 * part counter: their registers (the permuter found the first).
 */
void Model_InitSkeleton(u8 *base, Model *m, s32 kind, s32 alt)
{
    EmBone *bones;
    s32 b;
    s32 off;
    SkelHeader h;
    EmBone *bone;
    s32 first;
    EmBone *bones2;
    s32 n;

    if (alt != 0) {
        base = (u8 *)base + 0x298000;
    } else {
        base = (u8 *)base + 0x128000;
    }
    m->boneKind = kind;
    switch (kind) {
    case 0:
        m->boneFirst = 0;
        m->boneCount = 0;
        m->bones = NULL;
        m->bones2 = NULL;
        m->partCount = 0;
        return;
    case 1:
        n = SkelFile_BoneCount(base);
        break;
    case 2:
        off = SkelFile_BoneCount(base);
        n = off * 2;
        break;
    }
    first = Bone_Alloc(n);
    if (first < 0) {
        return;
    }
    m->boneFirst = first;
    m->boneCount = n;
    m->bones = Bone_Get(first);
    if (kind == 2) {
        m->bones2 = Bone_Get(first + n / 2);
    } else {
        m->bones2 = NULL;
    }
    Bone_Reserve(first, n);
    m->partCount = SkelFile_PartCount(base);
    b = 0;
    bones = m->bones;
    bones2 = m->bones2;
    for (first = 0; first < m->partCount; first++) {
        SkelFile_BuildPart(&h, (SkelHeader *)gLoadBuf, base, first);
        m->partSkels[first] = (s32)Skeleton_CopyData((u8 *)&h);
        off = b * sizeof(EmBone);
        Bone_InitTreeRoot((EmBone *)((u8 *)bones + off), m->partSkels[first]);
        m->partBones[first] = (EmBone *)((u8 *)bones + off);
        if (kind == 2) {
            Bone_InitTreeRoot((EmBone *)((u8 *)bones2 + off), m->partSkels[first]);
            m->partBones2[first] = (EmBone *)((u8 *)bones2 + off);
        }
        b += SkelFile_PartBoneCount(base, first);
    }
}

/* Model_InitMatSets: sets up a model's material sets (0x8C-byte records) from its file:
 * draw states, texture is and each set's model, then its skeleton. */
extern char D_0046CEA0[];
extern char D_0046CEC0[];
extern char D_0046CEE0[];
extern char D_0046CF00[];
extern s32 gMatSetState15[];
extern s32 D_0042FB04[];
extern s32 D_0042FB14[];
extern s32 gMatSetState12[];
extern s32 D_0042FB34;
extern s32 gMatSetModelKinds[];
s32 ModelFile_PartCount(u8 *f);
s32 MatSet_FindFree(s32 n);
ModelMatSet *MatSet_Get(s32 i);
void MatSet_Reserve(s32 i, s32 n);
s32 ModelFile_GetDrawStates(void *ds, u8 *f, s32 i);
s32 ModelFile_PartMatCount(u8 *f, u32 idx);
s32 ModelFile_GetPartMats(u8 *f, u32 idx, s32 *out);
s32 ModelFile_LoadMesh(MeshDesc *a, u8 *h, u32 idx);
s32 Model_Create(MeshDesc *hdr, u32 flags);
s32 SysHeap_GetMark(void *mark);
void SysHeap_Release(void *mark);

void Model_InitMatSets(Model *m, s32 arg1, s32 kind, s32 alt)
{
    MeshDesc desc;
    DrawStates ds;
    u8 mark[8];
    s32 j;
    ModelMatSet *p;
    ModelMat *mats;
    u8 *base;
    s32 i;
    s32 sel;
    u16 flags;

    flags = arg1;
    sel = (flags & 0x100) / 256;
    base = m->file;
    m->matSetCount = ModelFile_PartCount(base);
    if (m->matSetCount == 0) {
        Debug_Message(D_0046CEA0, 0, 0, 0);
    }
    i = MatSet_FindFree(m->matSetCount);
    if (i < 0) {
        return;
    }
    m->matSetFirst = i;
    p = m->matSets = MatSet_Get(i);
    MatSet_Reserve(i, m->matSetCount);
    Gs_SetState(0xC, 0);
    mats = m->mats;
    for (i = 0; i < m->matSetCount; i++, p++) {
        if (ModelFile_GetDrawStates(&ds, base, i) >= 0) {
            DrawStates_Apply(&ds, &p->flags);
            if (flags & 0x800) {
                Gs_SetState(0x12, D_0042FB34);
            }
            if (flags & 0x1000) {
                Gs_SetState(0x66, 1);
            }
            if (flags & 0x2000) {
                Gs_SetState(0, 0x20);
            }
        } else {
            p->flags = 0;
            Gs_SetState(0x66, 0);
            Gs_SetState(0xC, 0);
            Gs_SetState(0x62, 0);
            Gs_SetState(0, 0);
            Gs_SetState(2, 0);
            Gs_SetState(0x15, gMatSetState15[0]);
            if (flags & 0x800) {
                Gs_SetState(0x12, D_0042FB34);
            } else {
                Gs_SetState(0x12, gMatSetState12[0]);
            }
            if (flags & 0x200) {
                Gs_SetState(1, D_0042FB14[0]);
            } else {
                Gs_SetState(1, D_0042FB04[0]);
            }
            if (flags & 0x2000) {
                Gs_SetState(0, 0x20);
            } else {
                Gs_SetState(0, 0);
            }
        }
        p->count = ModelFile_PartMatCount(base, i);
        if (p->count == 0) {
            Debug_Message(D_0046CEC0, (s16)i, m->matSetCount, 0);
        }
        if (ModelFile_GetPartMats(base, i, p->mat) == 0) {
            Debug_Message(D_0046CEE0, (s16)i, m->matSetCount, 0);
        }
        for (j = 0; j < p->count; j++) {
            Gs_SetState((u8)(j + 0x3A), (s32)&mats[p->mat[j]]);
        }
        SysHeap_GetMark(mark);
        if (ModelFile_LoadMesh(&desc, base, i) != 0) {
            p->id = Model_Create(&desc, gMatSetModelKinds[sel]);
        } else {
            Debug_Message(D_0046CF00, (s16)i, m->matSetCount, 0);
            p->id = -1;
        }
        SysHeap_Release(mark);
    }
    Model_InitSkeleton(base, m, kind, alt);
    Gs_SetState(0x12, 0);
    Gs_SetState(0x66, 0);
    Gs_SetState(0xC, 0);
}

/* Model_InitMatSets for a model with matrix slot slot: its material sets' matrix lists
 * (gModelPartLists[slot], -1 filled, then read by ModelFile_GetPartList), one heap mark for all. */
extern s16 gModelPartLists[][16][0x21];
s32 ModelFile_GetPartList(s16 *out, u8 *f, s32 idx);

void Model_InitMatSetsSlot(Model *m, s32 arg1, u8 slot, s32 kind)
{
    MeshDesc desc;
    DrawStates ds;
    u8 mark[8];
    s32 j;
    ModelMatSet *p;
    ModelMat *mats;
    u8 *base;
    s32 i;
    s32 sel;
    u16 flags;

    flags = arg1;
    sel = (flags & 0x100) / 256;
    base = m->file;
    m->matSetCount = ModelFile_PartCount(base);
    if (m->matSetCount == 0) {
        Debug_Message(D_0046CEA0, 0, 0, 0);
    }
    i = MatSet_FindFree(m->matSetCount);
    if (i < 0) {
        return;
    }
    m->matSetFirst = i;
    p = m->matSets = MatSet_Get(i);
    MatSet_Reserve(i, m->matSetCount);
    mats = m->mats;
    Gs_SetState(0xC, 0);
    SysHeap_GetMark(mark);
    for (i = 0; i < m->matSetCount; i++, p++) {
        if (ModelFile_GetDrawStates(&ds, base, i) >= 0) {
            DrawStates_Apply(&ds, &p->flags);
        } else {
            p->flags = 0;
            Gs_SetState(0x62, 0);
            Gs_SetState(0, 0);
            Gs_SetState(0x15, gMatSetState15[0]);
            Gs_SetState(0x66, 0);
            if (flags & 0x800) {
                Gs_SetState(0x12, D_0042FB34);
            } else {
                Gs_SetState(0x12, gMatSetState12[0]);
            }
            if (flags & 0x200) {
                Gs_SetState(1, D_0042FB14[0]);
            } else {
                Gs_SetState(1, D_0042FB04[0]);
            }
            Gs_SetState(2, 0);
        }
        p->count = ModelFile_PartMatCount(base, i);
        if (p->count == 0) {
            Debug_Message(D_0046CEC0, (s16)i, m->matSetCount, 0);
        }
        if (ModelFile_GetPartMats(base, i, p->mat) == 0) {
            Debug_Message(D_0046CEE0, (s16)i, m->matSetCount, 0);
        }
        for (j = 0; j < p->count; j++) {
            Gs_SetState((u8)(j + 0x3A), (s32)&mats[p->mat[j]]);
        }
        if (ModelFile_LoadMesh(&desc, base, i) != 0) {
            p->id = Model_Create(&desc, gMatSetModelKinds[sel]);
            if (p->id == 0) {
                Debug_Message(D_0046CF00, (s16)i, m->matSetCount, 0);
            }
        } else {
            Debug_Message(D_0046CF00, (s16)i, m->matSetCount, 0);
            p->id = -1;
        }
    }
    for (i = 0; i < m->matSetCount; i++) {
        for (j = 0; j < 0x40; j++) {
            gModelPartLists[slot][i][j] = -1;
        }
    }
    for (i = 0; i < m->matSetCount; i++) {
        ModelFile_GetPartList(gModelPartLists[slot][i], base, i);
    }
    SysHeap_Release(mark);
    Model_InitSkeleton(base, m, kind, 0);
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_335040", D_0046CEA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_335040", D_0046CEC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_335040", D_0046CEE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_335040", D_0046CF00);

/* the calls cast to s16: this unit's code takes the value as s16 */
s32 ModelFile_MatCount(u8 *f);
s32 ModelFile_ReadMat(s32 f, s32 i, ModelMat *mat);
s32 Material_FindFree(s32 n);
struct Obj50 *Material_Get(s32 i);
void Material_Reserve(s32 first, s32 n);

/* Allocates the materials of model data f (ModelFile_MatCount of them) and reads them, their texture ids made handles; -1 when the pool is full. */
s32 Model_LoadMaterials(Model *m, s32 f)
{
    s32 i;
    s32 first;
    s32 n;
    ModelMat *mat;

    n = (s16)ModelFile_MatCount((u8 *)f);
    if (n == 0) {
        Debug_Message("MATERIAL NUM IS ZERO !!", 0, 0, 0);
    }
    first = Material_FindFree(n);
    if (first < 0) {
        return -1;
    }
    m->matFirst = first;
    m->matCount = n;
    m->mats = (ModelMat *)Material_Get(first);
    Material_Reserve(first, n);
    mat = m->mats;
    for (i = 0; i < m->matCount; i++, mat++) {
        ModelFile_ReadMat(f, i, mat);
        mat->tex = gTexHandles[m->texSlot + mat->tex];
    }
    return 0;
}

/* unit's own declaration: s16 slot (the definitions, unk_333D70.c, take s32); with s32 Model_SetupWithPack differs by 15 words, Model_SetupWithPackEx by 32 */
s32 Tex_LoadPack(s32 id, s16 slot, s32 kind);
void Model_InitMatSets(Model *m, s32 a, s32 b, s32 c);
s32 Model_LoadMaterials(Model *m, s32 a);

/* Sets up model idx from data a with the texture pack file tex in slot (Model_SetupTexPack with two more arguments). */
Model *Model_SetupWithPack(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)a;
    m->texSlot = slot;
    m->texCount = Tex_LoadPack(tex, slot, 0);
    if (Model_LoadMaterials(m, a) == -1) {
        return NULL;
    }
    Model_InitMatSets(m, b, c, 0);
    return m;
}

/* unit's own declaration: as Tex_LoadPack above; with s32 Model_SetupWithPackRaw differs by 15 words */
s32 Tex_LoadPackRaw(s32 id, s16 slot, s32 kind);

/* As Model_SetupWithPack with Tex_LoadPackRaw, and Model_InitMatSets mode 1. */
Model *Model_SetupWithPackRaw(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)a;
    m->texSlot = slot;
    m->texCount = Tex_LoadPackRaw(tex, slot, 0);
    if (Model_LoadMaterials(m, a) == -1) {
        return NULL;
    }
    Model_InitMatSets(m, b, c, 1);
    return m;
}

/* unit's own declaration: s16 slot, as Tex_LoadPack above */
s32 Tex_LoadPackLz(s32 id, s16 slot, s32 kind, s32 d);

/* As Model_SetupWithPack with Tex_LoadPackLz (one more argument, d). */
Model *Model_SetupWithPackEx(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c, s32 d)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)a;
    m->texSlot = slot;
    m->texCount = Tex_LoadPackLz(tex, slot, 0, d);
    if (Model_LoadMaterials(m, a) == -1) {
        return NULL;
    }
    Model_InitMatSets(m, b, c, 0);
    return m;
}
void Tex_LoadPackFile(s32 *pack, s16 slot, s32 kind, u16 d); /* unit's own declaration: with the definition's void (TexPackHead *p, s32 slot, s32 kind, s32 d) (sub_main/unk_333D70.c), Model_SetupLoadedPack 8 of 62 words differ */

/* As Model_SetupWithPack with an already loaded texture pack (Tex_LoadPackFile), or no texture when pack is NULL. */
Model *Model_SetupLoadedPack(s16 idx, s32 a, s32 slot, s32 *pack, s32 b, s32 d, s32 c)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)a;
    m->texSlot = (u16)slot;
    if (pack != NULL) {
        m->texCount = *pack;
        Tex_LoadPackFile(pack, slot, 0, d);
    } else {
        m->texCount = 0;
    }
    if (Model_LoadMaterials(m, a) == -1) {
        return NULL;
    }
    Model_InitMatSets(m, b, c, 0);
    return m;
}
void Tex_LoadPackLzHigh(s32 *pack, s16 slot, s32 kind, u16 d, s32 e); /* unit's own declaration: with the definition's void (TexPackHead *p, s32 slot, s32 kind, s32 d, s32 src) (sub_main/unk_333D70.c), Model_SetupLoadedPackEx 38 of 67 words differ */

/* As Model_SetupLoadedPack with Tex_LoadPackLzHigh (one more argument, e). */
Model *Model_SetupLoadedPackEx(s16 idx, s32 a, s32 slot, s32 *pack, s32 b, s32 d, s32 c, s32 e)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)a;
    m->texSlot = (u16)slot;
    if (pack != NULL) {
        m->texCount = *pack;
        Tex_LoadPackLzHigh(pack, slot, 0, d, e);
    } else {
        m->texCount = 0;
    }
    if (Model_LoadMaterials(m, a) == -1) {
        return NULL;
    }
    Model_InitMatSets(m, b, c, 0);
    return m;
}

/* unk_335040's .data (0x4224E0-0x42FC30) */
extern u32 D_005B2458[];
s32 gAreaCamDataFiles[200] = {
#include "sub_main/004224E0.inc"
};
u32 gEquipModelPackFilesA[24] = {
#include "sub_main/00422800.inc"
};
u32 gEquipModelPackFilesB[24] = {
#include "sub_main/00422860.inc"
};
s32 gTexPackFiles[2836] = {
#include "sub_main/004228C0.inc"
};
u32 gAreaTexFiles[198] = {
#include "sub_main/00425510.inc"
};
u32 gArea83TexFiles[2] = {
#include "sub_main/00425828.inc"
};
u32 gArea84TexFiles[2] = {
#include "sub_main/00425830.inc"
};
u32 gPlayerLookFiles_0_3[2] = {
#include "sub_main/00425838.inc"
};
u32 gPlayerLookFiles_0_5[2] = {
#include "sub_main/00425840.inc"
};
u32 gPlayerLookFiles_0_7[2] = {
#include "sub_main/00425848.inc"
};
u32 gPlayerLookFiles_0_9[2] = {
#include "sub_main/00425850.inc"
};
u32 gPlayerLookFiles_0_11[2] = {
#include "sub_main/00425858.inc"
};
u32 gPlayerLookFiles_0_13[2] = {
#include "sub_main/00425860.inc"
};
u32 gPlayerLookFiles_0_15[2] = {
#include "sub_main/00425868.inc"
};
u32 gPlayerLookFiles_0_17[2] = {
#include "sub_main/00425870.inc"
};
u32 gPlayerLookFiles_0_19[2] = {
#include "sub_main/00425878.inc"
};
u32 gPlayerLookFiles_0_21[2] = {
#include "sub_main/00425880.inc"
};
u32 gPlayerLookFiles_0_23[2] = {
#include "sub_main/00425888.inc"
};
u32 gPlayerLookFiles_0_25[2] = {
#include "sub_main/00425890.inc"
};
u32 gPlayerLookFiles_0_27[2] = {
#include "sub_main/00425898.inc"
};
u32 gPlayerLookFiles_0_29[2] = {
#include "sub_main/004258A0.inc"
};
u32 gPlayerLookFiles_0_31[2] = {
#include "sub_main/004258A8.inc"
};
u32 gPlayerLookFiles_0_33[2] = {
#include "sub_main/004258B0.inc"
};
u32 gPlayerLookFiles_0_35[2] = {
#include "sub_main/004258B8.inc"
};
u32 gPlayerLookFiles_0_37[2] = {
#include "sub_main/004258C0.inc"
};
u32 gPlayerLookFiles_0_39[2] = {
#include "sub_main/004258C8.inc"
};
u32 gPlayerLookFiles_0_41[2] = {
#include "sub_main/004258D0.inc"
};
u32 gPlayerLookFiles_0_43[2] = {
#include "sub_main/004258D8.inc"
};
u32 gPlayerLookFiles_0_45[2] = {
#include "sub_main/004258E0.inc"
};
u32 gPlayerLookFiles_0_47[2] = {
#include "sub_main/004258E8.inc"
};
u32 gPlayerLookFiles_0_49[2] = {
#include "sub_main/004258F0.inc"
};
u32 gPlayerLookFiles_0_51[2] = {
#include "sub_main/004258F8.inc"
};
u32 gPlayerLookFiles_0_53[2] = {
#include "sub_main/00425900.inc"
};
u32 gPlayerLookFiles_0_55[2] = {
#include "sub_main/00425908.inc"
};
u32 gPlayerLookFiles_0_57[2] = {
#include "sub_main/00425910.inc"
};
u32 gPlayerLookFiles_0_59[2] = {
#include "sub_main/00425918.inc"
};
u32 gPlayerLookFiles_0_61[2] = {
#include "sub_main/00425920.inc"
};
u32 gPlayerLookFiles_0_63[2] = {
#include "sub_main/00425928.inc"
};
u32 gPlayerLookFiles_0_65[2] = {
#include "sub_main/00425930.inc"
};
u32 gPlayerLookFiles_0_67[2] = {
#include "sub_main/00425938.inc"
};
u32 gPlayerLookFiles_0_69[2] = {
#include "sub_main/00425940.inc"
};
u32 gPlayerLookFiles_0_71[2] = {
#include "sub_main/00425948.inc"
};
u32 gPlayerLookFiles_0_73[2] = {
#include "sub_main/00425950.inc"
};
u32 gPlayerLookFiles_0_75[2] = {
#include "sub_main/00425958.inc"
};
u32 gPlayerLookFiles_0_77[2] = {
#include "sub_main/00425960.inc"
};
u32 gPlayerLookFiles_0_79[2] = {
#include "sub_main/00425968.inc"
};
u32 gPlayerLookFiles_0_81[2] = {
#include "sub_main/00425970.inc"
};
u32 gPlayerLookFiles_0_83[2] = {
#include "sub_main/00425978.inc"
};
u32 gPlayerLookFiles_0_85[2] = {
#include "sub_main/00425980.inc"
};
u32 gPlayerLookFiles_0_87[2] = {
#include "sub_main/00425988.inc"
};
u32 gPlayerLookFiles_0_89[2] = {
#include "sub_main/00425990.inc"
};
u32 gPlayerLookFiles_0_91[2] = {
#include "sub_main/00425998.inc"
};
u32 gPlayerLookFiles_0_93[2] = {
#include "sub_main/004259A0.inc"
};
u32 gPlayerLookFiles_0_95[2] = {
#include "sub_main/004259A8.inc"
};
u32 gPlayerLookFiles_0_97[2] = {
#include "sub_main/004259B0.inc"
};
u32 gPlayerLookFiles_0_99[2] = {
#include "sub_main/004259B8.inc"
};
u32 gPlayerLookFiles_0_101[2] = {
#include "sub_main/004259C0.inc"
};
u32 gPlayerLookFiles_0_103[2] = {
#include "sub_main/004259C8.inc"
};
u32 gPlayerLookFiles_0_105[2] = {
#include "sub_main/004259D0.inc"
};
u32 gPlayerLookFiles_0_107[2] = {
#include "sub_main/004259D8.inc"
};
u32 gPlayerLookFiles_0_109[2] = {
#include "sub_main/004259E0.inc"
};
u32 gPlayerLookFiles_0_111[2] = {
#include "sub_main/004259E8.inc"
};
u32 gPlayerLookFiles_0_113[2] = {
#include "sub_main/004259F0.inc"
};
u32 gPlayerLookFiles_0_115[2] = {
#include "sub_main/004259F8.inc"
};
u32 gPlayerLookFiles_0_117[2] = {
#include "sub_main/00425A00.inc"
};
u32 gPlayerLookFiles_0_119[2] = {
#include "sub_main/00425A08.inc"
};
u32 gPlayerLookFiles_0_121[2] = {
#include "sub_main/00425A10.inc"
};
u32 gPlayerLookFiles_0_123[2] = {
#include "sub_main/00425A18.inc"
};
u32 gPlayerLookFiles_0_125[2] = {
#include "sub_main/00425A20.inc"
};
u32 gPlayerLookFiles_0_127[2] = {
#include "sub_main/00425A28.inc"
};
u32 gPlayerLookFiles_0_129[2] = {
#include "sub_main/00425A30.inc"
};
u32 gPlayerLookFiles_0_131[2] = {
#include "sub_main/00425A38.inc"
};
u32 gPlayerLookFiles_0_133[2] = {
#include "sub_main/00425A40.inc"
};
u32 gPlayerLookFiles_0_135[2] = {
#include "sub_main/00425A48.inc"
};
u32 gPlayerLookFiles_0_137[2] = {
#include "sub_main/00425A50.inc"
};
u32 gPlayerLookFiles_0_139[2] = {
#include "sub_main/00425A58.inc"
};
u32 gPlayerLookFiles_0_141[2] = {
#include "sub_main/00425A60.inc"
};
u32 gPlayerLookFiles_0_143[2] = {
#include "sub_main/00425A68.inc"
};
u32 gPlayerLookFiles_0_145[2] = {
#include "sub_main/00425A70.inc"
};
u32 gPlayerLookFiles_0_147[2] = {
#include "sub_main/00425A78.inc"
};
u32 gPlayerLookFiles_0_149[2] = {
#include "sub_main/00425A80.inc"
};
u32 gPlayerLookFiles_0_151[2] = {
#include "sub_main/00425A88.inc"
};
u32 gPlayerLookFiles_0_153[2] = {
#include "sub_main/00425A90.inc"
};
u32 gPlayerLookFiles_0_155[2] = {
#include "sub_main/00425A98.inc"
};
u32 gPlayerLookFiles_0_157[2] = {
#include "sub_main/00425AA0.inc"
};
u32 gPlayerLookFiles_0_159[2] = {
#include "sub_main/00425AA8.inc"
};
u32 gPlayerLookFiles_0_161[2] = {
#include "sub_main/00425AB0.inc"
};
u32 gPlayerLookFiles_0_163[2] = {
#include "sub_main/00425AB8.inc"
};
u32 gPlayerLookFiles_0_165[2] = {
#include "sub_main/00425AC0.inc"
};
u32 gPlayerLookFiles_0_167[2] = {
#include "sub_main/00425AC8.inc"
};
u32 gPlayerLookFiles_0_169[2] = {
#include "sub_main/00425AD0.inc"
};
u32 gPlayerLookFiles_0_171[2] = {
#include "sub_main/00425AD8.inc"
};
u32 gPlayerLookFiles_0_173[2] = {
#include "sub_main/00425AE0.inc"
};
u32 gPlayerLookFiles_0_175[2] = {
#include "sub_main/00425AE8.inc"
};
u32 gPlayerLookFiles_0_177[2] = {
#include "sub_main/00425AF0.inc"
};
u32 gPlayerLookFiles_0_179[2] = {
#include "sub_main/00425AF8.inc"
};
u32 gPlayerLookFiles_0_181[2] = {
#include "sub_main/00425B00.inc"
};
u32 gPlayerLookFiles_0_183[2] = {
#include "sub_main/00425B08.inc"
};
u32 gPlayerLookFiles_0_185[2] = {
#include "sub_main/00425B10.inc"
};
u32 gPlayerLookFiles_0_187[2] = {
#include "sub_main/00425B18.inc"
};
u32 gPlayerLookFiles_0_189[2] = {
#include "sub_main/00425B20.inc"
};
u32 gPlayerLookFiles_0_191[2] = {
#include "sub_main/00425B28.inc"
};
u32 gPlayerLookFiles_0_193[2] = {
#include "sub_main/00425B30.inc"
};
u32 gPlayerLookFiles_0_195[2] = {
#include "sub_main/00425B38.inc"
};
u32 gPlayerLookFiles_0_197[2] = {
#include "sub_main/00425B40.inc"
};
u32 gPlayerLookFiles_0_199[2] = {
#include "sub_main/00425B48.inc"
};
u32 gPlayerLookFiles_0_201[2] = {
#include "sub_main/00425B50.inc"
};
u32 gPlayerLookFiles_0_203[2] = {
#include "sub_main/00425B58.inc"
};
u32 gPlayerLookFiles_0_205[2] = {
#include "sub_main/00425B60.inc"
};
u32 gPlayerLookFiles_0_207[2] = {
#include "sub_main/00425B68.inc"
};
u32 gPlayerLookFiles_0_209[2] = {
#include "sub_main/00425B70.inc"
};
u32 gPlayerLookFiles_0_211[2] = {
#include "sub_main/00425B78.inc"
};
u32 gPlayerLookFiles_0_213[2] = {
#include "sub_main/00425B80.inc"
};
u32 gPlayerLookFiles_0_215[2] = {
#include "sub_main/00425B88.inc"
};
u32 gPlayerLookFiles_0_217[2] = {
#include "sub_main/00425B90.inc"
};
u32 gPlayerLookFiles_0_219[2] = {
#include "sub_main/00425B98.inc"
};
u32 gPlayerLookFiles_0_221[2] = {
#include "sub_main/00425BA0.inc"
};
u32 gPlayerLookFiles_0_223[2] = {
#include "sub_main/00425BA8.inc"
};
u32 gPlayerLookFiles_0_225[2] = {
#include "sub_main/00425BB0.inc"
};
u32 gPlayerLookFiles_0_227[2] = {
#include "sub_main/00425BB8.inc"
};
u32 gPlayerLookFiles_0_229[2] = {
#include "sub_main/00425BC0.inc"
};
u32 gPlayerLookFiles_0_231[2] = {
#include "sub_main/00425BC8.inc"
};
u32 gPlayerLookFiles_0_233[2] = {
#include "sub_main/00425BD0.inc"
};
u32 gPlayerLookFiles_0_235[2] = {
#include "sub_main/00425BD8.inc"
};
u32 gPlayerLookFiles_0_237[2] = {
#include "sub_main/00425BE0.inc"
};
u32 gPlayerLookFiles_0_239[2] = {
#include "sub_main/00425BE8.inc"
};
u32 gPlayerLookFiles_0_241[2] = {
#include "sub_main/00425BF0.inc"
};
u32 gPlayerLookFiles_0_243[2] = {
#include "sub_main/00425BF8.inc"
};
u32 gPlayerLookFiles_0_245[2] = {
#include "sub_main/00425C00.inc"
};
u32 gPlayerLookFiles_0_247[2] = {
#include "sub_main/00425C08.inc"
};
u32 gPlayerLookFiles_0_249[2] = {
#include "sub_main/00425C10.inc"
};
u32 gPlayerLookFiles_0_251[2] = {
#include "sub_main/00425C18.inc"
};
u32 gPlayerLookFiles_0_253[2] = {
#include "sub_main/00425C20.inc"
};
u32 gPlayerLookFiles_0_255[2] = {
#include "sub_main/00425C28.inc"
};
u32 gPlayerLookFiles_0_257[2] = {
#include "sub_main/00425C30.inc"
};
u32 gPlayerLookFiles_0_259[2] = {
#include "sub_main/00425C38.inc"
};
u32 gPlayerLookFiles_0_261[2] = {
#include "sub_main/00425C40.inc"
};
u32 gPlayerLookFiles_0_263[2] = {
#include "sub_main/00425C48.inc"
};
u32 gPlayerLookFiles_0_265[2] = {
#include "sub_main/00425C50.inc"
};
u32 gPlayerLookFiles_0_267[2] = {
#include "sub_main/00425C58.inc"
};
u32 gPlayerLookFiles_0_269[2] = {
#include "sub_main/00425C60.inc"
};
u32 gPlayerLookFiles_0_271[2] = {
#include "sub_main/00425C68.inc"
};
u32 gPlayerLookFiles_0_273[2] = {
#include "sub_main/00425C70.inc"
};
u32 gPlayerLookFiles_0_275[2] = {
#include "sub_main/00425C78.inc"
};
u32 gPlayerLookFiles_0_277[2] = {
#include "sub_main/00425C80.inc"
};
u32 gPlayerLookFiles_0_279[2] = {
#include "sub_main/00425C88.inc"
};
u32 gPlayerLookFiles_0_281[2] = {
#include "sub_main/00425C90.inc"
};
u32 gPlayerLookFiles_0_283[2] = {
#include "sub_main/00425C98.inc"
};
u32 gPlayerLookFiles_0_285[2] = {
#include "sub_main/00425CA0.inc"
};
u32 gPlayerLookFiles_0_287[2] = {
#include "sub_main/00425CA8.inc"
};
u32 gPlayerLookFiles_0_289[2] = {
#include "sub_main/00425CB0.inc"
};
u32 gPlayerLookFiles_0_291[2] = {
#include "sub_main/00425CB8.inc"
};
u32 gPlayerLookFiles_0_293[2] = {
#include "sub_main/00425CC0.inc"
};
u32 gPlayerLookFiles_0_295[2] = {
#include "sub_main/00425CC8.inc"
};
u32 gPlayerLookFiles_0_297[2] = {
#include "sub_main/00425CD0.inc"
};
u32 gPlayerLookFiles_0_299[2] = {
#include "sub_main/00425CD8.inc"
};
u32 gPlayerLookFiles_0_301[2] = {
#include "sub_main/00425CE0.inc"
};
u32 gPlayerLookFiles_0_303[2] = {
#include "sub_main/00425CE8.inc"
};
u32 gPlayerLookFiles_0_305[2] = {
#include "sub_main/00425CF0.inc"
};
u32 gPlayerLookFiles_0_307[2] = {
#include "sub_main/00425CF8.inc"
};
u32 gPlayerLookFiles_0_309[2] = {
#include "sub_main/00425D00.inc"
};
u32 gPlayerLookFiles_0_311[2] = {
#include "sub_main/00425D08.inc"
};
u32 gPlayerLookFiles_0_313[2] = {
#include "sub_main/00425D10.inc"
};
u32 gPlayerLookFiles_0_315[2] = {
#include "sub_main/00425D18.inc"
};
u32 gPlayerLookFiles_0_317[2] = {
#include "sub_main/00425D20.inc"
};
u32 gPlayerLookFiles_0_319[2] = {
#include "sub_main/00425D28.inc"
};
u32 gPlayerLookFiles_0_321[2] = {
#include "sub_main/00425D30.inc"
};
u32 gPlayerLookFiles_0_323[2] = {
#include "sub_main/00425D38.inc"
};
u32 gPlayerLookFiles_0_325[2] = {
#include "sub_main/00425D40.inc"
};
u32 gPlayerLookFiles_0_327[2] = {
#include "sub_main/00425D48.inc"
};
u32 gPlayerLookFiles_0_329[2] = {
#include "sub_main/00425D50.inc"
};
u32 gPlayerLookFiles_0_331[2] = {
#include "sub_main/00425D58.inc"
};
u32 gPlayerLookFiles_0_333[2] = {
#include "sub_main/00425D60.inc"
};
u32 gPlayerLookFiles_0_335[2] = {
#include "sub_main/00425D68.inc"
};
u32 gPlayerLookFiles_0_337[2] = {
#include "sub_main/00425D70.inc"
};
u32 gPlayerLookFiles_0_339[2] = {
#include "sub_main/00425D78.inc"
};
u32 gPlayerLookFiles_0_341[2] = {
#include "sub_main/00425D80.inc"
};
u32 gPlayerLookFiles_0_343[2] = {
#include "sub_main/00425D88.inc"
};
u32 gPlayerLookFiles_0_345[2] = {
#include "sub_main/00425D90.inc"
};
u32 gPlayerLookFiles_0_347[2] = {
#include "sub_main/00425D98.inc"
};
u32 gPlayerLookFiles_0_349[2] = {
#include "sub_main/00425DA0.inc"
};
u32 gPlayerLookFiles_0_351[2] = {
#include "sub_main/00425DA8.inc"
};
u32 gPlayerLookFiles_0_353[2] = {
#include "sub_main/00425DB0.inc"
};
u32 gPlayerLookFiles_0_355[2] = {
#include "sub_main/00425DB8.inc"
};
u32 gPlayerLookFiles_0_357[2] = {
#include "sub_main/00425DC0.inc"
};
u32 gPlayerLookFiles_0_359[2] = {
#include "sub_main/00425DC8.inc"
};
u32 gPlayerLookFiles_0_361[2] = {
#include "sub_main/00425DD0.inc"
};
u32 gPlayerLookFiles_0_363[2] = {
#include "sub_main/00425DD8.inc"
};
u32 gPlayerLookFiles_0_365[2] = {
#include "sub_main/00425DE0.inc"
};
u32 gPlayerLookFiles_0_367[2] = {
#include "sub_main/00425DE8.inc"
};
u32 gPlayerLookFiles_0_369[2] = {
#include "sub_main/00425DF0.inc"
};
u32 gPlayerLookFiles_0_371[2] = {
#include "sub_main/00425DF8.inc"
};
u32 gPlayerLookFiles_0_373[2] = {
#include "sub_main/00425E00.inc"
};
u32 gPlayerLookFiles_0_375[2] = {
#include "sub_main/00425E08.inc"
};
u32 gPlayerLookFiles_0_377[2] = {
#include "sub_main/00425E10.inc"
};
u32 gPlayerLookFiles_0_379[2] = {
#include "sub_main/00425E18.inc"
};
u32 gPlayerLookFiles_0_381[2] = {
#include "sub_main/00425E20.inc"
};
u32 gPlayerLookFiles_0_383[2] = {
#include "sub_main/00425E28.inc"
};
u32 gPlayerLookFiles_0_385[2] = {
#include "sub_main/00425E30.inc"
};
u32 gPlayerLookFiles_0_387[2] = {
#include "sub_main/00425E38.inc"
};
u32 gPlayerLookFiles_0_389[2] = {
#include "sub_main/00425E40.inc"
};
u32 gPlayerLookFiles_0_391[2] = {
#include "sub_main/00425E48.inc"
};
u32 gPlayerLookFiles_0_393[2] = {
#include "sub_main/00425E50.inc"
};
u32 gPlayerLookFiles_0_395[2] = {
#include "sub_main/00425E58.inc"
};
u32 gPlayerLookFiles_0_397[4] = {
#include "sub_main/00425E60.inc"
};
u32 gPlayerLookFiles_0[398] = {
#include "sub_main/00425E70.inc"
};
u32 gPlayerLookFiles_1_1[2] = {
#include "sub_main/004264A8.inc"
};
u32 gPlayerLookFiles_1_3[2] = {
#include "sub_main/004264B0.inc"
};
u32 gPlayerLookFiles_1_5[2] = {
#include "sub_main/004264B8.inc"
};
u32 gPlayerLookFiles_1_7[2] = {
#include "sub_main/004264C0.inc"
};
u32 gPlayerLookFiles_1_9[2] = {
#include "sub_main/004264C8.inc"
};
u32 gPlayerLookFiles_1_11[2] = {
#include "sub_main/004264D0.inc"
};
u32 gPlayerLookFiles_1_13[2] = {
#include "sub_main/004264D8.inc"
};
u32 gPlayerLookFiles_1_15[2] = {
#include "sub_main/004264E0.inc"
};
u32 gPlayerLookFiles_1_17[2] = {
#include "sub_main/004264E8.inc"
};
u32 gPlayerLookFiles_1_19[2] = {
#include "sub_main/004264F0.inc"
};
u32 gPlayerLookFiles_1_21[2] = {
#include "sub_main/004264F8.inc"
};
u32 gPlayerLookFiles_1_23[2] = {
#include "sub_main/00426500.inc"
};
u32 gPlayerLookFiles_1_25[2] = {
#include "sub_main/00426508.inc"
};
u32 gPlayerLookFiles_1_27[2] = {
#include "sub_main/00426510.inc"
};
u32 gPlayerLookFiles_1_29[2] = {
#include "sub_main/00426518.inc"
};
u32 gPlayerLookFiles_1_31[2] = {
#include "sub_main/00426520.inc"
};
u32 gPlayerLookFiles_1_33[2] = {
#include "sub_main/00426528.inc"
};
u32 gPlayerLookFiles_1_35[2] = {
#include "sub_main/00426530.inc"
};
u32 gPlayerLookFiles_1_37[2] = {
#include "sub_main/00426538.inc"
};
u32 gPlayerLookFiles_1_39[2] = {
#include "sub_main/00426540.inc"
};
u32 gPlayerLookFiles_1_41[2] = {
#include "sub_main/00426548.inc"
};
u32 gPlayerLookFiles_1_43[2] = {
#include "sub_main/00426550.inc"
};
u32 gPlayerLookFiles_1_45[2] = {
#include "sub_main/00426558.inc"
};
u32 gPlayerLookFiles_1_47[2] = {
#include "sub_main/00426560.inc"
};
u32 gPlayerLookFiles_1_49[2] = {
#include "sub_main/00426568.inc"
};
u32 gPlayerLookFiles_1_51[2] = {
#include "sub_main/00426570.inc"
};
u32 gPlayerLookFiles_1_53[2] = {
#include "sub_main/00426578.inc"
};
u32 gPlayerLookFiles_1[54] = {
#include "sub_main/00426580.inc"
};
u32 gPlayerLookFiles_2_1[2] = {
#include "sub_main/00426658.inc"
};
u32 gPlayerLookFiles_2_3[2] = {
#include "sub_main/00426660.inc"
};
u32 gPlayerLookFiles_2_5[2] = {
#include "sub_main/00426668.inc"
};
u32 gPlayerLookFiles_2_7[2] = {
#include "sub_main/00426670.inc"
};
u32 gPlayerLookFiles_2_9[2] = {
#include "sub_main/00426678.inc"
};
u32 gPlayerLookFiles_2_11[2] = {
#include "sub_main/00426680.inc"
};
u32 gPlayerLookFiles_2_13[2] = {
#include "sub_main/00426688.inc"
};
u32 gPlayerLookFiles_2_15[2] = {
#include "sub_main/00426690.inc"
};
u32 gPlayerLookFiles_2_17[2] = {
#include "sub_main/00426698.inc"
};
u32 gPlayerLookFiles_2_19[2] = {
#include "sub_main/004266A0.inc"
};
u32 gPlayerLookFiles_2_21[2] = {
#include "sub_main/004266A8.inc"
};
u32 gPlayerLookFiles_2_23[2] = {
#include "sub_main/004266B0.inc"
};
u32 gPlayerLookFiles_2_25[2] = {
#include "sub_main/004266B8.inc"
};
u32 gPlayerLookFiles_2_27[2] = {
#include "sub_main/004266C0.inc"
};
u32 gPlayerLookFiles_2_29[2] = {
#include "sub_main/004266C8.inc"
};
u32 gPlayerLookFiles_2_31[2] = {
#include "sub_main/004266D0.inc"
};
u32 gPlayerLookFiles_2_33[2] = {
#include "sub_main/004266D8.inc"
};
u32 gPlayerLookFiles_2_35[2] = {
#include "sub_main/004266E0.inc"
};
u32 gPlayerLookFiles_2_37[2] = {
#include "sub_main/004266E8.inc"
};
u32 gPlayerLookFiles_2_39[2] = {
#include "sub_main/004266F0.inc"
};
u32 gPlayerLookFiles_2_41[2] = {
#include "sub_main/004266F8.inc"
};
u32 gPlayerLookFiles_2_43[2] = {
#include "sub_main/00426700.inc"
};
u32 gPlayerLookFiles_2_45[2] = {
#include "sub_main/00426708.inc"
};
u32 gPlayerLookFiles_2_47[2] = {
#include "sub_main/00426710.inc"
};
u32 gPlayerLookFiles_2_49[2] = {
#include "sub_main/00426718.inc"
};
u32 gPlayerLookFiles_2_51[2] = {
#include "sub_main/00426720.inc"
};
u32 gPlayerLookFiles_2_53[2] = {
#include "sub_main/00426728.inc"
};
u32 gPlayerLookFiles_2_55[2] = {
#include "sub_main/00426730.inc"
};
u32 gPlayerLookFiles_2_57[2] = {
#include "sub_main/00426738.inc"
};
u32 gPlayerLookFiles_2_59[2] = {
#include "sub_main/00426740.inc"
};
u32 gPlayerLookFiles_2_61[2] = {
#include "sub_main/00426748.inc"
};
u32 gPlayerLookFiles_2_63[2] = {
#include "sub_main/00426750.inc"
};
u32 gPlayerLookFiles_2_65[2] = {
#include "sub_main/00426758.inc"
};
u32 gPlayerLookFiles_2_67[2] = {
#include "sub_main/00426760.inc"
};
u32 gPlayerLookFiles_2_69[2] = {
#include "sub_main/00426768.inc"
};
u32 gPlayerLookFiles_2_71[2] = {
#include "sub_main/00426770.inc"
};
u32 gPlayerLookFiles_2_73[2] = {
#include "sub_main/00426778.inc"
};
u32 gPlayerLookFiles_2_75[2] = {
#include "sub_main/00426780.inc"
};
u32 gPlayerLookFiles_2_77[2] = {
#include "sub_main/00426788.inc"
};
u32 gPlayerLookFiles_2_79[2] = {
#include "sub_main/00426790.inc"
};
u32 gPlayerLookFiles_2_81[2] = {
#include "sub_main/00426798.inc"
};
u32 gPlayerLookFiles_2_83[2] = {
#include "sub_main/004267A0.inc"
};
u32 gPlayerLookFiles_2_85[2] = {
#include "sub_main/004267A8.inc"
};
u32 gPlayerLookFiles_2_87[2] = {
#include "sub_main/004267B0.inc"
};
u32 gPlayerLookFiles_2_89[2] = {
#include "sub_main/004267B8.inc"
};
u32 gPlayerLookFiles_2_91[2] = {
#include "sub_main/004267C0.inc"
};
u32 gPlayerLookFiles_2_93[2] = {
#include "sub_main/004267C8.inc"
};
u32 gPlayerLookFiles_2_95[2] = {
#include "sub_main/004267D0.inc"
};
u32 gPlayerLookFiles_2_97[2] = {
#include "sub_main/004267D8.inc"
};
u32 gPlayerLookFiles_2_99[2] = {
#include "sub_main/004267E0.inc"
};
u32 gPlayerLookFiles_2_101[2] = {
#include "sub_main/004267E8.inc"
};
u32 gPlayerLookFiles_2_103[2] = {
#include "sub_main/004267F0.inc"
};
u32 gPlayerLookFiles_2_105[2] = {
#include "sub_main/004267F8.inc"
};
u32 gPlayerLookFiles_2_107[2] = {
#include "sub_main/00426800.inc"
};
u32 gPlayerLookFiles_2_109[2] = {
#include "sub_main/00426808.inc"
};
u32 gPlayerLookFiles_2_111[2] = {
#include "sub_main/00426810.inc"
};
u32 gPlayerLookFiles_2_113[2] = {
#include "sub_main/00426818.inc"
};
u32 gPlayerLookFiles_2_115[2] = {
#include "sub_main/00426820.inc"
};
u32 gPlayerLookFiles_2_117[2] = {
#include "sub_main/00426828.inc"
};
u32 gPlayerLookFiles_2_119[2] = {
#include "sub_main/00426830.inc"
};
u32 gPlayerLookFiles_2_121[2] = {
#include "sub_main/00426838.inc"
};
u32 gPlayerLookFiles_2_123[2] = {
#include "sub_main/00426840.inc"
};
u32 gPlayerLookFiles_2_125[2] = {
#include "sub_main/00426848.inc"
};
u32 gPlayerLookFiles_2_127[2] = {
#include "sub_main/00426850.inc"
};
u32 gPlayerLookFiles_2_129[2] = {
#include "sub_main/00426858.inc"
};
u32 gPlayerLookFiles_2_131[2] = {
#include "sub_main/00426860.inc"
};
u32 gPlayerLookFiles_2_133[2] = {
#include "sub_main/00426868.inc"
};
u32 gPlayerLookFiles_2_135[2] = {
#include "sub_main/00426870.inc"
};
u32 gPlayerLookFiles_2_137[2] = {
#include "sub_main/00426878.inc"
};
u32 gPlayerLookFiles_2_139[2] = {
#include "sub_main/00426880.inc"
};
u32 gPlayerLookFiles_2_141[2] = {
#include "sub_main/00426888.inc"
};
u32 gPlayerLookFiles_2_143[2] = {
#include "sub_main/00426890.inc"
};
u32 gPlayerLookFiles_2_145[2] = {
#include "sub_main/00426898.inc"
};
u32 gPlayerLookFiles_2_147[2] = {
#include "sub_main/004268A0.inc"
};
u32 gPlayerLookFiles_2_149[2] = {
#include "sub_main/004268A8.inc"
};
u32 gPlayerLookFiles_2_151[2] = {
#include "sub_main/004268B0.inc"
};
u32 gPlayerLookFiles_2_153[2] = {
#include "sub_main/004268B8.inc"
};
u32 gPlayerLookFiles_2_155[2] = {
#include "sub_main/004268C0.inc"
};
u32 gPlayerLookFiles_2_157[2] = {
#include "sub_main/004268C8.inc"
};
u32 gPlayerLookFiles_2_159[2] = {
#include "sub_main/004268D0.inc"
};
u32 gPlayerLookFiles_2_161[2] = {
#include "sub_main/004268D8.inc"
};
u32 gPlayerLookFiles_2_163[2] = {
#include "sub_main/004268E0.inc"
};
u32 gPlayerLookFiles_2_165[2] = {
#include "sub_main/004268E8.inc"
};
u32 gPlayerLookFiles_2_167[2] = {
#include "sub_main/004268F0.inc"
};
u32 gPlayerLookFiles_2_169[2] = {
#include "sub_main/004268F8.inc"
};
u32 gPlayerLookFiles_2_171[2] = {
#include "sub_main/00426900.inc"
};
u32 gPlayerLookFiles_2_173[2] = {
#include "sub_main/00426908.inc"
};
u32 gPlayerLookFiles_2_175[2] = {
#include "sub_main/00426910.inc"
};
u32 gPlayerLookFiles_2_177[2] = {
#include "sub_main/00426918.inc"
};
u32 gPlayerLookFiles_2_179[2] = {
#include "sub_main/00426920.inc"
};
u32 gPlayerLookFiles_2_181[2] = {
#include "sub_main/00426928.inc"
};
u32 gPlayerLookFiles_2_183[2] = {
#include "sub_main/00426930.inc"
};
u32 gPlayerLookFiles_2_185[2] = {
#include "sub_main/00426938.inc"
};
u32 gPlayerLookFiles_2_187[2] = {
#include "sub_main/00426940.inc"
};
u32 gPlayerLookFiles_2_189[2] = {
#include "sub_main/00426948.inc"
};
u32 gPlayerLookFiles_2_191[2] = {
#include "sub_main/00426950.inc"
};
u32 gPlayerLookFiles_2_193[2] = {
#include "sub_main/00426958.inc"
};
u32 gPlayerLookFiles_2_195[2] = {
#include "sub_main/00426960.inc"
};
u32 gPlayerLookFiles_2_197[2] = {
#include "sub_main/00426968.inc"
};
u32 gPlayerLookFiles_2_199[2] = {
#include "sub_main/00426970.inc"
};
u32 gPlayerLookFiles_2_201[2] = {
#include "sub_main/00426978.inc"
};
u32 gPlayerLookFiles_2_203[2] = {
#include "sub_main/00426980.inc"
};
u32 gPlayerLookFiles_2_205[2] = {
#include "sub_main/00426988.inc"
};
u32 gPlayerLookFiles_2_207[2] = {
#include "sub_main/00426990.inc"
};
u32 gPlayerLookFiles_2_209[2] = {
#include "sub_main/00426998.inc"
};
u32 gPlayerLookFiles_2_211[2] = {
#include "sub_main/004269A0.inc"
};
u32 gPlayerLookFiles_2_213[2] = {
#include "sub_main/004269A8.inc"
};
u32 gPlayerLookFiles_2_215[2] = {
#include "sub_main/004269B0.inc"
};
u32 gPlayerLookFiles_2_217[2] = {
#include "sub_main/004269B8.inc"
};
u32 gPlayerLookFiles_2_219[2] = {
#include "sub_main/004269C0.inc"
};
u32 gPlayerLookFiles_2_221[2] = {
#include "sub_main/004269C8.inc"
};
u32 gPlayerLookFiles_2_223[2] = {
#include "sub_main/004269D0.inc"
};
u32 gPlayerLookFiles_2_225[2] = {
#include "sub_main/004269D8.inc"
};
u32 gPlayerLookFiles_2_227[2] = {
#include "sub_main/004269E0.inc"
};
u32 gPlayerLookFiles_2_229[2] = {
#include "sub_main/004269E8.inc"
};
u32 gPlayerLookFiles_2_231[2] = {
#include "sub_main/004269F0.inc"
};
u32 gPlayerLookFiles_2_233[2] = {
#include "sub_main/004269F8.inc"
};
u32 gPlayerLookFiles_2_235[2] = {
#include "sub_main/00426A00.inc"
};
u32 gPlayerLookFiles_2_237[2] = {
#include "sub_main/00426A08.inc"
};
u32 gPlayerLookFiles_2_239[2] = {
#include "sub_main/00426A10.inc"
};
u32 gPlayerLookFiles_2_241[2] = {
#include "sub_main/00426A18.inc"
};
u32 gPlayerLookFiles_2_243[2] = {
#include "sub_main/00426A20.inc"
};
u32 gPlayerLookFiles_2_245[2] = {
#include "sub_main/00426A28.inc"
};
u32 gPlayerLookFiles_2_247[2] = {
#include "sub_main/00426A30.inc"
};
u32 gPlayerLookFiles_2_249[2] = {
#include "sub_main/00426A38.inc"
};
u32 gPlayerLookFiles_2_251[2] = {
#include "sub_main/00426A40.inc"
};
u32 gPlayerLookFiles_2_253[2] = {
#include "sub_main/00426A48.inc"
};
u32 gPlayerLookFiles_2_255[2] = {
#include "sub_main/00426A50.inc"
};
u32 gPlayerLookFiles_2_257[2] = {
#include "sub_main/00426A58.inc"
};
u32 gPlayerLookFiles_2_259[2] = {
#include "sub_main/00426A60.inc"
};
u32 gPlayerLookFiles_2_261[2] = {
#include "sub_main/00426A68.inc"
};
u32 gPlayerLookFiles_2_263[2] = {
#include "sub_main/00426A70.inc"
};
u32 gPlayerLookFiles_2_265[2] = {
#include "sub_main/00426A78.inc"
};
u32 gPlayerLookFiles_2_267[2] = {
#include "sub_main/00426A80.inc"
};
u32 gPlayerLookFiles_2_269[2] = {
#include "sub_main/00426A88.inc"
};
u32 gPlayerLookFiles_2_271[2] = {
#include "sub_main/00426A90.inc"
};
u32 gPlayerLookFiles_2_273[2] = {
#include "sub_main/00426A98.inc"
};
u32 gPlayerLookFiles_2_275[2] = {
#include "sub_main/00426AA0.inc"
};
u32 gPlayerLookFiles_2_277[2] = {
#include "sub_main/00426AA8.inc"
};
u32 gPlayerLookFiles_2_279[2] = {
#include "sub_main/00426AB0.inc"
};
u32 gPlayerLookFiles_2_281[2] = {
#include "sub_main/00426AB8.inc"
};
u32 gPlayerLookFiles_2_283[2] = {
#include "sub_main/00426AC0.inc"
};
u32 gPlayerLookFiles_2_285[2] = {
#include "sub_main/00426AC8.inc"
};
u32 gPlayerLookFiles_2_287[2] = {
#include "sub_main/00426AD0.inc"
};
u32 gPlayerLookFiles_2_289[2] = {
#include "sub_main/00426AD8.inc"
};
u32 gPlayerLookFiles_2_291[2] = {
#include "sub_main/00426AE0.inc"
};
u32 gPlayerLookFiles_2_293[2] = {
#include "sub_main/00426AE8.inc"
};
u32 gPlayerLookFiles_2_295[2] = {
#include "sub_main/00426AF0.inc"
};
u32 gPlayerLookFiles_2_297[2] = {
#include "sub_main/00426AF8.inc"
};
u32 gPlayerLookFiles_2_299[2] = {
#include "sub_main/00426B00.inc"
};
u32 gPlayerLookFiles_2_301[2] = {
#include "sub_main/00426B08.inc"
};
u32 gPlayerLookFiles_2_303[2] = {
#include "sub_main/00426B10.inc"
};
u32 gPlayerLookFiles_2_305[2] = {
#include "sub_main/00426B18.inc"
};
u32 gPlayerLookFiles_2_307[2] = {
#include "sub_main/00426B20.inc"
};
u32 gPlayerLookFiles_2_309[2] = {
#include "sub_main/00426B28.inc"
};
u32 gPlayerLookFiles_2_311[2] = {
#include "sub_main/00426B30.inc"
};
u32 gPlayerLookFiles_2_313[2] = {
#include "sub_main/00426B38.inc"
};
u32 gPlayerLookFiles_2_315[2] = {
#include "sub_main/00426B40.inc"
};
u32 gPlayerLookFiles_2_317[2] = {
#include "sub_main/00426B48.inc"
};
u32 gPlayerLookFiles_2_319[2] = {
#include "sub_main/00426B50.inc"
};
u32 gPlayerLookFiles_2_321[2] = {
#include "sub_main/00426B58.inc"
};
u32 gPlayerLookFiles_2_323[2] = {
#include "sub_main/00426B60.inc"
};
u32 gPlayerLookFiles_2_325[2] = {
#include "sub_main/00426B68.inc"
};
u32 gPlayerLookFiles_2_327[2] = {
#include "sub_main/00426B70.inc"
};
u32 gPlayerLookFiles_2_329[2] = {
#include "sub_main/00426B78.inc"
};
u32 gPlayerLookFiles_2_331[2] = {
#include "sub_main/00426B80.inc"
};
u32 gPlayerLookFiles_2_333[2] = {
#include "sub_main/00426B88.inc"
};
u32 gPlayerLookFiles_2_335[2] = {
#include "sub_main/00426B90.inc"
};
u32 gPlayerLookFiles_2_337[2] = {
#include "sub_main/00426B98.inc"
};
u32 gPlayerLookFiles_2_339[2] = {
#include "sub_main/00426BA0.inc"
};
u32 gPlayerLookFiles_2_341[2] = {
#include "sub_main/00426BA8.inc"
};
u32 gPlayerLookFiles_2_343[2] = {
#include "sub_main/00426BB0.inc"
};
u32 gPlayerLookFiles_2_345[2] = {
#include "sub_main/00426BB8.inc"
};
u32 gPlayerLookFiles_2_347[2] = {
#include "sub_main/00426BC0.inc"
};
u32 gPlayerLookFiles_2_349[2] = {
#include "sub_main/00426BC8.inc"
};
u32 gPlayerLookFiles_2_351[2] = {
#include "sub_main/00426BD0.inc"
};
u32 gPlayerLookFiles_2_353[2] = {
#include "sub_main/00426BD8.inc"
};
u32 gPlayerLookFiles_2_355[2] = {
#include "sub_main/00426BE0.inc"
};
u32 gPlayerLookFiles_2_357[2] = {
#include "sub_main/00426BE8.inc"
};
u32 gPlayerLookFiles_2_359[2] = {
#include "sub_main/00426BF0.inc"
};
u32 gPlayerLookFiles_2_361[2] = {
#include "sub_main/00426BF8.inc"
};
u32 gPlayerLookFiles_2_363[2] = {
#include "sub_main/00426C00.inc"
};
u32 gPlayerLookFiles_2_365[2] = {
#include "sub_main/00426C08.inc"
};
u32 gPlayerLookFiles_2_367[2] = {
#include "sub_main/00426C10.inc"
};
u32 gPlayerLookFiles_2_369[2] = {
#include "sub_main/00426C18.inc"
};
u32 gPlayerLookFiles_2_371[2] = {
#include "sub_main/00426C20.inc"
};
u32 gPlayerLookFiles_2_373[2] = {
#include "sub_main/00426C28.inc"
};
u32 gPlayerLookFiles_2_375[2] = {
#include "sub_main/00426C30.inc"
};
u32 gPlayerLookFiles_2_377[2] = {
#include "sub_main/00426C38.inc"
};
u32 gPlayerLookFiles_2_379[2] = {
#include "sub_main/00426C40.inc"
};
u32 gPlayerLookFiles_2_381[2] = {
#include "sub_main/00426C48.inc"
};
u32 gPlayerLookFiles_2_383[2] = {
#include "sub_main/00426C50.inc"
};
u32 gPlayerLookFiles_2_385[2] = {
#include "sub_main/00426C58.inc"
};
u32 gPlayerLookFiles_2_387[2] = {
#include "sub_main/00426C60.inc"
};
u32 gPlayerLookFiles_2_389[2] = {
#include "sub_main/00426C68.inc"
};
u32 gPlayerLookFiles_2_391[2] = {
#include "sub_main/00426C70.inc"
};
u32 gPlayerLookFiles_2_393[2] = {
#include "sub_main/00426C78.inc"
};
u32 gPlayerLookFiles_2_395[2] = {
#include "sub_main/00426C80.inc"
};
u32 gPlayerLookFiles_2_397[2] = {
#include "sub_main/00426C88.inc"
};
u32 gPlayerLookFiles_2_399[2] = {
#include "sub_main/00426C90.inc"
};
u32 gPlayerLookFiles_2_401[2] = {
#include "sub_main/00426C98.inc"
};
u32 gPlayerLookFiles_2_403[2] = {
#include "sub_main/00426CA0.inc"
};
u32 gPlayerLookFiles_2_405[2] = {
#include "sub_main/00426CA8.inc"
};
u32 gPlayerLookFiles_2_407[2] = {
#include "sub_main/00426CB0.inc"
};
u32 gPlayerLookFiles_2_409[2] = {
#include "sub_main/00426CB8.inc"
};
u32 gPlayerLookFiles_2_411[2] = {
#include "sub_main/00426CC0.inc"
};
u32 gPlayerLookFiles_2_413[2] = {
#include "sub_main/00426CC8.inc"
};
u32 gPlayerLookFiles_2_415[2] = {
#include "sub_main/00426CD0.inc"
};
u32 gPlayerLookFiles_2_417[2] = {
#include "sub_main/00426CD8.inc"
};
u32 gPlayerLookFiles_2_419[2] = {
#include "sub_main/00426CE0.inc"
};
u32 gPlayerLookFiles_2_421[2] = {
#include "sub_main/00426CE8.inc"
};
u32 gPlayerLookFiles_2_423[2] = {
#include "sub_main/00426CF0.inc"
};
u32 gPlayerLookFiles_2_425[2] = {
#include "sub_main/00426CF8.inc"
};
u32 gPlayerLookFiles_2_427[2] = {
#include "sub_main/00426D00.inc"
};
u32 gPlayerLookFiles_2_429[2] = {
#include "sub_main/00426D08.inc"
};
u32 gPlayerLookFiles_2_431[2] = {
#include "sub_main/00426D10.inc"
};
u32 gPlayerLookFiles_2_433[2] = {
#include "sub_main/00426D18.inc"
};
u32 gPlayerLookFiles_2_435[2] = {
#include "sub_main/00426D20.inc"
};
u32 gPlayerLookFiles_2_437[2] = {
#include "sub_main/00426D28.inc"
};
u32 gPlayerLookFiles_2[438] = {
#include "sub_main/00426D30.inc"
};
u32 gPlayerLookFiles_3_1[2] = {
#include "sub_main/00427408.inc"
};
u32 gPlayerLookFiles_3_3[2] = {
#include "sub_main/00427410.inc"
};
u32 gPlayerLookFiles_3_5[2] = {
#include "sub_main/00427418.inc"
};
u32 gPlayerLookFiles_3_7[2] = {
#include "sub_main/00427420.inc"
};
u32 gPlayerLookFiles_3_9[2] = {
#include "sub_main/00427428.inc"
};
u32 gPlayerLookFiles_3_11[2] = {
#include "sub_main/00427430.inc"
};
u32 gPlayerLookFiles_3_13[2] = {
#include "sub_main/00427438.inc"
};
u32 gPlayerLookFiles_3_15[2] = {
#include "sub_main/00427440.inc"
};
u32 gPlayerLookFiles_3_17[2] = {
#include "sub_main/00427448.inc"
};
u32 gPlayerLookFiles_3_19[2] = {
#include "sub_main/00427450.inc"
};
u32 gPlayerLookFiles_3_21[2] = {
#include "sub_main/00427458.inc"
};
u32 gPlayerLookFiles_3_23[2] = {
#include "sub_main/00427460.inc"
};
u32 gPlayerLookFiles_3_25[2] = {
#include "sub_main/00427468.inc"
};
u32 gPlayerLookFiles_3_27[2] = {
#include "sub_main/00427470.inc"
};
u32 gPlayerLookFiles_3_29[2] = {
#include "sub_main/00427478.inc"
};
u32 gPlayerLookFiles_3_31[2] = {
#include "sub_main/00427480.inc"
};
u32 gPlayerLookFiles_3_33[2] = {
#include "sub_main/00427488.inc"
};
u32 gPlayerLookFiles_3_35[2] = {
#include "sub_main/00427490.inc"
};
u32 gPlayerLookFiles_3_37[2] = {
#include "sub_main/00427498.inc"
};
u32 gPlayerLookFiles_3_39[2] = {
#include "sub_main/004274A0.inc"
};
u32 gPlayerLookFiles_3_41[2] = {
#include "sub_main/004274A8.inc"
};
u32 gPlayerLookFiles_3_43[2] = {
#include "sub_main/004274B0.inc"
};
u32 gPlayerLookFiles_3_45[2] = {
#include "sub_main/004274B8.inc"
};
u32 gPlayerLookFiles_3_47[2] = {
#include "sub_main/004274C0.inc"
};
u32 gPlayerLookFiles_3_49[2] = {
#include "sub_main/004274C8.inc"
};
u32 gPlayerLookFiles_3_51[2] = {
#include "sub_main/004274D0.inc"
};
u32 gPlayerLookFiles_3_53[2] = {
#include "sub_main/004274D8.inc"
};
u32 gPlayerLookFiles_3_55[2] = {
#include "sub_main/004274E0.inc"
};
u32 gPlayerLookFiles_3_57[2] = {
#include "sub_main/004274E8.inc"
};
u32 gPlayerLookFiles_3_59[2] = {
#include "sub_main/004274F0.inc"
};
u32 gPlayerLookFiles_3_61[2] = {
#include "sub_main/004274F8.inc"
};
u32 gPlayerLookFiles_3_63[2] = {
#include "sub_main/00427500.inc"
};
u32 gPlayerLookFiles_3_65[2] = {
#include "sub_main/00427508.inc"
};
u32 gPlayerLookFiles_3_67[2] = {
#include "sub_main/00427510.inc"
};
u32 gPlayerLookFiles_3_69[2] = {
#include "sub_main/00427518.inc"
};
u32 gPlayerLookFiles_3_71[2] = {
#include "sub_main/00427520.inc"
};
u32 gPlayerLookFiles_3_73[2] = {
#include "sub_main/00427528.inc"
};
u32 gPlayerLookFiles_3_75[2] = {
#include "sub_main/00427530.inc"
};
u32 gPlayerLookFiles_3_77[2] = {
#include "sub_main/00427538.inc"
};
u32 gPlayerLookFiles_3_79[2] = {
#include "sub_main/00427540.inc"
};
u32 gPlayerLookFiles_3_81[2] = {
#include "sub_main/00427548.inc"
};
u32 gPlayerLookFiles_3_83[2] = {
#include "sub_main/00427550.inc"
};
u32 gPlayerLookFiles_3_85[2] = {
#include "sub_main/00427558.inc"
};
u32 gPlayerLookFiles_3_87[2] = {
#include "sub_main/00427560.inc"
};
u32 gPlayerLookFiles_3_89[2] = {
#include "sub_main/00427568.inc"
};
u32 gPlayerLookFiles_3_91[2] = {
#include "sub_main/00427570.inc"
};
u32 gPlayerLookFiles_3_93[2] = {
#include "sub_main/00427578.inc"
};
u32 gPlayerLookFiles_3_95[2] = {
#include "sub_main/00427580.inc"
};
u32 gPlayerLookFiles_3_97[2] = {
#include "sub_main/00427588.inc"
};
u32 gPlayerLookFiles_3_99[2] = {
#include "sub_main/00427590.inc"
};
u32 gPlayerLookFiles_3_101[2] = {
#include "sub_main/00427598.inc"
};
u32 gPlayerLookFiles_3_103[2] = {
#include "sub_main/004275A0.inc"
};
u32 gPlayerLookFiles_3_105[2] = {
#include "sub_main/004275A8.inc"
};
u32 gPlayerLookFiles_3_107[2] = {
#include "sub_main/004275B0.inc"
};
u32 gPlayerLookFiles_3_109[2] = {
#include "sub_main/004275B8.inc"
};
u32 gPlayerLookFiles_3_111[2] = {
#include "sub_main/004275C0.inc"
};
u32 gPlayerLookFiles_3_113[2] = {
#include "sub_main/004275C8.inc"
};
u32 gPlayerLookFiles_3_115[2] = {
#include "sub_main/004275D0.inc"
};
u32 gPlayerLookFiles_3_117[2] = {
#include "sub_main/004275D8.inc"
};
u32 gPlayerLookFiles_3_119[2] = {
#include "sub_main/004275E0.inc"
};
u32 gPlayerLookFiles_3_121[2] = {
#include "sub_main/004275E8.inc"
};
u32 gPlayerLookFiles_3_123[2] = {
#include "sub_main/004275F0.inc"
};
u32 gPlayerLookFiles_3_125[2] = {
#include "sub_main/004275F8.inc"
};
u32 gPlayerLookFiles_3_127[2] = {
#include "sub_main/00427600.inc"
};
u32 gPlayerLookFiles_3_129[2] = {
#include "sub_main/00427608.inc"
};
u32 gPlayerLookFiles_3_131[2] = {
#include "sub_main/00427610.inc"
};
u32 gPlayerLookFiles_3_133[2] = {
#include "sub_main/00427618.inc"
};
u32 gPlayerLookFiles_3_135[2] = {
#include "sub_main/00427620.inc"
};
u32 gPlayerLookFiles_3_137[2] = {
#include "sub_main/00427628.inc"
};
u32 gPlayerLookFiles_3_139[2] = {
#include "sub_main/00427630.inc"
};
u32 gPlayerLookFiles_3_141[2] = {
#include "sub_main/00427638.inc"
};
u32 gPlayerLookFiles_3_143[2] = {
#include "sub_main/00427640.inc"
};
u32 gPlayerLookFiles_3_145[2] = {
#include "sub_main/00427648.inc"
};
u32 gPlayerLookFiles_3_147[2] = {
#include "sub_main/00427650.inc"
};
u32 gPlayerLookFiles_3_149[2] = {
#include "sub_main/00427658.inc"
};
u32 gPlayerLookFiles_3_151[2] = {
#include "sub_main/00427660.inc"
};
u32 gPlayerLookFiles_3_153[2] = {
#include "sub_main/00427668.inc"
};
u32 gPlayerLookFiles_3_155[2] = {
#include "sub_main/00427670.inc"
};
u32 gPlayerLookFiles_3_157[2] = {
#include "sub_main/00427678.inc"
};
u32 gPlayerLookFiles_3_159[2] = {
#include "sub_main/00427680.inc"
};
u32 gPlayerLookFiles_3_161[2] = {
#include "sub_main/00427688.inc"
};
u32 gPlayerLookFiles_3_163[2] = {
#include "sub_main/00427690.inc"
};
u32 gPlayerLookFiles_3_165[2] = {
#include "sub_main/00427698.inc"
};
u32 gPlayerLookFiles_3_167[2] = {
#include "sub_main/004276A0.inc"
};
u32 gPlayerLookFiles_3_169[2] = {
#include "sub_main/004276A8.inc"
};
u32 gPlayerLookFiles_3_171[2] = {
#include "sub_main/004276B0.inc"
};
u32 gPlayerLookFiles_3_173[2] = {
#include "sub_main/004276B8.inc"
};
u32 gPlayerLookFiles_3_175[2] = {
#include "sub_main/004276C0.inc"
};
u32 gPlayerLookFiles_3_177[2] = {
#include "sub_main/004276C8.inc"
};
u32 gPlayerLookFiles_3_179[2] = {
#include "sub_main/004276D0.inc"
};
u32 gPlayerLookFiles_3_181[2] = {
#include "sub_main/004276D8.inc"
};
u32 gPlayerLookFiles_3_183[2] = {
#include "sub_main/004276E0.inc"
};
u32 gPlayerLookFiles_3_185[2] = {
#include "sub_main/004276E8.inc"
};
u32 gPlayerLookFiles_3_187[2] = {
#include "sub_main/004276F0.inc"
};
u32 gPlayerLookFiles_3_189[2] = {
#include "sub_main/004276F8.inc"
};
u32 gPlayerLookFiles_3_191[2] = {
#include "sub_main/00427700.inc"
};
u32 gPlayerLookFiles_3_193[2] = {
#include "sub_main/00427708.inc"
};
u32 gPlayerLookFiles_3_195[2] = {
#include "sub_main/00427710.inc"
};
u32 gPlayerLookFiles_3_197[2] = {
#include "sub_main/00427718.inc"
};
u32 gPlayerLookFiles_3_199[2] = {
#include "sub_main/00427720.inc"
};
u32 gPlayerLookFiles_3_201[2] = {
#include "sub_main/00427728.inc"
};
u32 gPlayerLookFiles_3_203[2] = {
#include "sub_main/00427730.inc"
};
u32 gPlayerLookFiles_3_205[2] = {
#include "sub_main/00427738.inc"
};
u32 gPlayerLookFiles_3_207[2] = {
#include "sub_main/00427740.inc"
};
u32 gPlayerLookFiles_3_209[2] = {
#include "sub_main/00427748.inc"
};
u32 gPlayerLookFiles_3_211[2] = {
#include "sub_main/00427750.inc"
};
u32 gPlayerLookFiles_3_213[2] = {
#include "sub_main/00427758.inc"
};
u32 gPlayerLookFiles_3_215[2] = {
#include "sub_main/00427760.inc"
};
u32 gPlayerLookFiles_3_217[2] = {
#include "sub_main/00427768.inc"
};
u32 gPlayerLookFiles_3_219[2] = {
#include "sub_main/00427770.inc"
};
u32 gPlayerLookFiles_3_221[2] = {
#include "sub_main/00427778.inc"
};
u32 gPlayerLookFiles_3_223[2] = {
#include "sub_main/00427780.inc"
};
u32 gPlayerLookFiles_3_225[2] = {
#include "sub_main/00427788.inc"
};
u32 gPlayerLookFiles_3_227[2] = {
#include "sub_main/00427790.inc"
};
u32 gPlayerLookFiles_3_229[2] = {
#include "sub_main/00427798.inc"
};
u32 gPlayerLookFiles_3_231[2] = {
#include "sub_main/004277A0.inc"
};
u32 gPlayerLookFiles_3_233[2] = {
#include "sub_main/004277A8.inc"
};
u32 gPlayerLookFiles_3_235[2] = {
#include "sub_main/004277B0.inc"
};
u32 gPlayerLookFiles_3_237[2] = {
#include "sub_main/004277B8.inc"
};
u32 gPlayerLookFiles_3_239[2] = {
#include "sub_main/004277C0.inc"
};
u32 gPlayerLookFiles_3_241[2] = {
#include "sub_main/004277C8.inc"
};
u32 gPlayerLookFiles_3_243[2] = {
#include "sub_main/004277D0.inc"
};
u32 gPlayerLookFiles_3_245[2] = {
#include "sub_main/004277D8.inc"
};
u32 gPlayerLookFiles_3_247[2] = {
#include "sub_main/004277E0.inc"
};
u32 gPlayerLookFiles_3_249[2] = {
#include "sub_main/004277E8.inc"
};
u32 gPlayerLookFiles_3_251[2] = {
#include "sub_main/004277F0.inc"
};
u32 gPlayerLookFiles_3_253[2] = {
#include "sub_main/004277F8.inc"
};
u32 gPlayerLookFiles_3_255[2] = {
#include "sub_main/00427800.inc"
};
u32 gPlayerLookFiles_3_257[2] = {
#include "sub_main/00427808.inc"
};
u32 gPlayerLookFiles_3_259[2] = {
#include "sub_main/00427810.inc"
};
u32 gPlayerLookFiles_3_261[2] = {
#include "sub_main/00427818.inc"
};
u32 gPlayerLookFiles_3_263[2] = {
#include "sub_main/00427820.inc"
};
u32 gPlayerLookFiles_3_265[2] = {
#include "sub_main/00427828.inc"
};
u32 gPlayerLookFiles_3_267[2] = {
#include "sub_main/00427830.inc"
};
u32 gPlayerLookFiles_3_269[2] = {
#include "sub_main/00427838.inc"
};
u32 gPlayerLookFiles_3_271[2] = {
#include "sub_main/00427840.inc"
};
u32 gPlayerLookFiles_3_273[2] = {
#include "sub_main/00427848.inc"
};
u32 gPlayerLookFiles_3_275[2] = {
#include "sub_main/00427850.inc"
};
u32 gPlayerLookFiles_3_277[2] = {
#include "sub_main/00427858.inc"
};
u32 gPlayerLookFiles_3_279[2] = {
#include "sub_main/00427860.inc"
};
u32 gPlayerLookFiles_3_281[2] = {
#include "sub_main/00427868.inc"
};
u32 gPlayerLookFiles_3_283[2] = {
#include "sub_main/00427870.inc"
};
u32 gPlayerLookFiles_3_285[2] = {
#include "sub_main/00427878.inc"
};
u32 gPlayerLookFiles_3_287[2] = {
#include "sub_main/00427880.inc"
};
u32 gPlayerLookFiles_3_289[2] = {
#include "sub_main/00427888.inc"
};
u32 gPlayerLookFiles_3_291[2] = {
#include "sub_main/00427890.inc"
};
u32 gPlayerLookFiles_3_293[2] = {
#include "sub_main/00427898.inc"
};
u32 gPlayerLookFiles_3_295[2] = {
#include "sub_main/004278A0.inc"
};
u32 gPlayerLookFiles_3_297[2] = {
#include "sub_main/004278A8.inc"
};
u32 gPlayerLookFiles_3_299[2] = {
#include "sub_main/004278B0.inc"
};
u32 gPlayerLookFiles_3_301[2] = {
#include "sub_main/004278B8.inc"
};
u32 gPlayerLookFiles_3_303[2] = {
#include "sub_main/004278C0.inc"
};
u32 gPlayerLookFiles_3_305[2] = {
#include "sub_main/004278C8.inc"
};
u32 gPlayerLookFiles_3_307[2] = {
#include "sub_main/004278D0.inc"
};
u32 gPlayerLookFiles_3_309[2] = {
#include "sub_main/004278D8.inc"
};
u32 gPlayerLookFiles_3_311[2] = {
#include "sub_main/004278E0.inc"
};
u32 gPlayerLookFiles_3_313[2] = {
#include "sub_main/004278E8.inc"
};
u32 gPlayerLookFiles_3_315[2] = {
#include "sub_main/004278F0.inc"
};
u32 gPlayerLookFiles_3_317[2] = {
#include "sub_main/004278F8.inc"
};
u32 gPlayerLookFiles_3_319[2] = {
#include "sub_main/00427900.inc"
};
u32 gPlayerLookFiles_3_321[2] = {
#include "sub_main/00427908.inc"
};
u32 gPlayerLookFiles_3_323[2] = {
#include "sub_main/00427910.inc"
};
u32 gPlayerLookFiles_3_325[2] = {
#include "sub_main/00427918.inc"
};
u32 gPlayerLookFiles_3_327[2] = {
#include "sub_main/00427920.inc"
};
u32 gPlayerLookFiles_3_329[2] = {
#include "sub_main/00427928.inc"
};
u32 gPlayerLookFiles_3_331[2] = {
#include "sub_main/00427930.inc"
};
u32 gPlayerLookFiles_3_333[2] = {
#include "sub_main/00427938.inc"
};
u32 gPlayerLookFiles_3_335[2] = {
#include "sub_main/00427940.inc"
};
u32 gPlayerLookFiles_3_337[2] = {
#include "sub_main/00427948.inc"
};
u32 gPlayerLookFiles_3_339[2] = {
#include "sub_main/00427950.inc"
};
u32 gPlayerLookFiles_3_341[2] = {
#include "sub_main/00427958.inc"
};
u32 gPlayerLookFiles_3_343[2] = {
#include "sub_main/00427960.inc"
};
u32 gPlayerLookFiles_3_345[2] = {
#include "sub_main/00427968.inc"
};
u32 gPlayerLookFiles_3_347[2] = {
#include "sub_main/00427970.inc"
};
u32 gPlayerLookFiles_3_349[2] = {
#include "sub_main/00427978.inc"
};
u32 gPlayerLookFiles_3_351[2] = {
#include "sub_main/00427980.inc"
};
u32 gPlayerLookFiles_3_353[2] = {
#include "sub_main/00427988.inc"
};
u32 gPlayerLookFiles_3_355[2] = {
#include "sub_main/00427990.inc"
};
u32 gPlayerLookFiles_3_357[2] = {
#include "sub_main/00427998.inc"
};
u32 gPlayerLookFiles_3_359[2] = {
#include "sub_main/004279A0.inc"
};
u32 gPlayerLookFiles_3_361[2] = {
#include "sub_main/004279A8.inc"
};
u32 gPlayerLookFiles_3_363[2] = {
#include "sub_main/004279B0.inc"
};
u32 gPlayerLookFiles_3_365[2] = {
#include "sub_main/004279B8.inc"
};
u32 gPlayerLookFiles_3_367[2] = {
#include "sub_main/004279C0.inc"
};
u32 gPlayerLookFiles_3_369[2] = {
#include "sub_main/004279C8.inc"
};
u32 gPlayerLookFiles_3_371[2] = {
#include "sub_main/004279D0.inc"
};
u32 gPlayerLookFiles_3_373[2] = {
#include "sub_main/004279D8.inc"
};
u32 gPlayerLookFiles_3_375[2] = {
#include "sub_main/004279E0.inc"
};
u32 gPlayerLookFiles_3_377[2] = {
#include "sub_main/004279E8.inc"
};
u32 gPlayerLookFiles_3_379[2] = {
#include "sub_main/004279F0.inc"
};
u32 gPlayerLookFiles_3_381[2] = {
#include "sub_main/004279F8.inc"
};
u32 gPlayerLookFiles_3_383[2] = {
#include "sub_main/00427A00.inc"
};
u32 gPlayerLookFiles_3_385[2] = {
#include "sub_main/00427A08.inc"
};
u32 gPlayerLookFiles_3_387[2] = {
#include "sub_main/00427A10.inc"
};
u32 gPlayerLookFiles_3_389[2] = {
#include "sub_main/00427A18.inc"
};
u32 gPlayerLookFiles_3_391[2] = {
#include "sub_main/00427A20.inc"
};
u32 gPlayerLookFiles_3_393[2] = {
#include "sub_main/00427A28.inc"
};
u32 gPlayerLookFiles_3_395[2] = {
#include "sub_main/00427A30.inc"
};
u32 gPlayerLookFiles_3_397[2] = {
#include "sub_main/00427A38.inc"
};
u32 gPlayerLookFiles_3_399[2] = {
#include "sub_main/00427A40.inc"
};
u32 gPlayerLookFiles_3_401[2] = {
#include "sub_main/00427A48.inc"
};
u32 gPlayerLookFiles_3_403[2] = {
#include "sub_main/00427A50.inc"
};
u32 gPlayerLookFiles_3_405[2] = {
#include "sub_main/00427A58.inc"
};
u32 gPlayerLookFiles_3_407[2] = {
#include "sub_main/00427A60.inc"
};
u32 gPlayerLookFiles_3_409[2] = {
#include "sub_main/00427A68.inc"
};
u32 gPlayerLookFiles_3_411[2] = {
#include "sub_main/00427A70.inc"
};
u32 gPlayerLookFiles_3_413[2] = {
#include "sub_main/00427A78.inc"
};
u32 gPlayerLookFiles_3_415[2] = {
#include "sub_main/00427A80.inc"
};
u32 gPlayerLookFiles_3_417[2] = {
#include "sub_main/00427A88.inc"
};
u32 gPlayerLookFiles_3_419[2] = {
#include "sub_main/00427A90.inc"
};
u32 gPlayerLookFiles_3_421[2] = {
#include "sub_main/00427A98.inc"
};
u32 gPlayerLookFiles_3_423[2] = {
#include "sub_main/00427AA0.inc"
};
u32 gPlayerLookFiles_3_425[2] = {
#include "sub_main/00427AA8.inc"
};
u32 gPlayerLookFiles_3_427[2] = {
#include "sub_main/00427AB0.inc"
};
u32 gPlayerLookFiles_3_429[2] = {
#include "sub_main/00427AB8.inc"
};
u32 gPlayerLookFiles_3_431[2] = {
#include "sub_main/00427AC0.inc"
};
u32 gPlayerLookFiles_3_433[2] = {
#include "sub_main/00427AC8.inc"
};
u32 gPlayerLookFiles_3_435[2] = {
#include "sub_main/00427AD0.inc"
};
u32 gPlayerLookFiles_3_437[2] = {
#include "sub_main/00427AD8.inc"
};
u32 gPlayerLookFiles_3_439[2] = {
#include "sub_main/00427AE0.inc"
};
u32 gPlayerLookFiles_3_441[2] = {
#include "sub_main/00427AE8.inc"
};
u32 gPlayerLookFiles_3_443[2] = {
#include "sub_main/00427AF0.inc"
};
u32 gPlayerLookFiles_3_445[2] = {
#include "sub_main/00427AF8.inc"
};
u32 gPlayerLookFiles_3_447[2] = {
#include "sub_main/00427B00.inc"
};
u32 gPlayerLookFiles_3_449[2] = {
#include "sub_main/00427B08.inc"
};
u32 gPlayerLookFiles_3_451[2] = {
#include "sub_main/00427B10.inc"
};
u32 gPlayerLookFiles_3_453[2] = {
#include "sub_main/00427B18.inc"
};
u32 gPlayerLookFiles_3_455[2] = {
#include "sub_main/00427B20.inc"
};
u32 gPlayerLookFiles_3_457[2] = {
#include "sub_main/00427B28.inc"
};
u32 gPlayerLookFiles_3_459[2] = {
#include "sub_main/00427B30.inc"
};
u32 gPlayerLookFiles_3_461[2] = {
#include "sub_main/00427B38.inc"
};
u32 gPlayerLookFiles_3_463[2] = {
#include "sub_main/00427B40.inc"
};
u32 gPlayerLookFiles_3_465[2] = {
#include "sub_main/00427B48.inc"
};
u32 gPlayerLookFiles_3_467[2] = {
#include "sub_main/00427B50.inc"
};
u32 gPlayerLookFiles_3_469[2] = {
#include "sub_main/00427B58.inc"
};
u32 gPlayerLookFiles_3_471[4] = {
#include "sub_main/00427B60.inc"
};
u32 gPlayerLookFiles_3[472] = {
#include "sub_main/00427B70.inc"
};
u32 gPlayerLookFiles_4_1[2] = {
#include "sub_main/004282D0.inc"
};
u32 gPlayerLookFiles_4_3[2] = {
#include "sub_main/004282D8.inc"
};
u32 gPlayerLookFiles_4_5[2] = {
#include "sub_main/004282E0.inc"
};
u32 gPlayerLookFiles_4_7[2] = {
#include "sub_main/004282E8.inc"
};
u32 gPlayerLookFiles_4_9[2] = {
#include "sub_main/004282F0.inc"
};
u32 gPlayerLookFiles_4_11[2] = {
#include "sub_main/004282F8.inc"
};
u32 gPlayerLookFiles_4_13[2] = {
#include "sub_main/00428300.inc"
};
u32 gPlayerLookFiles_4_15[2] = {
#include "sub_main/00428308.inc"
};
u32 gPlayerLookFiles_4_17[2] = {
#include "sub_main/00428310.inc"
};
u32 gPlayerLookFiles_4_19[2] = {
#include "sub_main/00428318.inc"
};
u32 gPlayerLookFiles_4_21[2] = {
#include "sub_main/00428320.inc"
};
u32 gPlayerLookFiles_4_23[2] = {
#include "sub_main/00428328.inc"
};
u32 gPlayerLookFiles_4_25[2] = {
#include "sub_main/00428330.inc"
};
u32 gPlayerLookFiles_4_27[2] = {
#include "sub_main/00428338.inc"
};
u32 gPlayerLookFiles_4_29[2] = {
#include "sub_main/00428340.inc"
};
u32 gPlayerLookFiles_4_31[2] = {
#include "sub_main/00428348.inc"
};
u32 gPlayerLookFiles_4_33[2] = {
#include "sub_main/00428350.inc"
};
u32 gPlayerLookFiles_4_35[2] = {
#include "sub_main/00428358.inc"
};
u32 gPlayerLookFiles_4_37[2] = {
#include "sub_main/00428360.inc"
};
u32 gPlayerLookFiles_4_39[2] = {
#include "sub_main/00428368.inc"
};
u32 gPlayerLookFiles_4_41[2] = {
#include "sub_main/00428370.inc"
};
u32 gPlayerLookFiles_4_43[2] = {
#include "sub_main/00428378.inc"
};
u32 gPlayerLookFiles_4_45[2] = {
#include "sub_main/00428380.inc"
};
u32 gPlayerLookFiles_4_47[2] = {
#include "sub_main/00428388.inc"
};
u32 gPlayerLookFiles_4_49[2] = {
#include "sub_main/00428390.inc"
};
u32 gPlayerLookFiles_4_51[2] = {
#include "sub_main/00428398.inc"
};
u32 gPlayerLookFiles_4_53[2] = {
#include "sub_main/004283A0.inc"
};
u32 gPlayerLookFiles_4_55[2] = {
#include "sub_main/004283A8.inc"
};
u32 gPlayerLookFiles_4_57[2] = {
#include "sub_main/004283B0.inc"
};
u32 gPlayerLookFiles_4_59[2] = {
#include "sub_main/004283B8.inc"
};
u32 gPlayerLookFiles_4_61[2] = {
#include "sub_main/004283C0.inc"
};
u32 gPlayerLookFiles_4_63[2] = {
#include "sub_main/004283C8.inc"
};
u32 gPlayerLookFiles_4_65[2] = {
#include "sub_main/004283D0.inc"
};
u32 gPlayerLookFiles_4_67[2] = {
#include "sub_main/004283D8.inc"
};
u32 gPlayerLookFiles_4_69[2] = {
#include "sub_main/004283E0.inc"
};
u32 gPlayerLookFiles_4_71[2] = {
#include "sub_main/004283E8.inc"
};
u32 gPlayerLookFiles_4_73[2] = {
#include "sub_main/004283F0.inc"
};
u32 gPlayerLookFiles_4_75[2] = {
#include "sub_main/004283F8.inc"
};
u32 gPlayerLookFiles_4_77[2] = {
#include "sub_main/00428400.inc"
};
u32 gPlayerLookFiles_4_79[2] = {
#include "sub_main/00428408.inc"
};
u32 gPlayerLookFiles_4_81[2] = {
#include "sub_main/00428410.inc"
};
u32 gPlayerLookFiles_4_83[2] = {
#include "sub_main/00428418.inc"
};
u32 gPlayerLookFiles_4_85[2] = {
#include "sub_main/00428420.inc"
};
u32 gPlayerLookFiles_4_87[2] = {
#include "sub_main/00428428.inc"
};
u32 gPlayerLookFiles_4_89[2] = {
#include "sub_main/00428430.inc"
};
u32 gPlayerLookFiles_4_91[2] = {
#include "sub_main/00428438.inc"
};
u32 gPlayerLookFiles_4_93[2] = {
#include "sub_main/00428440.inc"
};
u32 gPlayerLookFiles_4_95[2] = {
#include "sub_main/00428448.inc"
};
u32 gPlayerLookFiles_4_97[2] = {
#include "sub_main/00428450.inc"
};
u32 gPlayerLookFiles_4_99[2] = {
#include "sub_main/00428458.inc"
};
u32 gPlayerLookFiles_4_101[2] = {
#include "sub_main/00428460.inc"
};
u32 gPlayerLookFiles_4_103[2] = {
#include "sub_main/00428468.inc"
};
u32 gPlayerLookFiles_4_105[2] = {
#include "sub_main/00428470.inc"
};
u32 gPlayerLookFiles_4_107[2] = {
#include "sub_main/00428478.inc"
};
u32 gPlayerLookFiles_4_109[2] = {
#include "sub_main/00428480.inc"
};
u32 gPlayerLookFiles_4_111[2] = {
#include "sub_main/00428488.inc"
};
u32 gPlayerLookFiles_4_113[2] = {
#include "sub_main/00428490.inc"
};
u32 gPlayerLookFiles_4_115[2] = {
#include "sub_main/00428498.inc"
};
u32 gPlayerLookFiles_4_117[2] = {
#include "sub_main/004284A0.inc"
};
u32 gPlayerLookFiles_4_119[2] = {
#include "sub_main/004284A8.inc"
};
u32 gPlayerLookFiles_4_121[2] = {
#include "sub_main/004284B0.inc"
};
u32 gPlayerLookFiles_4_123[2] = {
#include "sub_main/004284B8.inc"
};
u32 gPlayerLookFiles_4_125[2] = {
#include "sub_main/004284C0.inc"
};
u32 gPlayerLookFiles_4_127[2] = {
#include "sub_main/004284C8.inc"
};
u32 gPlayerLookFiles_4_129[2] = {
#include "sub_main/004284D0.inc"
};
u32 gPlayerLookFiles_4_131[2] = {
#include "sub_main/004284D8.inc"
};
u32 gPlayerLookFiles_4_133[2] = {
#include "sub_main/004284E0.inc"
};
u32 gPlayerLookFiles_4_135[2] = {
#include "sub_main/004284E8.inc"
};
u32 gPlayerLookFiles_4_137[2] = {
#include "sub_main/004284F0.inc"
};
u32 gPlayerLookFiles_4_139[2] = {
#include "sub_main/004284F8.inc"
};
u32 gPlayerLookFiles_4_141[2] = {
#include "sub_main/00428500.inc"
};
u32 gPlayerLookFiles_4_143[2] = {
#include "sub_main/00428508.inc"
};
u32 gPlayerLookFiles_4_145[2] = {
#include "sub_main/00428510.inc"
};
u32 gPlayerLookFiles_4_147[2] = {
#include "sub_main/00428518.inc"
};
u32 gPlayerLookFiles_4_149[2] = {
#include "sub_main/00428520.inc"
};
u32 gPlayerLookFiles_4_151[2] = {
#include "sub_main/00428528.inc"
};
u32 gPlayerLookFiles_4_153[2] = {
#include "sub_main/00428530.inc"
};
u32 gPlayerLookFiles_4_155[2] = {
#include "sub_main/00428538.inc"
};
u32 gPlayerLookFiles_4_157[2] = {
#include "sub_main/00428540.inc"
};
u32 gPlayerLookFiles_4_159[2] = {
#include "sub_main/00428548.inc"
};
u32 gPlayerLookFiles_4_161[2] = {
#include "sub_main/00428550.inc"
};
u32 gPlayerLookFiles_4_163[2] = {
#include "sub_main/00428558.inc"
};
u32 gPlayerLookFiles_4_165[2] = {
#include "sub_main/00428560.inc"
};
u32 gPlayerLookFiles_4_167[2] = {
#include "sub_main/00428568.inc"
};
u32 gPlayerLookFiles_4_169[2] = {
#include "sub_main/00428570.inc"
};
u32 gPlayerLookFiles_4_171[2] = {
#include "sub_main/00428578.inc"
};
u32 gPlayerLookFiles_4_173[2] = {
#include "sub_main/00428580.inc"
};
u32 gPlayerLookFiles_4_175[2] = {
#include "sub_main/00428588.inc"
};
u32 gPlayerLookFiles_4_177[2] = {
#include "sub_main/00428590.inc"
};
u32 gPlayerLookFiles_4_179[2] = {
#include "sub_main/00428598.inc"
};
u32 gPlayerLookFiles_4_181[2] = {
#include "sub_main/004285A0.inc"
};
u32 gPlayerLookFiles_4_183[2] = {
#include "sub_main/004285A8.inc"
};
u32 gPlayerLookFiles_4_185[2] = {
#include "sub_main/004285B0.inc"
};
u32 gPlayerLookFiles_4_187[2] = {
#include "sub_main/004285B8.inc"
};
u32 gPlayerLookFiles_4_189[2] = {
#include "sub_main/004285C0.inc"
};
u32 gPlayerLookFiles_4_191[2] = {
#include "sub_main/004285C8.inc"
};
u32 gPlayerLookFiles_4_193[2] = {
#include "sub_main/004285D0.inc"
};
u32 gPlayerLookFiles_4_195[2] = {
#include "sub_main/004285D8.inc"
};
u32 gPlayerLookFiles_4_197[2] = {
#include "sub_main/004285E0.inc"
};
u32 gPlayerLookFiles_4_199[2] = {
#include "sub_main/004285E8.inc"
};
u32 gPlayerLookFiles_4_201[2] = {
#include "sub_main/004285F0.inc"
};
u32 gPlayerLookFiles_4_203[2] = {
#include "sub_main/004285F8.inc"
};
u32 gPlayerLookFiles_4_205[2] = {
#include "sub_main/00428600.inc"
};
u32 gPlayerLookFiles_4_207[2] = {
#include "sub_main/00428608.inc"
};
u32 gPlayerLookFiles_4_209[2] = {
#include "sub_main/00428610.inc"
};
u32 gPlayerLookFiles_4_211[2] = {
#include "sub_main/00428618.inc"
};
u32 gPlayerLookFiles_4_213[2] = {
#include "sub_main/00428620.inc"
};
u32 gPlayerLookFiles_4_215[2] = {
#include "sub_main/00428628.inc"
};
u32 gPlayerLookFiles_4_217[2] = {
#include "sub_main/00428630.inc"
};
u32 gPlayerLookFiles_4_219[2] = {
#include "sub_main/00428638.inc"
};
u32 gPlayerLookFiles_4_221[2] = {
#include "sub_main/00428640.inc"
};
u32 gPlayerLookFiles_4_223[2] = {
#include "sub_main/00428648.inc"
};
u32 gPlayerLookFiles_4_225[2] = {
#include "sub_main/00428650.inc"
};
u32 gPlayerLookFiles_4_227[2] = {
#include "sub_main/00428658.inc"
};
u32 gPlayerLookFiles_4_229[2] = {
#include "sub_main/00428660.inc"
};
u32 gPlayerLookFiles_4_231[2] = {
#include "sub_main/00428668.inc"
};
u32 gPlayerLookFiles_4_233[2] = {
#include "sub_main/00428670.inc"
};
u32 gPlayerLookFiles_4_235[2] = {
#include "sub_main/00428678.inc"
};
u32 gPlayerLookFiles_4_237[2] = {
#include "sub_main/00428680.inc"
};
u32 gPlayerLookFiles_4_239[2] = {
#include "sub_main/00428688.inc"
};
u32 gPlayerLookFiles_4_241[2] = {
#include "sub_main/00428690.inc"
};
u32 gPlayerLookFiles_4_243[2] = {
#include "sub_main/00428698.inc"
};
u32 gPlayerLookFiles_4_245[2] = {
#include "sub_main/004286A0.inc"
};
u32 gPlayerLookFiles_4_247[2] = {
#include "sub_main/004286A8.inc"
};
u32 gPlayerLookFiles_4_249[2] = {
#include "sub_main/004286B0.inc"
};
u32 gPlayerLookFiles_4_251[2] = {
#include "sub_main/004286B8.inc"
};
u32 gPlayerLookFiles_4_253[2] = {
#include "sub_main/004286C0.inc"
};
u32 gPlayerLookFiles_4_255[2] = {
#include "sub_main/004286C8.inc"
};
u32 gPlayerLookFiles_4_257[2] = {
#include "sub_main/004286D0.inc"
};
u32 gPlayerLookFiles_4_259[2] = {
#include "sub_main/004286D8.inc"
};
u32 gPlayerLookFiles_4_261[2] = {
#include "sub_main/004286E0.inc"
};
u32 gPlayerLookFiles_4_263[2] = {
#include "sub_main/004286E8.inc"
};
u32 gPlayerLookFiles_4_265[2] = {
#include "sub_main/004286F0.inc"
};
u32 gPlayerLookFiles_4_267[2] = {
#include "sub_main/004286F8.inc"
};
u32 gPlayerLookFiles_4_269[2] = {
#include "sub_main/00428700.inc"
};
u32 gPlayerLookFiles_4_271[2] = {
#include "sub_main/00428708.inc"
};
u32 gPlayerLookFiles_4_273[2] = {
#include "sub_main/00428710.inc"
};
u32 gPlayerLookFiles_4_275[2] = {
#include "sub_main/00428718.inc"
};
u32 gPlayerLookFiles_4_277[2] = {
#include "sub_main/00428720.inc"
};
u32 gPlayerLookFiles_4_279[2] = {
#include "sub_main/00428728.inc"
};
u32 gPlayerLookFiles_4_281[2] = {
#include "sub_main/00428730.inc"
};
u32 gPlayerLookFiles_4_283[2] = {
#include "sub_main/00428738.inc"
};
u32 gPlayerLookFiles_4_285[2] = {
#include "sub_main/00428740.inc"
};
u32 gPlayerLookFiles_4_287[2] = {
#include "sub_main/00428748.inc"
};
u32 gPlayerLookFiles_4_289[2] = {
#include "sub_main/00428750.inc"
};
u32 gPlayerLookFiles_4_291[2] = {
#include "sub_main/00428758.inc"
};
u32 gPlayerLookFiles_4_293[2] = {
#include "sub_main/00428760.inc"
};
u32 gPlayerLookFiles_4_295[2] = {
#include "sub_main/00428768.inc"
};
u32 gPlayerLookFiles_4_297[2] = {
#include "sub_main/00428770.inc"
};
u32 gPlayerLookFiles_4_299[2] = {
#include "sub_main/00428778.inc"
};
u32 gPlayerLookFiles_4_301[2] = {
#include "sub_main/00428780.inc"
};
u32 gPlayerLookFiles_4_303[2] = {
#include "sub_main/00428788.inc"
};
u32 gPlayerLookFiles_4_305[2] = {
#include "sub_main/00428790.inc"
};
u32 gPlayerLookFiles_4_307[2] = {
#include "sub_main/00428798.inc"
};
u32 gPlayerLookFiles_4_309[2] = {
#include "sub_main/004287A0.inc"
};
u32 gPlayerLookFiles_4_311[2] = {
#include "sub_main/004287A8.inc"
};
u32 gPlayerLookFiles_4_313[2] = {
#include "sub_main/004287B0.inc"
};
u32 gPlayerLookFiles_4_315[2] = {
#include "sub_main/004287B8.inc"
};
u32 gPlayerLookFiles_4_317[2] = {
#include "sub_main/004287C0.inc"
};
u32 gPlayerLookFiles_4_319[2] = {
#include "sub_main/004287C8.inc"
};
u32 gPlayerLookFiles_4_321[2] = {
#include "sub_main/004287D0.inc"
};
u32 gPlayerLookFiles_4_323[2] = {
#include "sub_main/004287D8.inc"
};
u32 gPlayerLookFiles_4_325[2] = {
#include "sub_main/004287E0.inc"
};
u32 gPlayerLookFiles_4_327[2] = {
#include "sub_main/004287E8.inc"
};
u32 gPlayerLookFiles_4_329[2] = {
#include "sub_main/004287F0.inc"
};
u32 gPlayerLookFiles_4_331[2] = {
#include "sub_main/004287F8.inc"
};
u32 gPlayerLookFiles_4_333[2] = {
#include "sub_main/00428800.inc"
};
u32 gPlayerLookFiles_4_335[2] = {
#include "sub_main/00428808.inc"
};
u32 gPlayerLookFiles_4_337[2] = {
#include "sub_main/00428810.inc"
};
u32 gPlayerLookFiles_4_339[2] = {
#include "sub_main/00428818.inc"
};
u32 gPlayerLookFiles_4_341[2] = {
#include "sub_main/00428820.inc"
};
u32 gPlayerLookFiles_4_343[2] = {
#include "sub_main/00428828.inc"
};
u32 gPlayerLookFiles_4_345[2] = {
#include "sub_main/00428830.inc"
};
u32 gPlayerLookFiles_4_347[2] = {
#include "sub_main/00428838.inc"
};
u32 gPlayerLookFiles_4_349[2] = {
#include "sub_main/00428840.inc"
};
u32 gPlayerLookFiles_4_351[2] = {
#include "sub_main/00428848.inc"
};
u32 gPlayerLookFiles_4_353[2] = {
#include "sub_main/00428850.inc"
};
u32 gPlayerLookFiles_4_355[2] = {
#include "sub_main/00428858.inc"
};
u32 gPlayerLookFiles_4_357[2] = {
#include "sub_main/00428860.inc"
};
u32 gPlayerLookFiles_4_359[2] = {
#include "sub_main/00428868.inc"
};
u32 gPlayerLookFiles_4_361[2] = {
#include "sub_main/00428870.inc"
};
u32 gPlayerLookFiles_4_363[2] = {
#include "sub_main/00428878.inc"
};
u32 gPlayerLookFiles_4_365[2] = {
#include "sub_main/00428880.inc"
};
u32 gPlayerLookFiles_4_367[2] = {
#include "sub_main/00428888.inc"
};
u32 gPlayerLookFiles_4_369[2] = {
#include "sub_main/00428890.inc"
};
u32 gPlayerLookFiles_4_371[2] = {
#include "sub_main/00428898.inc"
};
u32 gPlayerLookFiles_4_373[2] = {
#include "sub_main/004288A0.inc"
};
u32 gPlayerLookFiles_4_375[2] = {
#include "sub_main/004288A8.inc"
};
u32 gPlayerLookFiles_4_377[2] = {
#include "sub_main/004288B0.inc"
};
u32 gPlayerLookFiles_4_379[2] = {
#include "sub_main/004288B8.inc"
};
u32 gPlayerLookFiles_4_381[2] = {
#include "sub_main/004288C0.inc"
};
u32 gPlayerLookFiles_4_383[2] = {
#include "sub_main/004288C8.inc"
};
u32 gPlayerLookFiles_4_385[2] = {
#include "sub_main/004288D0.inc"
};
u32 gPlayerLookFiles_4_387[2] = {
#include "sub_main/004288D8.inc"
};
u32 gPlayerLookFiles_4_389[2] = {
#include "sub_main/004288E0.inc"
};
u32 gPlayerLookFiles_4_391[2] = {
#include "sub_main/004288E8.inc"
};
u32 gPlayerLookFiles_4_393[2] = {
#include "sub_main/004288F0.inc"
};
u32 gPlayerLookFiles_4_395[2] = {
#include "sub_main/004288F8.inc"
};
u32 gPlayerLookFiles_4_397[2] = {
#include "sub_main/00428900.inc"
};
u32 gPlayerLookFiles_4_399[2] = {
#include "sub_main/00428908.inc"
};
u32 gPlayerLookFiles_4_401[2] = {
#include "sub_main/00428910.inc"
};
u32 gPlayerLookFiles_4_403[2] = {
#include "sub_main/00428918.inc"
};
u32 gPlayerLookFiles_4_405[2] = {
#include "sub_main/00428920.inc"
};
u32 gPlayerLookFiles_4_407[2] = {
#include "sub_main/00428928.inc"
};
u32 gPlayerLookFiles_4_409[2] = {
#include "sub_main/00428930.inc"
};
u32 gPlayerLookFiles_4_411[2] = {
#include "sub_main/00428938.inc"
};
u32 gPlayerLookFiles_4_413[2] = {
#include "sub_main/00428940.inc"
};
u32 gPlayerLookFiles_4_415[2] = {
#include "sub_main/00428948.inc"
};
u32 gPlayerLookFiles_4_417[2] = {
#include "sub_main/00428950.inc"
};
u32 gPlayerLookFiles_4_419[2] = {
#include "sub_main/00428958.inc"
};
u32 gPlayerLookFiles_4_421[2] = {
#include "sub_main/00428960.inc"
};
u32 gPlayerLookFiles_4_423[2] = {
#include "sub_main/00428968.inc"
};
u32 gPlayerLookFiles_4_425[2] = {
#include "sub_main/00428970.inc"
};
u32 gPlayerLookFiles_4_427[2] = {
#include "sub_main/00428978.inc"
};
u32 gPlayerLookFiles_4_429[2] = {
#include "sub_main/00428980.inc"
};
u32 gPlayerLookFiles_4_431[2] = {
#include "sub_main/00428988.inc"
};
u32 gPlayerLookFiles_4_433[2] = {
#include "sub_main/00428990.inc"
};
u32 gPlayerLookFiles_4_435[2] = {
#include "sub_main/00428998.inc"
};
u32 gPlayerLookFiles_4_437[2] = {
#include "sub_main/004289A0.inc"
};
u32 gPlayerLookFiles_4_439[2] = {
#include "sub_main/004289A8.inc"
};
u32 gPlayerLookFiles_4_441[2] = {
#include "sub_main/004289B0.inc"
};
u32 gPlayerLookFiles_4_443[2] = {
#include "sub_main/004289B8.inc"
};
u32 gPlayerLookFiles_4_445[2] = {
#include "sub_main/004289C0.inc"
};
u32 gPlayerLookFiles_4_447[2] = {
#include "sub_main/004289C8.inc"
};
u32 gPlayerLookFiles_4_449[2] = {
#include "sub_main/004289D0.inc"
};
u32 gPlayerLookFiles_4_451[2] = {
#include "sub_main/004289D8.inc"
};
u32 gPlayerLookFiles_4_453[2] = {
#include "sub_main/004289E0.inc"
};
u32 gPlayerLookFiles_4_455[2] = {
#include "sub_main/004289E8.inc"
};
u32 gPlayerLookFiles_4[456] = {
#include "sub_main/004289F0.inc"
};
u32 gPlayerLookFiles_5_1[2] = {
#include "sub_main/00429110.inc"
};
u32 gPlayerLookFiles_5_3[2] = {
#include "sub_main/00429118.inc"
};
u32 gPlayerLookFiles_5_5[2] = {
#include "sub_main/00429120.inc"
};
u32 gPlayerLookFiles_5_7[2] = {
#include "sub_main/00429128.inc"
};
u32 gPlayerLookFiles_5_9[2] = {
#include "sub_main/00429130.inc"
};
u32 gPlayerLookFiles_5_11[2] = {
#include "sub_main/00429138.inc"
};
u32 gPlayerLookFiles_5_13[2] = {
#include "sub_main/00429140.inc"
};
u32 gPlayerLookFiles_5_15[2] = {
#include "sub_main/00429148.inc"
};
u32 gPlayerLookFiles_5_17[2] = {
#include "sub_main/00429150.inc"
};
u32 gPlayerLookFiles_5_19[2] = {
#include "sub_main/00429158.inc"
};
u32 gPlayerLookFiles_5_21[2] = {
#include "sub_main/00429160.inc"
};
u32 gPlayerLookFiles_5_23[2] = {
#include "sub_main/00429168.inc"
};
u32 gPlayerLookFiles_5_25[2] = {
#include "sub_main/00429170.inc"
};
u32 gPlayerLookFiles_5_27[2] = {
#include "sub_main/00429178.inc"
};
u32 gPlayerLookFiles_5_29[2] = {
#include "sub_main/00429180.inc"
};
u32 gPlayerLookFiles_5_31[2] = {
#include "sub_main/00429188.inc"
};
u32 gPlayerLookFiles_5_33[2] = {
#include "sub_main/00429190.inc"
};
u32 gPlayerLookFiles_5_35[2] = {
#include "sub_main/00429198.inc"
};
u32 gPlayerLookFiles_5_37[2] = {
#include "sub_main/004291A0.inc"
};
u32 gPlayerLookFiles_5_39[2] = {
#include "sub_main/004291A8.inc"
};
u32 gPlayerLookFiles_5_41[2] = {
#include "sub_main/004291B0.inc"
};
u32 gPlayerLookFiles_5_43[2] = {
#include "sub_main/004291B8.inc"
};
u32 gPlayerLookFiles_5_45[2] = {
#include "sub_main/004291C0.inc"
};
u32 gPlayerLookFiles_5_47[2] = {
#include "sub_main/004291C8.inc"
};
u32 gPlayerLookFiles_5_49[2] = {
#include "sub_main/004291D0.inc"
};
u32 gPlayerLookFiles_5_51[2] = {
#include "sub_main/004291D8.inc"
};
u32 gPlayerLookFiles_5_53[2] = {
#include "sub_main/004291E0.inc"
};
u32 gPlayerLookFiles_5_55[2] = {
#include "sub_main/004291E8.inc"
};
u32 gPlayerLookFiles_5_57[2] = {
#include "sub_main/004291F0.inc"
};
u32 gPlayerLookFiles_5_59[2] = {
#include "sub_main/004291F8.inc"
};
u32 gPlayerLookFiles_5_61[2] = {
#include "sub_main/00429200.inc"
};
u32 gPlayerLookFiles_5_63[2] = {
#include "sub_main/00429208.inc"
};
u32 gPlayerLookFiles_5_65[2] = {
#include "sub_main/00429210.inc"
};
u32 gPlayerLookFiles_5_67[2] = {
#include "sub_main/00429218.inc"
};
u32 gPlayerLookFiles_5_69[2] = {
#include "sub_main/00429220.inc"
};
u32 gPlayerLookFiles_5_71[2] = {
#include "sub_main/00429228.inc"
};
u32 gPlayerLookFiles_5_73[2] = {
#include "sub_main/00429230.inc"
};
u32 gPlayerLookFiles_5_75[2] = {
#include "sub_main/00429238.inc"
};
u32 gPlayerLookFiles_5_77[2] = {
#include "sub_main/00429240.inc"
};
u32 gPlayerLookFiles_5_79[2] = {
#include "sub_main/00429248.inc"
};
u32 gPlayerLookFiles_5_81[2] = {
#include "sub_main/00429250.inc"
};
u32 gPlayerLookFiles_5_83[2] = {
#include "sub_main/00429258.inc"
};
u32 gPlayerLookFiles_5_85[2] = {
#include "sub_main/00429260.inc"
};
u32 gPlayerLookFiles_5_87[2] = {
#include "sub_main/00429268.inc"
};
u32 gPlayerLookFiles_5_89[2] = {
#include "sub_main/00429270.inc"
};
u32 gPlayerLookFiles_5_91[2] = {
#include "sub_main/00429278.inc"
};
u32 gPlayerLookFiles_5_93[2] = {
#include "sub_main/00429280.inc"
};
u32 gPlayerLookFiles_5_95[2] = {
#include "sub_main/00429288.inc"
};
u32 gPlayerLookFiles_5_97[2] = {
#include "sub_main/00429290.inc"
};
u32 gPlayerLookFiles_5_99[2] = {
#include "sub_main/00429298.inc"
};
u32 gPlayerLookFiles_5_101[2] = {
#include "sub_main/004292A0.inc"
};
u32 gPlayerLookFiles_5_103[2] = {
#include "sub_main/004292A8.inc"
};
u32 gPlayerLookFiles_5_105[2] = {
#include "sub_main/004292B0.inc"
};
u32 gPlayerLookFiles_5_107[2] = {
#include "sub_main/004292B8.inc"
};
u32 gPlayerLookFiles_5_109[2] = {
#include "sub_main/004292C0.inc"
};
u32 gPlayerLookFiles_5_111[2] = {
#include "sub_main/004292C8.inc"
};
u32 gPlayerLookFiles_5_113[2] = {
#include "sub_main/004292D0.inc"
};
u32 gPlayerLookFiles_5_115[2] = {
#include "sub_main/004292D8.inc"
};
u32 gPlayerLookFiles_5_117[2] = {
#include "sub_main/004292E0.inc"
};
u32 gPlayerLookFiles_5_119[2] = {
#include "sub_main/004292E8.inc"
};
u32 gPlayerLookFiles_5_121[2] = {
#include "sub_main/004292F0.inc"
};
u32 gPlayerLookFiles_5_123[2] = {
#include "sub_main/004292F8.inc"
};
u32 gPlayerLookFiles_5_125[2] = {
#include "sub_main/00429300.inc"
};
u32 gPlayerLookFiles_5_127[2] = {
#include "sub_main/00429308.inc"
};
u32 gPlayerLookFiles_5_129[2] = {
#include "sub_main/00429310.inc"
};
u32 gPlayerLookFiles_5_131[2] = {
#include "sub_main/00429318.inc"
};
u32 gPlayerLookFiles_5_133[2] = {
#include "sub_main/00429320.inc"
};
u32 gPlayerLookFiles_5_135[2] = {
#include "sub_main/00429328.inc"
};
u32 gPlayerLookFiles_5_137[2] = {
#include "sub_main/00429330.inc"
};
u32 gPlayerLookFiles_5_139[2] = {
#include "sub_main/00429338.inc"
};
u32 gPlayerLookFiles_5_141[2] = {
#include "sub_main/00429340.inc"
};
u32 gPlayerLookFiles_5_143[2] = {
#include "sub_main/00429348.inc"
};
u32 gPlayerLookFiles_5_145[2] = {
#include "sub_main/00429350.inc"
};
u32 gPlayerLookFiles_5_147[2] = {
#include "sub_main/00429358.inc"
};
u32 gPlayerLookFiles_5_149[2] = {
#include "sub_main/00429360.inc"
};
u32 gPlayerLookFiles_5_151[2] = {
#include "sub_main/00429368.inc"
};
u32 gPlayerLookFiles_5_153[2] = {
#include "sub_main/00429370.inc"
};
u32 gPlayerLookFiles_5_155[2] = {
#include "sub_main/00429378.inc"
};
u32 gPlayerLookFiles_5_157[2] = {
#include "sub_main/00429380.inc"
};
u32 gPlayerLookFiles_5_159[2] = {
#include "sub_main/00429388.inc"
};
u32 gPlayerLookFiles_5_161[2] = {
#include "sub_main/00429390.inc"
};
u32 gPlayerLookFiles_5_163[2] = {
#include "sub_main/00429398.inc"
};
u32 gPlayerLookFiles_5_165[2] = {
#include "sub_main/004293A0.inc"
};
u32 gPlayerLookFiles_5_167[2] = {
#include "sub_main/004293A8.inc"
};
u32 gPlayerLookFiles_5_169[2] = {
#include "sub_main/004293B0.inc"
};
u32 gPlayerLookFiles_5_171[2] = {
#include "sub_main/004293B8.inc"
};
u32 gPlayerLookFiles_5_173[2] = {
#include "sub_main/004293C0.inc"
};
u32 gPlayerLookFiles_5_175[2] = {
#include "sub_main/004293C8.inc"
};
u32 gPlayerLookFiles_5_177[2] = {
#include "sub_main/004293D0.inc"
};
u32 gPlayerLookFiles_5_179[2] = {
#include "sub_main/004293D8.inc"
};
u32 gPlayerLookFiles_5_181[2] = {
#include "sub_main/004293E0.inc"
};
u32 gPlayerLookFiles_5_183[2] = {
#include "sub_main/004293E8.inc"
};
u32 gPlayerLookFiles_5_185[2] = {
#include "sub_main/004293F0.inc"
};
u32 gPlayerLookFiles_5_187[2] = {
#include "sub_main/004293F8.inc"
};
u32 gPlayerLookFiles_5_189[2] = {
#include "sub_main/00429400.inc"
};
u32 gPlayerLookFiles_5_191[2] = {
#include "sub_main/00429408.inc"
};
u32 gPlayerLookFiles_5_193[2] = {
#include "sub_main/00429410.inc"
};
u32 gPlayerLookFiles_5_195[2] = {
#include "sub_main/00429418.inc"
};
u32 gPlayerLookFiles_5_197[2] = {
#include "sub_main/00429420.inc"
};
u32 gPlayerLookFiles_5_199[2] = {
#include "sub_main/00429428.inc"
};
u32 gPlayerLookFiles_5_201[2] = {
#include "sub_main/00429430.inc"
};
u32 gPlayerLookFiles_5_203[2] = {
#include "sub_main/00429438.inc"
};
u32 gPlayerLookFiles_5_205[2] = {
#include "sub_main/00429440.inc"
};
u32 gPlayerLookFiles_5_207[2] = {
#include "sub_main/00429448.inc"
};
u32 gPlayerLookFiles_5_209[2] = {
#include "sub_main/00429450.inc"
};
u32 gPlayerLookFiles_5_211[2] = {
#include "sub_main/00429458.inc"
};
u32 gPlayerLookFiles_5_213[2] = {
#include "sub_main/00429460.inc"
};
u32 gPlayerLookFiles_5_215[2] = {
#include "sub_main/00429468.inc"
};
u32 gPlayerLookFiles_5_217[2] = {
#include "sub_main/00429470.inc"
};
u32 gPlayerLookFiles_5_219[2] = {
#include "sub_main/00429478.inc"
};
u32 gPlayerLookFiles_5_221[2] = {
#include "sub_main/00429480.inc"
};
u32 gPlayerLookFiles_5_223[2] = {
#include "sub_main/00429488.inc"
};
u32 gPlayerLookFiles_5_225[2] = {
#include "sub_main/00429490.inc"
};
u32 gPlayerLookFiles_5_227[2] = {
#include "sub_main/00429498.inc"
};
u32 gPlayerLookFiles_5_229[2] = {
#include "sub_main/004294A0.inc"
};
u32 gPlayerLookFiles_5_231[2] = {
#include "sub_main/004294A8.inc"
};
u32 gPlayerLookFiles_5_233[2] = {
#include "sub_main/004294B0.inc"
};
u32 gPlayerLookFiles_5_235[2] = {
#include "sub_main/004294B8.inc"
};
u32 gPlayerLookFiles_5_237[2] = {
#include "sub_main/004294C0.inc"
};
u32 gPlayerLookFiles_5_239[2] = {
#include "sub_main/004294C8.inc"
};
u32 gPlayerLookFiles_5_241[2] = {
#include "sub_main/004294D0.inc"
};
u32 gPlayerLookFiles_5_243[2] = {
#include "sub_main/004294D8.inc"
};
u32 gPlayerLookFiles_5_245[2] = {
#include "sub_main/004294E0.inc"
};
u32 gPlayerLookFiles_5_247[2] = {
#include "sub_main/004294E8.inc"
};
u32 gPlayerLookFiles_5_249[2] = {
#include "sub_main/004294F0.inc"
};
u32 gPlayerLookFiles_5_251[2] = {
#include "sub_main/004294F8.inc"
};
u32 gPlayerLookFiles_5_253[2] = {
#include "sub_main/00429500.inc"
};
u32 gPlayerLookFiles_5_255[2] = {
#include "sub_main/00429508.inc"
};
u32 gPlayerLookFiles_5_257[2] = {
#include "sub_main/00429510.inc"
};
u32 gPlayerLookFiles_5_259[2] = {
#include "sub_main/00429518.inc"
};
u32 gPlayerLookFiles_5_261[2] = {
#include "sub_main/00429520.inc"
};
u32 gPlayerLookFiles_5_263[2] = {
#include "sub_main/00429528.inc"
};
u32 gPlayerLookFiles_5_265[2] = {
#include "sub_main/00429530.inc"
};
u32 gPlayerLookFiles_5_267[2] = {
#include "sub_main/00429538.inc"
};
u32 gPlayerLookFiles_5_269[2] = {
#include "sub_main/00429540.inc"
};
u32 gPlayerLookFiles_5_271[2] = {
#include "sub_main/00429548.inc"
};
u32 gPlayerLookFiles_5_273[2] = {
#include "sub_main/00429550.inc"
};
u32 gPlayerLookFiles_5_275[2] = {
#include "sub_main/00429558.inc"
};
u32 gPlayerLookFiles_5_277[2] = {
#include "sub_main/00429560.inc"
};
u32 gPlayerLookFiles_5_279[2] = {
#include "sub_main/00429568.inc"
};
u32 gPlayerLookFiles_5_281[2] = {
#include "sub_main/00429570.inc"
};
u32 gPlayerLookFiles_5_283[2] = {
#include "sub_main/00429578.inc"
};
u32 gPlayerLookFiles_5_285[2] = {
#include "sub_main/00429580.inc"
};
u32 gPlayerLookFiles_5_287[2] = {
#include "sub_main/00429588.inc"
};
u32 gPlayerLookFiles_5_289[2] = {
#include "sub_main/00429590.inc"
};
u32 gPlayerLookFiles_5_291[2] = {
#include "sub_main/00429598.inc"
};
u32 gPlayerLookFiles_5_293[2] = {
#include "sub_main/004295A0.inc"
};
u32 gPlayerLookFiles_5_295[2] = {
#include "sub_main/004295A8.inc"
};
u32 gPlayerLookFiles_5_297[2] = {
#include "sub_main/004295B0.inc"
};
u32 gPlayerLookFiles_5_299[2] = {
#include "sub_main/004295B8.inc"
};
u32 gPlayerLookFiles_5_301[2] = {
#include "sub_main/004295C0.inc"
};
u32 gPlayerLookFiles_5_303[2] = {
#include "sub_main/004295C8.inc"
};
u32 gPlayerLookFiles_5_305[2] = {
#include "sub_main/004295D0.inc"
};
u32 gPlayerLookFiles_5_307[2] = {
#include "sub_main/004295D8.inc"
};
u32 gPlayerLookFiles_5_309[2] = {
#include "sub_main/004295E0.inc"
};
u32 gPlayerLookFiles_5_311[2] = {
#include "sub_main/004295E8.inc"
};
u32 gPlayerLookFiles_5_313[2] = {
#include "sub_main/004295F0.inc"
};
u32 gPlayerLookFiles_5_315[2] = {
#include "sub_main/004295F8.inc"
};
u32 gPlayerLookFiles_5_317[2] = {
#include "sub_main/00429600.inc"
};
u32 gPlayerLookFiles_5_319[2] = {
#include "sub_main/00429608.inc"
};
u32 gPlayerLookFiles_5_321[2] = {
#include "sub_main/00429610.inc"
};
u32 gPlayerLookFiles_5_323[2] = {
#include "sub_main/00429618.inc"
};
u32 gPlayerLookFiles_5_325[2] = {
#include "sub_main/00429620.inc"
};
u32 gPlayerLookFiles_5_327[2] = {
#include "sub_main/00429628.inc"
};
u32 gPlayerLookFiles_5_329[2] = {
#include "sub_main/00429630.inc"
};
u32 gPlayerLookFiles_5_331[2] = {
#include "sub_main/00429638.inc"
};
u32 gPlayerLookFiles_5_333[2] = {
#include "sub_main/00429640.inc"
};
u32 gPlayerLookFiles_5_335[2] = {
#include "sub_main/00429648.inc"
};
u32 gPlayerLookFiles_5_337[2] = {
#include "sub_main/00429650.inc"
};
u32 gPlayerLookFiles_5_339[2] = {
#include "sub_main/00429658.inc"
};
u32 gPlayerLookFiles_5_341[2] = {
#include "sub_main/00429660.inc"
};
u32 gPlayerLookFiles_5_343[2] = {
#include "sub_main/00429668.inc"
};
u32 gPlayerLookFiles_5_345[2] = {
#include "sub_main/00429670.inc"
};
u32 gPlayerLookFiles_5_347[2] = {
#include "sub_main/00429678.inc"
};
u32 gPlayerLookFiles_5_349[2] = {
#include "sub_main/00429680.inc"
};
u32 gPlayerLookFiles_5_351[2] = {
#include "sub_main/00429688.inc"
};
u32 gPlayerLookFiles_5_353[2] = {
#include "sub_main/00429690.inc"
};
u32 gPlayerLookFiles_5_355[2] = {
#include "sub_main/00429698.inc"
};
u32 gPlayerLookFiles_5_357[2] = {
#include "sub_main/004296A0.inc"
};
u32 gPlayerLookFiles_5_359[2] = {
#include "sub_main/004296A8.inc"
};
u32 gPlayerLookFiles_5_361[2] = {
#include "sub_main/004296B0.inc"
};
u32 gPlayerLookFiles_5_363[2] = {
#include "sub_main/004296B8.inc"
};
u32 gPlayerLookFiles_5_365[2] = {
#include "sub_main/004296C0.inc"
};
u32 gPlayerLookFiles_5_367[2] = {
#include "sub_main/004296C8.inc"
};
u32 gPlayerLookFiles_5_369[2] = {
#include "sub_main/004296D0.inc"
};
u32 gPlayerLookFiles_5_371[2] = {
#include "sub_main/004296D8.inc"
};
u32 gPlayerLookFiles_5_373[2] = {
#include "sub_main/004296E0.inc"
};
u32 gPlayerLookFiles_5_375[2] = {
#include "sub_main/004296E8.inc"
};
u32 gPlayerLookFiles_5_377[2] = {
#include "sub_main/004296F0.inc"
};
u32 gPlayerLookFiles_5_379[2] = {
#include "sub_main/004296F8.inc"
};
u32 gPlayerLookFiles_5_381[2] = {
#include "sub_main/00429700.inc"
};
u32 gPlayerLookFiles_5_383[2] = {
#include "sub_main/00429708.inc"
};
u32 gPlayerLookFiles_5_385[2] = {
#include "sub_main/00429710.inc"
};
u32 gPlayerLookFiles_5_387[2] = {
#include "sub_main/00429718.inc"
};
u32 gPlayerLookFiles_5_389[2] = {
#include "sub_main/00429720.inc"
};
u32 gPlayerLookFiles_5_391[2] = {
#include "sub_main/00429728.inc"
};
u32 gPlayerLookFiles_5_393[2] = {
#include "sub_main/00429730.inc"
};
u32 gPlayerLookFiles_5_395[2] = {
#include "sub_main/00429738.inc"
};
u32 gPlayerLookFiles_5_397[2] = {
#include "sub_main/00429740.inc"
};
u32 gPlayerLookFiles_5_399[2] = {
#include "sub_main/00429748.inc"
};
u32 gPlayerLookFiles_5_401[2] = {
#include "sub_main/00429750.inc"
};
u32 gPlayerLookFiles_5_403[2] = {
#include "sub_main/00429758.inc"
};
u32 gPlayerLookFiles_5_405[2] = {
#include "sub_main/00429760.inc"
};
u32 gPlayerLookFiles_5_407[2] = {
#include "sub_main/00429768.inc"
};
u32 gPlayerLookFiles_5_409[2] = {
#include "sub_main/00429770.inc"
};
u32 gPlayerLookFiles_5_411[2] = {
#include "sub_main/00429778.inc"
};
u32 gPlayerLookFiles_5_413[4] = {
#include "sub_main/00429780.inc"
};
u32 gPlayerLookFiles_5[414] = {
#include "sub_main/00429790.inc"
};
u32 gPlayerLookFiles_6_1[2] = {
#include "sub_main/00429E08.inc"
};
u32 gPlayerLookFiles_6_3[2] = {
#include "sub_main/00429E10.inc"
};
u32 gPlayerLookFiles_6_5[2] = {
#include "sub_main/00429E18.inc"
};
u32 gPlayerLookFiles_6_7[2] = {
#include "sub_main/00429E20.inc"
};
u32 gPlayerLookFiles_6_9[2] = {
#include "sub_main/00429E28.inc"
};
u32 gPlayerLookFiles_6_11[2] = {
#include "sub_main/00429E30.inc"
};
u32 gPlayerLookFiles_6_13[2] = {
#include "sub_main/00429E38.inc"
};
u32 gPlayerLookFiles_6_15[2] = {
#include "sub_main/00429E40.inc"
};
u32 gPlayerLookFiles_6_17[2] = {
#include "sub_main/00429E48.inc"
};
u32 gPlayerLookFiles_6_19[2] = {
#include "sub_main/00429E50.inc"
};
u32 gPlayerLookFiles_6_21[2] = {
#include "sub_main/00429E58.inc"
};
u32 gPlayerLookFiles_6_23[2] = {
#include "sub_main/00429E60.inc"
};
u32 gPlayerLookFiles_6_25[2] = {
#include "sub_main/00429E68.inc"
};
u32 gPlayerLookFiles_6_27[2] = {
#include "sub_main/00429E70.inc"
};
u32 gPlayerLookFiles_6_29[2] = {
#include "sub_main/00429E78.inc"
};
u32 gPlayerLookFiles_6_31[2] = {
#include "sub_main/00429E80.inc"
};
u32 gPlayerLookFiles_6_33[2] = {
#include "sub_main/00429E88.inc"
};
u32 gPlayerLookFiles_6_35[2] = {
#include "sub_main/00429E90.inc"
};
u32 gPlayerLookFiles_6_37[2] = {
#include "sub_main/00429E98.inc"
};
u32 gPlayerLookFiles_6_39[2] = {
#include "sub_main/00429EA0.inc"
};
u32 gPlayerLookFiles_6_41[2] = {
#include "sub_main/00429EA8.inc"
};
u32 gPlayerLookFiles_6[42] = {
#include "sub_main/00429EB0.inc"
};
u32 gPlayerLookFiles_7_1[2] = {
#include "sub_main/00429F58.inc"
};
u32 gPlayerLookFiles_7_3[2] = {
#include "sub_main/00429F60.inc"
};
u32 gPlayerLookFiles_7_5[2] = {
#include "sub_main/00429F68.inc"
};
u32 gPlayerLookFiles_7_7[2] = {
#include "sub_main/00429F70.inc"
};
u32 gPlayerLookFiles_7_9[2] = {
#include "sub_main/00429F78.inc"
};
u32 gPlayerLookFiles_7_11[2] = {
#include "sub_main/00429F80.inc"
};
u32 gPlayerLookFiles_7_13[2] = {
#include "sub_main/00429F88.inc"
};
u32 gPlayerLookFiles_7_15[2] = {
#include "sub_main/00429F90.inc"
};
u32 gPlayerLookFiles_7_17[2] = {
#include "sub_main/00429F98.inc"
};
u32 gPlayerLookFiles_7_19[2] = {
#include "sub_main/00429FA0.inc"
};
u32 gPlayerLookFiles_7_21[2] = {
#include "sub_main/00429FA8.inc"
};
u32 gPlayerLookFiles_7_23[2] = {
#include "sub_main/00429FB0.inc"
};
u32 gPlayerLookFiles_7_25[2] = {
#include "sub_main/00429FB8.inc"
};
u32 gPlayerLookFiles_7_27[2] = {
#include "sub_main/00429FC0.inc"
};
u32 gPlayerLookFiles_7_29[2] = {
#include "sub_main/00429FC8.inc"
};
u32 gPlayerLookFiles_7_31[2] = {
#include "sub_main/00429FD0.inc"
};
u32 gPlayerLookFiles_7_33[2] = {
#include "sub_main/00429FD8.inc"
};
u32 gPlayerLookFiles_7_35[2] = {
#include "sub_main/00429FE0.inc"
};
u32 gPlayerLookFiles_7_37[2] = {
#include "sub_main/00429FE8.inc"
};
u32 gPlayerLookFiles_7_39[2] = {
#include "sub_main/00429FF0.inc"
};
u32 gPlayerLookFiles_7_41[2] = {
#include "sub_main/00429FF8.inc"
};
u32 gPlayerLookFiles_7_43[2] = {
#include "sub_main/0042A000.inc"
};
u32 gPlayerLookFiles_7_45[2] = {
#include "sub_main/0042A008.inc"
};
u32 gPlayerLookFiles_7_47[2] = {
#include "sub_main/0042A010.inc"
};
u32 gPlayerLookFiles_7_49[2] = {
#include "sub_main/0042A018.inc"
};
u32 gPlayerLookFiles_7_51[2] = {
#include "sub_main/0042A020.inc"
};
u32 gPlayerLookFiles_7_53[2] = {
#include "sub_main/0042A028.inc"
};
u32 gPlayerLookFiles_7_55[2] = {
#include "sub_main/0042A030.inc"
};
u32 gPlayerLookFiles_7_57[2] = {
#include "sub_main/0042A038.inc"
};
u32 gPlayerLookFiles_7_59[2] = {
#include "sub_main/0042A040.inc"
};
u32 gPlayerLookFiles_7_61[2] = {
#include "sub_main/0042A048.inc"
};
u32 gPlayerLookFiles_7_63[2] = {
#include "sub_main/0042A050.inc"
};
u32 gPlayerLookFiles_7_65[2] = {
#include "sub_main/0042A058.inc"
};
u32 gPlayerLookFiles_7_67[2] = {
#include "sub_main/0042A060.inc"
};
u32 gPlayerLookFiles_7_69[2] = {
#include "sub_main/0042A068.inc"
};
u32 gPlayerLookFiles_7_71[2] = {
#include "sub_main/0042A070.inc"
};
u32 gPlayerLookFiles_7_73[2] = {
#include "sub_main/0042A078.inc"
};
u32 gPlayerLookFiles_7_75[2] = {
#include "sub_main/0042A080.inc"
};
u32 gPlayerLookFiles_7_77[2] = {
#include "sub_main/0042A088.inc"
};
u32 gPlayerLookFiles_7_79[2] = {
#include "sub_main/0042A090.inc"
};
u32 gPlayerLookFiles_7_81[2] = {
#include "sub_main/0042A098.inc"
};
u32 gPlayerLookFiles_7_83[2] = {
#include "sub_main/0042A0A0.inc"
};
u32 gPlayerLookFiles_7_85[2] = {
#include "sub_main/0042A0A8.inc"
};
u32 gPlayerLookFiles_7_87[2] = {
#include "sub_main/0042A0B0.inc"
};
u32 gPlayerLookFiles_7_89[2] = {
#include "sub_main/0042A0B8.inc"
};
u32 gPlayerLookFiles_7_91[2] = {
#include "sub_main/0042A0C0.inc"
};
u32 gPlayerLookFiles_7_93[2] = {
#include "sub_main/0042A0C8.inc"
};
u32 gPlayerLookFiles_7_95[2] = {
#include "sub_main/0042A0D0.inc"
};
u32 gPlayerLookFiles_7_97[2] = {
#include "sub_main/0042A0D8.inc"
};
u32 gPlayerLookFiles_7_99[2] = {
#include "sub_main/0042A0E0.inc"
};
u32 gPlayerLookFiles_7_101[2] = {
#include "sub_main/0042A0E8.inc"
};
u32 gPlayerLookFiles_7_103[2] = {
#include "sub_main/0042A0F0.inc"
};
u32 gPlayerLookFiles_7_105[2] = {
#include "sub_main/0042A0F8.inc"
};
u32 gPlayerLookFiles_7_107[2] = {
#include "sub_main/0042A100.inc"
};
u32 gPlayerLookFiles_7_109[2] = {
#include "sub_main/0042A108.inc"
};
u32 gPlayerLookFiles_7_111[2] = {
#include "sub_main/0042A110.inc"
};
u32 gPlayerLookFiles_7_113[2] = {
#include "sub_main/0042A118.inc"
};
u32 gPlayerLookFiles_7_115[2] = {
#include "sub_main/0042A120.inc"
};
u32 gPlayerLookFiles_7_117[2] = {
#include "sub_main/0042A128.inc"
};
u32 gPlayerLookFiles_7_119[2] = {
#include "sub_main/0042A130.inc"
};
u32 gPlayerLookFiles_7_121[2] = {
#include "sub_main/0042A138.inc"
};
u32 gPlayerLookFiles_7_123[2] = {
#include "sub_main/0042A140.inc"
};
u32 gPlayerLookFiles_7_125[2] = {
#include "sub_main/0042A148.inc"
};
u32 gPlayerLookFiles_7_127[2] = {
#include "sub_main/0042A150.inc"
};
u32 gPlayerLookFiles_7_129[2] = {
#include "sub_main/0042A158.inc"
};
u32 gPlayerLookFiles_7_131[2] = {
#include "sub_main/0042A160.inc"
};
u32 gPlayerLookFiles_7_133[2] = {
#include "sub_main/0042A168.inc"
};
u32 gPlayerLookFiles_7_135[2] = {
#include "sub_main/0042A170.inc"
};
u32 gPlayerLookFiles_7_137[2] = {
#include "sub_main/0042A178.inc"
};
u32 gPlayerLookFiles_7_139[2] = {
#include "sub_main/0042A180.inc"
};
u32 gPlayerLookFiles_7_141[2] = {
#include "sub_main/0042A188.inc"
};
u32 gPlayerLookFiles_7_143[2] = {
#include "sub_main/0042A190.inc"
};
u32 gPlayerLookFiles_7_145[2] = {
#include "sub_main/0042A198.inc"
};
u32 gPlayerLookFiles_7_147[2] = {
#include "sub_main/0042A1A0.inc"
};
u32 gPlayerLookFiles_7_149[2] = {
#include "sub_main/0042A1A8.inc"
};
u32 gPlayerLookFiles_7_151[2] = {
#include "sub_main/0042A1B0.inc"
};
u32 gPlayerLookFiles_7_153[2] = {
#include "sub_main/0042A1B8.inc"
};
u32 gPlayerLookFiles_7_155[2] = {
#include "sub_main/0042A1C0.inc"
};
u32 gPlayerLookFiles_7_157[2] = {
#include "sub_main/0042A1C8.inc"
};
u32 gPlayerLookFiles_7_159[2] = {
#include "sub_main/0042A1D0.inc"
};
u32 gPlayerLookFiles_7_161[2] = {
#include "sub_main/0042A1D8.inc"
};
u32 gPlayerLookFiles_7_163[2] = {
#include "sub_main/0042A1E0.inc"
};
u32 gPlayerLookFiles_7_165[2] = {
#include "sub_main/0042A1E8.inc"
};
u32 gPlayerLookFiles_7_167[2] = {
#include "sub_main/0042A1F0.inc"
};
u32 gPlayerLookFiles_7_169[2] = {
#include "sub_main/0042A1F8.inc"
};
u32 gPlayerLookFiles_7_171[2] = {
#include "sub_main/0042A200.inc"
};
u32 gPlayerLookFiles_7_173[2] = {
#include "sub_main/0042A208.inc"
};
u32 gPlayerLookFiles_7_175[2] = {
#include "sub_main/0042A210.inc"
};
u32 gPlayerLookFiles_7_177[2] = {
#include "sub_main/0042A218.inc"
};
u32 gPlayerLookFiles_7_179[2] = {
#include "sub_main/0042A220.inc"
};
u32 gPlayerLookFiles_7_181[2] = {
#include "sub_main/0042A228.inc"
};
u32 gPlayerLookFiles_7_183[2] = {
#include "sub_main/0042A230.inc"
};
u32 gPlayerLookFiles_7_185[2] = {
#include "sub_main/0042A238.inc"
};
u32 gPlayerLookFiles_7_187[2] = {
#include "sub_main/0042A240.inc"
};
u32 gPlayerLookFiles_7_189[2] = {
#include "sub_main/0042A248.inc"
};
u32 gPlayerLookFiles_7_191[2] = {
#include "sub_main/0042A250.inc"
};
u32 gPlayerLookFiles_7_193[2] = {
#include "sub_main/0042A258.inc"
};
u32 gPlayerLookFiles_7_195[2] = {
#include "sub_main/0042A260.inc"
};
u32 gPlayerLookFiles_7_197[2] = {
#include "sub_main/0042A268.inc"
};
u32 gPlayerLookFiles_7_199[2] = {
#include "sub_main/0042A270.inc"
};
u32 gPlayerLookFiles_7_201[2] = {
#include "sub_main/0042A278.inc"
};
u32 gPlayerLookFiles_7_203[2] = {
#include "sub_main/0042A280.inc"
};
u32 gPlayerLookFiles_7_205[2] = {
#include "sub_main/0042A288.inc"
};
u32 gPlayerLookFiles_7_207[2] = {
#include "sub_main/0042A290.inc"
};
u32 gPlayerLookFiles_7_209[2] = {
#include "sub_main/0042A298.inc"
};
u32 gPlayerLookFiles_7_211[2] = {
#include "sub_main/0042A2A0.inc"
};
u32 gPlayerLookFiles_7_213[2] = {
#include "sub_main/0042A2A8.inc"
};
u32 gPlayerLookFiles_7_215[2] = {
#include "sub_main/0042A2B0.inc"
};
u32 gPlayerLookFiles_7_217[2] = {
#include "sub_main/0042A2B8.inc"
};
u32 gPlayerLookFiles_7_219[2] = {
#include "sub_main/0042A2C0.inc"
};
u32 gPlayerLookFiles_7_221[2] = {
#include "sub_main/0042A2C8.inc"
};
u32 gPlayerLookFiles_7_223[2] = {
#include "sub_main/0042A2D0.inc"
};
u32 gPlayerLookFiles_7_225[2] = {
#include "sub_main/0042A2D8.inc"
};
u32 gPlayerLookFiles_7_227[2] = {
#include "sub_main/0042A2E0.inc"
};
u32 gPlayerLookFiles_7_229[2] = {
#include "sub_main/0042A2E8.inc"
};
u32 gPlayerLookFiles_7_231[2] = {
#include "sub_main/0042A2F0.inc"
};
u32 gPlayerLookFiles_7_233[2] = {
#include "sub_main/0042A2F8.inc"
};
u32 gPlayerLookFiles_7_235[2] = {
#include "sub_main/0042A300.inc"
};
u32 gPlayerLookFiles_7_237[2] = {
#include "sub_main/0042A308.inc"
};
u32 gPlayerLookFiles_7_239[2] = {
#include "sub_main/0042A310.inc"
};
u32 gPlayerLookFiles_7_241[2] = {
#include "sub_main/0042A318.inc"
};
u32 gPlayerLookFiles_7_243[2] = {
#include "sub_main/0042A320.inc"
};
u32 gPlayerLookFiles_7_245[2] = {
#include "sub_main/0042A328.inc"
};
u32 gPlayerLookFiles_7_247[2] = {
#include "sub_main/0042A330.inc"
};
u32 gPlayerLookFiles_7_249[2] = {
#include "sub_main/0042A338.inc"
};
u32 gPlayerLookFiles_7_251[2] = {
#include "sub_main/0042A340.inc"
};
u32 gPlayerLookFiles_7_253[2] = {
#include "sub_main/0042A348.inc"
};
u32 gPlayerLookFiles_7_255[2] = {
#include "sub_main/0042A350.inc"
};
u32 gPlayerLookFiles_7_257[2] = {
#include "sub_main/0042A358.inc"
};
u32 gPlayerLookFiles_7_259[2] = {
#include "sub_main/0042A360.inc"
};
u32 gPlayerLookFiles_7_261[2] = {
#include "sub_main/0042A368.inc"
};
u32 gPlayerLookFiles_7_263[2] = {
#include "sub_main/0042A370.inc"
};
u32 gPlayerLookFiles_7_265[2] = {
#include "sub_main/0042A378.inc"
};
u32 gPlayerLookFiles_7_267[2] = {
#include "sub_main/0042A380.inc"
};
u32 gPlayerLookFiles_7_269[2] = {
#include "sub_main/0042A388.inc"
};
u32 gPlayerLookFiles_7_271[2] = {
#include "sub_main/0042A390.inc"
};
u32 gPlayerLookFiles_7_273[2] = {
#include "sub_main/0042A398.inc"
};
u32 gPlayerLookFiles_7_275[2] = {
#include "sub_main/0042A3A0.inc"
};
u32 gPlayerLookFiles_7_277[2] = {
#include "sub_main/0042A3A8.inc"
};
u32 gPlayerLookFiles_7_279[2] = {
#include "sub_main/0042A3B0.inc"
};
u32 gPlayerLookFiles_7_281[2] = {
#include "sub_main/0042A3B8.inc"
};
u32 gPlayerLookFiles_7_283[2] = {
#include "sub_main/0042A3C0.inc"
};
u32 gPlayerLookFiles_7_285[2] = {
#include "sub_main/0042A3C8.inc"
};
u32 gPlayerLookFiles_7_287[2] = {
#include "sub_main/0042A3D0.inc"
};
u32 gPlayerLookFiles_7_289[2] = {
#include "sub_main/0042A3D8.inc"
};
u32 gPlayerLookFiles_7_291[2] = {
#include "sub_main/0042A3E0.inc"
};
u32 gPlayerLookFiles_7_293[2] = {
#include "sub_main/0042A3E8.inc"
};
u32 gPlayerLookFiles_7_295[2] = {
#include "sub_main/0042A3F0.inc"
};
u32 gPlayerLookFiles_7_297[2] = {
#include "sub_main/0042A3F8.inc"
};
u32 gPlayerLookFiles_7_299[2] = {
#include "sub_main/0042A400.inc"
};
u32 gPlayerLookFiles_7_301[2] = {
#include "sub_main/0042A408.inc"
};
u32 gPlayerLookFiles_7_303[2] = {
#include "sub_main/0042A410.inc"
};
u32 gPlayerLookFiles_7_305[2] = {
#include "sub_main/0042A418.inc"
};
u32 gPlayerLookFiles_7_307[2] = {
#include "sub_main/0042A420.inc"
};
u32 gPlayerLookFiles_7_309[2] = {
#include "sub_main/0042A428.inc"
};
u32 gPlayerLookFiles_7_311[2] = {
#include "sub_main/0042A430.inc"
};
u32 gPlayerLookFiles_7_313[2] = {
#include "sub_main/0042A438.inc"
};
u32 gPlayerLookFiles_7_315[2] = {
#include "sub_main/0042A440.inc"
};
u32 gPlayerLookFiles_7_317[2] = {
#include "sub_main/0042A448.inc"
};
u32 gPlayerLookFiles_7_319[2] = {
#include "sub_main/0042A450.inc"
};
u32 gPlayerLookFiles_7_321[2] = {
#include "sub_main/0042A458.inc"
};
u32 gPlayerLookFiles_7_323[2] = {
#include "sub_main/0042A460.inc"
};
u32 gPlayerLookFiles_7_325[2] = {
#include "sub_main/0042A468.inc"
};
u32 gPlayerLookFiles_7_327[2] = {
#include "sub_main/0042A470.inc"
};
u32 gPlayerLookFiles_7_329[2] = {
#include "sub_main/0042A478.inc"
};
u32 gPlayerLookFiles_7_331[2] = {
#include "sub_main/0042A480.inc"
};
u32 gPlayerLookFiles_7_333[2] = {
#include "sub_main/0042A488.inc"
};
u32 gPlayerLookFiles_7_335[2] = {
#include "sub_main/0042A490.inc"
};
u32 gPlayerLookFiles_7_337[2] = {
#include "sub_main/0042A498.inc"
};
u32 gPlayerLookFiles_7_339[2] = {
#include "sub_main/0042A4A0.inc"
};
u32 gPlayerLookFiles_7_341[2] = {
#include "sub_main/0042A4A8.inc"
};
u32 gPlayerLookFiles_7_343[2] = {
#include "sub_main/0042A4B0.inc"
};
u32 gPlayerLookFiles_7_345[2] = {
#include "sub_main/0042A4B8.inc"
};
u32 gPlayerLookFiles_7_347[2] = {
#include "sub_main/0042A4C0.inc"
};
u32 gPlayerLookFiles_7_349[2] = {
#include "sub_main/0042A4C8.inc"
};
u32 gPlayerLookFiles_7_351[2] = {
#include "sub_main/0042A4D0.inc"
};
u32 gPlayerLookFiles_7_353[2] = {
#include "sub_main/0042A4D8.inc"
};
u32 gPlayerLookFiles_7_355[2] = {
#include "sub_main/0042A4E0.inc"
};
u32 gPlayerLookFiles_7_357[2] = {
#include "sub_main/0042A4E8.inc"
};
u32 gPlayerLookFiles_7_359[2] = {
#include "sub_main/0042A4F0.inc"
};
u32 gPlayerLookFiles_7_361[2] = {
#include "sub_main/0042A4F8.inc"
};
u32 gPlayerLookFiles_7_363[2] = {
#include "sub_main/0042A500.inc"
};
u32 gPlayerLookFiles_7_365[2] = {
#include "sub_main/0042A508.inc"
};
u32 gPlayerLookFiles_7_367[2] = {
#include "sub_main/0042A510.inc"
};
u32 gPlayerLookFiles_7_369[2] = {
#include "sub_main/0042A518.inc"
};
u32 gPlayerLookFiles_7_371[2] = {
#include "sub_main/0042A520.inc"
};
u32 gPlayerLookFiles_7_373[2] = {
#include "sub_main/0042A528.inc"
};
u32 gPlayerLookFiles_7_375[2] = {
#include "sub_main/0042A530.inc"
};
u32 gPlayerLookFiles_7_377[2] = {
#include "sub_main/0042A538.inc"
};
u32 gPlayerLookFiles_7_379[2] = {
#include "sub_main/0042A540.inc"
};
u32 gPlayerLookFiles_7_381[2] = {
#include "sub_main/0042A548.inc"
};
u32 gPlayerLookFiles_7_383[2] = {
#include "sub_main/0042A550.inc"
};
u32 gPlayerLookFiles_7_385[2] = {
#include "sub_main/0042A558.inc"
};
u32 gPlayerLookFiles_7_387[2] = {
#include "sub_main/0042A560.inc"
};
u32 gPlayerLookFiles_7_389[2] = {
#include "sub_main/0042A568.inc"
};
u32 gPlayerLookFiles_7_391[2] = {
#include "sub_main/0042A570.inc"
};
u32 gPlayerLookFiles_7_393[2] = {
#include "sub_main/0042A578.inc"
};
u32 gPlayerLookFiles_7_395[2] = {
#include "sub_main/0042A580.inc"
};
u32 gPlayerLookFiles_7_397[2] = {
#include "sub_main/0042A588.inc"
};
u32 gPlayerLookFiles_7_399[4] = {
#include "sub_main/0042A590.inc"
};
u32 gPlayerLookFiles_7[400] = {
#include "sub_main/0042A5A0.inc"
};
u32 gPlayerLookFiles_8_1[2] = {
#include "sub_main/0042ABE0.inc"
};
u32 gPlayerLookFiles_8_3[2] = {
#include "sub_main/0042ABE8.inc"
};
u32 gPlayerLookFiles_8_5[2] = {
#include "sub_main/0042ABF0.inc"
};
u32 gPlayerLookFiles_8_7[2] = {
#include "sub_main/0042ABF8.inc"
};
u32 gPlayerLookFiles_8_9[2] = {
#include "sub_main/0042AC00.inc"
};
u32 gPlayerLookFiles_8_11[2] = {
#include "sub_main/0042AC08.inc"
};
u32 gPlayerLookFiles_8_13[2] = {
#include "sub_main/0042AC10.inc"
};
u32 gPlayerLookFiles_8_15[2] = {
#include "sub_main/0042AC18.inc"
};
u32 gPlayerLookFiles_8_17[2] = {
#include "sub_main/0042AC20.inc"
};
u32 gPlayerLookFiles_8_19[2] = {
#include "sub_main/0042AC28.inc"
};
u32 gPlayerLookFiles_8_21[2] = {
#include "sub_main/0042AC30.inc"
};
u32 gPlayerLookFiles_8_23[2] = {
#include "sub_main/0042AC38.inc"
};
u32 gPlayerLookFiles_8_25[2] = {
#include "sub_main/0042AC40.inc"
};
u32 gPlayerLookFiles_8_27[2] = {
#include "sub_main/0042AC48.inc"
};
u32 gPlayerLookFiles_8_29[2] = {
#include "sub_main/0042AC50.inc"
};
u32 gPlayerLookFiles_8_31[2] = {
#include "sub_main/0042AC58.inc"
};
u32 gPlayerLookFiles_8_33[2] = {
#include "sub_main/0042AC60.inc"
};
u32 gPlayerLookFiles_8_35[2] = {
#include "sub_main/0042AC68.inc"
};
u32 gPlayerLookFiles_8_37[2] = {
#include "sub_main/0042AC70.inc"
};
u32 gPlayerLookFiles_8_39[2] = {
#include "sub_main/0042AC78.inc"
};
u32 gPlayerLookFiles_8_41[2] = {
#include "sub_main/0042AC80.inc"
};
u32 gPlayerLookFiles_8_43[2] = {
#include "sub_main/0042AC88.inc"
};
u32 gPlayerLookFiles_8_45[2] = {
#include "sub_main/0042AC90.inc"
};
u32 gPlayerLookFiles_8_47[2] = {
#include "sub_main/0042AC98.inc"
};
u32 gPlayerLookFiles_8_49[2] = {
#include "sub_main/0042ACA0.inc"
};
u32 gPlayerLookFiles_8_51[2] = {
#include "sub_main/0042ACA8.inc"
};
u32 gPlayerLookFiles_8_53[4] = {
#include "sub_main/0042ACB0.inc"
};
u32 gPlayerLookFiles_8[54] = {
#include "sub_main/0042ACC0.inc"
};
u32 gPlayerLookFiles_9_1[2] = {
#include "sub_main/0042AD98.inc"
};
u32 gPlayerLookFiles_9_3[2] = {
#include "sub_main/0042ADA0.inc"
};
u32 gPlayerLookFiles_9_5[2] = {
#include "sub_main/0042ADA8.inc"
};
u32 gPlayerLookFiles_9_7[2] = {
#include "sub_main/0042ADB0.inc"
};
u32 gPlayerLookFiles_9_9[2] = {
#include "sub_main/0042ADB8.inc"
};
u32 gPlayerLookFiles_9_11[2] = {
#include "sub_main/0042ADC0.inc"
};
u32 gPlayerLookFiles_9_13[2] = {
#include "sub_main/0042ADC8.inc"
};
u32 gPlayerLookFiles_9_15[2] = {
#include "sub_main/0042ADD0.inc"
};
u32 gPlayerLookFiles_9_17[2] = {
#include "sub_main/0042ADD8.inc"
};
u32 gPlayerLookFiles_9_19[2] = {
#include "sub_main/0042ADE0.inc"
};
u32 gPlayerLookFiles_9_21[2] = {
#include "sub_main/0042ADE8.inc"
};
u32 gPlayerLookFiles_9_23[2] = {
#include "sub_main/0042ADF0.inc"
};
u32 gPlayerLookFiles_9_25[2] = {
#include "sub_main/0042ADF8.inc"
};
u32 gPlayerLookFiles_9_27[2] = {
#include "sub_main/0042AE00.inc"
};
u32 gPlayerLookFiles_9_29[2] = {
#include "sub_main/0042AE08.inc"
};
u32 gPlayerLookFiles_9_31[2] = {
#include "sub_main/0042AE10.inc"
};
u32 gPlayerLookFiles_9_33[2] = {
#include "sub_main/0042AE18.inc"
};
u32 gPlayerLookFiles_9_35[2] = {
#include "sub_main/0042AE20.inc"
};
u32 gPlayerLookFiles_9_37[2] = {
#include "sub_main/0042AE28.inc"
};
u32 gPlayerLookFiles_9_39[2] = {
#include "sub_main/0042AE30.inc"
};
u32 gPlayerLookFiles_9_41[2] = {
#include "sub_main/0042AE38.inc"
};
u32 gPlayerLookFiles_9_43[2] = {
#include "sub_main/0042AE40.inc"
};
u32 gPlayerLookFiles_9_45[2] = {
#include "sub_main/0042AE48.inc"
};
u32 gPlayerLookFiles_9_47[2] = {
#include "sub_main/0042AE50.inc"
};
u32 gPlayerLookFiles_9_49[2] = {
#include "sub_main/0042AE58.inc"
};
u32 gPlayerLookFiles_9_51[2] = {
#include "sub_main/0042AE60.inc"
};
u32 gPlayerLookFiles_9_53[2] = {
#include "sub_main/0042AE68.inc"
};
u32 gPlayerLookFiles_9_55[2] = {
#include "sub_main/0042AE70.inc"
};
u32 gPlayerLookFiles_9_57[2] = {
#include "sub_main/0042AE78.inc"
};
u32 gPlayerLookFiles_9_59[2] = {
#include "sub_main/0042AE80.inc"
};
u32 gPlayerLookFiles_9_61[2] = {
#include "sub_main/0042AE88.inc"
};
u32 gPlayerLookFiles_9_63[2] = {
#include "sub_main/0042AE90.inc"
};
u32 gPlayerLookFiles_9_65[2] = {
#include "sub_main/0042AE98.inc"
};
u32 gPlayerLookFiles_9_67[2] = {
#include "sub_main/0042AEA0.inc"
};
u32 gPlayerLookFiles_9_69[2] = {
#include "sub_main/0042AEA8.inc"
};
u32 gPlayerLookFiles_9_71[2] = {
#include "sub_main/0042AEB0.inc"
};
u32 gPlayerLookFiles_9_73[2] = {
#include "sub_main/0042AEB8.inc"
};
u32 gPlayerLookFiles_9_75[2] = {
#include "sub_main/0042AEC0.inc"
};
u32 gPlayerLookFiles_9_77[2] = {
#include "sub_main/0042AEC8.inc"
};
u32 gPlayerLookFiles_9_79[2] = {
#include "sub_main/0042AED0.inc"
};
u32 gPlayerLookFiles_9_81[2] = {
#include "sub_main/0042AED8.inc"
};
u32 gPlayerLookFiles_9_83[2] = {
#include "sub_main/0042AEE0.inc"
};
u32 gPlayerLookFiles_9_85[2] = {
#include "sub_main/0042AEE8.inc"
};
u32 gPlayerLookFiles_9_87[2] = {
#include "sub_main/0042AEF0.inc"
};
u32 gPlayerLookFiles_9_89[2] = {
#include "sub_main/0042AEF8.inc"
};
u32 gPlayerLookFiles_9_91[2] = {
#include "sub_main/0042AF00.inc"
};
u32 gPlayerLookFiles_9_93[2] = {
#include "sub_main/0042AF08.inc"
};
u32 gPlayerLookFiles_9_95[2] = {
#include "sub_main/0042AF10.inc"
};
u32 gPlayerLookFiles_9_97[2] = {
#include "sub_main/0042AF18.inc"
};
u32 gPlayerLookFiles_9_99[2] = {
#include "sub_main/0042AF20.inc"
};
u32 gPlayerLookFiles_9_101[2] = {
#include "sub_main/0042AF28.inc"
};
u32 gPlayerLookFiles_9_103[2] = {
#include "sub_main/0042AF30.inc"
};
u32 gPlayerLookFiles_9_105[2] = {
#include "sub_main/0042AF38.inc"
};
u32 gPlayerLookFiles_9_107[2] = {
#include "sub_main/0042AF40.inc"
};
u32 gPlayerLookFiles_9_109[2] = {
#include "sub_main/0042AF48.inc"
};
u32 gPlayerLookFiles_9_111[2] = {
#include "sub_main/0042AF50.inc"
};
u32 gPlayerLookFiles_9_113[2] = {
#include "sub_main/0042AF58.inc"
};
u32 gPlayerLookFiles_9_115[2] = {
#include "sub_main/0042AF60.inc"
};
u32 gPlayerLookFiles_9_117[2] = {
#include "sub_main/0042AF68.inc"
};
u32 gPlayerLookFiles_9_119[2] = {
#include "sub_main/0042AF70.inc"
};
u32 gPlayerLookFiles_9_121[2] = {
#include "sub_main/0042AF78.inc"
};
u32 gPlayerLookFiles_9_123[2] = {
#include "sub_main/0042AF80.inc"
};
u32 gPlayerLookFiles_9_125[2] = {
#include "sub_main/0042AF88.inc"
};
u32 gPlayerLookFiles_9_127[2] = {
#include "sub_main/0042AF90.inc"
};
u32 gPlayerLookFiles_9_129[2] = {
#include "sub_main/0042AF98.inc"
};
u32 gPlayerLookFiles_9_131[2] = {
#include "sub_main/0042AFA0.inc"
};
u32 gPlayerLookFiles_9_133[2] = {
#include "sub_main/0042AFA8.inc"
};
u32 gPlayerLookFiles_9_135[2] = {
#include "sub_main/0042AFB0.inc"
};
u32 gPlayerLookFiles_9_137[2] = {
#include "sub_main/0042AFB8.inc"
};
u32 gPlayerLookFiles_9_139[2] = {
#include "sub_main/0042AFC0.inc"
};
u32 gPlayerLookFiles_9_141[2] = {
#include "sub_main/0042AFC8.inc"
};
u32 gPlayerLookFiles_9_143[2] = {
#include "sub_main/0042AFD0.inc"
};
u32 gPlayerLookFiles_9_145[2] = {
#include "sub_main/0042AFD8.inc"
};
u32 gPlayerLookFiles_9_147[2] = {
#include "sub_main/0042AFE0.inc"
};
u32 gPlayerLookFiles_9_149[2] = {
#include "sub_main/0042AFE8.inc"
};
u32 gPlayerLookFiles_9_151[2] = {
#include "sub_main/0042AFF0.inc"
};
u32 gPlayerLookFiles_9_153[2] = {
#include "sub_main/0042AFF8.inc"
};
u32 gPlayerLookFiles_9_155[2] = {
#include "sub_main/0042B000.inc"
};
u32 gPlayerLookFiles_9_157[2] = {
#include "sub_main/0042B008.inc"
};
u32 gPlayerLookFiles_9_159[2] = {
#include "sub_main/0042B010.inc"
};
u32 gPlayerLookFiles_9_161[2] = {
#include "sub_main/0042B018.inc"
};
u32 gPlayerLookFiles_9_163[2] = {
#include "sub_main/0042B020.inc"
};
u32 gPlayerLookFiles_9_165[2] = {
#include "sub_main/0042B028.inc"
};
u32 gPlayerLookFiles_9_167[2] = {
#include "sub_main/0042B030.inc"
};
u32 gPlayerLookFiles_9_169[2] = {
#include "sub_main/0042B038.inc"
};
u32 gPlayerLookFiles_9_171[2] = {
#include "sub_main/0042B040.inc"
};
u32 gPlayerLookFiles_9_173[2] = {
#include "sub_main/0042B048.inc"
};
u32 gPlayerLookFiles_9_175[2] = {
#include "sub_main/0042B050.inc"
};
u32 gPlayerLookFiles_9_177[2] = {
#include "sub_main/0042B058.inc"
};
u32 gPlayerLookFiles_9_179[2] = {
#include "sub_main/0042B060.inc"
};
u32 gPlayerLookFiles_9_181[2] = {
#include "sub_main/0042B068.inc"
};
u32 gPlayerLookFiles_9_183[2] = {
#include "sub_main/0042B070.inc"
};
u32 gPlayerLookFiles_9_185[2] = {
#include "sub_main/0042B078.inc"
};
u32 gPlayerLookFiles_9_187[2] = {
#include "sub_main/0042B080.inc"
};
u32 gPlayerLookFiles_9_189[2] = {
#include "sub_main/0042B088.inc"
};
u32 gPlayerLookFiles_9_191[2] = {
#include "sub_main/0042B090.inc"
};
u32 gPlayerLookFiles_9_193[2] = {
#include "sub_main/0042B098.inc"
};
u32 gPlayerLookFiles_9_195[2] = {
#include "sub_main/0042B0A0.inc"
};
u32 gPlayerLookFiles_9_197[2] = {
#include "sub_main/0042B0A8.inc"
};
u32 gPlayerLookFiles_9_199[2] = {
#include "sub_main/0042B0B0.inc"
};
u32 gPlayerLookFiles_9_201[2] = {
#include "sub_main/0042B0B8.inc"
};
u32 gPlayerLookFiles_9_203[2] = {
#include "sub_main/0042B0C0.inc"
};
u32 gPlayerLookFiles_9_205[2] = {
#include "sub_main/0042B0C8.inc"
};
u32 gPlayerLookFiles_9_207[2] = {
#include "sub_main/0042B0D0.inc"
};
u32 gPlayerLookFiles_9_209[2] = {
#include "sub_main/0042B0D8.inc"
};
u32 gPlayerLookFiles_9_211[2] = {
#include "sub_main/0042B0E0.inc"
};
u32 gPlayerLookFiles_9_213[2] = {
#include "sub_main/0042B0E8.inc"
};
u32 gPlayerLookFiles_9_215[2] = {
#include "sub_main/0042B0F0.inc"
};
u32 gPlayerLookFiles_9_217[2] = {
#include "sub_main/0042B0F8.inc"
};
u32 gPlayerLookFiles_9_219[2] = {
#include "sub_main/0042B100.inc"
};
u32 gPlayerLookFiles_9_221[2] = {
#include "sub_main/0042B108.inc"
};
u32 gPlayerLookFiles_9_223[2] = {
#include "sub_main/0042B110.inc"
};
u32 gPlayerLookFiles_9_225[2] = {
#include "sub_main/0042B118.inc"
};
u32 gPlayerLookFiles_9_227[2] = {
#include "sub_main/0042B120.inc"
};
u32 gPlayerLookFiles_9_229[2] = {
#include "sub_main/0042B128.inc"
};
u32 gPlayerLookFiles_9_231[2] = {
#include "sub_main/0042B130.inc"
};
u32 gPlayerLookFiles_9_233[2] = {
#include "sub_main/0042B138.inc"
};
u32 gPlayerLookFiles_9_235[2] = {
#include "sub_main/0042B140.inc"
};
u32 gPlayerLookFiles_9_237[2] = {
#include "sub_main/0042B148.inc"
};
u32 gPlayerLookFiles_9_239[2] = {
#include "sub_main/0042B150.inc"
};
u32 gPlayerLookFiles_9_241[2] = {
#include "sub_main/0042B158.inc"
};
u32 gPlayerLookFiles_9_243[2] = {
#include "sub_main/0042B160.inc"
};
u32 gPlayerLookFiles_9_245[2] = {
#include "sub_main/0042B168.inc"
};
u32 gPlayerLookFiles_9_247[2] = {
#include "sub_main/0042B170.inc"
};
u32 gPlayerLookFiles_9_249[2] = {
#include "sub_main/0042B178.inc"
};
u32 gPlayerLookFiles_9_251[2] = {
#include "sub_main/0042B180.inc"
};
u32 gPlayerLookFiles_9_253[2] = {
#include "sub_main/0042B188.inc"
};
u32 gPlayerLookFiles_9_255[2] = {
#include "sub_main/0042B190.inc"
};
u32 gPlayerLookFiles_9_257[2] = {
#include "sub_main/0042B198.inc"
};
u32 gPlayerLookFiles_9_259[2] = {
#include "sub_main/0042B1A0.inc"
};
u32 gPlayerLookFiles_9_261[2] = {
#include "sub_main/0042B1A8.inc"
};
u32 gPlayerLookFiles_9_263[2] = {
#include "sub_main/0042B1B0.inc"
};
u32 gPlayerLookFiles_9_265[2] = {
#include "sub_main/0042B1B8.inc"
};
u32 gPlayerLookFiles_9_267[2] = {
#include "sub_main/0042B1C0.inc"
};
u32 gPlayerLookFiles_9_269[2] = {
#include "sub_main/0042B1C8.inc"
};
u32 gPlayerLookFiles_9_271[2] = {
#include "sub_main/0042B1D0.inc"
};
u32 gPlayerLookFiles_9_273[2] = {
#include "sub_main/0042B1D8.inc"
};
u32 gPlayerLookFiles_9_275[2] = {
#include "sub_main/0042B1E0.inc"
};
u32 gPlayerLookFiles_9_277[2] = {
#include "sub_main/0042B1E8.inc"
};
u32 gPlayerLookFiles_9_279[2] = {
#include "sub_main/0042B1F0.inc"
};
u32 gPlayerLookFiles_9_281[2] = {
#include "sub_main/0042B1F8.inc"
};
u32 gPlayerLookFiles_9_283[2] = {
#include "sub_main/0042B200.inc"
};
u32 gPlayerLookFiles_9_285[2] = {
#include "sub_main/0042B208.inc"
};
u32 gPlayerLookFiles_9_287[2] = {
#include "sub_main/0042B210.inc"
};
u32 gPlayerLookFiles_9_289[2] = {
#include "sub_main/0042B218.inc"
};
u32 gPlayerLookFiles_9_291[2] = {
#include "sub_main/0042B220.inc"
};
u32 gPlayerLookFiles_9_293[2] = {
#include "sub_main/0042B228.inc"
};
u32 gPlayerLookFiles_9_295[2] = {
#include "sub_main/0042B230.inc"
};
u32 gPlayerLookFiles_9_297[2] = {
#include "sub_main/0042B238.inc"
};
u32 gPlayerLookFiles_9_299[2] = {
#include "sub_main/0042B240.inc"
};
u32 gPlayerLookFiles_9_301[2] = {
#include "sub_main/0042B248.inc"
};
u32 gPlayerLookFiles_9_303[2] = {
#include "sub_main/0042B250.inc"
};
u32 gPlayerLookFiles_9_305[2] = {
#include "sub_main/0042B258.inc"
};
u32 gPlayerLookFiles_9_307[2] = {
#include "sub_main/0042B260.inc"
};
u32 gPlayerLookFiles_9_309[2] = {
#include "sub_main/0042B268.inc"
};
u32 gPlayerLookFiles_9_311[2] = {
#include "sub_main/0042B270.inc"
};
u32 gPlayerLookFiles_9_313[2] = {
#include "sub_main/0042B278.inc"
};
u32 gPlayerLookFiles_9_315[2] = {
#include "sub_main/0042B280.inc"
};
u32 gPlayerLookFiles_9_317[2] = {
#include "sub_main/0042B288.inc"
};
u32 gPlayerLookFiles_9_319[2] = {
#include "sub_main/0042B290.inc"
};
u32 gPlayerLookFiles_9_321[2] = {
#include "sub_main/0042B298.inc"
};
u32 gPlayerLookFiles_9_323[2] = {
#include "sub_main/0042B2A0.inc"
};
u32 gPlayerLookFiles_9_325[2] = {
#include "sub_main/0042B2A8.inc"
};
u32 gPlayerLookFiles_9_327[2] = {
#include "sub_main/0042B2B0.inc"
};
u32 gPlayerLookFiles_9_329[2] = {
#include "sub_main/0042B2B8.inc"
};
u32 gPlayerLookFiles_9_331[2] = {
#include "sub_main/0042B2C0.inc"
};
u32 gPlayerLookFiles_9_333[2] = {
#include "sub_main/0042B2C8.inc"
};
u32 gPlayerLookFiles_9_335[2] = {
#include "sub_main/0042B2D0.inc"
};
u32 gPlayerLookFiles_9_337[2] = {
#include "sub_main/0042B2D8.inc"
};
u32 gPlayerLookFiles_9_339[2] = {
#include "sub_main/0042B2E0.inc"
};
u32 gPlayerLookFiles_9_341[2] = {
#include "sub_main/0042B2E8.inc"
};
u32 gPlayerLookFiles_9_343[2] = {
#include "sub_main/0042B2F0.inc"
};
u32 gPlayerLookFiles_9_345[2] = {
#include "sub_main/0042B2F8.inc"
};
u32 gPlayerLookFiles_9_347[2] = {
#include "sub_main/0042B300.inc"
};
u32 gPlayerLookFiles_9_349[2] = {
#include "sub_main/0042B308.inc"
};
u32 gPlayerLookFiles_9_351[2] = {
#include "sub_main/0042B310.inc"
};
u32 gPlayerLookFiles_9_353[2] = {
#include "sub_main/0042B318.inc"
};
u32 gPlayerLookFiles_9_355[2] = {
#include "sub_main/0042B320.inc"
};
u32 gPlayerLookFiles_9_357[2] = {
#include "sub_main/0042B328.inc"
};
u32 gPlayerLookFiles_9_359[2] = {
#include "sub_main/0042B330.inc"
};
u32 gPlayerLookFiles_9_361[2] = {
#include "sub_main/0042B338.inc"
};
u32 gPlayerLookFiles_9_363[2] = {
#include "sub_main/0042B340.inc"
};
u32 gPlayerLookFiles_9_365[2] = {
#include "sub_main/0042B348.inc"
};
u32 gPlayerLookFiles_9_367[2] = {
#include "sub_main/0042B350.inc"
};
u32 gPlayerLookFiles_9_369[2] = {
#include "sub_main/0042B358.inc"
};
u32 gPlayerLookFiles_9_371[2] = {
#include "sub_main/0042B360.inc"
};
u32 gPlayerLookFiles_9_373[2] = {
#include "sub_main/0042B368.inc"
};
u32 gPlayerLookFiles_9_375[2] = {
#include "sub_main/0042B370.inc"
};
u32 gPlayerLookFiles_9_377[2] = {
#include "sub_main/0042B378.inc"
};
u32 gPlayerLookFiles_9_379[2] = {
#include "sub_main/0042B380.inc"
};
u32 gPlayerLookFiles_9_381[2] = {
#include "sub_main/0042B388.inc"
};
u32 gPlayerLookFiles_9_383[2] = {
#include "sub_main/0042B390.inc"
};
u32 gPlayerLookFiles_9_385[2] = {
#include "sub_main/0042B398.inc"
};
u32 gPlayerLookFiles_9_387[2] = {
#include "sub_main/0042B3A0.inc"
};
u32 gPlayerLookFiles_9_389[2] = {
#include "sub_main/0042B3A8.inc"
};
u32 gPlayerLookFiles_9_391[2] = {
#include "sub_main/0042B3B0.inc"
};
u32 gPlayerLookFiles_9_393[2] = {
#include "sub_main/0042B3B8.inc"
};
u32 gPlayerLookFiles_9_395[2] = {
#include "sub_main/0042B3C0.inc"
};
u32 gPlayerLookFiles_9_397[2] = {
#include "sub_main/0042B3C8.inc"
};
u32 gPlayerLookFiles_9_399[2] = {
#include "sub_main/0042B3D0.inc"
};
u32 gPlayerLookFiles_9_401[2] = {
#include "sub_main/0042B3D8.inc"
};
u32 gPlayerLookFiles_9_403[2] = {
#include "sub_main/0042B3E0.inc"
};
u32 gPlayerLookFiles_9_405[2] = {
#include "sub_main/0042B3E8.inc"
};
u32 gPlayerLookFiles_9_407[2] = {
#include "sub_main/0042B3F0.inc"
};
u32 gPlayerLookFiles_9_409[2] = {
#include "sub_main/0042B3F8.inc"
};
u32 gPlayerLookFiles_9_411[2] = {
#include "sub_main/0042B400.inc"
};
u32 gPlayerLookFiles_9_413[2] = {
#include "sub_main/0042B408.inc"
};
u32 gPlayerLookFiles_9_415[2] = {
#include "sub_main/0042B410.inc"
};
u32 gPlayerLookFiles_9_417[2] = {
#include "sub_main/0042B418.inc"
};
u32 gPlayerLookFiles_9_419[2] = {
#include "sub_main/0042B420.inc"
};
u32 gPlayerLookFiles_9_421[2] = {
#include "sub_main/0042B428.inc"
};
u32 gPlayerLookFiles_9_423[2] = {
#include "sub_main/0042B430.inc"
};
u32 gPlayerLookFiles_9_425[2] = {
#include "sub_main/0042B438.inc"
};
u32 gPlayerLookFiles_9_427[2] = {
#include "sub_main/0042B440.inc"
};
u32 gPlayerLookFiles_9_429[2] = {
#include "sub_main/0042B448.inc"
};
u32 gPlayerLookFiles_9_431[2] = {
#include "sub_main/0042B450.inc"
};
u32 gPlayerLookFiles_9_433[2] = {
#include "sub_main/0042B458.inc"
};
u32 gPlayerLookFiles_9_435[2] = {
#include "sub_main/0042B460.inc"
};
u32 gPlayerLookFiles_9_437[2] = {
#include "sub_main/0042B468.inc"
};
u32 gPlayerLookFiles_9[438] = {
#include "sub_main/0042B470.inc"
};
u32 gPlayerLookFiles_10_1[2] = {
#include "sub_main/0042BB48.inc"
};
u32 gPlayerLookFiles_10_3[2] = {
#include "sub_main/0042BB50.inc"
};
u32 gPlayerLookFiles_10_5[2] = {
#include "sub_main/0042BB58.inc"
};
u32 gPlayerLookFiles_10_7[2] = {
#include "sub_main/0042BB60.inc"
};
u32 gPlayerLookFiles_10_9[2] = {
#include "sub_main/0042BB68.inc"
};
u32 gPlayerLookFiles_10_11[2] = {
#include "sub_main/0042BB70.inc"
};
u32 gPlayerLookFiles_10_13[2] = {
#include "sub_main/0042BB78.inc"
};
u32 gPlayerLookFiles_10_15[2] = {
#include "sub_main/0042BB80.inc"
};
u32 gPlayerLookFiles_10_17[2] = {
#include "sub_main/0042BB88.inc"
};
u32 gPlayerLookFiles_10_19[2] = {
#include "sub_main/0042BB90.inc"
};
u32 gPlayerLookFiles_10_21[2] = {
#include "sub_main/0042BB98.inc"
};
u32 gPlayerLookFiles_10_23[2] = {
#include "sub_main/0042BBA0.inc"
};
u32 gPlayerLookFiles_10_25[2] = {
#include "sub_main/0042BBA8.inc"
};
u32 gPlayerLookFiles_10_27[2] = {
#include "sub_main/0042BBB0.inc"
};
u32 gPlayerLookFiles_10_29[2] = {
#include "sub_main/0042BBB8.inc"
};
u32 gPlayerLookFiles_10_31[2] = {
#include "sub_main/0042BBC0.inc"
};
u32 gPlayerLookFiles_10_33[2] = {
#include "sub_main/0042BBC8.inc"
};
u32 gPlayerLookFiles_10_35[2] = {
#include "sub_main/0042BBD0.inc"
};
u32 gPlayerLookFiles_10_37[2] = {
#include "sub_main/0042BBD8.inc"
};
u32 gPlayerLookFiles_10_39[2] = {
#include "sub_main/0042BBE0.inc"
};
u32 gPlayerLookFiles_10_41[2] = {
#include "sub_main/0042BBE8.inc"
};
u32 gPlayerLookFiles_10_43[2] = {
#include "sub_main/0042BBF0.inc"
};
u32 gPlayerLookFiles_10_45[2] = {
#include "sub_main/0042BBF8.inc"
};
u32 gPlayerLookFiles_10_47[2] = {
#include "sub_main/0042BC00.inc"
};
u32 gPlayerLookFiles_10_49[2] = {
#include "sub_main/0042BC08.inc"
};
u32 gPlayerLookFiles_10_51[2] = {
#include "sub_main/0042BC10.inc"
};
u32 gPlayerLookFiles_10_53[2] = {
#include "sub_main/0042BC18.inc"
};
u32 gPlayerLookFiles_10_55[2] = {
#include "sub_main/0042BC20.inc"
};
u32 gPlayerLookFiles_10_57[2] = {
#include "sub_main/0042BC28.inc"
};
u32 gPlayerLookFiles_10_59[2] = {
#include "sub_main/0042BC30.inc"
};
u32 gPlayerLookFiles_10_61[2] = {
#include "sub_main/0042BC38.inc"
};
u32 gPlayerLookFiles_10_63[2] = {
#include "sub_main/0042BC40.inc"
};
u32 gPlayerLookFiles_10_65[2] = {
#include "sub_main/0042BC48.inc"
};
u32 gPlayerLookFiles_10_67[2] = {
#include "sub_main/0042BC50.inc"
};
u32 gPlayerLookFiles_10_69[2] = {
#include "sub_main/0042BC58.inc"
};
u32 gPlayerLookFiles_10_71[2] = {
#include "sub_main/0042BC60.inc"
};
u32 gPlayerLookFiles_10_73[2] = {
#include "sub_main/0042BC68.inc"
};
u32 gPlayerLookFiles_10_75[2] = {
#include "sub_main/0042BC70.inc"
};
u32 gPlayerLookFiles_10_77[2] = {
#include "sub_main/0042BC78.inc"
};
u32 gPlayerLookFiles_10_79[2] = {
#include "sub_main/0042BC80.inc"
};
u32 gPlayerLookFiles_10_81[2] = {
#include "sub_main/0042BC88.inc"
};
u32 gPlayerLookFiles_10_83[2] = {
#include "sub_main/0042BC90.inc"
};
u32 gPlayerLookFiles_10_85[2] = {
#include "sub_main/0042BC98.inc"
};
u32 gPlayerLookFiles_10_87[2] = {
#include "sub_main/0042BCA0.inc"
};
u32 gPlayerLookFiles_10_89[2] = {
#include "sub_main/0042BCA8.inc"
};
u32 gPlayerLookFiles_10_91[2] = {
#include "sub_main/0042BCB0.inc"
};
u32 gPlayerLookFiles_10_93[2] = {
#include "sub_main/0042BCB8.inc"
};
u32 gPlayerLookFiles_10_95[2] = {
#include "sub_main/0042BCC0.inc"
};
u32 gPlayerLookFiles_10_97[2] = {
#include "sub_main/0042BCC8.inc"
};
u32 gPlayerLookFiles_10_99[2] = {
#include "sub_main/0042BCD0.inc"
};
u32 gPlayerLookFiles_10_101[2] = {
#include "sub_main/0042BCD8.inc"
};
u32 gPlayerLookFiles_10_103[2] = {
#include "sub_main/0042BCE0.inc"
};
u32 gPlayerLookFiles_10_105[2] = {
#include "sub_main/0042BCE8.inc"
};
u32 gPlayerLookFiles_10_107[2] = {
#include "sub_main/0042BCF0.inc"
};
u32 gPlayerLookFiles_10_109[2] = {
#include "sub_main/0042BCF8.inc"
};
u32 gPlayerLookFiles_10_111[2] = {
#include "sub_main/0042BD00.inc"
};
u32 gPlayerLookFiles_10_113[2] = {
#include "sub_main/0042BD08.inc"
};
u32 gPlayerLookFiles_10_115[2] = {
#include "sub_main/0042BD10.inc"
};
u32 gPlayerLookFiles_10_117[2] = {
#include "sub_main/0042BD18.inc"
};
u32 gPlayerLookFiles_10_119[2] = {
#include "sub_main/0042BD20.inc"
};
u32 gPlayerLookFiles_10_121[2] = {
#include "sub_main/0042BD28.inc"
};
u32 gPlayerLookFiles_10_123[2] = {
#include "sub_main/0042BD30.inc"
};
u32 gPlayerLookFiles_10_125[2] = {
#include "sub_main/0042BD38.inc"
};
u32 gPlayerLookFiles_10_127[2] = {
#include "sub_main/0042BD40.inc"
};
u32 gPlayerLookFiles_10_129[2] = {
#include "sub_main/0042BD48.inc"
};
u32 gPlayerLookFiles_10_131[2] = {
#include "sub_main/0042BD50.inc"
};
u32 gPlayerLookFiles_10_133[2] = {
#include "sub_main/0042BD58.inc"
};
u32 gPlayerLookFiles_10_135[2] = {
#include "sub_main/0042BD60.inc"
};
u32 gPlayerLookFiles_10_137[2] = {
#include "sub_main/0042BD68.inc"
};
u32 gPlayerLookFiles_10_139[2] = {
#include "sub_main/0042BD70.inc"
};
u32 gPlayerLookFiles_10_141[2] = {
#include "sub_main/0042BD78.inc"
};
u32 gPlayerLookFiles_10_143[2] = {
#include "sub_main/0042BD80.inc"
};
u32 gPlayerLookFiles_10_145[2] = {
#include "sub_main/0042BD88.inc"
};
u32 gPlayerLookFiles_10_147[2] = {
#include "sub_main/0042BD90.inc"
};
u32 gPlayerLookFiles_10_149[2] = {
#include "sub_main/0042BD98.inc"
};
u32 gPlayerLookFiles_10_151[2] = {
#include "sub_main/0042BDA0.inc"
};
u32 gPlayerLookFiles_10_153[2] = {
#include "sub_main/0042BDA8.inc"
};
u32 gPlayerLookFiles_10_155[2] = {
#include "sub_main/0042BDB0.inc"
};
u32 gPlayerLookFiles_10_157[2] = {
#include "sub_main/0042BDB8.inc"
};
u32 gPlayerLookFiles_10_159[2] = {
#include "sub_main/0042BDC0.inc"
};
u32 gPlayerLookFiles_10_161[2] = {
#include "sub_main/0042BDC8.inc"
};
u32 gPlayerLookFiles_10_163[2] = {
#include "sub_main/0042BDD0.inc"
};
u32 gPlayerLookFiles_10_165[2] = {
#include "sub_main/0042BDD8.inc"
};
u32 gPlayerLookFiles_10_167[2] = {
#include "sub_main/0042BDE0.inc"
};
u32 gPlayerLookFiles_10_169[2] = {
#include "sub_main/0042BDE8.inc"
};
u32 gPlayerLookFiles_10_171[2] = {
#include "sub_main/0042BDF0.inc"
};
u32 gPlayerLookFiles_10_173[2] = {
#include "sub_main/0042BDF8.inc"
};
u32 gPlayerLookFiles_10_175[2] = {
#include "sub_main/0042BE00.inc"
};
u32 gPlayerLookFiles_10_177[2] = {
#include "sub_main/0042BE08.inc"
};
u32 gPlayerLookFiles_10_179[2] = {
#include "sub_main/0042BE10.inc"
};
u32 gPlayerLookFiles_10_181[2] = {
#include "sub_main/0042BE18.inc"
};
u32 gPlayerLookFiles_10_183[2] = {
#include "sub_main/0042BE20.inc"
};
u32 gPlayerLookFiles_10_185[2] = {
#include "sub_main/0042BE28.inc"
};
u32 gPlayerLookFiles_10_187[2] = {
#include "sub_main/0042BE30.inc"
};
u32 gPlayerLookFiles_10_189[2] = {
#include "sub_main/0042BE38.inc"
};
u32 gPlayerLookFiles_10_191[2] = {
#include "sub_main/0042BE40.inc"
};
u32 gPlayerLookFiles_10_193[2] = {
#include "sub_main/0042BE48.inc"
};
u32 gPlayerLookFiles_10_195[2] = {
#include "sub_main/0042BE50.inc"
};
u32 gPlayerLookFiles_10_197[2] = {
#include "sub_main/0042BE58.inc"
};
u32 gPlayerLookFiles_10_199[2] = {
#include "sub_main/0042BE60.inc"
};
u32 gPlayerLookFiles_10_201[2] = {
#include "sub_main/0042BE68.inc"
};
u32 gPlayerLookFiles_10_203[2] = {
#include "sub_main/0042BE70.inc"
};
u32 gPlayerLookFiles_10_205[2] = {
#include "sub_main/0042BE78.inc"
};
u32 gPlayerLookFiles_10_207[2] = {
#include "sub_main/0042BE80.inc"
};
u32 gPlayerLookFiles_10_209[2] = {
#include "sub_main/0042BE88.inc"
};
u32 gPlayerLookFiles_10_211[2] = {
#include "sub_main/0042BE90.inc"
};
u32 gPlayerLookFiles_10_213[2] = {
#include "sub_main/0042BE98.inc"
};
u32 gPlayerLookFiles_10_215[2] = {
#include "sub_main/0042BEA0.inc"
};
u32 gPlayerLookFiles_10_217[2] = {
#include "sub_main/0042BEA8.inc"
};
u32 gPlayerLookFiles_10_219[2] = {
#include "sub_main/0042BEB0.inc"
};
u32 gPlayerLookFiles_10_221[2] = {
#include "sub_main/0042BEB8.inc"
};
u32 gPlayerLookFiles_10_223[2] = {
#include "sub_main/0042BEC0.inc"
};
u32 gPlayerLookFiles_10_225[2] = {
#include "sub_main/0042BEC8.inc"
};
u32 gPlayerLookFiles_10_227[2] = {
#include "sub_main/0042BED0.inc"
};
u32 gPlayerLookFiles_10_229[2] = {
#include "sub_main/0042BED8.inc"
};
u32 gPlayerLookFiles_10_231[2] = {
#include "sub_main/0042BEE0.inc"
};
u32 gPlayerLookFiles_10_233[2] = {
#include "sub_main/0042BEE8.inc"
};
u32 gPlayerLookFiles_10_235[2] = {
#include "sub_main/0042BEF0.inc"
};
u32 gPlayerLookFiles_10_237[2] = {
#include "sub_main/0042BEF8.inc"
};
u32 gPlayerLookFiles_10_239[2] = {
#include "sub_main/0042BF00.inc"
};
u32 gPlayerLookFiles_10_241[2] = {
#include "sub_main/0042BF08.inc"
};
u32 gPlayerLookFiles_10_243[2] = {
#include "sub_main/0042BF10.inc"
};
u32 gPlayerLookFiles_10_245[2] = {
#include "sub_main/0042BF18.inc"
};
u32 gPlayerLookFiles_10_247[2] = {
#include "sub_main/0042BF20.inc"
};
u32 gPlayerLookFiles_10_249[2] = {
#include "sub_main/0042BF28.inc"
};
u32 gPlayerLookFiles_10_251[2] = {
#include "sub_main/0042BF30.inc"
};
u32 gPlayerLookFiles_10_253[2] = {
#include "sub_main/0042BF38.inc"
};
u32 gPlayerLookFiles_10_255[2] = {
#include "sub_main/0042BF40.inc"
};
u32 gPlayerLookFiles_10_257[2] = {
#include "sub_main/0042BF48.inc"
};
u32 gPlayerLookFiles_10_259[2] = {
#include "sub_main/0042BF50.inc"
};
u32 gPlayerLookFiles_10_261[2] = {
#include "sub_main/0042BF58.inc"
};
u32 gPlayerLookFiles_10_263[2] = {
#include "sub_main/0042BF60.inc"
};
u32 gPlayerLookFiles_10_265[2] = {
#include "sub_main/0042BF68.inc"
};
u32 gPlayerLookFiles_10_267[2] = {
#include "sub_main/0042BF70.inc"
};
u32 gPlayerLookFiles_10_269[2] = {
#include "sub_main/0042BF78.inc"
};
u32 gPlayerLookFiles_10_271[2] = {
#include "sub_main/0042BF80.inc"
};
u32 gPlayerLookFiles_10_273[2] = {
#include "sub_main/0042BF88.inc"
};
u32 gPlayerLookFiles_10_275[2] = {
#include "sub_main/0042BF90.inc"
};
u32 gPlayerLookFiles_10_277[2] = {
#include "sub_main/0042BF98.inc"
};
u32 gPlayerLookFiles_10_279[2] = {
#include "sub_main/0042BFA0.inc"
};
u32 gPlayerLookFiles_10_281[2] = {
#include "sub_main/0042BFA8.inc"
};
u32 gPlayerLookFiles_10_283[2] = {
#include "sub_main/0042BFB0.inc"
};
u32 gPlayerLookFiles_10_285[2] = {
#include "sub_main/0042BFB8.inc"
};
u32 gPlayerLookFiles_10_287[2] = {
#include "sub_main/0042BFC0.inc"
};
u32 gPlayerLookFiles_10_289[2] = {
#include "sub_main/0042BFC8.inc"
};
u32 gPlayerLookFiles_10_291[2] = {
#include "sub_main/0042BFD0.inc"
};
u32 gPlayerLookFiles_10_293[2] = {
#include "sub_main/0042BFD8.inc"
};
u32 gPlayerLookFiles_10_295[2] = {
#include "sub_main/0042BFE0.inc"
};
u32 gPlayerLookFiles_10_297[2] = {
#include "sub_main/0042BFE8.inc"
};
u32 gPlayerLookFiles_10_299[2] = {
#include "sub_main/0042BFF0.inc"
};
u32 gPlayerLookFiles_10_301[2] = {
#include "sub_main/0042BFF8.inc"
};
u32 gPlayerLookFiles_10_303[2] = {
#include "sub_main/0042C000.inc"
};
u32 gPlayerLookFiles_10_305[2] = {
#include "sub_main/0042C008.inc"
};
u32 gPlayerLookFiles_10_307[2] = {
#include "sub_main/0042C010.inc"
};
u32 gPlayerLookFiles_10_309[2] = {
#include "sub_main/0042C018.inc"
};
u32 gPlayerLookFiles_10_311[2] = {
#include "sub_main/0042C020.inc"
};
u32 gPlayerLookFiles_10_313[2] = {
#include "sub_main/0042C028.inc"
};
u32 gPlayerLookFiles_10_315[2] = {
#include "sub_main/0042C030.inc"
};
u32 gPlayerLookFiles_10_317[2] = {
#include "sub_main/0042C038.inc"
};
u32 gPlayerLookFiles_10_319[2] = {
#include "sub_main/0042C040.inc"
};
u32 gPlayerLookFiles_10_321[2] = {
#include "sub_main/0042C048.inc"
};
u32 gPlayerLookFiles_10_323[2] = {
#include "sub_main/0042C050.inc"
};
u32 gPlayerLookFiles_10_325[2] = {
#include "sub_main/0042C058.inc"
};
u32 gPlayerLookFiles_10_327[2] = {
#include "sub_main/0042C060.inc"
};
u32 gPlayerLookFiles_10_329[2] = {
#include "sub_main/0042C068.inc"
};
u32 gPlayerLookFiles_10_331[2] = {
#include "sub_main/0042C070.inc"
};
u32 gPlayerLookFiles_10_333[2] = {
#include "sub_main/0042C078.inc"
};
u32 gPlayerLookFiles_10_335[2] = {
#include "sub_main/0042C080.inc"
};
u32 gPlayerLookFiles_10_337[2] = {
#include "sub_main/0042C088.inc"
};
u32 gPlayerLookFiles_10_339[2] = {
#include "sub_main/0042C090.inc"
};
u32 gPlayerLookFiles_10_341[2] = {
#include "sub_main/0042C098.inc"
};
u32 gPlayerLookFiles_10_343[2] = {
#include "sub_main/0042C0A0.inc"
};
u32 gPlayerLookFiles_10_345[2] = {
#include "sub_main/0042C0A8.inc"
};
u32 gPlayerLookFiles_10_347[2] = {
#include "sub_main/0042C0B0.inc"
};
u32 gPlayerLookFiles_10_349[2] = {
#include "sub_main/0042C0B8.inc"
};
u32 gPlayerLookFiles_10_351[2] = {
#include "sub_main/0042C0C0.inc"
};
u32 gPlayerLookFiles_10_353[2] = {
#include "sub_main/0042C0C8.inc"
};
u32 gPlayerLookFiles_10_355[2] = {
#include "sub_main/0042C0D0.inc"
};
u32 gPlayerLookFiles_10_357[2] = {
#include "sub_main/0042C0D8.inc"
};
u32 gPlayerLookFiles_10_359[2] = {
#include "sub_main/0042C0E0.inc"
};
u32 gPlayerLookFiles_10_361[2] = {
#include "sub_main/0042C0E8.inc"
};
u32 gPlayerLookFiles_10_363[2] = {
#include "sub_main/0042C0F0.inc"
};
u32 gPlayerLookFiles_10_365[2] = {
#include "sub_main/0042C0F8.inc"
};
u32 gPlayerLookFiles_10_367[2] = {
#include "sub_main/0042C100.inc"
};
u32 gPlayerLookFiles_10_369[2] = {
#include "sub_main/0042C108.inc"
};
u32 gPlayerLookFiles_10_371[2] = {
#include "sub_main/0042C110.inc"
};
u32 gPlayerLookFiles_10_373[2] = {
#include "sub_main/0042C118.inc"
};
u32 gPlayerLookFiles_10_375[2] = {
#include "sub_main/0042C120.inc"
};
u32 gPlayerLookFiles_10_377[2] = {
#include "sub_main/0042C128.inc"
};
u32 gPlayerLookFiles_10_379[2] = {
#include "sub_main/0042C130.inc"
};
u32 gPlayerLookFiles_10_381[2] = {
#include "sub_main/0042C138.inc"
};
u32 gPlayerLookFiles_10_383[2] = {
#include "sub_main/0042C140.inc"
};
u32 gPlayerLookFiles_10_385[2] = {
#include "sub_main/0042C148.inc"
};
u32 gPlayerLookFiles_10_387[2] = {
#include "sub_main/0042C150.inc"
};
u32 gPlayerLookFiles_10_389[2] = {
#include "sub_main/0042C158.inc"
};
u32 gPlayerLookFiles_10_391[2] = {
#include "sub_main/0042C160.inc"
};
u32 gPlayerLookFiles_10_393[2] = {
#include "sub_main/0042C168.inc"
};
u32 gPlayerLookFiles_10_395[2] = {
#include "sub_main/0042C170.inc"
};
u32 gPlayerLookFiles_10_397[2] = {
#include "sub_main/0042C178.inc"
};
u32 gPlayerLookFiles_10_399[2] = {
#include "sub_main/0042C180.inc"
};
u32 gPlayerLookFiles_10_401[2] = {
#include "sub_main/0042C188.inc"
};
u32 gPlayerLookFiles_10_403[2] = {
#include "sub_main/0042C190.inc"
};
u32 gPlayerLookFiles_10_405[2] = {
#include "sub_main/0042C198.inc"
};
u32 gPlayerLookFiles_10_407[2] = {
#include "sub_main/0042C1A0.inc"
};
u32 gPlayerLookFiles_10_409[2] = {
#include "sub_main/0042C1A8.inc"
};
u32 gPlayerLookFiles_10_411[2] = {
#include "sub_main/0042C1B0.inc"
};
u32 gPlayerLookFiles_10_413[2] = {
#include "sub_main/0042C1B8.inc"
};
u32 gPlayerLookFiles_10_415[2] = {
#include "sub_main/0042C1C0.inc"
};
u32 gPlayerLookFiles_10_417[2] = {
#include "sub_main/0042C1C8.inc"
};
u32 gPlayerLookFiles_10_419[2] = {
#include "sub_main/0042C1D0.inc"
};
u32 gPlayerLookFiles_10_421[2] = {
#include "sub_main/0042C1D8.inc"
};
u32 gPlayerLookFiles_10_423[2] = {
#include "sub_main/0042C1E0.inc"
};
u32 gPlayerLookFiles_10_425[2] = {
#include "sub_main/0042C1E8.inc"
};
u32 gPlayerLookFiles_10_427[2] = {
#include "sub_main/0042C1F0.inc"
};
u32 gPlayerLookFiles_10_429[2] = {
#include "sub_main/0042C1F8.inc"
};
u32 gPlayerLookFiles_10_431[2] = {
#include "sub_main/0042C200.inc"
};
u32 gPlayerLookFiles_10_433[2] = {
#include "sub_main/0042C208.inc"
};
u32 gPlayerLookFiles_10_435[2] = {
#include "sub_main/0042C210.inc"
};
u32 gPlayerLookFiles_10_437[2] = {
#include "sub_main/0042C218.inc"
};
u32 gPlayerLookFiles_10_439[2] = {
#include "sub_main/0042C220.inc"
};
u32 gPlayerLookFiles_10_441[2] = {
#include "sub_main/0042C228.inc"
};
u32 gPlayerLookFiles_10_443[2] = {
#include "sub_main/0042C230.inc"
};
u32 gPlayerLookFiles_10_445[2] = {
#include "sub_main/0042C238.inc"
};
u32 gPlayerLookFiles_10_447[2] = {
#include "sub_main/0042C240.inc"
};
u32 gPlayerLookFiles_10_449[2] = {
#include "sub_main/0042C248.inc"
};
u32 gPlayerLookFiles_10_451[2] = {
#include "sub_main/0042C250.inc"
};
u32 gPlayerLookFiles_10_453[2] = {
#include "sub_main/0042C258.inc"
};
u32 gPlayerLookFiles_10_455[2] = {
#include "sub_main/0042C260.inc"
};
u32 gPlayerLookFiles_10_457[2] = {
#include "sub_main/0042C268.inc"
};
u32 gPlayerLookFiles_10_459[2] = {
#include "sub_main/0042C270.inc"
};
u32 gPlayerLookFiles_10_461[2] = {
#include "sub_main/0042C278.inc"
};
u32 gPlayerLookFiles_10_463[2] = {
#include "sub_main/0042C280.inc"
};
u32 gPlayerLookFiles_10_465[2] = {
#include "sub_main/0042C288.inc"
};
u32 gPlayerLookFiles_10_467[2] = {
#include "sub_main/0042C290.inc"
};
u32 gPlayerLookFiles_10_469[2] = {
#include "sub_main/0042C298.inc"
};
u32 gPlayerLookFiles_10[470] = {
#include "sub_main/0042C2A0.inc"
};
u32 gPlayerLookFiles_11_1[2] = {
#include "sub_main/0042C9F8.inc"
};
u32 gPlayerLookFiles_11_3[2] = {
#include "sub_main/0042CA00.inc"
};
u32 gPlayerLookFiles_11_5[2] = {
#include "sub_main/0042CA08.inc"
};
u32 gPlayerLookFiles_11_7[2] = {
#include "sub_main/0042CA10.inc"
};
u32 gPlayerLookFiles_11_9[2] = {
#include "sub_main/0042CA18.inc"
};
u32 gPlayerLookFiles_11_11[2] = {
#include "sub_main/0042CA20.inc"
};
u32 gPlayerLookFiles_11_13[2] = {
#include "sub_main/0042CA28.inc"
};
u32 gPlayerLookFiles_11_15[2] = {
#include "sub_main/0042CA30.inc"
};
u32 gPlayerLookFiles_11_17[2] = {
#include "sub_main/0042CA38.inc"
};
u32 gPlayerLookFiles_11_19[2] = {
#include "sub_main/0042CA40.inc"
};
u32 gPlayerLookFiles_11_21[2] = {
#include "sub_main/0042CA48.inc"
};
u32 gPlayerLookFiles_11_23[2] = {
#include "sub_main/0042CA50.inc"
};
u32 gPlayerLookFiles_11_25[2] = {
#include "sub_main/0042CA58.inc"
};
u32 gPlayerLookFiles_11_27[2] = {
#include "sub_main/0042CA60.inc"
};
u32 gPlayerLookFiles_11_29[2] = {
#include "sub_main/0042CA68.inc"
};
u32 gPlayerLookFiles_11_31[2] = {
#include "sub_main/0042CA70.inc"
};
u32 gPlayerLookFiles_11_33[2] = {
#include "sub_main/0042CA78.inc"
};
u32 gPlayerLookFiles_11_35[2] = {
#include "sub_main/0042CA80.inc"
};
u32 gPlayerLookFiles_11_37[2] = {
#include "sub_main/0042CA88.inc"
};
u32 gPlayerLookFiles_11_39[2] = {
#include "sub_main/0042CA90.inc"
};
u32 gPlayerLookFiles_11_41[2] = {
#include "sub_main/0042CA98.inc"
};
u32 gPlayerLookFiles_11_43[2] = {
#include "sub_main/0042CAA0.inc"
};
u32 gPlayerLookFiles_11_45[2] = {
#include "sub_main/0042CAA8.inc"
};
u32 gPlayerLookFiles_11_47[2] = {
#include "sub_main/0042CAB0.inc"
};
u32 gPlayerLookFiles_11_49[2] = {
#include "sub_main/0042CAB8.inc"
};
u32 gPlayerLookFiles_11_51[2] = {
#include "sub_main/0042CAC0.inc"
};
u32 gPlayerLookFiles_11_53[2] = {
#include "sub_main/0042CAC8.inc"
};
u32 gPlayerLookFiles_11_55[2] = {
#include "sub_main/0042CAD0.inc"
};
u32 gPlayerLookFiles_11_57[2] = {
#include "sub_main/0042CAD8.inc"
};
u32 gPlayerLookFiles_11_59[2] = {
#include "sub_main/0042CAE0.inc"
};
u32 gPlayerLookFiles_11_61[2] = {
#include "sub_main/0042CAE8.inc"
};
u32 gPlayerLookFiles_11_63[2] = {
#include "sub_main/0042CAF0.inc"
};
u32 gPlayerLookFiles_11_65[2] = {
#include "sub_main/0042CAF8.inc"
};
u32 gPlayerLookFiles_11_67[2] = {
#include "sub_main/0042CB00.inc"
};
u32 gPlayerLookFiles_11_69[2] = {
#include "sub_main/0042CB08.inc"
};
u32 gPlayerLookFiles_11_71[2] = {
#include "sub_main/0042CB10.inc"
};
u32 gPlayerLookFiles_11_73[2] = {
#include "sub_main/0042CB18.inc"
};
u32 gPlayerLookFiles_11_75[2] = {
#include "sub_main/0042CB20.inc"
};
u32 gPlayerLookFiles_11_77[2] = {
#include "sub_main/0042CB28.inc"
};
u32 gPlayerLookFiles_11_79[2] = {
#include "sub_main/0042CB30.inc"
};
u32 gPlayerLookFiles_11_81[2] = {
#include "sub_main/0042CB38.inc"
};
u32 gPlayerLookFiles_11_83[2] = {
#include "sub_main/0042CB40.inc"
};
u32 gPlayerLookFiles_11_85[2] = {
#include "sub_main/0042CB48.inc"
};
u32 gPlayerLookFiles_11_87[2] = {
#include "sub_main/0042CB50.inc"
};
u32 gPlayerLookFiles_11_89[2] = {
#include "sub_main/0042CB58.inc"
};
u32 gPlayerLookFiles_11_91[2] = {
#include "sub_main/0042CB60.inc"
};
u32 gPlayerLookFiles_11_93[2] = {
#include "sub_main/0042CB68.inc"
};
u32 gPlayerLookFiles_11_95[2] = {
#include "sub_main/0042CB70.inc"
};
u32 gPlayerLookFiles_11_97[2] = {
#include "sub_main/0042CB78.inc"
};
u32 gPlayerLookFiles_11_99[2] = {
#include "sub_main/0042CB80.inc"
};
u32 gPlayerLookFiles_11_101[2] = {
#include "sub_main/0042CB88.inc"
};
u32 gPlayerLookFiles_11_103[2] = {
#include "sub_main/0042CB90.inc"
};
u32 gPlayerLookFiles_11_105[2] = {
#include "sub_main/0042CB98.inc"
};
u32 gPlayerLookFiles_11_107[2] = {
#include "sub_main/0042CBA0.inc"
};
u32 gPlayerLookFiles_11_109[2] = {
#include "sub_main/0042CBA8.inc"
};
u32 gPlayerLookFiles_11_111[2] = {
#include "sub_main/0042CBB0.inc"
};
u32 gPlayerLookFiles_11_113[2] = {
#include "sub_main/0042CBB8.inc"
};
u32 gPlayerLookFiles_11_115[2] = {
#include "sub_main/0042CBC0.inc"
};
u32 gPlayerLookFiles_11_117[2] = {
#include "sub_main/0042CBC8.inc"
};
u32 gPlayerLookFiles_11_119[2] = {
#include "sub_main/0042CBD0.inc"
};
u32 gPlayerLookFiles_11_121[2] = {
#include "sub_main/0042CBD8.inc"
};
u32 gPlayerLookFiles_11_123[2] = {
#include "sub_main/0042CBE0.inc"
};
u32 gPlayerLookFiles_11_125[2] = {
#include "sub_main/0042CBE8.inc"
};
u32 gPlayerLookFiles_11_127[2] = {
#include "sub_main/0042CBF0.inc"
};
u32 gPlayerLookFiles_11_129[2] = {
#include "sub_main/0042CBF8.inc"
};
u32 gPlayerLookFiles_11_131[2] = {
#include "sub_main/0042CC00.inc"
};
u32 gPlayerLookFiles_11_133[2] = {
#include "sub_main/0042CC08.inc"
};
u32 gPlayerLookFiles_11_135[2] = {
#include "sub_main/0042CC10.inc"
};
u32 gPlayerLookFiles_11_137[2] = {
#include "sub_main/0042CC18.inc"
};
u32 gPlayerLookFiles_11_139[2] = {
#include "sub_main/0042CC20.inc"
};
u32 gPlayerLookFiles_11_141[2] = {
#include "sub_main/0042CC28.inc"
};
u32 gPlayerLookFiles_11_143[2] = {
#include "sub_main/0042CC30.inc"
};
u32 gPlayerLookFiles_11_145[2] = {
#include "sub_main/0042CC38.inc"
};
u32 gPlayerLookFiles_11_147[2] = {
#include "sub_main/0042CC40.inc"
};
u32 gPlayerLookFiles_11_149[2] = {
#include "sub_main/0042CC48.inc"
};
u32 gPlayerLookFiles_11_151[2] = {
#include "sub_main/0042CC50.inc"
};
u32 gPlayerLookFiles_11_153[2] = {
#include "sub_main/0042CC58.inc"
};
u32 gPlayerLookFiles_11_155[2] = {
#include "sub_main/0042CC60.inc"
};
u32 gPlayerLookFiles_11_157[2] = {
#include "sub_main/0042CC68.inc"
};
u32 gPlayerLookFiles_11_159[2] = {
#include "sub_main/0042CC70.inc"
};
u32 gPlayerLookFiles_11_161[2] = {
#include "sub_main/0042CC78.inc"
};
u32 gPlayerLookFiles_11_163[2] = {
#include "sub_main/0042CC80.inc"
};
u32 gPlayerLookFiles_11_165[2] = {
#include "sub_main/0042CC88.inc"
};
u32 gPlayerLookFiles_11_167[2] = {
#include "sub_main/0042CC90.inc"
};
u32 gPlayerLookFiles_11_169[2] = {
#include "sub_main/0042CC98.inc"
};
u32 gPlayerLookFiles_11_171[2] = {
#include "sub_main/0042CCA0.inc"
};
u32 gPlayerLookFiles_11_173[2] = {
#include "sub_main/0042CCA8.inc"
};
u32 gPlayerLookFiles_11_175[2] = {
#include "sub_main/0042CCB0.inc"
};
u32 gPlayerLookFiles_11_177[2] = {
#include "sub_main/0042CCB8.inc"
};
u32 gPlayerLookFiles_11_179[2] = {
#include "sub_main/0042CCC0.inc"
};
u32 gPlayerLookFiles_11_181[2] = {
#include "sub_main/0042CCC8.inc"
};
u32 gPlayerLookFiles_11_183[2] = {
#include "sub_main/0042CCD0.inc"
};
u32 gPlayerLookFiles_11_185[2] = {
#include "sub_main/0042CCD8.inc"
};
u32 gPlayerLookFiles_11_187[2] = {
#include "sub_main/0042CCE0.inc"
};
u32 gPlayerLookFiles_11_189[2] = {
#include "sub_main/0042CCE8.inc"
};
u32 gPlayerLookFiles_11_191[2] = {
#include "sub_main/0042CCF0.inc"
};
u32 gPlayerLookFiles_11_193[2] = {
#include "sub_main/0042CCF8.inc"
};
u32 gPlayerLookFiles_11_195[2] = {
#include "sub_main/0042CD00.inc"
};
u32 gPlayerLookFiles_11_197[2] = {
#include "sub_main/0042CD08.inc"
};
u32 gPlayerLookFiles_11_199[2] = {
#include "sub_main/0042CD10.inc"
};
u32 gPlayerLookFiles_11_201[2] = {
#include "sub_main/0042CD18.inc"
};
u32 gPlayerLookFiles_11_203[2] = {
#include "sub_main/0042CD20.inc"
};
u32 gPlayerLookFiles_11_205[2] = {
#include "sub_main/0042CD28.inc"
};
u32 gPlayerLookFiles_11_207[2] = {
#include "sub_main/0042CD30.inc"
};
u32 gPlayerLookFiles_11_209[2] = {
#include "sub_main/0042CD38.inc"
};
u32 gPlayerLookFiles_11_211[2] = {
#include "sub_main/0042CD40.inc"
};
u32 gPlayerLookFiles_11_213[2] = {
#include "sub_main/0042CD48.inc"
};
u32 gPlayerLookFiles_11_215[2] = {
#include "sub_main/0042CD50.inc"
};
u32 gPlayerLookFiles_11_217[2] = {
#include "sub_main/0042CD58.inc"
};
u32 gPlayerLookFiles_11_219[2] = {
#include "sub_main/0042CD60.inc"
};
u32 gPlayerLookFiles_11_221[2] = {
#include "sub_main/0042CD68.inc"
};
u32 gPlayerLookFiles_11_223[2] = {
#include "sub_main/0042CD70.inc"
};
u32 gPlayerLookFiles_11_225[2] = {
#include "sub_main/0042CD78.inc"
};
u32 gPlayerLookFiles_11_227[2] = {
#include "sub_main/0042CD80.inc"
};
u32 gPlayerLookFiles_11_229[2] = {
#include "sub_main/0042CD88.inc"
};
u32 gPlayerLookFiles_11_231[2] = {
#include "sub_main/0042CD90.inc"
};
u32 gPlayerLookFiles_11_233[2] = {
#include "sub_main/0042CD98.inc"
};
u32 gPlayerLookFiles_11_235[2] = {
#include "sub_main/0042CDA0.inc"
};
u32 gPlayerLookFiles_11_237[2] = {
#include "sub_main/0042CDA8.inc"
};
u32 gPlayerLookFiles_11_239[2] = {
#include "sub_main/0042CDB0.inc"
};
u32 gPlayerLookFiles_11_241[2] = {
#include "sub_main/0042CDB8.inc"
};
u32 gPlayerLookFiles_11_243[2] = {
#include "sub_main/0042CDC0.inc"
};
u32 gPlayerLookFiles_11_245[2] = {
#include "sub_main/0042CDC8.inc"
};
u32 gPlayerLookFiles_11_247[2] = {
#include "sub_main/0042CDD0.inc"
};
u32 gPlayerLookFiles_11_249[2] = {
#include "sub_main/0042CDD8.inc"
};
u32 gPlayerLookFiles_11_251[2] = {
#include "sub_main/0042CDE0.inc"
};
u32 gPlayerLookFiles_11_253[2] = {
#include "sub_main/0042CDE8.inc"
};
u32 gPlayerLookFiles_11_255[2] = {
#include "sub_main/0042CDF0.inc"
};
u32 gPlayerLookFiles_11_257[2] = {
#include "sub_main/0042CDF8.inc"
};
u32 gPlayerLookFiles_11_259[2] = {
#include "sub_main/0042CE00.inc"
};
u32 gPlayerLookFiles_11_261[2] = {
#include "sub_main/0042CE08.inc"
};
u32 gPlayerLookFiles_11_263[2] = {
#include "sub_main/0042CE10.inc"
};
u32 gPlayerLookFiles_11_265[2] = {
#include "sub_main/0042CE18.inc"
};
u32 gPlayerLookFiles_11_267[2] = {
#include "sub_main/0042CE20.inc"
};
u32 gPlayerLookFiles_11_269[2] = {
#include "sub_main/0042CE28.inc"
};
u32 gPlayerLookFiles_11_271[2] = {
#include "sub_main/0042CE30.inc"
};
u32 gPlayerLookFiles_11_273[2] = {
#include "sub_main/0042CE38.inc"
};
u32 gPlayerLookFiles_11_275[2] = {
#include "sub_main/0042CE40.inc"
};
u32 gPlayerLookFiles_11_277[2] = {
#include "sub_main/0042CE48.inc"
};
u32 gPlayerLookFiles_11_279[2] = {
#include "sub_main/0042CE50.inc"
};
u32 gPlayerLookFiles_11_281[2] = {
#include "sub_main/0042CE58.inc"
};
u32 gPlayerLookFiles_11_283[2] = {
#include "sub_main/0042CE60.inc"
};
u32 gPlayerLookFiles_11_285[2] = {
#include "sub_main/0042CE68.inc"
};
u32 gPlayerLookFiles_11_287[2] = {
#include "sub_main/0042CE70.inc"
};
u32 gPlayerLookFiles_11_289[2] = {
#include "sub_main/0042CE78.inc"
};
u32 gPlayerLookFiles_11_291[2] = {
#include "sub_main/0042CE80.inc"
};
u32 gPlayerLookFiles_11_293[2] = {
#include "sub_main/0042CE88.inc"
};
u32 gPlayerLookFiles_11_295[2] = {
#include "sub_main/0042CE90.inc"
};
u32 gPlayerLookFiles_11_297[2] = {
#include "sub_main/0042CE98.inc"
};
u32 gPlayerLookFiles_11_299[2] = {
#include "sub_main/0042CEA0.inc"
};
u32 gPlayerLookFiles_11_301[2] = {
#include "sub_main/0042CEA8.inc"
};
u32 gPlayerLookFiles_11_303[2] = {
#include "sub_main/0042CEB0.inc"
};
u32 gPlayerLookFiles_11_305[2] = {
#include "sub_main/0042CEB8.inc"
};
u32 gPlayerLookFiles_11_307[2] = {
#include "sub_main/0042CEC0.inc"
};
u32 gPlayerLookFiles_11_309[2] = {
#include "sub_main/0042CEC8.inc"
};
u32 gPlayerLookFiles_11_311[2] = {
#include "sub_main/0042CED0.inc"
};
u32 gPlayerLookFiles_11_313[2] = {
#include "sub_main/0042CED8.inc"
};
u32 gPlayerLookFiles_11_315[2] = {
#include "sub_main/0042CEE0.inc"
};
u32 gPlayerLookFiles_11_317[2] = {
#include "sub_main/0042CEE8.inc"
};
u32 gPlayerLookFiles_11_319[2] = {
#include "sub_main/0042CEF0.inc"
};
u32 gPlayerLookFiles_11_321[2] = {
#include "sub_main/0042CEF8.inc"
};
u32 gPlayerLookFiles_11_323[2] = {
#include "sub_main/0042CF00.inc"
};
u32 gPlayerLookFiles_11_325[2] = {
#include "sub_main/0042CF08.inc"
};
u32 gPlayerLookFiles_11_327[2] = {
#include "sub_main/0042CF10.inc"
};
u32 gPlayerLookFiles_11_329[2] = {
#include "sub_main/0042CF18.inc"
};
u32 gPlayerLookFiles_11_331[2] = {
#include "sub_main/0042CF20.inc"
};
u32 gPlayerLookFiles_11_333[2] = {
#include "sub_main/0042CF28.inc"
};
u32 gPlayerLookFiles_11_335[2] = {
#include "sub_main/0042CF30.inc"
};
u32 gPlayerLookFiles_11_337[2] = {
#include "sub_main/0042CF38.inc"
};
u32 gPlayerLookFiles_11_339[2] = {
#include "sub_main/0042CF40.inc"
};
u32 gPlayerLookFiles_11_341[2] = {
#include "sub_main/0042CF48.inc"
};
u32 gPlayerLookFiles_11_343[2] = {
#include "sub_main/0042CF50.inc"
};
u32 gPlayerLookFiles_11_345[2] = {
#include "sub_main/0042CF58.inc"
};
u32 gPlayerLookFiles_11_347[2] = {
#include "sub_main/0042CF60.inc"
};
u32 gPlayerLookFiles_11_349[2] = {
#include "sub_main/0042CF68.inc"
};
u32 gPlayerLookFiles_11_351[2] = {
#include "sub_main/0042CF70.inc"
};
u32 gPlayerLookFiles_11_353[2] = {
#include "sub_main/0042CF78.inc"
};
u32 gPlayerLookFiles_11_355[2] = {
#include "sub_main/0042CF80.inc"
};
u32 gPlayerLookFiles_11_357[2] = {
#include "sub_main/0042CF88.inc"
};
u32 gPlayerLookFiles_11_359[2] = {
#include "sub_main/0042CF90.inc"
};
u32 gPlayerLookFiles_11_361[2] = {
#include "sub_main/0042CF98.inc"
};
u32 gPlayerLookFiles_11_363[2] = {
#include "sub_main/0042CFA0.inc"
};
u32 gPlayerLookFiles_11_365[2] = {
#include "sub_main/0042CFA8.inc"
};
u32 gPlayerLookFiles_11_367[2] = {
#include "sub_main/0042CFB0.inc"
};
u32 gPlayerLookFiles_11_369[2] = {
#include "sub_main/0042CFB8.inc"
};
u32 gPlayerLookFiles_11_371[2] = {
#include "sub_main/0042CFC0.inc"
};
u32 gPlayerLookFiles_11_373[2] = {
#include "sub_main/0042CFC8.inc"
};
u32 gPlayerLookFiles_11_375[2] = {
#include "sub_main/0042CFD0.inc"
};
u32 gPlayerLookFiles_11_377[2] = {
#include "sub_main/0042CFD8.inc"
};
u32 gPlayerLookFiles_11_379[2] = {
#include "sub_main/0042CFE0.inc"
};
u32 gPlayerLookFiles_11_381[2] = {
#include "sub_main/0042CFE8.inc"
};
u32 gPlayerLookFiles_11_383[2] = {
#include "sub_main/0042CFF0.inc"
};
u32 gPlayerLookFiles_11_385[2] = {
#include "sub_main/0042CFF8.inc"
};
u32 gPlayerLookFiles_11_387[2] = {
#include "sub_main/0042D000.inc"
};
u32 gPlayerLookFiles_11_389[2] = {
#include "sub_main/0042D008.inc"
};
u32 gPlayerLookFiles_11_391[2] = {
#include "sub_main/0042D010.inc"
};
u32 gPlayerLookFiles_11_393[2] = {
#include "sub_main/0042D018.inc"
};
u32 gPlayerLookFiles_11_395[2] = {
#include "sub_main/0042D020.inc"
};
u32 gPlayerLookFiles_11_397[2] = {
#include "sub_main/0042D028.inc"
};
u32 gPlayerLookFiles_11_399[2] = {
#include "sub_main/0042D030.inc"
};
u32 gPlayerLookFiles_11_401[2] = {
#include "sub_main/0042D038.inc"
};
u32 gPlayerLookFiles_11_403[2] = {
#include "sub_main/0042D040.inc"
};
u32 gPlayerLookFiles_11_405[2] = {
#include "sub_main/0042D048.inc"
};
u32 gPlayerLookFiles_11_407[2] = {
#include "sub_main/0042D050.inc"
};
u32 gPlayerLookFiles_11_409[2] = {
#include "sub_main/0042D058.inc"
};
u32 gPlayerLookFiles_11_411[2] = {
#include "sub_main/0042D060.inc"
};
u32 gPlayerLookFiles_11_413[2] = {
#include "sub_main/0042D068.inc"
};
u32 gPlayerLookFiles_11_415[2] = {
#include "sub_main/0042D070.inc"
};
u32 gPlayerLookFiles_11_417[2] = {
#include "sub_main/0042D078.inc"
};
u32 gPlayerLookFiles_11_419[2] = {
#include "sub_main/0042D080.inc"
};
u32 gPlayerLookFiles_11_421[2] = {
#include "sub_main/0042D088.inc"
};
u32 gPlayerLookFiles_11_423[2] = {
#include "sub_main/0042D090.inc"
};
u32 gPlayerLookFiles_11_425[2] = {
#include "sub_main/0042D098.inc"
};
u32 gPlayerLookFiles_11_427[2] = {
#include "sub_main/0042D0A0.inc"
};
u32 gPlayerLookFiles_11_429[2] = {
#include "sub_main/0042D0A8.inc"
};
u32 gPlayerLookFiles_11_431[2] = {
#include "sub_main/0042D0B0.inc"
};
u32 gPlayerLookFiles_11_433[2] = {
#include "sub_main/0042D0B8.inc"
};
u32 gPlayerLookFiles_11_435[2] = {
#include "sub_main/0042D0C0.inc"
};
u32 gPlayerLookFiles_11_437[2] = {
#include "sub_main/0042D0C8.inc"
};
u32 gPlayerLookFiles_11_439[2] = {
#include "sub_main/0042D0D0.inc"
};
u32 gPlayerLookFiles_11_441[2] = {
#include "sub_main/0042D0D8.inc"
};
u32 gPlayerLookFiles_11_443[2] = {
#include "sub_main/0042D0E0.inc"
};
u32 gPlayerLookFiles_11_445[2] = {
#include "sub_main/0042D0E8.inc"
};
u32 gPlayerLookFiles_11_447[2] = {
#include "sub_main/0042D0F0.inc"
};
u32 gPlayerLookFiles_11_449[2] = {
#include "sub_main/0042D0F8.inc"
};
u32 gPlayerLookFiles_11_451[2] = {
#include "sub_main/0042D100.inc"
};
u32 gPlayerLookFiles_11_453[2] = {
#include "sub_main/0042D108.inc"
};
u32 gPlayerLookFiles_11_455[4] = {
#include "sub_main/0042D110.inc"
};
u32 gPlayerLookFiles_11[456] = {
#include "sub_main/0042D120.inc"
};
u32 gPlayerLookFiles_12_1[2] = {
#include "sub_main/0042D840.inc"
};
u32 gPlayerLookFiles_12_3[2] = {
#include "sub_main/0042D848.inc"
};
u32 gPlayerLookFiles_12_5[2] = {
#include "sub_main/0042D850.inc"
};
u32 gPlayerLookFiles_12_7[2] = {
#include "sub_main/0042D858.inc"
};
u32 gPlayerLookFiles_12_9[2] = {
#include "sub_main/0042D860.inc"
};
u32 gPlayerLookFiles_12_11[2] = {
#include "sub_main/0042D868.inc"
};
u32 gPlayerLookFiles_12_13[2] = {
#include "sub_main/0042D870.inc"
};
u32 gPlayerLookFiles_12_15[2] = {
#include "sub_main/0042D878.inc"
};
u32 gPlayerLookFiles_12_17[2] = {
#include "sub_main/0042D880.inc"
};
u32 gPlayerLookFiles_12_19[2] = {
#include "sub_main/0042D888.inc"
};
u32 gPlayerLookFiles_12_21[2] = {
#include "sub_main/0042D890.inc"
};
u32 gPlayerLookFiles_12_23[2] = {
#include "sub_main/0042D898.inc"
};
u32 gPlayerLookFiles_12_25[2] = {
#include "sub_main/0042D8A0.inc"
};
u32 gPlayerLookFiles_12_27[2] = {
#include "sub_main/0042D8A8.inc"
};
u32 gPlayerLookFiles_12_29[2] = {
#include "sub_main/0042D8B0.inc"
};
u32 gPlayerLookFiles_12_31[2] = {
#include "sub_main/0042D8B8.inc"
};
u32 gPlayerLookFiles_12_33[2] = {
#include "sub_main/0042D8C0.inc"
};
u32 gPlayerLookFiles_12_35[2] = {
#include "sub_main/0042D8C8.inc"
};
u32 gPlayerLookFiles_12_37[2] = {
#include "sub_main/0042D8D0.inc"
};
u32 gPlayerLookFiles_12_39[2] = {
#include "sub_main/0042D8D8.inc"
};
u32 gPlayerLookFiles_12_41[2] = {
#include "sub_main/0042D8E0.inc"
};
u32 gPlayerLookFiles_12_43[2] = {
#include "sub_main/0042D8E8.inc"
};
u32 gPlayerLookFiles_12_45[2] = {
#include "sub_main/0042D8F0.inc"
};
u32 gPlayerLookFiles_12_47[2] = {
#include "sub_main/0042D8F8.inc"
};
u32 gPlayerLookFiles_12_49[2] = {
#include "sub_main/0042D900.inc"
};
u32 gPlayerLookFiles_12_51[2] = {
#include "sub_main/0042D908.inc"
};
u32 gPlayerLookFiles_12_53[2] = {
#include "sub_main/0042D910.inc"
};
u32 gPlayerLookFiles_12_55[2] = {
#include "sub_main/0042D918.inc"
};
u32 gPlayerLookFiles_12_57[2] = {
#include "sub_main/0042D920.inc"
};
u32 gPlayerLookFiles_12_59[2] = {
#include "sub_main/0042D928.inc"
};
u32 gPlayerLookFiles_12_61[2] = {
#include "sub_main/0042D930.inc"
};
u32 gPlayerLookFiles_12_63[2] = {
#include "sub_main/0042D938.inc"
};
u32 gPlayerLookFiles_12_65[2] = {
#include "sub_main/0042D940.inc"
};
u32 gPlayerLookFiles_12_67[2] = {
#include "sub_main/0042D948.inc"
};
u32 gPlayerLookFiles_12_69[2] = {
#include "sub_main/0042D950.inc"
};
u32 gPlayerLookFiles_12_71[2] = {
#include "sub_main/0042D958.inc"
};
u32 gPlayerLookFiles_12_73[2] = {
#include "sub_main/0042D960.inc"
};
u32 gPlayerLookFiles_12_75[2] = {
#include "sub_main/0042D968.inc"
};
u32 gPlayerLookFiles_12_77[2] = {
#include "sub_main/0042D970.inc"
};
u32 gPlayerLookFiles_12_79[2] = {
#include "sub_main/0042D978.inc"
};
u32 gPlayerLookFiles_12_81[2] = {
#include "sub_main/0042D980.inc"
};
u32 gPlayerLookFiles_12_83[2] = {
#include "sub_main/0042D988.inc"
};
u32 gPlayerLookFiles_12_85[2] = {
#include "sub_main/0042D990.inc"
};
u32 gPlayerLookFiles_12_87[2] = {
#include "sub_main/0042D998.inc"
};
u32 gPlayerLookFiles_12_89[2] = {
#include "sub_main/0042D9A0.inc"
};
u32 gPlayerLookFiles_12_91[2] = {
#include "sub_main/0042D9A8.inc"
};
u32 gPlayerLookFiles_12_93[2] = {
#include "sub_main/0042D9B0.inc"
};
u32 gPlayerLookFiles_12_95[2] = {
#include "sub_main/0042D9B8.inc"
};
u32 gPlayerLookFiles_12_97[2] = {
#include "sub_main/0042D9C0.inc"
};
u32 gPlayerLookFiles_12_99[2] = {
#include "sub_main/0042D9C8.inc"
};
u32 gPlayerLookFiles_12_101[2] = {
#include "sub_main/0042D9D0.inc"
};
u32 gPlayerLookFiles_12_103[2] = {
#include "sub_main/0042D9D8.inc"
};
u32 gPlayerLookFiles_12_105[2] = {
#include "sub_main/0042D9E0.inc"
};
u32 gPlayerLookFiles_12_107[2] = {
#include "sub_main/0042D9E8.inc"
};
u32 gPlayerLookFiles_12_109[2] = {
#include "sub_main/0042D9F0.inc"
};
u32 gPlayerLookFiles_12_111[2] = {
#include "sub_main/0042D9F8.inc"
};
u32 gPlayerLookFiles_12_113[2] = {
#include "sub_main/0042DA00.inc"
};
u32 gPlayerLookFiles_12_115[2] = {
#include "sub_main/0042DA08.inc"
};
u32 gPlayerLookFiles_12_117[2] = {
#include "sub_main/0042DA10.inc"
};
u32 gPlayerLookFiles_12_119[2] = {
#include "sub_main/0042DA18.inc"
};
u32 gPlayerLookFiles_12_121[2] = {
#include "sub_main/0042DA20.inc"
};
u32 gPlayerLookFiles_12_123[2] = {
#include "sub_main/0042DA28.inc"
};
u32 gPlayerLookFiles_12_125[2] = {
#include "sub_main/0042DA30.inc"
};
u32 gPlayerLookFiles_12_127[2] = {
#include "sub_main/0042DA38.inc"
};
u32 gPlayerLookFiles_12_129[2] = {
#include "sub_main/0042DA40.inc"
};
u32 gPlayerLookFiles_12_131[2] = {
#include "sub_main/0042DA48.inc"
};
u32 gPlayerLookFiles_12_133[2] = {
#include "sub_main/0042DA50.inc"
};
u32 gPlayerLookFiles_12_135[2] = {
#include "sub_main/0042DA58.inc"
};
u32 gPlayerLookFiles_12_137[2] = {
#include "sub_main/0042DA60.inc"
};
u32 gPlayerLookFiles_12_139[2] = {
#include "sub_main/0042DA68.inc"
};
u32 gPlayerLookFiles_12_141[2] = {
#include "sub_main/0042DA70.inc"
};
u32 gPlayerLookFiles_12_143[2] = {
#include "sub_main/0042DA78.inc"
};
u32 gPlayerLookFiles_12_145[2] = {
#include "sub_main/0042DA80.inc"
};
u32 gPlayerLookFiles_12_147[2] = {
#include "sub_main/0042DA88.inc"
};
u32 gPlayerLookFiles_12_149[2] = {
#include "sub_main/0042DA90.inc"
};
u32 gPlayerLookFiles_12_151[2] = {
#include "sub_main/0042DA98.inc"
};
u32 gPlayerLookFiles_12_153[2] = {
#include "sub_main/0042DAA0.inc"
};
u32 gPlayerLookFiles_12_155[2] = {
#include "sub_main/0042DAA8.inc"
};
u32 gPlayerLookFiles_12_157[2] = {
#include "sub_main/0042DAB0.inc"
};
u32 gPlayerLookFiles_12_159[2] = {
#include "sub_main/0042DAB8.inc"
};
u32 gPlayerLookFiles_12_161[2] = {
#include "sub_main/0042DAC0.inc"
};
u32 gPlayerLookFiles_12_163[2] = {
#include "sub_main/0042DAC8.inc"
};
u32 gPlayerLookFiles_12_165[2] = {
#include "sub_main/0042DAD0.inc"
};
u32 gPlayerLookFiles_12_167[2] = {
#include "sub_main/0042DAD8.inc"
};
u32 gPlayerLookFiles_12_169[2] = {
#include "sub_main/0042DAE0.inc"
};
u32 gPlayerLookFiles_12_171[2] = {
#include "sub_main/0042DAE8.inc"
};
u32 gPlayerLookFiles_12_173[2] = {
#include "sub_main/0042DAF0.inc"
};
u32 gPlayerLookFiles_12_175[2] = {
#include "sub_main/0042DAF8.inc"
};
u32 gPlayerLookFiles_12_177[2] = {
#include "sub_main/0042DB00.inc"
};
u32 gPlayerLookFiles_12_179[2] = {
#include "sub_main/0042DB08.inc"
};
u32 gPlayerLookFiles_12_181[2] = {
#include "sub_main/0042DB10.inc"
};
u32 gPlayerLookFiles_12_183[2] = {
#include "sub_main/0042DB18.inc"
};
u32 gPlayerLookFiles_12_185[2] = {
#include "sub_main/0042DB20.inc"
};
u32 gPlayerLookFiles_12_187[2] = {
#include "sub_main/0042DB28.inc"
};
u32 gPlayerLookFiles_12_189[2] = {
#include "sub_main/0042DB30.inc"
};
u32 gPlayerLookFiles_12_191[2] = {
#include "sub_main/0042DB38.inc"
};
u32 gPlayerLookFiles_12_193[2] = {
#include "sub_main/0042DB40.inc"
};
u32 gPlayerLookFiles_12_195[2] = {
#include "sub_main/0042DB48.inc"
};
u32 gPlayerLookFiles_12_197[2] = {
#include "sub_main/0042DB50.inc"
};
u32 gPlayerLookFiles_12_199[2] = {
#include "sub_main/0042DB58.inc"
};
u32 gPlayerLookFiles_12_201[2] = {
#include "sub_main/0042DB60.inc"
};
u32 gPlayerLookFiles_12_203[2] = {
#include "sub_main/0042DB68.inc"
};
u32 gPlayerLookFiles_12_205[2] = {
#include "sub_main/0042DB70.inc"
};
u32 gPlayerLookFiles_12_207[2] = {
#include "sub_main/0042DB78.inc"
};
u32 gPlayerLookFiles_12_209[2] = {
#include "sub_main/0042DB80.inc"
};
u32 gPlayerLookFiles_12_211[2] = {
#include "sub_main/0042DB88.inc"
};
u32 gPlayerLookFiles_12_213[2] = {
#include "sub_main/0042DB90.inc"
};
u32 gPlayerLookFiles_12_215[2] = {
#include "sub_main/0042DB98.inc"
};
u32 gPlayerLookFiles_12_217[2] = {
#include "sub_main/0042DBA0.inc"
};
u32 gPlayerLookFiles_12_219[2] = {
#include "sub_main/0042DBA8.inc"
};
u32 gPlayerLookFiles_12_221[2] = {
#include "sub_main/0042DBB0.inc"
};
u32 gPlayerLookFiles_12_223[2] = {
#include "sub_main/0042DBB8.inc"
};
u32 gPlayerLookFiles_12_225[2] = {
#include "sub_main/0042DBC0.inc"
};
u32 gPlayerLookFiles_12_227[2] = {
#include "sub_main/0042DBC8.inc"
};
u32 gPlayerLookFiles_12_229[2] = {
#include "sub_main/0042DBD0.inc"
};
u32 gPlayerLookFiles_12_231[2] = {
#include "sub_main/0042DBD8.inc"
};
u32 gPlayerLookFiles_12_233[2] = {
#include "sub_main/0042DBE0.inc"
};
u32 gPlayerLookFiles_12_235[2] = {
#include "sub_main/0042DBE8.inc"
};
u32 gPlayerLookFiles_12_237[2] = {
#include "sub_main/0042DBF0.inc"
};
u32 gPlayerLookFiles_12_239[2] = {
#include "sub_main/0042DBF8.inc"
};
u32 gPlayerLookFiles_12_241[2] = {
#include "sub_main/0042DC00.inc"
};
u32 gPlayerLookFiles_12_243[2] = {
#include "sub_main/0042DC08.inc"
};
u32 gPlayerLookFiles_12_245[2] = {
#include "sub_main/0042DC10.inc"
};
u32 gPlayerLookFiles_12_247[2] = {
#include "sub_main/0042DC18.inc"
};
u32 gPlayerLookFiles_12_249[2] = {
#include "sub_main/0042DC20.inc"
};
u32 gPlayerLookFiles_12_251[2] = {
#include "sub_main/0042DC28.inc"
};
u32 gPlayerLookFiles_12_253[2] = {
#include "sub_main/0042DC30.inc"
};
u32 gPlayerLookFiles_12_255[2] = {
#include "sub_main/0042DC38.inc"
};
u32 gPlayerLookFiles_12_257[2] = {
#include "sub_main/0042DC40.inc"
};
u32 gPlayerLookFiles_12_259[2] = {
#include "sub_main/0042DC48.inc"
};
u32 gPlayerLookFiles_12_261[2] = {
#include "sub_main/0042DC50.inc"
};
u32 gPlayerLookFiles_12_263[2] = {
#include "sub_main/0042DC58.inc"
};
u32 gPlayerLookFiles_12_265[2] = {
#include "sub_main/0042DC60.inc"
};
u32 gPlayerLookFiles_12_267[2] = {
#include "sub_main/0042DC68.inc"
};
u32 gPlayerLookFiles_12_269[2] = {
#include "sub_main/0042DC70.inc"
};
u32 gPlayerLookFiles_12_271[2] = {
#include "sub_main/0042DC78.inc"
};
u32 gPlayerLookFiles_12_273[2] = {
#include "sub_main/0042DC80.inc"
};
u32 gPlayerLookFiles_12_275[2] = {
#include "sub_main/0042DC88.inc"
};
u32 gPlayerLookFiles_12_277[2] = {
#include "sub_main/0042DC90.inc"
};
u32 gPlayerLookFiles_12_279[2] = {
#include "sub_main/0042DC98.inc"
};
u32 gPlayerLookFiles_12_281[2] = {
#include "sub_main/0042DCA0.inc"
};
u32 gPlayerLookFiles_12_283[2] = {
#include "sub_main/0042DCA8.inc"
};
u32 gPlayerLookFiles_12_285[2] = {
#include "sub_main/0042DCB0.inc"
};
u32 gPlayerLookFiles_12_287[2] = {
#include "sub_main/0042DCB8.inc"
};
u32 gPlayerLookFiles_12_289[2] = {
#include "sub_main/0042DCC0.inc"
};
u32 gPlayerLookFiles_12_291[2] = {
#include "sub_main/0042DCC8.inc"
};
u32 gPlayerLookFiles_12_293[2] = {
#include "sub_main/0042DCD0.inc"
};
u32 gPlayerLookFiles_12_295[2] = {
#include "sub_main/0042DCD8.inc"
};
u32 gPlayerLookFiles_12_297[2] = {
#include "sub_main/0042DCE0.inc"
};
u32 gPlayerLookFiles_12_299[2] = {
#include "sub_main/0042DCE8.inc"
};
u32 gPlayerLookFiles_12_301[2] = {
#include "sub_main/0042DCF0.inc"
};
u32 gPlayerLookFiles_12_303[2] = {
#include "sub_main/0042DCF8.inc"
};
u32 gPlayerLookFiles_12_305[2] = {
#include "sub_main/0042DD00.inc"
};
u32 gPlayerLookFiles_12_307[2] = {
#include "sub_main/0042DD08.inc"
};
u32 gPlayerLookFiles_12_309[2] = {
#include "sub_main/0042DD10.inc"
};
u32 gPlayerLookFiles_12_311[2] = {
#include "sub_main/0042DD18.inc"
};
u32 gPlayerLookFiles_12_313[2] = {
#include "sub_main/0042DD20.inc"
};
u32 gPlayerLookFiles_12_315[2] = {
#include "sub_main/0042DD28.inc"
};
u32 gPlayerLookFiles_12_317[2] = {
#include "sub_main/0042DD30.inc"
};
u32 gPlayerLookFiles_12_319[2] = {
#include "sub_main/0042DD38.inc"
};
u32 gPlayerLookFiles_12_321[2] = {
#include "sub_main/0042DD40.inc"
};
u32 gPlayerLookFiles_12_323[2] = {
#include "sub_main/0042DD48.inc"
};
u32 gPlayerLookFiles_12_325[2] = {
#include "sub_main/0042DD50.inc"
};
u32 gPlayerLookFiles_12_327[2] = {
#include "sub_main/0042DD58.inc"
};
u32 gPlayerLookFiles_12_329[2] = {
#include "sub_main/0042DD60.inc"
};
u32 gPlayerLookFiles_12_331[2] = {
#include "sub_main/0042DD68.inc"
};
u32 gPlayerLookFiles_12_333[2] = {
#include "sub_main/0042DD70.inc"
};
u32 gPlayerLookFiles_12_335[2] = {
#include "sub_main/0042DD78.inc"
};
u32 gPlayerLookFiles_12_337[2] = {
#include "sub_main/0042DD80.inc"
};
u32 gPlayerLookFiles_12_339[2] = {
#include "sub_main/0042DD88.inc"
};
u32 gPlayerLookFiles_12_341[2] = {
#include "sub_main/0042DD90.inc"
};
u32 gPlayerLookFiles_12_343[2] = {
#include "sub_main/0042DD98.inc"
};
u32 gPlayerLookFiles_12_345[2] = {
#include "sub_main/0042DDA0.inc"
};
u32 gPlayerLookFiles_12_347[2] = {
#include "sub_main/0042DDA8.inc"
};
u32 gPlayerLookFiles_12_349[2] = {
#include "sub_main/0042DDB0.inc"
};
u32 gPlayerLookFiles_12_351[2] = {
#include "sub_main/0042DDB8.inc"
};
u32 gPlayerLookFiles_12_353[2] = {
#include "sub_main/0042DDC0.inc"
};
u32 gPlayerLookFiles_12_355[2] = {
#include "sub_main/0042DDC8.inc"
};
u32 gPlayerLookFiles_12_357[2] = {
#include "sub_main/0042DDD0.inc"
};
u32 gPlayerLookFiles_12_359[2] = {
#include "sub_main/0042DDD8.inc"
};
u32 gPlayerLookFiles_12_361[2] = {
#include "sub_main/0042DDE0.inc"
};
u32 gPlayerLookFiles_12_363[2] = {
#include "sub_main/0042DDE8.inc"
};
u32 gPlayerLookFiles_12_365[2] = {
#include "sub_main/0042DDF0.inc"
};
u32 gPlayerLookFiles_12_367[2] = {
#include "sub_main/0042DDF8.inc"
};
u32 gPlayerLookFiles_12_369[2] = {
#include "sub_main/0042DE00.inc"
};
u32 gPlayerLookFiles_12_371[2] = {
#include "sub_main/0042DE08.inc"
};
u32 gPlayerLookFiles_12_373[2] = {
#include "sub_main/0042DE10.inc"
};
u32 gPlayerLookFiles_12_375[2] = {
#include "sub_main/0042DE18.inc"
};
u32 gPlayerLookFiles_12_377[2] = {
#include "sub_main/0042DE20.inc"
};
u32 gPlayerLookFiles_12_379[2] = {
#include "sub_main/0042DE28.inc"
};
u32 gPlayerLookFiles_12_381[2] = {
#include "sub_main/0042DE30.inc"
};
u32 gPlayerLookFiles_12_383[2] = {
#include "sub_main/0042DE38.inc"
};
u32 gPlayerLookFiles_12_385[2] = {
#include "sub_main/0042DE40.inc"
};
u32 gPlayerLookFiles_12_387[2] = {
#include "sub_main/0042DE48.inc"
};
u32 gPlayerLookFiles_12_389[2] = {
#include "sub_main/0042DE50.inc"
};
u32 gPlayerLookFiles_12_391[2] = {
#include "sub_main/0042DE58.inc"
};
u32 gPlayerLookFiles_12_393[2] = {
#include "sub_main/0042DE60.inc"
};
u32 gPlayerLookFiles_12_395[2] = {
#include "sub_main/0042DE68.inc"
};
u32 gPlayerLookFiles_12_397[2] = {
#include "sub_main/0042DE70.inc"
};
u32 gPlayerLookFiles_12_399[2] = {
#include "sub_main/0042DE78.inc"
};
u32 gPlayerLookFiles_12_401[2] = {
#include "sub_main/0042DE80.inc"
};
u32 gPlayerLookFiles_12_403[2] = {
#include "sub_main/0042DE88.inc"
};
u32 gPlayerLookFiles_12_405[2] = {
#include "sub_main/0042DE90.inc"
};
u32 gPlayerLookFiles_12_407[2] = {
#include "sub_main/0042DE98.inc"
};
u32 gPlayerLookFiles_12_409[2] = {
#include "sub_main/0042DEA0.inc"
};
u32 gPlayerLookFiles_12_411[2] = {
#include "sub_main/0042DEA8.inc"
};
u32 gPlayerLookFiles_12_413[4] = {
#include "sub_main/0042DEB0.inc"
};
u32 gPlayerLookFiles_12[414] = {
#include "sub_main/0042DEC0.inc"
};
u32 gPlayerLookFiles_13_1[2] = {
#include "sub_main/0042E538.inc"
};
u32 gPlayerLookFiles_13_3[2] = {
#include "sub_main/0042E540.inc"
};
u32 gPlayerLookFiles_13_5[2] = {
#include "sub_main/0042E548.inc"
};
u32 gPlayerLookFiles_13_7[2] = {
#include "sub_main/0042E550.inc"
};
u32 gPlayerLookFiles_13_9[2] = {
#include "sub_main/0042E558.inc"
};
u32 gPlayerLookFiles_13_11[2] = {
#include "sub_main/0042E560.inc"
};
u32 gPlayerLookFiles_13_13[2] = {
#include "sub_main/0042E568.inc"
};
u32 gPlayerLookFiles_13_15[2] = {
#include "sub_main/0042E570.inc"
};
u32 gPlayerLookFiles_13_17[2] = {
#include "sub_main/0042E578.inc"
};
u32 gPlayerLookFiles_13_19[2] = {
#include "sub_main/0042E580.inc"
};
u32 gPlayerLookFiles_13_21[2] = {
#include "sub_main/0042E588.inc"
};
u32 gPlayerLookFiles_13_23[2] = {
#include "sub_main/0042E590.inc"
};
u32 gPlayerLookFiles_13_25[2] = {
#include "sub_main/0042E598.inc"
};
u32 gPlayerLookFiles_13_27[2] = {
#include "sub_main/0042E5A0.inc"
};
u32 gPlayerLookFiles_13_29[2] = {
#include "sub_main/0042E5A8.inc"
};
u32 gPlayerLookFiles_13_31[2] = {
#include "sub_main/0042E5B0.inc"
};
u32 gPlayerLookFiles_13_33[2] = {
#include "sub_main/0042E5B8.inc"
};
u32 gPlayerLookFiles_13_35[2] = {
#include "sub_main/0042E5C0.inc"
};
u32 gPlayerLookFiles_13_37[2] = {
#include "sub_main/0042E5C8.inc"
};
u32 gPlayerLookFiles_13_39[2] = {
#include "sub_main/0042E5D0.inc"
};
u32 gPlayerLookFiles_13_41[2] = {
#include "sub_main/0042E5D8.inc"
};
u32 gPlayerLookFiles_13[44] = {
#include "sub_main/0042E5E0.inc"
};
u32 gEmSlotTexPacks[76] = {
#include "sub_main/0042E690.inc"
};
u32 gEmSlotModelPacks[76] = {
#include "sub_main/0042E7C0.inc"
};
s32 gModelPackIds[340] = {
#include "sub_main/0042E8F0.inc"
};
u32 gAreaSubModelTexFiles[198] = {
#include "sub_main/0042EE40.inc"
};
u32 gArea83SubModelTexFiles[2] = {
#include "sub_main/0042F158.inc"
};
u32 gArea83SubModelTexFileS1[4] = {
#include "sub_main/0042F160.inc"
};
u32 gArea89SubModelTexFiles[12] = {
#include "sub_main/0042F170.inc"
};
s32 gSystemTexFiles[6] = {
#include "sub_main/0042F1A0.inc"
};
u32 D_0042F1B8[2] = {
#include "sub_main/0042F1B8.inc"
};
u32 gCommonModelTexFiles[2] = {
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
u32 gTexFileIds[4] = {
#include "sub_main/0042F300.inc"
};
u32 gTexFileIds8[4] = {
#include "sub_main/0042F310.inc"
};
u32 gStageTexFileIds[16] = {
#include "sub_main/0042F320.inc"
};
s16 gTexByIdFiles[8] = {
#include "sub_main/0042F360.inc"
};
u32 gArea83PackFiles[2] = {
#include "sub_main/0042F370.inc"
};
u32 gArea83SubModelFiles[2] = {
#include "sub_main/0042F378.inc"
};
u32 gArea83SubModelFileS1[2] = {
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
u32 gMapPackTexIndex[8] = {
#include "sub_main/0042F3C0.inc"
};
u32 gCommonPackTexSlots[2] = {
#include "sub_main/0042F3E0.inc"
};
u32 gSystemPackTexSlots[2] = {
#include "sub_main/0042F3E8.inc"
};
u32 gSystemPackTexSlotsB[4] = {
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
FxRange gEquipFxRanges[8] = {
#include "sub_main/0042F4A0.inc"
};
f32 gEquipFxUps[4] = {
#include "sub_main/0042F4C0.inc"
};
f32 gEquipFxBases[14][3] = {
#include "sub_main/0042F4D0.inc"
};
f32 gEquipFxLimits[12] = {
#include "sub_main/0042F580.inc"
};
f32 gEquipFxTargets[4][3] = {
#include "sub_main/0042F5B0.inc"
};
f32 gEquipFxClamps[16][6] = {
#include "sub_main/0042F5E0.inc"
};
f32 gEquipFxDamps[4] = {
#include "sub_main/0042F760.inc"
};
f32 gEquipFxPulls[4] = {
#include "sub_main/0042F770.inc"
};
u32 gEffectListPoints[224] = {
#include "sub_main/0042F780.inc"
};
s32 gMatSetState15[12] = {
#include "sub_main/0042FB00.inc"
};
s32 gMatSetState12[4] = {
#include "sub_main/0042FB30.inc"
};
s32 gMatSetModelKinds[4] = {
#include "sub_main/0042FB40.inc"
};
u32 gAreaSubModelKinds[2] = {
#include "sub_main/0042FB50.inc"
};
s32 gDrawState15Values[2] = {
#include "sub_main/0042FB58.inc"
};
s32 gDrawState12Values[4] = {
#include "sub_main/0042FB60.inc"
};
s32 gDrawState01Values[12] = {
#include "sub_main/0042FB70.inc"
};
s32 gDrawState02Values[4] = {
#include "sub_main/0042FBA0.inc"
};
s32 gDrawState0CValues[2] = {
#include "sub_main/0042FBB0.inc"
};
s32 gDrawState66Values[2] = {
#include "sub_main/0042FBB8.inc"
};
s32 gDrawState62Values[4] = {
#include "sub_main/0042FBC0.inc"
};
s32 gDrawState00Values[4] = {
#include "sub_main/0042FBD0.inc"
};
s32 gDrawBlendFactors[10] = {
#include "sub_main/0042FBE0.inc"
};
s32 gDrawBlendOps[3] = {
#include "sub_main/0042FC08.inc"
};
s32 gDrawState63Values[2] = {
#include "sub_main/0042FC18.inc"
};
s32 gDrawState64Values[4] = {
#include "sub_main/0042FC20.inc"
};
