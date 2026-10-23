/*
 * NETWORK: the EE side of the AVE TCP wrapper, an IOP module reached as
 * the SIF RPC server AVE_WRAP_RPC_ID. AveWrap_Bind binds gAveWrapClient
 * to it. Each AveIf_*, AveTcp_*, AveRoute_*, AvePpp_*, AveEth_*, AveDhcp_*
 * and AveDns_* function takes gAveWrapSema, fills the shared buffer
 * gAveWrapBuf (AveWrapBuf: three argument words, the result and the
 * AvePayload member of the call), sends it as one AVE_FN_* call and reads
 * the reply back from the same buffer: a halfword result (negative is an
 * error, AVE_ERR_RPC when the RPC itself fails) and any outputs in the
 * payload. The sizes sent and received are rounded up to 16 bytes.
 *
 * The calls are grouped by layer, the groups in the order of the
 * wrapper's call numbers (AVE_FN_*). The interface's event and option
 * calls (AVE_FN_IF_POLL_EVENT on) have the highest numbers, so they come
 * last, apart from the other AveIf_* calls. Every layer is the same
 * client of one RPC server, through one buffer and one lock, so the file
 * is one subject and isn't cut by layer.
 *
 * The callers are the Net* layer (NetIf_*, NetPpp_*, NetTcp_*, NetDns_*),
 * which turns the game's network settings into these calls, and the
 * sockets and DNS cache built on it. The library behind the wrapper
 * stays assembly. The file stays byte-identical for online play.
 */
#include "common.h"
#include "sif.h"
#include "ave.h"
#include "libkernl.h"

extern s32 gAveWrapSema;
extern s16 gAveWrapUnused;
extern s16 gAveWrapNoWait;
extern SifClientData gAveWrapClient;
extern AveWrapBuf gAveWrapBuf;

#define ALIGN16(x) (((x) % 16 != 0) ? (16 - (x) % 16) + (x) : (x))

void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, void *src, u32 n);
char *strcpy(char *dst, char *src);
char *strncpy(char *dst, char *src, u32 n);

/*
 * Creates the semaphore gAveWrapSema if it does not exist yet (it starts
 * negative), clears gAveWrapNoWait and gAveWrapUnused, then binds
 * gAveWrapClient to the RPC server AVE_WRAP_RPC_ID of the wrapper,
 * retrying after a short busy wait until the bind succeeds and the
 * server is there. The wait counter is volatile so the empty loop is
 * kept. Returns 0.
 */
s32 AveWrap_Bind(void)
{
    SemaParam semaParam;
    volatile s32 i;

    if (gAveWrapSema < 0) {
        memset(&semaParam, 0, sizeof(semaParam));
        semaParam.initCount = 1;
        semaParam.maxCount = 1;
        gAveWrapSema = CreateSema(&semaParam);
    }
    gAveWrapNoWait = 0;
    gAveWrapUnused = 0;
    for (;;) {
        if (sceSifBindRpc(&gAveWrapClient, AVE_WRAP_RPC_ID, 0) >= 0 && gAveWrapClient.serve != NULL) {
            break;
        }
        for (i = 0x10000; i != 0; i--) {
        }
    }
    return 0;
}

/* Forgets the RPC binding by clearing gAveWrapClient. Returns 0. */
s32 AveWrap_Unbind(void)
{
    memset(&gAveWrapClient, 0, sizeof(SifClientData));
    return 0;
}

/*
 * Sends gAveWrapBuf to the wrapper as call funcNo and waits for the
 * reply, which comes back into gAveWrapBuf. sendSize and recvSize are
 * the bytes sent and received, header included; both are rounded up to
 * 16. Writes the buffer back from the data cache first. When
 * gAveWrapNoWait is set it starts the call without waiting instead
 * (AveWrap_CallAsync). Returns the halfword result of the reply
 * (negative is an error), or AVE_ERR_RPC when the RPC call fails.
 */
s16 AveWrap_Call(s32 funcNo, s32 sendSize, s32 recvSize)
{
    if (gAveWrapNoWait != 0) {
        return AveWrap_CallAsync(funcNo, sendSize, recvSize);
    }
    SyncDCache(&gAveWrapBuf, &gAveWrapBuf.data.raw[sizeof(gAveWrapBuf.data.raw) - 1]);
    recvSize = ALIGN16(recvSize);
    sendSize = ALIGN16(sendSize);
    if (sceSifCallRpc(&gAveWrapClient, funcNo, 0, &gAveWrapBuf, sendSize, &gAveWrapBuf, recvSize, NULL, NULL) < 0) {
        return AVE_ERR_RPC;
    }
    return gAveWrapBuf.result.code;
}

/*
 * Starts the call like AveWrap_Call but does not wait for the reply.
 * Returns 0 when the call was started, AVE_ERR_RPC when an earlier call
 * is still running or the RPC call fails.
 */
s16 AveWrap_CallAsync(s32 funcNo, s32 sendSize, s32 recvSize)
{
    if (AveWrap_IsIdle() == 0) {
        return AVE_ERR_RPC;
    }
    SyncDCache(&gAveWrapBuf, &gAveWrapBuf.data.raw[sizeof(gAveWrapBuf.data.raw) - 1]);
    recvSize = ALIGN16(recvSize);
    sendSize = ALIGN16(sendSize);
    if (sceSifCallRpc(&gAveWrapClient, funcNo, 1, &gAveWrapBuf, sendSize, &gAveWrapBuf, recvSize, NULL, NULL) < 0) {
        return AVE_ERR_RPC;
    }
    return 0;
}

/* Returns 1 when no call to the wrapper is in progress, 0 while one is. */
s32 AveWrap_IsIdle(void)
{
    return (s16)!sceSifCheckStatRpc(&gAveWrapClient);
}

/*
 * Takes gAveWrapSema; every wrapper call holds it while it uses
 * gAveWrapBuf.
 */
void AveWrap_Lock(void)
{
    WaitSema(gAveWrapSema);
}

/* Releases gAveWrapSema. */
void AveWrap_Unlock(void)
{
    SignalSema(gAveWrapSema);
}

/*
 * Brings the network interface up: sends ifFlag and ifKind in the first
 * two argument words of the buffer (AVE_FN_IF_UP). NetIf_Up picks the
 * pair from the device type and driver (seen: ifFlag is 1 only for the
 * AN986 and LANEGG Ethernet drivers). Returns the wrapper result.
 */
s16 AveIf_Up(s32 ifFlag, s32 ifKind)
{
    s16 callResult;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = ifFlag;
    gAveWrapBuf.arg[1] = ifKind;
    callResult = AveWrap_Call(AVE_FN_IF_UP, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Takes the network interface down (AVE_FN_IF_DOWN). Returns the
 * wrapper result.
 */
s16 AveIf_Down(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_IF_DOWN, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Gives the interface its address, netmask and broadcast address
 * (AVE_FN_IF_SET_ADDRESS). Returns the wrapper result.
 */
s16 AveIf_SetAddress(s32 address, s32 netmask, s32 broadcast)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->ifAddr.address = address;
    payload->ifAddr.netmask = netmask;
    payload->ifAddr.broadcast = broadcast;
    callResult = AveWrap_Call(AVE_FN_IF_SET_ADDRESS, 44, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Removes the address of the interface (AVE_FN_IF_CLEAR_ADDRESS).
 * Returns the wrapper result.
 */
s16 AveIf_ClearAddress(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_IF_CLEAR_ADDRESS, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Sets option number option of TCP connection sock to the len bytes at
 * src (AVE_FN_TCP_SET_OPTION; at most 256 bytes fit). Returns the
 * wrapper result.
 */
s16 AveTcp_SetOption(s16 sock, s16 option, s16 len, void *src)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->tcpOpt.sock = sock;
    payload->tcpOpt.option = option;
    payload->tcpOpt.len = len;
    memcpy(payload->tcpOpt.data, src, len);
    callResult = AveWrap_Call(AVE_FN_TCP_SET_OPTION, 294, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads option number option of TCP connection sock
 * (AVE_FN_TCP_GET_OPTION). The result is the length of the option: when
 * it is no more than len, that many bytes are copied to dst. Returns the
 * result (negative on error).
 */
s16 AveTcp_GetOption(s16 sock, s16 option, s16 len, void *dst)
{
    s16 callResult;
    AvePayload *payload;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->tcpOpt.sock = sock;
    payload->tcpOpt.option = option;
    payload->tcpOpt.len = len;
    callResult = AveWrap_Call(AVE_FN_TCP_GET_OPTION, 294, 294);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    if (callResult <= len) {
        memcpy(dst, payload->tcpOpt.data, callResult);
    }
    AveWrap_Unlock();
    return callResult;
}

/*
 * Opens a TCP connection to address addr, port port (AVE_FN_TCP_OPEN).
 * openArg is sent with them (seen: 0, or NetAddr.openArg). Returns the
 * wrapper result: the handle of the new connection, or negative on
 * error.
 */
s16 AveTcp_Open(s32 addr, u16 port, u16 openArg)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->tcpOpen.addr = addr;
    payload->tcpOpen.port = port;
    payload->tcpOpen.openArg = openArg;
    callResult = AveWrap_Call(AVE_FN_TCP_OPEN, 40, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Aborts TCP connection sock (AVE_FN_TCP_ABORT). Returns the wrapper
 * result.
 */
s16 AveTcp_Abort(s16 sock)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->tcpIo.sock = sock;
    callResult = AveWrap_Call(AVE_FN_TCP_ABORT, 34, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads the state of TCP connection sock (AVE_FN_TCP_STATUS). On success
 * stores the connection state, the room left to send and the bytes
 * waiting to be read in *state, *sendSpace and *recvLen. Returns the
 * wrapper result (negative on error, the outputs then unchanged).
 */
s16 AveTcp_Status(s16 sock, u16 *state, u16 *sendSpace, u16 *recvLen)
{
    s16 callResult;
    AvePayload *payload;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->tcpStatus.sock = sock;
    callResult = AveWrap_Call(AVE_FN_TCP_STATUS, 42, 42);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    *state = payload->tcpStatus.state;
    *sendSpace = payload->tcpStatus.sendSpace;
    *recvLen = payload->tcpStatus.recvLen;
    AveWrap_Unlock();
    return callResult;
}

/*
 * Sends len bytes from data on TCP connection sock (AVE_FN_TCP_SEND), in
 * pieces of at most AVE_TCP_CHUNK_MAX bytes with a short busy wait
 * between pieces. Stops at the first error. Returns the result for the
 * last piece, the error, or 0 when len is 0 or less.
 * The size limit is written as an if/else that sets chunkLen in both
 * arms, and the wait counter is stepped before the seven nops of the
 * inline asm: the compiler lays out the branches and the two loops as
 * the game's code has them only in this form.
 */
s16 AveTcp_Send(s16 sock, u8 *data, s16 len)
{
    volatile s32 i;
    s16 callResult;
    AvePayload *payload;
    s16 chunkLen;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    callResult = 0;
    payload->tcpIo.sock = sock;
    while (0 < len) {
        if (len > AVE_TCP_CHUNK_MAX) {
            chunkLen = AVE_TCP_CHUNK_MAX;
        } else {
            chunkLen = len;
        }
        payload->tcpIo.len = chunkLen;
        memcpy(payload->tcpIo.data, data, chunkLen);
        gAveWrapBuf.arg[0] = 0;
        gAveWrapBuf.arg[1] = 0;
        callResult = AveWrap_Call(AVE_FN_TCP_SEND, chunkLen + 38, chunkLen + 38);
        if (callResult < 0) {
            AveWrap_Unlock();
            return callResult;
        }
        len -= chunkLen;
        data += chunkLen;
        if (len <= 0) {
            break;
        }
        i = 0;
        while (i < 10000) {
            i++;
            __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop");
        }
    }
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads up to len bytes (at most AVE_TCP_CHUNK_MAX) from TCP connection
 * sock into buf (AVE_FN_TCP_RECV). Returns the bytes copied: the length
 * in the reply when it is no more than the size asked for, else that
 * size; 0 when len is 0 or less; negative on error.
 */
s16 AveTcp_Recv(s16 sock, void *buf, s16 len)
{
    s16 callResult;
    AvePayload *payload;

    if (len <= 0) {
        return 0;
    }
    AveWrap_Lock();
    if (len > AVE_TCP_CHUNK_MAX) {
        len = AVE_TCP_CHUNK_MAX;
    }
    payload = &gAveWrapBuf.data;
    payload->tcpIo.sock = sock;
    payload->tcpIo.len = len;
    callResult = AveWrap_Call(AVE_FN_TCP_RECV, len + 38, len + 38);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    if (gAveWrapBuf.result.len <= len) {
        len = gAveWrapBuf.result.len;
    }
    if (0 < len) {
        memcpy(buf, payload->tcpIo.data, len);
    }
    AveWrap_Unlock();
    return len;
}

/*
 * Frees the handle of TCP connection sock (AVE_FN_TCP_DELETE). Returns
 * the wrapper result.
 */
s16 AveTcp_Delete(s16 sock)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->tcpIo.sock = sock;
    callResult = AveWrap_Call(AVE_FN_TCP_DELETE, 34, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Adds a route to dest through gateway (AVE_FN_ROUTE_ADD; seen: dest 0,
 * the default route). Returns the wrapper result.
 */
s16 AveRoute_Add(s32 dest, s32 gateway)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->route.dest = dest;
    payload->route.gateway = gateway;
    callResult = AveWrap_Call(AVE_FN_ROUTE_ADD, 40, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Removes the route AveRoute_Add made (AVE_FN_ROUTE_DELETE). Returns
 * the wrapper result.
 */
s16 AveRoute_Delete(s32 dest, s32 gateway)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->route.dest = dest;
    payload->route.gateway = gateway;
    callResult = AveWrap_Call(AVE_FN_ROUTE_DELETE, 40, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Starts the PPP layer of the wrapper (AVE_FN_PPP_INIT). Returns the
 * wrapper result.
 */
s16 AvePpp_Init(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_PPP_INIT, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Stops the PPP layer of the wrapper (AVE_FN_PPP_TERM). Returns the
 * wrapper result.
 */
s16 AvePpp_Term(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_PPP_TERM, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Dials with the connection settings: phyMode, isPppoe and lineMode go
 * in the three argument words; the payload (AveConfigArgs) gets the two
 * modem chat scripts, the configuration text, the account name and key, the
 * auth text, the auth flag and the five modem options, copied from
 * settings (AVE_FN_PPP_CONNECT). Returns the wrapper result.
 */
s16 AvePpp_Connect(AveConfig *settings)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = settings->phyMode;
    payload = &gAveWrapBuf.data;
    gAveWrapBuf.arg[1] = settings->isPppoe;
    gAveWrapBuf.arg[2] = settings->lineMode;
    memset(payload, 0, sizeof(AveConfigArgs));
    payload->pppConfig.unk0 = settings->unk0;
    strcpy(payload->pppConfig.chat0, settings->chat0);
    strcpy(payload->pppConfig.chat1, settings->chat1);
    strcpy(payload->pppConfig.confText, settings->confText);
    strcpy(payload->pppConfig.authName, settings->authName);
    strcpy(payload->pppConfig.authKey, settings->authKey);
    payload->pppConfig.authFlag = settings->authFlag;
    strcpy(payload->pppConfig.authText, settings->authText);
    payload->pppConfig.modemOpt[0] = settings->modemOpt[0];
    payload->pppConfig.modemOpt[1] = settings->modemOpt[1];
    payload->pppConfig.modemOpt[2] = settings->modemOpt[2];
    payload->pppConfig.modemOpt[3] = settings->modemOpt[3];
    payload->pppConfig.modemOpt[4] = settings->modemOpt[4];
    callResult = AveWrap_Call(AVE_FN_PPP_CONNECT, 814, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Hangs up the PPP connection (AVE_FN_PPP_DISCONNECT). Returns the
 * wrapper result.
 */
s16 AvePpp_Disconnect(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_PPP_DISCONNECT, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads the status of the PPP connection (AVE_FN_PPP_GET_STATUS) and
 * copies it to *out (AveInfo: phase, speed, error, the DNS servers PPP
 * was given). Returns the wrapper result (negative on error, *out then
 * unchanged).
 */
s16 AvePpp_GetStatus(AveInfo *out)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_PPP_GET_STATUS, 32, 76);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    payload = &gAveWrapBuf.data;
    *out = payload->pppStatus;
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads one status word of the PPP device (AVE_FN_PPP_GET_LINK_STATE)
 * into *out. Returns the wrapper result; NetPpp_GetLinkState uses *out
 * only when the result is 1, and treats anything else as no device.
 */
s16 AvePpp_GetLinkState(s32 *out)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_PPP_GET_LINK_STATE, 32, 76);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    payload = &gAveWrapBuf.data;
    *out = payload->linkState;
    AveWrap_Unlock();
    return callResult;
}

/*
 * Sets PPP option number option to the len bytes at src
 * (AVE_FN_PPP_SET_OPTION; at most 256 bytes fit; seen: option 1 with
 * a 4-byte value). Returns the wrapper result.
 */
s16 AvePpp_SetOption(s16 option, s16 len, void *src)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->pppOpt.option = option;
    payload->pppOpt.len = len;
    memcpy(payload->pppOpt.data, src, len);
    callResult = AveWrap_Call(AVE_FN_PPP_SET_OPTION, 292, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Starts the Ethernet layer of the wrapper (AVE_FN_ETH_INIT). Returns
 * the wrapper result.
 */
s16 AveEth_Init(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_ETH_INIT, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Stops the Ethernet layer of the wrapper (AVE_FN_ETH_TERM). Returns
 * the wrapper result.
 */
s16 AveEth_Term(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_ETH_TERM, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Starts the Ethernet link in mode phyMode (gNetPhyMode, sent in the
 * first argument word; AVE_FN_ETH_START). Returns the wrapper result.
 */
s16 AveEth_Start(s16 phyMode)
{
    s16 callResult;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = phyMode;
    callResult = AveWrap_Call(AVE_FN_ETH_START, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Asks whether the Ethernet link is up (AVE_FN_ETH_IS_LINK_UP). Returns
 * the wrapper result: nonzero once the link is up (seen: the connect
 * steps wait for nonzero).
 */
s16 AveEth_IsLinkUp(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_ETH_IS_LINK_UP, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/* Stops the Ethernet link (AVE_FN_ETH_STOP). Returns the wrapper result. */
s16 AveEth_Stop(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_ETH_STOP, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Starts the DHCP client of the wrapper (AVE_FN_DHCP_INIT). Returns the
 * wrapper result.
 */
s16 AveDhcp_Init(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_DHCP_INIT, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Stops the DHCP client of the wrapper (AVE_FN_DHCP_TERM). Returns the
 * wrapper result.
 */
s16 AveDhcp_Term(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_DHCP_TERM, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads what DHCP gave for name resolution (AVE_FN_DHCP_GET_DNS): copies
 * the domain name string to domain and the two DNS server addresses to
 * *primaryDns and *secondaryDns. Returns the wrapper result (negative on error, the
 * outputs then unchanged).
 */
s16 AveDhcp_GetDns(char *domain, s32 *primaryDns, s32 *secondaryDns)
{
    s16 callResult;
    AvePayload *payload;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_DHCP_GET_DNS, 32, 296);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    payload = &gAveWrapBuf.data;
    strcpy(domain, payload->dns.domain);
    *primaryDns = payload->dns.primaryDns;
    *secondaryDns = payload->dns.secondaryDns;
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads the router (gateway) address DHCP gave into *out
 * (AVE_FN_DHCP_GET_ROUTER). Returns the wrapper result (negative on
 * error, *out then unchanged).
 */
s16 AveDhcp_GetRouter(s32 *out)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_DHCP_GET_ROUTER, 32, 36);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    payload = &gAveWrapBuf.data;
    *out = payload->router;
    AveWrap_Unlock();
    return callResult;
}

/*
 * Starts asking for an address by DHCP (AVE_FN_DHCP_START). address and
 * dhcpArg are sent as given (seen: 0 and 5). Returns the wrapper result.
 */
s16 AveDhcp_Start(s32 address, s32 dhcpArg)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->dhcp.address = address;
    payload->dhcp.dhcpArg = dhcpArg;
    callResult = AveWrap_Call(AVE_FN_DHCP_START, 40, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads the address and netmask DHCP gave, and a third word (seen: its
 * caller drops it), into *address, *netmask and *broadcast
 * (AVE_FN_DHCP_GET_ADDRESS). Returns the wrapper result (negative on
 * error, the outputs then unchanged).
 */
s16 AveDhcp_GetAddress(s32 *address, s32 *netmask, s32 *broadcast)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_DHCP_GET_ADDRESS, 32, 44);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    payload = &gAveWrapBuf.data;
    *address = payload->ifAddr.address;
    *netmask = payload->ifAddr.netmask;
    *broadcast = payload->ifAddr.broadcast;
    AveWrap_Unlock();
    return callResult;
}

/*
 * Returns the state of the DHCP client (AVE_FN_DHCP_GET_STATE); seen:
 * the connect steps take 3 as address obtained and negative as failed.
 */
s16 AveDhcp_GetState(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_DHCP_GET_STATE, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Gives the leased address back (AVE_FN_DHCP_RELEASE); dhcpArg is sent
 * as given (seen: 5). Returns the wrapper result.
 */
s16 AveDhcp_Release(s32 address, s32 dhcpArg)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->dhcp.address = address;
    payload->dhcp.dhcpArg = dhcpArg;
    callResult = AveWrap_Call(AVE_FN_DHCP_RELEASE, 40, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Starts the resolver with domain name domain (an empty string when
 * NULL) and the DNS servers primaryDns and secondaryDns (AVE_FN_DNS_INIT). Returns
 * the wrapper result.
 */
s16 AveDns_Init(char *domain, s32 primaryDns, s32 secondaryDns)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    if (domain != NULL) {
        strcpy(payload->dns.domain, domain);
    } else {
        payload->dns.domain[0] = 0;
    }
    payload->dns.primaryDns = primaryDns;
    payload->dns.secondaryDns = secondaryDns;
    callResult = AveWrap_Call(AVE_FN_DNS_INIT, 296, 32);
    AveWrap_Unlock();
    return callResult;
}

/* Stops the resolver (AVE_FN_DNS_TERM). Returns the wrapper result. */
s16 AveDns_Term(void)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_DNS_TERM, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Starts looking up host name name (copied with at most 511
 * characters and always terminated; AVE_FN_DNS_REQUEST). Returns the
 * wrapper result: the id of the request for AveDns_LookUp and
 * AveDns_Free, or negative on error.
 */
s16 AveDns_Request(char *name)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    strncpy(payload->dnsName, name, sizeof(payload->dnsName));
    payload->dnsName[sizeof(payload->dnsName) - 1] = 0;
    callResult = AveWrap_Call(AVE_FN_DNS_REQUEST, 544, 32);
    AveWrap_Unlock();
    return callResult;
}

/* Frees DNS request id (AVE_FN_DNS_FREE). Returns the wrapper result. */
s16 AveDns_Free(s16 id)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->dnsLookUp.id = id;
    callResult = AveWrap_Call(AVE_FN_DNS_FREE, 34, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Asks for the answer to DNS request id (AVE_FN_DNS_LOOKUP); when the
 * result is not negative stores the address found in *addr. Returns the
 * wrapper result (seen: NetDns_LookUp tells -6 and -2 apart from other
 * errors).
 */
s16 AveDns_LookUp(s16 id, s32 *addr)
{
    s16 callResult;
    AvePayload *payload;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->dnsLookUp.id = id;
    callResult = AveWrap_Call(AVE_FN_DNS_LOOKUP, 40, 40);
    if (0 <= callResult) {
        *addr = payload->dnsLookUp.addr;
    }
    AveWrap_Unlock();
    return callResult;
}

/*
 * Asks whether the interface has an event waiting
 * (AVE_FN_IF_POLL_EVENT); when the result is not negative stores the
 * first argument word of the reply in *out. Returns the wrapper result:
 * nonzero when an event is waiting (seen: NetIf_PollEvent).
 */
s16 AveIf_PollEvent(s32 *out)
{
    s16 callResult;

    AveWrap_Lock();
    callResult = AveWrap_Call(AVE_FN_IF_POLL_EVENT, 32, 32);
    if (0 <= callResult) {
        *out = gAveWrapBuf.arg[0];
    }
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads the waiting interface event (AVE_FN_IF_GET_EVENT): sends
 * selector (seen: 0) and stores the first three words of the reply in
 * *status, *eventArgA and *eventArgB, whatever the result. Returns the wrapper
 * result (seen: positive when an event was read).
 */
s16 AveIf_GetEvent(s32 selector, s32 *status, s32 *eventArgA, s32 *eventArgB)
{
    AvePayload *payload;
    s16 callResult;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = selector;
    callResult = AveWrap_Call(AVE_FN_IF_GET_EVENT, 32, 812);
    payload = &gAveWrapBuf.data;
    *status = payload->ifEvent.status;
    *eventArgA = payload->ifEvent.eventArgA;
    *eventArgB = payload->ifEvent.eventArgB;
    AveWrap_Unlock();
    return callResult;
}

/*
 * Acknowledges the event AveIf_GetEvent read, sending its status
 * back (AVE_FN_IF_ACK_EVENT). Returns the wrapper result (seen: 0 on
 * success).
 */
s16 AveIf_AckEvent(s32 status)
{
    s16 callResult;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = status;
    callResult = AveWrap_Call(AVE_FN_IF_ACK_EVENT, 32, 32);
    AveWrap_Unlock();
    return callResult;
}

/*
 * Reads interface option number option (AVE_FN_IF_GET_OPTION). The
 * result is the length of the option: when it is no more than len, that
 * many bytes are copied to dst. Returns the result (negative on error).
 * Seen: option 8, 4 bytes, nonzero once the connection is usable.
 */
s16 AveIf_GetOption(s32 option, s32 len, void *dst)
{
    s16 callResult;
    AvePayload *payload;

    AveWrap_Lock();
    payload = &gAveWrapBuf.data;
    payload->ifOpt.option = option;
    payload->ifOpt.len = len;
    callResult = AveWrap_Call(AVE_FN_IF_GET_OPTION, 296, 296);
    if (callResult < 0) {
        AveWrap_Unlock();
        return callResult;
    }
    if (callResult <= len) {
        memcpy(dst, payload->ifOpt.data, callResult);
    }
    AveWrap_Unlock();
    return callResult;
}

/* Starts negative, so that the first AveWrap_Bind creates the semaphore. */
s32 gAveWrapSema = {
#include "sub_main/00405178.inc"
};
