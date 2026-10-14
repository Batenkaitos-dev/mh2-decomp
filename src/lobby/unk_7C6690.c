/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"

void *memset(void *dst, s32 c, u32 n);
void LobbyText_Open(s32 msg);
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
extern s8 gLobbyLevel1Sel;
extern u8 gLobbyScreenMode;
u8 SeqA_GetState(void);
void SeqA_Start(s32);
s32 LobbyLevel1_Leave(void);
s32 LobbyLevel1_Enter(void);
void LobbyLevel0_LeaveCountdown(void);
void LobbyVillage_BeginReenter(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Town_Reset(void);
void Lobby_Disconnect(s32);
s32 LobbyTown_WantsMovie(void);
void LobbyTown_StartMovie(void);
s32 LobbyTown_MovieDone(void);
s32 LobbyScreen_EnterSeq(s32);
u8 SeqB_GetState(void);
void SeqB_Start(s32);
s32 Lobby_HandleLeaveRequest(void);
void LobbyScene_Run(void);
void LobbyTown_Begin(void);
void Stream_Stop(s32 ch);
void LobbyVillage_Restart1(void);

s32 LobbyVillage_IsLeaving(void) {
    if ((gLobby->phase == 2) && (gLobby->mode == 4)) {
        return 1;
    }
    return 0;
}

/* leaving the lobby's first level for the village (mode its step): the
 * second level left (LobbyLevel1_Leave), the menus, the town
 * (LobbyTown_WantsMovie/LobbyTown_StartMovie, LobbyTown_MovieDone), or back in
 * (LobbyLevel1_Enter) */
void LobbyLevel1_ToVillage(void)
{
    switch (gLobby->mode) {
    case 0:
        gLobby->mode++;
        gLobby->step = 0;
        gLobby->seqStep = 0;
        if (SeqA_GetState() != 0) {
            SeqA_Start(0xC);
        }
        gLobbyMenu->redraw = 1;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel1_Leave()) {
        case 0:
            gLobby->mode++;
            if (gLobby->groupJoin != 0) {
                gLobby->mode = 5;
                LobbyVillage_BeginReenter();
            } else {
                LobbyMenu_SetDraw(0, 0, 0, 0);
                Town_Reset();
            }
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 2:
        if (LobbyTown_WantsMovie() == 1) {
            gLobby->mode = 6;
            LobbyTown_StartMovie();
        } else {
            gLobbyMenu->hideLayers = 1;
            switch (LobbyScreen_EnterSeq(gLobby->enterSeq)) {
            case 0:
                if (SeqB_GetState() != 0) {
                    SeqB_Start(0xC);
                }
                gLobbyScreenMode = 2;
                gLobby->mode++;
                break;
            }
        }
        break;
    case 3:
        if (Lobby_HandleLeaveRequest() == 0) {
            if (LobbyTown_WantsMovie() == 1) {
                gLobby->mode = 6;
                LobbyTown_StartMovie();
            } else if (gLobby->linkState == 0) {
                LobbyScene_Run();
            }
        }
        break;
    case 4:
        LobbyLevel0_LeaveCountdown();
        break;
    case 5:
        switch (LobbyLevel1_Enter()) {
        case 0:
            LobbyTown_Begin();
            Stream_Stop(0);
            Stream_Stop(1);
            break;
        case 1:
            gLobby->mode = 2;
            memset(gLobby, 0, 0x1C);
            break;
        }
        break;
    case 6:
        if (LobbyTown_MovieDone() == 1) {
            gLobby->movieState = 0;
            LobbyVillage_Restart1();
            gLobby->mode = 2;
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
}

void LobbyVillage_BeginReenter(void) {
    memset(gMenuWork, 0, 0xF00);
    LobbyText_Open(0x78);
    gLobbyMenu->redraw = 1;
    gLobbyLevel1Sel = (s8) gLobby->land;
}
