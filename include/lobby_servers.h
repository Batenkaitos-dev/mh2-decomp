#ifndef LOBBY_SERVERS_H
#define LOBBY_SERVERS_H
#include "menu_task.h"

/*
 * The lobby's servers: the list the network screen reads from the
 * server browser (gLobbyServers, browser.h's LbsList), and the server
 * choice menu that lets the player pick one of them (SrvMenu_Run).
 */

#define SERVER_ID_LEN 12        /* a server id's characters (LbsRow.key) */
#define SERVER_LIST_MAX 40      /* the servers the list holds (LbsList.row) */
#define SRV_MENU_PAGE_LEN 8     /* the servers the menu lists on a page */

/* SrvMenu_Run's result besides a server's index. */
#define SRV_MENU_NONE -1        /* the menu is still running */
#define SRV_MENU_QUIT -2        /* the player quit the menu */

/* The server choice menu's pages (LobbyMenu.screen; gSrvMenuPages). */
enum SrvMenuPage {
    SRV_PAGE_START = 0,         /* SrvMenu_Start: opens the menu */
    SRV_PAGE_SLIDE_IN = 1,      /* SrvMenu_SlideIn: the window slides in */
    SRV_PAGE_CHOOSE = 2,        /* SrvMenu_Choose: the list and its cursor */
    SRV_PAGE_INTRO = 3,         /* SrvMenu_Intro: a message before the list */
    SRV_PAGE_QUIT = 4           /* SrvMenu_Quit: asks whether to quit */
};

s32 SrvMenu_Run(void);
void SrvMenu_Start(void);
void SrvMenu_SlideIn(void);
void SrvMenu_Choose(void);
void SrvMenu_Intro(void);
void SrvMenu_Quit(void);
void SrvMenu_DrawList(s16 x, s16 y);
void SrvMenu_DrawInfo(MenuTask *task);
void SrvMenu_DrawWindow(MenuTask *task);
char *LobbySrv_MyName(void);
char *LobbySrv_Name(char *id);
s32 LobbySrv_CurIndex(void);
s32 LobbySrv_IsCur(char *id);
s32 LobbySrv_Index(char *id);
s32 LobbySrv_ColorAt(s32 i);
s32 LobbySrv_Color(char *id);
void LobbyDialog_DrawPending(MenuTask *task);
s32 LobbySrv_HasCurName(void);

s32 NetScreen_Run(void);
void ServerList_ClearTried(void);
void ServerList_MarkTried(void);
void ServerList_Sort(s32 kind);
s32 ServerList_TakeCurrent(void);
s32 NetScreen_Step0(void);
s32 NetScreen_Step1(void);
s32 NetScreen_Step2(void);
s32 NetScreen_Step3(void);

#endif /* LOBBY_SERVERS_H */
