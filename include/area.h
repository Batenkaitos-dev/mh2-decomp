#ifndef AREA_H
#define AREA_H

/*
 * The state of the current area: gAreaState (sub_main 0x5EC460, 0x120 bytes). Area_ClearState
 * clears it, Area_Enter and Area_Load set it up, the area pack's part 2 fills its fog, lights
 * and sky (AreaPack_ReadPart2), Area_UpdateTimers runs it each frame (sub_main unk_379EC0.c,
 * unk_37AC80.c), and the area's sky and props draw from it (unk_37B570.c).
 */

#include "common.h"

/* Fog parameters (Area_ApplyFog: render states 0xF, 0x10, 0x11; gFogDefault, gFogArea56, the
 * area pack's part 0: the colour first, then the two distances). */
typedef struct FogParam {
    f32 nearDist;               /* 0x0: render state 0x10 (gGsFogNear) */
    f32 farDist;                /* 0x4: render state 0x11 (gGsFogFar) */
    u8 r;                       /* 0x8 */
    u8 g;                       /* 0x9 */
    u8 b;                       /* 0xA */
    u8 a;                       /* 0xB */
} FogParam;
typedef char FogParam_size_check[(sizeof(FogParam) == 0xC) ? 1 : -1];

/* A sky band: its rectangle and its corner colours for four headings (Sky_DrawBand). */
typedef struct SkyBand {
    s16 y;                      /* 0x00 */
    s16 h;                      /* 0x02 */
    u32 c[8];                   /* 0x04 */
} SkyBand;
typedef char SkyBand_size_check[(sizeof(SkyBand) == 0x24) ? 1 : -1];

typedef struct AreaState {
    u8 active;                  /* 0x000: 1 once set up (Area_Enter); the sky is drawn only then */
    u8 drawSky;                 /* 0x001: the sky and the glow props are drawn (AreaSky_Draw, Sky_Draw,
                                 * Area_DrawGlowProps): set once the field is up (Scene_StartField) */
    u8 unk002[0x004 - 0x002];
    u8 timerState;              /* 0x004: Area_UpdateTimers: 0 starts (Area_StartTimers), 1 runs
                                 * (Area_Update) */
    u8 unk005;                  /* 0x005 */
    u8 unk006;                  /* 0x006 */
    u8 unk007;                  /* 0x007 */
    s16 frame;                  /* 0x008: the frame counter (+1 per frame): the area animations' time */
    s16 spin;                   /* 0x00A: +0x2E per frame (AreaAnim_SpinScroll's angle) */
    u8 unk00C[0x010 - 0x00C];
    f32 pos[3];                 /* 0x010: the area model's position (AreaSky_Draw) */
    f32 unk01C[3];              /* 0x01C */
    f32 rot[3];                 /* 0x028: and its rotation */
    s16 modelCount;             /* 0x034: the area's model slots (Area_FreeModels): how many */
    u8 unk036[0x038 - 0x036];
    s16 modelSlot;              /* 0x038: the first one (Area_Load) */
    u8 unk03A[0x03C - 0x03A];
    struct Model *model;        /* 0x03C: the area model (Model_Get(modelSlot)) */
    u8 unk040[0x048 - 0x040];
    struct AreaRecord *record;  /* 0x048: the area's place in the map (Stage_GetAreaRecord) */
    u8 unk04C;                  /* 0x04C */
    u8 envBlocks;               /* 0x04D: the blocks the area pack's part 2 gave (AreaPack_ReadPart2):
                                 * 0 none, 1 the fog and the lights, 2 also the sky (skyPos, band);
                                 * below 2 Area_StartObjects sets the sky's defaults */
    u16 area;                   /* 0x04E: the area id */
    FogParam fog;               /* 0x050: the area's own fog (Area_ApplyFog) */
    f32 lightDir[3][3];         /* 0x05C: the area's own three lights (Light_SetDefault): directions */
    f32 lightAmb[3][3];         /* 0x080: their ambient colours */
    f32 lightCol[3][3];         /* 0x0A4: their colours */
    f32 skyPos[3];              /* 0x0C8: a far point (5000.0 each by default): Sky_DrawBand turns the
                                 * band's colours by the camera's heading to it, AmbientSound_Draw
                                 * fades by the camera's angle to it, AmbientSound_Update places
                                 * its sound there, unk_3BE060.c turns a player's part to it */
    SkyBand band;               /* 0x0D4: the sky band (Sky_Draw) */
    u8 look[4];                 /* 0x0F8: [0] the night level (0 day .. 0xFF night: online it follows
                                 * LobbyTime_IsAm, 0xC a frame; darkens the props and notices);
                                 * [1] the area's variant (the season of AREA_HUB_0x83, the house level
                                 * of 0xAD-0xAF; Area_DrawProp's props of one variant);
                                 * [1..3] the facility levels of AREA_HUB_0x89 (Area_Enter) */
    u8 monsterState;            /* 0x0FC: cached result of Area_MonsterState (0-2) */
    u8 flyer;                   /* 0x0FD: set while a kind 0x1D object exists (StageObj_AreaFlyer_Create) */
    u16 propTimer;              /* 0x0FE: the frames of the moving props' state (AREA_HUB_0x89, Area_Update) */
    f32 propOfs[3];             /* 0x100: the moving props' offset (Area_DrawProp's kind 13) */
    f32 flyerPos[3];            /* 0x10C: the kind 0x1D object's position (StageObj_AreaFlyer_Run) */
    u8 propState;               /* 0x118: the moving props' state (func_7BC280: 1 out, 2 back) */
    u8 propHold;                /* 0x119: frames state 1 is held after func_7BC280 left it */
    u8 unk11A[0x120 - 0x11A];
} AreaState;
typedef char AreaState_size_check[(sizeof(AreaState) == 0x120) ? 1 : -1];

extern AreaState gAreaState;

/*
 * The maps: gScene.map and QuestInfo.map, the index of gMapAreas,
 * gMapAreaCount and gStartAreaTables, and of the maps' names table
 * (gStageNames, sub_main 0x446E80). Each is named from its entry there,
 * described rather than quoted, and the area temperatures agree
 * (gAreaTemperatures: the volcano's areas are hot or very hot, the snowy
 * mountains' cold or very cold, the desert's hot by day and cold at night,
 * the swamp's caves cold). Maps 16 to 25 are the night versions of the day
 * maps gMapDayIds gives. The three tower maps share most of their areas,
 * and Map_Group counts the other two as MAP_TOWER.
 */
enum MapId {
    MAP_NONE                    =  0,   /* no map */
    MAP_FORTRESS                =  1,   /* a fortress */
    MAP_FOREST_HILLS            =  2,   /* forest and hills */
    MAP_DESERT                  =  3,   /* a desert */
    MAP_SWAMP                   =  4,   /* a swamp */
    MAP_VOLCANO                 =  5,   /* a volcano */
    MAP_JUNGLE                  =  6,   /* a jungle */
    MAP_CASTLE                  =  7,   /* a castle */
    MAP_BATTLEFIELD             =  8,   /* a battlefield */
    MAP_GREAT_ARENA             =  9,   /* the great arena */
    MAP_ARENA                   = 10,   /* the arena */
    MAP_SNOWY_MOUNTAINS         = 11,   /* snowy mountains */
    MAP_BATTLE_TOWN             = 12,   /* a town built for fighting */
    MAP_TOWER                   = 13,   /* a tower */
    MAP_TOWER_B                 = 14,   /* the tower with one more area */
    MAP_TOWER_C                 = 15,   /* the tower with three more areas, one fewer */
    MAP_FOREST_HILLS_NIGHT      = 16,   /* the night version of MAP_FOREST_HILLS */
    MAP_DESERT_NIGHT            = 17,   /* the night version of MAP_DESERT */
    MAP_SWAMP_NIGHT             = 18,   /* the night version of MAP_SWAMP */
    MAP_VOLCANO_NIGHT           = 19,   /* the night version of MAP_VOLCANO */
    MAP_JUNGLE_NIGHT            = 20,   /* the night version of MAP_JUNGLE */
    MAP_SNOWY_MOUNTAINS_NIGHT   = 21,   /* the night version of MAP_SNOWY_MOUNTAINS */
    MAP_BATTLE_TOWN_NIGHT       = 22,   /* the night version of MAP_BATTLE_TOWN */
    MAP_FORTRESS_NIGHT          = 23,   /* the night version of MAP_FORTRESS */
    MAP_GREAT_ARENA_NIGHT       = 24,   /* the night version of MAP_GREAT_ARENA */
    MAP_ARENA_NIGHT             = 25,   /* the night version of MAP_ARENA */
};

/*
 * The area ids: gScene.area, Player.area, Em.area, the stage files' areas,
 * the arguments of the Area_ functions. A map's zone has one id by day and
 * another by night (gAreaOtherTime pairs them, gAreaTimeOfDay tells them
 * apart), and the night ids are listed by the night maps.
 * AREA_<map>_<n> is entry n of the map's list (gMapAreas: Map_AreaIndex
 * gives n, Map_AreaAt the id; the stage file's data per area is indexed by
 * n), AREA_<map>_NIGHT_<n> entry n of its night map's list. The three tower
 * maps order their lists differently, so their extra areas end with the id,
 * and so do the hub areas (the village's and the town's), which are in no
 * map's list.
 */
enum AreaId {
    /* MAP_FORTRESS (7 areas) */
    AREA_FORTRESS_0               = 0x0A,
    AREA_FORTRESS_1               = 0x0E,
    AREA_FORTRESS_2               = 0x1E,
    AREA_FORTRESS_3               = 0x1C,
    AREA_FORTRESS_4               = 0x0B,
    AREA_FORTRESS_5               = 0x0C,
    AREA_FORTRESS_6               = 0x1F,
    /* MAP_FOREST_HILLS (13 areas) */
    AREA_FOREST_HILLS_0           = 0x15,
    AREA_FOREST_HILLS_1           = 0x27,
    AREA_FOREST_HILLS_2           = 0x26,
    AREA_FOREST_HILLS_3           = 0x21,
    AREA_FOREST_HILLS_4           = 0x25,
    AREA_FOREST_HILLS_5           = 0x28,
    AREA_FOREST_HILLS_6           = 0x20,
    AREA_FOREST_HILLS_7           = 0x29,
    AREA_FOREST_HILLS_8           = 0x23,
    AREA_FOREST_HILLS_9           = 0x24,
    AREA_FOREST_HILLS_10          = 0x22,
    AREA_FOREST_HILLS_11          = 0x2A,
    AREA_FOREST_HILLS_12          = 0x2B,
    /* MAP_DESERT (12 areas) */
    AREA_DESERT_0                 = 0x8C,
    AREA_DESERT_1                 = 0x8D,
    AREA_DESERT_2                 = 0x8E,
    AREA_DESERT_3                 = 0x8F,
    AREA_DESERT_4                 = 0x90,
    AREA_DESERT_5                 = 0x91,
    AREA_DESERT_6                 = 0x92,
    AREA_DESERT_7                 = 0x93,
    AREA_DESERT_8                 = 0x94,
    AREA_DESERT_9                 = 0x95,
    AREA_DESERT_10                = 0x96,
    AREA_DESERT_11                = 0x07,
    /* MAP_SWAMP (11 areas) */
    AREA_SWAMP_0                  = 0x97,
    AREA_SWAMP_1                  = 0x98,
    AREA_SWAMP_2                  = 0x99,
    AREA_SWAMP_3                  = 0x9A,
    AREA_SWAMP_4                  = 0x9B,
    AREA_SWAMP_5                  = 0x9C,
    AREA_SWAMP_6                  = 0x9D,
    AREA_SWAMP_7                  = 0x9E,
    AREA_SWAMP_8                  = 0x9F,
    AREA_SWAMP_9                  = 0xA0,
    AREA_SWAMP_10                 = 0x09,
    /* MAP_VOLCANO (10 areas) */
    AREA_VOLCANO_0                = 0xA1,
    AREA_VOLCANO_1                = 0xA2,
    AREA_VOLCANO_2                = 0xA3,
    AREA_VOLCANO_3                = 0xA4,
    AREA_VOLCANO_4                = 0xA5,
    AREA_VOLCANO_5                = 0xA6,
    AREA_VOLCANO_6                = 0xA7,
    AREA_VOLCANO_7                = 0xA8,
    AREA_VOLCANO_8                = 0xA9,   /* also entry 8 of MAP_VOLCANO_NIGHT's list */
    AREA_VOLCANO_9                = 0x4A,
    /* MAP_JUNGLE (11 areas) */
    AREA_JUNGLE_0                 = 0x6E,
    AREA_JUNGLE_1                 = 0x6F,
    AREA_JUNGLE_2                 = 0x70,
    AREA_JUNGLE_3                 = 0x71,
    AREA_JUNGLE_4                 = 0x72,
    AREA_JUNGLE_5                 = 0x73,
    AREA_JUNGLE_6                 = 0x74,
    AREA_JUNGLE_7                 = 0x75,
    AREA_JUNGLE_8                 = 0x76,
    AREA_JUNGLE_9                 = 0x77,
    AREA_JUNGLE_10                = 0x78,   /* also the last of MAP_JUNGLE_NIGHT's list */
    /* MAP_CASTLE (2 areas) */
    AREA_CASTLE_0                 = 0x14,
    AREA_CASTLE_1                 = 0x19,
    /* MAP_BATTLEFIELD (2 areas) */
    AREA_BATTLEFIELD_0            = 0x08,
    AREA_BATTLEFIELD_1            = 0x58,
    /* MAP_GREAT_ARENA and MAP_ARENA (2 areas each) */
    AREA_ARENAS_0                 = 0x59,   /* both arenas' first area */
    AREA_GREAT_ARENA_1            = 0x5A,
    AREA_ARENA_1                  = 0x5B,
    /* MAP_SNOWY_MOUNTAINS (10 areas) */
    AREA_SNOWY_MOUNTAINS_0        = 0x62,
    AREA_SNOWY_MOUNTAINS_1        = 0x5C,
    AREA_SNOWY_MOUNTAINS_2        = 0x5D,
    AREA_SNOWY_MOUNTAINS_3        = 0x5E,
    AREA_SNOWY_MOUNTAINS_4        = 0x5F,
    AREA_SNOWY_MOUNTAINS_5        = 0x60,
    AREA_SNOWY_MOUNTAINS_6        = 0x61,
    AREA_SNOWY_MOUNTAINS_7        = 0x63,
    AREA_SNOWY_MOUNTAINS_8        = 0x64,
    AREA_SNOWY_MOUNTAINS_9        = 0x06,
    /* MAP_BATTLE_TOWN (4 areas) */
    AREA_BATTLE_TOWN_0            = 0xB3,
    AREA_BATTLE_TOWN_1            = 0xB0,
    AREA_BATTLE_TOWN_2            = 0xB1,
    AREA_BATTLE_TOWN_3            = 0xB2,
    /* MAP_TOWER (9 areas) */
    AREA_TOWER_0                  = 0x79,
    AREA_TOWER_1                  = 0x7A,
    AREA_TOWER_2                  = 0x7B,
    AREA_TOWER_3                  = 0x7D,
    AREA_TOWER_4                  = 0x7E,
    AREA_TOWER_5                  = 0x7F,
    AREA_TOWER_6                  = 0x80,
    AREA_TOWER_7                  = 0x81,
    AREA_TOWER_8                  = 0x82,
    /* MAP_TOWER_B (10 areas: MAP_TOWER's and this) */
    AREA_TOWER_0x8A               = 0x8A,
    /* MAP_TOWER_C (11 areas: MAP_TOWER's but AREA_TOWER_4, and these) */
    AREA_TOWER_0x7C               = 0x7C,
    AREA_TOWER_0x53               = 0x53,
    AREA_TOWER_0x54               = 0x54,
    /* MAP_FORTRESS_NIGHT (7 areas) */
    AREA_FORTRESS_NIGHT_0         = 0x4C,
    AREA_FORTRESS_NIGHT_1         = 0x4F,
    AREA_FORTRESS_NIGHT_2         = 0x51,
    AREA_FORTRESS_NIGHT_3         = 0x50,
    AREA_FORTRESS_NIGHT_4         = 0x4D,
    AREA_FORTRESS_NIGHT_5         = 0x4E,
    AREA_FORTRESS_NIGHT_6         = 0x52,
    /* MAP_FOREST_HILLS_NIGHT (13 areas) */
    AREA_FOREST_HILLS_NIGHT_0     = 0xB8,
    AREA_FOREST_HILLS_NIGHT_1     = 0xC0,
    AREA_FOREST_HILLS_NIGHT_2     = 0xBF,
    AREA_FOREST_HILLS_NIGHT_3     = 0xBA,
    AREA_FOREST_HILLS_NIGHT_4     = 0xBE,
    AREA_FOREST_HILLS_NIGHT_5     = 0xC1,
    AREA_FOREST_HILLS_NIGHT_6     = 0xB9,
    AREA_FOREST_HILLS_NIGHT_7     = 0xC2,
    AREA_FOREST_HILLS_NIGHT_8     = 0xBC,
    AREA_FOREST_HILLS_NIGHT_9     = 0xBD,
    AREA_FOREST_HILLS_NIGHT_10    = 0xBB,
    AREA_FOREST_HILLS_NIGHT_11    = 0xC3,
    AREA_FOREST_HILLS_NIGHT_12    = 0xC4,
    /* MAP_DESERT_NIGHT (12 areas) */
    AREA_DESERT_NIGHT_0           = 0x2D,
    AREA_DESERT_NIGHT_1           = 0x2F,
    AREA_DESERT_NIGHT_2           = 0x30,
    AREA_DESERT_NIGHT_3           = 0x31,
    AREA_DESERT_NIGHT_4           = 0x32,
    AREA_DESERT_NIGHT_5           = 0x33,
    AREA_DESERT_NIGHT_6           = 0x34,
    AREA_DESERT_NIGHT_7           = 0x35,
    AREA_DESERT_NIGHT_8           = 0x36,
    AREA_DESERT_NIGHT_9           = 0x37,
    AREA_DESERT_NIGHT_10          = 0x38,
    AREA_DESERT_NIGHT_11          = 0x18,
    /* MAP_SWAMP_NIGHT (11 areas) */
    AREA_SWAMP_NIGHT_0            = 0x10,
    AREA_SWAMP_NIGHT_1            = 0x2C,
    AREA_SWAMP_NIGHT_2            = 0x43,
    AREA_SWAMP_NIGHT_3            = 0x44,
    AREA_SWAMP_NIGHT_4            = 0x45,
    AREA_SWAMP_NIGHT_5            = 0x46,
    AREA_SWAMP_NIGHT_6            = 0x47,
    AREA_SWAMP_NIGHT_7            = 0x48,
    AREA_SWAMP_NIGHT_8            = 0x49,
    AREA_SWAMP_NIGHT_9            = 0x4B,
    AREA_SWAMP_NIGHT_10           = 0x1D,
    /* MAP_VOLCANO_NIGHT (10 areas) */
    AREA_VOLCANO_NIGHT_0          = 0x3A,
    AREA_VOLCANO_NIGHT_1          = 0x3B,
    AREA_VOLCANO_NIGHT_2          = 0x3C,
    AREA_VOLCANO_NIGHT_3          = 0x3D,
    AREA_VOLCANO_NIGHT_4          = 0x3E,
    AREA_VOLCANO_NIGHT_5          = 0x3F,
    AREA_VOLCANO_NIGHT_6          = 0x40,
    AREA_VOLCANO_NIGHT_7          = 0x41,
    AREA_VOLCANO_NIGHT_9          = 0x1B,
    /* MAP_JUNGLE_NIGHT (11 areas) */
    AREA_JUNGLE_NIGHT_0           = 0x01,
    AREA_JUNGLE_NIGHT_1           = 0x02,
    AREA_JUNGLE_NIGHT_2           = 0x03,
    AREA_JUNGLE_NIGHT_3           = 0x04,
    AREA_JUNGLE_NIGHT_4           = 0x05,
    AREA_JUNGLE_NIGHT_5           = 0x12,
    AREA_JUNGLE_NIGHT_6           = 0x13,
    AREA_JUNGLE_NIGHT_7           = 0x16,
    AREA_JUNGLE_NIGHT_8           = 0x17,
    AREA_JUNGLE_NIGHT_9           = 0x1A,
    /* MAP_SNOWY_MOUNTAINS_NIGHT (10 areas) */
    AREA_SNOWY_MOUNTAINS_NIGHT_0  = 0x6B,
    AREA_SNOWY_MOUNTAINS_NIGHT_1  = 0x65,
    AREA_SNOWY_MOUNTAINS_NIGHT_2  = 0x66,
    AREA_SNOWY_MOUNTAINS_NIGHT_3  = 0x67,
    AREA_SNOWY_MOUNTAINS_NIGHT_4  = 0x68,
    AREA_SNOWY_MOUNTAINS_NIGHT_5  = 0x69,
    AREA_SNOWY_MOUNTAINS_NIGHT_6  = 0x6A,
    AREA_SNOWY_MOUNTAINS_NIGHT_7  = 0x6C,
    AREA_SNOWY_MOUNTAINS_NIGHT_8  = 0x6D,
    /* Named by the rule above: entry 9 of the night list, paired with AREA_SNOWY_MOUNTAINS_9
     * by day, and in no other list. Unlike the other snowy-mountain areas it isn't cold
     * (gAreaTemperatures), and monsters start there (seen: EmRathalos_Init and the Init
     * functions of em_kushala.c, em_kutku.c and em_garuga.c give a start position in it). The
     * code doesn't show what place it is. */
    AREA_SNOWY_MOUNTAINS_NIGHT_9  = 0x0F,
    /* MAP_BATTLE_TOWN_NIGHT (4 areas) */
    AREA_BATTLE_TOWN_NIGHT_0      = 0xB7,
    AREA_BATTLE_TOWN_NIGHT_1      = 0xB4,
    AREA_BATTLE_TOWN_NIGHT_2      = 0xB5,
    AREA_BATTLE_TOWN_NIGHT_3      = 0xB6,
    /* MAP_GREAT_ARENA_NIGHT and MAP_ARENA_NIGHT (2 areas each) */
    AREA_ARENAS_NIGHT_0           = 0x2E,   /* both arenas' first area at night */
    AREA_GREAT_ARENA_NIGHT_1      = 0x39,
    AREA_ARENA_NIGHT_1            = 0x42,
    /* the hub areas: in no map's list, neither day nor night (gAreaTimeOfDay 2) */
    AREA_HOUSE                    = 0x56,   /* the house offline (Area_IsHouse, MyHouse_InOwnHouse) */
    AREA_HUB_0x57                 = 0x57,   /* seen: with AREA_HOUSE, Village_AreaKind 2; the way in and out of the house */
    AREA_HUB_0x83                 = 0x83,   /* seen: Village_AreaKind 1; gathering kind 1; its own area packs (gArea83PackFiles) */
    AREA_HUB_0x84                 = 0x84,   /* seen: the town's players are loaded there (Area_HasTownPcs) */
    AREA_HUB_0x85                 = 0x85,
    AREA_HUB_0x86                 = 0x86,   /* seen: Area_Is86to88; the contest's seats (town mode 0x37) */
    AREA_HUB_0x87                 = 0x87,   /* seen: Area_Is86to88; its own music (Bgm_StopInArea87) */
    AREA_HUB_0x88                 = 0x88,   /* seen: Area_Is86to88; its own item table (GameMenu_Start) */
    AREA_HUB_0x89                 = 0x89,   /* seen: gathering kind 0; the pet's name test; NPC pairs as in AREA_HUB_0x57 */
    AREA_HUB_0xAA                 = 0xAA,
    AREA_HUB_0xAB                 = 0xAB,
    AREA_HUB_0xAC                 = 0xAC,
    AREA_ONLINE_HOUSE_0           = 0xAD,   /* the house online at house level 0 (Area_IsOnlineHouse) */
    AREA_ONLINE_HOUSE_1           = 0xAE,   /* at house level 1 */
    AREA_ONLINE_HOUSE_2           = 0xAF,   /* at house level 2 */
    AREA_NONE                     = 0xFFFF   /* no area: the area functions take and give it */
};

/* An area's time of day (gAreaTimeOfDay): DAYTIME_DAY and DAYTIME_NIGHT's
 * values (Area_IdForCurrentTime compares them), and 2 for the hub areas. */
#define AREA_TIME_DAY           0
#define AREA_TIME_NIGHT         1
#define AREA_TIME_NONE          2

/* Map_AreaIndex's answer when the map has no such area. */
#define AREA_INDEX_NONE 0xFFFF

/*
 * An area's temperature (gAreaTemperatures, Area_Temperature). The hot ones
 * need a heat resistance skill, the cold ones a cold one, or the player
 * idles with a hot or cold motion (Player_StartIdleMotion); a cool drink
 * wears off twice as fast in a very hot area, a hot drink in a very cold
 * one (unk_399F70.c).
 */
#define AREA_TEMP_NORMAL        0
#define AREA_TEMP_HOT           1
#define AREA_TEMP_COLD          2
#define AREA_TEMP_VERY_HOT      3
#define AREA_TEMP_VERY_COLD     4

/* The header of an area's grid (the two parts of its table file, gAreaTableFiles;
 * Area_LoadTables and AreaTable_Relocate in unk_3240E0.c): the grid's offsets are
 * from this header. */
typedef struct AreaGridHeader {
    u32 cellX;          /* 0x00: gAreaGridCellX */
    u32 cellZ;          /* 0x04: gAreaGridCellZ */
    u32 rows;           /* 0x08 */
    u32 cols;           /* 0x0C */
    u8 unk10[0x18 - 0x10];
    u32 tblOff;         /* 0x18: rows * cols offsets of the cells' lists, each ended by -1 */
    u32 dataOff;        /* 0x1C: the base of the offsets in the lists */
} AreaGridHeader;

#endif /* AREA_H */
