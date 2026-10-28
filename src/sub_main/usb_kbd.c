
/*
 * The USB keyboard (libusbkb, linked in main). Its set-up is a run of steps, each
 * given the step counter gUsbKbStep: a step advances it on success, resets it to 0
 * on failure, and returns 1 to go on (0 when it reset it, or at the end).
 * jtbl_00403140 holds the whole run, UsbKb_StepNext to UsbKb_StepDone (it keeps
 * splat's jump-table name);
 * gUsbKbSetupSteps the same steps from UsbKb_StepWaitConnect on, which UsbKb_Poll
 * runs until gUsbKbReady is set.
 * The data: those two step tables.
 */

#include "common.h"
#include "model.h"
#include "kbd.h"
#include "usb_kbd.h"
#include "libc.h"

extern s16 gUsbKbReady;

extern s8 gUsbKbStep;
extern s32 (*jtbl_00403140[])(s8 *step);

/* Runs step gUsbKbStep of jtbl_00403140. */
s32 UsbKb_RunInitStep(void)
{
    return jtbl_00403140[gUsbKbStep](&gUsbKbStep);
}

/* The first step: only advances. */
s32 UsbKb_StepNext(s8 *step)
{
    (*step)++;
    return 1;
}

s32 sceUsbKbInit(s32 *st);
s32 sceUsbKbGetInfo(void *p);
s32 sceUsbKbSync(s32 a, s32 *st);
extern s8 gUsbKbLibInit;
extern s32 gUsbKbSyncResult;
extern UsbKbInfo gUsbKbInfo;

/* Initialises libusbkb (sceUsbKbInit) once (gUsbKbLibInit), advancing when it
 * reports a nonzero status. */
s32 UsbKb_StepInit(s8 *step)
{
    s32 status;

    if (gUsbKbLibInit == 0) {
        if (sceUsbKbInit(&status) != 0) {
            *step = 0;
            return 0;
        }
        if (status == 0) {
            *step = 0;
            return 0;
        }
        gUsbKbSyncResult = 0;
        gUsbKbLibInit = 1;
        (*step)++;
        return 1;
    } else {
        *step = 0;
        return 0;
    }
}

/* Reads the keyboards' state into gUsbKbInfo (sceUsbKbGetInfo, then sceUsbKbSync);
 * advances when a keyboard is connected, resets when none is. */
s32 UsbKb_StepWaitConnect(s8 *step)
{
    s32 status;

    status = sceUsbKbGetInfo(&gUsbKbInfo);
    if (status != 0) {
        *step = 0;
        return 0;
    }
    gUsbKbSyncResult = sceUsbKbSync(0, &status);
    if (gUsbKbSyncResult == 0 && status == 0) {
        if (gUsbKbInfo.connectedCount == 0) {
            *step = 0;
            return 0;
        }
        (*step)++;
    }
    return 1;
}

s32 sceUsbKbSetRepeat(s32, s32, s32);
s32 sceUsbKbSetCodeType(s32 port, u32 mode);
s32 sceUsbKbSetArrangement(s32, u32);
s32 sceUsbKbSetReadMode(s32, u32);
extern s32 D_005819C0;
extern s32 D_005819C8;

/* Sets keyboard 0 up (no key repeat, code type 0, arrangement 1, read mode 0) when it
 * is connected, else resets; advances when every call succeeds. Two words
 * are written between the calls (1 and 0) and never read. */
s32 UsbKb_StepSetup(s8 *step)
{
    s32 repeatErr;
    s32 codeTypeErr;
    s32 arrangeErr;
    s32 readModeErr;

    if (gUsbKbInfo.connected[0] == 0) {
        *step = 0;
        return 0;
    }
    repeatErr = sceUsbKbSetRepeat(0, 0, 0);
    codeTypeErr = sceUsbKbSetCodeType(0, 0U);
    D_005819C8 = 1;
    arrangeErr = sceUsbKbSetArrangement(0, 1U);
    D_005819C0 = 0;
    readModeErr = sceUsbKbSetReadMode(0, 0U);
    if ((repeatErr == 0) && (codeTypeErr == 0) && (arrangeErr == 0) && (readModeErr == 0)) {
        *step += 1;
    }
    return 1;
}

s32 sceUsbKbSetLEDMode(s32 a, s32 b);

/* Sets keyboard 0's LED mode to 0 (sceUsbKbSetLEDMode) and waits for it (sceUsbKbSync);
 * advances when both succeed. */
s32 UsbKb_StepSetLed(s8 *step)
{
    s32 status;

    status = sceUsbKbSetLEDMode(0, 0);
    if (status == 0) {
        gUsbKbSyncResult = sceUsbKbSync(0, &status);
        if (gUsbKbSyncResult == 0 && status == 0) {
            (*step)++;
        }
    }
    return 1;
}

/* The last step: sets gUsbKbReady and starts the run over. Returns 0. */
s32 UsbKb_StepDone(s8 *step)
{
    *step = 0;
    gUsbKbReady = 1;
    return 0;
}

s32 sceUsbKbRead(s32 port, void *buf);
extern s32 (*gUsbKbSetupSteps[])(s8 *step);
extern u8 gKbdSettings[136];
extern UsbKbData gUsbKbData;
extern u8 D_005819A0[];
extern u8 gUsbKbReport[9];
extern u8 *D_005819B0;          /* its first byte is gUsbKbReport[8] */

/*
 * The USB keyboard poll: runs step gUsbKbStep of the set-up table at gUsbKbSetupSteps until
 * gUsbKbReady is set; then reads keyboard 0 (sceUsbKbRead) into gUsbKbData, copies
 * sizeof(gKbdSettings) bytes from there into gKbdSettings (and then gKbdSettings onto
 * itself, as the game does) and, when it has key codes, returns them as a 9-byte report
 * in gUsbKbReport (else NULL). A pointer over the report's last byte is written with
 * the address of the word before the report, and
 * then its low byte with the last key code (gUsbKbReport[8] is the same byte).
 */
u8 *UsbKb_Poll(void)
{
    s32 status;

    memset(gKbdSettings, 0, sizeof(gKbdSettings));
    if (gUsbKbReady == 0) {
        gUsbKbSetupSteps[gUsbKbStep](&gUsbKbStep);
        return NULL;
    }
    if (UsbKb_IsConnected(0) == -1) {
        return NULL;
    }
    sceUsbKbRead(0, &gUsbKbData);
    memcpy(gKbdSettings, &gUsbKbData, sizeof(gKbdSettings));
    sceUsbKbSync(0, &status);
    memcpy(gKbdSettings, gKbdSettings, sizeof(gKbdSettings));
    if (gUsbKbData.keyCount == 0) {
        return NULL;
    }
    gUsbKbReport[0] = gUsbKbData.modifiers;
    D_005819B0 = D_005819A0;
    gUsbKbReport[1] = gUsbKbData.leds;
    gUsbKbReport[2] = gUsbKbData.keys[0];
    gUsbKbReport[3] = gUsbKbData.keys[1];
    gUsbKbReport[4] = gUsbKbData.keys[2];
    gUsbKbReport[5] = gUsbKbData.keys[3];
    gUsbKbReport[6] = gUsbKbData.keys[4];
    gUsbKbReport[7] = gUsbKbData.keys[5];
    gUsbKbReport[8] = gUsbKbData.keys[6];
    return gUsbKbReport;
}

/* 0 when keyboard port is connected (gUsbKbInfo, read again), else -1. */
s32 UsbKb_IsConnected(s32 port)
{
    s32 status;

    if (sceUsbKbGetInfo(&gUsbKbInfo) != 0) {
        return -1;
    }
    sceUsbKbSync(0, &status);
    if (status != 0) {
        return -1;
    }
    if (gUsbKbInfo.connected[port] == 0) {
        return -1;
    }
    return 0;
}

/* This file's initialised data. */
/* This file's initialised data. */
s32 (*jtbl_00403140[8])(s8 *step) = {
    UsbKb_StepNext, UsbKb_StepInit, UsbKb_StepWaitConnect, UsbKb_StepSetLed,
    UsbKb_StepSetup, UsbKb_StepDone, NULL, NULL,
};
s32 (*gUsbKbSetupSteps[4])(s8 *step) = {
    UsbKb_StepWaitConnect, UsbKb_StepSetLed, UsbKb_StepSetup, UsbKb_StepDone,
};
