/*
 * The game menu's chat page (an entry of its online lists): its list of
 * four entries (CHAT_ENTRY_*: the chat log, the phrase menu, the hunters
 * and where the chat goes), GameMenu_UpdateChat and GameMenu_DrawChat, and
 * the hunter page (ChatPage_*): the hunters in the chat log, the actions on
 * one (CHAT_ACT_*: the hunter's card, mute, unmute) and the muted list.
 * The muted list is gChatBlockList, CHAT_BLOCK_LIST_LEN records of a
 * hunter's id and name (BlockedUser): MuteList_Add and MuteList_Remove
 * change it, and a chat message from a hunter on it is dropped as it
 * arrives (LobbyRoute_ChatMessage). The pages keep
 * their state in the game menu's work (gSubMenuWork, GameMenu). The chat
 * state page, the last entry, follows in its own unit.
 * The .data holds the page's list window and the hunter page's windows;
 * the .rodata the hunter page's two line formats.
 */
#include "common.h"
#include "game_menu.h"
#include "em.h"
#include "lobby_globals.h"
#include "scene.h"
#include "window.h"
#include "area.h"
#include "chat_log.h"
#include "hud.h"
#include "lobby_menu.h"
#include "pad.h"
#include "sound.h"
#include "lobby_labels.h"

s32 IdList_FindFree(s8 *, s32);
u8 ChatPage_CountMuted(void);
char *strcpy(void *dst, const void *src);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
extern s8 gChatBlockList;
s32 IdList_Find(s8 *list, s32 n, void *id);
s32 ChatLog_ListSpeakers(struct ChatSpeaker *out);
void ChatPage_MarkMuted(void);

/* Whether the hunter page can open: only while the chat goes to the lobby's
 * chat (CHAT_SEND_LOBBY), whose log lists the hunters. */
s32 ChatMenu_CanListSpeakers(void)
{
    return gChatSendTo == CHAT_SEND_LOBBY;
}

/* Whether the chat state page can open: always. */
s32 ChatMenu_CanSetState(void)
{
    return 1;
}

/* The chat page's open handler: its own list at its first entry, with the chat help
 * page. Returns 0 (it always opens). */
s32 GameMenu_OpenChat(void)
{
    gSubMenuWork->subStep = 0;
    gSubMenuWork->subEntry = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_CHAT;
    gHelpLine = 0;
    return 0;
}

void GameMenu_Back(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
/* unit's own declaration: declared without parameters, so GameMenu_UpdateChat's call passes no menu, as the game's code does (PhraseMenu_Open ignores its menu argument) */
s32 PhraseMenu_Open();
s32 ChatPage_OpenSpeakers(void);
s32 ChatPage_OpenState(void);
s32 ChatLog_Close(void);
s32 Chat_OpenWindow(void);
u16 ChatLog_Scroll(u16 pad);
u16 Chat_LogScroll(u16 buttons);
u16 PhraseMenu_Update(u16 buttons);
u16 ChatPage_UpdateSpeakers(u16 pad);
/* unit's own declaration: the buttons are declared u16 (the definition takes an s32): with s32 the compiler passes pad to it unlike the game's code at GameMenu_UpdateChat's call */
s32 ChatPage_UpdateState(u16 pad);

/* The chat page's input. subStep 0, its list (subEntry, CHAT_ENTRY_*):
 * PAD_CIRCLE opens the log, the phrase menu, the hunter page or the chat
 * state page unless its open handler refuses; PAD_START closes the game
 * menu; the help line follows the cursor (AREA_HUB_0x83 has its own line
 * for the state entry: only one state there). subStep 1: the open entry
 * takes the buttons, and PAD_CROSS left over goes back to the list.
 * Returns the buttons for GameMenu_Update: all of them from the list
 * (PAD_CROSS goes back to the game menu's list), only PAD_START (the menu
 * closes) from an entry. */
u16 GameMenu_UpdateChat(u16 pad)
{
    s32 refused;
    u8 *cur;
    GameMenu *menu;

    switch (gSubMenuWork->subStep) {
    case 0:
        if (pad & PAD_START) {
            GameMenu_Back();
            return 0;
        }
        Menu_CursorUpDown(&gSubMenuWork->subEntry, pad, CHAT_ENTRIES);
        menu = gSubMenuWork;
        cur = &menu->subEntry;
        gHelpLine = *cur;
        if (*cur == CHAT_ENTRY_STATE && gScene.area == AREA_HUB_0x83) {
            gHelpLine = 4; /* the state entry's line where there is only one state */
        }
        if (pad & PAD_CIRCLE) {
            refused = 0;
            switch (*cur) {
            case CHAT_ENTRY_PHRASES:
                refused = PhraseMenu_Open();
                break;
            case CHAT_ENTRY_HUNTERS:
                refused = ChatPage_OpenSpeakers();
                break;
            case CHAT_ENTRY_STATE:
                refused = ChatPage_OpenState();
                break;
            case CHAT_ENTRY_LOG:
                if (gChatSendTo == 0) {
                    refused = ChatLog_Close();
                } else {
                    refused = Chat_OpenWindow();
                }
                break;
            }
            if (refused == 0) {
                gSubMenuWork->subStep++;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
            } else {
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
            }
        }
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case CHAT_ENTRY_LOG:
            if (gChatSendTo == CHAT_SEND_LOBBY) {
                pad = ChatLog_Scroll(pad);
            } else {
                pad = Chat_LogScroll(pad);
            }
            break;
        case CHAT_ENTRY_PHRASES:
            pad = PhraseMenu_Update(pad);
            break;
        case CHAT_ENTRY_HUNTERS:
            pad = ChatPage_UpdateSpeakers(pad);
            break;
        case CHAT_ENTRY_STATE:
            pad = ChatPage_UpdateState(pad);
            break;
        }
        if (pad & PAD_START) {
            return PAD_START;
        }
        if (pad & PAD_CROSS) {
            gSubMenuWork->subStep = 0;
            gHelpShow = 1;
            gHelpPage = HELP_PAGE_CHAT;
            gHelpLine = gSubMenuWork->subEntry;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        }
        pad = 0;
        break;
    }
    return pad;
}

void GameMenu_DrawList(s8 dim);
s32 ChatMenu_CanListSpeakers(void);
s32 ChatMenu_CanSetState(void);
void ChatPage_DrawSpeakers(void);
void ChatPage_DrawState(void);
void ChatLog_Draw(void);
void PhraseMenu_Draw(void);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Chat_DrawLog(void);
extern ListPanel gChatPanel;
extern char *gChatMenuTexts[];

/* The chat page's draw: in its list, the game menu's list dimmed with the
 * four entries over it (the hunter and state entries take their other
 * texts when ChatMenu_CanListSpeakers or ChatMenu_CanSetState says no); in
 * an entry, that entry's own draw. */
void GameMenu_DrawChat(void)
{
    char *lines[4];

    switch (gSubMenuWork->subStep) {
    case 0:
        GameMenu_DrawList(1);
        lines[CHAT_ENTRY_LOG] = gChatMenuTexts[CHAT_ENTRY_LOG];
        lines[CHAT_ENTRY_PHRASES] = gChatMenuTexts[CHAT_ENTRY_PHRASES];
        lines[CHAT_ENTRY_HUNTERS] = gChatMenuTexts[ChatMenu_CanListSpeakers() ? CHAT_ENTRY_HUNTERS : 4];
        lines[CHAT_ENTRY_STATE] = gChatMenuTexts[ChatMenu_CanSetState() ? CHAT_ENTRY_STATE : 5];
        gChatPanel.items = lines;
        ListPanel_Draw(&gChatPanel, 0, 0, gSubMenuWork->subEntry);
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case CHAT_ENTRY_PHRASES:
            PhraseMenu_Draw();
            break;
        case CHAT_ENTRY_HUNTERS:
            ChatPage_DrawSpeakers();
            break;
        case CHAT_ENTRY_STATE:
            ChatPage_DrawState();
            break;
        case CHAT_ENTRY_LOG:
            if (gChatSendTo == CHAT_SEND_LOBBY) {
                ChatLog_Draw();
            } else {
                Chat_DrawLog();
            }
            break;
        }
        break;
    }
}

/* Opens the muted list over the hunter list: counts its records (with its
 * cursor on the first, ChatPage_CountMuted) and shows its help line. */
void ChatPage_OpenMuted(void)
{
    ChatPage_CountMuted();
    gSubMenuWork->page++;
    gSubMenuWork->pageStep = 0;
    gHelpShow = 1;
    gHelpLine = 0x26; /* the muted list's line */
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
}

/* Closes the muted list: back to the hunter list, its cursor on the first
 * hunter, the help line hidden. */
void ChatPage_CloseMuted(void)
{
    gSubMenuWork->page = CHAT_PAGE_HUNTERS;
    gSubMenuWork->pageStep = 0;
    gHelpShow = 0;
    gSubMenuWork->speakerGrid.index = gSubMenuWork->speakerGrid.row = gSubMenuWork->speakerGrid.col = 0;
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
}

void Shake_Start(u8 id, u8 flags, f32 amp);
void ChatPage_LoadSpeakers(void);

/* The chat page's open handler for the hunter page: returns 1 (refused)
 * unless the chat goes to the lobby's chat (ChatMenu_CanListSpeakers); else
 * lists the hunters in the chat log (ChatPage_LoadSpeakers), puts the
 * cursor on the first (CHAT_LIST_ROWS to a column), slides the window in
 * and returns 0. */
s32 ChatPage_OpenSpeakers(void)
{
    if (ChatMenu_CanListSpeakers() != 1) {
        return 1;
    }
    gSubMenuWork->pageStep = 0;
    gSubMenuWork->page = CHAT_PAGE_HUNTERS;
    gHelpPage = HELP_PAGE_LOBBY;
    gHelpShow = 0;
    ChatPage_LoadSpeakers();
    gSubMenuWork->speakerGrid.index = 0;
    gSubMenuWork->speakerGrid.rows = CHAT_LIST_ROWS;
    gSubMenuWork->speakerGrid.row = 0;
    gSubMenuWork->speakerGrid.col = 0;
    gSubMenuWork->speakerGrid.cols = (gSubMenuWork->speakerCount + (CHAT_LIST_ROWS - 1)) / CHAT_LIST_ROWS;
    gSubMenuWork->speakerGrid.lastRows = gSubMenuWork->speakerCount - (gSubMenuWork->speakerGrid.cols - 1) * CHAT_LIST_ROWS;
    Shake_Start(SHAKE_CHAT_HUNTERS, SHAKE_AXIS_X, 240.0f);
    return 0;
}

s32 Shake_Update(u8 id);
u8 Shake_Flags(u8 id);
void Menu_GridCursor(struct Grid *g, u16 buttons);
void HunterInfo_Open(void *src, s32 flags);
s32 MuteList_Add(ChatSpeaker *e);
s32 MuteList_Remove(ChatSpeaker *e);
s32 HunterInfo_Update(u16 pad);
struct BlockedUser;
s32 IdList_Remove(struct BlockedUser *r, u8 idx, u8 n);

/* The hunter page's input, by page (GameMenu.page):
 * - CHAT_PAGE_HUNTERS, the hunter list: PAD_CIRCLE opens the chosen
 *   hunter's actions (pageStep 1, cursor GameMenu.action): info opens the
 *   hunter's card (pageStep 2, HunterInfo_Update, until it ends), mute adds
 *   the hunter to the muted list (MuteList_Add), unmute takes it off
 *   (MuteList_Remove); PAD_CROSS closes the actions.
 * - CHAT_PAGE_MUTED, the muted list: PAD_CIRCLE asks to remove the chosen
 *   record (pageStep 1), PAD_CIRCLE again removes it from gChatBlockList;
 *   PAD_CROSS steps back.
 * PAD_SQUARE switches between the lists. PAD_START slides the window out
 * and, once it is out, returns PAD_START (the game menu closes); while the
 * window slides in, the D-pad and PAD_CROSS are ignored. Returns the
 * buttons left for GameMenu_UpdateChat (PAD_CROSS from the hunter list
 * goes back to the chat page's list). */
u16 ChatPage_UpdateSpeakers(u16 pad)
{
    u8 *step;
    GameMenu *menu;
    s32 buttons;
    u16 sel;

    if (Shake_Flags(SHAKE_CHAT_HUNTERS) & SHAKE_SLIDE) { /* sliding out: the menu closes once it is out */
        if (Shake_Update(SHAKE_CHAT_HUNTERS) == 0) {
            return PAD_START;
        }
        return 0;
    }
    if (pad & PAD_START) {
        Shake_Start(SHAKE_CHAT_HUNTERS, SHAKE_SLIDE, 240.0f);
        gSubMenuWork->pageStep = 0;
        gHelpShow = 0;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        return 0;
    }
    if (Shake_Update(SHAKE_CHAT_HUNTERS) == 1) {
        pad &= 0xFFFF & ~(PAD_DPAD | PAD_CROSS); /* no D-pad or cancel while the window slides in */
    }
    menu = gSubMenuWork;
    switch (menu->page) {
    case CHAT_PAGE_HUNTERS:
        gHelpShow = 0;
        step = &menu->pageStep;
        switch (*step) {
        case 0:
            buttons = pad;
            if (buttons & PAD_SQUARE) {
                ChatPage_OpenMuted();
            } else if (menu->speakerGrid.cols != 0) {
                Menu_GridCursor(&menu->speakerGrid, pad);
                if (buttons & PAD_CIRCLE) {
                    gSubMenuWork->pageStep++;
                    gSubMenuWork->action = 0;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                }
            }
            break;
        case 1:
            if (pad & PAD_CROSS) {
                *step = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                pad = 0;
            }
            buttons = pad;
            if (buttons & PAD_SQUARE) {
                ChatPage_OpenMuted();
                break;
            }
            Menu_CursorUpDown(&gSubMenuWork->action, pad, CHAT_ACTS);
            if (buttons & PAD_CIRCLE) {
                menu = gSubMenuWork;
                switch (menu->action) {
                case CHAT_ACT_INFO:
                    HunterInfo_Open(&menu->speakers[menu->speakerGrid.index], HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND);
                    gSubMenuWork->pageStep++;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    break;
                case CHAT_ACT_MUTE:
                    sel = menu->speakerGrid.index;
                    if (menu->speakerMuted[sel] == 0 && MuteList_Add(&menu->speakers[sel]) == 1) {
                        menu = gSubMenuWork;
                        menu->speakerMuted[menu->speakerGrid.index] = 1;
                        ChatPage_CountMuted();
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                    }
                    break;
                case CHAT_ACT_UNMUTE:
                    if (MuteList_Remove(&menu->speakers[menu->speakerGrid.index]) == 1) {
                        menu = gSubMenuWork;
                        menu->speakerMuted[menu->speakerGrid.index] = 0;
                        ChatPage_CountMuted();
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                    }
                    break;
                }
            }
            break;
        case 2:
            switch (menu->action) {
            case CHAT_ACT_INFO:
                switch (HunterInfo_Update(pad)) {
                case MENU_CANCEL: /* back to the actions */
                    gFieldControl = 1;
                    pad = 0;
                    gSubMenuWork->pageStep--;
                    break;
                case MENU_UNDONE: /* the card ended: back to the list */
                    gFieldControl = 1;
                    pad = 0;
                    gSubMenuWork->pageStep = 0;
                    break;
                case MENU_CLOSE:
                    gFieldControl = 1;
                    pad = PAD_START;
                    break;
                case MENU_RUNNING:
                    pad = 0;
                    break;
                }
            }
            break;
        }
        break;
    case CHAT_PAGE_MUTED:
        gHelpShow = 1;
        step = &menu->pageStep;
        switch (*step) {
        case 0:
            buttons = pad;
            if (buttons & (PAD_SQUARE | PAD_CROSS)) {
                ChatPage_CloseMuted();
                pad = 0;
            } else if (menu->mutedGrid.cols != 0) {
                Menu_GridCursor(&menu->mutedGrid, pad);
                if (buttons & PAD_CIRCLE) {
                    menu = gSubMenuWork;
                    menu->pageStep++;
                    gHelpLine = 0x27; /* the removal question's line */
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                }
            }
            break;
        case 1:
            if (pad & PAD_CROSS) {
                *step = 0;
                gHelpLine = 0x26;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
                pad = 0;
            }
            buttons = pad;
            if (buttons & PAD_SQUARE) {
                ChatPage_CloseMuted();
            } else if (buttons & PAD_CIRCLE) {
                if (IdList_Remove((struct BlockedUser *) &gChatBlockList, gSubMenuWork->mutedGrid.index, CHAT_BLOCK_LIST_LEN) == 1) {
                    ChatPage_CountMuted();
                    ChatPage_MarkMuted();
                    gSubMenuWork->pageStep = 0;
                    gHelpLine = 0x26;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                } else {
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                }
            }
            break;
        }
        break;
    }
    return pad;
}

/* Marks each listed hunter muted (speakerMuted 1) when its id is on
 * gChatBlockList, else 0. */
void ChatPage_MarkMuted(void)
{
    s32 i;

    for (i = 0; i < gSubMenuWork->speakerCount; i++) {
        if (IdList_Find(&gChatBlockList, CHAT_BLOCK_LIST_LEN, (u8 *)&gSubMenuWork->speakers[i]) != -1) {
            gSubMenuWork->speakerMuted[i] = 1;
        } else {
            gSubMenuWork->speakerMuted[i] = 0;
        }
    }
}

/* Fills the hunter list with the hunters in the chat log
 * (ChatLog_ListSpeakers), ends it with an empty id and marks the muted
 * ones. */
void ChatPage_LoadSpeakers(void)
{
    gSubMenuWork->speakerCount = ChatLog_ListSpeakers(gSubMenuWork->speakers);
    gSubMenuWork->speakers[gSubMenuWork->speakerCount].id[0] = 0;
    ChatPage_MarkMuted();
}

/* Adds a hunter to the chat's muted list (gChatBlockList): its id and name
 * into the first free record. Returns 1, or 0 when the list is full. */
s32 MuteList_Add(ChatSpeaker *speaker)
{
    s32 free;
    BlockedUser *rec;

    free = IdList_FindFree(&gChatBlockList, CHAT_BLOCK_LIST_LEN);
    if (free != -1) {
        rec = &((BlockedUser *)&gChatBlockList)[free];
        strcpy(rec->userId, speaker->id);
        strcpy(rec->name, speaker->name);
        return 1;
    }
    return 0;
}

void Draw_Icons(Icon *icons, u8 count);
void Window_DrawText(void *p, s16 *pos, u8 *text);
extern Panel gMutedTitleWindow;
extern s16 gMutedTitleIcons[];
s32 IdList_Count(s8 *p, u8 n);

/* Takes a hunter off the chat's muted list (gChatBlockList), found by id;
 * the later records move down. Returns 1, or 0 when it isn't on the
 * list. */
s32 MuteList_Remove(ChatSpeaker *speaker)
{
    s32 idx;

    idx = IdList_Find((s8 *) &gChatBlockList, CHAT_BLOCK_LIST_LEN, speaker);
    if (idx != -1) {
        return IdList_Remove((struct BlockedUser *) &gChatBlockList, idx & 0xFF, CHAT_BLOCK_LIST_LEN);
    }
    return 0;
}

/* Counts the muted list's records into mutedCount and puts its cursor
 * (mutedGrid) on the first, CHAT_LIST_ROWS to a column. Returns the
 * count. */
u8 ChatPage_CountMuted(void)
{
    gSubMenuWork->mutedCount = IdList_Count((s8 *) &gChatBlockList, CHAT_BLOCK_LIST_LEN);
    gSubMenuWork->mutedGrid.index = 0;
    gSubMenuWork->mutedGrid.rows = CHAT_LIST_ROWS;
    gSubMenuWork->mutedGrid.row = 0;
    gSubMenuWork->mutedGrid.col = 0;
    gSubMenuWork->mutedGrid.cols = (gSubMenuWork->mutedCount + (CHAT_LIST_ROWS - 1)) / CHAT_LIST_ROWS;
    gSubMenuWork->mutedGrid.lastRows = gSubMenuWork->mutedCount - (gSubMenuWork->mutedGrid.cols - 1) * CHAT_LIST_ROWS;
    return gSubMenuWork->mutedCount;
}

struct ShakeOffset;
s32 Shake_CopyOffset(u8 id, struct ShakeOffset *out);
void Menu_DrawFrame(void *b, s16 *pos);
void Gfx_DrawMarker(s16 x, s16 y, s16 w, s16 h, s32 flags);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(const char *fmt, ...);
s32 sprintf(char *, const char *, ...);
void Sjis_Widen(char *src, char *dst);
void Text_Draw(char *s);
void ChatPage_DrawMutedTitle(s16 *pos);
void HunterInfo_DrawIfOpen(void);
s32 Shake_Value(u8);
extern ListPanel gSpeakerPanel;
extern ListPanel gSpeakerActionPanel;
extern ListPanel gMutedRemovePanel;
extern char *gChatUserActTexts[][3];

/* The hunter page's draw, by page (GameMenu.page):
 * - CHAT_PAGE_HUNTERS: a column of CHAT_LIST_ROWS of the hunters in the
 *   chat log (name, then id; a marker on the muted ones), the column's
 *   number and count in the title, or a line saying no one is listed; the
 *   action list over it while a hunter is chosen (pageStep 1; its texts
 *   change when the hunter is muted or the muted list is full), the list's
 *   cursor grey under it; the hunter's card while it is open (pageStep 2).
 * - CHAT_PAGE_MUTED: the same for gChatBlockList's records, the removal
 *   box while one is chosen, and the count of muted hunters out of
 *   CHAT_BLOCK_LIST_LEN, in the warning colour when the list is full.
 * The window moves with its slide (Shake_CopyOffset). In both loops i
 * counts the rows down and entry is the record's index. menuBytes reads
 * the game menu (gSubMenuWork) through a byte pointer, its fields through
 * casts and three of them as bytes: with a GameMenu pointer the compiler
 * loads each list's column count twice where the game's code loads it
 * once, and with the three fields written as fields it orders their
 * additions unlike the game's code. */
void ChatPage_DrawSpeakers(void)
{
    char title[0x20];
    char countWide[0x10];
    char countText[0xC];
    s16 pos[2];
    s32 i;
    u32 color;
    s32 actionSet;
    s32 sel;
    s16 y;
    s32 entry;
    u8 *menuBytes;

    Shake_CopyOffset(SHAKE_CHAT_HUNTERS, (struct ShakeOffset *) pos);
    menuBytes = (u8 *)gSubMenuWork;
    switch (((GameMenu *)menuBytes)->page) {
    case CHAT_PAGE_HUNTERS:
        color = 0xFF0A9182;
        switch (((GameMenu *)menuBytes)->pageStep) {
        case 1:
            if (menuBytes[((GameMenu *)menuBytes)->speakerGrid.index + 0x12] == 0) { /* speakerMuted[] of the chosen hunter */
                entry = ((GameMenu *)menuBytes)->mutedCount < CHAT_BLOCK_LIST_LEN; /* room on the muted list */
                actionSet = 0;
                if (!entry) {
                    actionSet = 1;
                }
            } else {
                actionSet = 2;
            }
            gSpeakerActionPanel.items = gChatUserActTexts[actionSet];
            ListPanel_Draw(&gSpeakerActionPanel, pos, NULL, ((GameMenu *)menuBytes)->action);
            color = 0xFF808080; /* the list's cursor grey under the actions */
        case 0:
            menuBytes = (u8 *)gSubMenuWork;
            sel = ((GameMenu *)menuBytes)->speakerGrid.cols != 0 ? ((GameMenu *)menuBytes)->speakerGrid.row + 1 : -1;
            if (((GameMenu *)menuBytes)->speakerGrid.cols <= 0) {
                sprintf(title, gChatPageTexts[CHAT_PAGE_TEXT_SPEAKERS_TITLE]);
            } else {
                sprintf(title, gChatPageTexts[CHAT_PAGE_TEXT_SPEAKERS_TITLE_FMT], ((GameMenu *)menuBytes)->speakerGrid.col + 1, ((GameMenu *)menuBytes)->speakerGrid.cols);
            }
            gSpeakerPanel.cursorColor = color;
            ListPanel_Draw(&gSpeakerPanel, pos, title, sel);
            if (gSubMenuWork->speakerGrid.cols != 0) {
                if (gSubMenuWork->speakerGrid.cols > 1) {
                    Menu_DrawFrame(&gSpeakerPanel, pos);
                }
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x1C1, 0x52);
                Text_PrintfColored(gChatPageTexts[CHAT_PAGE_TEXT_SPEAKERS_HEAD]);
                Font_SetColor(0x807F7F7F);
                y = 0x66;
                i = CHAT_LIST_ROWS;
                entry = gSubMenuWork->speakerGrid.col * CHAT_LIST_ROWS;
                do {
                    menuBytes = (u8 *)gSubMenuWork;
                    if (((GameMenu *)menuBytes)->speakers[entry].id[0] == 0) {
                        break;
                    }
                    if ((menuBytes + entry)[0x12] != 0) { /* speakerMuted[entry] */
                        Gfx_DrawMarker(pos[0] + 0x1A3, y, 0x19, 0x14, MARKER_ALERT);
                    }
                    Font_SetPos(pos[0] + 0x1C1, y);
                    menuBytes = (u8 *)gSubMenuWork + entry * 0x19;
                    Text_PrintfColored("%-8.8s   %-6.6s", menuBytes + 0x5B, menuBytes + 0x53); /* speakers[entry]'s name and id */
                    y += 0x14;
                    i--;
                    entry++;
                } while (i > 0);
            } else {
                Font_SetColor(0x802B7F7F);
                Font_SetPos(pos[0] + 0x1B8, 0x78);
                Text_PrintfColored(gChatPageTexts[CHAT_PAGE_TEXT_NO_SPEAKERS]);
            }
            ChatPage_DrawMutedTitle(pos);
            break;
        case 2:
            switch (((GameMenu *)menuBytes)->action) {
            case CHAT_ACT_INFO:
                HunterInfo_DrawIfOpen();
            }
            break;
        }
        break;
    case CHAT_PAGE_MUTED:
        switch (((GameMenu *)menuBytes)->pageStep) {
        case 1:
            ListPanel_Draw(&gMutedRemovePanel, NULL, NULL, 0);
        case 0:
            menuBytes = (u8 *)gSubMenuWork;
            sel = ((GameMenu *)menuBytes)->mutedGrid.cols != 0 ? ((GameMenu *)menuBytes)->mutedGrid.row + 1 : -1;
            if (((GameMenu *)menuBytes)->mutedGrid.cols <= 0) {
                sprintf(title, gChatPageTexts[CHAT_PAGE_TEXT_MUTED_TITLE]);
            } else {
                sprintf(title, gChatPageTexts[CHAT_PAGE_TEXT_MUTED_TITLE_FMT], ((GameMenu *)menuBytes)->mutedGrid.col + 1, ((GameMenu *)menuBytes)->mutedGrid.cols);
            }
            gSpeakerPanel.cursorColor = 0xFF0A9182;
            ListPanel_Draw(&gSpeakerPanel, pos, title, sel);
            if (gSubMenuWork->mutedGrid.cols != 0) {
                if (gSubMenuWork->mutedGrid.cols > 1) {
                    Menu_DrawFrame(&gSpeakerPanel, pos);
                }
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x1C1, 0x52);
                Text_PrintfColored(gChatPageTexts[CHAT_PAGE_TEXT_SPEAKERS_HEAD]);
                Font_SetColor(0x807F7F7F);
                y = 0x66;
                i = CHAT_LIST_ROWS;
                entry = gSubMenuWork->mutedGrid.col * CHAT_LIST_ROWS;
                do {
                    if ((s8)((BlockedUser *)&gChatBlockList)[entry].userId[0] == 0) { /* the id's first byte read signed, as the game's code does */
                        break;
                    }
                    Gfx_DrawMarker(pos[0] + 0x1A3, y, 0x19, 0x14, MARKER_ALERT);
                    Font_SetPos(pos[0] + 0x1C1, y);
                    Text_PrintfColored("%-8.8s   %-6.6s", ((BlockedUser *)&gChatBlockList)[entry].name, ((BlockedUser *)&gChatBlockList)[entry].userId);
                    y += 0x14;
                    i--;
                    entry++;
                } while (i > 0);
            } else {
                Font_SetColor(0x802B7F7F);
                Font_SetPos(pos[0] + 0x1A6, 0x78);
                Text_PrintfColored(gChatPageTexts[CHAT_PAGE_TEXT_NO_MUTED]);
            }
            if (Shake_Value(SHAKE_CHAT_HUNTERS) == 0) {
                Font_SetColor(0x807F7F28);
                Font_SetPos(pos[0] + 0x184, 0x190);
                Text_PrintfColored(gChatPageTexts[CHAT_PAGE_TEXT_MUTED_LABEL]);
                if (gSubMenuWork->mutedCount >= CHAT_BLOCK_LIST_LEN) {
                    Font_SetColor(0x807F212E);
                } else {
                    Font_SetColor(0x807F7F7F);
                }
                Font_SetPos(pos[0] + 0x1F0, 0x190);
                sprintf(countText, "%2d/20", gSubMenuWork->mutedCount);
                Sjis_Widen(countText, countWide);
                Text_Draw(countWide);
            }
            break;
        }
        break;
    }
}

/* Draws the muted list's label beside the hunter list (gMutedTitleWindow;
 * PAD_SQUARE opens the list) with its icon, both moved by the window's
 * slide offset at pos. */
void ChatPage_DrawMutedTitle(s16 *pos)
{
    Window_DrawText(&gMutedTitleWindow, (s16 *)pos, (u8 *)gChatPageTexts[CHAT_PAGE_TEXT_MUTED_PAGE_TITLE]);
    gMutedTitleIcons[0] = *pos + 0x1C1;
    Draw_Icons((Icon *) gMutedTitleIcons, 1);
}

/* The chat page's list window (GameMenu_DrawChat sets its entries). */
ListPanel gChatPanel = {
#include "lobby/007F1F30.inc"
};
/* The hunter page's windows: the list, the action list, the muted list's
 * removal box, and the muted list's label with its icon. */
ListPanel gSpeakerPanel = {
#include "lobby/007F1F50.inc"
};
ListPanel gSpeakerActionPanel = {
#include "lobby/007F1F70.inc"
};
ListPanel gMutedRemovePanel = {
#include "lobby/007F1F90.inc"
};
Panel gMutedTitleWindow = {
#include "lobby/007F1FB0.inc"
};
s16 gMutedTitleIcons[4] = {
#include "lobby/007F1FC8.inc"
};
