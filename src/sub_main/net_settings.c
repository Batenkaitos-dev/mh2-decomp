/*
 * The network settings' reset (NetCfg_Reset): no device in use, every
 * address, PPP, DNS and authentication setting cleared, and the option
 * list the network library reads rebuilt from its default keys and
 * values. With it, NetTcp_SetOption, which sets the two AVE TCP options
 * the lobby's socket uses.
 */

#include "common.h"
#include "yn.h"                /* NetDevice, gNetDevs */
#include "net_tcp.h"
#include "net_settings.h"
#include "libscf.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

void strncpy(char *dst, const char *src, u32 n);
void * memset(void *p, s32 c, u32 n);
extern s8 *gNetOptList;
extern s8 *D_0059D684;
extern s8 *D_0059D688;
extern s8 *D_0059D68C;
extern u8 *D_0059D690;
extern u8 *D_0059D694;
extern s32 D_0059D698;
extern s32 D_0059D69C;
extern s8 gNetOptLatency;
extern s8 gNetOptLatencyKey;
extern s8 gNetOptMaxRetries;
extern s8 gNetOptMaxRetriesKey;
extern u8 D_0059D700;
extern s32 D_0059D800;
extern s32 D_0059D808;
extern s32 gNetSema;
extern s32 D_0059D818;
extern u8 D_0059D820;
extern u8 D_0059D8A0;
extern s32 gNetGateway;
extern s32 gNetNetmask;
extern s32 gNetIpAddr;
extern u8 gNetPppLineModeFirst;
extern u8 gNetPhyMode;
extern u8 gNetAuthKey;
extern u8 gNetAuthName;
extern s32 gNetDns2;
extern s32 gNetDns1;
extern s8 gNetDomain;
extern s8 gNetPppAuthText;
extern u8 gNetPppConfBuf;
extern u8 gNetPppFlagSaved;
extern s8 gNetLanMode;
extern s16 D_0059DCF0;
extern s32 D_0059DCF8;
extern u8 D_0059DD00;
extern s8 D_0059DE00;
extern s8 gNetDriverType;
extern s32 gNetDevCount;
extern s32 gNetDevSeenCount;
extern NetDevice *gNetDevCur;   /* the device in use (in gNetDevs) */
/*
 * Reads socket sock's two AVE TCP options, then sets option 8200 to 1 and option
 * 8193 to 0. The lobby applies it to gNetSock.
 */
void NetTcp_SetOption(s32 sock)
{
    s32 opts[2];

    NetTcp_GetOptions(sock, opts);
    opts[0] = 1;
    opts[1] = 0;
    NetTcp_SetOptions(sock, opts);
}

const char D_0046A8E0[12] = {
#include "sub_main/0046A8E0.inc"
};
const char D_0046A8F0[3] = {
#include "sub_main/0046A8F0.inc"
};
const char D_0046A900[16] = {
#include "sub_main/0046A900.inc"
};
const char D_0046A910[5] = {
#include "sub_main/0046A910.inc"
};
const char D_0046A918[13] = {
#include "sub_main/0046A918.inc"
};
const char D_0046A928[2] = {
#include "sub_main/0046A928.inc"
};

/*
 * Clears the network settings: no device in use (gNetDevCur NULL, gNetDevs filled with
 * 255, both device counts -1, gNetDriverType 0), static LAN mode, the PPP, address, DNS
 * and authentication settings zeroed, the system language saved, gNetSema -1. Rebuilds
 * gNetOptList: the max-retries and initial-latency keys and values are copied into their
 * buffers, the DVD-init pair is pointed at directly, then two NULLs.
 */
void NetCfg_Reset(void)
{
    gNetDevCur = NULL;
    memset((void *) &gNetDevs, 255, 980U);
    gNetDriverType = 0;
    gNetDevSeenCount = -1;
    gNetDevCount = -1;
    D_0059DE00 = 0;
    memset((void *) &D_0059DD00, 0, 256U);
    D_0059DCF8 = 0;
    D_0059DCF0 = 0;
    gNetLanMode = 0;
    gNetPppFlagSaved = 0;
    memset((void *) &gNetPppConfBuf, 0, 240U);
    memset((void *) &gNetPppAuthText, 0, 16U);
    memset((void *) &gNetDomain, 0, 256U);
    gNetDns1 = 0;
    gNetDns2 = 0;
    memset((void *) &gNetAuthName, 0, 256U);
    memset((void *) &gNetAuthKey, 0, 256U);
    gNetPhyMode = 0;
    gNetPppLineModeFirst = 0;
    gNetIpAddr = 0;
    gNetNetmask = 0;
    gNetGateway = 0;
    D_0059D8A0 = sceScfGetLanguage();
    memset((void *) &D_0059D820, 0, 128U);
    D_0059D818 = 0;
    gNetSema = -1;
    D_0059D800 = -1;
    D_0059D808 = 0;
    memset((void *) &D_0059D700, 0, 256U);
    strncpy((char *) &gNetOptMaxRetriesKey, D_0046A8E0, 32U);
    strncpy((char *) &gNetOptMaxRetries, D_0046A8F0, 16U);
    strncpy((char *) &gNetOptLatencyKey, D_0046A900, 32U);
    strncpy((char *) &gNetOptLatency, D_0046A910, 16U);
    D_0059D698 = 0;
    gNetOptList = &gNetOptMaxRetriesKey;
    D_0059D684 = &gNetOptMaxRetries;
    D_0059D688 = &gNetOptLatencyKey;
    D_0059D68C = &gNetOptLatency;
    D_0059D690 = (u8 *)D_0046A918;
    D_0059D694 = (u8 *)D_0046A928;
    D_0059D69C = 0;
}
