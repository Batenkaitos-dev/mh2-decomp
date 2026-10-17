#ifndef QNET_H
#define QNET_H

#include "common.h"
#include "net_tcp.h"

/*
 * NETWORK: the in-quest protocol (gm_sub qnet.c; preserve byte for byte for
 * online play). One session, gQNet (QNet), over one TCP connection;
 * QNetIf_Start (qnet_if.c) starts it with a QNetConfig.
 */

#define QNET_PEERS 4                /* the session's players, the local one included */

/* A byte queue: QuestNet_Queue appends messages, the senders and readers
 * take whole messages from the front. */
typedef struct QNetBuf {
    s32 len;    /* 0x0: bytes queued */
    s32 cap;    /* 0x4: bytes it can hold */
    u8 *data;   /* 0x8 */
} QNetBuf;

#define QNET_HDR_LEN 4              /* the bytes of a QNetHdr */

/* The 4-byte header of a packet and of each message in it. */
typedef struct QNetHdr {
    u8 len;     /* 0: bytes, header included (never QNET_TEXT_MARK) */
    u8 flags;   /* 1: a packet's channel (QNET_CHAN_*) and QNET_FLAG_STATES; a game message's
                 * QNET_MSG_DROPPABLE */
    u8 from;    /* 2: the sender's index (system messages: the command) */
    u8 seq;     /* 3: the sender's sequence number (0xFF in queued messages) */
} QNetHdr;

/* QNetHdr.flags of a packet: the low nibble is the channel. */
#define QNET_CHAN_MASK 0x0F
#define QNET_CHAN_SYS 1             /* system messages (QNetSysMsg) */
#define QNET_CHAN_GAME 2            /* the game's messages (qnet_sync.c's QSyncHead) */
#define QNET_FLAG_MASK 0xF0
#define QNET_FLAG_STATES 0x10       /* the sender's view of the QNET_PEERS peer states follows the header */
/* QNetHdr.flags of a queued game message: QuestNet_Send drops it when the
 * socket's send space is short (QNET_SEND_SPACE_LOW). */
#define QNET_MSG_DROPPABLE 1

#define QNET_SEQ_NONE 0xFF          /* QNetHdr.seq of a queued message */

/* A text header starts with this byte (QuestNet_Parse skips such a header and
 * its body); a packet's length is never this value. */
#define QNET_TEXT_MARK '('

/* QuestNet_Send's packet sizes against the socket's send space
 * (NetTcpStatus.sendSpace). */
#define QNET_PACKET_MAX 0xFF        /* a packet's bytes, header included */
#define QNET_SEND_RESERVE 0x1E2A    /* send space a packet leaves free */
#define QNET_SEND_SPACE_LOW 0x2000  /* below it, droppable messages are dropped */
#define QNET_SEND_SPACE_IDLE 0x1FFC /* at or above it the link is not stalled (QuestNet_EverySecond) */
#define QNET_RECV_MAX 0x384         /* bytes QuestNet_Recv reads at a time */

/* QNetSysMsg.cmd */
enum QNetSysCmd {
    QNET_SYS_ALIVE = 1,         /* the join's beacon, once a second; answered once */
    QNET_SYS_READY = 2,         /* the sender is ready, with its timeout */
    QNET_SYS_UNREADY = 3,
    QNET_SYS_LEAVE = 4,         /* the sender leaves the quest */
    QNET_SYS_TIME = 5,          /* the sender's clock, twice a second during the join */
    QNET_SYS_TIME_REPLY = 6     /* a QNET_SYS_TIME sent back: the asker's ping */
};

/* A system message (channel QNET_CHAN_SYS): QuestNet_Sys* queue them in
 * gQNetSysSend, QuestNet_OnSysMsg handles them. The header's third byte is
 * the command and its seq is 0xFF. */
typedef struct QNetSysMsg {
    u8 len;     /* 0: 4; 5 (ready), 8 (time), 9 (time reply) */
    u8 flags;   /* 1: 0 */
    u8 cmd;     /* 2: enum QNetSysCmd */
    u8 seq;     /* 3: 0xFF */
    union {
        u8 ready;       /* 4: QNET_SYS_READY: the sender's ready timeout in seconds (QNet.readyTimeout) */
        struct {
            f32 time;   /* 4: QNET_SYS_TIME: the sender's clock (QNet.now); QNET_SYS_TIME_REPLY: the clock it answers */
            u8 to;      /* 8: QNET_SYS_TIME_REPLY: the player that asked (its ping is now - time) */
        } t;
    } u;
} QNetSysMsg;
typedef char QNetSysMsg_size_check[(sizeof(QNetSysMsg) == 0xC) ? 1 : -1];

/* QNet.state, stepped by QuestNet_Update each frame. */
enum QNetState {
    QNET_STATE_INIT,            /* 0: QuestNet_Init's */
    QNET_STATE_INIT2,           /* 1: a frame on to 2 */
    QNET_STATE_OPEN,            /* 2: starts the clocks, then the join */
    QNET_STATE_JOIN,            /* 3: until every peer has joined (QuestNet_UpdateJoin) */
    QNET_STATE_RUN,             /* 4: the session runs */
    QNET_STATE_UNREADY,         /* 5: unreadies every peer, back to the join */
    QNET_STATE_LEAVE,           /* 6: the local player has left (QuestNet_Leave) */
    QNET_STATE_ERROR,           /* 7: a socket error ended it */
    QNET_STATE_CLOSED           /* 8: QuestNet_Close */
};

/* QNet.peerState[]: each player's state, the local one included. Each
 * packet with QNET_FLAG_STATES carries the sender's view of them. */
enum QNetPeerState {
    QNET_PEER_NONE = 0,         /* no player, or the session closed */
    QNET_PEER_JOINING = 1,      /* QuestNet_Init's: no alive answered yet */
    QNET_PEER_ALIVE = 2,        /* answered an alive; its ping is measured */
    QNET_PEER_JOINED = 3,       /* in the session */
    QNET_PEER_READY = 4,        /* ready, until its timeout (QNet.readyTimeout) */
    QNET_PEER_LEFT = 5,         /* left the quest (QNet.onLeave) */
    QNET_PEER_LOST = 6          /* silent too long or dropped by the others (QNet.onLost) */
};

/* The time replies that make a peer's ping: the one that reaches this count
 * sets gQNetPing (QuestNet_OnSysMsg). */
#define QNET_PING_SAMPLES 3

/* QuestNet_Parse's states, over a stream of packets and text headers. */
enum QNetParseState {
    QNET_PARSE_START = 0,       /* the first byte: QNET_TEXT_MARK starts a text header */
    QNET_PARSE_TEXT_HEAD = 1,   /* the text header's 12 bytes, its body's length at 4 */
    QNET_PARSE_TEXT_BODY = 2,   /* skips the text header and its body */
    QNET_PARSE_HEAD = 3,        /* a packet's header */
    QNET_PARSE_BODY = 4         /* the packet is whole: its length */
};

/* The bytes of QNet's join counters (joinSecond to joinHalfSecond), which
 * the join clears with memset. */
#define QNET_JOIN_COUNTERS 8

/* QNet.mode: QuestNet_Tick steps the session only in this mode, the one
 * QNetIf_Start sets. */
#define QNET_MODE_STEPPED 1

/* The errors (QuestNet_GetError; the error functions return them negated). */
enum QNetError {
    QNET_ERR_NONE = 0,
    QNET_ERR_SEND = 1,          /* NetTcp_Send failed */
    QNET_ERR_QUEUE_FULL = 2,    /* a send queue is full (QuestNet_Queue) */
    QNET_ERR_RECV = 3,          /* NetTcp_Recv read less than was waiting */
    QNET_ERR_RECV_FULL = 4,     /* a receive queue is full */
    QNET_ERR_BAD_MSG = 5,       /* a message shorter than its header, or an unknown channel */
    QNET_ERR_NOT_CONNECTED = 6, /* the TCP connection's state is not connected */
    QNET_ERR_LOST = 7,          /* the local player is QNET_PEER_LOST */
    QNET_ERR_STALLED = 9,       /* the send space stayed short for maxStall seconds */
    QNET_ERR_STATUS = 10,       /* NetTcp_Status failed */
    QNET_ERR_NETIF = 11,        /* the network interface reports an error (NetIf_GetError) */
    QNET_ERR_HANDLER = 12       /* a message's handler failed */
};

/* QuestNet_CanSend's results. */
enum QNetSendBlock {
    QNET_CAN_SEND = 0,
    QNET_SEND_NOT_JOINED = 1,   /* the local player is not joined or ready */
    QNET_SEND_OVER_BUDGET = 2,  /* sendBudget bytes sent this second */
    QNET_SEND_TOO_SOON = 3      /* less than sendInterval since the last send */
};

typedef s32 (*QNetMsgFunc)(u8, u8 *, s32);
typedef void (*QNetPeerFunc)(u8, s32, u8);

/* The session (gQNet, 0xD0 bytes). Times are seconds of the session's clock
 * (now, QuestNet_Clock). */
typedef struct QNet {
    s16 joinSecond;         /* 0x00: the join's whole seconds: each one sends an alive (QuestNet_UpdateJoin) */
    s16 nPeersUp;           /* 0x02: the peers joining or alive, counted by QuestNet_UpdateJoin (all of them pinged: joined) */
    s16 nPeersPinged;       /* 0x04: the peers alive with a ping (gQNetPing), counted by QuestNet_UpdateJoin */
    s16 joinHalfSecond;     /* 0x06: the join's half seconds: each one sends the time */
    s32 sock;               /* 0x08 */
    s32 active;             /* 0x0C: 1 from QuestNet_Init to QuestNet_Close */
    u8 mode;                /* 0x10: QNET_MODE_STEPPED */
    u8 state;               /* 0x11: QNET_STATE_* */
    u8 count;               /* 0x12: players, up to QNET_PEERS */
    u8 self;                /* 0x13: our index */
    u32 aliveMask;          /* 0x14: a bit per peer whose alive was answered (QuestNet_OnSysMsg) */
    u8 peerState[QNET_PEERS]; /* 0x18: enum QNetPeerState */
    f32 lastStatesTime;     /* 0x1C: when the peer states were last sent (QNET_FLAG_STATES) */
    s64 clock;              /* 0x20: the last clock reading (QuestNet_Clock) */
    f32 now;                /* 0x28: the session's clock */
    f32 lastTime;           /* 0x2C: the time of the last step (now when set, QuestNet_Update); the joins wait from it (QuestNet_UpdateJoin) */
    f32 lastTotalsTime;     /* 0x30: when sentTotal and receivedTotal were last cleared (each minute) */
    s32 unk34;              /* 0x34 */
    s32 unk38;              /* 0x38 */
    s32 sent;               /* 0x3C: bytes sent this second */
    s32 received;           /* 0x40: bytes received this second */
    s32 sentLastSec;        /* 0x44: bytes sent in the last whole second (QuestNet_EverySecond) */
    s32 receivedLastSec;    /* 0x48 */
    s32 sentTotal;          /* 0x4C: bytes sent since lastTotalsTime */
    s32 receivedTotal;      /* 0x50 */
    f32 sendInterval;       /* 0x54: seconds between game sends (QNetConfig) */
    f32 lastSendTime;       /* 0x58: now at the last send (QuestNet_CanSend waits sendInterval from it) */
    s32 sendBudget;         /* 0x5C: bytes a second: no game send once sent reaches it */
    NetTcpStatus tcp;       /* 0x60: the connection's state (NetTcp_Status) */
    f32 peerTime[QNET_PEERS]; /* 0x68: when each peer was last heard from */
    f32 readyTime[QNET_PEERS]; /* 0x78: when each peer became ready */
    u16 prevSendSpace;      /* 0x88: the send space a second ago (QuestNet_EverySecond) */
    u16 stallSecs;          /* 0x8A: seconds the send space has stayed short */
    u8 firstSeq[QNET_PEERS]; /* 0x8C: each peer's first sequence number seen (QuestNet_CheckSeq) */
    u8 seq[QNET_PEERS];     /* 0x90: each player's last sequence number */
    s32 recvError;          /* 0x94: QNET_ERR_* (QuestNet_RecvError) */
    s32 sendError;          /* 0x98: QNET_ERR_* (QuestNet_SendError) */
    s32 linkError;          /* 0x9C: QNET_ERR_* (QuestNet_LinkError) */
    s32 errDetail;          /* 0xA0: the last error's detail: a result or a length */
    f32 peerTimeout;        /* 0xA4: seconds of silence after which a peer is lost (QuestNet_CheckTimeouts) */
    f32 joinTimeout;        /* 0xA8: seconds the join waits (QuestNet_UpdateJoin); QuestNet_Ready's default argument */
    u8 readyTimeout[QNET_PEERS]; /* 0xAC: seconds each ready peer may stay ready (QNET_SYS_READY) */
    s32 maxStall;           /* 0xB0: seconds the TCP send space may stay short before QNET_ERR_STALLED (QuestNet_EverySecond) */
    QNetMsgFunc onMsg;      /* 0xB4: a game message arrived */
    s32 onMsgArg;           /* 0xB8 */
    QNetPeerFunc onLeave;   /* 0xBC: a player became QNET_PEER_LEFT */
    s32 onLeaveArg;         /* 0xC0 */
    QNetPeerFunc onLost;    /* 0xC4: a player became QNET_PEER_LOST */
    s32 onLostArg;          /* 0xC8 */
    s32 unkCC;              /* 0xCC */
} QNet;
typedef char QNet_size_check[(sizeof(QNet) == 0xD0) ? 1 : -1];

/* QuestNet_Init's argument (built by QNetIf_Start). */
typedef struct QNetConfig {
    u8 mode;                /* 0x00: -> QNet.mode */
    u8 count;               /* 0x01: -> QNet.count */
    u8 self;                /* 0x02: -> QNet.self */
    s32 sock;               /* 0x04: -> QNet.sock */
    f32 sendInterval;       /* 0x08: -> QNet.sendInterval */
    s32 sendBudget;         /* 0x0C: -> QNet.sendBudget */
    f32 peerTimeout;        /* 0x10: -> QNet.peerTimeout */
    f32 maxStall;           /* 0x14: -> QNet.maxStall */
    f32 joinTimeout;        /* 0x18: -> QNet.joinTimeout */
    QNetMsgFunc onMsg;      /* 0x1C */
    s32 onMsgArg;           /* 0x20 */
    QNetPeerFunc onLeave;   /* 0x24 */
    s32 onLeaveArg;         /* 0x28 */
    QNetPeerFunc onLost;    /* 0x2C */
    s32 onLostArg;          /* 0x30 */
} QNetConfig;

#endif /* QNET_H */
