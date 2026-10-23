/*
 * The players' looks, and the file id tables of the models and textures.
 * - A look (PlayerLook: equipment and colours) is queued
 *   (Player_QueueLookFiles), loaded into gReadBuf by the file loader,
 *   then applied (Player_ApplyLook, Player_LoadLookModels).
 * - Before them: the stage's model files (Stage_LoadFilePair), an
 *   area's camera data (Area_LoadCamData), LzDecompressSized, the
 *   frame's texture marks (Tex_Prepare) and an empty hook the quest's
 *   set-up calls (Quest_EmptyHook).
 * Then the file's data: the file id tables of areas, equipment, texture
 * packs, player looks and sub-models.
 */
#include "common.h"
#include "player.h"
#include "model.h"
#include "lobby_state.h"
#include "syssettings.h"
#include "texture.h"
#include "overlay.h"
#include "camera.h"
#include "stream.h"
#include "file_load.h"
#include "main.h"
#include "mem_util.h"
#include "packs.h"
#include "tex_files.h"

/* prototypes */
extern s32 gAreaCamDataFiles[];
/* end of prototypes */

extern FilePair gStageFilePairs[];
extern u8 *gModelLoadBuf;

/* Loads stage file pair i into gModelLoadBuf (FilePair). */
void Stage_LoadFilePair(s32 i)
{
    u8 *buf = gModelLoadBuf;

    LoadFile(buf, gStageFilePairs[i].first);
    LoadFile(buf + MODEL_LOAD_ALT_SKEL_OFFSET, gStageFilePairs[i].second);
}

/* An empty function: the quest's set-up calls it last,
 * and plsel calls it too. */
void Quest_EmptyHook(void)
{
}

extern u32 gTexHandles[];
extern s16 gTexPrepFrameCount;
extern s16 gTexPrepLastCount;
extern s16 gTexPrepLast;

/* Marks the n texture handles from gTexHandles[tex] in use this frame
 * (Tex_Use), and counts them in gTexPrepFrameCount. */
void Tex_Prepare(s32 n, s32 tex)
{
    if (n != 0) {
        Tex_Use(n, &gTexHandles[tex]);
        gTexPrepLast = tex;
        gTexPrepLastCount = n;
        gTexPrepFrameCount += n;
    }
}

/* Loads area's camera data file (gAreaCamDataFiles; 0: none) into
 * gSysSettings.camData and relocates it, or relocates NULL when the area
 * has none. */
void Area_LoadCamData(s32 area)
{
    s32 id;
    s32 camData;

    id = gAreaCamDataFiles[area];
    camData = 0;
    if (id != 0) {
        LoadCompressedFile(gSysSettings.camData, id);
        camData = (s32)gSysSettings.camData;
    }
    CamData_Relocate((void *) camData);
}

/* LzDecompress; size, the buffer's, is not used. */
s32 LzDecompressSized(u8 *src, u8 *dst, s32 size)
{
    return LzDecompress(src, dst);
}

extern u8 gLookDelay[];

/* Clears every player's look delay (gLookDelay): no look is pending. */
void PlayerLook_ResetLoaded(void)
{
    s16 i;

    for (i = 0; i < 10; i++) {
        gLookDelay[i] = 0;
    }
}

/* A player's equipment and colours as sent between players (94 bytes; gLookPending has
 * one per player). */
typedef struct PlayerLook {
    EquipItem weapon;   /* 0x00 */
    u16 kind;           /* 0x0C: the weapon kind (Player.unk30C) */
    EquipItem armour[6];/* 0x0E: Player.equip[0-5] */
    u8 colour[6];       /* 0x56: Player.armourKind */
    u8 variant;         /* 0x5C: Player.lookVariant (Player_BuildPartIds) */
    u8 pad5D;
} PlayerLook;
u8 WeaponClass_IsGunner(u8 cls);
u8 WeaponType_Flags(u8 cls);
u8 Equip_GetBladeSpecial(EquipItem *e);
extern u8 gWeaponKindClass[];

/* Puts look l on player p: the equipment, colours and weapon, and what depends on the weapon. */
void Player_SetLook(Player *pl, PlayerLook *look)
{
    s32 i;

    pl->weaponKind = look->kind;
    for (i = 0; i < 6; i++) {
        pl->equip[i] = look->armour[i];
        pl->armourKind[i] = look->colour[i];
    }
    pl->equip[6] = look->weapon;
    pl->weaponType = gWeaponKindClass[pl->weaponKind];
    pl->isGunner = WeaponClass_IsGunner(pl->weaponType);
    pl->weaponFlags = WeaponType_Flags(pl->weaponType);
    pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[6]);
}

/* The six armour pieces as one object, so that they copy as a whole. */
typedef struct EquipSet {
    EquipItem e[6];
} EquipSet;

extern u8 *gReadBuf;
/* Per gender (7 sets each: the six armour parts, then the torso's
 * alternative), per part model id, the part's texture pack. */
extern TexPackHead *gPlayerLookFiles[];
void Player_BuildPartIds(Player *p, u8 *ids, EquipItem *a, u8 *b);
s16 Look_MapPartId(Player *p, s16 kind, s16 id, u8 k);

extern u8 *gLoadBuf;

/*
 * Loads the player's six look models (the body and five parts) from the
 * files Player_QueueLookFiles read into gReadBuf: the weapon's pack at 0,
 * the model packs at 0x18000, then per part i the model at 0x20000 +
 * i * 0x20000 and its texture pack 0x18000 after it. Each is unpacked
 * through gLoadBuf into gModelLoadBuf and set up in a new model slot.
 */
void Player_LoadLookModels(Player *pl)
{
    s16 fileSet;
    u8 *end;
    s16 i;
    s32 part;
    s16 lookId;
    s16 id;
    TexPackHead *pack;
    u8 *buf;

    buf = gModelLoadBuf;
    end = buf + MODEL_LOAD_SKEL_OFFSET;

    LzDecompressSized(gReadBuf, gLoadBuf, 0x18000);
    Pack_CopyParts01(gLoadBuf, buf, end);
    Player_AllocWeaponModel(pl->weaponKind, pl->index, buf, gReadBuf + 0x18000);
    Player_BuildPartIds(pl, pl->armourKind, pl->equip, &pl->lookVariant);
    for (i = 0; i < 6; i++) {
        part = i;
        lookId = Look_MapPartId(pl, i, pl->armourKind[part], pl->lookVariant);
        if (part == 2) {
            if (Armor_LacksSexModel(pl->equip[2].id, pl->gender) == 1) {
                pl->headAltModel = 1;
                fileSet = 6;
            } else {
                pl->headAltModel = 0;
                fileSet = i;
            }
        } else {
            fileSet = i;
        }
        id = Model_Alloc(1);
        if (id < 0) {
            return;
        }
        if (part == 0) {
            pl->modelFirst = id;
            pl->modelCount = 1;
            pl->model = Model_Get(id);
        } else {
            pl->partModelFirst[i] = id;
            pl->partModelCount[i] = 1;
            pl->partModels[i] = Model_Get(id);
        }
        Model_Reserve(id, 1);
        pack = gPlayerLookFiles[fileSet + pl->gender * 7];
        pack += lookId;
        pl->lookPartId[part] = lookId;
        switch (part) {
        case 0:
            LzDecompressSized(gReadBuf + 0x20000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(pl->index * 12 + 43 + i * 2), (s32 *)pack, 2304, (u16)(pl->lookVariant + pl->gender * 4), 2, (s32)(gReadBuf + 0x38000));
            break;
        case 1:
            LzDecompressSized(gReadBuf + 0x40000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(pl->index * 12 + 43 + i * 2), (s32 *)pack, 2304, (u16)(pl->lookVariant + pl->gender * 4), 1, (s32)(gReadBuf + 0x58000));
            break;
        case 2:
            LzDecompressSized(gReadBuf + 0x60000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(pl->index * 12 + 43 + i * 2), (s32 *)pack, 2304, (u16)(pl->lookVariant + pl->gender * 4), 1, (s32)(gReadBuf + 0x78000));
            break;
        case 3:
            LzDecompressSized(gReadBuf + 0x80000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(pl->index * 12 + 43 + i * 2), (s32 *)pack, 2304, (u16)(pl->lookVariant + pl->gender * 4), 1, (s32)(gReadBuf + 0x98000));
            break;
        case 4:
            LzDecompressSized(gReadBuf + 0xA0000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(pl->index * 12 + 43 + i * 2), (s32 *)pack, 2304, (u16)(pl->lookVariant + pl->gender * 4), 1, (s32)(gReadBuf + 0xB8000));
            break;
        case 5:
            LzDecompressSized(gReadBuf + 0xC0000, gLoadBuf, 0x18000);
            Pack_CopyParts01(gLoadBuf, buf, end);
            Model_SetupLoadedPackEx(id, (s32)gModelLoadBuf, (s16)(pl->index * 12 + 43 + i * 2), (s32 *)pack, 2304, (u16)(pl->lookVariant + pl->gender * 4), 1, (s32)(gReadBuf + 0xD8000));
            break;
        }
    }
}

extern PlayerLook gLookPending[];
extern LobbyState gLobbyState;
void Player_FreeWeaponModel(Player *p);
void Player_FreeModels(Player *p);

/* Applies player p's pending look (gLookPending) once its delay (gLookDelay) runs out: reloads
 * its models and motions and, in the lobby, marks it loaded; in the lobby a player that left
 * (or with its slot state 2 or more) is only cleared. */
void Player_ApplyLook(Player *pl)
{
    PlayerLook *look = &gLookPending[pl->index];

    if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1) {
        Lobby_ReceiveWhileWaiting();
    }
    if (gLookDelay[pl->index] > 0) {
        gLookDelay[pl->index]--;
        if (gLookDelay[pl->index] > 0) {
            return;
        }
    }
    if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1 && ((s8)gLobbyState.lookPending[pl->index] >= 2 || pl->active == 0)) {
        gLobbyState.modelLoading[pl->index] = 0;
        gLobbyState.modelLoaded[pl->index] = 0;
        gLobbyState.pcLoaded[pl->index] = 0;
        gLobbyState.lookPending[pl->index] = 0;
        return;
    }
    Player_SetLook(pl, look);
    Player_FreeWeaponModel(pl);
    Player_FreeModels(pl);
    Mem_Compact();
    Player_InitPartTable(pl->index);
    Player_LoadLookModels(pl);
    Player_ClearFxBlock(pl);
    Player_StartMotionLayer(pl, pl->motionId[0], 0, 0, 0);
    Player_StartMotionLayer(pl, pl->motionId[1], 0, 0, 1);
    Player_InitRecords(pl);
    pl->visible = 1;
    if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1) {
        gLobbyState.modelLoading[pl->index] = 0;
        gLobbyState.modelLoaded[pl->index] = 1;
        gLobbyState.pcLoaded[pl->index] = 1;
        gLobbyState.lookPending[pl->index] = 0;
    }
}

/* Queues file id of the data archive (id | 0x30000, as LoadFile reads it)
 * for the file loader, to be read into dst. */
void FileQueue_AddPlayerFile(s32 id, s32 dst)
{
    FileQueue_AddFile(id | 0x30000, (void *)dst);
}

u16 Equip_GetKind(EquipItem *e);

/*
 * Queues the files of player p's pending look (gLookPending, filled here
 * from its equipment) for the file loader, into gReadBuf as
 * Player_LoadLookModels reads them, then a call of Player_ApplyLook.
 * Returns 1, or 0 in the lobby when the player's look is already loaded
 * or queued.
 */
s32 Player_QueueLookFiles(Player *pl)
{
    s16 part;
    s16 lookId;
    s16 fileSet;
    s32 *modelFile;
    TexPackHead *pack;
    s16 i;
    PlayerLook *look = &gLookPending[pl->index];

    if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1) {
        if ((s8)gLobbyState.pcLoaded[pl->index] == 1) {
            return 0;
        }
        if ((s8)gLobbyState.lookPending[pl->index] == 1) {
            return 0;
        }
    }
    if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1 && (s8)gLobbyState.lookPending[pl->index] == 0) {
        gLobbyState.lookPending[pl->index] = 1;
    }
    pl->weaponKind = Equip_GetKind(&pl->equip[6]);
    look->kind = pl->weaponKind;
    look->weapon = pl->equip[6];
    *(EquipSet *)look->armour = *(EquipSet *)pl->equip;
    Player_BuildPartIds(pl, look->colour, look->armour, &look->variant);
    Tex_LoadById(look->variant + pl->gender * 4, (s32)(gReadBuf + 0xE0000), 0);
    gLookDelay[pl->index]++;
    FileQueue_AddPlayerFile(gWeaponModelPackFiles[look->kind], (s32)gReadBuf);
    FileQueue_AddPlayerFile(gModelPackIds[look->kind], (s32)(gReadBuf + 0x18000));
    for (i = 0; i < 6; i++) {
        part = i;
        lookId = Look_MapPartId(pl, i, look->colour[part], look->variant);
        if (part == 2) {
            if (Armor_LacksSexModel(look->armour[2].id, pl->gender) == 1) {
                pl->headAltModel = 1;
                fileSet = 6;
            } else {
                pl->headAltModel = 0;
                fileSet = part;
            }
        } else {
            fileSet = part;
        }
        if (pl->gender == 0) {
            modelFile = gPartModelFilesMale[fileSet];
        } else {
            modelFile = gPartModelFilesFemale[fileSet];
        }
        pack = gPlayerLookFiles[fileSet + pl->gender * 7];
        modelFile += lookId;
        pack += lookId;
        switch (part) {
        case 0:
            FileQueue_AddPlayerFile(*modelFile, (s32)(gReadBuf + 0x20000));
            FileQueue_AddPlayerFile(gTexPackFiles[*pack->file], (s32)(gReadBuf + 0x38000));
            break;
        case 3:
            FileQueue_AddPlayerFile(*modelFile, (s32)(gReadBuf + 0x80000));
            FileQueue_AddPlayerFile(gTexPackFiles[*pack->file], (s32)(gReadBuf + 0x98000));
            break;
        case 4:
            FileQueue_AddPlayerFile(*modelFile, (s32)(gReadBuf + 0xA0000));
            FileQueue_AddPlayerFile(gTexPackFiles[*pack->file], (s32)(gReadBuf + 0xB8000));
            break;
        case 1:
            FileQueue_AddPlayerFile(*modelFile, (s32)(gReadBuf + 0x40000));
            FileQueue_AddPlayerFile(gTexPackFiles[*pack->file], (s32)(gReadBuf + 0x58000));
            break;
        case 2:
            FileQueue_AddPlayerFile(*modelFile, (s32)(gReadBuf + 0x60000));
            FileQueue_AddPlayerFile(gTexPackFiles[*pack->file], (s32)(gReadBuf + 0x78000));
            break;
        case 5:
            FileQueue_AddPlayerFile(*modelFile, (s32)(gReadBuf + 0xC0000));
            FileQueue_AddPlayerFile(gTexPackFiles[*pack->file], (s32)(gReadBuf + 0xD8000));
            break;
        }
    }
    FileQueue_AddCall((void (*)(void))Player_ApplyLook, 1, (s32)pl, 0, 0, 0);
    return 1;
}

/* This file's initialised data. */
extern u32 D_005B2458[];
s32 gAreaCamDataFiles[200] = {
#include "sub_main/004224E0.inc"
};
u32 gEquipModelPackFilesMale[24] = {
#include "sub_main/00422800.inc"
};
u32 gEquipModelPackFilesFemale[24] = {
#include "sub_main/00422860.inc"
};
s32 gTexPackFiles[2836] = {
#include "sub_main/004228C0.inc"
};
u32 gAreaTexFiles[198] = {
#include "sub_main/00425510.inc"
};
u32 gOnlineVillageTexFiles[2] = {
#include "sub_main/00425828.inc"
};
u32 gArea84TexFiles[2] = {
#include "sub_main/00425830.inc"
};
s32 gPlayerLookFiles_0_3[2] = {
#include "sub_main/00425838.inc"
};
s32 gPlayerLookFiles_0_5[2] = {
#include "sub_main/00425840.inc"
};
s32 gPlayerLookFiles_0_7[2] = {
#include "sub_main/00425848.inc"
};
s32 gPlayerLookFiles_0_9[2] = {
#include "sub_main/00425850.inc"
};
s32 gPlayerLookFiles_0_11[2] = {
#include "sub_main/00425858.inc"
};
s32 gPlayerLookFiles_0_13[2] = {
#include "sub_main/00425860.inc"
};
s32 gPlayerLookFiles_0_15[2] = {
#include "sub_main/00425868.inc"
};
s32 gPlayerLookFiles_0_17[2] = {
#include "sub_main/00425870.inc"
};
s32 gPlayerLookFiles_0_19[2] = {
#include "sub_main/00425878.inc"
};
s32 gPlayerLookFiles_0_21[2] = {
#include "sub_main/00425880.inc"
};
s32 gPlayerLookFiles_0_23[2] = {
#include "sub_main/00425888.inc"
};
s32 gPlayerLookFiles_0_25[2] = {
#include "sub_main/00425890.inc"
};
s32 gPlayerLookFiles_0_27[2] = {
#include "sub_main/00425898.inc"
};
s32 gPlayerLookFiles_0_29[2] = {
#include "sub_main/004258A0.inc"
};
s32 gPlayerLookFiles_0_31[2] = {
#include "sub_main/004258A8.inc"
};
s32 gPlayerLookFiles_0_33[2] = {
#include "sub_main/004258B0.inc"
};
s32 gPlayerLookFiles_0_35[2] = {
#include "sub_main/004258B8.inc"
};
s32 gPlayerLookFiles_0_37[2] = {
#include "sub_main/004258C0.inc"
};
s32 gPlayerLookFiles_0_39[2] = {
#include "sub_main/004258C8.inc"
};
s32 gPlayerLookFiles_0_41[2] = {
#include "sub_main/004258D0.inc"
};
s32 gPlayerLookFiles_0_43[2] = {
#include "sub_main/004258D8.inc"
};
s32 gPlayerLookFiles_0_45[2] = {
#include "sub_main/004258E0.inc"
};
s32 gPlayerLookFiles_0_47[2] = {
#include "sub_main/004258E8.inc"
};
s32 gPlayerLookFiles_0_49[2] = {
#include "sub_main/004258F0.inc"
};
s32 gPlayerLookFiles_0_51[2] = {
#include "sub_main/004258F8.inc"
};
s32 gPlayerLookFiles_0_53[2] = {
#include "sub_main/00425900.inc"
};
s32 gPlayerLookFiles_0_55[2] = {
#include "sub_main/00425908.inc"
};
s32 gPlayerLookFiles_0_57[2] = {
#include "sub_main/00425910.inc"
};
s32 gPlayerLookFiles_0_59[2] = {
#include "sub_main/00425918.inc"
};
s32 gPlayerLookFiles_0_61[2] = {
#include "sub_main/00425920.inc"
};
s32 gPlayerLookFiles_0_63[2] = {
#include "sub_main/00425928.inc"
};
s32 gPlayerLookFiles_0_65[2] = {
#include "sub_main/00425930.inc"
};
s32 gPlayerLookFiles_0_67[2] = {
#include "sub_main/00425938.inc"
};
s32 gPlayerLookFiles_0_69[2] = {
#include "sub_main/00425940.inc"
};
s32 gPlayerLookFiles_0_71[2] = {
#include "sub_main/00425948.inc"
};
s32 gPlayerLookFiles_0_73[2] = {
#include "sub_main/00425950.inc"
};
s32 gPlayerLookFiles_0_75[2] = {
#include "sub_main/00425958.inc"
};
s32 gPlayerLookFiles_0_77[2] = {
#include "sub_main/00425960.inc"
};
s32 gPlayerLookFiles_0_79[2] = {
#include "sub_main/00425968.inc"
};
s32 gPlayerLookFiles_0_81[2] = {
#include "sub_main/00425970.inc"
};
s32 gPlayerLookFiles_0_83[2] = {
#include "sub_main/00425978.inc"
};
s32 gPlayerLookFiles_0_85[2] = {
#include "sub_main/00425980.inc"
};
s32 gPlayerLookFiles_0_87[2] = {
#include "sub_main/00425988.inc"
};
s32 gPlayerLookFiles_0_89[2] = {
#include "sub_main/00425990.inc"
};
s32 gPlayerLookFiles_0_91[2] = {
#include "sub_main/00425998.inc"
};
s32 gPlayerLookFiles_0_93[2] = {
#include "sub_main/004259A0.inc"
};
s32 gPlayerLookFiles_0_95[2] = {
#include "sub_main/004259A8.inc"
};
s32 gPlayerLookFiles_0_97[2] = {
#include "sub_main/004259B0.inc"
};
s32 gPlayerLookFiles_0_99[2] = {
#include "sub_main/004259B8.inc"
};
s32 gPlayerLookFiles_0_101[2] = {
#include "sub_main/004259C0.inc"
};
s32 gPlayerLookFiles_0_103[2] = {
#include "sub_main/004259C8.inc"
};
s32 gPlayerLookFiles_0_105[2] = {
#include "sub_main/004259D0.inc"
};
s32 gPlayerLookFiles_0_107[2] = {
#include "sub_main/004259D8.inc"
};
s32 gPlayerLookFiles_0_109[2] = {
#include "sub_main/004259E0.inc"
};
s32 gPlayerLookFiles_0_111[2] = {
#include "sub_main/004259E8.inc"
};
s32 gPlayerLookFiles_0_113[2] = {
#include "sub_main/004259F0.inc"
};
s32 gPlayerLookFiles_0_115[2] = {
#include "sub_main/004259F8.inc"
};
s32 gPlayerLookFiles_0_117[2] = {
#include "sub_main/00425A00.inc"
};
s32 gPlayerLookFiles_0_119[2] = {
#include "sub_main/00425A08.inc"
};
s32 gPlayerLookFiles_0_121[2] = {
#include "sub_main/00425A10.inc"
};
s32 gPlayerLookFiles_0_123[2] = {
#include "sub_main/00425A18.inc"
};
s32 gPlayerLookFiles_0_125[2] = {
#include "sub_main/00425A20.inc"
};
s32 gPlayerLookFiles_0_127[2] = {
#include "sub_main/00425A28.inc"
};
s32 gPlayerLookFiles_0_129[2] = {
#include "sub_main/00425A30.inc"
};
s32 gPlayerLookFiles_0_131[2] = {
#include "sub_main/00425A38.inc"
};
s32 gPlayerLookFiles_0_133[2] = {
#include "sub_main/00425A40.inc"
};
s32 gPlayerLookFiles_0_135[2] = {
#include "sub_main/00425A48.inc"
};
s32 gPlayerLookFiles_0_137[2] = {
#include "sub_main/00425A50.inc"
};
s32 gPlayerLookFiles_0_139[2] = {
#include "sub_main/00425A58.inc"
};
s32 gPlayerLookFiles_0_141[2] = {
#include "sub_main/00425A60.inc"
};
s32 gPlayerLookFiles_0_143[2] = {
#include "sub_main/00425A68.inc"
};
s32 gPlayerLookFiles_0_145[2] = {
#include "sub_main/00425A70.inc"
};
s32 gPlayerLookFiles_0_147[2] = {
#include "sub_main/00425A78.inc"
};
s32 gPlayerLookFiles_0_149[2] = {
#include "sub_main/00425A80.inc"
};
s32 gPlayerLookFiles_0_151[2] = {
#include "sub_main/00425A88.inc"
};
s32 gPlayerLookFiles_0_153[2] = {
#include "sub_main/00425A90.inc"
};
s32 gPlayerLookFiles_0_155[2] = {
#include "sub_main/00425A98.inc"
};
s32 gPlayerLookFiles_0_157[2] = {
#include "sub_main/00425AA0.inc"
};
s32 gPlayerLookFiles_0_159[2] = {
#include "sub_main/00425AA8.inc"
};
s32 gPlayerLookFiles_0_161[2] = {
#include "sub_main/00425AB0.inc"
};
s32 gPlayerLookFiles_0_163[2] = {
#include "sub_main/00425AB8.inc"
};
s32 gPlayerLookFiles_0_165[2] = {
#include "sub_main/00425AC0.inc"
};
s32 gPlayerLookFiles_0_167[2] = {
#include "sub_main/00425AC8.inc"
};
s32 gPlayerLookFiles_0_169[2] = {
#include "sub_main/00425AD0.inc"
};
s32 gPlayerLookFiles_0_171[2] = {
#include "sub_main/00425AD8.inc"
};
s32 gPlayerLookFiles_0_173[2] = {
#include "sub_main/00425AE0.inc"
};
s32 gPlayerLookFiles_0_175[2] = {
#include "sub_main/00425AE8.inc"
};
s32 gPlayerLookFiles_0_177[2] = {
#include "sub_main/00425AF0.inc"
};
s32 gPlayerLookFiles_0_179[2] = {
#include "sub_main/00425AF8.inc"
};
s32 gPlayerLookFiles_0_181[2] = {
#include "sub_main/00425B00.inc"
};
s32 gPlayerLookFiles_0_183[2] = {
#include "sub_main/00425B08.inc"
};
s32 gPlayerLookFiles_0_185[2] = {
#include "sub_main/00425B10.inc"
};
s32 gPlayerLookFiles_0_187[2] = {
#include "sub_main/00425B18.inc"
};
s32 gPlayerLookFiles_0_189[2] = {
#include "sub_main/00425B20.inc"
};
s32 gPlayerLookFiles_0_191[2] = {
#include "sub_main/00425B28.inc"
};
s32 gPlayerLookFiles_0_193[2] = {
#include "sub_main/00425B30.inc"
};
s32 gPlayerLookFiles_0_195[2] = {
#include "sub_main/00425B38.inc"
};
s32 gPlayerLookFiles_0_197[2] = {
#include "sub_main/00425B40.inc"
};
s32 gPlayerLookFiles_0_199[2] = {
#include "sub_main/00425B48.inc"
};
s32 gPlayerLookFiles_0_201[2] = {
#include "sub_main/00425B50.inc"
};
s32 gPlayerLookFiles_0_203[2] = {
#include "sub_main/00425B58.inc"
};
s32 gPlayerLookFiles_0_205[2] = {
#include "sub_main/00425B60.inc"
};
s32 gPlayerLookFiles_0_207[2] = {
#include "sub_main/00425B68.inc"
};
s32 gPlayerLookFiles_0_209[2] = {
#include "sub_main/00425B70.inc"
};
s32 gPlayerLookFiles_0_211[2] = {
#include "sub_main/00425B78.inc"
};
s32 gPlayerLookFiles_0_213[2] = {
#include "sub_main/00425B80.inc"
};
s32 gPlayerLookFiles_0_215[2] = {
#include "sub_main/00425B88.inc"
};
s32 gPlayerLookFiles_0_217[2] = {
#include "sub_main/00425B90.inc"
};
s32 gPlayerLookFiles_0_219[2] = {
#include "sub_main/00425B98.inc"
};
s32 gPlayerLookFiles_0_221[2] = {
#include "sub_main/00425BA0.inc"
};
s32 gPlayerLookFiles_0_223[2] = {
#include "sub_main/00425BA8.inc"
};
s32 gPlayerLookFiles_0_225[2] = {
#include "sub_main/00425BB0.inc"
};
s32 gPlayerLookFiles_0_227[2] = {
#include "sub_main/00425BB8.inc"
};
s32 gPlayerLookFiles_0_229[2] = {
#include "sub_main/00425BC0.inc"
};
s32 gPlayerLookFiles_0_231[2] = {
#include "sub_main/00425BC8.inc"
};
s32 gPlayerLookFiles_0_233[2] = {
#include "sub_main/00425BD0.inc"
};
s32 gPlayerLookFiles_0_235[2] = {
#include "sub_main/00425BD8.inc"
};
s32 gPlayerLookFiles_0_237[2] = {
#include "sub_main/00425BE0.inc"
};
s32 gPlayerLookFiles_0_239[2] = {
#include "sub_main/00425BE8.inc"
};
s32 gPlayerLookFiles_0_241[2] = {
#include "sub_main/00425BF0.inc"
};
s32 gPlayerLookFiles_0_243[2] = {
#include "sub_main/00425BF8.inc"
};
s32 gPlayerLookFiles_0_245[2] = {
#include "sub_main/00425C00.inc"
};
s32 gPlayerLookFiles_0_247[2] = {
#include "sub_main/00425C08.inc"
};
s32 gPlayerLookFiles_0_249[2] = {
#include "sub_main/00425C10.inc"
};
s32 gPlayerLookFiles_0_251[2] = {
#include "sub_main/00425C18.inc"
};
s32 gPlayerLookFiles_0_253[2] = {
#include "sub_main/00425C20.inc"
};
s32 gPlayerLookFiles_0_255[2] = {
#include "sub_main/00425C28.inc"
};
s32 gPlayerLookFiles_0_257[2] = {
#include "sub_main/00425C30.inc"
};
s32 gPlayerLookFiles_0_259[2] = {
#include "sub_main/00425C38.inc"
};
s32 gPlayerLookFiles_0_261[2] = {
#include "sub_main/00425C40.inc"
};
s32 gPlayerLookFiles_0_263[2] = {
#include "sub_main/00425C48.inc"
};
s32 gPlayerLookFiles_0_265[2] = {
#include "sub_main/00425C50.inc"
};
s32 gPlayerLookFiles_0_267[2] = {
#include "sub_main/00425C58.inc"
};
s32 gPlayerLookFiles_0_269[2] = {
#include "sub_main/00425C60.inc"
};
s32 gPlayerLookFiles_0_271[2] = {
#include "sub_main/00425C68.inc"
};
s32 gPlayerLookFiles_0_273[2] = {
#include "sub_main/00425C70.inc"
};
s32 gPlayerLookFiles_0_275[2] = {
#include "sub_main/00425C78.inc"
};
s32 gPlayerLookFiles_0_277[2] = {
#include "sub_main/00425C80.inc"
};
s32 gPlayerLookFiles_0_279[2] = {
#include "sub_main/00425C88.inc"
};
s32 gPlayerLookFiles_0_281[2] = {
#include "sub_main/00425C90.inc"
};
s32 gPlayerLookFiles_0_283[2] = {
#include "sub_main/00425C98.inc"
};
s32 gPlayerLookFiles_0_285[2] = {
#include "sub_main/00425CA0.inc"
};
s32 gPlayerLookFiles_0_287[2] = {
#include "sub_main/00425CA8.inc"
};
s32 gPlayerLookFiles_0_289[2] = {
#include "sub_main/00425CB0.inc"
};
s32 gPlayerLookFiles_0_291[2] = {
#include "sub_main/00425CB8.inc"
};
s32 gPlayerLookFiles_0_293[2] = {
#include "sub_main/00425CC0.inc"
};
s32 gPlayerLookFiles_0_295[2] = {
#include "sub_main/00425CC8.inc"
};
s32 gPlayerLookFiles_0_297[2] = {
#include "sub_main/00425CD0.inc"
};
s32 gPlayerLookFiles_0_299[2] = {
#include "sub_main/00425CD8.inc"
};
s32 gPlayerLookFiles_0_301[2] = {
#include "sub_main/00425CE0.inc"
};
s32 gPlayerLookFiles_0_303[2] = {
#include "sub_main/00425CE8.inc"
};
s32 gPlayerLookFiles_0_305[2] = {
#include "sub_main/00425CF0.inc"
};
s32 gPlayerLookFiles_0_307[2] = {
#include "sub_main/00425CF8.inc"
};
s32 gPlayerLookFiles_0_309[2] = {
#include "sub_main/00425D00.inc"
};
s32 gPlayerLookFiles_0_311[2] = {
#include "sub_main/00425D08.inc"
};
s32 gPlayerLookFiles_0_313[2] = {
#include "sub_main/00425D10.inc"
};
s32 gPlayerLookFiles_0_315[2] = {
#include "sub_main/00425D18.inc"
};
s32 gPlayerLookFiles_0_317[2] = {
#include "sub_main/00425D20.inc"
};
s32 gPlayerLookFiles_0_319[2] = {
#include "sub_main/00425D28.inc"
};
s32 gPlayerLookFiles_0_321[2] = {
#include "sub_main/00425D30.inc"
};
s32 gPlayerLookFiles_0_323[2] = {
#include "sub_main/00425D38.inc"
};
s32 gPlayerLookFiles_0_325[2] = {
#include "sub_main/00425D40.inc"
};
s32 gPlayerLookFiles_0_327[2] = {
#include "sub_main/00425D48.inc"
};
s32 gPlayerLookFiles_0_329[2] = {
#include "sub_main/00425D50.inc"
};
s32 gPlayerLookFiles_0_331[2] = {
#include "sub_main/00425D58.inc"
};
s32 gPlayerLookFiles_0_333[2] = {
#include "sub_main/00425D60.inc"
};
s32 gPlayerLookFiles_0_335[2] = {
#include "sub_main/00425D68.inc"
};
s32 gPlayerLookFiles_0_337[2] = {
#include "sub_main/00425D70.inc"
};
s32 gPlayerLookFiles_0_339[2] = {
#include "sub_main/00425D78.inc"
};
s32 gPlayerLookFiles_0_341[2] = {
#include "sub_main/00425D80.inc"
};
s32 gPlayerLookFiles_0_343[2] = {
#include "sub_main/00425D88.inc"
};
s32 gPlayerLookFiles_0_345[2] = {
#include "sub_main/00425D90.inc"
};
s32 gPlayerLookFiles_0_347[2] = {
#include "sub_main/00425D98.inc"
};
s32 gPlayerLookFiles_0_349[2] = {
#include "sub_main/00425DA0.inc"
};
s32 gPlayerLookFiles_0_351[2] = {
#include "sub_main/00425DA8.inc"
};
s32 gPlayerLookFiles_0_353[2] = {
#include "sub_main/00425DB0.inc"
};
s32 gPlayerLookFiles_0_355[2] = {
#include "sub_main/00425DB8.inc"
};
s32 gPlayerLookFiles_0_357[2] = {
#include "sub_main/00425DC0.inc"
};
s32 gPlayerLookFiles_0_359[2] = {
#include "sub_main/00425DC8.inc"
};
s32 gPlayerLookFiles_0_361[2] = {
#include "sub_main/00425DD0.inc"
};
s32 gPlayerLookFiles_0_363[2] = {
#include "sub_main/00425DD8.inc"
};
s32 gPlayerLookFiles_0_365[2] = {
#include "sub_main/00425DE0.inc"
};
s32 gPlayerLookFiles_0_367[2] = {
#include "sub_main/00425DE8.inc"
};
s32 gPlayerLookFiles_0_369[2] = {
#include "sub_main/00425DF0.inc"
};
s32 gPlayerLookFiles_0_371[2] = {
#include "sub_main/00425DF8.inc"
};
s32 gPlayerLookFiles_0_373[2] = {
#include "sub_main/00425E00.inc"
};
s32 gPlayerLookFiles_0_375[2] = {
#include "sub_main/00425E08.inc"
};
s32 gPlayerLookFiles_0_377[2] = {
#include "sub_main/00425E10.inc"
};
s32 gPlayerLookFiles_0_379[2] = {
#include "sub_main/00425E18.inc"
};
s32 gPlayerLookFiles_0_381[2] = {
#include "sub_main/00425E20.inc"
};
s32 gPlayerLookFiles_0_383[2] = {
#include "sub_main/00425E28.inc"
};
s32 gPlayerLookFiles_0_385[2] = {
#include "sub_main/00425E30.inc"
};
s32 gPlayerLookFiles_0_387[2] = {
#include "sub_main/00425E38.inc"
};
s32 gPlayerLookFiles_0_389[2] = {
#include "sub_main/00425E40.inc"
};
s32 gPlayerLookFiles_0_391[2] = {
#include "sub_main/00425E48.inc"
};
s32 gPlayerLookFiles_0_393[2] = {
#include "sub_main/00425E50.inc"
};
s32 gPlayerLookFiles_0_395[2] = {
#include "sub_main/00425E58.inc"
};
s32 gPlayerLookFiles_0_397[4] = {
#include "sub_main/00425E60.inc"
};
TexPackHead gPlayerLookFiles_0[199] = {
#include "sub_main/00425E70.inc"
};
s32 gPlayerLookFiles_1_1[2] = {
#include "sub_main/004264A8.inc"
};
s32 gPlayerLookFiles_1_3[2] = {
#include "sub_main/004264B0.inc"
};
s32 gPlayerLookFiles_1_5[2] = {
#include "sub_main/004264B8.inc"
};
s32 gPlayerLookFiles_1_7[2] = {
#include "sub_main/004264C0.inc"
};
s32 gPlayerLookFiles_1_9[2] = {
#include "sub_main/004264C8.inc"
};
s32 gPlayerLookFiles_1_11[2] = {
#include "sub_main/004264D0.inc"
};
s32 gPlayerLookFiles_1_13[2] = {
#include "sub_main/004264D8.inc"
};
s32 gPlayerLookFiles_1_15[2] = {
#include "sub_main/004264E0.inc"
};
s32 gPlayerLookFiles_1_17[2] = {
#include "sub_main/004264E8.inc"
};
s32 gPlayerLookFiles_1_19[2] = {
#include "sub_main/004264F0.inc"
};
s32 gPlayerLookFiles_1_21[2] = {
#include "sub_main/004264F8.inc"
};
s32 gPlayerLookFiles_1_23[2] = {
#include "sub_main/00426500.inc"
};
s32 gPlayerLookFiles_1_25[2] = {
#include "sub_main/00426508.inc"
};
s32 gPlayerLookFiles_1_27[2] = {
#include "sub_main/00426510.inc"
};
s32 gPlayerLookFiles_1_29[2] = {
#include "sub_main/00426518.inc"
};
s32 gPlayerLookFiles_1_31[2] = {
#include "sub_main/00426520.inc"
};
s32 gPlayerLookFiles_1_33[2] = {
#include "sub_main/00426528.inc"
};
s32 gPlayerLookFiles_1_35[2] = {
#include "sub_main/00426530.inc"
};
s32 gPlayerLookFiles_1_37[2] = {
#include "sub_main/00426538.inc"
};
s32 gPlayerLookFiles_1_39[2] = {
#include "sub_main/00426540.inc"
};
s32 gPlayerLookFiles_1_41[2] = {
#include "sub_main/00426548.inc"
};
s32 gPlayerLookFiles_1_43[2] = {
#include "sub_main/00426550.inc"
};
s32 gPlayerLookFiles_1_45[2] = {
#include "sub_main/00426558.inc"
};
s32 gPlayerLookFiles_1_47[2] = {
#include "sub_main/00426560.inc"
};
s32 gPlayerLookFiles_1_49[2] = {
#include "sub_main/00426568.inc"
};
s32 gPlayerLookFiles_1_51[2] = {
#include "sub_main/00426570.inc"
};
s32 gPlayerLookFiles_1_53[2] = {
#include "sub_main/00426578.inc"
};
TexPackHead gPlayerLookFiles_1[27] = {
#include "sub_main/00426580.inc"
};
s32 gPlayerLookFiles_2_1[2] = {
#include "sub_main/00426658.inc"
};
s32 gPlayerLookFiles_2_3[2] = {
#include "sub_main/00426660.inc"
};
s32 gPlayerLookFiles_2_5[2] = {
#include "sub_main/00426668.inc"
};
s32 gPlayerLookFiles_2_7[2] = {
#include "sub_main/00426670.inc"
};
s32 gPlayerLookFiles_2_9[2] = {
#include "sub_main/00426678.inc"
};
s32 gPlayerLookFiles_2_11[2] = {
#include "sub_main/00426680.inc"
};
s32 gPlayerLookFiles_2_13[2] = {
#include "sub_main/00426688.inc"
};
s32 gPlayerLookFiles_2_15[2] = {
#include "sub_main/00426690.inc"
};
s32 gPlayerLookFiles_2_17[2] = {
#include "sub_main/00426698.inc"
};
s32 gPlayerLookFiles_2_19[2] = {
#include "sub_main/004266A0.inc"
};
s32 gPlayerLookFiles_2_21[2] = {
#include "sub_main/004266A8.inc"
};
s32 gPlayerLookFiles_2_23[2] = {
#include "sub_main/004266B0.inc"
};
s32 gPlayerLookFiles_2_25[2] = {
#include "sub_main/004266B8.inc"
};
s32 gPlayerLookFiles_2_27[2] = {
#include "sub_main/004266C0.inc"
};
s32 gPlayerLookFiles_2_29[2] = {
#include "sub_main/004266C8.inc"
};
s32 gPlayerLookFiles_2_31[2] = {
#include "sub_main/004266D0.inc"
};
s32 gPlayerLookFiles_2_33[2] = {
#include "sub_main/004266D8.inc"
};
s32 gPlayerLookFiles_2_35[2] = {
#include "sub_main/004266E0.inc"
};
s32 gPlayerLookFiles_2_37[2] = {
#include "sub_main/004266E8.inc"
};
s32 gPlayerLookFiles_2_39[2] = {
#include "sub_main/004266F0.inc"
};
s32 gPlayerLookFiles_2_41[2] = {
#include "sub_main/004266F8.inc"
};
s32 gPlayerLookFiles_2_43[2] = {
#include "sub_main/00426700.inc"
};
s32 gPlayerLookFiles_2_45[2] = {
#include "sub_main/00426708.inc"
};
s32 gPlayerLookFiles_2_47[2] = {
#include "sub_main/00426710.inc"
};
s32 gPlayerLookFiles_2_49[2] = {
#include "sub_main/00426718.inc"
};
s32 gPlayerLookFiles_2_51[2] = {
#include "sub_main/00426720.inc"
};
s32 gPlayerLookFiles_2_53[2] = {
#include "sub_main/00426728.inc"
};
s32 gPlayerLookFiles_2_55[2] = {
#include "sub_main/00426730.inc"
};
s32 gPlayerLookFiles_2_57[2] = {
#include "sub_main/00426738.inc"
};
s32 gPlayerLookFiles_2_59[2] = {
#include "sub_main/00426740.inc"
};
s32 gPlayerLookFiles_2_61[2] = {
#include "sub_main/00426748.inc"
};
s32 gPlayerLookFiles_2_63[2] = {
#include "sub_main/00426750.inc"
};
s32 gPlayerLookFiles_2_65[2] = {
#include "sub_main/00426758.inc"
};
s32 gPlayerLookFiles_2_67[2] = {
#include "sub_main/00426760.inc"
};
s32 gPlayerLookFiles_2_69[2] = {
#include "sub_main/00426768.inc"
};
s32 gPlayerLookFiles_2_71[2] = {
#include "sub_main/00426770.inc"
};
s32 gPlayerLookFiles_2_73[2] = {
#include "sub_main/00426778.inc"
};
s32 gPlayerLookFiles_2_75[2] = {
#include "sub_main/00426780.inc"
};
s32 gPlayerLookFiles_2_77[2] = {
#include "sub_main/00426788.inc"
};
s32 gPlayerLookFiles_2_79[2] = {
#include "sub_main/00426790.inc"
};
s32 gPlayerLookFiles_2_81[2] = {
#include "sub_main/00426798.inc"
};
s32 gPlayerLookFiles_2_83[2] = {
#include "sub_main/004267A0.inc"
};
s32 gPlayerLookFiles_2_85[2] = {
#include "sub_main/004267A8.inc"
};
s32 gPlayerLookFiles_2_87[2] = {
#include "sub_main/004267B0.inc"
};
s32 gPlayerLookFiles_2_89[2] = {
#include "sub_main/004267B8.inc"
};
s32 gPlayerLookFiles_2_91[2] = {
#include "sub_main/004267C0.inc"
};
s32 gPlayerLookFiles_2_93[2] = {
#include "sub_main/004267C8.inc"
};
s32 gPlayerLookFiles_2_95[2] = {
#include "sub_main/004267D0.inc"
};
s32 gPlayerLookFiles_2_97[2] = {
#include "sub_main/004267D8.inc"
};
s32 gPlayerLookFiles_2_99[2] = {
#include "sub_main/004267E0.inc"
};
s32 gPlayerLookFiles_2_101[2] = {
#include "sub_main/004267E8.inc"
};
s32 gPlayerLookFiles_2_103[2] = {
#include "sub_main/004267F0.inc"
};
s32 gPlayerLookFiles_2_105[2] = {
#include "sub_main/004267F8.inc"
};
s32 gPlayerLookFiles_2_107[2] = {
#include "sub_main/00426800.inc"
};
s32 gPlayerLookFiles_2_109[2] = {
#include "sub_main/00426808.inc"
};
s32 gPlayerLookFiles_2_111[2] = {
#include "sub_main/00426810.inc"
};
s32 gPlayerLookFiles_2_113[2] = {
#include "sub_main/00426818.inc"
};
s32 gPlayerLookFiles_2_115[2] = {
#include "sub_main/00426820.inc"
};
s32 gPlayerLookFiles_2_117[2] = {
#include "sub_main/00426828.inc"
};
s32 gPlayerLookFiles_2_119[2] = {
#include "sub_main/00426830.inc"
};
s32 gPlayerLookFiles_2_121[2] = {
#include "sub_main/00426838.inc"
};
s32 gPlayerLookFiles_2_123[2] = {
#include "sub_main/00426840.inc"
};
s32 gPlayerLookFiles_2_125[2] = {
#include "sub_main/00426848.inc"
};
s32 gPlayerLookFiles_2_127[2] = {
#include "sub_main/00426850.inc"
};
s32 gPlayerLookFiles_2_129[2] = {
#include "sub_main/00426858.inc"
};
s32 gPlayerLookFiles_2_131[2] = {
#include "sub_main/00426860.inc"
};
s32 gPlayerLookFiles_2_133[2] = {
#include "sub_main/00426868.inc"
};
s32 gPlayerLookFiles_2_135[2] = {
#include "sub_main/00426870.inc"
};
s32 gPlayerLookFiles_2_137[2] = {
#include "sub_main/00426878.inc"
};
s32 gPlayerLookFiles_2_139[2] = {
#include "sub_main/00426880.inc"
};
s32 gPlayerLookFiles_2_141[2] = {
#include "sub_main/00426888.inc"
};
s32 gPlayerLookFiles_2_143[2] = {
#include "sub_main/00426890.inc"
};
s32 gPlayerLookFiles_2_145[2] = {
#include "sub_main/00426898.inc"
};
s32 gPlayerLookFiles_2_147[2] = {
#include "sub_main/004268A0.inc"
};
s32 gPlayerLookFiles_2_149[2] = {
#include "sub_main/004268A8.inc"
};
s32 gPlayerLookFiles_2_151[2] = {
#include "sub_main/004268B0.inc"
};
s32 gPlayerLookFiles_2_153[2] = {
#include "sub_main/004268B8.inc"
};
s32 gPlayerLookFiles_2_155[2] = {
#include "sub_main/004268C0.inc"
};
s32 gPlayerLookFiles_2_157[2] = {
#include "sub_main/004268C8.inc"
};
s32 gPlayerLookFiles_2_159[2] = {
#include "sub_main/004268D0.inc"
};
s32 gPlayerLookFiles_2_161[2] = {
#include "sub_main/004268D8.inc"
};
s32 gPlayerLookFiles_2_163[2] = {
#include "sub_main/004268E0.inc"
};
s32 gPlayerLookFiles_2_165[2] = {
#include "sub_main/004268E8.inc"
};
s32 gPlayerLookFiles_2_167[2] = {
#include "sub_main/004268F0.inc"
};
s32 gPlayerLookFiles_2_169[2] = {
#include "sub_main/004268F8.inc"
};
s32 gPlayerLookFiles_2_171[2] = {
#include "sub_main/00426900.inc"
};
s32 gPlayerLookFiles_2_173[2] = {
#include "sub_main/00426908.inc"
};
s32 gPlayerLookFiles_2_175[2] = {
#include "sub_main/00426910.inc"
};
s32 gPlayerLookFiles_2_177[2] = {
#include "sub_main/00426918.inc"
};
s32 gPlayerLookFiles_2_179[2] = {
#include "sub_main/00426920.inc"
};
s32 gPlayerLookFiles_2_181[2] = {
#include "sub_main/00426928.inc"
};
s32 gPlayerLookFiles_2_183[2] = {
#include "sub_main/00426930.inc"
};
s32 gPlayerLookFiles_2_185[2] = {
#include "sub_main/00426938.inc"
};
s32 gPlayerLookFiles_2_187[2] = {
#include "sub_main/00426940.inc"
};
s32 gPlayerLookFiles_2_189[2] = {
#include "sub_main/00426948.inc"
};
s32 gPlayerLookFiles_2_191[2] = {
#include "sub_main/00426950.inc"
};
s32 gPlayerLookFiles_2_193[2] = {
#include "sub_main/00426958.inc"
};
s32 gPlayerLookFiles_2_195[2] = {
#include "sub_main/00426960.inc"
};
s32 gPlayerLookFiles_2_197[2] = {
#include "sub_main/00426968.inc"
};
s32 gPlayerLookFiles_2_199[2] = {
#include "sub_main/00426970.inc"
};
s32 gPlayerLookFiles_2_201[2] = {
#include "sub_main/00426978.inc"
};
s32 gPlayerLookFiles_2_203[2] = {
#include "sub_main/00426980.inc"
};
s32 gPlayerLookFiles_2_205[2] = {
#include "sub_main/00426988.inc"
};
s32 gPlayerLookFiles_2_207[2] = {
#include "sub_main/00426990.inc"
};
s32 gPlayerLookFiles_2_209[2] = {
#include "sub_main/00426998.inc"
};
s32 gPlayerLookFiles_2_211[2] = {
#include "sub_main/004269A0.inc"
};
s32 gPlayerLookFiles_2_213[2] = {
#include "sub_main/004269A8.inc"
};
s32 gPlayerLookFiles_2_215[2] = {
#include "sub_main/004269B0.inc"
};
s32 gPlayerLookFiles_2_217[2] = {
#include "sub_main/004269B8.inc"
};
s32 gPlayerLookFiles_2_219[2] = {
#include "sub_main/004269C0.inc"
};
s32 gPlayerLookFiles_2_221[2] = {
#include "sub_main/004269C8.inc"
};
s32 gPlayerLookFiles_2_223[2] = {
#include "sub_main/004269D0.inc"
};
s32 gPlayerLookFiles_2_225[2] = {
#include "sub_main/004269D8.inc"
};
s32 gPlayerLookFiles_2_227[2] = {
#include "sub_main/004269E0.inc"
};
s32 gPlayerLookFiles_2_229[2] = {
#include "sub_main/004269E8.inc"
};
s32 gPlayerLookFiles_2_231[2] = {
#include "sub_main/004269F0.inc"
};
s32 gPlayerLookFiles_2_233[2] = {
#include "sub_main/004269F8.inc"
};
s32 gPlayerLookFiles_2_235[2] = {
#include "sub_main/00426A00.inc"
};
s32 gPlayerLookFiles_2_237[2] = {
#include "sub_main/00426A08.inc"
};
s32 gPlayerLookFiles_2_239[2] = {
#include "sub_main/00426A10.inc"
};
s32 gPlayerLookFiles_2_241[2] = {
#include "sub_main/00426A18.inc"
};
s32 gPlayerLookFiles_2_243[2] = {
#include "sub_main/00426A20.inc"
};
s32 gPlayerLookFiles_2_245[2] = {
#include "sub_main/00426A28.inc"
};
s32 gPlayerLookFiles_2_247[2] = {
#include "sub_main/00426A30.inc"
};
s32 gPlayerLookFiles_2_249[2] = {
#include "sub_main/00426A38.inc"
};
s32 gPlayerLookFiles_2_251[2] = {
#include "sub_main/00426A40.inc"
};
s32 gPlayerLookFiles_2_253[2] = {
#include "sub_main/00426A48.inc"
};
s32 gPlayerLookFiles_2_255[2] = {
#include "sub_main/00426A50.inc"
};
s32 gPlayerLookFiles_2_257[2] = {
#include "sub_main/00426A58.inc"
};
s32 gPlayerLookFiles_2_259[2] = {
#include "sub_main/00426A60.inc"
};
s32 gPlayerLookFiles_2_261[2] = {
#include "sub_main/00426A68.inc"
};
s32 gPlayerLookFiles_2_263[2] = {
#include "sub_main/00426A70.inc"
};
s32 gPlayerLookFiles_2_265[2] = {
#include "sub_main/00426A78.inc"
};
s32 gPlayerLookFiles_2_267[2] = {
#include "sub_main/00426A80.inc"
};
s32 gPlayerLookFiles_2_269[2] = {
#include "sub_main/00426A88.inc"
};
s32 gPlayerLookFiles_2_271[2] = {
#include "sub_main/00426A90.inc"
};
s32 gPlayerLookFiles_2_273[2] = {
#include "sub_main/00426A98.inc"
};
s32 gPlayerLookFiles_2_275[2] = {
#include "sub_main/00426AA0.inc"
};
s32 gPlayerLookFiles_2_277[2] = {
#include "sub_main/00426AA8.inc"
};
s32 gPlayerLookFiles_2_279[2] = {
#include "sub_main/00426AB0.inc"
};
s32 gPlayerLookFiles_2_281[2] = {
#include "sub_main/00426AB8.inc"
};
s32 gPlayerLookFiles_2_283[2] = {
#include "sub_main/00426AC0.inc"
};
s32 gPlayerLookFiles_2_285[2] = {
#include "sub_main/00426AC8.inc"
};
s32 gPlayerLookFiles_2_287[2] = {
#include "sub_main/00426AD0.inc"
};
s32 gPlayerLookFiles_2_289[2] = {
#include "sub_main/00426AD8.inc"
};
s32 gPlayerLookFiles_2_291[2] = {
#include "sub_main/00426AE0.inc"
};
s32 gPlayerLookFiles_2_293[2] = {
#include "sub_main/00426AE8.inc"
};
s32 gPlayerLookFiles_2_295[2] = {
#include "sub_main/00426AF0.inc"
};
s32 gPlayerLookFiles_2_297[2] = {
#include "sub_main/00426AF8.inc"
};
s32 gPlayerLookFiles_2_299[2] = {
#include "sub_main/00426B00.inc"
};
s32 gPlayerLookFiles_2_301[2] = {
#include "sub_main/00426B08.inc"
};
s32 gPlayerLookFiles_2_303[2] = {
#include "sub_main/00426B10.inc"
};
s32 gPlayerLookFiles_2_305[2] = {
#include "sub_main/00426B18.inc"
};
s32 gPlayerLookFiles_2_307[2] = {
#include "sub_main/00426B20.inc"
};
s32 gPlayerLookFiles_2_309[2] = {
#include "sub_main/00426B28.inc"
};
s32 gPlayerLookFiles_2_311[2] = {
#include "sub_main/00426B30.inc"
};
s32 gPlayerLookFiles_2_313[2] = {
#include "sub_main/00426B38.inc"
};
s32 gPlayerLookFiles_2_315[2] = {
#include "sub_main/00426B40.inc"
};
s32 gPlayerLookFiles_2_317[2] = {
#include "sub_main/00426B48.inc"
};
s32 gPlayerLookFiles_2_319[2] = {
#include "sub_main/00426B50.inc"
};
s32 gPlayerLookFiles_2_321[2] = {
#include "sub_main/00426B58.inc"
};
s32 gPlayerLookFiles_2_323[2] = {
#include "sub_main/00426B60.inc"
};
s32 gPlayerLookFiles_2_325[2] = {
#include "sub_main/00426B68.inc"
};
s32 gPlayerLookFiles_2_327[2] = {
#include "sub_main/00426B70.inc"
};
s32 gPlayerLookFiles_2_329[2] = {
#include "sub_main/00426B78.inc"
};
s32 gPlayerLookFiles_2_331[2] = {
#include "sub_main/00426B80.inc"
};
s32 gPlayerLookFiles_2_333[2] = {
#include "sub_main/00426B88.inc"
};
s32 gPlayerLookFiles_2_335[2] = {
#include "sub_main/00426B90.inc"
};
s32 gPlayerLookFiles_2_337[2] = {
#include "sub_main/00426B98.inc"
};
s32 gPlayerLookFiles_2_339[2] = {
#include "sub_main/00426BA0.inc"
};
s32 gPlayerLookFiles_2_341[2] = {
#include "sub_main/00426BA8.inc"
};
s32 gPlayerLookFiles_2_343[2] = {
#include "sub_main/00426BB0.inc"
};
s32 gPlayerLookFiles_2_345[2] = {
#include "sub_main/00426BB8.inc"
};
s32 gPlayerLookFiles_2_347[2] = {
#include "sub_main/00426BC0.inc"
};
s32 gPlayerLookFiles_2_349[2] = {
#include "sub_main/00426BC8.inc"
};
s32 gPlayerLookFiles_2_351[2] = {
#include "sub_main/00426BD0.inc"
};
s32 gPlayerLookFiles_2_353[2] = {
#include "sub_main/00426BD8.inc"
};
s32 gPlayerLookFiles_2_355[2] = {
#include "sub_main/00426BE0.inc"
};
s32 gPlayerLookFiles_2_357[2] = {
#include "sub_main/00426BE8.inc"
};
s32 gPlayerLookFiles_2_359[2] = {
#include "sub_main/00426BF0.inc"
};
s32 gPlayerLookFiles_2_361[2] = {
#include "sub_main/00426BF8.inc"
};
s32 gPlayerLookFiles_2_363[2] = {
#include "sub_main/00426C00.inc"
};
s32 gPlayerLookFiles_2_365[2] = {
#include "sub_main/00426C08.inc"
};
s32 gPlayerLookFiles_2_367[2] = {
#include "sub_main/00426C10.inc"
};
s32 gPlayerLookFiles_2_369[2] = {
#include "sub_main/00426C18.inc"
};
s32 gPlayerLookFiles_2_371[2] = {
#include "sub_main/00426C20.inc"
};
s32 gPlayerLookFiles_2_373[2] = {
#include "sub_main/00426C28.inc"
};
s32 gPlayerLookFiles_2_375[2] = {
#include "sub_main/00426C30.inc"
};
s32 gPlayerLookFiles_2_377[2] = {
#include "sub_main/00426C38.inc"
};
s32 gPlayerLookFiles_2_379[2] = {
#include "sub_main/00426C40.inc"
};
s32 gPlayerLookFiles_2_381[2] = {
#include "sub_main/00426C48.inc"
};
s32 gPlayerLookFiles_2_383[2] = {
#include "sub_main/00426C50.inc"
};
s32 gPlayerLookFiles_2_385[2] = {
#include "sub_main/00426C58.inc"
};
s32 gPlayerLookFiles_2_387[2] = {
#include "sub_main/00426C60.inc"
};
s32 gPlayerLookFiles_2_389[2] = {
#include "sub_main/00426C68.inc"
};
s32 gPlayerLookFiles_2_391[2] = {
#include "sub_main/00426C70.inc"
};
s32 gPlayerLookFiles_2_393[2] = {
#include "sub_main/00426C78.inc"
};
s32 gPlayerLookFiles_2_395[2] = {
#include "sub_main/00426C80.inc"
};
s32 gPlayerLookFiles_2_397[2] = {
#include "sub_main/00426C88.inc"
};
s32 gPlayerLookFiles_2_399[2] = {
#include "sub_main/00426C90.inc"
};
s32 gPlayerLookFiles_2_401[2] = {
#include "sub_main/00426C98.inc"
};
s32 gPlayerLookFiles_2_403[2] = {
#include "sub_main/00426CA0.inc"
};
s32 gPlayerLookFiles_2_405[2] = {
#include "sub_main/00426CA8.inc"
};
s32 gPlayerLookFiles_2_407[2] = {
#include "sub_main/00426CB0.inc"
};
s32 gPlayerLookFiles_2_409[2] = {
#include "sub_main/00426CB8.inc"
};
s32 gPlayerLookFiles_2_411[2] = {
#include "sub_main/00426CC0.inc"
};
s32 gPlayerLookFiles_2_413[2] = {
#include "sub_main/00426CC8.inc"
};
s32 gPlayerLookFiles_2_415[2] = {
#include "sub_main/00426CD0.inc"
};
s32 gPlayerLookFiles_2_417[2] = {
#include "sub_main/00426CD8.inc"
};
s32 gPlayerLookFiles_2_419[2] = {
#include "sub_main/00426CE0.inc"
};
s32 gPlayerLookFiles_2_421[2] = {
#include "sub_main/00426CE8.inc"
};
s32 gPlayerLookFiles_2_423[2] = {
#include "sub_main/00426CF0.inc"
};
s32 gPlayerLookFiles_2_425[2] = {
#include "sub_main/00426CF8.inc"
};
s32 gPlayerLookFiles_2_427[2] = {
#include "sub_main/00426D00.inc"
};
s32 gPlayerLookFiles_2_429[2] = {
#include "sub_main/00426D08.inc"
};
s32 gPlayerLookFiles_2_431[2] = {
#include "sub_main/00426D10.inc"
};
s32 gPlayerLookFiles_2_433[2] = {
#include "sub_main/00426D18.inc"
};
s32 gPlayerLookFiles_2_435[2] = {
#include "sub_main/00426D20.inc"
};
s32 gPlayerLookFiles_2_437[2] = {
#include "sub_main/00426D28.inc"
};
TexPackHead gPlayerLookFiles_2[219] = {
#include "sub_main/00426D30.inc"
};
s32 gPlayerLookFiles_3_1[2] = {
#include "sub_main/00427408.inc"
};
s32 gPlayerLookFiles_3_3[2] = {
#include "sub_main/00427410.inc"
};
s32 gPlayerLookFiles_3_5[2] = {
#include "sub_main/00427418.inc"
};
s32 gPlayerLookFiles_3_7[2] = {
#include "sub_main/00427420.inc"
};
s32 gPlayerLookFiles_3_9[2] = {
#include "sub_main/00427428.inc"
};
s32 gPlayerLookFiles_3_11[2] = {
#include "sub_main/00427430.inc"
};
s32 gPlayerLookFiles_3_13[2] = {
#include "sub_main/00427438.inc"
};
s32 gPlayerLookFiles_3_15[2] = {
#include "sub_main/00427440.inc"
};
s32 gPlayerLookFiles_3_17[2] = {
#include "sub_main/00427448.inc"
};
s32 gPlayerLookFiles_3_19[2] = {
#include "sub_main/00427450.inc"
};
s32 gPlayerLookFiles_3_21[2] = {
#include "sub_main/00427458.inc"
};
s32 gPlayerLookFiles_3_23[2] = {
#include "sub_main/00427460.inc"
};
s32 gPlayerLookFiles_3_25[2] = {
#include "sub_main/00427468.inc"
};
s32 gPlayerLookFiles_3_27[2] = {
#include "sub_main/00427470.inc"
};
s32 gPlayerLookFiles_3_29[2] = {
#include "sub_main/00427478.inc"
};
s32 gPlayerLookFiles_3_31[2] = {
#include "sub_main/00427480.inc"
};
s32 gPlayerLookFiles_3_33[2] = {
#include "sub_main/00427488.inc"
};
s32 gPlayerLookFiles_3_35[2] = {
#include "sub_main/00427490.inc"
};
s32 gPlayerLookFiles_3_37[2] = {
#include "sub_main/00427498.inc"
};
s32 gPlayerLookFiles_3_39[2] = {
#include "sub_main/004274A0.inc"
};
s32 gPlayerLookFiles_3_41[2] = {
#include "sub_main/004274A8.inc"
};
s32 gPlayerLookFiles_3_43[2] = {
#include "sub_main/004274B0.inc"
};
s32 gPlayerLookFiles_3_45[2] = {
#include "sub_main/004274B8.inc"
};
s32 gPlayerLookFiles_3_47[2] = {
#include "sub_main/004274C0.inc"
};
s32 gPlayerLookFiles_3_49[2] = {
#include "sub_main/004274C8.inc"
};
s32 gPlayerLookFiles_3_51[2] = {
#include "sub_main/004274D0.inc"
};
s32 gPlayerLookFiles_3_53[2] = {
#include "sub_main/004274D8.inc"
};
s32 gPlayerLookFiles_3_55[2] = {
#include "sub_main/004274E0.inc"
};
s32 gPlayerLookFiles_3_57[2] = {
#include "sub_main/004274E8.inc"
};
s32 gPlayerLookFiles_3_59[2] = {
#include "sub_main/004274F0.inc"
};
s32 gPlayerLookFiles_3_61[2] = {
#include "sub_main/004274F8.inc"
};
s32 gPlayerLookFiles_3_63[2] = {
#include "sub_main/00427500.inc"
};
s32 gPlayerLookFiles_3_65[2] = {
#include "sub_main/00427508.inc"
};
s32 gPlayerLookFiles_3_67[2] = {
#include "sub_main/00427510.inc"
};
s32 gPlayerLookFiles_3_69[2] = {
#include "sub_main/00427518.inc"
};
s32 gPlayerLookFiles_3_71[2] = {
#include "sub_main/00427520.inc"
};
s32 gPlayerLookFiles_3_73[2] = {
#include "sub_main/00427528.inc"
};
s32 gPlayerLookFiles_3_75[2] = {
#include "sub_main/00427530.inc"
};
s32 gPlayerLookFiles_3_77[2] = {
#include "sub_main/00427538.inc"
};
s32 gPlayerLookFiles_3_79[2] = {
#include "sub_main/00427540.inc"
};
s32 gPlayerLookFiles_3_81[2] = {
#include "sub_main/00427548.inc"
};
s32 gPlayerLookFiles_3_83[2] = {
#include "sub_main/00427550.inc"
};
s32 gPlayerLookFiles_3_85[2] = {
#include "sub_main/00427558.inc"
};
s32 gPlayerLookFiles_3_87[2] = {
#include "sub_main/00427560.inc"
};
s32 gPlayerLookFiles_3_89[2] = {
#include "sub_main/00427568.inc"
};
s32 gPlayerLookFiles_3_91[2] = {
#include "sub_main/00427570.inc"
};
s32 gPlayerLookFiles_3_93[2] = {
#include "sub_main/00427578.inc"
};
s32 gPlayerLookFiles_3_95[2] = {
#include "sub_main/00427580.inc"
};
s32 gPlayerLookFiles_3_97[2] = {
#include "sub_main/00427588.inc"
};
s32 gPlayerLookFiles_3_99[2] = {
#include "sub_main/00427590.inc"
};
s32 gPlayerLookFiles_3_101[2] = {
#include "sub_main/00427598.inc"
};
s32 gPlayerLookFiles_3_103[2] = {
#include "sub_main/004275A0.inc"
};
s32 gPlayerLookFiles_3_105[2] = {
#include "sub_main/004275A8.inc"
};
s32 gPlayerLookFiles_3_107[2] = {
#include "sub_main/004275B0.inc"
};
s32 gPlayerLookFiles_3_109[2] = {
#include "sub_main/004275B8.inc"
};
s32 gPlayerLookFiles_3_111[2] = {
#include "sub_main/004275C0.inc"
};
s32 gPlayerLookFiles_3_113[2] = {
#include "sub_main/004275C8.inc"
};
s32 gPlayerLookFiles_3_115[2] = {
#include "sub_main/004275D0.inc"
};
s32 gPlayerLookFiles_3_117[2] = {
#include "sub_main/004275D8.inc"
};
s32 gPlayerLookFiles_3_119[2] = {
#include "sub_main/004275E0.inc"
};
s32 gPlayerLookFiles_3_121[2] = {
#include "sub_main/004275E8.inc"
};
s32 gPlayerLookFiles_3_123[2] = {
#include "sub_main/004275F0.inc"
};
s32 gPlayerLookFiles_3_125[2] = {
#include "sub_main/004275F8.inc"
};
s32 gPlayerLookFiles_3_127[2] = {
#include "sub_main/00427600.inc"
};
s32 gPlayerLookFiles_3_129[2] = {
#include "sub_main/00427608.inc"
};
s32 gPlayerLookFiles_3_131[2] = {
#include "sub_main/00427610.inc"
};
s32 gPlayerLookFiles_3_133[2] = {
#include "sub_main/00427618.inc"
};
s32 gPlayerLookFiles_3_135[2] = {
#include "sub_main/00427620.inc"
};
s32 gPlayerLookFiles_3_137[2] = {
#include "sub_main/00427628.inc"
};
s32 gPlayerLookFiles_3_139[2] = {
#include "sub_main/00427630.inc"
};
s32 gPlayerLookFiles_3_141[2] = {
#include "sub_main/00427638.inc"
};
s32 gPlayerLookFiles_3_143[2] = {
#include "sub_main/00427640.inc"
};
s32 gPlayerLookFiles_3_145[2] = {
#include "sub_main/00427648.inc"
};
s32 gPlayerLookFiles_3_147[2] = {
#include "sub_main/00427650.inc"
};
s32 gPlayerLookFiles_3_149[2] = {
#include "sub_main/00427658.inc"
};
s32 gPlayerLookFiles_3_151[2] = {
#include "sub_main/00427660.inc"
};
s32 gPlayerLookFiles_3_153[2] = {
#include "sub_main/00427668.inc"
};
s32 gPlayerLookFiles_3_155[2] = {
#include "sub_main/00427670.inc"
};
s32 gPlayerLookFiles_3_157[2] = {
#include "sub_main/00427678.inc"
};
s32 gPlayerLookFiles_3_159[2] = {
#include "sub_main/00427680.inc"
};
s32 gPlayerLookFiles_3_161[2] = {
#include "sub_main/00427688.inc"
};
s32 gPlayerLookFiles_3_163[2] = {
#include "sub_main/00427690.inc"
};
s32 gPlayerLookFiles_3_165[2] = {
#include "sub_main/00427698.inc"
};
s32 gPlayerLookFiles_3_167[2] = {
#include "sub_main/004276A0.inc"
};
s32 gPlayerLookFiles_3_169[2] = {
#include "sub_main/004276A8.inc"
};
s32 gPlayerLookFiles_3_171[2] = {
#include "sub_main/004276B0.inc"
};
s32 gPlayerLookFiles_3_173[2] = {
#include "sub_main/004276B8.inc"
};
s32 gPlayerLookFiles_3_175[2] = {
#include "sub_main/004276C0.inc"
};
s32 gPlayerLookFiles_3_177[2] = {
#include "sub_main/004276C8.inc"
};
s32 gPlayerLookFiles_3_179[2] = {
#include "sub_main/004276D0.inc"
};
s32 gPlayerLookFiles_3_181[2] = {
#include "sub_main/004276D8.inc"
};
s32 gPlayerLookFiles_3_183[2] = {
#include "sub_main/004276E0.inc"
};
s32 gPlayerLookFiles_3_185[2] = {
#include "sub_main/004276E8.inc"
};
s32 gPlayerLookFiles_3_187[2] = {
#include "sub_main/004276F0.inc"
};
s32 gPlayerLookFiles_3_189[2] = {
#include "sub_main/004276F8.inc"
};
s32 gPlayerLookFiles_3_191[2] = {
#include "sub_main/00427700.inc"
};
s32 gPlayerLookFiles_3_193[2] = {
#include "sub_main/00427708.inc"
};
s32 gPlayerLookFiles_3_195[2] = {
#include "sub_main/00427710.inc"
};
s32 gPlayerLookFiles_3_197[2] = {
#include "sub_main/00427718.inc"
};
s32 gPlayerLookFiles_3_199[2] = {
#include "sub_main/00427720.inc"
};
s32 gPlayerLookFiles_3_201[2] = {
#include "sub_main/00427728.inc"
};
s32 gPlayerLookFiles_3_203[2] = {
#include "sub_main/00427730.inc"
};
s32 gPlayerLookFiles_3_205[2] = {
#include "sub_main/00427738.inc"
};
s32 gPlayerLookFiles_3_207[2] = {
#include "sub_main/00427740.inc"
};
s32 gPlayerLookFiles_3_209[2] = {
#include "sub_main/00427748.inc"
};
s32 gPlayerLookFiles_3_211[2] = {
#include "sub_main/00427750.inc"
};
s32 gPlayerLookFiles_3_213[2] = {
#include "sub_main/00427758.inc"
};
s32 gPlayerLookFiles_3_215[2] = {
#include "sub_main/00427760.inc"
};
s32 gPlayerLookFiles_3_217[2] = {
#include "sub_main/00427768.inc"
};
s32 gPlayerLookFiles_3_219[2] = {
#include "sub_main/00427770.inc"
};
s32 gPlayerLookFiles_3_221[2] = {
#include "sub_main/00427778.inc"
};
s32 gPlayerLookFiles_3_223[2] = {
#include "sub_main/00427780.inc"
};
s32 gPlayerLookFiles_3_225[2] = {
#include "sub_main/00427788.inc"
};
s32 gPlayerLookFiles_3_227[2] = {
#include "sub_main/00427790.inc"
};
s32 gPlayerLookFiles_3_229[2] = {
#include "sub_main/00427798.inc"
};
s32 gPlayerLookFiles_3_231[2] = {
#include "sub_main/004277A0.inc"
};
s32 gPlayerLookFiles_3_233[2] = {
#include "sub_main/004277A8.inc"
};
s32 gPlayerLookFiles_3_235[2] = {
#include "sub_main/004277B0.inc"
};
s32 gPlayerLookFiles_3_237[2] = {
#include "sub_main/004277B8.inc"
};
s32 gPlayerLookFiles_3_239[2] = {
#include "sub_main/004277C0.inc"
};
s32 gPlayerLookFiles_3_241[2] = {
#include "sub_main/004277C8.inc"
};
s32 gPlayerLookFiles_3_243[2] = {
#include "sub_main/004277D0.inc"
};
s32 gPlayerLookFiles_3_245[2] = {
#include "sub_main/004277D8.inc"
};
s32 gPlayerLookFiles_3_247[2] = {
#include "sub_main/004277E0.inc"
};
s32 gPlayerLookFiles_3_249[2] = {
#include "sub_main/004277E8.inc"
};
s32 gPlayerLookFiles_3_251[2] = {
#include "sub_main/004277F0.inc"
};
s32 gPlayerLookFiles_3_253[2] = {
#include "sub_main/004277F8.inc"
};
s32 gPlayerLookFiles_3_255[2] = {
#include "sub_main/00427800.inc"
};
s32 gPlayerLookFiles_3_257[2] = {
#include "sub_main/00427808.inc"
};
s32 gPlayerLookFiles_3_259[2] = {
#include "sub_main/00427810.inc"
};
s32 gPlayerLookFiles_3_261[2] = {
#include "sub_main/00427818.inc"
};
s32 gPlayerLookFiles_3_263[2] = {
#include "sub_main/00427820.inc"
};
s32 gPlayerLookFiles_3_265[2] = {
#include "sub_main/00427828.inc"
};
s32 gPlayerLookFiles_3_267[2] = {
#include "sub_main/00427830.inc"
};
s32 gPlayerLookFiles_3_269[2] = {
#include "sub_main/00427838.inc"
};
s32 gPlayerLookFiles_3_271[2] = {
#include "sub_main/00427840.inc"
};
s32 gPlayerLookFiles_3_273[2] = {
#include "sub_main/00427848.inc"
};
s32 gPlayerLookFiles_3_275[2] = {
#include "sub_main/00427850.inc"
};
s32 gPlayerLookFiles_3_277[2] = {
#include "sub_main/00427858.inc"
};
s32 gPlayerLookFiles_3_279[2] = {
#include "sub_main/00427860.inc"
};
s32 gPlayerLookFiles_3_281[2] = {
#include "sub_main/00427868.inc"
};
s32 gPlayerLookFiles_3_283[2] = {
#include "sub_main/00427870.inc"
};
s32 gPlayerLookFiles_3_285[2] = {
#include "sub_main/00427878.inc"
};
s32 gPlayerLookFiles_3_287[2] = {
#include "sub_main/00427880.inc"
};
s32 gPlayerLookFiles_3_289[2] = {
#include "sub_main/00427888.inc"
};
s32 gPlayerLookFiles_3_291[2] = {
#include "sub_main/00427890.inc"
};
s32 gPlayerLookFiles_3_293[2] = {
#include "sub_main/00427898.inc"
};
s32 gPlayerLookFiles_3_295[2] = {
#include "sub_main/004278A0.inc"
};
s32 gPlayerLookFiles_3_297[2] = {
#include "sub_main/004278A8.inc"
};
s32 gPlayerLookFiles_3_299[2] = {
#include "sub_main/004278B0.inc"
};
s32 gPlayerLookFiles_3_301[2] = {
#include "sub_main/004278B8.inc"
};
s32 gPlayerLookFiles_3_303[2] = {
#include "sub_main/004278C0.inc"
};
s32 gPlayerLookFiles_3_305[2] = {
#include "sub_main/004278C8.inc"
};
s32 gPlayerLookFiles_3_307[2] = {
#include "sub_main/004278D0.inc"
};
s32 gPlayerLookFiles_3_309[2] = {
#include "sub_main/004278D8.inc"
};
s32 gPlayerLookFiles_3_311[2] = {
#include "sub_main/004278E0.inc"
};
s32 gPlayerLookFiles_3_313[2] = {
#include "sub_main/004278E8.inc"
};
s32 gPlayerLookFiles_3_315[2] = {
#include "sub_main/004278F0.inc"
};
s32 gPlayerLookFiles_3_317[2] = {
#include "sub_main/004278F8.inc"
};
s32 gPlayerLookFiles_3_319[2] = {
#include "sub_main/00427900.inc"
};
s32 gPlayerLookFiles_3_321[2] = {
#include "sub_main/00427908.inc"
};
s32 gPlayerLookFiles_3_323[2] = {
#include "sub_main/00427910.inc"
};
s32 gPlayerLookFiles_3_325[2] = {
#include "sub_main/00427918.inc"
};
s32 gPlayerLookFiles_3_327[2] = {
#include "sub_main/00427920.inc"
};
s32 gPlayerLookFiles_3_329[2] = {
#include "sub_main/00427928.inc"
};
s32 gPlayerLookFiles_3_331[2] = {
#include "sub_main/00427930.inc"
};
s32 gPlayerLookFiles_3_333[2] = {
#include "sub_main/00427938.inc"
};
s32 gPlayerLookFiles_3_335[2] = {
#include "sub_main/00427940.inc"
};
s32 gPlayerLookFiles_3_337[2] = {
#include "sub_main/00427948.inc"
};
s32 gPlayerLookFiles_3_339[2] = {
#include "sub_main/00427950.inc"
};
s32 gPlayerLookFiles_3_341[2] = {
#include "sub_main/00427958.inc"
};
s32 gPlayerLookFiles_3_343[2] = {
#include "sub_main/00427960.inc"
};
s32 gPlayerLookFiles_3_345[2] = {
#include "sub_main/00427968.inc"
};
s32 gPlayerLookFiles_3_347[2] = {
#include "sub_main/00427970.inc"
};
s32 gPlayerLookFiles_3_349[2] = {
#include "sub_main/00427978.inc"
};
s32 gPlayerLookFiles_3_351[2] = {
#include "sub_main/00427980.inc"
};
s32 gPlayerLookFiles_3_353[2] = {
#include "sub_main/00427988.inc"
};
s32 gPlayerLookFiles_3_355[2] = {
#include "sub_main/00427990.inc"
};
s32 gPlayerLookFiles_3_357[2] = {
#include "sub_main/00427998.inc"
};
s32 gPlayerLookFiles_3_359[2] = {
#include "sub_main/004279A0.inc"
};
s32 gPlayerLookFiles_3_361[2] = {
#include "sub_main/004279A8.inc"
};
s32 gPlayerLookFiles_3_363[2] = {
#include "sub_main/004279B0.inc"
};
s32 gPlayerLookFiles_3_365[2] = {
#include "sub_main/004279B8.inc"
};
s32 gPlayerLookFiles_3_367[2] = {
#include "sub_main/004279C0.inc"
};
s32 gPlayerLookFiles_3_369[2] = {
#include "sub_main/004279C8.inc"
};
s32 gPlayerLookFiles_3_371[2] = {
#include "sub_main/004279D0.inc"
};
s32 gPlayerLookFiles_3_373[2] = {
#include "sub_main/004279D8.inc"
};
s32 gPlayerLookFiles_3_375[2] = {
#include "sub_main/004279E0.inc"
};
s32 gPlayerLookFiles_3_377[2] = {
#include "sub_main/004279E8.inc"
};
s32 gPlayerLookFiles_3_379[2] = {
#include "sub_main/004279F0.inc"
};
s32 gPlayerLookFiles_3_381[2] = {
#include "sub_main/004279F8.inc"
};
s32 gPlayerLookFiles_3_383[2] = {
#include "sub_main/00427A00.inc"
};
s32 gPlayerLookFiles_3_385[2] = {
#include "sub_main/00427A08.inc"
};
s32 gPlayerLookFiles_3_387[2] = {
#include "sub_main/00427A10.inc"
};
s32 gPlayerLookFiles_3_389[2] = {
#include "sub_main/00427A18.inc"
};
s32 gPlayerLookFiles_3_391[2] = {
#include "sub_main/00427A20.inc"
};
s32 gPlayerLookFiles_3_393[2] = {
#include "sub_main/00427A28.inc"
};
s32 gPlayerLookFiles_3_395[2] = {
#include "sub_main/00427A30.inc"
};
s32 gPlayerLookFiles_3_397[2] = {
#include "sub_main/00427A38.inc"
};
s32 gPlayerLookFiles_3_399[2] = {
#include "sub_main/00427A40.inc"
};
s32 gPlayerLookFiles_3_401[2] = {
#include "sub_main/00427A48.inc"
};
s32 gPlayerLookFiles_3_403[2] = {
#include "sub_main/00427A50.inc"
};
s32 gPlayerLookFiles_3_405[2] = {
#include "sub_main/00427A58.inc"
};
s32 gPlayerLookFiles_3_407[2] = {
#include "sub_main/00427A60.inc"
};
s32 gPlayerLookFiles_3_409[2] = {
#include "sub_main/00427A68.inc"
};
s32 gPlayerLookFiles_3_411[2] = {
#include "sub_main/00427A70.inc"
};
s32 gPlayerLookFiles_3_413[2] = {
#include "sub_main/00427A78.inc"
};
s32 gPlayerLookFiles_3_415[2] = {
#include "sub_main/00427A80.inc"
};
s32 gPlayerLookFiles_3_417[2] = {
#include "sub_main/00427A88.inc"
};
s32 gPlayerLookFiles_3_419[2] = {
#include "sub_main/00427A90.inc"
};
s32 gPlayerLookFiles_3_421[2] = {
#include "sub_main/00427A98.inc"
};
s32 gPlayerLookFiles_3_423[2] = {
#include "sub_main/00427AA0.inc"
};
s32 gPlayerLookFiles_3_425[2] = {
#include "sub_main/00427AA8.inc"
};
s32 gPlayerLookFiles_3_427[2] = {
#include "sub_main/00427AB0.inc"
};
s32 gPlayerLookFiles_3_429[2] = {
#include "sub_main/00427AB8.inc"
};
s32 gPlayerLookFiles_3_431[2] = {
#include "sub_main/00427AC0.inc"
};
s32 gPlayerLookFiles_3_433[2] = {
#include "sub_main/00427AC8.inc"
};
s32 gPlayerLookFiles_3_435[2] = {
#include "sub_main/00427AD0.inc"
};
s32 gPlayerLookFiles_3_437[2] = {
#include "sub_main/00427AD8.inc"
};
s32 gPlayerLookFiles_3_439[2] = {
#include "sub_main/00427AE0.inc"
};
s32 gPlayerLookFiles_3_441[2] = {
#include "sub_main/00427AE8.inc"
};
s32 gPlayerLookFiles_3_443[2] = {
#include "sub_main/00427AF0.inc"
};
s32 gPlayerLookFiles_3_445[2] = {
#include "sub_main/00427AF8.inc"
};
s32 gPlayerLookFiles_3_447[2] = {
#include "sub_main/00427B00.inc"
};
s32 gPlayerLookFiles_3_449[2] = {
#include "sub_main/00427B08.inc"
};
s32 gPlayerLookFiles_3_451[2] = {
#include "sub_main/00427B10.inc"
};
s32 gPlayerLookFiles_3_453[2] = {
#include "sub_main/00427B18.inc"
};
s32 gPlayerLookFiles_3_455[2] = {
#include "sub_main/00427B20.inc"
};
s32 gPlayerLookFiles_3_457[2] = {
#include "sub_main/00427B28.inc"
};
s32 gPlayerLookFiles_3_459[2] = {
#include "sub_main/00427B30.inc"
};
s32 gPlayerLookFiles_3_461[2] = {
#include "sub_main/00427B38.inc"
};
s32 gPlayerLookFiles_3_463[2] = {
#include "sub_main/00427B40.inc"
};
s32 gPlayerLookFiles_3_465[2] = {
#include "sub_main/00427B48.inc"
};
s32 gPlayerLookFiles_3_467[2] = {
#include "sub_main/00427B50.inc"
};
s32 gPlayerLookFiles_3_469[2] = {
#include "sub_main/00427B58.inc"
};
s32 gPlayerLookFiles_3_471[4] = {
#include "sub_main/00427B60.inc"
};
TexPackHead gPlayerLookFiles_3[236] = {
#include "sub_main/00427B70.inc"
};
s32 gPlayerLookFiles_4_1[2] = {
#include "sub_main/004282D0.inc"
};
s32 gPlayerLookFiles_4_3[2] = {
#include "sub_main/004282D8.inc"
};
s32 gPlayerLookFiles_4_5[2] = {
#include "sub_main/004282E0.inc"
};
s32 gPlayerLookFiles_4_7[2] = {
#include "sub_main/004282E8.inc"
};
s32 gPlayerLookFiles_4_9[2] = {
#include "sub_main/004282F0.inc"
};
s32 gPlayerLookFiles_4_11[2] = {
#include "sub_main/004282F8.inc"
};
s32 gPlayerLookFiles_4_13[2] = {
#include "sub_main/00428300.inc"
};
s32 gPlayerLookFiles_4_15[2] = {
#include "sub_main/00428308.inc"
};
s32 gPlayerLookFiles_4_17[2] = {
#include "sub_main/00428310.inc"
};
s32 gPlayerLookFiles_4_19[2] = {
#include "sub_main/00428318.inc"
};
s32 gPlayerLookFiles_4_21[2] = {
#include "sub_main/00428320.inc"
};
s32 gPlayerLookFiles_4_23[2] = {
#include "sub_main/00428328.inc"
};
s32 gPlayerLookFiles_4_25[2] = {
#include "sub_main/00428330.inc"
};
s32 gPlayerLookFiles_4_27[2] = {
#include "sub_main/00428338.inc"
};
s32 gPlayerLookFiles_4_29[2] = {
#include "sub_main/00428340.inc"
};
s32 gPlayerLookFiles_4_31[2] = {
#include "sub_main/00428348.inc"
};
s32 gPlayerLookFiles_4_33[2] = {
#include "sub_main/00428350.inc"
};
s32 gPlayerLookFiles_4_35[2] = {
#include "sub_main/00428358.inc"
};
s32 gPlayerLookFiles_4_37[2] = {
#include "sub_main/00428360.inc"
};
s32 gPlayerLookFiles_4_39[2] = {
#include "sub_main/00428368.inc"
};
s32 gPlayerLookFiles_4_41[2] = {
#include "sub_main/00428370.inc"
};
s32 gPlayerLookFiles_4_43[2] = {
#include "sub_main/00428378.inc"
};
s32 gPlayerLookFiles_4_45[2] = {
#include "sub_main/00428380.inc"
};
s32 gPlayerLookFiles_4_47[2] = {
#include "sub_main/00428388.inc"
};
s32 gPlayerLookFiles_4_49[2] = {
#include "sub_main/00428390.inc"
};
s32 gPlayerLookFiles_4_51[2] = {
#include "sub_main/00428398.inc"
};
s32 gPlayerLookFiles_4_53[2] = {
#include "sub_main/004283A0.inc"
};
s32 gPlayerLookFiles_4_55[2] = {
#include "sub_main/004283A8.inc"
};
s32 gPlayerLookFiles_4_57[2] = {
#include "sub_main/004283B0.inc"
};
s32 gPlayerLookFiles_4_59[2] = {
#include "sub_main/004283B8.inc"
};
s32 gPlayerLookFiles_4_61[2] = {
#include "sub_main/004283C0.inc"
};
s32 gPlayerLookFiles_4_63[2] = {
#include "sub_main/004283C8.inc"
};
s32 gPlayerLookFiles_4_65[2] = {
#include "sub_main/004283D0.inc"
};
s32 gPlayerLookFiles_4_67[2] = {
#include "sub_main/004283D8.inc"
};
s32 gPlayerLookFiles_4_69[2] = {
#include "sub_main/004283E0.inc"
};
s32 gPlayerLookFiles_4_71[2] = {
#include "sub_main/004283E8.inc"
};
s32 gPlayerLookFiles_4_73[2] = {
#include "sub_main/004283F0.inc"
};
s32 gPlayerLookFiles_4_75[2] = {
#include "sub_main/004283F8.inc"
};
s32 gPlayerLookFiles_4_77[2] = {
#include "sub_main/00428400.inc"
};
s32 gPlayerLookFiles_4_79[2] = {
#include "sub_main/00428408.inc"
};
s32 gPlayerLookFiles_4_81[2] = {
#include "sub_main/00428410.inc"
};
s32 gPlayerLookFiles_4_83[2] = {
#include "sub_main/00428418.inc"
};
s32 gPlayerLookFiles_4_85[2] = {
#include "sub_main/00428420.inc"
};
s32 gPlayerLookFiles_4_87[2] = {
#include "sub_main/00428428.inc"
};
s32 gPlayerLookFiles_4_89[2] = {
#include "sub_main/00428430.inc"
};
s32 gPlayerLookFiles_4_91[2] = {
#include "sub_main/00428438.inc"
};
s32 gPlayerLookFiles_4_93[2] = {
#include "sub_main/00428440.inc"
};
s32 gPlayerLookFiles_4_95[2] = {
#include "sub_main/00428448.inc"
};
s32 gPlayerLookFiles_4_97[2] = {
#include "sub_main/00428450.inc"
};
s32 gPlayerLookFiles_4_99[2] = {
#include "sub_main/00428458.inc"
};
s32 gPlayerLookFiles_4_101[2] = {
#include "sub_main/00428460.inc"
};
s32 gPlayerLookFiles_4_103[2] = {
#include "sub_main/00428468.inc"
};
s32 gPlayerLookFiles_4_105[2] = {
#include "sub_main/00428470.inc"
};
s32 gPlayerLookFiles_4_107[2] = {
#include "sub_main/00428478.inc"
};
s32 gPlayerLookFiles_4_109[2] = {
#include "sub_main/00428480.inc"
};
s32 gPlayerLookFiles_4_111[2] = {
#include "sub_main/00428488.inc"
};
s32 gPlayerLookFiles_4_113[2] = {
#include "sub_main/00428490.inc"
};
s32 gPlayerLookFiles_4_115[2] = {
#include "sub_main/00428498.inc"
};
s32 gPlayerLookFiles_4_117[2] = {
#include "sub_main/004284A0.inc"
};
s32 gPlayerLookFiles_4_119[2] = {
#include "sub_main/004284A8.inc"
};
s32 gPlayerLookFiles_4_121[2] = {
#include "sub_main/004284B0.inc"
};
s32 gPlayerLookFiles_4_123[2] = {
#include "sub_main/004284B8.inc"
};
s32 gPlayerLookFiles_4_125[2] = {
#include "sub_main/004284C0.inc"
};
s32 gPlayerLookFiles_4_127[2] = {
#include "sub_main/004284C8.inc"
};
s32 gPlayerLookFiles_4_129[2] = {
#include "sub_main/004284D0.inc"
};
s32 gPlayerLookFiles_4_131[2] = {
#include "sub_main/004284D8.inc"
};
s32 gPlayerLookFiles_4_133[2] = {
#include "sub_main/004284E0.inc"
};
s32 gPlayerLookFiles_4_135[2] = {
#include "sub_main/004284E8.inc"
};
s32 gPlayerLookFiles_4_137[2] = {
#include "sub_main/004284F0.inc"
};
s32 gPlayerLookFiles_4_139[2] = {
#include "sub_main/004284F8.inc"
};
s32 gPlayerLookFiles_4_141[2] = {
#include "sub_main/00428500.inc"
};
s32 gPlayerLookFiles_4_143[2] = {
#include "sub_main/00428508.inc"
};
s32 gPlayerLookFiles_4_145[2] = {
#include "sub_main/00428510.inc"
};
s32 gPlayerLookFiles_4_147[2] = {
#include "sub_main/00428518.inc"
};
s32 gPlayerLookFiles_4_149[2] = {
#include "sub_main/00428520.inc"
};
s32 gPlayerLookFiles_4_151[2] = {
#include "sub_main/00428528.inc"
};
s32 gPlayerLookFiles_4_153[2] = {
#include "sub_main/00428530.inc"
};
s32 gPlayerLookFiles_4_155[2] = {
#include "sub_main/00428538.inc"
};
s32 gPlayerLookFiles_4_157[2] = {
#include "sub_main/00428540.inc"
};
s32 gPlayerLookFiles_4_159[2] = {
#include "sub_main/00428548.inc"
};
s32 gPlayerLookFiles_4_161[2] = {
#include "sub_main/00428550.inc"
};
s32 gPlayerLookFiles_4_163[2] = {
#include "sub_main/00428558.inc"
};
s32 gPlayerLookFiles_4_165[2] = {
#include "sub_main/00428560.inc"
};
s32 gPlayerLookFiles_4_167[2] = {
#include "sub_main/00428568.inc"
};
s32 gPlayerLookFiles_4_169[2] = {
#include "sub_main/00428570.inc"
};
s32 gPlayerLookFiles_4_171[2] = {
#include "sub_main/00428578.inc"
};
s32 gPlayerLookFiles_4_173[2] = {
#include "sub_main/00428580.inc"
};
s32 gPlayerLookFiles_4_175[2] = {
#include "sub_main/00428588.inc"
};
s32 gPlayerLookFiles_4_177[2] = {
#include "sub_main/00428590.inc"
};
s32 gPlayerLookFiles_4_179[2] = {
#include "sub_main/00428598.inc"
};
s32 gPlayerLookFiles_4_181[2] = {
#include "sub_main/004285A0.inc"
};
s32 gPlayerLookFiles_4_183[2] = {
#include "sub_main/004285A8.inc"
};
s32 gPlayerLookFiles_4_185[2] = {
#include "sub_main/004285B0.inc"
};
s32 gPlayerLookFiles_4_187[2] = {
#include "sub_main/004285B8.inc"
};
s32 gPlayerLookFiles_4_189[2] = {
#include "sub_main/004285C0.inc"
};
s32 gPlayerLookFiles_4_191[2] = {
#include "sub_main/004285C8.inc"
};
s32 gPlayerLookFiles_4_193[2] = {
#include "sub_main/004285D0.inc"
};
s32 gPlayerLookFiles_4_195[2] = {
#include "sub_main/004285D8.inc"
};
s32 gPlayerLookFiles_4_197[2] = {
#include "sub_main/004285E0.inc"
};
s32 gPlayerLookFiles_4_199[2] = {
#include "sub_main/004285E8.inc"
};
s32 gPlayerLookFiles_4_201[2] = {
#include "sub_main/004285F0.inc"
};
s32 gPlayerLookFiles_4_203[2] = {
#include "sub_main/004285F8.inc"
};
s32 gPlayerLookFiles_4_205[2] = {
#include "sub_main/00428600.inc"
};
s32 gPlayerLookFiles_4_207[2] = {
#include "sub_main/00428608.inc"
};
s32 gPlayerLookFiles_4_209[2] = {
#include "sub_main/00428610.inc"
};
s32 gPlayerLookFiles_4_211[2] = {
#include "sub_main/00428618.inc"
};
s32 gPlayerLookFiles_4_213[2] = {
#include "sub_main/00428620.inc"
};
s32 gPlayerLookFiles_4_215[2] = {
#include "sub_main/00428628.inc"
};
s32 gPlayerLookFiles_4_217[2] = {
#include "sub_main/00428630.inc"
};
s32 gPlayerLookFiles_4_219[2] = {
#include "sub_main/00428638.inc"
};
s32 gPlayerLookFiles_4_221[2] = {
#include "sub_main/00428640.inc"
};
s32 gPlayerLookFiles_4_223[2] = {
#include "sub_main/00428648.inc"
};
s32 gPlayerLookFiles_4_225[2] = {
#include "sub_main/00428650.inc"
};
s32 gPlayerLookFiles_4_227[2] = {
#include "sub_main/00428658.inc"
};
s32 gPlayerLookFiles_4_229[2] = {
#include "sub_main/00428660.inc"
};
s32 gPlayerLookFiles_4_231[2] = {
#include "sub_main/00428668.inc"
};
s32 gPlayerLookFiles_4_233[2] = {
#include "sub_main/00428670.inc"
};
s32 gPlayerLookFiles_4_235[2] = {
#include "sub_main/00428678.inc"
};
s32 gPlayerLookFiles_4_237[2] = {
#include "sub_main/00428680.inc"
};
s32 gPlayerLookFiles_4_239[2] = {
#include "sub_main/00428688.inc"
};
s32 gPlayerLookFiles_4_241[2] = {
#include "sub_main/00428690.inc"
};
s32 gPlayerLookFiles_4_243[2] = {
#include "sub_main/00428698.inc"
};
s32 gPlayerLookFiles_4_245[2] = {
#include "sub_main/004286A0.inc"
};
s32 gPlayerLookFiles_4_247[2] = {
#include "sub_main/004286A8.inc"
};
s32 gPlayerLookFiles_4_249[2] = {
#include "sub_main/004286B0.inc"
};
s32 gPlayerLookFiles_4_251[2] = {
#include "sub_main/004286B8.inc"
};
s32 gPlayerLookFiles_4_253[2] = {
#include "sub_main/004286C0.inc"
};
s32 gPlayerLookFiles_4_255[2] = {
#include "sub_main/004286C8.inc"
};
s32 gPlayerLookFiles_4_257[2] = {
#include "sub_main/004286D0.inc"
};
s32 gPlayerLookFiles_4_259[2] = {
#include "sub_main/004286D8.inc"
};
s32 gPlayerLookFiles_4_261[2] = {
#include "sub_main/004286E0.inc"
};
s32 gPlayerLookFiles_4_263[2] = {
#include "sub_main/004286E8.inc"
};
s32 gPlayerLookFiles_4_265[2] = {
#include "sub_main/004286F0.inc"
};
s32 gPlayerLookFiles_4_267[2] = {
#include "sub_main/004286F8.inc"
};
s32 gPlayerLookFiles_4_269[2] = {
#include "sub_main/00428700.inc"
};
s32 gPlayerLookFiles_4_271[2] = {
#include "sub_main/00428708.inc"
};
s32 gPlayerLookFiles_4_273[2] = {
#include "sub_main/00428710.inc"
};
s32 gPlayerLookFiles_4_275[2] = {
#include "sub_main/00428718.inc"
};
s32 gPlayerLookFiles_4_277[2] = {
#include "sub_main/00428720.inc"
};
s32 gPlayerLookFiles_4_279[2] = {
#include "sub_main/00428728.inc"
};
s32 gPlayerLookFiles_4_281[2] = {
#include "sub_main/00428730.inc"
};
s32 gPlayerLookFiles_4_283[2] = {
#include "sub_main/00428738.inc"
};
s32 gPlayerLookFiles_4_285[2] = {
#include "sub_main/00428740.inc"
};
s32 gPlayerLookFiles_4_287[2] = {
#include "sub_main/00428748.inc"
};
s32 gPlayerLookFiles_4_289[2] = {
#include "sub_main/00428750.inc"
};
s32 gPlayerLookFiles_4_291[2] = {
#include "sub_main/00428758.inc"
};
s32 gPlayerLookFiles_4_293[2] = {
#include "sub_main/00428760.inc"
};
s32 gPlayerLookFiles_4_295[2] = {
#include "sub_main/00428768.inc"
};
s32 gPlayerLookFiles_4_297[2] = {
#include "sub_main/00428770.inc"
};
s32 gPlayerLookFiles_4_299[2] = {
#include "sub_main/00428778.inc"
};
s32 gPlayerLookFiles_4_301[2] = {
#include "sub_main/00428780.inc"
};
s32 gPlayerLookFiles_4_303[2] = {
#include "sub_main/00428788.inc"
};
s32 gPlayerLookFiles_4_305[2] = {
#include "sub_main/00428790.inc"
};
s32 gPlayerLookFiles_4_307[2] = {
#include "sub_main/00428798.inc"
};
s32 gPlayerLookFiles_4_309[2] = {
#include "sub_main/004287A0.inc"
};
s32 gPlayerLookFiles_4_311[2] = {
#include "sub_main/004287A8.inc"
};
s32 gPlayerLookFiles_4_313[2] = {
#include "sub_main/004287B0.inc"
};
s32 gPlayerLookFiles_4_315[2] = {
#include "sub_main/004287B8.inc"
};
s32 gPlayerLookFiles_4_317[2] = {
#include "sub_main/004287C0.inc"
};
s32 gPlayerLookFiles_4_319[2] = {
#include "sub_main/004287C8.inc"
};
s32 gPlayerLookFiles_4_321[2] = {
#include "sub_main/004287D0.inc"
};
s32 gPlayerLookFiles_4_323[2] = {
#include "sub_main/004287D8.inc"
};
s32 gPlayerLookFiles_4_325[2] = {
#include "sub_main/004287E0.inc"
};
s32 gPlayerLookFiles_4_327[2] = {
#include "sub_main/004287E8.inc"
};
s32 gPlayerLookFiles_4_329[2] = {
#include "sub_main/004287F0.inc"
};
s32 gPlayerLookFiles_4_331[2] = {
#include "sub_main/004287F8.inc"
};
s32 gPlayerLookFiles_4_333[2] = {
#include "sub_main/00428800.inc"
};
s32 gPlayerLookFiles_4_335[2] = {
#include "sub_main/00428808.inc"
};
s32 gPlayerLookFiles_4_337[2] = {
#include "sub_main/00428810.inc"
};
s32 gPlayerLookFiles_4_339[2] = {
#include "sub_main/00428818.inc"
};
s32 gPlayerLookFiles_4_341[2] = {
#include "sub_main/00428820.inc"
};
s32 gPlayerLookFiles_4_343[2] = {
#include "sub_main/00428828.inc"
};
s32 gPlayerLookFiles_4_345[2] = {
#include "sub_main/00428830.inc"
};
s32 gPlayerLookFiles_4_347[2] = {
#include "sub_main/00428838.inc"
};
s32 gPlayerLookFiles_4_349[2] = {
#include "sub_main/00428840.inc"
};
s32 gPlayerLookFiles_4_351[2] = {
#include "sub_main/00428848.inc"
};
s32 gPlayerLookFiles_4_353[2] = {
#include "sub_main/00428850.inc"
};
s32 gPlayerLookFiles_4_355[2] = {
#include "sub_main/00428858.inc"
};
s32 gPlayerLookFiles_4_357[2] = {
#include "sub_main/00428860.inc"
};
s32 gPlayerLookFiles_4_359[2] = {
#include "sub_main/00428868.inc"
};
s32 gPlayerLookFiles_4_361[2] = {
#include "sub_main/00428870.inc"
};
s32 gPlayerLookFiles_4_363[2] = {
#include "sub_main/00428878.inc"
};
s32 gPlayerLookFiles_4_365[2] = {
#include "sub_main/00428880.inc"
};
s32 gPlayerLookFiles_4_367[2] = {
#include "sub_main/00428888.inc"
};
s32 gPlayerLookFiles_4_369[2] = {
#include "sub_main/00428890.inc"
};
s32 gPlayerLookFiles_4_371[2] = {
#include "sub_main/00428898.inc"
};
s32 gPlayerLookFiles_4_373[2] = {
#include "sub_main/004288A0.inc"
};
s32 gPlayerLookFiles_4_375[2] = {
#include "sub_main/004288A8.inc"
};
s32 gPlayerLookFiles_4_377[2] = {
#include "sub_main/004288B0.inc"
};
s32 gPlayerLookFiles_4_379[2] = {
#include "sub_main/004288B8.inc"
};
s32 gPlayerLookFiles_4_381[2] = {
#include "sub_main/004288C0.inc"
};
s32 gPlayerLookFiles_4_383[2] = {
#include "sub_main/004288C8.inc"
};
s32 gPlayerLookFiles_4_385[2] = {
#include "sub_main/004288D0.inc"
};
s32 gPlayerLookFiles_4_387[2] = {
#include "sub_main/004288D8.inc"
};
s32 gPlayerLookFiles_4_389[2] = {
#include "sub_main/004288E0.inc"
};
s32 gPlayerLookFiles_4_391[2] = {
#include "sub_main/004288E8.inc"
};
s32 gPlayerLookFiles_4_393[2] = {
#include "sub_main/004288F0.inc"
};
s32 gPlayerLookFiles_4_395[2] = {
#include "sub_main/004288F8.inc"
};
s32 gPlayerLookFiles_4_397[2] = {
#include "sub_main/00428900.inc"
};
s32 gPlayerLookFiles_4_399[2] = {
#include "sub_main/00428908.inc"
};
s32 gPlayerLookFiles_4_401[2] = {
#include "sub_main/00428910.inc"
};
s32 gPlayerLookFiles_4_403[2] = {
#include "sub_main/00428918.inc"
};
s32 gPlayerLookFiles_4_405[2] = {
#include "sub_main/00428920.inc"
};
s32 gPlayerLookFiles_4_407[2] = {
#include "sub_main/00428928.inc"
};
s32 gPlayerLookFiles_4_409[2] = {
#include "sub_main/00428930.inc"
};
s32 gPlayerLookFiles_4_411[2] = {
#include "sub_main/00428938.inc"
};
s32 gPlayerLookFiles_4_413[2] = {
#include "sub_main/00428940.inc"
};
s32 gPlayerLookFiles_4_415[2] = {
#include "sub_main/00428948.inc"
};
s32 gPlayerLookFiles_4_417[2] = {
#include "sub_main/00428950.inc"
};
s32 gPlayerLookFiles_4_419[2] = {
#include "sub_main/00428958.inc"
};
s32 gPlayerLookFiles_4_421[2] = {
#include "sub_main/00428960.inc"
};
s32 gPlayerLookFiles_4_423[2] = {
#include "sub_main/00428968.inc"
};
s32 gPlayerLookFiles_4_425[2] = {
#include "sub_main/00428970.inc"
};
s32 gPlayerLookFiles_4_427[2] = {
#include "sub_main/00428978.inc"
};
s32 gPlayerLookFiles_4_429[2] = {
#include "sub_main/00428980.inc"
};
s32 gPlayerLookFiles_4_431[2] = {
#include "sub_main/00428988.inc"
};
s32 gPlayerLookFiles_4_433[2] = {
#include "sub_main/00428990.inc"
};
s32 gPlayerLookFiles_4_435[2] = {
#include "sub_main/00428998.inc"
};
s32 gPlayerLookFiles_4_437[2] = {
#include "sub_main/004289A0.inc"
};
s32 gPlayerLookFiles_4_439[2] = {
#include "sub_main/004289A8.inc"
};
s32 gPlayerLookFiles_4_441[2] = {
#include "sub_main/004289B0.inc"
};
s32 gPlayerLookFiles_4_443[2] = {
#include "sub_main/004289B8.inc"
};
s32 gPlayerLookFiles_4_445[2] = {
#include "sub_main/004289C0.inc"
};
s32 gPlayerLookFiles_4_447[2] = {
#include "sub_main/004289C8.inc"
};
s32 gPlayerLookFiles_4_449[2] = {
#include "sub_main/004289D0.inc"
};
s32 gPlayerLookFiles_4_451[2] = {
#include "sub_main/004289D8.inc"
};
s32 gPlayerLookFiles_4_453[2] = {
#include "sub_main/004289E0.inc"
};
s32 gPlayerLookFiles_4_455[2] = {
#include "sub_main/004289E8.inc"
};
TexPackHead gPlayerLookFiles_4[228] = {
#include "sub_main/004289F0.inc"
};
s32 gPlayerLookFiles_5_1[2] = {
#include "sub_main/00429110.inc"
};
s32 gPlayerLookFiles_5_3[2] = {
#include "sub_main/00429118.inc"
};
s32 gPlayerLookFiles_5_5[2] = {
#include "sub_main/00429120.inc"
};
s32 gPlayerLookFiles_5_7[2] = {
#include "sub_main/00429128.inc"
};
s32 gPlayerLookFiles_5_9[2] = {
#include "sub_main/00429130.inc"
};
s32 gPlayerLookFiles_5_11[2] = {
#include "sub_main/00429138.inc"
};
s32 gPlayerLookFiles_5_13[2] = {
#include "sub_main/00429140.inc"
};
s32 gPlayerLookFiles_5_15[2] = {
#include "sub_main/00429148.inc"
};
s32 gPlayerLookFiles_5_17[2] = {
#include "sub_main/00429150.inc"
};
s32 gPlayerLookFiles_5_19[2] = {
#include "sub_main/00429158.inc"
};
s32 gPlayerLookFiles_5_21[2] = {
#include "sub_main/00429160.inc"
};
s32 gPlayerLookFiles_5_23[2] = {
#include "sub_main/00429168.inc"
};
s32 gPlayerLookFiles_5_25[2] = {
#include "sub_main/00429170.inc"
};
s32 gPlayerLookFiles_5_27[2] = {
#include "sub_main/00429178.inc"
};
s32 gPlayerLookFiles_5_29[2] = {
#include "sub_main/00429180.inc"
};
s32 gPlayerLookFiles_5_31[2] = {
#include "sub_main/00429188.inc"
};
s32 gPlayerLookFiles_5_33[2] = {
#include "sub_main/00429190.inc"
};
s32 gPlayerLookFiles_5_35[2] = {
#include "sub_main/00429198.inc"
};
s32 gPlayerLookFiles_5_37[2] = {
#include "sub_main/004291A0.inc"
};
s32 gPlayerLookFiles_5_39[2] = {
#include "sub_main/004291A8.inc"
};
s32 gPlayerLookFiles_5_41[2] = {
#include "sub_main/004291B0.inc"
};
s32 gPlayerLookFiles_5_43[2] = {
#include "sub_main/004291B8.inc"
};
s32 gPlayerLookFiles_5_45[2] = {
#include "sub_main/004291C0.inc"
};
s32 gPlayerLookFiles_5_47[2] = {
#include "sub_main/004291C8.inc"
};
s32 gPlayerLookFiles_5_49[2] = {
#include "sub_main/004291D0.inc"
};
s32 gPlayerLookFiles_5_51[2] = {
#include "sub_main/004291D8.inc"
};
s32 gPlayerLookFiles_5_53[2] = {
#include "sub_main/004291E0.inc"
};
s32 gPlayerLookFiles_5_55[2] = {
#include "sub_main/004291E8.inc"
};
s32 gPlayerLookFiles_5_57[2] = {
#include "sub_main/004291F0.inc"
};
s32 gPlayerLookFiles_5_59[2] = {
#include "sub_main/004291F8.inc"
};
s32 gPlayerLookFiles_5_61[2] = {
#include "sub_main/00429200.inc"
};
s32 gPlayerLookFiles_5_63[2] = {
#include "sub_main/00429208.inc"
};
s32 gPlayerLookFiles_5_65[2] = {
#include "sub_main/00429210.inc"
};
s32 gPlayerLookFiles_5_67[2] = {
#include "sub_main/00429218.inc"
};
s32 gPlayerLookFiles_5_69[2] = {
#include "sub_main/00429220.inc"
};
s32 gPlayerLookFiles_5_71[2] = {
#include "sub_main/00429228.inc"
};
s32 gPlayerLookFiles_5_73[2] = {
#include "sub_main/00429230.inc"
};
s32 gPlayerLookFiles_5_75[2] = {
#include "sub_main/00429238.inc"
};
s32 gPlayerLookFiles_5_77[2] = {
#include "sub_main/00429240.inc"
};
s32 gPlayerLookFiles_5_79[2] = {
#include "sub_main/00429248.inc"
};
s32 gPlayerLookFiles_5_81[2] = {
#include "sub_main/00429250.inc"
};
s32 gPlayerLookFiles_5_83[2] = {
#include "sub_main/00429258.inc"
};
s32 gPlayerLookFiles_5_85[2] = {
#include "sub_main/00429260.inc"
};
s32 gPlayerLookFiles_5_87[2] = {
#include "sub_main/00429268.inc"
};
s32 gPlayerLookFiles_5_89[2] = {
#include "sub_main/00429270.inc"
};
s32 gPlayerLookFiles_5_91[2] = {
#include "sub_main/00429278.inc"
};
s32 gPlayerLookFiles_5_93[2] = {
#include "sub_main/00429280.inc"
};
s32 gPlayerLookFiles_5_95[2] = {
#include "sub_main/00429288.inc"
};
s32 gPlayerLookFiles_5_97[2] = {
#include "sub_main/00429290.inc"
};
s32 gPlayerLookFiles_5_99[2] = {
#include "sub_main/00429298.inc"
};
s32 gPlayerLookFiles_5_101[2] = {
#include "sub_main/004292A0.inc"
};
s32 gPlayerLookFiles_5_103[2] = {
#include "sub_main/004292A8.inc"
};
s32 gPlayerLookFiles_5_105[2] = {
#include "sub_main/004292B0.inc"
};
s32 gPlayerLookFiles_5_107[2] = {
#include "sub_main/004292B8.inc"
};
s32 gPlayerLookFiles_5_109[2] = {
#include "sub_main/004292C0.inc"
};
s32 gPlayerLookFiles_5_111[2] = {
#include "sub_main/004292C8.inc"
};
s32 gPlayerLookFiles_5_113[2] = {
#include "sub_main/004292D0.inc"
};
s32 gPlayerLookFiles_5_115[2] = {
#include "sub_main/004292D8.inc"
};
s32 gPlayerLookFiles_5_117[2] = {
#include "sub_main/004292E0.inc"
};
s32 gPlayerLookFiles_5_119[2] = {
#include "sub_main/004292E8.inc"
};
s32 gPlayerLookFiles_5_121[2] = {
#include "sub_main/004292F0.inc"
};
s32 gPlayerLookFiles_5_123[2] = {
#include "sub_main/004292F8.inc"
};
s32 gPlayerLookFiles_5_125[2] = {
#include "sub_main/00429300.inc"
};
s32 gPlayerLookFiles_5_127[2] = {
#include "sub_main/00429308.inc"
};
s32 gPlayerLookFiles_5_129[2] = {
#include "sub_main/00429310.inc"
};
s32 gPlayerLookFiles_5_131[2] = {
#include "sub_main/00429318.inc"
};
s32 gPlayerLookFiles_5_133[2] = {
#include "sub_main/00429320.inc"
};
s32 gPlayerLookFiles_5_135[2] = {
#include "sub_main/00429328.inc"
};
s32 gPlayerLookFiles_5_137[2] = {
#include "sub_main/00429330.inc"
};
s32 gPlayerLookFiles_5_139[2] = {
#include "sub_main/00429338.inc"
};
s32 gPlayerLookFiles_5_141[2] = {
#include "sub_main/00429340.inc"
};
s32 gPlayerLookFiles_5_143[2] = {
#include "sub_main/00429348.inc"
};
s32 gPlayerLookFiles_5_145[2] = {
#include "sub_main/00429350.inc"
};
s32 gPlayerLookFiles_5_147[2] = {
#include "sub_main/00429358.inc"
};
s32 gPlayerLookFiles_5_149[2] = {
#include "sub_main/00429360.inc"
};
s32 gPlayerLookFiles_5_151[2] = {
#include "sub_main/00429368.inc"
};
s32 gPlayerLookFiles_5_153[2] = {
#include "sub_main/00429370.inc"
};
s32 gPlayerLookFiles_5_155[2] = {
#include "sub_main/00429378.inc"
};
s32 gPlayerLookFiles_5_157[2] = {
#include "sub_main/00429380.inc"
};
s32 gPlayerLookFiles_5_159[2] = {
#include "sub_main/00429388.inc"
};
s32 gPlayerLookFiles_5_161[2] = {
#include "sub_main/00429390.inc"
};
s32 gPlayerLookFiles_5_163[2] = {
#include "sub_main/00429398.inc"
};
s32 gPlayerLookFiles_5_165[2] = {
#include "sub_main/004293A0.inc"
};
s32 gPlayerLookFiles_5_167[2] = {
#include "sub_main/004293A8.inc"
};
s32 gPlayerLookFiles_5_169[2] = {
#include "sub_main/004293B0.inc"
};
s32 gPlayerLookFiles_5_171[2] = {
#include "sub_main/004293B8.inc"
};
s32 gPlayerLookFiles_5_173[2] = {
#include "sub_main/004293C0.inc"
};
s32 gPlayerLookFiles_5_175[2] = {
#include "sub_main/004293C8.inc"
};
s32 gPlayerLookFiles_5_177[2] = {
#include "sub_main/004293D0.inc"
};
s32 gPlayerLookFiles_5_179[2] = {
#include "sub_main/004293D8.inc"
};
s32 gPlayerLookFiles_5_181[2] = {
#include "sub_main/004293E0.inc"
};
s32 gPlayerLookFiles_5_183[2] = {
#include "sub_main/004293E8.inc"
};
s32 gPlayerLookFiles_5_185[2] = {
#include "sub_main/004293F0.inc"
};
s32 gPlayerLookFiles_5_187[2] = {
#include "sub_main/004293F8.inc"
};
s32 gPlayerLookFiles_5_189[2] = {
#include "sub_main/00429400.inc"
};
s32 gPlayerLookFiles_5_191[2] = {
#include "sub_main/00429408.inc"
};
s32 gPlayerLookFiles_5_193[2] = {
#include "sub_main/00429410.inc"
};
s32 gPlayerLookFiles_5_195[2] = {
#include "sub_main/00429418.inc"
};
s32 gPlayerLookFiles_5_197[2] = {
#include "sub_main/00429420.inc"
};
s32 gPlayerLookFiles_5_199[2] = {
#include "sub_main/00429428.inc"
};
s32 gPlayerLookFiles_5_201[2] = {
#include "sub_main/00429430.inc"
};
s32 gPlayerLookFiles_5_203[2] = {
#include "sub_main/00429438.inc"
};
s32 gPlayerLookFiles_5_205[2] = {
#include "sub_main/00429440.inc"
};
s32 gPlayerLookFiles_5_207[2] = {
#include "sub_main/00429448.inc"
};
s32 gPlayerLookFiles_5_209[2] = {
#include "sub_main/00429450.inc"
};
s32 gPlayerLookFiles_5_211[2] = {
#include "sub_main/00429458.inc"
};
s32 gPlayerLookFiles_5_213[2] = {
#include "sub_main/00429460.inc"
};
s32 gPlayerLookFiles_5_215[2] = {
#include "sub_main/00429468.inc"
};
s32 gPlayerLookFiles_5_217[2] = {
#include "sub_main/00429470.inc"
};
s32 gPlayerLookFiles_5_219[2] = {
#include "sub_main/00429478.inc"
};
s32 gPlayerLookFiles_5_221[2] = {
#include "sub_main/00429480.inc"
};
s32 gPlayerLookFiles_5_223[2] = {
#include "sub_main/00429488.inc"
};
s32 gPlayerLookFiles_5_225[2] = {
#include "sub_main/00429490.inc"
};
s32 gPlayerLookFiles_5_227[2] = {
#include "sub_main/00429498.inc"
};
s32 gPlayerLookFiles_5_229[2] = {
#include "sub_main/004294A0.inc"
};
s32 gPlayerLookFiles_5_231[2] = {
#include "sub_main/004294A8.inc"
};
s32 gPlayerLookFiles_5_233[2] = {
#include "sub_main/004294B0.inc"
};
s32 gPlayerLookFiles_5_235[2] = {
#include "sub_main/004294B8.inc"
};
s32 gPlayerLookFiles_5_237[2] = {
#include "sub_main/004294C0.inc"
};
s32 gPlayerLookFiles_5_239[2] = {
#include "sub_main/004294C8.inc"
};
s32 gPlayerLookFiles_5_241[2] = {
#include "sub_main/004294D0.inc"
};
s32 gPlayerLookFiles_5_243[2] = {
#include "sub_main/004294D8.inc"
};
s32 gPlayerLookFiles_5_245[2] = {
#include "sub_main/004294E0.inc"
};
s32 gPlayerLookFiles_5_247[2] = {
#include "sub_main/004294E8.inc"
};
s32 gPlayerLookFiles_5_249[2] = {
#include "sub_main/004294F0.inc"
};
s32 gPlayerLookFiles_5_251[2] = {
#include "sub_main/004294F8.inc"
};
s32 gPlayerLookFiles_5_253[2] = {
#include "sub_main/00429500.inc"
};
s32 gPlayerLookFiles_5_255[2] = {
#include "sub_main/00429508.inc"
};
s32 gPlayerLookFiles_5_257[2] = {
#include "sub_main/00429510.inc"
};
s32 gPlayerLookFiles_5_259[2] = {
#include "sub_main/00429518.inc"
};
s32 gPlayerLookFiles_5_261[2] = {
#include "sub_main/00429520.inc"
};
s32 gPlayerLookFiles_5_263[2] = {
#include "sub_main/00429528.inc"
};
s32 gPlayerLookFiles_5_265[2] = {
#include "sub_main/00429530.inc"
};
s32 gPlayerLookFiles_5_267[2] = {
#include "sub_main/00429538.inc"
};
s32 gPlayerLookFiles_5_269[2] = {
#include "sub_main/00429540.inc"
};
s32 gPlayerLookFiles_5_271[2] = {
#include "sub_main/00429548.inc"
};
s32 gPlayerLookFiles_5_273[2] = {
#include "sub_main/00429550.inc"
};
s32 gPlayerLookFiles_5_275[2] = {
#include "sub_main/00429558.inc"
};
s32 gPlayerLookFiles_5_277[2] = {
#include "sub_main/00429560.inc"
};
s32 gPlayerLookFiles_5_279[2] = {
#include "sub_main/00429568.inc"
};
s32 gPlayerLookFiles_5_281[2] = {
#include "sub_main/00429570.inc"
};
s32 gPlayerLookFiles_5_283[2] = {
#include "sub_main/00429578.inc"
};
s32 gPlayerLookFiles_5_285[2] = {
#include "sub_main/00429580.inc"
};
s32 gPlayerLookFiles_5_287[2] = {
#include "sub_main/00429588.inc"
};
s32 gPlayerLookFiles_5_289[2] = {
#include "sub_main/00429590.inc"
};
s32 gPlayerLookFiles_5_291[2] = {
#include "sub_main/00429598.inc"
};
s32 gPlayerLookFiles_5_293[2] = {
#include "sub_main/004295A0.inc"
};
s32 gPlayerLookFiles_5_295[2] = {
#include "sub_main/004295A8.inc"
};
s32 gPlayerLookFiles_5_297[2] = {
#include "sub_main/004295B0.inc"
};
s32 gPlayerLookFiles_5_299[2] = {
#include "sub_main/004295B8.inc"
};
s32 gPlayerLookFiles_5_301[2] = {
#include "sub_main/004295C0.inc"
};
s32 gPlayerLookFiles_5_303[2] = {
#include "sub_main/004295C8.inc"
};
s32 gPlayerLookFiles_5_305[2] = {
#include "sub_main/004295D0.inc"
};
s32 gPlayerLookFiles_5_307[2] = {
#include "sub_main/004295D8.inc"
};
s32 gPlayerLookFiles_5_309[2] = {
#include "sub_main/004295E0.inc"
};
s32 gPlayerLookFiles_5_311[2] = {
#include "sub_main/004295E8.inc"
};
s32 gPlayerLookFiles_5_313[2] = {
#include "sub_main/004295F0.inc"
};
s32 gPlayerLookFiles_5_315[2] = {
#include "sub_main/004295F8.inc"
};
s32 gPlayerLookFiles_5_317[2] = {
#include "sub_main/00429600.inc"
};
s32 gPlayerLookFiles_5_319[2] = {
#include "sub_main/00429608.inc"
};
s32 gPlayerLookFiles_5_321[2] = {
#include "sub_main/00429610.inc"
};
s32 gPlayerLookFiles_5_323[2] = {
#include "sub_main/00429618.inc"
};
s32 gPlayerLookFiles_5_325[2] = {
#include "sub_main/00429620.inc"
};
s32 gPlayerLookFiles_5_327[2] = {
#include "sub_main/00429628.inc"
};
s32 gPlayerLookFiles_5_329[2] = {
#include "sub_main/00429630.inc"
};
s32 gPlayerLookFiles_5_331[2] = {
#include "sub_main/00429638.inc"
};
s32 gPlayerLookFiles_5_333[2] = {
#include "sub_main/00429640.inc"
};
s32 gPlayerLookFiles_5_335[2] = {
#include "sub_main/00429648.inc"
};
s32 gPlayerLookFiles_5_337[2] = {
#include "sub_main/00429650.inc"
};
s32 gPlayerLookFiles_5_339[2] = {
#include "sub_main/00429658.inc"
};
s32 gPlayerLookFiles_5_341[2] = {
#include "sub_main/00429660.inc"
};
s32 gPlayerLookFiles_5_343[2] = {
#include "sub_main/00429668.inc"
};
s32 gPlayerLookFiles_5_345[2] = {
#include "sub_main/00429670.inc"
};
s32 gPlayerLookFiles_5_347[2] = {
#include "sub_main/00429678.inc"
};
s32 gPlayerLookFiles_5_349[2] = {
#include "sub_main/00429680.inc"
};
s32 gPlayerLookFiles_5_351[2] = {
#include "sub_main/00429688.inc"
};
s32 gPlayerLookFiles_5_353[2] = {
#include "sub_main/00429690.inc"
};
s32 gPlayerLookFiles_5_355[2] = {
#include "sub_main/00429698.inc"
};
s32 gPlayerLookFiles_5_357[2] = {
#include "sub_main/004296A0.inc"
};
s32 gPlayerLookFiles_5_359[2] = {
#include "sub_main/004296A8.inc"
};
s32 gPlayerLookFiles_5_361[2] = {
#include "sub_main/004296B0.inc"
};
s32 gPlayerLookFiles_5_363[2] = {
#include "sub_main/004296B8.inc"
};
s32 gPlayerLookFiles_5_365[2] = {
#include "sub_main/004296C0.inc"
};
s32 gPlayerLookFiles_5_367[2] = {
#include "sub_main/004296C8.inc"
};
s32 gPlayerLookFiles_5_369[2] = {
#include "sub_main/004296D0.inc"
};
s32 gPlayerLookFiles_5_371[2] = {
#include "sub_main/004296D8.inc"
};
s32 gPlayerLookFiles_5_373[2] = {
#include "sub_main/004296E0.inc"
};
s32 gPlayerLookFiles_5_375[2] = {
#include "sub_main/004296E8.inc"
};
s32 gPlayerLookFiles_5_377[2] = {
#include "sub_main/004296F0.inc"
};
s32 gPlayerLookFiles_5_379[2] = {
#include "sub_main/004296F8.inc"
};
s32 gPlayerLookFiles_5_381[2] = {
#include "sub_main/00429700.inc"
};
s32 gPlayerLookFiles_5_383[2] = {
#include "sub_main/00429708.inc"
};
s32 gPlayerLookFiles_5_385[2] = {
#include "sub_main/00429710.inc"
};
s32 gPlayerLookFiles_5_387[2] = {
#include "sub_main/00429718.inc"
};
s32 gPlayerLookFiles_5_389[2] = {
#include "sub_main/00429720.inc"
};
s32 gPlayerLookFiles_5_391[2] = {
#include "sub_main/00429728.inc"
};
s32 gPlayerLookFiles_5_393[2] = {
#include "sub_main/00429730.inc"
};
s32 gPlayerLookFiles_5_395[2] = {
#include "sub_main/00429738.inc"
};
s32 gPlayerLookFiles_5_397[2] = {
#include "sub_main/00429740.inc"
};
s32 gPlayerLookFiles_5_399[2] = {
#include "sub_main/00429748.inc"
};
s32 gPlayerLookFiles_5_401[2] = {
#include "sub_main/00429750.inc"
};
s32 gPlayerLookFiles_5_403[2] = {
#include "sub_main/00429758.inc"
};
s32 gPlayerLookFiles_5_405[2] = {
#include "sub_main/00429760.inc"
};
s32 gPlayerLookFiles_5_407[2] = {
#include "sub_main/00429768.inc"
};
s32 gPlayerLookFiles_5_409[2] = {
#include "sub_main/00429770.inc"
};
s32 gPlayerLookFiles_5_411[2] = {
#include "sub_main/00429778.inc"
};
s32 gPlayerLookFiles_5_413[4] = {
#include "sub_main/00429780.inc"
};
TexPackHead gPlayerLookFiles_5[207] = {
#include "sub_main/00429790.inc"
};
s32 gPlayerLookFiles_6_1[2] = {
#include "sub_main/00429E08.inc"
};
s32 gPlayerLookFiles_6_3[2] = {
#include "sub_main/00429E10.inc"
};
s32 gPlayerLookFiles_6_5[2] = {
#include "sub_main/00429E18.inc"
};
s32 gPlayerLookFiles_6_7[2] = {
#include "sub_main/00429E20.inc"
};
s32 gPlayerLookFiles_6_9[2] = {
#include "sub_main/00429E28.inc"
};
s32 gPlayerLookFiles_6_11[2] = {
#include "sub_main/00429E30.inc"
};
s32 gPlayerLookFiles_6_13[2] = {
#include "sub_main/00429E38.inc"
};
s32 gPlayerLookFiles_6_15[2] = {
#include "sub_main/00429E40.inc"
};
s32 gPlayerLookFiles_6_17[2] = {
#include "sub_main/00429E48.inc"
};
s32 gPlayerLookFiles_6_19[2] = {
#include "sub_main/00429E50.inc"
};
s32 gPlayerLookFiles_6_21[2] = {
#include "sub_main/00429E58.inc"
};
s32 gPlayerLookFiles_6_23[2] = {
#include "sub_main/00429E60.inc"
};
s32 gPlayerLookFiles_6_25[2] = {
#include "sub_main/00429E68.inc"
};
s32 gPlayerLookFiles_6_27[2] = {
#include "sub_main/00429E70.inc"
};
s32 gPlayerLookFiles_6_29[2] = {
#include "sub_main/00429E78.inc"
};
s32 gPlayerLookFiles_6_31[2] = {
#include "sub_main/00429E80.inc"
};
s32 gPlayerLookFiles_6_33[2] = {
#include "sub_main/00429E88.inc"
};
s32 gPlayerLookFiles_6_35[2] = {
#include "sub_main/00429E90.inc"
};
s32 gPlayerLookFiles_6_37[2] = {
#include "sub_main/00429E98.inc"
};
s32 gPlayerLookFiles_6_39[2] = {
#include "sub_main/00429EA0.inc"
};
s32 gPlayerLookFiles_6_41[2] = {
#include "sub_main/00429EA8.inc"
};
TexPackHead gPlayerLookFiles_6[21] = {
#include "sub_main/00429EB0.inc"
};
s32 gPlayerLookFiles_7_1[2] = {
#include "sub_main/00429F58.inc"
};
s32 gPlayerLookFiles_7_3[2] = {
#include "sub_main/00429F60.inc"
};
s32 gPlayerLookFiles_7_5[2] = {
#include "sub_main/00429F68.inc"
};
s32 gPlayerLookFiles_7_7[2] = {
#include "sub_main/00429F70.inc"
};
s32 gPlayerLookFiles_7_9[2] = {
#include "sub_main/00429F78.inc"
};
s32 gPlayerLookFiles_7_11[2] = {
#include "sub_main/00429F80.inc"
};
s32 gPlayerLookFiles_7_13[2] = {
#include "sub_main/00429F88.inc"
};
s32 gPlayerLookFiles_7_15[2] = {
#include "sub_main/00429F90.inc"
};
s32 gPlayerLookFiles_7_17[2] = {
#include "sub_main/00429F98.inc"
};
s32 gPlayerLookFiles_7_19[2] = {
#include "sub_main/00429FA0.inc"
};
s32 gPlayerLookFiles_7_21[2] = {
#include "sub_main/00429FA8.inc"
};
s32 gPlayerLookFiles_7_23[2] = {
#include "sub_main/00429FB0.inc"
};
s32 gPlayerLookFiles_7_25[2] = {
#include "sub_main/00429FB8.inc"
};
s32 gPlayerLookFiles_7_27[2] = {
#include "sub_main/00429FC0.inc"
};
s32 gPlayerLookFiles_7_29[2] = {
#include "sub_main/00429FC8.inc"
};
s32 gPlayerLookFiles_7_31[2] = {
#include "sub_main/00429FD0.inc"
};
s32 gPlayerLookFiles_7_33[2] = {
#include "sub_main/00429FD8.inc"
};
s32 gPlayerLookFiles_7_35[2] = {
#include "sub_main/00429FE0.inc"
};
s32 gPlayerLookFiles_7_37[2] = {
#include "sub_main/00429FE8.inc"
};
s32 gPlayerLookFiles_7_39[2] = {
#include "sub_main/00429FF0.inc"
};
s32 gPlayerLookFiles_7_41[2] = {
#include "sub_main/00429FF8.inc"
};
s32 gPlayerLookFiles_7_43[2] = {
#include "sub_main/0042A000.inc"
};
s32 gPlayerLookFiles_7_45[2] = {
#include "sub_main/0042A008.inc"
};
s32 gPlayerLookFiles_7_47[2] = {
#include "sub_main/0042A010.inc"
};
s32 gPlayerLookFiles_7_49[2] = {
#include "sub_main/0042A018.inc"
};
s32 gPlayerLookFiles_7_51[2] = {
#include "sub_main/0042A020.inc"
};
s32 gPlayerLookFiles_7_53[2] = {
#include "sub_main/0042A028.inc"
};
s32 gPlayerLookFiles_7_55[2] = {
#include "sub_main/0042A030.inc"
};
s32 gPlayerLookFiles_7_57[2] = {
#include "sub_main/0042A038.inc"
};
s32 gPlayerLookFiles_7_59[2] = {
#include "sub_main/0042A040.inc"
};
s32 gPlayerLookFiles_7_61[2] = {
#include "sub_main/0042A048.inc"
};
s32 gPlayerLookFiles_7_63[2] = {
#include "sub_main/0042A050.inc"
};
s32 gPlayerLookFiles_7_65[2] = {
#include "sub_main/0042A058.inc"
};
s32 gPlayerLookFiles_7_67[2] = {
#include "sub_main/0042A060.inc"
};
s32 gPlayerLookFiles_7_69[2] = {
#include "sub_main/0042A068.inc"
};
s32 gPlayerLookFiles_7_71[2] = {
#include "sub_main/0042A070.inc"
};
s32 gPlayerLookFiles_7_73[2] = {
#include "sub_main/0042A078.inc"
};
s32 gPlayerLookFiles_7_75[2] = {
#include "sub_main/0042A080.inc"
};
s32 gPlayerLookFiles_7_77[2] = {
#include "sub_main/0042A088.inc"
};
s32 gPlayerLookFiles_7_79[2] = {
#include "sub_main/0042A090.inc"
};
s32 gPlayerLookFiles_7_81[2] = {
#include "sub_main/0042A098.inc"
};
s32 gPlayerLookFiles_7_83[2] = {
#include "sub_main/0042A0A0.inc"
};
s32 gPlayerLookFiles_7_85[2] = {
#include "sub_main/0042A0A8.inc"
};
s32 gPlayerLookFiles_7_87[2] = {
#include "sub_main/0042A0B0.inc"
};
s32 gPlayerLookFiles_7_89[2] = {
#include "sub_main/0042A0B8.inc"
};
s32 gPlayerLookFiles_7_91[2] = {
#include "sub_main/0042A0C0.inc"
};
s32 gPlayerLookFiles_7_93[2] = {
#include "sub_main/0042A0C8.inc"
};
s32 gPlayerLookFiles_7_95[2] = {
#include "sub_main/0042A0D0.inc"
};
s32 gPlayerLookFiles_7_97[2] = {
#include "sub_main/0042A0D8.inc"
};
s32 gPlayerLookFiles_7_99[2] = {
#include "sub_main/0042A0E0.inc"
};
s32 gPlayerLookFiles_7_101[2] = {
#include "sub_main/0042A0E8.inc"
};
s32 gPlayerLookFiles_7_103[2] = {
#include "sub_main/0042A0F0.inc"
};
s32 gPlayerLookFiles_7_105[2] = {
#include "sub_main/0042A0F8.inc"
};
s32 gPlayerLookFiles_7_107[2] = {
#include "sub_main/0042A100.inc"
};
s32 gPlayerLookFiles_7_109[2] = {
#include "sub_main/0042A108.inc"
};
s32 gPlayerLookFiles_7_111[2] = {
#include "sub_main/0042A110.inc"
};
s32 gPlayerLookFiles_7_113[2] = {
#include "sub_main/0042A118.inc"
};
s32 gPlayerLookFiles_7_115[2] = {
#include "sub_main/0042A120.inc"
};
s32 gPlayerLookFiles_7_117[2] = {
#include "sub_main/0042A128.inc"
};
s32 gPlayerLookFiles_7_119[2] = {
#include "sub_main/0042A130.inc"
};
s32 gPlayerLookFiles_7_121[2] = {
#include "sub_main/0042A138.inc"
};
s32 gPlayerLookFiles_7_123[2] = {
#include "sub_main/0042A140.inc"
};
s32 gPlayerLookFiles_7_125[2] = {
#include "sub_main/0042A148.inc"
};
s32 gPlayerLookFiles_7_127[2] = {
#include "sub_main/0042A150.inc"
};
s32 gPlayerLookFiles_7_129[2] = {
#include "sub_main/0042A158.inc"
};
s32 gPlayerLookFiles_7_131[2] = {
#include "sub_main/0042A160.inc"
};
s32 gPlayerLookFiles_7_133[2] = {
#include "sub_main/0042A168.inc"
};
s32 gPlayerLookFiles_7_135[2] = {
#include "sub_main/0042A170.inc"
};
s32 gPlayerLookFiles_7_137[2] = {
#include "sub_main/0042A178.inc"
};
s32 gPlayerLookFiles_7_139[2] = {
#include "sub_main/0042A180.inc"
};
s32 gPlayerLookFiles_7_141[2] = {
#include "sub_main/0042A188.inc"
};
s32 gPlayerLookFiles_7_143[2] = {
#include "sub_main/0042A190.inc"
};
s32 gPlayerLookFiles_7_145[2] = {
#include "sub_main/0042A198.inc"
};
s32 gPlayerLookFiles_7_147[2] = {
#include "sub_main/0042A1A0.inc"
};
s32 gPlayerLookFiles_7_149[2] = {
#include "sub_main/0042A1A8.inc"
};
s32 gPlayerLookFiles_7_151[2] = {
#include "sub_main/0042A1B0.inc"
};
s32 gPlayerLookFiles_7_153[2] = {
#include "sub_main/0042A1B8.inc"
};
s32 gPlayerLookFiles_7_155[2] = {
#include "sub_main/0042A1C0.inc"
};
s32 gPlayerLookFiles_7_157[2] = {
#include "sub_main/0042A1C8.inc"
};
s32 gPlayerLookFiles_7_159[2] = {
#include "sub_main/0042A1D0.inc"
};
s32 gPlayerLookFiles_7_161[2] = {
#include "sub_main/0042A1D8.inc"
};
s32 gPlayerLookFiles_7_163[2] = {
#include "sub_main/0042A1E0.inc"
};
s32 gPlayerLookFiles_7_165[2] = {
#include "sub_main/0042A1E8.inc"
};
s32 gPlayerLookFiles_7_167[2] = {
#include "sub_main/0042A1F0.inc"
};
s32 gPlayerLookFiles_7_169[2] = {
#include "sub_main/0042A1F8.inc"
};
s32 gPlayerLookFiles_7_171[2] = {
#include "sub_main/0042A200.inc"
};
s32 gPlayerLookFiles_7_173[2] = {
#include "sub_main/0042A208.inc"
};
s32 gPlayerLookFiles_7_175[2] = {
#include "sub_main/0042A210.inc"
};
s32 gPlayerLookFiles_7_177[2] = {
#include "sub_main/0042A218.inc"
};
s32 gPlayerLookFiles_7_179[2] = {
#include "sub_main/0042A220.inc"
};
s32 gPlayerLookFiles_7_181[2] = {
#include "sub_main/0042A228.inc"
};
s32 gPlayerLookFiles_7_183[2] = {
#include "sub_main/0042A230.inc"
};
s32 gPlayerLookFiles_7_185[2] = {
#include "sub_main/0042A238.inc"
};
s32 gPlayerLookFiles_7_187[2] = {
#include "sub_main/0042A240.inc"
};
s32 gPlayerLookFiles_7_189[2] = {
#include "sub_main/0042A248.inc"
};
s32 gPlayerLookFiles_7_191[2] = {
#include "sub_main/0042A250.inc"
};
s32 gPlayerLookFiles_7_193[2] = {
#include "sub_main/0042A258.inc"
};
s32 gPlayerLookFiles_7_195[2] = {
#include "sub_main/0042A260.inc"
};
s32 gPlayerLookFiles_7_197[2] = {
#include "sub_main/0042A268.inc"
};
s32 gPlayerLookFiles_7_199[2] = {
#include "sub_main/0042A270.inc"
};
s32 gPlayerLookFiles_7_201[2] = {
#include "sub_main/0042A278.inc"
};
s32 gPlayerLookFiles_7_203[2] = {
#include "sub_main/0042A280.inc"
};
s32 gPlayerLookFiles_7_205[2] = {
#include "sub_main/0042A288.inc"
};
s32 gPlayerLookFiles_7_207[2] = {
#include "sub_main/0042A290.inc"
};
s32 gPlayerLookFiles_7_209[2] = {
#include "sub_main/0042A298.inc"
};
s32 gPlayerLookFiles_7_211[2] = {
#include "sub_main/0042A2A0.inc"
};
s32 gPlayerLookFiles_7_213[2] = {
#include "sub_main/0042A2A8.inc"
};
s32 gPlayerLookFiles_7_215[2] = {
#include "sub_main/0042A2B0.inc"
};
s32 gPlayerLookFiles_7_217[2] = {
#include "sub_main/0042A2B8.inc"
};
s32 gPlayerLookFiles_7_219[2] = {
#include "sub_main/0042A2C0.inc"
};
s32 gPlayerLookFiles_7_221[2] = {
#include "sub_main/0042A2C8.inc"
};
s32 gPlayerLookFiles_7_223[2] = {
#include "sub_main/0042A2D0.inc"
};
s32 gPlayerLookFiles_7_225[2] = {
#include "sub_main/0042A2D8.inc"
};
s32 gPlayerLookFiles_7_227[2] = {
#include "sub_main/0042A2E0.inc"
};
s32 gPlayerLookFiles_7_229[2] = {
#include "sub_main/0042A2E8.inc"
};
s32 gPlayerLookFiles_7_231[2] = {
#include "sub_main/0042A2F0.inc"
};
s32 gPlayerLookFiles_7_233[2] = {
#include "sub_main/0042A2F8.inc"
};
s32 gPlayerLookFiles_7_235[2] = {
#include "sub_main/0042A300.inc"
};
s32 gPlayerLookFiles_7_237[2] = {
#include "sub_main/0042A308.inc"
};
s32 gPlayerLookFiles_7_239[2] = {
#include "sub_main/0042A310.inc"
};
s32 gPlayerLookFiles_7_241[2] = {
#include "sub_main/0042A318.inc"
};
s32 gPlayerLookFiles_7_243[2] = {
#include "sub_main/0042A320.inc"
};
s32 gPlayerLookFiles_7_245[2] = {
#include "sub_main/0042A328.inc"
};
s32 gPlayerLookFiles_7_247[2] = {
#include "sub_main/0042A330.inc"
};
s32 gPlayerLookFiles_7_249[2] = {
#include "sub_main/0042A338.inc"
};
s32 gPlayerLookFiles_7_251[2] = {
#include "sub_main/0042A340.inc"
};
s32 gPlayerLookFiles_7_253[2] = {
#include "sub_main/0042A348.inc"
};
s32 gPlayerLookFiles_7_255[2] = {
#include "sub_main/0042A350.inc"
};
s32 gPlayerLookFiles_7_257[2] = {
#include "sub_main/0042A358.inc"
};
s32 gPlayerLookFiles_7_259[2] = {
#include "sub_main/0042A360.inc"
};
s32 gPlayerLookFiles_7_261[2] = {
#include "sub_main/0042A368.inc"
};
s32 gPlayerLookFiles_7_263[2] = {
#include "sub_main/0042A370.inc"
};
s32 gPlayerLookFiles_7_265[2] = {
#include "sub_main/0042A378.inc"
};
s32 gPlayerLookFiles_7_267[2] = {
#include "sub_main/0042A380.inc"
};
s32 gPlayerLookFiles_7_269[2] = {
#include "sub_main/0042A388.inc"
};
s32 gPlayerLookFiles_7_271[2] = {
#include "sub_main/0042A390.inc"
};
s32 gPlayerLookFiles_7_273[2] = {
#include "sub_main/0042A398.inc"
};
s32 gPlayerLookFiles_7_275[2] = {
#include "sub_main/0042A3A0.inc"
};
s32 gPlayerLookFiles_7_277[2] = {
#include "sub_main/0042A3A8.inc"
};
s32 gPlayerLookFiles_7_279[2] = {
#include "sub_main/0042A3B0.inc"
};
s32 gPlayerLookFiles_7_281[2] = {
#include "sub_main/0042A3B8.inc"
};
s32 gPlayerLookFiles_7_283[2] = {
#include "sub_main/0042A3C0.inc"
};
s32 gPlayerLookFiles_7_285[2] = {
#include "sub_main/0042A3C8.inc"
};
s32 gPlayerLookFiles_7_287[2] = {
#include "sub_main/0042A3D0.inc"
};
s32 gPlayerLookFiles_7_289[2] = {
#include "sub_main/0042A3D8.inc"
};
s32 gPlayerLookFiles_7_291[2] = {
#include "sub_main/0042A3E0.inc"
};
s32 gPlayerLookFiles_7_293[2] = {
#include "sub_main/0042A3E8.inc"
};
s32 gPlayerLookFiles_7_295[2] = {
#include "sub_main/0042A3F0.inc"
};
s32 gPlayerLookFiles_7_297[2] = {
#include "sub_main/0042A3F8.inc"
};
s32 gPlayerLookFiles_7_299[2] = {
#include "sub_main/0042A400.inc"
};
s32 gPlayerLookFiles_7_301[2] = {
#include "sub_main/0042A408.inc"
};
s32 gPlayerLookFiles_7_303[2] = {
#include "sub_main/0042A410.inc"
};
s32 gPlayerLookFiles_7_305[2] = {
#include "sub_main/0042A418.inc"
};
s32 gPlayerLookFiles_7_307[2] = {
#include "sub_main/0042A420.inc"
};
s32 gPlayerLookFiles_7_309[2] = {
#include "sub_main/0042A428.inc"
};
s32 gPlayerLookFiles_7_311[2] = {
#include "sub_main/0042A430.inc"
};
s32 gPlayerLookFiles_7_313[2] = {
#include "sub_main/0042A438.inc"
};
s32 gPlayerLookFiles_7_315[2] = {
#include "sub_main/0042A440.inc"
};
s32 gPlayerLookFiles_7_317[2] = {
#include "sub_main/0042A448.inc"
};
s32 gPlayerLookFiles_7_319[2] = {
#include "sub_main/0042A450.inc"
};
s32 gPlayerLookFiles_7_321[2] = {
#include "sub_main/0042A458.inc"
};
s32 gPlayerLookFiles_7_323[2] = {
#include "sub_main/0042A460.inc"
};
s32 gPlayerLookFiles_7_325[2] = {
#include "sub_main/0042A468.inc"
};
s32 gPlayerLookFiles_7_327[2] = {
#include "sub_main/0042A470.inc"
};
s32 gPlayerLookFiles_7_329[2] = {
#include "sub_main/0042A478.inc"
};
s32 gPlayerLookFiles_7_331[2] = {
#include "sub_main/0042A480.inc"
};
s32 gPlayerLookFiles_7_333[2] = {
#include "sub_main/0042A488.inc"
};
s32 gPlayerLookFiles_7_335[2] = {
#include "sub_main/0042A490.inc"
};
s32 gPlayerLookFiles_7_337[2] = {
#include "sub_main/0042A498.inc"
};
s32 gPlayerLookFiles_7_339[2] = {
#include "sub_main/0042A4A0.inc"
};
s32 gPlayerLookFiles_7_341[2] = {
#include "sub_main/0042A4A8.inc"
};
s32 gPlayerLookFiles_7_343[2] = {
#include "sub_main/0042A4B0.inc"
};
s32 gPlayerLookFiles_7_345[2] = {
#include "sub_main/0042A4B8.inc"
};
s32 gPlayerLookFiles_7_347[2] = {
#include "sub_main/0042A4C0.inc"
};
s32 gPlayerLookFiles_7_349[2] = {
#include "sub_main/0042A4C8.inc"
};
s32 gPlayerLookFiles_7_351[2] = {
#include "sub_main/0042A4D0.inc"
};
s32 gPlayerLookFiles_7_353[2] = {
#include "sub_main/0042A4D8.inc"
};
s32 gPlayerLookFiles_7_355[2] = {
#include "sub_main/0042A4E0.inc"
};
s32 gPlayerLookFiles_7_357[2] = {
#include "sub_main/0042A4E8.inc"
};
s32 gPlayerLookFiles_7_359[2] = {
#include "sub_main/0042A4F0.inc"
};
s32 gPlayerLookFiles_7_361[2] = {
#include "sub_main/0042A4F8.inc"
};
s32 gPlayerLookFiles_7_363[2] = {
#include "sub_main/0042A500.inc"
};
s32 gPlayerLookFiles_7_365[2] = {
#include "sub_main/0042A508.inc"
};
s32 gPlayerLookFiles_7_367[2] = {
#include "sub_main/0042A510.inc"
};
s32 gPlayerLookFiles_7_369[2] = {
#include "sub_main/0042A518.inc"
};
s32 gPlayerLookFiles_7_371[2] = {
#include "sub_main/0042A520.inc"
};
s32 gPlayerLookFiles_7_373[2] = {
#include "sub_main/0042A528.inc"
};
s32 gPlayerLookFiles_7_375[2] = {
#include "sub_main/0042A530.inc"
};
s32 gPlayerLookFiles_7_377[2] = {
#include "sub_main/0042A538.inc"
};
s32 gPlayerLookFiles_7_379[2] = {
#include "sub_main/0042A540.inc"
};
s32 gPlayerLookFiles_7_381[2] = {
#include "sub_main/0042A548.inc"
};
s32 gPlayerLookFiles_7_383[2] = {
#include "sub_main/0042A550.inc"
};
s32 gPlayerLookFiles_7_385[2] = {
#include "sub_main/0042A558.inc"
};
s32 gPlayerLookFiles_7_387[2] = {
#include "sub_main/0042A560.inc"
};
s32 gPlayerLookFiles_7_389[2] = {
#include "sub_main/0042A568.inc"
};
s32 gPlayerLookFiles_7_391[2] = {
#include "sub_main/0042A570.inc"
};
s32 gPlayerLookFiles_7_393[2] = {
#include "sub_main/0042A578.inc"
};
s32 gPlayerLookFiles_7_395[2] = {
#include "sub_main/0042A580.inc"
};
s32 gPlayerLookFiles_7_397[2] = {
#include "sub_main/0042A588.inc"
};
s32 gPlayerLookFiles_7_399[4] = {
#include "sub_main/0042A590.inc"
};
TexPackHead gPlayerLookFiles_7[200] = {
#include "sub_main/0042A5A0.inc"
};
s32 gPlayerLookFiles_8_1[2] = {
#include "sub_main/0042ABE0.inc"
};
s32 gPlayerLookFiles_8_3[2] = {
#include "sub_main/0042ABE8.inc"
};
s32 gPlayerLookFiles_8_5[2] = {
#include "sub_main/0042ABF0.inc"
};
s32 gPlayerLookFiles_8_7[2] = {
#include "sub_main/0042ABF8.inc"
};
s32 gPlayerLookFiles_8_9[2] = {
#include "sub_main/0042AC00.inc"
};
s32 gPlayerLookFiles_8_11[2] = {
#include "sub_main/0042AC08.inc"
};
s32 gPlayerLookFiles_8_13[2] = {
#include "sub_main/0042AC10.inc"
};
s32 gPlayerLookFiles_8_15[2] = {
#include "sub_main/0042AC18.inc"
};
s32 gPlayerLookFiles_8_17[2] = {
#include "sub_main/0042AC20.inc"
};
s32 gPlayerLookFiles_8_19[2] = {
#include "sub_main/0042AC28.inc"
};
s32 gPlayerLookFiles_8_21[2] = {
#include "sub_main/0042AC30.inc"
};
s32 gPlayerLookFiles_8_23[2] = {
#include "sub_main/0042AC38.inc"
};
s32 gPlayerLookFiles_8_25[2] = {
#include "sub_main/0042AC40.inc"
};
s32 gPlayerLookFiles_8_27[2] = {
#include "sub_main/0042AC48.inc"
};
s32 gPlayerLookFiles_8_29[2] = {
#include "sub_main/0042AC50.inc"
};
s32 gPlayerLookFiles_8_31[2] = {
#include "sub_main/0042AC58.inc"
};
s32 gPlayerLookFiles_8_33[2] = {
#include "sub_main/0042AC60.inc"
};
s32 gPlayerLookFiles_8_35[2] = {
#include "sub_main/0042AC68.inc"
};
s32 gPlayerLookFiles_8_37[2] = {
#include "sub_main/0042AC70.inc"
};
s32 gPlayerLookFiles_8_39[2] = {
#include "sub_main/0042AC78.inc"
};
s32 gPlayerLookFiles_8_41[2] = {
#include "sub_main/0042AC80.inc"
};
s32 gPlayerLookFiles_8_43[2] = {
#include "sub_main/0042AC88.inc"
};
s32 gPlayerLookFiles_8_45[2] = {
#include "sub_main/0042AC90.inc"
};
s32 gPlayerLookFiles_8_47[2] = {
#include "sub_main/0042AC98.inc"
};
s32 gPlayerLookFiles_8_49[2] = {
#include "sub_main/0042ACA0.inc"
};
s32 gPlayerLookFiles_8_51[2] = {
#include "sub_main/0042ACA8.inc"
};
s32 gPlayerLookFiles_8_53[4] = {
#include "sub_main/0042ACB0.inc"
};
TexPackHead gPlayerLookFiles_8[27] = {
#include "sub_main/0042ACC0.inc"
};
s32 gPlayerLookFiles_9_1[2] = {
#include "sub_main/0042AD98.inc"
};
s32 gPlayerLookFiles_9_3[2] = {
#include "sub_main/0042ADA0.inc"
};
s32 gPlayerLookFiles_9_5[2] = {
#include "sub_main/0042ADA8.inc"
};
s32 gPlayerLookFiles_9_7[2] = {
#include "sub_main/0042ADB0.inc"
};
s32 gPlayerLookFiles_9_9[2] = {
#include "sub_main/0042ADB8.inc"
};
s32 gPlayerLookFiles_9_11[2] = {
#include "sub_main/0042ADC0.inc"
};
s32 gPlayerLookFiles_9_13[2] = {
#include "sub_main/0042ADC8.inc"
};
s32 gPlayerLookFiles_9_15[2] = {
#include "sub_main/0042ADD0.inc"
};
s32 gPlayerLookFiles_9_17[2] = {
#include "sub_main/0042ADD8.inc"
};
s32 gPlayerLookFiles_9_19[2] = {
#include "sub_main/0042ADE0.inc"
};
s32 gPlayerLookFiles_9_21[2] = {
#include "sub_main/0042ADE8.inc"
};
s32 gPlayerLookFiles_9_23[2] = {
#include "sub_main/0042ADF0.inc"
};
s32 gPlayerLookFiles_9_25[2] = {
#include "sub_main/0042ADF8.inc"
};
s32 gPlayerLookFiles_9_27[2] = {
#include "sub_main/0042AE00.inc"
};
s32 gPlayerLookFiles_9_29[2] = {
#include "sub_main/0042AE08.inc"
};
s32 gPlayerLookFiles_9_31[2] = {
#include "sub_main/0042AE10.inc"
};
s32 gPlayerLookFiles_9_33[2] = {
#include "sub_main/0042AE18.inc"
};
s32 gPlayerLookFiles_9_35[2] = {
#include "sub_main/0042AE20.inc"
};
s32 gPlayerLookFiles_9_37[2] = {
#include "sub_main/0042AE28.inc"
};
s32 gPlayerLookFiles_9_39[2] = {
#include "sub_main/0042AE30.inc"
};
s32 gPlayerLookFiles_9_41[2] = {
#include "sub_main/0042AE38.inc"
};
s32 gPlayerLookFiles_9_43[2] = {
#include "sub_main/0042AE40.inc"
};
s32 gPlayerLookFiles_9_45[2] = {
#include "sub_main/0042AE48.inc"
};
s32 gPlayerLookFiles_9_47[2] = {
#include "sub_main/0042AE50.inc"
};
s32 gPlayerLookFiles_9_49[2] = {
#include "sub_main/0042AE58.inc"
};
s32 gPlayerLookFiles_9_51[2] = {
#include "sub_main/0042AE60.inc"
};
s32 gPlayerLookFiles_9_53[2] = {
#include "sub_main/0042AE68.inc"
};
s32 gPlayerLookFiles_9_55[2] = {
#include "sub_main/0042AE70.inc"
};
s32 gPlayerLookFiles_9_57[2] = {
#include "sub_main/0042AE78.inc"
};
s32 gPlayerLookFiles_9_59[2] = {
#include "sub_main/0042AE80.inc"
};
s32 gPlayerLookFiles_9_61[2] = {
#include "sub_main/0042AE88.inc"
};
s32 gPlayerLookFiles_9_63[2] = {
#include "sub_main/0042AE90.inc"
};
s32 gPlayerLookFiles_9_65[2] = {
#include "sub_main/0042AE98.inc"
};
s32 gPlayerLookFiles_9_67[2] = {
#include "sub_main/0042AEA0.inc"
};
s32 gPlayerLookFiles_9_69[2] = {
#include "sub_main/0042AEA8.inc"
};
s32 gPlayerLookFiles_9_71[2] = {
#include "sub_main/0042AEB0.inc"
};
s32 gPlayerLookFiles_9_73[2] = {
#include "sub_main/0042AEB8.inc"
};
s32 gPlayerLookFiles_9_75[2] = {
#include "sub_main/0042AEC0.inc"
};
s32 gPlayerLookFiles_9_77[2] = {
#include "sub_main/0042AEC8.inc"
};
s32 gPlayerLookFiles_9_79[2] = {
#include "sub_main/0042AED0.inc"
};
s32 gPlayerLookFiles_9_81[2] = {
#include "sub_main/0042AED8.inc"
};
s32 gPlayerLookFiles_9_83[2] = {
#include "sub_main/0042AEE0.inc"
};
s32 gPlayerLookFiles_9_85[2] = {
#include "sub_main/0042AEE8.inc"
};
s32 gPlayerLookFiles_9_87[2] = {
#include "sub_main/0042AEF0.inc"
};
s32 gPlayerLookFiles_9_89[2] = {
#include "sub_main/0042AEF8.inc"
};
s32 gPlayerLookFiles_9_91[2] = {
#include "sub_main/0042AF00.inc"
};
s32 gPlayerLookFiles_9_93[2] = {
#include "sub_main/0042AF08.inc"
};
s32 gPlayerLookFiles_9_95[2] = {
#include "sub_main/0042AF10.inc"
};
s32 gPlayerLookFiles_9_97[2] = {
#include "sub_main/0042AF18.inc"
};
s32 gPlayerLookFiles_9_99[2] = {
#include "sub_main/0042AF20.inc"
};
s32 gPlayerLookFiles_9_101[2] = {
#include "sub_main/0042AF28.inc"
};
s32 gPlayerLookFiles_9_103[2] = {
#include "sub_main/0042AF30.inc"
};
s32 gPlayerLookFiles_9_105[2] = {
#include "sub_main/0042AF38.inc"
};
s32 gPlayerLookFiles_9_107[2] = {
#include "sub_main/0042AF40.inc"
};
s32 gPlayerLookFiles_9_109[2] = {
#include "sub_main/0042AF48.inc"
};
s32 gPlayerLookFiles_9_111[2] = {
#include "sub_main/0042AF50.inc"
};
s32 gPlayerLookFiles_9_113[2] = {
#include "sub_main/0042AF58.inc"
};
s32 gPlayerLookFiles_9_115[2] = {
#include "sub_main/0042AF60.inc"
};
s32 gPlayerLookFiles_9_117[2] = {
#include "sub_main/0042AF68.inc"
};
s32 gPlayerLookFiles_9_119[2] = {
#include "sub_main/0042AF70.inc"
};
s32 gPlayerLookFiles_9_121[2] = {
#include "sub_main/0042AF78.inc"
};
s32 gPlayerLookFiles_9_123[2] = {
#include "sub_main/0042AF80.inc"
};
s32 gPlayerLookFiles_9_125[2] = {
#include "sub_main/0042AF88.inc"
};
s32 gPlayerLookFiles_9_127[2] = {
#include "sub_main/0042AF90.inc"
};
s32 gPlayerLookFiles_9_129[2] = {
#include "sub_main/0042AF98.inc"
};
s32 gPlayerLookFiles_9_131[2] = {
#include "sub_main/0042AFA0.inc"
};
s32 gPlayerLookFiles_9_133[2] = {
#include "sub_main/0042AFA8.inc"
};
s32 gPlayerLookFiles_9_135[2] = {
#include "sub_main/0042AFB0.inc"
};
s32 gPlayerLookFiles_9_137[2] = {
#include "sub_main/0042AFB8.inc"
};
s32 gPlayerLookFiles_9_139[2] = {
#include "sub_main/0042AFC0.inc"
};
s32 gPlayerLookFiles_9_141[2] = {
#include "sub_main/0042AFC8.inc"
};
s32 gPlayerLookFiles_9_143[2] = {
#include "sub_main/0042AFD0.inc"
};
s32 gPlayerLookFiles_9_145[2] = {
#include "sub_main/0042AFD8.inc"
};
s32 gPlayerLookFiles_9_147[2] = {
#include "sub_main/0042AFE0.inc"
};
s32 gPlayerLookFiles_9_149[2] = {
#include "sub_main/0042AFE8.inc"
};
s32 gPlayerLookFiles_9_151[2] = {
#include "sub_main/0042AFF0.inc"
};
s32 gPlayerLookFiles_9_153[2] = {
#include "sub_main/0042AFF8.inc"
};
s32 gPlayerLookFiles_9_155[2] = {
#include "sub_main/0042B000.inc"
};
s32 gPlayerLookFiles_9_157[2] = {
#include "sub_main/0042B008.inc"
};
s32 gPlayerLookFiles_9_159[2] = {
#include "sub_main/0042B010.inc"
};
s32 gPlayerLookFiles_9_161[2] = {
#include "sub_main/0042B018.inc"
};
s32 gPlayerLookFiles_9_163[2] = {
#include "sub_main/0042B020.inc"
};
s32 gPlayerLookFiles_9_165[2] = {
#include "sub_main/0042B028.inc"
};
s32 gPlayerLookFiles_9_167[2] = {
#include "sub_main/0042B030.inc"
};
s32 gPlayerLookFiles_9_169[2] = {
#include "sub_main/0042B038.inc"
};
s32 gPlayerLookFiles_9_171[2] = {
#include "sub_main/0042B040.inc"
};
s32 gPlayerLookFiles_9_173[2] = {
#include "sub_main/0042B048.inc"
};
s32 gPlayerLookFiles_9_175[2] = {
#include "sub_main/0042B050.inc"
};
s32 gPlayerLookFiles_9_177[2] = {
#include "sub_main/0042B058.inc"
};
s32 gPlayerLookFiles_9_179[2] = {
#include "sub_main/0042B060.inc"
};
s32 gPlayerLookFiles_9_181[2] = {
#include "sub_main/0042B068.inc"
};
s32 gPlayerLookFiles_9_183[2] = {
#include "sub_main/0042B070.inc"
};
s32 gPlayerLookFiles_9_185[2] = {
#include "sub_main/0042B078.inc"
};
s32 gPlayerLookFiles_9_187[2] = {
#include "sub_main/0042B080.inc"
};
s32 gPlayerLookFiles_9_189[2] = {
#include "sub_main/0042B088.inc"
};
s32 gPlayerLookFiles_9_191[2] = {
#include "sub_main/0042B090.inc"
};
s32 gPlayerLookFiles_9_193[2] = {
#include "sub_main/0042B098.inc"
};
s32 gPlayerLookFiles_9_195[2] = {
#include "sub_main/0042B0A0.inc"
};
s32 gPlayerLookFiles_9_197[2] = {
#include "sub_main/0042B0A8.inc"
};
s32 gPlayerLookFiles_9_199[2] = {
#include "sub_main/0042B0B0.inc"
};
s32 gPlayerLookFiles_9_201[2] = {
#include "sub_main/0042B0B8.inc"
};
s32 gPlayerLookFiles_9_203[2] = {
#include "sub_main/0042B0C0.inc"
};
s32 gPlayerLookFiles_9_205[2] = {
#include "sub_main/0042B0C8.inc"
};
s32 gPlayerLookFiles_9_207[2] = {
#include "sub_main/0042B0D0.inc"
};
s32 gPlayerLookFiles_9_209[2] = {
#include "sub_main/0042B0D8.inc"
};
s32 gPlayerLookFiles_9_211[2] = {
#include "sub_main/0042B0E0.inc"
};
s32 gPlayerLookFiles_9_213[2] = {
#include "sub_main/0042B0E8.inc"
};
s32 gPlayerLookFiles_9_215[2] = {
#include "sub_main/0042B0F0.inc"
};
s32 gPlayerLookFiles_9_217[2] = {
#include "sub_main/0042B0F8.inc"
};
s32 gPlayerLookFiles_9_219[2] = {
#include "sub_main/0042B100.inc"
};
s32 gPlayerLookFiles_9_221[2] = {
#include "sub_main/0042B108.inc"
};
s32 gPlayerLookFiles_9_223[2] = {
#include "sub_main/0042B110.inc"
};
s32 gPlayerLookFiles_9_225[2] = {
#include "sub_main/0042B118.inc"
};
s32 gPlayerLookFiles_9_227[2] = {
#include "sub_main/0042B120.inc"
};
s32 gPlayerLookFiles_9_229[2] = {
#include "sub_main/0042B128.inc"
};
s32 gPlayerLookFiles_9_231[2] = {
#include "sub_main/0042B130.inc"
};
s32 gPlayerLookFiles_9_233[2] = {
#include "sub_main/0042B138.inc"
};
s32 gPlayerLookFiles_9_235[2] = {
#include "sub_main/0042B140.inc"
};
s32 gPlayerLookFiles_9_237[2] = {
#include "sub_main/0042B148.inc"
};
s32 gPlayerLookFiles_9_239[2] = {
#include "sub_main/0042B150.inc"
};
s32 gPlayerLookFiles_9_241[2] = {
#include "sub_main/0042B158.inc"
};
s32 gPlayerLookFiles_9_243[2] = {
#include "sub_main/0042B160.inc"
};
s32 gPlayerLookFiles_9_245[2] = {
#include "sub_main/0042B168.inc"
};
s32 gPlayerLookFiles_9_247[2] = {
#include "sub_main/0042B170.inc"
};
s32 gPlayerLookFiles_9_249[2] = {
#include "sub_main/0042B178.inc"
};
s32 gPlayerLookFiles_9_251[2] = {
#include "sub_main/0042B180.inc"
};
s32 gPlayerLookFiles_9_253[2] = {
#include "sub_main/0042B188.inc"
};
s32 gPlayerLookFiles_9_255[2] = {
#include "sub_main/0042B190.inc"
};
s32 gPlayerLookFiles_9_257[2] = {
#include "sub_main/0042B198.inc"
};
s32 gPlayerLookFiles_9_259[2] = {
#include "sub_main/0042B1A0.inc"
};
s32 gPlayerLookFiles_9_261[2] = {
#include "sub_main/0042B1A8.inc"
};
s32 gPlayerLookFiles_9_263[2] = {
#include "sub_main/0042B1B0.inc"
};
s32 gPlayerLookFiles_9_265[2] = {
#include "sub_main/0042B1B8.inc"
};
s32 gPlayerLookFiles_9_267[2] = {
#include "sub_main/0042B1C0.inc"
};
s32 gPlayerLookFiles_9_269[2] = {
#include "sub_main/0042B1C8.inc"
};
s32 gPlayerLookFiles_9_271[2] = {
#include "sub_main/0042B1D0.inc"
};
s32 gPlayerLookFiles_9_273[2] = {
#include "sub_main/0042B1D8.inc"
};
s32 gPlayerLookFiles_9_275[2] = {
#include "sub_main/0042B1E0.inc"
};
s32 gPlayerLookFiles_9_277[2] = {
#include "sub_main/0042B1E8.inc"
};
s32 gPlayerLookFiles_9_279[2] = {
#include "sub_main/0042B1F0.inc"
};
s32 gPlayerLookFiles_9_281[2] = {
#include "sub_main/0042B1F8.inc"
};
s32 gPlayerLookFiles_9_283[2] = {
#include "sub_main/0042B200.inc"
};
s32 gPlayerLookFiles_9_285[2] = {
#include "sub_main/0042B208.inc"
};
s32 gPlayerLookFiles_9_287[2] = {
#include "sub_main/0042B210.inc"
};
s32 gPlayerLookFiles_9_289[2] = {
#include "sub_main/0042B218.inc"
};
s32 gPlayerLookFiles_9_291[2] = {
#include "sub_main/0042B220.inc"
};
s32 gPlayerLookFiles_9_293[2] = {
#include "sub_main/0042B228.inc"
};
s32 gPlayerLookFiles_9_295[2] = {
#include "sub_main/0042B230.inc"
};
s32 gPlayerLookFiles_9_297[2] = {
#include "sub_main/0042B238.inc"
};
s32 gPlayerLookFiles_9_299[2] = {
#include "sub_main/0042B240.inc"
};
s32 gPlayerLookFiles_9_301[2] = {
#include "sub_main/0042B248.inc"
};
s32 gPlayerLookFiles_9_303[2] = {
#include "sub_main/0042B250.inc"
};
s32 gPlayerLookFiles_9_305[2] = {
#include "sub_main/0042B258.inc"
};
s32 gPlayerLookFiles_9_307[2] = {
#include "sub_main/0042B260.inc"
};
s32 gPlayerLookFiles_9_309[2] = {
#include "sub_main/0042B268.inc"
};
s32 gPlayerLookFiles_9_311[2] = {
#include "sub_main/0042B270.inc"
};
s32 gPlayerLookFiles_9_313[2] = {
#include "sub_main/0042B278.inc"
};
s32 gPlayerLookFiles_9_315[2] = {
#include "sub_main/0042B280.inc"
};
s32 gPlayerLookFiles_9_317[2] = {
#include "sub_main/0042B288.inc"
};
s32 gPlayerLookFiles_9_319[2] = {
#include "sub_main/0042B290.inc"
};
s32 gPlayerLookFiles_9_321[2] = {
#include "sub_main/0042B298.inc"
};
s32 gPlayerLookFiles_9_323[2] = {
#include "sub_main/0042B2A0.inc"
};
s32 gPlayerLookFiles_9_325[2] = {
#include "sub_main/0042B2A8.inc"
};
s32 gPlayerLookFiles_9_327[2] = {
#include "sub_main/0042B2B0.inc"
};
s32 gPlayerLookFiles_9_329[2] = {
#include "sub_main/0042B2B8.inc"
};
s32 gPlayerLookFiles_9_331[2] = {
#include "sub_main/0042B2C0.inc"
};
s32 gPlayerLookFiles_9_333[2] = {
#include "sub_main/0042B2C8.inc"
};
s32 gPlayerLookFiles_9_335[2] = {
#include "sub_main/0042B2D0.inc"
};
s32 gPlayerLookFiles_9_337[2] = {
#include "sub_main/0042B2D8.inc"
};
s32 gPlayerLookFiles_9_339[2] = {
#include "sub_main/0042B2E0.inc"
};
s32 gPlayerLookFiles_9_341[2] = {
#include "sub_main/0042B2E8.inc"
};
s32 gPlayerLookFiles_9_343[2] = {
#include "sub_main/0042B2F0.inc"
};
s32 gPlayerLookFiles_9_345[2] = {
#include "sub_main/0042B2F8.inc"
};
s32 gPlayerLookFiles_9_347[2] = {
#include "sub_main/0042B300.inc"
};
s32 gPlayerLookFiles_9_349[2] = {
#include "sub_main/0042B308.inc"
};
s32 gPlayerLookFiles_9_351[2] = {
#include "sub_main/0042B310.inc"
};
s32 gPlayerLookFiles_9_353[2] = {
#include "sub_main/0042B318.inc"
};
s32 gPlayerLookFiles_9_355[2] = {
#include "sub_main/0042B320.inc"
};
s32 gPlayerLookFiles_9_357[2] = {
#include "sub_main/0042B328.inc"
};
s32 gPlayerLookFiles_9_359[2] = {
#include "sub_main/0042B330.inc"
};
s32 gPlayerLookFiles_9_361[2] = {
#include "sub_main/0042B338.inc"
};
s32 gPlayerLookFiles_9_363[2] = {
#include "sub_main/0042B340.inc"
};
s32 gPlayerLookFiles_9_365[2] = {
#include "sub_main/0042B348.inc"
};
s32 gPlayerLookFiles_9_367[2] = {
#include "sub_main/0042B350.inc"
};
s32 gPlayerLookFiles_9_369[2] = {
#include "sub_main/0042B358.inc"
};
s32 gPlayerLookFiles_9_371[2] = {
#include "sub_main/0042B360.inc"
};
s32 gPlayerLookFiles_9_373[2] = {
#include "sub_main/0042B368.inc"
};
s32 gPlayerLookFiles_9_375[2] = {
#include "sub_main/0042B370.inc"
};
s32 gPlayerLookFiles_9_377[2] = {
#include "sub_main/0042B378.inc"
};
s32 gPlayerLookFiles_9_379[2] = {
#include "sub_main/0042B380.inc"
};
s32 gPlayerLookFiles_9_381[2] = {
#include "sub_main/0042B388.inc"
};
s32 gPlayerLookFiles_9_383[2] = {
#include "sub_main/0042B390.inc"
};
s32 gPlayerLookFiles_9_385[2] = {
#include "sub_main/0042B398.inc"
};
s32 gPlayerLookFiles_9_387[2] = {
#include "sub_main/0042B3A0.inc"
};
s32 gPlayerLookFiles_9_389[2] = {
#include "sub_main/0042B3A8.inc"
};
s32 gPlayerLookFiles_9_391[2] = {
#include "sub_main/0042B3B0.inc"
};
s32 gPlayerLookFiles_9_393[2] = {
#include "sub_main/0042B3B8.inc"
};
s32 gPlayerLookFiles_9_395[2] = {
#include "sub_main/0042B3C0.inc"
};
s32 gPlayerLookFiles_9_397[2] = {
#include "sub_main/0042B3C8.inc"
};
s32 gPlayerLookFiles_9_399[2] = {
#include "sub_main/0042B3D0.inc"
};
s32 gPlayerLookFiles_9_401[2] = {
#include "sub_main/0042B3D8.inc"
};
s32 gPlayerLookFiles_9_403[2] = {
#include "sub_main/0042B3E0.inc"
};
s32 gPlayerLookFiles_9_405[2] = {
#include "sub_main/0042B3E8.inc"
};
s32 gPlayerLookFiles_9_407[2] = {
#include "sub_main/0042B3F0.inc"
};
s32 gPlayerLookFiles_9_409[2] = {
#include "sub_main/0042B3F8.inc"
};
s32 gPlayerLookFiles_9_411[2] = {
#include "sub_main/0042B400.inc"
};
s32 gPlayerLookFiles_9_413[2] = {
#include "sub_main/0042B408.inc"
};
s32 gPlayerLookFiles_9_415[2] = {
#include "sub_main/0042B410.inc"
};
s32 gPlayerLookFiles_9_417[2] = {
#include "sub_main/0042B418.inc"
};
s32 gPlayerLookFiles_9_419[2] = {
#include "sub_main/0042B420.inc"
};
s32 gPlayerLookFiles_9_421[2] = {
#include "sub_main/0042B428.inc"
};
s32 gPlayerLookFiles_9_423[2] = {
#include "sub_main/0042B430.inc"
};
s32 gPlayerLookFiles_9_425[2] = {
#include "sub_main/0042B438.inc"
};
s32 gPlayerLookFiles_9_427[2] = {
#include "sub_main/0042B440.inc"
};
s32 gPlayerLookFiles_9_429[2] = {
#include "sub_main/0042B448.inc"
};
s32 gPlayerLookFiles_9_431[2] = {
#include "sub_main/0042B450.inc"
};
s32 gPlayerLookFiles_9_433[2] = {
#include "sub_main/0042B458.inc"
};
s32 gPlayerLookFiles_9_435[2] = {
#include "sub_main/0042B460.inc"
};
s32 gPlayerLookFiles_9_437[2] = {
#include "sub_main/0042B468.inc"
};
TexPackHead gPlayerLookFiles_9[219] = {
#include "sub_main/0042B470.inc"
};
s32 gPlayerLookFiles_10_1[2] = {
#include "sub_main/0042BB48.inc"
};
s32 gPlayerLookFiles_10_3[2] = {
#include "sub_main/0042BB50.inc"
};
s32 gPlayerLookFiles_10_5[2] = {
#include "sub_main/0042BB58.inc"
};
s32 gPlayerLookFiles_10_7[2] = {
#include "sub_main/0042BB60.inc"
};
s32 gPlayerLookFiles_10_9[2] = {
#include "sub_main/0042BB68.inc"
};
s32 gPlayerLookFiles_10_11[2] = {
#include "sub_main/0042BB70.inc"
};
s32 gPlayerLookFiles_10_13[2] = {
#include "sub_main/0042BB78.inc"
};
s32 gPlayerLookFiles_10_15[2] = {
#include "sub_main/0042BB80.inc"
};
s32 gPlayerLookFiles_10_17[2] = {
#include "sub_main/0042BB88.inc"
};
s32 gPlayerLookFiles_10_19[2] = {
#include "sub_main/0042BB90.inc"
};
s32 gPlayerLookFiles_10_21[2] = {
#include "sub_main/0042BB98.inc"
};
s32 gPlayerLookFiles_10_23[2] = {
#include "sub_main/0042BBA0.inc"
};
s32 gPlayerLookFiles_10_25[2] = {
#include "sub_main/0042BBA8.inc"
};
s32 gPlayerLookFiles_10_27[2] = {
#include "sub_main/0042BBB0.inc"
};
s32 gPlayerLookFiles_10_29[2] = {
#include "sub_main/0042BBB8.inc"
};
s32 gPlayerLookFiles_10_31[2] = {
#include "sub_main/0042BBC0.inc"
};
s32 gPlayerLookFiles_10_33[2] = {
#include "sub_main/0042BBC8.inc"
};
s32 gPlayerLookFiles_10_35[2] = {
#include "sub_main/0042BBD0.inc"
};
s32 gPlayerLookFiles_10_37[2] = {
#include "sub_main/0042BBD8.inc"
};
s32 gPlayerLookFiles_10_39[2] = {
#include "sub_main/0042BBE0.inc"
};
s32 gPlayerLookFiles_10_41[2] = {
#include "sub_main/0042BBE8.inc"
};
s32 gPlayerLookFiles_10_43[2] = {
#include "sub_main/0042BBF0.inc"
};
s32 gPlayerLookFiles_10_45[2] = {
#include "sub_main/0042BBF8.inc"
};
s32 gPlayerLookFiles_10_47[2] = {
#include "sub_main/0042BC00.inc"
};
s32 gPlayerLookFiles_10_49[2] = {
#include "sub_main/0042BC08.inc"
};
s32 gPlayerLookFiles_10_51[2] = {
#include "sub_main/0042BC10.inc"
};
s32 gPlayerLookFiles_10_53[2] = {
#include "sub_main/0042BC18.inc"
};
s32 gPlayerLookFiles_10_55[2] = {
#include "sub_main/0042BC20.inc"
};
s32 gPlayerLookFiles_10_57[2] = {
#include "sub_main/0042BC28.inc"
};
s32 gPlayerLookFiles_10_59[2] = {
#include "sub_main/0042BC30.inc"
};
s32 gPlayerLookFiles_10_61[2] = {
#include "sub_main/0042BC38.inc"
};
s32 gPlayerLookFiles_10_63[2] = {
#include "sub_main/0042BC40.inc"
};
s32 gPlayerLookFiles_10_65[2] = {
#include "sub_main/0042BC48.inc"
};
s32 gPlayerLookFiles_10_67[2] = {
#include "sub_main/0042BC50.inc"
};
s32 gPlayerLookFiles_10_69[2] = {
#include "sub_main/0042BC58.inc"
};
s32 gPlayerLookFiles_10_71[2] = {
#include "sub_main/0042BC60.inc"
};
s32 gPlayerLookFiles_10_73[2] = {
#include "sub_main/0042BC68.inc"
};
s32 gPlayerLookFiles_10_75[2] = {
#include "sub_main/0042BC70.inc"
};
s32 gPlayerLookFiles_10_77[2] = {
#include "sub_main/0042BC78.inc"
};
s32 gPlayerLookFiles_10_79[2] = {
#include "sub_main/0042BC80.inc"
};
s32 gPlayerLookFiles_10_81[2] = {
#include "sub_main/0042BC88.inc"
};
s32 gPlayerLookFiles_10_83[2] = {
#include "sub_main/0042BC90.inc"
};
s32 gPlayerLookFiles_10_85[2] = {
#include "sub_main/0042BC98.inc"
};
s32 gPlayerLookFiles_10_87[2] = {
#include "sub_main/0042BCA0.inc"
};
s32 gPlayerLookFiles_10_89[2] = {
#include "sub_main/0042BCA8.inc"
};
s32 gPlayerLookFiles_10_91[2] = {
#include "sub_main/0042BCB0.inc"
};
s32 gPlayerLookFiles_10_93[2] = {
#include "sub_main/0042BCB8.inc"
};
s32 gPlayerLookFiles_10_95[2] = {
#include "sub_main/0042BCC0.inc"
};
s32 gPlayerLookFiles_10_97[2] = {
#include "sub_main/0042BCC8.inc"
};
s32 gPlayerLookFiles_10_99[2] = {
#include "sub_main/0042BCD0.inc"
};
s32 gPlayerLookFiles_10_101[2] = {
#include "sub_main/0042BCD8.inc"
};
s32 gPlayerLookFiles_10_103[2] = {
#include "sub_main/0042BCE0.inc"
};
s32 gPlayerLookFiles_10_105[2] = {
#include "sub_main/0042BCE8.inc"
};
s32 gPlayerLookFiles_10_107[2] = {
#include "sub_main/0042BCF0.inc"
};
s32 gPlayerLookFiles_10_109[2] = {
#include "sub_main/0042BCF8.inc"
};
s32 gPlayerLookFiles_10_111[2] = {
#include "sub_main/0042BD00.inc"
};
s32 gPlayerLookFiles_10_113[2] = {
#include "sub_main/0042BD08.inc"
};
s32 gPlayerLookFiles_10_115[2] = {
#include "sub_main/0042BD10.inc"
};
s32 gPlayerLookFiles_10_117[2] = {
#include "sub_main/0042BD18.inc"
};
s32 gPlayerLookFiles_10_119[2] = {
#include "sub_main/0042BD20.inc"
};
s32 gPlayerLookFiles_10_121[2] = {
#include "sub_main/0042BD28.inc"
};
s32 gPlayerLookFiles_10_123[2] = {
#include "sub_main/0042BD30.inc"
};
s32 gPlayerLookFiles_10_125[2] = {
#include "sub_main/0042BD38.inc"
};
s32 gPlayerLookFiles_10_127[2] = {
#include "sub_main/0042BD40.inc"
};
s32 gPlayerLookFiles_10_129[2] = {
#include "sub_main/0042BD48.inc"
};
s32 gPlayerLookFiles_10_131[2] = {
#include "sub_main/0042BD50.inc"
};
s32 gPlayerLookFiles_10_133[2] = {
#include "sub_main/0042BD58.inc"
};
s32 gPlayerLookFiles_10_135[2] = {
#include "sub_main/0042BD60.inc"
};
s32 gPlayerLookFiles_10_137[2] = {
#include "sub_main/0042BD68.inc"
};
s32 gPlayerLookFiles_10_139[2] = {
#include "sub_main/0042BD70.inc"
};
s32 gPlayerLookFiles_10_141[2] = {
#include "sub_main/0042BD78.inc"
};
s32 gPlayerLookFiles_10_143[2] = {
#include "sub_main/0042BD80.inc"
};
s32 gPlayerLookFiles_10_145[2] = {
#include "sub_main/0042BD88.inc"
};
s32 gPlayerLookFiles_10_147[2] = {
#include "sub_main/0042BD90.inc"
};
s32 gPlayerLookFiles_10_149[2] = {
#include "sub_main/0042BD98.inc"
};
s32 gPlayerLookFiles_10_151[2] = {
#include "sub_main/0042BDA0.inc"
};
s32 gPlayerLookFiles_10_153[2] = {
#include "sub_main/0042BDA8.inc"
};
s32 gPlayerLookFiles_10_155[2] = {
#include "sub_main/0042BDB0.inc"
};
s32 gPlayerLookFiles_10_157[2] = {
#include "sub_main/0042BDB8.inc"
};
s32 gPlayerLookFiles_10_159[2] = {
#include "sub_main/0042BDC0.inc"
};
s32 gPlayerLookFiles_10_161[2] = {
#include "sub_main/0042BDC8.inc"
};
s32 gPlayerLookFiles_10_163[2] = {
#include "sub_main/0042BDD0.inc"
};
s32 gPlayerLookFiles_10_165[2] = {
#include "sub_main/0042BDD8.inc"
};
s32 gPlayerLookFiles_10_167[2] = {
#include "sub_main/0042BDE0.inc"
};
s32 gPlayerLookFiles_10_169[2] = {
#include "sub_main/0042BDE8.inc"
};
s32 gPlayerLookFiles_10_171[2] = {
#include "sub_main/0042BDF0.inc"
};
s32 gPlayerLookFiles_10_173[2] = {
#include "sub_main/0042BDF8.inc"
};
s32 gPlayerLookFiles_10_175[2] = {
#include "sub_main/0042BE00.inc"
};
s32 gPlayerLookFiles_10_177[2] = {
#include "sub_main/0042BE08.inc"
};
s32 gPlayerLookFiles_10_179[2] = {
#include "sub_main/0042BE10.inc"
};
s32 gPlayerLookFiles_10_181[2] = {
#include "sub_main/0042BE18.inc"
};
s32 gPlayerLookFiles_10_183[2] = {
#include "sub_main/0042BE20.inc"
};
s32 gPlayerLookFiles_10_185[2] = {
#include "sub_main/0042BE28.inc"
};
s32 gPlayerLookFiles_10_187[2] = {
#include "sub_main/0042BE30.inc"
};
s32 gPlayerLookFiles_10_189[2] = {
#include "sub_main/0042BE38.inc"
};
s32 gPlayerLookFiles_10_191[2] = {
#include "sub_main/0042BE40.inc"
};
s32 gPlayerLookFiles_10_193[2] = {
#include "sub_main/0042BE48.inc"
};
s32 gPlayerLookFiles_10_195[2] = {
#include "sub_main/0042BE50.inc"
};
s32 gPlayerLookFiles_10_197[2] = {
#include "sub_main/0042BE58.inc"
};
s32 gPlayerLookFiles_10_199[2] = {
#include "sub_main/0042BE60.inc"
};
s32 gPlayerLookFiles_10_201[2] = {
#include "sub_main/0042BE68.inc"
};
s32 gPlayerLookFiles_10_203[2] = {
#include "sub_main/0042BE70.inc"
};
s32 gPlayerLookFiles_10_205[2] = {
#include "sub_main/0042BE78.inc"
};
s32 gPlayerLookFiles_10_207[2] = {
#include "sub_main/0042BE80.inc"
};
s32 gPlayerLookFiles_10_209[2] = {
#include "sub_main/0042BE88.inc"
};
s32 gPlayerLookFiles_10_211[2] = {
#include "sub_main/0042BE90.inc"
};
s32 gPlayerLookFiles_10_213[2] = {
#include "sub_main/0042BE98.inc"
};
s32 gPlayerLookFiles_10_215[2] = {
#include "sub_main/0042BEA0.inc"
};
s32 gPlayerLookFiles_10_217[2] = {
#include "sub_main/0042BEA8.inc"
};
s32 gPlayerLookFiles_10_219[2] = {
#include "sub_main/0042BEB0.inc"
};
s32 gPlayerLookFiles_10_221[2] = {
#include "sub_main/0042BEB8.inc"
};
s32 gPlayerLookFiles_10_223[2] = {
#include "sub_main/0042BEC0.inc"
};
s32 gPlayerLookFiles_10_225[2] = {
#include "sub_main/0042BEC8.inc"
};
s32 gPlayerLookFiles_10_227[2] = {
#include "sub_main/0042BED0.inc"
};
s32 gPlayerLookFiles_10_229[2] = {
#include "sub_main/0042BED8.inc"
};
s32 gPlayerLookFiles_10_231[2] = {
#include "sub_main/0042BEE0.inc"
};
s32 gPlayerLookFiles_10_233[2] = {
#include "sub_main/0042BEE8.inc"
};
s32 gPlayerLookFiles_10_235[2] = {
#include "sub_main/0042BEF0.inc"
};
s32 gPlayerLookFiles_10_237[2] = {
#include "sub_main/0042BEF8.inc"
};
s32 gPlayerLookFiles_10_239[2] = {
#include "sub_main/0042BF00.inc"
};
s32 gPlayerLookFiles_10_241[2] = {
#include "sub_main/0042BF08.inc"
};
s32 gPlayerLookFiles_10_243[2] = {
#include "sub_main/0042BF10.inc"
};
s32 gPlayerLookFiles_10_245[2] = {
#include "sub_main/0042BF18.inc"
};
s32 gPlayerLookFiles_10_247[2] = {
#include "sub_main/0042BF20.inc"
};
s32 gPlayerLookFiles_10_249[2] = {
#include "sub_main/0042BF28.inc"
};
s32 gPlayerLookFiles_10_251[2] = {
#include "sub_main/0042BF30.inc"
};
s32 gPlayerLookFiles_10_253[2] = {
#include "sub_main/0042BF38.inc"
};
s32 gPlayerLookFiles_10_255[2] = {
#include "sub_main/0042BF40.inc"
};
s32 gPlayerLookFiles_10_257[2] = {
#include "sub_main/0042BF48.inc"
};
s32 gPlayerLookFiles_10_259[2] = {
#include "sub_main/0042BF50.inc"
};
s32 gPlayerLookFiles_10_261[2] = {
#include "sub_main/0042BF58.inc"
};
s32 gPlayerLookFiles_10_263[2] = {
#include "sub_main/0042BF60.inc"
};
s32 gPlayerLookFiles_10_265[2] = {
#include "sub_main/0042BF68.inc"
};
s32 gPlayerLookFiles_10_267[2] = {
#include "sub_main/0042BF70.inc"
};
s32 gPlayerLookFiles_10_269[2] = {
#include "sub_main/0042BF78.inc"
};
s32 gPlayerLookFiles_10_271[2] = {
#include "sub_main/0042BF80.inc"
};
s32 gPlayerLookFiles_10_273[2] = {
#include "sub_main/0042BF88.inc"
};
s32 gPlayerLookFiles_10_275[2] = {
#include "sub_main/0042BF90.inc"
};
s32 gPlayerLookFiles_10_277[2] = {
#include "sub_main/0042BF98.inc"
};
s32 gPlayerLookFiles_10_279[2] = {
#include "sub_main/0042BFA0.inc"
};
s32 gPlayerLookFiles_10_281[2] = {
#include "sub_main/0042BFA8.inc"
};
s32 gPlayerLookFiles_10_283[2] = {
#include "sub_main/0042BFB0.inc"
};
s32 gPlayerLookFiles_10_285[2] = {
#include "sub_main/0042BFB8.inc"
};
s32 gPlayerLookFiles_10_287[2] = {
#include "sub_main/0042BFC0.inc"
};
s32 gPlayerLookFiles_10_289[2] = {
#include "sub_main/0042BFC8.inc"
};
s32 gPlayerLookFiles_10_291[2] = {
#include "sub_main/0042BFD0.inc"
};
s32 gPlayerLookFiles_10_293[2] = {
#include "sub_main/0042BFD8.inc"
};
s32 gPlayerLookFiles_10_295[2] = {
#include "sub_main/0042BFE0.inc"
};
s32 gPlayerLookFiles_10_297[2] = {
#include "sub_main/0042BFE8.inc"
};
s32 gPlayerLookFiles_10_299[2] = {
#include "sub_main/0042BFF0.inc"
};
s32 gPlayerLookFiles_10_301[2] = {
#include "sub_main/0042BFF8.inc"
};
s32 gPlayerLookFiles_10_303[2] = {
#include "sub_main/0042C000.inc"
};
s32 gPlayerLookFiles_10_305[2] = {
#include "sub_main/0042C008.inc"
};
s32 gPlayerLookFiles_10_307[2] = {
#include "sub_main/0042C010.inc"
};
s32 gPlayerLookFiles_10_309[2] = {
#include "sub_main/0042C018.inc"
};
s32 gPlayerLookFiles_10_311[2] = {
#include "sub_main/0042C020.inc"
};
s32 gPlayerLookFiles_10_313[2] = {
#include "sub_main/0042C028.inc"
};
s32 gPlayerLookFiles_10_315[2] = {
#include "sub_main/0042C030.inc"
};
s32 gPlayerLookFiles_10_317[2] = {
#include "sub_main/0042C038.inc"
};
s32 gPlayerLookFiles_10_319[2] = {
#include "sub_main/0042C040.inc"
};
s32 gPlayerLookFiles_10_321[2] = {
#include "sub_main/0042C048.inc"
};
s32 gPlayerLookFiles_10_323[2] = {
#include "sub_main/0042C050.inc"
};
s32 gPlayerLookFiles_10_325[2] = {
#include "sub_main/0042C058.inc"
};
s32 gPlayerLookFiles_10_327[2] = {
#include "sub_main/0042C060.inc"
};
s32 gPlayerLookFiles_10_329[2] = {
#include "sub_main/0042C068.inc"
};
s32 gPlayerLookFiles_10_331[2] = {
#include "sub_main/0042C070.inc"
};
s32 gPlayerLookFiles_10_333[2] = {
#include "sub_main/0042C078.inc"
};
s32 gPlayerLookFiles_10_335[2] = {
#include "sub_main/0042C080.inc"
};
s32 gPlayerLookFiles_10_337[2] = {
#include "sub_main/0042C088.inc"
};
s32 gPlayerLookFiles_10_339[2] = {
#include "sub_main/0042C090.inc"
};
s32 gPlayerLookFiles_10_341[2] = {
#include "sub_main/0042C098.inc"
};
s32 gPlayerLookFiles_10_343[2] = {
#include "sub_main/0042C0A0.inc"
};
s32 gPlayerLookFiles_10_345[2] = {
#include "sub_main/0042C0A8.inc"
};
s32 gPlayerLookFiles_10_347[2] = {
#include "sub_main/0042C0B0.inc"
};
s32 gPlayerLookFiles_10_349[2] = {
#include "sub_main/0042C0B8.inc"
};
s32 gPlayerLookFiles_10_351[2] = {
#include "sub_main/0042C0C0.inc"
};
s32 gPlayerLookFiles_10_353[2] = {
#include "sub_main/0042C0C8.inc"
};
s32 gPlayerLookFiles_10_355[2] = {
#include "sub_main/0042C0D0.inc"
};
s32 gPlayerLookFiles_10_357[2] = {
#include "sub_main/0042C0D8.inc"
};
s32 gPlayerLookFiles_10_359[2] = {
#include "sub_main/0042C0E0.inc"
};
s32 gPlayerLookFiles_10_361[2] = {
#include "sub_main/0042C0E8.inc"
};
s32 gPlayerLookFiles_10_363[2] = {
#include "sub_main/0042C0F0.inc"
};
s32 gPlayerLookFiles_10_365[2] = {
#include "sub_main/0042C0F8.inc"
};
s32 gPlayerLookFiles_10_367[2] = {
#include "sub_main/0042C100.inc"
};
s32 gPlayerLookFiles_10_369[2] = {
#include "sub_main/0042C108.inc"
};
s32 gPlayerLookFiles_10_371[2] = {
#include "sub_main/0042C110.inc"
};
s32 gPlayerLookFiles_10_373[2] = {
#include "sub_main/0042C118.inc"
};
s32 gPlayerLookFiles_10_375[2] = {
#include "sub_main/0042C120.inc"
};
s32 gPlayerLookFiles_10_377[2] = {
#include "sub_main/0042C128.inc"
};
s32 gPlayerLookFiles_10_379[2] = {
#include "sub_main/0042C130.inc"
};
s32 gPlayerLookFiles_10_381[2] = {
#include "sub_main/0042C138.inc"
};
s32 gPlayerLookFiles_10_383[2] = {
#include "sub_main/0042C140.inc"
};
s32 gPlayerLookFiles_10_385[2] = {
#include "sub_main/0042C148.inc"
};
s32 gPlayerLookFiles_10_387[2] = {
#include "sub_main/0042C150.inc"
};
s32 gPlayerLookFiles_10_389[2] = {
#include "sub_main/0042C158.inc"
};
s32 gPlayerLookFiles_10_391[2] = {
#include "sub_main/0042C160.inc"
};
s32 gPlayerLookFiles_10_393[2] = {
#include "sub_main/0042C168.inc"
};
s32 gPlayerLookFiles_10_395[2] = {
#include "sub_main/0042C170.inc"
};
s32 gPlayerLookFiles_10_397[2] = {
#include "sub_main/0042C178.inc"
};
s32 gPlayerLookFiles_10_399[2] = {
#include "sub_main/0042C180.inc"
};
s32 gPlayerLookFiles_10_401[2] = {
#include "sub_main/0042C188.inc"
};
s32 gPlayerLookFiles_10_403[2] = {
#include "sub_main/0042C190.inc"
};
s32 gPlayerLookFiles_10_405[2] = {
#include "sub_main/0042C198.inc"
};
s32 gPlayerLookFiles_10_407[2] = {
#include "sub_main/0042C1A0.inc"
};
s32 gPlayerLookFiles_10_409[2] = {
#include "sub_main/0042C1A8.inc"
};
s32 gPlayerLookFiles_10_411[2] = {
#include "sub_main/0042C1B0.inc"
};
s32 gPlayerLookFiles_10_413[2] = {
#include "sub_main/0042C1B8.inc"
};
s32 gPlayerLookFiles_10_415[2] = {
#include "sub_main/0042C1C0.inc"
};
s32 gPlayerLookFiles_10_417[2] = {
#include "sub_main/0042C1C8.inc"
};
s32 gPlayerLookFiles_10_419[2] = {
#include "sub_main/0042C1D0.inc"
};
s32 gPlayerLookFiles_10_421[2] = {
#include "sub_main/0042C1D8.inc"
};
s32 gPlayerLookFiles_10_423[2] = {
#include "sub_main/0042C1E0.inc"
};
s32 gPlayerLookFiles_10_425[2] = {
#include "sub_main/0042C1E8.inc"
};
s32 gPlayerLookFiles_10_427[2] = {
#include "sub_main/0042C1F0.inc"
};
s32 gPlayerLookFiles_10_429[2] = {
#include "sub_main/0042C1F8.inc"
};
s32 gPlayerLookFiles_10_431[2] = {
#include "sub_main/0042C200.inc"
};
s32 gPlayerLookFiles_10_433[2] = {
#include "sub_main/0042C208.inc"
};
s32 gPlayerLookFiles_10_435[2] = {
#include "sub_main/0042C210.inc"
};
s32 gPlayerLookFiles_10_437[2] = {
#include "sub_main/0042C218.inc"
};
s32 gPlayerLookFiles_10_439[2] = {
#include "sub_main/0042C220.inc"
};
s32 gPlayerLookFiles_10_441[2] = {
#include "sub_main/0042C228.inc"
};
s32 gPlayerLookFiles_10_443[2] = {
#include "sub_main/0042C230.inc"
};
s32 gPlayerLookFiles_10_445[2] = {
#include "sub_main/0042C238.inc"
};
s32 gPlayerLookFiles_10_447[2] = {
#include "sub_main/0042C240.inc"
};
s32 gPlayerLookFiles_10_449[2] = {
#include "sub_main/0042C248.inc"
};
s32 gPlayerLookFiles_10_451[2] = {
#include "sub_main/0042C250.inc"
};
s32 gPlayerLookFiles_10_453[2] = {
#include "sub_main/0042C258.inc"
};
s32 gPlayerLookFiles_10_455[2] = {
#include "sub_main/0042C260.inc"
};
s32 gPlayerLookFiles_10_457[2] = {
#include "sub_main/0042C268.inc"
};
s32 gPlayerLookFiles_10_459[2] = {
#include "sub_main/0042C270.inc"
};
s32 gPlayerLookFiles_10_461[2] = {
#include "sub_main/0042C278.inc"
};
s32 gPlayerLookFiles_10_463[2] = {
#include "sub_main/0042C280.inc"
};
s32 gPlayerLookFiles_10_465[2] = {
#include "sub_main/0042C288.inc"
};
s32 gPlayerLookFiles_10_467[2] = {
#include "sub_main/0042C290.inc"
};
s32 gPlayerLookFiles_10_469[2] = {
#include "sub_main/0042C298.inc"
};
TexPackHead gPlayerLookFiles_10[235] = {
#include "sub_main/0042C2A0.inc"
};
s32 gPlayerLookFiles_11_1[2] = {
#include "sub_main/0042C9F8.inc"
};
s32 gPlayerLookFiles_11_3[2] = {
#include "sub_main/0042CA00.inc"
};
s32 gPlayerLookFiles_11_5[2] = {
#include "sub_main/0042CA08.inc"
};
s32 gPlayerLookFiles_11_7[2] = {
#include "sub_main/0042CA10.inc"
};
s32 gPlayerLookFiles_11_9[2] = {
#include "sub_main/0042CA18.inc"
};
s32 gPlayerLookFiles_11_11[2] = {
#include "sub_main/0042CA20.inc"
};
s32 gPlayerLookFiles_11_13[2] = {
#include "sub_main/0042CA28.inc"
};
s32 gPlayerLookFiles_11_15[2] = {
#include "sub_main/0042CA30.inc"
};
s32 gPlayerLookFiles_11_17[2] = {
#include "sub_main/0042CA38.inc"
};
s32 gPlayerLookFiles_11_19[2] = {
#include "sub_main/0042CA40.inc"
};
s32 gPlayerLookFiles_11_21[2] = {
#include "sub_main/0042CA48.inc"
};
s32 gPlayerLookFiles_11_23[2] = {
#include "sub_main/0042CA50.inc"
};
s32 gPlayerLookFiles_11_25[2] = {
#include "sub_main/0042CA58.inc"
};
s32 gPlayerLookFiles_11_27[2] = {
#include "sub_main/0042CA60.inc"
};
s32 gPlayerLookFiles_11_29[2] = {
#include "sub_main/0042CA68.inc"
};
s32 gPlayerLookFiles_11_31[2] = {
#include "sub_main/0042CA70.inc"
};
s32 gPlayerLookFiles_11_33[2] = {
#include "sub_main/0042CA78.inc"
};
s32 gPlayerLookFiles_11_35[2] = {
#include "sub_main/0042CA80.inc"
};
s32 gPlayerLookFiles_11_37[2] = {
#include "sub_main/0042CA88.inc"
};
s32 gPlayerLookFiles_11_39[2] = {
#include "sub_main/0042CA90.inc"
};
s32 gPlayerLookFiles_11_41[2] = {
#include "sub_main/0042CA98.inc"
};
s32 gPlayerLookFiles_11_43[2] = {
#include "sub_main/0042CAA0.inc"
};
s32 gPlayerLookFiles_11_45[2] = {
#include "sub_main/0042CAA8.inc"
};
s32 gPlayerLookFiles_11_47[2] = {
#include "sub_main/0042CAB0.inc"
};
s32 gPlayerLookFiles_11_49[2] = {
#include "sub_main/0042CAB8.inc"
};
s32 gPlayerLookFiles_11_51[2] = {
#include "sub_main/0042CAC0.inc"
};
s32 gPlayerLookFiles_11_53[2] = {
#include "sub_main/0042CAC8.inc"
};
s32 gPlayerLookFiles_11_55[2] = {
#include "sub_main/0042CAD0.inc"
};
s32 gPlayerLookFiles_11_57[2] = {
#include "sub_main/0042CAD8.inc"
};
s32 gPlayerLookFiles_11_59[2] = {
#include "sub_main/0042CAE0.inc"
};
s32 gPlayerLookFiles_11_61[2] = {
#include "sub_main/0042CAE8.inc"
};
s32 gPlayerLookFiles_11_63[2] = {
#include "sub_main/0042CAF0.inc"
};
s32 gPlayerLookFiles_11_65[2] = {
#include "sub_main/0042CAF8.inc"
};
s32 gPlayerLookFiles_11_67[2] = {
#include "sub_main/0042CB00.inc"
};
s32 gPlayerLookFiles_11_69[2] = {
#include "sub_main/0042CB08.inc"
};
s32 gPlayerLookFiles_11_71[2] = {
#include "sub_main/0042CB10.inc"
};
s32 gPlayerLookFiles_11_73[2] = {
#include "sub_main/0042CB18.inc"
};
s32 gPlayerLookFiles_11_75[2] = {
#include "sub_main/0042CB20.inc"
};
s32 gPlayerLookFiles_11_77[2] = {
#include "sub_main/0042CB28.inc"
};
s32 gPlayerLookFiles_11_79[2] = {
#include "sub_main/0042CB30.inc"
};
s32 gPlayerLookFiles_11_81[2] = {
#include "sub_main/0042CB38.inc"
};
s32 gPlayerLookFiles_11_83[2] = {
#include "sub_main/0042CB40.inc"
};
s32 gPlayerLookFiles_11_85[2] = {
#include "sub_main/0042CB48.inc"
};
s32 gPlayerLookFiles_11_87[2] = {
#include "sub_main/0042CB50.inc"
};
s32 gPlayerLookFiles_11_89[2] = {
#include "sub_main/0042CB58.inc"
};
s32 gPlayerLookFiles_11_91[2] = {
#include "sub_main/0042CB60.inc"
};
s32 gPlayerLookFiles_11_93[2] = {
#include "sub_main/0042CB68.inc"
};
s32 gPlayerLookFiles_11_95[2] = {
#include "sub_main/0042CB70.inc"
};
s32 gPlayerLookFiles_11_97[2] = {
#include "sub_main/0042CB78.inc"
};
s32 gPlayerLookFiles_11_99[2] = {
#include "sub_main/0042CB80.inc"
};
s32 gPlayerLookFiles_11_101[2] = {
#include "sub_main/0042CB88.inc"
};
s32 gPlayerLookFiles_11_103[2] = {
#include "sub_main/0042CB90.inc"
};
s32 gPlayerLookFiles_11_105[2] = {
#include "sub_main/0042CB98.inc"
};
s32 gPlayerLookFiles_11_107[2] = {
#include "sub_main/0042CBA0.inc"
};
s32 gPlayerLookFiles_11_109[2] = {
#include "sub_main/0042CBA8.inc"
};
s32 gPlayerLookFiles_11_111[2] = {
#include "sub_main/0042CBB0.inc"
};
s32 gPlayerLookFiles_11_113[2] = {
#include "sub_main/0042CBB8.inc"
};
s32 gPlayerLookFiles_11_115[2] = {
#include "sub_main/0042CBC0.inc"
};
s32 gPlayerLookFiles_11_117[2] = {
#include "sub_main/0042CBC8.inc"
};
s32 gPlayerLookFiles_11_119[2] = {
#include "sub_main/0042CBD0.inc"
};
s32 gPlayerLookFiles_11_121[2] = {
#include "sub_main/0042CBD8.inc"
};
s32 gPlayerLookFiles_11_123[2] = {
#include "sub_main/0042CBE0.inc"
};
s32 gPlayerLookFiles_11_125[2] = {
#include "sub_main/0042CBE8.inc"
};
s32 gPlayerLookFiles_11_127[2] = {
#include "sub_main/0042CBF0.inc"
};
s32 gPlayerLookFiles_11_129[2] = {
#include "sub_main/0042CBF8.inc"
};
s32 gPlayerLookFiles_11_131[2] = {
#include "sub_main/0042CC00.inc"
};
s32 gPlayerLookFiles_11_133[2] = {
#include "sub_main/0042CC08.inc"
};
s32 gPlayerLookFiles_11_135[2] = {
#include "sub_main/0042CC10.inc"
};
s32 gPlayerLookFiles_11_137[2] = {
#include "sub_main/0042CC18.inc"
};
s32 gPlayerLookFiles_11_139[2] = {
#include "sub_main/0042CC20.inc"
};
s32 gPlayerLookFiles_11_141[2] = {
#include "sub_main/0042CC28.inc"
};
s32 gPlayerLookFiles_11_143[2] = {
#include "sub_main/0042CC30.inc"
};
s32 gPlayerLookFiles_11_145[2] = {
#include "sub_main/0042CC38.inc"
};
s32 gPlayerLookFiles_11_147[2] = {
#include "sub_main/0042CC40.inc"
};
s32 gPlayerLookFiles_11_149[2] = {
#include "sub_main/0042CC48.inc"
};
s32 gPlayerLookFiles_11_151[2] = {
#include "sub_main/0042CC50.inc"
};
s32 gPlayerLookFiles_11_153[2] = {
#include "sub_main/0042CC58.inc"
};
s32 gPlayerLookFiles_11_155[2] = {
#include "sub_main/0042CC60.inc"
};
s32 gPlayerLookFiles_11_157[2] = {
#include "sub_main/0042CC68.inc"
};
s32 gPlayerLookFiles_11_159[2] = {
#include "sub_main/0042CC70.inc"
};
s32 gPlayerLookFiles_11_161[2] = {
#include "sub_main/0042CC78.inc"
};
s32 gPlayerLookFiles_11_163[2] = {
#include "sub_main/0042CC80.inc"
};
s32 gPlayerLookFiles_11_165[2] = {
#include "sub_main/0042CC88.inc"
};
s32 gPlayerLookFiles_11_167[2] = {
#include "sub_main/0042CC90.inc"
};
s32 gPlayerLookFiles_11_169[2] = {
#include "sub_main/0042CC98.inc"
};
s32 gPlayerLookFiles_11_171[2] = {
#include "sub_main/0042CCA0.inc"
};
s32 gPlayerLookFiles_11_173[2] = {
#include "sub_main/0042CCA8.inc"
};
s32 gPlayerLookFiles_11_175[2] = {
#include "sub_main/0042CCB0.inc"
};
s32 gPlayerLookFiles_11_177[2] = {
#include "sub_main/0042CCB8.inc"
};
s32 gPlayerLookFiles_11_179[2] = {
#include "sub_main/0042CCC0.inc"
};
s32 gPlayerLookFiles_11_181[2] = {
#include "sub_main/0042CCC8.inc"
};
s32 gPlayerLookFiles_11_183[2] = {
#include "sub_main/0042CCD0.inc"
};
s32 gPlayerLookFiles_11_185[2] = {
#include "sub_main/0042CCD8.inc"
};
s32 gPlayerLookFiles_11_187[2] = {
#include "sub_main/0042CCE0.inc"
};
s32 gPlayerLookFiles_11_189[2] = {
#include "sub_main/0042CCE8.inc"
};
s32 gPlayerLookFiles_11_191[2] = {
#include "sub_main/0042CCF0.inc"
};
s32 gPlayerLookFiles_11_193[2] = {
#include "sub_main/0042CCF8.inc"
};
s32 gPlayerLookFiles_11_195[2] = {
#include "sub_main/0042CD00.inc"
};
s32 gPlayerLookFiles_11_197[2] = {
#include "sub_main/0042CD08.inc"
};
s32 gPlayerLookFiles_11_199[2] = {
#include "sub_main/0042CD10.inc"
};
s32 gPlayerLookFiles_11_201[2] = {
#include "sub_main/0042CD18.inc"
};
s32 gPlayerLookFiles_11_203[2] = {
#include "sub_main/0042CD20.inc"
};
s32 gPlayerLookFiles_11_205[2] = {
#include "sub_main/0042CD28.inc"
};
s32 gPlayerLookFiles_11_207[2] = {
#include "sub_main/0042CD30.inc"
};
s32 gPlayerLookFiles_11_209[2] = {
#include "sub_main/0042CD38.inc"
};
s32 gPlayerLookFiles_11_211[2] = {
#include "sub_main/0042CD40.inc"
};
s32 gPlayerLookFiles_11_213[2] = {
#include "sub_main/0042CD48.inc"
};
s32 gPlayerLookFiles_11_215[2] = {
#include "sub_main/0042CD50.inc"
};
s32 gPlayerLookFiles_11_217[2] = {
#include "sub_main/0042CD58.inc"
};
s32 gPlayerLookFiles_11_219[2] = {
#include "sub_main/0042CD60.inc"
};
s32 gPlayerLookFiles_11_221[2] = {
#include "sub_main/0042CD68.inc"
};
s32 gPlayerLookFiles_11_223[2] = {
#include "sub_main/0042CD70.inc"
};
s32 gPlayerLookFiles_11_225[2] = {
#include "sub_main/0042CD78.inc"
};
s32 gPlayerLookFiles_11_227[2] = {
#include "sub_main/0042CD80.inc"
};
s32 gPlayerLookFiles_11_229[2] = {
#include "sub_main/0042CD88.inc"
};
s32 gPlayerLookFiles_11_231[2] = {
#include "sub_main/0042CD90.inc"
};
s32 gPlayerLookFiles_11_233[2] = {
#include "sub_main/0042CD98.inc"
};
s32 gPlayerLookFiles_11_235[2] = {
#include "sub_main/0042CDA0.inc"
};
s32 gPlayerLookFiles_11_237[2] = {
#include "sub_main/0042CDA8.inc"
};
s32 gPlayerLookFiles_11_239[2] = {
#include "sub_main/0042CDB0.inc"
};
s32 gPlayerLookFiles_11_241[2] = {
#include "sub_main/0042CDB8.inc"
};
s32 gPlayerLookFiles_11_243[2] = {
#include "sub_main/0042CDC0.inc"
};
s32 gPlayerLookFiles_11_245[2] = {
#include "sub_main/0042CDC8.inc"
};
s32 gPlayerLookFiles_11_247[2] = {
#include "sub_main/0042CDD0.inc"
};
s32 gPlayerLookFiles_11_249[2] = {
#include "sub_main/0042CDD8.inc"
};
s32 gPlayerLookFiles_11_251[2] = {
#include "sub_main/0042CDE0.inc"
};
s32 gPlayerLookFiles_11_253[2] = {
#include "sub_main/0042CDE8.inc"
};
s32 gPlayerLookFiles_11_255[2] = {
#include "sub_main/0042CDF0.inc"
};
s32 gPlayerLookFiles_11_257[2] = {
#include "sub_main/0042CDF8.inc"
};
s32 gPlayerLookFiles_11_259[2] = {
#include "sub_main/0042CE00.inc"
};
s32 gPlayerLookFiles_11_261[2] = {
#include "sub_main/0042CE08.inc"
};
s32 gPlayerLookFiles_11_263[2] = {
#include "sub_main/0042CE10.inc"
};
s32 gPlayerLookFiles_11_265[2] = {
#include "sub_main/0042CE18.inc"
};
s32 gPlayerLookFiles_11_267[2] = {
#include "sub_main/0042CE20.inc"
};
s32 gPlayerLookFiles_11_269[2] = {
#include "sub_main/0042CE28.inc"
};
s32 gPlayerLookFiles_11_271[2] = {
#include "sub_main/0042CE30.inc"
};
s32 gPlayerLookFiles_11_273[2] = {
#include "sub_main/0042CE38.inc"
};
s32 gPlayerLookFiles_11_275[2] = {
#include "sub_main/0042CE40.inc"
};
s32 gPlayerLookFiles_11_277[2] = {
#include "sub_main/0042CE48.inc"
};
s32 gPlayerLookFiles_11_279[2] = {
#include "sub_main/0042CE50.inc"
};
s32 gPlayerLookFiles_11_281[2] = {
#include "sub_main/0042CE58.inc"
};
s32 gPlayerLookFiles_11_283[2] = {
#include "sub_main/0042CE60.inc"
};
s32 gPlayerLookFiles_11_285[2] = {
#include "sub_main/0042CE68.inc"
};
s32 gPlayerLookFiles_11_287[2] = {
#include "sub_main/0042CE70.inc"
};
s32 gPlayerLookFiles_11_289[2] = {
#include "sub_main/0042CE78.inc"
};
s32 gPlayerLookFiles_11_291[2] = {
#include "sub_main/0042CE80.inc"
};
s32 gPlayerLookFiles_11_293[2] = {
#include "sub_main/0042CE88.inc"
};
s32 gPlayerLookFiles_11_295[2] = {
#include "sub_main/0042CE90.inc"
};
s32 gPlayerLookFiles_11_297[2] = {
#include "sub_main/0042CE98.inc"
};
s32 gPlayerLookFiles_11_299[2] = {
#include "sub_main/0042CEA0.inc"
};
s32 gPlayerLookFiles_11_301[2] = {
#include "sub_main/0042CEA8.inc"
};
s32 gPlayerLookFiles_11_303[2] = {
#include "sub_main/0042CEB0.inc"
};
s32 gPlayerLookFiles_11_305[2] = {
#include "sub_main/0042CEB8.inc"
};
s32 gPlayerLookFiles_11_307[2] = {
#include "sub_main/0042CEC0.inc"
};
s32 gPlayerLookFiles_11_309[2] = {
#include "sub_main/0042CEC8.inc"
};
s32 gPlayerLookFiles_11_311[2] = {
#include "sub_main/0042CED0.inc"
};
s32 gPlayerLookFiles_11_313[2] = {
#include "sub_main/0042CED8.inc"
};
s32 gPlayerLookFiles_11_315[2] = {
#include "sub_main/0042CEE0.inc"
};
s32 gPlayerLookFiles_11_317[2] = {
#include "sub_main/0042CEE8.inc"
};
s32 gPlayerLookFiles_11_319[2] = {
#include "sub_main/0042CEF0.inc"
};
s32 gPlayerLookFiles_11_321[2] = {
#include "sub_main/0042CEF8.inc"
};
s32 gPlayerLookFiles_11_323[2] = {
#include "sub_main/0042CF00.inc"
};
s32 gPlayerLookFiles_11_325[2] = {
#include "sub_main/0042CF08.inc"
};
s32 gPlayerLookFiles_11_327[2] = {
#include "sub_main/0042CF10.inc"
};
s32 gPlayerLookFiles_11_329[2] = {
#include "sub_main/0042CF18.inc"
};
s32 gPlayerLookFiles_11_331[2] = {
#include "sub_main/0042CF20.inc"
};
s32 gPlayerLookFiles_11_333[2] = {
#include "sub_main/0042CF28.inc"
};
s32 gPlayerLookFiles_11_335[2] = {
#include "sub_main/0042CF30.inc"
};
s32 gPlayerLookFiles_11_337[2] = {
#include "sub_main/0042CF38.inc"
};
s32 gPlayerLookFiles_11_339[2] = {
#include "sub_main/0042CF40.inc"
};
s32 gPlayerLookFiles_11_341[2] = {
#include "sub_main/0042CF48.inc"
};
s32 gPlayerLookFiles_11_343[2] = {
#include "sub_main/0042CF50.inc"
};
s32 gPlayerLookFiles_11_345[2] = {
#include "sub_main/0042CF58.inc"
};
s32 gPlayerLookFiles_11_347[2] = {
#include "sub_main/0042CF60.inc"
};
s32 gPlayerLookFiles_11_349[2] = {
#include "sub_main/0042CF68.inc"
};
s32 gPlayerLookFiles_11_351[2] = {
#include "sub_main/0042CF70.inc"
};
s32 gPlayerLookFiles_11_353[2] = {
#include "sub_main/0042CF78.inc"
};
s32 gPlayerLookFiles_11_355[2] = {
#include "sub_main/0042CF80.inc"
};
s32 gPlayerLookFiles_11_357[2] = {
#include "sub_main/0042CF88.inc"
};
s32 gPlayerLookFiles_11_359[2] = {
#include "sub_main/0042CF90.inc"
};
s32 gPlayerLookFiles_11_361[2] = {
#include "sub_main/0042CF98.inc"
};
s32 gPlayerLookFiles_11_363[2] = {
#include "sub_main/0042CFA0.inc"
};
s32 gPlayerLookFiles_11_365[2] = {
#include "sub_main/0042CFA8.inc"
};
s32 gPlayerLookFiles_11_367[2] = {
#include "sub_main/0042CFB0.inc"
};
s32 gPlayerLookFiles_11_369[2] = {
#include "sub_main/0042CFB8.inc"
};
s32 gPlayerLookFiles_11_371[2] = {
#include "sub_main/0042CFC0.inc"
};
s32 gPlayerLookFiles_11_373[2] = {
#include "sub_main/0042CFC8.inc"
};
s32 gPlayerLookFiles_11_375[2] = {
#include "sub_main/0042CFD0.inc"
};
s32 gPlayerLookFiles_11_377[2] = {
#include "sub_main/0042CFD8.inc"
};
s32 gPlayerLookFiles_11_379[2] = {
#include "sub_main/0042CFE0.inc"
};
s32 gPlayerLookFiles_11_381[2] = {
#include "sub_main/0042CFE8.inc"
};
s32 gPlayerLookFiles_11_383[2] = {
#include "sub_main/0042CFF0.inc"
};
s32 gPlayerLookFiles_11_385[2] = {
#include "sub_main/0042CFF8.inc"
};
s32 gPlayerLookFiles_11_387[2] = {
#include "sub_main/0042D000.inc"
};
s32 gPlayerLookFiles_11_389[2] = {
#include "sub_main/0042D008.inc"
};
s32 gPlayerLookFiles_11_391[2] = {
#include "sub_main/0042D010.inc"
};
s32 gPlayerLookFiles_11_393[2] = {
#include "sub_main/0042D018.inc"
};
s32 gPlayerLookFiles_11_395[2] = {
#include "sub_main/0042D020.inc"
};
s32 gPlayerLookFiles_11_397[2] = {
#include "sub_main/0042D028.inc"
};
s32 gPlayerLookFiles_11_399[2] = {
#include "sub_main/0042D030.inc"
};
s32 gPlayerLookFiles_11_401[2] = {
#include "sub_main/0042D038.inc"
};
s32 gPlayerLookFiles_11_403[2] = {
#include "sub_main/0042D040.inc"
};
s32 gPlayerLookFiles_11_405[2] = {
#include "sub_main/0042D048.inc"
};
s32 gPlayerLookFiles_11_407[2] = {
#include "sub_main/0042D050.inc"
};
s32 gPlayerLookFiles_11_409[2] = {
#include "sub_main/0042D058.inc"
};
s32 gPlayerLookFiles_11_411[2] = {
#include "sub_main/0042D060.inc"
};
s32 gPlayerLookFiles_11_413[2] = {
#include "sub_main/0042D068.inc"
};
s32 gPlayerLookFiles_11_415[2] = {
#include "sub_main/0042D070.inc"
};
s32 gPlayerLookFiles_11_417[2] = {
#include "sub_main/0042D078.inc"
};
s32 gPlayerLookFiles_11_419[2] = {
#include "sub_main/0042D080.inc"
};
s32 gPlayerLookFiles_11_421[2] = {
#include "sub_main/0042D088.inc"
};
s32 gPlayerLookFiles_11_423[2] = {
#include "sub_main/0042D090.inc"
};
s32 gPlayerLookFiles_11_425[2] = {
#include "sub_main/0042D098.inc"
};
s32 gPlayerLookFiles_11_427[2] = {
#include "sub_main/0042D0A0.inc"
};
s32 gPlayerLookFiles_11_429[2] = {
#include "sub_main/0042D0A8.inc"
};
s32 gPlayerLookFiles_11_431[2] = {
#include "sub_main/0042D0B0.inc"
};
s32 gPlayerLookFiles_11_433[2] = {
#include "sub_main/0042D0B8.inc"
};
s32 gPlayerLookFiles_11_435[2] = {
#include "sub_main/0042D0C0.inc"
};
s32 gPlayerLookFiles_11_437[2] = {
#include "sub_main/0042D0C8.inc"
};
s32 gPlayerLookFiles_11_439[2] = {
#include "sub_main/0042D0D0.inc"
};
s32 gPlayerLookFiles_11_441[2] = {
#include "sub_main/0042D0D8.inc"
};
s32 gPlayerLookFiles_11_443[2] = {
#include "sub_main/0042D0E0.inc"
};
s32 gPlayerLookFiles_11_445[2] = {
#include "sub_main/0042D0E8.inc"
};
s32 gPlayerLookFiles_11_447[2] = {
#include "sub_main/0042D0F0.inc"
};
s32 gPlayerLookFiles_11_449[2] = {
#include "sub_main/0042D0F8.inc"
};
s32 gPlayerLookFiles_11_451[2] = {
#include "sub_main/0042D100.inc"
};
s32 gPlayerLookFiles_11_453[2] = {
#include "sub_main/0042D108.inc"
};
s32 gPlayerLookFiles_11_455[4] = {
#include "sub_main/0042D110.inc"
};
TexPackHead gPlayerLookFiles_11[228] = {
#include "sub_main/0042D120.inc"
};
s32 gPlayerLookFiles_12_1[2] = {
#include "sub_main/0042D840.inc"
};
s32 gPlayerLookFiles_12_3[2] = {
#include "sub_main/0042D848.inc"
};
s32 gPlayerLookFiles_12_5[2] = {
#include "sub_main/0042D850.inc"
};
s32 gPlayerLookFiles_12_7[2] = {
#include "sub_main/0042D858.inc"
};
s32 gPlayerLookFiles_12_9[2] = {
#include "sub_main/0042D860.inc"
};
s32 gPlayerLookFiles_12_11[2] = {
#include "sub_main/0042D868.inc"
};
s32 gPlayerLookFiles_12_13[2] = {
#include "sub_main/0042D870.inc"
};
s32 gPlayerLookFiles_12_15[2] = {
#include "sub_main/0042D878.inc"
};
s32 gPlayerLookFiles_12_17[2] = {
#include "sub_main/0042D880.inc"
};
s32 gPlayerLookFiles_12_19[2] = {
#include "sub_main/0042D888.inc"
};
s32 gPlayerLookFiles_12_21[2] = {
#include "sub_main/0042D890.inc"
};
s32 gPlayerLookFiles_12_23[2] = {
#include "sub_main/0042D898.inc"
};
s32 gPlayerLookFiles_12_25[2] = {
#include "sub_main/0042D8A0.inc"
};
s32 gPlayerLookFiles_12_27[2] = {
#include "sub_main/0042D8A8.inc"
};
s32 gPlayerLookFiles_12_29[2] = {
#include "sub_main/0042D8B0.inc"
};
s32 gPlayerLookFiles_12_31[2] = {
#include "sub_main/0042D8B8.inc"
};
s32 gPlayerLookFiles_12_33[2] = {
#include "sub_main/0042D8C0.inc"
};
s32 gPlayerLookFiles_12_35[2] = {
#include "sub_main/0042D8C8.inc"
};
s32 gPlayerLookFiles_12_37[2] = {
#include "sub_main/0042D8D0.inc"
};
s32 gPlayerLookFiles_12_39[2] = {
#include "sub_main/0042D8D8.inc"
};
s32 gPlayerLookFiles_12_41[2] = {
#include "sub_main/0042D8E0.inc"
};
s32 gPlayerLookFiles_12_43[2] = {
#include "sub_main/0042D8E8.inc"
};
s32 gPlayerLookFiles_12_45[2] = {
#include "sub_main/0042D8F0.inc"
};
s32 gPlayerLookFiles_12_47[2] = {
#include "sub_main/0042D8F8.inc"
};
s32 gPlayerLookFiles_12_49[2] = {
#include "sub_main/0042D900.inc"
};
s32 gPlayerLookFiles_12_51[2] = {
#include "sub_main/0042D908.inc"
};
s32 gPlayerLookFiles_12_53[2] = {
#include "sub_main/0042D910.inc"
};
s32 gPlayerLookFiles_12_55[2] = {
#include "sub_main/0042D918.inc"
};
s32 gPlayerLookFiles_12_57[2] = {
#include "sub_main/0042D920.inc"
};
s32 gPlayerLookFiles_12_59[2] = {
#include "sub_main/0042D928.inc"
};
s32 gPlayerLookFiles_12_61[2] = {
#include "sub_main/0042D930.inc"
};
s32 gPlayerLookFiles_12_63[2] = {
#include "sub_main/0042D938.inc"
};
s32 gPlayerLookFiles_12_65[2] = {
#include "sub_main/0042D940.inc"
};
s32 gPlayerLookFiles_12_67[2] = {
#include "sub_main/0042D948.inc"
};
s32 gPlayerLookFiles_12_69[2] = {
#include "sub_main/0042D950.inc"
};
s32 gPlayerLookFiles_12_71[2] = {
#include "sub_main/0042D958.inc"
};
s32 gPlayerLookFiles_12_73[2] = {
#include "sub_main/0042D960.inc"
};
s32 gPlayerLookFiles_12_75[2] = {
#include "sub_main/0042D968.inc"
};
s32 gPlayerLookFiles_12_77[2] = {
#include "sub_main/0042D970.inc"
};
s32 gPlayerLookFiles_12_79[2] = {
#include "sub_main/0042D978.inc"
};
s32 gPlayerLookFiles_12_81[2] = {
#include "sub_main/0042D980.inc"
};
s32 gPlayerLookFiles_12_83[2] = {
#include "sub_main/0042D988.inc"
};
s32 gPlayerLookFiles_12_85[2] = {
#include "sub_main/0042D990.inc"
};
s32 gPlayerLookFiles_12_87[2] = {
#include "sub_main/0042D998.inc"
};
s32 gPlayerLookFiles_12_89[2] = {
#include "sub_main/0042D9A0.inc"
};
s32 gPlayerLookFiles_12_91[2] = {
#include "sub_main/0042D9A8.inc"
};
s32 gPlayerLookFiles_12_93[2] = {
#include "sub_main/0042D9B0.inc"
};
s32 gPlayerLookFiles_12_95[2] = {
#include "sub_main/0042D9B8.inc"
};
s32 gPlayerLookFiles_12_97[2] = {
#include "sub_main/0042D9C0.inc"
};
s32 gPlayerLookFiles_12_99[2] = {
#include "sub_main/0042D9C8.inc"
};
s32 gPlayerLookFiles_12_101[2] = {
#include "sub_main/0042D9D0.inc"
};
s32 gPlayerLookFiles_12_103[2] = {
#include "sub_main/0042D9D8.inc"
};
s32 gPlayerLookFiles_12_105[2] = {
#include "sub_main/0042D9E0.inc"
};
s32 gPlayerLookFiles_12_107[2] = {
#include "sub_main/0042D9E8.inc"
};
s32 gPlayerLookFiles_12_109[2] = {
#include "sub_main/0042D9F0.inc"
};
s32 gPlayerLookFiles_12_111[2] = {
#include "sub_main/0042D9F8.inc"
};
s32 gPlayerLookFiles_12_113[2] = {
#include "sub_main/0042DA00.inc"
};
s32 gPlayerLookFiles_12_115[2] = {
#include "sub_main/0042DA08.inc"
};
s32 gPlayerLookFiles_12_117[2] = {
#include "sub_main/0042DA10.inc"
};
s32 gPlayerLookFiles_12_119[2] = {
#include "sub_main/0042DA18.inc"
};
s32 gPlayerLookFiles_12_121[2] = {
#include "sub_main/0042DA20.inc"
};
s32 gPlayerLookFiles_12_123[2] = {
#include "sub_main/0042DA28.inc"
};
s32 gPlayerLookFiles_12_125[2] = {
#include "sub_main/0042DA30.inc"
};
s32 gPlayerLookFiles_12_127[2] = {
#include "sub_main/0042DA38.inc"
};
s32 gPlayerLookFiles_12_129[2] = {
#include "sub_main/0042DA40.inc"
};
s32 gPlayerLookFiles_12_131[2] = {
#include "sub_main/0042DA48.inc"
};
s32 gPlayerLookFiles_12_133[2] = {
#include "sub_main/0042DA50.inc"
};
s32 gPlayerLookFiles_12_135[2] = {
#include "sub_main/0042DA58.inc"
};
s32 gPlayerLookFiles_12_137[2] = {
#include "sub_main/0042DA60.inc"
};
s32 gPlayerLookFiles_12_139[2] = {
#include "sub_main/0042DA68.inc"
};
s32 gPlayerLookFiles_12_141[2] = {
#include "sub_main/0042DA70.inc"
};
s32 gPlayerLookFiles_12_143[2] = {
#include "sub_main/0042DA78.inc"
};
s32 gPlayerLookFiles_12_145[2] = {
#include "sub_main/0042DA80.inc"
};
s32 gPlayerLookFiles_12_147[2] = {
#include "sub_main/0042DA88.inc"
};
s32 gPlayerLookFiles_12_149[2] = {
#include "sub_main/0042DA90.inc"
};
s32 gPlayerLookFiles_12_151[2] = {
#include "sub_main/0042DA98.inc"
};
s32 gPlayerLookFiles_12_153[2] = {
#include "sub_main/0042DAA0.inc"
};
s32 gPlayerLookFiles_12_155[2] = {
#include "sub_main/0042DAA8.inc"
};
s32 gPlayerLookFiles_12_157[2] = {
#include "sub_main/0042DAB0.inc"
};
s32 gPlayerLookFiles_12_159[2] = {
#include "sub_main/0042DAB8.inc"
};
s32 gPlayerLookFiles_12_161[2] = {
#include "sub_main/0042DAC0.inc"
};
s32 gPlayerLookFiles_12_163[2] = {
#include "sub_main/0042DAC8.inc"
};
s32 gPlayerLookFiles_12_165[2] = {
#include "sub_main/0042DAD0.inc"
};
s32 gPlayerLookFiles_12_167[2] = {
#include "sub_main/0042DAD8.inc"
};
s32 gPlayerLookFiles_12_169[2] = {
#include "sub_main/0042DAE0.inc"
};
s32 gPlayerLookFiles_12_171[2] = {
#include "sub_main/0042DAE8.inc"
};
s32 gPlayerLookFiles_12_173[2] = {
#include "sub_main/0042DAF0.inc"
};
s32 gPlayerLookFiles_12_175[2] = {
#include "sub_main/0042DAF8.inc"
};
s32 gPlayerLookFiles_12_177[2] = {
#include "sub_main/0042DB00.inc"
};
s32 gPlayerLookFiles_12_179[2] = {
#include "sub_main/0042DB08.inc"
};
s32 gPlayerLookFiles_12_181[2] = {
#include "sub_main/0042DB10.inc"
};
s32 gPlayerLookFiles_12_183[2] = {
#include "sub_main/0042DB18.inc"
};
s32 gPlayerLookFiles_12_185[2] = {
#include "sub_main/0042DB20.inc"
};
s32 gPlayerLookFiles_12_187[2] = {
#include "sub_main/0042DB28.inc"
};
s32 gPlayerLookFiles_12_189[2] = {
#include "sub_main/0042DB30.inc"
};
s32 gPlayerLookFiles_12_191[2] = {
#include "sub_main/0042DB38.inc"
};
s32 gPlayerLookFiles_12_193[2] = {
#include "sub_main/0042DB40.inc"
};
s32 gPlayerLookFiles_12_195[2] = {
#include "sub_main/0042DB48.inc"
};
s32 gPlayerLookFiles_12_197[2] = {
#include "sub_main/0042DB50.inc"
};
s32 gPlayerLookFiles_12_199[2] = {
#include "sub_main/0042DB58.inc"
};
s32 gPlayerLookFiles_12_201[2] = {
#include "sub_main/0042DB60.inc"
};
s32 gPlayerLookFiles_12_203[2] = {
#include "sub_main/0042DB68.inc"
};
s32 gPlayerLookFiles_12_205[2] = {
#include "sub_main/0042DB70.inc"
};
s32 gPlayerLookFiles_12_207[2] = {
#include "sub_main/0042DB78.inc"
};
s32 gPlayerLookFiles_12_209[2] = {
#include "sub_main/0042DB80.inc"
};
s32 gPlayerLookFiles_12_211[2] = {
#include "sub_main/0042DB88.inc"
};
s32 gPlayerLookFiles_12_213[2] = {
#include "sub_main/0042DB90.inc"
};
s32 gPlayerLookFiles_12_215[2] = {
#include "sub_main/0042DB98.inc"
};
s32 gPlayerLookFiles_12_217[2] = {
#include "sub_main/0042DBA0.inc"
};
s32 gPlayerLookFiles_12_219[2] = {
#include "sub_main/0042DBA8.inc"
};
s32 gPlayerLookFiles_12_221[2] = {
#include "sub_main/0042DBB0.inc"
};
s32 gPlayerLookFiles_12_223[2] = {
#include "sub_main/0042DBB8.inc"
};
s32 gPlayerLookFiles_12_225[2] = {
#include "sub_main/0042DBC0.inc"
};
s32 gPlayerLookFiles_12_227[2] = {
#include "sub_main/0042DBC8.inc"
};
s32 gPlayerLookFiles_12_229[2] = {
#include "sub_main/0042DBD0.inc"
};
s32 gPlayerLookFiles_12_231[2] = {
#include "sub_main/0042DBD8.inc"
};
s32 gPlayerLookFiles_12_233[2] = {
#include "sub_main/0042DBE0.inc"
};
s32 gPlayerLookFiles_12_235[2] = {
#include "sub_main/0042DBE8.inc"
};
s32 gPlayerLookFiles_12_237[2] = {
#include "sub_main/0042DBF0.inc"
};
s32 gPlayerLookFiles_12_239[2] = {
#include "sub_main/0042DBF8.inc"
};
s32 gPlayerLookFiles_12_241[2] = {
#include "sub_main/0042DC00.inc"
};
s32 gPlayerLookFiles_12_243[2] = {
#include "sub_main/0042DC08.inc"
};
s32 gPlayerLookFiles_12_245[2] = {
#include "sub_main/0042DC10.inc"
};
s32 gPlayerLookFiles_12_247[2] = {
#include "sub_main/0042DC18.inc"
};
s32 gPlayerLookFiles_12_249[2] = {
#include "sub_main/0042DC20.inc"
};
s32 gPlayerLookFiles_12_251[2] = {
#include "sub_main/0042DC28.inc"
};
s32 gPlayerLookFiles_12_253[2] = {
#include "sub_main/0042DC30.inc"
};
s32 gPlayerLookFiles_12_255[2] = {
#include "sub_main/0042DC38.inc"
};
s32 gPlayerLookFiles_12_257[2] = {
#include "sub_main/0042DC40.inc"
};
s32 gPlayerLookFiles_12_259[2] = {
#include "sub_main/0042DC48.inc"
};
s32 gPlayerLookFiles_12_261[2] = {
#include "sub_main/0042DC50.inc"
};
s32 gPlayerLookFiles_12_263[2] = {
#include "sub_main/0042DC58.inc"
};
s32 gPlayerLookFiles_12_265[2] = {
#include "sub_main/0042DC60.inc"
};
s32 gPlayerLookFiles_12_267[2] = {
#include "sub_main/0042DC68.inc"
};
s32 gPlayerLookFiles_12_269[2] = {
#include "sub_main/0042DC70.inc"
};
s32 gPlayerLookFiles_12_271[2] = {
#include "sub_main/0042DC78.inc"
};
s32 gPlayerLookFiles_12_273[2] = {
#include "sub_main/0042DC80.inc"
};
s32 gPlayerLookFiles_12_275[2] = {
#include "sub_main/0042DC88.inc"
};
s32 gPlayerLookFiles_12_277[2] = {
#include "sub_main/0042DC90.inc"
};
s32 gPlayerLookFiles_12_279[2] = {
#include "sub_main/0042DC98.inc"
};
s32 gPlayerLookFiles_12_281[2] = {
#include "sub_main/0042DCA0.inc"
};
s32 gPlayerLookFiles_12_283[2] = {
#include "sub_main/0042DCA8.inc"
};
s32 gPlayerLookFiles_12_285[2] = {
#include "sub_main/0042DCB0.inc"
};
s32 gPlayerLookFiles_12_287[2] = {
#include "sub_main/0042DCB8.inc"
};
s32 gPlayerLookFiles_12_289[2] = {
#include "sub_main/0042DCC0.inc"
};
s32 gPlayerLookFiles_12_291[2] = {
#include "sub_main/0042DCC8.inc"
};
s32 gPlayerLookFiles_12_293[2] = {
#include "sub_main/0042DCD0.inc"
};
s32 gPlayerLookFiles_12_295[2] = {
#include "sub_main/0042DCD8.inc"
};
s32 gPlayerLookFiles_12_297[2] = {
#include "sub_main/0042DCE0.inc"
};
s32 gPlayerLookFiles_12_299[2] = {
#include "sub_main/0042DCE8.inc"
};
s32 gPlayerLookFiles_12_301[2] = {
#include "sub_main/0042DCF0.inc"
};
s32 gPlayerLookFiles_12_303[2] = {
#include "sub_main/0042DCF8.inc"
};
s32 gPlayerLookFiles_12_305[2] = {
#include "sub_main/0042DD00.inc"
};
s32 gPlayerLookFiles_12_307[2] = {
#include "sub_main/0042DD08.inc"
};
s32 gPlayerLookFiles_12_309[2] = {
#include "sub_main/0042DD10.inc"
};
s32 gPlayerLookFiles_12_311[2] = {
#include "sub_main/0042DD18.inc"
};
s32 gPlayerLookFiles_12_313[2] = {
#include "sub_main/0042DD20.inc"
};
s32 gPlayerLookFiles_12_315[2] = {
#include "sub_main/0042DD28.inc"
};
s32 gPlayerLookFiles_12_317[2] = {
#include "sub_main/0042DD30.inc"
};
s32 gPlayerLookFiles_12_319[2] = {
#include "sub_main/0042DD38.inc"
};
s32 gPlayerLookFiles_12_321[2] = {
#include "sub_main/0042DD40.inc"
};
s32 gPlayerLookFiles_12_323[2] = {
#include "sub_main/0042DD48.inc"
};
s32 gPlayerLookFiles_12_325[2] = {
#include "sub_main/0042DD50.inc"
};
s32 gPlayerLookFiles_12_327[2] = {
#include "sub_main/0042DD58.inc"
};
s32 gPlayerLookFiles_12_329[2] = {
#include "sub_main/0042DD60.inc"
};
s32 gPlayerLookFiles_12_331[2] = {
#include "sub_main/0042DD68.inc"
};
s32 gPlayerLookFiles_12_333[2] = {
#include "sub_main/0042DD70.inc"
};
s32 gPlayerLookFiles_12_335[2] = {
#include "sub_main/0042DD78.inc"
};
s32 gPlayerLookFiles_12_337[2] = {
#include "sub_main/0042DD80.inc"
};
s32 gPlayerLookFiles_12_339[2] = {
#include "sub_main/0042DD88.inc"
};
s32 gPlayerLookFiles_12_341[2] = {
#include "sub_main/0042DD90.inc"
};
s32 gPlayerLookFiles_12_343[2] = {
#include "sub_main/0042DD98.inc"
};
s32 gPlayerLookFiles_12_345[2] = {
#include "sub_main/0042DDA0.inc"
};
s32 gPlayerLookFiles_12_347[2] = {
#include "sub_main/0042DDA8.inc"
};
s32 gPlayerLookFiles_12_349[2] = {
#include "sub_main/0042DDB0.inc"
};
s32 gPlayerLookFiles_12_351[2] = {
#include "sub_main/0042DDB8.inc"
};
s32 gPlayerLookFiles_12_353[2] = {
#include "sub_main/0042DDC0.inc"
};
s32 gPlayerLookFiles_12_355[2] = {
#include "sub_main/0042DDC8.inc"
};
s32 gPlayerLookFiles_12_357[2] = {
#include "sub_main/0042DDD0.inc"
};
s32 gPlayerLookFiles_12_359[2] = {
#include "sub_main/0042DDD8.inc"
};
s32 gPlayerLookFiles_12_361[2] = {
#include "sub_main/0042DDE0.inc"
};
s32 gPlayerLookFiles_12_363[2] = {
#include "sub_main/0042DDE8.inc"
};
s32 gPlayerLookFiles_12_365[2] = {
#include "sub_main/0042DDF0.inc"
};
s32 gPlayerLookFiles_12_367[2] = {
#include "sub_main/0042DDF8.inc"
};
s32 gPlayerLookFiles_12_369[2] = {
#include "sub_main/0042DE00.inc"
};
s32 gPlayerLookFiles_12_371[2] = {
#include "sub_main/0042DE08.inc"
};
s32 gPlayerLookFiles_12_373[2] = {
#include "sub_main/0042DE10.inc"
};
s32 gPlayerLookFiles_12_375[2] = {
#include "sub_main/0042DE18.inc"
};
s32 gPlayerLookFiles_12_377[2] = {
#include "sub_main/0042DE20.inc"
};
s32 gPlayerLookFiles_12_379[2] = {
#include "sub_main/0042DE28.inc"
};
s32 gPlayerLookFiles_12_381[2] = {
#include "sub_main/0042DE30.inc"
};
s32 gPlayerLookFiles_12_383[2] = {
#include "sub_main/0042DE38.inc"
};
s32 gPlayerLookFiles_12_385[2] = {
#include "sub_main/0042DE40.inc"
};
s32 gPlayerLookFiles_12_387[2] = {
#include "sub_main/0042DE48.inc"
};
s32 gPlayerLookFiles_12_389[2] = {
#include "sub_main/0042DE50.inc"
};
s32 gPlayerLookFiles_12_391[2] = {
#include "sub_main/0042DE58.inc"
};
s32 gPlayerLookFiles_12_393[2] = {
#include "sub_main/0042DE60.inc"
};
s32 gPlayerLookFiles_12_395[2] = {
#include "sub_main/0042DE68.inc"
};
s32 gPlayerLookFiles_12_397[2] = {
#include "sub_main/0042DE70.inc"
};
s32 gPlayerLookFiles_12_399[2] = {
#include "sub_main/0042DE78.inc"
};
s32 gPlayerLookFiles_12_401[2] = {
#include "sub_main/0042DE80.inc"
};
s32 gPlayerLookFiles_12_403[2] = {
#include "sub_main/0042DE88.inc"
};
s32 gPlayerLookFiles_12_405[2] = {
#include "sub_main/0042DE90.inc"
};
s32 gPlayerLookFiles_12_407[2] = {
#include "sub_main/0042DE98.inc"
};
s32 gPlayerLookFiles_12_409[2] = {
#include "sub_main/0042DEA0.inc"
};
s32 gPlayerLookFiles_12_411[2] = {
#include "sub_main/0042DEA8.inc"
};
s32 gPlayerLookFiles_12_413[4] = {
#include "sub_main/0042DEB0.inc"
};
TexPackHead gPlayerLookFiles_12[207] = {
#include "sub_main/0042DEC0.inc"
};
s32 gPlayerLookFiles_13_1[2] = {
#include "sub_main/0042E538.inc"
};
s32 gPlayerLookFiles_13_3[2] = {
#include "sub_main/0042E540.inc"
};
s32 gPlayerLookFiles_13_5[2] = {
#include "sub_main/0042E548.inc"
};
s32 gPlayerLookFiles_13_7[2] = {
#include "sub_main/0042E550.inc"
};
s32 gPlayerLookFiles_13_9[2] = {
#include "sub_main/0042E558.inc"
};
s32 gPlayerLookFiles_13_11[2] = {
#include "sub_main/0042E560.inc"
};
s32 gPlayerLookFiles_13_13[2] = {
#include "sub_main/0042E568.inc"
};
s32 gPlayerLookFiles_13_15[2] = {
#include "sub_main/0042E570.inc"
};
s32 gPlayerLookFiles_13_17[2] = {
#include "sub_main/0042E578.inc"
};
s32 gPlayerLookFiles_13_19[2] = {
#include "sub_main/0042E580.inc"
};
s32 gPlayerLookFiles_13_21[2] = {
#include "sub_main/0042E588.inc"
};
s32 gPlayerLookFiles_13_23[2] = {
#include "sub_main/0042E590.inc"
};
s32 gPlayerLookFiles_13_25[2] = {
#include "sub_main/0042E598.inc"
};
s32 gPlayerLookFiles_13_27[2] = {
#include "sub_main/0042E5A0.inc"
};
s32 gPlayerLookFiles_13_29[2] = {
#include "sub_main/0042E5A8.inc"
};
s32 gPlayerLookFiles_13_31[2] = {
#include "sub_main/0042E5B0.inc"
};
s32 gPlayerLookFiles_13_33[2] = {
#include "sub_main/0042E5B8.inc"
};
s32 gPlayerLookFiles_13_35[2] = {
#include "sub_main/0042E5C0.inc"
};
s32 gPlayerLookFiles_13_37[2] = {
#include "sub_main/0042E5C8.inc"
};
s32 gPlayerLookFiles_13_39[2] = {
#include "sub_main/0042E5D0.inc"
};
s32 gPlayerLookFiles_13_41[2] = {
#include "sub_main/0042E5D8.inc"
};
TexPackHead gPlayerLookFiles_13[22] = {
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
s32 gAreaSubModelTexFiles[198] = {
#include "sub_main/0042EE40.inc"
};
u32 gOnlineVillageSubModelTexFiles[2] = {
#include "sub_main/0042F158.inc"
};
u32 gOnlineVillageSubModelTexFileSeason1[4] = {
#include "sub_main/0042F160.inc"
};
u32 gArea89SubModelTexFiles[12] = {
#include "sub_main/0042F170.inc"
};
