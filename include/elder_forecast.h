#ifndef ELDER_FORECAST_H
#define ELDER_FORECAST_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The elder dragon forecast and the raid's town movies. The forecast is a
 * town menu (TOWN_MODE_ELDER_FORECAST) of three rows: where the elder
 * dragons are this season, the townsperson's line about the raid, and the
 * downloaded event quests. ElderForecast_Open, ElderForecast_Update and
 * ElderForecast_Draw run it, and ElderForecast_Roll fills the season's quest
 * lists offline. ElderRaid_SetState and ElderRaid_CheckChanged turn the raid
 * on and off, each asking once for its town movie, which the town's update
 * starts and runs (LobbyTown_WantsMovie, LobbyTown_StartMovie,
 * LobbyTown_MovieDone).
 */

void ElderForecast_Open(void);
void ElderForecast_Close(void);
void ElderForecast_Update(void);
s32 ElderForecast_ListInput(ElderForecastMenu *menu, u16 pad);
void ElderForecast_Load(u16 *out, s32 season);
s32 ElderForecast_ViewInput(ElderForecastMenu *menu, u16 pad);
void ElderForecast_SayRaid(void);
s32 ElderForecast_MenuInput(ElderForecastMenu *menu, u16 pad);
void ElderForecast_DrawPanel(u16 *quests, s16 x, s16 y, s32 season, s32 isPm);
void ElderForecast_DrawQuest(ElderForecastMenu *menu, s16 x, s16 y);
void ElderForecast_Draw(MenuTask *task);
void ElderForecast_RollList(s32 season, u16 *out);
s32 ElderForecast_HasQuest(u16 id);
void ElderForecast_Roll(void);
void ElderRaid_SetState(void);
void ElderRaid_CheckChanged(void);
void LobbyTown_StartMovie(void);
s32 LobbyTown_MovieDone(void);
s32 LobbyTown_WantsMovie(void);
extern char gTimeLeftFormat[];

#endif
