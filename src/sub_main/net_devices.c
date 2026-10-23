/*
 * The network devices: the list of devices the IOP's USB device
 * searcher reports (gNetDevs, read through its RPC server by
 * NetDev_Refresh), which of them this game supports (gNetDevNames, and
 * the network adaptor, gNetDefaultDev), and the IRX drivers each kind
 * needs (Net_LoadDrivers, Net_StopDevice; their paths are this file's
 * data). Net_Start starts it all: the settings' reset, the AVE TCP
 * module, the adaptor when its driver is loaded, the device searcher.
 * Networking code stays byte-identical.
 */

#include "common.h"
#include "sif.h"
#include "yn.h"                /* NetDevice, gNetDevs */
#include "net_tcp.h"
#include "net_devices.h"
#include "net_settings.h"
#include "libkernl.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

s32 strcmp(s8 *a, s8 *b);
void * memset(void *p, s32 c, u32 n);
extern NetDevice gNetDevNames[];
extern s8 gNetDriverType;
extern s32 gNetDevCount;
extern s32 gNetDevSeenCount;
extern NetDevice *gNetDevCur;   /* the device in use (in gNetDevs) */

/* The vendor and product names of the known network devices (gNetDevNames, gNetDefaultDev). */
#pragma rodata on
char gNetUsbLanVendor[12] = {
#include "sub_main/0046A508.inc"
};
char gNetUsbLanProduct1[12] = {
#include "sub_main/0046A518.inc"
};
char gNetUsbLanProduct2[12] = {
#include "sub_main/0046A528.inc"
};
char gNetAdaptorVendor[8] = {
#include "sub_main/0046A538.inc"
};
char gNetAdaptorProduct[32] = {
#include "sub_main/0046A540.inc"
};
#pragma rodata off

extern s16 gUsbKbReady;

/*
 * The device in use, for the lobby: its vendor and product ids, whether the USB keyboard
 * is ready, and for modem-type devices (types 2 and 3) the link speed / 100 (otherwise 0).
 */
void Net_GetDeviceState(s16 *outVendor, s16 *outProduct, s16 *outKbReady, s16 *outSpeed)
{
    NetConnInfo info;

    *outVendor = gNetDevCur->vendorId;
    *outProduct = gNetDevCur->productId;
    *outKbReady = gUsbKbReady & 1;
    *outSpeed = 0;
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
    case NET_DEV_PPP_OTHER:
        NetPpp_GetStatus(&info);
        *outSpeed = info.speed / 100;
        break;
    }
}

/* Stops and unloads IRX module id, retrying each step until it succeeds (as Yn_UnloadModule). */
void Irx_Unload(s32 modId)
{
    s32 result;
    s32 ret;

    while ((ret = sceSifStopModule(modId, 0, 0, &result)) < 0) {
        __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
    }
    if (0 <= ret) {
        while (sceSifUnloadModule(modId) < 0) {
            __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
        }
    }
}

/*
 * Loads the IRX module at path with its argument block (argc bytes at argv). If startResult
 * is given, the module is also started and its start result goes there. Retries forever when
 * tries is 0, otherwise at most tries times.
 */
void Net_LoadIrx(s32 path, s32 argc, char *argv, s32 startResult, s32 tries)
{
    s32 i;

    if (startResult == 0) {
        if (tries == 0) {
            while (sceSifLoadModule((char *)path, argc, argv) < 0) {
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadModule((char *)path, argc, argv)) {
                    i = 0;
                    break;
                }
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        }
    } else {
        if (tries == 0) {
            while (sceSifLoadStartModule((char *)path, argc, argv, (void *)startResult) < 0) {
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadStartModule((char *)path, argc, argv, (void *)startResult)) {
                    i = 0;
                    break;
                }
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        }
    }
}

extern s32 gNetDevRpcBuf;
extern SifClientData gNetDevRpc;

/*
 * Calls function fno of the device-search RPC server, with gNetDevRpcBuf (64 bytes) as
 * both send and receive buffer, after writing back the data cache. Returns the call's result,
 * or -1.
 */
s32 NetDev_CallRpc(s32 fno)
{
    s32 r;

    FlushCache(0);
    r = sceSifCallRpc(&gNetDevRpc, fno, 0, (s32 *) &gNetDevRpcBuf, 64, (s32 *) &gNetDevRpcBuf, 64, 0, 0);
    if (r < 0) {
        return -1;
    }
    return r;
}

/*
 * Fills dev for the IOP's device (vendor, product): copies type, driver and names from the
 * matching gNetDevNames row, plus the ids and status. Returns 0, or -1 for an unknown device,
 * with dev set to 255 in every byte (unused).
 */
s32 NetDev_Identify(NetDevice *dev, s32 vendor, s32 product, s32 rpcWord2, s32 devStatus)
{
    NetDevice *row;
    s32 rowVendor;

    row = gNetDevNames;
loop_1:
    rowVendor = row->vendorId;
    if (rowVendor != 0) {
        if ((rowVendor == vendor) && (row->productId == product)) {
            dev->type = row->type;
            dev->driver = row->driver;
            dev->vendorId = vendor;
            dev->productId = product;
            dev->status = devStatus;
            dev->vendor = row->vendor;
            dev->product = row->product;
            return 0;
        }
        row++;
        goto loop_1;
    }
    memset(dev, 255, sizeof(NetDevice));
    return -1;
}

/*
 * When no device is in use, clears the device list and sets both counts to -1, so the next
 * NetDev_Refresh reads it again. Only the first 32 bytes of gNetDevs are filled with 255.
 */
void NetDev_ClearList(void)
{
    if (gNetDevCur == 0) {
        memset(&gNetDevs, 255, 32U);
        gNetDevSeenCount = -1;
        gNetDevCount = -1;
    }
}

extern NetDevice gNetDefaultDev[];

/*
 * Whether vendor/product are the network adaptor's names (gNetDevAdaptor[0]). 0 for NULL
 * names.
 */
s32 NetDev_MatchesDefault(s8 *vendor, s8 *product)
{
    if (vendor == 0 || product == 0) {
        return 0;
    }
    return strcmp(vendor, (s8 *)gNetDefaultDev[0].vendor) == 0 && strcmp(product, (s8 *)gNetDefaultDev[0].product) == 0;
}

/*
 * Whether vendor/product name a device this game supports: a gNetDevNames row or the
 * network adaptor. 0 for NULL names.
 */
s32 NetDev_IsKnownName(s8 *vendor, s8 *product)
{
    NetDevice *known;

    if ((vendor == 0) || (product == 0)) {
        return 0;
    }
    known = gNetDevNames;
loop_4:
    if (known->vendorId != 0) {
        if ((strcmp(vendor, (s8 *)known->vendor) == 0) && (strcmp(product, (s8 *)known->product) == 0)) {
            return 1;
        }
        known++;
        goto loop_4;
    }
    return NetDev_MatchesDefault(vendor, product) != 0;
}

extern s32 gNetStopReq;
extern s32 gNetIrxUsbsrch;

#pragma rodata on
char D_0046A560[23] = {
#include "sub_main/0046A560.inc"
};
char D_0046A580[19] = {
#include "sub_main/0046A580.inc"
};
char D_0046A5A0[17] = {
#include "sub_main/0046A5A0.inc"
};
#pragma rodata off

/*
 * Releases the Ethernet device in use (sets gNetStopReq). For the LANEGG or SMAP driver
 * kinds it unloads the AVE TCP wrapper module, stops the TCP layer, unloads the driver
 * module, reloads the USB device searcher, binds its RPC server again, forgets the device and
 * re-reads the device list. Returns 0, or -1 for modem-type devices and the AN986 driver kind.
 */
s32 Net_StopDevice(void)
{
    s32 modId;

    gNetStopReq = 1;
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
        return -1;
    case NET_DEV_ETHERNET:
        switch (gNetDevCur->driver) {
        case NET_DRV_AN986:
            return -1;
        }
        if (0 <= (modId = sceSifSearchModuleByName(D_0046A560))) {
            Irx_Unload(modId);
        }
        modId = -1;
        NetTcp_Term();
        switch (gNetDevCur->driver) {
        case NET_DRV_LANEGG:
            modId = sceSifSearchModuleByName(D_0046A580);
            break;
        case NET_DRV_SMAP:
            modId = sceSifSearchModuleByName(D_0046A5A0);
            break;
        }
        if (0 <= modId) {
            Irx_Unload(modId);
        }
        Net_LoadIrx(gNetIrxUsbsrch, 0, 0, 0, 0);
        SifRpc_BindWait(&gNetDevRpc, 0x1270010);
        gNetDevCur = 0;
        NetDev_ClearList();
        NetDev_Refresh();
        break;
    case NET_DEV_PPP_OTHER:
        return -1;
    }
    return 0;
}

extern s32 gNetIrxAn986;
extern s32 gNetIrxLanegg;
extern s32 gNetIrxSmap;
extern s32 gNetIrxAvewrape;

#pragma rodata on
char D_0046A5C0[27] = {
#include "sub_main/0046A5C0.inc"
};
char D_0046A5E0[8] = {
#include "sub_main/0046A5E0.inc"
};
char D_0046A5E8[9] = {
#include "sub_main/0046A5E8.inc"
};
#pragma rodata off

/*
 * Unloads the USB device searcher if it is loaded. For an Ethernet device, loads its driver
 * (AN986, LANEGG or SMAP, with a thread-priority argument) and the Ethernet AVE wrapper, then
 * Net_Init. Always returns 1.
 */
s32 Net_LoadDriverType1(void)
{
    s32 modId;

    if (0 <= (modId = sceSifSearchModuleByName(D_0046A5C0))) {
        Irx_Unload(modId);
    }
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
        break;
    case NET_DEV_ETHERNET:
        switch (gNetDevCur->driver) {
        case NET_DRV_AN986:
            Net_LoadIrx(gNetIrxAn986, 8, D_0046A5E0, 0, 0);
            break;
        case NET_DRV_LANEGG:
            Net_LoadIrx(gNetIrxLanegg, 8, D_0046A5E0, 0, 0);
            break;
        case NET_DRV_SMAP:
            Net_LoadIrx(gNetIrxSmap, 9, D_0046A5E8, 0, 0);
            break;
        }
        Net_LoadIrx(gNetIrxAvewrape, 0, 0, 0, 0);
        Net_Init();
        break;
    case NET_DEV_PPP_OTHER:
        break;
    }
    return 1;
}

extern s32 gNetIrxCxtmdm;
extern s32 gNetIrxOstmdmA;
extern s32 gNetIrxAveppp;
extern s32 gNetIrxAvewrapm;
extern s32 gNetIrxAvedhcp;
extern s32 gNetIrxDevglue;

#pragma rodata on
#pragma explicit_zero_data on
char D_0046A5F8[1] = {
#include "sub_main/0046A5F8.inc"
};
#pragma explicit_zero_data reset
#pragma rodata off

/*
 * Loads the support modules for the device in use, unless gNetStopReq is set. First unloads
 * the USB device searcher. A modem gets its modem driver (kind 2 or 4), AVE PPP and the modem
 * AVE wrapper, then Net_Init (gNetDriverType = 2). An Ethernet device gets the device glue,
 * AVE DHCP and AVE PPP modules (gNetDriverType = 1). Always returns 1.
 */
s32 Net_LoadDrivers(void)
{
    s32 modId;

    if (gNetStopReq != 0) {
        return 1;
    }
    if (0 <= (modId = sceSifSearchModuleByName(D_0046A5C0))) {
        Irx_Unload(modId);
    }
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
        switch (gNetDevCur->driver) {
        case NET_DRV_CXTMDM:
            Net_LoadIrx(gNetIrxCxtmdm, 0, 0, 0, 0);
            break;
        case NET_DRV_OSTMDM:
            Net_LoadIrx(gNetIrxOstmdmA, 0, 0, 0, 0);
            break;
        }
        Net_LoadIrx(gNetIrxAveppp, 1, D_0046A5F8, 0, 0);
        Net_LoadIrx(gNetIrxAvewrapm, 0, 0, 0, 0);
        Net_Init();
        gNetDriverType = 2;
        break;
    case NET_DEV_ETHERNET:
        Net_LoadIrx(gNetIrxDevglue, 1, D_0046A5F8, 0, 0);
        Net_LoadIrx(gNetIrxAvedhcp, 0, 0, 0, 0);
        Net_LoadIrx(gNetIrxAveppp, 1, D_0046A5F8, 0, 0);
        gNetDriverType = 1;
        break;
    case NET_DEV_PPP_OTHER:
        break;
    }
    return 1;
}

extern s32 D_0059CBC4;
extern s32 D_0059CBC8;
extern s32 D_0059CBCC;

/*
 * Re-reads the device list from the IOP when no device is in use and the IOP's count has
 * changed (RPC 0). For each of the 32 slots it asks RPC 1 for the device, then fills
 * gNetDevs[i] (NetDev_Identify) or marks it unused. gNetDevCount also counts the extra slots
 * 32-34 that are in use. Returns 1 if the list was re-read, else 0.
 */
s32 NetDev_Refresh(void)
{
    u32 count;
    s32 i;
    NetDevice *dev;
    s32 *pStatus;
    s32 *pRpcWord2;
    s32 *pProductId;

    if (gNetDevCur != 0) {
        return 0;
    }
    NetDev_CallRpc(0);
    count = gNetDevRpcBuf;
    if (count != gNetDevSeenCount) {
        i = 0;
        dev = gNetDevs;
        pStatus = &D_0059CBCC;
        pRpcWord2 = &D_0059CBC8;
        pProductId = &D_0059CBC4;
        for (; i < 32; i++, dev++) {
            gNetDevRpcBuf = i;
            NetDev_CallRpc(1);
            if (gNetDevRpcBuf == -1) {
                memset(dev, 255, sizeof(NetDevice));
            } else {
                NetDev_Identify(dev, gNetDevRpcBuf, *pProductId, *pRpcWord2, *pStatus);
            }
        }
        gNetDevSeenCount = count;
        gNetDevCount = count;
        for (i = 32; i < 35; i++) {
            if (gNetDevs[i].type != -1) {
                gNetDevCount++;
            }
        }
        return 1;
    }
    return 0;
}

/* Binds an RPC client to server id, retrying until the server answers. */
void SifRpc_BindWait(SifClientData *client, u32 serverId)
{
    volatile s32 i;

    for (;;) {
        if (sceSifBindRpc(client, serverId, 0) >= 0 && client->serve != NULL) {
            break;
        }
        for (i = 0x10000; i != 0; i--) {
        }
    }
}

/* Starts the TCP layer (NetTcp_Init). Returns 0. */
s32 Net_Init(void)
{
    NetTcp_Init();
    return 0;
}

extern s32 D_0059CC30;
extern s32 gNetIrxAvetcp;

#pragma rodata on
char D_0046A600[5] = {
#include "sub_main/0046A600.inc"
};
char gNetIrxCxtmdm_0[40] = {
#include "sub_main/0046A610.inc"
};
char gNetIrxOstmdmA_0[42] = {
#include "sub_main/0046A640.inc"
};
char gNetIrxAvetcp_0[40] = {
#include "sub_main/0046A670.inc"
};
char gNetIrxAveppp_0[40] = {
#include "sub_main/0046A6A0.inc"
};
char gNetIrxAvewrapm_0[42] = {
#include "sub_main/0046A6D0.inc"
};
char gNetIrxUsbsrch_0[41] = {
#include "sub_main/0046A700.inc"
};
char gNetIrxAvewrape_0[44] = {
#include "sub_main/0046A730.inc"
};
char gNetIrxAvedhcp_0[43] = {
#include "sub_main/0046A760.inc"
};
char gNetIrxDevglue_0[43] = {
#include "sub_main/0046A790.inc"
};
char gNetIrxAn986_0[41] = {
#include "sub_main/0046A7C0.inc"
};
char gNetIrxLanegg_0[42] = {
#include "sub_main/0046A7F0.inc"
};
char gNetIrxSmap_0[40] = {
#include "sub_main/0046A820.inc"
};
#pragma rodata off

/*
 * Starts the network. Resets the settings and device list, loads the AVE TCP module, and adds
 * the network adaptor as gNetDevs[34] (status 33) when the expansion-bay driver is already
 * loaded. Then loads the USB device searcher, binds its RPC server and reads the device list.
 * Returns 0.
 */
s32 Net_Start(void)
{
    NetCfg_Reset();
    NetDev_ClearList();
    gNetStopReq = 0;
    D_0059CC30 = 0;
    Net_LoadIrx(gNetIrxAvetcp, 1, D_0046A5F8, 0, 0);
    if (0 <= sceSifSearchModuleByName(D_0046A600)) {
        gNetDevs[34].type = gNetDefaultDev[0].type;
        gNetDevs[34].driver = gNetDefaultDev[0].driver;
        gNetDevs[34].vendorId = gNetDefaultDev[0].vendorId;
        gNetDevs[34].productId = gNetDefaultDev[0].productId;
        gNetDevs[34].status = 33;
        gNetDevs[34].vendor = gNetDefaultDev[0].vendor;
        gNetDevs[34].product = gNetDefaultDev[0].product;
    }
    Net_LoadIrx(gNetIrxUsbsrch, 0, 0, 0, 0);
    SifRpc_BindWait(&gNetDevRpc, 0x1270010);
    NetDev_Refresh();
    return 0;
}

/* This file's initialised data. */
extern char gNetUsbLanVendor[];
extern char gNetUsbLanProduct1[];
extern char gNetUsbLanProduct2[];
extern char gNetAdaptorVendor[];
extern char gNetAdaptorProduct[];
NetDevice gNetDevNames[3] = {
#include "sub_main/004053C0.inc"
};
NetDevice gNetDefaultDev[2] = {
#include "sub_main/00405420.inc"
};
s32 gNetIrxCxtmdm = (s32)gNetIrxCxtmdm_0;
s32 gNetIrxOstmdmA = (s32)gNetIrxOstmdmA_0;
s32 gNetIrxAvetcp = (s32)gNetIrxAvetcp_0;
s32 gNetIrxAveppp = (s32)gNetIrxAveppp_0;
s32 gNetIrxAvewrapm = (s32)gNetIrxAvewrapm_0;
s32 gNetIrxUsbsrch = (s32)gNetIrxUsbsrch_0;
s32 gNetIrxAvewrape = (s32)gNetIrxAvewrape_0;
s32 gNetIrxAvedhcp = (s32)gNetIrxAvedhcp_0;
s32 gNetIrxDevglue = (s32)gNetIrxDevglue_0;
s32 gNetIrxAn986 = (s32)gNetIrxAn986_0;
s32 gNetIrxLanegg = (s32)gNetIrxLanegg_0;
s32 gNetIrxSmap = (s32)gNetIrxSmap_0;
