#ifndef NET_SETTINGS_H
#define NET_SETTINGS_H

#include "common.h"

/*
 * The network settings' reset (NetCfg_Reset), and NetTcp_SetOption, which
 * sets the two AVE TCP options the lobby's socket uses.
 */

void NetTcp_SetOption(s32 sock);
void NetCfg_Reset(void);

#endif
