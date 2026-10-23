/*
 * The NET file's saves to the memory card, and the countdown of their
 * messages.
 *
 * Two savers write the lobby's network data (gNetSaveData) to the card in
 * the slot the system settings keep, one step a frame: LobbyCard_SaveNetFile
 * (the set-up's save, with the network screen's messages; its step is
 * gNetMode.innerStep) and LobbyCard_RunNetSave (the save from the lobby,
 * with the web browser's message box: LobbyCard_OpenMsgBox and
 * LobbyCard_CloseMsgBox open and close it; its step is gNetMode.saveStep).
 * LobbyCard_LoadFiles loads the NET file's icons first. Both write a copy
 * whose settings LobbyCard_FixNetValues mends; LobbyCard_CheckNetValues,
 * LobbyCard_DecodeNetFile and LobbyCard_CheckNetFile check a file read
 * back. LobbyMsg_DrawCountdown, between the two savers, draws the seconds
 * left of a message, counting gNetMode.timer down. Only the two savers
 * queue it (NetModeMsg_QueueCountdown), with the 1860 frames they give a
 * message.
 */
#include "common.h"
#include "net_save.h"
#include "sprite.h"
#include "syssettings.h"
#include "net_mode.h"
#include "render_state.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "card.h"
#include "net_menu.h"
#include "browser.h"
#include "texture.h"
#include "net_file_save.h"
#include "file_load.h"
#include "font.h"
#include "libc.h"




extern u8 *gLoadBuf;
s32 Str_Length(char *s);
void Font_Printf(char *fmt, ...);
extern char **D_0081B4F4;               /* entry 0: the line under the countdown */


/* Loads the NET file's icons into the card menu's image buffer, a step a
 * frame (gNetMode.innerStep): mode 0 loads NET_FILE_CARD_PACK and gives them to
 * the card task (CardOp_SetFiles), any other mode loads NET_FILE_CARD_PACK_ALT.
 * Returns 1 once loaded (from the third call), else 0. */
s32 LobbyCard_LoadFiles(s32 mode)
{
    s32 ret;
    u8 step;

    step = gNetMode.innerStep;
    ret = 0;
    switch (step) {
    case 0:
        gNetMode.innerStep = (u8) (step + 1);
        if (mode == 0) {
            LoadCompressedFile((u8 *)((s32)gLoadBuf + CARD_BUF_IMAGE), NET_FILE_CARD_PACK);
            CardOp_SetFiles((s32 *) (*(s32 *)&gLoadBuf + CARD_BUF_IMAGE));
        } else {
            LoadCompressedFile((u8 *)((s32)gLoadBuf + CARD_BUF_IMAGE), NET_FILE_CARD_PACK_ALT);
        }
        break;
    case 1:
        gNetMode.innerStep = (u8) (step + 1);
        
    case 2:
        ret = 1;
        break;
    }
    return ret;
}

/* The set-up's save of the NET file, a step a frame (gNetMode.innerStep):
 * finds the file on the card in the slot the system settings keep, then
 * writes a copy of gNetSaveData (its settings mended by
 * LobbyCard_FixNetValues), with the network screen's messages. On a failure
 * it shows the failure's message and watches the card for up to 1860
 * frames: a card put in or changed starts the search again, a button gives
 * up. Returns 1 when saved, -1 when given up, else 0. */
s32 LobbyCard_SaveNetFile(void)
{
    NetModeState *st = &gNetMode;
    s32 r;

    r = 0;
    switch (st->innerStep) {
    case 0:
        Lobby_SetSysBusy();
        st->innerStep++;
        st->slot = gSysSettings.cardSlot;
        st->slotUsed = st->slot;
        CardTask_Begin(1);
        LoadCompressedFile((u8 *)((s32)gLoadBuf + CARD_BUF_IMAGE), NET_FILE_CARD_PACK);
        st->files = CardOp_SetFiles((s32 *)((s32)gLoadBuf + CARD_BUF_IMAGE));
        CardTask_StartFind(st->slotUsed, gLoadBuf, 0);
        break;
    case 1:
        NetModeMsg_Page1(NETMODE_MSG_CHECKING_CARD);
        CardTask_Update();
        switch (CardTask_Result()) {
        case -1:
            break;
        case 0:
        case CARD_ERR_BAD_DATA:
            st->innerStep++;
            st->slot = st->slotUsed;
            memset(gLoadBuf, 0, sizeof(NetSaveData));
            memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
            LobbyCard_FixNetValues(gLoadBuf);
            CardTask_StartSave(st->slot, gLoadBuf);
            break;
        case CARD_ERR_NO_CARD:
            st->msg = 23;
            goto check;
        case CARD_ERR_FAILED:
            st->msg = NETMODE_MSG_CARD_ERROR;
            goto check;
        default:
            st->msg = 24;
        check:
            /* the failures of both steps share this ending through gotos,
             * laid out as the game's code is */
            Card_ClearChanged();
            CardTask_StartPoll();
            st->innerStep = 3;
            st->timer = 1860;
            break;
        }
        break;
    case 2:
        NetModeMsg_Page1(NETMODE_MSG_SAVING);
        CardTask_Update();
        switch (CardTask_Result()) {
        case -1:
            break;
        case 0:
            Lobby_ClearSysBusy();
            r = 1;
            break;
        case CARD_ERR_NO_CARD:
            st->msg = 23;
            goto check;
        case CARD_ERR_FAILED:
        case CARD_ERR_BAD_DATA:
            st->msg = NETMODE_MSG_SAVE_FAILED;
            goto check;
        }
        break;
    case 3:
        NetModeMsg_Page1(st->msg);
        NetModeMsg_QueueCountdown(0);
        CardTask_Update();
        if (st->timer > 0) {
            if (Card_IsChanged(st->slotUsed)) {
                CardTask_StartFind(st->slotUsed, gLoadBuf, 0);
                st->innerStep = 1;
            } else if (LoginPad_Decide(2)) {
                Lobby_ClearSysBusy();
                r = -1;
            }
        } else {
            Lobby_ClearSysBusy();
            r = -1;
        }
        break;
    }
    return r;
}

/* Has LobbyCard_RunNetSave open the web browser's message box on message
 * msg, then go on to step `step`, with 1860 frames on the clock. */
void LobbyCard_OpenMsgBox(s8 step, s8 msg) {
    gNetMode.saveNext = step;
    gNetMode.cardMsg = msg;
    gNetMode.saveStep = 100;
    gNetMode.percent = 8;
    gNetMode.timer = 1860;
}

/* Has LobbyCard_RunNetSave close the message box, then go on to step
 * `step`. */
void LobbyCard_CloseMsgBox(s8 step) {
    gNetMode.saveNext = step;
    gNetMode.saveStep = 110;
}

/* Counts gNetMode.timer down and prints the seconds left in full-width
 * digits, centred, with the line under it; `mode` (0-2) picks the layout:
 * 0 at y 352 and 306 in the smaller size, 1 at y 332 and 286 in the larger,
 * 2 at y 332 and 286 in the smaller. */
void LobbyMsg_DrawCountdown(u8 mode)
{
    char buf[8];
    char *p;
    char **q;
    char *d;
    s32 sec;
    s32 tens;
    s32 ones;
    s16 x;

    if (gNetMode.timer > 0) {
        (gNetMode.timer)--;
    }
    sec = gNetMode.timer / 60;
    tens = sec / 10;
    ones = sec % 10;
    p = buf;
    if (tens > 0) {
        d = &gFullWidthDigits[0][tens * 2];
        *p++ = d[0];
        *p++ = d[1];
    }
    d = &gFullWidthDigits[0][ones * 2];
    *p++ = d[0];
    *p++ = d[1];
    *p = 0;
    Font_SetSpacing(22, 22);
    Font_SetColorIndex(5);
    x = (640 - Str_Length(buf) * 11) / 2.0f;
    switch (mode) {
    case 0:
        Font_SetPos(x, 352);
        Font_SetSize(478.57144f);
        Font_Printf(buf);
        Font_SetSpacing(18, 18);
        Font_SetColorIndex(0);
        q = D_0081B4F4;
        x = (640 - Str_Length(*q) * 9) / 2.0f;
        Font_SetPos(x, 306);
        Font_Printf(*q);
        Font_DrawPage(3);
        Font_SetSize(621.4286f);
        break;
    case 1:
        Font_SetPos(x, 332);
        Font_SetSize(764.28577f);
        Font_Printf(buf);
        Font_SetSpacing(18, 18);
        Font_SetColorIndex(0);
        q = D_0081B4F4;
        x = (640 - Str_Length(*q) * 9) / 2.0f;
        Font_SetPos(x, 286);
        Font_Printf(*q);
        Font_DrawPage(5);
        Font_SetSize(621.4286f);
        break;
    case 2:
        Font_SetSize(478.57144f);
        Font_SetPos(x, 332);
        Font_Printf(buf);
        Font_SetSpacing(18, 18);
        Font_SetColorIndex(0);
        q = D_0081B4F4;
        x = (640 - Str_Length(*q) * 9) / 2.0f;
        Font_SetPos(x, 286);
        Font_Printf(*q);
        Font_DrawPage(3);
        Font_SetSize(621.4286f);
        break;
    }
}

/* Puts LobbyCard_RunNetSave back at its first step. */
void LobbyCard_ResetNetSave(void) {
    gNetMode.saveStep = 0;
    gNetMode.saveNext = 0;
}

/* Saves the lobby's network data (gNetSaveData) to the memory card in the
 * slot the lobby keeps in gSysSettings, one step a frame (gNetMode.saveStep;
 * saveNext is the step after a message box). Steps 0-7: load the NET
 * file's icons, find the file on the card, write a copy of the data (its
 * settings mended by LobbyCard_FixNetValues), then show the result for 150
 * frames (a button closes it after the first 30). On a failure: 30 no card
 * (waits for one to be put in, then starts again), 60 the write failed
 * (waits for a button), 70 no space, no file or an unformatted card (waits
 * for the card to be taken out, then starts again); each gives up when the
 * button is pressed or the message's time runs out. 90 done, 91 and 92
 * given up. 100-101 open the message box on message gNetMode.cardMsg (a
 * part of page 5) and wait 8 frames, 110-111 close it and wait; nothing
 * sets 120 or 130. Returns 1 when saved, -1 when given up, 0 while
 * running. */
s32 LobbyCard_RunNetSave(void)
{
    s32 result;

    result = 0;
    switch (gNetMode.saveStep) {
    case 0:
        gNetMode.innerStep = 0;
        NetMode_BeginCardTask(1);
        gNetMode.unk8D = 0;
        gNetMode.slot = gSysSettings.cardSlot;
        gNetMode.slotUsed = gNetMode.slot;
        LobbyCard_OpenMsgBox(1, 1);
        break;
    case 1:
        if (LobbyCard_LoadFiles(0) != 0) {
            gNetMode.saveNext = 0;
            gNetMode.saveStep++;
            /* the NET file's icons, which LobbyCard_LoadFiles loaded where
             * the card menu keeps its card image */
            gNetMode.cardFiles = CardOp_SetFiles((s32 *)((s32)gLoadBuf + CARD_BUF_IMAGE));
            CardTask_StartFind(gNetMode.slot, gLoadBuf, 0);
        }
        break;
    case 2:
        CardTask_Update();
        gNetMode.sel2 = CardTask_Result();
        switch (gNetMode.sel2) {
        case 0:
            gNetMode.saveStep++;
            break;
        case CARD_ERR_FAILED:
        case CARD_ERR_BAD_DATA:
            gNetMode.saveStep = 60;
            break;
        case CARD_ERR_NO_CARD:
            gNetMode.saveStep = 30;
            break;
        case CARD_ERR_NO_SPACE:
        case CARD_ERR_NO_FILE:
        case CARD_ERR_UNFORMATTED:
            gNetMode.saveStep = 70;
            break;
        }
        gNetMode.saveNext = 0;
        break;
    case 3:
        LobbyCard_OpenMsgBox(4, 2);
        memset(gLoadBuf, 0, sizeof(NetSaveData));
        memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
        LobbyCard_FixNetValues(gLoadBuf);
        CardTask_StartSave(gNetMode.slot, gLoadBuf);
        break;
    case 4:
        CardTask_Update();
        gNetMode.sel2 = CardTask_Result();
        if (gNetMode.sel2 != -1) {
            LobbyCard_CloseMsgBox(5);
        }
        break;
    case 5:
        switch (gNetMode.sel2) {
        case 0:
            LobbyCard_OpenMsgBox(6, 3);
            break;
        default:
            gNetMode.saveStep = 60;
            gNetMode.saveNext = 0;
            break;
        }
        break;
    case 6:
        gNetMode.saveStep++;
        gNetMode.percent = 150; /* frames: the result shows up to 2.5 s */
    case 7:
        if (--gNetMode.percent < 121) {
            if (gNetMode.percent == 0 || LoginPad_Decide(2)) {
                LobbyCard_CloseMsgBox(90);
            }
        }
        break;
    case 30:
        LobbyCard_OpenMsgBox(31, 4);
        CardTask_StartPoll();
        break;
    case 31:
        NetModeMsg_QueueCountdown(2);
        CardTask_Update();
        if (Card_PortState(gNetMode.slotUsed) != 0) {
            LobbyCard_CloseMsgBox(0);
        } else if (LoginPad_Decide(2) || gNetMode.timer == 0) {
            LobbyCard_CloseMsgBox(92);
        }
        break;
    case 60:
        LobbyCard_OpenMsgBox(61, 5);
        break;
    case 61:
        NetModeMsg_QueueCountdown(2);
        if (LoginPad_Decide(2)) {
            LobbyCard_CloseMsgBox(91);
        } else if (gNetMode.timer == 0) {
            LobbyCard_CloseMsgBox(92);
        }
        break;
    case 70:
        LobbyCard_OpenMsgBox(71, 6);
        CardTask_StartPoll();
        break;
    case 71:
        NetModeMsg_QueueCountdown(2);
        CardTask_Update();
        if (Card_PortState(gNetMode.slotUsed) == 0) {
            LobbyCard_CloseMsgBox(0);
        } else if (LoginPad_Decide(2) || gNetMode.timer == 0) {
            LobbyCard_CloseMsgBox(92);
        }
        break;
    case 90:
        result = 1;
    reset:
        /* steps 91 and 92 share this ending through gotos, as the game's
         * code does */
        gNetMode.saveNext = 0;
        gNetMode.saveStep = 0;
        gNetMode.percent = 0;
        gNetMode.timer = 0;
        break;
    case 91:
        result = -1;
        goto reset;
    case 92:
        result = -1;
        goto reset;
    case 100:
        gNetMode.saveStep++;
        Browser_SetErrorState(NetMode_CardMsgError(gNetMode.cardMsg));
        break;
    case 101:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = gNetMode.saveNext;
            gNetMode.saveNext = 0;
        }
        break;
    case 110:
        gNetMode.saveStep++;
        gNetMode.percent = 8;
        Browser_EmptyHook();
        gNetMode.cardMsg = 0;
        break;
    case 111:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = gNetMode.saveNext;
            gNetMode.saveNext = 0;
        }
        break;
    case 120:
        gNetMode.saveStep++;
        break;
    case 121:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = gNetMode.saveNext;
            gNetMode.saveNext = 0;
        }
        break;
    case 130:
        gNetMode.saveStep++;
        gNetMode.cardMsg = 0;
        break;
    case 131:
        if (--gNetMode.percent == 0) {
            gNetMode.saveStep = 110;
        }
        break;
    }
    return result;
}

/* The web browser's error kind (Browser_SetErrorState) for message box
 * message msg: 29, 49, 50, 44, 35 and 114 have one (the box then shows its
 * own message for kinds 40 and 41, page 5's part 4, 5 or 6 for -10, -21
 * and -23, and part 1 for 20); any other message, the save's own 1-6
 * among them, gives 0. */
s32 NetMode_CardMsgError(s8 msg)
{
    s32 kind;

    kind = 0;
    switch (msg) {
    case 29:
        kind = 20;
        break;
    case 49:
        kind = 40;
        break;
    case 50:
        kind = 41;
        break;
    case 44:
        kind = -10;
        break;
    case 35:
        kind = -21;
        break;
    case 114:
        kind = -23;
        break;
    }
    return kind;
}

/* 0 when each of the 16 settings at byte 3197 of a copy of the network
 * data (buf, as gNetSaveData) is 0, 1 or 2; -1 at the first that is not. */
s32 LobbyCard_CheckNetValues(u8 *buf)
{
    s32 i;
    s8 value;

    i = 0;
    /* the loop is written with a goto: so the compiler lays it out as the
     * game's code does */
loop_1:
    value = *(s8 *)((u8 *)((buf + i)) + 3197);
    if ((value < 0) || (value > 2)) {
        return -1;
    }
    i += 1;
    if (i >= 16) {
        return 0;
    }
    goto loop_1;
}

/* sets to 0 each of the 16 settings at byte 3197 of a copy of the network
 * data (as gNetSaveData) that is not 0, 1 or 2 (the online set-up's
 * result, setupResult, is the eighth) */
void LobbyCard_FixNetValues(u8 *data)
{
    s32 i;

    i = 0;
    do {
        if (((s8 *)(data + i))[3197] < 0 || ((s8 *)(data + i))[3197] > 2) {
            ((s8 *)(data + i))[3197] = 0;
        }
        i++;
    } while (i < 16);
}

/* Checks the file read into gLoadBuf: -1 when LobbyCard_DecodeNetFile
 * finds it bad, -2 when its date stamp is not the system file's
 * (SaveMenu_SystemDateMatches on gCardMenu), else 0. */
s32 LobbyCard_CheckNetFile(s32 unused)
{
    if (LobbyCard_DecodeNetFile((void *) (*(void * *)&gLoadBuf)) != 0) {
        return -1;
    }
    return (SaveMenu_SystemDateMatches((struct NetMenu *)&gCardMenu) == 0) ? -2 : 0;
}

/* decodes the NET file's data in place (672 u16 words after an 8-byte
 * header: the version 256, the key, the sum) and checks the sum: 0 when
 * right, -1 when not (or not version 256). The key steps as the system
 * save's does (SaveFile_Encrypt): key * 176 % 65363, 0 taken as 1. */
s32 LobbyCard_DecodeNetFile(u16 *words)
{
    u16 key;
    s32 i;
    u16 check;
    u16 sum;

    if (words[0] != 256) {
        return -1;
    }
    key = words[1];
    sum = 0;
    i = 0;
    check = *(words += 2);
    words += 2;
    do {
        *words ^= key;
        sum += *words;
        words++;
        if (key == 0) {
            key = 1;
        }
        key = (key * 176) % 65363;
        i++;
    } while (i < 672);
    return -(check != sum);
}

/* The file's .rodata: the full-width digits */
#pragma rodata on
char gFullWidthDigits_0[32] = {
#include "lobby/00847260.inc"
};
#pragma rodata off
