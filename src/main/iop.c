/*
 * IOP reboot and module loading. The IOP is rebooted with
 * DNAS300.IMG, then the modules packed in MH2DRIVE.DAT are read to EE
 * 0x1000000, sent to an IOP heap block and started from there (the list
 * gIopModArgs gives the module and its arguments); DEV9.IRX is loaded if
 * no dev9 module is resident, then USBKB.IRX.
 */
#include "common.h"
#include "ram_areas.h"
#include "texture.h"
#include "iop.h"
#include "power_off.h"
#include "libcdvd.h"

/* MH2DRIVE.DAT: a count and the offset and size of each module (the
 * offsets are made IOP addresses once the pack is on the IOP). */
typedef struct IopModPack {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s32 count;
    /* 0x8 */ struct IopModEntry {
        u32 addr;
        u32 size;
    } mod[1];
} IopModPack;

typedef struct IopModArg {
    s32 id;         /* module of the pack; -1 ends the list */
    s32 argsize;
    char *args;
} IopModArg;

typedef struct SifDmaTransfer {
    void *src;
    void *dest;
    s32 size;
    s32 attr;
} SifDmaTransfer;

extern IopModArg gIopModArgs[];
/* the entries of the pack at 0x1000000 */
extern struct IopModEntry D_1000008[];
extern char D_001C7D80[];   /* "load module error, '%s'.\n" */
extern char D_001C7DA0[];   /* "result = %d.\n" */
extern char D_001C7DB0[];   /* "cdrom0:\\IOP\\MODULES\\DNAS300.IMG;1" */
extern char D_001C7DE0[];   /* "cdrom0:\\IOP\\MODULES\\MH2DRIVE.DAT;1" */
extern char D_001C7E08[];   /* "dev9" */
extern char D_001C7E10[];   /* "cdrom0:\\IOP\\MODULES\\DEV9.IRX;1" */
extern char D_001C7E30[];   /* "cdrom0:\\IOP\\MODULES\\USBKB.IRX;1" */
extern char D_001C7E50[];   /* "keybd=1" */

s32 scePrintf(const char *fmt, ...);
void FlushCache(s32 op);
s32 sceSifInitRpc(s32 mode);
s32 sceSifInitIopHeap(void);
s32 sceFsReset(void);
s32 sceSifRebootIop(const char *img);
s32 sceSifSyncIop(void);
s32 sceSifLoadFileReset(void);
s32 sceOpen(const char *name, s32 mode);
s32 sceLseek(s32 fd, s32 offset, s32 whence);
s32 sceRead(s32 fd, void *buf, s32 size);
s32 sceClose(s32 fd);
void *sceSifAllocIopHeap(s32 size);
s32 sceSifFreeIopHeap(void *addr);
u32 SifSetDma(SifDmaTransfer *t, s32 n);
s32 SifDmaStat(u32 id);
s32 sceSifLoadStartModule(const char *name, s32 argsize, const char *args, s32 *result);
s32 sceSifLoadStartModuleBuffer(void *addr, s32 argsize, const char *args, s32 *result);
s32 sceSifSearchModuleByName(const char *name);
void func_00162878(s32, s32);

/* Starts module id of the pack (already on the IOP); returns its result. */
s32 Iop_StartPackedModule(IopModPack *pack, s32 id, s32 argsize, char *args)
{
    void *addr = (void *)pack->mod[id].addr;
    s32 result;

    do {
    } while (sceSifLoadStartModuleBuffer(addr, argsize, args, &result) < 0);
    return result;
}

/* Starts an IOP module file (hangs if it cannot be loaded); -1 when the
 * module did not stay resident. */
s32 Iop_StartModule(char *path, s32 argsize, char *args)
{
    s32 result;

    while (sceSifLoadStartModule(path, argsize, args, &result) < 0) {
        scePrintf(D_001C7D80, path);
        for (;;) {
        }
    }
    scePrintf(D_001C7DA0, result);
    return result == 1 ? -1 : 0;
}

void Iop_Init(void)
{
    u32 *p;
    s32 i;
    s32 n;

    s32 fd;
    u32 id;
    s32 size;
    IopModArg *m;
    s32 modId;
    void *iop;
    IopModPack *pack;
    SifDmaTransfer dma;

    sceSifInitRpc(0);
    sceSifInitIopHeap();
    sceCdInit(0);
    sceFsReset();
    while (!sceSifRebootIop(D_001C7DB0)) {
    }
    while (!sceSifSyncIop()) {
    }
    sceSifInitRpc(0);
    sceSifLoadFileReset();
    sceFsReset();
    sceSifInitIopHeap();
    sceCdInit(0);
    sceCdMmode(2);
    while ((fd = sceOpen(D_001C7DE0, 1)) < 0) {
    }
    size = sceLseek(fd, 0, 2);
    sceLseek(fd, 0, 0);
    sceRead(fd, (void *)IOP_MODPACK_BUF, size);
    sceClose(fd);
    while ((iop = sceSifAllocIopHeap(size)) == NULL) {
    }
    dma.src = (void *)IOP_MODPACK_BUF;
    dma.dest = iop;
    dma.size = size;
    dma.attr = 0;
    FlushCache(0);
    id = SifSetDma(&dma, 1);
    while (SifDmaStat(id) >= 0) {
    }
    pack = (IopModPack *)IOP_MODPACK_BUF;
    n = pack->count;
    p = (u32 *)D_1000008;
    for (i = 0; i < n; i++, p += 2) {
        *p += (u32)iop;
    }
    m = gIopModArgs;
    modId = m->id;
    while (modId != -1) {
        switch (modId) {
        case 7:
            break;
        case 10:
            func_00162878(2, 2);
            break;
        case 4:
            break;
        case 3:
            goto next;
        }
        Iop_StartPackedModule(pack, m->id, m->argsize, m->args);
    next:
        m++;
        modId = m->id;
    }
    sceSifFreeIopHeap(iop);
    if (sceSifSearchModuleByName(D_001C7E08) >= 0 || Iop_StartModule(D_001C7E10, 0, NULL) >= 0) {
        PowerOff_Init();
    }
    Iop_LoadModule(D_001C7E30, 16, D_001C7E50, 0);
}

/* This file's initialised data. */
extern char gIopModArgs_9[];
extern char gIopModArgs_10[];
IopModArg gIopModArgs[12] = {
#include "main/001BAAA0.inc"
};

/* This file's read-only data. */
#pragma rodata on
char gIopModArgs_9[32] = {
#include "main/001C7D30.inc"
};
char gIopModArgs_10[48] = {
#include "main/001C7D50.inc"
};
char D_001C7D80[32] = {
#include "main/001C7D80.inc"
};
char D_001C7DA0[16] = {
#include "main/001C7DA0.inc"
};
char D_001C7DB0[48] = {
#include "main/001C7DB0.inc"
};
char D_001C7DE0[40] = {
#include "main/001C7DE0.inc"
};
char D_001C7E08[8] = {
#include "main/001C7E08.inc"
};
char D_001C7E10[32] = {
#include "main/001C7E10.inc"
};
char D_001C7E30[32] = {
#include "main/001C7E30.inc"
};
char D_001C7E50[16] = {
#include "main/001C7E50.inc"
};
#pragma rodata off
