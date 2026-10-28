#ifndef NET_DEVICES_H
#define NET_DEVICES_H

#include "common.h"
#include "sif.h"

/*
 * The network devices: the list the IOP's USB device searcher reports
 * (NetDev_Refresh), the IRX drivers each kind needs (Net_LoadDrivers,
 * Net_StopDevice), and Net_Start, which starts the network: the settings'
 * reset, the AVE TCP module, the network adaptor and the device searcher.
 */

void Net_GetDeviceState(s16 *outVendor, s16 *outProduct, s16 *outKbReady, s16 *outSpeed);
void Net_LoadIrx(char *path, s32 argc, char *argv, s32 startResult, s32 tries);
void NetDev_ClearList(void);
s32 NetDev_MatchesDefault(s8 *vendor, s8 *product);
s32 Net_StopDevice(void);
s32 Net_LoadDriverType1(void);
s32 Net_LoadDrivers(void);
s32 NetDev_Refresh(void);
void SifRpc_BindWait(SifClientData *client, u32 serverId);
s32 Net_Init(void);
s32 Net_Start(void);

#endif
