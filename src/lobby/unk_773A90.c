/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "game_menu.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "window.h"
s32 Gs_SetState(u8, u32);

extern M2C_UNK gNetUsers;
void LobbyMenu_Init(void);
extern LobbyMenu *gLobbyMenu;
s32 memcmp(void *, void *, s32);

/* unk_773A90's .rodata (0x83B4A8-0x83B4B8) */
#pragma rodata on
char D_0083B4A8[13] = {
#include "lobby/0083B4A8.inc"
};
#pragma rodata off

void GameMenu_DrawPause(void) {

}

void *NetUser_Get(s32 idx)
{
    return (u8 *)&gNetUsers + ((idx & 0xFF) * 0x11C);
}

s32 Card_IsFlagE055(void);
void GameMenu_DrawList(s8 dim);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
struct Frame;
void WindowFrame_Draw(struct Frame *f, s16 *off);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
extern ListPanel gQuitPanel;
extern Frame gQuitFrame;
extern char *gYesNoTexts[];
extern char *gQuitSaveTexts[];
extern char *gQuitConfirmTexts[];
extern char *gQuitMenuTexts[];
extern char *gQuitMenuDimTexts[];

void Text_Printf(const char *, ...);
void Text_PrintfColored(const char *, ...);

void GameMenu_DrawQuit(void)
{
    s8 k;

    switch (gSubMenuWork->subStep) {
    case 0:
        GameMenu_DrawList(1);
        gQuitPanel.items = (Card_IsFlagE055() == 1) ? gQuitMenuTexts : gQuitMenuDimTexts;
        ListPanel_Draw(&gQuitPanel, 0, 0, gSubMenuWork->subEntry);
        break;
    case 1:
        Font_SetSpacing(0x12, 0x12);
        Font_SetColor(0x807F7F7F);
        switch (gSubMenuWork->page) {
        case 0:
            k = gSubMenuWork->subEntry;
            Font_SetPos(0xE6, 0x104);
            Text_Printf(gYesNoTexts[gSubMenuWork->yesNo]);
            break;
        case 1:
            k = 2;
            Font_SetPos(0xE6, 0x104);
            Text_Printf(gYesNoTexts[gSubMenuWork->yesNo]);
            break;
        case 2:
            k = 3;
            Font_SetPos(0x9E, 0x104);
            Text_Printf(gQuitSaveTexts[gSubMenuWork->yesNo]);
            break;
        default:
            k = -1;
            break;
        }
        if (k >= 0) {
            WindowFrame_Draw(&gQuitFrame, NULL);
            Font_SetColor(0x807F7F7F);
            Font_SetPos(0x9A, 0xB2);
            Text_PrintfColored(gQuitConfirmTexts[k]);
        }
        break;
    }
}

s32 LobbyMember_OpenList(void) {
    u8 temp_v1;

    LobbyMenu_Init();
    temp_v1 = gLobbyMenu->cursor;
    if (temp_v1 == gScene.localPlayer) {
        gLobbyMenu->cursor = (u8) (temp_v1 + 1);
    }
    gLobbyMenu->step = 0;
    gHelpShow = 0;
    return 0;
}

void GameMenu_Back(void);
s32 NetUser_State(s8 i);
void LobbyMember_LoadEquip(u8 i);
void LobbySe_Play(s32 se);
s32 FriendList_AddInput(u16 pad);
void FriendList_AskAdd(void *id, void *name);
void LobbyText_Open(s32);
void LobbyDialog_Grow(void);
extern u8 gTownMsg0DPending;

u16 LobbyMember_ListInput(u16 pad)
{
    s16 c;
    u8 *cur;
    u8 old;
    u8 *b;
    s32 k;
    s32 b2;

    switch (gLobbyMenu->screen) {
    case 0:
        if (pad & 0x8000) {
            GameMenu_Back();
            return 0;
        }
        if (pad & 0x20) {
            if (gPlayers[gLobbyMenu->cursor].active != 0 && NetUser_State(gLobbyMenu->cursor) != 2) {
                LobbyMember_LoadEquip(gLobbyMenu->cursor);
                gLobbyMenu->screen++;
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
            break;
        }
        if (pad & 0x40) {
            return pad;
        }
        cur = &gLobbyMenu->cursor;
        old = *cur;
        c = old;
        if (pad & 0x2000) {
            c--;
            if (c < 0) {
                c = 9;
            }
            if (c == gScene.localPlayer) {
                c--;
            }
            if (c < 0) {
                c = 9;
            }
        } else if (pad & 0x1000) {
            c++;
            if (c >= 10) {
                c = 0;
            }
            if (c == gScene.localPlayer) {
                c++;
            }
            if (c >= 10) {
                c = 0;
            }
        }
        if (c != old) {
            *cur = c;
            LobbySe_Play(1);
            gLobbyMenu->step = 0;
        }
        break;
    case 1:
        b2 = pad;
        if (b2 & 0x8000) {
            GameMenu_Back();
            return 0;
        }
        if (NetUser_State(gLobbyMenu->cursor) == 2) {
            gLobbyMenu->screen--;
            LobbySe_Play(4);
        } else if (b2 & 0x40) {
            gLobbyMenu->screen--;
            LobbySe_Play(4);
            pad = b2 & ~0x40;
        } else if (gLobbyMenu->step == 0 && (b2 & 0x200)) {
            if (((u8 *)gLobby + gLobbyMenu->cursor * 0x11C)[0xA41] != 0) {
                gLobbyMenu->screen = 3;
                gFieldControl = 0;
                LobbyText_Open(0x93);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
                LobbySe_Play(8);
            } else {
                gLobbyMenu->screen++;
                gLobby->connected = 0;
                b = (u8 *)gLobby + gLobbyMenu->cursor * 0x11C;
                FriendList_AskAdd(b + 0xA16, b + 0xA1E);
                LobbySe_Play(8);
            }
        } else if (b2 & 0xC00) {
            gLobbyMenu->step ^= 1;
            LobbySe_Play(0xB);
        }
        break;
    case 2:
        pad &= 0x7FFF;
        switch (FriendList_AddInput(pad)) {
        case 0:
        case 1:
            gLobby->connected = 1;
            gTownMsg0DPending = 1;
            k = gLobbyMenu->cursor * 0x11C;
            if (memcmp((u8 *)&gNetUsers + k, (u8 *)gLobby + k + 0xA16, 8) != 0) {
                gLobbyMenu->screen -= 2;
            } else {
                gLobbyMenu->screen--;
            }
            break;
        }
        break;
    case 3:
        b2 = pad;
        gLobbyMenu->redraw = 1;
        if (b2 & 0x20) {
            gLobbyMenu->screen = 1;
            gFieldControl = 1;
            LobbySe_Play(8);
        }
        pad = b2 & ~0x40;
        break;
    }
    if (gLobby->connected != 0) {
        return pad;
    }
    return 0;
}

s32 Game_IsOnline(void);
s32 LobbyMember_InMyParty(s32 arg0);
s32 sprintf(char *, const char *, ...);
struct ListWin;
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel);
void Icon_DrawKind59(s32 a, s16 b, s32 c, s32 d, s16 e); /* unit's own declaration: with the definition's void (s32 a, s32 b, s32 c, s32 d, s16 e) (lobby/unk_78F070.c), LobbyMember_DrawList 6 of 248 words, LobbyMenu_DrawMemberNames 84 of 381 words differ */
void Lobby_DrawSpriteFE9B0(s32 x, s16 y, s32 w, s32 color, s16 kind);
s32 MemberCard_Draw(u8, u8);
void MemberCard_DrawFriendReg(s16, u8);
extern char *D_007F2048;
extern ListWin gMemberListPanel;
extern s32 gMemberFlagSprites[];
extern char gMemberListTexts[10][0x14];   /* the member list texts */
extern char *D_008B4CA0[];          /* the member list (text pointers) */

#define LM ((LobbyMenu *)gLobbyMenu)

/* The member list: one coloured name line per other player (6 when
 * LobbyMember_InMyParty says so, 10 when in no area, 0 otherwise), then their marks
 * and status icons (0xA3A: 0x80 or 0x40, 0x20 blinks), or the member page. */
void LobbyMember_DrawList(void)
{
    u8 marks[10];
    u8 *base;
    Player *p;
    s32 off;
    u8 *mk;
    char *name;
    u8 *self;
    char *buf;
    char **list;
    s32 i;
    s16 color;
    s32 cur;
    s16 y;
    u8 f;
    u8 hi;

    hi = 0x80;
    switch (LM->screen) {
    case 0:
        GameMenu_DrawList(1);
        i = 0;
        p = gPlayers;
        off = 0;
        self = (u8 *)&gNetUsers;
        base = marks;
        buf = gMemberListTexts[0];
        mk = base;
        list = D_008B4CA0;
        do {
            if (i != gScene.localPlayer) {
                if (p->active != 0 && NetUser_State(i) != 2) {
                    name = (char *)gLobby + off + 0xA1E;
                    if (Game_IsOnline() == 1) {
                        if (gChatHideNames != 0) {
                            name = (char *)self;
                        }
                        *mk = *(u8 *)((u32)gLobby + off + 0xA34);
                    }
                    if (LobbyMember_InMyParty(i) == 1) {
                        color = 6;
                    } else if (p->area == 0) {
                        color = 10;
                    } else {
                        color = 0;
                    }
                    sprintf(buf, "~C%02d~%%%s~%%", color, name);
                    *list = buf;
                } else {
                    *list = D_007F2048;
                }
                buf += 0x14;
                list++;
            }
            i++;
            p++;
            off += 0x11C;
            self += 0x11C;
            mk++;
        } while (i < 10);
        cur = LM->cursor;
        if (gScene.localPlayer < cur) {
            cur--;
        }
        gMemberListPanel.items = D_008B4CA0;
        ListPanel_DrawDim(&gMemberListPanel, 0, cur);
      {
        Player *p2;
        s32 off2;
        s32 i2;

        y = 0x3E;
        i2 = 0;
        p2 = gPlayers;
        off2 = 0;
        do {
            if (i2 != gScene.localPlayer) {
                if (p2->active != 0) {
                    if (NetUser_State(i2) == 0) {
                        Icon_DrawKind59(0x1A4, y, 0x12, -1, *base);
                    }
                    if (p2->area != 0) {
                        f = *(u8 *)((u32)gLobby + off2 + 0xA3A);
                        if (f & 0x80) {
                            if (!(f & 0x20) || (gFrameTick & 0x10)) {
                                Lobby_DrawSpriteFE9B0(0x18E, y, 0x12, gMemberFlagSprites[f & 0xF], 10);
                            }
                        } else if (f & 0x40) {
                            if (!(f & 0x20) || (gFrameTick & 0x10)) {
                                Lobby_DrawSpriteFE9B0(0x18E, y, 0x12, gMemberFlagSprites[f & 0xF], 9);
                            }
                        }
                    }
                }
                y += 0x15;
            }
            i2++;
            p2++;
            base++;
            off2 += 0x11C;
        } while (i2 < 10);
      }
        break;
    case 1:
        hi = 0;
    case 2:
        if (LM->redraw != 0) {
            hi = 0x80;
        }
        MemberCard_Draw(LM->cursor, LM->step | hi);
        if (LM->step == 0) {
            MemberCard_DrawFriendReg(0x184, LM->screen - 1);
        }
        break;
    }
}

s32 LobbyMember_InMyParty(s32 idx)
{
    s32 i;

    if (gLobby->questTaken == 0) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (memcmp(gLobby->members[idx].id, gLobby->players[i].id, 8) == 0) {
            return 1;
        }
    }
    return 0;
}

s32 GameMenu_OpenMail(void *m);
void NameTags_DrawMembers(u8 *arg0);
void Text_SetSize(s32 size);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
void *memset(void *, s32, u32);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Win_Draw(s16 x, s16 y, s32 w, s16 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), LobbyMenu_DrawMemberNames 181 of 379 words differ */
void Icon_DrawKind48(s16, s16, s32, s32, u8, s32);
u32 Color_Cycle8(void);
extern s32 gTownMode;
extern u8 gLobbyLevel1Sel;
extern u8 gMemberNamesOff;
extern char D_008B4AD0[10][0x14];
extern char *gMemberNameTexts[];
extern char *D_0080C21C;
extern u32 gPlayerColorWords[];

void LobbyMenu_DrawMemberNames(u8 *t)
{
    s32 i;
    char *name;
    s16 y;
    s32 slot;
    u8 *m;
    Player *p;
    s8 n;
    u8 *list[10];
    u8 *e;
    u8 **pp;
    s32 k;
    u8 f;
    u8 c;

    NameTags_DrawMembers(t);
    Gs_SetState(0x60, 0);
    Text_SetSize(*(s32 *)(t + 0x18));
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    if (Game_IsOnline() == 0) {
        return;
    }
    if (gTownMode != 0 && gTownMode != 7 && gTownMode != 0x19) {
        return;
    }
    if (gLobbyLevel1Sel == 0) {
        return;
    }
    if (gMemberNamesOff != 0) {
        return;
    }
    memset(list, 0, sizeof(list));
    n = 0;
    i = 0;
    slot = 1;
    m = (u8 *)&gNetUsers;
    p = gPlayers;
    for (; i < 10; i++) {
        if ((s8)m[0] != 0) {
            e = m + 0x1C;
            if (i == gScene.localPlayer) {
                k = 0;
                slot--;
            } else {
                k = slot;
            }
            pp = &list[k];
            *pp = e;
            if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                if (LobbyMember_InMyParty(i) == 1) {
                    name = D_008B4AD0[k];
                    sprintf(name, "~C06~%%%s~%%", m);
                } else if (p->area == 0) {
                    name = D_008B4AD0[k];
                    sprintf(name, "~C10~%%%s~%%", m);
                    *pp = NULL;
                } else {
                    name = D_008B4AD0[k];
                    sprintf(name, "~C00~%%%s~%%", m);
                }
            } else {
                if (LobbyMember_InMyParty(i) == 1) {
                    name = D_008B4AD0[k];
                    sprintf(name, "~C06~%%%s~%%", m + 8);
                } else if (p->area == 0) {
                    name = D_008B4AD0[k];
                    sprintf(name, "~C10~%%%s~%%", m + 8);
                    *pp = NULL;
                } else {
                    name = D_008B4AD0[k];
                    sprintf(name, "~C00~%%%s~%%", m + 8);
                }
            }
            gMemberNameTexts[k] = name;
            slot++;
            n++;
        }
        m += 0x11C;
        p++;
    }
    Font_SetSpacing(0x12, 0x12);
    i = n;
    y = 0x1A4 - (i + 1) * 22;
    Win_Draw(0x1CC, y, 0x96, (i + 1) * 22 - 2, 0xBDB59F6C, 0x80202020, 6);
    Gfx_DrawLine(0x1CC, y, 0x94, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(0x1D2, y + 2);
    Text_Printf("%s", D_0080C21C);
    y += 0x16;
    for (slot = 0; slot < i; slot++) {
        e = list[slot];
        Gfx_DrawLine(0x1CC, y - 2, 0x94, 2, 0x80FFFFFF);
        if (e != NULL) {
            f = e[8];
            if (f & 0x80) {
                Lobby_DrawSpriteFE9B0(0x232, y, 0x12, gMemberFlagSprites[f & 0xF], 10);
            } else if (f & 0x40) {
                if (!(f & 0x20) || (gFrameTick & 0x10)) {
                    Lobby_DrawSpriteFE9B0(0x232, y, 0x12, gMemberFlagSprites[f & 0xF], 9);
                }
            }
            if (e[0xD7] == 0 || (gFrameTick & 0x10)) {
                Icon_DrawKind59(0x24A, y, 0x12, -1, e[2]);
            } else {
                Lobby_DrawSpriteFE9B0(0x24A, y, 0x14, 0xFF40FF40, 0x13);
            }
            c = e[0xC];
            if (c != 0xFF) {
                if (c == 0xB) {
                    Gfx_DrawLine(0x1CC, y, 0x16, 0x14, Color_Cycle8());
                } else {
                    Gfx_DrawLine(0x1CC, y, 0x16, 0x14, gPlayerColorWords[c]);
                }
            }
            Icon_DrawKind48(0x1CC, y, 0x14, -1, e[3], 0);
        }
        Font_SetPos(0x1E4, y);
        Text_Printf("%s", gMemberNameTexts[slot]);
        y += 0x16;
    }
}

s32 GameMenu_OpenMail(void *m) {
    gSubMenuWork->subStep = 0;
    gSubMenuWork->subEntry = 0;
    gHelpShow = 1;
    gHelpPage = 4;
    gHelpLine = 0x20;
    return 0;
}

/* unk_773A90's .data (0x7F2010-0x7F2070) */
extern char D_0083B4A8[];
ListPanel gQuitPanel = {
#include "lobby/007F2010.inc"
};
Frame gQuitFrame = {
#include "lobby/007F2030.inc"
};
char *D_007F2048 = (char *)D_0083B4A8;
ListWin gMemberListPanel = {
#include "lobby/007F2050.inc"
};
