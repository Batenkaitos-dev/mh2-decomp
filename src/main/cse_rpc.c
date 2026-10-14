/*
 * CSE SIF RPC client, "sndtest(EE_RPC)" in its error messages: binds the
 * sound engine's three IOP servers and makes one call at a time. Every call
 * sends 0x40 bytes and receives up to 0x400; the reply is copied to a
 * per-server buffer whose address CseRpc_Call returns.
 */
#include "cse.h"
#include "sif.h"

extern s32 gCseRpcSema;
extern s32 gCseRpcReqSema;
extern s32 gCseRpcDoneSema;
/* Never set: calls always go through CseRpc_Send, not a helper thread. */
extern void (*gCseRpcEndFunc)(void *);
extern u8 gCseThmnReply[0x400];
extern u8 gCseStatReply[0x40];
extern u8 gCseCommReply[0x40];
extern SifClientData *gCseRpcClient;
extern u8 gCseRpcRecvBuf[0x400];
extern u8 gCseRpcSendBuf[0x40];
extern s32 gCseRpcFno;
extern SifClientData gCseClientThmn;
extern SifClientData gCseClientStat;
extern SifClientData gCseClientComm;

void scePrintf(char *fmt, ...);
s32 DelayThread(s32 usec);
s32 func_001145F0(void);    /* nonzero with interrupts disabled */
void FlushCache(s32 op);
void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, void *src, u32 n);

s32 CseRpc_Init(void)
{
    SemaParam sp;

    sceSifInitRpc(0);
    do {
        if (sceSifBindRpc(&gCseClientComm, CSE_SID_COMM, 0) < 0) {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf("sndtest(EE_RPC) : Error : sceSifBindRpc \n");
        }
        DelayThread(1000);
    } while (gCseClientComm.serve == NULL);
    do {
        if (sceSifBindRpc(&gCseClientStat, CSE_SID_STAT, 0) < 0) {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf("sndtest(EE_RPC) : Error : sceSifBindRpc \n");
        }
        DelayThread(1000);
    } while (gCseClientStat.serve == NULL);
    do {
        if (sceSifBindRpc(&gCseClientThmn, CSE_SID_THMN, 0) < 0) {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf("sndtest(EE_RPC) : Error : sceSifBindRpc \n");
        }
        DelayThread(1000);
    } while (gCseClientThmn.serve == NULL);
    sp.initCount = 1;
    sp.maxCount = 1;
    gCseRpcSema = CreateSema(&sp);
    if (gCseRpcSema == -1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf("[CSE] Missing at CreateSema\n");
    }
    return 0;
}

/*
 * Sends a request (at most 0x40 bytes) to the server that handles fno and
 * returns the copy of its reply: 0x40 bytes for COMM, 0x20 for STAT, 0x400
 * for THMN. An unknown fno returns NULL without releasing the semaphore.
 */
void *CseRpc_Call(s32 fno, void *data, u32 size)
{
    void *reply;

    WaitSema(gCseRpcSema);
    switch (fno) {
    case CSE_FNO_INIT:
    case CSE_FNO_UPLOAD:
    case CSE_FNO_COMM:
        gCseRpcClient = &gCseClientComm;
        break;
    case CSE_FNO_STAT:
        gCseRpcClient = &gCseClientStat;
        break;
    case CSE_FNO_THMN:
        gCseRpcClient = &gCseClientThmn;
        break;
    default:
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf("sndtest(EE_RPC) : Error : Unknown command.\n");
        return NULL;
    }
    memset(gCseRpcSendBuf, 0xFF, sizeof(gCseRpcSendBuf));
    memset(gCseRpcRecvBuf, 0xFF, sizeof(gCseRpcRecvBuf));
    size = (size < 0x40) ? size : 0x40;
    memcpy(gCseRpcSendBuf, data, size);
    gCseRpcFno = fno;
    if (gCseRpcEndFunc != NULL) {
        SignalSema(gCseRpcReqSema);
        WaitSema(gCseRpcDoneSema);
    } else {
        CseRpc_Send();
    }
    switch (fno) {
    case CSE_FNO_INIT:
    case CSE_FNO_UPLOAD:
    case CSE_FNO_COMM:
        memcpy(gCseCommReply, gCseRpcRecvBuf, 0x40);
        reply = gCseCommReply;
        break;
    case CSE_FNO_STAT:
        memcpy(gCseStatReply, gCseRpcRecvBuf, 0x20);
        reply = gCseStatReply;
        break;
    case CSE_FNO_THMN:
        memcpy(gCseThmnReply, gCseRpcRecvBuf, 0x400);
        reply = gCseThmnReply;
        break;
    }
    SignalSema(gCseRpcSema);
    return reply;
}

/* Waits for the previous call to finish, then calls until the call succeeds. */
void CseRpc_Send(void)
{
    s32 ret;

    while (sceSifCheckStatRpc(gCseRpcClient) == 1) {
        DelayThread(100);
    }
    if (func_001145F0() == 0) {
        FlushCache(0);
    }
    do {
        ret = sceSifCallRpc(gCseRpcClient, gCseRpcFno, 0, gCseRpcSendBuf, 0x40, gCseRpcRecvBuf, 0x400,
                            gCseRpcEndFunc, NULL);
        if (ret < 0) {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf("sceSifCallRpc : error!\n");
        }
    } while (ret != 0);
}

/* cse_rpc's .data (0x1BCC80-0x1BCC98) */
s32 gCseRpcSema = {
#include "main/001BCC80.inc"
};
s32 gCseRpcReqSema = {
#include "main/001BCC88.inc"
};
s32 gCseRpcDoneSema = {
#include "main/001BCC90.inc"
};
