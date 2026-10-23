/* The image decoders ("pl" in their messages: plPNGSetContextFromImage,
 * plBMPSetContextFromImage), one of the game's units: the PNG decoder's
 * heap, reading the file from memory, unfiltering the rows (the PNG
 * filters None, Sub, Up, Average, Paeth), setting up the rows and
 * counting them, a PNG's rows through zlib, its header and chunks,
 * texture sizes, the palettes, a BMP's pixels (zlib's adler32 follows,
 * in adler32.c). The state is gPng (include/html.h). Png_Read is
 * static: Png_ReadRow and plPNGSetContextFromImage keep a0 (&gPng) across
 * its calls. */
#include "common.h"
#include "html.h"
#include "texture.h"
#include "heap.h"
#include "arena.h"

void Arena_Free(Arena *h, void *p);               /* sub_main: free to a heap */
s32 memcmp(const void *a, const void *b, u32 n); /* main: memcmp */
s32 Debug_Sprintf(const char *fmt, ...);             /* main: debug printf */
u8 *Bmp_GetPixels(u8 *bmp);                           /* sub_main: a BMP's pixels */
void *Mem_Alloc(u32 size, s32 flag);
void *Mem_GetAddr(void *handle);
void Mem_Free(void *handle);
void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
extern Arena gImgHeap;
void Img_InitHeap(void);
void Png_ReadRow(u8 *dst);
s32 Img_TexWidth(s32 w, s32 h);
s32 plPNGSetContextFromImage(Image *img, u8 *data);
static s32 Img_TexSize(s32 n); /* static: Bmp_CreateTexture keeps t0 across the call */
u32 Png_CreatePalette(s32 mode);
void Png_ReadImage(Image *img, u8 *dst);

/* The chunk names and the signature, compared with memcmp. */
const char gPngIHDR[] = "IHDR";
const char gPngIDAT[] = "IDAT";
const char gPngIEND[] = "IEND";
const char gPngtRNS[] = "tRNS";
const u8 gPngSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
const char gPngPLTE[] = "PLTE";

extern u8 gClutOrder[];   /* the order of a CLUT's colours in GS memory, per 32 */

/* zlib's zcfree (the game's own version): back to the image heap. */
void zcfree(void *opaque, void *ptr)
{
    Arena_Free(&gImgHeap, ptr);
}

/* n bytes from the image heap, cleared. */
void *Png_Alloc(u32 n)
{
    return zcalloc(NULL, 1, n);
}

/* Frees everything on the image heap (sets it up again). */
void Png_FreeAll(void) {
    Img_InitHeap();
}

/* Reads n bytes of the file (fewer at its end) into dst; err -3 when
 * nothing is left. Returns the bytes read. */
s32 Png_ReadInto(PngState *s, u8 *dst, s32 n) {
    s32 left = s->left;

    if (left == 0) {
        s->err = -3;
        return 0;
    }
    if (left < n) {
        n = left;
    }
    memcpy(dst, s->data + (s->size - left), n);
    s->left -= n;
    return n;
}

/* Reads n bytes of the file (fewer at its end): s->ptr points at them;
 * err -3 when nothing is left. Returns the bytes read. */
static s32 Png_Read(PngState *s, s32 n) {
    s32 left = s->left;

    if (left == 0) {
        s->err = -3;
        return 0;
    }
    if (left < n) {
        n = left;
    }
    s->ptr = s->data + (s->size - left);
    s->left -= n;
    return n;
}

/* Undoes a row's filter (the row's first byte): 1 Sub, 2 Up, 3 Average,
 * 4 Paeth; prev is the row above (zeros for the first row). */
void Png_FilterRow(PngRowInfo *info, u8 *row, u8 *prev, s32 filter) {
    switch (filter) {
    case 0:
        break;
    case 1: {
        u32 i;
        u32 end = info->rowBytes;
        u32 bpp = (info->pixelBits + 7) >> 3;
        u8 *rp = row + bpp;
        u8 *lp = row;

        for (i = bpp; i < end; i++) {
            *rp = *rp + *lp++;
            rp++;
        }
        break;
    }
    case 2: {
        u32 i;
        u32 end = info->rowBytes;
        u8 *rp = row;
        u8 *pp = prev;

        for (i = 0; i < end; i++) {
            *rp = *rp + *pp++;
            rp++;
        }
        break;
    }
    case 3: {
        u32 i;
        u8 *rp = row;
        u8 *pp = prev;
        u8 *lp = row;
        u32 bpp = (info->pixelBits + 7) >> 3;
        u32 end = info->rowBytes - bpp;

        for (i = 0; i < bpp; i++) {
            u32 v = (u32)*pp++ >> 1;
            *rp = *rp + v;
            rp++;
        }
        for (i = 0; i < end; i++) {
            *rp = *rp + (*pp++ + *lp++) / 2;
            rp++;
        }
        break;
    }
    case 4: {
        u32 i;
        u8 *rp = row;
        u8 *pp = prev;
        u8 *lp = row;
        u8 *cp = prev;
        u32 bpp = (info->pixelBits + 7) >> 3;
        u32 end = info->rowBytes - bpp;

        for (i = 0; i < bpp; i++) {
            *rp = *rp + *pp++;
            rp++;
        }
        for (i = 0; i < end; i++) {
            s32 a, b, c, pa, pb, pc, p;

            b = *pp++;
            c = *cp++;
            a = *lp++;
            p = b - c;
            pc = a - c;
            pa = p < 0 ? -p : p;
            pb = pc < 0 ? -pc : pc;
            pc = (p + pc) < 0 ? -(p + pc) : p + pc;
            p = (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
            *rp = *rp + p;
            rp++;
        }
        break;
    }
    default:
        *row = 0;
        break;
    }
}

/* Sets up the rows: the row buffer (32 bytes of room before it) and the
 * previous row (zeros). */
void Png_StartRows(void) {
    u32 n;
    u8 depth;

    gPng.zs.avail_in = 0;
    gPng.rows = gPng.height;
    gPng.rowWidth = gPng.width;
    gPng.rowSize = gPng.rowBytes + 1;
    depth = gPng.pixelBits;
    n = (gPng.width + 7) & ~7;
    n = ((n * depth + 7) >> 3) + 1 + ((depth + 7) >> 3);
    gPng.rowAlloc = Png_Alloc(n + 0x40);
    gPng.row = gPng.rowAlloc + 0x20;
    gPng.prevRow = Png_Alloc(gPng.rowBytes + 1);
    memset(gPng.prevRow, 0, gPng.rowBytes + 1);
    gPng.started = 1;
}

/* The next row: after the last one, zlib is reset. */
void Png_FinishRow(void) {
    gPng.rowNo++;
    if (gPng.rowNo >= gPng.rows) {
        inflateReset(&gPng.zs);
    }
}

/* A big-endian u32 of a PNG. */
#define PNG_U32(p) (((p)[0] << 24) + ((p)[1] << 16) + ((p)[2] << 8) + (p)[3])

/* Reads the next row of the image into dst (png_read_row's work): IDAT
 * data through zlib, then the row's filter undone. */
void Png_ReadRow(u8 *dst) {
    s32 ret;

    if (!gPng.started) {
        Png_StartRows();
    }
    gPng.zs.next_out = gPng.row;
    gPng.zs.avail_out = gPng.rowSize;
    do {
        if (gPng.zs.avail_in == 0) {
            while (!gPng.idatLeft) {
                Png_Read(&gPng, 4);
                Png_Read(&gPng, 4);
                gPng.idatLeft = PNG_U32(gPng.ptr);
                Png_Read(&gPng, 4);
                if (memcmp(gPng.ptr, gPngIDAT, 4) != 0) {
                    if (memcmp(gPng.ptr, gPngIEND, 4) == 0) {
                        goto done;
                    }
                    Png_Read(&gPng, gPng.idatLeft);
                    gPng.idatLeft = 0;
                }
                if (gPng.err) {
                    goto done;
                }
            }
            gPng.zs.avail_in = gPng.zbufSize;
            gPng.zs.next_in = gPng.zbuf;
            if (gPng.idatLeft < gPng.zbufSize) {
                gPng.zs.avail_in = gPng.idatLeft;
            }
            Png_ReadInto(&gPng, gPng.zbuf, gPng.zs.avail_in);
            if (gPng.err) {
                goto done;
            }
            gPng.idatLeft -= gPng.zs.avail_in;
        }
        ret = inflate(&gPng.zs, Z_PARTIAL_FLUSH);
        if (ret == Z_STREAM_END) {
            break;
        }
        if (ret < 0) {
            gPng.err = ret;
            return;
        }
    } while (gPng.zs.avail_out);
done:
    gPng.rowInfo.colorType = gPng.colorType;
    gPng.rowInfo.width = gPng.rowWidth;
    gPng.rowInfo.channels = gPng.channels;
    gPng.rowInfo.bitDepth = gPng.bitDepth;
    gPng.rowInfo.pixelBits = gPng.pixelBits;
    gPng.rowInfo.rowBytes = (gPng.rowInfo.width * gPng.rowInfo.pixelBits + 7) >> 3;
    if (((char *)gPng.row)[0]) {
        Png_FilterRow(&gPng.rowInfo, gPng.row + 1, gPng.prevRow + 1, ((char *)gPng.row)[0]);
    }
    memcpy(gPng.prevRow, gPng.row, gPng.rowBytes + 1);
    memcpy(dst, gPng.row + 1, (gPng.width * gPng.rowInfo.pixelBits + 7) >> 3);
    Png_FinishRow();
}

/* A texture width for an image w wide (see Img_TexSize); 8-bit images
 * not 32 or 64 high are at least 128 wide. */
s32 Img_TexWidth(s32 w, s32 h) {
    s32 r;

    if (gPng.pixelBits != 8 || h == 32 || h == 64) {
        return Img_TexSize(w);
    }
    switch (w) {
    default:
        if (w < 128) {
            r = 128;
        } else if (w < 256) {
            r = 256;
        } else {
            r = 512;
        }
        return r;
    case 512:
    case 256:
    case 128:
        return w;
    }
}

/* plPNGSetContextFromImage: checks the signature, sets up zlib, reads the
 * chunks up to IDAT (IHDR, PLTE, tRNS) and describes the image in img
 * (sizes rounded up for a texture: 8-bit palette or 32-bit RGBA). */
s32 plPNGSetContextFromImage(Image *img, u8 *data) {
    u32 len;
    s32 ct;

    if (memcmp(data, gPngSignature, 8) != 0) {
        Debug_Sprintf("This is not PNG image. @plPNGSetContextFromImage");
        return 0;
    }
    gPng.zbufSize = 0x2100;
    gPng.zbuf = Png_Alloc(gPng.zbufSize);
    gPng.zs.opaque = &gPng;
    switch (inflateInit_(&gPng.zs, "1.1.4", sizeof(z_stream))) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
    case Z_STREAM_ERROR:
    case Z_VERSION_ERROR:
    default:
        return 0;
    }
    gPng.data = data;
    gPng.zs.next_out = gPng.zbuf;
    gPng.zs.avail_out = gPng.zbufSize;
    Png_Read(&gPng, 8);
    do {
        Png_Read(&gPng, 4);
        len = PNG_U32(gPng.ptr);
        Png_Read(&gPng, 4);
        if (memcmp(gPng.ptr, gPngIHDR, 4) == 0) {
            Png_Read(&gPng, 13);
            gPng.width = PNG_U32(gPng.ptr);
            gPng.height = PNG_U32(gPng.ptr + 4);
            gPng.bitDepth = gPng.ptr[8];
            gPng.colorType = gPng.ptr[9];
            gPng.compression = gPng.ptr[10];
            gPng.filterMethod = gPng.ptr[11];
            gPng.interlace = gPng.ptr[12];
            Png_Read(&gPng, 4);
            switch (gPng.colorType) {
            case 0:
            case 3:
                gPng.channels = 1;
                break;
            case 2:
                gPng.channels = 3;
                break;
            case 4:
                gPng.channels = 2;
                break;
            case 6:
                gPng.channels = 4;
                break;
            }
            gPng.pixelBits = gPng.bitDepth * gPng.channels;
            gPng.rowBytes = (gPng.width * gPng.pixelBits + 7) >> 3;
        } else if (memcmp(gPng.ptr, gPngtRNS, 4) == 0) {
            if (gPng.colorType == 2) {
                Png_Read(&gPng, len);
                gPng.hasTrns = 1;
                gPng.trnsR = (gPng.ptr[0] << 8) + gPng.ptr[1];
                gPng.trnsG = (gPng.ptr[2] << 8) + gPng.ptr[3];
                gPng.trnsB = (gPng.ptr[4] << 8) + gPng.ptr[5];
            } else if (gPng.colorType == 3) {
                gPng.trns = gPng.ptr + 4;
                gPng.hasTrns = 1;
                Png_Read(&gPng, len);
                gPng.trnsCount = len;
            }
            Png_Read(&gPng, 4);
        } else if (memcmp(gPng.ptr, gPngIEND, 4) == 0) {
            break;
        } else if (memcmp(gPng.ptr, gPngIDAT, 4) == 0) {
            gPng.idatLeft = len;
            break;
        } else if (memcmp(gPng.ptr, gPngPLTE, 4) == 0) {
            gPng.palette = gPng.ptr + 4;
            Png_Read(&gPng, len);
            gPng.paletteCount = len / 3;
            Png_Read(&gPng, 4);
        } else {
            Png_Read(&gPng, len);
            Png_Read(&gPng, 4);
        }
    } while (!gPng.err);
    if (gPng.err != 0) {
        return 0;
    }
    if (gPng.bitDepth != 8 || gPng.width > 0x100 || gPng.height > 0x100 ||
        ((ct = gPng.colorType & 7) != 2 && gPng.pixelBits != 24 && ct != 3 && gPng.pixelBits != 8)) {
        Debug_Sprintf("format not support %d %d. @plPNGSetContextFromImage", gPng.colorType, gPng.bitDepth);
        return 0;
    }
    img->width = gPng.width;
    img->height = gPng.height;
    img->height = Img_TexSize(img->height);
    img->width = Img_TexWidth(img->width, img->height);
    img->flags = 0;
    if (gPng.pixelBits != 8) {
        img->bpp = 4;
        img->pitch = img->width * 4;
        IMAGE_RGBA8888(img);
        IMAGE_GS_ORDER_32(img);
    } else {
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
    }
    return 1;
}

/* A texture size for n: n itself for 32 .. 512, else the next of them. */
static s32 Img_TexSize(s32 n) {
    s32 r;

    switch (n) {
    default:
        if (n < 32) {
            r = 32;
        } else if (n < 64) {
            r = 64;
        } else if (n < 128) {
            r = 128;
        } else if (n < 256) {
            r = 256;
        } else {
            r = 512;
        }
        return r;
    case 512:
    case 256:
    case 128:
    case 64:
    case 32:
        return n;
    }
}

/* The palette of an 8-bit PNG as a palette handle (256 colours in the
 * GS's CLUT order; transparent where tRNS says so). */
u32 Png_CreatePalette(s32 mode) {
    Image img;
    u32 handle;
    TexRec *t;
    u8 *pal;
    u8 *src;
    u8 *p;
    u16 i;
    u16 k;

    handle = Pal_FindFree();
    if (handle == 0) {
        return handle;
    }
    t = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
    img.flags = 0;
    img.bpp = 4;
    IMAGE_RGBA8888(&img);
    IMAGE_GS_ORDER_32(&img);
    img.width = 0x100;
    img.height = 1;
    img.pitch = img.bpp * img.width;
    Pal_SetupRec(&img, handle, mode);
    t->mem = Mem_Alloc(t->size, 2);
    pal = Mem_GetAddr(t->mem);
    src = gPng.palette;
    for (i = 0; i < gPng.paletteCount; i++) {
        k = (i & 0xE0) + gClutOrder[i & 0x1F];
        k <<= 2;
        p = pal + k;
        p[0] = *src++;
        p[1] = *src++;
        p[2] = *src++;
        if (gPng.hasTrns) {
            if (i < gPng.trnsCount && gPng.trns[i] != 0xFF) {
                p[3] = 0;
            } else {
                p[3] = 0x80;
            }
        } else {
            p[3] = 0x80;
        }
    }
    Pal_Upload(handle, mode);
    return handle;
}

/* Reads the image's rows into dst, a texture of img's size: 8-bit
 * indices, or RGB with alpha 0x80 (0 for tRNS's colour); the rest of the
 * texture is cleared. */
void Png_ReadImage(Image *img, u8 *dst) {
    u8 *row;
    u8 *src;
    s32 y;
    s32 x;
    u8 r;
    u8 g;
    u8 b;

    row = Png_Alloc(gPng.rowBytes + 1);
    switch (gPng.pixelBits) {
    case 24:
        for (y = 0; y < img->height; y++) {
            if (gPng.height <= y) {
                for (x = 0; x < img->width; x++) {
                    *dst++ = 0;
                    *dst++ = 0;
                    *dst++ = 0;
                    *dst++ = 0x80;
                }
            } else {
                Png_ReadRow(row);
                if (gPng.err) {
                    return;
                }
                src = row;
                for (x = 0; x < img->width; x++) {
                    if (x >= gPng.width) {
                        for (; x < img->width; x++) {
                            *dst++ = 0;
                            *dst++ = 0;
                            *dst++ = 0;
                            *dst++ = 0x80;
                        }
                        break;
                    }
                    r = *src++;
                    g = *src++;
                    b = *src++;
                    *dst++ = r;
                    *dst++ = g;
                    *dst++ = b;
                    if (gPng.hasTrns) {
                        if (gPng.trnsR == r && gPng.trnsG == g && gPng.trnsB == b) {
                            *dst++ = 0;
                        } else {
                            *dst++ = 0x80;
                        }
                    } else {
                        *dst++ = 0x80;
                    }
                }
            }
        }
        break;
    case 8:
        for (y = 0; y < img->height; y++) {
            if (gPng.height <= y) {
                for (x = 0; x < img->width; x++) {
                    *dst++ = 0;
                }
            } else {
                Png_ReadRow(row);
                if (gPng.err) {
                    return;
                }
                src = row;
                for (x = 0; x < img->width; x++) {
                    if (x >= gPng.width) {
                        for (; x < img->width; x++) {
                            *dst++ = 0;
                        }
                        break;
                    }
                    *dst++ = *src++;
                }
            }
        }
        break;
    }
}

/* Makes a texture handle of a PNG file (data, size bytes) for the texture
 * mode; *w, *h get the image's size. Returns the handle (with the
 * palette's for 8-bit images) or -1 no free texture, -2 a bad file, -3
 * no room. */
s32 Png_CreateTexture(u8 *data, s32 size, s32 mode, u16 *w, u16 *h) {
    Image img;
    u32 handle;
    TexRec *t;

    Img_InitHeap();
    memset(&gPng, 0, sizeof(PngState));
    gPng.size = size;
    gPng.left = size;
    gPng.ptr = NULL;
    handle = Tex_FindFree();
    if (handle == 0) {
        return -1;
    }
    t = &gTexRecs[(handle & 0xFFFF) - 1];
    if (plPNGSetContextFromImage(&img, data) == 0) {
        Png_FreeAll();
        return -2;
    }
    if (Tex_SetupRec(&img, 1, handle, mode) == 0) {
        Png_FreeAll();
        return -3;
    }
    t->mem = Mem_Alloc(t->size, 2);
    Png_ReadImage(&img, Mem_GetAddr(t->mem));
    if (gPng.err) {
        inflateEnd(&gPng.zs);
        Png_FreeAll();
        Mem_Free(t->mem);
        return -2;
    }
    Tex_Upload(handle, mode);
    if (gPng.pixelBits == 8) {
        handle |= Png_CreatePalette(mode);
    }
    inflateEnd(&gPng.zs);
    Png_FreeAll();
    *w = gPng.width;
    *h = gPng.height;
    return handle;
}

/* A BMP file's headers (BITMAPFILEHEADER, BITMAPINFOHEADER), read as
 * u16s: the u32 fields are not 4-aligned. */
typedef struct BmpHeader {
    u16 type;       /* 0x00: "BM" */
    u8 pad02[0x10];
    u16 widthLo;    /* 0x12 */
    u16 widthHi;    /* 0x14 */
    u16 heightLo;   /* 0x16 */
    u16 heightHi;   /* 0x18 */
    u8 pad1A[2];
    u16 bitCount;   /* 0x1C: 8 or 24 */
} BmpHeader;

/* plBMPSetContextFromImage: describes a BMP (8-bit palette or 24-bit
 * RGB) in img. */
s32 plBMPSetContextFromImage(Image *img, BmpHeader *bmp) {
    u16 bits;

    if (bmp->type != 0x4D42) {
        Debug_Sprintf("This is not BMP image. @plBMPSetContextFromImage");
        return 0;
    }
    img->width = bmp->widthLo | (bmp->widthHi << 16);
    img->height = bmp->heightLo | (bmp->heightHi << 16);
    bits = bmp->bitCount;
    if (bits != 24 && bits != 8) {
        Debug_Sprintf("Not support except 8bit or 24bit. @plCreateContextFromBMPImage");
        return 0;
    }
    img->flags = 0;
    if (bits == 24) {
        img->bpp = 3;
        img->pitch = img->width * 3;
        IMAGE_RGB888(img);
    } else {
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
    }
    return 1;
}

/* A BMP's pixels (w x h, rows bottom up, each padded to 4 bytes) into dst,
 * a texture of img's size: 8-bit indices, or RGB from BGR; the rest of the
 * texture is cleared. */
void Bmp_ReadImage(Image *img, u8 *dst, u8 *bits, s32 depth, u16 w, u16 h, BmpHeader *bmp) {
    s32 y;
    s32 stride;
    s32 x;
    u8 *src;
    u8 r;
    u8 g;
    u8 b;

    switch (depth) {
    case 24:
        stride = w % 4 + w * 3;
        for (y = 0; y < img->height; y++) {
            if (h <= y) {
                for (x = 0; x < img->width; x++) {
                    *dst++ = 0;
                    *dst++ = 0;
                    *dst++ = 0;
                }
            } else {
                for (x = 0; x < img->width; x++) {
                    if (w <= x) {
                        *dst++ = 0;
                        *dst++ = 0;
                        *dst++ = 0;
                    } else {
                        src = bits + x * 3 + stride * (h - 1 - y);
                        b = *src++;
                        g = *src++;
                        r = *src++;
                        *dst++ = r;
                        *dst++ = g;
                        *dst++ = b;
                    }
                }
            }
        }
        break;
    case 8:
        stride = w % 4;
        if (stride != 0) {
            stride = 4 - stride;
        }
        stride += w;
        for (y = 0; y < img->height; y++) {
            if (h <= y) {
                for (x = 0; x < img->width; x++) {
                    *dst++ = 0;
                }
            } else {
                for (x = 0; x < img->width; x++) {
                    if (x >= w) {
                        for (; x < img->width; x++) {
                            *dst++ = 0;
                        }
                        break;
                    }
                    src = bits + x + stride * (h - 1 - y);
                    *dst++ = *src;
                }
            }
        }
        break;
    }
}

/* The palette of an 8-bit BMP as a palette handle (256 colours in the
 * GS's CLUT order, from BGRA). */
u32 Bmp_CreatePalette(s32 mode, u8 *bmp) {
    Image img;
    u32 handle;
    TexRec *t;
    u8 *pal;
    u8 *src;
    s32 i;
    u8 *p;

    handle = Pal_FindFree();
    if (handle == 0) {
        return handle;
    }
    t = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
    img.flags = 0;
    img.bpp = 4;
    IMAGE_RGBA8888(&img);
    IMAGE_GS_ORDER_32(&img);
    img.width = 0x100;
    img.height = 1;
    img.pitch = img.bpp * img.width;
    Pal_SetupRec(&img, handle, mode);
    t->mem = Mem_Alloc(t->size, 2);
    pal = Mem_GetAddr(t->mem);
    src = bmp + 0x36;
    for (i = 0; i < 0x100; i++) {
        p = pal + (u16)((u16)((i & 0xE0) + gClutOrder[i & 0x1F]) * 4);
        p[2] = *src++;
        p[1] = *src++;
        p[0] = *src++;
        p[3] = 0x80;
        src++;
    }
    Pal_Upload(handle, mode);
    return handle;
}

/* Makes a texture handle of a BMP file for the texture mode; *w, *h get
 * the image's size. Returns the handle (with the palette's for 8-bit
 * images) or -1 no free texture, -2 a bad file, -3 no room. */
s32 Bmp_CreateTexture(u8 *bmp, s32 mode, u16 *w, u16 *h) {
    u16 iw;
    u16 ih;
    Image img;
    u32 handle;
    TexRec *t;
    u8 *pixels;
    u8 *bits;
    s32 depth;

    handle = Tex_FindFree();
    if (handle == 0) {
        return -1;
    }
    t = &gTexRecs[(handle & 0xFFFF) - 1];
    if (plBMPSetContextFromImage(&img, (BmpHeader *)bmp) == 0) {
        return -2;
    }
    iw = img.width;
    ih = img.height;
    img.height = Img_TexSize(img.height);
    img.width = Img_TexWidth(img.width, img.height);
    if (Tex_SetupRec(&img, 1, handle, mode) == 0) {
        return -3;
    }
    t->mem = Mem_Alloc(t->size, 2);
    pixels = Mem_GetAddr(t->mem);
    bits = Bmp_GetPixels(bmp);
    depth = ((BmpHeader *)bmp)->bitCount;
    Bmp_ReadImage(&img, pixels, bits, depth, iw, ih, (BmpHeader *)bmp);
    Tex_Upload(handle, mode);
    if (depth == 8) {
        handle |= Bmp_CreatePalette(mode, bmp);
    }
    *w = iw;
    *h = ih;
    return handle;
}

/* image_decode.c's .data (0x7F06E0-0x7F0700) */
u8 gClutOrder[32] = {
#include "lobby/007F06E0.inc"
};
