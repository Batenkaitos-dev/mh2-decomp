#include "common.h"
#include "m2c_macros.h"
#include "player.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "chat_log.h"
#include "lobby_labels.h"
char *strcpy(char *, const char *);
void *memset(void *, s32, u32);

void LobbySe_Play(s32);
u16 LobbyPad_Get(s32);
void Menu_SetYesNo(s8);
extern LobbyMenu *gLobbyMenu;
u8 Notice_PushItem(char *item);
extern char *gLobbyNoticeTexts[];
extern s8 gNoticeCooldown;

/* Moves a cursor over a grid of total cells, cols per row, with the pad;
   returns the cell. (n, an s32 copy of total, came from the permuter; the
   declaration order from a sweep.) cols comes in as an s32 and is narrowed
   to the s16 range in place, as the game does (a2 extended in place). */
s16 Menu_GridMove(u8 *colp, s16 *rowp, s32 cols, s16 total, u16 pad)
{
    u8 col;
    s16 row;
    s32 idx;
    s32 cur;
    s32 n;
    s32 oldcol;
    s16 rows;
    s32 oldrow;

    cols = (s16)cols; /* the parameter narrowed to the s16 range */
    col = *colp;
    row = *rowp;
    oldrow = row;
    cur = (s16)(col + row * cols);
    oldcol = col;
    if (!(pad & 0x3C00) || total < 2) {
        return cur;
    }
    n = total;
    rows = n / cols;
    if ((n % cols) != 0) {
        rows++;
    }
    if (pad & 0x2000) {
        if (oldcol == 0) {
            if (oldrow < rows - 1 || (n % cols) == 0) {
                col = cols - 1;
            } else {
                col = (n % cols) - 1;
            }
        } else {
            col--;
        }
    } else if (pad & 0x1000) {
        if (oldrow < rows - 1 || (n % cols) == 0) {
            col++;
            if (col >= cols) {
                col = 0;
            }
        } else {
            col++;
            if (col >= (n % cols)) {
                col = 0;
            }
        }
    } else if (pad & 0x800) {
        row--;
        if (row < 0) {
            row = rows - 1;
        }
    } else if (pad & 0x400) {
        row++;
        if (row >= rows) {
            row = 0;
        }
    }
    idx = (s16)(col + row * cols);
    if (idx >= n) {
        col = 0;
        idx = (s16)(row * cols);
    }
    if (oldcol == col && oldrow == row) {
        return cur;
    }
    LobbySe_Play(1);
    *rowp = row;
    *colp = col;
    return idx;
}

s32 Menu_YesNoInput(void)
{
    s32 pad;

    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & 0x20) {
        if (gLobbyMenu->yesNo == 0) {
            LobbySe_Play(0);
            return 0;
        }
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x40) {
        if (gLobbyMenu->yesNo != 1) {
            Menu_SetYesNo(1);
            LobbySe_Play(4);
            goto block_15;
        }
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x800) {
        if (gLobbyMenu->yesNo != 0) {
            Menu_SetYesNo(0);
            LobbySe_Play(1);
        }
    } else if ((pad & 0x400) && (gLobbyMenu->yesNo != 1)) {
        Menu_SetYesNo(1);
        LobbySe_Play(1);
    }
block_15:
    return 2;
}

s32 LobbyMenu_YesNoInput(s32 se)
{
    s32 pad;

    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & 0x20) {
        if (gLobbyMenu->yesNo == 0) {
            LobbySe_Play(se);
            return 0;
        }
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x40) {
        if (gLobbyMenu->yesNo != 1) {
            Menu_SetYesNo(1);
            LobbySe_Play(4);
            goto block_15;
        }
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x800) {
        if (gLobbyMenu->yesNo != 0) {
            Menu_SetYesNo(0);
            LobbySe_Play(1);
        }
    } else if ((pad & 0x400) && (gLobbyMenu->yesNo != 1)) {
        Menu_SetYesNo(1);
        LobbySe_Play(1);
    }
block_15:
    return 2;
}

extern s8 gMenuInputLock;

s32 WinSlide_Step(WinSlide *s, s16 from, s16 to)
{
    switch (s->state) {
    case 0:
        s->state++;
        s->pos = from;
        s->timer = 5;
        s->step = (to - s->pos) / s->timer;
        gMenuInputLock = 6;
        break;
    case 1:
        if (--s->timer < 0) {
            s->state++;
            if (s->step >= 0) {
                s->pos = to + 6;
            } else {
                s->pos = to - 6;
            }
        } else {
            s->pos += s->step;
        }
        break;
    case 2:
        s->pos = to;
        s->state = 0;
        return 0;
    }
    return 2;
}

s32 Win_Shake(WinShake *s)
{
    switch (s->state) {
    case 0:
        s->timer = 5;
        s->state++;
        LobbySe_Play(0x18);
        break;
    case 1:
        switch (s->timer) {
        case 3:
            s->dx = -2;
            s->dy = 0;
            break;
        case 2:
            s->dx = 0;
            s->dy = 1;
            break;
        case 1:
            s->dx = 0;
            s->dy = 0;
            break;
        }
        if (--s->timer <= 0) {
            s->state = 0;
            s->dx = 0;
            s->dy = 0;
            return 0;
        }
        break;
    }
    return 2;
}

s32 Menu_CountPages(s16 count, s16 perPage)
{
    s32 n;
    s32 per;
    s32 ret;

    per = perPage;
    if (per == 0) {
        return 1;
    }
    n = count;
    ret = (s16)((n / per));
    if ((n % per) != 0) {
        ret = (s16)((ret + 1));
    }
    return ret;
}

extern u16 gTalkLine;
extern s32 gTalkTextFrames;
extern s8 gTalkMode;
extern s8 D_008BDC02;
extern s32 gTalkNpcName;
char *Talk_LineText(u16 id);
s32 Talk_LineKind(u16 id);
s32 Notice_OpenTextN(char *, s32, s32, s32, s32); /* unit's own declaration: with the definition's s32 (char *text, u32 width, s32 mode, s32 sel, s32 arg4) (sub_main/menu_windows.c), Talk_YesNoBox 2 of 147 words differ */

/* the yes/no box of message gTalkLine: 1 when closed, 0 while open */
s32 Talk_YesNoBox(s8 flags)
{
    u16 pad;
    char *text;
    s32 yes;

    pad = LobbyPad_Get(0);
    text = Talk_LineText(gTalkLine);
    yes = Talk_LineKind(gTalkLine);
    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    Notice_OpenTextN(text, gTalkTextFrames, yes, gTalkMode, gTalkNpcName);
    if (D_008BDC02 == 0) {
        if (!(flags & 1)) {
            if (pad & 0x20) {
                gTalkTextFrames = 0;
                if (gTalkMode != 1) {
                    if (!(flags & 2)) {
                        LobbySe_Play(0);
                    }
                } else if (!(flags & 2)) {
                    LobbySe_Play(4);
                }
                return 1;
            }
            if (pad & 0x40) {
                if (yes == 1) {
                    if (gTalkMode != 1) {
                        gTalkMode = 1;
                        LobbySe_Play(4);
                    } else {
                        gTalkTextFrames = 0;
                        if (!(flags & 2)) {
                            LobbySe_Play(4);
                        }
                        return 1;
                    }
                } else {
                    gTalkTextFrames = 0;
                    if (!(flags & 2)) {
                        LobbySe_Play(4);
                    }
                    return 1;
                }
                return 0;
            }
            if (yes == 1) {
                if (pad & 0x800) {
                    if (gTalkMode != 0) {
                        LobbySe_Play(1);
                        gTalkMode = 0;
                    }
                } else if ((pad & 0x400) && gTalkMode != 1) {
                    LobbySe_Play(1);
                    gTalkMode = 1;
                }
            }
        }
    } else if (!(flags & 1) && (pad & 0x60)) {
        gTalkTextFrames = -1;
        return 0;
    }
    return 0;
}

s32 Menu_PushNoticeOnce(s32 idx)
{
    if (gNoticeCooldown == 0) {
        Notice_PushItem(gLobbyNoticeTexts[idx]);
        gNoticeCooldown = 0x5A;
        return 1;
    }
    return 0;
}

void Menu_PushNotice(s32 msg)
{
    if (msg < 0x29) {
        Notice_PushItem(gLobbyNoticeTexts[msg]);
        gNoticeCooldown = 0x5A;
    }
}

extern char *gLobbyEventTexts[];
s32 sprintf(char *buf, char *fmt, ...);
s32 Lobby_GetPlace(void);
void Chat_AddReceived(u8 arg0, s8 *arg1);
void ChatLog_Receive(ChatMsg *msg);

void Chat_SystemMsg(s32 msg, char *arg)
{
    ChatMsg m;

    memset(&m, 0, sizeof(m));
    strcpy(m.id, gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
    strcpy(m.name, gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
    if (arg == NULL) {
        strcpy(m.text, gLobbyEventTexts[msg]);
    } else {
        sprintf(m.text, gLobbyEventTexts[msg], arg);
    }
    m.color[1] = 6;
    m.color[0] = 6;
    m.color[2] = 6;
    if (Lobby_GetPlace() != 0) {
        Chat_AddReceived(0xFF, (s8 *)&m);
        return;
    }
    ChatLog_Receive(&m);
}

s32 NetUser_State(s8 i);
s32 memcmp(void *, void *, s32);
s32 Game_IsOnline(void);
extern u8 gNetUsers[];

s32 NetUser_StateByName(u8 *name)
{
    s8 i;
    u8 *p;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    for (i = 0, p = gNetUsers; i < 10; i++, p += 0x11C) {
        if (memcmp(name, p, 8) == 0) {
            return NetUser_State(i);
        }
    }
    return 2;
}

s32 NetUser_State(s8 i)
{
    Player *p;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (*(s8 *)(gNetUsers + i * 0x11C) != 0) {
        p = &gPlayers[i];
        if (p->area == 0) {
            return 1;
        }
        return 0;
    }
    return 2;
}