#ifndef PAD_H
#define PAD_H

#include "common.h"
#include "padio.h"

/* One controller port (34 bytes); gPadState[0] is the first port. */
typedef struct PadState {
    u16 held;       /* 0x00: buttons down */
    u16 prevHeld;   /* 0x02: held of the previous frame (the repeat counters compare it with held) */
    u16 pressed;    /* 0x04: newly pressed this frame */
    u16 released;   /* 0x06 */
    u16 dir;        /* 0x08: the left stick's direction as pad bits (Pad_StickToDpad) */
    u16 dirPrev;    /* 0x0A: dir of the previous frame */
    u16 dirPressed; /* 0x0C */
    u16 dirReleased; /* 0x0E */
    u16 degL;       /* 0x10: the left stick's angle (ANGLE_TURN to a turn) */
    u16 degR;       /* 0x12: the right stick's angle */
    u16 magL;       /* 0x14: the left stick's tilt (Pad_StickDeadZone; > 64 counts as held) */
    u16 magR;       /* 0x16: the right stick's tilt */
    u16 repeat;     /* 0x18: pressed, then auto-repeated while held */
    u16 repeatCnt;  /* 0x1A: frames held unchanged (the repeat counter) */
    s16 on;         /* 0x1C: 1 while a pad is connected (its kind) */
    s16 onPrev;     /* 0x1E */
    s16 unk20;      /* 0x20: 0 */
} PadState;
typedef char PadState_size_check[(sizeof(PadState) == 0x22) ? 1 : -1];

extern PadState gPadState[2];

/*
 * The game's button bits (PadState.held, pressed, repeat ...;
 * PadPortInput.buttons), which PadState_Update builds from
 * the core's bits (PADIO_*, include/padio.h). They are named for the
 * buttons of the default configuration: the core's PadConfig can remap
 * them first. In the menus PAD_CIRCLE confirms and PAD_CROSS cancels.
 */
#define PAD_R3 0x0001
#define PAD_L3 0x0002
#define PAD_L2 0x0004
#define PAD_L1 0x0008
#define PAD_R2 0x0010
#define PAD_CIRCLE 0x0020
#define PAD_CROSS 0x0040
#define PAD_R1 0x0080
#define PAD_TRIANGLE 0x0100
#define PAD_SQUARE 0x0200
#define PAD_RIGHT 0x0400
#define PAD_LEFT 0x0800
#define PAD_DOWN 0x1000
#define PAD_UP 0x2000
#define PAD_SELECT 0x4000
#define PAD_START 0x8000
#define PAD_DPAD (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT) /* 0x3C00 */
#define PAD_ALL 0xFFFF                                      /* every button */
#define PAD_REPLAY_END 0xFFFF                               /* a pad replay's end marker, in place of a frame count (Pad_ReadButtons) */

/*
 * PadState.dir: the sticks' directions as bits. The left stick uses the
 * D-pad's bits (PAD_UP ... PAD_RIGHT); the right stick these.
 */
#define PAD_RSTICK_UP 0x0020
#define PAD_RSTICK_DOWN 0x0010
#define PAD_RSTICK_LEFT 0x0008
#define PAD_RSTICK_RIGHT 0x0004

/* A pad value this frame and the frame before (PadPortInput's buttons and direction). */
typedef struct PadPair {
    u16 cur;
    u16 prev;
} PadPair;

typedef struct PadPairS {
    s16 cur;
    s16 prev;
} PadPairS;

/* A stick value of both sticks, this frame only (PadPortInput's stickTilt and stickAngle). */
typedef struct PadStickPair {
    u16 left;   /* the left stick's (gPadState's magL or degL) */
    u16 right;  /* the right stick's (magR or degR) */
} PadStickPair;

/* The pad input per controller port (Pad_UpdateInput fills it from gPadState;
 * Player_CopyPadInput copies it into a player's PlayerInput, whose stick fields have the same
 * names). The buttons and the direction keep the previous frame's value; the sticks keep
 * both sticks instead. */
typedef struct PadPortInput {
    PadStickPair stickTilt[2];  /* 0x00: gPadState's magL and magR */
    PadStickPair stickAngle[2]; /* 0x08: gPadState's degL and degR */
    PadPair stickDir[2];    /* 0x10: gPadState's dir: pad bits 0x3C00, which survive the input locks */
    PadPairS buttons[2];    /* 0x18: the game's button bits */
} PadPortInput;
typedef char PadPortInput_size_check[(sizeof(PadPortInput) == 0x20) ? 1 : -1];
extern PadPortInput gPadInput;

void Pad_ClearMem(void *dst, s32 size);
s32 Pad_Init(void);
void Pad_End(void);
void Pad_ClearInputs(void);
void Pad_SetConfig(PadConfig *cfg, s32 port);
void Pad_Update(void);
void Pad_Remap(void);
void Pad_RaisePressure(u8 *pressure, u8 value, u32 bits);
void Pad_StickAngle(PadStick *stick);
void Pad_SetHeld(PadInput *pad, u32 held);
void Pad_CountHolds(PadInput *pad);
u8 Pad_GetConnected(void);
void Pad_EnableVibration(u8 enable);
s32 Pad_Open(void);
void Pad_Close(void);
void Pad_SetVibrationFlag(u8 enable);
void Pad_SetLimits(s32 port, s16 pressMin, s16 deadL, s16 deadR);
void Pad_Read(void);
void Pad_ClearBuffers(void);

s32 Pad_ConvertData(s32 port);
void Pad_StickPolar(PadStick *stick, s16 deadZone);
u8 Pad_StickDirBits(PadStick *stick);
s32 Pad_StartLibs(void);
void Pad_OpenSocket(s32 port, s32 slot, PadPort *padPort);
void Pad_EndLibs(void);
void Pad_ReadPort(s32 port);
s32 Pad_SetVibration(s32 port, s32 level, s32 frames);


#endif /* PAD_H */
