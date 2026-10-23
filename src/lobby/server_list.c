/*
 * The lobby server list and the steps of the network screen.
 *
 * The lobby servers (gLobbyServers, LbsRow: an id, a name, the capacity
 * and the users) are sorted and filtered here (ServerList_*): by free
 * places, with the current server first, or only those whose id lies in
 * the range gServerIdMin..gServerIdMax. ServerList_TakeCurrent makes a
 * server the current one (gCurServerId), skipping those already tried
 * (gServerTried) when gServerSkipTried asks it to; ServerList_ClearTried
 * forgets them.
 *
 * NetScreen_Run, first, runs the network screen's current step through the
 * function table at the end of the file.
 * NetScreen_Step0 to NetScreen_Step3 are the network screen's first steps:
 * by gNetEntryMode (LOGIN_ENTRY_*: the first entry, the second
 * connection or a later one, the return after a quest), they pick the server to
 * connect to, automatically or through the server list screen
 * (SrvMenu_Run), which closes after SERVER_MENU_TIMEOUT frames without
 * input. NetScreen_Step4 and NetScreen_Step5 are its last steps
 * (gNetScreenStep counts within one), after the lobby was left: the
 * account's portal page in the browser, then the message that says why
 * the lobby was left.
 */
#include "common.h"
#include "em.h"
#include "lobby_globals.h"
#include "pad.h"
#include "browser.h"
#include "lobby_servers.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "net_mode.h"
#include "font.h"
#include "game_util.h"
#include "lobby_connect.h"
#include "lobby_dialog_box.h"
#include "lobby_frame.h"
#include "lobby_util.h"
#include "screen_fade.h"
#include "libkernl.h"
int strncmp(const char *, const char *, unsigned int);

#define SERVERS_TRIED_MAX 40            /* gServerTried's entries */
#define SERVER_TRIED_SIZE 20            /* each one's bytes */
#define SERVER_MENU_TIMEOUT 36000       /* frames: 10 minutes */
#define NET_ERR_NO_SERVER_LEFT -17      /* gLobbyNetError: every server was tried */

/* prototypes */
void *memcpy(void *dst, const void *src, u32 n);
extern char gCurServerId[];
extern char gServerTried[SERVERS_TRIED_MAX][SERVER_TRIED_SIZE];
/* end of prototypes */
extern u8 gNetEntryMode;
extern LobbyMenu gLobbyMenuData;        /* the menu state gLobbyMenu points to */
extern LobbyMenu *gLobbyMenu;
extern u8 gServerPick;
extern u8 gServerSkipTried;
extern char gServerIdMin[];
extern char gServerIdMax[];
void *memset(void *dst, s32 c, u32 n);
s32 strncmp(const char *a, const char *b, u32 n);
extern u8 gNetConnStep;
extern u8 gLbsConnectStep;
extern u8 gNetScreenStep;
extern s8 gNetDisconnectStep;
extern s16 gNetStepTimeout;                  /* frames left before the step fails */

/* The network screen's steps: a function table (defined at the end of this
 * file). */
extern s32 (*jtbl_008193E0[])(void);

/* The network screen's current step (gNetConnStep): NetScreen_Step0 to
 * NetScreen_Step5. */
s32 NetScreen_Run(void)
{
    return jtbl_008193E0[gNetConnStep]();
}

/* Forgets the servers tried: the next pick may take any server. */
void ServerList_ClearTried(void)
{
    memset(gServerTried, 0, sizeof(gServerTried));
    gServerTriedCount = 0;
    gServerSkipTried = 0;
}

/* Adds the current server (gCurServerId) to the servers tried, while
 * gServerTried has room. */
void ServerList_MarkTried(void)
{
    u8 count;

    count = gServerTriedCount;
    if ((s32) count < SERVERS_TRIED_MAX) {
        memcpy(gServerTried[count], gCurServerId, SERVER_TRIED_SIZE);
        gServerTriedCount = (u8) (gServerTriedCount + 1);
    }
}

/* Returns 1 when server id id lies between low and high, inclusive (their
 * first SERVER_ID_LEN characters compared), else 0. */
s32 ServerList_IdInRange(char *low, char *high, char *id)
{
    if (strncmp(id, low, SERVER_ID_LEN) > 0 || strncmp(id, low, SERVER_ID_LEN) == 0) {
        if (strncmp(id, high, SERVER_ID_LEN) < 0 || strncmp(id, high, SERVER_ID_LEN) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Keeps only the servers whose id lies between gServerIdMin and
 * gServerIdMax, unless that would leave none.
 * gLobbyServerCount is gLobbyServers.count read as an object of its own,
 * here and in ServerList_Sort: read as the field, the compiler reloads it
 * after each store to the rows, and ServerList_Sort's loop is no longer
 * unrolled as the game's is. */
void ServerList_FilterRange(void)
{
    LbsRow kept[40];
    LbsRow *src;
    LbsRow *dst;
    s32 i;
    s32 keptCount;

    memset(kept, 0, sizeof(kept));
    keptCount = 0;
    i = 0;
    if (0 < gLobbyServerCount) {
        dst = kept;
        src = gLobbyServers.row;
        do {
            if (ServerList_IdInRange(gServerIdMin, gServerIdMax, src->key) != 0) {
                memcpy(dst, src, sizeof(LbsRow));
                dst++;
                keptCount++;
            }
            i++;
            src++;
        } while (i < gLobbyServerCount);
    }
    if (keptCount != 0) {
        gLobbyServerCount = keptCount;
        memcpy(gLobbyServers.row, kept, sizeof(kept));
    }
}

/* Sorts the lobby server list by kind: 0 keeps the servers in range, by
 * free places, most first; 1 puts the current server first and keeps those
 * in range, and if the current server is out of range the first one left
 * becomes the current one; 2 only the servers in range.
 * The empty if at the end of kind 1 is a range check of found whose body
 * the compiler drops: it keeps found alive there, which keeps the loop's
 * count and pointer in the registers the game's code uses. */
void ServerList_Sort(s32 kind)
{
    LbsRow sorted[40];
    s32 j;
    s32 found;
    s32 i;
    s32 n;

    gServerPick = 0;
    memset(sorted, 0, sizeof(sorted));
    if ((u8)kind == 0) {
        if (gLobbyServerCount == 0) {
            return;
        }
        for (i = 0; i < gLobbyServerCount; i++) {
            gLobbyServers.row[i].free = gLobbyServers.row[i].capacity - gLobbyServers.row[i].users;
        }
        ServerList_FilterRange();
        for (i = 0; i < gLobbyServerCount - 1; i++) {
            for (j = i + 1; j < gLobbyServerCount; j++) {
                if (gLobbyServers.row[i].free < gLobbyServers.row[j].free) {
                    memcpy(sorted, &gLobbyServers.row[j], sizeof(LbsRow));
                    memcpy(&gLobbyServers.row[j], &gLobbyServers.row[i], sizeof(LbsRow));
                    memcpy(&gLobbyServers.row[i], sorted, sizeof(LbsRow));
                }
            }
        }
    } else if ((u8)kind == 1) {
        found = 0;
        for (i = 0; i < gLobbyServerCount; i++) {
            if (strncmp(gCurServerId, gLobbyServers.row[i].key, SERVER_ID_LEN) == 0) {
                memcpy(sorted, &gLobbyServers.row[i], sizeof(LbsRow));
                found = 1;
                break;
            }
        }
        for (i = 0; i < gLobbyServerCount; i++) {
            if (strncmp(gCurServerId, (char *)&gLobbyServers.row[i], SERVER_ID_LEN) != 0) {
                memcpy(&sorted[found], &gLobbyServers.row[i], sizeof(LbsRow));
                found++;
            }
        }
        memcpy(gLobbyServers.row, sorted, sizeof(LbsRow));
        ServerList_FilterRange();
        if (ServerList_IdInRange(gServerIdMin, gServerIdMax, gCurServerId) == 0) {
            memcpy(gCurServerId, gLobbyServers.row, SERVER_ID_LEN);
            if (found >= 0 && found < 40) { /* compiled-out check */
            }
        }
    } else if ((u8)kind == 2) {
        ServerList_FilterRange();
    }
}

/* Makes server gServerPick the current one (gCurServerId) when it has
 * places (a capacity) and, when gServerSkipTried is set, has not been
 * tried yet (gServerTried). Returns 1 when it was taken, else 0. */
s32 ServerList_TakeCurrent(void)
{
    s32 i;
    u8 *tried;

    if (gLobbyServers.row[gServerPick].capacity != 0) {
        if (gServerSkipTried != 0) {
            i = 0;
            if (0 < gServerTriedCount) {
                tried = (u8 *)gServerTried;
                do {
                    if (strncmp((char *)tried, gLobbyServers.row[gServerPick].key, SERVER_ID_LEN) == 0) {
                        return 0;
                    }
                    i++;
                    tried += SERVER_TRIED_SIZE;
                } while (i < gServerTriedCount);
            }
        }
        memcpy(gCurServerId, gLobbyServers.row[gServerPick].key, SERVER_ID_LEN);
        return 1;
    }
    return 0;
}

extern u8 gLoginState;
char *strcpy(char *dst, const char *src);

/* The network screen's first step, by gNetEntryMode: on the first entry
 * resets the server range and the steps and takes the server with the
 * most free places; on a later connection opens the server list screen
 * again; after a quest takes the next server by gLobbyConnDone.
 * Returns 0, -1 when no server could be taken, or -5 when the portal asks
 * for a new login. */
s32 NetScreen_Step0(void)
{
    s32 result = 0;
    u8 connDone;

    if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
        D_5CAA40 = 0;
        strcpy(gServerIdMin, "000000000000");
        strcpy(gServerIdMax, "ffffffffffff");
        gNetConnStep = 0;
        gLoginState = 2;
        gLbsConnectStep = 0;
        gNetScreenStep = 0;
        ServerList_Sort(0);
        if (ServerList_TakeCurrent() == 0) {
            return -1;
        }
    } else if (gNetEntryMode == LOGIN_ENTRY_SERVER || gNetEntryMode == LOGIN_ENTRY_AGAIN) {
        gLbsConnectStep = 0;
        gNetScreenStep = 0;
        gNetConnStep = 2;
        ServerList_Sort(2);
        memset(&gLobbyMenuData, 0, 68);
        SrvMenu_Run();
    } else if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
        connDone = gLobbyConnDone;
        if (connDone == 0) {
            gLoginState = 2;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            if (gServerSkipTried == 0) {
                ServerList_Sort(1);
            } else {
                ServerList_Sort(0);
                if (ServerList_TakeCurrent() == 0) {
                    return -1;
                }
            }
        } else if (connDone == 1) {
            result = -5;
            gPortalPage = PORTAL_PAGE_BACK;
            gLobbyConnDone = 2;
        } else if (connDone == 2) {
            gLoginState = 2;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            if (gServerSkipTried == 0) {
                ServerList_Sort(0);
            } else {
                ServerList_Sort(0);
                if (ServerList_TakeCurrent() == 0) {
                    return -1;
                }
            }
        }
    }
    return result;
}

/* The network screen's step after a failed connection: picks the next
 * server, automatically (LOGIN_ENTRY_FIRST and LOGIN_ENTRY_QUEST_RETURN) or
 * through the server list screen (LOGIN_ENTRY_SERVER, LOGIN_ENTRY_AGAIN). Returns 1 when one was taken, -1 when the one picked was
 * tried already, -2 when none is left (gLobbyNetError is then
 * NET_ERR_NO_SERVER_LEFT), else 0. */
s32 NetScreen_Step1(void)
{
    s32 result = 0;
    u8 connDone;

    if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
        gServerPick++;
        if (gServerPick < gLobbyServers.count) {
            if (ServerList_TakeCurrent() == 0) {
                return -1;
            }
            result = 1;
        } else {
            result = -2;
            gLobbyNetError = NET_ERR_NO_SERVER_LEFT;
        }
    } else if (gNetEntryMode == LOGIN_ENTRY_SERVER || gNetEntryMode == LOGIN_ENTRY_AGAIN) {
        if (gLobbyServers.count == 1) {
            result = -2;
            gLobbyNetError = NET_ERR_NO_SERVER_LEFT;
        } else {
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gNetConnStep++;
            memset(&gLobbyMenuData, 0, 68);
            gLobbyMenu->promptStep = 1;
            SrvMenu_Run();
        }
    } else if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
        connDone = gLobbyConnDone;
        if (connDone == 1 || connDone == 2) {
            gLobbyConnDone = 0;
        } else {
            gServerPick++;
        }
        if (gServerPick < gLobbyServers.count) {
            if (ServerList_TakeCurrent() == 0) {
                return -1;
            }
            result = 1;
        } else {
            result = -2;
            gLobbyNetError = NET_ERR_NO_SERVER_LEFT;
        }
    }
    return result;
}

extern s32 gNetMenuTimeout;                  /* the server list screen's frames left */

/* The server list screen's opening: starts its timeout and goes on to its
 * next step. Returns 0. */
s32 NetScreen_Step2(void)
{
    gLbsConnectStep = 0;
    gNetMenuTimeout = SERVER_MENU_TIMEOUT;
    gNetConnStep++;
    return 0;
}

/* The server list screen (SrvMenu_Run): a server chosen becomes the
 * current one (returns 1); a network error or SERVER_MENU_TIMEOUT frames
 * without input leave the screen. Returns 0 while it runs. */
s32 NetScreen_Step3(void)
{
    s32 result = 0;
    s32 choice;

    if (NetIf_GetError() != 0) {
        gLbsConnectStep = 0;
        gNetConnStep = 5;
        gNetScreenStep = 0;
    }
    if (gPadState[0].held != 0) {
        gNetMenuTimeout = SERVER_MENU_TIMEOUT;
    }
    if (--gNetMenuTimeout < 0) {
        gNetScreenStep = 0;
        gNetConnStep = 5;
        gLbsConnectStep = 1;
        return 0;
    }
    choice = SrvMenu_Run();
    switch (choice) {
    default:
        memcpy(gCurServerId, gLobbyServers.row[choice].key, SERVER_ID_LEN);
        result = 1;
        break;
    case SRV_MENU_NONE:
        break;
    case SRV_MENU_QUIT:
        gNetConnStep++;
        break;
    }
    return result;
}

/* The network screen's step 4: resets the game's subsystems; returns -3. */
s32 NetScreen_Step4(void)
{
    Game_ResetSubsystems();
    return -3;
}

extern char gBrowserUrl[256];            /* the browser's URL */
extern MmbbData gAccountData;

/*
 * The network screen's step 5, after the lobby was left: fades out, waits for the
 * disconnection, shows the account's portal page (PORTAL_PAGE_NEXT) in the browser when
 * the lobby server's connection had begun, then, after 8 frames, the message that
 * says why the line was cut (no input for too long, or an unstable line), until
 * PAD_CIRCLE. Returns -4 then (the step's end), else 0.
 */
s32 NetScreen_Step5(void)
{
    u16 pad = Pad_GetPressedRepeat(0);
    s32 result = 0;

    switch (gNetScreenStep) {
    case 0:
        gNetScreenStep++;
        LobbyStream_Stop();
        FadeA_Start(1);
        break;
    case 1:
        if (LobbyFade_IsRunning() == 0) {
            if (gLbsConnectStep != 0) {
                gNetScreenStep++;
            } else {
                gNetScreenStep = 7;
            }
            LobbyMenu_SetDraw(0, Lobby_DrawBlackLine, LobbyDialog_DrawPending, 0);
            NetMode_FadeReset();
            gNetDisconnectStep = 0;
        }
        break;
    case 2:
        gNetScreenStep++;
    case 3:
        gNetScreenStep++;
        FlushCache(0);
        memset(gBrowserUrl, 0, 256);
        gPortalPage = PORTAL_PAGE_NEXT;
        gBrowserTopMode = BROWSER_TOP_LOBBY;
        strcpy(gBrowserUrl, gAccountData.nextUrl);
        Browser_Init(0);
        break;
    case 4:
        switch (Browser_Update()) {
        case 0:
            break;
        case -100:
            LobbyKddi_CloseBrowser();
            NetScene_BackToPortal();
            break;
        default:
            LobbyKddi_CloseBrowser();
            gNetScreenStep++;
            break;
        }
        break;
    case 5:
        gNetScreenStep++;
        break;
    case 6:
        gNetScreenStep++;
    case 7:
        if (LobbyNet_DisconnectWait() != 0) {
            gNetScreenStep++;
        }
        break;
    case 8:
        gNetScreenStep = 9;
        gNetStepTimeout = 8;
        NetMode_FadeStart(0, 10, 0);
        if (gLbsConnectStep == 0) {
            LobbyText_Open(LOBBY_TEXT_LINE_UNSTABLE);
        } else {
            LobbyText_Open(LOBBY_TEXT_IDLE_CUT);
        }
        gLobbyMenuData.redraw = 1;
        break;
    case 9:
        gLobbyMenuData.redraw = 1;
        gNetStepTimeout--;
        if (gNetStepTimeout < 0) {
            gNetScreenStep++;
        }
        break;
    case 10:
        gLobbyMenuData.redraw = 1;
        if (pad & PAD_CIRCLE) {
            result = -4;
            gLobbyNetError = 0;
            gPortalPage = PORTAL_PAGE_TOP;
        }
        Text_SetColorIndex(5);
        Font_SetSpacing(20, 20);
        break;
    }
    LobbyMenu_QueueLayers();
    return result;
}

/* The network screen's steps (this file's .data). The table's name is the
 * one the build's tools look for in a table of jumps; it stays. */
s32 (*jtbl_008193E0[8])(void) = {
    NetScreen_Step0, NetScreen_Step1, NetScreen_Step2, NetScreen_Step3,
    NetScreen_Step4, NetScreen_Step5, NULL, NULL,
};
