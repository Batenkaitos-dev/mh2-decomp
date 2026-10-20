/*
 * emlRpcQueue: a ring of 64 requests for the IOP sound engine. Requests are
 * queued as they are made and sent once a frame by Cse_Update, one
 * CseRpc_Call each. mlRpcQueueSendIntr (for interrupt context) hands the
 * queue to a helper thread instead, but the game never creates that thread.
 */
#include "cse.h"
#include "main/ml_rpc_queue.h"   /* the texts this file uses (tools/content_gen.py) */

typedef struct MlRpcQueueEntry {
    s32 fno;
    u8 data[0x40];
    u32 size;
} MlRpcQueueEntry;

extern s32 gMlRpcQueueThread;
extern s32 gMlRpcQueueSema;
extern s32 gMlRpcQueueUploads;
extern volatile s32 gMlRpcQueueIntrCount;
extern MlRpcQueueEntry gMlRpcQueueIntrBuf[64];
extern volatile s32 gMlRpcQueueCount;
extern s32 gMlRpcQueueRead;
extern s32 gMlRpcQueueWrite;
extern MlRpcQueueEntry gMlRpcQueue[64];

void scePrintf(char *fmt, ...);
void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, void *src, u32 n);
s32 func_00114C60(s32 thid, s32 arg);
s32 SignalSema(s32 sema);

s32 MlRpcQueue_Init(void)
{
    memset(gMlRpcQueue, 0, sizeof(gMlRpcQueue));
    gMlRpcQueueWrite = 0;
    gMlRpcQueueRead = 0;
    gMlRpcQueueCount = 0;
    gMlRpcQueueUploads = 0;
    if (gMlRpcQueueThread >= 0) {
        func_00114C60(gMlRpcQueueThread, 0);
    }
    return 0;
}

/* Bank uploads (CSE_FNO_UPLOAD) still in the queue. */
s32 MlRpcQueue_NumUploads(void)
{
    return gMlRpcQueueUploads;
}

s32 mlRpcQueueReferStatus(void)
{
    if (gMlRpcQueueCount == 64) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf("mlRpcQueueReferStatus : Queue is full!\n");
        return 0x80000010;
    }
    return 0;
}

/*
 * Queues a request; gives it a sequence number (word 1) if it has none, and
 * returns that number (0 if the queue is full).
 */
u32 MlRpcQueue_Set(s32 fno, void *req, s32 size)
{
    u32 *hdr = req;

    if (mlRpcQueueReferStatus() < 0) {
        return 0;
    }
    if (hdr[1] == 0) {
        hdr[1] = Cse_NextSeq();
    }
    gMlRpcQueue[gMlRpcQueueWrite].fno = fno;
    gMlRpcQueue[gMlRpcQueueWrite].size = size;
    memset(gMlRpcQueue[gMlRpcQueueWrite].data, 0, 0x40);
    memcpy(gMlRpcQueue[gMlRpcQueueWrite].data, req, size);
    gMlRpcQueueWrite = (gMlRpcQueueWrite + 1) & 0x3F;
    gMlRpcQueueCount++;
    if (fno == CSE_FNO_UPLOAD) {
        gMlRpcQueueUploads++;
    }
    return hdr[1];
}

/*
 * Sends the queued requests in order. Stops at the first one the IOP refuses
 * (a negative first reply word: its queue is full); that one is sent again
 * next time. Returns the number still queued.
 */
s32 mlRpcQueueSend(void)
{
    s32 n;

    for (n = gMlRpcQueueCount; n != 0; n--) {
        MlRpcQueueEntry *e = &gMlRpcQueue[gMlRpcQueueRead];

        if (*(s32 *)CseRpc_Call(e->fno, e->data, e->size) >= 0) {
            gMlRpcQueueCount--;
            if (gMlRpcQueue[gMlRpcQueueRead].fno == CSE_FNO_UPLOAD) {
                gMlRpcQueueUploads--;
            }
        } else {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf(TXT(0x001C83C0) /* debug message: the IOP's queue (message box) is full */);
            break;
        }
        gMlRpcQueueRead = (gMlRpcQueueRead + 1) & 0x3F;
    }
    return gMlRpcQueueCount;
}

s32 mlRpcQueueSendIntr(void)
{
    s32 n;
    MlRpcQueueEntry *dst;

    if (gMlRpcQueueThread == -1 || gMlRpcQueueSema == -1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8400) /* debug message: the RPC queue module's thread was not created */);
    }
    n = gMlRpcQueueCount;
    gMlRpcQueueIntrCount = 0;
    memset(gMlRpcQueueIntrBuf, 0, sizeof(gMlRpcQueueIntrBuf));
    if (n != 0) {
        dst = gMlRpcQueueIntrBuf;
        do {
            *dst = gMlRpcQueue[gMlRpcQueueRead];
            gMlRpcQueueRead = (gMlRpcQueueRead + 1) & 0x3F;
            n--;
            dst++;
            gMlRpcQueueIntrCount++;
            gMlRpcQueueCount--;
        } while (n != 0);
    }
    SignalSema(gMlRpcQueueSema);
    return gMlRpcQueueCount;
}

/* This file's initialised data. */
s32 gMlRpcQueueThread = {
#include "main/001BCCA0.inc"
};
s32 gMlRpcQueueSema = {
#include "main/001BCCA8.inc"
};
