#ifndef PADIO_H
#define PADIO_H

/*
 * The core's pad input (src/main/unk_189F90.c, unk_18AFE0.c), over Sony's
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

/* The game's button bits (PadInput.held ...): bit i is gPadBit[i]. */
#define PAD_DIR_MASK 0x000F     /* 0..3: the directional buttons */
#define PAD_LSTICK_SHIFT 16     /* 16..19: the left stick as 4 directions */
#define PAD_RSTICK_SHIFT 20     /* 20..23: the right stick */
#define PAD_BUTTONS 24

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
    /* 0x02 */ u16 kind;        /* 0: none; 1 ...; 0x8000: not supported */
    /* 0x04 */ PadId id;
    /* 0x08 */ u32 buttons;     /* the game's bits */
    /* 0x0C */ u8 pressure[16]; /* per button bit 0..15 */
    /* 0x1C */ PadStick stickL;
    /* 0x28 */ PadStick stickR;
} PadRaw;

#define PAD_CAP_PRESS_DIR 0x01  /* pressure of the kind-2 buttons */
#define PAD_CAP_PRESS_SHAPE 0x02 /* pressure of the kind-3 buttons */
#define PAD_CAP_PRESS_PAD 0x10  /* pressure of the kind-1 buttons */
#define PAD_CAP_STICK_L 0x20
#define PAD_CAP_STICK_R 0x40

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
    /* 0x1C */ u8 pressure[16];
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

/* The libpad-style data of a read (0x20 bytes). */
typedef struct PadData {
    /* 0x00 */ u8 status;
    /* 0x01 */ u8 mode;
    /* 0x02 */ u16 buttons;     /* active low */
    /* 0x04 */ u8 rx;
    /* 0x05 */ u8 ry;
    /* 0x06 */ u8 lx;
    /* 0x07 */ u8 ly;
    /* 0x08 */ u8 press[12];    /* right left up down, triangle circle
                                   cross square, L1 R1 L2 R2 */
    /* 0x14 */ u8 unk14[0x20 - 0x14];
} PadData;

/* A controller button: its libpad2 bit and its kind (0x08 bytes). */
typedef struct PadButton {
    /* 0x00 */ u32 mask;        /* in PadData.buttons (inverted) */
    /* 0x04 */ s32 kind;        /* 1..3: pressure from PadData.press, 0: none */
} PadButton;

/* A libpad2 socket (0x18 bytes). */
typedef struct PadPort {
    /* 0x00 */ u8 state;        /* 0: closed; 1: the data is this frame's (read,
                                   or no pad: gPadDataNone); 2: not read */
    /* 0x01 */ u8 setup;        /* profile read */
    /* 0x02 */ u8 port;
    /* 0x03 */ u8 slot;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 vibOk;        /* has actuators */
    /* 0x06 */ u8 socket;
    /* 0x07 */ u8 type;         /* 0: digital, 1: analog, 2: pressure */
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
