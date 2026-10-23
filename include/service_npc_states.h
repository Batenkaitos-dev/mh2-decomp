#ifndef SERVICE_NPC_STATES_H
#define SERVICE_NPC_STATES_H

#include "common.h"
#include "em.h"

/*
 * The service townspeople's state handlers: what the house cook, the
 * hairdresser, the bait seller and townsperson 73 do in each actor state.
 * ServiceNpc_RunState runs their state; the cook's kitchen route
 * (Npc_SetKitchenRoute, Npc_AdvanceRoute) and the placing of a townsperson
 * on a route (Npc_PlaceOnRoute) are here too.
 */

void ServiceNpc_StateTalk(Em *npc);
void Npc_SetKitchenRoute(Em *npc, u8 spot);
void Npc_AdvanceRoute(Em *npc);
void Npc_PlaceOnRoute(Em *npc, u8 set, u8 routeNo);
s32 ServiceNpc_RunState(Em *npc);

#endif
