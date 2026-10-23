/*
 * Loading the game's data files by id.
 *
 * LoadFile and LoadCompressedFile read a file of the data archive; the
 * rest of the file loads through them, into gLoadBuf:
 * - the fixed texture slots' files (Tex_LoadSystemPack, Tex_LoadFile,
 *   Tex_LoadStageFile) and the quest end's two pictures, made into
 *   textures from their TIM2 images;
 * - the animation sets of the player, the monsters and the scene
 *   (Load_*Anim), which the animation code then sets up from gLoadBuf;
 * - the model packs of the areas, the monsters, the weapons and the
 *   armour (Load_*ModelPack): a pack's part 0, the model file, and part
 *   1, its skeleton, are copied to the model load buffer for the model
 *   set-up.
 * The tables of file ids by kind, area and gender that these read end the
 * file.
 *
 * The game compiled this code at optimization level 3 (Tex_LoadSystemPack
 * matches only so) and without lifetime analysis: without it, the compiler
 * keeps Tex_LoadFile's walk of the pack's entries in one pointer register
 * stepped by 8, as the game does; every other function compiles the same
 * either way. The texture packs' code before it is compiled at level 4,
 * which is why they are separate files.
 */
#include "common.h"
#include "syssettings.h"
#include "scene.h"
#include "area.h"
#include "image.h"
#include "model.h"
#include "texture.h"
#include "bgm.h"
#include "place_rules.h"
#include "dma.h"
#include "quest.h"
#include "village_cond.h"
#include "file_load.h"
#include "lobby_world.h"
#include "main.h"
#include "packet_tim2.h"
#include "packs.h"
#include "libkernl.h"

#pragma opt_lifetimes off

/* The data files' AFS partition, in a file id's high half (LoadFile,
 * LoadCompressedFile). */
#define AFS_DATA_FILES 0x30000

/* The quest end's file: two TIM2 pictures of 256 x 256 8-bit pixels, each
 * with a 256-colour palette after its pixels. */
#define QUEST_END_FILE 40
#define QUEST_END_PIXELS (sizeof(Tim2FileHeader) + 48)     /* past the file's and the picture's headers */
#define QUEST_END_CLUT (QUEST_END_PIXELS + 256 * 256)

extern u8 *gLoadBuf;
extern u8 *gReadBuf;

extern s32 gTexHandles[];
extern s32 gSystemTexFiles;
extern s32 gSystemExtraTexFiles;
extern s8 gSystemPackTexSlots[];
extern s8 gSystemPackTexSlotsB[];

void Mem_Copy64ByQwords(u8 *src, s32 dst, s32 n);

/* A pack file's entry, after its 8-byte head: a part's offset from the
 * file's start and its size. */
typedef struct PackEntry {
    s32 off;
    s32 size;
} PackEntry;

u8 Village_FacilityLevel(s32 k);

extern s16 gTexFileIds[];
extern s16 gStageTexFileIds[];
extern u8 gMapPackTexIndex[];
extern s32 gAreaPackFiles[];
extern s32 gOnlineVillagePackFiles[2];
extern s32 gOnlineVillageSubModelFiles[2];
extern s32 gOnlineVillageSubModelFileSeason1;
extern s32 gArea84PackFiles[2];
extern s32 gFarmFacilityModelFiles[];

extern s32 gPlayerAnimFiles[];
extern s32 gCommonAnimFiles[];
extern s32 gEmAnimFiles[];
extern s32 gCommonModelPackFiles[];
extern s32 gEmMainModelPackFiles[];
extern s32 gEmSlotModelPackFiles[];
extern s32 gEquipModelPackFilesMale[];
extern s32 gEquipModelPackFilesFemale[];

/*
 * Loads the system texture pack (gSystemTexFiles[0]) and makes a texture of
 * each of its pictures, in the fixed slot gSystemPackTexSlots gives it.
 * mode is a SYSTEM_TEX_* set: the lobby's leaves out slots 2, 4 and 5, the
 * character select's 10 and 11 as well. The full set also loads slot 3
 * from a file of its own (gSystemExtraTexFiles).
 */
void Tex_LoadSystemPack(s32 mode)
{
    u8 *buf = gLoadBuf;
    s32 i;
    s32 count;

    LoadCompressedFile(buf, gSystemTexFiles);
    count = Pack_Count((s32 *) buf);
    for (i = 0; i < count; i++) {
        s32 slot = gSystemPackTexSlots[i];

        if (mode != SYSTEM_TEX_ALL) {
            if (slot == 2 || slot == 4 || slot == 5) {
                continue;
            }
            if (mode != SYSTEM_TEX_LOBBY) {
                if (slot == 10 || slot == 11) {
                    continue;
                }
            }
        }
        gTexHandles[slot] = Tex_CreateFromTim2(Pack_GetPart(buf, i), gTexCreateKinds[0]);
    }
    if (mode == SYSTEM_TEX_ALL) {
        LoadCompressedFile(buf, gSystemExtraTexFiles);
        gTexHandles[3] = Tex_CreateFromTim2(Pack_GetPart(buf, 0), gTexCreateKinds[0]);
    }
}

/*
 * Loads the system texture pack again for slots 8 and 11 only, by
 * gSystemPackTexSlotsB (the ending calls it).
 */
void Tex_LoadSystemPackB(void)
{
    u8 *buf = gLoadBuf;
    s32 i;
    s32 count;

    LoadCompressedFile(buf, gSystemTexFiles);
    count = Pack_Count((s32 *) buf);
    for (i = 0; i < count; i++) {
        s32 slot = gSystemPackTexSlotsB[i];

        if (slot == 8 || slot == 11) {
            gTexHandles[slot] = Tex_CreateFromTim2(Pack_GetPart(buf, i), gTexCreateKinds[0]);
        }
    }
}

/*
 * Loads the quest end's file into gLoadBuf, ahead of the quest's end, and
 * prepares its two pictures' palettes for the GS: each colour's alpha is
 * halved, and 255 becomes 128. Does nothing while debug switch 156 is on
 * (never in the retail game).
 */
void Tex_LoadQuestEndFile(void)
{
    u8 *entry = gLoadBuf;
    s32 j;
    s32 i;
    u32 *clut;
    u32 alpha;
    u32 color;

    if (Debug_IsOn(156) == 0) {
        LoadCompressedFile(entry, QUEST_END_FILE);
        FlushCache(0);
        Dma_WaitIdle();
        entry += 8;
        for (j = 0; j < 2; j++) {
            clut = (u32 *)((s32)gLoadBuf + *(s32 *)entry + QUEST_END_CLUT);
            for (i = 0; i < 256; i++) {
                color = *clut;
                alpha = color & 0xFF000000;
                if (alpha == 0xFF000000) {
                    alpha = 0x80000000;
                } else if (alpha != 0) {
                    alpha = (alpha >> 1) & 0xFF000000;
                }
                *clut = (color & 0xFFFFFF) | alpha;
                clut++;
            }
            entry += 8;
        }
    }
}

/*
 * Copies the quest end's two pictures, which Tex_LoadQuestEndFile left in
 * gLoadBuf, into the textures of slot TEX_SLOT_AREA_MAP and slot 326,
 * pixels and palette, and frees the GS memory so that they are uploaded
 * again. A slot's handle holds the texture's handle in its low half and its
 * palette's in the high half. Does nothing while debug switch 156 is on.
 */
void Tex_SetQuestEndImages(void)
{
    u8 *entry = gLoadBuf;
    s32 picture;
    s32 i;
    s32 texHandle;
    s32 palHandle;
    s32 handles;
    Image info;

    if (Debug_IsOn(156) == 0) {
        entry += 8;
        for (i = 0; i < 2; i++) {
            picture = (s32)gLoadBuf + *(s32 *)entry;
            handles = gTexHandles[i == 0 ? TEX_SLOT_AREA_MAP : 326];
            texHandle = handles & 0xFFFF;
            palHandle = (u32)(handles & 0xFFFF0000) >> 16;
            flLockTexture(0, texHandle, &info, 2);
            Mem_Copy64ByQwords((u8 *)(picture + QUEST_END_PIXELS), (s32)info.pixels, 256 * 256 / 64);
            flUnlockTexture(texHandle);
            flLockPalette(0, palHandle, &info, 2);
            Mem_Copy64ByQwords((u8 *)(picture + QUEST_END_CLUT), (s32)info.pixels, 256 * 4 / 64);
            flUnlockPalette(palHandle);
            entry += 8;
        }
        VramSlot_FreeAll();
    }
}

/*
 * Loads texture file file (gTexFileIds) into texture slot slot, made as
 * gTexCreateKinds[kind]. The slots 0, 328, TEX_SLOT_ICONS and
 * TEX_SLOT_AREA_MAP are loaded once and kept. The area map's file is a pack
 * of maps: its picture is gMapPackTexIndex's by gScene.map.
 */
void Tex_LoadFile(s32 file, s32 slot, s32 kind)
{
    u8 *buf = gLoadBuf;
    s32 off;
    PackEntry *entry;
    s32 i;
    s32 picture;

    switch (slot) {
    case 0:
    case 328:
    case TEX_SLOT_ICONS:
    case TEX_SLOT_AREA_MAP:
        if (gTexHandles[slot] != 0) {
            return;
        }
    }
    LoadCompressedFile(buf, gTexFileIds[file]);
    if (slot == TEX_SLOT_AREA_MAP) {
        picture = 0;
        if (gScene.map < MAP_ARENA_NIGHT + 1) {
            picture = gMapPackTexIndex[gScene.map];
        }
        entry = (PackEntry *)(buf + 8);
        for (i = 0; i < picture; i++) {
            off = entry->off;
            /* stepped by its bytes: entry++ makes the compiler unroll the
             * walk differently from the game */
            entry = (PackEntry *)((u8 *)entry + 8);
        }
        off += (s32)buf;
        gTexHandles[slot] = Tex_CreateFromTim2((void *)off, gTexCreateKinds[0]);
    } else {
        gTexHandles[slot] = Tex_CreateFromTim2(buf, gTexCreateKinds[kind]);
    }
}

/*
 * Loads the stage's texture file for map map (gStageTexFileIds) into slot
 * slot, made as gTexCreateKinds[kind]. The great arena's maps use file 16
 * in a versus quest; on other maps, quests 1 and 2 use file 11, and quests
 * 3 to 5 file 12 in the jungle and 26 elsewhere. The quest is read from
 * gScene, not from unusedQuestId.
 */
void Tex_LoadStageFile(s32 map, s32 unusedQuestId, s32 slot, s32 kind)
{
    s32 id;
    u8 *buf;

    buf = gLoadBuf;
    id = gStageTexFileIds[map];
    if (map == MAP_GREAT_ARENA || map == MAP_GREAT_ARENA_NIGHT) {
        if (Quest_IsVersus() == 1) {
            id = 16;
        }
    } else {
        switch (gScene.questId) {
        case 1:
        case 2:
            id = 11;
            break;
        case 3:
        case 4:
        case 5:
            id = (map == MAP_JUNGLE) ? 12 : 26;
            break;
        }
    }
    LoadCompressedFile(buf, id);
    gTexHandles[slot] = Tex_CreateFromTim2(buf, gTexCreateKinds[kind]);
}

/*
 * Reads data file id (compressed) into gReadBuf and decompresses it to dst.
 * Returns the decompressed size, 0 when the read failed, and -1 for a
 * negative id.
 */
s32 LoadCompressedFile(u8 *dst, s32 id)
{
    s32 size;

    if (id < 0) {
        return -1;
    }
    size = Afs_ReadId(id | AFS_DATA_FILES, gReadBuf);
    if (size != 0) {
        size = LzDecompress(gReadBuf, dst);
    }
    return size;
}

/*
 * Reads data file id to dst as it is. Returns the size read, or -1 for a
 * negative id.
 */
s32 LoadFile(u8 *dst, s32 id)
{
    if (id < 0) {
        return -1;
    }
    return Afs_ReadId(id | AFS_DATA_FILES, dst);
}

/* Loads the player's animation set for weapon type weaponType into gLoadBuf. */
void Load_PlayerAnim(s32 unusedIdx, s32 weaponType)
{
    LoadCompressedFile(gLoadBuf, gPlayerAnimFiles[weaponType]);
}

/* Loads common animation set set into gLoadBuf (not compressed). */
void Load_CommonAnim(s32 set)
{
    LoadFile(gLoadBuf, gCommonAnimFiles[set]);
}

/* Loads the animation set of monster kind kind into gLoadBuf. */
void Load_EmAnim(s32 unusedIdx, s32 kind)
{
    LoadCompressedFile(gLoadBuf, gEmAnimFiles[kind]);
}

/*
 * Loads animation set set of a monster slot's extra animations into
 * gLoadBuf; slots without such sets (a NULL in gEmSlotAnimFiles) load
 * nothing.
 */
void Load_EmSlotAnim(s32 slot, s32 set)
{
    s32 *files = gEmSlotAnimFiles[slot];
    u8 *buf = gLoadBuf;

    if (files != NULL) {
        LoadCompressedFile(buf, files[set]);
    }
}

/*
 * Loads area area's model pack and sets it up: its model file and skeleton
 * are copied to the model load buffer and its part 2 read. AREA_ONLINE_VILLAGE and
 * AREA_HUB_0x84 have a second pack, used in the lobby while
 * ElderRaid_IsOn.
 */
void Load_AreaPack(s32 area)
{
    u8 *base;
    u8 *end;
    s32 file;

    end = (base = (*(u8 * *)&gSysSettings.modelPack)) + MODEL_LOAD_SKEL_OFFSET;
    switch (area) {
    case AREA_ONLINE_VILLAGE:
        if (gScene.inLobby != 0 && ElderRaid_IsOn() == 1) {
            file = gOnlineVillagePackFiles[1];
        } else {
            file = gOnlineVillagePackFiles[0];
        }
        break;
    case AREA_HUB_0x84:
        if (gScene.inLobby != 0 && ElderRaid_IsOn() == 1) {
            file = gArea84PackFiles[1];
        } else {
            file = gArea84PackFiles[0];
        }
        break;
    default:
        file = gAreaPackFiles[area];
        break;
    }
    LoadCompressedFile(gLoadBuf, file);
    Pack_CopyParts01(gLoadBuf, base, end);
    AreaPack_ReadPart2((s32 *) gLoadBuf);
}

/*
 * Loads common model pack index and copies its model file and skeleton to
 * the model load buffer.
 */
void Load_CommonModelPack(s32 index)
{
    u8 *base = (*(u8 * *)&gSysSettings.modelPack);

    LoadCompressedFile(gLoadBuf, gCommonModelPackFiles[index]);
    Pack_CopyParts01(gLoadBuf, base, base + MODEL_LOAD_SKEL_OFFSET);
}

/*
 * Loads weapon weapon's model pack and copies its model file and skeleton
 * to the model load buffer.
 */
void Load_WeaponModelPack(s32 weapon)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gWeaponModelPackFiles[weapon]);
    Pack_CopyParts01(gLoadBuf, base, base + MODEL_LOAD_SKEL_OFFSET);
}

/*
 * Loads the model pack of armour part id of type type, for a female hunter
 * or not, and copies its model file and skeleton to the model load buffer.
 */
void Load_PartModelPack(s32 id, s32 type, s32 female)
{
    s32 *files;
    u8 *base;

    if (female == 0) {
        files = gPartModelFilesMale[type];
    } else {
        files = gPartModelFilesFemale[type];
    }
    base = (*(u8 * *)&gSysSettings.dataBuf);
    files += id;
    LoadCompressedFile(gLoadBuf, *files);
    Pack_CopyParts01(gLoadBuf, base, base + MODEL_LOAD_SKEL_OFFSET);
}

/*
 * Loads monster kind kind's model pack and copies its model file and
 * skeleton to the model load buffer.
 */
void Load_EmModelPack(s32 kind)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gEmMainModelPackFiles[kind]);
    Pack_CopyParts01(gLoadBuf, base, base + MODEL_LOAD_SKEL_OFFSET);
}

/*
 * Loads monster kind kind's second model pack, its part model, and copies
 * its model file and skeleton to the model load buffer.
 */
void Load_EmPartModelPack(s32 kind)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gEmPartModelPackFiles[kind]);
    Pack_CopyParts01(gLoadBuf, base, base + MODEL_LOAD_SKEL_OFFSET);
}

/*
 * Loads the model pack of a monster slot's kind kind and copies its model
 * file and skeleton to the model load buffer.
 */
void Load_EmSlotModelPack(s32 kind)
{
    u8 *base = (*(u8 * *)&gSysSettings.dataBuf);

    LoadCompressedFile(gLoadBuf, gEmSlotModelPackFiles[kind]);
    Pack_CopyParts01(gLoadBuf, base, base + MODEL_LOAD_SKEL_OFFSET);
}

/*
 * Loads equipment id's model pack, for a female hunter or not, and copies
 * its model file and skeleton to the model load buffer.
 */
void Load_EquipModelPack(s32 id, s32 female)
{
    u8 *base;
    u8 *end;

    end = (base = (*(u8 * *)&gSysSettings.dataBuf)) + MODEL_LOAD_SKEL_OFFSET;
    if (female == 0) {
        LoadCompressedFile(gLoadBuf, gEquipModelPackFilesMale[id]);
    } else {
        LoadCompressedFile(gLoadBuf, gEquipModelPackFilesFemale[id]);
    }
    Pack_CopyParts01(gLoadBuf, base, end);
}

/*
 * Loads one of area area's extra model packs and copies its model file and
 * skeleton to the model load buffer. sub 0 is the area's own: in the lobby,
 * AREA_ONLINE_VILLAGE's follows the season (LobbyWorld_GetSeason). sub 1 and on are
 * AREA_HUB_0x89's facilities, by facility and level; other areas have none.
 */
void Load_AreaSubModelPack(s32 area, s32 sub)
{
    s32 file;
    u8 *base;

    if (sub == 0) {
        switch (area) {
        case AREA_ONLINE_VILLAGE:
            if (gScene.inLobby != 0) {
                switch (LobbyWorld_GetSeason()) {
                case 2:
                default:
                    file = gOnlineVillageSubModelFiles[0];
                    break;
                case 0:
                    file = gOnlineVillageSubModelFiles[1];
                    break;
                case 1:
                    file = gOnlineVillageSubModelFileSeason1;
                    break;
                }
            } else {
                file = gOnlineVillageSubModelFiles[0];
            }
            break;
        default:
            file = gAreaSubModelFiles[area];
            break;
        }
    } else {
        switch (area) {
        case AREA_HUB_0x89:
            file = gFarmFacilityModelFiles[(sub - 1) * 3 + (u8)Village_FacilityLevel(sub)];
            break;
        default:
            return;
        }
    }
    base = (*(u8 * *)&gSysSettings.modelPack);
    LoadCompressedFile(gLoadBuf, file);
    Pack_CopyParts01(gLoadBuf, base, base + MODEL_LOAD_SKEL_OFFSET);
}

/* The file ids the loaders read, by kind, area, part and gender. */
s32 gPlayerAnimFiles[12] = {
#include "sub_main/0041ED50.inc"
};
s32 gCommonAnimFiles[8] = {
#include "sub_main/0041ED80.inc"
};
s32 gPartModelFilesMale_0[200] = {
#include "sub_main/0041EDA0.inc"
};
s32 gPartModelFilesMale_1[28] = {
#include "sub_main/0041F0C0.inc"
};
s32 gPartModelFilesMale_2[220] = {
#include "sub_main/0041F130.inc"
};
s32 gPartModelFilesMale_3[236] = {
#include "sub_main/0041F4A0.inc"
};
s32 gPartModelFilesMale_4[228] = {
#include "sub_main/0041F850.inc"
};
s32 gPartModelFilesMale_5[208] = {
#include "sub_main/0041FBE0.inc"
};
s32 gPartModelFilesMale_6[24] = {
#include "sub_main/0041FF20.inc"
};
s32 *gPartModelFilesMale[8] = {
    gPartModelFilesMale_0, gPartModelFilesMale_1, gPartModelFilesMale_2, gPartModelFilesMale_3,
    gPartModelFilesMale_4, gPartModelFilesMale_5, gPartModelFilesMale_6, NULL,
};
s32 gPartModelFilesFemale_0[200] = {
#include "sub_main/0041FFA0.inc"
};
s32 gPartModelFilesFemale_1[28] = {
#include "sub_main/004202C0.inc"
};
s32 gPartModelFilesFemale_2[220] = {
#include "sub_main/00420330.inc"
};
s32 gPartModelFilesFemale_3[236] = {
#include "sub_main/004206A0.inc"
};
s32 gPartModelFilesFemale_4[228] = {
#include "sub_main/00420A50.inc"
};
s32 gPartModelFilesFemale_5[208] = {
#include "sub_main/00420DE0.inc"
};
s32 gPartModelFilesFemale_6[24] = {
#include "sub_main/00421120.inc"
};
s32 *gPartModelFilesFemale[8] = {
    gPartModelFilesFemale_0, gPartModelFilesFemale_1, gPartModelFilesFemale_2, gPartModelFilesFemale_3,
    gPartModelFilesFemale_4, gPartModelFilesFemale_5, gPartModelFilesFemale_6, NULL,
};
s32 gWeaponModelPackFiles[340] = {
#include "sub_main/004211A0.inc"
};
s32 gEmMainModelPackFiles[76] = {
#include "sub_main/004216F0.inc"
};
s32 gEmAnimFiles[76] = {
#include "sub_main/00421820.inc"
};
s32 gEmPartModelPackFiles[76] = {
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
FilePair gStageFilePairs[2] = {
#include "sub_main/004224B0.inc"
};
s32 gCommonModelPackFiles[8] = {
#include "sub_main/004224C0.inc"
};
