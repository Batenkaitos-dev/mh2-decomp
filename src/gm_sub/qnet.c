/* NETWORK: the in-quest protocol (preserve byte for byte for online play).
 *
 * One session, gQNet (QNet, 0xD0 bytes), over one TCP connection
 * (NetTcp_*). Packets are a 4-byte header {len, flags, sender, seq} and
 * sub-messages of the same layout; the bytes after the header are
 * scrambled with a running XOR. Queues (QNetBuf): gQNetSysSend system
 * messages to send, gQNetSysRecv system messages received, gQNetGameSend game
 * messages to send, gQNetGameRecv game messages received, gQNetRecv the raw
 * receive buffer. */
#include "common.h"
#include "qnet.h"

/* .bss, declared from the highest address down (MWCC emits the last
 * declared first). */
QNet gQNet;
QNetBuf gQNetRecv;              /* the TCP receive buffer */
QNetBuf gQNetGameRecv;          /* game messages received */
QNetBuf gQNetGameSend;          /* game messages to send */
QNetBuf gQNetSysRecv;           /* system messages received */
QNetBuf gQNetSysSend;           /* system messages to send */
u8 gQNetRecvData[0x400];
u8 gQNetGameRecvData[0x4000];
u8 gQNetGameSendData[0x1000];
u8 gQNetSysRecvData[0x400];
u8 gQNetSysSendData[0x400];
u8 gQNetMsg[0x200];             /* a packet or message being built */
f32 gQNetPing[4];
u8 gQNetPingCounts[4];

void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
struct NetTcpStatus;
s32 NetTcp_Status(s32 sock, struct NetTcpStatus *);
s32 NetTcp_Recv(s32, void *, s16);
s32 NetTcp_Send(s32, void *, s16);
void *memmove(void *dst, const void *src, u32 n);
s64 GetTimerSystemTime(void);
void TimerBusClock2USec(s64, u32 *, u32 *);
/* unit's own declaration: with the definition's u16 (s32 x) (sub_main/unk_30E420.c), QuestNet_Parse: 45 of 124 words differ */
u16 Swap16(u16);
s32 rand(void);
s32 NetIf_GetError(void);
s32 QuestNet_Send(QNet *, QNetBuf *, u8);
s32 QuestNet_Recv(QNet *);
s32 QuestNet_Parse(QNet *, u8 *, s32, QNetHdr *);
s32 QuestNet_Dispatch(QNet *, u8, u8, u8 *, u8);
s32 QuestNet_OnSysMsg(QNet *, u8, QNetSysMsg *);
s32 QuestNet_ProcessQueue(QNet *, QNetBuf *);
void QuestNet_Update(QNet *);
s32 QuestNet_GetError(void);
s32 QuestNet_CanSend(void);

void QuestNet_Clock(f32 *t, s64 *last) {
    u32 sec;
    u32 usec;
    s64 now;

    if (*last == 0) {
        *last = GetTimerSystemTime();
        *t = 0.0f;
    } else {
        now = GetTimerSystemTime();
        if (now - *last != 0) {
            TimerBusClock2USec(now - *last, &sec, &usec);
            *t += (f32)sec + 0.000001f * (f32)usec;
            *last = now;
        }
    }
}

s32 QuestNet_SendError(QNet *q, s32 err, s32 detail) {
    q->err98 = err;
    q->errDetail = detail;
    return -err;
}

s32 QuestNet_RecvError(QNet *q, s32 err, s32 detail) {
    q->err94 = err;
    q->errDetail = detail;
    gQNetGameRecv.len = 0;
    gQNetSysRecv.len = 0;
    return -err;
}

s32 QuestNet_LinkError(QNet *q, s32 err, s32 detail) {
    q->err9C = err;
    q->errDetail = detail;
    return -err;
}

s32 QuestNet_Loopback(QNet *q, u8 *data, s32 len, u8 type) {
    QNetHdr hdr;
    QNetBuf *buf = &gQNetGameRecv;

    if (buf->cap < buf->len + 4 + len) {
        return QuestNet_SendError(q, 4, buf->len + 4 + len);
    }
    hdr.len = (len + 4 == 0x28) ? len + 5 : len + 4;
    hdr.flags = type;
    hdr.from = q->self;
    hdr.seq = 0xFF;
    memcpy(buf->data + buf->len, &hdr, 4);
    memcpy(buf->data + (buf->len + 4), data, len);
    buf->len += hdr.len;
    return 0;
}

/* Sends what fits of a queue (1 system, 2 game) as one packet: the header,
 * the peer states every second (flag 0x10), then whole messages; drops
 * messages flagged 1 when the socket has less than 0x2000 bytes free.
 * The `switch` on the send result, with `default:` first (-1: try again
 * later), is what leaves the delay slots of every conditional branch of the
 * function empty, as in the original: with an `if` there, MWCC fills them
 * from the fall-through blocks. */
s32 QuestNet_Send(QNet *q, QNetBuf *queue, u8 type) {
    s32 pos;
    s32 space;
    s32 len;
    u8 *msg;
    u8 *p;
    s32 send;
    s32 drop;
    s32 t;
    s32 r;
    s32 j;
    s32 i;
    s32 n;
    QNetHdr hdr;

    r = NetTcp_Status(q->sock, (struct NetTcpStatus *)&q->tcpState);
    if (r < 0) {
        return QuestNet_SendError(q, 10, r);
    }
    if (q->tcpState != 4) {
        return QuestNet_SendError(q, 6, q->tcpState);
    }
    drop = 0;
    if (q->sendSpace < 0x2000) {
        drop = 1;
    }
    t = type;
    if (t == 2) {
        send = 0;
        for (i = 0; i < q->count; i++) {
            if (i != q->self && q->peer[i] != 6 && q->peer[i] != 5 && q->peer[i] != 0) {
                send = 1;
                break;
            }
        }
    } else {
        send = 1;
    }
    space = 0xFF;
    if (q->sendSpace - 0xFF < 0x1E2B) {
        space = q->sendSpace - 0x1E2A;
    }
    if (space < 4) {
        return 0;
    }
    space -= 4;
    len = 4;
    msg = (u8 *)&gQNetMsg;
    p = gQNetMsg + 4;
    msg[1] = t & 0xF;
    msg[2] = q->self;
    if ((1.0f + q->unk1C < q->now && 0 < queue->len) || 2.0f + q->unk1C < q->now) {
        if (space < 4) {
            return 0;
        }
        space -= 4;
        for (i = 0; i < q->count; i++) {
            p[i] = q->peer[i];
        }
        len += 4;
        p += 4;
        msg[1] |= 0x10;
    }
    pos = 0;
    while (0 < space && pos < queue->len) {
        memcpy(&hdr, queue->data + pos, 4);
        if (space < hdr.len) {
            break;
        }
        if (drop && hdr.flags == 1) {
            pos += hdr.len;
        } else {
            memcpy(p, queue->data + pos, hdr.len);
            space -= hdr.len;
            len += hdr.len;
            p += hdr.len;
            pos += hdr.len;
        }
    }
    if (len == 4) {
        return 0;
    }
    msg[3] = ++q->seq[q->self];
    msg[0] = (len == 0x28) ? len + 1 : len;
    n = msg[0];
    for (j = 0; j < n - 4; j++) {
        if (j == 0) {
            msg[4] ^= (u8)(((QNetHdr *)msg)->seq ^ (((QNetHdr *)msg)->len ^ ((QNetHdr *)msg)->from));
        } else {
            *(u8 *)(j + (u32)msg + 4) ^= (u8)(j ^ *(u8 *)(j + (u32)msg + 3));
        }
    }
    if (send == 1 || (msg[1] & 0xF0)) {
        r = NetTcp_Send(q->sock, msg, n);
        switch (r) {
        default:
            if (r < 0) {
                return QuestNet_SendError(q, 1, r);
            }
            break;
        case -1:
            break;
        }
        q->sent += n;
        q->unk58 = q->now;
        if (msg[1] & 0x10) {
            q->unk1C = q->now;
        }
    }
    if (pos != 0) {
        if (t == 2) {
            QuestNet_Loopback(q, queue->data, pos, type);
        }
        n = queue->len - pos;
        if (0 < n) {
            memmove(queue->data, queue->data + pos, n);
        }
        queue->len = n;
    }
    return pos;
}

void QuestNet_SetPeerState(QNet *q, u8 peer, u8 state) {
    s32 old = q->peer[peer];

    q->peer[peer] = state;
    if (old != 5 && state == 5) {
        if (peer != q->self) {
            q->onLeave(peer, q->onLeaveArg, state);
        }
    } else if (old != 6 && state == 6 && peer != q->self) {
        q->onLost(peer, q->onLostArg, state);
    }
}

void QuestNet_DropPeers(QNet *q, u8 *states) {
    s32 i;

    for (i = 0; i < q->count; i++) {
        if (q->peer[i] != 6 && states[i] == 6) {
            QuestNet_SetPeerState(q, i, 6);
        }
    }
}

s32 QuestNet_Queue(QNet *q, QNetBuf *buf, void *msg) {
    QNetHdr *h = msg;

    if (buf->cap < buf->len + h->len) {
        return QuestNet_SendError(q, 2, buf->len + h->len);
    }
    memcpy(buf->data + buf->len, msg, h->len);
    buf->len += h->len;
    return h->len;
}

s32 QuestNet_SysAlive(QNet *q) {
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = 4;
    msg->flags = 0;
    msg->cmd = 1;
    msg->seq = 0xFF;
    return QuestNet_Queue(q, &gQNetSysSend, msg);
}

s32 QuestNet_SysReady(QNet *q, u8 arg) {
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = 5;
    msg->flags = 0;
    msg->cmd = 2;
    msg->seq = 0xFF;
    msg->u.ready = arg;
    return QuestNet_Queue(q, &gQNetSysSend, msg);
}

s32 QuestNet_SysUnready(QNet *q) {
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = 4;
    msg->flags = 0;
    msg->cmd = 3;
    msg->seq = 0xFF;
    return QuestNet_Queue(q, &gQNetSysSend, msg);
}

s32 QuestNet_SysLeave(QNet *q) {
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = 4;
    msg->flags = 0;
    msg->cmd = 4;
    msg->seq = 0xFF;
    return QuestNet_Queue(q, &gQNetSysSend, msg);
}

s32 QuestNet_SysTime(QNet *q) {
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = 8;
    msg->flags = 0;
    msg->cmd = 5;
    msg->seq = 0xFF;
    msg->u.t.time = q->now;
    return QuestNet_Queue(q, &gQNetSysSend, msg);
}

s32 QuestNet_SysTimeReply(QNet *q, f32 time, u8 arg) {
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = 9;
    msg->flags = 0;
    msg->cmd = 6;
    msg->seq = 0xFF;
    msg->u.t.time = time;
    msg->u.t.to = arg;
    return QuestNet_Queue(q, &gQNetSysSend, msg);
}

s32 QuestNet_OnSysMsg(QNet *q, u8 from, QNetSysMsg *msg) {
    s32 i;
    f32 *t;

    switch (msg->cmd) {
    case 1:
        if (q->peer[q->self] == 1 && q->peer[from] == 1) {
            QuestNet_SetPeerState(q, from, 2);
            QuestNet_SysAlive(q);
            q->unk14 |= 1 << from;
        }
        break;
    case 2:
        i = from;
        if (q->peer[i] == 3) {
            QuestNet_SetPeerState(q, from, 4);
            q->unk78[i] = q->now;
            q->unkAC[i] = msg->u.ready;
        }
        break;
    case 3:
        i = from;
        if (q->peer[i] == 4) {
            QuestNet_SetPeerState(q, from, 3);
            q->unk78[i] = 0.0f;
            q->unkAC[i] = 0;
        }
        break;
    case 4:
        if (q->peer[from] == 3) {
            QuestNet_SetPeerState(q, from, 5);
        }
        break;
    case 5:
        QuestNet_SysTimeReply(q, msg->u.t.time, from);
        break;
    case 6:
        t = &msg->u.t.time;
        if (msg->u.t.to == q->self) {
            if (gQNetPing[from] == 0.0f) {
                gQNetPingCounts[from]++;
                if (gQNetPingCounts[from] >= 3) {
                    gQNetPing[from] = q->now - *t;
                }
            }
        }
        break;
    default:
        return 0;
    }
    q->peerTime[from] = q->now;
    return 0;
}

s32 QuestNet_OnGameMsg(QNet *q, u8 from, u8 *msg) {
    return q->onMsg(from, msg, q->onMsgArg);
}

s32 QuestNet_OnPacket(QNet *q, u8 peer, u8 flags, u8 *states) {
    u8 st;

    if (q->count <= peer) {
        return 0;
    }
    st = q->peer[peer];
    switch (st) {
    case 0:
    case 6:
        return 0;
    }
    if (flags & 0x10) {
        if (st == 3 || st == 4) {
            QuestNet_DropPeers(q, states);
        }
        q->peerTime[peer] = q->now;
    }
    return 0;
}

s32 QuestNet_Dispatch(QNet *q, u8 from, u8 flags, u8 *data, u8 len) {
    u8 *msg = (u8 *)&gQNetMsg;
    s32 pos = 0;
    s32 r;
    QNetHdr hdr;

    if (q->count <= from) {
        return 0;
    }
    switch (q->peer[from]) {
    case 0:
    case 5:
    case 6:
        return 0;
    }
    if (flags & 0xF0) {
        if (len == 4) {
            return 0;
        }
        data += 4;
        len -= 4;
    }
    while (pos < len) {
        if (len - pos < 4) {
            break;
        }
        memcpy(&hdr, data + pos, 4);
        if (hdr.len < 4) {
            return QuestNet_RecvError(q, 5, hdr.len);
        }
        memcpy(msg, data + pos, hdr.len);
        switch (flags & 0xF) {
        case 0:
            break;
        case 1:
            r = QuestNet_OnSysMsg(q, from, (QNetSysMsg *)msg);
            if (r < 0) {
                return QuestNet_RecvError(q, 12, r);
            }
            break;
        case 2:
            r = QuestNet_OnGameMsg(q, from, msg);
            if (r < 0) {
                return QuestNet_RecvError(q, 12, r);
            }
            break;
        default:
            return QuestNet_RecvError(q, 5, flags);
        }
        pos += hdr.len;
    }
    return 0;
}

s32 QuestNet_CheckSeq(QNet *q, QNetHdr *h) {
    if (q->count <= h->from) {
        return 0;
    }
    if (q->self == h->from) {
        return 0;
    }
    if (q->unk8C[h->from] == 0) {
        q->unk8C[h->from] = h->seq;
    } else {
        q->seq[h->from]++;
    }
    q->seq[h->from] = h->seq;
    return 0;
}

s32 QuestNet_Parse(QNet *q, u8 *data, s32 len, QNetHdr *out) {
    QNetHdr hdr;
    u8 text[12];
    s32 state = 0;
    s32 n;

    while (0 < len) {
        switch (state) {
        case 0:
            if (len <= 0) {
                return 0;
            }
            len--;
            if (*data == '(') {
                state = 1;
            } else {
                state = 3;
            }
            break;
        case 1:
            if (len < sizeof(text) - 1) {
                return 0;
            }
            len -= 11;
            memcpy(text, data, 12);
            n = Swap16(*(u16 *)&text[4]);
            if (n == 0) {
                return -12;
            }
            state = 2;
            break;
        case 2:
            return (len < n) ? 0 : (n + sizeof(text)) * -1;
        case 3:
            if (len < 3) {
                return 0;
            }
            len -= 3;
            memcpy(&hdr, data, 4);
            if (hdr.len < 4) {
                QuestNet_RecvError(q, 5, hdr.len);
                return 0;
            }
            n = hdr.len - 4;
            if (n == 0) {
                return -4;
            }
            state = 4;
            break;
        case 4:
            if (len < n) {
                return 0;
            }
            if (out != NULL) {
                memcpy(out, &hdr, 4);
            }
            return hdr.len;
        }
    }
    return 0;
}

s32 QuestNet_ProcessQueue(QNet *q, QNetBuf *buf) {
    s32 pos = 0;
    u32 n;
    s32 r;
    QNetHdr hdr;

    for (;;) {
        r = QuestNet_Parse(q, buf->data + pos, buf->len - pos, &hdr);
        if (r < 0) {
            pos -= r;
            continue;
        }
        if (r == 0) {
            break;
        }
        n = r;
        if (QuestNet_Dispatch(q, hdr.from, hdr.flags, buf->data + (pos + 4), r - 4) < 0) {
            return -1;
        }
        pos += n;
    }
    buf->len -= pos;
    if (0 < buf->len) {
        memmove(buf->data, buf->data + pos, buf->len);
    }
    return 0;
}

s32 QuestNet_Recv(QNet *q) {
    QNetBuf *raw;
    QNetBuf *queue;
    s32 r;
    u8 *fl;
    s32 n;
    s32 len;
    u8 *hp;
    u8 *body;
    s32 i;
    s32 pos;
    u8 *fr;
    QNetHdr hdr;

    r = NetTcp_Status(q->sock, (struct NetTcpStatus *)&q->tcpState);
    if (r < 0) {
        return QuestNet_RecvError(q, 10, r);
    }
    if (q->tcpState != 4) {
        return QuestNet_RecvError(q, 6, q->tcpState);
    }
    len = q->recvLen;
    if (len == 0) {
        return 0;
    }
    raw = &gQNetRecv;
    len = (len > 0x384) ? 0x384 : len;
    if (raw->cap < raw->len + len) {
        len = raw->cap - raw->len;
    }
    if (len == 0) {
        return 0;
    }
    r = NetTcp_Recv(q->sock, raw->data + raw->len, len);
    if (r != len) {
        return QuestNet_RecvError(q, 3, r);
    }
    pos = 0;
    fl = &hdr.flags;
    fr = &hdr.from;
    raw->len += len;
    q->received += len;
    q->peerTime[q->self] = q->now;
    for (;;) {
        n = QuestNet_Parse(q, raw->data + pos, raw->len - pos, &hdr);
        if (n < 0) {
            pos -= n;
            continue;
        }
        if (n == 0) {
            break;
        }
        if (QuestNet_CheckSeq(q, &hdr) < 0) {
            pos += n;
            continue;
        }
        if ((*fl & 0xF) == 2) {
            queue = &gQNetGameRecv;
        } else {
            queue = &gQNetSysRecv;
        }
        if (queue->cap < queue->len + n) {
            return QuestNet_RecvError(q, 4, queue->len + n);
        }
        memcpy(queue->data + queue->len, raw->data + pos, n);
        body = queue->data + (queue->len + 4);
        hp = queue->data + queue->len;
        for (i = n - 5; i >= 0; i--) {
            if (i == 0) {
                body[0] ^= (u8)(hp[3] ^ (hp[0] ^ hp[2]));
            } else {
                body[i] ^= (u8)(i ^ body[i - 1]);
            }
        }
        QuestNet_OnPacket(q, *fr, *fl, queue->data + (queue->len + 4));
        pos += n;
        queue->len += n;
    }
    raw->len -= pos;
    if (0 < raw->len) {
        memmove(raw->data, raw->data + pos, raw->len);
    }
    return 0;
}

s32 QuestNet_EverySecond(QNet *q) {
    s32 r;
    s32 i;
    u16 n;
    u16 prev;

    r = NetTcp_Status(q->sock, (struct NetTcpStatus *)&q->tcpState);
    if (r < 0) {
        return QuestNet_RecvError(q, 10, r);
    }
    if (q->tcpState != 4) {
        return QuestNet_RecvError(q, 6, q->tcpState);
    }
    prev = q->unk88;
    n = q->unk8A;
    if (q->sendSpace >= 0x1FFC) {
        n = 0;
    } else if (prev - q->sent < q->sendSpace) {
        if (0 < n) {
            n--;
        }
    } else if (n < 0xFFFF) {
        n++;
        for (i = 0; i < q->count; i++) {
            if (i != q->self) {
                switch (q->peer[i]) {
                case 1:
                case 2:
                case 3:
                case 4:
                    q->peerTime[i] += 1.0f;
                    break;
                }
            }
        }
    }
    q->unk88 = q->sendSpace;
    q->unk8A = n;
    if (q->unkB0 <= n) {
        return QuestNet_LinkError(q, 9, q->sendSpace);
    }
    q->unk4C += q->sent;
    q->unk50 += q->received;
    q->unk44 = q->sent;
    q->unk48 = q->received;
    q->sent = 0;
    q->received = 0;
    return 0;
}

static s32 QuestNet_ResetTotals(QNet *q) {
    q->unk4C = 0;
    q->unk50 = 0;
    return 0;
}

s32 QuestNet_CheckTimeouts(QNet *q) {
    s32 i;

    for (i = 0; i < q->count; i++) {
        if (i != q->self) {
            switch (q->peer[i]) {
            case 4:
                if (q->unk78[i] + q->unkAC[i] <= q->now) {
                    QuestNet_SetPeerState(q, i, 6);
                }
            case 1:
            case 2:
            case 3:
                if (q->peerTime[i] + q->unkA4 <= q->now) {
                    QuestNet_SetPeerState(q, i, 6);
                }
                break;
            }
        }
    }
    return 0;
}

void QuestNet_UnreadyAll(QNet *q) {
    s32 i;

    q->unk14 = 0;
    for (i = 0; i < q->count; i++) {
        if (q->peer[i] == 3) {
            QuestNet_SetPeerState(q, i, 1);
        }
    }
}

s32 QuestNet_UpdateJoin(QNet *q) {
    s32 i;

    if (q->count == 1) {
        QuestNet_SetPeerState(q, q->self, 3);
        memset(q, 0, 8);
        return 1;
    }
    if (q->lastTime + 0.5f * q->unk6 < q->now) {
        q->unk6++;
        QuestNet_SysTime(q);
    }
    if (q->lastTime + q->unk0 < q->now) {
        q->unk0++;
        QuestNet_EverySecond(q);
        QuestNet_SysAlive(q);
    }
    q->nPeersUp = 0;
    for (i = 0; i < q->count; i++) {
        if (i != q->self) {
            switch (q->peer[i]) {
            case 0:
            case 6:
                break;
            case 1:
            case 2:
                if (!(q->unk14 & (1 << i))) {
                    q->nPeersUp++;
                }
                break;
            }
        }
    }
    if (q->nPeersUp == 0) {
        QuestNet_SetPeerState(q, q->self, 2);
    }
    q->nPeersUp = 0;
    q->nPeersPinged = 0;
    for (i = 0; i < q->count; i++) {
        switch (q->peer[i]) {
        case 2:
            if (gQNetPing[i]) {
                q->nPeersPinged++;
            }
        case 1:
            q->nPeersUp++;
            break;
        }
    }
    if (q->nPeersUp == q->nPeersPinged) {
        for (i = 0; i < q->count; i++) {
            if (q->peer[i] == 2) {
                QuestNet_SetPeerState(q, i, 3);
            }
        }
        memset(q, 0, 8);
        return 1;
    }
    if (q->lastTime + q->unkA8 < q->now) {
        for (i = 0; i < q->count; i++) {
            if (i == q->self) {
                QuestNet_SetPeerState(q, i, 3);
            } else {
                switch (q->peer[i]) {
                case 1:
                    QuestNet_SetPeerState(q, i, 6);
                    break;
                case 2:
                    QuestNet_SetPeerState(q, i, 3);
                    break;
                }
            }
        }
        memset(q, 0, 8);
        return 1;
    }
    return 0;
}

void QuestNet_Update(QNet *q) {
    QuestNet_Clock(&q->now, &q->clock);
    switch (q->state) {
    case 0:
        q->state = 1;
        break;
    case 1:
        q->state = 2;
        break;
    case 2:
        q->state = 3;
        q->lastTime = q->now;
        for (q->unk0 = 0; q->unk0 < q->count; q->unk0++) {
            q->peerTime[q->unk0] = q->now;
        }
        memset(q, 0, 8);
        gQNetPing[q->self] = 0.034f;
        break;
    case 3:
        if (QuestNet_GetError() != 0) {
            q->state = 7;
            break;
        }
        QuestNet_Recv(q);
        if (QuestNet_GetError() != 0) {
            q->state = 7;
            break;
        }
        if (QuestNet_UpdateJoin(q) != 0) {
            q->state = 4;
            q->lastTime = q->now;
            q->unk30 = q->now;
        }
        QuestNet_Send(q, &gQNetSysSend, 1);
        QuestNet_ProcessQueue(q, &gQNetSysRecv);
        break;
    case 4:
        if (QuestNet_GetError() != 0) {
            q->state = 7;
            break;
        }
        QuestNet_Recv(q);
        QuestNet_CheckTimeouts(q);
        if (QuestNet_GetError() != 0) {
            q->state = 7;
            break;
        }
        if (1.0f + q->lastTime <= q->now) {
            q->lastTime = q->now;
            QuestNet_EverySecond(q);
        }
        if (60.0f + q->unk30 <= q->now) {
            q->unk30 = q->now;
            QuestNet_ResetTotals(q);
        }
        QuestNet_Send(q, &gQNetSysSend, 1);
        QuestNet_ProcessQueue(q, &gQNetSysRecv);
        break;
    case 5:
        QuestNet_UnreadyAll(q);
        q->state = 3;
        break;
    case 6:
        if (QuestNet_GetError() == 0) {
            QuestNet_Recv(q);
            QuestNet_Send(q, &gQNetSysSend, 1);
            QuestNet_ProcessQueue(q, &gQNetSysRecv);
        }
        break;
    case 7:
    case 8:
        break;
    }
}

void QuestNet_Init(QNetConfig *cfg) {
    u8 count = cfg->count;
    s32 i;

    if (count > 4) {
        count = 4;
    }
    memset(&gQNet, 0, sizeof(QNet));
    memset(&gQNetRecv, 0, sizeof(QNetBuf));
    memset(&gQNetGameRecv, 0, sizeof(QNetBuf));
    memset(&gQNetGameSend, 0, sizeof(QNetBuf));
    memset(&gQNetSysRecv, 0, sizeof(QNetBuf));
    memset(&gQNetSysSend, 0, sizeof(QNetBuf));
    memset(gQNetPing, 0, sizeof(gQNetPing));
    memset(&gQNetPingCounts, 0, sizeof(gQNetPingCounts));
    gQNet.active = 1;
    for (i = 0; i < count; i++) {
        QuestNet_SetPeerState(&gQNet, i, 1);
    }
    gQNetRecv.cap = 0x3FC;
    gQNetRecv.data = gQNetRecvData;
    gQNetGameRecv.cap = 0x4000;
    gQNetGameRecv.data = gQNetGameRecvData;
    gQNetGameSend.cap = 0x1000;
    gQNetGameSend.data = gQNetGameSendData;
    gQNetSysRecv.cap = 0x400;
    gQNetSysRecv.data = gQNetSysRecvData;
    gQNetSysSend.cap = 0x400;
    gQNetSysSend.data = gQNetSysSendData;
    for (i = 0; i < count; i++) {
        gQNet.seq[i] = rand() | 0x80;
    }
    gQNet.mode = cfg->mode;
    gQNet.count = count;
    gQNet.self = cfg->self;
    gQNet.sock = cfg->sock;
    gQNet.unk54 = cfg->unk8;
    gQNet.unk5C = cfg->unkC;
    gQNet.unkA4 = cfg->unk10;
    gQNet.unkA8 = cfg->unk18;
    gQNet.unkB0 = cfg->unk14;
    gQNet.onMsg = cfg->onMsg;
    gQNet.onMsgArg = cfg->onMsgArg;
    gQNet.onLeave = cfg->onLeave;
    gQNet.onLeaveArg = cfg->onLeaveArg;
    gQNet.onLost = cfg->onLost;
    gQNet.onLostArg = cfg->onLostArg;
}

s32 QuestNet_IsActive(void) {
    return gQNet.active;
}

s32 QuestNet_GetState(void) {
    return gQNet.state;
}

s32 QuestNet_IsRunning(void) {
    return gQNet.state == 4;
}

u8 QuestNet_GetCount(void) {
    return gQNet.count;
}

s32 QuestNet_GetError(void) {
    QNet *q = &gQNet;

    if (q->err94 != 0) {
        return q->err94;
    }
    if (q->err98 != 0) {
        return q->err98;
    }
    if (q->err9C != 0) {
        return q->err9C;
    }
    if (q->peer[q->self] == 6) {
        return 7;
    }
    return (NetIf_GetError() != 0) ? 0xB : 0;
}

s32 QuestNet_CanSend(void) {
    u8 st = gQNet.peer[gQNet.self];

    if (st != 3 && st != 4) {
        return 1;
    }
    if (gQNet.unk5C <= gQNet.sent) {
        return 2;
    }
    if (gQNet.now < gQNet.unk58 + gQNet.unk54) {
        return 3;
    }
    return 0;
}

s32 QuestNet_GetPing(u8 peer) {
    s32 n;

    if (gQNet.count <= peer) {
        return 0;
    }
    if (gQNet.self == peer) {
        return 0;
    }
    if (gQNet.peer[peer] != 3 && gQNet.peer[peer] != 4) {
        return 0;
    }
    if (gQNetPing[peer] <= 0.0f) {
        return 0;
    }
    n = 1000.0f * gQNetPing[peer] / 66.6667f;
    if (n == 0) {
        n = 1;
    }
    return n;
}

s32 QuestNet_NoPeerReady(void) {
    s32 i;

    for (i = 0; i < gQNet.count; i++) {
        if (gQNet.peer[i] == 4) {
            break;
        }
    }
    return i == gQNet.count;
}

void QuestNet_Post(void *msg) {
    if (gQNet.active != 0) {
        QuestNet_Queue(&gQNet, &gQNetGameSend, msg);
    }
}

void QuestNet_Flush(void) {
    if ((gQNet.active != 0) && (QuestNet_GetError() == 0) && (QuestNet_CanSend() == 0)) {
        QuestNet_Send(&gQNet, &gQNetGameSend, 2);
    }
}

void QuestNet_Receive(void) {
    if (gQNet.active != 0) {
        QuestNet_ProcessQueue(&gQNet, &gQNetGameRecv);
    }
}

void QuestNet_Tick(void) {
    if (gQNet.active != 0) {
        switch (gQNet.mode) {
        case 1:
            QuestNet_Update(&gQNet);
            break;
        }
    }
}

void QuestNet_Ready(u8 arg) {
    if (gQNet.peer[gQNet.self] == 3) {
        if (arg == 0) {
            arg = gQNet.unkA8;
        }
        QuestNet_SysReady(&gQNet, arg);
        QuestNet_SetPeerState(&gQNet, gQNet.self, 4);
    }
}

void QuestNet_Unready(void) {
    if (gQNet.peer[gQNet.self] == 4) {
        QuestNet_SysUnready(&gQNet);
        QuestNet_SetPeerState(&gQNet, gQNet.self, 3);
    }
}

void QuestNet_Leave(void) {
    if (gQNet.peer[gQNet.self] == 3) {
        QuestNet_SysLeave(&gQNet);
        QuestNet_SetPeerState(&gQNet, gQNet.self, 5);
        gQNet.state = 6;
    }
}

void QuestNet_Close(void) {
    s32 i;

    gQNet.active = 0;
    gQNet.state = 8;
    for (i = 0; i < gQNet.count; i++) {
        QuestNet_SetPeerState(&gQNet, i, 0);
    }
}
