/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "area.h"
#pragma optimization_level 4
void *memset(void *, s32, u32);

/* The pick menu (town mode 0x4C, gMenuWork: PickMenu_Open, Update, Draw) */
typedef struct PickSeq {
    u8 *npc;        /* 0x0: the npc offering (an OfferTask) */
    s16 cursor;     /* 0x4 */
    s16 count;      /* 0x6: the choices: 2 when OfferMenu_BuildList finds offers */
    s8 state;       /* 0x8 */
} PickSeq;

void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_PushNoticeOnce(s32 arg0);
extern s32 gTownMode;
extern u8 gMenuWork[];
u8 Village_FacilityLevel(s32);
s32 Cond_Check(u16);
void CamFocus_End(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMenuOpen;
void LobbyMenu_Init(void);
s32 Items_HaveEnough(u16, s32);

s32 func_006B5300(void) {
    if ((gTownMode == 0x4C) && (((PickSeq *)gMenuWork)->state == 3)) {
        return 1;
    }
    return 0;
}

s32 func_006B5340(void) {
    if ((Cond_Check(0x1A) == 1) && ((Village_FacilityLevel(3) & 0xFF) == 2)) {
        return 1;
    }
    return 0;
}

extern u8 gMenuInputLock;
struct OfferTask;
s16 OfferMenu_BuildList(struct OfferTask *t, s32 b);
void PickMenu_Draw(MenuTask *t);
void PickMenu_Open(u8 *a)
{
    PickSeq *m;

    gTownMenuOpen = 1;
    m = (PickSeq *)gMenuWork;
    gTownMode = 0x4C;
    memset(m, 0, 0xF00);
    m->npc = a;
    if (OfferMenu_BuildList((struct OfferTask *) a, 0) != 0) {
        m->count = 2;
    } else {
        m->count = 1;
    }
    LobbyMenu_SetDraw(PickMenu_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    LobbySe_Play(0x11);
}

void PickMenu_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
}

u16 LobbyPad_Get(s32 a);
s32 PickMenu_ChoiceInput(PickSeq *m, s32 pad);
void Talk_Say(s32 msg, s8 a);
void OfferMenu_Open(struct OfferTask *t);
s32 Talk_Update(void);
void CamReq_Lobby44(s32 n);
void Jingle_Play(void *obj, s32 id);
s32 CamReq_State(void);
void FadeA_Start(s32 a);
u8 FadeA_GetState(void);
void AreaMenu_Open(u16 id);


void PickMenu_Update(void)
{
    PickSeq *m = (PickSeq *)gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];
    s32 pad = LobbyPad_Get(0);
    s32 r;

    switch (m->state) {
    case 0:
        switch (PickMenu_ChoiceInput(m, pad)) {
        case 0:
            if (m->cursor == 0) {
                m->state += 1;
                Talk_Say(3, 1);
            } else {
                OfferMenu_Open((struct OfferTask *) m->npc);
            }
            break;
        case 3:
            m->state = 5;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            m->state += 1;
            Talk_Say(4, 0);
            gFieldControl = 0;
            break;
        case 3:
            m->state = 5;
            Talk_Say(5, 0);
            break;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state += 1;
            CamReq_Lobby44(0xE);
            Jingle_Play(p, 0xD9);
            p->visible = 0;
            gScene.mealAttack = 0;
            gScene.mealDefense = 0;
            gScene.mealHealth = 0;
            gScene.mealStamina = 0;
            gLobby->mealEaten = 0;
            break;
        }
        break;
    case 3:
        p->visible = 0;
        switch (CamReq_State()) {
        case 0:
        case -1:
            FadeA_Start(1);
            m->state += 1;
            break;
        }
        break;
    case 4:
        p->visible = 0;
        if (FadeA_GetState() != 1) {
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = 0;
            AreaMenu_Open(AREA_HUB_0x57);
        }
        break;
    case 5:
        switch (Talk_Update()) {
        case 0:
        case 3:
            PickMenu_Close();
            break;
        }
        break;
    }
}

s32 PickMenu_ChoiceInput(PickSeq *m, s32 pad)
{
    s32 btn;

    btn = pad & 0xFFFF;
    if (btn & 0x20) {
        if ((m->cursor == 0) && (gLobby->questTaken != 0)) {
            Menu_PushNoticeOnce(1);
            LobbySe_Play(9);
            goto block_9;
        }
        LobbySe_Play(8);
        return 0;
    }
    if (btn & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    m->cursor = Menu_WrapUpDown((s32) m->cursor, (s32) m->count);
block_9:
    return 2;
}

extern char *gBoardShipTexts[];
void Text_SetSize(s32 size);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void PickMenu_Draw(MenuTask *t)
{
    PickSeq *m = (PickSeq *)gMenuWork;

    Text_SetSize(t->textSize);
    switch (m->state) {
    case 0:
        Menu_DrawLabelList(0x1CC, gBoardShipTexts, m->count, m->cursor, 0xFFB0B000);
        break;
    }
}

void Talk_Say(s32 msg, s8 a);
void Mine_DrawMenu(MenuTask *t);
void Mine_OpenMenu(u8 *a)
{
    u8 *m;

    gTownMenuOpen = 1;
    m = (u8 *)gMenuWork;
    gTownMode = 0x33;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Mine_DrawMenu, 0, 0, 0);
    *(s32 *)(m + 0x10) = (s32) a;
    if (gSave.mine.active == 2) {
        switch (gSave.mine.digs) {
        case 1:
            Talk_Say(4, 0);
            break;
        case 2:
        case 3:
            Talk_Say(5, 0);
            break;
        default:
            Talk_Say(6, 0);
            break;
        }
        m[0x19] = 5;
    } else {
        Talk_Say(1, 0);
    }
    gMenuInputLock = 6;
}

void Mine_CloseMenu(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
}

s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
void PickMenu_Build(PickMenu *m)
{
    WinSlide_Step(&m->win, 0x280, 0x118);
    memset(m->items, 0, 6);
    m->count = 0;
    if (Items_HaveEnough(0x96, 1) == 1) {
        m->items[m->count] = 0x96;
        m->count++;
    }
    if (Items_HaveEnough(0x97, 1) == 1) {
        m->items[m->count] = 0x97;
        m->count++;
    }
    if (Items_HaveEnough(0x98, 1) == 1) {
        m->items[m->count] = 0x98;
        m->count++;
    }
}

typedef struct MineRow {
    u8 unk0[0xC];
    u16 unkC;
    u8 unkE[0x14 - 0xE];
} MineRow;
typedef struct MineDig {
    u8 unk0[0xC];
    MineRow *rows;
    u8 unk10[0x3C - 0x10];
    s16 row;
} MineDig;
extern MineRow gMineRowsHigh[];
extern MineRow gMineRowsLow[];
extern MineRow gMineRowsMid[];
s32 PickMenu_Input(PickMenu *m, s32 pad); /* unit's own declaration: with the definition's s32 (PickMenu *m, u16 pad) (lobby/unk_6B5E70.c), Mine_UpdateMenu 9 of 207 words differ */
void Mine_Dig(PickMenu *m);
void Actor_SetState(void *, s32, s32);
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
u8 Notice_Push(u8, s16, s16);
void Menu_PushNotice(s32 a);
typedef struct MineMenu {
    WinSlide win;
    u8 unk8[0x8];
    u8 *owner;
    u8 unk14[5];
    u8 state;
} MineMenu;
/* Raw addresses kept here: the named form changes the code. */
void Mine_UpdateMenu(void)
{
    MineMenu *m = (MineMenu *)gMenuWork;
    MineDig *d = (MineDig *)(m->owner + 0x438);
    s32 pad = LobbyPad_Get(0);

    switch (m->state) {
    case 0:
        switch (Talk_Update()) {
        case 0:
            m->state += 1;
            PickMenu_Build((PickMenu *)m);
            break;
        case 3:
            m->state = 4;
            Talk_Say(0, 0);
            break;
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->state += 1;
            break;
        }
        break;
    case 2:
        switch (PickMenu_Input((PickMenu *)m, pad)) {
        case 0:
            m->state += 1;
            Talk_Say(3, 0);
            Mine_Dig((PickMenu *) m);
            d->row = 0;
            if ((u8)Village_FacilityLevel(0) >= 6) {
                d->rows = gMineRowsHigh;
            } else if ((u8)Village_FacilityLevel(0) < 3) {
                d->rows = gMineRowsLow;
            } else {
                d->rows = gMineRowsMid;
            }
            Actor_SetState(m->owner, 0, d->rows[d->row].unkC);
            break;
        case 3:
            m->state = 4;
            Talk_Say(0, 0);
            break;
        }
        break;
    case 3:
        switch (Talk_Update()) {
        case 0:
        case 3:
            Mine_CloseMenu();
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
        case 3:
            Mine_CloseMenu();
            break;
        }
        break;
    case 5:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state += 1;
            Talk_Say(7, 0);
            break;
        }
        break;
    case 6:
        switch (Talk_Update()) {
        case 0:
        case 3:
            if (SavePouch_Space(gSave.mine.find) > 0) {
                SavePouch_Add(gSave.mine.find, 1);
                Notice_Push(1, 0xD, *(s16 *)&gSave.mine.find);
                gSave.mine.find = 0;
                gSave.mine.digs = 0;
                gSave.mine.active = 0;
            } else {
                Menu_PushNotice(0x15);
            }
            Mine_CloseMenu();
            break;
        }
        break;
    }
}

s32 Mine_IsOpen(void) {
    if ((Cond_Check(0x15) == 1) && (Cond_Check(0x19) == 0) && (Items_HaveEnough(0x2DA, 1) == 0)) {
        return 1;
    }
    return 0;
}
