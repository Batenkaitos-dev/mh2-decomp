/*
 * The lobby world: its clock and calendar, the town arena's music show, the
 * light fades keyed by the show, the world's timed events and the elder
 * raid's movies. All of it lives in gLobbyWorld (a LobbyWorld).
 *
 * The clock: LobbyWorld_Tick counts the frames of the vsync counter once a
 * frame and advances the world's time of day (0-86400 seconds); a day lasts
 * gTownDayTicks seconds of play. Past midnight comes the next day, past a
 * season's last day the next season, three seasons to a year. Online the
 * season, day and year follow the server (gTownSeason, gTownDay,
 * gTownYear), offline the save.
 *
 * The show: a track asked for at the song menu (LobbyBgm_Play) streams on
 * channel 0 as a list of entries, each a stream file of its own
 * (gLobbyBgmFiles) starting at a position in frames (gLobbyBgmMarks;
 * gLobbyBgmInfo gives the count and the track's length). LobbyBgm_StepPlay
 * runs it each town frame: at each new entry the music actor (the
 * townsperson LobbyBgm_SetActor hands it) starts the entry's motion script,
 * the stage camera takes the entry's shot, and three lights of the arena
 * fade between colours by the track's key lists (gLobbyFadeTables,
 * LobbyFadeKey); four more take the glow's alpha. Area_DrawGlowProps reads
 * the colours and the alpha.
 *
 * The events: four LobbyWorldEvent slots, each a function called once the
 * world's time reaches the event's season, day and tick (the elder raid's
 * start and end, LobbyWorld_ScheduleEvents). The movie: LobbyMovie_Prepare
 * and LobbyMovie_Run play the raid's start or end movie in town.
 *
 * The file's data: the show's tracks and entries, the fade key lists and
 * the raid movies' lengths.
 */
#include "common.h"
#include "lobby_state.h"
#include "movie.h"
#include "syssettings.h"
#include "em.h"
#include "lobby_vobj.h"
#include "area.h"
#include "stream.h"
#include "save.h"
#include "pad.h"
#include "area_draw.h"
#include "bgm.h"
#include "place_rules.h"
#include "camera.h"
#include "lobby_net.h"
#include "lobby_world.h"
#include "game_util.h"
#include "music_actor.h"
#include "rand.h"
#include "village_clock.h"
#include "libc.h"
#include "card.h"

/*
 * The lobby world's state (gLobbyWorld, 512 bytes; the events at +0x130
 * have a label of their own too): its clock and calendar (a day of
 * `dayFrames` frames, three seasons
 * of seasonDays[] days; LobbyWorld_Tick advances it by the frames counted
 * by the vsync counter (gSubVSyncCount), LobbyWorld_AddDays by a fraction
 * of a day, LobbyWorld_NextDay starts the
 * next day and, past a season's last day, the next season), the background
 * music (Stream_* channel 0: the track, its position table and a file per
 * entry), three colour fades keyed by time (LobbyWorld_NextFadeKey) and four timed
 * events. The game's code reaches many fields by labels of their own, so
 * the world's functions read those fields as objects of their own (the
 * externs below).
 */
/* LobbyWorld.streamState: the show's stream, as LobbyBgm_Update steps it
 * (state 1 is never set). */
enum LobbyBgmState {
    LOBBY_BGM_IDLE = 0,
    LOBBY_BGM_STREAMING = 2,        /* until the stream stops, then the area's music */
    LOBBY_BGM_WAITING = 3,          /* in AREA_TOWN_ARENA, once channel 0 plays, the track is asked for again */
    LOBBY_BGM_STARTING = 4,         /* the area's music fades out, then the entry's file is prepared and started */
    LOBBY_BGM_TO_AREA_MUSIC = 5,    /* channel 0 stops and the area's music plays, then LOBBY_BGM_WAITING */
    LOBBY_BGM_ENDED = 6             /* the show was stopped or never cued */
};

/* LobbyWorld.movieStep: LobbyMovie_Run's steps, which it returns. */
enum LobbyMovieStep {
    LOBBY_MOVIE_START = 0,
    LOBBY_MOVIE_PLAYING = 1,
    LOBBY_MOVIE_STOP = 2,
    LOBBY_MOVIE_DONE = 3
};

typedef struct LobbyWorld {
    /* 0x000 */ u8 unk0[4];
    /* 0x004 */ u32 lastVsync;      /* gSubVSyncCount when last counted (LobbyWorld_Tick) */
    /* 0x008 */ u32 frames;         /* frames counted */
    /* 0x00C */ f32 dayTicks;       /* LobbyWorld_GetTime's units per day */
    /* 0x010 */ f32 dayFrames;      /* frames per day */
    /* 0x014 */ f32 dayLen30;          /* the length of a second cycle (total30, dayPos30) */
    /* 0x018 */ f32 startTime;
    /* 0x01C */ s32 seasonDays[3];  /* the three seasons' lengths in days */
    /* 0x028 */ u32 startSeason;
    /* 0x02C */ s32 startDay;
    /* 0x030 */ s32 startYear;
    /* 0x034 */ f32 total30;          /* the second cycle's total */
    /* 0x038 */ f32 seconds;        /* seconds since the start */
    /* 0x03C */ u8 pad3C[4];
    /* 0x040 */ s32 days;           /* days since the start */
    /* 0x044 */ u8 pad44[4];
    /* 0x048 */ s32 years;          /* counted with year */
    /* 0x04C */ f32 dayPos30;          /* the second cycle's position */
    /* 0x050 */ f32 daySeconds;     /* the time of day in seconds (0-86400) */
    /* 0x054 */ f32 dayFrac;        /* daySeconds / 86400 */
    /* 0x058 */ s32 hour;
    /* 0x05C */ s32 day;            /* in the season */
    /* 0x060 */ s32 season;         /* 0-2 */
    /* 0x064 */ s32 year;
    /* 0x068 */ u8 pad68[8];
    /* 0x070 */ s32 pm;             /* 0 from hour 0, 1 from hour 12 */
    /* 0x074 */ s32 hold;           /* 1: the next count is skipped */
    /* 0x078 */ u8 active;          /* the music plays */
    /* 0x079 */ u8 streamState;     /* enum LobbyBgmState */
    /* 0x07A */ u8 cur;             /* the entry of the table selected (LobbyBgm_SelectEntry) */
    /* 0x07B */ u8 track;           /* LobbyBgm_Play's id */
    /* 0x07C */ u32 posVsync;
    /* 0x080 */ u32 pos;            /* the selected entry's position */
    /* 0x084 */ u32 startPos;          /* LobbyBgm_Play's position */
    /* 0x088 */ s32 startWait;
    /* 0x08C */ s32 streamStat;
    /* 0x090 */ s32 n;              /* the table's entries */
    /* 0x094 */ u32 endPos;
    /* 0x098 */ s32 startEntry;
    /* 0x09C */ u16 *files;         /* per entry, the stream file (LobbyBgm_Restart) */
    /* 0x0A0 */ u32 *table;         /* the entries' positions (LobbyBgm_FindEntry) */
    /* 0x0A4 */ f32 fade[3][4];     /* three colours (LobbyWorld_NextFadeKey) */
    /* 0x0D4 */ f32 fadeStep[3][4];
    /* 0x104 */ s16 fadeTime[3];
    /* 0x10A */ s16 fadeClock;         /* the time the fades' keys are compared with */
    /* 0x10C */ struct LobbyFadeKey ***fadeTbl; /* the track's key tables (gLobbyFadeTables) */
    /* 0x110 */ u16 glowAlpha;
    /* 0x112 */ s8 fadeOn[3];
    /* 0x115 */ s8 fadeSet;
    /* 0x116 */ s8 startStep;
    /* 0x117 */ s8 unk117;
    /* 0x118 */ s8 actorShown;
    /* 0x119 */ u8 forgetBgm;
    /* 0x11A */ s8 streaming;
    /* 0x11B */ u8 deferStart;
    /* 0x11C */ u8 playStep;          /* 1 started, 3 playing, 4 stopped */
    /* 0x11D */ u8 entry;          /* the entry playing (LobbyBgm_GetEntry) */
    /* 0x11E */ s8 unk11E;
    /* 0x11F */ u8 stageCam;
    /* 0x120 */ s8 unk120;
    /* 0x121 */ u8 propStep;
    /* 0x122 */ u8 pad122[2];
    /* 0x124 */ Em *actor;           /* the lobby NPC the music's motion scripts move (Npc_Init: LobbyBgm_SetActor; MusicActor_StartScript, MusicActor_Update, MusicActor_Draw) */
    /* 0x128 */ s8 movieStep;       /* enum LobbyMovieStep */
    /* 0x129 */ s8 movie;           /* Movie_Play's id less 2 (LobbyMovie_Prepare) */
    /* 0x12A */ s16 movieLen;       /* in frames (gRaidMovieLengths[movie]) */
    /* 0x12C */ s32 movieFrames;
    /* 0x130 */ LobbyWorldEvent events[4];
    /* 0x190 */ u8 unk190[112];
} LobbyWorld;

extern LobbyWorld gLobbyWorld;

/* a key of a fade (12 bytes): from its frame on, the colour goes from
 * `from` to `to` in `time` frames; a frame of -1 ends the list */
typedef struct LobbyFadeKey {
    /* 0x0 */ s16 frame;
    /* 0x2 */ s16 time;
    /* 0x4 */ u8 from[4];
    /* 0x8 */ u8 to[4];
} LobbyFadeKey;

/* the music tracks: per track its entries' positions (gLobbyBgmMarks), their
 * count and the track's length in frames */
typedef struct LobbyBgmInfo {
    /* 0x0 */ s32 n;
    /* 0x4 */ u32 len;
} LobbyBgmInfo;

extern u32 *gLobbyBgmMarks[];
extern LobbyBgmInfo gLobbyBgmInfo[];

extern s16 gRaidMovieLengths[];
extern LobbyFadeKey ***gLobbyFadeTables[];
extern s8 gLobbyWorldMovie;
extern s16 gLobbyWorldMovieLen;
extern s32 gLobbyWorldMovieFrames;
extern s32 gTownDayTicks;
extern u32 gRandGaussSum[];         /* sub_main 0x6AE390 */

/* gSubVSyncCount (0x6AE398), the count of vsyncs the display adds to,
 * has no symbol of its own: the game's code addresses it as an object of
 * its own, so it is reached through its neighbour gRandGaussSum */
#define gSubVSyncCount (gRandGaussSum[2])

extern u32 gTownDay;
extern u32 gTownSeason;
extern u32 gLobbyCycle2Pos;
extern u32 gLobbyTime;
extern s32 gTownSeasonDays[3];              /* the seasons lengths in days (the server mode) */
extern s32 gTownYear;                 /* the year (the server mode) */
extern u32 gLobbyBgmLens[][2];          /* gLobbyBgmInfo[].len through its own label */
extern u16 *gLobbyBgmFiles[];              /* per track, the stream file of each entry */
extern u8 gLobbyWorldActive;
extern f32 gLobbyWorldDayFrac;
extern f32 gLobbyWorldDayLen30;
extern s32 gLobbyWorldSeason;
extern s32 gLobbyWorldDay;
extern s32 gLobbyWorldHold;
extern s32 gLobbyWorldStreamStat;
extern u8 gLobbyWorldStreamState;
extern s8 gLobbyWorldStartStep;
extern s8 gLobbyWorldActorShown;
extern u8 gLobbyWorldStageCam;
extern s8 gLobbyWorldMovieStep;
extern u32 gSubVSyncCount;

/* sets the world's clock to `time` seconds of the day (taken modulo a day):
 * a day lasts gTownDayTicks seconds of play (86400 when unset); the calendar
 * starts from the server's season and day online, from the save's offline
 * (with its count of days), the year from gTownYear, each season lasting
 * gTownSeasonDays[] days (5 when unset). The counters start again from 0,
 * the events are cleared and the show ends. */
void LobbyWorld_SetClock(u32 time)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 i;
    s32 *days;

    if (gTownDayTicks == 0) {
        gTownDayTicks = LOBBY_DAY_SECONDS;
    }
    time %= LOBBY_DAY_SECONDS;
    world->dayTicks = (u32)gTownDayTicks;
    world->dayLen30 = 30.0f * world->dayTicks;
    world->dayFrames = world->dayTicks * gFps;
    world->startTime = time;
    world->dayFrac = (f32)time / (f32)LOBBY_DAY_SECONDS;
    if (Game_IsOnline() == 1) {
        world->startSeason = gTownSeason;
        world->startDay = gTownDay;
    } else {
        world->startDay = gSave.day;
        world->startSeason = gSave.season;
    }
    for (i = 0, days = gTownSeasonDays; i < 3; i++, days++) {
        if (*days == 0) {
            *days = 5;
        }
        world->seasonDays[i] = *days;
    }
    world->startYear = gTownYear;
    *(u32 *)world->unk0 = 0;
    world->lastVsync = 0;
    world->frames = 0;
    world->total30 = 0.0f;
    world->seconds = 0.0f;
    world->days = 0;
    world->years = 0;
    if (Game_IsOnline() != 1) {
        world->days = gSave.townDays;
    }
    world->day = world->startDay;
    world->season = world->startSeason;
    world->year = world->startYear;
    world->daySeconds = world->startTime;
    world->dayPos30 = world->dayFrac * world->dayLen30;
    world->hour = world->daySeconds / (f32)LOBBY_HOUR_SECONDS;
    if ((u32)world->hour < 12) {
        world->pm = 0;
    } else {
        world->pm = 1;
    }
    gSubVSyncCount = *(u32 *)world->unk0;
    gLobbyTime = world->daySeconds;
    gLobbyCycle2Pos = world->dayPos30;
    LobbyWorld_ClearEvents();
    LobbyBgm_End();
}

/* the next day: past the season's last day, the next season (past the
 * third, the next year); the save's (or the server mode's) copies follow */
void LobbyWorld_NextDay(void)
{
    LobbyWorld *world;
    u32 seasonLen;

    world = &gLobbyWorld;
    world->days++;
    world->day++;
    seasonLen = world->seasonDays[world->season];
    if (world->day >= seasonLen) {
        world->day -= seasonLen;
        world->season++;
        if ((u32)world->season >= 3) {
            world->season -= 3;
            world->years++;
            world->year++;
        }
        if (Game_IsOnline() != 1) {
            gSave.season = world->season;
        } else {
            gTownSeason = world->season;
        }
        LobbyWorld_OnNewSeason();
    }
    if (Game_IsOnline() != 1) {
        gSave.day = world->day;
        gSave.townDays = world->days;
    } else {
        gTownDay = world->day;
    }
}

/* advances the world's clock by d days: the seconds, the time of day (the
 * next day at 86400), the hour (VillageClock_Wobble each hour, LobbyWorld_OnHalfDay at 0
 * and 12), the finer day position (gLobbyCycle2Pos: 30 units a day tick,
 * starting again each day), the day's fraction; offline the save keeps the
 * time */
void LobbyWorld_AddDays(f32 days)
{
    LobbyWorld *world = &gLobbyWorld;
    f32 step;
    f32 period;
    f32 timeOfDay;
    s32 hourOfDay;

    period = (f32)LOBBY_DAY_SECONDS;
    step = days * period;
    world->seconds += step;
    timeOfDay = world->daySeconds + step;
    world->daySeconds = timeOfDay;
    while (timeOfDay >= period) {
        world->daySeconds -= period;
        LobbyWorld_NextDay();
        timeOfDay = world->daySeconds;
    }
    gLobbyTime = timeOfDay;
    hourOfDay = world->daySeconds / (f32)LOBBY_HOUR_SECONDS;
    if (world->hour != hourOfDay) {
        world->hour = hourOfDay;
        VillageClock_Wobble();
        if (world->hour == 0) {
            world->pm = 0;
            LobbyWorld_OnHalfDay();
        }
        if (world->hour == 12) {
            world->pm = 1;
            LobbyWorld_OnHalfDay();
        }
    }
    period = world->dayLen30;
    step = days * period;
    world->total30 += step;
    world->dayPos30 += step;
    if (world->dayPos30 >= period) {
        world->dayPos30 -= period;
    }
    gLobbyCycle2Pos = world->dayPos30;
    if (Game_IsOnline() != 1) {
        gSave.time = gLobbyTime;
    }
    gLobbyDayFrac = world->dayFrac = world->daySeconds / (f32)LOBBY_DAY_SECONDS;
}

/* once a frame: the frames counted since the last call move the clock on
 * (not while gTownDayTicks is 0; skipped once after `hold`), then the events */
void LobbyWorld_Tick(void)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 hold;
    u32 now;
    f32 elapsed;

    hold = world->hold;
    if (hold != 0) {
        world->hold = 0;
    }
    if (gTownDayTicks != 0 && world->lastVsync < (now = gSubVSyncCount)) {
        elapsed = now - world->lastVsync;
        world->lastVsync = now;
        if (hold == 0) {
            world->frames += (u32)elapsed;
            LobbyWorld_AddDays(elapsed / world->dayFrames);
            LobbyWorld_RunEvents();
        }
    }
}

/* t (of gTownDayTicks) as seconds of a day */
u32 LobbyWorld_TicksToSeconds(u32 ticks)
{
    f32 frac;

    if (gTownDayTicks == 0) {
        return 0;
    }
    frac = (f32)ticks / (f32)(u32)gTownDayTicks;
    frac *= (f32)LOBBY_DAY_SECONDS;
    return frac;
}

/* skips n seconds of play (n times gFps frames): the vsync counter and the
 * frames counted move on, and the clock advances by the counter's new value
 * in days, as the game's code does. The vsync counter is reached through a
 * named object, not a fixed address: the compiler then schedules its
 * address load as the game's code does */
void LobbyWorld_Skip(u32 seconds)
{
    LobbyWorld *world = &gLobbyWorld;
    u32 frameCount;
    u32 vsync;
    u32 skipFrames;

    skipFrames = seconds * gFps;
    vsync = gSubVSyncCount + skipFrames;
    frameCount = world->frames + skipFrames;
    gSubVSyncCount = vsync;
    world->frames = frameCount;
    world->lastVsync = vsync;
    LobbyWorld_AddDays(vsync / world->dayFrames);
}

/* catches the clock up with the last quest's time (the quest record's
 * elapsed time, in frames); then offline, once after the quest's results
 * went on to the opening movie (gSysSettings.openingAfterResults): when the
 * save's season or day changed meanwhile, 0.8 of a day more if the day is
 * past its first quarter; else on to 0.1 of a day into the next day */
void LobbyWorld_CatchUp(void)
{
    u8 day;
    u8 season;
    LobbyWorld *world;

    world = &gLobbyWorld;
    season = gSave.season;
    day = gSave.day;
    LobbyWorld_Skip(gQuestRecordElapsed / 30.0f);
    if (Game_IsOnline() != 1 && gSysSettings.openingAfterResults != 0) {
        gSysSettings.openingAfterResults = 0;
        if (season != gSave.season || day != gSave.day) {
            if (world->dayFrac > 0.25f) {
                LobbyWorld_AddDays(0.8f);
            }
            return;
        }
        LobbyWorld_AddDays(0.1f + (1.0f - world->dayFrac));
    }
}

s32 LobbyWorld_GetSeason(void)
{
    return gLobbyWorldSeason;
}

s32 LobbyWorld_GetSeasonDay(void)
{
    return gLobbyWorldDay;
}

void LobbyWorld_HoldClock(void)
{
    gLobbyWorldHold = 1;
}

f32 LobbyWorld_GetDayLen30(void)
{
    return gLobbyWorldDayLen30;
}

f32 LobbyWorld_GetDayFrac(void)
{
    return gLobbyWorldDayFrac;
}

void LobbyWorld_PlayAreaMusic(void)
{
    Bgm_StopInTownArena();
    Bgm_PlayAreaMusic(gScene.area);
}

/* selects the table entry nearest to position t (-1 past the last) */
static s32 LobbyBgm_NearestEntry(u32 pos)
{
    u32 *marks;
    s32 entry;
    u32 next;
    s32 i;
    LobbyWorld *world;
    s32 count;
    u32 start;
    u32 mid;

    world = &gLobbyWorld;
    marks = gLobbyWorld.table;
    entry = -1;
    if (pos >= marks[gLobbyWorld.n - 1] + (marks[gLobbyWorld.n] - marks[gLobbyWorld.n - 1]) / 2) {
        return entry;
    }
    count = world->n;
    for (i = 0; i < count; i++, marks++) {
        start = marks[0];
        next = marks[1];
        mid = start + (next - start) / 2;
        if (pos >= start && pos < mid) {
            entry = i;
            break;
        }
        if (pos >= mid && pos < next) {
            entry = i + 1;
            break;
        }
    }
    if (entry >= count || entry == -1) {
        return -1;
    }
    world->cur = entry;
    return entry;
}

/* cues track (4-7 are tracks 0-3 again) at position pos, or a running
 * stream's track at its own position (only while it waits or goes back to
 * the area's music): the track's files, entry positions, count and length
 * are taken and the nearest entry is chosen. Returns 1 when cued (the stream
 * waits, state 3), 2 when pos is past the track's end or no entry is near
 * (the show ends, state 6), 0 while a stream is busy */
s32 LobbyBgm_Cue(s32 track, u32 pos)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 entry;

    if (world->active != 0) {
        if (world->streamState != LOBBY_BGM_WAITING && world->streamState != LOBBY_BGM_TO_AREA_MUSIC) {
            return 0;
        }
        pos = world->pos;
    } else {
        if (track >= 4) {
            track -= 4;
        }
        world->pos = pos;
    }
    world->track = track;
    world->startPos = pos;
    world->posVsync = gSubVSyncCount;
    world->streaming = 0;
    world->files = gLobbyBgmFiles[track];
    world->table = gLobbyBgmMarks[track];
    world->n = gLobbyBgmInfo[track].n;
    world->endPos = gLobbyBgmLens[track][0];
    if (world->active != 0) {
        if (world->table[world->n - 1] <= world->startPos) {
            LobbyBgm_End();
            world->streamState = LOBBY_BGM_ENDED;
            return 2;
        }
    } else if (world->table[world->n] <= world->startPos) {
        LobbyBgm_End();
        world->streamState = LOBBY_BGM_ENDED;
        return 2;
    }
    entry = LobbyBgm_NearestEntry(pos);
    if (entry < 0 || entry >= world->n) {
        LobbyBgm_End();
        world->streamState = LOBBY_BGM_ENDED;
        return 2;
    }
    world->startEntry = entry;
    world->active = 1;
    world->stageCam = 0;
    if (world->active == 0) {
        return 0;
    }
    world->streamState = LOBBY_BGM_WAITING;
    return 1;
}

/* prepares the stream at the entry nearest to its position (the entry's
 * file, files[entry]); past the last entry it goes back to the area's
 * music; 1 if prepared */
s32 LobbyBgm_Restart(void)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 entry;

    if (world->active == 0) {
        return 0;
    }
    entry = LobbyBgm_NearestEntry(world->pos);
    if (entry >= world->n) {
        LobbyBgm_EndToArea();
        return 0;
    }
    Stream_Prepare(0, world->files[entry], 0, 0);
    return 1;
}

/* channel 0's stream status, kept in the world: 1 playing, -1 still being
 * prepared, 0 otherwise (stopped, at its end, or failed) */
s32 LobbyBgm_PollStat(void)
{
    s32 stat;

    stat = Stream_GetStat(0);
    gLobbyWorldStreamStat = stat;
    if (stat == ADXT_STAT_PLAYING) {
        return 1;
    }
    if ((stat < ADXT_STAT_PLAYING) && (stat != ADXT_STAT_STOP)) {
        return -1;
    }
    return 0;
}

/* starts the cued stream: LobbyBgm_Update fades the area's music out and
 * starts the stream at the cued entry (state 4); returns 1 */
s32 LobbyBgm_StartStream(void)
{
    gLobbyWorldStartStep = 0;
    gLobbyWorldStreamState = LOBBY_BGM_STARTING;
    return 1;
}

/* stops the stream */
void LobbyBgm_Stop(void)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active != 0) {
        world->startStep = 0;
        Stream_Stop(0);
        world->streamState = LOBBY_BGM_WAITING;
        world->playStep = 4;
        if (world->stageCam == 1) {
            CamReq_Cancel();
        }
        world->actorShown = 0;
    }
}

/* stops the stream, ends the show and plays the area's music again */
void LobbyBgm_EndToArea(void)
{
    gLobbyWorldActive = 0;
    gLobbyWorldStreamState = LOBBY_BGM_IDLE;
    Stream_Stop(0);
    LobbyWorld_ClearFades();
    LobbyWorld_PlayAreaMusic();
    LobbyBgm_ClearState();
}

/* ends the show (its state and fades cleared) without stopping the stream */
void LobbyBgm_End(void)
{
    gLobbyWorldActive = 0;
    gLobbyWorldStreamState = LOBBY_BGM_IDLE;
    LobbyWorld_ClearFades();
    LobbyBgm_ClearState();
}

/* stops the show's stream, and channel 0 even when no show runs; the actor
 * is hidden */
void LobbyBgm_StopAll(void)
{
    LobbyBgm_Stop();
    Stream_Stop(0);
    gLobbyWorldActorShown = 0;
}

/* once a frame: steps the stream's state (enum LobbyBgmState; in
 * LOBBY_BGM_STARTING the area's music fades out over 30 frames, channel 1
 * stops, and 32 frames later the entry's file is prepared and started when
 * ready). While a show runs its position follows the vsync counter, the
 * entry moves on with it, and at the track's length the show ends */
void LobbyBgm_Update(void)
{
    u32 now;
    LobbyWorld *world;
    s32 stat;
    s32 entry;

    now = gSubVSyncCount;
    world = &gLobbyWorld;
    stat = LobbyBgm_PollStat();
    switch (world->streamState) {
    case LOBBY_BGM_IDLE:
    case 1:
    case LOBBY_BGM_ENDED:
        break;
    case LOBBY_BGM_STREAMING:
        if (stat == 0) {
            LobbyBgm_EndToArea();
        }
        break;
    case LOBBY_BGM_WAITING:
        if (gScene.area == AREA_TOWN_ARENA && stat == 1) {
            LobbyBgm_Play(world->track, world->startPos, 0);
        }
        break;
    case LOBBY_BGM_STARTING:
        switch (world->startStep) {
        case 0:
            world->startStep++;
            world->forgetBgm = 0;
            if (world->streamStat == ADXT_STAT_PLAYING) {
                world->forgetBgm = 1;
            }
            Stream_FadeOut(0, 30);
            Stream_Stop(1);
            world->startWait = 32;
        case 1:
            if (--world->startWait == 0 && LobbyBgm_Restart() != 0) {
                if (world->forgetBgm == 1) {
                    Bgm_ForgetCurrent();
                }
                world->startStep++;
            }
            break;
        case 2:
            if (Stream_IsReady(0) != 0) {
                world->startStep = 0;
                world->posVsync = now;
                world->streaming = 1;
                world->streamState = LOBBY_BGM_STREAMING;
                LobbyBgm_SelectEntry(world->cur);
                Stream_StartPrepared(0);
            }
            break;
        }
        break;
    case LOBBY_BGM_TO_AREA_MUSIC:
        Stream_Stop(0);
        LobbyWorld_PlayAreaMusic();
        world->streamState = LOBBY_BGM_WAITING;
        break;
    }
    if (world->active != 0) {
        if (world->posVsync < now) {
            world->pos += now - world->posVsync;
            world->posVsync = now;
            entry = (u8)LobbyBgm_FindEntry(world->pos);
            if (world->cur < entry) {
                world->cur = entry;
            }
        }
        if (world->pos >= world->endPos) {
            if (world->streamState == LOBBY_BGM_STARTING || world->streamState == LOBBY_BGM_STREAMING) {
                LobbyBgm_EndToArea();
            } else {
                LobbyBgm_End();
            }
        }
    }
}

/* runs a show LobbyBgm_Play asked for, each town frame: step 1 cues the
 * track and starts it (or only cues it, deferStart); 2 waits for the stream,
 * then starts the actor's script, the area prop and the fades of entry 0;
 * 3 plays: the actor is shown from position 100, the area prop is spawned
 * again 100 frames before the end and the stage camera stops 50 before; at
 * each new entry the actor's script, the stage camera's shot (from entry 2)
 * and the fades start again; 4 waits for a cued stream to end; 5 and 6 wait
 * for the show to end and stop the stage camera. Returns 1 once the show has
 * ended */
s32 LobbyBgm_StepPlay(void)
{
    LobbyWorld *world;
    s32 done;
    s32 streamState;
    u32 pos;

    world = &gLobbyWorld;
    done = 0;
    switch (world->playStep) {
    case 1:
        world->playStep++;
        if (world->deferStart == 0) {
            if (LobbyBgm_Cue(world->track, world->startPos) == 2) {
                break;
            }
            LobbyBgm_StartStream();
        } else {
            if (LobbyBgm_Cue(world->track, world->startPos) == 2) {
                break;
            }
            world->startStep = 0;
            world->streamState = LOBBY_BGM_WAITING;
            world->playStep = 4;
            break;
        }
        if (world->active == 0) {
            world->playStep = 0;
        } else {
            world->entry = LobbyBgm_NearestEntry(world->pos);
        }
        break;
    case 2:
        streamState = LobbyBgm_GetState();
        if (streamState != LOBBY_BGM_STARTING && streamState != LOBBY_BGM_STREAMING) {
            LobbyBgm_EndToArea();
            done = 1;
            world->playStep = 5;
        } else if (streamState == LOBBY_BGM_STREAMING) {
            world->playStep++;
            world->entry = LobbyBgm_GetEntry();
            if (world->actor != NULL) {
                MusicActor_StartScript(world->actor, world->track, world->entry);
            }
            if (world->entry == 0) {
                AreaProp_Spawn(0);
                world->propStep = 1;
            }
            LobbyWorld_StartFades(world->track, world->entry);
            /* the playing step runs on this frame too */
            goto playing;
        }
        break;
    case 3:
        if (LobbyBgm_IsActive() == 0) {
            world->actorShown = 0;
            if (world->propStep == 1) {
                AreaProp_Spawn(0);
                world->propStep = 0;
            }
            LobbyBgm_End();
            done = 1;
            world->playStep = 5;
            break;
        }
    playing:
        pos = LobbyBgm_GetPos();
        if (world->propStep < 2 && pos >= 100) {
            world->actorShown = 1;
            world->propStep = 1;
        }
        if (world->propStep == 1 && pos >= LobbyBgm_GetEnd() - 100) {
            world->propStep = 2;
            AreaProp_Spawn(0);
        }
        if (world->propStep == 2 && pos >= LobbyBgm_GetEnd() - 50 && world->stageCam == 1) {
            CamReq_Cancel();
        }
        pos = LobbyBgm_GetEntry();
        if (world->entry != pos) {
            world->entry = pos;
            if (world->actor != NULL) {
                MusicActor_StartScript(world->actor, world->track, world->entry);
            }
            if ((s32)pos >= 2 && world->stageCam != 0) {
                CamReq_StageShot(world->track, world->entry - 2);
            }
            LobbyWorld_StartFades(world->track, world->entry);
        }
        if (world->actor != NULL) {
            MusicActor_Update(world->actor);
        }
        LobbyWorld_StepFades();
        break;
    case 4:
        if (LobbyBgm_IsActive() == 0) {
            LobbyBgm_End();
            done = 1;
            world->playStep = 5;
        }
        break;
    case 5:
        if (LobbyBgm_IsActive() != 1) {
            if (world->stageCam == 1) {
                CamReq_Cancel();
            }
            done = 1;
        }
        break;
    case 6:
        if (LobbyBgm_IsActive() != 1) {
            if (world->stageCam == 1) {
                CamReq_Cancel();
            }
            done = 1;
        }
        break;
    }
    if (Debug_IsOn(0) != 0 && Debug_IsOn(293) != 0) {
        LobbyBgm_DebugDraw();
    }
    return done;
}

/* the zero words after LobbyBgm_StepPlay's jump table, up to the end of the
 * overlay's .rodata */
const u32 D_00849F60[8] = {0};

/* the entry of the stream's table at position t (the last whose next entry
 * is past it), 0 if none */
u8 LobbyBgm_FindEntry(u32 pos)
{
    u32 *marks;
    s32 i;

    marks = gLobbyWorld.table;
    for (i = 0; i < gLobbyWorld.n; i++, marks++) {
        if (pos < marks[1]) {
            return i;
        }
    }
    return 0;
}

s32 LobbyBgm_IsActive(void)
{
    return gLobbyWorldActive;
}

s32 LobbyBgm_GetState(void)
{
    return gLobbyWorldStreamState;
}

/* the music show's position in its track, or 0 when no stream plays */
u32 LobbyBgm_GetPos(void)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active == 0) {
        return 0;
    }
    return world->pos;
}

/* the entry selected (LobbyBgm_SelectEntry), or -1 when no stream plays */
s32 LobbyBgm_GetEntry(void)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active == 0) {
        return -1;
    }
    return world->cur;
}

/* the music show's track, or -1 when no stream plays */
s32 LobbyBgm_GetTrack(void)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active == 0) {
        return -1;
    }
    return world->track;
}

/* 1 once the show is past its opening: the music actor is drawn */
u8 LobbyBgm_IsActorShown(void)
{
    return gLobbyWorldActorShown;
}

void LobbyBgm_DebugDraw(void)
{
}

/* selects entry i of the stream's table */
void LobbyBgm_SelectEntry(s32 i)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active != 0) {
        u32 *marks = world->table;
        world->cur = i;
        marks += i;
        world->pos = *marks;
    }
}

/* starts fade i with key set k of tbl: the first key at or after the fades'
 * clock sets the fade's colour and its step per frame towards the key's end
 * colour over the key's time; returns 1, or 0 when the list has no such key
 * (or no list: the fade is then cleared).
 * FAKEMATCH: the fade's time is written and read through a byte offset with
 * an unsigned cast, *(s16 *)((u8 *)s->fadeTime + (u32)(i * 2)). Written as
 * s->fadeTime[i], or through an s16 pointer, the compiler loads the key's
 * time first and every register moves up one. The key search tests
 * p->frame >= n twice because the game's code compares twice. The same form
 * makes LobbyWorld_StartFades and LobbyWorld_StepFades match too. */
static s32 LobbyWorld_NextFadeKey(s32 i, s32 keySet, LobbyFadeKey ***tbl)
{
    LobbyWorld *world = &gLobbyWorld;
    LobbyFadeKey *key;
    s32 j;
    s32 keyFrame;
    f32 fromRgba[4];
    f32 toRgba[4];

    if (tbl[i] == NULL) {
clear:
        world->fade[i][0] = 0.0f;
        world->fadeStep[i][0] = 0.0f;
        world->fade[i][1] = 0.0f;
        world->fadeStep[i][1] = 0.0f;
        world->fade[i][2] = 0.0f;
        world->fadeStep[i][2] = 0.0f;
        world->fade[i][3] = 0.0f;
        world->fadeStep[i][3] = 0.0f;
        return 0;
    }
    key = tbl[i][keySet];
    if (key == NULL) {
        /* into the clearing above: the game's code shares it */
        goto clear;
    }
    for (;;) {
        keyFrame = world->fadeClock;
        if (key->frame >= keyFrame && key->frame >= keyFrame) {
            break;
        }
        key++;
        keyFrame = key->frame;
        if (keyFrame == -1) {
            return 0;
        }
    }
    *(s16 *)((u8 *)world->fadeTime + (u32)(i * 2)) = key->time;
    for (j = 0; j < 4; j++) {
        fromRgba[j] = key->from[j];
        toRgba[j] = key->to[j];
        world->fade[i][j] = fromRgba[j];
        if (*(s16 *)((u8 *)world->fadeTime + (u32)(i * 2)) == 0) {
            world->fadeStep[i][j] = 0.0f;
        } else {
            world->fadeStep[i][j] = (toRgba[j] - fromRgba[j]) / *(s16 *)((u8 *)world->fadeTime + (u32)(i * 2));
        }
    }
    return 1;
}

/* starts track t's three fades with key set k (all cleared when the track
 * has none). LobbyWorld_NextFadeKey stays a static function of this file:
 * the compiler then knows which registers it leaves alone and keeps this
 * function's values in them across the call, as the game's code does */
void LobbyWorld_StartFades(s32 track, s32 keySet)
{
    LobbyWorld *world;
    LobbyFadeKey ***tbl;
    s32 i;

    gLobbyWorld.fadeClock = 0;
    gLobbyWorld.fadeSet = keySet;
    world = &gLobbyWorld;
    tbl = gLobbyFadeTables[track];
    gLobbyWorld.fadeTbl = tbl;
    if (tbl == NULL) {
        for (i = 0; i < 3; i++) {
            world->fadeOn[i] = 0;
            world->fade[i][0] = 0.0f;
            world->fadeStep[i][0] = 0.0f;
            world->fade[i][1] = 0.0f;
            world->fadeStep[i][1] = 0.0f;
            world->fade[i][2] = 0.0f;
            world->fadeStep[i][2] = 0.0f;
            world->fade[i][3] = 0.0f;
            world->fadeStep[i][3] = 0.0f;
        }
    } else {
        for (i = 0; i < 3; i++) {
            world->fadeOn[i] = LobbyWorld_NextFadeKey(i, keySet, tbl);
        }
    }
}

/* steps the fades: a running fade adds its steps until its time runs out,
 * then starts its next key (through the static LobbyWorld_NextFadeKey, for
 * the reason LobbyWorld_StartFades gives); the glow rises by 8 a frame to
 * 255 while entry 1 plays, and is 255 otherwise */
void LobbyWorld_StepFades(void)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 i;
    s32 j;

    gLobbyWorld.fadeClock++;
    i = 0;
    for (;;) {
        if (i >= 3) {
            break;
        }
        switch (world->fadeOn[i]) {
        case 0:
            break;
        case 1:
            if (--world->fadeTime[i] <= 0) {
                world->fadeOn[i] = LobbyWorld_NextFadeKey(i, world->fadeSet, world->fadeTbl);
            } else {
                for (j = 0; j < 4; j++) {
                    if (world->fadeStep[i][j] != 0.0f) {
                        world->fade[i][j] += world->fadeStep[i][j];
                    }
                }
            }
            break;
        }
        i++;
    }
    if (world->cur == 1) {
        world->glowAlpha += 8;
        if (world->glowAlpha >= 256) {
            world->glowAlpha = 255;
        }
    } else {
        world->glowAlpha = 255;
    }
}

/* fade i's colour as 0xRRGGBBAA (0 when the stream is idle) */
u32 LobbyWorld_GetFadeColor(s32 i)
{
    LobbyWorld *world = &gLobbyWorld;
    u8 rgba[4];
    s32 j;

    if (world->active == 0) {
        return 0;
    }
    for (j = 0; j < 4; j++) {
        rgba[j] = world->fade[i][j];
    }
    return (rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | rgba[3];
}

/* the alpha of the arena's glowing lights (0 while no show runs) */
u8 LobbyWorld_GetGlowAlpha(void)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active == 0) {
        return 0;
    }
    return world->glowAlpha;
}

/* asks for the show of track id from position pos (when pos is before the
 * track's end; LobbyBgm_StepPlay starts it); a playing show is stopped
 * first. deferStart nonzero: the track is only cued, to wait */
void LobbyBgm_Play(s32 trackId, s32 pos, s32 deferStart)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active != 0 && world->playStep == 3) {
        Stream_Stop(0);
        world->active = 0;
        world->streamState = LOBBY_BGM_IDLE;
    }
    LobbyWorld_ClearFades();
    world->propStep = 0;
    if (pos < gLobbyBgmMarks[trackId][gLobbyBgmInfo[trackId].n]) {
        world->playStep = 1;
        world->track = trackId;
        world->startPos = pos;
        world->deferStart = deferStart;
        world->unk120 = world->track % 3;
        if (trackId == 0 && pos >= 100) {
            world->actorShown = 1;
        }
    }
}

/* asks for the running show's track again from its current position */
void LobbyBgm_Replay(void)
{
    s32 track;

    if (gLobbyWorldActive != 0) {
        track = LobbyBgm_GetTrack();
        LobbyBgm_Play(track, LobbyBgm_GetPos(), 0);
    }
}

/* while a show runs, turns its stage camera on (on == 1) or off */
void LobbyBgm_SetStageCam(s32 enable)
{
    LobbyWorld *world = &gLobbyWorld;

    if (world->active != 0) {
        if (enable == 1) {
            world->stageCam = 1;
            CamReq_StageStart();
        } else {
            world->stageCam = 0;
            CamReq_Cancel();
        }
    }
}

s32 LobbyBgm_IsStageCamOn(void)
{
    return gLobbyWorldStageCam != 0;
}

void LobbyBgm_SetActor(Em *actor)
{
    gLobbyWorld.actor = actor;
}

Em *LobbyBgm_GetActor(void)
{
    return gLobbyWorld.actor;
}

/* the stream's end: the last entry of its table */
u32 LobbyBgm_GetEnd(void)
{
    return gLobbyBgmMarks[gLobbyWorld.track][gLobbyBgmInfo[gLobbyWorld.track].n];
}

/* 1 when track i, started now, would end before the day does */
s32 LobbyBgm_EndsToday(s32 track)
{
    f32 end;

    end = gLobbyBgmInfo[track].len / gLobbyWorld.dayFrames;
    end += gLobbyWorld.dayFrac;
    return !(end >= 1.0f);
}

/* the stream player's reset */
void LobbyWorld_Reset(void)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 i;

    world->active = 0;
    world->cur = 0;
    world->track = 0;
    world->posVsync = 0;
    world->pos = 0;
    world->startPos = 0;
    world->startWait = 0;
    world->streamStat = 0;
    for (i = 0; i < 3; i++) {
        world->fade[i][0] = 0.0f;
        world->fadeStep[i][0] = 0.0f;
        world->fade[i][1] = 0.0f;
        world->fadeStep[i][1] = 0.0f;
        world->fade[i][2] = 0.0f;
        world->fadeStep[i][2] = 0.0f;
        world->fade[i][3] = 0.0f;
        world->fadeStep[i][3] = 0.0f;
        world->fadeTime[i] = 0;
        world->fadeOn[i] = 0;
    }
    world->fadeClock = 0;
    world->fadeTbl = NULL;
    world->glowAlpha = 0;
    world->fadeSet = 0;
    world->startStep = 0;
    world->unk117 = 0;
    world->forgetBgm = 0;
    world->streaming = 0;
    world->deferStart = 0;
    world->playStep = 0;
    world->entry = 0;
    world->unk11E = 0;
    world->stageCam = 0;
    world->unk120 = 0;
    world->movieStep = LOBBY_MOVIE_START;
}

/* clears the three fades */
void LobbyWorld_ClearFades(void)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 i;

    for (i = 0; i < 3; i++) {
        world->fade[i][0] = 0.0f;
        world->fadeStep[i][0] = 0.0f;
        world->fade[i][1] = 0.0f;
        world->fadeStep[i][1] = 0.0f;
        world->fade[i][2] = 0.0f;
        world->fadeStep[i][2] = 0.0f;
        world->fade[i][3] = 0.0f;
        world->fadeStep[i][3] = 0.0f;
        world->fadeTime[i] = 0;
        world->fadeOn[i] = 0;
    }
    world->fadeClock = 0;
    world->fadeTbl = NULL;
    world->glowAlpha = 0;
    world->fadeSet = 0;
    world->unk117 = 0;
    world->stageCam = 0;
    world->unk120 = 0;
}

/* stops the stream and resets the show, its fades and the movie step */
void LobbyBgm_Shutdown(void)
{
    Stream_Stop(0);
    gLobbyWorldStreamState = LOBBY_BGM_IDLE;
    LobbyWorld_ClearFades();
    LobbyBgm_ClearState();
    LobbyWorld_Reset();
}

/* the world's time: the days since the start (the seasons' days times
 * the years, the days of this year's past seasons, the day) in dayTicks,
 * with the time of day */
static u32 LobbyWorld_GetTime(void)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 i;
    u32 time;
    s32 yearDays;

    yearDays = 0;
    for (i = 0; i < 3; i++) {
        yearDays += world->seasonDays[i];
    }
    time = 0;
    time += yearDays * world->year;
    for (i = 0; i < world->season; i++) {
        time += world->seasonDays[i];
    }
    time += world->day;
    time *= world->dayTicks;
    time += (u32)(world->dayTicks * world->dayFrac);
    return time;
}

/* event e's time, counted as LobbyWorld_GetTime counts */
static u32 LobbyWorld_EventTime(LobbyWorldEvent *event)
{
    LobbyWorld *world = &gLobbyWorld;
    s32 i;
    u32 time;
    s32 yearDays;

    yearDays = 0;
    for (i = 0; i < 3; i++) {
        yearDays += world->seasonDays[i];
    }
    time = 0;
    time += yearDays * event->year;
    for (i = 0; i < event->season; i++) {
        time += world->seasonDays[i];
    }
    time += event->day;
    time *= world->dayTicks;
    time += event->tick;
    return time;
}

/* clears the four events */
void LobbyWorld_ClearEvents(void)
{
    LobbyWorldEvent *event;
    s32 i;

    event = gLobbyWorld.events;
    i = 0;
    do {
        memset(event, 0, sizeof(LobbyWorldEvent));
        i++;
        event++;
    } while (i < 4);
}

/* calls the events whose time has come, once */
void LobbyWorld_RunEvents(void)
{
    LobbyWorldEvent *event;
    s32 i;
    u32 now;

    now = LobbyWorld_GetTime();
    event = gLobbyWorld.events;
    for (i = 0; i < 4; i++, event++) {
        if (event->active != 0 && now >= LobbyWorld_EventTime(event)) {
            if (event->func != NULL) {
                event->func();
            }
            event->active = 0;
            event->func = NULL;
        }
    }
}

/* puts the two events of ev (a start and an end) in free event slots and
 * sets flags[i] to 1 for each one taken. The first is left out (flags[0] =
 * 0) once 95 percent of the time between the two has passed, the second
 * (flags[1] = 0) from three days after its own time */
void LobbyWorld_ScheduleEvents(LobbyWorldEvent *event, s32 *scheduled)
{
    LobbyWorld *world = &gLobbyWorld;
    LobbyWorldEvent copy;
    s32 done[2];
    u32 end;
    LobbyWorldEvent *slot;
    u32 start;
    s32 j;
    s32 i;
    u32 mid;
    u32 now;

    start = LobbyWorld_EventTime(event);
    now = LobbyWorld_EventTime(event + 1);
    mid = (now - start) * 95 / 100;
    mid += start;
    end = now + (u32)(3.0f * world->dayTicks);
    done[1] = 0;
    done[0] = 0;
    now = LobbyWorld_GetTime();
    if (now >= mid) {
        done[0] = 1;
        scheduled[0] = 0;
    }
    if (now >= end) {
        done[1] = 1;
        scheduled[1] = 0;
    }
    for (i = 0; i < 2; i++, event++, scheduled++) {
        if (done[i] == 0) {
            copy = *event;
            for (slot = world->events, j = 0; j < 4; j++, slot++) {
                if (slot->active == 0) {
                    *slot = copy;
                    slot->active = 1;
                    *scheduled = 1;
                    break;
                }
            }
        }
    }
}

/* prepares the raid's movie (0 its start, 1 its end; LobbyMovie_Run plays
 * it): the show and the music stop, the movie's length in frames is taken
 * from gRaidMovieLengths, and the system data's event flag 46 + movie is set
 * (the movie was seen) */
void LobbyMovie_Prepare(s32 movie)
{
    LobbyBgm_Shutdown();
    Bgm_StopInTownArena();
    Stream_Stop(0);
    gLobbyWorldMovieStep = LOBBY_MOVIE_START;
    gLobbyWorldMovie = (s8) movie;
    gLobbyWorldMovieLen = gRaidMovieLengths[movie];
    gLobbyWorldMovieFrames = 0;
    EventFlag_Set(movie + 46);
}

void LobbyMovie_Stop(void)
{
    Movie_RequestStop();
    gLobbyWorldMovieStep = LOBBY_MOVIE_STOP;
}

/* plays movie `movie` + 2 (LobbyMovie_Prepare picks it): 0 start, 1 playing
 * until it ends, start is pressed or `movieLen` frames pass, 2 stop, 3 done;
 * returns the step */
s32 LobbyMovie_Run(void)
{
    LobbyWorld *world = &gLobbyWorld;

    switch (world->movieStep) {
    case LOBBY_MOVIE_START:
        Movie_Play(world->movie + 2);
        world->movieStep++;
        break;
    case LOBBY_MOVIE_PLAYING:
        Movie_Update();
        if (Movie_GetStatus() == MOVIE_STATUS_FINISHED || (gPadState[0].pressed & PAD_START)) {
            Movie_RequestStop();
            world->movieStep++;
        }
        break;
    case LOBBY_MOVIE_STOP:
        Movie_Stop();
        world->movieStep++;
        break;
    case LOBBY_MOVIE_DONE:
        break;
    }
    if (world->movieStep == LOBBY_MOVIE_PLAYING) {
        world->movieFrames++;
        if (world->movieFrames >= world->movieLen) {
            LobbyMovie_Stop();
        }
    }
    return world->movieStep;
}

/* The file's data: the show's tracks (each entry's stream file, the entries'
 * positions, their count and the track's length), the light fades' key
 * lists by track, fade and key set, and the raid movies' lengths */
u16 gLobbyBgmFiles_0[8] = {
#include "lobby/0081B650.inc"
};
u16 gLobbyBgmFiles_1[16] = {
#include "lobby/0081B660.inc"
};
u16 gLobbyBgmFiles_2[8] = {
#include "lobby/0081B680.inc"
};
u16 gLobbyBgmFiles_3[16] = {
#include "lobby/0081B690.inc"
};
u16 *gLobbyBgmFiles[4] = {
    gLobbyBgmFiles_0, gLobbyBgmFiles_1, gLobbyBgmFiles_2, gLobbyBgmFiles_3,
};
u32 gLobbyBgmMarks_0[12] = {
#include "lobby/0081B6C0.inc"
};
u32 gLobbyBgmMarks_1[12] = {
#include "lobby/0081B6F0.inc"
};
u32 gLobbyBgmMarks_2[12] = {
#include "lobby/0081B720.inc"
};
u32 gLobbyBgmMarks_3[16] = {
#include "lobby/0081B750.inc"
};
u32 *gLobbyBgmMarks[4] = {
    gLobbyBgmMarks_0, gLobbyBgmMarks_1, gLobbyBgmMarks_2, gLobbyBgmMarks_3,
};
LobbyBgmInfo gLobbyBgmInfo[4] = {
#include "lobby/0081B7A0.inc"
};
LobbyFadeKey gLobbyFadeTables_0_0_2[2] = {
#include "lobby/0081B7C0.inc"
};
LobbyFadeKey gLobbyFadeTables_0_0_3[2] = {
#include "lobby/0081B7E0.inc"
};
LobbyFadeKey gLobbyFadeTables_0_0_5[2] = {
#include "lobby/0081B800.inc"
};
LobbyFadeKey gLobbyFadeTables_0_0_6[4] = {
#include "lobby/0081B820.inc"
};
LobbyFadeKey gLobbyFadeTables_0_0_7[2] = {
#include "lobby/0081B850.inc"
};
LobbyFadeKey *gLobbyFadeTables_0_0[8] = {
    NULL, NULL, gLobbyFadeTables_0_0_2, gLobbyFadeTables_0_0_3,
    gLobbyFadeTables_0_0_3, gLobbyFadeTables_0_0_5, gLobbyFadeTables_0_0_6, gLobbyFadeTables_0_0_7,
};
LobbyFadeKey* *gLobbyFadeTables_0[4] = {
    gLobbyFadeTables_0_0, gLobbyFadeTables_0_0, gLobbyFadeTables_0_0, NULL,
};
LobbyFadeKey gLobbyFadeTables_1_0_2[2] = {
#include "lobby/0081B8A0.inc"
};
LobbyFadeKey gLobbyFadeTables_1_0_3[2] = {
#include "lobby/0081B8C0.inc"
};
LobbyFadeKey gLobbyFadeTables_2_0_3[2] = {
#include "lobby/0081B8E0.inc"
};
LobbyFadeKey gLobbyFadeTables_1_0_7[2] = {
#include "lobby/0081B900.inc"
};
LobbyFadeKey gLobbyFadeTables_1_0_8[2] = {
#include "lobby/0081B920.inc"
};
LobbyFadeKey gLobbyFadeTables_1_0_5[5] = {
#include "lobby/0081B940.inc"
};
LobbyFadeKey *gLobbyFadeTables_1_0[10] = {
    NULL, NULL, gLobbyFadeTables_1_0_2, gLobbyFadeTables_1_0_3,
    gLobbyFadeTables_1_0_3, gLobbyFadeTables_1_0_5, gLobbyFadeTables_1_0_3, gLobbyFadeTables_1_0_7,
    gLobbyFadeTables_1_0_8, NULL,
};
LobbyFadeKey* *gLobbyFadeTables_1[3] = {
    gLobbyFadeTables_1_0, gLobbyFadeTables_1_0, gLobbyFadeTables_1_0,
};
LobbyFadeKey gLobbyFadeTables_2_0_7[4] = {
#include "lobby/0081B9C0.inc"
};
LobbyFadeKey gLobbyFadeTables_2_0_2[2] = {
#include "lobby/0081B9F0.inc"
};
LobbyFadeKey gLobbyFadeTables_2_0_5[2] = {
#include "lobby/0081BA10.inc"
};
LobbyFadeKey gLobbyFadeTables_2_0_6[2] = {
#include "lobby/0081BA30.inc"
};
LobbyFadeKey *gLobbyFadeTables_2_0[8] = {
    NULL, NULL, gLobbyFadeTables_2_0_2, gLobbyFadeTables_2_0_3,
    gLobbyFadeTables_2_0_3, gLobbyFadeTables_2_0_5, gLobbyFadeTables_2_0_6, gLobbyFadeTables_2_0_7,
};
LobbyFadeKey* *gLobbyFadeTables_2[4] = {
    gLobbyFadeTables_2_0, gLobbyFadeTables_2_0, gLobbyFadeTables_2_0, NULL,
};
LobbyFadeKey gLobbyFadeTables_3_0_5[2] = {
#include "lobby/0081BA80.inc"
};
LobbyFadeKey gLobbyFadeTables_3_0_7[2] = {
#include "lobby/0081BAA0.inc"
};
LobbyFadeKey gLobbyFadeTables_3_0_9[2] = {
#include "lobby/0081BAC0.inc"
};
LobbyFadeKey gLobbyFadeTables_3_0_11[4] = {
#include "lobby/0081BAE0.inc"
};
LobbyFadeKey gLobbyFadeTables_3_0_12[2] = {
#include "lobby/0081BB10.inc"
};
LobbyFadeKey gLobbyFadeTables_3_0_6[2] = {
#include "lobby/0081BB30.inc"
};
LobbyFadeKey gLobbyFadeTables_3_0_10[2] = {
#include "lobby/0081BB50.inc"
};
LobbyFadeKey gLobbyFadeTables_3_0_8[2] = {
#include "lobby/0081BB70.inc"
};
LobbyFadeKey *gLobbyFadeTables_3_0[14] = {
    NULL, NULL, gLobbyFadeTables_1_0_2, gLobbyFadeTables_1_0_3,
    gLobbyFadeTables_1_0_3, gLobbyFadeTables_3_0_5, gLobbyFadeTables_3_0_6, gLobbyFadeTables_3_0_7,
    gLobbyFadeTables_3_0_8, gLobbyFadeTables_3_0_9, gLobbyFadeTables_3_0_10, gLobbyFadeTables_3_0_11,
    gLobbyFadeTables_3_0_12, NULL,
};
LobbyFadeKey* *gLobbyFadeTables_3[3] = {
    gLobbyFadeTables_3_0, gLobbyFadeTables_3_0, gLobbyFadeTables_3_0,
};
LobbyFadeKey** *gLobbyFadeTables[4] = {
    gLobbyFadeTables_0, gLobbyFadeTables_1, gLobbyFadeTables_2, gLobbyFadeTables_3,
};
s16 gRaidMovieLengths[8] = {
#include "lobby/0081BBF0.inc"
};
