#include "common.h"
#include "em.h"
#include "lobby_globals.h"
#include "pad.h"
#include "browser.h"
#include "lobby_state.h"
int strncmp(const char *, const char *, unsigned int);

/* prototypes */
void *memcpy(void *dst, const void *src, u32 n);
extern char gCurServerId[];
extern u8 gServerTried;
/* end of prototypes */
extern u8 gNetEntryMode;
extern u8 gLobbyMenuData[];
extern LobbyMenu *gLobbyMenu;
s32 ServerList_TakeCurrent(void);
extern u8 D_5C0AD8[];
extern u8 gServerPick;
extern u8 gServerSkipTried;
/* the number of servers (sub_main 0x5C34F8, after the list; read through the
 * list's symbol, a named object, as the original schedules it) */
extern char gServerIdMin[];
extern char gServerIdMax[];
void *memset(void *dst, s32 c, u32 n);
s32 NetIf_GetError(void);
s32 SrvMenu_Run(void);
s32 strncmp(const char *a, const char *b, u32 n);
extern u8 gNetConnStep;
extern u8 gLbsConnectStep;
extern u8 gNetScreenStep;

void ServerList_MarkTried(void)
{
    u8 n;

    n = gServerTriedCount;
    if ((s32) n < 0x28) {
        memcpy(&gServerTried + (n * 0x14), gCurServerId, 0x14);
        gServerTriedCount = (u8) (gServerTriedCount + 1);
    }
}

/* 1 when id is between lo and hi (12 characters, inclusive) */
s32 ServerList_IdInRange(char *lo, char *hi, char *id)
{
    if (strncmp(id, lo, 12) > 0 || strncmp(id, lo, 12) == 0) {
        if (strncmp(id, hi, 12) < 0 || strncmp(id, hi, 12) == 0) {
            return 1;
        }
    }
    return 0;
}

/* keeps only the servers whose id is between sub_main 0x5CAA50 and 0x5CAA5D */
/* D_5C34F8 is gLobbyServers.count (browser.h): ServerList_FilterRange and ServerList_Sort
 * spell it as its own object (the field form reloads it after
 * each store to the rows: ServerList_Sort loses its unrolled loop). */
void ServerList_FilterRange(void)
{
    LbsRow buf[40];
    LbsRow *f;
    LbsRow *d;
    s32 i;
    s32 n;

    memset(buf, 0, 0x2B20);
    n = 0;
    i = 0;
    if (0 < D_5C34F8) {
        d = buf;
        f = gLobbyServers.row;
        do {
            if (ServerList_IdInRange(gServerIdMin, gServerIdMax, f->key) != 0) {
                memcpy(d, f, 0x114);
                d++;
                n++;
            }
            i++;
            f++;
        } while (i < D_5C34F8);
    }
    if (n != 0) {
        D_5C34F8 = n;
        memcpy(gLobbyServers.row, buf, 0x2B20);
    }
}

/* the lobby server list sorted: 0 by free places (most first), 1 the
 * current server first, 2 only the servers in range (ServerList_FilterRange).
 * The empty if after case 1's memcpy is a compiled-out range check of the
 * index (docs/mwcc-matching.md, "Statements scheduled as if a check were still
 * there"): it keeps found live, so the dead found++ (s2) stays next to the
 * reduced &buf[found] walker as in the game. Without it: 5 words. */
void ServerList_Sort(s32 kind)
{
    LbsRow buf[40];
    s32 j;
    s32 found;
    s32 i;
    s32 n;

    gServerPick = 0;
    memset(buf, 0, 0x2B20);
    if ((u8)kind == 0) {
        if (D_5C34F8 == 0) {
            return;
        }
        for (i = 0; i < D_5C34F8; i++) {
            gLobbyServers.row[i].free = gLobbyServers.row[i].capacity - gLobbyServers.row[i].users;
        }
        ServerList_FilterRange();
        for (i = 0; i < D_5C34F8 - 1; i++) {
            for (j = i + 1; j < D_5C34F8; j++) {
                if (gLobbyServers.row[i].free < gLobbyServers.row[j].free) {
                    memcpy(buf, &gLobbyServers.row[j], 0x114);
                    memcpy(&gLobbyServers.row[j], &gLobbyServers.row[i], 0x114);
                    memcpy(&gLobbyServers.row[i], buf, 0x114);
                }
            }
        }
    } else if ((u8)kind == 1) {
        found = 0;
        for (i = 0; i < D_5C34F8; i++) {
            if (strncmp(gCurServerId, gLobbyServers.row[i].key, 0xC) == 0) {
                memcpy(buf, &gLobbyServers.row[i], 0x114);
                found = 1;
                break;
            }
        }
        for (i = 0; i < D_5C34F8; i++) {
            if (strncmp(gCurServerId, (char *)&gLobbyServers.row[i], 0xC) != 0) {
                memcpy(&buf[found], &gLobbyServers.row[i], 0x114);
                found++;
            }
        }
        memcpy(gLobbyServers.row, buf, 0x114);
        ServerList_FilterRange();
        if (ServerList_IdInRange(gServerIdMin, gServerIdMax, gCurServerId) == 0) {
            memcpy(gCurServerId, gLobbyServers.row, 0xC);
            if (found >= 0 && found < 40) { /* compiled-out check */
            }
        }
    } else if ((u8)kind == 2) {
        ServerList_FilterRange();
    }
}

/* takes server gServerPick as the current one (sub_main 0x5CAA70) when it
 * is up (+0x108) and, if gServerSkipTried, not already tried (sub_main 0x5CAA90,
 * 0x14 bytes each, 0x5CADB0 of them) */
s32 ServerList_TakeCurrent(void)
{
    s32 i;
    u8 *t;

    if (*(s32 *)(D_5C0AD8 + gServerPick * 0x114) != 0) {
        if (gServerSkipTried != 0) {
            i = 0;
            if (0 < gServerTriedCount) {
                t = (u8 *)&gServerTried;
                do {
                    if (strncmp((char *)t, gLobbyServers.row[gServerPick].key, 0xC) == 0) {
                        return 0;
                    }
                    i++;
                    t += 0x14;
                } while (i < gServerTriedCount);
            }
        }
        memcpy(gCurServerId, gLobbyServers.row[gServerPick].key, 0xC);
        return 1;
    }
    return 0;
}

extern u8 gLoginState;
void ServerList_Sort(s32 kind);
char *strcpy(char *dst, const char *src);

/* the network screen step 0: back from the login, the server choice or the
 * portal (gNetEntryMode), or the first entry: the server list screen or the
 * connection (ServerList_TakeCurrent picks a server; none: -1); -5 when the portal
 * asks for a new login */
s32 NetScreen_Step0(void)
{
    s32 r = 0;
    u8 c;

    if (gNetEntryMode == 0) {
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
    } else if (gNetEntryMode == 1 || gNetEntryMode == 2) {
        gLbsConnectStep = 0;
        gNetScreenStep = 0;
        gNetConnStep = 2;
        ServerList_Sort(2);
        memset(gLobbyMenuData, 0, 0x44);
        SrvMenu_Run();
    } else if (gNetEntryMode == 3) {
        c = gLobbyConnDone;
        if (c == 0) {
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
        } else if (c == 1) {
            r = -5;
            gPortalPage = 1;
            gLobbyConnDone = 2;
        } else if (c == 2) {
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
    return r;
}

/* the network screen step 2: chooses the next server of the list (mode
 * gNetEntryMode: 0 automatic, 1 and 2 by the list screen, 3 after a failure);
 * 1 when one was taken, -2 when none is left (sub_main 0x5CADC8 = 0xEF),
 * -1 when the one tried was already tried */
s32 NetScreen_Step1(void)
{
    s32 r = 0;
    u8 v;

    if (gNetEntryMode == 0) {
        gServerPick++;
        if (gServerPick < gLobbyServers.count) {
            if (ServerList_TakeCurrent() == 0) {
                return -1;
            }
            r = 1;
        } else {
            r = -2;
            gLobbyNetError = -0x11;
        }
    } else if (gNetEntryMode == 1 || gNetEntryMode == 2) {
        if (gLobbyServers.count == 1) {
            r = -2;
            gLobbyNetError = -0x11;
        } else {
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gNetConnStep++;
            memset(gLobbyMenuData, 0, 0x44);
            gLobbyMenu->promptStep = 1;
            SrvMenu_Run();
        }
    } else if (gNetEntryMode == 3) {
        v = gLobbyConnDone;
        if (v == 1 || v == 2) {
            gLobbyConnDone = 0;
        } else {
            gServerPick++;
        }
        if (gServerPick < gLobbyServers.count) {
            if (ServerList_TakeCurrent() == 0) {
                return -1;
            }
            r = 1;
        } else {
            r = -2;
            gLobbyNetError = -0x11;
        }
    }
    return r;
}

extern s32 gNetMenuTimeout;                  /* the server list screen's timer (sub_main) */

/* network screen step 2 of the list: gNetMenuTimeout (sub_main, after the tried
 * servers) counts 0x8CA0 frames */
s32 NetScreen_Step2(void)
{
    gLbsConnectStep = 0;
    gNetMenuTimeout = 0x8CA0;
    gNetConnStep++;
    return 0;
}

/* the network screen step 3: the lobby server list screen (SrvMenu_Run); a
 * choice becomes the player id (sub_main 0x5CAA70); 0x5CADB8 counts down */
s32 NetScreen_Step3(void)
{
    s32 r = 0;
    s32 k;

    if (NetIf_GetError() != 0) {
        gLbsConnectStep = 0;
        gNetConnStep = 5;
        gNetScreenStep = 0;
    }
    if (gPadState[0].held != 0) {
        gNetMenuTimeout = 0x8CA0;
    }
    if (--gNetMenuTimeout < 0) {
        gNetScreenStep = 0;
        gNetConnStep = 5;
        gLbsConnectStep = 1;
        return 0;
    }
    k = SrvMenu_Run();
    switch (k) {
    default:
        memcpy(gCurServerId, gLobbyServers.row[k].key, 0xC);
        r = 1;
        break;
    case -1:
        break;
    case -2:
        gNetConnStep++;
        break;
    }
    return r;
}
