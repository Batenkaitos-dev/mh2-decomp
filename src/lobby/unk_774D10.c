#include "common.h"
#include "game_menu.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "window.h"

/* prototypes */
void GameMenu_DrawList(s8 dim);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s32 sel); /* unit's own declaration: sel s32 (the definition has s16): with s16 GameMenu_DrawMail changes by 10 of 27 words */
extern ListPanel gMailPanel;
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void TextColor_Close(void);
void TextColor_Draw(u8 *m);
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32, s32);
void LobbyText_UpdateByte2(void);
extern LobbyMenu *gLobbyMenu;
/* end of prototypes */

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void GameMenu_Back(void);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
void MailMenu_Open(void);
void ShortMail_Open(void *id, void *name, u32 flags);
void BlockList_Open(void);
s32 MailMenu_Update(u16 pad);
s32 ShortMail_Step(u16 pad);
s32 BlockList_Update(u16 pad);

u16 GameMenu_UpdateMail(u16 pad)
{
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
        gHelpLine = m->subEntry + 0x20;
        cur = &m->subEntry;
        if (pad & 0x20) {
            switch (*cur) {
            case 0:
                m->subStep++;
                MailMenu_Open();
                break;
            case 1:
                m->subStep++;
                ShortMail_Open(NULL, NULL, 1);
                break;
            case 2:
                m->subStep++;
                ShortMail_Open(NULL, NULL, 9);
                break;
            case 3:
                m->subStep++;
                BlockList_Open();
                break;
            }
            Sound_PlaySeFlat(0, 0x13, 0);
        }
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case 0:
            switch (MailMenu_Update(pad)) {
            case 3:
                gSubMenuWork->subStep--;
                return 0;
            case 1:
            case 4:
                gFieldControl = 1;
                pad = 0x8000;
                break;
            case 2:
                return 0;
            }
            break;
        case 1:
        case 2:
            switch (ShortMail_Step(pad)) {
            case 3:
                gSubMenuWork->subStep--;
                return 0;
            case 1:
            case 4:
                pad = 0x8000;
                break;
            case 2:
                return 0;
            }
            break;
        case 3:
            switch (BlockList_Update(pad)) {
            case 3:
                gSubMenuWork->subStep--;
                return 0;
            case 1:
            case 4:
                pad = 0x8000;
                break;
            case 2:
                return 0;
            }
            break;
        }
        if (pad & 0x8000) {
            GameMenu_Back();
            pad = 0;
        }
        break;
    }
    if (gSubMenuWork->subStep == 0) {
        gHelpShow = 1;
    } else {
        gHelpShow = 0;
    }
    return pad;
}

void GameMenu_DrawMail(void)
{
    u8 temp_v1;

    temp_v1 = gSubMenuWork->subStep;
    switch (temp_v1) {
    case 0:
        GameMenu_DrawList(1);
        ListPanel_Draw(&gMailPanel, (s16 *)0, (char *)0, (s32) gSubMenuWork->subEntry);
        
    case 1:
        break;
    }
}

void TextColor_Open(void)
{
    gLobbyMenu->screen = 0;
    gLobbyMenu->cursor = gPlayerText.card.chatColor;
    LobbyMenu_SetDraw(NULL, TextColor_Draw, NULL, NULL);
}

void TextColor_Close(void)
{
    gLobbyMenu->screen = 0;
    gLobbyMenu->cursor = 0;
    gHelpLine = gLobbyMenu->cursor + 5;
    LobbyMenu_SetDraw(NULL, NULL, NULL, NULL);
}

s32 TextColor_Input(s32 pad)
{
    s32 btn;
    u8 sel;

    switch (gLobbyMenu->screen) {
    case 0:
        btn = pad & 0xFFFF;
        if (btn & 0x20) {
            sel = gLobbyMenu->cursor;
            if (gChatTextColor != sel) {
                gChatTextColor = sel;
                LobbyText_UpdateByte2();
                LobbySe_Play(8);
            }
        } else if (btn & 0x40) {
            TextColor_Close();
        } else if (btn & 0x8000) {
            TextColor_Close();
        } else {
            gLobbyMenu->cursor = Menu_WrapUpDown(gLobbyMenu->cursor, 9);
            gHelpLine = gLobbyMenu->cursor + 5;
        }
        break;
    }
    return pad;
}

void *memcpy(void *dst, const void *src, u32 n);
void Text_SetSize(s32);
u32 Color_PulseAlpha(u32 color);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
extern char *gHunterStateTexts[];
extern ListPanel gTextColorPanel;
extern char *gOnlineMarkTexts[];

/* The window of the 9 colours (gPlayerText.card.chatColor: the chosen one, 0 none): each
 * entry's text colour code, then the swatches. */
void TextColor_Draw(u8 *m)
{
    s16 i;
    char **p;
    HunterCard *c;
    s16 y;
    u32 color;

    c = &gPlayerText.card;
    for (i = 0, p = gHunterStateTexts; i < 9; i++, p++) {
        if (c->chatColor == 0) {
            memcpy(*p, "~C00", 4);
        } else if (c->chatColor == i) {
            memcpy(*p, "~C04", 4);
        } else {
            memcpy(*p, "~C10", 4);
        }
    }
    Text_SetSize(*(s32 *)(m + 0x18));
    gTextColorPanel.cursorColor = Color_PulseAlpha(0x8080);
    ListPanel_Draw(&gTextColorPanel, (s16 *)0, gOnlineMarkTexts[0], gLobbyMenu->cursor);
    for (y = 0x6B, i = 1; i < 9; y += 0x17, i++) {
        if (c->chatColor == i || c->chatColor == 0) {
            color = -1;
        } else {
            color = 0xFF707070;
        }
        Icon_Draw(0x1A3, y, 0x19, 0x14, color, i + 0x59);
    }
}

/* the padding at the end of the file's .rodata */
const u8 D_0083B518[8] = { 0 };

/* unk_774D10's .data (0x7F2070-0x7F20B0) */
extern char *gMailboxMenuTexts[];
ListPanel gMailPanel = {
#include "lobby/007F2070.inc"
};
ListPanel gTextColorPanel = {
#include "lobby/007F2090.inc"
};
