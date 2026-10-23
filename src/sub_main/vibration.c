/*
 * The pads' vibration: starting a pattern (gVibPatterns, VibPattern) on a
 * pad port or on the local player's, and stopping it; nothing starts while
 * the vibration option is off.
 */
#include "common.h"
#include "syssave.h"
#include "pad.h"
#include "player.h"
#include "vibration.h"
/* A vibration pattern: what Vib_Start hands Pad_SetVibration. */
typedef struct VibPattern {
    s32 level;          /* the motor's strength */
    s32 frames;         /* how long it runs */
} VibPattern;
extern VibPattern gVibPatterns[];
/* end of prototypes */

/* Static, as the match needs: its callers then keep their first argument in its register
 * across the call, as the game's code does. 1 while the vibration option is off. */
static s32 Vib_IsOff(void)
{
    return gSysSave.options.vibration == 0;
}

/* Starts vibration pattern i on a pad port, unless vibration is off. */
void Vib_Start(s32 port, s32 pattern)
{
    if (!Vib_IsOff()) {
        Pad_SetVibration(port, gVibPatterns[pattern].level, gVibPatterns[pattern].frames);
    }
}

/* The same on port 0, when Player_IsLocal allows it. */
void Vib_StartLocal(void *player, s32 pattern)
{
    if (!Vib_IsOff() && Player_IsLocal(player) != 0) {
        Pad_SetVibration(0, gVibPatterns[pattern].level, gVibPatterns[pattern].frames);
    }
}

void Vib_Stop(s32 port)
{
    Pad_SetVibration(port, 0, 0);
}

/* Stops the vibration of both pad ports. */
void Vib_StopAll(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        Vib_Stop(i);
    }
}

/* The file's initialised data, in the game's order */
VibPattern gVibPatterns[6] = {
#include "sub_main/0043ECF0.inc"
};
