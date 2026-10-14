/*
 * main 0x189F90-0x18AFE0: the movie library's start-up (CRI), then the pad
 * input (include/padio.h): the port's state after a read (gPadRaw, filled
 * in unk_18AFE0.c) is copied into gPadIn and remapped by the configuration
 * into gPad; button edges, hold counters and stick angles.
 */
#include "common.h"
#include "padio.h"

/* Built at optimization level 4: Pad_Remap needs it (a pointer kept
 * across its switches), and every other function matches either way. */
#pragma optimization_level 4

typedef struct SfdInitPrm {
    /* 0x00 */ f32 vhz;         /* display rate */
    /* 0x04 */ s32 dispCycle;
    /* 0x08 */ s32 dispLatency;
    /* 0x0C */ u8 decSvr;
    /* 0x10 */ s32 rsv[4];
} SfdInitPrm;

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
extern u32 gPadBit[];        /* bit i */
extern u8 gPadDirRemap[][16];
extern u8 gPadPressRemap[][4];

typedef struct PadPress {
    u8 v[16];
} PadPress;

double atan2(double y, double x);
void func_00161F00(s32 arg0);
void func_00161750(void (*func)(void), s32 arg1);
void mwPlyInitSfdFx(SfdInitPrm *prm);
void *memset(void *dst, s32 c, u32 n);

void func_00189FA0(void);
void Pad_ClearMem(void *dst, s32 size);
void Pad_ClearInputs(void);
void Pad_SetConfig(PadConfig *cfg, s32 port);
void Pad_Remap(void);
void Pad_RaisePressure(u8 *pressure, u8 value, u32 bits);
void Pad_StickAngle(PadStick *s);
void Pad_SetHeld(PadInput *p, u32 held);
void Pad_CountHolds(PadInput *p);
s32 Pad_Open(void);
void Pad_Close(void);
void Pad_SetVibrationFlag(u8 on);
void Pad_SetLimits(s32 port, s16 pressMin, s16 deadL, s16 deadR);
void Pad_Read(void);
void Pad_ClearBuffers(void);

/* unk_18AFE0.c */
s32 Pad_ConvertData(s32 port);
void Pad_StickPolar(PadStick *s, s16 dead);
u8 Pad_StickDirBits(PadStick *s);
s32 Pad_StartLibs(void);
void Pad_OpenSocket(s32 port, s32 slot, PadPort *p);
void Pad_EndLibs(void);

void AdxWrap_VintHook(s32 arg0)
{
    func_00161F00(arg0);
}

void func_00189FA0(void)
{
}

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
    func_00161750(func_00189FA0, 0);
    mwPlyInitSfdFx(&prm);
}

/* Clears size bytes: words, then the bytes left. */
void Pad_ClearMem(void *dst, s32 size)
{
    s32 i;
    s32 rest;
    s32 n;
    u32 *w = dst;
    u8 *b;

    rest = size & 3;
    i = 0;
    n = size / 4;
    for (; i < n; i++) {
        *w++ = 0;
    }
    if (rest != 0) {
        b = (u8 *)w;
        for (size = 0; size < rest; size++) {
            *b++ = 0;
        }
    }
}

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

void Pad_Update(void)
{
    PadRaw *raw;
    PadInput *in;

    Pad_Read();
    gPadConnected = 0;
    raw = (PadRaw *)gPadRaw;
    gPadInputPtr->state = raw->state;
    gPadInputPtr->caps = raw->caps;
    gPadInputPtr->kind = raw->kind;
    gPadInputPtr->id = raw->id;
    in = gPadInputPtr;
    if (in->kind != 0 && in->kind != 0x8000) {
        gPadConnected++;
    }
    in->stickL = raw->stickL;
    in->stickR = raw->stickR;
    *(PadPress *)in->pressure = *(PadPress *)raw->pressure;
    Pad_SetHeld(in, raw->buttons);
    Pad_CountHolds(gPadInputPtr);
    gPadInputPtr->trigger = gPadInputPtr->pressed;
    Pad_Remap();
}

/* gPad from gPadIn, remapped by the configuration of port 0. */
void Pad_Remap(void)
{
    u8 press[PAD_BUTTONS];
    PadConfig *cfg = (PadConfig *)gPadConfig;
    u32 bits;
    s16 i;
    u32 held;
    u8 *pp = press;
    u8 k;

    gPadPtr->state = gPadInputPtr->state;
    gPadPtr->caps = gPadInputPtr->caps;
    gPadPtr->kind = gPadInputPtr->kind;
    gPadPtr->id = gPadInputPtr->id;
    held = gPadInputPtr->held & 0xFFF0;
    held |= gPadDirRemap[cfg->dirMode][gPadInputPtr->held & 0xF];
    held |= gPadDirRemap[cfg->stickLMode][(gPadInputPtr->held >> 16) & 0xF] << 16;
    held |= gPadDirRemap[cfg->stickRMode][(gPadInputPtr->held >> 20) & 0xF] << 20;
    /* the directions pressed as remapped (pressure[] is at 0x1C) */
    for (i = 0; i < 4; i++) {
        press[i] = ((u8 *)gPadInputPtr)[gPadPressRemap[cfg->dirMode][i] + 0x1C];
    }
    for (i = 4; i < 16; i++) {
        press[i] = gPadInputPtr->pressure[i];
    }
    for (i = 0; i < 16; i++) {
        gPadPtr->pressure[i] = 0;
    }
    bits = 0;
    for (i = 0; i < PAD_BUTTONS; i++, pp++) {
        if (held & gPadBit[i]) {
            bits |= gPadBit[cfg->map[i]];
        }
        k = cfg->map[i];
        if (k < 16) {
            if (gPadPtr->pressure[k] < *pp) {
                gPadPtr->pressure[k] = *pp;
            }
        } else if (k > 24) {
            Pad_RaisePressure(gPadPtr->pressure, *pp, gPadBit[k]);
        }
    }
    Pad_SetHeld(gPadPtr, bits);
    Pad_CountHolds(gPadPtr);
    gPadPtr->trigger = gPadPtr->pressed;
    gPadPtr->stickL = gPadInputPtr->stickL;
    switch (cfg->stickLMode) {
    case 1:
        gPadPtr->stickL.x = -gPadPtr->stickL.x;
        gPadPtr->stickL.deg = 540 - gPadPtr->stickL.deg;
        break;
    case 2:
        gPadPtr->stickL.y = -gPadPtr->stickL.y;
        gPadPtr->stickL.deg = 360 - gPadPtr->stickL.deg;
        break;
    case 3:
        gPadPtr->stickL.x = -gPadPtr->stickL.x;
        gPadPtr->stickL.y = -gPadPtr->stickL.y;
        gPadPtr->stickL.deg += 180;
        break;
    }
    gPadPtr->stickR = gPadInputPtr->stickR;
    switch (cfg->stickRMode) {
    case 1:
        gPadPtr->stickR.x = -gPadPtr->stickR.x;
        gPadPtr->stickR.deg = 540 - gPadPtr->stickR.deg;
        break;
    case 2:
        gPadPtr->stickR.y = -gPadPtr->stickR.y;
        gPadPtr->stickR.deg = 360 - gPadPtr->stickR.deg;
        break;
    case 3:
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

    for (i = 0; i < 16; i++) {
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

/* The stick's direction in radians (0 when centred). */
void Pad_StickAngle(PadStick *s)
{
    s16 x = s->x;
    s16 y = s->y;
    f32 a;

    if ((y | x) == 0) {
        a = 0.0f;
    } else {
        a = atan2(-y, x);
        if (a < 0.0f) {
            a += 6.2831855f;
        }
    }
    s->rad = a;
}

void Pad_SetHeld(PadInput *p, u32 held)
{
    p->prev = p->held;
    p->held = held;
    p->pressed = p->held & (p->prev ^ p->held);
    p->released = p->prev & (p->prev ^ p->held);
    p->changed = p->pressed | p->released;
}

/* Counts the frames each button has been held. */
void Pad_CountHolds(PadInput *p)
{
    s16 i;

    for (i = 0; i < PAD_BUTTONS; i++) {
        if (p->held & gPadBit[i]) {
            if (p->hold[i].count != 0xFF) {
                p->hold[i].count++;
            }
        } else {
            *(u16 *)&p->hold[i] = 0;
        }
    }
}

u8 Pad_GetConnected(void)
{
    return gPadConnected;
}

void Pad_EnableVibration(u8 on)
{
    Pad_SetVibrationFlag(on);
}

/* Starts libpad2 and opens the port's socket; 0 on failure. */
s32 Pad_Open(void)
{
    s32 i;

    if (Pad_StartLibs() == 0) {
        return 0;
    }
    Pad_ClearBuffers();
    gPadDataNone.status = 0xFF;
    gPadDataNone.buttons = 0xFFFF;
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

void Pad_SetVibrationFlag(u8 on)
{
    gPadVibration = on;
}

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
        raw->buttons = (raw->buttons & 0xFFF0) | gPadDpadFix[raw->buttons & 0xF];
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

/* unk_189F90's .data (0x1BAFE0-0x1BAFF0) */
s32 gPadFirstSocket = {
#include "main/001BAFE0.inc"
};

/* unk_189F90's .rodata (0x1C7F10-0x1C7F60) */
#pragma rodata on
u8 gPadDirRemap[4][16] = {
#include "main/001C7F10.inc"
};
u8 gPadPressRemap[4][4] = {
#include "main/001C7F50.inc"
};
#pragma rodata off
