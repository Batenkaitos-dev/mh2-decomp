#include "common.h"
#include "lobby_text.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_vobj.h"
#include "lobby_menu.h"
#include "lobby_labels.h"

#pragma optimization_level 4

typedef struct V006FBBD0_0 {
    u8 pad0[0x3E8];
    s16 f3E8;
    s16 f3EA;
    s16 f3EC;
} V006FBBD0_0;
/* a group (a board entry: its host's id, area, land and conditions) */
typedef struct V8BD1E0 {
    /* 0x000 */ char name[0x14];
    /* 0x014 */ u8 id[0x10];
    /* 0x024 */ char place0[0x104]; /* the current location (four lines) */
    /* 0x128 */ char place1[0x104];
    /* 0x22C */ char place2[0x44];
    /* 0x270 */ char place3[0x44];
    /* 0x2B4 */ char message[0x104]; /* the recruiting message */
    /* 0x3B8 */ u16 area; /* gLobbyLevel0Sel's number; 0: any */
    /* 0x3BA */ u16 land; /* gLobbyLevel1Sel's number; 0: any */
    /* 0x3BC */ u16 members; /* the group's member count */
    /* 0x3BE */ u16 maxMembers;
    /* 0x3C0 */ u8 kind; /* 1, 2: Lobby_HrAtLeast31 decides who may join */
    /* 0x3C1 */ u8 target; /* the target (the lobby's quest records) */
    /* 0x3C2 */ u8 pad3C2[2];
    /* 0x3C4 */ u32 color; /* the first place's mark colour */
    /* 0x3C8 */ u8 state; /* 3: open */
    /* 0x3C9 */ u8 rank; /* 1-based: the hunter rank range in the room's table */
    /* 0x3CA */ u8 pad3CA[2];
    /* 0x3CC */ u8 unk3CC[8]; /* the hunter's id (ShortMail_Open, the ID row) */
    s8 items[4]; /* 0x3D4: the rows (gMemberActTexts: mail, add as a friend, jump...) */
    WinSlide win; /* 0x3D8 */
    s32 flags; /* 0x3E0: the rows offered (HunterInfo_Open) */
    s32 joinArea; /* 0x3E4: gLobbyLevel0Sel when joining */
    s16 f3E8;
    s16 f3EA;
    s16 count; /* 0x3EC: the rows */
    s8 f3EE;
    s8 step; /* 0x3EF: Group_Join's step */
} V8BD1E0;
/* the hunter shown by the hunter panel (gHunterPanel) */
typedef struct Hunter8BD6EC {
    /* 0x00 */ u16 hr;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 weapon; /* the weapon (gWeaponTypeNames) */
    /* 0x04 */ u8 unk4[8];
    /* 0x0C */ u8 color; /* the symbol colour (gSymbolColorNames); 0xFF: none */
    /* 0x0D */ u8 unkD;
    /* 0x0E */ u8 title; /* the title (D_447754, 8-byte records) */
    /* 0x0F */ u8 unkF;
    /* 0x10 */ char comment[0x62]; /* the comment */
    /* 0x72 */ s16 arenaWins; /* the grand arena's wins */
    /* 0x74 */ EquipItem weaponItem;
    /* 0x80 */ EquipItem armor[5];
} Hunter8BD6EC;
extern Hunter8BD6EC gHunterPanel;
/* the mail menu (gMenuWork): its actions list */
typedef struct MailMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ WinSlide win2;
    /* 0x10 */ MailEntry rec; /* the chosen mail */
    /* 0xA2 */ s8 items[3]; /* gMailActTexts: reply, add as a friend, block */
    /* 0xA5 */ u8 unkA5;
    /* 0xA6 */ s16 sel; /* the chosen mail (gMailBox) */
    /* 0xA8 */ s16 mails; /* the mails listed (MailMenu_CountMails) */
    /* 0xAA */ s16 cursor;
    /* 0xAC */ s16 count;
    /* 0xAE */ s8 state;
    /* 0xAF */ s8 step;
} MailMenu;
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Font_SetSpacing(u8 a, u8 b);
extern V8BD1E0 gHunterInfoMenu;
void HunterInfo_Draw(V8BD1E0 *m, s16 x, s16 y);
extern MailEntry gMailBox[8];
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
extern u8 gMenuWork;
void Text_SetSize(s32);
void MailMenu_DrawMailbox(MailMenu *m, s16 x, s16 y);
void ShortMail_DrawForm(MailEntry *p, s16 sel, s32 flags, s16 x, s16 y);
void MailMenu_DrawActions(MailMenu *m, s16 x, s16 y);

void LobbyText_Open(s32);

extern u8 gDrinkLevel0[];
extern u8 gDrinkLevel1[];
extern u8 gDrinkLevel2[];
extern u8 gDrinkLevel3[];
extern u8 gDrinkLevel4[];
extern LobbyMenu *gLobbyMenu;
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);

s32 DrinkContest_IsOn(void) {
    return ((Minigame *)&gMenuWork)->running == 1;
}

u8 *DrinkContest_GetLevel(void) {
    u16 recordCounts;

    recordCounts = gSave.recordCounts[35];
    if ((s32) recordCounts < 5) {
        return gDrinkLevel0;
    }
    if ((s32) recordCounts < 0xA) {
        return gDrinkLevel1;
    }
    if ((s32) recordCounts < 0x14) {
        return gDrinkLevel2;
    }
    if ((s32) recordCounts < 0x28) {
        return gDrinkLevel3;
    }
    return gDrinkLevel4;
}

void HunterInfo_Open(void *src, s32 flags)
{
    V8BD1E0 *m = &gHunterInfoMenu;
    s8 i;
    s8 n;

    memset(m, 0, sizeof(*m));
    m->flags = flags;
    memcpy(m->unk3CC, src, 8);
    LobbyText_Open(0x20);
    gLobbyMenu->redraw = 1;
    WinSlide_Step(&m->win, 0x280, 0x118);
    n = 0;
    for (i = 0; i < 3; i++) {
        if (flags & (1 << i)) {
            m->items[n++] = i;
        }
    }
    m->count = n;
    gFieldControl = 0;
}

static void func_006FB250(void) {

}

s32 LobbyUser_FetchDetails(void *out, void *id);
void FriendList_AskAdd(void *id, void *name);
s32 FriendList_AddInput(u16 pad);
s32 ShortMail_Step(u16 pad);
s32 HunterInfo_ActionInput(V006FBBD0_0 *arg0, u16 arg1);
s32 Group_Join(V8BD1E0 *g, u16 pad);
void ShortMail_Open(void *, void *, u32);
void LobbyDialog_Grow(void);
extern LobbyMenu *gLobbyMenu;

s32 HunterInfo_Update(u16 pad)
{
    V8BD1E0 *m = &gHunterInfoMenu;

    switch (gHunterInfoMenu.f3EE) {
    case 0:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_FetchDetails(m, m->unk3CC)) {
        case 0:
            m->f3EE++;
            gFieldControl = 1;
            break;
        case 1:
            m->f3EE = 6;
            break;
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->f3EE++;
            break;
        }
        break;
    case 2:
        switch (HunterInfo_ActionInput((V006FBBD0_0 *)m, pad)) {
        case 0:
            switch (m->items[m->f3EA]) {
            case 0:
                m->f3EE = 5;
                ShortMail_Open(m->unk3CC, m, 4);
                gFieldControl = 0;
                break;
            case 1:
                if (gHunterPanel.unkF != 0) {
                    m->f3EE = 7;
                    gFieldControl = 0;
                    LobbyText_Open(0x93);
                    LobbyDialog_Grow();
                    gLobbyMenu->redraw = 1;
                } else {
                    m->f3EE = 4;
                    FriendList_AskAdd(m->unk3CC, m);
                }
                break;
            case 2:
                m->f3EE = 3;
                gFieldControl = 0;
                break;
            }
            break;
        case 3:
            return 3;
        case 4:
            return 4;
        }
        break;
    case 3:
        switch (Group_Join(m, pad)) {
        case 0:
            gFieldControl = 1;
            break;
        case 1:
            m->f3EE = 7;
            gFieldControl = 0;
            break;
        case 3:
            m->f3EE = 2;
            gFieldControl = 1;
            break;
        case 4:
            gFieldControl = 1;
            return 4;
        }
        break;
    case 4:
        switch (FriendList_AddInput(pad)) {
        case 0:
        case 1:
            m->f3EE = 2;
            break;
        case 4:
            gFieldControl = 1;
            return 4;
        }
        break;
    case 5:
        switch (ShortMail_Step(pad)) {
        case 0:
        case 3:
            m->f3EE = 2;
            break;
        case 4:
            return 4;
        }
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            func_006FB250();
            gFieldControl = 1;
            LobbySe_Play(8);
            return 1;
        }
        if (pad & 0x8000) {
            gFieldControl = 1;
            return 4;
        }
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->f3EE = 2;
            gFieldControl = 1;
            LobbySe_Play(8);
        } else if (pad & 0x8000) {
            gFieldControl = 1;
            return 4;
        }
        break;
    }
    return 2;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6FB0B0", D_00838390);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6FB0B0", D_008383A0);

typedef struct HrRange {
    s16 lo;
    s16 hi;
} HrRange;
typedef struct RoomFB630 {
    /* 0x000 */ u8 unk0[0x26C];
    /* 0x26C */ HrRange ranks[8];
} RoomFB630;
extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
s32 LobbySrv_Exists(s8 *id);
s32 LobbySrv_IsCur(s8 *id);
s32 Lobby_HrAtLeast31(void);
s32 Group_CanJoin(V8BD1E0 *g)
{
    u16 *hr = &gPlayerText.card.hr;
    RoomFB630 *room = (RoomFB630 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20);
    HrRange *r;

    if (g->state != 3 || LobbySrv_Exists((s8 *)g->id) == 0) {
        LobbyText_Open(0x2F);
        return 0;
    }
    if (LobbySrv_IsCur((s8 *)g->id) == 1 && g->area == gLobbyLevel0Sel && g->land == gLobbyLevel1Sel) {
        LobbyText_Open(0x5B);
        return 0;
    }
    if (g->area != 0) {
        if (*hr < room->ranks[(s8)(g->rank - 1)].lo) {
            LobbyText_Open(0xF);
            return 0;
        }
        if (room->ranks[(s8)(g->rank - 1)].hi < *hr) {
            LobbyText_Open(0xF);
            return 0;
        }
        if (g->land != 0) {
            switch (g->kind) {
            case 0:
                return 1;
            case 1:
                if (Lobby_HrAtLeast31() == 1) {
                    LobbyText_Open(0x88);
                    return 0;
                }
                break;
            case 2:
                if (Lobby_HrAtLeast31() == 0) {
                    LobbyText_Open(0x88);
                    return 0;
                }
                break;
            }
        }
    }
    return 1;
}

void Menu_SetYesNo(s8 arg0);
void LobbyDialog_Grow(void);
s32 Menu_YesNoInput(void);
void ChatLog_Clear(void);
void FadeB_Start(s32);
void Lobby_Disconnect(s32);
void FadeA_Start(s32);
void Stream_Stop(s32);
s32 LobbyLogin_EnterEntry(s32 a, s32 b);
void LobbyVillage_BeginStay(void);
void LobbyVillage_Begin(void);
void LobbyDialog_OpenWait(void *text);
void Town_PlaceLocalPlayer(s32 area);
void LobbyTown_Begin(void);
s32 LobbyLevel0_Leave(void);
s32 Group_Join(V8BD1E0 *g, u16 pad)
{
    s32 area;

    switch (g->step) {
    case 0:
        if (Group_CanJoin(g) == 1) {
            g->step++;
            if (LobbySrv_IsCur((s8 *)g->id) == 1) {
                LobbyText_Open(0x8A);
            } else {
                LobbyText_Open(0x89);
            }
            Menu_SetYesNo(1);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
        } else {
            gLobbyMenu->redraw = 1;
            LobbyDialog_Grow();
            return 1;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (pad & 0x8000) {
            return 4;
        }
        switch (Menu_YesNoInput()) {
        case 0:
            gLobby->groupJoin = 1;
            memcpy(gLobby->groupId, g->id, 0x10);
            gLobby->area = g->area;
            gLobby->land = g->land;
            g->joinArea = gLobbyLevel0Sel;
            if (LobbySrv_IsCur((s8 *)g->id) == 1) {
                area = gLobbyLevel0Sel;
                if (area != 0) {
                    if (g->area == 0 && g->land == 0) {
                        memset(gLobby, 0, 0x1C);
                        g->step = 4;
                    } else {
                        g->step++;
                        if (area != gLobby->land) {
                            ChatLog_Clear();
                        }
                        FadeB_Start(1);
                    }
                    LobbyText_Open(0x1D);
                }
            } else {
                Lobby_Disconnect(0);
                FadeA_Start(10);
                Stream_Stop(0);
                Stream_Stop(1);
            }
            break;
        case 3:
            g->step = 0;
            return 3;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyLogin_EnterEntry(gLobby->area, gLobby->land)) {
        case 0:
            if (gLobby->land == 0) {
                g->step = 0;
                if (gLobbyLevel1Sel == 0 && gLobbyLevel0Sel != 0) {
                    LobbyVillage_BeginStay();
                } else {
                    LobbyVillage_Begin();
                }
                FadeB_Start(10);
                memset(gLobby, 0, 0x1C);
            } else {
                g->step++;
                memset(gLobby, 0, 0x1C);
            }
            break;
        case 1:
            g->step = 0;
            gLobbyLevel1Sel = 0;
            memset(gLobby, 0, 0x1C);
            if (gLobby->errText[0] != 0) {
                LobbyDialog_OpenWait(gLobby->errText);
            }
            LobbyDialog_Grow();
            FadeB_Start(2);
            if (g->joinArea != gLobbyLevel0Sel && gLobbyLevel0Sel != 0) {
                Town_PlaceLocalPlayer(gScene.area);
            }
            return 1;
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        LobbyTown_Begin();
        break;
    case 4:
        LobbyLevel0_Leave();
        break;
    }
    return 2;
}

s32 HunterInfo_ActionInput(V006FBBD0_0 *m, u16 pad)
{
    s32 btn;

    btn = pad & 0xFFFF;
    if (btn & 0x20) {
        LobbySe_Play(8);
        return 0;
    }
    if (btn & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (btn & 0x8000) {
        return 4;
    }
    if (btn & 0xC00) {
        m->f3E8 = Menu_WrapLeftRight((s32) m->f3E8, 3);
    } else {
        m->f3EA = Menu_WrapUpDown((s32) m->f3EA, (s32) m->f3EC);
    }
    return 2;
}

void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
typedef struct QuestBC80 {
    /* 0x00 */ char name[0x14];
    /* 0x14 */ u8 used;
    /* 0x15 */ u8 pad15[3];
} QuestBC80;
typedef struct LobbyBC80 {
    /* 0x000 */ u8 unk0[0xBBC];
    /* 0xBBC */ QuestBC80 quests[64];
} LobbyBC80;
extern char D_00838390[];
extern char D_008383A0[];
extern char D_008383B0[];
extern char D_008383C0[];
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
s32 Text_DisplayLength(char *s);
void Text_Printf(const char *fmt, ...);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
u32 AreaList_KindColor(u8 rank);
void HunterInfo_DrawGroup(V8BD1E0 *g, s16 x, s16 y)
{
    Hunter8BD6EC *h = (Hunter8BD6EC *)(u8 *)&gHunterPanel;
    u32 color = 0;
    s16 tx = x;
    s16 ty = y;

    ty += 20;
    tx += 14;
    Font_SetPos(tx, ty);
    Text_PrintfColored(D_00838390, gNetLabels[NET_LABEL_NAME], gLobbyWords[LOBBY_WORD_COLON], g->name);
    Font_SetPos(tx + 144, ty);
    Text_PrintfColored(D_00838390, gNetLabels[NET_LABEL_ID], gLobbyWords[LOBBY_WORD_COLON], g->unk3CC);
    Font_SetPos(tx, ty += 20);
    Text_PrintfColored(D_00838390, gNetLabels[NET_LABEL_MARK], gLobbyWords[LOBBY_WORD_COLON], gOnlineMarkNames[h->unk2] + 4);
    if (h->unk2 != 0) {
        Icon_Draw(tx + 72, ty, 0x19, 0x14, -1, h->unk2 + 0x59);
    }
    Font_SetPos(tx, ty += 20);
    Text_PrintfColored(D_008383A0, gNetLabels[NET_LABEL_LOCATION]);
    Font_SetColor(0x807F7F7F);
    tx += Text_DisplayLength(gNetLabels[NET_LABEL_LOCATION]) * 9 + 22;
    Font_SetPos(tx, ty);
    Text_PrintfColored("%s", g->place0);
    Font_SetPos(tx, ty += 20);
    Text_PrintfColored("%s", g->place1);
    LobbyUi_DrawIcon(tx - 14, ty + 9, 0x14, g->color | 0xFF000000, 1);
    Font_SetPos(tx, ty += 20);
    if (g->area != 0) {
        Text_PrintfColored("%s", g->place2);
        color = AreaList_KindColor(g->rank);
        color |= 0xFF000000;
        LobbyUi_DrawIcon(tx - 14, ty + 9, 0x14, color, 2);
    }
    Font_SetPos(tx, ty += 20);
    if (g->land == 0) {
        return;
    }
    Text_PrintfColored("%s", g->place3);
    LobbyUi_DrawIcon(tx - 14, ty + 9, 0x14, color, 3);
    x = x + 14;
    Font_SetPos(x, ty += 20);
    Text_Printf(D_008383B0, gNetLabels[NET_LABEL_RECRUIT_MSG_LABEL], g->message);
    if (g->rank == 4) {
        Font_SetPos(x, ty += 20);
        Text_Printf(D_008383B0, gNetLabels[NET_LABEL_LIMIT_COND], gGroupLimitTexts[g->kind]);
        Font_SetPos(x, ty += 20);
        Text_Printf(D_008383B0, gNetLabels[NET_LABEL_TARGET_COLON], ((LobbyBC80 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20))->quests[g->target].name);
    }
    Font_SetPos(x, ty + 20);
    Text_Printf(D_008383C0, gNetLabels[NET_LABEL_GROUP_MEMBERS], g->members, g->maxMembers);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6FB0B0", D_008383B0);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6FB0B0", D_008383C0);

extern char *D_447754[];
extern char *gWeaponTypeNames[];
extern char *gSymbolColorNames[];
s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
void Text_DrawWrapped33(s16 x, s16 y, s16 lineh, char *s);
void HunterInfo_DrawStatus(V8BD1E0 *g, s16 x, s16 y)
{
    Hunter8BD6EC *h = (Hunter8BD6EC *)(u8 *)&gHunterPanel;
    s16 ty = y;
    s32 lx = x;
    char num[0x10];
    char buf[0x20];

    Font_SetPos(lx + 14, ty += 20);
    Text_PrintfColored(D_00838390, gNetLabels[NET_LABEL_NAME], gLobbyWords[LOBBY_WORD_COLON], g);
    Font_SetPos((s16)(lx + 14) + 144, ty);
    Text_PrintfColored(D_00838390, gNetLabels[NET_LABEL_ID], gLobbyWords[LOBBY_WORD_COLON], g->unk3CC);
    Font_SetPos(lx + 14, ty += 20);
    Text_PrintfColored(D_008383B0, gNetLabels[NET_LABEL_TITLE], D_447754[h->title * 2]);
    Font_SetPos(lx + 14, ty += 20);
    Text_PrintfColored(D_008383B0, gNetLabels[NET_LABEL_WEAPON_COLON], gWeaponTypeNames[h->weapon]);
    Font_SetPos(lx + 14, ty += 20);
    if (h->color != 0xFF) {
        Text_PrintfColored(D_008383B0, gNetLabels[NET_LABEL_SYMBOL_COLOR], gSymbolColorNames[h->color]);
    }
    Font_SetPos(lx + 14, ty += 20);
    sprintf(num, "%d", h->hr);
    Sjis_Widen(num, buf);
    Text_PrintfColored(D_00838390, gNetLabels[NET_LABEL_HR], gLobbyWords[LOBBY_WORD_COLON], buf);
    Font_SetPos(lx + 14, ty += 20);
    sprintf(num, "%d", h->arenaWins);
    Sjis_Widen(num, buf);
    Text_PrintfColored("~C05%s~C00%s~C05%s", gNetLabels[NET_LABEL_ARENA_CUP], buf, gNetLabels[NET_LABEL_WINS]);
    Font_SetPos(lx + 14, ty += 20);
    Text_PrintfColored(D_008383A0, gNetLabels[NET_LABEL_COMMENT]);
    Font_SetColor(0x807F7F7F);
    Text_DrawWrapped33(lx + 14, ty + 20, 0x14, h->comment);
}

extern s8 gHunterEquipOrder[];
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, EquipItem *e) (lobby/name_tags.c), HunterInfo_DrawEquipment 75 of 100 words differ */
char *Equip_GetName(u8 type, u16 id);
void HunterInfo_DrawEquipment(void *m, s16 x, s16 y)
{
    s16 k;
    Hunter8BD6EC *h = (Hunter8BD6EC *)(u8 *)&gHunterPanel;
    s16 ty = y;
    s16 i;

    Font_SetPos(x + 60, ty += 24);
    Lobby_DrawEquipIcon(x + 14, ty - 10, 0x24, 0x24, &h->weaponItem);
    Text_PrintfColored(D_008383B0, gLobbyWords[LOBBY_WORD_COLON], Equip_GetName(h->weaponItem.type, h->weaponItem.id));
    for (i = 0; i < 5; i++) {
        k = gHunterEquipOrder[i];
        ty += 36;
        Font_SetPos(x + 60, ty);
        Lobby_DrawEquipIcon(x + 14, ty - 10, 0x24, 0x24, &h->armor[k]);
        Text_PrintfColored(D_008383B0, gLobbyWords[LOBBY_WORD_COLON], Equip_GetName(h->armor[k].type, h->armor[k].id));
    }
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), HunterInfo_DrawActions 105 of 116 words, HunterInfo_Draw 100 of 108 words, MailMenu_DrawMailbox 239 of 262 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawLine(s16 x0, s16 y0, s16 x1, s16 y1, s8 flag);
void HunterInfo_DrawActions(V8BD1E0 *m, s16 x, s16 y)
{
    s16 i;
    s8 f;

    x -= 130;
    Win_Draw(x, y, 130, m->count * 22 + 14, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < m->count; i++) {
        if (i > 0) {
            Gfx_DrawLine(x + 8, y - 2, 0x72, 2, 0xBDB59F6C);
        }
        if (m->f3EA == i) {
            switch (m->f3EE) {
            case 2:
                f = 0;
                break;
            default:
                f = 1;
                break;
            }
            Menu_DrawLine(x + 8, y - 1, 0x72, 0x14, f);
        }
        Font_SetPos(x + 8, y);
        Text_PrintfColored(gMemberActTexts[m->items[i]]);
        y += 22;
    }
}

void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void HunterInfo_DrawGroup(V8BD1E0 *g, s16 x, s16 y);
void HunterInfo_DrawStatus(V8BD1E0 *g, s16 x, s16 y);
void HunterInfo_DrawEquipment(void *m, s16 x, s16 y);
void HunterInfo_DrawActions(V8BD1E0 *m, s16 x, s16 y);
void HunterInfo_Draw(V8BD1E0 *m, s16 x, s16 y)
{
    s16 ty = y;
    s16 cy;

    Win_Draw(x, y, 0x154, 0x10E, 0xBDB59F6C, 0xBB402010, 8);
    ty += 12;
    Gfx_DrawLine(x + 8, ty, 0x144, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 14, ty);
    Text_PrintfColored(gGameMenuLabels[MENU_LABEL_STATUS]);
    Menu_DrawPageNum(x + 250, ty, m->f3E8, 3, 1);
    if (m->f3EE != 1) {
        HunterInfo_DrawActions(m, x, y);
    }
    cy = ty + 8;
    switch (m->f3E8) {
    case 0:
        HunterInfo_DrawGroup(m, x, cy);
        break;
    case 1:
        HunterInfo_DrawStatus(m, x, cy);
        break;
    case 2:
        HunterInfo_DrawEquipment(m, x, cy);
        break;
    }
}

void HunterInfo_DrawIfOpen(void)
{
    V8BD1E0 *p = &gHunterInfoMenu;

    Font_SetSpacing(0x12, 0x12);
    if (gHunterInfoMenu.f3EE == 0 || p->f3EE == 5) {
        return;
    }
    HunterInfo_Draw(p, p->win.pos, 0x3A);
}

void MailMenu_Draw(u8 *o);
void MailMenu_Open(void)
{
    MailMenu *m = (MailMenu *)&gMenuWork;

    memset(m, 0, 0xF00);
    m->items[0] = 0;
    m->items[1] = 1;
    m->items[2] = 2;
    m->count = 3;
    WinSlide_Step(&m->win, 0x280, 0x118);
    LobbyMenu_SetDraw(0, MailMenu_Draw, 0, 0);
    gFieldControl = 0;
}

void MailMenu_Close(void)
{
    gFieldControl = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
}

struct RecListMenu;
s32 MailMenu_ListInput(struct RecListMenu *m, u16 pad);
s32 MailMenu_ActionInput(MailMenu *m, u16 pad);
void ShortMail_Open(void *id, void *name, u32 flags);
void FriendList_AskAdd(void *id, void *name);
void BlockList_AskAdd(void *id, void *name);
s32 BlockList_AddInput(u16 pad);
s32 FriendList_AddInput(u16 pad);
s32 ShortMail_Step(u16 pad);
s32 MailMenu_Update(u16 pad)
{
    MailMenu *m = (MailMenu *)&gMenuWork;

    switch (m->state) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 1:
        switch (MailMenu_ListInput((struct RecListMenu *)m, pad)) {
        case 0:
            m->state++;
            m->cursor = 0;
            gMailBox[m->sel].used = 0;
            memcpy(&m->rec, &gMailBox[m->sel], sizeof(MailEntry));
            WinSlide_Step(&m->win2, 0x280, 0x118);
            break;
        case 3:
            MailMenu_Close();
            return 3;
        case 4:
            MailMenu_Close();
            return 4;
        }
        break;
    case 2:
        switch (MailMenu_ActionInput(m, pad)) {
        case 0:
            switch (m->items[m->cursor]) {
            case 0:
                m->state = 4;
                ShortMail_Open(m->rec.id, m->rec.name, 4);
                break;
            case 1:
                m->state = 3;
                FriendList_AskAdd(m->rec.id, m->rec.name);
                break;
            case 2:
                m->state = 5;
                BlockList_AskAdd(m->rec.id, m->rec.name);
                break;
            }
            break;
        case 3:
            m->state = 1;
            break;
        case 4:
            MailMenu_Close();
            return 4;
        }
        break;
    case 5:
        switch (BlockList_AddInput(pad)) {
        case 0:
        case 3:
        case 1:
            m->state = 2;
            m->step = 1;
            break;
        case 4:
            MailMenu_Close();
            return 4;
        }
        break;
    case 3:
        switch (FriendList_AddInput(pad)) {
        case 0:
        case 3:
        case 1:
            m->state = 2;
            m->step = 1;
            break;
        case 4:
            return 4;
        }
        break;
    case 4:
        switch (ShortMail_Step(pad)) {
        case 0:
        case 3:
            m->state = 2;
            m->step = 1;
            break;
        case 4:
            return 4;
        }
        break;
    }
    return 2;
}

s32 MailMenu_ActionInput(MailMenu *m, u16 pad)
{
    switch (m->step) {
    case 0:
        switch (WinSlide_Step(&m->win2, 0x280, 0x118)) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->step = 0;
            LobbySe_Play(8);
            return 0;
        }
        if (pad & 0x40) {
            m->step = 0;
            LobbySe_Play(4);
            return 3;
        }
        if (pad & 0x8000) {
            m->step = 0;
            return 4;
        }
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
        break;
    }
    return 2;
}

static s16 MailMenu_CountMails(void)
{
    s16 n = 0;
    s16 i;

    for (i = 0; i < 8; i++) {
        if (gMailBox[i].id[0] != 0) {
            n++;
        }
    }
    return n;
}

typedef struct RecListMenu {
    u8 unk0[0xA6];
    s16 cursor;
    s16 count;
} RecListMenu;
s32 MailMenu_ListInput(RecListMenu *m, u16 pad)
{
    m->count = MailMenu_CountMails();
    if (pad & 0x20) {
        if (m->count != 0) {
            LobbySe_Play(8);
            return 0;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x8000) {
        return 4;
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
    }
    return 2;
}

/* "%d" and "%s" again: the first copies belong to the functions above (the game
   has one copy per unit, so this function probably starts another unit) */
const char D_00838418[3] = {
#include "lobby/00838418.inc"
};
const char D_00838420[3] = {
#include "lobby/00838420.inc"
};
extern char *gMailboxMenuTexts[];
void MailMenu_DrawMailbox(MailMenu *m, s16 x, s16 y)
{
    char wide[0x40];
    s8 f;
    char buf[0x40];
    s16 ty = y;
    s16 i;
    MailEntry *p;

    Win_Draw(x, y, 0x154, 0x122, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 0xC, 0x144, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0xC, y + 0xC);
    Text_PrintfColored(gMailboxMenuTexts[0]);
    if (gMailBox[0].id[0] == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(gEmptyListTexts[EMPTY_LIST_MAIL], x + 0xAA, 0x12), (s16)(y + 0xC) + 0x5A);
        Text_PrintfColored(gEmptyListTexts[EMPTY_LIST_MAIL]);
        return;
    }
    y += 0x28;
    Font_SetPos(x + 0xC, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_NUMBER]);
    Font_SetPos(x + 0x2A, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_MAIL_UNIT]);
    Font_SetPos(x + 0x46, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_NAME]);
    Font_SetPos(x + 0xE0, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_ID]);
    ty += 0x3A;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < m->mails; i++) {
        p = &gMailBox[i];
        if (p->id[0] != 0) {
            if (m->sel == i) {
                switch (m->state) {
                case 1:
                    f = 0;
                    break;
                default:
                    f = 1;
                    break;
                }
                Menu_DrawLine(x + 8, ty - 1, 0x144, 0x14, f);
            }
            Font_SetPos(x + 0xC, ty);
            sprintf(buf, D_00838418, i + 1);
            Sjis_Widen(buf, wide);
            Text_PrintfColored(D_00838420, wide);
            if (p->used == 1) {
                Icon_Draw(x + 0x2A, ty - 1, 0x14, 0x14, -1, 0x55);
            } else {
                Icon_Draw(x + 0x2A, ty - 1, 0x14, 0x14, -1, 0x56);
            }
            Font_SetPos(x + 0x46, ty);
            sprintf(wide, D_00838420, p->name);
            Sjis_Widen(wide, buf);
            Text_PrintfColored(D_00838420, buf);
            Font_SetPos(x + 0xE0, ty);
            sprintf(wide, D_00838420, p->id);
            Sjis_Widen(wide, buf);
            Text_PrintfColored(D_00838420, buf);
        }
        ty += 0x16;
    }
}

void MailMenu_DrawActions(MailMenu *m, s16 x, s16 y)
{
    s16 i;
    s8 f;

    x -= 130;
    Win_Draw(x, y, 130, m->count * 22 + 14, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < m->count; i++) {
        if (i > 0) {
            Gfx_DrawLine(x + 8, y - 2, 0x72, 2, 0xBDB59F6C);
        }
        if (m->cursor == i) {
            switch (m->state) {
            case 2:
                f = 0;
                break;
            default:
                f = 1;
                break;
            }
            Menu_DrawLine(x + 8, y - 1, 0x72, 0x14, f);
        }
        Font_SetPos(x + 8, y);
        Text_PrintfColored(gMailActTexts[m->items[i]]);
        y += 22;
    }
}

void MailMenu_Draw(u8 *o)
{
    MailMenu *m = (MailMenu *)&gMenuWork;
    s16 row;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    switch (m->state) {
    case 0:
    case 1:
        MailMenu_DrawMailbox(m, m->win.pos, 0x36);
        break;
    case 2:
        row = m->win2.pos;
        ShortMail_DrawForm(&m->rec, 0, 2, row, 0x36);
        MailMenu_DrawActions(m, row, 0x36);
        break;
    case 4:
        break;
    }
}

extern ShortMailMenu gShortMailMenu;
extern MailEntry gShortMailMsg;
void ShortMail_Draw(MenuTask *task);
void ShortMail_Draw2(MenuTask *task);
void ShortMail_Open(void *id, void *name, u32 flags)
{
    ShortMailMenu *m = &gShortMailMenu;

    memset(m, 0, 0x4B4);
    gShortMailMenu.layer[0].textSize = 0;
    gShortMailMenu.layer[0].draw = ShortMail_Draw;
    gShortMailMenu.layer[1].draw = ShortMail_Draw2;
    gShortMailMenu.layer[1].textSize = 1;
    gShortMailMenu.flags = flags;
    WinSlide_Step(&m->win, 0x280, 0x118);
    gFieldControl = 0;
    if (name != NULL) {
        memcpy(m->mail.name, name, 0x10);
    }
    if (id != NULL) {
        memcpy(m->mail.id, id, 8);
    } else if (flags & 8) {
        memcpy(&m->mail, &gShortMailMsg, sizeof(MailEntry));
    }
    if (flags & 9) {
        m->count = 3;
    } else if (flags & 4) {
        m->count = 2;
    }
}

/* unk_6FB0B0's .data (0x7EC170-0x7EC178) */
s8 gHunterEquipOrder[8] = {
#include "lobby/007EC170.inc"
};
