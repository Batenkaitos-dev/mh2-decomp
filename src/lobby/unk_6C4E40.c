#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "item.h"
#include "scene.h"
#include "save.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "area.h"
s32 Village_MealPointsReady(void);
u16 LobbyPad_Get(s32);
int abs(int);

typedef struct MenuHdr {
    /* 0x0 */ u8 state;
    /* 0x1 */ u8 sub;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ s8 unk3;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ s16 count;
} MenuHdr;

extern u8 gMenuWork[];
extern char *gSmithyMenuTexts[];
extern char *gSmithyMakeTexts[];
void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void Deco_Draw2(u8 *unused0);
void Text_SetSize(s32 a);
void WeaponForge_Open(s8 *task);
void ArmorForge_Open(s8 *task);
void func_006DA9D0(void);
void LobbySe_Play(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
void CamReq_Cancel(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* Items listed by gKitchenIngredients: NULL-terminated lists of 8-byte records. */
typedef struct ItemRec {
    u16 item;
    s8 stat[3][2];      /* per grade: the two stats of gMealStatKinds[kind] */
} ItemRec;
extern ItemRec *gKitchenIngredients[];
extern char D_008260B0[];
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);

struct Em *Npc_Find(u8 kind);
void Actor_SetState(void *, s32, s32);
s32 Game_IsOnline(void);
struct OfferTask;
void OfferMenu_Open(struct OfferTask *t);
void Talk_Say(s32 msg, s8 a);
s32 Talk_Update(void);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 se);
void Chat_SetColor(s16 color);
void CamReq_Workshop(u8 lv); /* unit's own declaration: with the definition's void (s32 a) (sub_main/unk_323C90.c), Workshop_Update 1 of 239 words differ */
/* unit's own declaration: with the definition's u8 (s32 k) (sub_main/unk_3B4C80.c), Workshop_Update: 1 of 239 words differ */
s32 Village_FacilityLevel(s32 a);
s32 Npc_NextRoutePoint(struct Em *);
void Tutorial_Start(s8 a, s32 b, s32 c);
s32 Tutorial_IsDone(void);
void Npc_PickIdleState(struct Em *p);
void Workshop_Close(void);
void Workshop_OpenMenu(void);
s32 Workshop_MenuInput(u16 pad);
s32 Workshop_MenuConfirm(u16 pad);
void *memset(void *dst, s32 c, u32 n);
void Workshop_Update(void)
{
    MenuHdr *m = (MenuHdr *)gMenuWork;
    u8 *p = gPlayers[gScene.localPlayer].target;
    u8 *npc = (u8 *)Npc_Find(0xE);
    NpcWorkshopWork *st = (NpcWorkshopWork *)(p + 0x438);
    u16 pad = LobbyPad_Get(0);

    switch (m->state) {
    case 0:
        if (pad & 0x20) {
            if (m->unk3 != 0) {
                if (m->sub == m->count - 1 && Game_IsOnline() == 0) {
                    OfferMenu_Open((struct OfferTask *) p);
                    LobbySe_Play(0x1B);
                    break;
                }
            } else if (m->sub == 3) {
                OfferMenu_Open((struct OfferTask *) p);
                LobbySe_Play(0x1B);
                break;
            }
            m->state++;
            memset((u8 *)m + 0xC, 0, 0xE00);
            Workshop_OpenMenu();
            LobbySe_Play(0x1B);
        } else if (pad & 0x40) {
            m->state = 5;
            Talk_Say(1, 0);
            LobbySe_Play(4);
        } else {
            m->sub = Menu_WrapUpDownSe(m->sub, m->count, 2);
        }
        break;
    case 1:
        switch (Workshop_MenuInput(pad)) {
        case 0:
            st->menuSave = m->unk4;
            m->state++;
            Chat_SetColor(-1);
            break;
        case 3:
            m->state--;
            break;
        }
        break;
    case 2:
        m->state++;
        if (Game_IsOnline() == 0) {
            if (gScene.area == AREA_HUB_0x57) {
                break;
            }
            if (st->id == 0xF) {
                Actor_SetState(npc, 0, 0x69);
                Npc_NextRoutePoint((struct Em *)p);
            } else {
                Actor_SetState(p, 0, 0x69);
            }
            CamReq_Workshop(Village_FacilityLevel(2));
        } else if (st->id == 0x58) {
            Tutorial_Start(1, 0, 0);
            CamReq_Workshop(4);
        } else {
            Tutorial_Start(0, 0, 0);
            CamReq_Workshop(3);
        }
        break;
    case 3:
        if (Game_IsOnline() == 0) {
            if (gScene.area == AREA_HUB_0x57) {
                m->state++;
                break;
            }
            if (npc[0x15] != 0x69) {
                CamReq_Cancel();
                m->state++;
                if (st->id == 0xF) {
                    Npc_PickIdleState((struct Em *)p);
                }
            }
        } else if (Tutorial_IsDone() == 1) {
            m->state++;
        }
        break;
    case 4:
        switch (Workshop_MenuConfirm(pad)) {
        case 0:
            m->state = 1;
            Chat_SetColor(0);
            break;
        }
        break;
    case 5:
        switch (Talk_Update()) {
        case 0:
            Workshop_Close();
            break;
        }
        break;
    }
}

void Workshop_OpenMenu(void)
{
    MenuHdr *m = (MenuHdr *)gMenuWork;

    switch (m->sub) {
    case 0:
        WeaponForge_Open((s8 *) m);
        break;
    case 1:
        ArmorForge_Open((s8 *) m);
        break;
    case 2:
        func_006DA9D0();
        break;
    }
}

void Chat_SetWindowMode(s8 a, s8 b);
s32 WeaponForge_Input(u8 *task, u16 pad);
s32 ArmorForge_Input(u8 *task, u16 pad);
s32 Deco_Input(u8 *task, u16 pad);
s32 Workshop_MenuInput(u16 pad)
{
    MenuHdr *m = (MenuHdr *)gMenuWork;

    Chat_SetWindowMode(2, 2);
    switch (m->sub) {
    case 0:
        return WeaponForge_Input((u8 *) m, pad);
    case 1:
        return ArmorForge_Input((u8 *) m, pad);
    case 2:
        return Deco_Input((u8 *) m, pad);
    }
    return 2;
}

s32 WeaponForge_Confirm(u8 *task, u16 pad);
s32 ArmorForge_Confirm(u8 *task, u16 pad);
s32 Deco_Confirm(u8 *task, u16 pad);
s32 Workshop_MenuConfirm(u16 pad)
{
    MenuHdr *m = (MenuHdr *)gMenuWork;

    Chat_SetWindowMode(2, 2);
    switch (m->sub) {
    case 0:
        return WeaponForge_Confirm((u8 *) m, pad);
    case 1:
        return ArmorForge_Confirm((u8 *) m, pad);
    case 2:
        return Deco_Confirm((u8 *) m, pad);
    }
    return 2;
}

void Workshop_DrawMenu(s32 x, s32 y)
{
    MenuHdr *m = (MenuHdr *)gMenuWork;

    Font_SetSpacing(0x14, 0x14);
    if (m->unk3 != 0) {
        Menu_DrawLabelList(0x1CC, gSmithyMakeTexts, m->count, m->sub, 0xFFB0B000);
    } else {
        Menu_DrawLabelList(0x1CC, gSmithyMenuTexts, m->count, m->sub, 0xFFB0B000);
    }
}

void WeaponForge_Draw(u8 *o);
void ArmorForge_Draw(u8 *task);
void Deco_Draw(u8 *task);
void WeaponForge_DrawPreview(u8 *o);
void ArmorForge_DrawEquipPrompt(u8 *task);
void Deco_DrawEquipPrompt(u8 *task);
void Workshop_Draw(void *task)
{
    MenuHdr *m = (MenuHdr *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    switch (m->state) {
    case 0:
        Workshop_DrawMenu(0x1CC, 0x40);
        break;
    case 1:
        switch (m->sub) {
        case 0:
            WeaponForge_Draw((u8 *) m);
            break;
        case 1:
            ArmorForge_Draw((u8 *) m);
            break;
        case 2:
            Deco_Draw((u8 *) m);
            break;
        }
        break;
    case 4:
        switch (m->sub) {
        case 0:
            WeaponForge_DrawPreview((u8 *) m);
            break;
        case 1:
            ArmorForge_DrawEquipPrompt((u8 *) m);
            break;
        case 2:
            Deco_DrawEquipPrompt((u8 *) m);
            break;
        }
        break;
    }
}

void Workshop_Draw2(void *task)
{
    MenuHdr *m = (MenuHdr *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    switch (m->state) {
    case 0:
        break;
    case 1:
        switch (m->sub) {
        case 2:
            Deco_Draw2((u8 *)m);
            break;
        }
        break;
    case 4:
        break;
    }
}

/* the kitchen (menu 0x48; gMenuWork) */
typedef struct Menu48 {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ WinSlide win2;
    /* 0x10 */ void *npc;
    /* 0x14 */ Player *player;
    /* 0x18 */ u8 ids[4][8]; /* the players (LobbyState +0x1564 entries) */
    /* 0x38 */ u8 unk38[0x68 - 0x38];
    /* 0x68 */ s8 unk68;
    /* 0x69 */ u8 unk69[0x78 - 0x69];
    /* 0x78 */ u16 unk78;
    /* 0x7A */ u16 unk7A;
    /* 0x7C */ u16 rand;
    /* 0x7E */ u8 unk7E[2];
    /* 0x80 */ s32 mask; /* which of the 16 entries */
    /* 0x84 */ s16 count;
    /* 0x86 */ u8 unk86[2];
    /* 0x88 */ s16 unk88;
    /* 0x8A */ u8 unk8A[4];
    /* 0x8E */ s16 timer;
    /* 0x90 */ u8 unk90;
    /* 0x91 */ u8 unk91;
    /* 0x92 */ u8 unk92;
    /* 0x93 */ s8 state;
    /* 0x94 */ s8 unk94;
} Menu48;
void Kitchen_Draw(void *task);
extern u8 gMenuInputLock;
extern u8 gTakeSpotId;
void Talk_Start(struct Em *npc, s8 quiet);
u16 Rand_Next(s32 stream);
void Kitchen_ListDiners(Menu48 *m);
void Kitchen_Broadcast(Menu48 *m, u8 kind);
void Kitchen_Open(void)
{
    Menu48 *m = (Menu48 *)gMenuWork;
    void *npc = Npc_Find(0x71);

    if (npc == NULL) {
        return;
    }
    gTownMenuOpen = 1;
    gTownMode = 0x48;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Kitchen_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    m->npc = npc;
    m->player = &gPlayers[gScene.localPlayer];
    m->count = 1;
    Actor_SetState(m->player, 0, 0x1E);
    Talk_Start(m->npc, 1);
    m->rand = Rand_Next(1);
    if (gScene.area == AREA_HUB_0x57) {
        m->unk91 = 3;
    } else if (gTakeSpotId <= 1) {
        m->unk91 = 0;
    } else if (gTakeSpotId <= 5) {
        m->unk91 = 1;
    } else {
        m->unk91 = 2;
    }
    if (Village_MealPointsReady() == 1) {
        m->unk90 = 1;
    }
    Kitchen_ListDiners(m);
    Kitchen_Broadcast(m, 0);
    m->timer = 1800;
}

void *memcpy(void *dst, const void *src, u32 n);
typedef struct Info48 {
    /* 0x0 */ s32 mask;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ u16 unk6;
    /* 0x8 */ u16 unk8;
    /* 0xA */ u16 rand;
    /* 0xC */ u8 unkC;
} Info48;
extern char gMyUserId[];
s32 memcmp(void *a, void *b, s32 n);
void TownMsg_Send31(u8 *p, void *to);
void Kitchen_Broadcast(Menu48 *m, u8 kind)
{
    Info48 info;
    u8 msg[0x50];
    s32 i;

    if (Game_IsOnline() == 0) {
        return;
    }
    if (m->count == 1) {
        return;
    }
    msg[0] = kind;
    switch (kind) {
    case 0:
        for (i = 0; i < m->count; i++) {
            if (memcmp(m->ids[i], gMyUserId, 8) != 0) {
                TownMsg_Send31(msg, m->ids[i]);
            }
        }
        break;
    case 2:
        info.unk4 = m->unk68;
        info.unk6 = m->unk78;
        info.unk8 = m->unk7A;
        info.rand = m->rand;
        info.mask = m->mask;
        info.unkC = m->unk91;
        memcpy(&msg[1], &info, 0x10);
        for (i = 0; i < m->count; i++) {
            if (memcmp(m->ids[i], gMyUserId, 8) != 0) {
                TownMsg_Send31(msg, m->ids[i]);
            }
        }
        break;
    case 1:
    case 3:
        msg[1] = m->unk91;
        TownMsg_Send31(msg, NULL);
        break;
    }
}

void Kitchen_Close(void)
{
    u8 localPlayer;

    gTownMenuOpen = 0;
    gTownMode = 0;
    localPlayer = gScene.localPlayer;
    gPlayers[localPlayer].target = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
    CamFocus_End();
    LobbySe_Play(4);
}

s32 Village_MealPointsReady(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        if (gSave.npcRecsB[i].flag != 0) {
            return 0;
        }
    }
    return gSave.mealPoints >= 300;
}

void Kitchen_ListDiners(Menu48 *m)
{
    Player *p = &gPlayers[gScene.localPlayer];
    s32 i;

    if (Game_IsOnline() == 0) {
        m->count = 1;
        m->mask = 1 << *(u16 *)p->spot;
        return;
    }
    m->count = 0;
    m->mask = 0;
    memset(m->ids, 0, sizeof(m->ids));
    if (*(u16 *)p->spot <= 1) {
        for (i = 0; i < 2; i++) {
            if ((s8)gLobby->slotIds[i * 8] != 0) {
                memcpy(m->ids[m->count], &gLobby->slotIds[i * 8], 8);
                m->mask |= 1 << i;
                m->count++;
            }
        }
    } else if (*(u16 *)p->spot <= 5) {
        for (i = 2; i < 6; i++) {
            if ((s8)gLobby->slotIds[i * 8] != 0) {
                memcpy(m->ids[m->count], &gLobby->slotIds[i * 8], 8);
                m->mask |= 1 << i;
                m->count++;
            }
        }
    } else {
        for (i = 6; i < 16; i++) {
            if ((s8)gLobby->slotIds[i * 8] != 0) {
                memcpy(m->ids[m->count], &gLobby->slotIds[i * 8], 8);
                m->mask |= 1 << i;
                m->count++;
            }
        }
    }
}

typedef struct ListMenu {
    /* 0x00 */ s16 x;
    /* 0x02 */ u8 unk2[0x68];
    /* 0x6A */ u16 items[7];
    /* 0x78 */ u16 unk78;
    /* 0x7A */ u16 unk7A;
    /* 0x7C */ u8 unk7C[8];
    /* 0x84 */ s16 unk84;
    /* 0x86 */ s16 cursor;
    /* 0x88 */ s16 count;
    /* 0x8A */ s16 page;
    /* 0x8C */ s16 pages;
    /* 0x8E */ u8 unk8E[4];
    /* 0x92 */ s8 unk92;
    /* 0x93 */ s8 state;
    /* 0x94 */ s8 unk94;
} ListMenu;
void IngredientList_BuildPage(ListMenu *m, s16 page);
s32 Kitchen_ChooseIngredient(ListMenu *m, u16 pad);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
void Npc_SetKitchenRoute(struct Em *npc, u8 kind);
struct Em;
void Npc_AdvanceRoute(struct Em *npc);
s32 WinSlide_Step(WinSlide *win, s16 from, s16 to);
char *Text_FillDollar(char *arg0, s32 arg1);
char *Talk_LineText(u16 id);
extern u16 gTalkLine;
extern char *gTalkLineText;
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
s32 LobbyWorld_GetSeason(void); /* real: s32 (void); the call below sets a0 = 3, so this form keeps the code */
void CamReq_Kitchen(s32 a);
s16 Kitchen_EatDish(s16 a, u16 b, u16 c, u16 d);
void Kitchen_EatCourse(u16 a, u16 b);
void Items_Take(s32 item, s32 n);
void Save_AddMoney(s32 n);
s32 Menu_PushNoticeOnce(s32);
s32 Award_Grant(s32 a);
s32 RateFlag_SetAndNotify(s32 a);
/* Raw addresses kept here: the named form changes the code. */
void Kitchen_Update(void)
{
    Menu48 *m = (Menu48 *)gMenuWork;
    NpcWork *npc = (NpcWork *)((u8 *)m->npc + 0x438);
    u16 pad = LobbyPad_Get(0);
    s16 pts;
    u16 r;
    u32 sum;

    Chat_SetWindowMode(1, 2);
    if (Game_IsOnline() == 1 && m->count > 1 && (m->state < 9 || m->state == 0xD)) {
        if (--m->timer <= 0) {
            m->state = 0xE;
            Talk_Say(1, 0);
        }
    }
    switch (m->state) {
    case 0:
        if (Motion_ReachedFrame(m->player, 0, 30.0f) != 0) {
            m->state++;
            Npc_SetKitchenRoute(m->npc, m->unk91);
            Kitchen_Broadcast(m, 1);
        }
        break;
    case 1:
        if (((u8 *)m->npc)[0x15] == 0x59) {
            m->state++;
            if (m->unk90 == 0) {
                Talk_Say(0, 0);
            } else {
                Talk_Say(5, 0);
            }
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
            IngredientList_BuildPage((ListMenu *)m, 0);
            if (m->unk90 == 0 && m->unk88 < 2) {
                m->state = 0xE;
                Talk_Say(2, 0);
            } else if (m->count > 1) {
                m->state++;
                Talk_Say(7, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), m->count);
            } else if (m->unk90 == 1) {
                m->state = 7;
                WinSlide_Step(&m->win2, -0x118, 0x3C);
            } else {
                m->state = 4;
                WinSlide_Step(&m->win, 0x280, 0x118);
            }
            break;
        case 3:
            m->state = 0xE;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 3:
        switch (Talk_Update()) {
        case 0:
            if (m->unk90 == 1) {
                m->state = 7;
                WinSlide_Step(&m->win2, -0x118, 0x3C);
            } else {
                m->state++;
                WinSlide_Step(&m->win, 0x280, 0x118);
            }
            break;
        case 3:
            m->state = 0xD;
            m->unk94 = 3;
            Talk_Say(3, 1);
            break;
        }
        break;
    case 4:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->state++;
            m->unk78 = 0;
            break;
        }
        break;
    case 5:
        switch (Kitchen_ChooseIngredient((ListMenu *)m, pad)) {
        case 0:
            m->state++;
            m->unk7A = 0;
            break;
        case 3:
            m->state = 0xD;
            Talk_Say(3, 1);
            break;
        }
        break;
    case 6:
        switch (Kitchen_ChooseIngredient((ListMenu *)m, pad)) {
        case 0:
            m->state++;
            WinSlide_Step(&m->win2, -0x118, 0x3C);
            Menu_SetYesNo(1);
            LobbySe_Play(0x17);
            break;
        case 3:
            m->unk78 = 0;
            m->state = 5;
            break;
        }
        break;
    case 7:
        switch (WinSlide_Step(&m->win2, -0x118, 0x3C)) {
        case 0:
            m->state++;
            m->unk94 = 6;
            break;
        }
        break;
    case 8:
        switch (Menu_YesNoInput()) {
        case 0:
            m->state++;
            m->unk68 = LobbyWorld_GetSeason();
            if (Game_IsOnline() == 1 || gSave.unk36E8 == 0) {
                CamReq_Kitchen(m->unk91);
            } else {
                CamReq_Kitchen(3);
            }
            if (m->unk90 != 0) {
                r = Rand_Next(1) % 100;
                m->unk78 = 0;
                if (r < 40) {
                    m->unk7A = 0;
                } else if (r < 70) {
                    m->unk7A = 1;
                } else if (r < 90) {
                    m->unk7A = 2;
                } else {
                    m->unk7A = 3;
                }
                Tutorial_Start(2, m->mask, 4);
            } else {
                Tutorial_Start(2, m->mask, (gItemData[m->unk78].rarity + gItemData[m->unk7A].rarity + 2) / 2 - 1);
            }
            Kitchen_Broadcast(m, 2);
            break;
        case 3:
            if (m->unk90 == 1) {
                m->state = 0xD;
                Talk_Say(3, 1);
            } else {
                m->state = m->unk94;
                m->unk7A = 0;
                if (m->unk94 == 3) {
                    Talk_Say(7, 0);
                    gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), m->count);
                }
            }
            break;
        }
        break;
    case 9:
        if (Tutorial_IsDone() == 1) {
            m->state++;
            Actor_SetState(m->player, 0, 0x29);
        }
        break;
    case 10:
        if (m->player->motionId[0] == 0x261) {
            m->state++;
            if (m->unk90 == 0) {
                Items_Take(m->unk78, m->count);
                Items_Take(m->unk7A, m->count);
                pts = Kitchen_EatDish(m->unk68, m->unk78, m->unk7A, m->rand);
                if (Game_IsOnline() == 1 && m->count > 1 && *(u16 *)(gLobbyWorkBuf + 0x7A98) > 0 && (Rand_Next(1) & 1)) {
                    Save_AddMoney((*(u16 *)(gLobbyWorkBuf + 0x7A98)));
                    Menu_PushNoticeOnce(0x27);
                }
                sum = m->count + 10;
                sum *= pts;
                if (gSave.mealPoints < 310) {
                    gSave.mealPoints += sum;
                }
            } else {
                gSave.mealPoints = 0;
                Kitchen_EatCourse(m->unk7A, m->rand);
                npc->unk4 = 0;
            }
            if (m->unk90 != 0) {
                if (gSave.recordCounts[33] < 5000) {
                    gSave.recordCounts[33] += 1;
                    if (gSave.recordCounts[33] >= 50) {
                        Award_Grant(0x22);
                    }
                }
            }
            if (gSave.recordCounts[32] < 65000) {
                gSave.recordCounts[32] += m->count;
                if (gSave.recordCounts[32] >= 100) {
                    if (!(gSave.rateFlags & 0x400)) {
                        RateFlag_SetAndNotify(0xA);
                    }
                    Award_Grant(0x21);
                }
            }
            Actor_SetState(m->npc, 0, 0x55);
        }
        break;
    case 11:
        if (m->player->state == 1) {
            m->state++;
            Talk_Say(4, 0);
            Npc_AdvanceRoute(m->npc);
            Kitchen_Broadcast(m, 3);
        }
        break;
    case 12:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state = 0xF;
            break;
        }
        break;
    case 13:
        switch (Talk_Update()) {
        case 0:
            m->state = 0xE;
            Talk_Say(1, 0);
            break;
        case 3:
            m->state = 2;
            if (m->unk90 == 0) {
                Talk_Say(0, 0);
            } else {
                Talk_Say(5, 0);
            }
            break;
        }
        break;
    case 14:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state = 0xF;
            Actor_SetState(m->player, 0, 0x15);
            Npc_AdvanceRoute(m->npc);
            Kitchen_Broadcast(m, 3);
            break;
        }
        break;
    case 15:
        Kitchen_Close();
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6C4E40", D_008260B0);

s32 Kitchen_IsIngredient(u16 item)
{
    s32 i;
    s32 j;
    ItemRec *p;

    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (p = gKitchenIngredients[i], j = 0; p[j].item != 0; j++) {
            if (item == p[j].item) {
                return 1;
            }
        }
    }
    return 0;
}

static s32 Kitchen_IngredientGroup(u16 item)
{
    ItemRec *p;
    s16 i;
    s32 j;

    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (p = gKitchenIngredients[i], j = 0; p[j].item != 0; j++) {
            if (item == p[j].item) {
                return i;
            }
        }
    }
    return 0;
}

static ItemRec *Kitchen_FindIngredient(u16 item)
{
    s32 i;
    s16 j;
    ItemRec *p;
    ItemRec *def;

    def = gKitchenIngredients[0]; /* not found: the first group's first ingredient */
    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (p = gKitchenIngredients[i], j = 0; p[j].item != 0; j++) {
            if (item == p[j].item) {
                return &p[j];
            }
        }
    }
    return def;
}

s32 abs(s32 x);
s32 Kitchen_EffectLevel(s32 x)
{
    x = abs(x);

    if (x < 6) {
        return 0;
    }
    if (x < 11) {
        return 1;
    }
    if (x < 16) {
        return 2;
    }
    if (x < 21) {
        return 3;
    }
    return x < 26 ? 4 : 5;
}

extern s16 gMealBonus[];
extern s16 D_007E7B2C[];
extern s16 D_007E7B38[];
extern s16 D_007E7B44[];
extern char *gMealHealthTexts[];
extern char *D_0080C90C;
extern char *gMealStaminaTexts[];
extern char *D_0080C914;
extern char *gMealAttackTexts[];
extern char *D_0080C91C;
extern char *gMealDefenseTexts[];
extern char *D_0080C924;
extern char **gMealTasteTexts[];
u8 Notice_PushItem(char *);
void Kitchen_EatCourse(u16 kind, u16 r)
{
    s32 up;
    s32 down;
    s32 d = 0;
    s32 pos;
    s16 k;
    s32 neg;
    s32 c = 0;
    Player *pl = &gPlayers[gScene.localPlayer];
    s32 msg;
    s32 a = 0;
    s32 b = 0;

    switch (kind) {
    default:
        c = 10;
        d = c;
        break;
    case 1:
        a = 0x14;
        b = a;
        break;
    case 2:
        c = 0x19;
        d = c;
        break;
    case 3:
        a = 0x1E;
        b = a;
        break;
    }
    neg = 0;
    pos = 0;
    if (a > 0) {
        pos += a;
    } else {
        neg -= a;
    }
    if (b > 0) {
        pos += b;
    } else {
        neg -= b;
    }
    if (c > 0) {
        pos += c;
    } else {
        neg -= c;
    }
    if (d > 0) {
        pos += d;
    } else {
        neg -= d;
    }
    up = 0;
    down = 0;
    k = Kitchen_EffectLevel(a);
    if (a < 0) {
        gScene.mealStamina = -gMealBonus[k];
    } else {
        gScene.mealStamina = gMealBonus[k];
    }
    if (gScene.mealStamina > 0) {
        Notice_PushItem(gMealStaminaTexts[0]);
        up += gScene.mealStamina;
    } else if (gScene.mealStamina < 0) {
        Notice_PushItem(D_0080C914);
        down += gScene.mealStamina;
    }
    k = Kitchen_EffectLevel(b);
    if (b < 0) {
        gScene.mealHealth = -D_007E7B2C[k];
    } else {
        gScene.mealHealth = D_007E7B2C[k];
    }
    if (gScene.mealHealth > 0) {
        Notice_PushItem(gMealHealthTexts[0]);
        up += gScene.mealHealth;
    } else if (gScene.mealHealth < 0) {
        Notice_PushItem(D_0080C90C);
        down += gScene.mealHealth;
    }
    k = Kitchen_EffectLevel(c);
    if (c < 0) {
        gScene.mealAttack = -D_007E7B38[k];
    } else {
        gScene.mealAttack = D_007E7B38[k];
    }
    if (gScene.mealAttack > 0) {
        Notice_PushItem(gMealAttackTexts[0]);
        up += gScene.mealAttack;
    } else if (gScene.mealAttack < 0) {
        Notice_PushItem(D_0080C91C);
        down += gScene.mealAttack;
    }
    k = Kitchen_EffectLevel(d);
    if (d < 0) {
        gScene.mealDefense = -D_007E7B44[k];
    } else {
        gScene.mealDefense = D_007E7B44[k];
    }
    if (gScene.mealDefense > 0) {
        Notice_PushItem(gMealDefenseTexts[0]);
        up += gScene.mealDefense;
    } else if (gScene.mealDefense < 0) {
        Notice_PushItem(D_0080C924);
        down += gScene.mealDefense;
    }
    if (up == 0 && down == 0) {
        msg = 3;
        Actor_SetState(pl, 0, 0x27);
    } else if (down == 0) {
        if (pos >= 0x15) {
            msg = 0;
        } else {
            msg = 1;
        }
        Actor_SetState(pl, 0, 0x18);
    } else if (up == 0) {
        if (neg >= 0x15) {
            msg = 6;
        } else {
            msg = 5;
        }
        Actor_SetState(pl, 0, 0x2A);
    } else {
        if (neg < pos) {
            msg = 2;
        } else {
            msg = 4;
        }
        Actor_SetState(pl, 0, 0x27);
    }
    Notice_PushItem(gMealTasteTexts[msg][r % 4]);
    gLobby->mealEaten = 1;
}

extern s8 gMealStatKinds[][2];
extern s8 gDishRolls[];

/* The dish's rank (1-3) from its two ingredients' rarity (unk2 3 and up):
 * inlined in Kitchen_EatDish (its return 1 shares the register of the 1
 * stored before it, and is converted to s16 there). */
static inline s16 Kitchen_DishRank(u16 b, u16 c)
{
    if (gItemData[b].rarity >= 3) {
        if (gItemData[c].rarity >= 3) {
            return 3;
        }
        return 2;
    }
    if (gItemData[c].rarity >= 3) {
        return 2;
    }
    return 1;
}

/* The effect of a dish of ingredients b and c at grade a: the four stat changes (as
 * Kitchen_EatCourse), the notices, and the dish's rank (Kitchen_DishRank). It
 * matches only with Kitchen_IngredientGroup and Kitchen_FindIngredient static and C
 * in this file, before it. */
s16 Kitchen_EatDish(s16 a, u16 b, u16 c, u16 d)
{
    s32 v2;
    s32 v3;
    s16 k;
    s32 up;
    s32 down;
    f32 r;
    ItemRec *rb;
    s32 v1;
    s32 kb;
    s32 pos;
    s32 neg;
    s32 msg;
    s32 kc;
    s32 v0;
    s32 e2;
    s32 e3;
    Player *pl;
    s32 e1;
    s32 e0;
    ItemRec *rc;

    pl = &gPlayers[gScene.localPlayer];
    kb = Kitchen_IngredientGroup(b);
    kc = Kitchen_IngredientGroup(c);
    rb = Kitchen_FindIngredient(b);
    rc = Kitchen_FindIngredient(c);
    v2 = 0;
    v3 = 0;
    v1 = 0;
    v0 = 0;
    switch (gMealStatKinds[kb][0]) {
    case 0:
        v0 += rb->stat[a][0];
        break;
    case 1:
        v1 += rb->stat[a][0];
        break;
    case 2:
        v2 += rb->stat[a][0];
        break;
    case 3:
        v3 += rb->stat[a][0];
        break;
    }
    switch (gMealStatKinds[kb][1]) {
    case 0:
        v0 += rb->stat[a][1];
        break;
    case 1:
        v1 += rb->stat[a][1];
        break;
    case 2:
        v2 += rb->stat[a][1];
        break;
    case 3:
        v3 += rb->stat[a][1];
        break;
    }
    switch (gMealStatKinds[kc][0]) {
    case 0:
        v0 += rc->stat[a][0];
        break;
    case 1:
        v1 += rc->stat[a][0];
        break;
    case 2:
        v2 += rc->stat[a][0];
        break;
    case 3:
        v3 += rc->stat[a][0];
        break;
    }
    switch (gMealStatKinds[kc][1]) {
    case 0:
        v0 += rc->stat[a][1];
        break;
    case 1:
        v1 += rc->stat[a][1];
        break;
    case 2:
        v2 += rc->stat[a][1];
        break;
    case 3:
        v3 += rc->stat[a][1];
        break;
    }
    r = gDishRolls[(b + c) % 16];
    r *= 0.1f;
    neg = 0;
    e0 = v0 * r;
    e1 = v1 * r;
    e2 = v2 * r;
    e3 = v3 * r;
    pos = 0;
    if (e0 > 0) {
        pos += e0;
    } else {
        neg -= e0;
    }
    if (e1 > 0) {
        pos += e1;
    } else {
        neg -= e1;
    }
    if (e2 > 0) {
        pos += e2;
    } else {
        neg -= e2;
    }
    if (e3 > 0) {
        pos += e3;
    } else {
        neg -= e3;
    }
    up = 0;
    down = 0;
    k = Kitchen_EffectLevel(e0);
    if (e0 < 0) {
        gScene.mealStamina = -gMealBonus[k];
    } else {
        gScene.mealStamina = gMealBonus[k];
    }
    if (gScene.mealStamina > 0) {
        Notice_PushItem(gMealStaminaTexts[0]);
        up += gScene.mealStamina;
    } else if (gScene.mealStamina < 0) {
        Notice_PushItem(D_0080C914);
        down += gScene.mealStamina;
    }
    k = Kitchen_EffectLevel(e1);
    if (e1 < 0) {
        gScene.mealHealth = -D_007E7B2C[k];
    } else {
        gScene.mealHealth = D_007E7B2C[k];
    }
    if (gScene.mealHealth > 0) {
        Notice_PushItem(gMealHealthTexts[0]);
        up += gScene.mealHealth;
    } else if (gScene.mealHealth < 0) {
        Notice_PushItem(D_0080C90C);
        down += gScene.mealHealth;
    }
    k = Kitchen_EffectLevel(e2);
    if (e2 < 0) {
        gScene.mealAttack = -D_007E7B38[k];
    } else {
        gScene.mealAttack = D_007E7B38[k];
    }
    if (gScene.mealAttack > 0) {
        Notice_PushItem(gMealAttackTexts[0]);
        up += gScene.mealAttack;
    } else if (gScene.mealAttack < 0) {
        Notice_PushItem(D_0080C91C);
        down += gScene.mealAttack;
    }
    k = Kitchen_EffectLevel(e3);
    if (e3 < 0) {
        gScene.mealDefense = -D_007E7B44[k];
    } else {
        gScene.mealDefense = D_007E7B44[k];
    }
    if (gScene.mealDefense > 0) {
        Notice_PushItem(gMealDefenseTexts[0]);
        up += gScene.mealDefense;
    } else if (gScene.mealDefense < 0) {
        Notice_PushItem(D_0080C924);
        down += gScene.mealDefense;
    }
    if (up == 0 && down == 0) {
        msg = 3;
        Actor_SetState(pl, 0, 0x27);
    } else if (down == 0) {
        if (pos >= 0x15) {
            msg = 0;
        } else {
            msg = 1;
        }
        Actor_SetState(pl, 0, 0x18);
    } else if (up == 0) {
        if (neg >= 0x15) {
            msg = 6;
        } else {
            msg = 5;
        }
        Actor_SetState(pl, 0, 0x2A);
    } else {
        if (neg < pos) {
            msg = 2;
        } else {
            msg = 4;
        }
        Actor_SetState(pl, 0, 0x27);
    }
    Notice_PushItem(gMealTasteTexts[msg][d % 4]);
    gLobby->mealEaten = 1;
    return Kitchen_DishRank(b, c);
}

s32 Items_HaveEnough(u16 item, s32 n);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void IngredientList_BuildPage(ListMenu *m, s16 page);
s32 Kitchen_ChooseIngredient(ListMenu *m, u16 pad)
{
    if (pad & 0x20) {
        if (Items_HaveEnough(m->items[m->cursor], m->unk84) != 0) {
            switch (m->state) {
            case 5:
                m->unk78 = m->items[m->cursor];
                LobbySe_Play(8);
                return 0;
            case 6:
                if (m->unk78 != m->items[m->cursor]) {
                    m->unk7A = m->items[m->cursor];
                    LobbySe_Play(8);
                    return 0;
                }
                LobbySe_Play(9);
                break;
            }
        } else {
            LobbySe_Play(9);
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        if (m->count != 0) {
            m->unk92 ^= 1;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0xC00) {
        if (m->pages > 1) {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
            IngredientList_BuildPage(m, m->page);
        }
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
    }
    return 2;
}

extern char *gIngredientABTexts[];
extern char *gItemNames[];
void Win_Draw(s32, s32, s32, s32, s32, s32, s16);
void Font_SetColor(s32 color);
void Kitchen_DrawFirstItem(u8 *m, s32 x, s32 y)
{
    s16 py = y;
    s16 px;

    Font_SetSpacing(0x12, 0x12);
    Win_Draw(x, y, 0xC8, 0x3C, 0xBDB59F6C, 0xBB402010, 3);
    Font_SetColor(0x807F7F28);
    px = x;
    Font_SetPos(px + 10, py += 10);
    Text_PrintfColored(D_008260B0, gIngredientABTexts[0]);
    if (*(u16 *)(m + 0x78) != 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(px + 0x1E, py + 0x16);
        Text_PrintfColored(D_008260B0, gItemNames[*(u16 *)(m + 0x78)]);
    }
}

extern char *D_0080C904;
void Kitchen_DrawSecondItem(u8 *m, s32 x, s32 y)
{
    s16 py = y;
    s16 px;

    Font_SetSpacing(0x12, 0x12);
    Win_Draw(x, y, 0xC8, 0x3C, 0xBDB59F6C, 0xBB402010, 3);
    Font_SetColor(0x807F7F28);
    px = x;
    Font_SetPos(px + 10, py += 10);
    Text_PrintfColored(D_008260B0, D_0080C904);
    if (*(u16 *)(m + 0x7A) != 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(px + 0x1E, py + 0x16);
        Text_PrintfColored(D_008260B0, gItemNames[*(u16 *)(m + 0x7A)]);
    }
}

extern char *gMealCourseTexts[];
extern char *D_0080C944;
void Kitchen_DrawCourse(s16 x, s16 y)
{
    char *s;

    Font_SetSpacing(0x18, 0x18);
    s = gMealCourseTexts[0];
    Font_SetPos(Text_CenterX(s, x + 0xA0, 0x18), y += 0x54);
    Text_PrintfColored(D_008260B0, s);
    s = D_0080C944;
    Font_SetPos(Text_CenterX(s, x + 0xA0, 0x18), y + 0x3C);
    Text_PrintfColored(D_008260B0, s);
}

extern char *D_0080C92C;
extern char *D_0080C934;
extern char *gMealNameTexts[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Text_Printf(const char *fmt, ...);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *); /* real: void Win_DrawText(s16, s16, s16, s16, u32, u32, char *); this form keeps the code */
/* FAKEMATCH: the second bar of each column adds an unsigned constant (10U, 0x10AU). The game
 * extends x once (a common subexpression, s6) but computes x + 10 and x + 0x10A again for the
 * second bar: two `x + 10` of one type are shared (123 words), an int copy of x is a variable
 * of its own numbered before the parameters (38: x, y, a, b one register up), and an s16 copy,
 * a static inline bar helper, `L` suffixes or casts change nothing or worse. Two expressions of
 * different types keep the extension shared and the sums apart: MATCH (round 29). */
void Kitchen_DrawConfirm(s16 x, s16 y, u16 a, u16 b)
{
    s16 tx;
    s16 k;
    s16 by;
    s16 ty = y;

    Font_SetSpacing(0x14, 0x14);
    Icon_Draw(x, y, 0x140, 0x104, 0xFFD4BF9F, 0);
    by = ty + 0x82;
    Icon_Draw(x + 10, by - 0x5A, 0x100, 0x20, 0x803E2502, 0x1B);
    Icon_Draw(x + 10U, by + 0x46, 0x100, 0x20, 0x803E2502, 0x1B);
    Icon_Draw(x + 0x10A, by - 0x5A, 0x2E, 0x20, 0x803E2502, 0x1C);
    Icon_Draw(x + 0x10AU, by + 0x46, 0x2E, 0x20, 0x803E2502, 0x1C);
    Font_SetColor(0x80101010);
    if (a == 0) {
        Kitchen_DrawCourse(x, y);
        if (gTownMode != 0x49) {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C934);
            Font_SetPos(0x1A2, 0x192);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        }
    } else {
        tx = x + 0x24;
        Font_SetPos(tx, ty += 0x4A);
        Text_PrintfColored("%s%s", gIngredientABTexts[0], gItemNames[a]);
        Font_SetPos(tx, ty += 0x30);
        Text_PrintfColored("%s%s", D_0080C904, gItemNames[b]);
        Font_SetColor(0x80301010);
        Font_SetPos(tx, ty + 0x30);
        k = (a + b) % 16;
        Text_PrintfColored("%s%s", D_0080C92C, gMealNameTexts[k % 16]);
        if (gTownMode != 0x49) {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C934);
            Font_SetPos(0x1A2, 0x192);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        }
    }
}

void Text_Printf(const char *fmt, ...);
void Kitchen_DrawIngredients(struct ListMenu *m, s16 x, s16 y);
void Kitchen_DrawFirstItem(u8 *m, s32 x, s32 y);
void Kitchen_DrawSecondItem(u8 *m, s32 x, s32 y);
void Kitchen_DrawConfirm(s16 x, s16 y, u16 a, u16 b);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *); /* real: void Win_DrawText(s16, s16, s16, s16, u32, u32, char *); this form keeps the code */
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
extern char *gMealPromptTexts[];
extern char *gMealPartyPromptTexts[];
extern char *D_0080C93C;
extern char *D_0080C20C;
void Kitchen_Draw(void *task)
{
    Menu48 *m = (Menu48 *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    switch (m->state) {
    case 4:
        Kitchen_DrawIngredients((struct ListMenu *)m, m->win.pos, 0x3A);
        if (m->count > 1) {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gMealPartyPromptTexts[0]);
        } else {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gMealPromptTexts[0]);
        }
        break;
    case 5:
        Kitchen_DrawIngredients((struct ListMenu *)m, m->win.pos, 0x3A);
        Kitchen_DrawFirstItem((u8 *)m, (s16)(m->win.pos - 200), 0x3A);
        if (m->count > 1) {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gMealPartyPromptTexts[0]);
        } else {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gMealPromptTexts[0]);
        }
        break;
    case 6:
        Kitchen_DrawIngredients((struct ListMenu *)m, m->win.pos, 0x3A);
        Kitchen_DrawFirstItem((u8 *)m, (s16)(m->win.pos - 200), 0x3A);
        Kitchen_DrawSecondItem((u8 *)m, (s16)(m->win.pos - 200), 0x76);
        if (m->count > 1) {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gMealPartyPromptTexts[0]);
        } else {
            Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gMealPromptTexts[0]);
        }
        break;
    case 8:
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(0x122, 0x12, 0xA0, 0xFFB0B000, D_0080C93C, 2, 1);
    case 7:
        Kitchen_DrawConfirm(0x122, m->win2.pos, m->unk78, m->unk7A);
        break;
    case 9:
        break;
    }
    if (Game_IsOnline() == 1 && m->count > 1 && (m->state < 9 || m->state == 13)) {
        Win_Draw(0x1EA, 0x14, 0x78, 0x16, 0xBDB59F6C, 0x80202020, 6);
        Font_SetPos(0x1EA, 0x16);
        Font_SetSpacing(0x14, 0x14);
        Font_SetColor(0x807F7F28);
        Text_Printf(D_0080C20C, m->timer / 30 + 1);
    }
}

/* unk_6C4E40's .data (0x7E7960-0x7E7B70) */
ItemRec gKitchenIngredients_0[6] = {
#include "lobby/007E7960.inc"
};
ItemRec gKitchenIngredients_1[6] = {
#include "lobby/007E7990.inc"
};
ItemRec gKitchenIngredients_2[6] = {
#include "lobby/007E79C0.inc"
};
ItemRec gKitchenIngredients_3[6] = {
#include "lobby/007E79F0.inc"
};
ItemRec gKitchenIngredients_4[6] = {
#include "lobby/007E7A20.inc"
};
ItemRec gKitchenIngredients_5[4] = {
#include "lobby/007E7A50.inc"
};
ItemRec gKitchenIngredients_6[4] = {
#include "lobby/007E7A70.inc"
};
ItemRec gKitchenIngredients_7[4] = {
#include "lobby/007E7A90.inc"
};
ItemRec gKitchenIngredients_8[2] = {
#include "lobby/007E7AB0.inc"
};
ItemRec gKitchenIngredients_9[2] = {
#include "lobby/007E7AC0.inc"
};
ItemRec *gKitchenIngredients[12] = {
    gKitchenIngredients_0, gKitchenIngredients_1, gKitchenIngredients_2, gKitchenIngredients_3,
    gKitchenIngredients_4, gKitchenIngredients_5, gKitchenIngredients_6, gKitchenIngredients_7,
    gKitchenIngredients_8, gKitchenIngredients_9, NULL, NULL,
};
s8 gMealStatKinds[16][2] = {
#include "lobby/007E7B00.inc"
};
s16 gMealBonus[32] = {
#include "lobby/007E7B20.inc"
};
s8 gDishRolls[16] = {
#include "lobby/007E7B60.inc"
};
