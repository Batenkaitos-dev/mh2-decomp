#ifndef LOBBY_WORLD_H
#define LOBBY_WORLD_H

#include "common.h"
#include "em.h"
#include "lobby_state.h"

/*
 * The lobby world (gLobbyWorld): its clock and calendar (LobbyWorld_Tick
 * and the clock's queries), the town arena's music show (LobbyBgm_*) with
 * the light fades keyed by it (LobbyWorld_StartFades,
 * LobbyWorld_StepFades), the world's timed events
 * (LobbyWorld_ScheduleEvents, LobbyWorld_RunEvents) and the elder raid's
 * movies (LobbyMovie_*).
 */

void LobbyWorld_SetClock(u32 time);
void LobbyWorld_NextDay(void);
void LobbyWorld_AddDays(f32 days);
void LobbyWorld_Tick(void);
u32 LobbyWorld_TicksToSeconds(u32 ticks);
void LobbyWorld_Skip(u32 seconds);
void LobbyWorld_CatchUp(void);
s32 LobbyWorld_GetSeason(void);
s32 LobbyWorld_GetSeasonDay(void);
void LobbyWorld_HoldClock(void);
f32 LobbyWorld_GetDayLen30(void);
f32 LobbyWorld_GetDayFrac(void);
void LobbyWorld_PlayAreaMusic(void);
s32 LobbyBgm_PollStat(void);
s32 LobbyBgm_StartStream(void);
void LobbyBgm_Stop(void);
void LobbyBgm_EndToArea(void);
void LobbyBgm_End(void);
void LobbyBgm_StopAll(void);
void LobbyBgm_Update(void);
s32 LobbyBgm_StepPlay(void);
u8 LobbyBgm_FindEntry(u32 pos);
s32 LobbyBgm_IsActive(void);
s32 LobbyBgm_GetState(void);
u32 LobbyBgm_GetPos(void);
s32 LobbyBgm_GetEntry(void);
s32 LobbyBgm_GetTrack(void);
u8 LobbyBgm_IsActorShown(void);
void LobbyBgm_DebugDraw(void);
void LobbyBgm_SelectEntry(s32 i);
void LobbyWorld_StartFades(s32 track, s32 keySet);
void LobbyWorld_StepFades(void);
u32 LobbyWorld_GetFadeColor(s32 i);
void LobbyBgm_Play(s32 trackId, s32 pos, s32 deferStart);
void LobbyBgm_Replay(void);
void LobbyBgm_SetStageCam(s32 enable);
s32 LobbyBgm_IsStageCamOn(void);
void LobbyBgm_SetActor(Em *actor);
Em *LobbyBgm_GetActor(void);
u32 LobbyBgm_GetEnd(void);
s32 LobbyBgm_EndsToday(s32 track);
void LobbyWorld_Reset(void);
void LobbyWorld_ClearFades(void);
void LobbyBgm_Shutdown(void);
void LobbyWorld_ClearEvents(void);
void LobbyWorld_RunEvents(void);
void LobbyWorld_ScheduleEvents(LobbyWorldEvent *event, s32 *scheduled);
void LobbyMovie_Prepare(s32 movie);
void LobbyMovie_Stop(void);
s32 LobbyMovie_Run(void);

#endif
