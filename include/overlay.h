#ifndef OVERLAY_H
#define OVERLAY_H

#include "common.h"

/*
 * The code overlays. Each is a CodeWarrior MWo3 image in DATA.BIN's code
 * archive, which LoadOverlay (src/main/main.c) reads into its window
 * (gOverlayWindow) and relocates. The scene's code (sub_main's
 * Scene_LoadOverlay, the lobby's NetOverlay_Load) loads them by id;
 * Overlay_IsLoaded asks whether one is in its window.
 */

/*
 * An overlay's id: its entry in the archive plus one (Overlay_ReadFile
 * reads entry id - 1), and the id in its header. The overlays that share a
 * window replace each other: select, game and the lobby; nethttp, gm_sub
 * and lbguild; the DNAS overlays and yn.
 */
enum OverlayId {
    OVERLAY_MAIN = 0,       /* the boot executable: always loaded */
    OVERLAY_SUB_MAIN = 1,   /* the engine and the shared game code */
    OVERLAY_SELECT = 2,     /* the title and the start menus (src/select) */
    OVERLAY_GAME = 3,       /* the hunt: the player's actions, the monsters (src/game) */
    OVERLAY_LOBBY = 4,      /* the online lobby (src/lobby) */
    OVERLAY_NETHTTP = 5,
    OVERLAY_GM_SUB = 6,     /* loaded with select and game (Overlay_LoadGmSub) */
    OVERLAY_DNAS_NET = 7,
    OVERLAY_DNAS_INS = 8,
    OVERLAY_YN = 9,
    OVERLAY_LBGUILD = 10,   /* data only, loaded with the lobby */
    OVERLAY_PLSEL = 11,     /* src/plsel: loaded with game for a quest */
    OVERLAY_TEST = 12
};

/*
 * The windows (gOverlayWindow[id - 1]; gOverlayWindowBase gives each one's
 * address). gSysSettings.loaded records the overlay in each window from
 * the scene window on.
 */
#define OVERLAY_WINDOW_SCENE 2      /* select, game, the lobby */
#define OVERLAY_WINDOW_GM_SUB 3     /* nethttp, gm_sub */
#define OVERLAY_WINDOW_LBGUILD 5    /* lbguild, at gm_sub's address */

/* The MWo3 header at the start of an overlay image; it is loaded with the
 * image. */
typedef struct OverlayHeader {
    u32 magic;          /* 0x00: "MWo3" (Overlay_Relocate checks the "MWo") */
    u32 id;             /* 0x04: the OverlayId */
    u32 load;           /* 0x08: the load address */
    u32 textSize;       /* 0x0C */
    u32 dataSize;       /* 0x10 */
    u32 bssSize;        /* 0x14 */
    u32 end;            /* 0x18: load + the file's size */
    u32 end2;           /* 0x1C */
    char name[0x20];
} OverlayHeader;

/* The magic's first three bytes, "MWo", as Overlay_Relocate reads them. */
#define OVERLAY_MAGIC 0x6F574D
#define OVERLAY_MAGIC_MASK 0xFFFFFF

#endif /* OVERLAY_H */
