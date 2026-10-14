/*
 * main 0x1930e0-0x1937a0: the per-frame stream update and the queue of
 * background reads (256 requests: AFS files, named files, or calls made
 * in order with the reads).
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"
#include "scene.h"
#include "lobby_state.h"
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
void func_36BDF0(s32 frames, s32);
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
        func_36BDF0(now - gLoadLastVSync, 0);
        Vib_Stop(0);
        gLoadLastVSync = gVSyncCount;
        if (Game_IsOnline() == 1 && gSysSettings.gmSubLoaded != 0) {
            func_8FC650();
        }
        if (Overlay_IsLoaded(4) == 1 && Game_IsOnline() == 1 && gScene.online != 0 && D_008EE27E != 0
            && gLobbyState.modelLoading == NULL) {
            Lobby_ReceiveAll(gNetSock);
        }
        if (Overlay_IsLoaded(1) == 1) {
            Quest_UpdateTimers(1);
        }
    }
}

void FileQueue_Init(void)
{
    s32 i;

    memset(&gFileQueue, 0, sizeof(FileQueue));
    for (i = 0; i < 256; i++) {
        gFileReqs[i].type = -1;
    }
}

/* id: ptid << 16 | flid. Waits for the queue to drain when it is full. */
void FileQueue_AddFile(u32 id, void *dst)
{
    FileReq *r = &gFileReqs[gFileQueue.write];
    FileQueue *q = &gFileQueue;

    if (r->type >= 0) {
        FileQueue_Flush();
        r = &gFileReqs[q->write];
    }
    r->type = FILEREQ_AFS;
    r->u.id = id;
    r->dst = dst;
    q->busy = 1;
    q->write = ++q->write % 256;
}

void FileQueue_AddCall(void (*func)(void), s32 nargs, s32 a0, s32 a1, s32 a2, s32 a3)
{
    FileReq *r = &gFileReqs[gFileQueue.write];

    r->type = FILEREQ_CALL;
    r->u.func = func;
    r->nargs = nargs;
    r->v.args[0] = a0;
    r->v.args[1] = a1;
    r->v.args[2] = a2;
    r->v.args[3] = a3;
    gFileQueue.busy = 1;
    gFileQueue.write = ++gFileQueue.write % 256;
}

/* Per frame: starts the next request, or finishes the read under way
 * (a read that fails is retried). */
void FileQueue_Update(void)
{
    FileQueue *q = &gFileQueue;
    FileReq *r = &gFileReqs[gFileQueue.read];
    s32 stat;

    switch (gFileQueue.state) {
    case 0:
        if (q->busy == 0) {
            break;
        }
        if (q->adxf != NULL) {
            ADXF_Stop(q->adxf);
            ADXF_Close(q->adxf);
        }
        if (r->type == FILEREQ_AFS) {
            q->adxf = ADXF_OpenAfs((r->u.id >> 16) & 0xFFFF, r->u.id & 0xFFFF);
            if (q->adxf == NULL) {
                break;
            }
            ADXF_ReadNw(q->adxf, ADXF_GetFsizeSct(q->adxf), r->dst);
            q->state++;
        } else if (r->type == FILEREQ_CALL) {
            FileQueue_Call(r);
            FileQueue_Next(r);
        } else if (r->type == FILEREQ_NAME) {
            q->adxf = ADXF_Open(r->v.name, NULL);
            if (q->adxf == NULL) {
                break;
            }
            ADXF_ReadNw(q->adxf, ADXF_GetFsizeSct(q->adxf), r->dst);
            q->state++;
        }
        break;
    case 1:
        stat = ADXF_GetStat(q->adxf);
        if (stat == ADXF_STAT_READING) {
            break;
        }
        ADXF_Close(q->adxf);
        q->adxf = NULL;
        if (stat == ADXF_STAT_ERROR) {
            break;
        }
        FileQueue_Next(r);
        q->state = 0;
        break;
    }
}

void FileQueue_Next(FileReq *r)
{
    FileQueue *q = &gFileQueue;

    r->type = -1;
    gFileQueue.read = ++gFileQueue.read % 256;
    if (gFileReqs[gFileQueue.read].type < 0) {
        q->busy = 0;
    }
}

/* Drops the queue and the read under way. */
void FileQueue_Cancel(void)
{
    FileQueue *q = &gFileQueue;
    s32 i;

    if (gFileQueue.adxf != NULL) {
        ADXF_Stop(q->adxf);
        ADXF_Close(q->adxf);
    }
    FileQueue_Init();
    PlayerLook_ResetLoaded();
    if (Overlay_IsLoaded(4) == 1) {
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

s8 func_00193670(s32 i)
{
    return gLobbyState.lookPending[i];
}

void FileQueue_Call(FileReq *r)
{
    void (*func)(void) = r->u.func;
    s32 a0 = r->v.args[0];
    s32 a1 = r->v.args[1];
    s32 a2 = r->v.args[2];
    s32 a3 = r->v.args[3];
    s32 n = r->nargs;

    if (n == 0) {
        func();
    } else if (n == 1) {
        ((void (*)(s32))func)(a0);
    } else if (n == 2) {
        ((void (*)(s32, s32))func)(a0, a1);
    } else if (n == 3) {
        ((void (*)(s32, s32, s32))func)(a0, a1, a2);
    } else if (n == 4) {
        ((void (*)(s32, s32, s32, s32))func)(a0, a1, a2, a3);
    }
}

void FileQueue_Flush(void)
{
    while (FileQueue_IsBusy()) {
        FileQueue_Update();
        Load_Idle();
    }
}
