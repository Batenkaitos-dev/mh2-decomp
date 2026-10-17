/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "menu_task.h"

/* prototypes */
void FadeA_Start(s32);
s32 LobbyNet_GetErrText(char *);
void Lobby_Disconnect(s32);
void LobbyNet_Get6105(Net6105 *dst);
void LobbyMenu_StartTimer(void);
s32 HunterCard_Sanitize(void *);
s32 LobbyNet_SetMe(char *id, char *name, u8 *text);
s32 LobbyNet_GetMyCharaId(char *dst);
s32 LobbyNet_GetMyCharaName(char *dst);
extern char gMyName[];             /* sub_main: the character's name (0x10) */
extern char gMyUserId[];             /* sub_main: the character's id (8) */
s32 LobbyNet_Get6149(u8 *out);
s32 LobbyReq_6148(u8 mode, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6149(u8 mode, u8 part, s32 data, u16 size, void (*onReply)(NetMsg, NetMsg *));
s32 Save_SummarySize(s32 mode);
void *Save_BuildSummary(s32 mode, s32 page);
s32 LobbyMenu_TickTimer(void);
void LobbyLogin_OnSummaryReply(NetMsg m, NetMsg *pm);

/* end of prototypes */

void Icon_DrawPreset(s32);
void LobbyTitle_Draw(void);
void LobbyMsg_Draw(void);
void LobbyEntry_DrawSprites(void);
s32 Gs_SetState(u8, u32);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetTexFilter(s32);
void Tex_Bind(s32);
void Text_SetSize(s32);
extern u8 gNetEntryMode;

s32 LobbyEntry_InStep9(void) {
    if ((gLobby->mode == 9) && (gLobby->phase == 0)) {
        return 1;
    }
    return 0;
}

/* the login task's progress (NetTask0's onDone): m.flag says how far it
 * got; kind -1 is a failure (7, 8: the server's error text, 9) */
void LobbyLogin_OnProgress(NetMsg m, NetMsg *pm)
{
    if (m.kind != -1) {
        switch (m.flag) {
        case 0xD:
            gLobby->mode = 9;
            gLobby->step = 0;
            break;
        case 4:
            gLobby->mode = 2;
            gLobby->step = 0;
            LobbyNet_Get6105((Net6105 *) &gLobby->msg6105);
            break;
        case 1:
            switch (gNetEntryMode) {
            case 0:
                gLobby->mode = 4;
                gLobby->step = 0;
                gLobby->subStep = 0;
                FadeA_Start(1);
                break;
            case 1:
            case 2:
                gLobby->mode = 4;
                gLobby->step = 6;
                gLobby->subStep = 0;
                LobbyMenu_StartTimer();
                HunterCard_Sanitize(gPlayerText.raw);
                LobbyNet_SetMe((char *) gLobby->myId, (char *) gLobby->myName, gPlayerText.raw);
                break;
            case 3:
                gLobby->mode = 4;
                gLobby->step = 6;
                gLobby->subStep = 0;
                LobbyMenu_StartTimer();
                HunterCard_Sanitize(gPlayerText.raw);
                LobbyNet_SetMe(gMyUserId, gMyName, gPlayerText.raw);
                break;
            }
            break;
        case 2:
            memset(gLobby->myId, 0, 8);
            LobbyNet_GetMyCharaId((char *) gLobby->myId);
            memset(gMyUserId, 0, 8);
            memcpy(gMyUserId, gLobby->myId, 8);
            memset(gLobby->myName, 0, 0x10);
            LobbyNet_GetMyCharaName((char *) gLobby->myName);
            memcpy(gMyName, gLobby->myName, 0x10);
            LobbyMenu_StartTimer();
            gLobby->step = 6;
            break;
        case 5:
            gLobby->mode = 1;
            gLobby->step = 0;
            break;
        case 3:
            gLobby->mode = 3;
            gLobby->step = 0;
            break;
        case 0:
            gLobby->mode = 6;
            gLobby->step = 0;
            break;
        }
    } else {
        switch (m.flag) {
        case 7:
            gLobby->mode = 5;
            gLobby->step = 0;
            gLobby->unk1D = 1;
            FadeA_Start(1);
            LobbyNet_GetErrText(gLobby->errText);
            break;
        case 8:
            gLobby->mode = 5;
            gLobby->step = 0;
            gLobby->unk1D = 2;
            LobbyNet_GetErrText(gLobby->errText);
            break;
        case 9:
            Lobby_Disconnect(5);
            break;
        }
    }
}

/* 1 if entry `item` (0-4) of the network menu may be chosen in the mode
 * gNetEntryMode (0: all, 3: up to 2 and 3-4, 2: 3-4, 1: 4 only) */
s32 Lobby_NetMenuAllowed(u8 item)
{
    switch (item) {
    case 0:
        return 1;
    case 1:
        if (gNetEntryMode == 0) {
            return 1;
        }
        break;
    case 2:
        if (gNetEntryMode == 0) {
            return 1;
        }
        if (gNetEntryMode == 3) {
            return 1;
        }
        break;
    case 3:
        if (gNetEntryMode == 0) {
            return 1;
        }
        if (gNetEntryMode == 3) {
            return 1;
        }
        if (gNetEntryMode == 2) {
            return 1;
        }
        break;
    case 4:
        if (gNetEntryMode == 0) {
            return 1;
        }
        if (gNetEntryMode == 3) {
            return 1;
        }
        if (gNetEntryMode == 2) {
            return 1;
        }
        if (gNetEntryMode == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* the same for the second menu's entries 0-3 (1: 1-3, 3: 2-3, 2: 3) */
s32 Lobby_NetMenu2Allowed(u8 item)
{
    switch (item) {
    case 0:
        return 0;
    case 1:
        if (gNetEntryMode == 1) {
            return 1;
        }
        break;
    case 2:
        if (gNetEntryMode == 1) {
            return 1;
        }
        if (gNetEntryMode == 3) {
            return 1;
        }
        break;
    case 3:
        if (gNetEntryMode == 1) {
            return 1;
        }
        if (gNetEntryMode == 3) {
            return 1;
        }
        if (gNetEntryMode == 2) {
            return 1;
        }
        break;
    }
    return 0;
}

/* sends the save's summary to the server at login, the part it asks for
 * each time: 6148 (the mode gNetEntryMode) first, then 6149 with part 21
 * (the equipment, Save_BuildSummary mode 1) or 1-20 (box page n - 1,
 * mode 2) until it asks for part 0. Returns 2 while running, 0 when done,
 * 1 when it failed */
s32 LobbyLogin_SendSummary(void)
{
    s32 page;
    void *data;
    s32 mode;
    s32 size;

    switch (gLobby->subStep) {
    case 0:
        gLobby->subStep++;
        gLobby->awaitReply = 10;
        LobbyMenu_StartTimer();
        LobbyReq_6148(gNetEntryMode, LobbyLogin_OnSummaryReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        switch (gLobby->subStep2) {
        case 0:
            gLobby->subStep = 0;
            gLobby->subStep2 = 0;
            return 0;
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:
        case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20:
            page = gLobby->subStep2 - 1;
            mode = 2;
            break;
        case 21:
            mode = 1;
            page = 0;
            break;
        default:
            gLobby->subStep = 0;
            gLobby->subStep2 = 0;
            return 1;
        }
        size = Save_SummarySize(mode);
        data = Save_BuildSummary(mode, page);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 11;
        LobbyReq_6149(gNetEntryMode, gLobby->subStep2, (s32)data, size, LobbyLogin_OnSummaryReply);
        gLobby->subStep = 1;
        break;
    case 99:
        gLobby->subStep = 0;
        gLobby->subStep2 = 0;
        return 1;
    default:
        gLobby->subStep = 0;
        gLobby->subStep2 = 0;
        return 1;
    }
    return 2;
}

/* the reply to 6148 and 6149 (awaited as 10 and 11): the part of the save
 * summary the server asks for next (unk393B), or 99 when it failed */
void LobbyLogin_OnSummaryReply(NetMsg m, NetMsg *pm)
{
    u8 *s;
    LobbyState *p = gLobby;
    u8 part;

    if (p->phase != 5) {
        s = &p->awaitReply;
        if (*s == 10 || *s == 11) {
            *s = 0;
            if (m.kind == 0) {
                LobbyNet_Get6149(&part);
                gLobby->subStep++;
                gLobby->subStep2 = part;
            } else {
                gLobby->subStep = 99;
            }
        }
    }
}

void LobbyEntry_Draw(MenuTask *task) {
    u8 mode;

    Text_SetSize(task->textSize);
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    if (((s32) gLobby->mode < 6) && (gNetEntryMode == 0)) {
        Icon_DrawPreset(2);
    } else {
        LobbyEntry_DrawSprites();
    }
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    mode = gLobby->mode;
    if ((mode == 4) && (mode == 3)) {
        LobbyTitle_Draw();
        LobbyMsg_Draw();
    }
}

void Lobby_MarkDisconnected(void) {
    gLobby->webStep = 0;
    gLobby->linkState = 1;
    gLobby->connected = 0;
}
