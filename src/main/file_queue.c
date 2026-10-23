/*
 * The per-frame stream update and the queue of background reads: a ring
 * of FILE_QUEUE_LEN requests (an AFS file, a file by name, or a call made
 * in order with the reads), one started or finished per frame
 * (FileQueue_Update). Load_Idle keeps the game ticking while a load
 * waits, and FileQueue_Flush runs the queue to its end with it.
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"
#include "scene.h"
#include "lobby_state.h"
#include "overlay.h"
#include "lobby_net.h"
#include "model.h"
#include "quest.h"
#include "game_util.h"
#include "main.h"
#include "vibration.h"
#include "libc.h"
extern s32 gLoadLastVSync;      /* the VSync count Load_Idle last ran at */

#define FILE_QUEUE_LEN 256  /* the requests in the ring */

#define FILEREQ_FREE -1     /* the slot is free */
#define FILEREQ_AFS 0       /* file (id & 0xFFFF) of partition (id >> 16) */
#define FILEREQ_NAME 1      /* file by name */
#define FILEREQ_CALL 2      /* call func with nargs of args */

/* FileQueue.state */
#define FILEQ_IDLE 0        /* the next request may start */
#define FILEQ_READING 1     /* a read is under way */

typedef struct FileReq {
    /* 0x00 */ s32 type;            /* FILEREQ_* */
    /* 0x04 */ union {
        u32 id;                     /* FILEREQ_AFS: ptid << 16 | flid */
        void (*func)(void);             /* FILEREQ_CALL */
    } u;
    /* 0x08 */ s32 nargs;           /* FILEREQ_CALL: 0..4 */
    /* 0x0C */ void *dst;
    /* 0x10 */ union {
        char name[240];            /* FILEREQ_NAME */
        s32 args[4];                /* FILEREQ_CALL */
    } v;
} FileReq; /* size 256 */

typedef struct FileQueue {
    /* 0x00 */ s8 busy;             /* requests pending */
    /* 0x01 */ s8 state;            /* FILEQ_* */
    /* 0x04 */ s32 read;             /* the next request to run */
    /* 0x08 */ s32 write;            /* the next request to fill */
    /* 0x0C */ ADXF adxf;
} FileQueue;

extern FileReq gFileReqs[FILE_QUEUE_LEN];
extern FileQueue gFileQueue;
extern s32 gVSyncCount;
/* lobby overlay data (only when overlay 4 is loaded) */
extern LobbyState gLobbyState;

/* gm_sub's QNetIf_UpdateOnline, called by its address-derived name: main's
 * link knows only sub_main's and the lobby's names. Naming the address in
 * main's own symbols would export the name to every overlay, where it would
 * replace gm_sub's own definition and label a point inside nethttp's
 * HttpReq_ReadBody, which nethttp loads at the same address. */
#define QNetIf_UpdateOnline func_8FC650
void QNetIf_UpdateOnline(void);
void FileQueue_Next(FileReq *r);
void FileQueue_Call(FileReq *r);

/* Per frame: the streams' fades, then the read queue. */
void Stream_Update(void)
{
    Stream_UpdateFades();
    FileQueue_Update();
}

extern s32 gNetSock;
extern u8 D_008EE27E;
/* Keeps the game ticking while a load waits: once per new VSync, the
 * frames advanced and pad 0's vibration stopped; online, the quest
 * network's step when gm_sub is loaded, and the lobby's receive when the
 * lobby is loaded and no model is loading; and sub_main's quest timers.
 * The lobby byte it tests has no writer in the code (seen: only this
 * read). */
void Load_Idle(void)
{
    s32 now = gVSyncCount;

    if (gLoadLastVSync != now) {
        Load_AdvanceFrames(now - gLoadLastVSync, 0);
        Vib_Stop(0);
        gLoadLastVSync = gVSyncCount;
        if (Game_IsOnline() == 1 && gSysSettings.gmSubLoaded != 0) {
            QNetIf_UpdateOnline();
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
 * Empties the background read queue: clears gFileQueue and marks every
 * request free.
 */
void FileQueue_Init(void)
{
    s32 i;

    memset(&gFileQueue, 0, sizeof(FileQueue));
    for (i = 0; i < FILE_QUEUE_LEN; i++) {
        gFileReqs[i].type = FILEREQ_FREE;
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
    queue->write = ++queue->write % FILE_QUEUE_LEN;
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
    gFileQueue.write = ++gFileQueue.write % FILE_QUEUE_LEN;
}

/* Per frame: starts the next request, or finishes the read under way
 * (a read that fails is retried). */
void FileQueue_Update(void)
{
    FileQueue *queue = &gFileQueue;
    FileReq *req = &gFileReqs[gFileQueue.read];
    s32 stat;

    switch (gFileQueue.state) {
    case FILEQ_IDLE:
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
    case FILEQ_READING:
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
        queue->state = FILEQ_IDLE;
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

    req->type = FILEREQ_FREE;
    gFileQueue.read = ++gFileQueue.read % FILE_QUEUE_LEN;
    if (gFileReqs[gFileQueue.read].type < 0) {
        queue->busy = 0;
    }
}

/* Drops the queue and the read under way, and the players' looks and
 * models waiting for it (in the lobby, the town's players'). */
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
        for (i = 0; i < LOBBY_TOWN_PLAYERS; i++) {
            gLobbyState.modelLoading[i] = 0;
            gLobbyState.lookPending[i] = 0;
        }
    }
}

/* Whether requests are pending. */
s32 FileQueue_IsBusy(void)
{
    return gFileQueue.busy;
}

/*
 * Whether lobby player playerIndex's look is still to be applied
 * (gLobbyState.lookPending, nonzero while pending).
 */
s32 Lobby_IsLookPending(s32 playerIndex)
{
    return (s8)gLobbyState.lookPending[playerIndex];
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
