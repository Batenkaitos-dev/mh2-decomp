/*
 * The song menu of the hub area (TOWN_MODE_SONG_MENU).
 *
 * A resident of the hub offers a list of music tracks; a few cost an item.
 * The hunter picks one, and the request goes to the host as a town message
 * (TownMsg_SendSongAsk). The host's reply either refuses it (busy:
 * SongMenu_OnBusy) or accepts it (SongMenu_OnAccepted), which takes the
 * item and adds the entry's points to the save's song counter. While a
 * requested track plays, SongMenu_IsPlaying tells the hub's residents.
 *
 * The menu's state is a SongMenu in gMenuWork; its list is copied from
 * gSongMenuItems, the file's one table.
 */
#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "scene.h"
#include "lobby_globals.h"
#include "save.h"
#include "quest.h"
#include "lobby_state.h"
#include "player.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "pad.h"
#include "area.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "lobby_net.h"
#include "text.h"
#include "song_menu.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "libc.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gMenuWork;

/* One entry of the song list (gSongMenuItems and SongMenu.entries). */
typedef struct SongMenuEntry {
    s32 track;                  /* 0x00: the lobby music track (LobbyBgm_EndsToday, TownMsg_SendSongAsk); SONG_LIST_END ends the list */
    u16 item;                   /* 0x04: the item a request costs, 0 for the free entry */
    u8 points;                  /* 0x06: added to gSave.songPoints when the request is accepted */
    u8 pad7;
} SongMenuEntry;

#define SONG_LIST_END -1        /* the track of the list's end marker */
#define SONG_MENU_ENTRIES 5     /* the entries the menu lists */
#define SONG_POINTS_MAX 100     /* the song counter rises no further once it reaches this */

/* SongMenu.state */
enum SongMenuState {
    SONG_MENU_SLIDE_IN = 0,     /* the window slides in */
    SONG_MENU_CHOOSE = 1,       /* the hunter chooses a track */
    SONG_MENU_WAIT_REPLY = 2    /* the request is sent; waiting for the host's reply */
};

/* The song menu's work area (gMenuWork while TOWN_MODE_SONG_MENU runs). */
typedef struct SongMenu {
    WinSlide win;
    SongMenuEntry entries[SONG_MENU_ENTRIES]; /* 0x08: the entries listed */
    s16 count;                  /* 0x30: the entries listed, the end marker apart */
    s16 cursor;                 /* 0x32 */
    s16 replyTimer;             /* 0x34: frames left to wait for the host's reply */
    s8 state;                   /* 0x36: enum SongMenuState */
    s8 infoMode;                /* 0x37: square flips the item description's page */
} SongMenu;

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: declared with s16 coordinates as the song list passes them; with the definition's s32 coordinates and s16 kind the call compiles differently */
void Font_SetPos(s16 x, s16 y);

/* Returns 1 while a requested track plays in the hub area (the lobby's music
 * stream is active there), else 0. The hub's residents check it. */
s32 SongMenu_IsPlaying(void)
{
    if (gScene.area != AREA_TOWN_ARENA) {
        return 0;
    }
    if (LobbyBgm_IsActive() != 0) {
        return 1;
    }
    return 0;
}

extern SongMenuEntry gSongMenuItems[];

/* Opens the song menu: clears gMenuWork, sets TOWN_MODE_SONG_MENU and the
 * draw callback, starts the window's slide and plays the confirm sound. It
 * lists the entries of gSongMenuItems whose item the hunter holds (the free
 * entry always), then copies the end marker. npc, the resident talked to,
 * is unused. */
void SongMenu_Open(u8 *npc)
{
    SongMenu *menu = (SongMenu *)&gMenuWork;
    SongMenuEntry *entry;
    s32 i;

    memset(menu, 0, MENU_WORK_SIZE);
    gTownMode = TOWN_MODE_SONG_MENU;
    gTownMenuOpen = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(SongMenu_Draw, 0, 0, 0);
    WinSlide_Step(&menu->win, 640, 280);
    LobbySe_Play(LOBBY_SE_CONFIRM);
    menu->count = 0;
    entry = gSongMenuItems;
    for (i = 0; i < SONG_MENU_ENTRIES; i++) {
        if (entry->track == SONG_LIST_END) {
            memcpy(&menu->entries[menu->count], entry, sizeof(SongMenuEntry));
            return;
        }
        if (Items_HaveEnough(entry->item, 1) == 1) {
            memcpy(&menu->entries[menu->count], entry, sizeof(SongMenuEntry));
            menu->count++;
        }
        entry++;
    }
}

/* Closes the song menu: back to TOWN_MODE_FREE, clears the draw callbacks
 * and ends the camera's focus on the resident. */
void SongMenu_Close(void)
{
    gTownMode = TOWN_MODE_FREE;
    gTownMenuOpen = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
}

/* Runs the song menu once a frame. After the slide-in, circle requests the
 * track under the cursor when it can be played now (it ends before the day
 * does, no track is playing, and it is afternoon): the request goes to the
 * host and the menu waits for the reply, closing if none comes in time.
 * Otherwise the can't-play notice shows. Cross closes the menu, square
 * flips the selected item's description page, and up and down move the
 * cursor. */
void SongMenu_Update(void)
{
    SongMenu *menu = (SongMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (menu->state) {
    case SONG_MENU_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case 0:
            menu->state++;
            break;
        }
        break;
    case SONG_MENU_CHOOSE:
        if (pad & PAD_CIRCLE) {
            if (LobbyBgm_EndsToday(menu->entries[menu->cursor].track) == 1 && LobbyBgm_GetTrack() == -1 && LobbyTime_IsPm() == 1) {
                menu->state++;
                menu->replyTimer = 210; /* frames: 7 s */
                TownMsg_SendSongAsk(menu->entries[menu->cursor].track);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                Menu_PushNoticeOnce(LOBBY_NOTICE_SONG_UNAVAILABLE);
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            SongMenu_Close();
        } else if (pad & PAD_SQUARE) {
            if (menu->entries[menu->cursor].item != 0) {
                menu->infoMode ^= 1;
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
        }
        break;
    case SONG_MENU_WAIT_REPLY:
        if (--menu->replyTimer <= 0) {
            SongMenu_Close();
        }
        break;
    }
}

/* The host refused this hunter's request: if the menu still waits for the
 * reply, shows the can't-play notice and closes the menu. */
void SongMenu_OnBusy(void)
{
    SongMenu *menu = (SongMenu *)&gMenuWork;

    if (gTownMode == TOWN_MODE_SONG_MENU && menu->state == SONG_MENU_WAIT_REPLY) {
        Menu_PushNoticeOnce(LOBBY_NOTICE_SONG_UNAVAILABLE);
        SongMenu_Close();
    }
}

/* The host accepted this hunter's request: if the menu waits for the reply,
 * takes one of the entry's item (when it costs one) and adds the entry's
 * points to gSave.songPoints while that is under SONG_POINTS_MAX. The first
 * request of track 3 also sets EVENT_FLAG_SONG_TRACK_3 and shows its notice.
 * Then the menu closes. */
void SongMenu_OnAccepted(void)
{
    SongMenu *menu = (SongMenu *)&gMenuWork;
    u8 points;

    if (gTownMode == TOWN_MODE_SONG_MENU && menu->state == SONG_MENU_WAIT_REPLY) {
        if (menu->entries[menu->cursor].item != 0) {
            Items_Take(menu->entries[menu->cursor].item, 1);
            points = gSave.songPoints;
            if (points < SONG_POINTS_MAX) {
                gSave.songPoints = points + menu->entries[menu->cursor].points;
            }
            if (menu->entries[menu->cursor].track == 3 && Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 0) {
                Save_SetEventFlag(EVENT_FLAG_SONG_TRACK_3);
                Menu_PushNotice(LOBBY_NOTICE_SONG_TRACK3_FIRST);
            }
        }
        SongMenu_Close();
    }
}

/* Draws the song list at (x, y): the label bar and the window, one row per
 * entry (the item's name and the count held, or the free entry's grey name)
 * with the cursor's bar, a separator, then the selected item's description
 * (its page by infoMode) or a help line, and the help box under the window. */
void SongMenu_DrawList(s16 x, s16 y)
{
    SongMenu *menu = (SongMenu *)&gMenuWork;
    s16 rowY = y;
    s16 i;
    u16 item;
    u16 selItem;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 140, 0xFFB0B000, gSongMenuTexts[SONG_TEXT_TITLE], 2, 1);
    Win_Draw(x, y, 340, 290, 0xBDB59F6C, 0x80202020, 2);
    rowY += 30;
    Font_SetSpacing(20, 20);
    for (i = 0; i < SONG_MENU_ENTRIES; i++) {
        if (menu->entries[i].track == SONG_LIST_END) {
            break;
        }
        item = menu->entries[i].item;
        Font_SetPos(x + 50, rowY);
        if (i == menu->cursor) {
            selItem = item;
            Menu_DrawRowHighlight(x + 10, rowY - 1, 320, 24, 0);
        }
        if (item == 0) {
            Font_SetColor(0x807F7F7F);
            Text_PrintfColored("%-16s%s%s", gSongMenuTexts[SONG_TEXT_SOUL_SONG], gLobbyWords[LOBBY_WORD_COLON], gSongMenuTexts[SONG_TEXT_DASHES]);
        } else {
            Menu_DrawItemCountRow(x + 50, rowY, item, 0x807F7F7F);
        }
        rowY += 22;
    }
    Gfx_DrawLine(x + 20, y + 186, 300, 2, 0xBDB59F6C);
    if (selItem != 0) {
        ItemInfo_Draw((s16)(x + 50) - 40, y + 190, 340, menu->infoMode, selItem, 0, -1, 0x807F7F7F);
    } else {
        Font_SetSpacing(18, 18);
        Font_SetPos(x + 90, 256);
        Text_Printf(gSongHelpTexts[0]);
    }
    Win_DrawText(x, y + 298, 340, 80, 0xBDB59F6C, 0x80202020, gSongHelpTexts[1]);
}

/* The song menu's draw callback: sets the task's text size and draws the
 * list at the window's sliding x while the menu slides in or the hunter
 * chooses. */
void SongMenu_Draw(MenuTask *task)
{
    SongMenu *menu = (SongMenu *)&gMenuWork;
    s16 x;

    Text_SetSize(task->textSize);
    x = menu->win.pos;
    switch (menu->state) {
    case SONG_MENU_SLIDE_IN:
    case SONG_MENU_CHOOSE:
        SongMenu_DrawList(x, 58);
        break;
    }
}

/* The song list: track, item cost and points of each entry, then the end
 * marker. */
SongMenuEntry gSongMenuItems[6] = {
#include "lobby/007E93E0.inc"
};
