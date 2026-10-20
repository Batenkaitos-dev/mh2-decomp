#ifndef NET_SAVE_H
#define NET_SAVE_H
#include "common.h"

/* The settings being edited, in the game's own form (0xA20 bytes). */
typedef struct YnConfig {
    s32 ifcType;                /* 0x000 */
    u8 unk4;
    u8 noPppoe;                 /* 0x005: 0: PPPoE (authName, authKey) */
    u8 fixedIp;                 /* 0x006: 0: DHCP (dhcpHostname), else address/netmask/gateway */
    u8 dnsSet;                  /* 0x007: dns1/dns2 given */
    u8 phyConfig;               /* 0x008: 0 auto, 1-4 the ethernet modes */
    u8 ifcType3;                /* 0x009: 1: interface type 3 (Yn_ConfigToIfc) */
    u8 unkA[2];
    char authName[0x200];       /* 0x00C */
    char authKey[0x200];        /* 0x20C */
    char dhcpHostname[0x200];   /* 0x40C */
    u8 address[4];              /* 0x60C */
    u8 netmask[4];              /* 0x610 */
    u8 gateway[4];              /* 0x614 */
    u8 dns1[4];                 /* 0x618 */
    u8 dns2[4];                 /* 0x61C */
    char name[0x200];           /* 0x620: the entry's name (Shift-JIS here, EUC-JP in the file) */
    char vendor[0x100];         /* 0x820 */
    char product[0x100];        /* 0x920 */
} YnConfig;

/* The proxy settings (0x208 bytes). */
typedef struct YnProxy {
    s32 use;                    /* 0x000 */
    u16 httpPort;               /* 0x004 */
    u16 httpsPort;              /* 0x006 */
    char http[0x100];           /* 0x008 */
    char https[0x100];          /* 0x108 */
} YnProxy;

/*
 * The online data kept in the NET file on the memory card: sub_main's
 * gNetSaveData (0x1BEC bytes). The online set-up reads it from the card
 * and writes it back whole; the lobby keeps working copies of the friend
 * and blocked lists (Lobby_BackupWork, Lobby_RestoreWork), the web
 * browser the MMBB login, and the yn overlay the connection settings
 * and proxies.
 */
typedef struct NetSaveData {
    u8 friends[0x960];          /* 0x0000: the friend list, 50 records of 0x30 bytes (gFriendList's saved copy) */
    char mmbbLoginId[0xB];      /* 0x0960: the MMBB login id the browser fills in */
    char mmbbPassword[9];       /* 0x096B: its password */
    YnProxy proxy;              /* 0x0974: the proxy settings (Yn_LoadProxy, Yn_StoreProxy) */
    u8 unkB7C[0xC84 - 0xB7C];
    s8 setupResult;             /* 0x0C84: the online set-up's saved outcome, which it tests for 0 and 1; at 1 yn's connected message closes by itself */
    u8 unkC85[0xC90 - 0xC85];
    YnConfig config;            /* 0x0C90: the connection settings in use (Yn_ApplyEntry) */
    YnProxy configProxy;        /* 0x16B0: the proxy that goes with them */
    u8 blocked[0xC0];           /* 0x18B8: the blocked ids, 8 records of 0x18 bytes (gMailBlockList's saved copy) */
    u8 unk1978[0x1BEC - 0x1978];
} NetSaveData;
typedef char NetSaveData_size_check[(sizeof(NetSaveData) == 0x1BEC) ? 1 : -1];

extern NetSaveData gNetSaveData;

#endif /* NET_SAVE_H */
