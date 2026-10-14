#include "common.h"
#include "lobby_state.h"

extern s8 gLobbyScreenMode;
s32 func_006B5300(void);
extern void (*gLobbyScreenSteps[])(void);
extern LobbyMenu *gLobbyMenu;
void LobbyMenu_QueueLayers(void);

s32 func_007BC2F0(void)
{
    if (gLobbyScreenMode == 4) {
        return 1;
    }
    if (func_006B5300() == 1) {
        return 1;
    }
    return 0;
}

/* the screen's step (gLobbyScreenSteps[gLobbyScreenMode]), then the frame's drawing */
s32 LobbyScreen_RunStep(void)
{
    gLobbyScreenSteps[gLobbyScreenMode]();
    if (gLobbyMenu->hideLayers == 0) {
        LobbyMenu_QueueLayers();
    }
    return 0;
}

/* unk_7BC2F0's .data (0x819630-0x819650) */
void LobbyScreen_EnterArea(void);
void Town_Frame(void);
void LobbyScreen_Start(void);
void LobbyScreen_LeaveArea(void);
void LobbyScreen_CamIntro(void);
void (*gLobbyScreenSteps[8])(void) = {
    LobbyScreen_Start, LobbyScreen_EnterArea, Town_Frame, LobbyScreen_LeaveArea,
    LobbyScreen_CamIntro, NULL, NULL, NULL,
};
