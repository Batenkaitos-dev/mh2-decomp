/*
 * The per-frame stream update and the queue of
 * background reads (256 requests: AFS files, named files, or calls made
 * in order with the reads).
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"
#include "scene.h"
#include "lobby_state.h"
#include "overlay.h"
extern s32 gLoadLastVSync;      /* sub_main 0x5D4A50 */

#define FILEREQ_AFS 0       /* file (id >> 16) of partition (id & 0xFFFF) */
#define FILEREQ_NAME 1      /* file by name */
#define FILEREQ_CALL 2      /* call func with nargs of args */

typedef struct FileReq {
    /* 0x00 */ s32 type;            /* FILEREQ_*, -1: free */
    /* 0x04 */ union {
        u32 id;                     /* FILEREQ_AFS: ptid << 16 | flid */
        void (*func)(void);             /* FILEREQ_CALL */
    } u;
    /* 0x08 */ s32 nargs;           /* FILEREQ_CALL: 0..4 */
    /* 0x0C */ void *dst;
    /* 0x10 */ union {
        char name[0xF0];            /* FILEREQ_NAME */
        s32 args[4];                /* FILEREQ_CALL */
    } v;
} FileReq; /* size 0x100 */

typedef struct FileQueue {
    /* 0x00 */ s8 busy;             /* requests pending */
    /* 0x01 */ s8 state;            /* 0: idle, 1: reading */
    /* 0x04 */ s32 read;
    /* 0x08 */ s32 write;
    /* 0x0C */ ADXF adxf;
} FileQueue;

extern FileReq gFileReqs[256];
extern FileQueue gFileQueue;
extern s32 gVSyncCount;
/* lobby overlay data (only when overlay 4 is loaded) */
extern LobbyState gLobbyState;

void *memset(void *dst, s32 c, u32 n);
s32 Overlay_IsLoaded(s32 id);
void Load_AdvanceFrames(s32 frames, s32 unused);
void Vib_Stop(s32 port);
s32 Game_IsOnline(void);
void func_8FC650(void);
s32 Lobby_ReceiveAll(s32 sock);
void Quest_UpdateTimers(s32 dt); /* Quest_UpdateTimers */
void PlayerLook_ResetLoaded(void);
void FileQueue_Next(FileReq *r);
void FileQueue_Call(FileReq *r);

void Stream_Update(void)
{
    Stream_UpdateFades();
    FileQueue_Update();
}

extern s32 gNetSock;
extern u8 D_008EE27E;
/* Keeps the game ticking while a load waits: once per new VSync, the
 * sub_main timers and, when their overlays are loaded, the gm_sub and
 * lobby (network) updates. */
void Load_Idle(void)
{
    s32 now = gVSyncCount;

    if (gLoadLastVSync != now) {
        Load_AdvanceFrames(now - gLoadLastVSync, 0);
        Vib_Stop(0);
        gLoadLastVSync = gVSyncCount;
        if (Game_IsOnline() == 1 && gSysSettings.gmSubLoaded != 0) {
            func_8FC650();
        }
        if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1 && Game_IsOnline() == 1 && gScene.inLobby != 0 && D_008EE27E != 0
            && gLobbyState.modelLoading == NULL) {
            Lobby_ReceiveAll(gNetSock);
        }
        if (Overlay_IsLoaded(OVERLAY_SUB_MAIN) == 1) {
            Quest_UpdateTimers(1);
        }
    }
}

/*
 * Empties the background read queue: clears gFileQueue and marks all 256
 * requests free (type -1).
 */
void FileQueue_Init(void)
{
    s32 i;

    memset(&gFileQueue, 0, sizeof(FileQueue));
    for (i = 0; i < 256; i++) {
        gFileReqs[i].type = -1;
    }
}

/*
 * Queues a read of an AFS file into dst; id is ptid << 16 | flid. When the
 * next request is still in use (the queue is full), first waits for the
 * queue to drain (FileQueue_Flush).
 */
void FileQueue_AddFile(u32 id, void *dst)
{
    FileReq *req = &gFileReqs[gFileQueue.write];
    FileQueue *queue = &gFileQueue;

    if (req->type >= 0) {
        FileQueue_Flush();
        req = &gFileReqs[queue->write];
    }
    req->type = FILEREQ_AFS;
    req->u.id = id;
    req->dst = dst;
    queue->busy = 1;
    queue->write = ++queue->write % 256;
}

/*
 * Queues a call of func with the first nargs (0-4) of firstArg..fourthArg,
 * made in order with the reads (FileQueue_Call). Unlike FileQueue_AddFile
 * it does not check for a full queue.
 */
void FileQueue_AddCall(void (*func)(void), s32 nargs, s32 firstArg, s32 secondArg, s32 thirdArg, s32 fourthArg)
{
    FileReq *req = &gFileReqs[gFileQueue.write];

    req->type = FILEREQ_CALL;
    req->u.func = func;
    req->nargs = nargs;
    req->v.args[0] = firstArg;
    req->v.args[1] = secondArg;
    req->v.args[2] = thirdArg;
    req->v.args[3] = fourthArg;
    gFileQueue.busy = 1;
    gFileQueue.write = ++gFileQueue.write % 256;
}

/* Per frame: starts the next request, or finishes the read under way
 * (a read that fails is retried). */
void FileQueue_Update(void)
{
    FileQueue *queue = &gFileQueue;
    FileReq *req = &gFileReqs[gFileQueue.read];
    s32 stat;

    switch (gFileQueue.state) {
    case 0:
        if (queue->busy == 0) {
            break;
        }
        if (queue->adxf != NULL) {
            ADXF_Stop(queue->adxf);
            ADXF_Close(queue->adxf);
        }
        if (req->type == FILEREQ_AFS) {
            queue->adxf = ADXF_OpenAfs((req->u.id >> 16) & 0xFFFF, req->u.id & 0xFFFF);
            if (queue->adxf == NULL) {
                break;
            }
            ADXF_ReadNw(queue->adxf, ADXF_GetFsizeSct(queue->adxf), req->dst);
            queue->state++;
        } else if (req->type == FILEREQ_CALL) {
            FileQueue_Call(req);
            FileQueue_Next(req);
        } else if (req->type == FILEREQ_NAME) {
            queue->adxf = ADXF_Open(req->v.name, NULL);
            if (queue->adxf == NULL) {
                break;
            }
            ADXF_ReadNw(queue->adxf, ADXF_GetFsizeSct(queue->adxf), req->dst);
            queue->state++;
        }
        break;
    case 1:
        stat = ADXF_GetStat(queue->adxf);
        if (stat == ADXF_STAT_READING) {
            break;
        }
        ADXF_Close(queue->adxf);
        queue->adxf = NULL;
        if (stat == ADXF_STAT_ERROR) {
            break;
        }
        FileQueue_Next(req);
        queue->state = 0;
        break;
    }
}

/*
 * Frees the finished request req and moves the read index on; clears
 * gFileQueue.busy when the next request is free.
 */
void FileQueue_Next(FileReq *req)
{
    FileQueue *queue = &gFileQueue;

    req->type = -1;
    gFileQueue.read = ++gFileQueue.read % 256;
    if (gFileReqs[gFileQueue.read].type < 0) {
        queue->busy = 0;
    }
}

/* Drops the queue and the read under way. */
void FileQueue_Cancel(void)
{
    FileQueue *queue = &gFileQueue;
    s32 i;

    if (gFileQueue.adxf != NULL) {
        ADXF_Stop(queue->adxf);
        ADXF_Close(queue->adxf);
    }
    FileQueue_Init();
    PlayerLook_ResetLoaded();
    if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1) {
        for (i = 0; i < 10; i++) {
            gLobbyState.modelLoading[i] = 0;
            gLobbyState.lookPending[i] = 0;
        }
    }
}

s32 FileQueue_IsBusy(void)
{
    return gFileQueue.busy;
}

/*
 * Whether lobby player playerIndex's look is still to be applied
 * (gLobbyState.lookPending, nonzero while pending).
 */
s8 func_00193670(s32 playerIndex)
{
    return gLobbyState.lookPending[playerIndex];
}

/*
 * Runs a FILEREQ_CALL request: calls its function with its first nargs
 * arguments (0-4); any other count calls nothing.
 */
void FileQueue_Call(FileReq *req)
{
    void (*func)(void) = req->u.func;
    s32 firstArg = req->v.args[0];
    s32 secondArg = req->v.args[1];
    s32 thirdArg = req->v.args[2];
    s32 fourthArg = req->v.args[3];
    s32 argCount = req->nargs;

    if (argCount == 0) {
        func();
    } else if (argCount == 1) {
        ((void (*)(s32))func)(firstArg);
    } else if (argCount == 2) {
        ((void (*)(s32, s32))func)(firstArg, secondArg);
    } else if (argCount == 3) {
        ((void (*)(s32, s32, s32))func)(firstArg, secondArg, thirdArg);
    } else if (argCount == 4) {
        ((void (*)(s32, s32, s32, s32))func)(firstArg, secondArg, thirdArg, fourthArg);
    }
}

/*
 * Runs the queue until it is empty, calling Load_Idle between steps so the
 * game keeps ticking.
 */
void FileQueue_Flush(void)
{
    while (FileQueue_IsBusy()) {
        FileQueue_Update();
        Load_Idle();
    }
}
