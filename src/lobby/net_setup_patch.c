/*
 * The network mode's longer set-up steps, and the patch's install at login.
 *
 * - The set-up's steps, which NetMode_RunSetup calls by gNetMode.mode: the
 *   network mode's music (NetMode_PlayBgm), the network overlays' loads
 *   (NetOverlay_Request: nethttp, DNAS_NET, DNAS_INS), the deletion of the
 *   saved login id and password with the NET file saved again
 *   (NetSetup_DeleteAccount), the DNAS check (NetSetup_Dnas) and the NET
 *   file's save (NetSetup_SaveNetFile). Each is a step function the caller
 *   runs once a frame until it reports its end.
 * - The patch's install at login (LobbyPatch_Install, which LobbyEntry_Patch
 *   runs): when the server announced a patch (LobbyState.patchInfo), the
 *   player is asked, the DNAS check runs, the patch is decoded through the
 *   DNAS install library, saved to the memory card with the extra file,
 *   copied to the patch area and applied (Patch_ExecAll).
 *
 * Both work the network screen's boxes, titles and messages (net_mode.h)
 * through gNetMode. The NET file's card steps and the message pages come
 * before this file; the login's stages after it.
 */
#include "common.h"
#include "lobby_globals.h"
#include "net_save.h"
#include "card.h"
#include "syssettings.h"
#include "net_mode.h"
#include "lobby_vobj.h"
#include "ram_areas.h"
#include "pad.h"
#include "overlay.h"
#include "sound.h"
#include "text.h"
#include "dnas_check.h"
#include "net_file_save.h"
#include "game_util.h"

extern u8 gLobbySaveBuf[];                /* the NET file id and password, saved (512 bytes) */
void *memcpy(void *dst, const void *src, u32 n);
char *strncpy(char *dst, const char *src, u32 n);  /* strncpy */
extern s32 gPatchCardSlot;
void *memset(void *dst, s32 c, u32 n);
u8 NetMenu_Update(void);

/* Starts the network mode's music: sound 0 of the bank in slot 6, at the
 * options' music volume (NetMode_BgmOn calls it while it isn't playing). */
void NetMode_PlayBgm(void)
{
    Sound_PlaySeMusicVol(6, 0, 0);
}

/* Loads the network overlay of `kind` (0 nethttp, 1 DNAS_NET, 2 DNAS_INS;
 * 3 loads none) unless a load is under way (NetOverlay_Busy, which always
 * says no). Returns 1 when the request was made, else 0. */
s32 NetOverlay_Request(s32 kind)
{
    s32 result;

    result = 0;
    if (NetOverlay_Busy() == 0) {
        switch (kind) {
        case 0:
            NetOverlay_Load(OVERLAY_NETHTTP);
            break;
        case 1:
            NetOverlay_Load(OVERLAY_DNAS_NET);
            break;
        case 2:
            NetOverlay_Load(OVERLAY_DNAS_INS);
            break;
        case 3:
            break;
        }
        result = 1;
    }
    return result;
}

/* 1 when no overlay load is under way (NetOverlay_Busy), else 0. The
 * argument (the kind requested) is not read. */
s32 NetOverlay_Ready(s32 unused)
{
    s32 ret;

    ret = 0;
    if (NetOverlay_Busy() == 0) {
        ret = 1;
    }
    return ret;
}

/*
 * The settings menu's deletion of the saved MMBB login (the id and the
 * password in gNetSaveData), one step a frame (gNetMode.subStep). With
 * `mode` 0 the two are first kept in gLobbySaveBuf (the id at 0, the
 * password at +256) and the yes/no box asks (message 38, the cursor on
 * no); yes clears them and saves the NET file (LobbyCard_SaveNetFile), and
 * a failed save puts them back. Mode 1 would keep them in gSaveBuf, but
 * no step follows its first (the only caller passes 0).
 * Returns 1 when saved, 2 when no was chosen, -1 when the save failed,
 * else 0.
 */
s32 NetSetup_DeleteAccount(s32 mode)
{
    NetModeState *net;
    u8 *savedId;
    u8 *savedPassword;
    s32 result;

    net = &gNetMode;
    result = 0;
    NetMode_SetTitle(NET_TITLE_ACCOUNT);
    if (mode == 0) {
        savedId = (u8 *)gLobbySaveBuf;
        savedPassword = savedId + 256;
    } else {
        savedId = *(u8 * *)&gSaveBuf;
        savedPassword = savedId + 256;
    }
    LoginPad_Triggered();
    switch (net->subStep) {
    case 0:
        net->innerStep = 0;
        if (mode == 1) {
            net->subStep++;
        } else {
            memcpy(savedId, gNetSaveData.mmbbLoginId, sizeof(gNetSaveData.mmbbLoginId));
            memcpy(savedPassword, gNetSaveData.mmbbPassword, sizeof(gNetSaveData.mmbbPassword));
            net->subStep = 3;
            net->slot = gSysSettings.cardSlot;
            NetMode_OpenYesNo(1);
        }
        break;
    case 1:
    case 2:
        break;
    case 3:
        NetModeMsg_Page1(NETMODE_MSG_ASK_DELETE_ID);
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            NetMode_CloseYesNo();
            memset(gNetSaveData.mmbbLoginId, 0, sizeof(gNetSaveData.mmbbLoginId));
            memset(gNetSaveData.mmbbPassword, 0, sizeof(gNetSaveData.mmbbPassword));
            net->subStep = 4;
            net->innerStep = 0;
            NetMode_BeginCardTask(1);
            gSysBusy = 1;
            break;
        case -1:
            NetMode_CloseYesNo();
            result = 2;
            break;
        }
        break;
    case 4:
        switch (LobbyCard_SaveNetFile()) {
        case 0:
            break;
        case 1:
            result = 1;
            gSysBusy = 0;
            break;
        case -1:
            memcpy(gNetSaveData.mmbbLoginId, savedId, sizeof(gNetSaveData.mmbbLoginId));
            memcpy(gNetSaveData.mmbbPassword, savedPassword, sizeof(gNetSaveData.mmbbPassword));
            result = -1;
            gSysBusy = 0;
            break;
        }
        break;
    }
    return result;
}

/* Counts in *sel2 the frames Start is held on pad 0 (0 once it is let
 * go). Returns 1 from the 30th frame (1 s), else 0: the DNAS steps then
 * cancel the check. */
s32 LoginPad_StartHeldLong(s16 *heldFrames)
{
    s32 ret;

    ret = 0;
    if (LoginPad_Held(0) & PAD_START) {
        *heldFrames += 1;
        if (*heldFrames >= 30) {
            ret = 1;
        }
    } else {
        *heldFrames = 0;
    }
    return ret;
}

/*
 * The set-up's DNAS check (NET_SETUP_DNAS), one step a frame
 * (gNetMode.step): opens the percent, icon and progress boxes, loads the
 * DNAS_NET overlay and runs Dnas_AuthStep (holding Start 30 frames
 * cancels it: the cancel count is set past NET_CANCEL_AT). Passed (steps
 * 11, 20-22), the boxes close and the sequence ends; failed (15, 9, 8,
 * 100-103), the error code (Dnas_GetError) shows with the error icon and
 * message 20 until a button. gNetMode.unk7E is 1 meanwhile: the error
 * page then adds its extra line. gNetMode.netResult is NET_RESULT_RUNNING,
 * then NET_RESULT_FAILED or NET_RESULT_DONE. Returns 1 when passed, -1 after the failure's message, else 0.
 */
s32 NetSetup_Dnas(void)
{
    s32 result;
    NetModeState *net;
    s32 authResult;

    result = 0;
    net = &gNetMode;
    switch (net->step) {
    case 0:
        net->step++;
        net->percent = 0;
        net->unk7E = 1;
        NetMode_OpenPercent();
        NetMode_OpenIcon2(3);
        NetMode_OpenProgress2();
        break;
    case 1:
        if (NetOverlay_Request(1) != 0) {
            net->step++;
        }
        break;
    case 2:
        if (NetOverlay_Ready(1) != 0) {
            net->step++;
            net->innerStep = 0;
            net->percent = 0;
            net->netResult = NET_RESULT_NONE;
            NetMode_OpenProgress2();
        }
        break;
    case 3:
        net->step++;
        break;
    case 4:
        net->step = 10;
        net->innerStep = 0;
        NetMode_OpenStatus();
        break;
    case 10:
        net->step = 5;
        net->timer = 0;
        net->netResult = NET_RESULT_RUNNING;
        net->sel2 = 0;
        break;
    case 5:
        if (LoginPad_StartHeldLong(&net->sel2) != 0) {
            net->timer = 61;     /* the cancel count, past NET_CANCEL_AT */
        }
        authResult = Dnas_AuthStep((s8 *)&net->innerStep, &net->timer, &net->percent);
        if (authResult < 0) {
            net->step = 15;
            net->subStep = 0;
            net->innerStep = 0;
            net->timer = 56;
            net->netResult = NET_RESULT_FAILED;
            net->err = Dnas_GetError();
            NetMode_CloseBoxes(NET_BOX_PROGRESS2);
        } else if (authResult > 0) {
            net->step = 11;
            net->subStep = 0;
            net->innerStep = 0;
            net->timer = 24;
            net->netResult = NET_RESULT_DONE;
        }
        break;
    case 9:
        if (--net->timer <= 0) {
            net->percent = 0;
            net->step = 8;
            net->timer = 10;
            NetMode_CloseBoxes(NET_BOX_STATUS_MSG);
            NetMode_CloseBoxes(NET_BOX_STATUS);
            NetMode_CloseBoxes(NET_BOX_STATUS_NONE);
            NetMode_CloseBoxes(NET_BOX_PERCENT);
            NetMode_CloseBoxes(NET_BOX_PROGRESS2);
        }
        break;
    case 11:
        if (--net->timer <= 0) {
            net->percent = 0;
            net->step = 20;
            net->timer = 0;
            NetMode_CloseBoxes(NET_BOX_PERCENT);
            NetMode_CloseBoxes(NET_BOX_PROGRESS2);
        }
        break;
    case 8:
        net->step = 100;
        net->percent = 0;
        NetMode_OpenIcon2(4);
        NetMode_OpenError();
        break;
    case 15:
        if (--net->timer <= 0) {
            net->percent = 0;
            net->step = 9;
            net->timer = 45;
            net->netResult = NET_RESULT_DONE;
        }
        break;
    case 100:
        if (net->percent <= 0) {
            if (LoginPad_Decide(2) != 0) {
                net->step++;
            } else {
                NetModeMsg_Page0(20);
            }
        } else {
            net->percent--;
        }
        break;
    case 101:
        net->step++;
    case 102:
        net->step++;
        NetMode_CloseAll();
    case 103:
        net->unk7E = 0;
        result = -1;
        break;
    case 20:
        if (NetMode_FadeStart(0, 10, 1) != 0) {
            net->step++;
        }
        break;
    case 21:
        if (NetMode_FadeRunning() != 0) {
            break;
        }
        net->step++;
        NetMode_CloseAll();
    case 22:
        net->unk7E = 0;
        result = 1;
        break;
    }
    return result;
}

/*
 * The set-up's save of the NET file (NET_SETUP_SAVE_NET_FILE), one step a
 * frame (gNetMode.step): nothing to do (returns 1 at once) when the saved
 * outcome is already 1 and yn changed nothing (gNetMode.ynResult 0); else
 * the outcome is set to 1, the top frame opens and LobbyCard_SaveNetFile
 * saves the file (a success also clears ynResult). Returns 1 when done,
 * saved or not, else 0.
 */
s32 NetSetup_SaveNetFile(void)
{
    NetModeState *net;
    s32 result;

    net = &gNetMode;
    result = 0;
    switch (net->step) {
    case 0:
        if (gNetSaveData.setupResult == 1 && net->ynResult == 0) {
            result = 1;
            break;
        }
        gNetSaveData.setupResult = 1;
        net->step++;
        net->innerStep = 0;
    case 1:
        net->sel3 = 0;
        net->step++;
        NetMode_EmptyFadeHook(0);
        NetMode_FadeReset();
        break;
    case 2:
        net->step++;
        net->percent = 10;
        NetMode_OpenFrameTop();
        break;
    case 3:
        switch (LobbyCard_SaveNetFile()) {
        case 0:
            break;
        case 1:
            net->ynResult = 0;
        case -1:
            net->step++;
            break;
        }
        break;
    case 4:
        result = 1;
        break;
    }
    return result;
}

/* Starts the patch's install (LobbyPatch_Install) at its first step and
 * resets the NET file save's state. */
void LobbyPatch_ResetInstall(void)
{
    gNetMode.xferStep = 0;
    gNetMode.xferSub = 0;
    gNetMode.pad15 = 1;
    LobbyCard_ResetNetSave();
}

/*
 * The patch's install at login, one step a frame (gNetMode.xferStep;
 * xferSub inside a step, xferState the DNAS library's state). The patch
 * is in gSysSettings.extraBuf: buf0 its data (64K), buf1 (the next 64K)
 * first a copy of it, buf2 a PatchHeader after them.
 * - 0: the network textures and the DNAS_NET overlay load, the frame and
 *   the yes/no box open; 1-2 the first question (message 1): yes goes to
 *   9, no to 40.
 * - 3-8 read the NET file from the card, 25-30 show the card's errors;
 *   no step leads to 3.
 * - 9-10, 50: the DNAS check before an install (Dnas_StateMachine;
 *   holding Start cancels it); passed, buf0 gets its copy back.
 * - 11-13: the DNAS_INS overlay loads, the data is decoded (NetXfer_Step,
 *   the announced size; the output's size goes to the header's recsSize),
 *   114 frames show the end (a button after 53).
 * - 14-15: the header gets the announced version and tag, and the save
 *   menu writes the card with the extra file (NETMENU_MODE_GAME_SAVE_EXTRA);
 *   a quit without saving goes to 20-21, which load DNAS_NET again and
 *   ask the first question again.
 * - 16: a copy of the data is decoded as the records (NetXfer_Step2); the
 *   data, header and records go to the patch area (PATCH_DATA,
 *   PATCH_HEADER, PATCH_RECS) and Patch_ExecAll applies them; 17-19 end
 *   the sequence, close the boxes and return 1.
 * - 35-39: a failure: the error icon and code show for 315 frames (a
 *   button after 105); 40-42 the second question (message 3): yes gives
 *   up (43-44 return -1), no asks the first question again (45, 21).
 * Returns 1 when installed, -1 when given up, else 0.
 */
s32 LobbyPatch_Install(void)
{
    NetModeState *net;
    s32 result;
    u8 *data;
    u8 *recs;
    PatchHeader *header;

    net = &gNetMode;
    data = gSysSettings.extraBuf;
    recs = data + PATCH_AREA_SIZE;
    header = (PatchHeader *)(recs + PATCH_AREA_SIZE);
    result = 0;
    switch (gNetMode.xferStep) {
    case 0:
        switch (net->xferSub) {
        case 0:
            net->xferSub++;
            net->netResult = NET_RESULT_NONE;
            net->cardMsg = 0;
            net->err = 0;
            gPatchCardSlot = D_6AEFA8;
            net->slotUsed = gPatchCardSlot;
            NetMode_ReleaseTextures(0);
            NetMode_EmptyResetHook();
            NetMode_EmptyFadeHook(0);
            NetMode_FadeReset();
            NetMode_LoadTextures(0);
            memset(recs, 0, PATCH_AREA_SIZE);
            memset(header, 0, sizeof(PatchHeader));
            memcpy(recs, data, PATCH_AREA_SIZE);
            break;
        case 1:
            net->xferSub++;
            break;
        case 2:
            net->xferSub++;
            break;
        case 3:
            if (NetOverlay_Request(1) != 0) {
                net->xferSub++;
            }
            break;
        case 4:
            if (NetOverlay_Ready(1) != 0) {
                net->xferStep++;
                net->xferSub = 0;
                net->percent = 2;
                net->sel2 = 0;
                NetMode_OpenFrame();
                NetMode_OpenMenu(11, 2);
                NetMode_ResetToFrame();
                NetMode_FadeStart(0, 10, 0);
                NetMode_OpenYesNo(1);
            }
            break;
        }
        break;
    case 1:
        NetModeMsg_Page3(1);
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 3600;
        }
        break;
    case 2:
        NetModeMsg_Page3(1);
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            net->xferStep = 9;
            net->percent = 2;
            NetMode_ResetToFrame();
            break;
        case -1:
            net->xferStep = 40;
            net->percent = 2;
            NetMode_ResetToFrame();
            break;
        }
        break;
    case 3:
        NetModeMsg_Page3(1);
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 2;
            gSysBusy = 1;
        }
        break;
    case 4:
        NetModeMsg_Page3(10);
        if (--net->percent <= 0) {
            net->xferStep++;
            net->xferState = 0;
            NetMode_BeginCardTask(0);
            CardTask_Begin(0);
            gSysBusy = 1;
        }
        break;
    case 5:
        NetModeMsg_Page3(10);
        if (LobbyCard_LoadFiles(1) != 0) {
            net->xferStep++;
            net->slotUsed = gPatchCardSlot;
            CardTask_StartFind(net->slotUsed, (u8 *) (*(s32 *)&gLoadBuf), 0);
        }
        break;
    case 6:
        NetModeMsg_Page3(10);
        CardTask_Update();
        switch (CardTask_Result()) {
        case 0:
            if (Card_IsChanged(net->slotUsed) != 0) {
                net->xferStep++;
                CardTask_StartRead(net->slotUsed, (u8 *) (*(s32 *)&gLoadBuf));
            } else {
                net->xferStep = 8;
                net->percent = 2;
                NetMode_ResetToFrame();
                gSysBusy = 0;
            }
            break;
        case -251:
        case -254:
        case -256:
        case -253:
        case -252:
            net->xferStep = 27;
            net->percent = 2;
            net->cardMsg = 7;
            gSysBusy = 0;
            break;
        case -255:
            net->xferStep = 25;
            net->percent = 2;
            net->cardMsg = 6;
            gSysBusy = 0;
            break;
        }
        break;
    case 7:
        NetModeMsg_Page3(10);
        CardTask_Update();
        switch (CardTask_Result()) {
        case 0:
            switch (LobbyCard_CheckNetFile(0)) {
            case -1:
                net->xferStep = 29;
                net->percent = 2;
                net->cardMsg = 98;
                gSysBusy = 0;
                break;
            case -2:
                net->xferStep = 30;
                net->percent = 2;
                net->cardMsg = 120;
                gSysBusy = 0;
                break;
            default:
                net->xferStep++;
                Card_ClearChanged();
                net->percent = 2;
                NetMode_ResetToFrame();
                gSysBusy = 0;
                break;
            }
            break;
        case -251:
        case -254:
        case -256:
        case -253:
        case -252:
            net->xferStep = 27;
            net->percent = 2;
            net->cardMsg = 69;
            gSysBusy = 0;
            break;
        case -255:
            net->xferStep = 25;
            net->percent = 2;
            net->cardMsg = 68;
            gSysBusy = 0;
            break;
        }
        break;
    case 8:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 2;
            gSysBusy = 0;
        }
        break;
    case 9:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->xferState = 0;
            net->percent = 0;
            net->timer = 0;
            net->netResult = NET_RESULT_RUNNING;
            net->sel2 = 0;
            net->err = 0;
            NetMode_OpenStatus();
            NetMode_OpenPercent();
            NetMode_OpenIcon2(3);
            NetMode_OpenProgress2();
        }
        break;
    case 10: {
        s32 ret;

        if (LoginPad_StartHeldLong(&net->sel2) != 0) {
            net->timer = NET_CANCEL_AT;
        }
        ret = Dnas_StateMachine(&net->xferState, &net->timer, &net->percent);
        if (ret < 0) {
            net->xferStep = 35;
            net->xferSub = 0;
            net->xferState = 0;
            net->percent = 0;
            net->timer = 56;
            net->netResult = NET_RESULT_FAILED;
            net->err = Dnas_GetError();
            NetMode_CloseBoxes(NET_BOX_PROGRESS2);
        } else if (ret > 0) {
            net->timer = 24;
            net->xferStep = 50;
            net->xferSub = 0;
            net->xferState = 0;
            net->size = gLobbyState.patchInfo.size;
            memcpy(data, recs, PATCH_AREA_SIZE);
        }
        break;
    }
    case 11:
        switch (net->xferSub) {
        case 0:
            if (NetOverlay_Request(2) != 0) {
                net->xferSub++;
            }
            break;
        case 1:
            if (NetOverlay_Ready(2) != 0) {
                net->xferStep++;
                net->xferSub = 0;
            }
            break;
        }
        break;
    case 12: {
        s32 ret;

        ret = NetXfer_Step(&net->xferState, &net->timer, data, net->size, PATCH_AREA_SIZE);
        if (0 < ret) {
            net->percent = ret;
            net->xferStep = 13;
            net->xferSub = 0;
            net->xferState = 0;
            net->timer = 114;
            net->netResult = NET_RESULT_DONE;
            header->recsSize = ret;
        } else if (ret < 0) {
            net->percent = 0;
            net->xferStep = 35;
            net->xferSub = 0;
            net->xferState = 0;
            net->timer = 32;
            net->netResult = NET_RESULT_FAILED;
            net->err = NetXfer_GetResult();
        }
        break;
    }
    case 13:
        if (--net->timer <= 0) {
            net->xferStep++;
            net->percent = 2;
            NetMode_CloseBoxes(NET_BOX_STATUS_MSG);
            NetMode_CloseBoxes(NET_BOX_STATUS);
            NetMode_CloseBoxes(NET_BOX_STATUS_NONE);
            NetMode_CloseBoxes(NET_BOX_PERCENT);
            NetMode_CloseBoxes(NET_BOX_PROGRESS2);
        } else if (net->timer < 61 && LoginPad_Decide(2) != 0) {
            net->xferStep++;
            net->percent = 2;
            NetMode_CloseBoxes(NET_BOX_STATUS_MSG);
            NetMode_CloseBoxes(NET_BOX_STATUS);
            NetMode_CloseBoxes(NET_BOX_STATUS_NONE);
            NetMode_CloseBoxes(NET_BOX_PERCENT);
            NetMode_CloseBoxes(NET_BOX_PROGRESS2);
        }
        break;
    case 14:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->subStep = 0;
            net->innerStep = 0;
            strncpy(header->version, gLobbyState.patchInfo.version, PATCH_VERSION_LEN);
            strncpy(header->number, gLobbyState.patchInfo.tag, 4);
            NetMode_ResetToFrame();
            NetMenu_Open(NETMENU_MODE_GAME_SAVE_EXTRA);
        }
        break;
    case 15:
        switch ((u8)NetMenu_Update()) {
        case 1:
            net->xferStep = 16;
            net->xferState = 0;
            net->size = header->recsSize;
            net->timer = 0;
            memcpy(recs, data, PATCH_AREA_SIZE);
            net->err = 0;
            net->netResult = NET_RESULT_RUNNING;
            gSysBusy = 0;
            break;
        case 2:
            net->xferStep = 20;
            net->percent = 10;
            gSysBusy = 0;
            break;
        }
        break;
    case 16: {
        s32 ret;

        ret = NetXfer_Step2(&net->xferState, &net->timer, recs, net->size, PATCH_AREA_SIZE);
        if (0 < ret) {
            memcpy(PATCH_DATA, data, PATCH_AREA_SIZE);
            strncpy(((PatchHeader *)PATCH_HEADER)->version, header->version, PATCH_VERSION_LEN);
            strncpy(((PatchHeader *)PATCH_HEADER)->number, header->number, 4);
            ((PatchHeader *)PATCH_HEADER)->recsSize = header->recsSize;
            memcpy(PATCH_RECS, recs, PATCH_AREA_SIZE);
            Patch_ExecAll();
            net->percent = 0;
            net->xferStep++;
            net->xferSub = 0;
            net->xferState = 0;
            net->timer = 0;
            net->percent = 150;
            net->netResult = NET_RESULT_DONE;
        } else if (ret < 0) {
            net->percent = 0;
            net->xferStep = 35;
            net->xferSub = 0;
            net->xferState = 0;
            net->timer = 32;
            net->netResult = NET_RESULT_FAILED;
            net->err = NetXfer_GetResult();
        }
        break;
    }
    case 17:
        net->xferStep++;
        NetMode_FadeStart(0, 20, 1);
        break;
    case 18:
        if (NetMode_FadeRunning() != 0) {
            break;
        }
        net->xferStep++;
        net->slotUsed = net->slot;
        NetMode_CloseAll();
        LobbyLogin_FreeTextures();
    case 19:
        Gfx_ClearModelBuffers();
        result = 1;
        break;
    case 20:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 2;
            net->sel2 = 0;
            NetMode_OpenMenu(11, 2);
            NetMode_ResetToFrame();
        }
        break;
    case 21:
        switch (net->xferSub) {
        case 0:
            if (NetOverlay_Request(1) != 0) {
                net->xferSub++;
            }
            break;
        case 1:
            if (NetOverlay_Ready(1) != 0) {
                net->xferStep = 1;
                net->xferSub = 0;
                NetMode_OpenMenu(11, 2);
                NetMode_ResetToFrame();
                NetMode_OpenYesNo(1);
            }
            break;
        }
        break;
    case 25:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 150;
            net->cardMsg = 6;
        }
        break;
    case 26:
        NetModeMsg_Page3(net->cardMsg);
        if (--net->percent <= 0) {
            net->xferStep = 40;
            net->percent = 2;
        } else if (net->percent < 151) {
            NetMode_EmptyMsgHook(net->cardMsg);
            if (LoginPad_Decide(2) != 0) {
                net->xferStep = 40;
                net->percent = 2;
            }
        }
        break;
    case 27:
        if (--net->percent <= 0) {
            net->xferStep = 26;
            net->percent = 150;
            net->cardMsg = 7;
        }
        break;
    case 28:
        if (--net->percent <= 0) {
            net->xferStep = 26;
            net->percent = 150;
            net->cardMsg = 9;
        }
        break;
    case 29:
        if (--net->percent <= 0) {
            net->xferStep = 26;
            net->percent = 150;
            net->cardMsg = 15;
        }
        break;
    case 30:
        if (--net->percent <= 0) {
            net->xferStep = 26;
            net->percent = 150;
            net->cardMsg = 8;
        }
        break;
    case 35:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->netResult = NET_RESULT_DONE;
            net->percent = 45;
            NetMode_CloseBoxes(NET_BOX_PERCENT);
        }
        break;
    case 36:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 2;
            NetMode_CloseBoxes(NET_BOX_STATUS_MSG);
            NetMode_CloseBoxes(NET_BOX_STATUS);
            NetMode_CloseBoxes(NET_BOX_STATUS_NONE);
            NetMode_CloseBoxes(NET_BOX_ICON);
        }
        break;
    case 37:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 2;
            NetMode_OpenIcon2(4);
            NetMode_OpenError();
        }
        break;
    case 38:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 315;
            net->timer = 210;
        }
        break;
    case 39:
        if (--net->percent <= 0) {
            net->xferStep = 40;
            net->percent = 2;
            net->sel2 = 0;
            NetMode_CloseBoxes(NET_BOX_ERROR);
        } else if (net->percent <= net->timer && LoginPad_Decide(2) != 0) {
            net->xferStep = 40;
            net->percent = 2;
            NetMode_CloseBoxes(NET_BOX_ERROR);
        }
        break;
    case 40:
        if (--net->percent <= 0) {
            net->xferStep++;
            net->percent = 10;
            net->sel2 = 0;
            NetMode_OpenMenu(11, 2);
            NetMode_ResetToFrame();
            NetMode_OpenYesNo(1);
        }
        break;
    case 41:
        NetModeMsg_Page3(3);
        net->xferStep++;
        net->percent = 3600;
        break;
    case 42:
        NetModeMsg_Page3(3);
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            net->xferStep++;
            net->percent = 60;
            NetMode_ResetToFrame();
            break;
        case -1:
            net->xferStep = 45;
            net->percent = 2;
            NetMode_ResetToFrame();
            break;
        }
        break;
    case 43:
        if (--net->percent <= 0) {
            net->xferStep++;
            NetMode_CloseAll();
            net->slotUsed = net->slot;
            LobbyLogin_FreeTextures();
        }
        break;
    case 44:
        Gfx_ClearModelBuffers();
        result = -1;
        break;
    case 45:
        if (--net->percent <= 0) {
            net->xferStep = 21;
            net->xferSub = 0;
            net->subStep = 0;
            net->percent = 2;
        }
        break;
    case 50:
        if (--net->timer <= 0) {
            NetMode_CloseBoxes(NET_BOX_PERCENT);
            NetMode_CloseBoxes(NET_BOX_PROGRESS2);
            net->xferStep = 11;
            net->xferSub = 0;
            net->xferState = 0;
            net->timer = 0;
        }
        break;
    }
    return result;
}
