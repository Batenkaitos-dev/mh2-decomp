/*
 * CSE SPU memory map: where each of the 16 banks' sample data goes in SPU
 * RAM. The map file ("SPUMAPDT", SpuMap.bin) holds pages, each a list of 16
 * bank sizes and the two cores' effect (reverb) area end addresses; selecting
 * a page lays the banks out one after another from 0x5020.
 */
#include "cse.h"
#include "main/cse_spumap.h"   /* the texts this file uses (tools/content_gen.py) */

typedef struct SpuMapPage {
    u32 size[16];       /* 0x00: bytes of SPU RAM per bank */
    u32 effectEnd[2];   /* 0x40: effect area end per core (low 17 bits set) */
} SpuMapPage;

typedef struct SpuMap {
    char magic[8];      /* 0x00: "SPUMAPDT" */
    u32 numPages;       /* 0x08 */
    u32 unkC;
    SpuMapPage pages[1]; /* 0x10 */
} SpuMap;

typedef struct SpuBanks {
    u32 addr[16];       /* SPU address of each bank */
    u32 size[16];
} SpuBanks;

extern volatile u32 gCseSpuMapPage;
extern SpuBanks gCseSpuBanks;
extern u32 gCseEffectEnd[2];
extern SpuMap *gCseSpuMap;

void scePrintf(char *fmt, ...);
s32 strncmp(char *a, char *b, u32 n);

s32 CseSpuMap_Init(void *map)
{
    u32 i;

    if (strncmp(map, "SPUMAPDT", 8) != 0) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8060) /* debug message: not SPU map data */);
        return 0x80000002;
    }
    for (i = 0; i < 16; i++) {
        gCseSpuBanks.addr[i] = 0x5020;
        gCseSpuBanks.size[i] = 0;
    }
    gCseSpuMap = map;
    gCseSpuMapPage = 0;
    gCseEffectEnd[0] = 0x1FFFFF;
    gCseEffectEnd[1] = 0x1FFFFF;
    return CseSpuMap_SetPage(0);
}

s32 CseSpuMap_SetPage(u32 page)
{
    SpuMap *map = gCseSpuMap;
    SpuMapPage *pg;
    s32 i;
    u32 addr;

    if (map->numPages <= page) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8080) /* debug message: no such page number */);
        return 0x80000003;
    }
    gCseSpuMapPage = page;
    addr = 0x5020;
    pg = &map->pages[gCseSpuMapPage];
    for (i = 0; i < 16; i++) {
        gCseSpuBanks.addr[i] = addr;
        gCseSpuBanks.size[i] = pg->size[i];
        if (0x200000 < gCseSpuBanks.size[i] + gCseSpuBanks.addr[i]) {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf(TXT(0x001C80A0) /* debug message: past the end of the SPU map */);
            return 0x80000004;
        }
        addr += pg->size[i];
    }
    if ((pg->effectEnd[0] & 0x1FFFF) != 0x1FFFF) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C80C0) /* debug message: core 0's effect end address is wrong */);
        return 0x80000005;
    }
    gCseEffectEnd[0] = pg->effectEnd[0];
    if ((pg->effectEnd[1] & 0x1FFFF) != 0x1FFFF) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C80F0) /* debug message: core 1's effect end address is wrong */);
        return 0x80000005;
    }
    gCseEffectEnd[1] = pg->effectEnd[1];
    return 0;
}

u32 CseSpuMap_GetBankAddr(u32 bank)
{
    if (bank >= 16) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8120) /* debug message: no such bank number */);
        return 0x80000001;
    }
    return gCseSpuBanks.addr[bank];
}

u32 CseSpuMap_GetEffectEnd(s32 core)
{
    return gCseEffectEnd[core];
}
