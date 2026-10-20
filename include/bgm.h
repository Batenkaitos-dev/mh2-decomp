#ifndef BGM_H
#define BGM_H

/*
 * The music manager (the Bgm_* and Jingle_* functions).
 *
 * The music streams on channel BGM_CH_MUSIC. Short tunes (jingles, the
 * event tunes, the town's shop tunes) stream on channel BGM_CH_TUNE; while
 * one plays, the music is turned down (MUSIC_FLAG_DUCKED) and restored
 * when it ends.
 *
 * Two state machines share gScene.musicState and gScene.musicTimer:
 * - in the field, Bgm_FieldUpdate runs every frame (enum MusicState): the
 *   area's music, the battle music once a fight begins, the music
 *   of the goal met or the quest lost;
 * - in the town, Bgm_TownUpdate (MUSIC_TOWN_*): the area's music, chosen
 *   by the season and the time of day, and the short tunes that mark a
 *   new season or time of day.
 *
 * Each area also sets the two sound cores' reverb (gAreaReverbs).
 */
#include "common.h"

#define BGM_CH_MUSIC        0       /* the stream channel of the music */
#define BGM_CH_TUNE         1       /* the stream channel of the jingles and event tunes */

#define BGM_VOL_MAX         0x7F    /* a stream's full volume */
#define BGM_FADE_FRAMES     15      /* frames: the music's fades in and out */
#define BGM_BATTLE_FRAMES   150     /* frames: the battle music plays at least this long */

/* The stream ids (afs00.afs's files) the music manager picks by role. */
#define BGM_BATTLE          0xD3    /* a fight with no large monster in it (Bgm_IsBattle 1) */
#define BGM_VERSUS          0xC4    /* the battle music of a versus quest */
#define BGM_FAINT           0xB8    /* the local player fainted (Bgm_PlayFaintMusic) */
#define BGM_GOAL_MET        0xB6    /* once the goal is met: Quest_GetOutcome(1) is 1 (MUSIC_GOAL_MET) */
#define BGM_QUEST_LOST      0xD4    /* once the quest is lost: Quest_GetOutcome(1) is -1 (MUSIC_LOST) */
#define BGM_QUEST_THEME  0xC3    /* all through a quest with gQuest.flags 0x800 */
#define BGM_SERVER_MENU     0x123   /* the lobby's server choice menu (SrvMenu_SlideIn, SrvMenu_Intro) */
#define BGM_AREA_LIST       0x124   /* the lobby's area list (AreaList_Init) */
#define BGM_TUNE_NPC        0xB1    /* the town's menus opened at a character (the lobby's menus) */
#define BGM_TUNE_RANDOM_START 0x30  /* a stage with a random start, entering an area of the hunt */
/* The tunes the town's tours start with (Jingle_Play). */
#define BGM_TUNE_TOUR_HUB_0xAA 0xD6 /* TOWN_TOUR_HUB_0xAA */
#define BGM_TUNE_TOUR_HUB_0xAB 0xD7
#define BGM_TUNE_TOUR_HUB_0xAC 0xD8
#define BGM_TUNE_TOUR_SHIP  0xD9    /* TOWN_TOUR_SHIP */
#define BGM_TUNE_TOUR_VILLAGE_RETURN 0xDA
#define BGM_TUNE_TOUR_HOUSE 0xDB    /* TOWN_TOUR_HOUSE_1 and _2 */

/* Bgm_IsBattle's answers: the fight in the local player's area. */
#define FIGHT_NONE          0
#define FIGHT_PLAYERS       1       /* a player in the area is in battle */
#define FIGHT_LARGE_MONSTER 2       /* a large monster in the area is in combat */

#define BGM_LIST_END        0xFFFF  /* ends gAreaTunes, gAreaMusicAlts and gAreaJingles */
#define JINGLE_LIST_END     0xFF    /* ends an area's VariantJingle list */
#define BATTLE_BGM_KINDS    9       /* the entries of gBattleBgmKinds Bgm_PlayBattle reads */

/* gScene.musicState in the field (Bgm_FieldUpdate). */
enum MusicState {
    MUSIC_AREA = 0,             /* the area's music; watches for a fight and the area's variant */
    MUSIC_VARIANT_FADE = 1,     /* the area's variant changed: fading out, then Bgm_EnterArea */
    MUSIC_BATTLE_FADE = 2,      /* a fight began: fading out, then BGM_BATTLE */
    MUSIC_BATTLE = 3,           /* BGM_BATTLE, at least BGM_BATTLE_FRAMES */
    MUSIC_MONSTER_FADE = 4,     /* a large monster fights: fading out, then its battle music */
    MUSIC_MONSTER_BATTLE = 5,   /* the monster's battle music (Bgm_PlayBattle), until the fight ends */
    MUSIC_BATTLE_OVER = 6,      /* the fight ended: fading out, then the area's music */
    MUSIC_FAINT = 7,            /* BGM_FAINT */
    MUSIC_GOAL_MET_WAIT = 8,    /* the goal met (Quest_GetOutcome(1) turned 1) in a quest with gQuest.flags 2: 100 frames */
    MUSIC_GOAL_MET_FADE = 9,    /* fading out, then BGM_GOAL_MET */
    MUSIC_GOAL_MET = 10,        /* BGM_GOAL_MET, until the quest is lost after all (Quest_GetOutcome(1) is -1) */
    MUSIC_LOST_FADE = 11,       /* the quest lost (Quest_GetOutcome(1) turned -1): fading out, then BGM_QUEST_LOST */
    MUSIC_LOST = 12             /* BGM_QUEST_LOST, until Quest_GetOutcome(1) is 1 again */
};

/* gScene.musicState in the town (Bgm_TownUpdate). */
#define MUSIC_TOWN_AREA     0       /* the area's music; watches for a new season or time of day */
#define MUSIC_TOWN_NEXT     1       /* AREA_TAVERN: fading out, then another of its six tracks */
#define MUSIC_TOWN_CHANGE   2       /* a new season or time of day: fading out, then its tune */
#define MUSIC_TOWN_TUNE     3       /* Bgm_PlayTownTune's tune, until it ends; then the area's music */

/* Bgm_PlayTownTune's kinds. */
#define TOWN_TUNE_EVENT     0       /* gTownTuneEvents: the farm's and the village's events */
#define TOWN_TUNE_SEASON    1       /* gTownTuneSeasons: a new season */
#define TOWN_TUNE_TIME      2       /* gTownTuneTimes: a new time of day */

/* gScene.musicFlags. */
#define MUSIC_FLAG_AREA_TUNE 0x01   /* an area's own tune (gAreaTunes) has played; cleared with the quest */
#define MUSIC_FLAG_DUCKED    0x02   /* the music is turned down under a tune on BGM_CH_TUNE */

/* A music stream and its volume. */
typedef struct BgmEntry {
    u16 id;                     /* 0x0: the stream */
    u16 vol;                    /* 0x2: 0 to BGM_VOL_MAX */
} BgmEntry;

/* gAreaMusic: an area's music, by area id. */
typedef struct AreaMusic {
    u16 id;                     /* 0x0: the stream */
    u8 vol;                     /* 0x2 */
    u8 pad3;
} AreaMusic;

/* gAreaMusicAlts: an area's music for one of its variants (gBgmVariant). */
typedef struct AreaMusicAlt {
    u16 area;                   /* 0x0: 0xFFFF ends the list */
    u16 variant;                /* 0x2: compared with gBgmVariant */
    u16 id;                     /* 0x4 */
    u8 vol;                     /* 0x6 */
    u8 pad7;
} AreaMusicAlt;

/* A stream by key: gBattleBgmKinds (a monster kind's battle music) and
 * gAreaTunes (an area's own tune, played once). */
typedef struct BgmKey {
    u16 key;                    /* 0x0: the monster kind or the area; 0xFFFF ends gAreaTunes */
    u16 id;                     /* 0x2: the stream */
} BgmKey;

/* A sound played when an area's music variant changes (Jingle_PlayForArea). */
typedef struct VariantJingle {
    u8 from;                    /* 0x0: the variant the music was playing for (gBgmIndex); 0xFF ends the list */
    u8 to;                      /* 0x1: the area's variant now (gBgmVariant) */
    u8 slot;                    /* 0x2: Sound_PlaySeFlat's slot */
    u8 se;                      /* 0x3: and sound */
} VariantJingle;

/* gAreaJingles: an area's list of VariantJingle. */
typedef struct AreaJingles {
    u16 area;                   /* 0x0: 0xFFFF ends the list */
    VariantJingle *list;        /* 0x4 */
} AreaJingles;

/* The reverb of the two sound cores (Sound_SetEffectOnce), one per area
 * kind (gAreaReverbIdx). */
typedef struct AreaReverb {
    s32 mode0;                  /* 0x0: core 0's effect mode */
    s32 depth0;                 /* 0x4: and depth */
    s32 mode1;                  /* 0x8: core 1's */
    s32 depth1;                 /* 0xC */
} AreaReverb;

extern u16 gBgmId;              /* the stream playing on BGM_CH_MUSIC (BGM_NONE: none) */
extern u8 gBgmVolume;           /* the area music's volume, restored after a tune */
extern u8 gBgmVariant;          /* the area's music variant (Area_MonsterState) */
extern u8 gBgmIndex;            /* field: the variant the music plays for; AREA_TAVERN: its track */
extern u8 gBgmPaused;           /* nonzero: Bgm_TownUpdate does nothing */
extern u8 gBgmPm;               /* the time of day the town's music was chosen for (LobbyTime_IsPm) */
extern u8 gBgmSeason;           /* the season it was chosen for (LobbyWorld_GetSeason) */

#endif /* BGM_H */
