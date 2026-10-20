/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "sprite.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_menu.h"
#include "lobby_labels.h"

void Font_SetSpacing(u8 a, u8 b);
void Text_SetSize(s32);
void CamFocus_End(void);
void LobbySe_Play(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void ShortMail_Close(void) {
    gFieldControl = 1;
}

s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
void ShortMail_Close(void);
s32 ShortMail_Input(ShortMailMenu *m, u16 pad);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void LobbyText_Open(s32);
s32 LobbyStep_ShortMailSend(MailEntry *msg);
extern ShortMailMenu gShortMailMenu;
extern LobbyMenu *gLobbyMenu;
s32 ShortMail_Step(u16 pad)
{
    ShortMailMenu *m = &gShortMailMenu;

    switch (gShortMailMenu.step) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            ((s8 *)m)[0x4B2]++;
            break;
        }
        break;
    case 1:
        switch (ShortMail_Input((ShortMailMenu *)m, pad)) {
        case 0:
            ((s8 *)m)[0x4B2]++;
            LobbyText_Open(0x44);
            gLobbyMenu->redraw = 1;
            break;
        case 3:
            ShortMail_Close();
            return 3;
        case 4:
            ShortMail_Close();
            return 4;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_ShortMailSend(&m->mail)) {
        case 0:
            m->step = 3;
            LobbyText_Open(0x3B);
            gLobbyMenu->redraw = 1;
            break;
        case 1:
            m->step = 3;
            gLobbyMenu->redraw = 1;
            break;
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            LobbySe_Play(8);
            ShortMail_Close();
            return 3;
        }
        break;
    }
    OT_AddAt(gDrawListText0b, &m->layer[0], 0, 1);
    OT_AddAt(gDrawListText1, &m->layer[1], 0, 1);
    return 2;
}

void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
s32 memcmp(void *a, void *b, s32 n);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void Kbd_Cancel(void);
void Chat_FilterWords(char *s);
void FriendList_Open(s32 a);
s32 LobbyUser_FetchDetails(void *out, void *id);
s32 FriendList_Update(u16 pad);
void *FriendList_GetSelectedId(void);
void *FriendList_GetSelectedName(void);
extern char gMyUserId[];
u32 strlen(const char *s);
void *memset(void *p, s32 c, u32 n);
void *memcpy(void *d, const void *s, u32 n);

/* sel is the s16 field; the wrap call reads m->sel again (the loaded value
 * passes on without an extension to Menu_WrapUpDown's s32 parameter). */
s32 ShortMail_Input(ShortMailMenu *m, u16 pad)
{
    s16 sel = m->sel;
    s8 r;

    switch (m->state) {
    case 0:
        if (pad & 0x20) {
            if (!(m->flags & 1)) {
                sel++;
            }
            switch (sel) {
            case 0:
                m->state = 2;
                m->sel2 = 0;
                LobbySe_Play(8);
                break;
            case 1:
                m->state = 1;
                Kbd_SetPos(0x32, 80.0f);
                Chat_OpenKbd(1, 0xE, 0x78, m->mail.text);
                LobbySe_Play(8);
                break;
            case 2:
                if (strlen(m->mail.id) < 6) {
                    LobbySe_Play(9);
                } else if (memcmp(m->mail.id, gMyUserId, 6) == 0) {
                    LobbySe_Play(9);
                } else {
                    m->state = 0;
                    LobbySe_Play(8);
                    return 0;
                }
                break;
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else if (pad & 0x8000) {
            return 4;
        } else {
            m->sel = Menu_WrapUpDown(m->sel, m->count);
        }
        break;
    case 1:
        if (gPadState[0].pressed & 0x4000) {
            r = -1;
        } else {
            r = Kbd_Run(m->mail.text, *(s16 *)&gPadState[0].held, gPadState[0].pressed);
        }
        switch (r) {
        case 1:
        case -1:
            m->state = 0;
            Chat_FilterWords(m->mail.text);
            Kbd_Cancel();
            break;
        default:
            if (pad == 0x4000) {
                m->state = 0;
                Kbd_Cancel();
                LobbySe_Play(4);
            }
            break;
        }
        break;
    case 2:
        if (pad & 0x20) {
            if (m->sel2 == 0) {
                m->state = 5;
                FriendList_Open(1);
            } else {
                m->state = 3;
                Kbd_SetPos(0x32, 80.0f);
                Chat_OpenKbd(0, 6, 6, m->mail.id);
            }
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->state = 0;
            LobbySe_Play(4);
        } else if (pad & 0x8000) {
            return 4;
        } else {
            m->sel2 = Menu_WrapUpDown(m->sel2, 2);
        }
        break;
    case 3:
        if (gPadState[0].pressed & 0x4000) {
            r = -1;
        } else {
            r = Kbd_Run(m->mail.id, *(s16 *)&gPadState[0].held, gPadState[0].pressed);
        }
        switch (r) {
        case 1:
        case -1:
            if (strlen(m->mail.id) < 6) {
                m->state = 6;
                memset(m->mail.name, 0, 0x10);
                LobbyText_Open(0x23);
                gLobbyMenu->redraw = 1;
            } else if (memcmp(m->mail.id, gMyUserId, 6) == 0) {
                m->state = 6;
                memset(m->mail.name, 0, 0x10);
                LobbyText_Open(0x27);
                gLobbyMenu->redraw = 1;
            } else {
                m->state++;
                LobbyText_Open(0x20);
                gLobbyMenu->redraw = 1;
            }
            Kbd_Cancel();
            break;
        default:
            if (pad == 0x4000) {
                m->state = 0;
                Kbd_Cancel();
                LobbySe_Play(4);
            }
            break;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_FetchDetails(m->details, m->mail.id)) {
        case 0:
            m->state = 0;
            memcpy(m->mail.name, m->details, 0x10);
            break;
        case 1:
            m->state = 6;
            memset(m->mail.name, 0, 0x10);
            gLobbyMenu->redraw = 1;
            break;
        }
        break;
    case 5:
        switch (FriendList_Update(pad)) {
        case 0:
            m->state = 0;
            memcpy(m->mail.id, FriendList_GetSelectedId(), 8);
            memcpy(m->mail.name, FriendList_GetSelectedName(), 0x10);
            break;
        case 3:
            m->state = 2;
            break;
        case 4:
            return 4;
        }
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->state = 0;
            LobbySe_Play(8);
        }
        break;
    }
    return 2;
}

void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *fmt, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Sjis_Widen(char *src, char *dst);
u32 Color_PulseAlpha(u32 color);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
s32 Text_DrawWrapped(s16, s16, s16, char *, s16);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);

/* ly is an s32 holding the row wrapped to 16 bits: a value of its own, numbered at its declaration
 * (an s16 ly assigned once is substituted and numbered after every local), as the game's registers
 * need; the body box's row is a second local, by. The title string goes through a void * view t. */
void ShortMail_DrawForm(MailEntry *p, s16 sel, s32 flags, s16 x, s16 y)
{
    u32 c;
    void *t;
    s16 cy;
    s32 f1;
    s32 ly;
    s32 f2;
    s32 sy;
    s32 sx;
    MailEntry *m;
    char buf[0x20];
    s16 tx;
    char *s;
    s16 by;

    cy = y;
    m = p;
    f1 = flags & 1;
    if (!f1) {
        sel += 1;
    }
    Icon_Draw(x, y, 0x154, 0x122, 0xFFE0D8C8, 0);
    Font_SetColor(0x80301010);
    t = gGameMenuLabels[MENU_LABEL_SHORT_MAIL];
    sx = x;
    Font_SetPos(Text_CenterX(t, x + 0xAA, 0x12), cy += 0xC);
    Text_PrintfColored(t);
    sy = cy;
    Font_SetPos(sx + 0x28, sy + 0x1E);
    Text_PrintfColored("%s%s", gNetLabels[NET_LABEL_NAME], gLobbyWords[LOBBY_WORD_COLON]);
    Font_SetColor(0x80101010);
    Font_SetPos(sx + 0x28, sy + 0x34);
    Sjis_Widen((char *)m->name, buf);
    Text_PrintfColored("%s", buf);
    Font_SetColor(0x80301010);
    Font_SetPos(sx + 0x28, sy + 0x4A);
    Text_PrintfColored("%s%s", gNetLabels[NET_LABEL_ID], gLobbyWords[LOBBY_WORD_COLON]);
    Font_SetColor(0x80101010);
    Font_SetPos(sx + 0x28, sy + 0x60);
    Sjis_Widen((char *)m->id, buf);
    Text_PrintfColored("%s", buf);
    f2 = flags & 2;
    if (!f2 && f1) {
        if (sel == 0) {
            c = Color_PulseAlpha(0xFF008080);
            Menu_DrawSideArrows(sx + 0x28, sy + 0x60, 0x78, 0x16, 0, 0xFF008080);
        } else {
            c = 0xFF808080;
        }
        ly = (s16)(sy + 0x60);
        Gfx_DrawRectFrame(sx + 0x28, ly - 1, 0x78, 0x16, c);
        Gfx_DrawRectFrame(sx + 0x28, ly - 2, 0x76, 0x18, c);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(sx + 0x28, sy + 0x76);
    Text_PrintfColored("%s%s", gNetLabels[NET_LABEL_MAIL_BODY], gLobbyWords[LOBBY_WORD_COLON]);
    Font_SetColor(0x80101010);
    by = sy + 0x8C;
    Text_DrawWrapped((s16)(sx + 0x28) - 10, by + 2, 0x16, m->text, 0x1F);
    if (!f2) {
        if (sel == 1) {
            c = Color_PulseAlpha(0xFF008080);
            Menu_DrawSideArrows(sx + 0x14, by + 0x16, 0x12C, 0x16, 0, 0xFF008080);
        } else {
            c = 0xFF808080;
        }
        Gfx_DrawRectFrame(sx + 0x14, sy + 0x8C, 0x12C, 0x5A, c);
        Gfx_DrawRectFrame(sx + 0x13, by - 1, 0x12E, 0x5C, c);
    }
    if (!f2) {
        s = gNetLabels[NET_LABEL_SEND];
        tx = Text_CenterX(s, x + 0xAA, 0x12);
        if (sel == 2) {
            c = 0x80301010;
            Menu_DrawArrowsRev(tx, sy + 0xEE, 0x12, gNetLabels[NET_LABEL_SEND], 0xFF008080);
        } else {
            c = 0x80404040;
        }
        Font_SetColor(c);
        Font_SetPos(tx, sy + 0xEE);
        Text_PrintfColored("%s", s);
        if (sel == 2) {
            Menu_DrawArrowsRev(tx, sy + 0xEE, 0x12, gNetLabels[NET_LABEL_SEND], 0xFF008080);
        }
    }
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), ShortMail_DrawActions 98 of 102 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawLine(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: with the definition's void (s16 x0, s16 y0, s16 x1, s16 y1, s8 flag) (lobby/unk_790EC0.c), ShortMail_DrawActions 37 of 104 words differ */
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *fmt, ...);
void ShortMail_DrawActions(ShortMailMenu *m, s16 x, s16 y);
void ShortMail_DrawActions(ShortMailMenu *m, s16 x, s16 y)
{
    s16 i;
    s32 sel;

    Win_Draw(x - 0x96, y, 0x96, 0x3A, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < 2; i++) {
        if (i > 0) {
            Gfx_DrawLine((s16)(x - 0x96) + 8, y - 2, 0x86, 2, 0xBDB59F6C);
        }
        if (m->sel2 == i) {
            switch (m->state) {
            case 2:
                sel = 0;
                break;
            default:
                sel = 1;
                break;
            }
            Menu_DrawLine((s16)(x - 0x96) + 8, y - 1, 0x86, 0x14, sel);
        }
        Font_SetPos((s16)(x - 0x96) + 8, y);
        Text_PrintfColored(gNetLabels[NET_LABEL_FRIEND_LIST + i]);
        y += 0x16;
    }
}

void ShortMail_Draw(MenuTask *task) {
    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
}

void FriendList_Draw(MenuTask *task);
void ShortMail_DrawForm(MailEntry *p, s16 sel, s32 flags, s16 x, s16 y);
s32 Kbd_IsOpen(void); /* unit's own declaration: the definition (sub_main) returns s8;
 * ShortMail_Draw2 is 17 words off with s8 */
void Chat_DrawKbdBox(s32 color);
extern u8 gLobbyLevel0Sel;
void ShortMail_Draw2(MenuTask *task)
{
    ShortMailMenu *m = &gShortMailMenu;
    s16 row;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    switch (gShortMailMenu.step) {
    case 0:
    case 1:
    case 2:
    case 3:
        row = m->win.pos;
        if (m->step != 1 || m->state != 5) {
            ShortMail_DrawForm(&m->mail, m->sel, m->flags, row, 0x36);
        }
        if (m->state == 2) {
            ShortMail_DrawActions(m, row, 0x36);
        } else if (m->state == 5) {
            FriendList_Draw(task);
        }
        break;
    }
    if (gLobbyLevel0Sel == 0 && Kbd_IsOpen() != 0) {
        Text_SetSize(task->textSize);
        Chat_DrawKbdBox(0xBD6C6CC5);
    }
}

extern u8 gMenuWork[];
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
void ElderForecast_Draw(u8 *);
void ElderForecast_Open(void)
{
    u8 *m = (u8 *)gMenuWork;

    memset(m, 0, 0xF00);
    *(s32 *)(m + 8) = LobbyWorld_GetSeason();
    *(s32 *)(m + 0xC) = LobbyTime_IsPm();
    gTownMode = 0x44;
    gTownMenuOpen = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ElderForecast_Draw, 0, 0, 0);
    LobbySe_Play(0);
}

void ElderForecast_Close(void) {
    gTownMode = 0;
    gTownMenuOpen = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
}
