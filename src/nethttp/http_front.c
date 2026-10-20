/*
 * nethttp.bin's Capcom front end: the game's HTTP
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
 * browser's heap, its strings); it reaches the lobby's globals
 * (gBrowserWork, gLobby, gLobbyNet) through the LOBBY_* macros below.
 */
#include "common.h"
#include "browser.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "nethttp.h"

/* The lobby's globals this file uses (nethttp links with the lobby's names) */
#define LOBBY_BROWSER_WORK gBrowserWork
extern s32 gHttpActive; /* main's .bss: libhttps is up (main's code never reads it) */
#define LOBBY gLobby
#define LOBBY_NET_HTTP (*(s8 *)&gLobbyNet.reqState)
#define LOBBY_NET_KEY gLobbyNet.key
#define LOBBY_NET_RECV_LEFT (*(u32 *)&gLobbyNet.recvResult)
extern LobbyHdr gLobbyRecvHdr;
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

/* The file's .data. */
/* how many of gHttpReq can be used (volatile: the loops over gHttpReq reload it
 * every iteration) */
volatile u8 gHttpReqMax = {
#include "nethttp/0094FF98.inc"
};
s32 gHttpsSecureOption = {
#include "nethttp/0094FFA0.inc"
};            /* the value HttpReq_SetOptions gives libhttps' option 0xE, for https:// only */
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

/* The file's .bss, declared from the highest address down: the compiler
 * places them last-declared first. */
HttpReq gHttpReq[HTTP_REQ_SLOTS];
HttpReq *gHttpLbsReq;

s32 Http_GetCertSize(s32 i)
{
    return gHttpCertSize[i];
}

/*
 * Starts the HTTP layer: clears gHttpReq, starts the sockets, empties the host cache, sets up the browser's heap and
 * hands Browser_Alloc, Browser_Realloc and Browser_Free to libhttps, then starts libhttps and gives it the two CA
 * certificates (LOBBY_BROWSER_WORK->cert0 and cert1, with their sizes from Http_GetCertSize).
 * Returns 0 and sets gHttpActive, or -1 when either of the last two libhttps calls fails.
 */
s32 Http_Init(void)
{
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

/*
 * Ends the HTTP layer: two libhttps calls, then clears gHttpActive. Returns 0.
 */
s32 Http_Term(void)
{
    func_00909738();
    func_00900660();
    gHttpActive = 0;
    return 0;
}

/*
 * Hands out the first unused request among the first gHttpReqMax of gHttpReq: clears it, sets used to HTTP_REQ_BUSY,
 * run to HttpReq_Step and index to its slot. Returns the request, or NULL when every slot is busy.
 */
HttpReq *HttpReq_Alloc(void)
{
    s32 i;
    HttpReq *req;

    for (i = 0, req = gHttpReq; i < gHttpReqMax; i++, req++) {
        if (req->used == HTTP_REQ_FREE) {
            memset(req, 0, sizeof(HttpReq));
            req->used = HTTP_REQ_BUSY;
            req->run = HttpReq_Step;
            req->index = i;
            return req;
        }
    }
    return NULL;
}

/*
 * Returns 1 when one of the first gHttpReqMax requests is unused (HTTP_REQ_FREE), else 0.
 */
s32 HttpReq_HasFree(void)
{
    s32 i;
    HttpReq *req;

    req = gHttpReq;
    for (i = 0; i < gHttpReqMax; i++, req++) {
        if (req->used == HTTP_REQ_FREE) {
            return 1;
        }
    }
    return 0;
}

/*
 * Returns 1 when no request is in use (every slot HTTP_REQ_FREE), else 0. It checks all HTTP_REQ_SLOTS slots of gHttpReq,
 * not only the first gHttpReqMax.
 */
s32 HttpReq_AllIdle(void)
{
    s32 i;
    HttpReq *req;

    /* the test is written !used == 0, the form whose code is the game's:
     * used != 0 compiles two instructions shorter */
    for (i = 0, req = gHttpReq; i < HTTP_REQ_SLOTS; i++, req++) {
        if (!req->used == 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * Clears a request that is in use, used included, so HttpReq_Alloc can hand the slot out again. Returns 0.
 */
s32 HttpReq_Free(HttpReq *req)
{
    if (req->used != 0) {
        memset(req, 0, sizeof(HttpReq));
    }
    return 0;
}

void HttpReq_Step(HttpReq *req)
{
    gHttpStateFn[req->state](req);
}

/* libhttps' callback when a connection started by HttpReq_Connect ends (option
 * HTTPS_OPT_CONNECT_CB): out is the request's async, status HTTP_ASYNC_DONE or
 * HTTP_ASYNC_FAILED. */
void HttpReq_OnConnectDone(void *ctx, u8 status, u8 *out)
{
    *out = status;
}

/* libhttps' callback when the sending started by HttpReq_Send ends (option
 * HTTPS_OPT_SEND_CB), as HttpReq_OnConnectDone. */
void HttpReq_OnSendDone(void *ctx, u8 status, u8 *out)
{
    *out = status;
}

/*
 * Scans n bytes of text for a byte 0x81-0x8D or 0x8F-0xA0 (a Shift-JIS lead byte). Sets req->sjis to 1 and returns 1
 * when it finds one, else sets req->sjis to 0 and returns 0. Seen: every caller converts the text with
 * Str_EucToSjis when it returns 0.
 */
s32 HttpReq_CheckSjis(HttpReq *req, u8 *text, s32 n)
{
    for (; n > 0; n--, text++) {
        if ((*text >= 0x81 && *text <= 0x8D) || (*text >= 0x8F && *text <= 0xA0)) {
            req->sjis = 1;
            return 1;
        }
    }
    req->sjis = 0;
    return 0;
}

/*
 * The first state: dispatches on Url_GetScheme(url). URL_FILE and URL_MEMORY copy the disc file or memPage[N] into buf
 * at once (at most bufSize - 1 bytes, NUL-terminated, len set, Shift-JIS when toSjis is set) and go to HttpReq_Done;
 * URL_AFS goes to HttpAfs_Read, URL_LBS to HttpLbs_Send, URL_HTTP and URL_HTTPS set secure and go to HttpReq_ParseUrl.
 * Failures go to HttpReq_Abort with HTTP_ERR_CANCELLED, HTTP_ERR_NOT_CONNECTED (NetIf_IsConnected is 0) or HTTP_ERR_URL (anything else).
 */
void HttpReq_Open(HttpReq *req)
{
    char path[0x100];
    s32 n;
    s32 i;

    if (req->cancel != 0) {
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_NONE;
        req->state = HTTP_ST_ABORT;
        return;
    }
    switch ((u8)Url_GetScheme(req->url)) {
    case URL_FILE:
        strncpy(path, req->url + 6, 0xFF);
        path[0xFF] = 0;
        Path_ToBackslashes(path);
        n = File_GetSize(path);
        if (n == 0) {
            req->err = HTTP_ERR_URL;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
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
        req->state = HTTP_ST_DONE;
        return;
    case URL_AFS:
        req->state = HTTP_ST_AFS_READ;
        req->step = 0;
        return;
    case URL_MEMORY:
        i = atoi(req->url + 9);
        if (i < 0 || i >= 3) {
            req->err = HTTP_ERR_URL;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
            return;
        }
        n = strlen((char *)LOBBY_BROWSER_WORK->memPage[i]);
        if (n == 0) {
            req->err = HTTP_ERR_URL;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
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
        req->state = HTTP_ST_DONE;
        return;
    case URL_LBS:
        if (NetIf_IsConnected() == 0) {
            req->err = HTTP_ERR_NOT_CONNECTED;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
            return;
        }
        req->state = HTTP_ST_LBS_SEND;
        req->step = 0;
        return;
    case URL_HTTP:
        if (NetIf_IsConnected() == 0) {
            req->err = HTTP_ERR_NOT_CONNECTED;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
            return;
        }
        req->secure = 0;
        req->state++;
        req->step = 0;
        return;
    case URL_HTTPS:
        if (NetIf_IsConnected() == 0) {
            req->err = HTTP_ERR_NOT_CONNECTED;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
            return;
        }
        req->secure = 1;
        req->state++;
        req->step = 0;
        return;
    default:
        req->err = HTTP_ERR_URL;
        req->errStep = HTTP_TEARDOWN_NONE;
        req->state = HTTP_ST_ABORT;
        return;
    }
}

/*
 * Turns NetIf_SetErrorCheck off, then on the next call has libhttps parse url into uri and, as proxyFlags asks,
 * proxyUrl into proxyUri and sslProxyUrl into sslProxyUri; then goes to HttpReq_CreateClient.
 * A parse failure is HTTP_ERR_URL (errStep HTTP_TEARDOWN_NONE for url, HTTP_TEARDOWN_URL for a proxy), a cancel HTTP_ERR_CANCELLED; both go to HttpReq_Abort.
 */
void HttpReq_ParseUrl(HttpReq *req)
{
    switch (req->step) {
    case 0:
        req->step = 5;
        NetIf_SetErrorCheck(0);
        break;
    case 5:
        if (req->cancel != 0) {
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
            break;
        }
        req->uri = func_009006F8(req->url, 0x60);
        if (req->uri == 0) {
            req->err = HTTP_ERR_URL;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_ABORT;
            break;
        }
        if (req->proxyFlags) {
            if (req->proxyFlags & 1) {
                req->proxyUri = func_009006F8(req->proxyUrl, 0);
                if (req->proxyUri == 0) {
                    req->err = HTTP_ERR_URL;
                    req->errStep = HTTP_TEARDOWN_URL;
                    req->state = HTTP_ST_ABORT;
                    break;
                }
            }
            if (req->proxyFlags & 2) {
                req->sslProxyUri = func_009006F8(req->sslProxyUrl, 0);
                if (req->sslProxyUri == 0) {
                    req->err = HTTP_ERR_URL;
                    req->errStep = HTTP_TEARDOWN_URL;
                    req->state = HTTP_ST_ABORT;
                    break;
                }
            }
        }
        req->state++;
        req->step = 0;
        break;
    }
}

/*
 * Creates the libhttps client into client and goes to HttpReq_SetOptions. A failure is HTTP_ERR_CLIENT, a cancel HTTP_ERR_CANCELLED
 * (both errStep HTTP_TEARDOWN_URL, to HttpReq_Abort).
 */
void HttpReq_CreateClient(HttpReq *req)
{
    if (req->cancel != 0) {
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_URL;
        req->state = HTTP_ST_ABORT;
        return;
    }
    if ((req->client = func_008FD8F0()) == 0) {
        req->err = HTTP_ERR_CLIENT;
        req->errStep = HTTP_TEARDOWN_URL;
        req->state = HTTP_ST_ABORT;
        return;
    }
    req->state++;
    req->step = 0;
}

/*
 * Configures the client. Step 0: the parsed URI, the method (HTTPS_METHOD_GET or HTTPS_METHOD_POST; another method is HTTP_ERR_OPTIONS),
 * for POST the Content-Type header (hdr[2]) and postData or a zero Content-Length (hdr[3]), the client-name string and fixed
 * options, the two callbacks that write async, the proxy (and option 0xE, gHttpsSecureOption, for https://); clears auth. Step 1, where
 * HttpReq_Authenticate comes back: refererHdr and, with noCache, hdr[0] and hdr[1]; then HttpReq_Connect. Errors: HTTP_ERR_OPTIONS, errStep HTTP_TEARDOWN_CLIENT.
 */
void HttpReq_SetOptions(HttpReq *req)
{
    s32 method;

    if (req->cancel != 0) {
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    }
    switch (req->method) {
    case HTTP_METHOD_GET:
        method = HTTPS_METHOD_GET;
        break;
    case HTTP_METHOD_POST:
        method = HTTPS_METHOD_POST;
        break;
    default:
        req->err = HTTP_ERR_OPTIONS;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    }
    switch (req->step) {
    case 0:
        if (func_00901600(req->client, HTTPS_OPT_URI, req->uri) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, HTTPS_OPT_METHOD, method) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (method == HTTPS_METHOD_POST) {
            if (func_00901600(req->client, 0x13, "Content-Length") < 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
            if ((req->hdr[2] = func_00900EE8(0, "Content-Type", "application/x-www-form-urlencoded")) == 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
            if (func_00901600(req->client, HTTPS_OPT_HEADER, req->hdr[2], 2) < 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
            if (strlen(req->postData) == 0) {
                if ((req->hdr[3] = func_00900EE8(0, "Content-Length", "0")) == 0) {
                    req->err = HTTP_ERR_OPTIONS;
                    req->errStep = HTTP_TEARDOWN_CLIENT;
                    req->state = HTTP_ST_ABORT;
                    return;
                }
                if (func_00901600(req->client, HTTPS_OPT_HEADER, req->hdr[3], 2) < 0) {
                    req->err = HTTP_ERR_OPTIONS;
                    req->errStep = HTTP_TEARDOWN_CLIENT;
                    req->state = HTTP_ST_ABORT;
                    return;
                }
            } else {
                if (func_00901600(req->client, HTTPS_OPT_POST_DATA, req->postData, strlen(req->postData), 0) < 0) {
                    req->err = HTTP_ERR_OPTIONS;
                    req->errStep = HTTP_TEARDOWN_CLIENT;
                    req->state = HTTP_ST_ABORT;
                    return;
                }
            }
        }
        if (func_00901600(req->client, HTTPS_OPT_USER_AGENT, "KDDI/Multi-Matching-BB_Browser") < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, 0x15, 60) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, 7, 60) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, 8, 60) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, 9, 0) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, HTTPS_OPT_SEND_CB, HttpReq_OnSendDone, &req->async) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, 0x16, 0) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (func_00901600(req->client, HTTPS_OPT_CONNECT_CB, HttpReq_OnConnectDone, &req->async) < 0) {
            req->err = HTTP_ERR_OPTIONS;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        if (req->secure != 0) {
            if (req->proxyFlags & 2) {
                if (func_00901600(req->client, HTTPS_OPT_PROXY, req->sslProxyUri) < 0) {
                    req->err = HTTP_ERR_OPTIONS;
                    req->errStep = HTTP_TEARDOWN_CLIENT;
                    req->state = HTTP_ST_ABORT;
                    return;
                }
            } else if (req->proxyFlags & 1) {
                if (func_00901600(req->client, HTTPS_OPT_PROXY, req->proxyUri) < 0) {
                    req->err = HTTP_ERR_OPTIONS;
                    req->errStep = HTTP_TEARDOWN_CLIENT;
                    req->state = HTTP_ST_ABORT;
                    return;
                }
            }
            /* the cast puts the global's load before req->client's, as in the game */
            if (func_00901600(req->client, 0xE, (u32)gHttpsSecureOption) < 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        } else if (req->proxyFlags & 1) {
            if (func_00901600(req->client, HTTPS_OPT_PROXY, req->proxyUri) < 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        }
        req->auth = NULL;
        req->step++;
        break;
    case 1:
        if (req->referer != NULL) {
            if ((req->refererHdr = func_00900EE8(0, "Referer", req->referer)) == 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
            if (func_00901600(req->client, HTTPS_OPT_HEADER, req->refererHdr, 2) < 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        }
        if (req->noCache != 0) {
            if ((req->hdr[0] = func_00900EE8(0, "Pragma", "no-cache")) == 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
            if (func_00901600(req->client, HTTPS_OPT_HEADER, req->hdr[0], 2) < 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
            if ((req->hdr[1] = func_00900EE8(0, "Cache-Control", "no-cache")) == 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
            if (func_00901600(req->client, HTTPS_OPT_HEADER, req->hdr[1], 2) < 0) {
                req->err = HTTP_ERR_OPTIONS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        }
        req->state++;
        req->step = 0;
        break;
    }
}

/*
 * Starts libhttps' connection (async HTTP_ASYNC_BUSY, timer HTTP_WAIT_FRAMES) and waits for HttpReq_OnConnectDone to change
 * async: HTTP_ASYNC_DONE goes on, two steps later, to HttpReq_Send; HTTP_ASYNC_FAILED is HTTP_ERR_CONNECT (HTTP_ERR_CANCELLED when cancel is set). When timer runs out it stops libhttps'
 * operation, waits for async to leave 1 and ends with HTTP_ERR_TIMEOUT. Errors have errStep HTTP_TEARDOWN_CONNECTION (HTTP_TEARDOWN_CLIENT for a cancel before starting)
 * and go to HttpReq_Abort.
 */
void HttpReq_Connect(HttpReq *req)
{
    switch (req->step) {
    case 0:
        if (req->cancel != 0) {
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            break;
        }
        req->step++;
        req->timer = HTTP_WAIT_FRAMES;
        req->async = HTTP_ASYNC_BUSY;
        if (func_00902BD0(req->client, 2, 1, 3) < 0) {
            req->err = HTTP_ERR_CONNECT;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
        }
        break;
    case 1:
        if (req->timer-- < 0) {
            func_00902280(req->client);
            req->step = HTTP_STEP_TIMEOUT_WAIT;
            req->timer = 0;
            break;
        }
        switch (req->async) {
        case HTTP_ASYNC_DONE:
            req->step++;
            req->timer = 0;
            break;
        case HTTP_ASYNC_BUSY:
            break;
        case HTTP_ASYNC_FAILED:
            if (req->cancel != 0) {
                req->step = HTTP_STEP_CANCEL_WAIT;
                req->timer = 0;
                break;
            }
            req->step = 0;
            req->timer = 0;
            req->err = HTTP_ERR_CONNECT;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
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
    case HTTP_STEP_CANCEL_WAIT:
        switch (req->async) {
        case HTTP_ASYNC_BUSY:
            break;
        case HTTP_ASYNC_DONE:
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            break;
        case HTTP_ASYNC_FAILED:
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            break;
        }
        break;
    case HTTP_STEP_TIMEOUT_WAIT:
        switch (req->async) {
        case HTTP_ASYNC_BUSY:
            break;
        case HTTP_ASYNC_DONE:
            req->err = HTTP_ERR_TIMEOUT;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            break;
        case HTTP_ASYNC_FAILED:
            req->err = HTTP_ERR_TIMEOUT;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            return;
        }
        break;
    }
}

/*
 * Starts the request (async HTTP_ASYNC_BUSY, timer HTTP_WAIT_FRAMES); while waiting it copies libhttps' received byte count into recvLen
 * and the reply's Content-Length into contentLen (0 when unknown), and a change of recvLen resets timer to HTTP_RECV_FRAMES.
 * HttpReq_OnSendDone's HTTP_ASYNC_DONE goes on, two steps later, to HttpReq_ReadBody; HTTP_ASYNC_FAILED is HTTP_ERR_SEND. A cancel or a time-out stops libhttps' operation,
 * waits for async to leave 1 and ends with HTTP_ERR_CANCELLED or HTTP_ERR_TIMEOUT. Errors have errStep HTTP_TEARDOWN_CONNECTION and go to HttpReq_Abort.
 */
void HttpReq_Send(HttpReq *req)
{
    s64 n;

    switch (req->step) {
    case 0:
        if (req->cancel != 0) {
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            break;
        }
        req->step++;
        req->timer = HTTP_WAIT_FRAMES;
        req->async = HTTP_ASYNC_BUSY;
        if (func_00900378(req->client) < 0) {
            req->err = HTTP_ERR_SEND;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
        }
        break;
    case 1:
        if (req->cancel != 0) {
            func_00902280(req->client);
            req->step = HTTP_STEP_CANCEL_WAIT;
            req->timer = 0;
            break;
        }
        n = func_009005A8(req->client);
        if (req->recvLen != n) {
            req->timer = HTTP_RECV_FRAMES;
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
            req->step = HTTP_STEP_TIMEOUT_WAIT;
            req->timer = 0;
            break;
        }
        switch (req->async) {
        case HTTP_ASYNC_DONE:
            req->step++;
            req->timer = 0;
            break;
        case HTTP_ASYNC_BUSY:
            break;
        case HTTP_ASYNC_FAILED:
            req->step = 0;
            req->timer = 0;
            req->err = HTTP_ERR_SEND;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
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
    case HTTP_STEP_CANCEL_WAIT:
        switch (req->async) {
        case HTTP_ASYNC_BUSY:
            break;
        case HTTP_ASYNC_DONE:
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            break;
        case HTTP_ASYNC_FAILED:
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            break;
        }
        break;
    case HTTP_STEP_TIMEOUT_WAIT:
        switch (req->async) {
        case HTTP_ASYNC_BUSY:
            break;
        case HTTP_ASYNC_DONE:
            req->err = HTTP_ERR_TIMEOUT;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            break;
        case HTTP_ASYNC_FAILED:
            req->err = HTTP_ERR_TIMEOUT;
            req->errStep = HTTP_TEARDOWN_CONNECTION;
            req->state = HTTP_ST_ABORT;
            return;
        }
        break;
    }
}

/*
 * Takes libhttps' reply into resp (HTTP_ERR_NO_REPLY and HttpReq_Abort when there is none) and copies its body into buf: at most
 * bufSize - 1 bytes, NUL-terminated, len set, converted to Shift-JIS when toSjis is set and HttpReq_CheckSjis finds
 * no Shift-JIS byte. With no body buf is left empty. Then goes to HttpReq_CheckStatus.
 */
void HttpReq_ReadBody(HttpReq *req)
{
    s32 n;

    if (req->cancel != 0) {
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_CONNECTION;
        req->state = HTTP_ST_ABORT;
        return;
    }
    if ((req->resp = func_00900548(req->client)) == NULL) {
        req->err = HTTP_ERR_NO_REPLY;
        req->errStep = HTTP_TEARDOWN_CONNECTION;
        req->state = HTTP_ST_ABORT;
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

/*
 * Acts on resp->status (HTTP_ERR_STATUS when its first libhttps call fails). A redirect (HTTP_STATUS_MULTIPLE_CHOICES to HTTP_STATUS_SEE_OTHER, HTTP_STATUS_USE_PROXY, HTTP_STATUS_TEMPORARY_REDIRECT) puts the new URL,
 * resolved against url, in buf and ends with HTTP_ERR_REDIRECTED; HTTP_STATUS_UNAUTHORIZED and HTTP_STATUS_PROXY_AUTH_REQUIRED go to HttpReq_Authenticate, or end with HTTP_ERR_NEED_USER or HTTP_ERR_NEED_PROXY_USER
 * when user/pass or proxyUser/proxyPass is NULL; listed 4xx and 5xx codes set httpErr; all others go to HttpReq_Close.
 * Then frees the old authInfo, reads the reply's into authInfo and applies it to digest (HTTP_ERR_AUTH_INFO), frees auth (HTTP_ERR_FREE if a free fails).
 */
void HttpReq_CheckStatus(HttpReq *req)
{
    char **loc;
    s32 ret;

    if (req->cancel != 0) {
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_CONNECTION;
        req->state = HTTP_ST_ABORT;
        return;
    }
    if (func_00902D28(req->client) < 0) {
        req->err = HTTP_ERR_STATUS;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    }
    req->httpErr = 0;
    switch ((u16)req->resp->status) {
    case HTTP_STATUS_MULTIPLE_CHOICES:
    case HTTP_STATUS_MOVED_PERMANENTLY:
    case HTTP_STATUS_FOUND:
    case HTTP_STATUS_SEE_OTHER:
    case HTTP_STATUS_TEMPORARY_REDIRECT:
    case HTTP_STATUS_USE_PROXY:
        loc = func_009066E0(req->resp);
        if (loc == NULL) {
            req->err = HTTP_ERR_NO_REPLY;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        Url_Copy(req->buf, *loc);
        Url_ClearBase();
        Url_Resolve(req->url, req->buf);
        Url_ClearBase();
        func_009067D0(loc);
        req->err = HTTP_ERR_REDIRECTED;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    case HTTP_STATUS_PROXY_AUTH_REQUIRED:
        if (req->proxyUser == NULL || req->proxyPass == NULL) {
            req->err = HTTP_ERR_NEED_PROXY_USER;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        req->state++;
        break;
    case HTTP_STATUS_UNAUTHORIZED:
        if (req->user == NULL || req->pass == NULL) {
            req->err = HTTP_ERR_NEED_USER;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        req->state++;
        break;
    case HTTP_STATUS_BAD_REQUEST:
    case HTTP_STATUS_PAYMENT_REQUIRED:
    case HTTP_STATUS_FORBIDDEN:
    case HTTP_STATUS_NOT_FOUND:
    case HTTP_STATUS_METHOD_NOT_ALLOWED:
    case HTTP_STATUS_NOT_ACCEPTABLE:
    case HTTP_STATUS_REQUEST_TIMEOUT:
    case HTTP_STATUS_CONFLICT:
    case HTTP_STATUS_GONE:
    case HTTP_STATUS_LENGTH_REQUIRED:
    case HTTP_STATUS_PRECONDITION_FAILED:
    case HTTP_STATUS_ENTITY_TOO_LARGE:
    case HTTP_STATUS_URI_TOO_LONG:
    case HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:
    case HTTP_STATUS_RANGE_NOT_SATISFIABLE:
    case HTTP_STATUS_EXPECTATION_FAILED:
    case HTTP_STATUS_UNPROCESSABLE_ENTITY:
    case HTTP_STATUS_LOCKED:
    case HTTP_STATUS_FAILED_DEPENDENCY:
    case HTTP_STATUS_INTERNAL_ERROR:
    case HTTP_STATUS_NOT_IMPLEMENTED:
    case HTTP_STATUS_BAD_GATEWAY:
    case HTTP_STATUS_SERVICE_UNAVAILABLE:
    case HTTP_STATUS_GATEWAY_TIMEOUT:
    case HTTP_STATUS_VERSION_NOT_SUPPORTED:
    case HTTP_STATUS_INSUFFICIENT_STORAGE:
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
            req->err = HTTP_ERR_FREE;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
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
            req->err = HTTP_ERR_AUTH_INFO;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
    }
    if (req->auth != NULL) {
        if (func_00905E38(req->auth) < 0) {
            req->auth = NULL;
            req->err = HTTP_ERR_FREE;
            req->errStep = HTTP_TEARDOWN_CLIENT;
            req->state = HTTP_ST_ABORT;
            return;
        }
        req->auth = NULL;
    }
}

/*
 * Answers the HTTP_STATUS_UNAUTHORIZED or HTTP_STATUS_PROXY_AUTH_REQUIRED challenge in resp, with proxyUser/proxyPass for the proxy's and user/pass otherwise: takes the challenge
 * into auth (HTTP_ERR_NO_CHALLENGE), prepares the client for a second request (HTTP_ERR_AUTH_RESET), then sets basic credentials or, when auth->digest
 * is set, fills digest from auth, uri's path, nc 1 and a random cnonce when qop is set (HTTP_ERR_CREDENTIALS). Goes back to HttpReq_SetOptions
 * step 1 and clears the four credential pointers: a second challenge then ends in HttpReq_CheckStatus with HTTP_ERR_NEED_USER or HTTP_ERR_NEED_PROXY_USER.
 */
void HttpReq_Authenticate(HttpReq *req)
{
    s32 qop;
    s32 mode;
    s32 cnonceSeed;
    s16 status;

    status = req->resp->status;
    if (req->cancel != 0) {
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    }
    req->auth = func_00905ED0(req->resp);
    if (req->auth == NULL) {
        req->err = HTTP_ERR_NO_CHALLENGE;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    }
    if (func_008FDA50(req->client) < 0) {
        req->err = HTTP_ERR_AUTH_RESET;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    }
    if (func_00901600(req->client, HTTPS_OPT_HEADER, 0, 1) < 0) {
        req->err = HTTP_ERR_AUTH_RESET;
        req->errStep = HTTP_TEARDOWN_CLIENT;
        req->state = HTTP_ST_ABORT;
        return;
    }
    switch (req->method) {
    case HTTP_METHOD_GET:
        mode = HTTPS_METHOD_GET;
        break;
    case HTTP_METHOD_POST:
        mode = HTTPS_METHOD_POST;
        break;
    }
    if (req->auth->digest == 0) {
        if ((u16)status == HTTP_STATUS_PROXY_AUTH_REQUIRED) {
            if (func_00904868(req->client, req->proxyUser, req->proxyPass, 1) < 0) {
                req->err = HTTP_ERR_CREDENTIALS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        } else {
            if (func_00904868(req->client, req->user, req->pass, 0) < 0) {
                req->err = HTTP_ERR_CREDENTIALS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        }
    } else {
        if ((u16)status == HTTP_STATUS_PROXY_AUTH_REQUIRED) {
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
            cnonceSeed = req->resp->unk30;
            sprintf(req->cnonce, "%08x", cnonceSeed + rand());
            req->digest.cnonce = req->cnonce;
        } else {
            req->digest.cnonce = NULL;
        }
        req->digest.method = mode;
        req->digest.unk2C = 0;
        req->digest.unk30 = 0;
        if ((u16)status == HTTP_STATUS_PROXY_AUTH_REQUIRED) {
            if (func_00906070(req->client, &req->digest, 1) < 0) {
                req->err = HTTP_ERR_CREDENTIALS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        } else {
            if (func_00906070(req->client, &req->digest, 0) < 0) {
                req->err = HTTP_ERR_CREDENTIALS;
                req->errStep = HTTP_TEARDOWN_CLIENT;
                req->state = HTTP_ST_ABORT;
                return;
            }
        }
    }
    req->state = HTTP_ST_SET_OPTIONS;
    req->step = 1;
    req->proxyUser = NULL;
    req->proxyPass = NULL;
    req->user = NULL;
    req->pass = NULL;
}

/*
 * Ends a finished request: frees auth, authInfo, hdr[0..3], refererHdr, the client and the parsed URLs (uri, proxyUri,
 * sslProxyUri), clearing each pointer, and goes to HttpReq_Done.
 */
void HttpReq_Close(HttpReq *req)
{
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

void HttpReq_Done(HttpReq *req)
{
    req->used = HTTP_REQ_FINISHED;
    NetIf_SetErrorCheck(1);
}

/*
 * Tears down a failed request as far as errStep says it got, falling through from 4: 4 and 3 make one libhttps call each,
 * 2 frees auth, authInfo, the headers and the client, 1 frees the parsed URLs. From level 2 down it maps libhttps' last
 * error to a class 1-8 that replaces errStep (0 from levels 0 and 1; BrowserReq_TakeHttpResult copies it). Then goes to HttpReq_Failed.
 */
void HttpReq_Abort(HttpReq *req)
{
    s32 code = 0;

    switch (req->errStep) {
    case HTTP_TEARDOWN_ALL:
        func_00902280(req->client);
    case HTTP_TEARDOWN_CONNECTION:
        func_00902D28(req->client);
    case HTTP_TEARDOWN_CLIENT:
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
    case HTTP_TEARDOWN_URL:
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
    case HTTP_TEARDOWN_NONE:
        break;
    }
    req->errStep = (u8)code;
    req->state = HTTP_ST_FAILED;
}

void HttpReq_Failed(HttpReq *req)
{
    req->used = HTTP_REQ_FINISHED;
    NetIf_SetErrorCheck(1);
}

/* Sends an lbs:// request (the lobby protocol's LOBBY_CMD_HTTP_GET plain or LOBBY_CMD_HTTP_POST
 * obfuscated) through the lobby's connection: the URL's tail and each
 * form field of postData go into a lobby packet, obfuscated with the
 * connection's key for LOBBY_CMD_HTTP_POST. Net_LockrockObfuscate's key is declared
 * s32, as its definition has it: with u16 the compiler places the key's
 * and the length's loads differently. */
void HttpLbs_Send(HttpReq *req)
{
    s32 count = 1;
    LobbyPkt *pkt;
    char *fieldEnd;
    s32 sum;
    u16 n;
    u8 *obfBuf;
    u16 seed;
    char *start;
    char *scan;

    if (req->cancel != 0) {
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_NONE;
        req->state = HTTP_ST_LBS_ABORT;
        return;
    }
    switch (req->method) {
    default:
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_NONE;
        req->state = HTTP_ST_LBS_ABORT;
        return;
    case HTTP_METHOD_GET:
        LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_HTTP_GET);
        LobbyPkt_PutBlob(&gLobbyPkt, req->url + HTTP_LBS_PREFIX_LEN, strlen(req->url) - HTTP_LBS_PREFIX_LEN);
        LobbyPkt_End(&gLobbyPkt);
        LobbyPkt_Send(&gLobbyPkt);
        break;
    case HTTP_METHOD_POST:
        pkt = (LobbyPkt *)LOBBY_BROWSER_WORK->buf64k;
        obfBuf = (u8 *)pkt + HTTP_LBS_OBF_OFFSET;
        LobbyPkt_BeginCmd(pkt, LOBBY_CMD_HTTP_POST);
        seed = (u16)(pkt->echo[0] << 8) + pkt->echo[1];
        n = strlen(req->url) - HTTP_LBS_PREFIX_LEN;
        sum = Net_LockrockObfuscate(obfBuf, (u8 *)req->url + HTTP_LBS_PREFIX_LEN, seed, n, LOBBY_NET_KEY);
        LobbyPkt_PutU16(pkt, n + 2);
        LobbyPkt_PutU16(pkt, sum);
        LobbyPkt_PutBytes(pkt, obfBuf, n);
        for (scan = req->postData; *scan != 0; scan++) {
            if (*scan == '&') {
                count++;
            }
        }
        LobbyPkt_PutU16(pkt, count);
        start = req->postData;
        for (fieldEnd = start; *start != 0; fieldEnd++) {
            if (*fieldEnd == 0 || *fieldEnd == '&') {
                n = fieldEnd - start;
                sum = Net_LockrockObfuscate(obfBuf, (u8 *)start, seed, n, LOBBY_NET_KEY);
                LobbyPkt_PutU16(pkt, n + 2);
                LobbyPkt_PutU16(pkt, sum);
                LobbyPkt_PutBytes(pkt, obfBuf, n);
                start = fieldEnd + 1;
            }
            if (*fieldEnd == 0) {
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

/*
 * Waits for the lobby's reply to a URL_LBS request (HttpLbs_Receive moves the request on). When gLobby.recvTimer runs out
 * or cancel is set, sets LOBBY_NET_HTTP (gLobbyNet.reqState) to 2 and goes to HttpLbs_Abort with HTTP_ERR_CANCELLED.
 */
void HttpLbs_Wait(HttpReq *req)
{
    if (LOBBY->recvTimer < 0) {
        LOBBY_NET_HTTP = 2;
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_NONE;
        req->state = HTTP_ST_LBS_ABORT;
        return;
    }
    if (req->cancel != 0) {
        LOBBY_NET_HTTP = 2;
        req->err = HTTP_ERR_CANCELLED;
        req->errStep = HTTP_TEARDOWN_NONE;
        req->state = HTTP_ST_LBS_ABORT;
        return;
    }
}

/* Called by the lobby's message handlers for each part of an lbs://
 * reply: appends the part to gHttpLbsReq's buf. On the last part it clears
 * LOBBY_NET_HTTP and ends with HTTP_ERR_CANCELLED if the reply's error byte is set,
 * else keeps only the reply's second length-prefixed block in buf and goes
 * to HttpLbs_Done.
 * The block's length is built a byte at a time (n = hi << 8; n += lo): as
 * one expression the sum stays in another register. n is declared first. */
void HttpLbs_Receive(void)
{
    u16 n;
    HttpReq *req = gHttpLbsReq;
    u8 *block;
    u16 len;

    if (LOBBY_NET_RECV_LEFT >= 2) {
        n = 0x1000;
    } else {
        n = (u16)(LOBBY_RECV_HDR.size[0] << 8);
        n += LOBBY_RECV_HDR.size[1];
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
        if (LOBBY_RECV_HDR.err != 0) {
            req->err = HTTP_ERR_CANCELLED;
            req->errStep = HTTP_TEARDOWN_NONE;
            req->state = HTTP_ST_LBS_ABORT;
            return;
        }
        block = (u8 *)req->buf;
        block += (u16)((u16)(block[0] << 8) + block[1]) + 2;
        len = (u16)(block[0] << 8) + block[1];
        memmove(req->buf, block + 2, len);
        req->len = len;
        if (req->toSjis != 0) {
            if (HttpReq_CheckSjis(req, (u8 *)req->buf, req->len) == 0) {
                Str_EucToSjis((u8 *)req->buf);
            }
            req->len = strlen(req->buf);
        }
    }
}

void HttpLbs_Done(HttpReq *req)
{
    req->used = HTTP_REQ_FINISHED;
}

void HttpLbs_Abort(HttpReq *req)
{
    req->errStep = HTTP_TEARDOWN_URL;
    req->state = HTTP_ST_LBS_FAILED;
}

void HttpLbs_Failed(HttpReq *req)
{
    req->used = HTTP_REQ_FINISHED;
}

void HttpAfs_Read(HttpReq *req)
{
    req->len = Http_ReadBrdata(req->buf, atoi(req->url + 9));
    req->state++;
}

void HttpAfs_Next(HttpReq *req)
{
    req->state++;
}

void HttpAfs_Done(HttpReq *req)
{
    req->used = HTTP_REQ_FINISHED;
}

void Http_LoadAfsPartition(void)
{
    Brdata_LoadPartition(LOBBY_BROWSER_WORK->afsPartInfo);
}

s32 Http_ReadBrdata(void *buf, s32 flid)
{
    return Brdata_Read(buf, flid);
}

s32 Http_ReadFile(void *buf, s32 flid)
{
    return Http_ReadBrdata(buf, flid);
}

