#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include "common.h"

/*
 * The network connection: the resolver's set-up (Net_InitDns) and host
 * lookups (Net_ResolveHost), and NetMsg_Update, which runs the connection's
 * state machines one frame at a time (the PPP or Ethernet connection, and
 * the disconnect).
 */

s32 net_inet_addr2(s8 *str);
s32 Net_ResolveHost(s8 *step, s16 *timer, s32 *out, s8 *name);
void Net_InitDns(void);
void Net_ClearState(void);
s32 NetMsg_Update(void);
s32 NetMsg_Dispatch(s8 *handler, s8 *step, s8 *discStep, s16 *timer, u16 *cancelHold, u16 *retries, u16 *retries2, s8 *err);
s32 Net_DisconnectStep(s8 *state, s16 *timer);

#endif
