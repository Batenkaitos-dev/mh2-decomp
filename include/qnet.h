#ifndef QNET_H
#define QNET_H

#include "common.h"

/*
 * NETWORK: the in-quest protocol (gm_sub qnet.c; preserve byte for byte for
 * online play). One session, gQNet (QNet), over one TCP connection;
 * QNetIf_Start (qnet_if.c) starts it with a QNetConfig.
 */
typedef struct QNetBuf {
    s32 len;    /* 0x0 */
    s32 cap;    /* 0x4 */
    u8 *data;   /* 0x8 */
} QNetBuf;

/* The 4-byte header of a packet and of each message in it. */
typedef struct QNetHdr {
    u8 len;     /* 0: bytes, header included (40 is never used: '(' starts a text header) */
    u8 flags;   /* 1: low nibble the channel (1 system, 2 game); 0x10: peer states follow */
    u8 from;    /* 2: the sender's index (system messages: the command) */
    u8 seq;     /* 3: the sender's sequence number (0xFF in queued messages) */
} QNetHdr;

/* A system message (channel 1): QuestNet_Sys* queue them in gQNetSysSend,
 * QuestNet_OnSysMsg handles them. The header's third byte is the command
 * and its seq is 0xFF. */
typedef struct QNetSysMsg {
    u8 len;     /* 0: 4; 5 (cmd 2), 8 (cmd 5), 9 (cmd 6) */
    u8 flags;   /* 1: 0 */
    u8 cmd;     /* 2: 1 alive, 2 ready, 3 unready, 4 leave, 5 time, 6 time reply */
    u8 seq;     /* 3: 0xFF */
    union {
        u8 ready;       /* 4: cmd 2: QuestNet_SysReady's argument (QNet.unkAC[from]) */
        struct {
            f32 time;   /* 4: cmd 5: the sender's clock (QNet.now); cmd 6: the clock it answers */
            u8 to;      /* 8: cmd 6: the player that asked (its ping is now - time) */
        } t;
    } u;
} QNetSysMsg;
typedef char QNetSysMsg_size_check[(sizeof(QNetSysMsg) == 0xC) ? 1 : -1];

typedef s32 (*QNetMsgFunc)(u8, u8 *, s32);
typedef void (*QNetPeerFunc)(u8, s32, u8);

typedef struct QNet {
    s16 unk0;               /* 0x00 */
    s16 nPeersUp;           /* 0x02: the peers in state 1 or 2, counted by QuestNet_UpdateJoin (all of them pinged: state 3) */
    s16 nPeersPinged;       /* 0x04: the peers in state 2 with a ping (gQNetPing), counted by QuestNet_UpdateJoin */
    s16 unk6;               /* 0x06 */
    s32 sock;               /* 0x08 */
    s32 active;             /* 0x0C */
    u8 mode;                /* 0x10 */
    u8 state;               /* 0x11 */
    u8 count;               /* 0x12: players */
    u8 self;                /* 0x13: our index */
    u32 unk14;              /* 0x14 */
    u8 peer[4];             /* 0x18: each player's state */
    f32 unk1C;              /* 0x1C */
    s64 clock;              /* 0x20: the last clock reading (QuestNet_Clock) */
    f32 now;                /* 0x28 */
    f32 lastTime;           /* 0x2C: the time of the last step (now when set, QuestNet_Update); the joins wait from it (QuestNet_UpdateJoin) */
    f32 unk30;              /* 0x30 */
    s32 unk34;              /* 0x34 */
    s32 unk38;              /* 0x38 */
    s32 sent;               /* 0x3C: bytes sent */
    s32 received;           /* 0x40: bytes received */
    s32 unk44;              /* 0x44 */
    s32 unk48;              /* 0x48 */
    s32 unk4C;              /* 0x4C */
    s32 unk50;              /* 0x50 */
    f32 unk54;              /* 0x54 */
    f32 unk58;              /* 0x58 */
    s32 unk5C;              /* 0x5C */
    s16 tcpState;           /* 0x60: NetTcp_Status fills 0x60-0x67 */
    s16 unk62;              /* 0x62 */
    u16 sendSpace;          /* 0x64 */
    u16 recvLen;            /* 0x66 */
    f32 peerTime[4];        /* 0x68 */
    f32 unk78[4];           /* 0x78 */
    u16 unk88;              /* 0x88 */
    u16 unk8A;              /* 0x8A */
    u8 unk8C[4];            /* 0x8C */
    u8 seq[4];              /* 0x90 */
    s32 err94;              /* 0x94 */
    s32 err98;              /* 0x98 */
    s32 err9C;              /* 0x9C */
    s32 errDetail;          /* 0xA0 */
    f32 unkA4;              /* 0xA4 */
    f32 unkA8;              /* 0xA8 */
    u8 unkAC[4];            /* 0xAC */
    s32 unkB0;              /* 0xB0 */
    QNetMsgFunc onMsg;      /* 0xB4 */
    s32 onMsgArg;           /* 0xB8 */
    QNetPeerFunc onLeave;   /* 0xBC: a player's state became 5 */
    s32 onLeaveArg;         /* 0xC0 */
    QNetPeerFunc onLost;    /* 0xC4: a player's state became 6 */
    s32 onLostArg;          /* 0xC8 */
    s32 unkCC;              /* 0xCC */
} QNet;

/* QuestNet_Init's argument (built by QNetIf_Start). */
typedef struct QNetConfig {
    u8 mode;                /* 0x00 */
    u8 count;               /* 0x01 */
    u8 self;                /* 0x02 */
    s32 sock;               /* 0x04 */
    f32 unk8;               /* 0x08 */
    s32 unkC;               /* 0x0C */
    f32 unk10;              /* 0x10 */
    f32 unk14;              /* 0x14 */
    f32 unk18;              /* 0x18 */
    QNetMsgFunc onMsg;      /* 0x1C */
    s32 onMsgArg;           /* 0x20 */
    QNetPeerFunc onLeave;   /* 0x24 */
    s32 onLeaveArg;         /* 0x28 */
    QNetPeerFunc onLost;    /* 0x2C */
    s32 onLostArg;          /* 0x30 */
} QNetConfig;

#endif /* QNET_H */
