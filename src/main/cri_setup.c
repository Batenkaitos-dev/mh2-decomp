/*
 * The CRI libraries' hooks in the boot ELF: the VBlank hook that
 * Display_AdxVint calls each frame, and the start-up of the Sofdec movie
 * library for the display's rate (Movie_InitSfd, which main calls once).
 */
#include "common.h"
#include "cri_setup.h"
#include "libc.h"

typedef struct SfdInitPrm {
    /* 0x00 */ f32 vhz;         /* display rate */
    /* 0x04 */ s32 dispCycle;
    /* 0x08 */ s32 dispLatency;
    /* 0x0C */ u8 decSvr;
    /* 0x10 */ s32 rsv[4];
} SfdInitPrm;

void func_00161F00(s32 arg0);
void func_00161750(void (*func)(void), s32 arg1);
void mwPlyInitSfdFx(SfdInitPrm *prm);

/* The VBlank hook of the CRI libraries: Display_AdxVint calls it with 0,
 * which goes on to the library. */
void AdxWrap_VintHook(s32 param)
{
    func_00161F00(param);
}

/* An empty callback Movie_InitSfd hands to the CRI library. */
void Movie_NullCallback(void)
{
}

/* Starts the Sofdec movie library for the display's rate (pal: 50 Hz,
 * else 59.94 Hz). */
void Movie_InitSfd(s32 pal)
{
    SfdInitPrm prm;

    memset(&prm, 0, sizeof(prm));
    if (pal) {
        prm.vhz = 50.0f;
    } else {
        prm.vhz = 59.94f;
    }
    prm.dispCycle = 1;
    prm.dispLatency = 1;
    prm.decSvr = 0;
    func_00161750(Movie_NullCallback, 0);
    mwPlyInitSfdFx(&prm);
}
