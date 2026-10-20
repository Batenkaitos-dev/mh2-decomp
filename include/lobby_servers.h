#ifndef LOBBY_SERVERS_H
#define LOBBY_SERVERS_H

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

#endif /* LOBBY_SERVERS_H */
