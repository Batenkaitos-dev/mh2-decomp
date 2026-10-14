#ifndef NETHTTP_H
#define NETHTTP_H

#include "common.h"

/* nethttp's request object and the views of libhttps' structs it reads (nethttp's
 * front end, src/nethttp/unk_8FB130.c; the lobby fills and reads requests too,
 * src/lobby/unk_722360.c). libhttps' structs are only the fields this code reads:
 * no size checks for them. */

/* A reply as libhttps returns it (func_00900548) */
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

/* A parsed URL (func_009006F8) */
typedef struct HttpUrl {
    u8 unk0[0x14];
    char *path;              /* 0x14 */
} HttpUrl;

/* An authentication challenge (func_00905ED0) */
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

/* A digest authentication's answer (func_00906070) */
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
    s32 unk28;               /* 0x28 */
    s32 unk2C;               /* 0x2C */
    s32 unk30;               /* 0x30 */
} HttpDigest;

typedef struct HttpReq HttpReq;
typedef void (*HttpStateFn)(HttpReq *req);

/* One request (0xE4 bytes): HttpReq_Alloc hands one out, the caller fills it (the lobby's
 * NameConn_Setup), HttpReq_Step runs it, HttpReq_Free returns it. err, sjis and index are
 * s8: nethttp only writes them, the lobby (unk_722360.c) reads them signed. */
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
    s8 method;               /* 0x2C: 1 GET, 2 POST (lbs://: 0x101 plain, 0x102 obfuscated) */
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
    s8 used;                 /* 0x7C: 1 busy, 2 done */
    s8 async;                /* 0x7D: libhttps' callbacks: 1 busy, 0 done, -1 failed */
    u8 unk7E[0x80 - 0x7E];
    s32 client;              /* 0x80: the libhttps client */
    HttpUrl *uri;            /* 0x84: url, parsed */
    HttpUrl *proxyUri;       /* 0x88 */
    HttpUrl *sslProxyUri;    /* 0x8C */
    HttpResp *resp;          /* 0x90 */
    HttpAuthReq *auth;       /* 0x94: the server's authentication challenge */
    s32 authInfo;            /* 0x98: the reply Authentication-Info, parsed (func_00906400), applied to digest */
    HttpDigest digest;       /* 0x9C */
    s32 refererHdr;          /* 0xD0: the Referer header */
    s32 hdr[4];              /* 0xD4: Pragma, Cache-Control, Content-Type, Content-Length */
};

typedef char HttpDigest_size_check[(sizeof(HttpDigest) == 0x34) ? 1 : -1];
typedef char HttpReq_size_check[(sizeof(HttpReq) == 0xE4) ? 1 : -1];

#endif
