#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_text.h"
#include "lobby_net.h"
#include "lobby_menu.h"

#pragma optimization_level 4
s32 Lobby_HrAtLeast31(void);
int abs(int);

/* a condition of a search (GroupBoard_BuildSearch) */
typedef struct Cond3E {
    /* 0x00 */ u8 kind;
    /* 0x01 */ u8 count;
    /* 0x02 */ u8 ids[0x42];
} Cond3E;
/* the request that LobbyLevel1_Search sends (0x124 bytes) */
typedef struct Req3E {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 n;
    /* 0x12 */ u8 unk12[2];
    /* 0x14 */ Cond3E conds[4];
} Req3E;

/* the lobby's quest list after the 0x20 bytes at +0x6EF4 of the lobby work
   buffer gLobbyWorkBuf (unk_6E82B0.c's Room, with the records starting at
   their names) */
typedef struct Quest3E {
    /* 0x00 */ char name[0x14];
    /* 0x14 */ u8 used;
    /* 0x15 */ u8 kind;
    /* 0x16 */ u8 arg1;
    /* 0x17 */ u8 arg2;
} Quest3E;
typedef struct Lobby3E {
    /* 0x000 */ u8 unk0[0xBBC];
    /* 0xBBC */ Quest3E quests[64];
} Lobby3E;
/* gLobbyWorkBuf read through a cast of its address: read by name,
 * GroupBoard_DrawSearchForm compiles differently. */
#define ROOM3E ((Lobby3E *)((s32)(*(u8 **)&gLobbyWorkBuf + 0x6EF4) + 0x20))

/* the room (quest board entry) chosen in the list (gChosenRoom) */
typedef struct Room3E {
    /* 0x000 */ u8 id[0xC];        /* compared with gCurServerId (the player's own) */
    /* 0x00C */ u8 unkC[4];
    /* 0x010 */ u16 unk10;
    /* 0x012 */ u16 unk12;
    /* 0x014 */ u8 unk14[0x9C - 0x14];
    /* 0x09C */ u8 kind;           /* 1, 2: Lobby_HrAtLeast31 decides who may join */
    /* 0x09D */ u8 unk9D[0x1B4 - 0x9D];
    /* 0x1B4 */ struct {
        s8 used;
        u8 unk1[0x1F];
    } slots[4];
} Room3E;

/* Menu 0x3E's work area (at gMenuWork) */
typedef struct Work3E {
    WinSlide win0;              /* 0x0 */
    u8 unk8[0xB24 - 8];
    s16 unkB24;                 /* 0xB24: the slot cursor */
    s16 unkB26;                 /* 0xB26: slots used */
    s8 unkB28;
    u8 unkB29;
    s8 unkB2A;
    u8 unkB2B;
    s8 list[16];                /* 0xB2C: the rooms shown (AreaBoard_LoadPage) */
    s16 rankCount;              /* 0xB3C */
    s16 rank;                   /* 0xB3E: the search's first condition (0-3, GroupBoard_BuildSearch) */
    s16 rankPage;               /* 0xB40: AreaBoard_LoadPage's page */
    s16 rankPages;              /* 0xB42 */
    Req3E req;                  /* 0xB44: LobbyLevel1_Search's request */
    WinSlide win2;              /* 0xC68 */
    WinSlide win;               /* 0xC70 */
    WinShake shake;             /* 0xC78: Win_Shake */
    s16 unkC80;                 /* 0xC80: GroupBoard_JoinStep's state */
    s8 unkC82;
    s8 unkC83;
    u8 unkC84[2];
    u8 unkC86[2];
    s32 unkC88;
    s16 timer;                  /* 0xC8C: counts down; 0 closes the menu (GroupBoard_RecruitInput) */
    s16 cursor;                 /* 0xC8E */
    s16 row;                    /* 0xC90: the search menu's row (0-2) */
    s16 listCursor;             /* 0xC92 */
    s16 mode;                   /* 0xC94: the first menu's choice (0, 1) */
    s16 page;                   /* 0xC96 */
    s16 pageCount;              /* 0xC98 */
    s16 unkC9A;
    s16 unkC9C;
    s16 listPage;               /* 0xC9E */
    s16 listPages;              /* 0xCA0 */
    s16 listCount;              /* 0xCA2 */
    s16 chosen;                 /* 0xCA4: entries chosen (negated in ids), at most 4 */
    s16 perPage[6];             /* 0xCA6: entries on each page */
    s16 ids[6][11];             /* 0xCB2: the pages' entries */
    s16 unkD36;
    s16 rand;                   /* 0xD38 */
    s8 state;                   /* 0xD3A */
    s8 unkD3B;
    s8 unkD3C;
    s8 unkD3D;
    s8 unkD3E;
    s8 unkD3F;
    s8 unkD40;
    s8 unkD41;
} Work3E;
extern Work3E gMenuWork;

extern s16 gLobbyLevel1Count;
s32 Menu_CountPages(s16 n, s16 per);
u16 Rand_Next(s32 stream);
s32 GroupBoard_QuestAllowed(s16 i);
void GroupBoard_LoadTargets(s16 mode)
{
    Work3E *w = &gMenuWork;
    s16 n = 0;
    s16 k = 0;
    s16 p = 0;
    s16 i;

    if (mode == 1) {
        k = n = 1;
        w->ids[0][0] = 999;
    } else {
        w->pageCount = Menu_CountPages(gLobbyLevel1Count, 10);
        w->rand = Rand_Next(1);
    }
    for (i = 0; i < 64; i++) {
        if (GroupBoard_QuestAllowed(i) == 1) {
            n++;
            if (mode == 1) {
                w->ids[p][k] = i + 1;
            } else {
                w->ids[p][k] = i;
            }
            k++;
            if (k >= 11) {
                w->perPage[p] = k;
                k = 0;
                p++;
            }
        }
    }
    w->perPage[p] = k;
    w->listPages = Menu_CountPages(n, 11);
    w->listCount = n;
    w->chosen = 0;
    w->unkC9C = 2;
    w->rank = 0;
    w->listCursor = 0;
    w->row = 0;
    w->cursor = 0;
    w->listPage = 0;
    w->unkC9A = 0;
    w->page = 0;
}

extern LobbyMenu *gLobbyMenu;
extern LobbyState *gLobby;
u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
void GroupBoard_Close(void);
s32 GroupBoard_Recruit(u16 pad);
s32 GroupBoard_Search(u16 pad);
s32 Win_Shake(WinShake *win);
s32 WinSlide_Step(WinSlide *win, s16 from, s16 to);
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32, s32);
void LobbyText_Open(s32);
void Menu_SetYesNo(u8); /* unit's own declaration: with the definition's void (s8 choice) (lobby/unk_7B0D80.c), GroupBoard_Update 3 of 199 words differ */
void LobbyDialog_OpenWait(void *text);
s32 LobbyStep_DownloadQuests(void);
void LobbyTown_Begin(void);
void Stream_Stop(s32);
void GroupBoard_Update(void)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (w->state) {
    case 0:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            GroupBoard_LoadTargets(w->mode);
            switch (w->mode) {
            case 0:
                w->state++;
                WinSlide_Step(&w->win2, -0x104, 0x36);
                LobbySe_Play(8);
                break;
            case 1:
                w->state = 2;
                WinSlide_Step(&w->win, 0x280, 0x140);
                LobbySe_Play(0x17);
                break;
            }
        } else if (pad & 0x40) {
            GroupBoard_Close();
            LobbySe_Play(4);
        } else {
            w->mode = Menu_WrapUpDown(w->mode, 2);
            Menu_SetYesNo(w->mode);
        }
        break;
    case 1:
        switch (GroupBoard_Recruit(pad)) {
        case 0:
            w->state = 3;
            w->unkD40 = 1;
            Win_Shake(&w->shake);
            break;
        case 1:
        case 3:
            gLobbyMenu->redraw = 1;
            w->state = 0;
            LobbyText_Open(0x8F);
            Menu_SetYesNo(w->mode);
            break;
        }
        break;
    case 2:
        switch (GroupBoard_Search(pad)) {
        case 0:
            w->state++;
            w->unkD40 = 1;
            Win_Shake(&w->shake);
            break;
        case 3:
            w->state = 0;
            gLobbyMenu->redraw = 1;
            LobbyText_Open(0x8F);
            Menu_SetYesNo(w->mode);
            break;
        case 1:
            w->state = 5;
            gLobbyMenu->redraw = 1;
            LobbyDialog_OpenWait(gLobby->errText);
            break;
        }
        break;
    case 3:
        switch (Win_Shake(&w->shake)) {
        case 0:
            w->state++;
            break;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_DownloadQuests()) {
        case 0:
        case 1:
            LobbyTown_Begin();
            Stream_Stop(0);
            Stream_Stop(1);
            break;
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            GroupBoard_Close();
            LobbySe_Play(8);
        }
        break;
    }
}

extern u8 *gGroupSearchResults;
s32 GroupBoard_SearchInput(u16 pad);
s32 GroupBoard_ResultsInput(u16 pad);
struct NetSearch6319;
s32 LobbyLevel1_Search(struct NetSearch6319 *p);
s32 GroupBoard_Search(u16 pad)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;

    switch (w->unkD3B) {
    case 0:
        switch (WinSlide_Step(&w->win, 0x280, 0x140)) {
        case 0:
            w->unkD3B++;
            break;
        }
        break;
    case 1:
        switch (GroupBoard_SearchInput(pad)) {
        case 0:
            w->unkD3B++;
            LobbyText_Open(0x54);
            gLobbyMenu->redraw = 1;
            break;
        case 3:
            w->unkD3B = 0;
            return 3;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel1_Search((struct NetSearch6319 *)&w->req)) {
        case 0:
            if (gGroupSearchResults[4] == 0) {
                w->unkD3B = 4;
                LobbyText_Open(0x55);
                gLobbyMenu->redraw = 1;
            } else {
                w->unkD3B++;
                w->pageCount = Menu_CountPages(gGroupSearchResults[4], 10);
                WinSlide_Step(&w->win2, -0x104, 0x36);
            }
            break;
        case 1:
            w->unkD3B = 0;
            return 1;
        }
        break;
    case 3:
        switch (GroupBoard_ResultsInput(pad)) {
        case 0:
            w->unkD3B = 0;
            return 0;
        case 3:
            w->unkD3B = 0;
            return 3;
        case 1:
            w->unkD3B = 0;
            return 1;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            w->unkD3B = 1;
            LobbySe_Play(8);
        }
        break;
    }
    return 2;
}

extern u8 gCurServerId[];
extern Room3E gChosenRoom;
s16 Menu_GridMove(u8 *colp, s16 *rowp, s32 cols, s16 total, u16 pad);
s32 LobbyLevel1_FetchInfo(u8 id);
void LobbyDialog_Grow(void);
s32 memcmp(void *a, void *b, s32 n);
void HunterInfo_Open(void *src, s32 flags);
s32 HunterInfo_Update(u16 pad);
s32 Menu_YesNoInput(void);
void *memcpy(void *dst, const void *src, u32 n);
void Lobby_Disconnect(s32);
void FadeA_Start(s32);
s32 LobbyLogin_EnterEntry(s32 entry, s32 sub);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 GroupBoard_ResultsInput(u16 pad)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    Room3E *r = &gChosenRoom;
    s32 i;

    switch (w->unkD3C) {
    case 0:
        switch (WinSlide_Step(&w->win2, -0x104, 0x36)) {
        case 0:
            w->unkD3C++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            w->unkD3C = 2;
            gLobbyMenu->redraw = 1;
            LobbyText_Open(0x56);
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            w->unkD3C = 0;
            LobbySe_Play(4);
            return 3;
        } else {
            Menu_GridMove((u8 *) &w->cursor, &w->page, 10, gGroupSearchResults[4], pad);
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel1_FetchInfo(*(u8 *)((s32)gGroupSearchResults + (w->cursor + w->page * 10) * 0x124 + 0x12A))) {
        case 0:
            w->unkD3C++;
            w->unkC9A = 0;
            WinSlide_Step(&w->win, 0x280, 0x140);
            w->unkB24 = 0;
            w->unkB26 = 0;
            for (i = 0; i < 4; i++) {
                if (r->slots[i].used) {
                    w->unkB26++;
                }
            }
            break;
        case 1:
            w->unkD3C = 0;
            return 1;
        }
        break;
    case 3:
        switch (WinSlide_Step(&w->win, 0x280, 0x140)) {
        case 0:
            w->unkD3C++;
            break;
        }
        break;
    case 4:
        w->unkB2A = 0;
        if (pad & 0x20) {
            switch (r->kind) {
            case 1:
                if (Lobby_HrAtLeast31() == 1) {
                    w->unkD3C = 7;
                    gLobbyMenu->redraw = 1;
                    LobbyText_Open(0x88);
                    LobbySe_Play(9);
                    return 2;
                }
                break;
            case 2:
                if (Lobby_HrAtLeast31() == 0) {
                    w->unkD3C = 7;
                    gLobbyMenu->redraw = 1;
                    LobbyText_Open(0x88);
                    LobbySe_Play(9);
                    return 2;
                }
                break;
            }
            w->unkD3C++;
            w->unkB2A = 1;
            if (memcmp(gCurServerId, r, 12) == 0) {
                LobbyText_Open(0x86);
            } else {
                LobbyText_Open(0x87);
            }
            Menu_SetYesNo(1);
            LobbyDialog_Grow();
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            w->unkD3C = 1;
            w->unkB28 = 0;
            LobbySe_Play(4);
        } else if (pad & 0x200) {
            if (w->unkC9A == 0 && w->unkB26 != 0) {
                w->unkD3C = 8;
                w->unkB24 = 0;
                w->unkB28 = 1;
                LobbySe_Play(1);
            }
        } else {
            w->unkC9A = Menu_WrapLeftRight(w->unkC9A, w->unkC9C);
            if (w->unkC9A == 0) {
                w->unkB28 = 0;
            } else {
                w->unkB28 = 1;
            }
        }
        break;
    case 8:
        if (pad & 0x20) {
            w->unkD3C = 9;
            w->unkB28 = 2;
            HunterInfo_Open(&r->slots[w->unkB24], 7);
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            w->unkD3C = 4;
            w->unkB28 = 0;
            LobbySe_Play(4);
        } else if (pad & 0x3000) {
            w->unkB24 = Menu_WrapUpDown(w->unkB24, w->unkB26);
        }
        break;
    case 9:
        switch (HunterInfo_Update(pad)) {
        case 0:
        case 1:
        case 3:
            w->unkD3C = 8;
            w->unkB28 = 1;
            break;
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            if (memcmp(gCurServerId, r, 12) == 0) {
                w->unkD3C++;
                LobbyText_Open(0x78);
            } else {
                gLobby->groupJoin = 1;
                memcpy(gLobby->groupId, r, 0x10);
                gLobby->area = r->unk10;
                gLobby->land = r->unk12;
                Lobby_Disconnect(0);
                FadeA_Start(10);
                Stream_Stop(0);
                Stream_Stop(1);
            }
            break;
        case 3:
            w->unkD3C = 4;
            break;
        }
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        switch (LobbyLogin_EnterEntry(r->unk10, r->unk12)) {
        case 0:
            w->unkD3C = 0;
            w->unkD41 = 1;
            return 0;
        case 1:
            w->unkD3C = 0;
            return 1;
        }
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            w->unkD3C = 4;
            LobbySe_Play(8);
        }
        break;
    }
    return 2;
}

void *memset(void *dst, s32 c, u32 n);
s32 Lobby_HrAtLeast31(void);
void GroupBoard_BuildSearch(void)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s32 k = 0;
    s32 i;
    s32 j;
    s32 m;
    s32 id;

    memset(&w->req, 0, sizeof(w->req));
    w->req.unk10 = 100;
    switch (w->rank) {
    case 0:
        w->req.n++;
        k++;
        w->req.conds[0].kind = 0;
        w->req.conds[0].count = 1;
        w->req.conds[0].ids[0] = 0xFF;
        break;
    case 1:
        w->req.n++;
        k++;
        w->req.conds[0].kind = 0;
        w->req.conds[0].count = 1;
        w->req.conds[0].ids[0] = 0;
        break;
    case 2:
        w->req.n++;
        w->req.conds[0].kind = 0;
        w->req.conds[0].count = 1;
        if (Lobby_HrAtLeast31() == 1) {
            w->req.conds[0].ids[0] = 2;
        } else {
            w->req.conds[0].ids[0] = 1;
        }
        k++;
        break;
    case 3:
        w->req.n++;
        w->req.conds[0].kind = 0;
        w->req.conds[0].count = 2;
        w->req.conds[0].ids[0] = 0;
        if (Lobby_HrAtLeast31() == 1) {
            w->req.conds[0].ids[1] = 2;
        } else {
            w->req.conds[0].ids[1] = 1;
        }
        k++;
        break;
    }
    if (w->chosen == 0) {
        w->req.n++;
        w->req.conds[k].kind = 1;
        w->req.conds[k].count++;
        w->req.conds[k].ids[0] = 0xFF;
    } else {
        w->req.n++;
        w->req.conds[k].kind = 1;
        m = 0;
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 11; j++) {
                id = w->ids[i][j];
                if (id < 0) {
                    w->req.conds[k].count++;
                    w->req.conds[k].ids[m++] = -1 - id;
                }
            }
        }
    }
}

s32 abs(s32 x);
void GroupBoard_BuildSearch(void);
s32 GroupBoard_SearchInput(u16 pad)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s16 i;
    s16 j;
    s16 *p;

    switch (w->unkD3C) {
    case 0:
        if (pad & 0x20) {
            if (w->row == 1) {
                w->unkD3C++;
                LobbySe_Play(8);
            } else if (w->row == 2) {
                GroupBoard_BuildSearch();
                LobbySe_Play(8);
                return 0;
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else if (pad & 0xC00) {
            if (w->row == 0) {
                w->rank = Menu_WrapLeftRight(w->rank, 4);
            }
        } else {
            w->row = Menu_WrapUpDown(w->row, 3);
        }
        break;
    case 1:
        if (pad & 0x40) {
            w->unkD3C = 0;
            LobbySe_Play(4);
        } else if (pad & 0x20) {
            p = (s16 *)w->ids + w->listPage * 11 + w->listCursor;
            if (*p != 999) {
                if (*p >= 0 && w->chosen >= 4) {
                    LobbySe_Play(9);
                } else {
                    *p *= -1;
                    if (w->ids[w->listPage][w->listCursor] < 0) {
                        w->chosen++;
                    } else {
                        w->chosen--;
                    }
                    LobbySe_Play(8);
                }
            } else {
                for (i = 0; i < 6; i++) {
                    for (j = 0; j < 11; j++) {
                        w->ids[i][j] = abs(w->ids[i][j]);
                    }
                }
                w->chosen = 0;
                LobbySe_Play(8);
            }
        } else if (pad & 0xC00) {
            if (w->listPages > 1) {
                w->listPage = Menu_WrapLeftRight(w->listPage, w->listPages);
                w->listCursor = 0;
            }
        } else {
            w->listCursor = Menu_WrapUpDown(w->listCursor, w->perPage[w->listPage]);
        }
        break;
    }
    return 2;
}

extern s8 gLobbyLevel1Sel;
s32 GroupBoard_RecruitFormInput(u16 pad);
s32 GroupBoard_RecruitSetup(u16 pad);
s32 AreaBoard_EnterStep(u16 pad);
struct RoomMenu;
s32 AreaBoard_Join(u16 pad, struct RoomMenu *w, s32 slot);
s32 GroupBoard_Recruit(u16 pad)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;

    switch (w->unkD3B) {
    case 0:
        switch (WinSlide_Step(&w->win2, -0x104, 0x36)) {
        case 0:
            w->unkD3B++;
            break;
        }
        break;
    case 1:
        switch (GroupBoard_RecruitFormInput(pad)) {
        case 0:
            switch (w->unkD3E) {
            case 1:
                w->unkD3B = 2;
                w->row = 0;
                w->unkD3F = 1;
                break;
            case 3:
            case 4:
                w->unkD3B = 4;
                w->unkD3F = 0;
                LobbyText_Open(0x56);
                WinSlide_Step(&w->win0, 0x280, 0x140);
                break;
            }
            break;
        case 3:
            w->unkD3B = 0;
            return 3;
        }
        break;
    case 2:
        switch (GroupBoard_RecruitSetup(pad)) {
        case 0:
            w->unkD3B = 0;
            return 0;
        case 3:
            w->unkD3B = 3;
            LobbyText_Open(0x77);
            break;
        case 1:
            w->unkD3B = 5;
            break;
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        switch (AreaBoard_EnterStep(pad)) {
        case 0:
            w->unkD3B = 1;
            gLobbyLevel1Sel = 0;
            break;
        case 1:
            w->unkD3B = 5;
            break;
        }
        break;
    case 4:
        switch (AreaBoard_Join(pad, (struct RoomMenu *)w, w->unkC88)) {
        case 0:
            w->unkD3B = 0;
            return 0;
        case 3:
            w->unkD3B = 1;
            break;
        case 1:
            w->unkD3B = 5;
            break;
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            w->unkD3B = 1;
            LobbySe_Play(8);
        }
        break;
    }
    return 2;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6E9310", D_00838138);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6E9310", D_00838140);

extern u16 gRecruitFrames;
s32 LobbyStep_6310(u16 id); /* unit's own declaration: with the definition's s32 (s32 id) (lobby/unk_7B3140.c), GroupBoard_RecruitSetup 1 of 118 words differ */
void AreaBoard_LoadPage(s16 page);
struct RoomJoin;
s16 GroupBoard_JoinStep(struct RoomJoin *p);
s32 GroupBoard_RecruitInput(u16 pad);
s32 GroupBoard_RecruitSetup(u16 pad)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;

    switch (w->unkD3C) {
    case 0:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_6310(w->unkC88)) {
        case 0:
            w->unkD3C++;
            w->timer = gRecruitFrames;
            w->row = 0;
            w->listCursor = 0;
            w->unkC80 = 0;
            AreaBoard_LoadPage(0);
            memset(&w->unkC82, 0, 4);
            WinSlide_Step(&w->win, 0x280, 0x140);
            break;
        case 1:
            w->unkD3C = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 1;
        }
        break;
    case 1:
        switch (WinSlide_Step(&w->win, 0x280, 0x140)) {
        case 0:
            w->unkD3C++;
            break;
        }
        break;
    case 2:
        switch (GroupBoard_RecruitInput(pad)) {
        case 0:
            w->unkD3C++;
            LobbyText_Open(0x78);
            break;
        case 3:
            w->unkD3C = 0;
            return 3;
        }
        break;
    case 3:
        switch (GroupBoard_JoinStep((struct RoomJoin *)&w->unkC80)) {
        case 0:
            w->unkD3C = 0;
            return 0;
        case 1:
            w->unkD3C = 0;
            return 1;
        }
        break;
    }
    return 2;
}

s32 GroupBoard_RecruitInput(u16 pad)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;

    if (--w->timer <= 0) {
        w->unkD3D = 0;
        LobbyText_Open(0x77);
        LobbySe_Play(4);
        return 3;
    }
    switch (w->unkD3D) {
    case 0:
        if (pad & 0x20) {
            switch (w->row) {
            case 1:
                w->unkD3D = 1;
                LobbySe_Play(8);
                break;
            case 2:
                w->unkD3D = 2;
                LobbySe_Play(8);
                break;
            case 3:
                LobbySe_Play(8);
                return 0;
            }
        } else if (pad & 0x40) {
            w->unkC82 = 0;
            w->unkC83 = 0;
            w->listPage = 0;
            w->listCursor = 0;
            w->rank = 0;
            LobbySe_Play(4);
            return 3;
        } else if (pad & 0xC00) {
            if (w->row == 0) {
                if (Lobby_HrAtLeast31() == 1) {
                    w->unkC82 ^= 2;
                } else {
                    w->unkC82 ^= 1;
                }
                LobbySe_Play(1);
            }
        } else {
            w->row = Menu_WrapUpDown(w->row, 4);
        }
        break;
    case 1:
        if (pad & 0x20) {
            w->unkD3D = 0;
            w->unkC83 = w->ids[w->listPage][w->listCursor];
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            w->unkD3D = 0;
            LobbySe_Play(4);
        } else if (pad & 0xC00) {
            if (w->listPages > 1) {
                w->listPage = Menu_WrapLeftRight(w->listPage, w->listPages);
                w->listCursor = 0;
            }
        } else {
            w->listCursor = Menu_WrapUpDown(w->listCursor, w->perPage[w->listPage]);
        }
        break;
    case 2:
        if (pad & 0x20) {
            w->unkD3D = 0;
            w->unkC80 = w->rank;
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            w->unkD3D = 0;
            LobbySe_Play(4);
        } else if (pad & 0xC00) {
            if (w->rankPages > 1) {
                w->rankPage = Menu_WrapLeftRight(w->rankPage, w->rankPages);
                AreaBoard_LoadPage(w->rankPage);
            }
        } else {
            w->rank = Menu_WrapUpDown(w->rank, w->rankCount);
        }
        break;
    }
    return 2;
}

extern u8 D_5C4346[];
s32 Menu_WrapLeftRight(s32, s32);
s32 GroupBoard_RecruitFormInput(u16 pad)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s32 idx;
    u8 st;

    if (pad & 0x20) {
        idx = w->cursor + w->page * 10;
        st = D_5C4346[idx * 0x164];
        if (st == 1) {
            w->unkD3E = 1;
            w->unkC88 = idx + 1;
            LobbyText_Open(0x4B);
            LobbySe_Play(0x17);
            return 0;
        } else if (st == 3 || st == 4) {
            w->unkD3E = 3;
            w->unkC88 = idx + 1;
            LobbySe_Play(0x17);
            return 0;
        } else {
            LobbySe_Play(9);
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0xC00) {
        w->page = Menu_WrapLeftRight(w->page, w->pageCount);
    } else {
        w->cursor = Menu_WrapUpDown(w->cursor, 10);
    }
    return 2;
}

void Font_SetSpacing(u8 a, u8 b);
void Text_SetSize(s32);
extern LevelEntry gLobbyLevel1Entries[];
extern char D_00838138[];
extern char D_00838140[];
extern char *gGroupLimitTexts[];
extern char *gGroupBoardTitleTexts[];
extern char *gGroupJoinPromptTexts[];
extern char *gLongDashTexts[];
extern char *D_0080C494;
extern char *gLimitedTexts[];
extern char *D_0080C4BC;
extern char *gRecruitMsgTexts[];
extern char *D_0080C49C;
extern char *gInTownTexts[];
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *title, u8 a, u8 b);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), GroupBoard_DrawRooms 249 of 298 words, GroupBoard_DrawResults 256 of 304 words, GroupBoard_DrawQuestPicker 119 of 136 words differ */
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Text_SetColor(s16 color);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void GroupBoard_DrawRooms(s16 x, s16 y)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    LevelEntry *g;
    u32 color;
    s32 i;
    s32 k;
    s16 ty = y;
    s8 f;

    if (w->unkD3B == 1) {
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(0x14, 0x12, 0xF0, 0xFFB0B000, gGroupBoardTitleTexts[0], 2, 1);
        Menu_DrawPageNum(x + 0x200, 0x16, w->page, w->pageCount, 1);
    }
    Win_Draw(x, y, 0x264, 0xFA, 0xBDB59F6C, 0xB02040F0, 7);
    Font_SetColor(0x807F7F28);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x + 0x14, ty += 2);
    Text_PrintfColored(D_0080C494);
    Font_SetPos(x + 0x3C, ty);
    Text_PrintfColored(gLimitedTexts[0]);
    Font_SetPos(x + 0x78, ty);
    Text_PrintfColored(D_0080C4BC);
    Font_SetPos(x + 0x13A, ty);
    Text_PrintfColored(gRecruitMsgTexts[0]);
    Font_SetPos(x + 0x1FE, ty);
    Text_PrintfColored(D_0080C49C);
    Font_SetPos(x + 0x23A, ty);
    Text_PrintfColored(gInTownTexts[0]);
    for (i = 0; i < 10; i++) {
        ty += 0x16;
        k = i + w->page * 10;
        if ((u16)gLobbyLevel1Count > k) {
            g = &gLobbyLevel1Entries[k];
            if (g->users == 0 || (g->state != 3 && g->state != 4)) {
                color = 0x80404040;
            } else {
                color = 0x807F7F7F;
            }
            Font_SetColor(color);
            Font_SetPos(x + 0x14, ty);
            Text_PrintfColored(D_00838138, k + 1);
            Font_SetPos(x + 0x3C, ty);
            Text_PrintfColored(gGroupLimitTexts[g->groupInfo[0] + 3]);
            Font_SetPos(x + 0x78, ty);
            Text_PrintfColored(ROOM3E->quests[g->groupInfo[1]].name);
            if (color == 0x807F7F7F) {
                Text_SetColor(g->color);
            }
            Font_SetPos(x + 0x13A, ty);
            Text_PrintfColored(D_00838140, g->text);
            Font_SetColor(color);
            Font_SetPos(x + 0x1F8, ty);
            Text_PrintfColored("%2d/%2d", g->users, g->maxUsers);
            Font_SetPos(x + 0x23A, ty);
            Text_PrintfColored(D_00838138, g->users - g->questUsers);
        } else {
            Font_SetColor(0x80404040);
            Font_SetPos(x + 0xA, ty);
            Text_PrintfColored(gLongDashTexts[0]);
        }
        if (i == w->cursor && w->state == 1) {
            switch (w->unkD3B) {
            case 0:
            case 1:
                f = 0;
                break;
            default:
                f = 1;
                break;
            }
            Menu_DrawLine(x, ty - 1, 0x264, 0x14, f);
        }
    }
    Win_DrawText(0x12C, 0x154, 0x140, 0x50, 0xBDB59F6C, 0xC0101020, gGroupJoinPromptTexts[0]);
}

/* a group of the search results (gGroupSearchResults + 8, 0x124 bytes each) */
typedef struct GroupEnt3E {
    /* 0x000 */ u8 id[8]; /* the host */
    /* 0x008 */ u8 unk8[0xFC];
    /* 0x104 */ char srv[0x10]; /* LobbySrv_Name */
    /* 0x114 */ u8 kind; /* gGroupLimitTexts[kind + 3] */
    /* 0x115 */ u8 target; /* the lobby's quest records */
    /* 0x116 */ u8 unk116[6];
    /* 0x11C */ u16 members;
    /* 0x11E */ u16 maxMembers;
    /* 0x120 */ u16 unk120;
    /* 0x122 */ u8 unk122[2];
} GroupEnt3E;
typedef struct GroupList3E {
    /* 0x0 */ u8 unk0[4];
    /* 0x4 */ u8 count;
    /* 0x5 */ u8 unk5[3];
    /* 0x8 */ GroupEnt3E ents[1];
} GroupList3E;
extern char *D_0080C4FC;
extern char *D_0080C068;
extern char *D_0080C05C;
char *LobbySrv_Name(char *id);
void GroupBoard_DrawResults(s16 x, s16 y)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    GroupEnt3E *g;
    s32 color;
    s32 i;
    s32 k;
    s16 ty = y;
    s8 f;
    char buf[0x20];

    if (w->unkD3C != 0) {
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(0x14, 0x12, 0xF0, 0xFFB0B000, D_0080C4FC, 2, 1);
        Menu_DrawPageNum(x + 0x200, 0x16, w->page, w->pageCount, 1);
    }
    Win_Draw(x, y, 0x264, 0xFA, 0xBDB59F6C, 0xB02040F0, 7);
    Font_SetColor(0x807F7F28);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x + 0x14, ty += 2);
    Text_PrintfColored(D_0080C494);
    Font_SetPos(x + 0x32, ty);
    Text_PrintfColored(D_0080C068);
    Font_SetPos(x + 0x9C, ty);
    Text_PrintfColored(gLimitedTexts[0]);
    Font_SetPos(x + 0xD2, ty);
    Text_PrintfColored(D_0080C4BC);
    Font_SetPos(x + 0x194, ty);
    Text_PrintfColored(gRecruitMsgTexts[0]);
    Font_SetPos(x + 0x204, ty);
    Text_PrintfColored(D_0080C49C);
    Font_SetPos(x + 0x23A, ty);
    Text_PrintfColored(gInTownTexts[0]);
    for (i = 0; i < 10; i++) {
        ty += 0x16;
        k = i + w->page * 10;
        g = &((GroupList3E *)gGroupSearchResults)->ents[k];
        if (((GroupList3E *)gGroupSearchResults)->count > k) {
            color = g->members == 0 ? 0x80404040 : 0x807F7F7F;
            Font_SetColor(color);
            Font_SetPos(x + 0x14, ty);
            Text_PrintfColored(D_00838138, k + 1);
            Font_SetPos(x + 0x32, ty);
            Text_PrintfColored(D_00838140, LobbySrv_Name(g->srv));
            Font_SetPos(x + 0x9C, ty);
            Text_PrintfColored(gGroupLimitTexts[g->kind + 3]);
            Font_SetPos(x + 0xD2, ty);
            Text_PrintfColored(ROOM3E->quests[g->target].name);
            Font_SetPos(x + 0x194, ty);
            memset(buf, 0, sizeof(buf));
            memcpy(buf, g->id, 8);
            Text_PrintfColored("%s%s", buf, D_0080C05C);
            Font_SetColor(color);
            Font_SetPos(x + 0x208, ty);
            Text_PrintfColored("%d/%d", g->members, g->maxMembers);
            Font_SetPos(x + 0x23A, ty);
            Text_PrintfColored(D_00838138, g->unk120);
        }
        if (i == w->cursor) {
            if (w->unkD3B == 3) {
                switch (w->unkD3C) {
                case 0:
                case 1:
                    f = 0;
                    break;
                default:
                    f = 1;
                    break;
                }
            } else {
                f = 1;
            }
            Menu_DrawLine(x, ty - 1, 0x264, 0x14, f);
        }
    }
    Win_DrawText(0x12C, 0x154, 0x140, 0x50, 0xBDB59F6C, 0xC0101020, gGroupJoinPromptTexts[0]);
}

extern u8 gLobbyLevel0Sel;
extern u8 D_5C8086[];
extern char *D_0080C4C4;
extern char *gLimitCondTexts[];
extern char *D_0080C4EC;
extern char *gRecruitMsgLabelTexts[];
extern char *D_0080C4CC;
extern char *gTimeLeftSecTexts[];
u32 LevelList_SlotColor(u8 slot);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 k);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 a, s16 b);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *s, s32 color);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *s, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 col);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);
void Text_Printf(const char *fmt, ...);
void GroupBoard_DrawRecruitForm(s16 x, s16 y)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s16 ty = y;
    s16 cx;
    char *s;
    s32 bx;
    s16 tx;
    u32 col;
    s32 ly;
    LevelEntry *r;
    s32 lx;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x140, 0x12, 0xC8, 0xFFB0B000, D_0080C4C4, 2, 1);
    Font_SetSpacing(0x12, 0x12);
    Icon_Draw(x, y, 0x12C, 0x122, 0xFFE0D8C8, 1);
    col = LevelList_SlotColor(D_5C8086[gLobbyLevel0Sel * 0x164]);
    ly = ty;
    LobbyUi_DrawIcon(x + 0x96, ly + 0x91, 0x70, col, 3);
    Font_SetColor(0x80101010);
    cx = x + 0x96;
    r = &gLobbyLevel1Entries[w->unkC88 - 1];
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(r->name, cx, 0x12), ty += 0x14);
    Text_PrintfColored(r->name);
    Font_SetPos(Text_CenterX(D_0080C4C4, cx, 0x12), ty += 0x16);
    lx = x + 0x1E;
    Font_SetPos(lx, ty += 0x28);
    Text_PrintfColored(gLimitCondTexts[0]);
    Font_SetColor(0x80101010);
    Font_SetPos(x + 0x9C, ty);
    s = gGroupLimitTexts[w->unkC82];
    Text_PrintfColored(s);
    if (w->row == 0 && w->unkD3D == 0) {
        Menu_DrawArrows(x + 0x9C, ty, 0x12, s, 0xFF106060);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(lx, ty += 0x1E);
    Text_PrintfColored(D_0080C4EC);
    Font_SetColor(0x80101010);
    s = ROOM3E->quests[w->unkC83].name;
    Font_SetPos(Text_CenterX(s, cx, 0x12), ty + 0x18);
    Text_PrintfColored(s);
    if (w->row == 1) {
        Gfx_DrawRectFrame(bx = x + 0x32, ty + 0x16, 0xD8, 0x16, 0xFF106060);
        if (w->unkD3D == 0) {
            Menu_DrawSideArrows(bx, ty + 0x16, 0xD8, 0x16, 0, 0xFF106060);
        }
    } else {
        Gfx_DrawRectFrame(bx = x + 0x32, ty + 0x16, 0xD8, 0x16, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(lx, ty += 0x32);
    Text_PrintfColored(gRecruitMsgLabelTexts[0]);
    Font_SetColor(0x80101010);
    s = (char *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20) + w->list[w->unkC80] * 0x20 + 8;
    Font_SetPos(Text_CenterX(s, cx, 0x12), ty + 0x18);
    Text_PrintfColored(s);
    if (w->row == 2) {
        Gfx_DrawRectFrame(bx, ty + 0x16, 0xD8, 0x16, 0xFF106060);
        if (w->unkD3D == 0) {
            Menu_DrawSideArrows(bx, ty + 0x16, 0xD8, 0x16, 0, 0xFF106060);
        }
    } else {
        Gfx_DrawRectFrame(bx, ty + 0x16, 0xD8, 0x16, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    tx = Text_CenterX(D_0080C4CC, cx, 0x12);
    Font_SetPos(tx, ty += 0x3C);
    Text_PrintfColored(D_0080C4CC);
    if (w->row == 3 && w->unkD3D == 0 && w->unkD3C == 2) {
        Menu_DrawArrowsRev(tx, ty - 2, 0x12, D_0080C4CC, 0xFF106060);
    }
    Font_SetPos(x + 0x9C, ly + 0x100);
    Text_Printf(gTimeLeftSecTexts[0], w->timer / 30);
    if (w->unkD40) {
        Menu_DrawStamp(x, y, 0x12C, 0x122, (u16)w->rand, w->shake.timer);
    }
}

void GroupBoard_DrawQuestPicker(s16 x, s16 y)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s16 ty = y;
    s16 i;
    s16 id;

    Win_Draw(x, y, 0x118, 0x122, 0xBDB59F6C, 0xC0101010, 2);
    ty += 10;
    Menu_DrawPageNum((s16)(x + 0x28) + 0xA0, ty, w->listPage, w->listPages, 1);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x28, ty);
    Text_PrintfColored(D_0080C4BC);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < w->perPage[w->listPage]; i++) {
        id = w->ids[w->listPage][i];
        if (ROOM3E->quests[id].used) {
            Font_SetPos(x + 0x28, ty += 0x16);
            Text_PrintfColored(D_00838140, ROOM3E->quests[id].name);
            if (w->listCursor == i) {
                Menu_DrawLine(x + 10, ty - 1, 0x104, 0x14, 0);
            }
        }
    }
}

extern char *D_0080C4F4;
extern char *gLimitCondTexts[];
extern char *D_0080C4EC;
extern char *gSearchStartTexts[];
extern char *gGroupSearchHelpTexts[];
extern char *gGroupLimitNewTexts[];
extern char *gGroupLimitExpTexts[];
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *s, s32 color);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *s, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 col);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);

void GroupBoard_DrawSearchForm(s16 x, s16 y)
{
    char *s;
    s16 ty = y;
    s16 cx;
    s16 tx;
    s16 i;
    s16 j;
    s16 id;
    s32 ly;
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s16 *p;
    char *name;

    Font_SetSpacing(0x12, 0x12);
    Icon_Draw(x, y, 0x12C, 0x122, 0xFFE0D8C8, 1);
    Font_SetColor(0x80101010);
    cx = x + 0x96;
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(D_0080C4F4, cx, 0x12), ty += 0x16);
    Text_PrintfColored(D_0080C4F4);
    Font_SetPos(x + 0x1E, ty += 0x28);
    Text_PrintfColored(gLimitCondTexts[0]);
    if (Lobby_HrAtLeast31() == 1) {
        s = gGroupLimitExpTexts[w->rank];
    } else {
        s = gGroupLimitNewTexts[w->rank];
    }
    Font_SetColor(0x80101010);
    tx = Text_CenterX(s, cx, 0x12);
    Font_SetPos(tx, ty += 0x1A);
    Text_PrintfColored(s);
    if (w->row == 0 && w->unkD3C == 0) {
        Menu_DrawArrows(tx, ty, 0x12, s, 0xFF106060);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(x + 0x1E, ty += 0x1E);
    Text_PrintfColored(D_0080C4EC);
    Font_SetColor(0x80101010);
    if (w->row == 1) {
        Gfx_DrawRectFrame(x + 0x32, ty + 0x16, 0xD8, 0x57, 0xFF106060);
        if (w->unkD3C == 0) {
            Menu_DrawSideArrows(x + 0x32, ty + 0x32, 0xD8, 0x16, 0, 0xFF106060);
        }
    } else {
        Gfx_DrawRectFrame(x + 0x32, ty + 0x16, 0xD8, 0x57, 0xFF606060);
    }
    ty += 3;
    ly = ty;
    for (i = 0; i < 6; i++) {
        for (j = 0, p = (s16 *)((u8 *)w + i * 22); j < 11; j++, p++) {
            if (p[0xCB2 / 2] < 0) {
                id = -p[0xCB2 / 2];
                name = ROOM3E->quests[(s16)(id - 1)].name;
                ly += 0x16;
                Font_SetPos(Text_CenterX(name, cx, 0x12), ly);
                Text_PrintfColored(name);
            }
        }
    }
    Font_SetColor(0x80301010);
    tx = Text_CenterX(gSearchStartTexts[0], cx, 0x12);
    Font_SetPos(tx, ty += 0x78);
    Text_PrintfColored(gSearchStartTexts[0]);
    if (w->row == 2) {
        Menu_DrawArrowsRev(tx, ty - 2, 0x12, gSearchStartTexts[0], 0xFF106060);
    }
    Win_DrawText(0x12C, 0x154, 0x140, 0x50, 0xBDB59F6C, 0xC0101020, gGroupSearchHelpTexts[0]);
}


void GroupBoard_DrawQuestList(s16 x, s16 y)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s16 ty = y;
    s16 i;
    s16 id;

    Win_Draw(x, y, 0x118, 0x122, 0xBDB59F6C, 0xC0101010, 2);
    ty += 10;
    Font_SetColor(0x807F7F7F);
    Menu_DrawPageNum((s16)(x + 0x28) + 0xA0, ty, w->listPage, w->listPages, 1);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x28, ty);
    Text_PrintfColored(D_0080C4BC);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < w->perPage[w->listPage]; i++) {
        id = w->ids[w->listPage][i];
        Font_SetPos(x + 0x28, ty += 0x16);
        if (w->listCursor == i) {
            Menu_DrawLine(x + 10, ty - 1, 0x104, 0x14, 0);
        }
        if (id == 999) {
            Text_PrintfColored(D_00838140, gGroupLimitNewTexts[0]);
        } else {
            if (id < 0) {
                Icon_Draw(x + 0x12, ty, 0x14, 0x14, 0xFFFF0000, 0x80);
                id *= -1;
            }
            Text_PrintfColored(D_00838140, ROOM3E->quests[(s16)(id - 1)].name);
        }
    }
}

extern char *D_0080C4A4;
extern char *gHrLabelTexts[];
extern char *D_0080C4B4;
extern char *gLocationLabelTexts[];
s32 LobbySrv_Value(char *id);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2);
void Icon_DrawKind48(s16, s16, s32, s32, u8, s32); /* unit's own declaration: with the definition's void (s32 a, s32 b, s32 c, s32 d, s16 e, s8 f) (lobby/unk_78F070.c), GroupBoard_DrawMemberList 3 of 310 words differ */
s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
void GroupBoard_DrawMemberMenu(void *w, s16 x, s16 y, u32 color);
void GroupBoard_DrawMemberList(s16 x, s16 y)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    u32 col;
    char buf[0x20];
    u8 *p = (u8 *)&gChosenRoom;
    u8 *s;
    s16 ty = y;
    char wbuf[0x20];
    s16 i;

    Font_SetSpacing(0x12, 0x12);
    Icon_Draw(x, y, 0x12C, 0x122, 0xFFE0D8C8, 1);
    col = LevelList_SlotColor(D_5C8086[gLobbyLevel0Sel * 0x164]);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xD2, y + 0x14, w->unkC9A, w->unkC9C, 1);
    Font_SetColor(0x80301010);
    Font_SetPos(x + 0x28, y + 0x14);
    Text_PrintfColored((char *)&gChosenRoom + 0x58);
    ty += 0x2A;
    if (w->unkC9A == 0) {
        Font_SetColor(0x80301010);
        Font_SetPos(x + 0x28, ty);
        Text_PrintfColored(D_0080C4A4);
        Font_SetPos(x + 0xBE, ty);
        Text_PrintfColored(gHrLabelTexts[0]);
        Font_SetPos(x + 0xF0, ty);
        Text_PrintfColored(D_0080C4B4);
        Font_SetColor(0x80101010);
        for (i = 0; i < 4; i++, p += 0x20) {
            ty += 0x16;
            s = p + 0x1B4;
            if (*(s8 *)s != 0) {
                if (w->unkB28 == 1 && i == w->unkB24) {
                    Menu_DrawCursorRow(x + 0x28, ty, 0xBE, 0x14, 0xC0106060, 0xFF106060);
                }
                Font_SetPos(x + 0x28, ty);
                Text_PrintfColored(D_00838140, s + 8);
                Font_SetPos(x + 0xAC, ty);
                sprintf(buf, "%3d", *(u16 *)(s + 0x1C));
                Sjis_Widen(buf, wbuf);
                Text_PrintfColored(wbuf);
                Icon_DrawKind48(x + 0xF0, ty, 0x18, -1, s[0x1E], 0);
            }
        }
    } else {
        Font_SetColor(0x80301010);
        Font_SetPos(x + 0x28, ty);
        Text_PrintfColored(gLocationLabelTexts[0]);
        Font_SetColor(0x80101010);
        Font_SetPos(x + 0x64, ty += 0x16);
        Text_PrintfColored(D_00838140, LobbySrv_Name((char *)p));
        Icon_Draw(x + 0x46, ty - 2, 0x16, 0x16, LobbySrv_Value((char *)p) | 0xB0000000, 0x75);
        Font_SetPos(x + 0x64, ty += 0x16);
        Text_PrintfColored(D_00838140, p + 0x14);
        Icon_Draw(x + 0x46, ty - 2, 0x16, 0x16, col, 0x76);
        Font_SetPos(x + 0x64, ty += 0x16);
        Text_PrintfColored(D_00838140, p + 0x58);
        Icon_Draw(x + 0x46, ty - 2, 0x16, 0x16, col, 0x77);
        Icon_Draw(x + 0x5A, ty + 0xF, strlen((char *)p + 0x58) * 9 + 0x14, 8, 0xA0FF0000, 0x6F);
    }
    if (w->unkD40) {
        Menu_DrawStamp(x, y, 0x12C, 0x122, w->rand, w->shake.timer);
    }
    GroupBoard_DrawMemberMenu(w, x + 0x28, y + 0xDC, 0x80101010);
}

extern char *D_0080C4D4;
extern char *gCancelLabelTexts[];
s16 Lobby_DrawSprite155(s16 x, s16 y, s32 i);
/* The three buttons of the member menu (the pad bits lower the pressed one). */
void GroupBoard_DrawMemberMenu(void *w, s16 x, s16 y, u32 color)
{
    Work3E *m = (Work3E *)w;
    u16 pad = LobbyPad_Get(0);
    s16 d;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(color);
    d = (pad & 0x200) != 0;
    if (m->unkB28 == 0) {
        Lobby_DrawSprite155(x, y + d, 3);
        Font_SetPos(x + 0x1A, y + 2);
        Text_PrintfColored(D_0080C4D4);
    }
    d = (pad & 0x20) != 0;
    if (m->unkB2A != 0 || gLobbyMenu->redraw != 0) {
        d = 0;
    }
    y += 0x1A;
    Lobby_DrawSprite155(x, y + d, 0);
    Font_SetPos(x + 0x1A, y + 2);
    Text_PrintfColored(D_0080C4CC);
    d = (pad & 0x40) != 0;
    if (m->unkB2A != 0 || gLobbyMenu->redraw != 0) {
        d = 0;
    }
    x += 0x78;
    Lobby_DrawSprite155(x, y + d, 1);
    Font_SetPos(x + 0x1A, y + 2);
    Text_PrintfColored(gCancelLabelTexts[0]);
}

void GroupBoard_DrawRooms(s16 x, s16 y);
void GroupBoard_DrawResults(s16 x, s16 y);
void GroupBoard_DrawSearchForm(s16 x, s16 y);
void GroupBoard_DrawQuestList(s16 x, s16 y);
void GroupBoard_Draw(u8 *o)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s16 id;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    switch (w->state) {
    case 1:
    case 3:
    case 4:
        id = w->win2.pos;
        if (w->unkD41 == 0) {
            GroupBoard_DrawRooms(0xE, id);
        } else {
            GroupBoard_DrawResults(0xE, id);
        }
        break;
    case 2:
        if (w->unkD3B == 3) {
            GroupBoard_DrawResults(0xE, w->win2.pos);
            break;
        }
        GroupBoard_DrawSearchForm(w->win.pos, 0x32);
        if (w->unkD3B == 1 && w->unkD3C == 1) {
            GroupBoard_DrawQuestList(0x1E, 0x36);
        }
        break;
    }
}

void GroupBoard_DrawMemberList(s16 x, s16 y);
void GroupBoard_DrawRecruitForm(s16 x, s16 y);
void HunterInfo_DrawIfOpen(void);
void GroupBoard_DrawRecruitText(s16 x, s16 y, s16 cur, s16 n);
void GroupBoard_DrawMembers(s16 x, s32 y, u32 id, WinShake *win, u16 rand, s8 k); /* unit's own declaration: with the definition's void (s16 x, s16 y, s32 n, WinShake *p, u16 a, s8 show) (lobby/unk_6EF380.c), GroupBoard_DrawDetails 4 of 161 words differ */
extern char *D_0080C4DC;
void GroupBoard_DrawDetails(u8 *o)
{
    Work3E *w = (Work3E *)(u8 *)&gMenuWork;
    s16 x;
    s16 x2;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    x = w->win.pos + w->shake.dx;
    switch (w->state) {
    case 2:
        if (w->unkD3B == 3) {
            switch (w->unkD3C) {
            case 3:
            case 4:
            case 5:
            case 6:
            case 8:
                GroupBoard_DrawMemberList(x, 0x32);
                break;
            case 9:
                HunterInfo_DrawIfOpen();
                break;
            }
        }
        break;
    case 1:
        if (w->unkD3B == 2) {
            switch (w->unkD3C) {
            case 3:
                GroupBoard_DrawRecruitForm(x, 0x32);
                break;
            case 1:
            case 2:
                GroupBoard_DrawRecruitForm(x, 0x32);
                switch (w->unkD3D) {
                case 1:
                    GroupBoard_DrawQuestPicker(0x14, 0x32);
                    break;
                case 2:
                    GroupBoard_DrawRecruitText(0x14, 0x32, w->rank, w->rankCount);
                    break;
                }
                break;
            }
        } else if (w->unkD3B == 4) {
            x2 = w->win0.pos + w->shake.dx;
            if (w->unkD3F == 0) {
                if (w->unkB28 == 2) {
                    HunterInfo_DrawIfOpen();
                } else {
                    Font_SetSpacing(0x16, 0x16);
                    Menu_DrawLabelBar(0x140, 0x12, 0xC8, 0xFFB0B000, D_0080C4DC, 2, 1);
                    GroupBoard_DrawMembers(x2, 0x32, w->unkC88, &w->shake, w->rand, w->unkD40);
                }
            }
        }
        break;
    case 3:
    case 4:
        if (w->unkD41) {
            GroupBoard_DrawMemberList(x, 0x32);
        } else if (w->unkD3F) {
            GroupBoard_DrawRecruitForm(x, 0x32);
        } else {
            GroupBoard_DrawMembers(w->win0.pos + w->shake.dx, 0x32, w->unkC88, &w->shake, w->rand, w->unkD40);
        }
        break;
    }
}
