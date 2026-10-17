/*
 * NETWORK: the in-quest protocol (preserve byte for byte for online play).
 *
 * One session, gQNet (QNet, qnet.h), over one TCP connection (NetTcp_*),
 * stepped each frame by QuestNet_Tick. The session joins its players
 * (alive and time messages measure each peer's ping), then runs: the
 * game's messages (qnet_sync.c, through qnet_if.c's QuestNet_Post,
 * QuestNet_Flush and QuestNet_Receive) travel on the game channel, the
 * session's own (ready, leave, time) on the system channel. A packet is a
 * QNetHdr, optionally the sender's view of the peer states, then messages
 * that each start with a QNetHdr; the bytes after the packet's header are
 * scrambled with a running XOR. The queues (QNetBuf): gQNetSysSend and
 * gQNetGameSend to send, gQNetSysRecv and gQNetGameRecv received,
 * gQNetRecv the raw receive buffer.
 */
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
s32 NetTcp_Status(s32 sock, NetTcpStatus *st);
s32 NetTcp_Recv(s32, void *, s16);
s32 NetTcp_Send(s32, void *, s16);
void *memmove(void *dst, const void *src, u32 n);
s64 GetTimerSystemTime(void);
void TimerBusClock2USec(s64, u32 *, u32 *);
/* unit's own declaration: the definition (sub_main/unk_30E420.c) is u16 Swap16(s32 x); QuestNet_Parse matches only with this one */
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

/*
 * Advances the clock *time (seconds) by the time since the system timer
 * reading *lastClock, and stores the new reading. The first call (no
 * reading yet) starts the clock at 0.
 */
void QuestNet_Clock(f32 *time, s64 *lastClock)
{
    u32 sec;
    u32 usec;
    s64 now;

    if (*lastClock == 0) {
        *lastClock = GetTimerSystemTime();
        *time = 0.0f;
    } else {
        now = GetTimerSystemTime();
        if (now - *lastClock != 0) {
            TimerBusClock2USec(now - *lastClock, &sec, &usec);
            *time += (f32)sec + 0.000001f * (f32)usec;
            *lastClock = now;
        }
    }
}

/*
 * Records a send-side error (QNET_ERR_*) and its detail (a result or a
 * length) in the session; returns the error negated.
 */
s32 QuestNet_SendError(QNet *net, s32 err, s32 detail)
{
    net->sendError = err;
    net->errDetail = detail;
    return -err;
}

/*
 * Records a receive-side error and its detail, and empties both receive
 * queues; returns the error negated.
 */
s32 QuestNet_RecvError(QNet *net, s32 err, s32 detail)
{
    net->recvError = err;
    net->errDetail = detail;
    gQNetGameRecv.len = 0;
    gQNetSysRecv.len = 0;
    return -err;
}

/*
 * Records a link error (a stalled send) and its detail; returns the error
 * negated.
 */
s32 QuestNet_LinkError(QNet *net, s32 err, s32 detail)
{
    net->linkError = err;
    net->errDetail = detail;
    return -err;
}

/*
 * Appends a copy of the local player's own sent game messages (len bytes
 * of data) to the game receive queue, under a header from the local
 * player, so the game handles its own messages as the others do. type is
 * the channel. Returns 0, or the error when the queue is full.
 */
s32 QuestNet_Loopback(QNet *net, u8 *data, s32 len, u8 type)
{
    QNetHdr hdr;
    QNetBuf *buf = &gQNetGameRecv;

    if (buf->cap < buf->len + QNET_HDR_LEN + len) {
        return QuestNet_SendError(net, QNET_ERR_RECV_FULL, buf->len + QNET_HDR_LEN + len);
    }
    hdr.len = (len + QNET_HDR_LEN == QNET_TEXT_MARK) ? len + QNET_HDR_LEN + 1 : len + QNET_HDR_LEN;
    hdr.flags = type;
    hdr.from = net->self;
    hdr.seq = QNET_SEQ_NONE;
    memcpy(buf->data + buf->len, &hdr, sizeof(QNetHdr));
    memcpy(buf->data + (buf->len + QNET_HDR_LEN), data, len);
    buf->len += hdr.len;
    return 0;
}

/*
 * Sends what fits of a queue (the channel type: QNET_CHAN_SYS or
 * QNET_CHAN_GAME) as one packet: the header, the peer states every second
 * (every two when the queue is empty: QNET_FLAG_STATES), then whole
 * messages. Droppable messages are dropped while the socket's send space is
 * under QNET_SEND_SPACE_LOW, and a packet leaves QNET_SEND_RESERVE bytes of
 * the space free. The bytes after the header are scrambled with a running
 * XOR before sending. A game packet goes out only while another player is
 * still in the session (or with the peer states), and what it took from the
 * queue is looped back to the local player. Returns the bytes
 * taken from the queue, or an error. The header is written as bytes of
 * gQNetMsg: the field forms change the stack frame. The `switch` on the
 * send result, with `default:` first (-1: try again later), is what leaves
 * the delay slots of every conditional branch of the function empty, as in
 * the original: with an `if` there, MWCC fills them from the fall-through
 * blocks.
 */
s32 QuestNet_Send(QNet *net, QNetBuf *queue, u8 type)
{
    s32 pos;
    s32 space;
    s32 len;
    u8 *msg;
    u8 *out;
    s32 send;
    s32 drop;
    s32 chan;
    s32 result;
    s32 j;
    s32 i;
    s32 size;
    QNetHdr hdr;

    result = NetTcp_Status(net->sock, &net->tcp);
    if (result < 0) {
        return QuestNet_SendError(net, QNET_ERR_STATUS, result);
    }
    if (net->tcp.state != 4) {
        return QuestNet_SendError(net, QNET_ERR_NOT_CONNECTED, net->tcp.state);
    }
    drop = 0;
    if (net->tcp.sendSpace < QNET_SEND_SPACE_LOW) {
        drop = 1;
    }
    chan = type;
    if (chan == QNET_CHAN_GAME) {
        send = 0;
        for (i = 0; i < net->count; i++) {
            if (i != net->self && net->peerState[i] != QNET_PEER_LOST && net->peerState[i] != QNET_PEER_LEFT && net->peerState[i] != QNET_PEER_NONE) {
                send = 1;
                break;
            }
        }
    } else {
        send = 1;
    }
    space = QNET_PACKET_MAX;
    if (net->tcp.sendSpace - QNET_PACKET_MAX < QNET_SEND_RESERVE + 1) {
        space = net->tcp.sendSpace - QNET_SEND_RESERVE;
    }
    if (space < QNET_HDR_LEN) {
        return 0;
    }
    space -= QNET_HDR_LEN;
    len = QNET_HDR_LEN;
    msg = (u8 *)&gQNetMsg;
    out = gQNetMsg + QNET_HDR_LEN;
    msg[1] = chan & QNET_CHAN_MASK;
    msg[2] = net->self;
    if ((1.0f + net->lastStatesTime < net->now && 0 < queue->len) || 2.0f + net->lastStatesTime < net->now) {
        if (space < QNET_PEERS) {
            return 0;
        }
        space -= QNET_PEERS;
        for (i = 0; i < net->count; i++) {
            out[i] = net->peerState[i];
        }
        len += QNET_PEERS;
        out += QNET_PEERS;
        msg[1] |= QNET_FLAG_STATES;
    }
    pos = 0;
    while (0 < space && pos < queue->len) {
        memcpy(&hdr, queue->data + pos, sizeof(QNetHdr));
        if (space < hdr.len) {
            break;
        }
        if (drop && hdr.flags == QNET_MSG_DROPPABLE) {
            pos += hdr.len;
        } else {
            memcpy(out, queue->data + pos, hdr.len);
            space -= hdr.len;
            len += hdr.len;
            out += hdr.len;
            pos += hdr.len;
        }
    }
    if (len == QNET_HDR_LEN) {
        return 0;
    }
    msg[3] = ++net->seq[net->self];
    msg[0] = (len == QNET_TEXT_MARK) ? len + 1 : len;
    size = msg[0];
    for (j = 0; j < size - QNET_HDR_LEN; j++) {
        if (j == 0) {
            msg[QNET_HDR_LEN] ^= (u8)(((QNetHdr *)msg)->seq ^ (((QNetHdr *)msg)->len ^ ((QNetHdr *)msg)->from));
        } else {
            *(u8 *)(j + (u32)msg + QNET_HDR_LEN) ^= (u8)(j ^ *(u8 *)(j + (u32)msg + QNET_HDR_LEN - 1));
        }
    }
    if (send == 1 || (msg[1] & QNET_FLAG_MASK)) {
        result = NetTcp_Send(net->sock, msg, size);
        switch (result) {
        default:
            if (result < 0) {
                return QuestNet_SendError(net, QNET_ERR_SEND, result);
            }
            break;
        case -1:
            break;
        }
        net->sent += size;
        net->lastSendTime = net->now;
        if (msg[1] & QNET_FLAG_STATES) {
            net->lastStatesTime = net->now;
        }
    }
    if (pos != 0) {
        if (chan == QNET_CHAN_GAME) {
            QuestNet_Loopback(net, queue->data, pos, type);
        }
        size = queue->len - pos;
        if (0 < size) {
            memmove(queue->data, queue->data + pos, size);
        }
        queue->len = size;
    }
    return pos;
}

/*
 * Sets a player's state, calling onLeave when another player becomes
 * QNET_PEER_LEFT and onLost when one becomes QNET_PEER_LOST.
 */
void QuestNet_SetPeerState(QNet *net, u8 peer, u8 state)
{
    s32 old = net->peerState[peer];

    net->peerState[peer] = state;
    if (old != QNET_PEER_LEFT && state == QNET_PEER_LEFT) {
        if (peer != net->self) {
            net->onLeave(peer, net->onLeaveArg, state);
        }
    } else if (old != QNET_PEER_LOST && state == QNET_PEER_LOST && peer != net->self) {
        net->onLost(peer, net->onLostArg, state);
    }
}

/*
 * Marks lost every player a peer's packet reports as lost (states: the
 * sender's view of the QNET_PEERS states).
 */
void QuestNet_DropPeers(QNet *net, u8 *states)
{
    s32 i;

    for (i = 0; i < net->count; i++) {
        if (net->peerState[i] != QNET_PEER_LOST && states[i] == QNET_PEER_LOST) {
            QuestNet_SetPeerState(net, i, QNET_PEER_LOST);
        }
    }
}

/*
 * Appends a message (its length in its header) to a send queue. Returns
 * the length, or the error when the queue is full.
 */
s32 QuestNet_Queue(QNet *net, QNetBuf *buf, void *msg)
{
    QNetHdr *hdr = msg;

    if (buf->cap < buf->len + hdr->len) {
        return QuestNet_SendError(net, QNET_ERR_QUEUE_FULL, buf->len + hdr->len);
    }
    memcpy(buf->data + buf->len, msg, hdr->len);
    buf->len += hdr->len;
    return hdr->len;
}

/* Queues the join's alive message. */
s32 QuestNet_SysAlive(QNet *net)
{
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = QNET_HDR_LEN;
    msg->flags = 0;
    msg->cmd = QNET_SYS_ALIVE;
    msg->seq = QNET_SEQ_NONE;
    return QuestNet_Queue(net, &gQNetSysSend, msg);
}

/*
 * Queues the ready message, with the seconds the local player may stay
 * ready (timeout).
 */
s32 QuestNet_SysReady(QNet *net, u8 timeout)
{
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = QNET_HDR_LEN + 1;
    msg->flags = 0;
    msg->cmd = QNET_SYS_READY;
    msg->seq = QNET_SEQ_NONE;
    msg->u.ready = timeout;
    return QuestNet_Queue(net, &gQNetSysSend, msg);
}

/* Queues the unready message. */
s32 QuestNet_SysUnready(QNet *net)
{
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = QNET_HDR_LEN;
    msg->flags = 0;
    msg->cmd = QNET_SYS_UNREADY;
    msg->seq = QNET_SEQ_NONE;
    return QuestNet_Queue(net, &gQNetSysSend, msg);
}

/* Queues the leave message. */
s32 QuestNet_SysLeave(QNet *net)
{
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = QNET_HDR_LEN;
    msg->flags = 0;
    msg->cmd = QNET_SYS_LEAVE;
    msg->seq = QNET_SEQ_NONE;
    return QuestNet_Queue(net, &gQNetSysSend, msg);
}

/*
 * Queues the time message: the session's clock, for the peers to send
 * back.
 */
s32 QuestNet_SysTime(QNet *net)
{
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = QNET_HDR_LEN + 4;
    msg->flags = 0;
    msg->cmd = QNET_SYS_TIME;
    msg->seq = QNET_SEQ_NONE;
    msg->u.t.time = net->now;
    return QuestNet_Queue(net, &gQNetSysSend, msg);
}

/*
 * Queues the answer to a peer's time message: its clock (time) sent back
 * to the player that asked (asker).
 */
s32 QuestNet_SysTimeReply(QNet *net, f32 time, u8 asker)
{
    QNetSysMsg *msg = (QNetSysMsg *)gQNetMsg;

    msg->len = QNET_HDR_LEN + 5;
    msg->flags = 0;
    msg->cmd = QNET_SYS_TIME_REPLY;
    msg->seq = QNET_SEQ_NONE;
    msg->u.t.time = time;
    msg->u.t.to = asker;
    return QuestNet_Queue(net, &gQNetSysSend, msg);
}

/*
 * Handles a system message from a peer: an alive answered once (the peer
 * becomes QNET_PEER_ALIVE), ready and unready with the ready timeout, leave,
 * the time (answered), and the time reply, whose QNET_PING_SAMPLES-th one
 * sets the peer's ping. Each message refreshes the peer's last-heard time.
 * Returns 0.
 */
s32 QuestNet_OnSysMsg(QNet *net, u8 from, QNetSysMsg *msg)
{
    s32 i;
    f32 *sentTime;

    switch (msg->cmd) {
    case QNET_SYS_ALIVE:
        if (net->peerState[net->self] == QNET_PEER_JOINING && net->peerState[from] == QNET_PEER_JOINING) {
            QuestNet_SetPeerState(net, from, QNET_PEER_ALIVE);
            QuestNet_SysAlive(net);
            net->aliveMask |= 1 << from;
        }
        break;
    case QNET_SYS_READY:
        i = from;
        if (net->peerState[i] == QNET_PEER_JOINED) {
            QuestNet_SetPeerState(net, from, QNET_PEER_READY);
            net->readyTime[i] = net->now;
            net->readyTimeout[i] = msg->u.ready;
        }
        break;
    case QNET_SYS_UNREADY:
        i = from;
        if (net->peerState[i] == QNET_PEER_READY) {
            QuestNet_SetPeerState(net, from, QNET_PEER_JOINED);
            net->readyTime[i] = 0.0f;
            net->readyTimeout[i] = 0;
        }
        break;
    case QNET_SYS_LEAVE:
        if (net->peerState[from] == QNET_PEER_JOINED) {
            QuestNet_SetPeerState(net, from, QNET_PEER_LEFT);
        }
        break;
    case QNET_SYS_TIME:
        QuestNet_SysTimeReply(net, msg->u.t.time, from);
        break;
    case QNET_SYS_TIME_REPLY:
        sentTime = &msg->u.t.time;
        if (msg->u.t.to == net->self) {
            if (gQNetPing[from] == 0.0f) {
                gQNetPingCounts[from]++;
                if (gQNetPingCounts[from] >= QNET_PING_SAMPLES) {
                    gQNetPing[from] = net->now - *sentTime;
                }
            }
        }
        break;
    default:
        return 0;
    }
    net->peerTime[from] = net->now;
    return 0;
}

/*
 * Passes a game message to the session's handler (onMsg); returns its
 * result.
 */
s32 QuestNet_OnGameMsg(QNet *net, u8 from, u8 *msg)
{
    return net->onMsg(from, msg, net->onMsgArg);
}

/*
 * Notes a packet from peer: a packet with the peer states refreshes the
 * peer's last-heard time, and from a joined or ready peer marks lost the
 * players it reports lost. Returns 0.
 */
s32 QuestNet_OnPacket(QNet *net, u8 peer, u8 flags, u8 *states)
{
    u8 state;

    if (net->count <= peer) {
        return 0;
    }
    state = net->peerState[peer];
    switch (state) {
    case QNET_PEER_NONE:
    case QNET_PEER_LOST:
        return 0;
    }
    if (flags & QNET_FLAG_STATES) {
        if (state == QNET_PEER_JOINED || state == QNET_PEER_READY) {
            QuestNet_DropPeers(net, states);
        }
        net->peerTime[peer] = net->now;
    }
    return 0;
}

/*
 * Hands each message of a received packet (len bytes of data, past the
 * peer states when flags has them) to its channel's handler. Ignores
 * packets from a player that is gone or left. Returns 0, or the error of a
 * bad message or a failed handler.
 */
s32 QuestNet_Dispatch(QNet *net, u8 from, u8 flags, u8 *data, u8 len)
{
    u8 *msg = (u8 *)&gQNetMsg;
    s32 pos = 0;
    s32 result;
    QNetHdr hdr;

    if (net->count <= from) {
        return 0;
    }
    switch (net->peerState[from]) {
    case QNET_PEER_NONE:
    case QNET_PEER_LEFT:
    case QNET_PEER_LOST:
        return 0;
    }
    if (flags & QNET_FLAG_MASK) {
        if (len == QNET_PEERS) {
            return 0;
        }
        data += QNET_PEERS;
        len -= QNET_PEERS;
    }
    while (pos < len) {
        if (len - pos < QNET_HDR_LEN) {
            break;
        }
        memcpy(&hdr, data + pos, sizeof(QNetHdr));
        if (hdr.len < QNET_HDR_LEN) {
            return QuestNet_RecvError(net, QNET_ERR_BAD_MSG, hdr.len);
        }
        memcpy(msg, data + pos, hdr.len);
        switch (flags & QNET_CHAN_MASK) {
        case 0:
            break;
        case QNET_CHAN_SYS:
            result = QuestNet_OnSysMsg(net, from, (QNetSysMsg *)msg);
            if (result < 0) {
                return QuestNet_RecvError(net, QNET_ERR_HANDLER, result);
            }
            break;
        case QNET_CHAN_GAME:
            result = QuestNet_OnGameMsg(net, from, msg);
            if (result < 0) {
                return QuestNet_RecvError(net, QNET_ERR_HANDLER, result);
            }
            break;
        default:
            return QuestNet_RecvError(net, QNET_ERR_BAD_MSG, flags);
        }
        pos += hdr.len;
    }
    return 0;
}

/*
 * Records a peer's sequence number from a packet's header, and the first
 * one seen. Returns 0.
 */
s32 QuestNet_CheckSeq(QNet *net, QNetHdr *hdr)
{
    if (net->count <= hdr->from) {
        return 0;
    }
    if (net->self == hdr->from) {
        return 0;
    }
    if (net->firstSeq[hdr->from] == 0) {
        net->firstSeq[hdr->from] = hdr->seq;
    } else {
        net->seq[hdr->from]++;
    }
    net->seq[hdr->from] = hdr->seq;
    return 0;
}

/*
 * Finds the next packet in len bytes of a stream (enum QNetParseState): a
 * text header (QNET_TEXT_MARK, 12 bytes with its body's length at 4) is
 * skipped with its body, an empty message is skipped. Returns the packet's
 * length (and copies its header to out), a negative count of bytes to
 * skip, or 0 when the packet is not whole yet.
 */
s32 QuestNet_Parse(QNet *net, u8 *data, s32 len, QNetHdr *out)
{
    QNetHdr hdr;
    u8 text[12];
    s32 state = QNET_PARSE_START;
    s32 bodyLen;

    while (0 < len) {
        switch (state) {
        case QNET_PARSE_START:
            if (len <= 0) {
                return 0;
            }
            len--;
            if (*data == QNET_TEXT_MARK) {
                state = QNET_PARSE_TEXT_HEAD;
            } else {
                state = QNET_PARSE_HEAD;
            }
            break;
        case QNET_PARSE_TEXT_HEAD:
            if (len < sizeof(text) - 1) {
                return 0;
            }
            len -= 11;
            memcpy(text, data, sizeof(text));
            bodyLen = Swap16(*(u16 *)&text[4]);
            if (bodyLen == 0) {
                return -12;
            }
            state = QNET_PARSE_TEXT_BODY;
            break;
        case QNET_PARSE_TEXT_BODY:
            return (len < bodyLen) ? 0 : (bodyLen + sizeof(text)) * -1;
        case QNET_PARSE_HEAD:
            if (len < QNET_HDR_LEN - 1) {
                return 0;
            }
            len -= QNET_HDR_LEN - 1;
            memcpy(&hdr, data, sizeof(QNetHdr));
            if (hdr.len < QNET_HDR_LEN) {
                QuestNet_RecvError(net, QNET_ERR_BAD_MSG, hdr.len);
                return 0;
            }
            bodyLen = hdr.len - QNET_HDR_LEN;
            if (bodyLen == 0) {
                return -QNET_HDR_LEN;
            }
            state = QNET_PARSE_BODY;
            break;
        case QNET_PARSE_BODY:
            if (len < bodyLen) {
                return 0;
            }
            if (out != NULL) {
                memcpy(out, &hdr, sizeof(QNetHdr));
            }
            return hdr.len;
        }
    }
    return 0;
}

/*
 * Dispatches every whole packet of a receive queue, then drops them from
 * it. Returns 0, or -1 when a handler failed.
 */
s32 QuestNet_ProcessQueue(QNet *net, QNetBuf *buf)
{
    s32 pos = 0;
    u32 size;
    s32 result;
    QNetHdr hdr;

    for (;;) {
        result = QuestNet_Parse(net, buf->data + pos, buf->len - pos, &hdr);
        if (result < 0) {
            pos -= result;
            continue;
        }
        if (result == 0) {
            break;
        }
        size = result;
        if (QuestNet_Dispatch(net, hdr.from, hdr.flags, buf->data + (pos + QNET_HDR_LEN), result - QNET_HDR_LEN) < 0) {
            return -1;
        }
        pos += size;
    }
    buf->len -= pos;
    if (0 < buf->len) {
        memmove(buf->data, buf->data + pos, buf->len);
    }
    return 0;
}

/*
 * Reads what the socket holds (up to QNET_RECV_MAX bytes), then moves each
 * whole packet into the system or the game receive queue, unscrambling the
 * bytes after its header, and notes it (QuestNet_OnPacket). Returns 0 or
 * an error. The unscrambling reads the header as bytes: the field forms
 * change the code.
 */
s32 QuestNet_Recv(QNet *net)
{
    QNetBuf *raw;
    QNetBuf *queue;
    s32 result;
    u8 *hdrFlags;
    s32 size;
    s32 len;
    u8 *head;
    u8 *body;
    s32 i;
    s32 pos;
    u8 *hdrFrom;
    QNetHdr hdr;

    result = NetTcp_Status(net->sock, &net->tcp);
    if (result < 0) {
        return QuestNet_RecvError(net, QNET_ERR_STATUS, result);
    }
    if (net->tcp.state != 4) {
        return QuestNet_RecvError(net, QNET_ERR_NOT_CONNECTED, net->tcp.state);
    }
    len = net->tcp.recvLen;
    if (len == 0) {
        return 0;
    }
    raw = &gQNetRecv;
    len = (len > QNET_RECV_MAX) ? QNET_RECV_MAX : len;
    if (raw->cap < raw->len + len) {
        len = raw->cap - raw->len;
    }
    if (len == 0) {
        return 0;
    }
    result = NetTcp_Recv(net->sock, raw->data + raw->len, len);
    if (result != len) {
        return QuestNet_RecvError(net, QNET_ERR_RECV, result);
    }
    pos = 0;
    hdrFlags = &hdr.flags;
    hdrFrom = &hdr.from;
    raw->len += len;
    net->received += len;
    net->peerTime[net->self] = net->now;
    for (;;) {
        size = QuestNet_Parse(net, raw->data + pos, raw->len - pos, &hdr);
        if (size < 0) {
            pos -= size;
            continue;
        }
        if (size == 0) {
            break;
        }
        if (QuestNet_CheckSeq(net, &hdr) < 0) {
            pos += size;
            continue;
        }
        if ((*hdrFlags & QNET_CHAN_MASK) == QNET_CHAN_GAME) {
            queue = &gQNetGameRecv;
        } else {
            queue = &gQNetSysRecv;
        }
        if (queue->cap < queue->len + size) {
            return QuestNet_RecvError(net, QNET_ERR_RECV_FULL, queue->len + size);
        }
        memcpy(queue->data + queue->len, raw->data + pos, size);
        body = queue->data + (queue->len + QNET_HDR_LEN);
        head = queue->data + queue->len;
        for (i = size - (QNET_HDR_LEN + 1); i >= 0; i--) {
            if (i == 0) {
                body[0] ^= (u8)(head[3] ^ (head[0] ^ head[2]));
            } else {
                body[i] ^= (u8)(i ^ body[i - 1]);
            }
        }
        QuestNet_OnPacket(net, *hdrFrom, *hdrFlags, queue->data + (queue->len + QNET_HDR_LEN));
        pos += size;
        queue->len += size;
    }
    raw->len -= pos;
    if (0 < raw->len) {
        memmove(raw->data, raw->data + pos, raw->len);
    }
    return 0;
}

/*
 * Once a second: counts the seconds the send space stays short (each
 * such second also gives the active peers a second more before their
 * timeout), ending the session with QNET_ERR_STALLED after maxStall of
 * them, and moves this second's byte counts to the last second's and the
 * totals. Returns 0 or an error.
 */
s32 QuestNet_EverySecond(QNet *net)
{
    s32 result;
    s32 i;
    u16 stallSecs;
    u16 prevSpace;

    result = NetTcp_Status(net->sock, &net->tcp);
    if (result < 0) {
        return QuestNet_RecvError(net, QNET_ERR_STATUS, result);
    }
    if (net->tcp.state != 4) {
        return QuestNet_RecvError(net, QNET_ERR_NOT_CONNECTED, net->tcp.state);
    }
    prevSpace = net->prevSendSpace;
    stallSecs = net->stallSecs;
    if (net->tcp.sendSpace >= QNET_SEND_SPACE_IDLE) {
        stallSecs = 0;
    } else if (prevSpace - net->sent < net->tcp.sendSpace) {
        if (0 < stallSecs) {
            stallSecs--;
        }
    } else if (stallSecs < 0xFFFF) {
        stallSecs++;
        for (i = 0; i < net->count; i++) {
            if (i != net->self) {
                switch (net->peerState[i]) {
                case QNET_PEER_JOINING:
                case QNET_PEER_ALIVE:
                case QNET_PEER_JOINED:
                case QNET_PEER_READY:
                    net->peerTime[i] += 1.0f;
                    break;
                }
            }
        }
    }
    net->prevSendSpace = net->tcp.sendSpace;
    net->stallSecs = stallSecs;
    if (net->maxStall <= stallSecs) {
        return QuestNet_LinkError(net, QNET_ERR_STALLED, net->tcp.sendSpace);
    }
    net->sentTotal += net->sent;
    net->receivedTotal += net->received;
    net->sentLastSec = net->sent;
    net->receivedLastSec = net->received;
    net->sent = 0;
    net->received = 0;
    return 0;
}

/* Clears the byte totals (each minute). Returns 0. */
static s32 QuestNet_ResetTotals(QNet *net)
{
    net->sentTotal = 0;
    net->receivedTotal = 0;
    return 0;
}

/*
 * Marks lost each peer that stayed ready past its ready timeout, or that
 * has been silent for peerTimeout seconds. Returns 0.
 */
s32 QuestNet_CheckTimeouts(QNet *net)
{
    s32 i;

    for (i = 0; i < net->count; i++) {
        if (i != net->self) {
            switch (net->peerState[i]) {
            case QNET_PEER_READY:
                if (net->readyTime[i] + net->readyTimeout[i] <= net->now) {
                    QuestNet_SetPeerState(net, i, QNET_PEER_LOST);
                }
            case QNET_PEER_JOINING:
            case QNET_PEER_ALIVE:
            case QNET_PEER_JOINED:
                if (net->peerTime[i] + net->peerTimeout <= net->now) {
                    QuestNet_SetPeerState(net, i, QNET_PEER_LOST);
                }
                break;
            }
        }
    }
    return 0;
}

/*
 * Sends every joined player back to joining and forgets the answered
 * alives (QNET_STATE_UNREADY).
 */
void QuestNet_UnreadyAll(QNet *net)
{
    s32 i;

    net->aliveMask = 0;
    for (i = 0; i < net->count; i++) {
        if (net->peerState[i] == QNET_PEER_JOINED) {
            QuestNet_SetPeerState(net, i, QNET_PEER_JOINING);
        }
    }
}

/*
 * The join, a frame: the time message every half second, the alive (and
 * QuestNet_EverySecond) every second. The local player is alive once every
 * joining peer has answered; when every alive player has a ping, they all
 * join. After joinTimeout seconds the join ends anyway: the alive peers
 * join, the joining ones are lost. Returns 1 when the join is over (the
 * join's counters cleared), else 0. A session of one joins at once.
 */
s32 QuestNet_UpdateJoin(QNet *net)
{
    s32 i;

    if (net->count == 1) {
        QuestNet_SetPeerState(net, net->self, QNET_PEER_JOINED);
        memset(net, 0, QNET_JOIN_COUNTERS);
        return 1;
    }
    if (net->lastTime + 0.5f * net->joinHalfSecond < net->now) {
        net->joinHalfSecond++;
        QuestNet_SysTime(net);
    }
    if (net->lastTime + net->joinSecond < net->now) {
        net->joinSecond++;
        QuestNet_EverySecond(net);
        QuestNet_SysAlive(net);
    }
    net->nPeersUp = 0;
    for (i = 0; i < net->count; i++) {
        if (i != net->self) {
            switch (net->peerState[i]) {
            case QNET_PEER_NONE:
            case QNET_PEER_LOST:
                break;
            case QNET_PEER_JOINING:
            case QNET_PEER_ALIVE:
                if (!(net->aliveMask & (1 << i))) {
                    net->nPeersUp++;
                }
                break;
            }
        }
    }
    if (net->nPeersUp == 0) {
        QuestNet_SetPeerState(net, net->self, QNET_PEER_ALIVE);
    }
    net->nPeersUp = 0;
    net->nPeersPinged = 0;
    for (i = 0; i < net->count; i++) {
        switch (net->peerState[i]) {
        case QNET_PEER_ALIVE:
            if (gQNetPing[i]) {
                net->nPeersPinged++;
            }
        case QNET_PEER_JOINING:
            net->nPeersUp++;
            break;
        }
    }
    if (net->nPeersUp == net->nPeersPinged) {
        for (i = 0; i < net->count; i++) {
            if (net->peerState[i] == QNET_PEER_ALIVE) {
                QuestNet_SetPeerState(net, i, QNET_PEER_JOINED);
            }
        }
        memset(net, 0, QNET_JOIN_COUNTERS);
        return 1;
    }
    if (net->lastTime + net->joinTimeout < net->now) {
        for (i = 0; i < net->count; i++) {
            if (i == net->self) {
                QuestNet_SetPeerState(net, i, QNET_PEER_JOINED);
            } else {
                switch (net->peerState[i]) {
                case QNET_PEER_JOINING:
                    QuestNet_SetPeerState(net, i, QNET_PEER_LOST);
                    break;
                case QNET_PEER_ALIVE:
                    QuestNet_SetPeerState(net, i, QNET_PEER_JOINED);
                    break;
                }
            }
        }
        memset(net, 0, QNET_JOIN_COUNTERS);
        return 1;
    }
    return 0;
}

/*
 * Steps the session a frame (QNet.state, enum QNetState): the clock, then
 * the opening, the join, and the running session (receiving, the timeouts,
 * the per-second and per-minute counts, the system messages sent and
 * handled). A socket error ends it (QNET_STATE_ERROR).
 */
void QuestNet_Update(QNet *net)
{
    QuestNet_Clock(&net->now, &net->clock);
    switch (net->state) {
    case QNET_STATE_INIT:
        net->state = QNET_STATE_INIT2;
        break;
    case QNET_STATE_INIT2:
        net->state = QNET_STATE_OPEN;
        break;
    case QNET_STATE_OPEN:
        net->state = QNET_STATE_JOIN;
        net->lastTime = net->now;
        for (net->joinSecond = 0; net->joinSecond < net->count; net->joinSecond++) {
            net->peerTime[net->joinSecond] = net->now;
        }
        memset(net, 0, QNET_JOIN_COUNTERS);
        gQNetPing[net->self] = 0.034f;
        break;
    case QNET_STATE_JOIN:
        if (QuestNet_GetError() != 0) {
            net->state = QNET_STATE_ERROR;
            break;
        }
        QuestNet_Recv(net);
        if (QuestNet_GetError() != 0) {
            net->state = QNET_STATE_ERROR;
            break;
        }
        if (QuestNet_UpdateJoin(net) != 0) {
            net->state = QNET_STATE_RUN;
            net->lastTime = net->now;
            net->lastTotalsTime = net->now;
        }
        QuestNet_Send(net, &gQNetSysSend, QNET_CHAN_SYS);
        QuestNet_ProcessQueue(net, &gQNetSysRecv);
        break;
    case QNET_STATE_RUN:
        if (QuestNet_GetError() != 0) {
            net->state = QNET_STATE_ERROR;
            break;
        }
        QuestNet_Recv(net);
        QuestNet_CheckTimeouts(net);
        if (QuestNet_GetError() != 0) {
            net->state = QNET_STATE_ERROR;
            break;
        }
        if (1.0f + net->lastTime <= net->now) {
            net->lastTime = net->now;
            QuestNet_EverySecond(net);
        }
        if (60.0f + net->lastTotalsTime <= net->now) {
            net->lastTotalsTime = net->now;
            QuestNet_ResetTotals(net);
        }
        QuestNet_Send(net, &gQNetSysSend, QNET_CHAN_SYS);
        QuestNet_ProcessQueue(net, &gQNetSysRecv);
        break;
    case QNET_STATE_UNREADY:
        QuestNet_UnreadyAll(net);
        net->state = QNET_STATE_JOIN;
        break;
    case QNET_STATE_LEAVE:
        if (QuestNet_GetError() == 0) {
            QuestNet_Recv(net);
            QuestNet_Send(net, &gQNetSysSend, QNET_CHAN_SYS);
            QuestNet_ProcessQueue(net, &gQNetSysRecv);
        }
        break;
    case QNET_STATE_ERROR:
    case QNET_STATE_CLOSED:
        break;
    }
}

/*
 * Starts a session from cfg: clears the session and its queues, sets
 * every player joining, gives the queues their buffers and each player a
 * random first sequence number (the top bit set), and copies the settings
 * and the callbacks.
 */
void QuestNet_Init(QNetConfig *cfg)
{
    u8 count = cfg->count;
    s32 i;

    if (count > QNET_PEERS) {
        count = QNET_PEERS;
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
        QuestNet_SetPeerState(&gQNet, i, QNET_PEER_JOINING);
    }
    gQNetRecv.cap = sizeof(gQNetRecvData) - 4;
    gQNetRecv.data = gQNetRecvData;
    gQNetGameRecv.cap = sizeof(gQNetGameRecvData);
    gQNetGameRecv.data = gQNetGameRecvData;
    gQNetGameSend.cap = sizeof(gQNetGameSendData);
    gQNetGameSend.data = gQNetGameSendData;
    gQNetSysRecv.cap = sizeof(gQNetSysRecvData);
    gQNetSysRecv.data = gQNetSysRecvData;
    gQNetSysSend.cap = sizeof(gQNetSysSendData);
    gQNetSysSend.data = gQNetSysSendData;
    for (i = 0; i < count; i++) {
        gQNet.seq[i] = rand() | 0x80;
    }
    gQNet.mode = cfg->mode;
    gQNet.count = count;
    gQNet.self = cfg->self;
    gQNet.sock = cfg->sock;
    gQNet.sendInterval = cfg->sendInterval;
    gQNet.sendBudget = cfg->sendBudget;
    gQNet.peerTimeout = cfg->peerTimeout;
    gQNet.joinTimeout = cfg->joinTimeout;
    gQNet.maxStall = cfg->maxStall;
    gQNet.onMsg = cfg->onMsg;
    gQNet.onMsgArg = cfg->onMsgArg;
    gQNet.onLeave = cfg->onLeave;
    gQNet.onLeaveArg = cfg->onLeaveArg;
    gQNet.onLost = cfg->onLost;
    gQNet.onLostArg = cfg->onLostArg;
}

/* 1 from QuestNet_Init until QuestNet_Close. */
s32 QuestNet_IsActive(void)
{
    return gQNet.active;
}

/* The session's state (enum QNetState). */
s32 QuestNet_GetState(void)
{
    return gQNet.state;
}

/* 1 while the session runs (QNET_STATE_RUN). */
s32 QuestNet_IsRunning(void)
{
    return gQNet.state == QNET_STATE_RUN;
}

/* The session's players. */
u8 QuestNet_GetCount(void)
{
    return gQNet.count;
}

/*
 * The session's error (enum QNetError): the receive, send and link errors
 * first, then QNET_ERR_LOST when the local player is lost, then the network
 * interface's. 0 when there is none.
 */
s32 QuestNet_GetError(void)
{
    QNet *net = &gQNet;

    if (net->recvError != 0) {
        return net->recvError;
    }
    if (net->sendError != 0) {
        return net->sendError;
    }
    if (net->linkError != 0) {
        return net->linkError;
    }
    if (net->peerState[net->self] == QNET_PEER_LOST) {
        return QNET_ERR_LOST;
    }
    return (NetIf_GetError() != 0) ? QNET_ERR_NETIF : QNET_ERR_NONE;
}

/*
 * Whether a game packet may go now (enum QNetSendBlock): the local player
 * must be joined or ready, under the second's byte budget, and
 * sendInterval past the last send.
 */
s32 QuestNet_CanSend(void)
{
    u8 state = gQNet.peerState[gQNet.self];

    if (state != QNET_PEER_JOINED && state != QNET_PEER_READY) {
        return QNET_SEND_NOT_JOINED;
    }
    if (gQNet.sendBudget <= gQNet.sent) {
        return QNET_SEND_OVER_BUDGET;
    }
    if (gQNet.now < gQNet.lastSendTime + gQNet.sendInterval) {
        return QNET_SEND_TOO_SOON;
    }
    return QNET_CAN_SEND;
}

/*
 * A joined or ready peer's ping, in units of 1/15 second (66.7 ms), at
 * least 1; 0 for the local player or a peer with no ping.
 */
s32 QuestNet_GetPing(u8 peer)
{
    s32 ping;

    if (gQNet.count <= peer) {
        return 0;
    }
    if (gQNet.self == peer) {
        return 0;
    }
    if (gQNet.peerState[peer] != QNET_PEER_JOINED && gQNet.peerState[peer] != QNET_PEER_READY) {
        return 0;
    }
    if (gQNetPing[peer] <= 0.0f) {
        return 0;
    }
    ping = 1000.0f * gQNetPing[peer] / 66.6667f;
    if (ping == 0) {
        ping = 1;
    }
    return ping;
}

/* 1 when no player is ready. */
s32 QuestNet_NoPeerReady(void)
{
    s32 i;

    for (i = 0; i < gQNet.count; i++) {
        if (gQNet.peerState[i] == QNET_PEER_READY) {
            break;
        }
    }
    return i == gQNet.count;
}

/*
 * Queues a game message (its length in its header) while the session is
 * active.
 */
void QuestNet_Post(void *msg)
{
    if (gQNet.active != 0) {
        QuestNet_Queue(&gQNet, &gQNetGameSend, msg);
    }
}

/*
 * Sends the queued game messages when the session has no error and a
 * send may go now.
 */
void QuestNet_Flush(void)
{
    if ((gQNet.active != 0) && (QuestNet_GetError() == 0) && (QuestNet_CanSend() == QNET_CAN_SEND)) {
        QuestNet_Send(&gQNet, &gQNetGameSend, QNET_CHAN_GAME);
    }
}

/* Handles the game messages received, while the session is active. */
void QuestNet_Receive(void)
{
    if (gQNet.active != 0) {
        QuestNet_ProcessQueue(&gQNet, &gQNetGameRecv);
    }
}

/* Steps the session a frame while it is active (QNET_MODE_STEPPED). */
void QuestNet_Tick(void)
{
    if (gQNet.active != 0) {
        switch (gQNet.mode) {
        case QNET_MODE_STEPPED:
            QuestNet_Update(&gQNet);
            break;
        }
    }
}

/*
 * Marks the local player ready and tells the peers, with the seconds it may
 * stay so (timeout, 0: joinTimeout's). Only while joined.
 */
void QuestNet_Ready(u8 timeout)
{
    if (gQNet.peerState[gQNet.self] == QNET_PEER_JOINED) {
        if (timeout == 0) {
            timeout = gQNet.joinTimeout;
        }
        QuestNet_SysReady(&gQNet, timeout);
        QuestNet_SetPeerState(&gQNet, gQNet.self, QNET_PEER_READY);
    }
}

/*
 * Marks the local player joined again and tells the peers. Only while
 * ready.
 */
void QuestNet_Unready(void)
{
    if (gQNet.peerState[gQNet.self] == QNET_PEER_READY) {
        QuestNet_SysUnready(&gQNet);
        QuestNet_SetPeerState(&gQNet, gQNet.self, QNET_PEER_JOINED);
    }
}

/*
 * The local player leaves: tells the peers and stops the session's steps
 * but receiving and sending (QNET_STATE_LEAVE). Only while joined.
 */
void QuestNet_Leave(void)
{
    if (gQNet.peerState[gQNet.self] == QNET_PEER_JOINED) {
        QuestNet_SysLeave(&gQNet);
        QuestNet_SetPeerState(&gQNet, gQNet.self, QNET_PEER_LEFT);
        gQNet.state = QNET_STATE_LEAVE;
    }
}

/* Ends the session: inactive, QNET_STATE_CLOSED, every player gone. */
void QuestNet_Close(void)
{
    s32 i;

    gQNet.active = 0;
    gQNet.state = QNET_STATE_CLOSED;
    for (i = 0; i < gQNet.count; i++) {
        QuestNet_SetPeerState(&gQNet, i, QNET_PEER_NONE);
    }
}
