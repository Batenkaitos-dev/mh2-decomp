/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "save.h"
#include "lobby_globals.h"
#include "net_tcp.h"
#include "browser.h"
void NetScene_ToState6(void);
struct NetAddr;
s32 NetTcp_Open(struct NetAddr *);
int strncmp(const char *, const char *, unsigned int);

extern u8 gLoginState;
extern u8 gNetScreenStep;

extern s32 (*jtbl_008193E0[])(void);    /* the network screen steps (a data table) */
extern s32 (*jtbl_008193D0[])(void);    /* the connection steps (a data table) */
extern u8 gNetConnStep;

extern u8 gLbsConnectStep;
extern char gLbsHost[0x40];           /* the lobby server's host */
extern u16 gLbsPort;                  /* its port */
extern u16 gLbsResolveTimer;
extern u8 gLoginConnectStep;
extern s8 gLbsConnectSecs;
extern s8 gLbsConnectTimer;
extern u8 gAccountData[];
extern char gCurServerId[];
s32 strncmp(const char *a, const char *b, u32 n);
char *strncpy(char *dst, const char *src, u32 n);  /* strncpy */
s32 Net_ResolveHost(s8 *st, s16 *timer, s32 *out, s8 *name);
/* LobbyNet_ConnectServer casts the value to s32 before Net_Connect's u16 parameter: MWCC keeps the game's andi that way */
u16 Swap16(s32 x);
extern s32 gNetSock;
s32 NetTcp_Abort(s32 sock);
s32 NetTcp_Delete(s32 *sock);
void *memset(void *dst, s32 c, u32 n);

s32 Talk_StepData(u16 k);
extern s32 gTalkLineText;
extern s32 gTalkTextFrames;
extern s32 gTalkLine;
extern s8 gTalkMode;
s32 Net_DisconnectStep(s8 *st, s16 *t);
extern s16 gNetStepTimeout;
extern s8 D_008F8A98;
extern s8 D_008F8AA7;
extern s8 gNetDisconnectStep;

extern s32 gLobbyRecvGot;
extern s32 gLobbyRecvSize;
extern s8 gLobbyRecvPart;
extern s8 gLobbyRecvHdrDone;

extern s8 gServerSkipTried;
extern M2C_UNK gServerTried;

char *Talk_LineText(u16 k) {
    return (char *)Talk_StepData(k);
}

extern u16 gTalkScript;
extern u8 (*gTalkLines[])[4];

/* The first byte of the current talk (gTalkLines[kind][talk]). The pointer
 * step (talks += talk) gives the original's pointer-first addu. The callers
 * pass the talk (u16), which it does not use. */
s32 Talk_FirstByte(u16 talk)
{
    u8 (*talks)[4];

    talks = gTalkLines[gTalkScript];
    talks += gTalkLine;
    return (*talks)[0];
}

u16 LobbyPad_Get(s32);
s32 Notice_OpenTextN(char *text, s32 width, s32 mode, s32 sel, s32 arg4); /* unit's own declaration: width s32 (the definition has u32): with u32 Talk_RunText and Talk_Step change by 2 words each */
extern s32 gTalkNpcName;

/* One frame of the current talk's text (Notice_OpenTextN: the text, gTalkTextFrames
 * its frame count, the talk's first byte, the cursor gTalkMode): 2 while
 * it runs, 0 at its end. */
s32 Talk_RunText(void)
{
    s32 data;

    LobbyPad_Get(0);
    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    data = gTalkLineText != 0 ? gTalkLineText : Talk_StepData((u16)gTalkLine);
    if (Notice_OpenTextN((char *)data, gTalkTextFrames, Talk_FirstByte(gTalkLine), gTalkMode, gTalkNpcName) == 0) {
        return 0;
    }
    return 2;
}

void LobbySe_Play(s32 se);

/* The same with the buttons once the text is shown (pad bits from
 * LobbyPad_Get): 0x20 and 0x40 go on to the next talk, by the yes/no
 * choice gTalkMode when the talk's first byte is 1 (the talk's bytes 1
 * and 2 skip ahead); 0x800/0x400 move the choice. Returns 2 while the
 * talk goes on, 0 at its end (with flag 1 also when a choice has no
 * talk after it). */
s32 Talk_Step(s8 flag)
{
    u16 pad = LobbyPad_Get(0);
    s32 data;
    s32 first;
    u8 (*talks)[4];
    s32 n;

    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    data = gTalkLineText != 0 ? gTalkLineText : Talk_StepData((u16)gTalkLine);
    first = Talk_FirstByte(gTalkLine);
    talks = gTalkLines[gTalkScript];
    talks += gTalkLine;
    if (Notice_OpenTextN((char *)data, gTalkTextFrames, first, gTalkMode, gTalkNpcName) == 0) {
        if (pad & 0x20) {
            if (first == 2) {
                gTalkTextFrames = 0;
                gTalkMode = 0;
                gTalkLine++;
            } else if (first == 1) {
                if (gTalkMode != 1) {
                    LobbySe_Play(0);
                    if ((n = (*talks)[1]) != 0) {
                        gTalkTextFrames = 0;
                        gTalkMode = 0;
                        gTalkLine += n;
                    } else if (flag == 1) {
                        gTalkTextFrames = 0;
                        return 0;
                    } else {
                        gTalkTextFrames = 0;
                        gTalkMode = 0;
                        gTalkLine++;
                    }
                } else {
                    LobbySe_Play(4);
                    if ((n = (*talks)[2]) != 0) {
                        gTalkTextFrames = 0;
                        gTalkMode = 0;
                        gTalkLine += n;
                    } else if (flag == 1) {
                        gTalkTextFrames = 0;
                        return 0;
                    } else {
                        gTalkTextFrames = 0;
                        gTalkMode = 0;
                        gTalkLine++;
                    }
                }
            } else {
                return 0;
            }
        } else if (pad & 0x40) {
            if (first == 2) {
                gTalkTextFrames = 0;
                gTalkMode = 0;
                gTalkLine++;
            } else if (first == 1) {
                LobbySe_Play(4);
                if (gTalkMode != 1) {
                    gTalkMode = 1;
                } else if ((n = (*talks)[2]) != 0) {
                    gTalkTextFrames = 0;
                    gTalkMode = 0;
                    gTalkLine += n;
                } else if (flag == 1) {
                    gTalkTextFrames = 0;
                    return 0;
                } else {
                    gTalkTextFrames = 0;
                    gTalkMode = 0;
                    gTalkLine++;
                }
            } else {
                gTalkTextFrames = 0;
                return 0;
            }
        } else if (first == 1) {
            if (pad & 0x800) {
                if (gTalkMode != 0) {
                    LobbySe_Play(1);
                    gTalkMode = 0;
                }
            } else if (pad & 0x400) {
                if (gTalkMode != 1) {
                    LobbySe_Play(1);
                    gTalkMode = 1;
                }
            }
        }
    } else if (pad & 0x60) {
        gTalkTextFrames = -1;
    }
    return 2;
}

void Save_AddMoney(s32 n);
s32 sprintf(char *, const char *, ...);
void Sjis_Widen(char *src, char *dst);
u8 Notice_PushText(char *);
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
u8 Notice_Push(u8, s16, s16);
void Menu_PushNotice(s32 msg);
u16 Box_FindFreeSlot(void *unused0);
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
extern char *gZennyReceivedTexts[];
typedef struct TalkGift {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 kind;         /* 1 money, 2 an item for the pouch, 3 equipment for the box */
    /* 0x4 */ union {
        s16 value;              /* money, or the item as the message takes it */
        u16 item;
        u8 type;
    } a;
    /* 0x6 */ union {
        s16 count;
        u16 id;
    } b;
} TalkGift;
extern TalkGift *gTalkGifts[];

/* Talk_Step with its end: 3 when "no" was chosen, else the talk's gift
 * (money, a pouch item or equipment for the box) and 0. */
s32 Talk_Update(void)
{
    TalkGift *g;
    u8 (*talks)[4];
    u8 gift;
    s16 n;
    char msg[0x20];
    char buf[0x20];

    if (Talk_Step(1) == 0) {
        if (gTalkMode == 1) {
            return 3;
        }
        talks = gTalkLines[gTalkScript];
        talks += gTalkLine;
        gift = (*talks)[3];
        if (gift != 0xFF) {
            g = gTalkGifts[gTalkScript];
            g += gift;
            switch (g->kind) {
            case 1:
                Save_AddMoney(g->a.value);
                LobbySe_Play(0xA);
                sprintf(msg, gZennyReceivedTexts[0], g->a.value);
                Sjis_Widen(msg, buf);
                Notice_PushText(buf);
                break;
            case 2:
                n = g->b.count;
                if (SavePouch_Space(g->a.item) >= n) {
                    SavePouch_Add(g->a.item, n);
                    Notice_Push(1, 0xD, g->a.value);
                } else {
                    Menu_PushNotice(0x15);
                }
                break;
            case 3:
                if ((u16)Box_FindFreeSlot(&gSave) != 0xFFFF) {
                    Box_AddItem(&gSave, g->a.type, g->b.id, 0);
                    Menu_PushNotice(0x23);
                } else {
                    Menu_PushNotice(0xB);
                }
                break;
            }
        }
        return 0;
    }
    return 2;
}

void Talk_Say(s32 line, s8 mode) {
    gTalkLine = line;
    gTalkMode = mode;
    gTalkLineText = 0;
    gTalkTextFrames = 0;
}

extern u8 gNetEntryMode;

void NetScene_ToPortalPage3(void)
{
    gNetEntryMode = 0;
    gPortalPage = 3;
    NetScene_ToState6();
}

s32 LobbyNet_DisconnectWait(void) {
    D_008F8A98 = 1;
    if (Net_DisconnectStep(&gNetDisconnectStep, &gNetStepTimeout) == 1) {
        D_008F8AA7 = 0;
        gNetDisconnectStep = 0;
        return 1;
    }
    return 0;
}

void LobbyRecv_Reset(void) {
    gLobbyRecvGot = 0;
    gLobbyRecvSize = 0;
    gLobbyRecvPart = 1;
    gLobbyRecvHdrDone = 0;
}

typedef struct NetAddr {
    s32 addr;
    u16 port;
    u16 unk6;
} NetAddr;

/* Falls off the end: v0 still holds NetTcp_Open's socket (LobbyRecv_Reset leaves v0 alone),
 * which both callers keep in gNetSock. A port returns NetTcp_Open's value. */
s32 Net_Connect(s32 addr, u16 port, u16 unk6)
{
    NetAddr a;

    a.addr = addr;
    a.port = port;
    a.unk6 = unk6;
    NetTcp_Open((struct NetAddr *)&a);
    LobbyRecv_Reset();
}


extern NetTcpStatus gNetTcpStatus;
s32 NetTcp_Status(s32 sock, NetTcpStatus *st);
s32 NetTcp_Recv(s32 sock, void *buf, s16 len);
/* the receive state (LobbyRecv_Reset resets it; declared s8 above) */
#define gRecvGot (gLobbyRecvGot)            /* bytes of the payload received */
#define gRecvSize (gLobbyRecvSize)           /* the payload size still to come */
#define gRecvPart (*(u8 *)&gLobbyRecvPart)   /* the part number, from 1 */
#define gRecvHdrDone (*(u8 *)&gLobbyRecvHdrDone) /* 1: the header was received */

/*
 * Receives one message, a part at a time: the 12-byte header first (once
 * NetTcp_Status says 12 bytes wait), then its payload (header bytes 4-5,
 * big-endian) into data, at most max bytes per part. Returns -1 on an error
 * or a lost connection, 0 while nothing complete came, 1 when the message is
 * complete, 2, 3... when data holds a full part of a longer message (the
 * next part starts again at data[0]).
 */
s32 Lobby_RecvPacket(s32 sock, u8 *hdr, u8 *data, u32 max)
{
    s32 n;
    s32 r;

    if (NetTcp_Status(sock, &gNetTcpStatus) < 0) {
        return -1;
    }
    if (gNetTcpStatus.state != 4) {
        return -1;
    }
    if (gNetTcpStatus.sendSpace <= 0x400) {
        return -1;
    }
    if (gRecvHdrDone == 0 && gNetTcpStatus.recvLen >= 12U) {
        if (NetTcp_Recv(sock, hdr, 12) != 12) {
            return -1;
        }
        gRecvHdrDone = 1;
        gRecvSize = (u16)(hdr[4] << 8);
        gRecvSize += hdr[5];
        gRecvSize = (u16)gRecvSize;
        gNetTcpStatus.recvLen -= 12;
        gRecvGot = 0;
    }
    if (gRecvHdrDone != 0) {
        n = gRecvSize - gRecvGot;
        if (n == 0) {
            LobbyRecv_Reset();
            return 1;
        }
        if (n > gNetTcpStatus.recvLen) {
            n = gNetTcpStatus.recvLen;
        }
        if (n > max - gRecvGot) {
            n = max - gRecvGot;
        }
        if (0 < n) {
            r = NetTcp_Recv(sock, data + gRecvGot, n);
            if (r < 0) {
                return -1;
            }
            gRecvGot += r;
            if (gRecvSize == gRecvGot) {
                LobbyRecv_Reset();
                return 1;
            }
            if (gRecvGot == max) {
                gRecvGot = 0;
                gRecvSize -= max;
                return ++gRecvPart;
            }
        }
    }
    return 0;
}

/*
 * Connects to the lobby server, one step a frame (gLbsConnectStep); returns -2
 * while busy, 0 once connected, -1 on a failure.
 * 0: the address: among sub_main's 40 records at 0x5CAED0 + 0x117F (0x101
 *    bytes each, a 12-character id first), the one with the id of the
 *    server chosen (sub_main 0x5CAA70, from the server list screens) gives
 *    the "host:port" string at 0x5CAED0 + 0x22F + i * 0x62 (0x40 bytes);
 *    the port is parsed as decimal (up to a space).
 * 1: Net_ResolveHost (DNS), then Net_Connect(address, htons(port)).
 * 2: waits for TCP state 4, 21 x 60 frames at most.
 */
s32 LobbyNet_ConnectServer(s32 unused)
{
    s32 i;
    u8 *e;
    u8 *p;
    s32 addr = 0;

    switch (gLbsConnectStep) {
    case 0:
        gLbsHost[0] = 0;
        gLbsPort = 0;
        gLbsResolveTimer = 0;
        gLoginConnectStep = 0;
        memset(gLbsHost, 0, 0x40);
        for (i = 0, e = gAccountData; i < 40; i++, e += 0x101) {
            if (strncmp(gCurServerId, (char *)(e + 0x117F), 0xC) == 0) {
                strncpy(gLbsHost, (char *)(gAccountData + i * 0x62 + 0x22F), 0x40);
                break;
            }
        }
        p = (u8 *)gLbsHost;
        for (;;) {
            if (*p == ':' || *p == 0) {
                break;
            }
            p++;
        }
        if (*p == ':') {
            *p = 0;
            p++;
            while (*p != ' ' && *p != 0) {
                gLbsPort *= 10;
                gLbsPort += *p++ - '0';
            }
        }
        gLbsConnectStep = 1;
        return -2;
    case 1:
        switch (Net_ResolveHost((s8 *) &gLoginConnectStep, (s16 *) &gLbsResolveTimer, &addr, (s8 *) gLbsHost)) {
        case 0:
            return -2;
        case 1:
            gLoginConnectStep = 0;
            break;
        case -1:
            gLoginConnectStep = 0;
            return -1;
        }
        if ((gNetSock = Net_Connect(addr, (s32)Swap16(gLbsPort), 0)) < 0) {
            return -1;
        }
        gLbsConnectSecs = 0;
        gLbsConnectStep = 2;
        gLbsConnectTimer = 60;
        return -2;
    case 2:
        gLbsConnectTimer--;
        if (gLbsConnectTimer <= 0) {
            gLbsConnectTimer = 60;
            gLbsConnectSecs++;
            if (gLbsConnectSecs > 20) {
                NetTcp_Abort(gNetSock);
                NetTcp_Delete(&gNetSock);
                gLbsConnectTimer = 0;
                gLbsConnectSecs = 0;
                return -1;
            }
        }
        if (NetTcp_Status(gNetSock, &gNetTcpStatus) < 0) {
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
            return -1;
        }
        if (gNetTcpStatus.state != 4) {
            return -2;
        }
        gLbsConnectTimer = 0;
        gLbsConnectSecs = 0;
        gLbsConnectStep = 0;
        return 0;
    }
    return -1;
}

s32 Browser_Update(void);
void Browser_Init(u8 mode);
void LobbyKddi_CloseBrowser(void);
void NetMode_SeqAReset(void);
void FlushCache(s32 kind);
char *strcpy(char *dst, const char *src);
extern u8 gPortalPage;                   /* sub_main: which portal URL (0 the default) */
extern char gBrowserUrl[0x100];            /* the browser's URL (sub_main) */
extern char gPortalUrlA[];                 /* sub_main: the portal URLs */
extern char gPortalUrlB[];

/* the network screen's first step: the KDDI MMBB portal in the HTML
 * browser (a URL from sub_main by 0x6ADFD7, or the default), until the
 * player leaves it; 1 done, -1 cancelled, 0 busy. Reading code only. */
s32 LobbyKddi_PortalStep(void)
{
    s32 r = 0;

    switch (gLbsConnectStep) {
    case 0:
        gLbsConnectStep++;
        NetMode_SeqAReset();
        D_5C34F0 = 0;
        break;
    case 1:
        gLbsConnectStep++;
        FlushCache(0);
        memset(gBrowserUrl, 0, 0x100);
        switch (gPortalPage) {
        case 1:
            strcpy(gBrowserUrl, gPortalUrlA);
            gBrowserTopMode = 0;
            Browser_Init(0);
            break;
        case 2:
        case 3:
            gBrowserTopMode = 1;
            strcpy(gBrowserUrl, gPortalUrlB);
            Browser_Init(0);
            break;
        default:
            gPortalPage = 0;
            gBrowserTopMode = 0;
            strcpy(gBrowserUrl, "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp");
            Browser_Init(0);
        }
        break;
    case 2:
        switch (Browser_Update()) {
        case 0:
            break;
        case 1:
            if (gPortalPage == 2 || (gPortalPage != 2 && gBrowserState->page == 0)) {
                LobbyKddi_CloseBrowser();
                r = -1;
                gPortalPage = 0;
            } else {
                gLbsConnectStep++;
                gPortalPage = 1;
            }
            break;
        case 2:
            LobbyKddi_CloseBrowser();
            gPortalPage = 0;
            gLbsConnectStep = 0x63;
            break;
        case -1:
            LobbyKddi_CloseBrowser();
            r = -1;
            gPortalPage = 0;
            gNetEntryMode = 0;
            break;
        case -100:
            LobbyKddi_CloseBrowser();
            gPortalPage = 0;
            gLbsConnectStep = 0x63;
        }
        break;
    case 0x63:
        gLbsConnectStep = 2;
        memset(gBrowserUrl, 0, 0x100);
        strcpy(gBrowserUrl, "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp");
        gBrowserTopMode = 0;
        Browser_Init(0);
        break;
    case 3:
        gLbsConnectStep++;
        LobbyKddi_CloseBrowser();
        break;
    case 4:
        gLbsConnectStep = 5;
        break;
    case 5:
        gLbsConnectStep = 6;
        break;
    case 6:
        r = 1;
        break;
    }
    return r;
}

/* the network screen's connection step (the table at 0x8193D0) */
s32 NetScreen_ConnectStep(void)
{
    return jtbl_008193D0[gNetConnStep]();
}

/* The first entry of the function table jtbl_008193D0: splat had merged it into
 * NetScreen_ConnectStep, the dispatcher before it (lead, round 8). Resets the
 * connection's states: step 1 next. */
s32 LobbyConnect_Reset(void)
{
    gLbsConnectStep = 0;
    gNetScreenStep = 0;
    gNetConnStep = 1;
    return 0;
}

/* the network screen step 1: connects to the lobby server (LobbyNet_ConnectServer);
 * 1 once connected (the connection steps start again at 0), -1 on a failure */
s32 LobbyConnect_Connect(void)
{
    s32 r = 0;

    switch (LobbyNet_ConnectServer(1)) {
    case -1:
        r = -1;
        break;
    case -2:
        break;
    case 0:
        gNetConnStep = 0;
        gLbsConnectStep = 0;
        gLoginState = 3;
        gNetScreenStep = 0;
        r = 1;
        gNetStepTimeout = 0;
        break;
    }
    return r;
}

/* the network screen step (the table at 0x8193E0) */
s32 NetScreen_Run(void)
{
    return jtbl_008193E0[gNetConnStep]();
}

void ServerList_ClearTried(void) {
    memset(&gServerTried, 0, 0x320);
    gServerTriedCount = 0;
    gServerSkipTried = 0;
}

/* unk_7AEF10's .data (0x8193D0-0x819400) */
s32 NetScreen_Step0(void);
s32 NetScreen_Step1(void);
s32 NetScreen_Step2(void);
s32 NetScreen_Step3(void);
s32 NetScreen_Step4(void);
s32 NetScreen_Step5(void);
s32 (*jtbl_008193D0[4])(void) = {
    LobbyConnect_Reset, LobbyConnect_Connect, NULL, NULL,
};
s32 (*jtbl_008193E0[8])(void) = {
    NetScreen_Step0, NetScreen_Step1, NetScreen_Step2, NetScreen_Step3,
    NetScreen_Step4, NetScreen_Step5, NULL, NULL,
};
