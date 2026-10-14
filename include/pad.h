#ifndef PAD_H
#define PAD_H

#include "common.h"

/* One controller port (0x22 bytes); gPadState[0] is the first port. */
typedef struct PadState {
    u16 held;       /* 0x00: buttons down */
    u16 prevHeld;   /* 0x02: held of the previous frame (the repeat counters compare it with held) */
    u16 pressed;    /* 0x04: newly pressed this frame */
    u16 released;   /* 0x06 */
    u16 dir;        /* 0x08: the left stick's direction as pad bits (Pad_StickToDpad) */
    u16 dirPrev;    /* 0x0A: dir of the previous frame */
    u16 dirPressed; /* 0x0C */
    u16 dirReleased; /* 0x0E */
    u16 degL;       /* 0x10: the left stick's angle (0x10000 a turn) */
    u16 degR;       /* 0x12: the right stick's angle */
    u16 magL;       /* 0x14: the left stick's tilt (Pad_StickDeadZone; > 0x40 counts as held) */
    u16 magR;       /* 0x16: the right stick's tilt */
    u16 repeat;     /* 0x18: pressed, then auto-repeated while held */
    u16 repeatCnt;  /* 0x1A: frames held unchanged (the repeat counter) */
    s16 on;         /* 0x1C: 1 while a pad is connected (its kind) */
    s16 onPrev;     /* 0x1E */
    s16 unk20;      /* 0x20: 0 */
} PadState;
typedef char PadState_size_check[(sizeof(PadState) == 0x22) ? 1 : -1];

extern PadState gPadState[2];

/* The game's button bits, as the menus use them: 0x20 confirms (circle),
 * 0x40 cancels (cross); the D-pad is 0x2000 up, 0x1000 down, 0x800 left,
 * 0x400 right. */

/* A pad value this frame and the frame before (PadPortInput). */
typedef struct PadPair {
    u16 cur;
    u16 prev;
} PadPair;

typedef struct PadPairS {
    s16 cur;
    s16 prev;
} PadPairS;

/* sub_main 0x6AE088: the pad input per controller port, current and previous frame (unk_36BE70.c
 * fills it from gPadState; Player_CopyPadInput copies it into a player's PlayerInput, whose
 * stick fields have the same names). */
typedef struct PadPortInput {
    PadPair stickTilt[2];   /* 0x00: gPadState's magL */
    PadPair stickAngle[2];  /* 0x08: gPadState's degL */
    PadPair stickDir[2];    /* 0x10: gPadState's dir: pad bits 0x3C00, which survive the input locks */
    PadPairS buttons[2];    /* 0x18: the game's button bits */
} PadPortInput;
typedef char PadPortInput_size_check[(sizeof(PadPortInput) == 0x20) ? 1 : -1];
extern PadPortInput gPadInput;

#endif /* PAD_H */
