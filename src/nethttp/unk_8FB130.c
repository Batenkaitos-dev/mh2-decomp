/*
 * nethttp.bin's Capcom front end (0x8FB130-0x8FD788): the game's HTTP
 * requests over Sony's libhttps (the rest of the overlay, which stays
 * assembly). NETWORK: this code must stay byte-for-byte for online play.
 *
 * Up to 8 requests (HttpReq, gHttpReq) run as state machines: each frame
 * HttpReq_Step calls gHttpStateFn[req->state]. A request's URL picks the
 * source: "file://" reads the disc, "memory://N" a page the browser holds,
 * "http://" and "https://" go through libhttps, "lbs://" the lobby
 * server's protocol and "afs://" the game's AFS archive (browser.h's URL_*).
 *
 * nethttp is loaded right after the lobby overlay and calls it (the
 * browser's heap, its strings): the lobby's globals are reached by address
 * here (the lobby's own C names them D_ or by struct).
 */
#include "common.h"
#include "browser.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "nethttp.h"

/* The lobby's globals this file uses (lobby.txt: nethttp links the lobby's names) */
#define LOBBY_BROWSER_WORK gBrowserWork
extern s32 gHttpActive; /* main's .bss: libhttps is up (main's code never reads it) */
#define LOBBY gLobby
#define LOBBY_NET_HTTP (*(s8 *)&gLobbyNet.reqState)
#define LOBBY_NET_KEY gLobbyNet.key
#define LOBBY_NET_RECV_LEFT (*(u32 *)&gLobbyNet.recvResult)
extern u8 gLobbyRecvHdr[];
#define LOBBY_RECV_HDR gLobbyRecvHdr
extern u8 gLobbyRecvData[];                            /* the lobby's: the reply's data */

void HttpReq_Open(HttpReq *req);
void HttpReq_ParseUrl(HttpReq *req);
void HttpReq_CreateClient(HttpReq *req);
void HttpReq_SetOptions(HttpReq *req);
void HttpReq_Connect(HttpReq *req);
void HttpReq_Send(HttpReq *req);
void HttpReq_ReadBody(HttpReq *req);
void HttpReq_CheckStatus(HttpReq *req);
void HttpReq_Authenticate(HttpReq *req);
void HttpReq_Close(HttpReq *req);
void HttpReq_Done(HttpReq *req);
void HttpReq_Abort(HttpReq *req);
void HttpReq_Failed(HttpReq *req);
void HttpLbs_Send(HttpReq *req);
void HttpLbs_Wait(HttpReq *req);
void HttpLbs_Done(HttpReq *req);
void HttpLbs_Abort(HttpReq *req);
void HttpLbs_Failed(HttpReq *req);
void HttpAfs_Read(HttpReq *req);
void HttpAfs_Next(HttpReq *req);
void HttpAfs_Done(HttpReq *req);

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void *memmove(void *dst, const void *src, u32 n);
u32 strlen(const void *s);
s32 atoi(const char *s);
void Net_InitSockets(void);
void net_clear_host_cache(void);
s32 NetIf_SetErrorCheck(s32 on);
s32 NetIf_IsConnected(void);
char *strncpy(char *dst, const char *src, u32 n);
s32 Path_ToBackslashes(char *path);
s32 File_GetSize(const char *name);
s32 File_Read(const char *name, void *buf, s32 size);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Url_GetScheme(const char *url);
void Str_EucToSjis(u8 *s);
s32 Brdata_Read(void *buf, s32 flid);
void Brdata_LoadPartition(void *ptinfo);
void Browser_InitHeap(void);
void *Browser_Alloc(u32 size);
void *Browser_Realloc(void *p, u32 size);
void Browser_Free(void *p);

/* libhttps (assembly) */
void func_00900660(void);
s32 func_009005F8(void);
void func_00908A88(void *(*alloc)(u32 size));
void func_00908AB0(void *(*realloc)(void *p, u32 size));
void func_00908AD8(void (*free)(void *p));
s32 func_00908EF0(s32 a0, s32 n, void *certs, s32 a3, s32 t0);
void func_00909738(void);
s32 func_00902D28(s32 h);
s32 func_00900550(s32 h);
s32 func_00905E38(HttpAuthReq *h);
s32 func_00906668(s32 h);
void func_00900FC8(s32 h);
void func_00900AA8(void *h);
void func_008FDA70(s32 h);
HttpResp *func_00900548(s32 h);
s32 func_00900378(s32 h);
s64 func_009005A8(s32 h);
s64 func_009005F0(s32 h);
s32 func_00902BD0(s32 h, s32 a1, s32 a2, s32 a3);
void func_00902280(s32 h);
HttpUrl *func_009006F8(char *url, s32 a1);
s32 func_008FD8F0(void);
HttpAuthReq *func_00905ED0(HttpResp *resp);
s32 func_008FDA50(s32 h);
s32 func_00901600(s32 h, s32 opt, ...);
s32 func_00900EE8(s32 a0, const char *name, const char *value);
s32 func_00904868(s32 h, char *user, char *pass, s32 proxy);
s32 func_00906070(s32 h, HttpDigest *d, s32 proxy);
s32 rand(void);
s32 sprintf(char *buf, const char *fmt, ...);
char **func_009066E0(HttpResp *resp);
void func_009067D0(char **p);
s32 func_00906400(HttpResp *resp);
s32 func_00906518(s32 h, s32 a1, HttpDigest *d, s32 a3);
void Url_Copy(char *dst, const char *src);
void Url_ClearBase(void);
char *Url_Resolve(char *base, char *url);

s32 Http_ReadBrdata(void *buf, s32 flid);
void HttpReq_Step(HttpReq *req);
s32 HttpReq_CheckSjis(HttpReq *req, u8 *s, s32 n);

/* The file's .data (0x94FF98-0x950008). */
/* how many of gHttpReq can be used (volatile: the loops over gHttpReq reload it
 * every iteration) */
volatile u8 gHttpReqMax = {
#include "nethttp/0094FF98.inc"
};
s32 D_0094FFA0 = {
#include "nethttp/0094FFA0.inc"
};            /* libhttps option 0xE for https:// (HttpReq_SetOptions) */
s32 gHttpCertSize[2] = {
#include "nethttp/0094FFA8.inc"
};  /* the sizes of the two CA certificates */
/* the request states' handlers, by HttpReq.state (HttpReq_Step) */
HttpStateFn gHttpStateFn[] = {
    HttpReq_Open,
    HttpReq_ParseUrl,
    HttpReq_CreateClient,
    HttpReq_SetOptions,
    HttpReq_Connect,
    HttpReq_Send,
    HttpReq_ReadBody,
    HttpReq_CheckStatus,
    HttpReq_Authenticate,
    HttpReq_Close,
    HttpReq_Done,
    HttpReq_Abort,
    HttpReq_Failed,
    HttpLbs_Send,
    HttpLbs_Wait,
    HttpLbs_Done,
    HttpLbs_Abort,
    HttpLbs_Failed,
    HttpAfs_Read,
    HttpAfs_Next,
    HttpAfs_Done,
};

/* .bss (0x95AA00), from the highest address down */
HttpReq gHttpReq[8];
HttpReq *gHttpLbsReq;

s32 Http_GetCertSize(s32 i) {
    return gHttpCertSize[i];
}

s32 Http_Init(void) {
    s32 certs[4];

    memset(gHttpReq, 0, sizeof(gHttpReq));
    Net_InitSockets();
    net_clear_host_cache();
    Browser_InitHeap();
    func_00908A88(Browser_Alloc);
    func_00908AB0(Browser_Realloc);
    func_00908AD8(Browser_Free);
    if (func_009005F8() < 0) {
        return -1;
    }
    certs[0] = (s32)LOBBY_BROWSER_WORK->cert0;
    certs[1] = Http_GetCertSize(0);
    certs[2] = (s32)LOBBY_BROWSER_WORK->cert1;
    certs[3] = Http_GetCertSize(1);
    if (func_00908EF0(0, 2, certs, 0, 0) < 0) {
        return -1;
    }
    gHttpActive = 1;
    return 0;
}

s32 Http_Term(void) {
    func_00909738();
    func_00900660();
    gHttpActive = 0;
    return 0;
}

HttpReq *HttpReq_Alloc(void) {
    s32 i;
    HttpReq *req;

    for (i = 0, req = gHttpReq; i < gHttpReqMax; i++, req++) {
        if (req->used == 0) {
            memset(req, 0, sizeof(HttpReq));
            req->used = 1;
            req->run = HttpReq_Step;
            req->index = i;
            return req;
        }
    }
    return NULL;
}

s32 HttpReq_HasFree(void) {
    s32 i;
    HttpReq *req;

    req = gHttpReq;
    for (i = 0; i < gHttpReqMax; i++, req++) {
        if (req->used == 0) {
            return 1;
        }
    }
    return 0;
}

s32 HttpReq_AllIdle(void) {
    s32 i;
    HttpReq *req;

    for (i = 0, req = gHttpReq; i < 8; i++, req++) {
        if (!req->used == 0) {
            return 0;
        }
    }
    return 1;
}

s32 HttpReq_Free(HttpReq *req) {
    if (req->used != 0) {
        memset(req, 0, sizeof(HttpReq));
    }
    return 0;
}

void HttpReq_Step(HttpReq *req) {
    gHttpStateFn[req->state](req);
}

void HttpReq_AsyncCb17(void *ctx, u8 c, u8 *out) {
    *out = c;
}

void HttpReq_AsyncCb0A(void *ctx, u8 c, u8 *out) {
    *out = c;
}

s32 HttpReq_CheckSjis(HttpReq *req, u8 *s, s32 n) {
    for (; n > 0; n--, s++) {
        if ((*s >= 0x81 && *s <= 0x8D) || (*s >= 0x8F && *s <= 0xA0)) {
            req->sjis = 1;
            return 1;
        }
    }
    req->sjis = 0;
    return 0;
}

void HttpReq_Open(HttpReq *req) {
    char path[0x100];
    s32 n;
    s32 i;

    if (req->cancel != 0) {
        req->err = 0x11;
        req->errStep = 0;
        req->state = 0xB;
        return;
    }
    switch ((u8)Url_GetScheme(req->url)) {
    case URL_FILE:
        strncpy(path, req->url + 6, 0xFF);
        path[0xFF] = 0;
        Path_ToBackslashes(path);
        n = File_GetSize(path);
        if (n == 0) {
            req->err = 1;
            req->errStep = 0;
            req->state = 0xB;
            return;
        }
        if (req->bufSize <= n) {
            n = req->bufSize - 1;
        }
        File_Read(path, req->buf, n);
        req->buf[n] = 0;
        req->len = n;
        if (req->toSjis != 0) {
            if (HttpReq_CheckSjis(req, (u8 *)req->buf, n) == 0) {
                Str_EucToSjis((u8 *)req->buf);
            }
            req->len = strlen(req->buf);
        }
        req->state = 0xA;
        return;
    case URL_AFS:
        req->state = 0x12;
        req->step = 0;
        return;
    case URL_MEMORY:
        i = atoi(req->url + 9);
        if (i < 0 || i >= 3) {
            req->err = 1;
            req->errStep = 0;
            req->state = 0xB;
            return;
        }
        n = strlen((char *)LOBBY_BROWSER_WORK->memPage[i]);
        if (n == 0) {
            req->err = 1;
            req->errStep = 0;
            req->state = 0xB;
            return;
        }
        if (req->bufSize <= n) {
            n = req->bufSize - 1;
        }
        memcpy(req->buf, LOBBY_BROWSER_WORK->memPage[i], n);
        req->buf[n] = 0;
        req->len = n;
        if (req->toSjis != 0) {
            if (HttpReq_CheckSjis(req, (u8 *)req->buf, n) == 0) {
                Str_EucToSjis((u8 *)req->buf);
            }
            req->len = strlen(req->buf);
        }
        req->state = 0xA;
        return;
    case URL_LBS:
        if (NetIf_IsConnected() == 0) {
            req->err = 0xF;
            req->errStep = 0;
            req->state = 0xB;
            return;
        }
        req->state = 0xD;
        req->step = 0;
        return;
    case URL_HTTP:
        if (NetIf_IsConnected() == 0) {
            req->err = 0xF;
            req->errStep = 0;
            req->state = 0xB;
            return;
        }
        req->secure = 0;
        req->state++;
        req->step = 0;
        return;
    case URL_HTTPS:
        if (NetIf_IsConnected() == 0) {
            req->err = 0xF;
            req->errStep = 0;
            req->state = 0xB;
            return;
        }
        req->secure = 1;
        req->state++;
        req->step = 0;
        return;
    default:
        req->err = 1;
        req->errStep = 0;
        req->state = 0xB;
        return;
    }
}

void HttpReq_ParseUrl(HttpReq *req) {
    switch (req->step) {
    case 0:
        req->step = 5;
        NetIf_SetErrorCheck(0);
        break;
    case 5:
        if (req->cancel != 0) {
            req->err = 0x11;
            req->errStep = 0;
            req->state = 0xB;
            break;
        }
        req->uri = func_009006F8(req->url, 0x60);
        if (req->uri == 0) {
            req->err = 1;
            req->errStep = 0;
            req->state = 0xB;
            break;
        }
        if (req->proxyFlags) {
            if (req->proxyFlags & 1) {
                req->proxyUri = func_009006F8(req->proxyUrl, 0);
                if (req->proxyUri == 0) {
                    req->err = 1;
                    req->errStep = 1;
                    req->state = 0xB;
                    break;
                }
            }
            if (req->proxyFlags & 2) {
                req->sslProxyUri = func_009006F8(req->sslProxyUrl, 0);
                if (req->sslProxyUri == 0) {
                    req->err = 1;
                    req->errStep = 1;
                    req->state = 0xB;
                    break;
                }
            }
        }
        req->state++;
        req->step = 0;
        break;
    }
}

void HttpReq_CreateClient(HttpReq *req) {
    if (req->cancel != 0) {
        req->err = 0x11;
        req->errStep = 1;
        req->state = 0xB;
        return;
    }
    if ((req->client = func_008FD8F0()) == 0) {
        req->err = 2;
        req->errStep = 1;
        req->state = 0xB;
        return;
    }
    req->state++;
    req->step = 0;
}

void HttpReq_SetOptions(HttpReq *req) {
    s32 method;

    if (req->cancel != 0) {
        req->err = 0x11;
        req->errStep = 2;
        req->state = 0xB;
        return;
    }
    switch (req->method) {
    case 1:
        method = 1;
        break;
    case 2:
        method = 3;
        break;
    default:
        req->err = 3;
        req->errStep = 2;
        req->state = 0xB;
        return;
    }
    switch (req->step) {
    case 0:
        if (func_00901600(req->client, 3, req->uri) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 2, method) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (method == 3) {
            if (func_00901600(req->client, 0x13, "Content-Length") < 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
            if ((req->hdr[2] = func_00900EE8(0, "Content-Type", "application/x-www-form-urlencoded")) == 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
            if (func_00901600(req->client, 5, req->hdr[2], 2) < 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
            if (strlen(req->postData) == 0) {
                if ((req->hdr[3] = func_00900EE8(0, "Content-Length", "0")) == 0) {
                    req->err = 3;
                    req->errStep = 2;
                    req->state = 0xB;
                    return;
                }
                if (func_00901600(req->client, 5, req->hdr[3], 2) < 0) {
                    req->err = 3;
                    req->errStep = 2;
                    req->state = 0xB;
                    return;
                }
            } else {
                if (func_00901600(req->client, 6, req->postData, strlen(req->postData), 0) < 0) {
                    req->err = 3;
                    req->errStep = 2;
                    req->state = 0xB;
                    return;
                }
            }
        }
        if (func_00901600(req->client, 0, "KDDI/Multi-Matching-BB_Browser") < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 0x15, 60) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 7, 60) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 8, 60) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 9, 0) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 0xA, HttpReq_AsyncCb0A, &req->async) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 0x16, 0) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (func_00901600(req->client, 0x17, HttpReq_AsyncCb17, &req->async) < 0) {
            req->err = 3;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        if (req->secure != 0) {
            if (req->proxyFlags & 2) {
                if (func_00901600(req->client, 4, req->sslProxyUri) < 0) {
                    req->err = 3;
                    req->errStep = 2;
                    req->state = 0xB;
                    return;
                }
            } else if (req->proxyFlags & 1) {
                if (func_00901600(req->client, 4, req->proxyUri) < 0) {
                    req->err = 3;
                    req->errStep = 2;
                    req->state = 0xB;
                    return;
                }
            }
            /* the cast puts the global's load before req->client's, as in the game */
        if (func_00901600(req->client, 0xE, (u32)D_0094FFA0) < 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        } else if (req->proxyFlags & 1) {
            if (func_00901600(req->client, 4, req->proxyUri) < 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        }
        req->auth = NULL;
        req->step++;
        break;
    case 1:
        if (req->referer != NULL) {
            if ((req->refererHdr = func_00900EE8(0, "Referer", req->referer)) == 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
            if (func_00901600(req->client, 5, req->refererHdr, 2) < 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        }
        if (req->noCache != 0) {
            if ((req->hdr[0] = func_00900EE8(0, "Pragma", "no-cache")) == 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
            if (func_00901600(req->client, 5, req->hdr[0], 2) < 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
            if ((req->hdr[1] = func_00900EE8(0, "Cache-Control", "no-cache")) == 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
            if (func_00901600(req->client, 5, req->hdr[1], 2) < 0) {
                req->err = 3;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        }
        req->state++;
        req->step = 0;
        break;
    }
}

void HttpReq_Connect(HttpReq *req) {
    switch (req->step) {
    case 0:
        if (req->cancel != 0) {
            req->err = 0x11;
            req->errStep = 2;
            req->state = 0xB;
            break;
        }
        req->step++;
        req->timer = 0xE10;
        req->async = 1;
        if (func_00902BD0(req->client, 2, 1, 3) < 0) {
            req->err = 4;
            req->errStep = 3;
            req->state = 0xB;
        }
        break;
    case 1:
        if (req->timer-- < 0) {
            func_00902280(req->client);
            req->step = 0xB;
            req->timer = 0;
            break;
        }
        switch (req->async) {
        case 0:
            req->step++;
            req->timer = 0;
            break;
        case 1:
            break;
        case -1:
            if (req->cancel != 0) {
                req->step = 0xA;
                req->timer = 0;
                break;
            }
            req->step = 0;
            req->timer = 0;
            req->err = 4;
            req->errStep = 3;
            req->state = 0xB;
            break;
        }
        break;
    case 2:
        req->step++;
        req->timer = 0;
        break;
    case 3:
        req->state++;
        req->step = 0;
        req->timer = 0;
        req->unk4A = 0;
        break;
    case 10:
        switch (req->async) {
        case 1:
            break;
        case 0:
            req->err = 0x11;
            req->errStep = 3;
            req->state = 0xB;
            break;
        case -1:
            req->err = 0x11;
            req->errStep = 3;
            req->state = 0xB;
            break;
        }
        break;
    case 11:
        switch (req->async) {
        case 1:
            break;
        case 0:
            req->err = 6;
            req->errStep = 3;
            req->state = 0xB;
            break;
        case -1:
            req->err = 6;
            req->errStep = 3;
            req->state = 0xB;
            return;
        }
        break;
    }
}

void HttpReq_Send(HttpReq *req) {
    s64 n;

    switch (req->step) {
    case 0:
        if (req->cancel != 0) {
            req->err = 0x11;
            req->errStep = 3;
            req->state = 0xB;
            break;
        }
        req->step++;
        req->timer = 0xE10;
        req->async = 1;
        if (func_00900378(req->client) < 0) {
            req->err = 5;
            req->errStep = 3;
            req->state = 0xB;
        }
        break;
    case 1:
        if (req->cancel != 0) {
            func_00902280(req->client);
            req->step = 0xA;
            req->timer = 0;
            break;
        }
        n = func_009005A8(req->client);
        if (req->recvLen != n) {
            req->timer = 0x708;
        }
        req->recvLen = n;
        n = func_009005F0(req->client);
        if (n <= 0) {
            req->contentLen = 0;
        } else {
            req->contentLen = n;
        }
        if (req->timer-- < 0) {
            func_00902280(req->client);
            req->step = 0xB;
            req->timer = 0;
            break;
        }
        switch (req->async) {
        case 0:
            req->step++;
            req->timer = 0;
            break;
        case 1:
            break;
        case -1:
            req->step = 0;
            req->timer = 0;
            req->err = 5;
            req->errStep = 3;
            req->state = 0xB;
            break;
        }
        break;
    case 2:
        req->step++;
        req->timer = 0;
        break;
    case 3:
        req->state++;
        req->step = 0;
        req->timer = 0;
        break;
    case 10:
        switch (req->async) {
        case 1:
            break;
        case 0:
            req->err = 0x11;
            req->errStep = 3;
            req->state = 0xB;
            break;
        case -1:
            req->err = 0x11;
            req->errStep = 3;
            req->state = 0xB;
            break;
        }
        break;
    case 11:
        switch (req->async) {
        case 1:
            break;
        case 0:
            req->err = 6;
            req->errStep = 3;
            req->state = 0xB;
            break;
        case -1:
            req->err = 6;
            req->errStep = 3;
            req->state = 0xB;
            return;
        }
        break;
    }
}

void HttpReq_ReadBody(HttpReq *req) {
    s32 n;

    if (req->cancel != 0) {
        req->err = 0x11;
        req->errStep = 3;
        req->state = 0xB;
        return;
    }
    if ((req->resp = func_00900548(req->client)) == NULL) {
        req->err = 7;
        req->errStep = 3;
        req->state = 0xB;
        return;
    }
    if (req->resp->body != NULL) {
        n = req->resp->bodyLen;
        if (req->bufSize <= n) {
            n = req->bufSize - 1;
        }
        memcpy(req->buf, req->resp->body, n);
        req->buf[n] = 0;
        req->len = n;
        if (req->toSjis != 0) {
            if (HttpReq_CheckSjis(req, (u8 *)req->buf, n) == 0) {
                Str_EucToSjis((u8 *)req->buf);
            }
            req->len = strlen(req->buf);
        }
    } else {
        req->buf[0] = 0;
        req->len = 0;
    }
    req->state++;
}

void HttpReq_CheckStatus(HttpReq *req) {
    char **loc;
    s32 ret;

    if (req->cancel != 0) {
        req->err = 0x11;
        req->errStep = 3;
        req->state = 0xB;
        return;
    }
    if (func_00902D28(req->client) < 0) {
        req->err = 8;
        req->errStep = 2;
        req->state = 0xB;
        return;
    }
    req->httpErr = 0;
    switch ((u16)req->resp->status) {
    case 300:
    case 301:
    case 302:
    case 303:
    case 307:
    case 305:
        loc = func_009066E0(req->resp);
        if (loc == NULL) {
            req->err = 7;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        Url_Copy(req->buf, *loc);
        Url_ClearBase();
        Url_Resolve(req->url, req->buf);
        Url_ClearBase();
        func_009067D0(loc);
        req->err = 0x12;
        req->errStep = 2;
        req->state = 0xB;
        return;
    case 407:
        if (req->proxyUser == NULL || req->proxyPass == NULL) {
            req->err = 0x14;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        req->state++;
        break;
    case 401:
        if (req->user == NULL || req->pass == NULL) {
            req->err = 0x13;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        req->state++;
        break;
    case 400:
    case 402:
    case 403:
    case 404:
    case 405:
    case 406:
    case 408:
    case 409:
    case 410:
    case 411:
    case 412:
    case 413:
    case 414:
    case 415:
    case 416:
    case 417:
    case 422:
    case 423:
    case 424:
    case 500:
    case 501:
    case 502:
    case 503:
    case 504:
    case 505:
    case 507:
        req->httpErr = 1;
        req->state += 2;
        break;
    default:
        req->state += 2;
        break;
    }
    if (req->authInfo != 0) {
        if (func_00906668(req->authInfo) < 0) {
            req->authInfo = 0;
            req->err = 0xD;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        req->authInfo = 0;
    }
    req->authInfo = func_00906400(req->resp);
    if (req->authInfo != 0) {
        switch (func_00906518(req->client, req->authInfo, &req->digest, 0)) {
        case 1:
        case 0:
            break;
        case -1:
        default:
            req->err = 0xC;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
    }
    if (req->auth != NULL) {
        if (func_00905E38(req->auth) < 0) {
            req->auth = NULL;
            req->err = 0xD;
            req->errStep = 2;
            req->state = 0xB;
            return;
        }
        req->auth = NULL;
    }
}

void HttpReq_Authenticate(HttpReq *req) {
    s32 qop;
    s32 mode;
    s32 t;
    s16 status;

    status = req->resp->status;
    if (req->cancel != 0) {
        req->err = 0x11;
        req->errStep = 2;
        req->state = 0xB;
        return;
    }
    req->auth = func_00905ED0(req->resp);
    if (req->auth == NULL) {
        req->err = 9;
        req->errStep = 2;
        req->state = 0xB;
        return;
    }
    if (func_008FDA50(req->client) < 0) {
        req->err = 0xA;
        req->errStep = 2;
        req->state = 0xB;
        return;
    }
    if (func_00901600(req->client, 5, 0, 1) < 0) {
        req->err = 0xA;
        req->errStep = 2;
        req->state = 0xB;
        return;
    }
    switch (req->method) {
    case 1:
        mode = 1;
        break;
    case 2:
        mode = 3;
        break;
    }
    if (req->auth->digest == 0) {
        if ((u16)status == 407) {
            if (func_00904868(req->client, req->proxyUser, req->proxyPass, 1) < 0) {
                req->err = 0xB;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        } else {
            if (func_00904868(req->client, req->user, req->pass, 0) < 0) {
                req->err = 0xB;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        }
    } else {
        if ((u16)status == 407) {
            req->digest.user = req->proxyUser;
            req->digest.pass = req->proxyPass;
        } else {
            req->digest.user = req->user;
            req->digest.pass = req->pass;
        }
        req->digest.realm = req->auth->realm;
        req->digest.uri = req->uri->path;
        req->digest.nonce = req->auth->nonce;
        req->digest.opaque = req->auth->opaque;
        req->digest.algorithm = req->auth->algorithm;
        req->digest.nc = 1;
        qop = req->auth->qop;
        if (qop & 1) {
            req->digest.qop = 1;
        } else {
            req->digest.qop = qop & 2;
        }
        if (req->digest.qop != 0) {
            t = req->resp->unk30;
            sprintf(req->cnonce, "%08x", t + rand());
            req->digest.cnonce = req->cnonce;
        } else {
            req->digest.cnonce = NULL;
        }
        req->digest.unk28 = mode;
        req->digest.unk2C = 0;
        req->digest.unk30 = 0;
        if ((u16)status == 407) {
            if (func_00906070(req->client, &req->digest, 1) < 0) {
                req->err = 0xB;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        } else {
            if (func_00906070(req->client, &req->digest, 0) < 0) {
                req->err = 0xB;
                req->errStep = 2;
                req->state = 0xB;
                return;
            }
        }
    }
    req->state = 3;
    req->step = 1;
    req->proxyUser = NULL;
    req->proxyPass = NULL;
    req->user = NULL;
    req->pass = NULL;
}

void HttpReq_Close(HttpReq *req) {
    if (req->auth != 0) {
        func_00905E38(req->auth);
    }
    if (req->authInfo != 0) {
        func_00906668(req->authInfo);
    }
    req->auth = 0;
    req->authInfo = 0;
    if (req->hdr[0] != 0) {
        func_00900FC8(req->hdr[0]);
    }
    if (req->hdr[1] != 0) {
        func_00900FC8(req->hdr[1]);
    }
    if (req->hdr[2] != 0) {
        func_00900FC8(req->hdr[2]);
    }
    if (req->hdr[3] != 0) {
        func_00900FC8(req->hdr[3]);
    }
    if (req->refererHdr != 0) {
        func_00900FC8(req->refererHdr);
    }
    req->refererHdr = 0;
    func_008FDA70(req->client);
    req->client = 0;
    if (req->sslProxyUri != 0) {
        func_00900AA8(req->sslProxyUri);
    }
    if (req->proxyUri != 0) {
        func_00900AA8(req->proxyUri);
    }
    if (req->uri != 0) {
        func_00900AA8(req->uri);
    }
    req->sslProxyUri = 0;
    req->proxyUri = 0;
    req->uri = 0;
    req->state++;
}

void HttpReq_Done(HttpReq *req) {
    req->used = 2;
    NetIf_SetErrorCheck(1);
}

void HttpReq_Abort(HttpReq *req) {
    s32 code = 0;

    switch (req->errStep) {
    case 4:
        func_00902280(req->client);
    case 3:
        func_00902D28(req->client);
    case 2:
        if (req->auth != 0) {
            func_00905E38(req->auth);
        }
        if (req->authInfo != 0) {
            func_00906668(req->authInfo);
        }
        req->auth = 0;
        req->authInfo = 0;
        switch (func_00900550(req->client)) {
        case -0x3F6:
        case -0x3F2:
        case -0x3EC:
        case -0x3EA:
        case -0x3E9:
            code = 1;
            break;
        case -0x3F5:
        case -0x3F0:
        case -0x3EF:
        case -0x3EB:
            code = 2;
            break;
        case -0x3ED:
            code = 3;
            break;
        case -0x3EE:
            code = 4;
            break;
        case -0x3F1:
            code = 5;
            break;
        case -0x3F3:
            code = 6;
            break;
        case -0x3F4:
            code = 7;
            break;
        case -0x3F7:
            code = 8;
            break;
        default:
            code = 1;
            break;
        }
        if (req->hdr[0] != 0) {
            func_00900FC8(req->hdr[0]);
        }
        if (req->hdr[1] != 0) {
            func_00900FC8(req->hdr[1]);
        }
        if (req->hdr[2] != 0) {
            func_00900FC8(req->hdr[2]);
        }
        if (req->hdr[3] != 0) {
            func_00900FC8(req->hdr[3]);
        }
        if (req->refererHdr != 0) {
            func_00900FC8(req->refererHdr);
        }
        req->refererHdr = 0;
        func_008FDA70(req->client);
    case 1:
        if (req->sslProxyUri != 0) {
            func_00900AA8(req->sslProxyUri);
        }
        if (req->proxyUri != 0) {
            func_00900AA8(req->proxyUri);
        }
        if (req->uri != 0) {
            func_00900AA8(req->uri);
        }
        req->sslProxyUri = 0;
        req->proxyUri = 0;
        req->uri = 0;
    case 0:
        break;
    }
    req->errStep = (u8)code;
    req->state = 0xC;
}

void HttpReq_Failed(HttpReq *req) {
    req->used = 2;
    NetIf_SetErrorCheck(1);
}

/* The lbs:// request (the lobby protocol's 0x101 or 0x102). It matched once
 * Net_LockrockObfuscate's key was declared s32, as its definition has it
 * (unk_308BC0.c): with u16 the key's t0 and n's a3 moved apart. */
void HttpLbs_Send(HttpReq *req) {
    s32 count = 1;
    LobbyPkt *pkt;
    char *s;
    s32 sum;
    u16 n;
    u8 *tmp;
    u16 seed;
    char *start;
    char *p;

    if (req->cancel != 0) {
        req->err = 0x11;
        req->errStep = 0;
        req->state = 0x10;
        return;
    }
    switch (req->method) {
    default:
        req->err = 0x11;
        req->errStep = 0;
        req->state = 0x10;
        return;
    case 1:
        LobbyPkt_BeginCmd(&gLobbyPkt, 0x101);
        LobbyPkt_PutBlob(&gLobbyPkt, req->url + 10, strlen(req->url) - 10);
        LobbyPkt_End(&gLobbyPkt);
        LobbyPkt_Send(&gLobbyPkt);
        break;
    case 2:
        pkt = *(LobbyPkt **)((u8 *)LOBBY_BROWSER_WORK + 0x1A660);
        tmp = (u8 *)pkt + 0xF000;
        LobbyPkt_BeginCmd(pkt, 0x102);
        seed = (u16)(pkt->echo[0] << 8) + pkt->echo[1];
        n = strlen(req->url) - 10;
        sum = Net_LockrockObfuscate(tmp, (u8 *)req->url + 10, seed, n, LOBBY_NET_KEY);
        LobbyPkt_PutU16(pkt, n + 2);
        LobbyPkt_PutU16(pkt, sum);
        LobbyPkt_PutBytes(pkt, tmp, n);
        for (p = req->postData; *p != 0; p++) {
            if (*p == '&') {
                count++;
            }
        }
        LobbyPkt_PutU16(pkt, count);
        start = req->postData;
        for (s = start; *start != 0; s++) {
            if (*s == 0 || *s == '&') {
                n = s - start;
                sum = Net_LockrockObfuscate(tmp, (u8 *)start, seed, n, LOBBY_NET_KEY);
                LobbyPkt_PutU16(pkt, n + 2);
                LobbyPkt_PutU16(pkt, sum);
                LobbyPkt_PutBytes(pkt, tmp, n);
                start = s + 1;
            }
            if (*s == 0) {
                break;
            }
        }
        LobbyPkt_End(pkt);
        LobbyPkt_Send(pkt);
        break;
    }
    LOBBY_NET_HTTP = 1;
    req->state++;
    gHttpLbsReq = req;
}

void HttpLbs_Wait(HttpReq *req) {
    if (LOBBY->recvTimer < 0) {
        LOBBY_NET_HTTP = 2;
        req->err = 0x11;
        req->errStep = 0;
        req->state = 0x10;
        return;
    }
    if (req->cancel != 0) {
        LOBBY_NET_HTTP = 2;
        req->err = 0x11;
        req->errStep = 0;
        req->state = 0x10;
        return;
    }
}

/* The reply's length is built a byte at a time (n = hi << 8; n += lo): the
 * one-expression sum leaves it in a scratch register. n is declared first. */
void HttpLbs_Receive(void) {
    u16 n;
    HttpReq *req = gHttpLbsReq;
    u8 *p;
    u16 len;

    if (LOBBY_NET_RECV_LEFT >= 2) {
        n = 0x1000;
    } else {
        n = (u16)(LOBBY_RECV_HDR[4] << 8);
        n += LOBBY_RECV_HDR[5];
        n -= req->len;
    }
    if (req->bufSize - req->len <= n) {
        n = req->bufSize - req->len - 1;
    }
    memcpy(req->buf + req->len, gLobbyRecvData, n);
    req->len += n;
    req->buf[req->len] = 0;
    if (LOBBY_NET_RECV_LEFT == 1) {
        LOBBY_NET_HTTP = 0;
        req->state++;
        if (((s8 *)LOBBY_RECV_HDR)[8] != 0) {
            req->err = 0x11;
            req->errStep = 0;
            req->state = 0x10;
            return;
        }
        p = (u8 *)req->buf;
        p += (u16)((u16)(p[0] << 8) + p[1]) + 2;
        len = (u16)(p[0] << 8) + p[1];
        memmove(req->buf, p + 2, len);
        req->len = len;
        if (req->toSjis != 0) {
            if (HttpReq_CheckSjis(req, (u8 *)req->buf, req->len) == 0) {
                Str_EucToSjis((u8 *)req->buf);
            }
            req->len = strlen(req->buf);
        }
    }
}

void HttpLbs_Done(HttpReq *req) {
    req->used = 2;
}

void HttpLbs_Abort(HttpReq *req) {
    req->errStep = 1;
    req->state = 0x11;
}

void HttpLbs_Failed(HttpReq *req) {
    req->used = 2;
}

void HttpAfs_Read(HttpReq *req) {
    req->len = Http_ReadBrdata(req->buf, atoi(req->url + 9));
    req->state++;
}

void HttpAfs_Next(HttpReq *req) {
    req->state++;
}

void HttpAfs_Done(HttpReq *req) {
    req->used = 2;
}

void Http_LoadAfsPartition(void) {
    Brdata_LoadPartition((u8 *)LOBBY_BROWSER_WORK + 0x1C2C0);
}

s32 Http_ReadBrdata(void *buf, s32 flid) {
    return Brdata_Read(buf, flid);
}

s32 Http_ReadFile(void *buf, s32 flid) {
    return Http_ReadBrdata(buf, flid);
}

