#ifndef AREA_H
#define AREA_H

/*
 * The state of the current area: gAreaState (0x5EC460, 288 bytes). Area_ClearState
 * clears it, Area_Enter and Area_Load set it up, the area pack's part 2 fills its fog, lights
 * and sky (AreaPack_ReadPart2), Area_UpdateTimers runs it each frame (sub_main area_update.c,
 * area_anim.c), and the area's sky and props draw from it (area_draw.c).
 */

#include "common.h"
#include "obj.h"
#include "sprite.h"
#include "objhead.h"

/* Fog parameters (Area_ApplyFog: RS_FOG_COLOR, RS_FOG_NEAR, RS_FOG_FAR; gFogDefault, gFogHouse, the
 * area pack's part 0: the colour first, then the two distances). */
typedef struct FogParam {
    f32 nearDist;               /* 0x0: RS_FOG_NEAR (gGsFogNear) */
    f32 farDist;                /* 0x4: RS_FOG_FAR (gGsFogFar) */
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
    u8 drawSky;                 /* 0x001: the sky and the glow props are drawn (Area_DrawModels, Sky_Draw,
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
    f32 pos[3];                 /* 0x010: the area model's position (Area_DrawModels) */
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
    f32 lightColor[3][3];       /* 0x080: their colours, scaled by each vertex's lighting (Light.color) */
    f32 lightAmbient[3][3];     /* 0x0A4: their parts of the ambient colour (Light.ambient) */
    f32 skyPos[3];              /* 0x0C8: a far point (5000.0 each by default): Sky_DrawBand turns the
                                 * band's colours by the camera's heading to it, AreaAtmos_Draw
                                 * fades by the camera's angle to it, AreaAtmos_Update places
                                 * its sound there, Shadow_Draw turns a player's two part
                                 * shadows by the heading to it */
    SkyBand band;               /* 0x0D4: the sky band (Sky_Draw) */
    u8 look[4];                 /* 0x0F8: [0] the night level (0 day .. 255 night: online it follows
                                 * LobbyTime_IsAm, 12 a frame; darkens the props and notices);
                                 * [1] the area's variant (the season of AREA_ONLINE_VILLAGE, the house level
                                 * of AREA_ONLINE_HOUSE_0 to AREA_ONLINE_HOUSE_2; Area_DrawProp's props of one variant);
                                 * [1..3] the facility levels of AREA_HUB_0x89 (Area_Enter) */
    u8 monsterState;            /* 0x0FC: cached result of Area_MonsterState (0-2) */
    u8 flyer;                   /* 0x0FD: set while a kind 29 object exists (AreaFlyer_Create) */
    u16 propTimer;              /* 0x0FE: the frames of the moving props' state (AREA_HUB_0x89, Area_Update) */
    f32 propOfs[3];             /* 0x100: the moving props' offset (Area_DrawProp's kind 13) */
    f32 flyerPos[3];            /* 0x10C: the kind 29 object's position (AreaFlyer_Run) */
    u8 propState;               /* 0x118: the moving props' state, from the lobby's (1 out, 2 back) */
    u8 propHold;                /* 0x119: frames state 1 is held after the lobby's state left it */
    u8 unk11A[0x120 - 0x11A];
} AreaState;
typedef char AreaState_size_check[(sizeof(AreaState) == 0x120) ? 1 : -1];

extern AreaState gAreaState;

/*
 * gAreaFade, the screen's flash after a blast (the game's blast effects
 * start it; Area_Update steps it, Sky_Draw draws it over the whole screen),
 * with gAreaFadeTimer counting each phase's frames.
 */
#define AREA_FADE_NONE          0
#define AREA_FADE_FLASH         1   /* 2 frames: the screen white */
#define AREA_FADE_DIP           2   /* 2 frames: white, blended by the timer */
#define AREA_FADE_OUT           3   /* 90 frames: a sine-shaped fade */

/*
 * The flags of an ambient sound source in the lobby's areas (the
 * gAmbSeFlagsHub* tables; Area_UpdateAmbientSounds). The facility bits
 * hold one bit per level of three of the village's facilities: the
 * source plays only when the bit of a facility's current level is set
 * (AMB_SE_FACILITYn_LEVEL0 << level).
 */
#define AMB_SE_PM_ONLY              0x0001  /* silent while LobbyTime_IsAm */
#define AMB_SE_FACILITY1_LEVEL0     0x0002  /* bits 1-3: facility 1's levels */
#define AMB_SE_FACILITY2_LEVEL0     0x0010  /* bits 4-6: facility 2's levels */
#define AMB_SE_FACILITY3_LEVEL0     0x0080  /* bits 7-9: facility 3's levels */
#define AMB_SE_FACILITY_LEVELS      0x03FE  /* any of the facility bits */
#define AMB_SE_NOT_IN_CONTEST       0x0400  /* silent while Contest_IsOpen */
#define AMB_SE_NOT_IN_DRINK_CONTEST 0x0800  /* silent while DrinkContest_IsOn */
#define AMB_SE_AM_ONLY              0x1000  /* silent unless LobbyTime_IsAm */

/*
 * The maps: gScene.map and QuestInfo.map, the index of gMapAreas,
 * gMapAreaCount and gStartAreaTables, and of the maps' names table
 * (gStageNames, 0x446E80). Each is named from its entry there,
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
    MAP_COUNT                   = 26    /* the number of maps: the tables by map have this many entries */
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
    AREA_HUB_0x57                 = 0x57,   /* seen: with AREA_HOUSE, Pet_AreaKind 2; the way in and out of the house */
    /* The village online: the player's area between the network's lists and a town
     * (LOBBY_ENTER_VILLAGE; the login starts the player there). Its look and music follow the
     * lobby's season; the care pet lives there (PET_AREA_CARE); chat goes to the whole lobby
     * and the game menu has no quest page there. */
    AREA_ONLINE_VILLAGE           = 0x83,
    AREA_HUB_0x84                 = 0x84,   /* seen: the town's players are loaded there (Area_HasTownPcs) */
    AREA_HUB_0x85                 = 0x85,
    /* The town's three halls, the places a quest can bring the player back to
     * (LobbyEnter_Begin opens LOBBY_TEXT_RETURN_* for each; Area_IsTownHall tests them). */
    AREA_TAVERN                   = 0x86,   /* the tavern: the drinking contest's seats are there */
    AREA_TOWN_ARENA               = 0x87,   /* the town's arena hall: its own music (Bgm_StopInTownArena) */
    AREA_ELDER_HALL               = 0x88,   /* the elders' hall: its own item table (GameMenu_Start) */
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
 * one (player_sys.c).
 */
#define AREA_TEMP_NORMAL        0
#define AREA_TEMP_HOT           1
#define AREA_TEMP_COLD          2
#define AREA_TEMP_VERY_HOT      3
#define AREA_TEMP_VERY_COLD     4

/* The header of an area's grid (the two parts of its table file, gAreaTableFiles,
 * loaded by Area_LoadTables and relocated by AreaTable_Relocate): the grid's
 * offsets are from this header. */
typedef struct AreaGridHeader {
    u32 cellX;          /* 0x00: gAreaGridCellX */
    u32 cellZ;          /* 0x04: gAreaGridCellZ */
    u32 rows;           /* 0x08 */
    u32 cols;           /* 0x0C */
    u8 unk10[0x18 - 0x10];
    u32 tblOff;         /* 0x18: rows * cols offsets of the cells' lists, each ended by -1 */
    u32 dataOff;        /* 0x1C: the base of the offsets in the lists */
} AreaGridHeader;

/* A triangle of the area's collision: an entry of the grid's cell lists
 * (gAreaGridCells, gAreaGrid2Cells), 56 bytes. */
typedef struct AreaTri {
    u8 kind;            /* 0x00: its kind: the row of the area's gAreaKindFlags and gAreaTriKindFlags */
    u8 seArg;           /* 0x01: the sound argument of the surface (WallContact.seArg, Em.floorArg: passed with the voice sounds) */
    u16 attr;           /* 0x02: AREA_TRI_ATTR_* bits: a sweep or segment test skips the triangle when they meet its mask (Em.floorAttr) */
    f32 v[3][3];        /* 0x04: the corners */
    f32 n[3];           /* 0x28: the normal */
    f32 d;              /* 0x34: the plane distance */
} AreaTri;
typedef char AreaTri_size_check[(sizeof(AreaTri) == 0x38) ? 1 : -1];

/* The first 4 bytes of an AreaTri, as the floor tests hand them out
 * (Em_GetFloorInfo writes them over Em.floorKind .. floorAttr). */
typedef struct TriHdr {
    u8 kind;
    u8 seArg;
    u16 attr;
} TriHdr;

/* AreaTri.attr's bits, as the sweeps' masks use them. */
#define AREA_TRI_ATTR_NO_PLAYER     0x0004  /* always skipped by the players' sweep */
#define AREA_TRI_ATTR_NOT_AIR       0x0200  /* skipped by a monster's sweep in EM_STANCE_AIR */
#define AREA_TRI_ATTR_FLAGGED       0x4000  /* skipped by every sweep while gScene.skipFlaggedTris is set */
#define AREA_TRI_ATTR_PLAYER_ON_WALL 0x8000 /* skipped by the players' sweep, except while Player_IsOnWall */

/* The area collision's limits. */
#define AREA_EDGE_MARGIN        8.0f    /* positions are kept this far inside the grids' and the area's edges */
#define AREA_HITS_MAX           9       /* the most hits a sweep records (gAreaHits) */
#define AREA_SEEN_TRIS          10      /* the triangles a sweep remembers as tested (a ring of them) */
#define AREA_RAY_TRIS           10      /* the triangles a segment test collects */
#define AREA_FLOOR_CANDIDATES   5       /* the floor heights a floor test collects under a point */
#define FLOOR_SLIDE_SLOPE       0x1500  /* the least slope (ANGLE_TURN to a turn: about 30 degrees) Tri_PushOut pushes off */

/* A row of gAreaKindFlags: what a triangle kind of the area's floor does
 * (one table of rows per area, indexed by AreaTri.kind). */
typedef struct AreaKindRow {
    u8 lightZone;       /* 0x0: the entry of gAreaLightZones[area] whose colours the object takes (0: none) */
    u8 lightZone2;      /* 0x1: the same in gAreaLightZones2 */
    u8 mustCrouch;      /* 0x2: copied to the object's mustCrouch */
    u8 unk3;            /* 0x3: tested by AreaGrid2_OnFlaggedFloor (seen: Gravios's work reads it) */
    u8 passThrough;     /* 0x4: not stood on: left out of the floor heights (Obj_OnPassThroughFloor) */
    u8 noLanding;       /* 0x5: left out of AreaGrid2_GetFloorYFiltered, the floor thrown objects and attacks land on */
    u8 surfaceKind;     /* 0x6: copied to Em.surfaceKind */
    u8 hazard;          /* 0x7: copied to the object's floorHazard */
} AreaKindRow;

/* A row of gAreaTriKindFlags: what a triangle kind of the area's walls does. */
typedef struct AreaWallKindRow {
    u8 special;         /* 0x0: the contact is WALL_CONTACT_SPECIAL (the players) */
    u8 emSpecial;       /* 0x1: sets Em.wallHitSpecial (the monsters) */
    u8 unk2[6];
} AreaWallKindRow;

/* A light zone of an area (gAreaLightZones, gAreaLightZones2; 24 bytes): an
 * object within far of pos takes the colours of gAreaLightColors' entry
 * nearColors within near, else farColors. */
typedef struct AreaLightZone {
    f32 pos[3];         /* 0x00 */
    f32 near;           /* 0x0C: nearer than this, nearColors */
    f32 far;            /* 0x10: farther than this, the area's own colours */
    u16 nearColors;     /* 0x14 */
    u16 farColors;      /* 0x16 */
} AreaLightZone;

void LightShaft_Create(void);
void LightShaft_Update(Obj *obj);
void LightShaft_Setup(Obj *obj);
void LightShaft_Run(Obj *obj);
void LightShaft_End(Obj *obj);
void LightShaft_Free(Obj *obj);
void LightShaft_Draw(DrawSlot *drawSlot);
void SkyStreak_Create(u8 streakType);
void SkyStreak_Update(Obj *obj);
void SkyStreak_Setup(Obj *obj);
void SkyStreak_Run(Obj *obj);
void SkyStreak_End(Obj *obj);
void SkyStreak_Free(Obj *obj);
void SkyStreak_Draw(DrawSlot *drawSlot);
void AreaDrop_Create(void);
void AreaDrop_Update(Obj *obj);
void AreaDrop_Setup(Obj *obj);
void AreaDrop_Run(Obj *obj);
void AreaDrop_RunDrip(Obj *obj);
void AreaDrop_RunWeaponBurst(Obj *obj);
void AreaDrop_End(Obj *obj);
void AreaDrop_Free(Obj *obj);
void AreaDrop_Draw(DrawSlot *drawSlot);
void AreaFlag_Create(void);
void AreaFlag_Update(Obj *obj);
void AreaFlag_Setup(Obj *obj);
void AreaFlag_Run(Obj *obj);
void AreaFlag_End(Obj *obj);
void AreaFlag_Free(Obj *obj);
void AreaFlag_Draw(DrawSlot *drawSlot);
void AreaWeapon_Create(u8 weaponSet);
void AreaWeapon_Update(Obj *obj);
void AreaWeapon_Setup(Obj *obj);
void AreaWeapon_Run(Obj *obj);
void AreaWeapon_End(Obj *obj);
void AreaWeapon_Free(Obj *obj);
void AreaWeapon_Draw(DrawSlot *drawSlot);
s32 AreaWeapon_GunIndex(Obj *obj);
void AnimatedProps_Create(void);
void AnimatedProps_Update(Obj *obj);
void AnimatedProps_Setup(void *objArg);
void AnimatedProps_Run(Obj *obj);
void AnimatedProps_End(Obj *obj);
void AnimatedProps_Free(Obj *obj);
void AnimatedProps_Draw(DrawSlot *drawSlot);
void AnimatedGroundTiles_Create(void);
void AnimatedGroundTiles_Update(Obj *obj);
void AnimatedGroundTiles_Setup(Obj *obj);
void AnimatedGroundTiles_Run(Obj *obj);
void AnimatedGroundTiles_End(Obj *obj);
void AnimatedGroundTiles_Free(Obj *obj);
void AnimatedGroundTiles_Draw(DrawSlot *drawSlot);
void FortressTremor_Create(void);
void FortressTremor_Update(Obj *obj);
void FortressTremor_Setup(Obj *obj);
void FortressTremor_Run(Obj *obj);
void FortressTremor_End(Obj *obj);
void FortressTremor_Free(Obj *obj);
void FortressTremor_Draw(DrawSlot *drawSlot);
void BreakableStructure_Create(void);
void BreakableStructure_Update(Obj *obj);
void BreakableStructure_Setup(Obj *obj);
void BreakableStructure_Run(Obj *obj);
void BreakableStructure_End(Obj *obj);
void BreakableStructure_Free(Obj *obj);
void BreakableStructure_Draw(DrawSlot *drawSlot);
void ScrollingOverlay_Create(void);
void ScrollingOverlay_Update(Obj *obj);
void ScrollingOverlay_Setup(Obj *obj);
void ScrollingOverlay_Run(Obj *obj);
void ScrollingOverlay_End(Obj *obj);
void ScrollingOverlay_Free(Obj *obj);
void ScrollingOverlay_Draw(DrawSlot *drawSlot);
void TurningProp_Create(void);
void TurningProp_Update(Obj *obj);
void TurningProp_Setup(Obj *obj);
void TurningProp_Run(Obj *obj);
void TurningProp_End(Obj *obj);
void TurningProp_Free(Obj *obj);
void TurningProp_Draw(DrawSlot *drawSlot);
void GroundTileGrid_Create(void);
void GroundTileGrid_Update(Obj *obj);
void GroundTileGrid_Setup(Obj *obj);
u32 GroundTileGrid_Run(Obj *obj);
void GroundTileGrid_End(Obj *obj);
void GroundTileGrid_Free(Obj *obj);
void GroundTileGrid_Draw(DrawSlot *drawSlot);
void AnimatedScenery_Create(u8 setId);
void AnimatedScenery_Update(Obj *obj);
void AnimatedScenery_Setup(Obj *obj);
u32 AnimatedScenery_Run(Obj *obj);
void AnimatedScenery_End(Obj *obj);
void AnimatedScenery_Free(Obj *obj);
void AnimatedScenery_Draw(DrawSlot *drawSlot);
void CastleGate_Create(u8 gateType);
void CastleGate_Update(Obj *obj);
void CastleGate_Setup(Obj *obj);
void CastleGate_Run(Obj *obj);
void CastleGate_End(Obj *obj);
void CastleGate_Free(Obj *obj);
void CastleGate_Draw(DrawSlot *drawSlot);
void DistantLightning_Create(void);
void DistantLightning_Update(Obj *obj);
void DistantLightning_Setup(Obj *obj);
void DistantLightning_Run(Obj *obj);
void DistantLightning_End(Obj *obj);
void DistantLightning_Free(Obj *obj);
void DistantLightning_Draw(DrawSlot *drawSlot);
void AreaObj_K17_Create(u8 phase);
void AreaObj_K17_Update(Obj *obj);
void AreaObj_K17_Setup(Obj *obj);
void AreaObj_K17_Run(Obj *obj);
void AreaObj_K17_End(Obj *obj);
void AreaObj_K17_Free(Obj *obj);
void AreaObj_K17_Draw(DrawSlot *slot);
void AreaObj_K19_Create(void);
void AreaObj_K19_Update(Obj *obj);
void AreaObj_K19_Setup(Obj *obj);
void AreaObj_K19_Run(Obj *obj);
void AreaObj_K19_End(Obj *obj);
void AreaObj_K19_Free(Obj *obj);
s32 AreaObj_K19_GetPoints(u16 area);
s16 AreaObj_K19_PointCount(u16 area);
extern f32 FortressTremor_PosX;
extern f32 FortressTremor_PosZ;
extern f32 FortressTremor_ImpactOffsets[12];
extern s16 BreakableStructure_ModelsDay[8];
extern u16 BreakableStructure_Yaw[8];
extern f32 BreakableStructure_Rise[6];
extern s16 BreakableStructure_Tilt[6];
extern f32 BreakableStructure_SoundPos[8];
extern s16 BreakableStructure_ModelsNight[6];
extern f32 TurningProp_Scales[6];
extern s16 TurningProp_Models[6];
extern u8 GroundTileGrid_Jungle1Tiles[32];
extern u8 GroundTileGrid_Jungle6Tiles[80];
extern u16 GroundTileGrid_Jungle6Turns[72];
extern f32 *AnimatedScenery_Positions[56];
extern s16 CastleGate_Models[4];
extern s16 AreaObj_K17_AngleTbl[6];

void Area_LoadTables(s32 id);
void AreaTable_Relocate(u8 *file);
void AreaTable2_Relocate(u8 *file);
s32 AreaGrid_SweepCells(f32 *from, f32 *target, f32 *out, u16 mask, f32 radius);
s32 List_Contains(s32 id, s32 *list, u32 n);
s32 AreaHitTris_Contains(s32 id, u32 n);
s32 AreaGrid_Quadrant(f32 *pos);
s32 AreaGrid2_Contains(f32 *pos);
s32 AreaGrid_Contains(f32 *pos);
s32 Area_ContainsPos(s32 id, f32 *pos);
s32 AreaGrid_ClampSegment(f32 *out, f32 *target, f32 *from);
s32 *AreaGrid_GetCell(f32 *pos);
s32 Capsule_TestTriLevel(f32 *cap, AreaTri *tri, f32 *from);
s32 Capsule_TestTri(f32 *cap, AreaTri *tri);
s32 Tri_ContainsPoint(f32 *point, f32 *tri, f32 *n);
s32 AreaHit_Resolve(f32 *cap, f32 *out, u8 *flag);
void Box_Accumulate(f32 *push, f32 *box, f32 lim);
f32 AreaGrid2_GetFloorY(f32 *pos);
f32 AreaGrid2_GetFloorYFiltered(f32 *pos);
s32 Em_GetFloorY(Em *em, f32 *pos, f32 *out);
s32 Em_GetFloorYAbove(Em *em, f32 *pos, f32 *out);
s32 Em_GetFloorInfo(Em *em, f32 *pos, TriHdr *info, f32 *out, s32 *surfaceY);
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, TriHdr *info, f32 *out, f32 *depth);
u8 AreaGrid2_OnFlaggedFloor(f32 *pos);
s32 *AreaGrid2_GetCell(f32 *pos);
s32 AreaGrid_TestSegment(Em *em, f32 *start, f32 *end, f32 *out, u16 mask);
s32 Em_FindFloorTri(Em *em, f32 *out, s32 apply);
u8 Tri_PushOut(f32 *tri, Em *obj, s32 *slope, f32 *out, f32 dropScale);
void Vec_AngleToUp(f32 *dir, s32 *out);
void Obj_UpdateLightColors(u8 floorKind, f32 *pos, ObjHead *obj);
void Obj_UpdateFloorEffects(ObjHead *obj);
void Em_UpdateLightColors(Em *em);
u8 Obj_OnPassThroughFloor(void *obj);
void Em_CollideField(Em *em, u8 keep);
extern f32 gPlayerBodySpheres[8];

void Area_Enter(u16 area);
void Area_ApplyFog(u16 area);
void Area_StartObjects(u16 area);
s16 Area_SubModelCount(u16 area);
void Player_CheckActionSpot(void);
void Area_ClearState(void);
void Area_ResetFade(void);
s16 Area_Temperature(u16 area);
u16 Map_DayId(u16 map);
s32 Area_IsDay(u16 area);
u16 Area_OtherTimeId(u16 area);
u16 Area_DayId(u16 area);
u16 Map_AreaIndex(u16 map, u16 area);
u16 Map_AreaAt(u16 map, u16 index);
s32 Area_NoBarrelBombs(u16 area);
f32 Area_GroundOffset(u16 area);
s32 Area_HasFlyer(void);
void Area_GetFlyerPos(f32 *out, u16 area);
void Area_StartTimers(AreaState *state);
void Area_UpdateAmbientSounds(AreaState *state);
void Area_Update(AreaState *state);

s32 Obj_InCurrentArea(void *obj);
u8 Player_InCurrentArea(void *obj);
void Player_UpdateAreaPos(void *obj);
void Area_ToWorld(s32 area, f32 *pos, f32 *out);

#endif /* AREA_H */
