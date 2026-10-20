/*
 * The elder dragon raid's end: ElderRaid_CheckChanged, the world event the
 * raid's second time calls (Lobby_ApplyTownData schedules it). Its start,
 * ElderRaid_SetState, and the forecast that announces it come just before;
 * the town movies they ask for are played by LobbyTown_StartMovie, just after.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "save.h"

extern ElderRaidEvent gElderRaidEvent;

/* The raid is over: if its end movie hasn't been played for this raid and
 * the raid was on with its movies, asks for the end movie; the raid is off. */
void ElderRaid_CheckChanged(void)
{
    if (gElderRaidEvent.id != gSave.raidEndMovieId) {
        if (gLobby->elderRaidState == 1) {
            gLobby->movieState = RAID_MOVIE_END;
        }
        gLobby->elderRaidState = 0;
    }
}
