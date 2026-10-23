/*
 * The music manager (bgm.h): the field's state machine (Bgm_FieldUpdate: the area's music,
 * the battle music, the music of the goal met or the quest lost), the town's (Bgm_TownUpdate:
 * the music by season and time of day), the jingles and event tunes on the second stream
 * channel, and the areas' reverb. Its tables are this file's data, with the areas' and the
 * quests' sound tables that other code reads.
 *
 * The file ends with the sound cores' effects, which the areas' reverb sets through
 * Sound_SetEffectOnce (an effect set once per core until Sound_ResetEffects), with
 * Sound_ReverbOn and Sound_ReverbOff, and with Bgm_StopInTownArena, which clears gBgmId
 * in the town's arena. Their flags (gSoundEffectSet) lie beside gBgmId in memory.
 *
 * The file begins with Gfx_DrawMarker, which draws the window texture's marker icon: its
 * colours (gMarkerColors) lie among the music's tables, so no cut between the marker and the
 * music keeps each side's data together.
 */
#include "common.h"
#include "sprite.h"
#include "chat_log.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "syssettings.h"
#include "objhead.h"
#include "objattach.h"
#include "obj.h"
#include "window.h"
#include "render_state.h"
#include "area.h"
#include "hud.h"
#include "adx.h"
#include "sound.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "lobby_state.h"
#include "area_draw.h"
#include "kbd.h"
#include "save.h"
#include "text.h"
#include "village_cond.h"
#include "lobby_world.h"
#include "rand.h"
#include "libc.h"

extern Em gEms[];
extern u8 gBgmVars[];           /* the first of the music's variables, which Bgm_Init clears together */
extern BgmEntry gTownTuneEvents[];
extern BgmEntry gTownTuneSeasons[];
extern BgmEntry gTownTuneTimes[];
extern u16 gBgmOnlineVillage[];        /* id, volume pairs */
extern u16 gBgmHub84[];
extern u16 gBgmOnlineHouse[];
extern u16 gBgmFarm[];
extern u16 gBgmHouse[];        /* id, volume, volume */
extern BgmEntry gBgmTavernTracks[];
extern s16 gBgmTavernTrackFrames[];
extern AreaMusic gAreaMusic[];
extern AreaReverb gAreaReverbs[];
extern u8 gAreaReverbIdx[];
extern AreaMusicAlt gAreaMusicAlts[];
extern AreaJingles gAreaJingles[];
extern BgmKey gBattleBgmKinds[];
extern u16 gMapBattleBgm[];
extern BgmKey gAreaTunes[];
void Stream_Play(s32 a, s32 id);
void Stream_PlayMusicVol(s32 a, s32 id);
void Stream_PlayLevel(s32 a, s32 id, s32 vol);
/* unit's own declaration: frames and level as int (s16 in the definition): with s16 the calls would narrow them, which the game's calls don't */
void Stream_PlayFadeIn(s32 a, s32 id, s32 fade, s32 vol);
void Stream_Pause(s32 a, s32 b);
void Stream_Stop(s32 a);
void Stream_SetVol(s32 a, s32 b);
s32 Stream_GetStat(s32 a);
void Stream_FadeOut(s32 ch, s32 frames);
void Stream_FadeToMax(s32 ch, s16 frames);
/* unit's own declaration: frames and level as int (s16 in the definition): with s16 the calls would narrow them, which the game's calls don't */
void Stream_FadeTo(s32 a, s32 fade, s32 vol);
extern s32 gMarkerColors[];
extern u16 gFrameTick;
extern u8 gSoundEffectSet[2];        /* per SPU core: effect already set */

/*
 * Draws the window texture's marker icon in the rectangle of w by h at
 * (x, y), in the colour gMarkerColors[flags & MARKER_COLOR_MASK]. With
 * MARKER_ALERT a red cell is drawn over it; with MARKER_CUE a cue to its
 * right blinks (gFrameTick: off for 22 frames of 64, then short, then
 * long).
 */
void Gfx_DrawMarker(s16 x, s16 y, s16 width, s16 height, s32 flags)
{
    SpriteWords spr;
    s32 tick;

    spr.x = x;
    spr.y = y;
    spr.w = width;
    spr.h = height;
    spr.color = gMarkerColors[flags & MARKER_COLOR_MASK];
    spr.uv0 = TEX_UV(220, 237);
    spr.uv1 = TEX_UV(240, 255);
    Sprite_Draw((struct Sprite *)&spr);
    if (flags & MARKER_ALERT) {
        spr.color = 0xA0FF2828;
        spr.uv0 = TEX_UV(80, 197);
        spr.uv1 = TEX_UV(100, 215);
        Sprite_Draw((struct Sprite *)&spr);
    }
    if (flags & MARKER_CUE) {
        tick = gFrameTick & 0x3F;
        if (tick > 21) {
            spr.x += width;
            spr.color = -1;
            if (tick < 43) {
                spr.w >>= 2;
                spr.uv0 = TEX_UV(165, 197);
                spr.uv1 = TEX_UV(170, 215);
            } else {
                spr.w >>= 1;
                spr.uv0 = TEX_UV(172, 197);
                spr.uv1 = TEX_UV(182, 215);
            }
            Sprite_Draw((struct Sprite *)&spr);
        }
    }
}

/* Clears the music's variables and forgets the music playing. */
void Bgm_Init(void)
{
    memset(gBgmVars, 0, 10);
    gBgmId = BGM_NONE;
}

/*
 * What fight the local player's area holds, for the field's music:
 * FIGHT_LARGE_MONSTER when an active, visible large monster is in combat in
 * the player's area and not defeated (Chameleos only while enraged);
 * FIGHT_PLAYERS when a player in the scene's area is in battle; else
 * FIGHT_NONE, as always during an event, in the camp's area or in a quest
 * with gQuest.flags 0x600. Outside versus quests, a Khezu or a Red Khezu
 * anywhere in the quest gives FIGHT_NONE, and so does a Chameleos that is
 * not enraged.
 */
s32 Bgm_IsBattle(void)
{
    s16 i;
    s16 found = 0;
    Em *em = gEms;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (gScene.eventRunning != 0) {
        return 0;
    }
    if (gScene.campArea == pl->area) {
        return 0;
    }
    if (gQuest.flags & 0x600) {
        return 0;
    }
    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active == 0 || em->visible == 0) {
            continue;
        }
        if (Quest_IsVersus() == 0) {
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                return FIGHT_NONE;
            }
            if (em->type == EM_KIND_CHAMELEOS) {
                found = 1;
            }
        }
        if (gEmIsLarge[em->type] == 1 && em->stateGroup != EM_GROUP_DEFEAT && em->combat == 1 &&
            em->area == pl->area && em->type != 29) {
            if (em->type == EM_KIND_CHAMELEOS) {
                if (em->rage != 0) {
                    return FIGHT_LARGE_MONSTER;
                }
            } else {
                return FIGHT_LARGE_MONSTER;
            }
        }
    }
    if (found != 0) {
        return FIGHT_NONE;
    }
    for (pl = gPlayers, i = 0; i < gScene.playerCount; i++, pl++) {
        if (pl->active != 0 && pl->inBattle != 0 && pl->area == gScene.area) {
            return FIGHT_PLAYERS;
        }
    }
    return FIGHT_NONE;
}

/* 1 while a large monster's battle music plays or is about to. */
s32 Bgm_IsBattleState(void)
{
    if (gScene.musicState == MUSIC_MONSTER_FADE || gScene.musicState == MUSIC_MONSTER_BATTLE) {
        return 1;
    }
    return 0;
}

s16 Area_MonsterState(u16 area);

/*
 * The field's music, every frame (enum MusicState). Counts down
 * gScene.musicTimer, restores the music's volume once a tune on
 * BGM_CH_TUNE has ended, and moves to BGM_GOAL_MET when the goal is
 * met (Quest_GetOutcome(1) turns 1), to BGM_QUEST_LOST when the quest is
 * lost (-1). From the area's music, a fight
 * (Bgm_IsBattle) fades to BGM_BATTLE, and a change of the area's variant
 * (outside events) to the music for the new variant, with its jingle. The
 * battle music plays at least BGM_BATTLE_FRAMES; then a large monster's
 * fight moves on to its own music, the end of the fight back to the
 * area's.
 */
void Bgm_FieldUpdate(void)
{
    u8 state;
    s32 outcome;

    gBgmVariant = Area_MonsterState(gScene.area);
    if (gScene.musicTimer > 0) {
        gScene.musicTimer--;
    }
    if ((gScene.musicFlags & MUSIC_FLAG_DUCKED) && Stream_GetStat(BGM_CH_TUNE) >= ADXT_STAT_DECEND) {
        if (gScene.musicState >= MUSIC_BATTLE_FADE) {
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, BGM_VOL_MAX);
        } else {
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
        }
        gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
    }
    if (gScene.musicState < MUSIC_GOAL_MET_WAIT) {
        outcome = Quest_GetOutcome(1);
        if (outcome != 0) {
            if (outcome == 1) {
                if (gQuest.flags & 2) {
                    gScene.musicState = MUSIC_GOAL_MET_WAIT;
                    gScene.musicTimer = 100;
                } else {
                    Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                    gScene.musicTimer = BGM_FADE_FRAMES;
                    gScene.musicState = MUSIC_GOAL_MET_FADE;
                }
            } else {
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                gScene.musicTimer = BGM_FADE_FRAMES;
                gScene.musicState = MUSIC_LOST_FADE;
            }
        }
    }
    state = gScene.musicState;
    switch (state) {
    case MUSIC_AREA:
        if (Bgm_IsBattle() != FIGHT_NONE) {
            gScene.musicState = MUSIC_BATTLE_FADE;
            Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
            gScene.musicTimer = BGM_FADE_FRAMES;
        } else if (gBgmVariant != gBgmIndex && gScene.eventRunning == 0) {
            gScene.musicState++;
            Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
            gScene.musicTimer = BGM_FADE_FRAMES;
            Jingle_PlayForArea(gScene.area);
        }
        break;
    case MUSIC_VARIANT_FADE:
        if (gScene.musicTimer <= 0) {
            Bgm_EnterArea(gScene.area);
        }
        break;
    case MUSIC_BATTLE_FADE:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = state + 1;
            gScene.musicTimer = BGM_BATTLE_FRAMES;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_BATTLE, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_BATTLE;
        }
        break;
    case MUSIC_BATTLE:
        if (gScene.musicTimer <= 0) {
            switch (Bgm_IsBattle()) {
            case FIGHT_NONE:
                gScene.musicState = MUSIC_BATTLE_OVER;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                break;
            case FIGHT_PLAYERS:
                break;
            case FIGHT_LARGE_MONSTER:
                gScene.musicState++;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                gScene.musicTimer = BGM_FADE_FRAMES;
                break;
            }
        }
        break;
    case MUSIC_MONSTER_FADE:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = state + 1;
            gScene.musicTimer = BGM_BATTLE_FRAMES;
            Bgm_PlayBattle();
        }
        break;
    case MUSIC_MONSTER_BATTLE:
        if (gScene.musicTimer <= 0) {
            switch (Bgm_IsBattle()) {
            case FIGHT_NONE:
                gScene.musicTimer = BGM_FADE_FRAMES;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                gScene.musicState++;
                break;
            case FIGHT_PLAYERS:
                break;
            case FIGHT_LARGE_MONSTER:
                break;
            }
        }
        break;
    case MUSIC_BATTLE_OVER:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = MUSIC_AREA;
            Bgm_EnterArea(gScene.area);
        }
        break;
    case MUSIC_GOAL_MET_WAIT:
        if (gScene.musicTimer > 0) {
            break;
        }
        Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
        gScene.musicTimer = BGM_FADE_FRAMES;
        gScene.musicState = MUSIC_GOAL_MET_FADE;
    case MUSIC_GOAL_MET_FADE:
        if (gScene.musicTimer > 0) {
            break;
        }
        gScene.musicState++;
        Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_GOAL_MET, BGM_FADE_FRAMES, BGM_VOL_MAX);
        gBgmId = BGM_GOAL_MET;
    case MUSIC_GOAL_MET:
        if (Quest_GetOutcome(1) == -1) {
            gScene.musicState = MUSIC_LOST;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_LOST, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_QUEST_LOST;
        }
        break;
    case MUSIC_LOST_FADE:
        if (gScene.musicTimer > 0) {
            break;
        }
        gScene.musicState = state + 1;
        Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_LOST, BGM_FADE_FRAMES, BGM_VOL_MAX);
        gBgmId = BGM_QUEST_LOST;
    case MUSIC_LOST:
        if (Quest_GetOutcome(1) == 1) {
            gScene.musicState = MUSIC_GOAL_MET;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_GOAL_MET, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_GOAL_MET;
        }
        break;
    }
}

/*
 * The town's music, every frame, unless paused (gBgmPaused) or while the
 * lobby's music runs (LobbyBgm_GetState 2); the lobby's own update runs
 * after it. In AREA_TAVERN the six tracks of gBgmTavernTracks follow one
 * another, each after its time, in a random order with no track twice in a
 * row, a sound marking the change. Elsewhere, a new season or time of day
 * fades the music out and plays its tune (Bgm_PlayTownTune), then the
 * area's music again; in AREA_TOWN_ARENA the evening comes straight back to
 * the area's music. Nothing changes while a town tune plays over the music.
 */
void Bgm_TownUpdate(void)
{
    u8 track;
    u16 id;
    s32 vol;

    if (gBgmPaused == 0 && LobbyBgm_GetState() != 2) {
        if (gScene.musicTimer > 0) {
            gScene.musicTimer--;
        }
        if ((gScene.musicFlags & MUSIC_FLAG_DUCKED) && Stream_GetStat(BGM_CH_TUNE) >= ADXT_STAT_DECEND) {
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
        }
        if (Bgm_IsDuckingTune() != 1) {
            switch (gScene.musicState) {
            case MUSIC_TOWN_AREA:
                if (gScene.area == AREA_TAVERN) {
                    if (gScene.musicTimer <= 0) {
                        gScene.musicState++;
                        Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                        gScene.musicTimer = BGM_FADE_FRAMES;
                        Sound_PlaySeFlat(7, 24, 0);   /* the track's change */
                    }
                } else if (gBgmPm != (u8)LobbyTime_IsPm() || gBgmSeason != LobbyWorld_GetSeason()) {
                    gScene.musicState = MUSIC_TOWN_CHANGE;
                    Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                    gScene.musicTimer = BGM_FADE_FRAMES;
                }
                break;
            case MUSIC_TOWN_NEXT:
                if (gScene.musicTimer <= 0) {
                    if (gScene.area == AREA_TAVERN) {
                        gScene.musicState = MUSIC_TOWN_AREA;
                        track = Rand_Next(0) % 6;
                        if (track == gBgmIndex) {
                            gBgmIndex = (track + 1) % 6;
                        } else {
                            gBgmIndex = track;
                        }
                        id = gBgmTavernTracks[gBgmIndex].id;
                        vol = gBgmTavernTracks[gBgmIndex].vol;
                        gBgmVolume = vol;
                        if ((gScene.musicFlags & MUSIC_FLAG_DUCKED) && (gScene.bgmId == 40 || gScene.bgmId == 41)) {
                            vol = 0;
                        }
                        Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, vol);
                        gBgmId = id;
                        gScene.musicTimer = gBgmTavernTrackFrames[gBgmIndex];
                    } else {
                        Bgm_PlayAreaMusic(gScene.area);
                    }
                }
                break;
            case MUSIC_TOWN_CHANGE:
                if (gScene.musicTimer <= 0) {
                    if (gScene.area == AREA_TOWN_ARENA && gBgmPm == 0) {
                        gScene.musicState = MUSIC_TOWN_AREA;
                        Bgm_PlayAreaMusic(gScene.area);
                    } else {
                        gScene.musicState++;
                        if (gBgmSeason != LobbyWorld_GetSeason()) {
                            Bgm_PlayTownTune(TOWN_TUNE_SEASON, LobbyWorld_GetSeason());
                        } else if (gBgmPm != (u8)LobbyTime_IsPm()) {
                            Bgm_PlayTownTune(TOWN_TUNE_TIME, LobbyTime_IsPm());
                        }
                    }
                }
                break;
            case MUSIC_TOWN_TUNE:
                if (Stream_GetStat(BGM_CH_MUSIC) >= ADXT_STAT_DECEND) {
                    gScene.musicState = MUSIC_TOWN_AREA;
                    Bgm_PlayAreaMusic(gScene.area);
                }
                break;
            }
        }
    }
    LobbyBgm_Update();
}

/*
 * Plays the tune id on BGM_CH_TUNE when owner is the local player, and
 * notes it in gScene.bgmId. In the town, the town's tunes (4, BGM_TUNE_NPC and
 * 35 to 41) turn the music down under them, to 30 for tune 4 and to
 * silence for the others; outside it, a tune during the battle music (before the
 * goal is met or the quest lost) turns it down to 60.
 */
void Jingle_Play(void *owner, s32 id)
{
    ObjHead *head = owner;
    if (Player_IsLocal(head) == 1) {
        gScene.bgmId = id;
        if (gScene.inLobby != 0) {
            switch (id) {
            case 4:
                gScene.musicFlags |= MUSIC_FLAG_DUCKED;
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 30);
                Stream_PlayMusicVol(BGM_CH_TUNE, id);
                break;
            case BGM_TUNE_NPC:
            case 37:
            case 35:
            case 36:
            case 38:
            case 39:
            case 40:
            case 41:
                gScene.musicFlags |= MUSIC_FLAG_DUCKED;
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 0);
                Stream_PlayMusicVol(BGM_CH_TUNE, id);
                break;
            default:
                Stream_Play(BGM_CH_TUNE, id);
                break;
            }
        } else {
            Stream_Play(BGM_CH_TUNE, id);
            if (gScene.musicState >= MUSIC_BATTLE_FADE && Quest_GetOutcome(1) == 0) {
                gScene.musicFlags |= MUSIC_FLAG_DUCKED;
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 60);
            }
        }
    }
}

/* 1 while BGM_TUNE_NPC plays over the turned-down music. */
s32 Bgm_IsNpcTunePlaying(void)
{
    if (gScene.bgmId == BGM_TUNE_NPC && (gScene.musicFlags & MUSIC_FLAG_DUCKED)) {
        return 1;
    }
    return 0;
}

/* 1 while one of the town's tunes that turn the music down (Jingle_Play)
 * plays over it; Bgm_TownUpdate then changes nothing. */
s32 Bgm_IsDuckingTune(void)
{
    switch (gScene.bgmId) {
    case BGM_TUNE_NPC:
    case 4:
    case 37:
    case 35:
    case 36:
    case 38:
    case 39:
    case 40:
    case 41:
        if (gScene.musicFlags & MUSIC_FLAG_DUCKED) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 1 when gScene.bgmId is one of the event tunes, which Bgm_PlayEvent lets
 * finish and Bgm_StopEvent stops. */
s32 Bgm_IsEventTune(void)
{
    switch (gScene.bgmId) {
    case 13:
    case 30:
    case 25:
    case 32:
    case 31:
    case 19:
    case 22:
    case 4:
    case BGM_TUNE_NPC:
        return 1;
    }
    return 0;
}

/*
 * Plays the event tune id on BGM_CH_TUNE, unless an event tune is still
 * playing, and turns the music down to 30 under it until the
 * goal is met or the quest lost (Quest_GetOutcome(1) is 0).
 */
void Bgm_PlayEvent(s32 id)
{
    if (Bgm_IsEventTune() == 1 && Stream_GetStat(BGM_CH_TUNE) < ADXT_STAT_DECEND) {
        return;
    }
    Stream_Play(BGM_CH_TUNE, id);
    gScene.bgmId = id;
    if (Quest_GetOutcome(1) == 0) {
        gScene.musicFlags |= MUSIC_FLAG_DUCKED;
        Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 30);
    }
}

/*
 * For the local player owner: stops an event tune that plays, and
 * restores the music's volume if a tune turned it down (the area's volume,
 * or full during the field's battle music).
 */
void Bgm_StopEvent(void *owner)
{
    ObjHead *head = owner;
    if (Player_IsLocal(head) == 1) {
        if (Bgm_IsEventTune() == 1) {
            Stream_Stop(BGM_CH_TUNE);
        }
        if (gScene.musicFlags & MUSIC_FLAG_DUCKED) {
            gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
            if (gScene.inLobby != 0) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else if (gScene.musicState < MUSIC_BATTLE_FADE) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, BGM_VOL_MAX);
            }
        }
    }
}

/*
 * For the local player pl: stops the town tune that plays (some at once,
 * others with a 60-frame fade) and restores the music's volume, as
 * Bgm_StopEvent does.
 */
void Bgm_StopEventFade(Player *pl)
{
    if (Player_IsLocal(pl) == 1) {
        switch (gScene.bgmId) {
        case 13:
        case 30:
        case 4:
        case 307:
            Stream_Stop(BGM_CH_TUNE);
            break;
        case 37:
        case 35:
        case 40:
        case BGM_TUNE_NPC:
            Stream_FadeOut(BGM_CH_TUNE, 60);
            break;
        }
        if (gScene.musicFlags & MUSIC_FLAG_DUCKED) {
            gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
            if (gScene.inLobby != 0) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else if (gScene.musicState < MUSIC_BATTLE_FADE) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, BGM_VOL_MAX);
            }
        }
    }
}

/* Plays BGM_FAINT, except in a versus quest. */
void Bgm_PlayFaintMusic(void)
{
    if (Quest_IsVersus() != 1) {
        gScene.musicState = MUSIC_FAINT;
        Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_FAINT, BGM_FADE_FRAMES, BGM_VOL_MAX);
        gBgmId = BGM_FAINT;
    }
}

/* Pauses (nonzero) or resumes Bgm_TownUpdate. */
void Bgm_SetPaused(u8 paused)
{
    gBgmPaused = paused;
}

/* Forgets the music playing, so that the next one starts from the top. */
void Bgm_ForgetCurrent(void)
{
    gBgmId = BGM_NONE;
}

/*
 * Plays the town's tune i of the kind kind (TOWN_TUNE_*): a farm or village
 * event, a season, a time of day; the town's update returns to the area's
 * music when it ends. Notes the season and time of day it was chosen for.
 */
void Bgm_PlayTownTune(s32 kind, s32 i)
{
    u16 id;
    u16 vol;

    gScene.musicState = MUSIC_TOWN_TUNE;
    switch (kind) {
    case TOWN_TUNE_EVENT:
        id = gTownTuneEvents[i].id;
        vol = gTownTuneEvents[i].vol;
        break;
    case TOWN_TUNE_SEASON:
        id = gTownTuneSeasons[i].id;
        vol = gTownTuneSeasons[i].vol;
        break;
    case TOWN_TUNE_TIME:
    default:
        id = gTownTuneTimes[i].id;
        vol = gTownTuneTimes[i].vol;
        break;
    }
    Stream_PlayLevel(BGM_CH_MUSIC, id, vol);
    gBgmPm = LobbyTime_IsPm();
    gBgmSeason = LobbyWorld_GetSeason();
    gBgmId = id;
}

/*
 * Plays the music of the town's area area and sets its reverb. The music
 * is picked by the area: by season and time of day (AREA_ONLINE_VILLAGE,
 * AREA_HUB_0x84, AREA_TOWN_ARENA by day), by time of day (the houses), by the
 * village's level and time of day (the farm areas), a random one of six
 * tracks (AREA_TAVERN), or one tune; the elder dragon raid and the
 * village's state 4 have their own in AREA_ONLINE_VILLAGE, AREA_HUB_0x84 and
 * AREA_TOWN_ARENA. Music already playing is only faded to its volume.
 */
void Bgm_PlayAreaMusic(u16 area)
{
    u8 season;
    u8 isPm;
    u16 id;
    s32 vol;
    s32 row;

    gScene.musicState = MUSIC_TOWN_AREA;
    isPm = LobbyTime_IsPm();
    season = LobbyWorld_GetSeason();
    gBgmPm = isPm;
    gBgmSeason = season;
    switch (area) {
    case AREA_ONLINE_VILLAGE:
        gBgmId = 0;
        if (ElderRaid_IsOn() == 1) {
            id = 162;
            vol = 100;
        } else if (Town_IsSaleEvent() == 1) {
            id = 164;
            vol = 100;
        } else {
            row = season * 4 + isPm * 2;
            id = gBgmOnlineVillage[row];
            vol = gBgmOnlineVillage[row + 1];
        }
        break;
    case AREA_HUB_0x84:
        gBgmId = 0;
        if (ElderRaid_IsOn() == 1) {
            id = 161;
            vol = 100;
        } else if (Town_IsSaleEvent() == 1) {
            id = 163;
            vol = 100;
        } else {
            row = season * 4 + isPm * 2;
            id = gBgmHub84[row];
            vol = gBgmHub84[row + 1];
        }
        break;
    case AREA_HUB_0x85:
        id = 55;
        vol = 70;
        break;
    case AREA_TAVERN:
        id = gBgmTavernTracks[gBgmIndex].id;
        if (gBgmId == id) {
            Stream_Pause(BGM_CH_MUSIC, 0);
            Stream_SetVol(BGM_CH_MUSIC, 0);
            vol = gBgmTavernTracks[gBgmIndex].vol;
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, vol);
        } else {
            gBgmIndex = Rand_Next(0) % 6;
            id = gBgmTavernTracks[gBgmIndex].id;
            vol = gBgmTavernTracks[gBgmIndex].vol;
            Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, vol);
            gScene.musicTimer = gBgmTavernTrackFrames[gBgmIndex];
        }
        gBgmId = id;
        gBgmVolume = vol;
        break;
    case AREA_TOWN_ARENA:
        if (isPm == 0) {
            id = gBgmOnlineVillage[season * 4 + isPm * 2];
            vol = 30;
            gBgmId = 0;
            if (ElderRaid_IsOn() == 1) {
                id = 162;
            } else if (Town_IsSaleEvent() == 1) {
                id = 164;
            }
        } else {
            id = 176;
            vol = 100;
        }
        break;
    case AREA_ELDER_HALL:
    default:
        id = 306;
        vol = 90;
        break;
    case AREA_ONLINE_HOUSE_0:
        row = isPm * 2;
        id = gBgmOnlineHouse[row];
        vol = gBgmOnlineHouse[row + 1];
        break;
    case AREA_ONLINE_HOUSE_1:
        row = isPm * 2 + 4;
        id = gBgmOnlineHouse[row];
        vol = gBgmOnlineHouse[row + 1];
        break;
    case AREA_ONLINE_HOUSE_2:
        row = isPm * 2 + 8;
        id = gBgmOnlineHouse[row];
        vol = gBgmOnlineHouse[row + 1];
        break;
    case AREA_HUB_0x89:
        row = (u8)Village_AverageLevel();
        row = row * 4 + isPm * 2;
        id = gBgmFarm[row];
        vol = gBgmFarm[row + 1];
        break;
    case AREA_HUB_0xAA:
    case AREA_HUB_0xAB:
    case AREA_HUB_0xAC:
        row = (u8)Village_AverageLevel();
        row = row * 4 + isPm * 2;
        id = gBgmFarm[row];
        vol = (u32)gBgmFarm[row + 1] >> 1;
        break;
    case AREA_HUB_0x57:
        row = isPm * 3;
        id = gBgmHouse[row];
        vol = gBgmHouse[row + 1];
        break;
    case AREA_HOUSE:
        row = isPm * 3;
        id = gBgmHouse[row];
        vol = gBgmHouse[row + 2];
        break;
    }
    if (area != AREA_TAVERN) {
        if (gBgmId == id) {
            Stream_Pause(BGM_CH_MUSIC, 0);
            Stream_SetVol(BGM_CH_MUSIC, 0);
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, vol);
        } else {
            Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, vol);
        }
        gBgmId = id;
        gBgmVolume = vol;
    }
    Sound_SetEffectOnce(0, gAreaReverbs[gAreaReverbIdx[area]].mode0, gAreaReverbs[gAreaReverbIdx[area]].depth0, 0, 0);
    Sound_SetEffectOnce(1, gAreaReverbs[gAreaReverbIdx[area]].mode1, gAreaReverbs[gAreaReverbIdx[area]].depth1, 0, 0);
}

/* Fades the music in from silence to its volume over fade frames. */
void Bgm_FadeInFromZero(s32 fade)
{
    Stream_SetVol(BGM_CH_MUSIC, 0);
    Stream_FadeTo(BGM_CH_MUSIC, fade, gBgmVolume);
}

/* Fades the music to its volume over fade frames. */
void Bgm_FadeToVolume(s32 fade)
{
    Stream_FadeTo(BGM_CH_MUSIC, fade, gBgmVolume);
}

/* Plays the music id at volume vol. */
void Bgm_PlayWithVolume(s32 id, s32 vol)
{
    Stream_PlayLevel(BGM_CH_MUSIC, id, vol);
    gBgmId = id;
}

/*
 * Plays the battle music: BGM_VERSUS in a versus quest; in the fortress
 * the area's (another set while Shen Gaoren is there); elsewhere the first
 * active monster's own (gBattleBgmKinds), or the map's (gMapBattleBgm).
 */
void Bgm_PlayBattle(void)
{
    Em *em = gEms;
    u16 id;
    s32 i;
    s32 j;
    u16 found;

    if (Quest_IsVersus() == 1) {
        id = BGM_VERSUS;
    } else if (Map_DayId(gScene.map) != MAP_FORTRESS) {
        id = 65535;
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0) {
                for (j = 0; j < BATTLE_BGM_KINDS; j++) {
                    if (em->type == gBattleBgmKinds[j].key) {
                        id = gBattleBgmKinds[j].id;
                        break;
                    }
                }
                if (id != 0xFFFF) {
                    break;
                }
            }
        }
        if (id == 0xFFFF) {
            id = gMapBattleBgm[gScene.map];
        }
    } else {
        found = 0;
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->type == EM_KIND_SHEN_GAOREN) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            switch (Area_DayId(gScene.area)) {
            case AREA_FORTRESS_1:
                id = 188;
                break;
            case AREA_FORTRESS_2:
                id = 185;
                break;
            case AREA_FORTRESS_3:
                id = 186;
                break;
            case AREA_FORTRESS_4:
                id = 187;
                break;
            case AREA_FORTRESS_5:
                id = 180;
                break;
            default:
                id = 188;
                break;
            }
        } else {
            switch (Area_DayId(gScene.area)) {
            case AREA_FORTRESS_5:
                id = 209;
                break;
            default:
                id = 208;
                break;
            }
        }
    }
    Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, BGM_VOL_MAX);
    gBgmId = id;
}

/* The area's own tune (gAreaTunes), or 0 when it has none. */
s32 Bgm_AreaTune(u8 area)
{
    BgmKey *entry = gAreaTunes;
    u16 key = gAreaTunes[0].key;

    while (key != BGM_LIST_END) {
        if (key == area) {
            return entry->id;
        }
        entry++;
        key = entry->key;
    }
    return 0;
}

/* The field area's music for the area's variant (gAreaMusicAlts, else
 * gAreaMusic), with its volume put in gBgmVolume. */
u16 Bgm_GetAreaMusic(u16 area)
{
    AreaMusicAlt *alt = gAreaMusicAlts;
    u8 vol = gAreaMusic[area].vol;
    u16 id = gAreaMusic[area].id;

    if (gBgmVariant != 0) {
        while (alt->area != BGM_LIST_END) {
            if (alt->area == area && alt->variant == gBgmVariant) {
                id = alt->id;
                vol = alt->vol;
                break;
            }
            alt++;
        }
    }
    gBgmVolume = vol;
    return id;
}

/* Plays the area's jingle for the change of its variant, from the one the
 * music plays for (gBgmIndex) to the area's now (gBgmVariant), if it has
 * one. */
void Jingle_PlayForArea(u16 area)
{
    AreaJingles *areaList;
    VariantJingle *jingle;

    for (areaList = gAreaJingles;; areaList++) {
        if (areaList->area == BGM_LIST_END) {
            return;
        }
        if (areaList->area == area) {
            jingle = areaList->list;
            break;
        }
    }
    for (;; jingle++) {
        if (jingle->from == JINGLE_LIST_END) {
            return;
        }
        if (jingle->from == gBgmIndex && jingle->to == gBgmVariant) {
            Sound_PlaySeFlat(jingle->slot, jingle->se, 0);
            return;
        }
    }
}

/* Plays the music id at full volume and restarts the field's state machine
 * from the area's music. */
void Bgm_Play(s32 id)
{
    gScene.musicState = MUSIC_AREA;
    Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, BGM_VOL_MAX);
    gBgmId = id;
}

/* In a quest whose stage has a random start, entering an area of the hunt
 * (not the camp's) plays BGM_TUNE_RANDOM_START, until an area's own tune has
 * played. */
void Jingle_OnAreaChange(u16 area)
{
    if (gQuest.stage != 0 && gStageHeader->randomStart != 0 && !(gScene.musicFlags & MUSIC_FLAG_AREA_TUNE)) {
        switch (Map_DayId(gScene.map)) {
        case MAP_FOREST_HILLS:
        case MAP_DESERT:
        case MAP_SWAMP:
        case MAP_VOLCANO:
        case MAP_JUNGLE:
        case MAP_SNOWY_MOUNTAINS:
        case MAP_BATTLE_TOWN:
        case MAP_TOWER:
        case MAP_TOWER_B:
        case MAP_TOWER_C:
            if (gScene.campArea != area) {
                Jingle_Play(&gPlayers[gScene.localPlayer], BGM_TUNE_RANDOM_START);
            }
            break;
        }
    }
}

/*
 * Starts the field's music for the area entered, and sets its reverb.
 * Once the goal is met or the quest lost, BGM_GOAL_MET or BGM_QUEST_LOST
 * goes on (or starts). Otherwise: BGM_QUEST_THEME or the battle music in quests
 * with QUEST_FLAG_THEME_MUSIC or QUEST_FLAG_BATTLE_MUSIC; the area's own tune, the first time;
 * else the area's music for its variant. Music already playing is only
 * faded back in.
 */
void Bgm_EnterArea(u16 area)
{
    s32 outcome;
    s32 id;

    outcome = Quest_GetOutcome(1);
    if (outcome != 0) {
        if (outcome == 1) {
            if (gBgmId == BGM_GOAL_MET) {
                Stream_Pause(BGM_CH_MUSIC, 0);
                Stream_SetVol(BGM_CH_MUSIC, 0);
                Stream_FadeToMax(BGM_CH_MUSIC, BGM_FADE_FRAMES);
            } else {
                gScene.musicState = MUSIC_GOAL_MET;
                Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_GOAL_MET, BGM_FADE_FRAMES, BGM_VOL_MAX);
                gBgmId = BGM_GOAL_MET;
            }
        } else if (gBgmId == BGM_QUEST_LOST) {
            Stream_Pause(BGM_CH_MUSIC, 0);
            Stream_SetVol(BGM_CH_MUSIC, 0);
            Stream_FadeToMax(BGM_CH_MUSIC, BGM_FADE_FRAMES);
        } else {
            gScene.musicState = MUSIC_LOST;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_LOST, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_QUEST_LOST;
        }
    } else {
        if (gQuest.flags & QUEST_FLAG_THEME_MUSIC) {
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_THEME, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_QUEST_THEME;
            gBgmVolume = 127;
        } else if (gQuest.flags & QUEST_FLAG_BATTLE_MUSIC) {
            gBgmVolume = 127;
            Bgm_PlayBattle();
        } else if (gScene.musicFlags == 0 && (id = Bgm_AreaTune(area)) != 0) {
            Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = id;
            gBgmVolume = 127;
            gScene.musicFlags |= MUSIC_FLAG_AREA_TUNE;
        } else {
            id = Bgm_GetAreaMusic(area);
            if (gBgmId == id) {
                Stream_Pause(BGM_CH_MUSIC, 0);
                Stream_SetVol(BGM_CH_MUSIC, 0);
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else {
                Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, gBgmVolume);
            }
            gBgmId = id;
        }
        gScene.musicState = MUSIC_AREA;
    }
    gBgmIndex = gBgmVariant;
    Sound_SetEffectOnce(0, gAreaReverbs[gAreaReverbIdx[area]].mode0, gAreaReverbs[gAreaReverbIdx[area]].depth0, 0, 0);
    Sound_SetEffectOnce(1, gAreaReverbs[gAreaReverbIdx[area]].mode1, gAreaReverbs[gAreaReverbIdx[area]].depth1, 0, 0);
}

/* Reverb (effect mode 4) on both cores, unless an effect was already set. */
void Sound_ReverbOn(void)
{
    Sound_SetEffectOnce(0, 4, 4096, 0, 0);
    Sound_SetEffectOnce(1, 4, 2560, 0, 0);
}

void Sound_ReverbOff(void)
{
    Sound_SetEffectOnce(0, 4, 0, 0, 0);
    Sound_SetEffectOnce(1, 4, 0, 0, 0);
}

void Sound_ResetEffects(void)
{
    gSoundEffectSet[0] = 0;
    gSoundEffectSet[1] = 0;
}

/* Sets an SPU core's effect (Sound_SetEffect), unless one was set already
 * since Sound_ResetEffects. */
void Sound_SetEffectOnce(s32 core, s32 mode, u32 depth, s32 delay, s32 feedback)
{
    u8 *set = &gSoundEffectSet[core];

    if (*set == 0) {
        *set = 1;
        Sound_SetEffect(core, mode, depth, delay, feedback);
    }
}

/* In AREA_TOWN_ARENA, clears gBgmId to BGM_NONE; the lobby calls it.
 * Returns 0. */
s32 Bgm_StopInTownArena(void)
{
    if (gScene.area != AREA_TOWN_ARENA) {
        return 0;
    }
    gBgmId = BGM_NONE;
    return 0;
}

/* The file's data: the music's tables, the sound tables of the areas and quests, and the
 * marker's colours. */
BgmEntry gTownTuneEvents[4] = {
#include "sub_main/0043B140.inc"
};
BgmEntry gTownTuneSeasons[4] = {
#include "sub_main/0043B150.inc"
};
BgmEntry gTownTuneTimes[4] = {
#include "sub_main/0043B160.inc"
};
u16 gBgmOnlineVillage[16] = {
#include "sub_main/0043B170.inc"
};
u16 gBgmHub84[16] = {
#include "sub_main/0043B190.inc"
};
u16 gBgmOnlineHouse[16] = {
#include "sub_main/0043B1B0.inc"
};
u16 gBgmFarm[12] = {
#include "sub_main/0043B1D0.inc"
};
u16 gBgmHouse[6] = {
#include "sub_main/0043B1E8.inc"
};
BgmEntry gBgmTavernTracks[6] = {
#include "sub_main/0043B200.inc"
};
s16 gBgmTavernTrackFrames[6] = {
#include "sub_main/0043B218.inc"
};
AreaMusic gAreaMusic[200] = {
#include "sub_main/0043B230.inc"
};
AreaReverb gAreaReverbs[10] = {
#include "sub_main/0043B550.inc"
};
u8 gAreaReverbIdx[208] = {
#include "sub_main/0043B5F0.inc"
};
s16 gAreaSeBanks[256] = {
#include "sub_main/0043B6C0.inc"
};
AreaMusicAlt gAreaMusicAlts[68] = {
#include "sub_main/0043B8C0.inc"
};
VariantJingle gAreaJingles_0[8] = {
#include "sub_main/0043BAE0.inc"
};
VariantJingle gAreaJingles_20[8] = {
#include "sub_main/0043BB00.inc"
};
VariantJingle gAreaJingles_14[8] = {
#include "sub_main/0043BB20.inc"
};
VariantJingle gAreaJingles_27[8] = {
#include "sub_main/0043BB40.inc"
};
AreaJingles gAreaJingles[35] = {
#include "sub_main/0043BB60.inc"
};
u16 gWeaponSeBanks[344] = {
#include "sub_main/0043BC80.inc"
};
s8 gArmorRattleKinds_0[224] = {
#include "sub_main/0043BF30.inc"
};
s8 gArmorRattleKinds_1[224] = {
#include "sub_main/0043C010.inc"
};
s8 gArmorRattleKinds_6[256] = {
#include "sub_main/0043C0F0.inc"
};
s8 gArmorRattleKinds_7[256] = {
#include "sub_main/0043C1F0.inc"
};
s8 gArmorRattleKinds_8[256] = {
#include "sub_main/0043C2F0.inc"
};
s8 gArmorRattleKinds_9[256] = {
#include "sub_main/0043C3F0.inc"
};
s8 gArmorRattleKinds_10[224] = {
#include "sub_main/0043C4F0.inc"
};
s8 gArmorRattleKinds_11[224] = {
#include "sub_main/0043C5D0.inc"
};
s8 *gArmorRattleKinds[12] = {
#include "sub_main/0043C6B0.inc"
};
/* gEmSeBankFiles and gSeBankByType, by monster kind: the kind's sound bank
 * file after SND_BANK_EM (SndLoad_EmBanks), and the bank its sounds play
 * from (Em_PlaySeType). */
s8 gEmSeBankFiles[80] = {
#include "sub_main/0043C6E0.inc"
};
s8 gSeBankByType[80] = {
#include "sub_main/0043C730.inc"
};
s8 gSeVolumeByType[80] = {
#include "sub_main/0043C780.inc"
};
BgmKey gBattleBgmKinds[20] = {
#include "sub_main/0043C7D0.inc"
};
s32 gMarkerColors[4] = {
#include "sub_main/0043C820.inc"
};
u16 gMapBattleBgm[8] = {
#include "sub_main/0043C830.inc"
};
u32 gTextCodeColors[12] = {
#include "sub_main/0043C840.inc"
};
BgmKey gAreaTunes[20] = {
#include "sub_main/0043C870.inc"
};
