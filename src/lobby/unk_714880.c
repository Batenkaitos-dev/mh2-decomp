#include "common.h"
#include "em.h"

/* prototypes */
void Lobby_RouteMessage(s32);
/* end of prototypes */

void LobbyReply_650B(void)
{
    Lobby_RouteMessage(0);
}
