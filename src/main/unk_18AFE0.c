/*
 * The pad's lower half (main 0x18AFE0-0x18C2E0), under unk_189F90.c's
 * Pad_Update: libpad2's sockets (Pad_OpenSocket, Pad_ReadPort), the
 * conversion of a read into the core's button bits, pressures and sticks
 * (Pad_ConvertData, Pad_StickPolar), and the vibration. A port's pad is
 * described by its PadPort; its PadButton table (gPadButtonMaps, from
 * gPadMapDigital or gPadMapAnalog) gives each core button its libpad2 bit.
 *
 * Then the TIM2 picture helpers (0x18C2E0-0x18CC10) that
 * Tex_CreateFromTim2 (unk_180110.c) uses.
 */
#include "common.h"
#include "image.h"
#include "padio.h"

/* Built at optimization level 4 like unk_189F90.c (the same unit, probably):
 * Pad_ConvertData needs it, and every other function matches either way. */
#pragma optimization_level 4

extern PadPort gPadPort[];
extern u8 gPadStickDirBits[];         /* stick direction (per 22.5 degrees) -> pad bits */

double atan2(double y, double x);
double sqrt(double x);

extern PadRaw gPadRaw[];
extern PadData gPadDataCur[];
extern PadData gPadData[];
extern PadButton gPadButtonMaps[][16];  /* per port: the controller's buttons */
extern PadLimits gPadLimits[];
extern u32 gPadBit[];            /* bit i */
extern u8 gPadPressIndex[16];           /* per button: its pressure in PadData.press */

void Pad_ReadPort(s32 port);
s32 Pad_SetVibration(s32 port, s32 level, s32 on);

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
    case 0x10:
    case 0x20:
        bits = 0;
        for (i = 0; i < PAD_KEYS; i++) {
            if (gPadDataCur[port].buttons & gPadButtonMaps[port][i].mask) {
                bits |= gPadBit[i];
                gPadRaw[port].pressure[i] = 0x7F;   /* full, unless the pad gives one */
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
            gPadRaw[port].stickL.x = gPadDataCur[port].lx - 0x80;
            gPadRaw[port].stickL.y = gPadDataCur[port].ly - 0x80;
            if (gPadRaw[port].stickL.x < -PAD_STICK_MAX) {
                gPadRaw[port].stickL.x = -PAD_STICK_MAX;
            }
            if (gPadRaw[port].stickL.y < -PAD_STICK_MAX) {
                gPadRaw[port].stickL.y = -PAD_STICK_MAX;
            }
        }
        if (gPadRaw[port].caps & PAD_CAP_STICK_R) {
            gPadRaw[port].stickR.x = gPadDataCur[port].rx - 0x80;
            gPadRaw[port].stickR.y = gPadDataCur[port].ry - 0x80;
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
    u8 data[0x28];
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


/*
 * TIM2 pictures (0x18C2E0-0x18CC10): the header checks and the addresses of
 * picture 0's parts that Tex_CreateFromTim2 (unk_180110.c) needs. The
 * structures and type numbers are in include/image.h.
 */
s32 Tim2_CheckHeader(u8 *file);
static Tim2PictureHeader *Tim2_GetPicture(u8 *file, s32 index);
u8 *Tim2_GetImage(u8 *file, s32 pictIndex, s32 level);
u8 *Tim2_GetPictClut(u8 *file, s32 pictIndex);

/* The number of mip levels after the first of picture 0; 0 when the file
 * is not TIM2. */
s32 Tim2_LastLevel(u8 *file)
{
    Tim2PictureHeader *pict;

    if (Tim2_CheckHeader(file) == 0) {
        return 0;
    }
    pict = Tim2_GetPicture(file, 0);
    return (u8)(pict->mipMaps - 1);
}

/* Describes the image levels of picture 0 (up to 7). */
s32 Tim2_DescribeLevels(Image *image, u8 *file)
{
    Tim2PictureHeader *pict;
    s32 i;
    s32 levelWidth;
    s32 levelHeight;

    if (Tim2_CheckHeader(file) == 0) {
        return 0;
    }
    pict = Tim2_GetPicture(file, 0);
    if (pict->mipMaps > TIM2_MAX_LEVELS) {
        return 0;
    }
    levelWidth = pict->width;
    levelHeight = pict->height;
    for (i = 0; i < pict->mipMaps; i++) {
        image->flags = 0;
        image->width = levelWidth;
        image->height = levelHeight;
        switch (pict->imageType) {
        case TIM2_TYPE_INDEX4:
            image->flags |= IMAGE_INDEXED | IMAGE_NIBBLE_HI;
            image->bpp = 0;
            image->pitch = image->width >> 1;
            image->r.shift = 0;
            image->r.bits = 0;
            image->r.mask = 0;
            image->g.shift = 0;
            image->g.bits = 0;
            image->g.mask = 0;
            image->b.shift = 0;
            image->b.bits = 0;
            image->b.mask = 0;
            image->a.shift = 0;
            image->a.bits = 0;
            image->a.mask = 0;
            break;
        case TIM2_TYPE_INDEX8:
            image->flags |= IMAGE_INDEXED;
            image->bpp = 1;
            image->pitch = image->bpp * image->width;
            image->r.shift = 0;
            image->r.bits = 0;
            image->r.mask = 0;
            image->g.shift = 0;
            image->g.bits = 0;
            image->g.mask = 0;
            image->b.shift = 0;
            image->b.bits = 0;
            image->b.mask = 0;
            image->a.shift = 0;
            image->a.bits = 0;
            image->a.mask = 0;
            break;
        case TIM2_TYPE_RGB16:
            image->bpp = 2;
            image->pitch = image->bpp * image->width;
            IMAGE_RGBA5551(image);
            image->r.shift = 0; /* the GS order, blue high */
            image->b.shift = 10;
            break;
        case TIM2_TYPE_RGB24:
            image->bpp = 3;
            image->pitch = image->bpp * image->width;
            IMAGE_RGB888(image);
            IMAGE_GS_ORDER_32(image);
            break;
        case TIM2_TYPE_RGB32:
            image->bpp = 4;
            image->pitch = image->bpp * image->width;
            IMAGE_RGBA8888(image);
            IMAGE_GS_ORDER_32(image);
            break;
        }
        image++;
        levelWidth >>= 1;
        levelHeight >>= 1;
    }
    return 1;
}

/* Describes picture 0's palette (16 or 256 colours). */
s32 Tim2_DescribeClut(Image *image, u8 *file)
{
    Tim2PictureHeader *pict;

    if (Tim2_CheckHeader(file) == 0) {
        return 0;
    }
    pict = Tim2_GetPicture(file, 0);
    if (pict->mipMaps > TIM2_MAX_LEVELS) {
        return 0;
    }
    image->flags = 0;
    switch (pict->clutType) {
    case TIM2_TYPE_RGB16:
        image->bpp = 2;
        IMAGE_RGBA5551(image);
        image->r.shift = 0; /* the GS order, blue high */
        image->b.shift = 10;
        break;
    case TIM2_TYPE_RGB24:
        image->bpp = 3;
        IMAGE_RGB888(image);
        IMAGE_GS_ORDER_32(image);
        break;
    case TIM2_TYPE_RGB32:
        image->bpp = 4;
        IMAGE_RGBA8888(image);
        IMAGE_GS_ORDER_32(image);
        break;
    default:
        return 0;
    }
    switch (pict->imageType) {
    case TIM2_TYPE_INDEX4:
        if (pict->clutColors != 16) {
            return 0;
        }
        image->width = 16;
        image->height = 1;
        break;
    case TIM2_TYPE_INDEX8:
        if (pict->clutColors != 256) {
            return 0;
        }
        image->width = 256;
        image->height = 1;
        break;
    default:
        return 0;
    }
    image->pitch = image->bpp * image->width;
    return 1;
}

/* Level `level` of picture 0's image. */
u8 *Tim2_GetLevel(u8 *file, s32 level)
{
    return Tim2_GetImage(file, 0, level);
}

/* Picture 0's palette. */
u8 *Tim2_GetClut(u8 *file)
{
    return Tim2_GetPictClut(file, 0);
}

/* 1 for a TIM2 file of version 3, or 4 with format 0 or 1, holding one
 * picture; else 0. */
s32 Tim2_CheckHeader(u8 *file)
{
    Tim2FileHeader *header = (Tim2FileHeader *)file;
    s32 kind;
    u8 fileFormat;

    if (header->id[0] == 'T' && header->id[1] == 'I' && header->id[2] == 'M' && header->id[3] == '2') {
        kind = 1;
    } else if (header->id[0] == 'C' && header->id[1] == 'L' && header->id[2] == 'T' && header->id[3] == '2') {
        /* a palette file (CLT2) */
        return 0;
    } else {
        return 0;
    }
    fileFormat = header->format;
    if (!(header->version == 3 || (header->version == 4 && fileFormat <= 1U))) {
        return 0;
    }
    if (header->pictures != 1) {
        return 0;
    }
    return kind;
}

/* size rounded up to the file's alignment. */
static u32 Tim2_Align(u8 *file, u32 size)
{
    u32 align = ((Tim2FileHeader *)file)->format == 0 ? 0x10 : 0x80;

    if (size % align != 0) {
        size += align - (size - size / align * align) % align;
    }
    return size;
}

/* Picture n's header. */
static Tim2PictureHeader *Tim2_GetPicture(u8 *file, s32 index)
{
    s32 i;
    Tim2FileHeader *header;
    Tim2PictureHeader *pict;

    header = (Tim2FileHeader *)file;
    file += Tim2_Align(file, sizeof(Tim2FileHeader));
    pict = (Tim2PictureHeader *)file;
    for (i = 0; i < index; i++) {
        if (i >= header->pictures) {
            break;
        }
        file += Tim2_Align((u8 *)header, sizeof(Tim2PictureHeader));
        file += pict->totalSize;
        pict = (Tim2PictureHeader *)file;
    }
    return pict;
}

/*
 * Level `level` of picture 0's image (pictIndex is not used): after the
 * picture header, its mip-map header and the levels before it, each
 * aligned. Two forms are kept for the match: pict is the picture header
 * as bytes, its mipMaps byte (0x11) read by index (a field read compiles
 * differently), and the level-size index is unsigned (with a signed one
 * MWCC strength-reduces the pointer).
 */
u8 *Tim2_GetImage(u8 *file, s32 pictIndex, s32 level)
{
    u32 *sizes[8];
    u8 *cursor;
    u8 *pict;
    u32 headerSize;
    u32 mipHeaderSize;
    s32 i;
    u32 offset;
    u8 *image;

    cursor = (u8 *)Tim2_GetPicture(file, 0);
    pict = cursor;
    headerSize = sizeof(Tim2PictureHeader);
    if (pict[0x11] > 1) {
        cursor = pict + sizeof(Tim2PictureHeader);
        for (i = 0; i < pict[0x11] - 1; i++) {
            sizes[i] = &((Tim2MipHeader *)cursor)->levelSize[(u32)i];
        }
        if (pict[0x11] <= 4) {
            mipHeaderSize = 0x20;
        } else {
            mipHeaderSize = 0x30;
        }
        headerSize += mipHeaderSize;
    }
    cursor = pict + Tim2_Align(file, headerSize);
    if (pict[0x11] > 1) {
        offset = 0;
        for (i = 0; i < level; i++) {
            offset += Tim2_Align(file, *sizes[i]);
        }
        image = cursor + offset;
    } else {
        image = cursor;
    }
    return image;
}

/* Picture 0's palette (pictIndex is not used): after the picture header,
 * its mip-map header and the image; NULL when it has none. */
u8 *Tim2_GetPictClut(u8 *file, s32 pictIndex)
{
    u8 *cursor;
    Tim2PictureHeader *pict;
    u32 headerSize;
    u32 mipHeaderSize;
    u8 *clut;

    cursor = (u8 *)Tim2_GetPicture(file, 0);
    pict = (Tim2PictureHeader *)cursor;
    headerSize = sizeof(Tim2PictureHeader);
    if (pict->mipMaps > 1) {
        if (pict->mipMaps <= 4) {
            mipHeaderSize = 0x20;
        } else {
            mipHeaderSize = 0x30;
        }
        headerSize += mipHeaderSize;
    }
    cursor += Tim2_Align(file, headerSize);
    if (pict->clutSize != 0) {
        clut = cursor + pict->imageSize;
    } else {
        return NULL;
    }
    return clut;
}

/* unk_18AFE0's .data (0x1BAFF0-0x1BB130) */
PadButton gPadMapAnalog[16] = {
#include "main/001BAFF0.inc"
};
PadButton gPadMapDigital[16] = {
#include "main/001BB070.inc"
};
u8 gPadPressIndex[16] = {
#include "main/001BB0F0.inc"
};
u32 gPadDefaultLimits[4] = {
#include "main/001BB100.inc"
};
u32 gPadDpadFix[4] = {
#include "main/001BB110.inc"
};
u8 gPadStickDirBits[16] = {
#include "main/001BB120.inc"
};
