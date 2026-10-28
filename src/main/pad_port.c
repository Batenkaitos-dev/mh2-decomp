/*
 * The pad's lower half, under Pad_Update: libpad2's sockets (Pad_OpenSocket, Pad_ReadPort), the
 * conversion of a read into the core's button bits, pressures and sticks
 * (Pad_ConvertData, Pad_StickPolar), and the vibration. A port's pad is
 * described by its PadPort; its PadButton table (gPadButtonMaps, from
 * gPadMapDigital or gPadMapAnalog) gives each core button its libpad2 bit.
 */
#include "common.h"
#include "image.h"
#include "padio.h"
#include "pad.h"
#include "texture.h"
#include "libm.h"

/* Built at optimization level 4, as the pad's upper half: Pad_ConvertData
 * needs it, and every other function matches either way. */
#pragma optimization_level 4

extern PadPort gPadPort[];
extern u8 gPadStickDirBits[];         /* stick direction (per 22.5 degrees) -> pad bits */

extern PadRaw gPadRaw[];
extern PadData gPadDataCur[];
extern PadData gPadData[];
extern PadButton gPadButtonMaps[][16];  /* per port: the controller's buttons */
extern PadLimits gPadLimits[];
extern u8 gPadPressIndex[16];           /* per button: its pressure in PadData.press */

/* Reads a port and converts its PadData into its PadRaw: the buttons as
 * the core's bits, the pressures (halved, 0 under the port's least), the
 * sticks centred on 0. A kind with neither bit 0 nor bit 1 becomes
 * PAD_KIND_UNSUPPORTED, which reads as nothing. Returns 0 when the port
 * is closed. */
s32 Pad_ConvertData(s32 port)
{
    u32 bits;
    u8 press[PAD_DATA_PRESSURES];
    s32 i;
    u8 value;

    if (gPadPort[port].state == PAD_PORT_CLOSED) {
        return 0;
    }
    Pad_ReadPort(port);
    if (gPadRaw[port].kind != PAD_KIND_NONE && (gPadRaw[port].kind & 3) == 0) {
        gPadRaw[port].kind = PAD_KIND_UNSUPPORTED;
    }
    for (i = 0; i < PAD_KEYS; i++) {
        gPadRaw[port].pressure[i] = 0;
    }
    for (i = 0; i < PAD_DATA_PRESSURES; i++) {
        press[i] = gPadDataCur[port].press[i] / 2;
    }
    gPadDataCur[port].buttons = ~gPadDataCur[port].buttons;
    /* the kinds with one bit set (Pad_ReadPort sets 1) */
    switch (gPadRaw[port].kind) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
        bits = 0;
        for (i = 0; i < PAD_KEYS; i++) {
            if (gPadDataCur[port].buttons & gPadButtonMaps[port][i].mask) {
                bits |= gPadBit[i];
                gPadRaw[port].pressure[i] = 127;   /* full, unless the pad gives one */
            }
            switch (gPadButtonMaps[port][i].kind) {
            case PAD_PRESS_DPAD:
                if (gPadRaw[port].caps & PAD_CAP_PRESS_DPAD) {
                    value = press[gPadPressIndex[i]];
                    if (value < gPadLimits[port].pressMin) {
                        gPadRaw[port].pressure[i] = 0;
                    } else {
                        gPadRaw[port].pressure[i] = value;
                    }
                }
                break;
            case PAD_PRESS_SHAPES:
                if (gPadRaw[port].caps & PAD_CAP_PRESS_SHAPES) {
                    value = press[gPadPressIndex[i]];
                    if (value < gPadLimits[port].pressMin) {
                        gPadRaw[port].pressure[i] = 0;
                    } else {
                        gPadRaw[port].pressure[i] = value;
                    }
                }
                break;
            case PAD_PRESS_SHOULDERS:
                if (gPadRaw[port].caps & PAD_CAP_PRESS_SHOULDERS) {
                    value = press[gPadPressIndex[i]];
                    if (value < gPadLimits[port].pressMin) {
                        gPadRaw[port].pressure[i] = 0;
                    } else {
                        gPadRaw[port].pressure[i] = value;
                    }
                }
                break;
            }
        }
        gPadRaw[port].buttons = bits;
        if (gPadRaw[port].caps & PAD_CAP_STICK_L) {
            gPadRaw[port].stickL.x = gPadDataCur[port].lx - 128;
            gPadRaw[port].stickL.y = gPadDataCur[port].ly - 128;
            if (gPadRaw[port].stickL.x < -PAD_STICK_MAX) {
                gPadRaw[port].stickL.x = -PAD_STICK_MAX;
            }
            if (gPadRaw[port].stickL.y < -PAD_STICK_MAX) {
                gPadRaw[port].stickL.y = -PAD_STICK_MAX;
            }
        }
        if (gPadRaw[port].caps & PAD_CAP_STICK_R) {
            gPadRaw[port].stickR.x = gPadDataCur[port].rx - 128;
            gPadRaw[port].stickR.y = gPadDataCur[port].ry - 128;
            if (gPadRaw[port].stickR.x < -PAD_STICK_MAX) {
                gPadRaw[port].stickR.x = -PAD_STICK_MAX;
            }
            if (gPadRaw[port].stickR.y < -PAD_STICK_MAX) {
                gPadRaw[port].stickR.y = -PAD_STICK_MAX;
            }
        }
        gPadData[port] = gPadDataCur[port];
        gPadData[port].buttons = ~gPadData[port].buttons;
        break;
    default:
        gPadRaw[port].buttons = 0;
        gPadRaw[port].stickL.x = 0;
        gPadRaw[port].stickL.y = 0;
        gPadRaw[port].stickR.x = 0;
        gPadRaw[port].stickR.y = 0;
        break;
    }
    return 1;
}

/* Polar coordinates of a stick: the angle (y up) in radians and degrees,
 * the distance up to PAD_STICK_MAX; inside the dead zone everything is 0. */
void Pad_StickPolar(PadStick *stick, s16 deadZone)
{
    f32 rad;

    if ((stick->y | stick->x) == 0) {
        rad = 0.0f;
    } else {
        rad = atan2(-stick->y, stick->x);
        if (rad < 0.0f) {
            rad += 6.2831855f;
        }
    }
    stick->rad = rad;
    stick->deg = 360.0f * (stick->rad / 6.2831855f);
    stick->mag = sqrt((double)(stick->x * stick->x) + (double)(stick->y * stick->y));
    if (stick->mag > PAD_STICK_MAX) {
        stick->mag = PAD_STICK_MAX;
    }
    if (stick->mag < deadZone) {
        stick->x = 0;
        stick->y = 0;
        stick->rad = 0.0f;
        stick->deg = 0;
        stick->mag = 0;
    }
}

/* The direction bits of a stick (PAD_DIR_MASK's, from its sixteen sectors
 * of 22.5 degrees), 0 near the centre. */
u8 Pad_StickDirBits(PadStick *stick)
{
    if (stick->mag < 37) {
        return 0;
    }
    return gPadStickDirBits[(u32)(stick->deg / 22.5f)];
}

/* Starts libdbc and libpad2; 1 when both are up. */
s32 Pad_StartLibs(void)
{
    s32 result;

    if (sceDbcInit() != 1) {
        return 0;
    }
    result = scePad2Init(0);
    return result == 1;
}

/* Opens the socket of a port and slot. */
void Pad_OpenSocket(s32 port, s32 slot, PadPort *padPort)
{
    scePad2SocketParam param;

    param.number = 0;
    param.port = port;
    param.slot = slot;
    param.flag = 2;
    if ((padPort->socket = scePad2CreateSocket(&param, padPort->buf)) >= 0) {
        padPort->state = 1;
        padPort->setup = 0;
        padPort->port = port;
        padPort->slot = slot;
    }
}

/* Closes the socket, then libpad2 and libdbc. */
void Pad_EndLibs(void)
{
    PadPort *padPort = (PadPort *)gPadPort;

    if (padPort->state != 0) {
        scePad2DeleteSocket(padPort->socket);
    }
    scePad2End();
    sceDbcEnd();
}

extern PadData gPadDataNone;              /* no pad */
extern PadButton gPadMapAnalog[16];        /* analog and pressure pads */
extern PadButton gPadMapDigital[16];        /* digital pads */


/* Reads a port: the socket's state, then the first time the pad's profile
 * (its type, the button table, the actuators), afterwards the data into
 * gPadDataCur[port] (the previous frame's stays in gPadData). */
void Pad_ReadPort(s32 port)
{
    u8 data[40];
    u8 buttonProfile[4];
    u8 vibBytes[4];
    s32 i;

    gPadDataCur[port] = gPadData[port];
    /* scePad2GetState: 1 goes on to read; 0 and the rest read as no pad */
    switch (scePad2GetState(gPadPort[port].socket)) {
    case 1:
        gPadPort[port].state = PAD_PORT_UNREAD;
        break;
    case 0:
        gPadPort[port].state = PAD_PORT_READ;
        gPadPort[port].setup = 0;
        gPadPort[port].unk4 = 0;
        gPadPort[port].vibOk = 0;
        gPadPort[port].profile = 0;
        gPadPort[port].vibProfile = 0;
        gPadDataCur[port] = gPadDataNone;
        gPadRaw[port].kind = PAD_KIND_NONE;
        gPadRaw[port].caps = 0;
        gPadRaw[port].state = gPadPort[port].state;
        return;
    case 2:
    case 3:
    default:
        gPadPort[port].state = PAD_PORT_UNREAD;
        gPadPort[port].setup = 0;
        gPadPort[port].unk4 = 0;
        gPadPort[port].vibOk = 0;
        gPadPort[port].profile = 0;
        gPadPort[port].vibProfile = 0;
        gPadDataCur[port] = gPadDataNone;
        gPadRaw[port].kind = PAD_KIND_NONE;
        gPadRaw[port].caps = 0;
        gPadRaw[port].state = gPadPort[port].state;
        return;
    }
    if (gPadPort[port].setup == 0) {
        if (scePad2GetButtonProfile(gPadPort[port].socket, buttonProfile) < 0) {
            return;
        }
        gPadPort[port].profile = (buttonProfile[3] << 24) | (buttonProfile[2] << 16) | (buttonProfile[1] << 8) | buttonProfile[0];
        if (sceVibGetProfile(gPadPort[port].socket, vibBytes) >= 0) {
            gPadPort[port].vibProfile = vibBytes[0];
        }
        /* the profile's high half lists the analog parts: the sticks from bit
         * 16, the pressures from bit 20 */
        if (gPadPort[port].profile & 0xFFFF0000) {
            if (gPadPort[port].profile & 0xFFF00000) {
                gPadPort[port].type = PAD_TYPE_PRESSURE;
            } else {
                gPadPort[port].type = PAD_TYPE_ANALOG;
            }
        } else {
            gPadPort[port].type = PAD_TYPE_DIGITAL;
        }
        switch (gPadPort[port].type) {
        case PAD_TYPE_DIGITAL:
            gPadRaw[port].kind = PAD_KIND_STANDARD;
            gPadRaw[port].caps = 0;
            for (i = 0; i < PAD_KEYS; i++) {
                gPadButtonMaps[port][i].mask = gPadMapDigital[i].mask;
                gPadButtonMaps[port][i].kind = gPadMapDigital[i].kind;
            }
            break;
        case PAD_TYPE_ANALOG:
            gPadRaw[port].kind = PAD_KIND_STANDARD;
            gPadRaw[port].caps = PAD_CAP_STICK_L | PAD_CAP_STICK_R;
            for (i = 0; i < PAD_KEYS; i++) {
                gPadButtonMaps[port][i].mask = gPadMapAnalog[i].mask;
                gPadButtonMaps[port][i].kind = gPadMapAnalog[i].kind;
            }
            break;
        case PAD_TYPE_PRESSURE:
            gPadRaw[port].kind = PAD_KIND_STANDARD;
            gPadRaw[port].caps = PAD_CAP_STICK_L | PAD_CAP_STICK_R | PAD_CAP_PRESS_DPAD |
                                    PAD_CAP_PRESS_SHOULDERS | PAD_CAP_PRESS_SHAPES;
            for (i = 0; i < PAD_KEYS; i++) {
                gPadButtonMaps[port][i].mask = gPadMapAnalog[i].mask;
                gPadButtonMaps[port][i].kind = gPadMapAnalog[i].kind;
            }
            break;
        }
        if ((gPadPort[port].vibProfile & 3) == 0) {   /* neither motor */
            gPadPort[port].vibOk = 0;
        } else {
            gPadPort[port].vibOk = 1;
        }
        gPadPort[port].setup++;
    } else {
        if (scePad2Read(gPadPort[port].socket, data) < 0) {
            return;
        }
        gPadPort[port].state = PAD_PORT_READ;
        gPadDataCur[port].status = 0;
        gPadDataCur[port].mode = 0;
        gPadDataCur[port].buttons = (data[0] << 8) | data[1];
        switch (gPadPort[port].type) {
        case PAD_TYPE_DIGITAL:
            break;
        case PAD_TYPE_ANALOG:
            gPadDataCur[port].rx = data[2];
            gPadDataCur[port].ry = data[3];
            gPadDataCur[port].lx = data[4];
            gPadDataCur[port].ly = data[5];
            break;
        case PAD_TYPE_PRESSURE:
            gPadDataCur[port].rx = data[2];
            gPadDataCur[port].ry = data[3];
            gPadDataCur[port].lx = data[4];
            gPadDataCur[port].ly = data[5];
            gPadDataCur[port].press[0] = data[6];
            gPadDataCur[port].press[1] = data[7];
            gPadDataCur[port].press[2] = data[8];
            gPadDataCur[port].press[3] = data[9];
            gPadDataCur[port].press[4] = data[10];
            gPadDataCur[port].press[5] = data[11];
            gPadDataCur[port].press[6] = data[12];
            gPadDataCur[port].press[7] = data[13];
            gPadDataCur[port].press[8] = data[14];
            gPadDataCur[port].press[9] = data[15];
            gPadDataCur[port].press[10] = data[16];
            gPadDataCur[port].press[11] = data[17];
            break;
        default:
            gPadDataCur[port] = gPadDataNone;
            break;
        }
        if (gPadPort[port].vibTime != 0) {
            if (--gPadPort[port].vibTime == 0) {
                Pad_SetVibration(port, 0, 0);
            }
        }
    }
    gPadRaw[port].state = gPadPort[port].state;
}

/*
 * Sets the vibration of a port: on at level 0..7 for `frames` frames
 * (Pad_ReadPort counts them down and stops it), or off when frames is 0.
 * Returns 0 when the pad has no actuators. The tests of vibProfile are
 * assignments of 3, so the two-actuator form is always taken; the code
 * is Capcom's, kept as it is for the match.
 */
s32 Pad_SetVibration(s32 port, s32 level, s32 frames)
{
    u8 actuators;
    u8 data[3];
    u8 motorBits;
    s32 dataSize;
    if (gPadPort[port].vibProfile == 0) {
        return 0;
    }
    gPadPort[port].vibTime = frames;
    if (frames == 0) {
        if ((gPadPort[port].vibProfile = 3) != 0) {
            actuators = 3;
            dataSize = 2;
            data[0] = 0;
            data[1] = 0;
        } else {
            data[0] = 0;
            dataSize = 1;
            actuators = 2;
        }
    } else {
        motorBits = ((level & 7) << 5) | 0xF;
        if ((gPadPort[port].vibProfile = 3) != 0) {
            actuators = 3;
            dataSize = 2;
            data[0] = ((motorBits & 0x7F) << 1) | 1;
            data[1] = (motorBits & 0x80) >> 7;
        } else {
            data[0] = motorBits;
            dataSize = 1;
            actuators = 2;
        }
    }
    sceVibSetActParam(gPadPort[port].socket, 1, &actuators, dataSize, data);
    return 1;
}

/* This file's initialised data. */
PadButton gPadMapAnalog[16] = {
#include "main/001BAFF0.inc"
};
PadButton gPadMapDigital[16] = {
#include "main/001BB070.inc"
};
u8 gPadPressIndex[16] = {
#include "main/001BB0F0.inc"
};
PadLimits gPadDefaultLimits = {
#include "main/001BB100.inc"
};
u8 gPadDpadFix[16] = {
#include "main/001BB110.inc"
};
u8 gPadStickDirBits[16] = {
#include "main/001BB120.inc"
};
