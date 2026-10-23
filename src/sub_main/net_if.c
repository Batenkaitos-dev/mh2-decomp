/*
 * NETWORK: the network's interface layer over the AVE wrapper (the
 * AveWrap_* and Ave* calls, which pass each request to the IOP's AVE TCP
 * server). It brings the interface up and down (NetIf_*), dials and hangs
 * up PPP (NetPpp_*, with the modem chat scripts of NetModem_GetChat),
 * runs a LAN link (NetEth_*, NetRoute_*, NetDhcp_*), the resolver
 * (NetDns_*) and the TCP sockets (NetTcp_*), which the lobby, the quest
 * network code and the HTTP client use.
 *
 * gNetIfState follows the connection (NET_IF_*): up after NetIf_Up,
 * connecting after NetPpp_Connect or NetEth_Start, connected after
 * NetPpp_SetConnected or NetEth_SetConnected, closed after
 * NetPpp_SetClosed or NetEth_ClearAddress, stopping after
 * NetPpp_Disconnect or NetEth_Stop, down after NetIf_Down.
 * NetIf_PollError watches a connection and sets gNetIfError.
 * gNetIfStarted keeps one flag per part started (NET_STARTED_*).
 * AVE's results are returned unchanged (NetTcp_Result), except that the
 * status calls translate AVE's states into this file's values.
 *
 * The file's .rodata is the jump tables of NetTcp_Status, NetPpp_Connect
 * and NetPpp_GetStatus. It stays byte-identical for online play.
 */
#include "common.h"
#include "sif.h"
#include "ave.h"
#include "net_tcp.h"
#include "net_sockets.h"


/*
 * The file's .bss. The compiler places uninitialised variables in the
 * reverse order of their declarations, so they are declared from the
 * highest address down.
 */
char gModemInitChat[256];
char gModemChat2[256];
u8 gNetIfStarted[NET_STARTED_COUNT];
s32 gNetIfError;
s32 gNetIfNoErrorCheck;
s32 gNetIfState;
s32 gNetPendingEvent;
u8 gNetEventPending;
/* The modem chat scripts NetModem_GetChat chooses from (the file's .data). */
static char gModemChatCxt[256] = {
#include "sub_main/00405180.inc"
};
static char gModemChatOst[256] = {
#include "sub_main/00405280.inc"
};
static char gModemChatDefault[33] = {
#include "sub_main/00405380.inc"
};

/* NetTcp_IsIdle takes its result as an s16 */
/* unit's own declaration: declared as this file's callers use it; with the definition's s16 (s16 a, s16 len, void *src), NetPpp_SetOption1 compiles differently */
s32 AvePpp_SetOption(s16 a, s16 len, void *src);
s16 AveDns_Request(char *name);
char *strcat(char *dst, char *src);
s32 sprintf(char *dst, char *fmt, ...);

/*
 * Clears the pending network event, the connection state (gNetIfState)
 * and the error-check switch, then binds the AVE wrapper's RPC client
 * (AveWrap_Bind). Returns 0.
 */
s32 NetTcp_Init(void)
{
    gNetEventPending = 0;
    gNetPendingEvent = 0;
    gNetIfState = NET_IF_DOWN;
    gNetIfNoErrorCheck = 0;
    AveWrap_Bind();
    return 0;
}

/*
 * Clears the same state as NetTcp_Init and unbinds the AVE wrapper's RPC
 * client (AveWrap_Unbind). Returns 0.
 */
s32 NetTcp_Term(void)
{
    gNetEventPending = 0;
    gNetPendingEvent = 0;
    gNetIfState = NET_IF_DOWN;
    gNetIfNoErrorCheck = 0;
    AveWrap_Unbind();
    return 0;
}

/*
 * Passes an AVE result code through unchanged; every NetTcp_* call
 * returns its AVE result through it.
 */
s32 NetTcp_Result(s32 result)
{
    return result;
}

/*
 * Returns nonzero when the AVE wrapper has no RPC call in flight
 * (AveWrap_IsIdle), taken as an s16.
 */
s32 NetTcp_IsIdle(void)
{
    return (s16)AveWrap_IsIdle();
}

/* Returns 1 while gNetIfState is NET_IF_CONNECTED, else 0. */
s32 NetIf_IsConnected(void)
{
    return gNetIfState == NET_IF_CONNECTED;
}

/*
 * Returns gNetIfState (NET_IF_*: down, up, connecting, connected, closed,
 * stopping).
 */
s32 NetIf_GetState(void)
{
    return gNetIfState;
}

/*
 * Turns NetIf_PollError's checks on (enable nonzero) or off. Returns the
 * new value of gNetIfNoErrorCheck (1 when off).
 */
s32 NetIf_SetErrorCheck(s32 enable)
{
    return gNetIfNoErrorCheck = !enable;
}

/*
 * Checks the connection and returns gNetIfError. Does nothing once an
 * error is set, while the checks are off (NetIf_SetErrorCheck) or while
 * the AVE wrapper is busy. devType is the device type (NET_DEV_*). For
 * Ethernet: NET_IF_ERR_QUERY when useDhcp is set and NetDhcp_GetState
 * fails, NET_IF_ERR_LINK_LOST when an interface event is waiting
 * (NetIf_PollEvent). For a modem or other PPP device:
 * NET_IF_ERR_QUERY when NetPpp_GetStatus fails, NET_IF_ERR_LINK_LOST when
 * the PPP phase is not NET_PPP_PHASE_UP. The main loop calls it every 32
 * frames while connected.
 */
s32 NetIf_PollError(s32 devType, s32 useDhcp)
{
    NetConnInfo info;
    s32 event;

    if (gNetIfError != 0) {
        return gNetIfError;
    }
    if (gNetIfNoErrorCheck != 0) {
        return gNetIfError;
    }
    if (NetTcp_IsIdle() == 0) {
        return gNetIfError;
    }
    switch (devType) {
    case NET_DEV_ETHERNET:
        if (useDhcp != 0 && NetDhcp_GetState() < 0) {
            gNetIfError = NET_IF_ERR_QUERY;
            break;
        }
        switch (NetIf_PollEvent(&event)) {
        case 0:
            break;
        case 1:
            gNetIfError = NET_IF_ERR_LINK_LOST;
            break;
        }
        break;
    case NET_DEV_MODEM:
    case NET_DEV_PPP_OTHER:
        if (NetPpp_GetStatus(&info) < 0) {
            gNetIfError = NET_IF_ERR_QUERY;
        } else if (info.phase != NET_PPP_PHASE_UP) {
            gNetIfError = NET_IF_ERR_LINK_LOST;
        }
        break;
    }
    return gNetIfError;
}

/* Returns gNetIfError (0, NET_IF_ERR_QUERY or NET_IF_ERR_LINK_LOST). */
s32 NetIf_GetError(void)
{
    return gNetIfError;
}

/*
 * Brings the AVE interface up (AveIf_Up) with a pair of values chosen by
 * the device type and driver (NET_DEV_*, NET_DRV_*), marks the interface
 * started and sets gNetIfState to NET_IF_UP. Returns AveIf_Up's result
 * (undefined for a device type other than 1-3).
 */
s16 NetIf_Up(s32 devType, s32 driver)
{
    s16 result;

    switch (devType) {
    case NET_DEV_PPP_OTHER:
    case NET_DEV_MODEM:
        switch (driver) {
        case NET_DRV_SMAP:
            result = AveIf_Up(0, 1);
            break;
        default:
            result = AveIf_Up(0, 4);
            break;
        }
        break;
    case NET_DEV_ETHERNET:
        switch (driver) {
        case NET_DRV_AN986:
        case NET_DRV_LANEGG:
            result = AveIf_Up(1, 5);
            break;
        default:
            result = AveIf_Up(0, 2);
            break;
        }
        break;
    }
    gNetIfStarted[NET_STARTED_IF] = 1;
    gNetIfState = NET_IF_UP;
    return result;
}

/*
 * Takes the AVE interface down, clears its started flag, sets gNetIfState
 * to NET_IF_DOWN and clears gNetIfError. Returns AveIf_Down's result.
 */
s16 NetIf_Down(void)
{
    s16 result = AveIf_Down();

    gNetIfStarted[NET_STARTED_IF] = 0;
    gNetIfState = NET_IF_DOWN;
    gNetIfError = 0;
    return result;
}

/*
 * Opens a TCP connection to dest's address and port (AveTcp_Open).
 * Returns the socket handle, or a negative AVE error.
 */
s32 NetTcp_Open(NetAddr *dest)
{
    return NetTcp_Result(AveTcp_Open(dest->addr, dest->port, dest->openArg));
}

/*
 * Reads AVE TCP options 8200 into opts[0] and 8193 into opts[1] (4
 * bytes each) for sock. Returns the last AVE result; stops at the first
 * negative one.
 */
s32 NetTcp_GetOptions(s32 sock, s32 *opts)
{
    s32 result;

    result = AveTcp_GetOption(sock, 8200, sizeof(s32), opts);
    result = NetTcp_Result(result);
    if (result < 0) {
        return result;
    }
    result = AveTcp_GetOption(sock, 8193, sizeof(s32), opts + 1);
    result = NetTcp_Result(result);
    return result;
}

/*
 * Sets AVE TCP options 8200 from opts[0] and 8193 from opts[1] for
 * sock. Returns the last AVE result; stops at the first negative one.
 */
s32 NetTcp_SetOptions(s32 sock, s32 *opts)
{
    s32 result;

    result = AveTcp_SetOption(sock, 8200, sizeof(s32), opts);
    result = NetTcp_Result(result);
    if (result < 0) {
        return result;
    }
    result = AveTcp_SetOption(sock, 8193, sizeof(s32), opts + 1);
    result = NetTcp_Result(result);
    return result;
}

/*
 * Fills status with sock's state, send space and bytes waiting
 * (AveTcp_Status). AVE's states 0-6 and 8-10 are copied, AVE's -1 becomes
 * NET_TCP_STATE_ERROR (11) and anything else NET_TCP_STATE_OTHER (7); the
 * cases that copy a value stay because the game's code has this jump
 * table. Returns AVE's result; status is left as AVE gave it on an error.
 */
s32 NetTcp_Status(s32 sock, NetTcpStatus *status)
{
    s32 result = NetTcp_Result(AveTcp_Status(sock, (u16 *)&status->state, &status->sendSpace, &status->recvLen));

    if (result < 0) {
        return result;
    }
    switch (status->state) {
    case 0:
        status->state = 0;
        break;
    case 1:
        status->state = 1;
        break;
    case 2:
        status->state = 2;
        break;
    case 3:
        status->state = 3;
        break;
    case 4:
        status->state = 4;
        break;
    case 5:
        status->state = 5;
        break;
    case 6:
        status->state = 6;
        break;
    default:
        status->state = NET_TCP_STATE_OTHER;
        break;
    case 8:
        status->state = 8;
        break;
    case 9:
        status->state = 9;
        break;
    case 10:
        status->state = 10;
        break;
    case AVE_TCP_STATE_NONE:
        status->state = NET_TCP_STATE_ERROR;
        break;
    }
    return result;
}

/*
 * Reads up to len bytes from sock into buf. Returns the byte count read
 * or a negative AVE error.
 */
s32 NetTcp_Recv(s32 sock, void *buf, s16 len)
{
    return NetTcp_Result(AveTcp_Recv(sock, buf, len));
}

/*
 * Sends len bytes of data on sock. Returns a negative AVE error, or the
 * count sent, with AVE's 0 taken as all len bytes sent.
 */
s32 NetTcp_Send(s32 sock, void *data, s16 len)
{
    s32 result = NetTcp_Result(AveTcp_Send(sock, data, len));

    if (result < 0) {
        return result;
    }
    if (result == 0) {
        result = len;
    }
    return result;
}

/* Aborts sock's connection (AveTcp_Abort). Returns AVE's result. */
s32 NetTcp_Abort(s32 sock)
{
    return NetTcp_Result(AveTcp_Abort(sock));
}

/*
 * Deletes the socket in *sock if it holds one (0 or more) and sets *sock
 * to NET_SOCK_FREE either way. Returns AVE's result, or 0 when there was
 * no socket.
 */
s32 NetTcp_Delete(s32 *sock)
{
    s32 result = 0;
    s16 aveResult;

    if (0 <= *sock) {
        aveResult = AveTcp_Delete(*sock);
        *sock = NET_SOCK_FREE;
        result = NetTcp_Result(aveResult);
    } else {
        *sock = NET_SOCK_FREE;
    }
    return result;
}

/*
 * Starts the resolver with the domain name and two DNS servers
 * (AveDns_Init) and marks DNS started. Returns AVE's result, with
 * AVE_ERR_RPC and -1 both made NET_DNS_ERROR.
 */
s32 NetDns_Init(char *domain, s32 primaryDns, s32 secondaryDns)
{
    s32 result = AveDns_Init(domain, primaryDns, secondaryDns);

    switch (result) {
    case AVE_ERR_RPC:
    case -1:
        result = NET_DNS_ERROR;
        break;
    }
    gNetIfStarted[NET_STARTED_DNS] = 1;
    return result;
}

/*
 * Stops the resolver and clears its started flag. Returns AVE's result,
 * with AVE_ERR_RPC and -1 both made NET_DNS_ERROR.
 */
s32 NetDns_Term(void)
{
    s32 result = AveDns_Term();

    switch (result) {
    case AVE_ERR_RPC:
    case -1:
        result = NET_DNS_ERROR;
        break;
    }
    gNetIfStarted[NET_STARTED_DNS] = 0;
    return result;
}

/*
 * Starts a lookup of name. Returns its id, or NET_DNS_ERROR (AVE_ERR_RPC
 * or -1 from AVE).
 */
s32 NetDns_Request(char *name)
{
    s32 result = AveDns_Request(name);

    switch (result) {
    case AVE_ERR_RPC:
    case -1:
        result = NET_DNS_ERROR;
        break;
    }
    return result;
}

/*
 * Frees lookup id. Returns AVE's result, with AVE_ERR_RPC and -1 both
 * made NET_DNS_ERROR.
 */
s32 NetDns_Free(s16 id)
{
    s32 result = AveDns_Free(id);

    switch (result) {
    case AVE_ERR_RPC:
    case -1:
        result = NET_DNS_ERROR;
        break;
    }
    return result;
}

/*
 * Asks for lookup id's answer; on success the address is in *addr.
 * Returns 1 with the address, NET_DNS_PENDING (-3, AVE's -2) while
 * waiting, NET_DNS_FAILED (-2, AVE's -6) or NET_DNS_ERROR (-1, AVE_ERR_RPC
 * or -1); any other AVE value is returned as it is.
 */
s32 NetDns_LookUp(s16 id, s32 *addr)
{
    s32 result = AveDns_LookUp(id, addr);

    switch (result) {
    case AVE_ERR_RPC:
        result = NET_DNS_ERROR;
        break;
    case -1:
        result = NET_DNS_ERROR;
        break;
    case -6:
        result = NET_DNS_FAILED;
        break;
    case AVE_DNS_PENDING:
        result = NET_DNS_PENDING;
        break;
    }
    return result;
}

/*
 * Starts AVE's PPP (AvePpp_Init) and marks PPP started. Returns AVE's
 * result.
 */
s32 NetPpp_Init(void)
{
    s16 result = AvePpp_Init();

    gNetIfStarted[NET_STARTED_PPP] = 1;
    return result;
}

/*
 * Sets AVE PPP option 1 (4 bytes) from *src. The dial-up passes 10.
 * Returns AVE's result.
 */
s16 NetPpp_SetOption1(void *src)
{
    return AvePpp_SetOption(1, sizeof(s32), src);
}

/*
 * Reads the PPP link's details (AvePpp_GetStatus) into out[0-5]: the
 * local address, a second address-sized word, the two DNS servers and
 * two more words. Returns AVE's result; out is untouched on an error.
 */
s32 NetPpp_GetInfo(s32 *out)
{
    AveInfo info;
    AveInfoSub *sub;
    s16 result;

    result = AvePpp_GetStatus(&info);
    if (result < 0) {
        return result;
    }
    out[0] = info.localAddr;
    out[1] = info.unk0;
    sub = &info.sub;
    out[2] = sub->primaryDns;
    out[3] = sub->secondaryDns;
    out[4] = sub->unk8;
    out[5] = sub->unkC;
    return result;
}

/*
 * Gives the modem chat scripts for a device: out[0] = gModemInitChat,
 * out[1] = gModemChat2 (always empty). For a modem (NET_DEV_MODEM) the
 * first holds the script for its driver: one for drivers NET_DRV_CXTMDM
 * and 5, one for NET_DRV_OSTMDM, a third for any other. Ethernet gets
 * two empty scripts; NET_DEV_PPP_OTHER leaves out untouched. unused2 and
 * unused3 are not read. Returns 0.
 */
s32 NetModem_GetChat(s32 devType, s32 driver, s32 unused2, s32 unused3, char **chats)
{
    switch (devType) {
    case NET_DEV_MODEM:
        gModemInitChat[0] = 0;
        switch (driver) {
        case NET_DRV_CXTMDM:
        case NET_DRV_SMAP:
            strcat(gModemInitChat, gModemChatCxt);
            break;
        case NET_DRV_OSTMDM:
            strcat(gModemInitChat, gModemChatOst);
            break;
        default:
            sprintf(gModemInitChat, gModemChatDefault);
            break;
        }
        gModemChat2[0] = 0;
        chats[0] = gModemInitChat;
        chats[1] = gModemChat2;
        break;
    case NET_DEV_PPP_OTHER:
        break;
    case NET_DEV_ETHERNET:
        gModemInitChat[0] = 0;
        gModemChat2[0] = 0;
        chats[0] = gModemInitChat;
        chats[1] = gModemChat2;
        break;
    }
    return 0;
}

/*
 * Returns the PPP link state word from AVE (AvePpp_GetLinkState) when
 * that call returns 1, else -1. The dial-up stores it in the device's
 * status and stops when it is negative.
 */
s32 NetPpp_GetLinkState(void)
{
    s32 linkState;

    if (AvePpp_GetLinkState(&linkState) == 1) {
        return linkState;
    }
    return -1;
}

/*
 * Dials: copies the dial-up settings arg points to (the chat scripts,
 * the account texts and the five modem options, the auth flag and kind,
 * the line mode, PPPoE and the Ethernet link mode) into an AveConfig,
 * turning each setting into AVE's value, and calls AvePpp_Connect. The
 * auth text is an empty string when the settings hold none, and is
 * cleared unless the auth kind is NET_PPP_AUTH_TEXT. Sets gNetIfState to
 * NET_IF_CONNECTING. Returns AvePpp_Connect's result.
 */
s32 NetPpp_Connect(NetDialArg *arg)
{
    AveConfig settings;
    char empty[4];

    empty[0] = 0;
    settings.unk0 = 1;
    settings.isPppoe = arg->isPppoe;
    settings.phyMode = arg->phyMode;
    settings.chat0 = arg->cfg->chat0;
    settings.chat1 = arg->cfg->chat1;
    settings.confText = arg->cfg->confText;
    settings.authName = arg->cfg->authName;
    settings.authKey = arg->cfg->authKey;
    settings.authText = arg->cfg->auth->authText;
    if (settings.authText == NULL) {
        settings.authText = empty;
    }
    switch (arg->cfg->modem->opt[0]) {
    default:
    case 5:
    case 6:
        settings.modemOpt[0] = 6;
        break;
    case 4:
        settings.modemOpt[0] = 5;
        break;
    case 3:
        settings.modemOpt[0] = 4;
        break;
    case 2:
        settings.modemOpt[0] = 3;
        break;
    case 1:
        settings.modemOpt[0] = 2;
        break;
    case 0:
        settings.modemOpt[0] = 0;
        break;
    }
    switch (arg->cfg->modem->opt[1]) {
    default:
        settings.modemOpt[1] = 0;
        break;
    case 1:
        settings.modemOpt[1] = 1;
        break;
    }
    switch (arg->cfg->modem->opt[2]) {
    default:
    case 2:
        settings.modemOpt[2] = 2;
        break;
    case 1:
        settings.modemOpt[2] = 1;
        break;
    case 0:
        settings.modemOpt[2] = 0;
        break;
    }
    switch (arg->cfg->modem->opt[3]) {
    default:
    case 1:
        settings.modemOpt[3] = 3;
        break;
    case 0:
        settings.modemOpt[3] = 2;
        break;
    }
    switch (arg->cfg->modem->opt[4]) {
    default:
    case 1:
        settings.modemOpt[4] = 1;
        break;
    case 2:
        settings.modemOpt[4] = 2;
        break;
    case 0:
        settings.modemOpt[4] = 0;
        break;
    }
    switch (arg->cfg->auth->flag) {
    default:
        settings.authFlag = 0;
        break;
    case 1:
        settings.authFlag = 1;
        break;
    }
    if (arg->cfg->auth->authKind != NET_PPP_AUTH_TEXT) {
        settings.authText[0] = 0;
    }
    switch (*arg->cfg->lineMode) {
    case 0:
        settings.lineMode = 0;
        break;
    case 1:
        settings.lineMode = 1;
        break;
    case 2:
        settings.lineMode = 2;
        break;
    case 3:
        settings.lineMode = 4;
        break;
    }
    {
        s16 result = AvePpp_Connect(&settings);

        gNetIfState = NET_IF_CONNECTING;
        return result;
    }
}

/*
 * Marks the PPP connection up: clears gNetIfError, sets gNetIfState to
 * NET_IF_CONNECTED and the connected flag. Returns 0.
 */
s32 NetPpp_SetConnected(s32 unused0)
{
    gNetIfError = 0;
    gNetIfState = NET_IF_CONNECTED;
    gNetIfStarted[NET_STARTED_CONNECTED] = 1;
    return 0;
}

/*
 * Clears the connected flag and sets gNetIfState to NET_IF_CLOSED.
 * Returns 0.
 */
s32 NetPpp_SetClosed(s32 unused0)
{
    gNetIfStarted[NET_STARTED_CONNECTED] = 0;
    gNetIfState = NET_IF_CLOSED;
    return 0;
}

/*
 * Fills out with the PPP link's details (AvePpp_GetStatus): the local
 * address, the link speed, the phase and the error. AVE's phases 0-7 and
 * 9 are copied, any other becomes NET_PPP_PHASE_OTHER (8); errors 0-10,
 * 12 and 13 are copied, any other becomes NET_PPP_ERR_OTHER (11). The
 * cases that copy a value stay because the game's code has these jump
 * tables. Returns AVE's result; out is untouched on an error.
 */
s32 NetPpp_GetStatus(NetConnInfo *out)
{
    AveInfo info;
    s32 result = AvePpp_GetStatus(&info);

    if (result < 0) {
        return result;
    }
    out->unk4 = info.unk0;
    out->localAddr = info.localAddr;
    switch (info.phase) {
    case 0:
        out->phase = 0;
        break;
    case 1:
        out->phase = 1;
        break;
    case 2:
        out->phase = 2;
        break;
    case 3:
        out->phase = 3;
        break;
    case 4:
        out->phase = 4;
        break;
    case 5:
        out->phase = 5;
        break;
    case 6:
        out->phase = 6;
        break;
    case 7:
        out->phase = 7;
        break;
    default:
        out->phase = NET_PPP_PHASE_OTHER;
        break;
    case 9:
        out->phase = 9;
        break;
    }
    out->speed = info.speed;
    out->unkE = info.unk10;
    switch (info.error) {
    case 0:
        out->error = 0;
        break;
    case 1:
        out->error = 1;
        break;
    case 2:
        out->error = 2;
        break;
    case 3:
        out->error = 3;
        break;
    case 4:
        out->error = 4;
        break;
    case 5:
        out->error = 5;
        break;
    case 6:
        out->error = 6;
        break;
    case 7:
        out->error = 7;
        break;
    case 8:
        out->error = 8;
        break;
    case 9:
        out->error = 9;
        break;
    case 10:
        out->error = 10;
        break;
    default:
        out->error = NET_PPP_ERR_OTHER;
        break;
    case 12:
        out->error = 12;
        break;
    case 13:
        out->error = 13;
        break;
    }
    return result;
}

/*
 * Hangs up (AvePpp_Disconnect) and sets gNetIfState to NET_IF_STOPPING.
 * Returns AVE's result.
 */
s32 NetPpp_Disconnect(s32 unused0)
{
    s16 result = AvePpp_Disconnect();

    gNetIfState = NET_IF_STOPPING;
    return result;
}

/* Stops AVE's PPP and clears its started flag. Returns AVE's result. */
s32 NetPpp_Term(void)
{
    s16 result = AvePpp_Term();

    gNetIfStarted[NET_STARTED_PPP] = 0;
    return result;
}

/*
 * Starts AVE's Ethernet (AveEth_Init) and marks Ethernet started.
 * Returns AVE's result.
 */
s32 NetEth_Init(void)
{
    s16 result = AveEth_Init();

    gNetIfStarted[NET_STARTED_ETH] = 1;
    return result;
}

/*
 * Sets the interface's address and netmask from addrMask[0] and [1]
 * (AveIf_SetAddress). The third value passed, addr | ~(addr & netmask),
 * is always 0xFFFFFFFF; the game's code computes it this way. Returns 0,
 * or -1 when AVE fails.
 */
s32 NetEth_SetAddress(s32 *addrMask)
{
    s32 netmask = addrMask[1];
    s32 addr = addrMask[0];

    return -(AveIf_SetAddress(addr, netmask, addr | ~(addr & netmask)) < 0);
}

/*
 * Starts the Ethernet link in link mode phyMode (gNetPhyMode) and sets
 * gNetIfState to NET_IF_CONNECTING. Returns AVE's result.
 */
s32 NetEth_Start(s32 phyMode)
{
    s16 result = AveEth_Start(phyMode);

    gNetIfState = NET_IF_CONNECTING;
    return result;
}

/* Returns AVE's Ethernet link state: nonzero while the link is up. */
s32 NetEth_IsLinkUp(void)
{
    return AveEth_IsLinkUp();
}

/*
 * Marks the LAN connection up: clears gNetIfError, sets gNetIfState to
 * NET_IF_CONNECTED and the connected flag. Returns 0.
 */
s32 NetEth_SetConnected(s32 unused0)
{
    gNetIfError = 0;
    gNetIfState = NET_IF_CONNECTED;
    gNetIfStarted[NET_STARTED_CONNECTED] = 1;
    return 0;
}

/*
 * Clears the interface's address (AveIf_ClearAddress), clears the
 * connected flag and sets gNetIfState to NET_IF_CLOSED. Returns 0.
 */
s32 NetEth_ClearAddress(s32 unused0)
{
    AveIf_ClearAddress();
    gNetIfStarted[NET_STARTED_CONNECTED] = 0;
    gNetIfState = NET_IF_CLOSED;
    return 0;
}

/*
 * Reads AVE interface option 8 (4 bytes) into buf. Returns 0 when
 * exactly 4 bytes came back, else -1.
 */
s32 NetIf_GetOption8(void *buf)
{
    return -(AveIf_GetOption(8, sizeof(s32), buf) != 4);
}

/*
 * Stops the Ethernet link and sets gNetIfState to NET_IF_STOPPING.
 * Returns AVE's result.
 */
s32 NetEth_Stop(void)
{
    s16 result = AveEth_Stop();

    gNetIfState = NET_IF_STOPPING;
    return result;
}

/*
 * Stops AVE's Ethernet and clears its started flag. Returns AVE's
 * result.
 */
s32 NetEth_Term(void)
{
    s16 result = AveEth_Term();

    gNetIfStarted[NET_STARTED_ETH] = 0;
    return result;
}

/*
 * Adds a route to dest through gateway (the LAN steps pass 0 and
 * gNetGateway) and marks the route set. Returns AVE's result, or -1 on
 * an error (the flag then stays as it was).
 */
s32 NetRoute_Add(s32 dest, s32 gateway)
{
    s16 result = AveRoute_Add(dest, gateway);

    if (result < 0) {
        return -1;
    }
    gNetIfStarted[NET_STARTED_ROUTE] = 1;
    return result;
}

/*
 * Deletes the route to dest through gateway and clears the route flag.
 * Returns AVE's result, or -1 on an error (the flag then stays as it
 * was).
 */
s32 NetRoute_Delete(s32 dest, s32 gateway)
{
    s16 result = AveRoute_Delete(dest, gateway);

    if (result < 0) {
        return -1;
    }
    gNetIfStarted[NET_STARTED_ROUTE] = 0;
    return result;
}

/* Starts AVE's DHCP client and marks DHCP started. Returns AVE's result. */
s32 NetDhcp_Init(void)
{
    s16 result = AveDhcp_Init();

    gNetIfStarted[NET_STARTED_DHCP] = 1;
    return result;
}

/* Stops AVE's DHCP client and clears its started flag. Returns 0. */
s32 NetDhcp_Term(void)
{
    AveDhcp_Term();
    gNetIfStarted[NET_STARTED_DHCP] = 0;
    return 0;
}

/*
 * Starts DHCP (AveDhcp_Start); seen: called with addr 0 and dhcpParam 5.
 * Returns AVE's result.
 */
s32 NetDhcp_Start(s32 addr, s32 dhcpParam)
{
    return AveDhcp_Start(addr, dhcpParam);
}

/*
 * Returns AVE's DHCP state: negative on a failure, 3 once an address is
 * bound (seen: the LAN steps wait for 3).
 */
s32 NetDhcp_GetState(void)
{
    return AveDhcp_GetState();
}

/*
 * Reads the address DHCP got into addrMask[0] and the netmask into
 * addrMask[1]; AVE's third word is dropped. Returns AVE's result.
 */
s32 NetDhcp_GetAddress(s32 *addrMask)
{
    s32 thirdWord;

    return AveDhcp_GetAddress(addrMask, addrMask + 1, &thirdWord);
}

/*
 * Reads the domain name DHCP gave into domain and the two DNS servers
 * into dnsServers[0] and [1]. Returns AVE's result.
 */
s32 NetDhcp_GetDns(char *domain, s32 *dnsServers)
{
    return AveDhcp_GetDns(domain, dnsServers, dnsServers + 1);
}

/*
 * Reads the router (gateway) address DHCP gave into *out. Returns AVE's
 * result.
 */
s32 NetDhcp_GetRouter(s32 *out)
{
    return AveDhcp_GetRouter(out);
}

/*
 * Releases the DHCP address addrMask[0] (AveDhcp_Release); seen:
 * dhcpParam 5. Returns AVE's result.
 */
s32 NetDhcp_Release(s32 *addrMask, s32 dhcpParam)
{
    return AveDhcp_Release(*addrMask, dhcpParam);
}

/*
 * Checks for an interface event. A polled event is kept (gNetPendingEvent)
 * until NetIf_GetEvent takes it, and returned again meanwhile.
 * Returns 1 with the event in *out, 0 when there is none, or AVE's other
 * result.
 */
s32 NetIf_PollEvent(s32 *out)
{
    s32 result;

    if (gNetEventPending != 0) {
        *out = gNetPendingEvent;
        return 1;
    }
    result = AveIf_PollEvent(out);
    if (result != 0) {
        gNetEventPending = 1;
        gNetPendingEvent = *out;
    }
    return result;
}

/*
 * Drops the kept event and reads the event's details from AVE
 * (AveIf_GetEvent): its first word into *event; the other two words
 * are dropped. query is passed to AVE (seen: 0). Returns AVE's result
 * (positive when an event was read).
 */
s32 NetIf_GetEvent(s32 query, s32 *event)
{
    s32 eventArgA;
    s32 eventArgB;

    gNetEventPending = 0;
    gNetPendingEvent = 0;
    return AveIf_GetEvent(query, event, &eventArgA, &eventArgB);
}

/*
 * Acknowledges event (AveIf_AckEvent). Returns AVE's result (0 when
 * accepted).
 */
s32 NetIf_AckEvent(s32 event)
{
    return AveIf_AckEvent(event);
}

/* Takes the socket layer's lock (gNetSockSema). */
void NetSock_Lock(void)
{
    WaitSema(gNetSockSema);
}

/* Releases the socket layer's lock (gNetSockSema). */
void NetSock_Unlock(void)
{
    SignalSema(gNetSockSema);
}
