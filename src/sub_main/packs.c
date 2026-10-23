/*
 * Packed files: LzDecompress unpacks compressed data (16-bit units,
 * literals and back-references), Pack_* read a pack (a count,
 * then each part's offset and size), and AreaPack_ReadPart2 reads the
 * area pack's environment block by block: the fog (AreaPack_ReadPart0),
 * the lights (AreaPack_ReadPart1) and the sky band (AreaPack_ReadBlock3).
 * It uses no data of its own.
 */

#include "common.h"
#include "area.h"
#include "packs.h"

/*
 * Unpacks src into dst (16-bit units); returns the bytes written. A flag word
 * (read when its 16 bits run out, high bit first) marks each unit: 0 copies a
 * literal, 1 a back-reference {count: 5 bits, distance: 11 bits} (count 0: the
 * count follows; distance 0: that many zeros, or the end when the count is 0).
 */
s32 LzDecompress(u8 *src, u8 *dst)
{
    s32 flags;
    s32 bit;
    u8 *start;
    u32 dist;
    u32 n;
    u8 *from;

    start = dst;
    bit = 0;
    flags = 0;

    for (;;) {
        if (bit == 0) {
            flags = *(s16 *)src;
            bit = 0x8000;
            src += 2;
        }
        if (flags & bit) {
            dist = *(u16 *)src;
            n = dist >> 11;
            src += 2;
            if (n != 0) {
                dist &= 0x7FF;
            } else {
                n = *(u16 *)src;
                src += 2;
            }
            if (dist == 0) {
                if (n == 0) {
                    break;
                }
                do {
                    *(u16 *)dst = 0;
                    n--;
                    dst += 2;
                } while (n != 0);
            } else {
                from = dst - dist * 2;
                do {
                    n--;
                    *(u16 *)dst = *(u16 *)from;
                    from += 2;
                    dst += 2;
                } while (n != 0);
            }
        } else {
            *(u16 *)dst = *(u16 *)src;
            src += 2;
            dst += 2;
        }
        bit >>= 1;
    }
    return dst - start;
}

/*
 * A pack file (areas, models, textures): an s32 count of parts, then per
 * part its offset from the pack's start and its size, s32 each.
 */

/* The pack's part count. */
s32 Pack_Count(s32 *pack)
{
    return *pack;
}

/* The address of the pack's part i. */
void *Pack_GetPart(u8 *pack, s32 i)
{
    return pack + *(s32 *)(pack + i * 8 + 4);
}

/* The size of the pack's part i, in bytes. */
s32 Pack_PartSize(u8 *pack, s32 i)
{
    return *(s32 *)(pack + i * 8 + 8);
}

void *memcpy(void *dst, const void *src, unsigned long n);

/* Copies parts 0 and 1 of the pack to firstDst and secondDst. */
void Pack_CopyParts01(u8 *pack, u8 *firstDst, u8 *secondDst)
{
    void *part;

    part = Pack_GetPart(pack, 0);
    memcpy(firstDst, part, Pack_PartSize(pack, 0));
    part = Pack_GetPart(pack, 1);
    memcpy(secondDst, part, Pack_PartSize(pack, 1));
}

/* Reads part 0 of the area's environment (AreaPack_ReadPart2): the fog's colour, then its two
 * distances. */
u8 * AreaPack_ReadPart0(u8 *src)
{
    memcpy((void *) &gAreaState.fog.r, (const void *) src, 4U);
    memcpy((void *) &gAreaState.fog, (const void *) (src + 4), 8U);
    return src + 12;
}

/* Reads part 1: the area's three lights (directions, colours, ambient parts). */
u8 * AreaPack_ReadPart1(u8 *src)
{
    memcpy((void *) gAreaState.lightDir, (const void *) src, 36U);
    memcpy((void *) gAreaState.lightColor, (const void *) (src + 36), 36U);
    memcpy((void *) gAreaState.lightAmbient, (const void *) (src + 72), 36U);
    return src + 108;
}

/* Reads block 3 of the area pack: the sky's far point and its band (the rectangle, then the
 * eight colours stored in a mixed byte order). Returns the end of the block. */
u8 *AreaPack_ReadBlock3(u8 *src)
{
    u8 bytes[4];

    memcpy(gAreaState.skyPos, src, 12);
    memcpy(&gAreaState.band, src + 12, 4);
    memcpy(bytes, src + 16, 4);
    gAreaState.band.c[0] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    memcpy(bytes, src + 20, 4);
    gAreaState.band.c[1] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    memcpy(bytes, src + 24, 4);
    gAreaState.band.c[2] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    memcpy(bytes, src + 28, 4);
    gAreaState.band.c[3] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    memcpy(bytes, src + 32, 4);
    gAreaState.band.c[4] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    memcpy(bytes, src + 36, 4);
    gAreaState.band.c[5] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    memcpy(bytes, src + 40, 4);
    gAreaState.band.c[6] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    memcpy(bytes, src + 44, 4);
    gAreaState.band.c[7] = (bytes[3] << 24) | (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    return src + 48;
}

/* Reads part 2 of the area pack: its first halfword (gAreaState.envBlocks) says which blocks follow. */
void AreaPack_ReadPart2(s32 *pack)
{
    AreaState *area = &gAreaState;
    u8 *part;

    if (Pack_Count(pack) < 3) {
        area->envBlocks = 0;
        return;
    }
    part = Pack_GetPart((u8 *)pack, 2);
    area->envBlocks = *(s16 *)part;
    switch (area->envBlocks) {
    case 1:
        AreaPack_ReadPart1(AreaPack_ReadPart0(part + 2));
        break;
    case 2:
        AreaPack_ReadBlock3(AreaPack_ReadPart1(AreaPack_ReadPart0(part + 2)));
        break;
    default:
        area->envBlocks = 0;
        break;
    }
}
