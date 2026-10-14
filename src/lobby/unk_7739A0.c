#include "common.h"
#include "lobby_globals.h"

s32 Lobby_CheckDay(s32);
s32 Lobby_GetDayState(void);
void Stream_Pause(s32, s32);
s32 Game_IsOnline(void);

s32 GameMenu_OpenPause(void *unused0)
{
    if ((Game_IsOnline() != 1) && (Lobby_GetDayState() == 0)) {
        gHelpShow = 0;
        gPauseMenuReq = 1;
        Lobby_CheckDay(1);
        gChatWinOpen = gChatWinTimer = 0;
        Stream_Pause(1, 1);
        Stream_Pause(0, 1);
        return 0;
    }
    return 1;
}

u16 GameMenu_UpdatePause(u16 pad)
{
    if (pad & 0x8060) {
        Lobby_CheckDay(0);
        Stream_Pause(1, 0);
        Stream_Pause(0, 0);
        return 0x8000;
    }
    return 0;
}
