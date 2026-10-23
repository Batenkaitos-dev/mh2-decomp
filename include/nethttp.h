#ifndef NETHTTP_H
#define NETHTTP_H

#include "common.h"

/* nethttp's request object and the views of libhttps' structs it reads (nethttp's
 * front end; the lobby's browser fills requests and reads their results too).
 * libhttps' structs are only the fields this code reads:
 * no size checks for them. */

/* A reply as libhttps returns it */
typedef struct HttpResp {
    u8 unk0[0x8];
    s16 status;              /* 0x08: the HTTP status code */
    u8 unkA[0x18 - 0xA];
    char *body;              /* 0x18 */
    u8 unk1C[0x20 - 0x1C];
    s32 bodyLen;             /* 0x20 */
    u8 unk24[0x30 - 0x24];
    s32 unk30;               /* 0x30 */
} HttpResp;

/* A URL as libhttps parses it */
typedef struct HttpUrl {
    u8 unk0[0x14];
    char *path;              /* 0x14 */
} HttpUrl;

/* An authentication challenge, as libhttps parses it */
typedef struct HttpAuthReq {
    u8 unk0[0x8];
    s32 digest;              /* 0x08: 0 basic */
    char *realm;             /* 0x0C */
    u8 unk10[0x18 - 0x10];
    char *nonce;             /* 0x18 */
    char *opaque;            /* 0x1C */
    u8 unk20[0x24 - 0x20];
    s32 algorithm;           /* 0x24 */
    s32 qop;                 /* 0x28 */
} HttpAuthReq;

/* A digest authentication's answer, as libhttps makes it */
typedef struct HttpDigest {
    char *user;              /* 0x00 */
    char *realm;             /* 0x04 */
    char *pass;              /* 0x08 */
    char *uri;               /* 0x0C */
    char *nonce;             /* 0x10 */
    char *cnonce;            /* 0x14 */
    char *opaque;            /* 0x18 */
    s32 algorithm;           /* 0x1C */
    s32 nc;                  /* 0x20 */
    s32 qop;                 /* 0x24 */
    s32 method;              /* 0x28: the request method, HTTPS_METHOD_* */
    s32 unk2C;               /* 0x2C */
    s32 unk30;               /* 0x30 */
} HttpDigest;

typedef struct HttpReq HttpReq;
typedef void (*HttpStateFn)(HttpReq *req);

/* One request (228 bytes): HttpReq_Alloc hands one out, the caller fills it (the lobby's
 * HttpReq_Setup), HttpReq_Step runs it, HttpReq_Free returns it. err, sjis and index are
 * s8: nethttp only writes them, the lobby's browser reads them signed. */
struct HttpReq {
    s8 secure;               /* 0x00: 1 for https:// */
    s8 proxyFlags;           /* 0x01: 1 proxyUrl for http://, 2 sslProxyUrl for https:// */
    s8 noCache;              /* 0x02: send Pragma and Cache-Control: no-cache */
    u8 unk3;
    char *url;               /* 0x04 */
    char *proxyUrl;          /* 0x08 */
    char *sslProxyUrl;       /* 0x0C */
    char *buf;               /* 0x10: the reply */
    s32 bufSize;             /* 0x14 */
    char *referer;           /* 0x18 */
    char *user;              /* 0x1C */
    char *pass;              /* 0x20 */
    char *proxyUser;         /* 0x24 */
    char *proxyPass;         /* 0x28 */
    s8 method;               /* 0x2C: HTTP_METHOD_* (lbs://: LOBBY_CMD_HTTP_GET, LOBBY_CMD_HTTP_POST) */
    u8 unk2D[0x30 - 0x2D];
    char *postData;          /* 0x30: the form data, '&'-separated */
    s8 toSjis;               /* 0x34: convert an EUC-JP reply to Shift-JIS */
    s8 cancel;               /* 0x35 */
    u8 unk36[0x38 - 0x36];
    s32 len;                 /* 0x38: the reply's length */
    s32 httpErr;             /* 0x3C: the server answered 4xx or 5xx */
    s32 errStep;             /* 0x40 */
    s8 err;                  /* 0x44: the error (0 none); the lobby switches on it (signed) */
    s8 sjis;                 /* 0x45: the reply has Shift-JIS lead bytes */
    u8 state;                /* 0x46: gHttpStateFn's index */
    u8 step;                 /* 0x47 */
    s16 timer;               /* 0x48 */
    s16 unk4A;               /* 0x4A */
    s8 index;                /* 0x4C: the slot in gHttpReq; the lobby indexes its page buffers with it */
    u8 unk4D[0x5C - 0x4D];
    char cnonce[0x10];       /* 0x5C */
    s32 contentLen;          /* 0x6C: the reply's Content-Length, 0 unknown */
    u32 recvLen;             /* 0x70: bytes received so far */
    u8 unk74[0x78 - 0x74];
    HttpStateFn run;         /* 0x78 */
    s8 used;                 /* 0x7C: HTTP_REQ_* */
    s8 async;                /* 0x7D: HTTP_ASYNC_*, written by libhttps' callbacks */
    u8 unk7E[0x80 - 0x7E];
    s32 client;              /* 0x80: the libhttps client */
    HttpUrl *uri;            /* 0x84: url, parsed */
    HttpUrl *proxyUri;       /* 0x88 */
    HttpUrl *sslProxyUri;    /* 0x8C */
    HttpResp *resp;          /* 0x90 */
    HttpAuthReq *auth;       /* 0x94: the server's authentication challenge */
    s32 authInfo;            /* 0x98: the reply Authentication-Info, parsed by libhttps, applied to digest */
    HttpDigest digest;       /* 0x9C */
    s32 refererHdr;          /* 0xD0: the Referer header */
    s32 hdr[4];              /* 0xD4: Pragma, Cache-Control, Content-Type, Content-Length */
};

typedef char HttpDigest_size_check[(sizeof(HttpDigest) == 0x34) ? 1 : -1];
typedef char HttpReq_size_check[(sizeof(HttpReq) == 0xE4) ? 1 : -1];

/* HttpReq.state: the index of its step in gHttpStateFn, named for the
 * handler. */
enum HttpState {
    HTTP_ST_OPEN,               /* 0x00: HttpReq_Open: picks the source by the URL's scheme */
    HTTP_ST_PARSE_URL,          /* 0x01 */
    HTTP_ST_CREATE_CLIENT,      /* 0x02 */
    HTTP_ST_SET_OPTIONS,        /* 0x03 */
    HTTP_ST_CONNECT,            /* 0x04 */
    HTTP_ST_SEND,               /* 0x05 */
    HTTP_ST_READ_BODY,          /* 0x06 */
    HTTP_ST_CHECK_STATUS,       /* 0x07 */
    HTTP_ST_AUTHENTICATE,       /* 0x08 */
    HTTP_ST_CLOSE,              /* 0x09 */
    HTTP_ST_DONE,               /* 0x0A: finished */
    HTTP_ST_ABORT,              /* 0x0B: tears down what was set up, then HTTP_ST_FAILED */
    HTTP_ST_FAILED,             /* 0x0C: finished with err set */
    HTTP_ST_LBS_SEND,           /* 0x0D: lbs://, the lobby server's protocol */
    HTTP_ST_LBS_WAIT,           /* 0x0E */
    HTTP_ST_LBS_DONE,           /* 0x0F */
    HTTP_ST_LBS_ABORT,          /* 0x10 */
    HTTP_ST_LBS_FAILED,         /* 0x11 */
    HTTP_ST_AFS_READ,           /* 0x12: afs://, the game's archive */
    HTTP_ST_AFS_NEXT,           /* 0x13 */
    HTTP_ST_AFS_DONE            /* 0x14 */
};

/* HttpReq.err: why a request failed, named for the step that sets it (the
 * lobby's browser switches on it). */
enum HttpErr {
    HTTP_ERR_NONE,              /* 0 */
    HTTP_ERR_URL,               /* 1: the URL or its source can't be used */
    HTTP_ERR_CLIENT,            /* 2: libhttps' client couldn't be made */
    HTTP_ERR_OPTIONS,           /* 3: an option of the client was refused */
    HTTP_ERR_CONNECT,           /* 4 */
    HTTP_ERR_SEND,              /* 5 */
    HTTP_ERR_TIMEOUT,           /* 6: the connection or the sending took too long */
    HTTP_ERR_NO_REPLY,          /* 7 */
    HTTP_ERR_STATUS,            /* 8: the reply's status couldn't be read */
    HTTP_ERR_NO_CHALLENGE,      /* 9: an authentication without a challenge */
    HTTP_ERR_AUTH_RESET,        /* 0xA: the client couldn't be reset for the authentication */
    HTTP_ERR_CREDENTIALS,       /* 0xB: the credentials couldn't be set */
    HTTP_ERR_AUTH_INFO,         /* 0xC: set on the reply's Authentication-Info */
    HTTP_ERR_FREE,              /* 0xD: libhttps couldn't free the reply */
    HTTP_ERR_NOT_CONNECTED = 0xF, /* 0xF: the network is down */
    HTTP_ERR_CANCELLED = 0x11,  /* 0x11: cancelled, or the lobby server's reply says it failed */
    HTTP_ERR_REDIRECTED,        /* 0x12 */
    HTTP_ERR_NEED_USER,         /* 0x13: HTTP_STATUS_UNAUTHORIZED and no user name to send */
    HTTP_ERR_NEED_PROXY_USER    /* 0x14: HTTP_STATUS_PROXY_AUTH_REQUIRED and no proxy user name to send */
};

/* HttpReq.errStep while a request runs: how far it got, which says what
 * HttpReq_Abort frees (each level also frees those below it). From
 * HttpReq_Abort on, it holds libhttps' error class instead (1-8, 0 none),
 * which the lobby's browser reads. */
enum HttpTeardown {
    HTTP_TEARDOWN_NONE,         /* 0: nothing to free */
    HTTP_TEARDOWN_URL,          /* 1: the parsed URLs */
    HTTP_TEARDOWN_CLIENT,       /* 2: the client, its headers and authentication */
    HTTP_TEARDOWN_CONNECTION,   /* 3: the connection */
    HTTP_TEARDOWN_ALL           /* 4: one more libhttps call (nothing in nethttp sets it) */
};

/* gHttpReq's slots (gHttpReqMax of them are handed out) */
#define HTTP_REQ_SLOTS 8

/* HttpReq.used */
#define HTTP_REQ_FREE 0
#define HTTP_REQ_BUSY 1             /* handed out, running */
#define HTTP_REQ_FINISHED 2         /* done or failed: err says which */

/* HttpReq.method, as the lobby sets it */
#define HTTP_METHOD_GET 1
#define HTTP_METHOD_POST 2

/* libhttps' method codes (its option HTTPS_OPT_METHOD, HttpDigest.method) */
#define HTTPS_METHOD_GET 1
#define HTTPS_METHOD_POST 3

/* HttpReq.async: written by libhttps' completion callbacks
 * (HttpReq_OnConnectDone, HttpReq_OnSendDone) */
#define HTTP_ASYNC_DONE 0
#define HTTP_ASYNC_BUSY 1
#define HTTP_ASYNC_FAILED (-1)

/* HttpReq.step in HttpReq_Connect and HttpReq_Send: after libhttps' operation
 * was stopped, wait for async to leave HTTP_ASYNC_BUSY, then fail */
#define HTTP_STEP_CANCEL_WAIT 10    /* with HTTP_ERR_CANCELLED */
#define HTTP_STEP_TIMEOUT_WAIT 11   /* with HTTP_ERR_TIMEOUT */

/* HttpReq.timer's limits, in frames */
#define HTTP_WAIT_FRAMES 0xE10      /* 60 s for the connection or the reply */
#define HTTP_RECV_FRAMES 0x708      /* 30 s without a new byte of the reply */

/* libhttps' client options (its set-option call), named by
 * what HttpReq_SetOptions and HttpReq_Authenticate pass with them. The
 * code also sets options 7, 8 and 21 to 60, 9 and 22 to 0, 19 to
 * "Content-Length" for POST, and 14 to gHttpsSecureOption for https://;
 * those stay numbers. */
#define HTTPS_OPT_USER_AGENT 0      /* the client's name */
#define HTTPS_OPT_METHOD 2          /* HTTPS_METHOD_* */
#define HTTPS_OPT_URI 3             /* the parsed URL */
#define HTTPS_OPT_PROXY 4           /* the parsed proxy URL */
#define HTTPS_OPT_HEADER 5          /* a header made by libhttps, and how to add it */
#define HTTPS_OPT_POST_DATA 6       /* the body, its length */
#define HTTPS_OPT_SEND_CB 0xA       /* the callback the request's sending ends with */
#define HTTPS_OPT_CONNECT_CB 0x17   /* the callback the connection ends with */

/* HTTP status codes (RFC 2616; 422-424 and 507 are WebDAV's, RFC 4918),
 * the ones HttpReq_CheckStatus handles */
#define HTTP_STATUS_MULTIPLE_CHOICES 300
#define HTTP_STATUS_MOVED_PERMANENTLY 301
#define HTTP_STATUS_FOUND 302
#define HTTP_STATUS_SEE_OTHER 303
#define HTTP_STATUS_USE_PROXY 305
#define HTTP_STATUS_TEMPORARY_REDIRECT 307
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_UNAUTHORIZED 401
#define HTTP_STATUS_PAYMENT_REQUIRED 402
#define HTTP_STATUS_FORBIDDEN 403
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405
#define HTTP_STATUS_NOT_ACCEPTABLE 406
#define HTTP_STATUS_PROXY_AUTH_REQUIRED 407
#define HTTP_STATUS_REQUEST_TIMEOUT 408
#define HTTP_STATUS_CONFLICT 409
#define HTTP_STATUS_GONE 410
#define HTTP_STATUS_LENGTH_REQUIRED 411
#define HTTP_STATUS_PRECONDITION_FAILED 412
#define HTTP_STATUS_ENTITY_TOO_LARGE 413
#define HTTP_STATUS_URI_TOO_LONG 414
#define HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_STATUS_RANGE_NOT_SATISFIABLE 416
#define HTTP_STATUS_EXPECTATION_FAILED 417
#define HTTP_STATUS_UNPROCESSABLE_ENTITY 422
#define HTTP_STATUS_LOCKED 423
#define HTTP_STATUS_FAILED_DEPENDENCY 424
#define HTTP_STATUS_INTERNAL_ERROR 500
#define HTTP_STATUS_NOT_IMPLEMENTED 501
#define HTTP_STATUS_BAD_GATEWAY 502
#define HTTP_STATUS_SERVICE_UNAVAILABLE 503
#define HTTP_STATUS_GATEWAY_TIMEOUT 504
#define HTTP_STATUS_VERSION_NOT_SUPPORTED 505
#define HTTP_STATUS_INSUFFICIENT_STORAGE 507

/* lbs:// requests: HttpLbs_Send's packet in the browser's buf64k, and the
 * scratch area after it where each piece is obfuscated */
#define HTTP_LBS_OBF_OFFSET 0xF000
#define HTTP_LBS_PREFIX_LEN 10      /* an lbs:// URL's characters before the part the server gets */

s32 Http_Init(void); /* nethttp */
s32 Http_Term(void);
HttpReq *HttpReq_Alloc(void);
s32 HttpReq_HasFree(void);
s32 HttpReq_AllIdle(void);
s32 HttpReq_Free(HttpReq *req);
void HttpReq_Step(HttpReq *req);
s32 HttpReq_CheckSjis(HttpReq *req, u8 *text, s32 n);
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
void HttpLbs_Receive(void); /* nethttp */
void HttpLbs_Done(HttpReq *req);
void HttpLbs_Abort(HttpReq *req);
void HttpLbs_Failed(HttpReq *req);
void HttpAfs_Read(HttpReq *req);
void HttpAfs_Next(HttpReq *req);
void HttpAfs_Done(HttpReq *req);
void Http_LoadAfsPartition(void); /* nethttp */
s32 Http_ReadBrdata(void *buf, s32 flid); /* nethttp */
s32 Http_ReadFile(void *buf, s32 flid); /* nethttp */

#endif
