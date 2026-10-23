/*
 * The core's pad input (include/padio.h).
 *
 * Every frame Pad_Update reads the port (Pad_Read, which calls
 * Pad_ConvertData) into gPadRaw, copies it into gPadIn
 * (PadInput: the buttons as read, their edges and hold counters, the
 * sticks) and remaps gPadIn by the button configuration (PadConfig) into
 * gPad, which sub_main's PadState_Update turns into the game's PAD_* bits
 * (include/pad.h). One port is used: the arrays indexed by port have one
 * element in use.
 */
#include "common.h"
#include "padio.h"
#include "pad.h"
#include "texture.h"
#include "libm.h"

/* Built at optimization level 4: Pad_Remap needs it (a pointer kept
 * across its switches), and every other function matches either way. */
#pragma optimization_level 4

extern u8 gPadConnected;           /* pads connected */
extern PadInput gPad;
extern PadConfig gPadConfig[];  /* per port */
extern PadInput gPadIn;
extern PadInput *gPadInputPtr;    /* -> gPadIn */
extern PadInput *gPadPtr;    /* -> gPad */
extern u8 gPadSocketBuf[];         /* the socket's buffer */
extern s32 gPadPortCount;
extern u8 gPadVibrationLast;
extern u8 gPadVibration;
extern PadData gPadDataNone __attribute__((aligned(16)));    /* no pad */
extern PadData gPadData[] __attribute__((aligned(16)));  /* per port */
extern PadData gPadDataCur[];
extern PadLimits gPadLimits[] __attribute__((aligned(8)));
extern PadPort gPadPort[];
extern PadRaw gPadRaw[];
extern s32 gPadFirstSocket;
extern PadLimits gPadDefaultLimits __attribute__((aligned(8)));
extern u8 gPadDpadFix[16];
extern PadConfig gPadConfigDefault;
extern u8 gPadDirRemap[][16];
extern u8 gPadPressRemap[][4];

/* Clears size bytes: words, then the bytes left. */
void Pad_ClearMem(void *dst, s32 size)
{
    s32 i;
    s32 rest;
    s32 wordCount;
    u32 *word = dst;
    u8 *byte;

    rest = size & (sizeof(u32) - 1);
    i = 0;
    wordCount = size / 4;
    for (; i < wordCount; i++) {
        *word++ = 0;
    }
    if (rest != 0) {
        byte = (u8 *)word;
        for (size = 0; size < rest; size++) {
            *byte++ = 0;
        }
    }
}

/* Opens the pad (Pad_Open), clears the inputs and sets the default button
 * configuration on port 0. Returns 0 if libpad2 failed to start. */
s32 Pad_Init(void)
{
    s32 ret;

    ret = Pad_Open();
    Pad_ClearInputs();
    gPadInputPtr = &gPadIn;
    gPadPtr = &gPad;
    Pad_SetConfig(&gPadConfigDefault, 0);
    return ret;
}

void Pad_End(void)
{
    Pad_Close();
}

void Pad_ClearInputs(void)
{
    Pad_ClearMem(&gPadIn, sizeof(PadInput));
    Pad_ClearMem(&gPad, sizeof(PadInput));
}

void Pad_SetConfig(PadConfig *cfg, s32 port)
{
    gPadConfig[port] = *cfg;
    Pad_SetLimits(port, gPadConfig[port].pressMin, gPadConfig[port].deadL, gPadConfig[port].deadR);
}

/* The frame's pad update: reads the port into gPadRaw, copies it into
 * gPadIn with the button edges and hold counts, and remaps it into gPad.
 * gPadConnected counts the ports with a supported pad. */
void Pad_Update(void)
{
    PadRaw *raw;
    PadInput *input;

    Pad_Read();
    gPadConnected = 0;
    raw = (PadRaw *)gPadRaw;
    gPadInputPtr->state = raw->state;
    gPadInputPtr->caps = raw->caps;
    gPadInputPtr->kind = raw->kind;
    gPadInputPtr->id = raw->id;
    input = gPadInputPtr;
    if (input->kind != PAD_KIND_NONE && input->kind != PAD_KIND_UNSUPPORTED) {
        gPadConnected++;
    }
    input->stickL = raw->stickL;
    input->stickR = raw->stickR;
    *(PadPressures *)input->pressure = *(PadPressures *)raw->pressure;
    Pad_SetHeld(input, raw->buttons);
    Pad_CountHolds(gPadInputPtr);
    gPadInputPtr->trigger = gPadInputPtr->pressed;
    Pad_Remap();
}

/*
 * gPad from gPadIn, remapped by port 0's button configuration: the
 * directions and each stick's directions through a gPadDirRemap row, then
 * every bit i moved to bit map[i] with its pressure (a map entry above 24
 * names several bits: their pressures all rise to the button's), and the
 * sticks mirrored as the configuration says.
 */
void Pad_Remap(void)
{
    u8 press[PAD_BUTTONS];
    PadConfig *cfg = (PadConfig *)gPadConfig;
    u32 bits;
    s16 i;
    u32 held;
    u8 *pressPtr = press;
    u8 target;

    gPadPtr->state = gPadInputPtr->state;
    gPadPtr->caps = gPadInputPtr->caps;
    gPadPtr->kind = gPadInputPtr->kind;
    gPadPtr->id = gPadInputPtr->id;
    held = gPadInputPtr->held & (0xFFFF & ~PAD_DIR_MASK);
    held |= gPadDirRemap[cfg->dirMode][gPadInputPtr->held & PAD_DIR_MASK];
    held |= gPadDirRemap[cfg->stickLMode][(gPadInputPtr->held >> PAD_LSTICK_SHIFT) & PAD_DIR_MASK] << PAD_LSTICK_SHIFT;
    held |= gPadDirRemap[cfg->stickRMode][(gPadInputPtr->held >> PAD_RSTICK_SHIFT) & PAD_DIR_MASK] << PAD_RSTICK_SHIFT;
    /* the directions' pressures, remapped as their bits are; written as a
     * byte offset (pressure[] is at 0x1C), as the match needs: indexing
     * pressure[] allocates the registers differently */
    for (i = 0; i < PAD_DIR_COUNT; i++) {
        press[i] = ((u8 *)gPadInputPtr)[gPadPressRemap[cfg->dirMode][i] + 28];
    }
    for (i = PAD_DIR_COUNT; i < PAD_KEYS; i++) {
        press[i] = gPadInputPtr->pressure[i];
    }
    for (i = 0; i < PAD_KEYS; i++) {
        gPadPtr->pressure[i] = 0;
    }
    bits = 0;
    for (i = 0; i < PAD_BUTTONS; i++, pressPtr++) {
        if (held & gPadBit[i]) {
            bits |= gPadBit[cfg->map[i]];
        }
        target = cfg->map[i];
        if (target < PAD_KEYS) {
            if (gPadPtr->pressure[target] < *pressPtr) {
                gPadPtr->pressure[target] = *pressPtr;
            }
        } else if (target > PAD_BUTTONS) {
            Pad_RaisePressure(gPadPtr->pressure, *pressPtr, gPadBit[target]);
        }
    }
    Pad_SetHeld(gPadPtr, bits);
    Pad_CountHolds(gPadPtr);
    gPadPtr->trigger = gPadPtr->pressed;
    gPadPtr->stickL = gPadInputPtr->stickL;
    switch (cfg->stickLMode) {
    case PAD_STICK_MIRROR_X:
        gPadPtr->stickL.x = -gPadPtr->stickL.x;
        gPadPtr->stickL.deg = 540 - gPadPtr->stickL.deg;
        break;
    case PAD_STICK_MIRROR_Y:
        gPadPtr->stickL.y = -gPadPtr->stickL.y;
        gPadPtr->stickL.deg = 360 - gPadPtr->stickL.deg;
        break;
    case PAD_STICK_MIRROR_XY:
        gPadPtr->stickL.x = -gPadPtr->stickL.x;
        gPadPtr->stickL.y = -gPadPtr->stickL.y;
        gPadPtr->stickL.deg += 180;
        break;
    }
    gPadPtr->stickR = gPadInputPtr->stickR;
    switch (cfg->stickRMode) {
    case PAD_STICK_MIRROR_X:
        gPadPtr->stickR.x = -gPadPtr->stickR.x;
        gPadPtr->stickR.deg = 540 - gPadPtr->stickR.deg;
        break;
    case PAD_STICK_MIRROR_Y:
        gPadPtr->stickR.y = -gPadPtr->stickR.y;
        gPadPtr->stickR.deg = 360 - gPadPtr->stickR.deg;
        break;
    case PAD_STICK_MIRROR_XY:
        gPadPtr->stickR.x = -gPadPtr->stickR.x;
        gPadPtr->stickR.y = -gPadPtr->stickR.y;
        gPadPtr->stickR.deg += 180;
        break;
    }
    gPadPtr->stickL.deg %= 360;
    gPadPtr->stickR.deg %= 360;
    Pad_StickAngle(&gPadPtr->stickL);
    Pad_StickAngle(&gPadPtr->stickR);
}

/* Raises the pressure of every button of bits (0..15) to value. */
void Pad_RaisePressure(u8 *pressure, u8 value, u32 bits)
{
    s32 i;

    for (i = 0; i < PAD_KEYS; i++) {
        if (bits & gPadBit[i]) {
            if (pressure[i] < value) {
                pressure[i] = value;
            }
            bits ^= gPadBit[i];
            if (bits == 0) {
                break;
            }
        }
    }
}

/* The stick's direction in radians, 0 to 2 pi (0 when centred). */
void Pad_StickAngle(PadStick *stick)
{
    s16 x = stick->x;
    s16 y = stick->y;
    f32 angle;

    if ((y | x) == 0) {
        angle = 0.0f;
    } else {
        angle = atan2(-y, x);
        if (angle < 0.0f) {
            angle += 6.2831855f;
        }
    }
    stick->rad = angle;
}

/* Sets the buttons held this frame, and from the previous frame's the
 * buttons pressed, released and changed. */
void Pad_SetHeld(PadInput *pad, u32 held)
{
    pad->prev = pad->held;
    pad->held = held;
    pad->pressed = pad->held & (pad->prev ^ pad->held);
    pad->released = pad->prev & (pad->prev ^ pad->held);
    pad->changed = pad->pressed | pad->released;
}

/* Counts the frames each button has been held, up to 255; a button let go
 * clears its whole PadHold (one halfword store). */
void Pad_CountHolds(PadInput *pad)
{
    s16 i;

    for (i = 0; i < PAD_BUTTONS; i++) {
        if (pad->held & gPadBit[i]) {
            if (pad->hold[i].count != 255) {
                pad->hold[i].count++;
            }
        } else {
            *(u16 *)&pad->hold[i] = 0;
        }
    }
}

u8 Pad_GetConnected(void)
{
    return gPadConnected;
}

void Pad_EnableVibration(u8 enable)
{
    Pad_SetVibrationFlag(enable);
}

/* Starts libpad2 and opens the port's socket; 0 on failure. */
s32 Pad_Open(void)
{
    s32 i;

    if (Pad_StartLibs() == 0) {
        return 0;
    }
    Pad_ClearBuffers();
    gPadDataNone.status = 255;
    gPadDataNone.buttons = 65535;  /* active low: none held */
    gPadData[0] = gPadDataNone;
    gPadPort[0].buf = gPadSocketBuf;
    gPadPort[0].state = 0;
    gPadPort[0].setup = 0;
    gPadPort[0].port = 0;
    gPadPort[0].slot = 0;
    gPadPort[0].vibOk = 0;
    if (gPadFirstSocket == -1) {
        Pad_OpenSocket(0, 0, &gPadPort[0]);
    } else {
        for (i = 0; i < gPadPortCount; i++) {
            Pad_OpenSocket(gPadFirstSocket, i, &gPadPort[i]);
        }
        if (gPadPortCount <= 0) {
            Pad_OpenSocket((gPadFirstSocket + 1) & 1, 0, &gPadPort[i]);
        }
    }
    gPadLimits[0] = gPadDefaultLimits;
    gPadVibrationLast = 0;
    gPadVibration = 0;
    gPadRaw[0].id.port = gPadPort[0].port;
    gPadRaw[0].id.slot = gPadPort[0].slot;
    return 1;
}

void Pad_Close(void)
{
    Pad_ClearBuffers();
    Pad_EndLibs();
}

void Pad_SetVibrationFlag(u8 enable)
{
    gPadVibration = enable;
}

/* Sets a port's thresholds: the least pressure that counts and the
 * sticks' dead zones. */
void Pad_SetLimits(s32 port, s16 pressMin, s16 deadL, s16 deadR)
{
    gPadLimits[port].pressMin = pressMin;
    gPadLimits[port].deadL = deadL;
    gPadLimits[port].deadR = deadR;
}

/* Reads port 0 into gPadRaw: the directions remapped, the sticks as
 * direction bits 16..23. */
void Pad_Read(void)
{
    PadPort *port;
    PadRaw *raw;
    PadLimits *lim;
    u8 *vibReq;
    u8 *vibCur;
    s32 dirL;
    s32 dirR;

    raw = &gPadRaw[0];
    port = &gPadPort[0];
    lim = &gPadLimits[0];
    vibReq = &gPadVibration;
    vibCur = &gPadVibrationLast;
    if (Pad_ConvertData(0) != 0) {
        raw->id.vib = port->vibOk;
        raw->buttons = (raw->buttons & (0xFFFF & ~PAD_DIR_MASK)) | gPadDpadFix[raw->buttons & PAD_DIR_MASK];
        Pad_StickPolar(&raw->stickL, lim->deadL);
        Pad_StickPolar(&raw->stickR, lim->deadR);
        dirL = Pad_StickDirBits(&raw->stickL);
        dirR = Pad_StickDirBits(&raw->stickR);
        raw->buttons |= dirL << PAD_LSTICK_SHIFT;
        raw->buttons |= dirR << PAD_RSTICK_SHIFT;
    }
    *vibCur = *vibReq;
}

/* Clears the port's state and read buffers. */
void Pad_ClearBuffers(void)
{
    Pad_ClearMem(&gPadRaw[0], sizeof(PadRaw));
    Pad_ClearMem(&gPadDataCur[0], sizeof(PadData));
    Pad_ClearMem(&gPadData[0], sizeof(PadData));
    Pad_ClearMem(&gPadDataNone, sizeof(PadData));
}

/* This file's initialised data. */
s32 gPadFirstSocket = {
#include "main/001BAFE0.inc"
};

/* This file's read-only data. */
#pragma rodata on
u8 gPadDirRemap[4][16] = {
#include "main/001C7F10.inc"
};
u8 gPadPressRemap[4][4] = {
#include "main/001C7F50.inc"
};
#pragma rodata off
