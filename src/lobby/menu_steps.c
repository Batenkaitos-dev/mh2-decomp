/*
 * The lobby menus' small common steps, each called once a frame by the
 * menus' input steps:
 * - the cursors: Menu_WrapUpDown and Menu_WrapLeftRight (a list's cursor,
 *   wrapping, with their forms that take the pad's buttons or a sound),
 *   Pad_StepWrap (silent), Menu_GridMove (a line on a page and the page),
 *   Menu_CountPages;
 * - the yes/no answers: the lobby menu's box (Menu_YesNoInput,
 *   LobbyMenu_YesNoInput) and a townsperson's talk line with its question
 *   (Talk_YesNoBox);
 * - a window's slide in and its shake (WinSlide, WinShake);
 * - the notices (Menu_PushNotice*) and the chat's system messages
 *   (Chat_SystemMsg).
 * They return the menus' MENU_* values where they run over frames.
 */
#include "common.h"
#include "lobby_menu.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "lobby_text.h"
#include "lobby_npc.h"
#include "pad.h"
#include "area_draw.h"
#include "window.h"
#include "menu_steps.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "talk.h"
char *strcpy(char *, const char *);
void *memset(void *, s32, u32);

extern LobbyMenu *gLobbyMenu;
extern s8 gNoticeCooldown;

/*
 * Moves a list's cursor cur through n entries by pad 0's d-pad (LobbyPad_Get):
 * PAD_UP one back, PAD_DOWN one on, wrapping at both ends, with
 * LOBBY_SE_SCROLL when the list has more than one entry. Returns the new
 * entry.
 */
s32 Menu_WrapUpDown(s32 cur, s32 n)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & PAD_UP) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & PAD_DOWN) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

/* As Menu_WrapUpDown, with the caller's pad buttons. */
s32 Menu_WrapUpDownPad(s32 cur, s32 n, u16 pad)
{
    s32 btn;
    s32 ret;

    btn = pad & 0xFFFF;
    ret = cur;
    if (btn & PAD_UP) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (btn & PAD_DOWN) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

/* As Menu_WrapUpDown, playing the sound `sound` (enum LobbySe). */
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 sound)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & PAD_UP) {
        if (n > 1) {
            LobbySe_Play(sound);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & PAD_DOWN) {
        if (n > 1) {
            LobbySe_Play(sound);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

/* As Menu_WrapUpDown, across: PAD_LEFT one back, PAD_RIGHT one on. */
s32 Menu_WrapLeftRight(s32 cur, s32 n)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & PAD_LEFT) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & PAD_RIGHT) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

/* As Menu_WrapLeftRight, playing the sound `sound` (enum LobbySe). */
s32 Menu_WrapLeftRightSe(s32 cur, s32 n, s32 sound)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & PAD_LEFT) {
        if (n > 1) {
            LobbySe_Play(sound);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & PAD_RIGHT) {
        if (n > 1) {
            LobbySe_Play(sound);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

/* As Menu_WrapLeftRight, with the caller's pad buttons. */
s32 Menu_WrapLeftRightPad(s32 cur, s32 n, u16 pad)
{
    s32 btn;
    s32 ret;

    btn = pad & 0xFFFF;
    ret = cur;
    if (btn & PAD_LEFT) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (btn & PAD_RIGHT) {
        if (n > 1) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

/* Steps `cur` through n entries by the pad's buttons: PAD_LEFT one back,
 * PAD_RIGHT one on, wrapping at both ends. Returns the new entry. */
s32 Pad_StepWrap(s32 cur, s32 n, u16 pad)
{
    s32 btn;
    s32 ret;

    ret = cur;
    btn = pad & 0xFFFF;
    if (btn & PAD_LEFT) {
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (btn & PAD_RIGHT) {
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

/*
 * Moves a cursor through total entries shown perPage to a page, by the pad
 * buttons pad: PAD_UP and PAD_DOWN move the line within the page, wrapping
 * (on the last page at its last entry), PAD_LEFT and PAD_RIGHT turn the
 * page, wrapping; a page too short for the line puts the cursor on its first
 * line. Plays LOBBY_SE_SCROLL when the cursor moved, and stores the line
 * and the page back. Returns the entry (page * perPage + line). perPage
 * comes in as an s32 and is narrowed to the s16 range in place, as the
 * game's code does; n, an s32 copy of total, and the order of the locals
 * are kept for the match (they decide the compiler's registers).
 */
s16 Menu_GridMove(u8 *linep, s16 *pagep, s32 perPage, s16 total, u16 pad)
{
    u8 line;
    s16 page;
    s32 entry;
    s32 cur;
    s32 n;
    s32 oldLine;
    s16 pages;
    s32 oldPage;

    perPage = (s16)perPage; /* the parameter narrowed to the s16 range */
    line = *linep;
    page = *pagep;
    oldPage = page;
    cur = (s16)(line + page * perPage);
    oldLine = line;
    if (!(pad & PAD_DPAD) || total < 2) {
        return cur;
    }
    n = total;
    pages = n / perPage;
    if ((n % perPage) != 0) {
        pages++;
    }
    if (pad & PAD_UP) {
        if (oldLine == 0) {
            if (oldPage < pages - 1 || (n % perPage) == 0) {
                line = perPage - 1;
            } else {
                line = (n % perPage) - 1;
            }
        } else {
            line--;
        }
    } else if (pad & PAD_DOWN) {
        if (oldPage < pages - 1 || (n % perPage) == 0) {
            line++;
            if (line >= perPage) {
                line = 0;
            }
        } else {
            line++;
            if (line >= (n % perPage)) {
                line = 0;
            }
        }
    } else if (pad & PAD_LEFT) {
        page--;
        if (page < 0) {
            page = pages - 1;
        }
    } else if (pad & PAD_RIGHT) {
        page++;
        if (page >= pages) {
            page = 0;
        }
    }
    entry = (s16)(line + page * perPage);
    if (entry >= n) {
        line = 0;
        entry = (s16)(page * perPage);
    }
    if (oldLine == line && oldPage == page) {
        return cur;
    }
    LobbySe_Play(LOBBY_SE_SCROLL);
    *pagep = page;
    *linep = line;
    return entry;
}

/*
 * The lobby menu's yes/no box (gLobbyMenu->yesNo, YES_NO_*), one frame:
 * PAD_LEFT and PAD_RIGHT move between yes and no (LOBBY_SE_SCROLL),
 * PAD_CIRCLE answers, PAD_CROSS moves to no or, on no, answers no.
 * Returns MENU_DONE for yes (LOBBY_SE_CONFIRM_ALT), MENU_CANCEL for no
 * (LOBBY_SE_CANCEL), else MENU_RUNNING. The goto is the game's code: the
 * cross's move to no leaves straight for the end.
 */
s32 Menu_YesNoInput(void)
{
    s32 pad;

    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & PAD_CIRCLE) {
        if (gLobbyMenu->yesNo == YES_NO_YES) {
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_CROSS) {
        if (gLobbyMenu->yesNo != YES_NO_NO) {
            Menu_SetYesNo(YES_NO_NO);
            LobbySe_Play(LOBBY_SE_CANCEL);
            goto running;
        }
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_LEFT) {
        if (gLobbyMenu->yesNo != YES_NO_YES) {
            Menu_SetYesNo(YES_NO_YES);
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if ((pad & PAD_RIGHT) && (gLobbyMenu->yesNo != YES_NO_NO)) {
        Menu_SetYesNo(YES_NO_NO);
        LobbySe_Play(LOBBY_SE_SCROLL);
    }
running:
    return MENU_RUNNING;
}

/* As Menu_YesNoInput, with the sound `sound` (enum LobbySe) for yes. */
s32 LobbyMenu_YesNoInput(s32 sound)
{
    s32 pad;

    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & PAD_CIRCLE) {
        if (gLobbyMenu->yesNo == YES_NO_YES) {
            LobbySe_Play(sound);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_CROSS) {
        if (gLobbyMenu->yesNo != YES_NO_NO) {
            Menu_SetYesNo(YES_NO_NO);
            LobbySe_Play(LOBBY_SE_CANCEL);
            goto running;
        }
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_LEFT) {
        if (gLobbyMenu->yesNo != YES_NO_YES) {
            Menu_SetYesNo(YES_NO_YES);
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if ((pad & PAD_RIGHT) && (gLobbyMenu->yesNo != YES_NO_NO)) {
        Menu_SetYesNo(YES_NO_NO);
        LobbySe_Play(LOBBY_SE_SCROLL);
    }
running:
    return MENU_RUNNING;
}

extern s8 gMenuInputLock;

/*
 * One frame of a window's slide from fromPos to toPos (WinSlide): the
 * start sets the step for 5 frames and locks the menus' input
 * (gMenuInputLock) for 6, the slide overshoots toPos by 6, and the last
 * frame settles on it. Returns MENU_DONE on the frame it settles,
 * MENU_RUNNING before.
 */
s32 WinSlide_Step(WinSlide *slide, s16 fromPos, s16 toPos)
{
    switch (slide->state) {
    case 0: /* start */
        slide->state++;
        slide->pos = fromPos;
        slide->timer = 5;
        slide->step = (toPos - slide->pos) / slide->timer;
        gMenuInputLock = 6;
        break;
    case 1: /* sliding */
        if (--slide->timer < 0) {
            slide->state++;
            if (slide->step >= 0) {
                slide->pos = toPos + 6;
            } else {
                slide->pos = toPos - 6;
            }
        } else {
            slide->pos += slide->step;
        }
        break;
    case 2: /* settle */
        slide->pos = toPos;
        slide->state = 0;
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * One frame of a window's shake (WinShake), 5 frames long with
 * LOBBY_SE_STAMP at its start: the offset (dx, dy) is 2 to the left on the
 * third frame, 1 down on the fourth, then none. Returns MENU_DONE on its
 * last frame, MENU_RUNNING before.
 */
s32 Win_Shake(WinShake *shake)
{
    switch (shake->state) {
    case 0:
        shake->timer = 5;
        shake->state++;
        LobbySe_Play(LOBBY_SE_STAMP);
        break;
    case 1:
        switch (shake->timer) {
        case 3:
            shake->dx = -2;
            shake->dy = 0;
            break;
        case 2:
            shake->dx = 0;
            shake->dy = 1;
            break;
        case 1:
            shake->dx = 0;
            shake->dy = 0;
            break;
        }
        if (--shake->timer <= 0) {
            shake->state = 0;
            shake->dx = 0;
            shake->dy = 0;
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/* The pages that count entries fill at perPage a page (at least 1 when
 * perPage is 0). */
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
extern s8 gTalkBoxHeld;
extern s32 gTalkNpcName;
/* Declared as the caller uses it, with an s32 frame count:
 * unit's own declaration: with the definition's u32 width, Talk_YesNoBox compiles differently. */
s32 Notice_OpenTextN(char *, s32, s32, s32, s32);

/*
 * One frame of a townsperson's talk line gTalkLine in its box
 * (Notice_OpenTextN with the townsperson's name, gTalkNpcName): the text
 * appears over gTalkTextFrames frames, and a question (TALK_LINE_ASK) shows
 * the yes/no cursor gTalkMode (TALK_CHOICE_*), moved by PAD_LEFT and
 * PAD_RIGHT. PAD_CIRCLE closes the line (LOBBY_SE_CONFIRM_ALT on yes,
 * LOBBY_SE_CANCEL on no); PAD_CROSS moves to no, or closes it on no or on
 * a plain line. flags: TALK_BOX_NO_INPUT, the pad is ignored;
 * TALK_BOX_SILENT, closing plays no sound. While gTalkBoxHeld is set, the
 * box doesn't close: a press only shows the whole text (gTalkTextFrames
 * -1). Returns 1 on the frame the line closes, else 0.
 */
s32 Talk_YesNoBox(s8 flags)
{
    u16 pad;
    char *text;
    s32 kind;

    pad = LobbyPad_Get(0);
    text = Talk_LineText(gTalkLine);
    kind = Talk_LineKind(gTalkLine);
    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    Notice_OpenTextN(text, gTalkTextFrames, kind, gTalkMode, gTalkNpcName);
    if (gTalkBoxHeld == 0) {
        if (!(flags & TALK_BOX_NO_INPUT)) {
            if (pad & PAD_CIRCLE) {
                gTalkTextFrames = 0;
                if (gTalkMode != TALK_CHOICE_NO) {
                    if (!(flags & TALK_BOX_SILENT)) {
                        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                    }
                } else if (!(flags & TALK_BOX_SILENT)) {
                    LobbySe_Play(LOBBY_SE_CANCEL);
                }
                return 1;
            }
            if (pad & PAD_CROSS) {
                if (kind == TALK_LINE_ASK) {
                    if (gTalkMode != TALK_CHOICE_NO) {
                        gTalkMode = TALK_CHOICE_NO;
                        LobbySe_Play(LOBBY_SE_CANCEL);
                    } else {
                        gTalkTextFrames = 0;
                        if (!(flags & TALK_BOX_SILENT)) {
                            LobbySe_Play(LOBBY_SE_CANCEL);
                        }
                        return 1;
                    }
                } else {
                    gTalkTextFrames = 0;
                    if (!(flags & TALK_BOX_SILENT)) {
                        LobbySe_Play(LOBBY_SE_CANCEL);
                    }
                    return 1;
                }
                return 0;
            }
            if (kind == TALK_LINE_ASK) {
                if (pad & PAD_LEFT) {
                    if (gTalkMode != TALK_CHOICE_YES) {
                        LobbySe_Play(LOBBY_SE_SCROLL);
                        gTalkMode = TALK_CHOICE_YES;
                    }
                } else if ((pad & PAD_RIGHT) && gTalkMode != TALK_CHOICE_NO) {
                    LobbySe_Play(LOBBY_SE_SCROLL);
                    gTalkMode = TALK_CHOICE_NO;
                }
            }
        }
    } else if (!(flags & TALK_BOX_NO_INPUT) && (pad & (PAD_CIRCLE | PAD_CROSS))) {
        gTalkTextFrames = -1;
        return 0;
    }
    return 0;
}

/* Shows notice idx (enum LobbyNotice) unless one was shown in the last 90
 * frames (gNoticeCooldown); returns 1 when shown. */
s32 Menu_PushNoticeOnce(s32 idx)
{
    if (gNoticeCooldown == 0) {
        Notice_PushItem(gLobbyNoticeTexts[idx]);
        gNoticeCooldown = 90;
        return 1;
    }
    return 0;
}

/* Shows notice msg (enum LobbyNotice) at once, and starts the 90 frames of
 * gNoticeCooldown; an id past the table shows nothing. */
void Menu_PushNotice(s32 msg)
{
    if (msg < LOBBY_NOTICE_COUNT) {
        Notice_PushItem(gLobbyNoticeTexts[msg]);
        gNoticeCooldown = 90;
    }
}

s32 sprintf(char *buf, char *fmt, ...);

/*
 * Posts the system message msg (enum LobbyEventText) to the chat, from the
 * system's name (gMemberCountTexts), with arg printed into it when given,
 * in colour 6. In a level of the server it goes in as a received line
 * (Chat_AddReceived), at its first level straight into the log.
 */
void Chat_SystemMsg(s32 msg, char *arg)
{
    ChatMsg chat;

    memset(&chat, 0, sizeof(chat));
    strcpy(chat.id, gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
    strcpy(chat.name, gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
    if (arg == NULL) {
        strcpy(chat.text, gLobbyEventTexts[msg]);
    } else {
        sprintf(chat.text, gLobbyEventTexts[msg], arg);
    }
    chat.color[1] = 6;
    chat.color[0] = 6;
    chat.color[2] = 6;
    if (Lobby_GetPlace() != LOBBY_PLACE_LEVEL0) {
        Chat_AddReceived(255, (s8 *)&chat);
        return;
    }
    ChatLog_Receive(&chat);
}
