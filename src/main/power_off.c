/*
 * The power-off handling: when the console's power button is pressed, the
 * library's power-off hook wakes a thread that marks the game as exiting
 * (gSysExiting), shuts down the hard disk drive or the DEV9 bay and the
 * CD/DVD drive, and switches the console off.
 */
#include "common.h"
#include "sif.h"
#include "power_off.h"
#include "libcdvd.h"
#include "libkernl.h"

extern s32 gSysExiting;
extern volatile s32 gPowerOffStat;
extern u8 gPowerOffStack[8192];
extern u8 _gp[];
s32 CreateThread(struct ThreadParam *p);
s32 func_00114C60(s32 tid, s32 arg);
s32 func_00118378(void (*func)(s32), s32 arg);

/* The devctl requests PowerOff_Thread makes, to the device named. */
#define POWEROFF_HDD_STATUS 18439       /* hdd: the drive's status */
#define POWEROFF_HDD_SKIP 3             /* the hdd: status on which only DEV9 is switched off */
#define POWEROFF_DEV9_OFF 17410         /* dev9x: switch the DEV9 bay off */
#define POWEROFF_PFS_CLOSE 20483        /* pfs: close every file */
#define POWEROFF_HDD_OFF 18438          /* hdd: shut the drive down */

/*
 * Sets up the power-off handling: creates a semaphore, sets the current
 * thread's priority to 2, creates and starts PowerOff_Thread (priority 1,
 * stack gPowerOffStack, 8192 bytes) with the semaphore as its
 * argument, and registers PowerOff_Callback with the library's power-off
 * hook. Called during the IOP set-up when the power-off module is there.
 * Returns 1.
 */
s32 PowerOff_Init(void)
{
    struct ThreadParam threadParam;
    SemaParam semaParam;
    s32 sema;

    semaParam.maxCount = 1;
    semaParam.initCount = 0;
    semaParam.option = 0;
    sema = CreateSema(&semaParam);
    ChangeThreadPriority(GetThreadId(), 2);
    threadParam.stackSize = sizeof(gPowerOffStack);
    threadParam.gpReg = _gp;
    threadParam.entry = PowerOff_Thread;
    threadParam.stack = gPowerOffStack;
    threadParam.initPriority = 1;
    func_00114C60(CreateThread(&threadParam), sema);
    func_00118378(PowerOff_Callback, sema);
    return 1;
}

/*
 * Waits on sema (signalled by PowerOff_Callback), then sets gSysExiting
 * and shuts the console down: with the hdd module loaded it asks hdd: for
 * its status; on POWEROFF_HDD_SKIP it switches the DEV9 bay off,
 * otherwise it closes pfs:'s files and shuts the drive down. Without the
 * module it switches DEV9 off. Each of these devctls but the pfs: one is
 * retried until it succeeds; then a devctl to cdrom0: (17192) the same
 * way, then sceCdPowerOff until it succeeds with gPowerOffStat 0, and the
 * thread spins forever.
 */
void PowerOff_Thread(s32 sema)
{
    WaitSema(sema);
    gSysExiting = 1;
    if (sceSifSearchModuleByName("hdd") >= 0) {
        gPowerOffStat = sceDevctl("hdd:", POWEROFF_HDD_STATUS, NULL, 0, NULL, 0);
        switch (gPowerOffStat) {
        case POWEROFF_HDD_SKIP:
            while (sceDevctl("dev9x:", POWEROFF_DEV9_OFF, NULL, 0, NULL, 0) < 0) {
            }
            break;
        default:
            sceDevctl("pfs:", POWEROFF_PFS_CLOSE, NULL, 0, NULL, 0);
            while (sceDevctl("hdd:", POWEROFF_HDD_OFF, NULL, 0, NULL, 0) < 0) {
            }
            break;
        }
    } else {
        while (sceDevctl("dev9x:", POWEROFF_DEV9_OFF, NULL, 0, NULL, 0) < 0) {
        }
    }
    while (sceDevctl("cdrom0:", 17192, NULL, 0, NULL, 0) < 0) {
    }
    while (!sceCdPowerOff(&gPowerOffStat) || gPowerOffStat != 0) {
    }
    for (;;) {
    }
}

/* The power-off callback: signals sema to wake PowerOff_Thread. */
void PowerOff_Callback(s32 sema)
{
    iSignalSema(sema);
}
