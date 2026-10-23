/*
 * The DNAS check: Dnas_Step gets the disc ready, starts Sony's DNAS
 * library with the pass phrase and the authentication data (this file's
 * data), and polls the check to its end, for the login
 * (Dnas_AuthStep) or before a patch download (Dnas_StateMachine). With
 * it, NetXfer_Step and NetXfer_Step2, which pass a buffer through the
 * DNAS install library. Networking code stays byte-identical.
 */

#include "common.h"
#include "yn.h"                /* NetDevice, gNetDevs */
#include "net_tcp.h"
#include "dnas_check.h"
#include "net_connection.h"
#include "net_sockets.h"
#include "libcdvd.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

void strncpy(char *dst, const char *src, u32 n);
void * memset(void *p, s32 c, u32 n);
extern s32 gNetXferResult;
extern s32 gDnasResult;
extern s8 gNetDomain;
extern NetDevice *gNetDevCur;   /* the device in use (in gNetDevs) */
s32 atoi(s8 *s);             /* atoi */
u32 strlen(s8 *s);

/*
 * The result of the last DNAS install call made by NetXfer_Step or NetXfer_Step2
 * (gNetXferResult), negative on error.
 */
s32 NetXfer_GetResult(void)
{
    return gNetXferResult;
}

extern s32 gNetXferSize;
s32 func_AE2518(void);
s32 func_AE8638(s32 a, s32 b, s32 *out);
s32 func_AE8688(s32 a, s32 b);
void func_AE2570(void);

/*
 * One step of passing size bytes at buf through the DNAS install library (the DNAS_INS
 * overlay must be loaded). States: 0 start, 10 get the output size (failed if over
 * maxSize), 11 process, 15 close, 99 failed, 100 done. Returns the output size
 * (gNetXferSize) when done, -1 on failure (the cause in gNetXferResult), 0 while busy.
 * timer is not used.
 */
s32 NetXfer_Step(s8 *state, s16 *timer, u8 *buf, s32 size, s32 maxSize)
{
    switch (*state) {
    case 0:
        *state = 10;
        gNetXferSize = 0;
        gNetXferResult = func_AE2518();
        if (gNetXferResult < 0) {
            *state = 99;
        }
        break;
    case 10:
        (*state)++;
        gNetXferResult = func_AE8638(size, (s32) buf, &gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 99;
        } else if (gNetXferSize > maxSize) {
            *state = 99;
        }
        break;
    case 11:
        *state = 15;
        gNetXferResult = func_AE8688(size, gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 99;
        }
        break;
    case 15:
        *state = 100;
        func_AE2570();
        break;
    case 99:
        *state = 0;
        func_AE2570();
        return -1;
    case 100:
        *state = 0;
        return gNetXferSize;
    }
    return 0;
}

s32 func_AE2588(s32 a, s32 b, s32 *out);
s32 func_AE2EC8(s32 a, s32 b);

/*
 * NetXfer_Step with the library's other pair of calls (for the patch records). In state
 * 11 the game passes a local it never set as the first argument.
 */
s32 NetXfer_Step2(s8 *state, s16 *timer, u8 *buf, s32 size, s32 maxSize)
{
    volatile s32 handle;

    switch (*state) {
    case 0:
        *state = 10;
        gNetXferSize = 0;
        gNetXferResult = func_AE2518();
        if (gNetXferResult < 0) {
            *state = 99;
        }
        break;
    case 10:
        (*state)++;
        gNetXferResult = func_AE2588(size, (s32) buf, &gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 99;
        } else if (gNetXferSize > maxSize) {
            *state = 99;
        }
        break;
    case 11:
        *state = 15;
        /* the game reads a stack word it never set: h is the word at sp+0x3C, nothing here writes it */
        gNetXferResult = func_AE2EC8(handle, gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 99;
        }
        break;
    case 15:
        *state = 100;
        func_AE2570();
        break;
    case 99:
        *state = 0;
        func_AE2570();
        return -1;
    case 100:
        *state = 0;
        return gNetXferSize;
    }
    return 0;
}

/*
 * The result of the last DNAS call (gDnasResult): the error the lobby shows after
 * Dnas_Step fails; DNAS_RESULT_CANCELED when cancelled or the disc did not get ready.
 */
s32 Dnas_GetError(void)
{
    return gDnasResult;
}

/*
 * One step of the DNAS authentication at login (Dnas_Step's DNAS_KIND_AUTH, which also
 * restarts the sockets and clears the host cache). Returns 1 when done, -1 on failure,
 * else 0.
 */
s32 Dnas_AuthStep(s8 *state, s16 *cancel, s16 *percent)
{
    return Dnas_Step(state, cancel, percent, DNAS_KIND_AUTH);
}

/*
 * One step of the DNAS check before a patch download (Dnas_Step's DNAS_KIND_INST).
 * Returns 1 when done, -1 on failure, else 0.
 */
s32 Dnas_StateMachine(s8 *state, s16 *cancel, s16 *percent)
{
    return Dnas_Step(state, cancel, percent, DNAS_KIND_INST);
}

extern u16 gDnasWait;
extern char gDnasProxyHost[];
extern s32 gDnasFixedDns;
extern s32 gDnasStatusPrev[4];
extern s32 D_004054D0;
extern s32 D_004054E0;
extern s8 gProxyMode;
extern char D_005C3518[];
extern char D_005C3627[];
s32 memcmp(void *a, void *b, s32 n);
void *memcpy(void *dst, const void *src, u32 n);
s32 func_AC99D0(void *p, s32 a, s32 b, s32 c, s32 d);
s32 func_AC9A08(void);
s32 func_AC9D78(void);
s32 func_ACA070(void);
s32 func_ACA188(s32 *out);
s32 func_ACA1E8(s32 a, char *host, u16 port);
s32 func_ACA4E0(void);

typedef struct DnasParam {
    s32 unk0;
    s32 *passPhrase;
    s32 passPhraseSize;
    s32 *authData;
    s32 authDataSize;
} DnasParam;

extern u8 *gPadInputPtr;
/*
 * One step of the DNAS check (state *st). Gets the disc ready, starts the DNAS library
 * with the pass phrase and the authentication data, passes on the HTTP or HTTPS proxy,
 * starts the check of the kind (DNAS_KIND_*), then polls its status, raising *percent to
 * 100. *cancel at NET_CANCEL_AT or more aborts (the lobby raises it while Start is held).
 * With a modem and Select+Triangle+Square held on pad 1 at the start, the check uses two
 * built-in DNS servers (gDnasFixedDns) and the normal resolver comes back after.
 * Returns 1 when done, -1 on failure (the cause in gDnasResult), 0 while busy.
 */
s32 Dnas_Step(s8 *state, s16 *cancel, s16 *percent, s32 kind)
{
    DnasParam authInfo;
    s32 status[4];
    char *colon;

    switch (*state) {
    case 0:
        gDnasResult = 0;
        gDnasFixedDns = 0;
        if (gNetDevCur->type == NET_DEV_MODEM && *(s32 *)(gPadInputPtr + 8) == 17152) {
            gDnasFixedDns = 1;
        }
        if (gDnasFixedDns != 0) {
            NetDns_Term();
        }
        NetIf_SetErrorCheck(0);
        *state = 16;
        *percent = 0;
        gDnasWait = 0;
        break;
    case 15:
        sceCdDiskReady(0, 3);
        *state = 1;
        *percent = 3;
        gDnasWait = 0;
        break;
    case 16:
        gDnasWait++;
        *percent = 1;
        if (sceCdStandby(1, 3) == 1 || gDnasWait > 300) {
            *state = 17;
            gDnasWait = 0;
            break;
        }
        if (*cancel >= NET_CANCEL_AT) {
            gDnasResult = DNAS_RESULT_CANCELED;
            *state = 0;
            NetIf_SetErrorCheck(1);
            if (gDnasFixedDns != 0) {
                *state = 98;
                break;
            }
            return -1;
        }
        break;
    case 17:
        gDnasWait++;
        *percent = 2;
        if (sceCdSync(1, 3) == 0 || gDnasWait > 300) {
            *state = 18;
            gDnasWait = 0;
        }
        break;
    case 18:
        gDnasWait++;
        *percent = 3;
        if (sceCdDiskReady(1, 3) == 2 || gDnasWait > 300) {
            *state = 1;
            gDnasWait = 0;
            break;
        }
        if (*cancel >= NET_CANCEL_AT) {
            gDnasResult = DNAS_RESULT_CANCELED;
            *state = 0;
            NetIf_SetErrorCheck(1);
            if (gDnasFixedDns != 0) {
                *state = 98;
                break;
            }
            return -1;
        }
        break;
    case 1:
        if (gDnasFixedDns != 0) {
            NetDns_Init((char *)&gNetDomain, 0x79F3D5CA, 0x59F3D5CA);
        }
        if (kind == DNAS_KIND_AUTH) {
            Net_InitSockets();
            net_clear_host_cache();
        }
        (*state)++;
        *percent = 5;
        break;
    case 2:
        memset(gDnasStatusPrev, 0, 16);
        memset(&authInfo, 0, 20);
        authInfo.unk0 = 1;
        authInfo.authData = &D_004054E0;
        authInfo.authDataSize = 0x10000;
        authInfo.passPhrase = &D_004054D0;
        authInfo.passPhraseSize = 8;
        gDnasResult = func_AC99D0(&authInfo, 48, 0, 0, 0);
        if (gDnasResult < 0) {
            *state = 99;
        } else {
            (*state)++;
            *percent = 10;
        }
        break;
    case 3:
        if (gProxyMode != 0) {
            gDnasProxyHost[271] = 0;
            if ((s8)(s32)gProxyMode & 2) {
                strncpy(gDnasProxyHost, D_005C3518, 271);
            } else {
                strncpy(gDnasProxyHost, D_005C3627, 271);
            }
            colon = gDnasProxyHost + strlen((s8 *)gDnasProxyHost) - 1;
            do {
                if (*colon == ':') {
                    break;
                }
                colon--;
            } while (gDnasProxyHost < colon);
            if (gDnasProxyHost < colon) {
                *colon = 0;
                func_ACA1E8(1, gDnasProxyHost, atoi((s8 *)colon + 1));
            }
        }
        if (gDnasResult < 0) {
            *state = 99;
        } else {
            (*state)++;
            *percent = 20;
        }
        break;
    case 4:
        switch (kind) {
        case DNAS_KIND_AUTH:
            gDnasResult = func_ACA4E0();
            break;
        case DNAS_KIND_INST:
            gDnasResult = func_AC9A08();
            break;
        default:
            while (1) {
            }
        }
        if (gDnasResult < 0) {
            *state = 99;
        } else {
            *state = 10;
            *percent = 30;
        }
        break;
    case 10:
        memset(status, 0, 16);
        gDnasResult = func_ACA188(status);
        if (gDnasResult < 0) {
            *state = 99;
        } else if (memcmp(gDnasStatusPrev, status, 16) != 0) {
            if (*percent < 80) {
                *percent += 10;
            }
            if (status[0] == 5) {
                memcpy(gDnasStatusPrev, status, 16);
                *state = 100;
            } else if (status[0] == 6 || status[0] == 7) {
                gDnasResult = status[1];
                *state = 99;
            } else {
                goto same;
            }
        } else {
        same:
            memcpy(gDnasStatusPrev, status, 16);
            if (*cancel >= NET_CANCEL_AT) {
                gDnasResult = func_ACA070();
                *state = 90;
            }
        }
        break;
    case 90:
        memset(status, 0, 16);
        gDnasResult = func_ACA188(status);
        if (status[0] == 2) {
            gDnasResult = DNAS_RESULT_CANCELED;
            *state = 99;
        }
        break;
    case 99:
        *state = 0;
        func_AC9D78();
        NetIf_SetErrorCheck(1);
        if (gDnasFixedDns != 0) {
            NetDns_Term();
            *state = 98;
            break;
        }
        return -1;
    case 98:
        *state = 0;
        Net_InitDns();
        return -1;
    case 100:
        *state = 127;
        gDnasResult = func_AC9D78();
        NetIf_SetErrorCheck(1);
        if (gDnasFixedDns != 0) {
            NetDns_Term();
            *state = 101;
        }
        if (*percent < 90) {
            *percent = 90;
        }
        break;
    case 101:
        *state = 127;
        Net_InitDns();
        break;
    case 127:
        *state = 0;
        if (*percent < 100) {
            *percent = 100;
        }
        return 1;
    }
    return 0;
}

/* This file's initialised data. */
INCLUDE_DATA("build/blobs/sub_main", D_004054D0);
INCLUDE_DATA("build/blobs/sub_main", D_004054E0);
