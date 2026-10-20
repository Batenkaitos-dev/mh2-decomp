#ifndef PADIO_H
#define PADIO_H

/*
 * The core's pad input, over Sony's
 * libpad2 (sockets on the device bus, libdbc) and libvib.
 *
 * Every frame (Pad_Update) the port is read (Pad_ReadPort: scePad2Read into
 * the libpad-style PadData), turned into a PadRaw (Pad_ConvertData: button
 * bits in the game's order, pressures, sticks), then copied into the pad
 * state gPadIn (the buttons as read) and remapped by the configuration
 * (PadConfig) into gPad, the state the game reads. The arrays indexed by
 * port have one element: one port is used.
 */
#include "common.h"

/*
 * The core's button bits (PadRaw.buttons, PadInput.held ...): bit i is
 * gPadBit[i]. Bits 0..15 are the buttons, in the order of gPadMapAnalog
 *, which gives each its libpad2 bit; bits 16..19 are the
 * left stick and 20..23 the right stick as directions (Pad_StickDirBits),
 * in the D-pad's order. sub_main's PadState_Update reorders them into the
 * game's PAD_* bits (include/pad.h). Named for the default configuration.
 */
#define PADIO_UP 0x0001
#define PADIO_DOWN 0x0002
#define PADIO_LEFT 0x0004
#define PADIO_RIGHT 0x0008
#define PADIO_CROSS 0x0010
#define PADIO_CIRCLE 0x0020
#define PADIO_R2 0x0040
#define PADIO_L2 0x0080
#define PADIO_SQUARE 0x0100
#define PADIO_TRIANGLE 0x0200
#define PADIO_R1 0x0400
#define PADIO_L1 0x0800
#define PADIO_L3 0x1000
#define PADIO_R3 0x2000
#define PADIO_SELECT 0x4000
#define PADIO_START 0x8000
#define PAD_DIR_MASK 0x000F     /* 0..3: the directional buttons */
#define PAD_DIR_COUNT 4
#define PAD_LSTICK_SHIFT 16     /* 16..19: the left stick as 4 directions */
#define PAD_RSTICK_SHIFT 20     /* 20..23: the right stick */
#define PAD_BUTTONS 24
#define PAD_KEYS 16             /* the pad's buttons, bits 0..15, each with a pressure */
#define PAD_STICK_MAX 127       /* a stick's reach from the centre */

/* PadRaw.kind, PadInput.kind: the pad in the port (0x8000 when its kind
 * has neither bit 0 nor bit 1: Pad_ConvertData). */
#define PAD_KIND_NONE 0
#define PAD_KIND_STANDARD 1     /* what Pad_ReadPort sets for every pad type */
#define PAD_KIND_UNSUPPORTED 0x8000

/* PadPort.state. */
#define PAD_PORT_CLOSED 0
#define PAD_PORT_READ 1         /* the data is this frame's (read, or no pad: gPadDataNone) */
#define PAD_PORT_UNREAD 2       /* opened, not read yet */

/* PadPort.type, from the pad's button profile (Pad_ReadPort). */
#define PAD_TYPE_DIGITAL 0
#define PAD_TYPE_ANALOG 1
#define PAD_TYPE_PRESSURE 2     /* analog with pressure-sensitive buttons */

/* PadButton.kind: which of the pad's pressures the button has, and which
 * PAD_CAP_PRESS_* bit gates it (gPadMapAnalog). */
#define PAD_PRESS_NONE 0
#define PAD_PRESS_DPAD 1        /* the directions */
#define PAD_PRESS_SHAPES 2      /* cross, circle, square and triangle */
#define PAD_PRESS_SHOULDERS 3   /* L1, R1, L2 and R2 */

/* PadConfig.stickLMode, stickRMode: how Pad_Remap mirrors the stick. */
#define PAD_STICK_MIRROR_X 1
#define PAD_STICK_MIRROR_Y 2
#define PAD_STICK_MIRROR_XY 3

/* An analog stick (0x0C bytes). */
typedef struct PadStick {
    /* 0x00 */ s16 x;           /* -128..127, 0 inside the dead zone */
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 mag;         /* distance from the centre, 0..127 */
    /* 0x06 */ s16 deg;         /* direction in degrees, 0..359 */
    /* 0x08 */ f32 rad;         /* the same in radians, 0..2 pi */
} PadStick;

/* Where the port is (4 bytes, copied as one word). */
typedef struct PadId {
    /* 0x00 */ u8 port;
    /* 0x01 */ u8 slot;
    /* 0x02 */ u8 vib;          /* PadPort.vibOk */
    /* 0x03 */ u8 unk3;
} __attribute__((aligned(4))) PadId;

/* The port's state after a read (0x34 bytes, gPadRaw). */
typedef struct PadRaw {
    /* 0x00 */ u8 state;        /* PadPort.state */
    /* 0x01 */ u8 caps;         /* PAD_CAP_* */
    /* 0x02 */ u16 kind;        /* PAD_KIND_* */
    /* 0x04 */ PadId id;
    /* 0x08 */ u32 buttons;     /* the game's bits */
    /* 0x0C */ u8 pressure[PAD_KEYS]; /* per button bit 0..15 */
    /* 0x1C */ PadStick stickL;
    /* 0x28 */ PadStick stickR;
} PadRaw;

#define PAD_CAP_PRESS_SHAPES 0x01    /* pressure of the PAD_PRESS_SHAPES buttons */
#define PAD_CAP_PRESS_SHOULDERS 0x02 /* pressure of the PAD_PRESS_SHOULDERS buttons */
#define PAD_CAP_PRESS_DPAD 0x10      /* pressure of the PAD_PRESS_DPAD buttons */
#define PAD_CAP_STICK_L 0x20
#define PAD_CAP_STICK_R 0x40

/* A port's PAD_KEYS pressures as one block: Pad_Update copies them
 * with one block copy, which its match needs. */
typedef struct PadPressures {
    u8 v[PAD_KEYS];
} PadPressures;

/* Frames a button has been held (2 bytes). */
typedef struct PadHold {
    /* 0x00 */ u8 count;        /* up to 0xFF */
    /* 0x01 */ u8 unk1;
} PadHold;

/* The pad state the game reads (0x88 bytes): gPadIn as read, gPad
 * remapped. */
typedef struct PadInput {
    /* 0x00 */ u8 state;
    /* 0x01 */ u8 caps;
    /* 0x02 */ u16 kind;
    /* 0x04 */ PadId id;
    /* 0x08 */ u32 held;
    /* 0x0C */ u32 prev;        /* held in the frame before */
    /* 0x10 */ u32 pressed;     /* newly down this frame */
    /* 0x14 */ u32 released;
    /* 0x18 */ u32 changed;
    /* 0x1C */ u8 pressure[PAD_KEYS];
    /* 0x2C */ PadStick stickL;
    /* 0x38 */ PadStick stickR;
    /* 0x44 */ u32 trigger;     /* = pressed */
    /* 0x48 */ PadHold hold[PAD_BUTTONS];
    /* 0x78 */ u8 unk78[0x88 - 0x78];
} PadInput;

/* Button mapping (0x2C bytes; the default is gPadDefaultConfig). */
typedef struct PadConfig {
    /* 0x00 */ u8 map[32];      /* game bit i -> bit map[i] (>= 25: several) */
    /* 0x20 */ u8 dirMode;      /* gPadDirMap / gPadDirPressMap row */
    /* 0x21 */ u8 stickLMode;   /* 1: x mirrored, 2: y, 3: both */
    /* 0x22 */ u8 stickRMode;
    /* 0x23 */ u8 unk23;
    /* 0x24 */ s16 pressMin;    /* PadLimits */
    /* 0x26 */ s16 deadL;
    /* 0x28 */ s16 deadR;
    /* 0x2A */ s16 unk2A;
} PadConfig;

/* Thresholds of a port (8 bytes). */
typedef struct PadLimits {
    /* 0x00 */ s16 pressMin;    /* a pressure below it reads as 0 */
    /* 0x02 */ s16 deadL;       /* stick dead zones (distance) */
    /* 0x04 */ s16 deadR;
    /* 0x06 */ s16 unk6;
} PadLimits;

#define PAD_DATA_PRESSURES 12  /* the pressures a read gives (PadData.press) */

/* The libpad-style data of a read (0x20 bytes). */
typedef struct PadData {
    /* 0x00 */ u8 status;
    /* 0x01 */ u8 mode;
    /* 0x02 */ u16 buttons;     /* active low */
    /* 0x04 */ u8 rx;
    /* 0x05 */ u8 ry;
    /* 0x06 */ u8 lx;
    /* 0x07 */ u8 ly;
    /* 0x08 */ u8 press[PAD_DATA_PRESSURES];    /* right left up down, triangle circle
                                   cross square, L1 R1 L2 R2 */
    /* 0x14 */ u8 unk14[0x20 - 0x14];
} PadData;

/* A controller button: its libpad2 bit and its kind (0x08 bytes). */
typedef struct PadButton {
    /* 0x00 */ u32 mask;        /* in PadData.buttons (inverted) */
    /* 0x04 */ s32 kind;        /* PAD_PRESS_*: its pressure in PadData.press */
} PadButton;

/* A libpad2 socket (0x18 bytes). */
typedef struct PadPort {
    /* 0x00 */ u8 state;        /* PAD_PORT_* */
    /* 0x01 */ u8 setup;        /* profile read */
    /* 0x02 */ u8 port;
    /* 0x03 */ u8 slot;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 vibOk;        /* has actuators */
    /* 0x06 */ u8 socket;
    /* 0x07 */ u8 type;         /* PAD_TYPE_* */
    /* 0x08 */ void *buf;       /* the socket's buffer, 64-byte aligned */
    /* 0x0C */ u32 profile;     /* scePad2GetButtonProfile */
    /* 0x10 */ u32 vibProfile;  /* sceVibGetProfile */
    /* 0x14 */ s32 vibTime;     /* frames left */
} PadPort;

/* libpad2, libvib, libdbc */
typedef struct scePad2SocketParam {
    s32 flag;
    s32 port;
    s32 slot;
    s32 number;
    u8 unk10[0x10];             /* not set by the game; the frame of
                                   Pad_OpenSocket needs the 0x20 bytes */
} scePad2SocketParam;

s32 sceDbcInit(void);
s32 sceDbcEnd(void);
s32 scePad2Init(s32 mode);
s32 scePad2End(void);
s32 scePad2CreateSocket(scePad2SocketParam *param, void *buf);
s32 scePad2DeleteSocket(s32 socket);
s32 scePad2Read(s32 socket, u8 *data);
s32 scePad2GetButtonProfile(s32 socket, u8 *profile);
s32 scePad2GetState(s32 socket);
s32 sceVibGetProfile(s32 socket, u8 *profile);
s32 sceVibSetActParam(s32 socket, s32 profileSize, u8 *profile, s32 dataSize, u8 *data);

#endif /* PADIO_H */
