/*
 * main 0x18AFE0-0x18CC10: the pad code (0x18AFE0-0x18C2E0, with unk_189F90.c), then the TIM2
 * picture helpers that Tex_CreateFromTim2 (unk_180110.c) uses.
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

/* Converts a port's PadData into its PadRaw: the buttons as the game's
 * bits, pressures (halved, 0 under the limit), sticks centred on 0. */
/* Round 21: the button table, gPadBit and gPadPressIndex indexed by i (walkers: 18 words off). */
s32 Pad_ConvertData(s32 port)
{
    u32 bits;
    u8 press[12];
    s32 i;
    u8 v;

    if (gPadPort[port].state == 0) {
        return 0;
    }
    Pad_ReadPort(port);
    if (gPadRaw[port].kind != 0 && (gPadRaw[port].kind & 3) == 0) {
        gPadRaw[port].kind = 0x8000;
    }
    for (i = 0; i < 16; i++) {
        gPadRaw[port].pressure[i] = 0;
    }
    for (i = 0; i < 12; i++) {
        press[i] = gPadDataCur[port].press[i] / 2;
    }
    gPadDataCur[port].buttons = ~gPadDataCur[port].buttons;
    switch (gPadRaw[port].kind) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 0x10:
    case 0x20:
        bits = 0;
        for (i = 0; i < 16; i++) {
            if (gPadDataCur[port].buttons & gPadButtonMaps[port][i].mask) {
                bits |= gPadBit[i];
                gPadRaw[port].pressure[i] = 0x7F;
            }
            switch (gPadButtonMaps[port][i].kind) {
            case 1:
                if (gPadRaw[port].caps & PAD_CAP_PRESS_PAD) {
                    v = press[gPadPressIndex[i]];
                    if (v < gPadLimits[port].pressMin) {
                        gPadRaw[port].pressure[i] = 0;
                    } else {
                        gPadRaw[port].pressure[i] = v;
                    }
                }
                break;
            case 2:
                if (gPadRaw[port].caps & PAD_CAP_PRESS_DIR) {
                    v = press[gPadPressIndex[i]];
                    if (v < gPadLimits[port].pressMin) {
                        gPadRaw[port].pressure[i] = 0;
                    } else {
                        gPadRaw[port].pressure[i] = v;
                    }
                }
                break;
            case 3:
                if (gPadRaw[port].caps & PAD_CAP_PRESS_SHAPE) {
                    v = press[gPadPressIndex[i]];
                    if (v < gPadLimits[port].pressMin) {
                        gPadRaw[port].pressure[i] = 0;
                    } else {
                        gPadRaw[port].pressure[i] = v;
                    }
                }
                break;
            }
        }
        gPadRaw[port].buttons = bits;
        if (gPadRaw[port].caps & PAD_CAP_STICK_L) {
            gPadRaw[port].stickL.x = gPadDataCur[port].lx - 0x80;
            gPadRaw[port].stickL.y = gPadDataCur[port].ly - 0x80;
            if (gPadRaw[port].stickL.x < -0x7F) {
                gPadRaw[port].stickL.x = -0x7F;
            }
            if (gPadRaw[port].stickL.y < -0x7F) {
                gPadRaw[port].stickL.y = -0x7F;
            }
        }
        if (gPadRaw[port].caps & PAD_CAP_STICK_R) {
            gPadRaw[port].stickR.x = gPadDataCur[port].rx - 0x80;
            gPadRaw[port].stickR.y = gPadDataCur[port].ry - 0x80;
            if (gPadRaw[port].stickR.x < -0x7F) {
                gPadRaw[port].stickR.x = -0x7F;
            }
            if (gPadRaw[port].stickR.y < -0x7F) {
                gPadRaw[port].stickR.y = -0x7F;
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

/* Polar coordinates of a stick: angle (y up), distance up to 127; inside
 * the dead zone everything is 0. */
void Pad_StickPolar(PadStick *s, s16 dead)
{
    f32 rad;

    if ((s->y | s->x) == 0) {
        rad = 0.0f;
    } else {
        rad = atan2(-s->y, s->x);
        if (rad < 0.0f) {
            rad += 6.2831855f;
        }
    }
    s->rad = rad;
    s->deg = 360.0f * (s->rad / 6.2831855f);
    s->mag = sqrt((double)(s->x * s->x) + (double)(s->y * s->y));
    if (s->mag > 127) {
        s->mag = 127;
    }
    if (s->mag < dead) {
        s->x = 0;
        s->y = 0;
        s->rad = 0.0f;
        s->deg = 0;
        s->mag = 0;
    }
}

/* The pad bits of a stick's direction (sixteen sectors), 0 near the
 * centre. */
u8 Pad_StickDirBits(PadStick *s)
{
    if (s->mag < 0x25) {
        return 0;
    }
    return gPadStickDirBits[(u32)(s->deg / 22.5f)];
}

/* Starts libdbc and libpad2; 1 when both are up. */
s32 Pad_StartLibs(void)
{
    s32 r;

    if (sceDbcInit() != 1) {
        return 0;
    }
    r = scePad2Init(0);
    return r == 1;
}

/* Opens the socket of a port and slot. */
void Pad_OpenSocket(s32 port, s32 slot, PadPort *p)
{
    scePad2SocketParam prm;

    prm.number = 0;
    prm.port = port;
    prm.slot = slot;
    prm.flag = 2;
    if ((p->socket = scePad2CreateSocket(&prm, p->buf)) >= 0) {
        p->state = 1;
        p->setup = 0;
        p->port = port;
        p->slot = slot;
    }
}

/* Closes the socket, then libpad2 and libdbc. */
void Pad_EndLibs(void)
{
    PadPort *p = (PadPort *)gPadPort;

    if (p->state != 0) {
        scePad2DeleteSocket(p->socket);
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
    u8 prof[4];
    u8 vib[4];
    s32 i;

    gPadDataCur[port] = gPadData[port];
    switch (scePad2GetState(gPadPort[port].socket)) {
    case 1:
        gPadPort[port].state = 2;
        break;
    case 0:
        gPadPort[port].state = 1;
        gPadPort[port].setup = 0;
        gPadPort[port].unk4 = 0;
        gPadPort[port].vibOk = 0;
        gPadPort[port].profile = 0;
        gPadPort[port].vibProfile = 0;
        gPadDataCur[port] = gPadDataNone;
        gPadRaw[port].kind = 0;
        gPadRaw[port].caps = 0;
        gPadRaw[port].state = gPadPort[port].state;
        return;
    case 2:
    case 3:
    default:
        gPadPort[port].state = 2;
        gPadPort[port].setup = 0;
        gPadPort[port].unk4 = 0;
        gPadPort[port].vibOk = 0;
        gPadPort[port].profile = 0;
        gPadPort[port].vibProfile = 0;
        gPadDataCur[port] = gPadDataNone;
        gPadRaw[port].kind = 0;
        gPadRaw[port].caps = 0;
        gPadRaw[port].state = gPadPort[port].state;
        return;
    }
    if (gPadPort[port].setup == 0) {
        if (scePad2GetButtonProfile(gPadPort[port].socket, prof) < 0) {
            return;
        }
        gPadPort[port].profile = (prof[3] << 24) | (prof[2] << 16) | (prof[1] << 8) | prof[0];
        if (sceVibGetProfile(gPadPort[port].socket, vib) >= 0) {
            gPadPort[port].vibProfile = vib[0];
        }
        if (gPadPort[port].profile & 0xFFFF0000) {
            if (gPadPort[port].profile & 0xFFF00000) {
                gPadPort[port].type = 2;
            } else {
                gPadPort[port].type = 1;
            }
        } else {
            gPadPort[port].type = 0;
        }
        switch (gPadPort[port].type) {
        case 0:
            gPadRaw[port].kind = 1;
            gPadRaw[port].caps = 0;
            for (i = 0; i < 16; i++) {
                gPadButtonMaps[port][i].mask = gPadMapDigital[i].mask;
                gPadButtonMaps[port][i].kind = gPadMapDigital[i].kind;
            }
            break;
        case 1:
            gPadRaw[port].kind = 1;
            gPadRaw[port].caps = PAD_CAP_STICK_L | PAD_CAP_STICK_R;
            for (i = 0; i < 16; i++) {
                gPadButtonMaps[port][i].mask = gPadMapAnalog[i].mask;
                gPadButtonMaps[port][i].kind = gPadMapAnalog[i].kind;
            }
            break;
        case 2:
            gPadRaw[port].kind = 1;
            gPadRaw[port].caps = PAD_CAP_STICK_L | PAD_CAP_STICK_R | PAD_CAP_PRESS_PAD |
                                    PAD_CAP_PRESS_SHAPE | PAD_CAP_PRESS_DIR;
            for (i = 0; i < 16; i++) {
                gPadButtonMaps[port][i].mask = gPadMapAnalog[i].mask;
                gPadButtonMaps[port][i].kind = gPadMapAnalog[i].kind;
            }
            break;
        }
        if ((gPadPort[port].vibProfile & 3) == 0) {
            gPadPort[port].vibOk = 0;
        } else {
            gPadPort[port].vibOk = 1;
        }
        gPadPort[port].setup++;
    } else {
        if (scePad2Read(gPadPort[port].socket, data) < 0) {
            return;
        }
        gPadPort[port].state = 1;
        gPadDataCur[port].status = 0;
        gPadDataCur[port].mode = 0;
        gPadDataCur[port].buttons = (data[0] << 8) | data[1];
        switch (gPadPort[port].type) {
        case 0:
            break;
        case 1:
            gPadDataCur[port].rx = data[2];
            gPadDataCur[port].ry = data[3];
            gPadDataCur[port].lx = data[4];
            gPadDataCur[port].ly = data[5];
            break;
        case 2:
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

/* Sets the vibration of a port: on with level 0..7 (the large motor and
 * the small one), or off. 0 when the pad has no actuators. */
s32 Pad_SetVibration(s32 port, s32 level, s32 on)
{
    u8 prof;
    u8 data[3];
    u8 v;
    s32 n;
    if (gPadPort[port].vibProfile == 0) {
        return 0;
    }
    gPadPort[port].vibTime = on;
    if (on == 0) {
        if ((gPadPort[port].vibProfile = 3) != 0) {
            prof = 3;
            n = 2;
            data[0] = 0;
            data[1] = 0;
        } else {
            data[0] = 0;
            n = 1;
            prof = 2;
        }
    } else {
        v = ((level & 7) << 5) | 0xF;
        if ((gPadPort[port].vibProfile = 3) != 0) {
            prof = 3;
            n = 2;
            data[0] = ((v & 0x7F) << 1) | 1;
            data[1] = (v & 0x80) >> 7;
        } else {
            data[0] = v;
            n = 1;
            prof = 2;
        }
    }
    sceVibSetActParam(gPadPort[port].socket, 1, &prof, n, data);
    return 1;
}


/*
 * TIM2 pictures (0x18C2E0-0x18CC10): the header checks and the addresses of
 * picture 0's parts that Tex_CreateFromTim2 (unk_180110.c) needs. The file
 * layout is TIM2's: a 16-byte file header, then per picture a header, its
 * mip-map header, the image levels and the palette (CLUT), each part
 * aligned to 16 bytes (format 0) or 128.
 */
typedef struct Tim2FileHeader {
    u8 id[4];                   /* 0x00: 'TIM2' */
    u8 version;                 /* 0x04: 3 or 4 */
    u8 format;                  /* 0x05: 0 = 16-byte alignment, 1 = 128 */
    u16 pictures;               /* 0x06 */
    u8 pad[8];                  /* 0x08 */
} Tim2FileHeader;

typedef struct Tim2PictureHeader {
    u32 totalSize;              /* 0x00 */
    u32 clutSize;               /* 0x04 */
    u32 imageSize;              /* 0x08 */
    u16 headerSize;             /* 0x0C */
    u16 clutColors;             /* 0x0E */
    u8 pictFormat;              /* 0x10 */
    u8 mipMaps;                 /* 0x11: image levels, 1 to 7 */
    u8 clutType;                /* 0x12: 1 16-bit, 2 24-bit, 3 32-bit colours */
    u8 imageType;               /* 0x13: the same, or 4 (4-bit), 5 (8-bit indices) */
    u16 width;                  /* 0x14 */
    u16 height;                 /* 0x16 */
    u64 gsTex0;                 /* 0x18 */
    u64 gsTex1;                 /* 0x20 */
    u32 gsTexaFbaPabe;          /* 0x28 */
    u32 gsTexClut;              /* 0x2C */
} Tim2PictureHeader;

/* After the picture header when there is more than one level. */
typedef struct Tim2MipHeader {
    u64 gsMiptbp1;              /* 0x00 */
    u64 gsMiptbp2;              /* 0x08 */
    u32 levelSize[8];           /* 0x10 */
} Tim2MipHeader;

s32 Tim2_CheckHeader(u8 *tim2);
static Tim2PictureHeader *Tim2_GetPicture(u8 *tim2, s32 n);
u8 *Tim2_GetImage(u8 *tim2, s32 pict, s32 level);
u8 *Tim2_GetPictClut(u8 *tim2, s32 pict);

/* The number of mip levels after the first of picture 0; 0 when the file
 * is not TIM2. */
s32 Tim2_LastLevel(u8 *tim2)
{
    Tim2PictureHeader *ph;

    if (Tim2_CheckHeader(tim2) == 0) {
        return 0;
    }
    ph = Tim2_GetPicture(tim2, 0);
    return (u8)(ph->mipMaps - 1);
}

/* Describes the image levels of picture 0 (up to 7). */
s32 Tim2_DescribeLevels(Image *img, u8 *tim2)
{
    Tim2PictureHeader *ph;
    s32 i;
    s32 w;
    s32 h;

    if (Tim2_CheckHeader(tim2) == 0) {
        return 0;
    }
    ph = Tim2_GetPicture(tim2, 0);
    if (ph->mipMaps > 7) {
        return 0;
    }
    w = ph->width;
    h = ph->height;
    for (i = 0; i < ph->mipMaps; i++) {
        img->flags = 0;
        img->width = w;
        img->height = h;
        switch (ph->imageType) {
        case 4:
            img->flags |= IMAGE_INDEXED | IMAGE_NIBBLE_HI;
            img->bpp = 0;
            img->pitch = img->width >> 1;
            img->r.shift = 0;
            img->r.bits = 0;
            img->r.mask = 0;
            img->g.shift = 0;
            img->g.bits = 0;
            img->g.mask = 0;
            img->b.shift = 0;
            img->b.bits = 0;
            img->b.mask = 0;
            img->a.shift = 0;
            img->a.bits = 0;
            img->a.mask = 0;
            break;
        case 5:
            img->flags |= IMAGE_INDEXED;
            img->bpp = 1;
            img->pitch = img->bpp * img->width;
            img->r.shift = 0;
            img->r.bits = 0;
            img->r.mask = 0;
            img->g.shift = 0;
            img->g.bits = 0;
            img->g.mask = 0;
            img->b.shift = 0;
            img->b.bits = 0;
            img->b.mask = 0;
            img->a.shift = 0;
            img->a.bits = 0;
            img->a.mask = 0;
            break;
        case 1:
            img->bpp = 2;
            img->pitch = img->bpp * img->width;
            IMAGE_RGBA5551(img);
            img->r.shift = 0; /* the GS order, blue high */
            img->b.shift = 10;
            break;
        case 2:
            img->bpp = 3;
            img->pitch = img->bpp * img->width;
            IMAGE_RGB888(img);
            IMAGE_GS_ORDER_32(img);
            break;
        case 3:
            img->bpp = 4;
            img->pitch = img->bpp * img->width;
            IMAGE_RGBA8888(img);
            IMAGE_GS_ORDER_32(img);
            break;
        }
        img++;
        w >>= 1;
        h >>= 1;
    }
    return 1;
}

/* Describes picture 0's palette (16 or 256 colours). */
s32 Tim2_DescribeClut(Image *img, u8 *tim2)
{
    Tim2PictureHeader *ph;

    if (Tim2_CheckHeader(tim2) == 0) {
        return 0;
    }
    ph = Tim2_GetPicture(tim2, 0);
    if (ph->mipMaps > 7) {
        return 0;
    }
    img->flags = 0;
    switch (ph->clutType) {
    case 1:
        img->bpp = 2;
        IMAGE_RGBA5551(img);
        img->r.shift = 0; /* the GS order, blue high */
        img->b.shift = 10;
        break;
    case 2:
        img->bpp = 3;
        IMAGE_RGB888(img);
        IMAGE_GS_ORDER_32(img);
        break;
    case 3:
        img->bpp = 4;
        IMAGE_RGBA8888(img);
        IMAGE_GS_ORDER_32(img);
        break;
    default:
        return 0;
    }
    switch (ph->imageType) {
    case 4:
        if (ph->clutColors != 16) {
            return 0;
        }
        img->width = 16;
        img->height = 1;
        break;
    case 5:
        if (ph->clutColors != 256) {
            return 0;
        }
        img->width = 256;
        img->height = 1;
        break;
    default:
        return 0;
    }
    img->pitch = img->bpp * img->width;
    return 1;
}

/* Level `level` of picture 0's image. */
u8 *Tim2_GetLevel(u8 *tim2, s32 level)
{
    return Tim2_GetImage(tim2, 0, level);
}

/* Picture 0's palette. */
u8 *Tim2_GetClut(u8 *tim2)
{
    return Tim2_GetPictClut(tim2, 0);
}

/* 1 for a TIM2 file of version 3, or 4 with format 0 or 1, holding one
 * picture; else 0. */
s32 Tim2_CheckHeader(u8 *tim2)
{
    Tim2FileHeader *h = (Tim2FileHeader *)tim2;
    s32 kind;
    u8 fmt;

    if (h->id[0] == 'T' && h->id[1] == 'I' && h->id[2] == 'M' && h->id[3] == '2') {
        kind = 1;
    } else if (h->id[0] == 'C' && h->id[1] == 'L' && h->id[2] == 'T' && h->id[3] == '2') {
        /* a palette file (CLT2) */
        return 0;
    } else {
        return 0;
    }
    fmt = h->format;
    if (!(h->version == 3 || (h->version == 4 && fmt <= 1U))) {
        return 0;
    }
    if (h->pictures != 1) {
        return 0;
    }
    return kind;
}

/* size rounded up to the file's alignment. */
static u32 Tim2_Align(u8 *tim2, u32 size)
{
    u32 align = ((Tim2FileHeader *)tim2)->format == 0 ? 0x10 : 0x80;

    if (size % align != 0) {
        size += align - (size - size / align * align) % align;
    }
    return size;
}

/* Picture n's header. */
static Tim2PictureHeader *Tim2_GetPicture(u8 *tim2, s32 n)
{
    s32 i;
    Tim2FileHeader *h;
    Tim2PictureHeader *ph;

    h = (Tim2FileHeader *)tim2;
    tim2 += Tim2_Align(tim2, sizeof(Tim2FileHeader));
    ph = (Tim2PictureHeader *)tim2;
    for (i = 0; i < n; i++) {
        if (i >= h->pictures) {
            break;
        }
        tim2 += Tim2_Align((u8 *)h, sizeof(Tim2PictureHeader));
        tim2 += ph->totalSize;
        ph = (Tim2PictureHeader *)tim2;
    }
    return ph;
}

/* Level `level` of picture 0's image (pict is not used). ph keeps the
 * picture's start (its mipMaps byte, 0x11, is read through it); p is a byte
 * cursor: the mip header, then the image. The level-size index is unsigned,
 * as the original's code shows: with a signed index MWCC strength-reduces
 * the pointer, the original computes i * 4 + p in each group of eight. */
u8 *Tim2_GetImage(u8 *tim2, s32 pict, s32 level)
{
    u32 *sizes[8];
    u8 *p;
    u8 *ph;
    u32 hsize;
    u32 add;
    s32 i;
    u32 skip;
    u8 *img;

    p = (u8 *)Tim2_GetPicture(tim2, 0);
    ph = p;
    hsize = sizeof(Tim2PictureHeader);
    if (ph[0x11] > 1) {
        p = ph + sizeof(Tim2PictureHeader);
        for (i = 0; i < ph[0x11] - 1; i++) {
            sizes[i] = &((Tim2MipHeader *)p)->levelSize[(u32)i];
        }
        if (ph[0x11] <= 4) {
            add = 0x20;
        } else {
            add = 0x30;
        }
        hsize += add;
    }
    p = ph + Tim2_Align(tim2, hsize);
    if (ph[0x11] > 1) {
        skip = 0;
        for (i = 0; i < level; i++) {
            skip += Tim2_Align(tim2, *sizes[i]);
        }
        img = p + skip;
    } else {
        img = p;
    }
    return img;
}

/* Picture 0's palette, or 0 when it has none. */
/* the CLUT of picture 0 of a TIM2 file: after the picture header (plus the
 * mipmap headers) and the image, or NULL when it has none */
u8 *Tim2_GetPictClut(u8 *tim2, s32 pict)
{
    u8 *p;
    Tim2PictureHeader *ph;
    u32 hsize;
    u32 add;
    u8 *r;

    p = (u8 *)Tim2_GetPicture(tim2, 0);
    ph = (Tim2PictureHeader *)p;
    hsize = sizeof(Tim2PictureHeader);
    if (ph->mipMaps > 1) {
        if (ph->mipMaps <= 4) {
            add = 0x20;
        } else {
            add = 0x30;
        }
        hsize += add;
    }
    p += Tim2_Align(tim2, hsize);
    if (ph->clutSize != 0) {
        r = p + ph->imageSize;
    } else {
        return NULL;
    }
    return r;
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
