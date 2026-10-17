#include "common.h"
#include "game_menu.h"
#include "em.h"
#include "lobby_globals.h"
#include "scene.h"
#include "window.h"
#include "area.h"

/* prototypes */
s32 IdList_FindFree(s8 *, s32);
u8 ChatPage_CountMuted(void);
char *strcpy(void *dst, const void *src);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
extern s8 gChatBlockList;
s32 IdList_Find(s8 *list, s32 n, void *id);
s32 ChatLog_ListSpeakers(struct ChatSpeaker *out);
void ChatPage_MarkMuted(void);
/* end of prototypes */

void GameMenu_Back(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
s32 PhraseMenu_Open(); /* unit's own declaration: PhraseMenu_Open ignores a0 (m), which holds the chat cursor m[0xA] (1) at GameMenu_UpdateChat's call */
s32 ChatPage_OpenSpeakers(void);
s32 ChatPage_OpenState(void);
s32 ChatLog_Close(void);
s32 Chat_OpenWindow(void);
u16 ChatLog_Scroll(u16 pad);
u16 Chat_LogScroll(u16 buttons);
u16 PhraseMenu_Update(u16 buttons);
u16 ChatPage_UpdateSpeakers(u16 pad);
s32 ChatPage_UpdateState(u16 pad); /* unit's own declaration: with the definition's s32 (s32 btn) (lobby/unk_772F30.c), GameMenu_UpdateChat 1 of 169 words differ */

u16 GameMenu_UpdateChat(u16 pad)
{
    s32 r;
    u8 *cur;
    GameMenu *m;

    switch (gSubMenuWork->subStep) {
    case 0:
        if (pad & 0x8000) {
            GameMenu_Back();
            return 0;
        }
        Menu_CursorUpDown(&gSubMenuWork->subEntry, pad, 4);
        m = gSubMenuWork;
        cur = &m->subEntry;
        gHelpLine = *cur;
        if (*cur == 3 && gScene.area == AREA_HUB_0x83) {
            gHelpLine = 4;
        }
        if (pad & 0x20) {
            r = 0;
            switch (*cur) {
            case 1:
                r = PhraseMenu_Open();
                break;
            case 2:
                r = ChatPage_OpenSpeakers();
                break;
            case 3:
                r = ChatPage_OpenState();
                break;
            case 0:
                if (gChatSendTo == 0) {
                    r = ChatLog_Close();
                } else {
                    r = Chat_OpenWindow();
                }
                break;
            }
            if (r == 0) {
                gSubMenuWork->subStep++;
                Sound_PlaySeFlat(0, 0x13, 0);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
        }
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case 0:
            if (gChatSendTo == 0) {
                pad = ChatLog_Scroll(pad);
            } else {
                pad = Chat_LogScroll(pad);
            }
            break;
        case 1:
            pad = PhraseMenu_Update(pad);
            break;
        case 2:
            pad = ChatPage_UpdateSpeakers(pad);
            break;
        case 3:
            pad = ChatPage_UpdateState(pad);
            break;
        }
        if (pad & 0x8000) {
            return 0x8000;
        }
        if (pad & 0x40) {
            gSubMenuWork->subStep = 0;
            gHelpShow = 1;
            gHelpPage = 0xB;
            gHelpLine = gSubMenuWork->subEntry;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        pad = 0;
        break;
    }
    return pad;
}

void GameMenu_DrawList(s8 dim);
s32 ChatMenu_CanListSpeakers(void);
s32 ChatMenu_CanSetState(void);
void Chat_DrawSpeakers(void);
void Chat_DrawState(void);
void ChatLog_Draw(void);
void PhraseMenu_Draw(void);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Chat_DrawLog(void);
extern ListPanel gChatPanel;
extern char *gChatMenuTexts[];

void GameMenu_DrawChat(void)
{
    char *lines[4];

    switch (gSubMenuWork->subStep) {
    case 0:
        GameMenu_DrawList(1);
        lines[0] = gChatMenuTexts[0];
        lines[1] = gChatMenuTexts[1];
        lines[2] = gChatMenuTexts[ChatMenu_CanListSpeakers() ? 2 : 4];
        lines[3] = gChatMenuTexts[ChatMenu_CanSetState() ? 3 : 5];
        gChatPanel.items = lines;
        ListPanel_Draw(&gChatPanel, 0, 0, gSubMenuWork->subEntry);
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case 1:
            PhraseMenu_Draw();
            break;
        case 2:
            Chat_DrawSpeakers();
            break;
        case 3:
            Chat_DrawState();
            break;
        case 0:
            if (gChatSendTo == 0) {
                ChatLog_Draw();
            } else {
                Chat_DrawLog();
            }
            break;
        }
        break;
    }
}

void ChatPage_OpenMuted(void)
{
    ChatPage_CountMuted();
    gSubMenuWork->page++;
    gSubMenuWork->pageStep = 0;
    gHelpShow = 1;
    gHelpLine = 0x26;
    Sound_PlaySeFlat(0, 0x13, 0);
}

void ChatPage_CloseMuted(void)
{
    gSubMenuWork->page = 0;
    gSubMenuWork->pageStep = 0;
    gHelpShow = 0;
    gSubMenuWork->speakerGrid.index = gSubMenuWork->speakerGrid.row = gSubMenuWork->speakerGrid.col = 0;
    Sound_PlaySeFlat(0, 0x13, 0);
}

void Shake_Start(u8 id, u8 flags, f32 amp);
s32 ChatMenu_CanListSpeakers(void);
void ChatPage_LoadSpeakers(void);

s32 ChatPage_OpenSpeakers(void)
{
    if (ChatMenu_CanListSpeakers() != 1) {
        return 1;
    }
    gSubMenuWork->pageStep = 0;
    gSubMenuWork->page = 0;
    gHelpPage = 4;
    gHelpShow = 0;
    ChatPage_LoadSpeakers();
    gSubMenuWork->speakerGrid.index = 0;
    gSubMenuWork->speakerGrid.rows = 10;
    gSubMenuWork->speakerGrid.row = 0;
    gSubMenuWork->speakerGrid.col = 0;
    gSubMenuWork->speakerGrid.cols = (gSubMenuWork->speakerCount + 9) / 10;
    gSubMenuWork->speakerGrid.lastRows = gSubMenuWork->speakerCount - (gSubMenuWork->speakerGrid.cols - 1) * 10;
    Shake_Start(0xF, 0, 240.0f);
    return 0;
}

s32 Shake_Update(u8 id);
u8 Shake_Flags(u8 id);
void Menu_GridCursor(struct Grid *g, u16 buttons);
void HunterInfo_Open(void *src, s32 flags);
s32 MuteList_Add(ChatSpeaker *e);
s32 MuteList_Remove(ChatSpeaker *e);
s32 HunterInfo_Update(u16 pad);
struct Rec18;
s32 IdList_Remove(struct Rec18 *r, u8 idx, u8 n);

u16 ChatPage_UpdateSpeakers(u16 pad)
{
    u8 *p;
    GameMenu *m;
    s32 b;
    u16 k;

    if (Shake_Flags(0xF) & 0x80) {
        if (Shake_Update(0xF) == 0) {
            return 0x8000;
        }
        return 0;
    }
    if (pad & 0x8000) {
        Shake_Start(0xF, 0x80, 240.0f);
        gSubMenuWork->pageStep = 0;
        gHelpShow = 0;
        Sound_PlaySeFlat(0, 0x14, 0);
        return 0;
    }
    if (Shake_Update(0xF) == 1) {
        pad &= 0xC3BF;
    }
    m = gSubMenuWork;
    switch (m->page) {
    case 0:
        gHelpShow = 0;
        p = &m->pageStep;
        switch (*p) {
        case 0:
            b = pad;
            if (b & 0x200) {
                ChatPage_OpenMuted();
            } else if (m->speakerGrid.cols != 0) {
                Menu_GridCursor(&m->speakerGrid, pad);
                if (b & 0x20) {
                    gSubMenuWork->pageStep++;
                    gSubMenuWork->action = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                }
            }
            break;
        case 1:
            if (pad & 0x40) {
                *p = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
                pad = 0;
            }
            b = pad;
            if (b & 0x200) {
                ChatPage_OpenMuted();
                break;
            }
            Menu_CursorUpDown(&gSubMenuWork->action, pad, 3);
            if (b & 0x20) {
                m = gSubMenuWork;
                switch (m->action) {
                case 0:
                    HunterInfo_Open(&m->speakers[m->speakerGrid.index], 3);
                    gSubMenuWork->pageStep++;
                    Sound_PlaySeFlat(0, 0x13, 0);
                    break;
                case 1:
                    k = m->speakerGrid.index;
                    if (m->speakerMuted[k] == 0 && MuteList_Add(&m->speakers[k]) == 1) {
                        m = gSubMenuWork;
                        m->speakerMuted[m->speakerGrid.index] = 1;
                        ChatPage_CountMuted();
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                    break;
                case 2:
                    if (MuteList_Remove(&m->speakers[m->speakerGrid.index]) == 1) {
                        m = gSubMenuWork;
                        m->speakerMuted[m->speakerGrid.index] = 0;
                        ChatPage_CountMuted();
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                    break;
                }
            }
            break;
        case 2:
            switch (m->action) {
            case 0:
            switch (HunterInfo_Update(pad)) {
            case 3:
                gFieldControl = 1;
                pad = 0;
                gSubMenuWork->pageStep--;
                break;
            case 1:
                gFieldControl = 1;
                pad = 0;
                gSubMenuWork->pageStep = 0;
                break;
            case 4:
                gFieldControl = 1;
                pad = 0x8000;
                break;
            case 2:
                pad = 0;
                break;
            }
            }
            break;
        }
        break;
    case 1:
        gHelpShow = 1;
        p = &m->pageStep;
        switch (*p) {
        case 0:
            b = pad;
            if (b & 0x240) {
                ChatPage_CloseMuted();
                pad = 0;
            } else if (m->mutedGrid.cols != 0) {
                Menu_GridCursor(&m->mutedGrid, pad);
                if (b & 0x20) {
                    m = gSubMenuWork;
                    m->pageStep++;
                    gHelpLine = 0x27;
                    Sound_PlaySeFlat(0, 0x13, 0);
                }
            }
            break;
        case 1:
            if (pad & 0x40) {
                *p = 0;
                gHelpLine = 0x26;
                Sound_PlaySeFlat(0, 0x14, 0);
                pad = 0;
            }
            b = pad;
            if (b & 0x200) {
                ChatPage_CloseMuted();
            } else if (b & 0x20) {
                if (IdList_Remove((struct Rec18 *) &gChatBlockList, gSubMenuWork->mutedGrid.index, 0x14) == 1) {
                    ChatPage_CountMuted();
                    ChatPage_MarkMuted();
                    gSubMenuWork->pageStep = 0;
                    gHelpLine = 0x26;
                    Sound_PlaySeFlat(0, 0x13, 0);
                } else {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
            }
            break;
        }
        break;
    }
    return pad;
}

void ChatPage_MarkMuted(void)
{
    s32 i;

    for (i = 0; i < gSubMenuWork->speakerCount; i++) {
        if (IdList_Find(&gChatBlockList, 0x14, (u8 *)&gSubMenuWork->speakers[i]) != -1) {
            gSubMenuWork->speakerMuted[i] = 1;
        } else {
            gSubMenuWork->speakerMuted[i] = 0;
        }
    }
}

void ChatPage_LoadSpeakers(void)
{
    gSubMenuWork->speakerCount = ChatLog_ListSpeakers(gSubMenuWork->speakers);
    gSubMenuWork->speakers[gSubMenuWork->speakerCount].id[0] = 0;
    ChatPage_MarkMuted();
}

s32 MuteList_Add(ChatSpeaker *e)
{
    s32 free;
    u8 *rec;

    free = IdList_FindFree(&gChatBlockList, 0x14);
    if (free != -1) {
        rec = (u8 *) (&gChatBlockList + (free * 0x18));
        strcpy(rec, e->id);
        strcpy(rec + 8, e->name);
        return 1;
    }
    return 0;
}

/* unk_771A60's .data (0x7F1F30-0x7F1F50) */
ListPanel gChatPanel = {
#include "lobby/007F1F30.inc"
};
