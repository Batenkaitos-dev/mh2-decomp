#include "common.h"
#include "window.h"
#include "syssettings.h"
#include "item.h"
#include "lobby_state.h"
#include "m2c_macros.h"

#include "lobby_menu.h"
#include "lobby_globals.h"
#include "pad.h"
#include "player.h"
#include "lobby_npc.h"
#include "em.h"
#include "scene.h"
#include "quest.h"
#include "save.h"
#include "lobby_text.h"
#include "lobby/unk_6CB690.h"   /* the texts this file uses (tools/content_gen.py) */
#include "area.h"
s32 PoogieMenu_Feed(void);
typedef struct PoogieTbl {
    u16 id;
    s16 flag;
    s16 value;
} PoogieTbl;
typedef struct PointLine {
    /* 0x0 */ u16 item;
    /* 0x2 */ u16 pad;
    /* 0x4 */ s32 price;
} PointLine;
typedef struct PointShop {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ PointLine *tbl;      /* the stock (gPointShopStock0[0]..D_007E7E84) */
    /* 0x14 */ PointLine line[8];   /* the page shown (7 lines) */
    /* 0x54 */ u8 unk54[0x48];
    /* 0x9C */ s16 page;
    /* 0x9E */ s16 pages;
    /* 0xA0 */ u8 unkA0[8];
    /* 0xA8 */ s16 count;           /* lines on the page */
    /* 0xAA */ s16 unkAA;           /* how many to take */
    /* 0xAC */ s16 cursor;
    /* 0xAE */ u8 unkAE[2];
    /* 0xB0 */ s8 state;
    /* 0xB1 */ u8 unkB1[4];
    /* 0xB5 */ s8 unkB5;            /* flipped by button 0x200 */
} PointShop;
typedef struct PoogieMenu {
    /* 0x00 */ u8 unk0[5];
    /* 0x05 */ u8 sel;              /* cursor among the outfits owned */
    /* 0x06 */ u8 unk6[2];
    /* 0x08 */ Em *task;             /* the Poogie */
    /* 0x0C */ u8 owned[5];         /* 1: outfit owned (outfit 0 always) */
    /* 0x11 */ u8 pad11[3];
    /* 0x14 */ char *lines[5];      /* the names (gOutfitNames) of the outfits owned */
    /* 0x28 */ u8 count;
    /* 0x29 */ s8 unk29;
    /* 0x2A */ WinSlide win;
    /* 0x32 */ u8 unk32[0x2];
    /* 0x34 */ s32 items[20];       /* pouch items accepted by Kitchen_IsIngredient */
    /* 0x84 */ s32 prices[20];      /* and their prices */
    /* 0xD4 */ u8 rows;             /* list rows in all, 7 per page */
    /* 0xD5 */ u8 unkD5[2];
    /* 0xD7 */ s8 page;
    /* 0xD8 */ s8 pages;
    /* 0xD9 */ u8 unkD9;
    /* 0xDA */ s16 unkDA;
    /* 0xDC */ u8 unkDC;
    /* 0xDD */ s8 unkDD;            /* frames since the menu opened (6 at most) */
    /* 0xDE */ u8 unkDE;
    /* 0xDF */ u8 unkDF;
    /* 0xE0 */ Em *task2;            /* the Poogie (PoogieMenu_OpenCare) */
    /* 0xE4 */ s8 unkE4;
    /* 0xE5 */ u8 unkE5;
    /* 0xE6 */ s16 mood[4];        /* copied from 0x8BD6C4 (the mood bars?) */
    /* 0xEE */ u8 unkEE;
} PoogieMenu;

typedef struct DojoMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ WinShake shake;        /* the window's shake (Win_Shake): dx, dy */
    /* 0x10 */ u8 unk10[0x3C];
    /* 0x4C */ Msg650B req;         /* the posting (LobbyLevel2_Send650B) */
    /* 0x64 */ void *npc;           /* the instructor (OfferMenu_Open) */
    /* 0x68 */ s32 unk68;           /* Quest_MatchesSeasonTime's arguments */
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ u16 list[12];
    /* 0x88 */ s16 choice[4];       /* what each line of the first choice does */
    /* 0x90 */ s16 unk90;           /* the cursor of the online settings (DojoMenu_OnlineSettings) */
    /* 0x92 */ s16 unk92;
    /* 0x94 */ s16 cat;             /* cursor in the categories */
    /* 0x96 */ s16 unk96;
    /* 0x98 */ s16 cats;            /* 3, or 4 once check 3 is met */
    /* 0x9A */ s16 unk9A;
    /* 0x9C */ s16 unk9C;
    /* 0x9E */ s16 unk9E;
    /* 0xA0 */ s16 unkA0;
    /* 0xA2 */ s16 unkA2;
    /* 0xA4 */ s16 count;
    /* 0xA6 */ s16 unkA6;
    /* 0xA8 */ u8 unkA8[6];
    /* 0xAE */ s8 mode;
    /* 0xAF */ s8 step;
    /* 0xB0 */ s8 state;
    /* 0xB1 */ s8 unkB1;            /* the online start sequence (DojoMenu_OnlineStart) */
    /* 0xB2 */ s8 unkB2;
    /* 0xB3 */ s8 next;             /* the step after a window slide */
    /* 0xB4 */ s8 unkB4;
    /* 0xB5 */ s8 unkB5;
    /* 0xB6 */ s8 unkB6;            /* 1: the quest was started (DojoMenu_QuestInput) */
    /* 0xB7 */ s8 unkB7;
} DojoMenu;
typedef struct Milestone {
    /* 0x00 */ u32 div;
    /* 0x04 */ s16 flag;
    /* 0x06 */ s16 time;             /* into SaveData.villageTimers[].left */
} Milestone;

extern u8 gMenuWork[];
extern char *gPoogieCareLabels[];
void Text_SetSize(s32 a);
void Menu_DrawLabelList(s16 x, s8 **b, s16 n, s16 cur, u32 color);
void PoogieMenu_DrawFeedList(s16 x, s16 y);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s16 gPoogieMoods;
extern s16 D_008BD6C6;
extern s16 D_008BD6C8;
extern s16 D_008BD6CA;
s32 PoogieMenu_CheckOutfits(u8 *owned);
u8 PoogieMenu_LastCursor(void);
u8 Notice_PushItem(char *);
void Actor_SetState(void *obj, s32 group, s32 state); /* real: void (void *obj, s32 group, s32 state); this file passes the object as an int */
void LobbySe_Play(s32 se);
void CamFocus_Start(Em *obj);
void LobbyMenu_SetDraw(void *draw0, void *draw1, void *draw2, void *draw3);
void PoogieMenu_Draw(void);
extern PoogieTbl gVillageNpcRecsB[];
s32 Cond_Check(u16 id);
s32 Village_FacilityEventPending(void);
extern f32 gGatherZoneBounds[];
u8 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius);
void PoogieMenu_DrawCare(void);
extern u8 gNpcPresenceRates[];
s32 LobbyTime_IsAm(void);
s32 LobbyWorld_GetSeason(void);
u16 Rand_Next(s32 stream);
extern s16 gCookRangeDefault[];
extern s16 gCookRangeKind1[];
u8 Village_AreaKind(void);
extern s32 gGatherValuesA[];
extern u8 gGatherValueRatesA[];
extern s32 gGatherValuesB[];
extern u8 gGatherValueRatesB[];
extern u8 gGatherLimitReqA[];
extern u8 gGatherLimitReqB[];
extern u8 gPoogieDigZone[];
extern s32 gPoogieDigSeason;
extern u8 gSeasonItemCounts[];
extern u8 D_007E8301[];
extern u8 D_007E8302;
extern u16 gSeasonItems0[];
extern u16 gSeasonItems1[];
extern u16 gSeasonItems2[];
extern ListPanel gOutfitListBox;
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 a);
void Gfx_SetTexFilter(s32 a);
s32 Gs_SetState(u8, u32);
void Gfx_SetBlend(s32 a, s32 b);
s32 Shake_Update(u8);
void *Shake_GetOffset(u8 id);
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void PoogieMenu_BuildOutfitList(void);
s32 Save_TestEventFlag(s32 flag);
extern PoogieTbl gVillageNpcRecsA[];
/* This file's view of item.h's gItemData (ItemData[]). */
s32 Kitchen_IsIngredient(u16 item);
s32 Menu_CountPages(s16 n, s16 per);
void *memset(void *d, s32 c, u32 n);
extern char gOutfitNames[][0x20];
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Dojo_AllCleared101(void);
s32 Dojo_AllCleared201(void);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, M2C_UNK k);
extern u16 gDojoQuestsCat1[];
extern u16 gDojoQuestsCat2[];
extern u16 gDojoQuestsCat2Cond[];
extern u16 gDojoQuestsCat3[];
void CamFocus_SetAside(s8 a);
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
u8 Village_FacilityLevel(s32 k);
s32 Village_MilestoneReady(s32 kind);
extern Milestone gVillageMilestones[];
void Save_SetEventFlag(s32 flag);
void Save_ClearEventFlag(s32 flag);
extern s16 D_007E829A[];
extern s16 D_007E82A2[];
s32 SavePouch_Space(u16 item);
s32 ItemBox_Room(u16 item);
extern u8 gSkillPreview[];
extern u16 *gHrGateQuests[];
extern s16 gGalleryCourseFees[];
void Save_AddMoney(s32 n);
void Equip_Refresh(SaveData *save);
void Player_CalcSkills(Player *p);
void SkillList_Build(Player *p, Player *q);
u8 SkillList_PageCount(void);
s32 Save_IsQuestCleared(u16 id);
u8 SavePouch_Add(u16 item, s16 n);
s16 VillageGather_RollValue(u8 kind);
extern s16 gDojoRulesPagesOn[];
extern s16 gDojoRulesPagesOff[];
s32 Game_IsOnline(void);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
extern char *gDojoMenuTexts[];
void Menu_DrawDecoFrame(s16 x, s16 y, s16 w, s16 h, s8 k, s8 c);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *s, s32 color);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
void Text_PrintfColored(const char *fmt, ...);
extern char *gDojoBasicsTexts[];
extern char *gDojoBasicsPages[];
void Text_Printf(const char *fmt, ...);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
extern char *gHunterDojoTexts[];

u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Talk_Update(void);
s32 func_006CB680(void);
void DojoMenu_Close(void);
s32 DojoMenu_OfflineInput(u16 pad);
s32 DojoMenu_OnlineInput(u16 pad);
struct OfferTask;
void OfferMenu_Open(struct OfferTask *t);
extern s8 gQuestMinMembers;
void Talk_Say(s32 msg, s8 a);
void DojoMenu_Init(DojoMenu *m);
s32 DojoMenu_ChoiceInput(u16 pad);

/* unk_6CB690's .rodata (0x8261A0-0x82C700) */
#pragma rodata on
char D_008261A0[0x180] = {
#include "lobby/008261A0.inc"
};
char D_00826320[0x170] = {
#include "lobby/00826320.inc"
};
char D_00826490[0x170] = {
#include "lobby/00826490.inc"
};
char D_00826600[0x180] = {
#include "lobby/00826600.inc"
};
char D_00826780[0x190] = {
#include "lobby/00826780.inc"
};
char D_00826910[0x190] = {
#include "lobby/00826910.inc"
};
char D_00826AA0[0x1A0] = {
#include "lobby/00826AA0.inc"
};
char D_00826C40[0x1B0] = {
#include "lobby/00826C40.inc"
};
char D_00826DF0[0x160] = {
#include "lobby/00826DF0.inc"
};
char D_00826F50[0x180] = {
#include "lobby/00826F50.inc"
};
char D_008270D0[0x170] = {
#include "lobby/008270D0.inc"
};
char D_00827240[0x180] = {
#include "lobby/00827240.inc"
};
char D_008273C0[0x1A0] = {
#include "lobby/008273C0.inc"
};
char D_00827560[0x150] = {
#include "lobby/00827560.inc"
};
char D_008276B0[0x160] = {
#include "lobby/008276B0.inc"
};
char D_00827810[0x180] = {
#include "lobby/00827810.inc"
};
char D_00827990[0x1D0] = {
#include "lobby/00827990.inc"
};
char D_00827B60[0x190] = {
#include "lobby/00827B60.inc"
};
char D_00827CF0[0x190] = {
#include "lobby/00827CF0.inc"
};
char D_00827E80[0x180] = {
#include "lobby/00827E80.inc"
};
char D_00828000[0x170] = {
#include "lobby/00828000.inc"
};
char D_00828170[0x170] = {
#include "lobby/00828170.inc"
};
char D_008282E0[0x190] = {
#include "lobby/008282E0.inc"
};
char D_00828470[0x180] = {
#include "lobby/00828470.inc"
};
char D_008285F0[0x180] = {
#include "lobby/008285F0.inc"
};
char D_00828770[0x190] = {
#include "lobby/00828770.inc"
};
char D_00828900[0x130] = {
#include "lobby/00828900.inc"
};
char D_00828A30[0x1A0] = {
#include "lobby/00828A30.inc"
};
char D_00828BD0[0x1A0] = {
#include "lobby/00828BD0.inc"
};
char D_00828D70[0x190] = {
#include "lobby/00828D70.inc"
};
char D_00828F00[0x180] = {
#include "lobby/00828F00.inc"
};
char D_00829080[0x190] = {
#include "lobby/00829080.inc"
};
char D_00829210[0x1A0] = {
#include "lobby/00829210.inc"
};
char D_008293B0[0x190] = {
#include "lobby/008293B0.inc"
};
char D_00829540[0x1A0] = {
#include "lobby/00829540.inc"
};
char D_008296E0[0x1A0] = {
#include "lobby/008296E0.inc"
};
char D_00829880[0x1A0] = {
#include "lobby/00829880.inc"
};
char D_00829A20[0x1C0] = {
#include "lobby/00829A20.inc"
};
char D_00829BE0[0x190] = {
#include "lobby/00829BE0.inc"
};
char D_00829D70[0x1A0] = {
#include "lobby/00829D70.inc"
};
char D_00829F10[0x170] = {
#include "lobby/00829F10.inc"
};
char D_0082A080[0x180] = {
#include "lobby/0082A080.inc"
};
char D_0082A200[0x1A0] = {
#include "lobby/0082A200.inc"
};
char D_0082A3A0[0x1A0] = {
#include "lobby/0082A3A0.inc"
};
char D_0082A540[0x1A0] = {
#include "lobby/0082A540.inc"
};
char D_0082A6E0[0x1B0] = {
#include "lobby/0082A6E0.inc"
};
char D_0082A890[0x180] = {
#include "lobby/0082A890.inc"
};
char D_0082AA10[0x160] = {
#include "lobby/0082AA10.inc"
};
char D_0082AB70[0x160] = {
#include "lobby/0082AB70.inc"
};
char D_0082ACD0[0x180] = {
#include "lobby/0082ACD0.inc"
};
char D_0082AE50[0x180] = {
#include "lobby/0082AE50.inc"
};
char D_0082AFD0[0x1A0] = {
#include "lobby/0082AFD0.inc"
};
char D_0082B170[0x180] = {
#include "lobby/0082B170.inc"
};
char D_0082B2F0[0x190] = {
#include "lobby/0082B2F0.inc"
};
char D_0082B480[0x140] = {
#include "lobby/0082B480.inc"
};
char D_0082B5C0[0x1A0] = {
#include "lobby/0082B5C0.inc"
};
char D_0082B760[0x1A0] = {
#include "lobby/0082B760.inc"
};
char D_0082B900[0x190] = {
#include "lobby/0082B900.inc"
};
char D_0082BA90[0x130] = {
#include "lobby/0082BA90.inc"
};
char D_0082BBC0[0x130] = {
#include "lobby/0082BBC0.inc"
};
char D_0082BCF0[0x150] = {
#include "lobby/0082BCF0.inc"
};
char D_0082BE40[0x150] = {
#include "lobby/0082BE40.inc"
};
char D_0082BF90[0x130] = {
#include "lobby/0082BF90.inc"
};
char D_0082C0C0[0x120] = {
#include "lobby/0082C0C0.inc"
};
char D_0082C1E0[0x160] = {
#include "lobby/0082C1E0.inc"
};
char D_0082C340[0x140] = {
#include "lobby/0082C340.inc"
};
char D_0082C480[0x130] = {
#include "lobby/0082C480.inc"
};
char D_0082C5B0[0x150] = {
#include "lobby/0082C5B0.inc"
};
#pragma rodata off

void DojoMenu_Update(void)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->mode) {
    case 0:
        switch (Talk_Update()) {
        case 0:
            if (func_006CB680() == 1) {
                if (Game_IsOnline() == 0) {
                    m->mode++;
                    LobbySe_Play(8);
                    break;
                }
                m->mode = 2;
                m->cat = 0;
                WinSlide_Step(&m->win, 0x280, 0x104);
                Talk_Say(2, 0);
                CamFocus_SetAside(1);
                break;
            }
            m->mode = 3;
            DojoMenu_Init(m);
            break;
        }
        break;
    case 1:
        switch (DojoMenu_ChoiceInput(pad)) {
        case 0:
            switch (m->choice[m->unk92]) {
            case 0:
                m->mode++;
                m->cat = 0;
                WinSlide_Step(&m->win, 0x280, 0x104);
                CamFocus_SetAside(1);
                LobbySe_Play(0x16);
                break;
            case 1:
                m->mode = 3;
                DojoMenu_Init(m);
                LobbySe_Play(0x16);
                break;
            case 2:
                OfferMenu_Open(m->npc);
                LobbySe_Play(0x17);
                break;
            }
            break;
        case 3:
            m->mode = 4;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 2:
        switch (DojoMenu_OnlineInput(pad)) {
        case 0:
            m->mode = 4;
            if (gQuestMinMembers == 0) {
                Talk_Say(0xA, 0);
            } else {
                Talk_Say(0xB, 0);
            }
            break;
        case 3:
            if (Game_IsOnline() == 0) {
                m->mode = 1;
                break;
            }
            m->mode = 4;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 3:
        switch (DojoMenu_OfflineInput(pad)) {
        case 0:
            m->mode = 4;
            Talk_Say(1, 0);
            break;
        case 3:
            if (func_006CB680() == 1) {
                m->mode = 1;
                break;
            }
            m->mode = 4;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
            DojoMenu_Close();
            break;
        }
        break;
    }
}

void DojoMenu_Init(DojoMenu *m)
{
    m->cat = 0;
    WinSlide_Step(&m->win, 0x280, 0x104);
    CamFocus_SetAside(1);
    if (Cond_Check(3) != 0) {
        m->cats = 4;
    } else {
        m->cats = 3;
    }
}

s32 DojoMenu_QuestInput(u16 pad);
s32 DojoMenu_CategoryInput(DojoMenu *m, u16 pad);
s32 DojoMenu_BasicsInput(DojoMenu *m, u16 pad);
void DojoMenu_LoadQuests(DojoMenu *m);

s32 DojoMenu_OfflineInput(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    switch (m->step) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x104)) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 1:
        switch (DojoMenu_CategoryInput(m, pad)) {
        case 0:
            switch (m->cat) {
            case 0:
                m->step = 5;
                m->next = 4;
                m->unk9C = 0;
                break;
            default:
                m->step = 5;
                m->next = 3;
                DojoMenu_LoadQuests(m);
                break;
            }
            break;
        case 3:
            m->step = 0;
            return 3;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->step = 5;
            m->next = 4;
            break;
        }
        break;
    case 3:
        switch (DojoMenu_QuestInput(pad)) {
        case 0:
            m->step = 0;
            return 0;
        case 3:
            m->step = 6;
            m->next = 1;
            WinSlide_Step(&m->win, 0x280, 0x104);
            break;
        }
        break;
    case 4:
        switch (DojoMenu_BasicsInput(m, pad)) {
        case 0:
        case 3:
            m->step = 6;
            m->next = 1;
            break;
        }
        break;
    case 5:
        switch (WinSlide_Step(&m->win, 0x104, 0x280)) {
        case 0:
            m->step = m->next;
            break;
        }
        break;
    case 6:
        switch (WinSlide_Step(&m->win, 0x280, 0x104)) {
        case 0:
            m->step = m->next;
            break;
        }
        break;
    }
    return 2;
}

void DojoMenu_LoadQuests(DojoMenu *m)
{
    u16 *src;
    u16 n;

    switch (m->cat) {
    case 1:
        src = gDojoQuestsCat1;
        break;
    case 2:
        if (Cond_Check(3) != 0) {
            src = gDojoQuestsCat2Cond;
        } else {
            src = gDojoQuestsCat2;
        }
        break;
    case 3:
        src = gDojoQuestsCat3;
        break;
    }
    memset(m->list, 0, 0x18);
    m->unk9C = 0;
    m->unk9E = 3;
    m->unkA6 = 0;
    n = 0;
    while (*src != 0) {
        m->list[n] = *src;
        n++;
        src++;
    }
    m->count = n;
}

s32 DojoMenu_BasicsInput(DojoMenu *m, u16 pad)
{
    s32 b = pad;

    if (!(b & 0x20)) {
        if (b & 0x40) {
            LobbySe_Play(0x17);
            return 3;
        }
        m->unk9C = Menu_WrapUpDownSe(m->unk9C, 10, 0x17);
    }
    return 2;
}

s32 DojoMenu_CategoryInput(DojoMenu *m, u16 pad)
{
    if (pad & 0x20) {
        LobbySe_Play(0x17);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    m->cat = Menu_WrapUpDown(m->cat, m->cats);
    return 2;
}

s32 DojoMenu_ChoiceInput(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    if (pad & 0x20) {
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    m->unk92 = Menu_WrapUpDown(m->unk92, m->unk9A);
    return 2;
}

s32 DojoMenu_OnlineCategoryInput(u16 pad);
s32 DojoMenu_QuestKindInput(u16 pad);
s32 PointShop_Input(u16 pad);
s32 DojoMenu_RulesInput(u16 pad);
void PointShop_LoadPage(s16 page);
void DojoMenu_ListHuntQuests(void);
void DojoMenu_ListArenaQuests(void);

s32 DojoMenu_OnlineInput(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    switch (m->step) {
    case 0:
        switch (DojoMenu_OnlineCategoryInput(pad)) {
        case 0:
            switch (m->cat) {
            case 0:
                m->step++;
                m->unk96 = 0;
                break;
            case 2:
                m->step = 3;
                m->next = 9;
                m->unk9C = 0;
                if (Game_IsOnline() == 1) {
                    if (Cond_Check(0x64) == 1) {
                        m->unk9E = 11;
                    } else {
                        m->unk9E = 10;
                    }
                    m->unkA2 = gDojoRulesPagesOn[m->unk9C];
                } else {
                    if ((u8)Village_FacilityLevel(1) == 2) {
                        m->unk9E = 11;
                    } else {
                        m->unk9E = 10;
                    }
                    m->unkA2 = gDojoRulesPagesOff[m->unk9C];
                }
                break;
            case 1:
                m->step = 3;
                m->next = 8;
                m->unkB5 = 0;
                m->unk9C = 0;
                PointShop_LoadPage(0);
                break;
            }
            break;
        case 3:
            m->step = 0;
            CamFocus_SetAside(0);
            return 3;
        }
        break;
    case 1:
        switch (DojoMenu_QuestKindInput(pad)) {
        case 0:
            switch (m->unk96) {
            case 0:
                DojoMenu_ListHuntQuests();
                break;
            case 1:
                DojoMenu_ListArenaQuests();
                break;
            }
            if (m->count != 0) {
                m->step++;
                m->next = 4;
                m->unkB7 = 0;
                m->unkA6 = 0;
                m->unk9C = 0;
                m->unk9E = 3;
                Talk_Say(2, 0);
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
            break;
        case 3:
            m->step = 0;
            m->state = 1;
            break;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
            m->step++;
            WinSlide_Step(&m->win, 0x104, 0x280);
            LobbySe_Play(0x17);
            break;
        }
        break;
    case 3:
        switch (WinSlide_Step(&m->win, 0x104, 0x280)) {
        case 0:
            m->step = m->next;
            break;
        }
        break;
    case 4:
        switch (DojoMenu_QuestInput(pad)) {
        case 0:
            m->step = 0;
            return 0;
        case 3:
            m->step = 7;
            m->next = 1;
            WinSlide_Step(&m->win, 0x280, 0x104);
            break;
        }
        break;
    case 5:
    case 6:
    case 7:
        switch (WinSlide_Step(&m->win, 0x280, 0x104)) {
        case 0:
            m->step = m->next;
            if (m->next == 0) {
                m->state = 1;
            }
            break;
        }
        break;
    case 8:
        switch (PointShop_Input(pad)) {
        case 0:
            m->step = 0;
            return 0;
        case 3:
            m->step = 5;
            m->next = 0;
            break;
        }
        break;
    case 9:
        switch (DojoMenu_RulesInput(pad)) {
        case 0:
            m->step = 0;
            return 0;
        case 3:
            m->step = 6;
            m->next = 0;
            break;
        }
        break;
    }
    return 2;
}

s32 DojoMenu_RulesInput(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    s16 old;

    if (!(pad & 0x20)) {
        if (pad & 0x40) {
            LobbySe_Play(0x17);
            return 3;
        }
        if (pad & 0x3000) {
            old = m->unk9C;
            m->unk9C = Menu_WrapUpDownSe(old, m->unk9E, 0x17);
            if (old != m->unk9C) {
                m->unkA0 = 0;
                if (Game_IsOnline() == 1) {
                    m->unkA2 = gDojoRulesPagesOn[m->unk9C];
                } else {
                    m->unkA2 = gDojoRulesPagesOff[m->unk9C];
                }
            }
        } else if (pad & 0xC00) {
            m->unkA0 = Menu_WrapLeftRight(m->unkA0, m->unkA2);
        }
    }
    return 2;
}

s32 Dojo_AllCleared101(void)
{
    s32 id;

    id = 0x65;
loop_1:
    if (Save_IsQuestCleared(id & 0xFFFF) == 0) {
        return 0;
    }
    id += 1;
    if (id > 0x6d) {
        return 1;
    }
    goto loop_1;
}

s32 Dojo_AllCleared201(void)
{
    s32 id;

    id = 0xC9;
loop_1:
    if (Save_IsQuestCleared(id & 0xFFFF) == 0) {
        return 0;
    }
    id += 1;
    if (id > 0xcd) {
        return 1;
    }
    goto loop_1;
}

s32 Dojo_AllClearedOffline(void)
{
    if (Dojo_AllCleared101() == 0) {
        return 0;
    }
    if (Dojo_AllCleared201() != 0) {
        return 1;
    }
    return 0;
}

s32 Dojo_AllCleared10001(void)
{
    s32 id;

    id = 0x2711;
loop_1:
    if (Save_IsQuestCleared(id & 0xFFFF) == 0) {
        return 0;
    }
    id += 1;
    if (id > 0x2719) {
        return 1;
    }
    goto loop_1;
}

s32 Dojo_AllCleared10101(void)
{
    s32 id;

    id = 0x2775;
loop_1:
    if (Save_IsQuestCleared(id & 0xFFFF) == 0) {
        return 0;
    }
    id += 1;
    if (id > 0x2779) {
        return 1;
    }
    goto loop_1;
}

extern PointLine *gPointShopStock0[];
extern PointLine *D_007E7E74;
extern PointLine *gPointShopStock1[];
extern PointLine *D_007E7E7C;
extern PointLine *gPointShopStock2[];
extern PointLine *D_007E7E84;
s32 Dojo_AllClearedOffline(void);
s32 Dojo_AllCleared10101(void);
void PointShop_LoadPage(s16 page)
{
    s16 last;
    PointShop *m = (PointShop *)gMenuWork;
    s16 i = 0;
    s16 n = 0;
    s16 first = page * 7;
    PointLine *p;

    last = first + 6;

    if (Game_IsOnline() == 0) {
        if (Dojo_AllClearedOffline() == 1) {
            m->tbl = p = gPointShopStock1[0];
        } else if ((u8)Village_FacilityLevel(1) < 2) {
            m->tbl = p = gPointShopStock0[0];
        } else {
            m->tbl = p = D_007E7E74;
        }
    } else if (Dojo_AllCleared10101() == 1) {
        m->tbl = p = D_007E7E84;
    } else if (Cond_Check(0x64) == 0) {
        m->tbl = p = D_007E7E7C;
    } else {
        m->tbl = p = gPointShopStock2[0];
    }
    for (; p->item != 0; p++) {
        if (i >= first && i <= last) {
            m->line[n] = *p;
            n++;
        }
        i++;
    }
    m->pages = Menu_CountPages(i, 7);
    m->count = n;
    m->cursor = 0;
}

s32 PointShop_CanBuy(s16 i, s16 n)
{
    PointLine *l = &((PointShop *)gMenuWork)->line[i];
    s32 room;
    s16 space;

    if (gSave.points < l->price * n) {
        return 0;
    }
    room = ItemBox_Room(l->item);
    space = SavePouch_Space(l->item);
    if (space == -1) {
        space = 0;
    }
    if (room + space < n) {
        return 0;
    }
    return 1;
}

s32 PointShop_CanBuy(s16 i, s16 n);
s8 Items_Give(s32 item, s32 n);
s32 Menu_YesNoInput(void);
void Menu_SetYesNo(s8 a);
s32 Menu_PushNoticeOnce(s32);

s32 PointShop_Input(u16 pad)
{
    PointShop *m = (PointShop *)gMenuWork;
    s32 n;
    s32 old;
    PointLine *l;

    switch (m->state) {
    case 0:
        if (pad & 0x20) {
            if (PointShop_CanBuy(m->cursor, 1) == 1) {
                m->state++;
                m->unkAA = 1;
                Menu_SetYesNo(1);
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(0x17);
            return 3;
        } else if (pad & 0x200) {
            m->unkB5 ^= 1;
            LobbySe_Play(0x39);
        } else if (pad & 0xC00) {
            if (m->pages > 1) {
                m->page = Menu_WrapLeftRight(m->page, m->pages);
                PointShop_LoadPage(m->page);
            }
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, m->count);
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->state++;
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->state = 0;
            LobbySe_Play(4);
        } else if (pad & 0x200) {
            m->unkB5 ^= 1;
            LobbySe_Play(0x39);
        } else if (pad & 0x2000) {
            if (PointShop_CanBuy(m->cursor, m->unkAA + 1) == 1 && m->unkAA <= 98) {
                m->unkAA++;
                LobbySe_Play(1);
            }
        } else if (pad & 0x1000) {
            if (m->unkAA > 1) {
                m->unkAA--;
                LobbySe_Play(1);
            }
        } else if (pad & 0x800) {
            if (m->unkAA > 1) {
                m->unkAA = 1;
                LobbySe_Play(1);
            }
        } else if (pad & 0x400) {
            old = m->unkAA;
            for (n = old; n < 99; n++) {
                if (PointShop_CanBuy(m->cursor, n) == 0) {
                    m->unkAA = n - 1;
                    break;
                }
            }
            if (n == 99) {
                m->unkAA = 99;
            }
            if (old != m->unkAA) {
                LobbySe_Play(1);
            }
        }
        break;
    case 2:
        if (pad & 0x200) {
            m->unkB5 ^= 1;
            LobbySe_Play(0x39);
        } else {
            switch (Menu_YesNoInput()) {
            case 0:
                m->state = 0;
                l = &m->line[m->cursor];
                gSave.points -= l->price * m->unkAA;
                if (Items_Give(l->item, m->unkAA) != 0) {
                    Menu_PushNoticeOnce(0xF);
                }
                m->unkAA = 1;
                break;
            case 3:
                m->state--;
                break;
            }
        }
        break;
    }
    return 2;
}

QuestInfo *Quest_GetInfoAny(u16 id);
s32 Quest_CheckOrderTerms(u8 id, u8 flag);
void LobbyText_Open(s32);
void LobbyDialog_Grow(void);
extern LobbyMenu *gLobbyMenu;
extern s16 gQuestStampRand;
s32 Win_Shake(WinShake *win);
void QuestRoom_ResetSettings(void);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
void Dojo_SetBestTime(s32 a, s32 value);
void Dojo_SetBestScore(s32 a, s32 value);
s32 DojoMenu_OnlineStart(u16 pad);

s32 DojoMenu_QuestInput(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    QuestInfo *q;

    switch (m->state) {
    case 0:
        if (pad & 0x20) {
            q = Quest_GetInfoAny(m->list[m->unkA6]);
            if (gLobby->questTaken != 0) {
                m->state = 4;
                Talk_Say(0xF, 0);
            } else if (Quest_CheckOrderTerms(q->terms, 0) == 0) {
                m->state = 5;
                LobbyText_Open(0x64);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
            } else {
                m->state++;
                Talk_Say(5, 0);
            }
            m->unkB7 = 1;
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            LobbySe_Play(0x17);
            return 3;
        } else if (pad & 0xC00) {
            m->unk9C = Menu_WrapLeftRight(m->unk9C, m->unk9E);
        } else if ((gPadState[0].held & 0x4000 && pad & 0x8000) || (pad & 0x4000 && gPadState[0].held & 0x8000)) {
            m->state = 6;
            LobbyText_Open(0x95);
            Menu_SetYesNo(1);
        } else {
            m->unkA6 = Menu_WrapUpDownSe(m->unkA6, m->count, 0x17);
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            if (Game_IsOnline() == 0) {
                m->state = 3;
                m->unkB4 = 1;
                gQuestStampRand = Rand_Next(1);
                Win_Shake(&m->shake);
            } else {
                m->state++;
                QuestRoom_ResetSettings();
                LobbyText_Open(0x78);
            }
            break;
        case 3:
            m->state = 0;
            m->unkB7 = 0;
            break;
        }
        break;
    case 2:
        switch (DojoMenu_OnlineStart(pad)) {
        case 0:
            m->state++;
            m->unkB4 = 1;
            gQuestStampRand = Rand_Next(1);
            Win_Shake(&m->shake);
            break;
        }
        break;
    case 3:
        gFieldControl = 0;
        switch (Win_Shake(&m->shake)) {
        case 0:
            gLobby->questPaid = 1;
            m->unkB6 = 1;
            QuestDesk_Accept(m->list[m->unkA6], 0, 1);
            gFieldControl = 1;
            return 0;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
            m->state = 0;
            m->unkB7 = 0;
            break;
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->state = 0;
            LobbySe_Play(8);
            return 3;
        }
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            m->state = 0;
            Dojo_SetBestTime(m->list[m->unkA6], 0);
            Dojo_SetBestScore(m->list[m->unkA6], 0);
            break;
        case 3:
            m->state = 0;
            break;
        }
        break;
    }
    return 2;
}

s32 LobbyLevel2_FetchList(void);
s32 LobbyLevel2_EnterFree(s32 v);
s32 LobbyLevel2_FetchRecords(void);
s32 LobbyLevel2_SendSettings(void);
s32 LobbyLevel2_Send6509(u32 v);
s32 LobbyLevel2_Send650B(Msg650B *d);
s32 LobbyStep_6501(void);
void LobbyDialog_OpenWait(void *text);
s32 DojoMenu_OnlineSettings(u16 pad);
extern u32 gQuestPostArg;
extern u16 gLobbyQuestId;
extern u8 D_6ADAD4[];

s32 DojoMenu_OnlineStart(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    u8 *p;

    switch (m->unkB1) {
    case 0:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchList()) {
        case 0:
            m->unkB1++;
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            m->unkB1 = 8;
            gSysSettings.fieldControl = 0;
            break;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_EnterFree(1)) {
        case 0:
            m->unkB1++;
            break;
        case 1:
            m->unkB1 = 8;
            gSysSettings.fieldControl = 0;
            break;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchRecords()) {
        case 0:
            m->unkB1++;
            LobbySe_Play(0x17);
            break;
        case 1:
            m->unkB1 = 8;
            gSysSettings.fieldControl = 0;
            break;
        }
        break;
    case 3:
        switch (DojoMenu_OnlineSettings(pad)) {
        case 0:
            m->unkB1++;
            LobbyText_Open(0x78);
            m->req.quest = m->list[m->unkA6];
            m->req.money = 0;
            m->req.desk = 2;
            m->req.area = gScene.area;
            break;
        case 3:
            m->unkB1 = 7;
            LobbyText_Open(0x77);
            break;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendSettings()) {
        case 0:
            m->unkB1++;
            break;
        case 1:
            break;
        }
        gSysSettings.fieldControl = 0;
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_Send650B(&m->req)) {
        case 0:
            m->unkB1++;
            gQuestPostArg = (gQuestPostArg & 0xFFFF0000) | gLobbyQuestId;
            break;
        case 1:
            m->unkB1 = 8;
            gSysSettings.fieldControl = 0;
            break;
        }
        gSysSettings.fieldControl = 0;
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_Send6509(gQuestPostArg)) {
        case 0:
            m->unkB1 = 0;
            gLobby->questPaid = 1;
            QuestDesk_Accept(m->list[m->unkA6], 0, 1);
            memset(D_6ADAD4, 0, 0x5C);
            gSysSettings.fieldControl = 0;
            return 0;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            m->unkB1 = 8;
            gSysSettings.fieldControl = 0;
            break;
        }
        gSysSettings.fieldControl = 0;
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_6501() == 1) {
            p = &gLobby->questPaid;
            if (*p == 1) {
                *p = 0;
            }
            m->unkB1 = 0;
            gSysSettings.fieldControl = 1;
            return 3;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->unkB1 = 0;
            DojoMenu_Close();
            LobbySe_Play(8);
        }
        break;
    }
    return 2;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6CB690", D_0082C798);

extern LobbyRecs gLobbyRecs;
void LobbyMenu_Init(void);
s32 LobbyMenu_KbdEntry(void *p);

s32 DojoMenu_OnlineSettings(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    s16 old;

    switch (m->unkB2) {
    case 0:
        if (pad & 0x20) {
            if (m->unk90 == 3) {
                m->unkB2 = 0;
                if (gLobbyRecs.usePass != 0 && gLobbyRecs.pass[0] == 0) {
                    Menu_PushNoticeOnce(2);
                    break;
                }
                gQuestMinMembers = gLobbyRecs.rec[0].cur;
                LobbySe_Play(8);
                return 0;
            }
            if (m->unk90 == 2) {
                m->unkB2++;
                gFieldControl = 0;
                LobbySe_Play(8);
                LobbyMenu_Init();
                LobbyMenu_KbdEntry(gLobbyRecs.pass);
            }
        } else if (pad & 0xC00) {
            if (m->unk90 == 1) {
                gLobbyRecs.usePass ^= 1;
                if (gLobbyRecs.usePass == 0) {
                    memset(gLobbyRecs.pass, 0, 9);
                }
                LobbySe_Play(1);
            } else if (m->unk90 == 0) {
                gLobbyRecs.rec[0].cur = Menu_WrapLeftRight(gLobbyRecs.rec[0].cur, 4);
            }
        } else {
            old = m->unk90;
            m->unk90 = Menu_WrapUpDown(old, 4);
            if ((s8)old != m->unk90 && m->unk90 == 2 && gLobbyRecs.usePass == 0) {
                m->unk90 += m->unk90 - (s8)old;
            }
        }
        break;
    case 1:
        if (LobbyMenu_KbdEntry(gLobbyRecs.pass) == 1) {
            m->unkB2 = 0;
            LobbyMenu_Init();
            gFieldControl = 1;
        }
        break;
    }
    return 2;
}


QuestInfo *Quest_GetDiscInfo(u16 id);
s32 Quest_MatchesSeasonTime(u16 id, s32 a, s32 b);

s32 DojoMenu_QuestAvailable(u16 id)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    QuestInfo *q = Quest_GetDiscInfo(id);

    if (q == NULL) {
        return 0;
    }
    if (!(q->flags & 0x200000)) {
        return 0;
    }
    if (Quest_MatchesSeasonTime(id, m->unk68, m->unk6C) == 0) {
        return 0;
    }
    switch (id) {
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
        if ((u8)Village_FacilityLevel(1) <= 0) {
            return 0;
        }
        break;
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
        if ((u8)Village_FacilityLevel(1) <= 1) {
            return 0;
        }
        break;
    case 0xCD:
        if (Save_IsQuestCleared(0xCC) == 0) {
            return 0;
        }
    case 0xCC:
        if (Save_IsQuestCleared(0xCB) == 0) {
            return 0;
        }
    case 0xCB:
        if (Save_IsQuestCleared(0xCA) == 0) {
            return 0;
        }
    case 0xCA:
        if (Save_IsQuestCleared(0xC9) == 0) {
            return 0;
        }
    case 0xC9:
        if ((u8)Village_FacilityLevel(1) != 2) {
            return 0;
        }
        break;
    case 0x2711:
    case 0x2712:
    case 0x2713:
    case 0x2714:
        return 1;
    case 0x2715:
    case 0x2716:
    case 0x2717:
    case 0x2718:
    case 0x2719:
        if (Cond_Check(0x64) == 0) {
            return 0;
        }
        break;
    case 0x2779:
        if (Save_IsQuestCleared(0x2778) == 0) {
            return 0;
        }
    case 0x2778:
        if (Save_IsQuestCleared(0x2777) == 0) {
            return 0;
        }
    case 0x2777:
        if (Save_IsQuestCleared(0x2776) == 0) {
            return 0;
        }
    case 0x2776:
        if (Save_IsQuestCleared(0x2775) == 0) {
            return 0;
        }
    case 0x2775:
        if (Cond_Check(0x5C) == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

extern u16 *gQuestIdLists[];
s32 DojoMenu_QuestAvailable(u16 id);
s32 QuestId_IsStandard(s32 id);
void DojoMenu_ListHuntQuests(void)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    u16 *p;
    u16 n;
    u16 i;
    s32 kind;

    memset(m->list, 0, 0x18);
    n = 0;
    for (i = 0; i < 8; i++) {
        for (p = gQuestIdLists[i]; *p != 0; p++) {
            kind = Quest_GetDiscInfo(*p)->map;
            if (kind == 9 || kind == 10) {
                continue;
            }
            if (DojoMenu_QuestAvailable(*p) == 1) {
                if (Game_IsOnline() == 0) {
                    if (QuestId_IsStandard(*p) == 1) {
                        m->list[n++] = *p;
                    }
                } else if (QuestId_IsStandard(*p) == 0) {
                    m->list[n++] = *p;
                }
            }
            if (n >= 12) {
                break;
            }
        }
        if (n >= 12) {
            break;
        }
    }
    m->count = n;
}

void DojoMenu_ListArenaQuests(void)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    u16 *p;
    u16 n;
    u16 i;
    s32 kind;

    memset(m->list, 0, 0x18);
    n = 0;
    for (i = 0; i < 8; i++) {
        for (p = gQuestIdLists[i]; *p != 0; p++) {
            kind = Quest_GetDiscInfo(*p)->map;
            if (kind != 9 && kind != 10) {
                continue;
            }
            if (DojoMenu_QuestAvailable(*p) == 1) {
                if (Game_IsOnline() == 0) {
                    if (QuestId_IsStandard(*p) == 1) {
                        m->list[n++] = *p;
                    }
                } else if (QuestId_IsStandard(*p) == 0) {
                    m->list[n++] = *p;
                }
            }
            if (n >= 12) {
                break;
            }
        }
        if (n >= 12) {
            break;
        }
    }
    m->count = n;
}

s32 DojoMenu_OnlineCategoryInput(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    switch (m->state) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x104)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->state = 0;
            LobbySe_Play(8);
            return 0;
        }
        if (pad & 0x40) {
            m->state = 0;
            LobbySe_Play(4);
            return 3;
        }
        m->cat = Menu_WrapUpDown(m->cat, 3);
        return 2;
    }
    return 2;
}

s32 DojoMenu_QuestKindInput(u16 pad)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    if (pad & 0x20) {
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    m->unk96 = Menu_WrapUpDown(m->unk96, 2);
    return 2;
}


/* Round 29: each case moves the parameters in place (`x = x + 0xAA`): x and y keep s6/s5 into
 * the cases, as the game's. Written `x += 0xAA`, MWCC adds to the parameter without extending
 * it first (138 words); separate lx/ly locals put the parameters in s1-s3 (18). */
void DojoMenu_DrawMenu(s16 x, s16 y, s8 k)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    char *s;
    s32 i;
    s16 tx;
    s8 v;

    Menu_DrawDecoFrame(x, y, 0x154, 0x168, 5, 0);
    Font_SetColor(0x807F7F7F);
    if (k == 0) {
        v = m->step;
    } else {
        v = m->next;
    }
    switch (v) {
    case 0:
        x = x + 0xAA;
        y = y + 0x64;
        Font_SetSpacing(0x18, 0x18);
        for (i = 0; i < 3; i++) {
            s = gDojoMenuTexts[i + 3];
            tx = Text_CenterX(s, x, 0x18);
            if (i == m->cat) {
                Menu_DrawArrowsRev(tx, y, 0x18, s, 0xFFB57510);
                Text_DrawShadowRgb(tx, y, 0xFF101010, 0xFF106060, s);
            } else {
                Font_SetColor(0xFF101010);
                Font_SetPos(tx, y);
                Text_PrintfColored(s);
            }
            y += 0x46;
        }
        break;
    case 1:
    case 2:
        x = x + 0xAA;
        y = y + 0x6E;
        Font_SetSpacing(0x1A, 0x1A);
        for (i = 0; i < 2; i++) {
            s = gDojoMenuTexts[i + 7];
            tx = Text_CenterX(s, x, 0x1A);
            if (i == m->unk96) {
                Menu_DrawArrowsRev(tx, y, 0x1A, s, 0xFFB57510);
                Text_DrawShadowRgb(tx, y, 0xFF101010, 0xFF106060, s);
            } else {
                Font_SetColor(0xFF101010);
                Font_SetPos(tx, y);
                Text_PrintfColored(s);
            }
            y += 0x64;
        }
        break;
    }
}

/* The quest fields the dojo quest draw reads. */
typedef struct QuestView {
    /* 0x00 */ u32 flags;
    /* 0x04 */ u8 unk4[0x1C];
    /* 0x20 */ u16 target;
    /* 0x22 */ u8 unk22[8];
    /* 0x2A */ u16 id;
} QuestView;
char *Quest_GetText(u16 id, s32 k);
u16 Map_Group(u16 id);
extern char *gStageNames[];
s32 Dojo_GetBestScore(u16 id);
s32 Dojo_GetBestTime(u16 id);
s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
char *Time_FormatMinSecInHour(u32);
extern char D_0082C798[];
extern char *gPointMarkTexts[];
extern char *gNoValueDashTexts[];
extern char *gClientLabelTexts[];
extern char *gDestinationTexts[];
extern char *gHighScoreTexts[];
extern char *D_0080CD84;
extern char *D_0080CD74;

void Quest_DrawDojoDetails(s16 x, s16 y, QuestView *q)
{
    s16 y0 = y;
    s32 id = q->id;
    char tmp[0x40];
    char buf[0x40];

    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(D_0082C798, Quest_GetText(id, 0));
    Font_SetPos(x + 0x7E, y += 0x2A);
    Text_PrintfColored(D_0082C798, Quest_GetText(id, 6));
    Font_SetPos(x + 0x7E, y += 0x18);
    Text_PrintfColored(gStageNames[Map_Group(q->target)]);
    Font_SetPos(x + 0x7E, y += 0x18);
    if (Save_IsQuestCleared(id) == 0 || Dojo_GetBestScore(id) == 0) {
        sprintf(buf, D_0082C798, gNoValueDashTexts[0]);
    } else {
        sprintf(tmp, "%d%s", Dojo_GetBestScore(id), gPointMarkTexts[0]);
        Sjis_Widen(tmp, buf);
    }
    Text_PrintfColored(D_0082C798, buf);
    Font_SetPos(x + 0x7E, y += 0x18);
    if (Save_IsQuestCleared(id) == 0 || Dojo_GetBestTime(id) / 30 == 0) {
        sprintf(buf, D_0082C798, gNoValueDashTexts[0]);
    } else {
        sprintf(tmp, D_0082C798, Time_FormatMinSecInHour(Dojo_GetBestTime(id) / 30), gPointMarkTexts[0]);
        Sjis_Widen(tmp, buf);
    }
    Text_PrintfColored(D_0082C798, buf);
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(D_0082C798, Quest_GetText(id, 7));
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = y0 + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(D_0082C798, gClientLabelTexts[0]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_0082C798, gDestinationTexts[0]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_0082C798, gHighScoreTexts[0]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_0082C798, D_0080CD84);
    Font_SetPos(x, y + 0x18);
    Text_PrintfColored(D_0082C798, D_0080CD74);
}

extern char *D_0080C46C[];
extern char *gMemberCountTexts[];
extern char *gNoneSetTexts[];
extern char *D_0080CD94;
extern char *gPlayerCountTexts[];
extern char *D_0080CD9C;
extern char *gCondDecideTexts[];
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);

void DojoMenu_DrawOnlineSettings(s16 x, s16 y)
{
    DojoMenu *m = (DojoMenu *)(s32)gMenuWork;
    s16 y0;
    s16 lx;
    s32 cx;
    s32 ax;
    s32 ty;
    s16 tx;
    s32 sx;

    Quest_GetDiscInfo(m->list[m->unkA6]);
    y0 = y;
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, D_0080C46C[m->unk96], 2, 1);
    Menu_DrawQuestSheet(x, y, 0x140, 0x168, NULL, 1, -1, -1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    y += 0x28;
    sx = x;
    cx = sx + 0xD8;
    Font_SetPos(cx, y += 0x2A);
    Text_PrintfColored(D_0082C798, gMemberCountTexts[gLobbyRecs.rec[0].cur]);
    if (m->unk90 == 0) {
        Menu_DrawArrows(cx, y, 0x12, gMemberCountTexts[gLobbyRecs.rec[0].cur], 0xFF106060);
    }
    Font_SetPos(cx, y += 0x2A);
    Text_PrintfColored(gNoneSetTexts[gLobbyRecs.usePass]);
    if (m->unk90 == 1) {
        Menu_DrawArrows(cx, y, 0x12, gNoneSetTexts[gLobbyRecs.usePass], 0xFF106060);
    }
    Font_SetPos(Text_CenterX(gLobbyRecs.pass, sx + 0xA0, 0x12), y += 0x2A);
    Text_PrintfColored(D_0082C798, gLobbyRecs.pass);
    if (m->unk90 == 2) {
        ax = sx + 0x64;
        Menu_DrawArrowsRev(ax, y, 0x12, "             ", 0xFF106060);
        Gfx_DrawRectFrame(ax, (s16)y - 1, 0x78, 0x14, 0xFF106060);
        Gfx_DrawRectFrame(sx + 0x63, (s16)y - 2, 0x7A, 0x16, 0xFF106060);
    } else {
        Gfx_DrawRectFrame(sx + 0x64, (s16)y - 1, 0x78, 0x14, 0xFF606060);
        Gfx_DrawRectFrame(sx + 0x63, (s16)y - 2, 0x7A, 0x16, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    ty = y0 + 0x28;
    y = ty;
    tx = sx + 0x28;
    Font_SetPos(Text_CenterX(D_0080CD94, 0x1B8, 0x12), ty);
    Text_PrintfColored(D_0082C798, D_0080CD94);
    Font_SetPos(tx, y += 0x2A);
    Text_PrintfColored(D_0082C798, gPlayerCountTexts[0]);
    Font_SetPos(tx, y += 0x2A);
    Text_PrintfColored(D_0082C798, D_0080CD9C);
    lx = Text_CenterX(gCondDecideTexts[0], 0x1B8, 0x12);
    Font_SetPos(lx, y += 0x78);
    Text_PrintfColored(D_0082C798, gCondDecideTexts[0]);
    if (m->unk90 == 3) {
        Menu_DrawArrowsRev(lx, y, 0x12, gCondDecideTexts[0], 0xFF106060);
    }
}

void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
extern char *D_0080C46C[];
extern s8 gMapGroupMapIds[][2];
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *q);
void Quest_DrawTerms(s16 x, s16 y, QuestInfo *q);
void Quest_DrawDetails(s16 x, s16 y, QuestInfo *q);
void Quest_DrawClearMark(s16 x, s16 y);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
u16 Map_Group(u16 id);

void DojoMenu_DrawQuestPage(s16 x, s16 y)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    QuestInfo *q = Quest_GetDiscInfo(m->list[m->unkA6]);
    s16 offs[12];
    s16 i;

    Font_SetSpacing(0x16, 0x16);
    if (q->flags & 0x4000000) {
        Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, (&gDojoBasicsTexts[0])[m->cat], 2, 0xFF);
    } else {
        Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, D_0080C46C[m->unk96], 2, 0xFF);
    }
    memset(offs, 0, sizeof(offs));
    for (i = 0; i < 12; i++) {
        if (m->list[i] == 0) {
            break;
        }
        offs[i] = Map_Group(Quest_GetDiscInfo(m->list[i])->map);
        if (i == m->unkA6) {
            offs[i] = -offs[i];
        }
    }
    Menu_DrawQuestSheet(x, y, 0x154, 0x168, offs, m->unkA6, gMapGroupMapIds[Map_Group(*(u16 *)&q->map)][0], -1);
    Font_SetColor(0x80101010);
    if (m->unkB7 == 0) {
        Menu_DrawPageNum(0x21C, y + 0xE, m->unk9C, 3, 1);
    }
    Font_SetSpacing(0x12, 0x12);
    switch (m->unk9C) {
    case 0:
        Quest_DrawOverview((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
        break;
    case 1:
        Quest_DrawTerms((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
        break;
    case 2:
        if (q->flags & 0x4000000) {
            Quest_DrawDetails((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
        } else {
            Quest_DrawDojoDetails((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, (QuestView *)q);
        }
        break;
    }
    if (Save_IsQuestCleared(*(u16 *)((u8 *)q + 0x2A))) {
        Quest_DrawClearMark(x + 0x28, y + 0xE);
    }
    if (m->unkB4) {
        Menu_DrawStamp(x + 0x28, y + 0xE, 0x154, 0x168, (u16)gQuestStampRand, m->shake.timer);
    }
}

void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color);
extern char *gDojoExchangeTexts[];
extern char *D_0080C474;
extern char *gExchangeCountTexts[];
extern char *D_0080C47C;
extern char *gItemNames[];
extern char *gYesNoTexts[];
void Menu_DrawPoints(s16 x, s16 y);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 color);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
char *Text_FillDollar(char *fmt, M2C_UNK n);

void PointShop_Draw(s16 x, s16 y)
{
    PointShop *m = (PointShop *)&gMenuWork;
    s16 ty = y;
    s16 i;
    s16 d;
    char buf[0x10];
    char wide[0x20];

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, gDojoExchangeTexts[0], 2, 1);
    Menu_DrawPoints(0x1E0, 0x10);
    Icon_Draw(x, y, 0x154, 0x168, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(0x1FE, ty + 0xE, m->page, m->pages, 1);
    Font_SetSpacing(0x14, 0x14);
    ty += 0x24;
    for (i = 0; i < m->count; i++) {
        if (PointShop_CanBuy(i, 1) == 1) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x80404040);
        }
        Font_SetPos(x + 0x2E, ty);
        Text_PrintfColored(D_0082C798, gItemNames[m->line[i].item]);
        Font_SetPos(x + 0xF0, ty);
        Text_PrintfColored("%5dP", m->line[i].price);
        Item_DrawIcon(x + 0x12, ty, 0x14, m->line[i].item);
        if (i == m->cursor) {
            if (gFrameTick & 0x10) d = 4; else d = 10;
            Icon_Draw(x + 0x2E, ty + 0xE, 0xFA, 8, 0xA03030A0, 0x6F);
            Menu_DrawCursorIcon(x - 4 - d, ty - 2, 0x18, 0xBDB59F6C);
        }
        ty += 0x18;
    }
    Menu_DrawDashLine(x + 0x14, 0x106, 0x12C, 0xFF000000);
    Font_SetColor(0x80101010);
    ItemInfo_Draw(x + 0xA, 0x10A, 0x12C, m->unkB5, m->line[m->cursor].item, 0, -1, 0x80101010);
    Font_SetSpacing(0x12, 0x12);
    switch (m->state) {
    case 0:
        Win_DrawText(0x104, 0x16A, 0x154, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C474);
        break;
    case 1:
        Win_DrawText(0x104, 0x16A, 0x154, 0x50, 0xBDB59F6C, 0xC0101010, gExchangeCountTexts[0]);
        Font_SetPos(0x1DF, 0x18E);
        sprintf(buf, "%02d", m->unkAA);
        Sjis_Widen(buf, wide);
        Text_PrintfColored(wide);
        if (PointShop_CanBuy(m->cursor, m->unkAA + 1) == 1 && m->unkAA < 0x63) {
            Icon_DrawMode(0x1E0, 0x178, 0x24, 0x16, 0xFF00FF00, 0x29, 2);
        }
        if (m->unkAA > 1) {
            Icon_DrawMode(0x1E1, 0x1A0, 0x24, 0x16, 0xFF00FF00, 0x28, 2);
        }
        break;
    case 2:
        Win_DrawText(0x104, 0x16A, 0x154, 0x50, 0xBDB59F6C, 0xC0101010,
                      Text_FillDollar(D_0080C47C, m->line[m->cursor].price * m->unkAA));
        Font_SetPos(0x186, 0x19C);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

extern char *D_0080C464;
extern char *gDojoRulesTextsOn[];
extern char *gDojoRulesTextsOff[];
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
s16 DojoMenu_RulesPageBase(s16 n);
void DojoMenu_DrawRules(s16 x, s16 y)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;
    s16 offs[10];
    s16 i;
    s16 k;
    char *str;

    for (i = 0; i < m->unk9E; i++) {
        offs[i] = 0x10;
        if (m->unk9C == i) {
            offs[i] = -offs[i];
        }
    }
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, D_0080C464, 2, 1);
    Menu_DrawQuestSheet(x, y, 0x154, 0x168, offs, m->unk9C, -1, -2);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0x104, y + 0x14, m->unkA0, m->unkA2, 1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x + 0x28, y + 0x32);
    k = m->unkA0 + DojoMenu_RulesPageBase(m->unk9C);
    if (Game_IsOnline() == 1) {
        str = gDojoRulesTextsOn[k];
    } else {
        str = gDojoRulesTextsOff[k];
    }
    Text_Printf(str);
}

s16 DojoMenu_RulesPageBase(s16 n)
{
    u8 i;
    s16 sum = 0;

    if (n == 0) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (Game_IsOnline() == 1) {
            sum += gDojoRulesPagesOn[i];
        } else {
            sum += gDojoRulesPagesOff[i];
        }
    }
    return sum;
}

void DojoMenu_DrawCategories(DojoMenu *m, s16 x, s16 y)
{
    char *s;
    s32 i;
    s16 cx;

    Menu_DrawDecoFrame(x, y, 0x154, 0x168, 5, 0);
    Font_SetColor(0x807F7F7F);
    x = x + 0xAA;
    y = y + 0x64;
    Font_SetSpacing(0x18, 0x18);
    for (i = 0; i < m->cats; i++) {
        s = gDojoMenuTexts[i + 12];
        cx = Text_CenterX(s, x, 0x18);
        if (i == m->cat) {
            Menu_DrawArrowsRev(cx, y, 0x18, s, 0xFFB57510);
            Text_DrawShadowRgb(cx, y, 0xFF101010, 0xFF106060, s);
        } else {
            Font_SetColor(0xFF101010);
            Font_SetPos(cx, y);
            Text_PrintfColored(s);
        }
        y += 0x3C;
    }
}

void DojoMenu_DrawBasics(DojoMenu *m, s16 x, s16 y)
{
    s16 offs[10];
    s16 i;

    for (i = 0; i < 10; i++) {
        offs[i] = 0x10;
        if (m->unk9C == i) {
            offs[i] = -offs[i];
        }
    }
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0x104, 0xFFB0B000, gDojoBasicsTexts[0], 2, 1);
    Menu_DrawQuestSheet(x, y, 0x154, 0x168, offs, m->unk9C, -1, -2);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x + 0x28, y + 0x28);
    Text_Printf(gDojoBasicsPages[m->unk9C]);
}

void DojoMenu_DrawQuestPage(s16 x, s16 y);
void PointShop_Draw(s16 x, s16 y);
void DojoMenu_DrawOnlineSettings(s16 x, s16 y);
void DojoMenu_DrawRules(s16 x, s16 y);
void DojoMenu_DrawBasics(DojoMenu *m, s16 x, s16 y);
void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind);

void DojoMenu_DrawPanels(void *task)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    switch (m->mode) {
    case 1:
        if (Cond_Check(10) == 1) {
            Menu_DrawLabelList(0x190, (s8 **)&gDojoMenuTexts[0], m->unk9A, m->unk92, 0xFFB0B000);
        } else {
            Menu_DrawLabelList(0x190, (s8 **)&gDojoMenuTexts[1], m->unk9A, m->unk92, 0xFFB0B000);
        }
        break;
    case 2:
        switch (m->step) {
        case 0:
        case 1:
        case 2:
            Menu_DrawDecoBox(m->win.pos, 0x3A, 0x154, 0x168, 5);
            break;
        case 3:
        case 5:
            if (m->next == 4) {
                DojoMenu_DrawQuestPage(m->shake.dx + 0x104, m->shake.dy + 0x3A);
            } else if (m->next == 9) {
                DojoMenu_DrawRules(0x104, 0x3A);
            } else {
                PointShop_Draw(0x104, 0x3A);
            }
            break;
        case 6:
            DojoMenu_DrawRules(0x104, 0x3A);
            break;
        case 7:
            DojoMenu_DrawQuestPage(m->shake.dx + 0x104, m->shake.dy + 0x3A);
            break;
        case 4:
            if (m->state == 2 && m->unkB1 == 3) {
                DojoMenu_DrawOnlineSettings(m->shake.dx + 0x118, m->shake.dy + 0x3A);
            } else {
                DojoMenu_DrawQuestPage(m->shake.dx + 0x104, m->shake.dy + 0x3A);
            }
            break;
        case 8:
            PointShop_Draw(0x104, 0x3A);
            break;
        case 9:
            DojoMenu_DrawRules(0x104, 0x3A);
            break;
        }
        break;
    case 3:
        switch (m->step) {
        case 0:
        case 1:
        case 2:
            Menu_DrawDecoBox(m->win.pos, 0x3A, 0x154, 0x168, 5);
            break;
        case 5:
        case 6:
            switch (m->cat) {
            case 0:
                DojoMenu_DrawBasics(m, 0x104, 0x3A);
                break;
            default:
                DojoMenu_DrawQuestPage(m->shake.dx + 0x104, m->shake.dy + 0x3A);
                break;
            }
            break;
        case 4:
            DojoMenu_DrawBasics(m, 0x104, 0x3A);
            break;
        case 3:
            DojoMenu_DrawQuestPage(m->shake.dx + 0x104, m->shake.dy + 0x3A);
            break;
        }
        break;
    case 4:
        if (gLobby->questPaid && m->unkB6) {
            DojoMenu_DrawQuestPage(m->shake.dx + 0x104, m->shake.dy + 0x3A);
        }
        break;
    }
}

void DojoMenu_DrawMenu(s16 x, s16 y, s8 k);
void DojoMenu_Draw(void *task)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    switch (m->mode) {
    case 1:
        break;
    case 2:
        switch (m->step) {
        case 3:
        case 5:
        case 6:
        case 7:
            DojoMenu_DrawMenu(m->win.pos, 0x3A, 1);
            break;
        case 0:
        case 1:
        case 2:
            Font_SetSpacing(0x16, 0x16);
            Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, gHunterDojoTexts[0], 2, 0xFF);
            DojoMenu_DrawMenu(m->win.pos, 0x3A, 0);
            break;
        }
        break;
    case 3:
        switch (m->step) {
        case 0:
        case 1:
        case 2:
            Font_SetSpacing(0x16, 0x16);
            Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, gHunterDojoTexts[0], 2, 0xFF);
        case 5:
        case 6:
            DojoMenu_DrawCategories(m, m->win.pos, 0x3A);
            break;
        }
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6CB690", D_0082C840);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6CB690", D_0082C850);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6CB690", D_0082C860);

extern u8 gNpc14Follows;
extern u8 gPoogieHappyKind0;
extern u8 gPoogieFollowing;
Em *Npc_Find(u8 kind);
void Talk_Start(Em *npc, s8 quiet);
/* gMenuWork while the player pets the Poogie (PoogiePet_*) */
typedef struct PetMenu {
    /* 0x0 */ u8 step;
    /* 0x1 */ u8 talk;              /* the talk's step (Talk_Say lines) */
    /* 0x2 */ u8 unk2[2];
    /* 0x4 */ Em *task;             /* the Poogie */
} PetMenu;
void PoogiePet_Open(Em *task)
{
    PetMenu *m = (PetMenu *)gMenuWork;
    Em *obj = Npc_Find(0x12);

    if (obj != 0) {
        gNpc14Follows = 0;
        gTownMode = 0x10;
        gTownMenuOpen = 1;
        m->task = task;
        m->step = 0;
        m->talk = 0;
        gPoogieHappyKind0 = 1;
        gPoogieFollowing = 0;
        Actor_SetState(obj, 0, 0x2C);
        Talk_Start(obj, 1);
        LobbySe_Play(0xE);
        CamFocus_Start(task);
    }
}

s8 Poogie_UpdatePetting(Em *task);
void Jingle_Play(void *obj, s32 id);
void CamFocus_End(void);
s32 CamFocus_IsOn(void);
void Actor_SetIdle(void *pl, s32 a, s32 n, s32 m);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_RunFiltered(char *text, s16 held, s16 pressed);
char *Talk_LineText(u16 id);
char *Text_FillDollarFmt(char *fmt, char *name);
void Kbd_Cancel(void);
extern u8 gTownJingleOn;
extern u8 gNpc14Left;
extern u16 gTalkLine;
extern char *gTalkLineText;
extern u8 gNpc14ByPlayer;

void PoogiePet_Update(void)
{
    PetMenu *m = (PetMenu *)&gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 *sv = (u8 *)&gSave;
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&m->task->work;
    u8 *e;

    if (Game_IsOnline() == 1) {
        return;
    }
    switch (m->step) {
    case 0:
        if (w->unk44 == 0 && m->task->state != 0x2C) {
            pl->cookTimer = 0;
            Actor_SetState(m->task, 0, 0x2C);
            Jingle_Play(pl, 4);
            gTownJingleOn = 0;
            m->step++;
        }
        break;
    case 1:
        switch ((u8)Poogie_UpdatePetting(m->task)) {
        case 2:
            CamFocus_End();
            gNpc14Follows = 1;
            m->step++;
            break;
        case 1:
            CamFocus_End();
            gNpc14Left = 1;
            m->step = 3;
            break;
        case 0:
        default:
            CamFocus_End();
            m->step++;
            break;
        case 3:
            break;
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_SetIdle(pl, 0, 8, 0);
            if (pl->cookResult == 2 || pl->cookResult == 4) {
                Talk_Say(3, 0);
                m->step = 4;
            } else {
                gTownMenuOpen = 0;
                CamFocus_End();
                gTownMode = 0;
                e = (u8 *)&w->route[w->routeIdx];
                if (gNpc14Follows == 1) {
                    Actor_SetState(m->task, 0, 0x58);
                } else {
                    Actor_SetState(m->task, 0, *(u16 *)(e + 0xC));
                }
            }
        }
        pl->weaponShown = 0;
        break;
    case 3:
        pl->weaponShown = 0;
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
            if (m->talk == 0) {
                Talk_Say(5, 0);
                m->talk = 1;
            } else if (m->talk == 1 || m->talk == 4) {
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 0xF, 8, sv + 0x2C98);
                gFieldControl = 0;
                m->step = 6;
            } else if (m->talk == 2) {
                Talk_Say((u16)Rand_Next(1) % 4 + 8, 0);
                m->talk = 3;
            } else if (m->talk == 3) {
                m->step = 5;
            } else if (m->talk == 5) {
                m->step = 5;
            }
            break;
        case 3:
            if (m->talk == 0) {
                Talk_Say(4, 0);
                m->talk = 4;
            } else if (m->talk == 4) {
                Talk_Say(6, 0);
                m->talk = 5;
            } else if (m->talk == 2) {
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 0xF, 8, sv + 0x2C98);
                gFieldControl = 0;
                m->step = 6;
            }
            break;
        }
        break;
    case 5:
        w->unk44 = 0;
        gTownMenuOpen = 0;
        if (CamFocus_IsOn() == 1) {
            CamFocus_End();
        }
        gTownMode = 0;
        break;
    case 6:
        if (Kbd_RunFiltered((char *)(sv + 0x2C98), *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            if ((s8)sv[0x2C98] == 0) {
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(0, 0xF, 8, sv + 0x2C98);
            } else {
                m->step = 4;
                Talk_Say(7, 0);
                gTalkLineText = Text_FillDollarFmt(Talk_LineText(gTalkLine), (char *)(sv + 0x2C98));
                m->talk = 2;
                gNpc14ByPlayer = 1;
                gPoogieHappyKind0 = 0;
                Kbd_Cancel();
                gFieldControl = 1;
            }
        }
        break;
    }
}

void PoogieMenu_Init(Em *task)
{
    PoogieMenu *m;

    gTownMode = 0x11;
    m = (PoogieMenu *)gMenuWork;
    gTownMenuOpen = 1;
    m->task = task;
    m->unk0[0] = 0;
    m->unk0[1] = 0;
    m->unk0[2] = 0;
    m->unk0[4] = 0;
    m->owned[0] = 1;
    PoogieMenu_CheckOutfits(&m->owned[1]);
    m->sel = PoogieMenu_LastCursor();
    m->lines[0] = "";
    m->count = 0;
    m->unk29 = -1;
    m->win.state = 0;
    m->win.pos = 0x280;
    m->unkD5[0] = 0;
    m->unkD5[1] = 0;
    m->unkD9 = 0;
    m->unkDA = 0x96;
    m->unk0[3] = 0;
    m->unkDC = 0;
    m->unkDD = 0;
    m->unkEE = 0;
    m->mood[0] = gPoogieMoods;
    m->mood[1] = D_008BD6C6;
    m->mood[2] = D_008BD6C8;
    m->mood[3] = D_008BD6CA;
    if (gOnlineBonusState == 2) {
        Notice_PushItem(TXT(0x0082C8A0) /* notice: the pet is holding something in its mouth */);
    }
    Actor_SetState(m->task, 0, 0xA9);
    LobbySe_Play(0xE);
    CamFocus_Start(task);
    LobbyMenu_SetDraw(PoogieMenu_Draw, 0, 0, 0);
}

void Bgm_FadeToVolume(s32 a);
void Cook_Reset(void);
void PoogieMenu_BuildItemList(void);
u8 PoogieMenu_RowsOnPage(PoogieMenu *m);
s32 PoogieMenu_MotionDone(void);
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
void Poogie_NotifyMood(u8 kind);
s32 Poogie_UpdatePettingDebug(Em *task);
u8 PoogieMenu_CursorOutfit(void);
s32 Debug_IsOn(s32 arg0);
void Shake_Start(u8 id, u8 flags, f32 amp);
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
void Shake_Stop(void);
void TownMsg_SendStateNow(Player *p);
extern f32 D_007E81FC[];
extern f32 D_007E8204[];
extern u8 gPoogiePetStreak;

void PoogieMenu_Update(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&m->task->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 *sv = (u8 *)&gSave;
    u8 *e;
    u16 k;
    u8 i;
    u8 n;
    s16 *p;
    f32 f;
    f32 *t;

    if (gOnlineBonusState == 2) {
        switch (m->unk0[3]) {
        case 0:
            if (m->task->motion[0].playing <= 0) {
                w->unk4 = 0;
                Actor_SetState(m->task, 0, 0x5E);
                Jingle_Play(pl, 5);
                Bgm_FadeToVolume(0xF);
                m->unk0[3]++;
            }
            break;
        case 1:
            if (m->task->motion[0].playing <= 0 || m->task->state == 0x58) {
                gNpc14Follows = 0;
                w->unk44 = 0;
                Actor_SetState(m->task, 0, 0xA9);
                m->unk0[3]++;
            }
            break;
        case 2:
            if (m->task->state == 1) {
                m->unk0[3]++;
            }
            break;
        case 3:
            f = 1.0f + 0.01f * ((u16)Rand_Next(1) % 10000);
            t = D_007E8204;
            for (i = 1; i <= 0x1F; i += 2) {
                if (f <= *t) {
                    k = (s32)D_007E81FC[i];
                    break;
                }
                t += 2;
            }
            /* the game reads a register it never set: k is s1, still the caller's, when f passes every threshold of the table */
            Pouch_GiveItem(pl, k, pl->gotItem, 1);
            m->unkDD = 0x1E;
            m->unk0[3]++;
            break;
        case 4:
            if (m->unkDD-- <= 0) {
                LobbyMenu_SetDraw(0, 0, 0, 0);
                Actor_SetIdle(pl, 0, 8, 0);
                gTownMenuOpen = 0;
                CamFocus_End();
                gTownMode = 0;
                gOnlineBonusState = 0;
            }
            break;
        }
    } else {
        switch (m->unk0[0]) {
        case 0:
            if (pad & 0x3000) {
                if (Game_IsOnline() == 0) {
                    m->unk0[4] = Menu_WrapUpDown(m->unk0[4], 3);
                } else {
                    m->unk0[4] = Menu_WrapUpDown(m->unk0[4], 2);
                }
            }
            if (m->unkDD++ >= 6) {
                m->unkDD = 6;
            }
            if (m->unkDD >= 6 && pad & 0x20) {
                switch (m->unk0[4]) {
                case 0:
                    if (Debug_IsOn(0x139)) {
                        m->unk0[0] = 6;
                        m->unkDD = 0;
                    } else {
                        Actor_SetState(m->task, 0, 0x2C);
                        Jingle_Play(pl, 4);
                        gTownJingleOn = 0;
                        Cook_Reset();
                        m->unk0[0] = 1;
                    }
                    break;
                case 1:
                    PoogieMenu_BuildItemList();
                    m->unk0[0] = 1;
                    m->unk0[1] = 0;
                    m->win.state = 0;
                    m->win.pos = 0x280;
                    m->unkD5[0] = 0;
                    break;
                case 2:
                    m->sel = PoogieMenu_LastCursor();
                    Shake_Start(0, 0, 180.0f);
                    m->unk0[0] = 1;
                    break;
                }
                LobbySe_Play(8);
            }
            if (pad & 0x40) {
                m->unk0[0] = 4;
                Actor_SetMotionOnLayer(m->task, 0x3E9, 0x18, 0, 0);
                LobbySe_Play(4);
            }
            break;
        case 1:
            switch (m->unk0[4]) {
            case 0:
                if (Debug_IsOn(0x139)) {
                    m->unk29 = Poogie_UpdatePettingDebug(m->task);
                } else {
                    m->unk29 = Poogie_UpdatePetting(m->task);
                }
                if (m->unk29 == 3) {
                    break;
                }
                Actor_SetMotionOnLayer(m->task, 0x3E9, 0x18, 0, 0);
                switch (m->unk29) {
                case 2:
                    gNpc14Follows = 1;
                    if (m->task->state == 0x5E) {
                        if (!Debug_IsOn(0x139)) {
                            gPoogiePetStreak++;
                            Poogie_NotifyMood(1);
                        }
                    } else {
                        gPoogiePetStreak = 0;
                    }
                    if (gPoogiePetStreak >= 2) {
                        m->unk0[0] = 4;
                        gPoogiePetStreak = 0;
                        m->unkDC = 1;
                    } else {
                        m->unk0[0] = 2;
                    }
                    break;
                case 1:
                    m->unk0[0] = 3;
                    gNpc14Left = 1;
                    gPoogiePetStreak = 0;
                    break;
                case 0:
                    m->unk0[0] = 4;
                    gPoogiePetStreak = 0;
                    break;
                }
                break;
            case 1:
                switch (m->unk0[1]) {
                case 0:
                    switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
                    case 0:
                        m->unkDD = 0;
                        m->unk0[1]++;
                        break;
                    }
                    break;
                case 1:
                    if (pad & 0x3000) {
                        m->unkD5[0] = Menu_WrapUpDown(m->unkD5[0], PoogieMenu_RowsOnPage(m));
                    }
                    if (pad & 0xC00) {
                        m->page = Menu_WrapLeftRight(m->page, m->pages);
                        n = PoogieMenu_RowsOnPage(m);
                        if (n < m->unkD5[0] + 1) {
                            m->unkD5[0] = n - 1;
                        }
                    }
                    if (pad & 0x200 && m->rows > 0) {
                        m->unkD9 ^= 1;
                        LobbySe_Play(0x39);
                    }
                    if (m->unkDD++ >= 6) {
                        m->unkDD = 6;
                    }
                    if (m->unkDD >= 6 && pad & 0x20 && m->rows > 0) {
                        m->unk0[1]++;
                        m->unkD5[1] = m->unkD5[0];
                        LobbySe_Play(8);
                    }
                    if (pad & 0x40) {
                        m->unkD9 = 0;
                        m->unk0[0] = 0;
                        m->unkDD = 0;
                        LobbySe_Play(4);
                    }
                    break;
                case 2:
                    if (PoogieMenu_Feed() == 0) {
                        m->unk0[0] = 4;
                    }
                    break;
                }
                break;
            case 2:
                if (pad & 0x3000) {
                    m->sel = Menu_WrapUpDown((u8)m->sel, m->count);
                }
                if (m->unkDD++ >= 6) {
                    m->unkDD = 6;
                }
                if (m->unkDD >= 6 && pad & 0x20) {
                    sv[0x2CAA] = PoogieMenu_CursorOutfit();
                    LobbySe_Play(8);
                }
                if (pad & 0x40) {
                    m->unk0[0] = 0;
                    m->unkDD = 0;
                    Shake_Stop();
                    LobbySe_Play(4);
                }
                break;
            }
            break;
        case 2:
            if (Debug_IsOn(0x139)) {
                gPoogieMoods = m->mood[0];
                D_008BD6C6 = m->mood[1];
                D_008BD6C8 = m->mood[2];
                D_008BD6CA = m->mood[3];
            }
            LobbyMenu_SetDraw(0, 0, 0, 0);
            if (PoogieMenu_MotionDone()) {
                Actor_SetIdle(pl, 0, 8, 0);
                TownMsg_SendStateNow(&gPlayers[gScene.localPlayer]);
                gTownMenuOpen = 0;
                CamFocus_End();
                gTownMode = 0;
                gOnlineBonusState = m->unkDC;
            }
            break;
        case 3:
            LobbyMenu_SetDraw(0, 0, 0, 0);
            if (Debug_IsOn(0x139) && m->task->motionId[0] == 0x3FA
                && m->task->motion[0].playing <= 0) {
                m->unk0[0] = 4;
                w->unk44 = 0;
            }
            break;
        case 4:
            if (m->unkDC == 1) {
                if (m->unk0[4] == 0 && !PoogieMenu_MotionDone()) {
                    break;
                }
                Actor_SetState(m->task, 0, 0x56);
                gNpc14Follows = 0;
            } else {
                if ((m->task->state == 0x55 || m->task->state == 0x5E)
                    && m->task->motion[0].playing > 0) {
                    break;
                }
                if (gNpc14Follows == 1) {
                    Actor_SetState(m->task, 0, 0x58);
                } else {
                    e = (u8 *)&w->route[w->routeIdx];
                    Actor_SetState(m->task, 0, *(u16 *)(e + 0xC));
                }
            }
            w->unk44 = 0;
            m->unk0[0] = 2;
            break;
        case 6:
            if (pad & 0x3000) {
                m->unkEE = Menu_WrapUpDown(m->unkEE, 4);
            }
            if (pad & 0x400 || pad & 0x80) {
                if (m->unkEE == 2) {
                    if (m->mood[2] < m->mood[1]) {
                        if (pad & 0x80) {
                            m->mood[m->unkEE] += 30;
                        } else {
                            m->mood[m->unkEE] += 1;
                        }
                    }
                } else if (pad & 0x80) {
                    m->mood[m->unkEE] += 30;
                } else {
                    m->mood[m->unkEE] += 1;
                }
            }
            if (pad & 0x800 || pad & 0x8) {
                p = (s16 *)(m->unkEE * 2 + (u8 *)m->mood);
                if (*p > 0) {
                    if (pad & 0x8) {
                        if (*p >= 30) {
                            *p -= 30;
                        } else {
                            *p = 0;
                        }
                    } else {
                        *p -= 1;
                    }
                }
            }
            if (m->unkDD++ >= 6) {
                m->unkDD = 6;
            }
            if (m->unkDD >= 6 && pad & 0x20) {
                Actor_SetState(m->task, 0, 0x2C);
                Jingle_Play(pl, 4);
                gTownJingleOn = 0;
                Cook_Reset();
                m->unk0[0] = 1;
                LobbySe_Play(8);
            }
            break;
        }
    }
}

extern u8 gPoogieDigState;
void Menu31_Open(Em *task)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];

    gTownMode = 0x31;
    gTownMenuOpen = 1;
    m->task = task;
    m->unk0[0] = 0;
    m->unk0[4] = 0;
    m->unk0[1] = 0;
    m->unk0[2] = 0;
    m->unk0[3] = 0;
    m->unk29 = -1;
    m->win.state = 0;
    m->win.pos = 0x280;
    m->unkD5[0] = 0;
    m->unkD5[1] = 0;
    m->unkD9 = 0;
    m->unkDA = 0x96;
    m->unkDD = 0;
    if (gPoogieDigState == 2) {
        Actor_SetState(m->task, 0, 0x2C);
        Jingle_Play(pl, 4);
        gTownJingleOn = 0;
        Cook_Reset();
        m->unk0[0] = 10;
        m->unk0[4] = 10;
    } else {
        m->unk0[0] = 0;
        Actor_SetState(m->task, 0, 0xA9);
    }
    LobbySe_Play(0xE);
    CamFocus_Start(task);
    LobbyMenu_SetDraw(PoogieMenu_DrawCare, 0, 0, 0);
}

s32 PoogieMenu_Feed(void);
void VillageGather_FindZone(f32 *pos, u8 *n);
extern s16 gPoogieDigTimer;
extern u8 gPoogieDigTaken;
extern u8 gPoogieDigSpotCount;
extern u8 *gPoogieDigSpots;
extern u8 gPoogieDigSpotCounts[];
extern u8 gPoogieDigSpotsZone1[];
extern u8 gPoogieDigSpotsZone2[];
extern u8 gPoogieDigSpotsZone3[];
extern u8 gPoogieDigSpotsZone4[];
extern u8 gPoogieDigSpotsZone5[];
extern u8 gPoogieDigSpotsZone6[];
extern s16 D_007E81F8[];

void Menu31_Update(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&m->task->work;
    s16 add;
    s16 *p;
    s16 *tbl;
    u8 rows;

    if (Game_IsOnline() == 1) {
        m->unk0[0] = 2;
    }
    switch (m->unk0[0]) {
    case 0:
        if (pad & 0x3000) {
            m->unk0[4] = Menu_WrapUpDown(m->unk0[4], 2);
        }
        if (m->unkDD++ >= 6) {
            m->unkDD = 6;
        }
        if (m->unkDD >= 6 && pad & 0x20) {
            switch (m->unk0[4]) {
            case 0:
                Actor_SetState(m->task, 0, 0x2C);
                Jingle_Play(pl, 4);
                gTownJingleOn = 0;
                Cook_Reset();
                m->unk0[0] = 1;
                break;
            case 1:
                PoogieMenu_BuildItemList();
                m->unk0[0] = 1;
                m->unk0[1] = 0;
                m->win.state = 0;
                m->win.pos = 0x280;
                m->unkD5[0] = 0;
                m->unkDD = 0;
                break;
            }
            LobbySe_Play(8);
        }
        if (pad & 0x40) {
            m->unk0[0] = 4;
            Actor_SetMotionOnLayer(m->task, 0x3E9, 0x18, 0, 0);
            LobbySe_Play(4);
        }
        break;
    case 1:
        switch (m->unk0[4]) {
        case 0:
            m->unk29 = Poogie_UpdatePetting(m->task);
            switch (m->unk29) {
            case 2:
                gNpc14Follows = 1;
                tbl = (s16 *)D_007E81F8;
                if (m->task->state == 0x55) {
                    add = tbl[0];
                } else {
                    add = tbl[1];
                }
                gLobby->poogieFeed += add;
                Poogie_NotifyMood(0);
                m->unk0[0] = 11;
                break;
            case 1:
                gNpc14Left = 1;
                m->unk0[0] = 3;
                break;
            case 0:
                m->unk0[0] = 4;
                break;
            case 3:
                break;
            }
            break;
        case 1:
            switch (m->unk0[1]) {
            case 0:
                switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
                case 0:
                    m->unk0[1]++;
                    break;
                }
                break;
            case 1:
                if (pad & 0x3000) {
                    m->unkD5[0] = Menu_WrapUpDown(m->unkD5[0], PoogieMenu_RowsOnPage(m));
                }
                if (pad & 0xC00) {
                    m->page = Menu_WrapLeftRight(m->page, m->pages);
                    rows = PoogieMenu_RowsOnPage(m);
                    if (m->unkD5[0] + 1 > rows) {
                        m->unkD5[0] = rows - 1;
                    }
                }
                if (pad & 0x200 && m->rows > 0) {
                    m->unkD9 ^= 1;
                    LobbySe_Play(0x39);
                }
                if (m->unkDD++ >= 6) {
                    m->unkDD = 6;
                }
                if (m->unkDD >= 6 && pad & 0x20 && m->rows > 0) {
                    m->unk0[1]++;
                    m->unkD5[1] = m->unkD5[0];
                    LobbySe_Play(8);
                }
                if (pad & 0x40) {
                    m->unkD9 = 0;
                    m->unk0[0] = 0;
                    m->unkDD = 0;
                    LobbySe_Play(4);
                }
                break;
            case 2:
                if (PoogieMenu_Feed() == 0) {
                    m->unk0[0] = 4;
                }
                break;
            }
            break;
        }
        break;
    case 11:
        p = &gLobby->poogieFeed;
        if (*p >= gLobby->poogieFeedMax) {
            *p = 0;
            m->unk0[0] = 5;
        } else {
            m->unk0[0] = 4;
        }
        break;
    case 3:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        break;
    case 5:
        if ((m->task->state != 0x55 && m->task->state != 0x5E) || m->task->motion[0].playing <= 0) {
            gPoogieDigTimer = 0;
            gPoogieDigTaken = 0;
            gPoogieDigZone[0] = 1;
            VillageGather_FindZone(&m->task->posX, gPoogieDigZone);
            gPoogieDigSpotCount = gPoogieDigSpotCounts[gPoogieDigZone[0]];
            switch (gPoogieDigZone[0]) {
            case 0:
                break;
            case 1:
                gPoogieDigSpots = gPoogieDigSpotsZone1;
                break;
            case 2:
                gPoogieDigSpots = gPoogieDigSpotsZone2;
                break;
            case 3:
                gPoogieDigSpots = gPoogieDigSpotsZone3;
                break;
            case 4:
                gPoogieDigSpots = gPoogieDigSpotsZone4;
                break;
            case 5:
                gPoogieDigSpots = gPoogieDigSpotsZone5;
                break;
            case 6:
                gPoogieDigSpots = gPoogieDigSpotsZone6;
                break;
            }
            Actor_SetState(m->task, 0, 0xAC);
            m->unk0[0] = 2;
        }
        break;
    case 4:
        if ((m->task->state != 0x55 && m->task->state != 0x5E) || m->task->motion[0].playing <= 0) {
            Actor_SetState(m->task, 0, 0x58);
            w->unk44 = 0;
            m->unk0[0] = 2;
        }
        break;
    case 2:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        if (PoogieMenu_MotionDone()) {
            w->unk44 = 0;
            Actor_SetIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = 0;
        }
        break;
    case 10:
        m->unk29 = Poogie_UpdatePetting(m->task);
        switch (m->unk29) {
        case 0:
        case 2:
            gNpc14Follows = 1;
            pl->cookResult = 0;
            m->unk0[0] = 4;
            if (m->task->state == 0x5E) {
                Poogie_NotifyMood(3);
            }
            break;
        case 1:
            m->unk0[0] = 3;
            gNpc14Left = 1;
            break;
        case 3:
            break;
        }
        break;
    }
}

void PoogieMenu_OpenCare(Em *task)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;

    gTownMode = 0x12;
    gTownMenuOpen = 1;
    m->task = task;
    m->unk0[0] = 0;
    m->unk0[4] = 0;
    m->unk0[1] = 0;
    m->unk0[2] = 0;
    m->unk0[3] = 0;
    m->unk29 = -1;
    m->win.state = 0;
    m->win.pos = 0x280;
    m->unkD5[0] = 0;
    m->unkD5[1] = 0;
    m->unkD9 = 0;
    m->unkDA = 0x96;
    m->unkDD = 0;
    Actor_SetState(m->task, 0, 0xA9);
    LobbySe_Play(0xE);
    CamFocus_Start(task);
    LobbyMenu_SetDraw(PoogieMenu_DrawCare, 0, 0, 0);
}

s32 VillageGather_IsSeasonItem(u16 item);
extern u8 gPoogieDigState;
extern u8 gNpc22Left;
extern u8 gPoogieDigSpotIdx;
extern u8 gPoogieHappyKind1;
extern u8 gPoogieFedSeasonItem;
extern u8 gPoogieDigSpotsSeason2[];
extern u8 gPoogieDigSpotsSeason0[];
extern u8 gPoogieDigSpotsSeason1[];

void PoogieMenu_UpdateCare(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    NpcWork *w = (NpcWork *)&m->task->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    s16 add;
    s16 *tbl;
    u8 rows;

    switch (m->unk0[0]) {
    case 0:
        if (gPoogieDigState == 2) {
            Actor_SetState(m->task, 0, 0x2C);
            Jingle_Play(pl, 4);
            gTownJingleOn = 0;
            Cook_Reset();
            m->unk0[0] = 7;
            break;
        }
        if (pad & 0x3000) {
            m->unk0[4] = Menu_WrapUpDown(m->unk0[4], 2);
        }
        if (m->unkDD++ >= 6) {
            m->unkDD = 6;
        }
        if (m->unkDD >= 6 && pad & 0x20) {
            switch (m->unk0[4]) {
            case 0:
                Actor_SetState(m->task, 0, 0x2C);
                Jingle_Play(pl, 4);
                gTownJingleOn = 0;
                Cook_Reset();
                m->unk0[0] = 1;
                break;
            case 1:
                PoogieMenu_BuildItemList();
                m->unk0[0] = 1;
                m->unk0[1] = 0;
                m->win.state = 0;
                m->win.pos = 0x280;
                m->unkD5[0] = 0;
                m->unkDD = 0;
                break;
            }
            LobbySe_Play(8);
        }
        if (pad & 0x40) {
            m->unk0[0] = 2;
            Actor_SetMotionOnLayer(m->task, 0x3E9, 0x18, 0, 0);
            LobbySe_Play(4);
        }
        break;
    case 1:
        switch (m->unk0[4]) {
        case 0:
            m->unk29 = Poogie_UpdatePetting(m->task);
            if (m->unk29 != 3) {
                Actor_SetMotionOnLayer(m->task, 0x3E9, 0x18, 0, 0);
                switch (m->unk29) {
                case 2:
                    tbl = (s16 *)D_007E81F8;
                    if (m->task->state == 0x55) {
                        add = tbl[0];
                    } else {
                        add = tbl[1];
                    }
                    gNpc14Follows = 1;
                    gLobby->poogieCare += add;
                    Poogie_NotifyMood(0);
                    if (gLobby->poogieCare >= gLobby->poogieCareMax) {
                        m->unk0[0] = 5;
                        gLobby->poogieCare = 0;
                    } else {
                        m->unk0[0] = 2;
                    }
                    break;
                case 1:
                    m->unk0[0] = 3;
                    gNpc22Left = 1;
                    break;
                case 0:
                    m->unk0[0] = 2;
                    break;
                }
            }
            break;
        case 1:
            switch (m->unk0[1]) {
            case 0:
                switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
                case 0:
                    m->unk0[1]++;
                    break;
                }
                break;
            case 1:
                if (pad & 0x3000) {
                    m->unkD5[0] = Menu_WrapUpDown(m->unkD5[0], PoogieMenu_RowsOnPage(m));
                }
                if (pad & 0xC00) {
                    m->page = Menu_WrapLeftRight(m->page, m->pages);
                    rows = PoogieMenu_RowsOnPage(m);
                    if (m->unkD5[0] + 1 > rows) {
                        m->unkD5[0] = rows - 1;
                    }
                }
                if (pad & 0x200 && m->rows > 0) {
                    m->unkD9 ^= 1;
                    LobbySe_Play(0x39);
                }
                if (m->unkDD++ >= 6) {
                    m->unkDD = 6;
                }
                if (m->unkDD >= 6 && pad & 0x20 && m->rows > 0) {
                    m->unk0[1]++;
                    m->unkD5[1] = m->unkD5[0];
                    gPoogieDigSeason = LobbyWorld_GetSeason();
                    if (VillageGather_IsSeasonItem(m->items[m->unkD5[1]]) == 1) {
                        gPoogieFedSeasonItem = 1;
                    }
                    LobbySe_Play(8);
                }
                if (pad & 0x40) {
                    m->unkD9 = 0;
                    m->unk0[0] = 0;
                    m->unkDD = 0;
                    LobbySe_Play(4);
                }
                break;
            case 2:
                if (PoogieMenu_Feed() == 0) {
                    if (gLobby->poogieCare <= 0) {
                        gPoogieHappyKind1 = 1;
                    }
                    m->unk0[1]++;
                }
                break;
            case 3:
                if ((m->task->state != 0x55 && m->task->state != 0x5E) || m->task->motion[0].playing <= 0) {
                    m->unk0[0] = 2;
                }
                break;
            }
            break;
        }
        break;
    case 5:
        if (m->task->motion[0].playing <= 0 || m->unk0[1] == 2) {
            gPoogieDigTimer = 0;
            gPoogieDigSpotIdx = 0;
            gPoogieDigZone[0] = 1;
            gPoogieDigTaken = 0;
            gPoogieDigSeason = LobbyWorld_GetSeason();
            switch (gPoogieDigSeason) {
            case 0:
                gPoogieDigSpotCount = 4;
                gPoogieDigSpots = gPoogieDigSpotsSeason0;
                break;
            case 1:
                gPoogieDigSpotCount = 2;
                gPoogieDigSpots = gPoogieDigSpotsSeason1;
                break;
            case 2:
            default:
                gPoogieDigSpotCount = 3;
                gPoogieDigSpots = gPoogieDigSpotsSeason2;
                break;
            }
            Actor_SetState(m->task, 0, 0xAC);
            w->unk44 = 0;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            Actor_SetIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = 0;
        }
        break;
    case 2:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        if (PoogieMenu_MotionDone()) {
            Actor_SetState(m->task, 0, *(u16 *)&w->route[w->routeIdx].state);
            w->unk44 = 0;
            Actor_SetIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = 0;
        }
        break;
    case 3:
        LobbyMenu_SetDraw(0, 0, 0, 0);
        break;
    case 7:
        m->unk29 = Poogie_UpdatePetting(m->task);
        switch (m->unk29) {
        case 0:
        default:
            m->unk0[0] = 2;
            break;
        case 2:
            gNpc14Follows = 1;
            if (m->task->state == 0x5E) {
                m->unk0[0] = 8;
            } else {
                m->unk0[0] = 2;
            }
            if (m->task->state == 0x5E) {
                Poogie_NotifyMood(3);
            }
            break;
        case 1:
            m->unk0[0] = 3;
            gNpc22Left = 1;
            break;
        case 3:
            break;
        }
        break;
    case 8:
        if (m->task->motion[0].playing <= 0) {
            gPoogieDigTaken = 0;
            Actor_SetState(m->task, 0, 0xAC);
            w->unk44 = 0;
            LobbyMenu_SetDraw(0, 0, 0, 0);
            Actor_SetIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = 0;
        }
        break;
    case 4:
    case 6:
        break;
    }
}

void Menu13_Open(Em *task)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;

    gTownMode = 0x13;
    gTownMenuOpen = 1;
    m->task2 = task;
    m->unkDE = 0;
    m->unkE4 = 0;
    Actor_SetState(m->task2, 0, 0xA9);
    LobbySe_Play(0xE);
    CamFocus_Start(m->task2);
}

extern u8 gNpc6DLeft;

void Menu13_Update(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&m->task2->work;

    switch (m->unkDE) {
    case 0:
        Actor_SetState(m->task2, 0, 0x2C);
        Jingle_Play(pl, 4);
        gTownJingleOn = 0;
        Cook_Reset();
        m->unkDE++;
        break;
    case 1:
        m->unkE4 = Poogie_UpdatePetting(m->task2);
        switch (m->unkE4) {
        case 0:
        default:
            m->unkDE = 2;
            break;
        case 2:
            m->unkDE = 9;
            break;
        case 1:
            m->unkDE = 3;
            gNpc6DLeft = 1;
            break;
        case 3:
            break;
        }
        break;
    case 3:
        break;
    case 9:
        if (m->task2->state == 0x5E) {
            Poogie_NotifyMood(2);
            if (Save_TestEventFlag(0x76) == 0) {
                Save_SetEventFlag(0x76);
                Notice_PushItem(TXT(0x0082C960) /* notice: a new outfit for the pet */);
            }
        }
        m->unkDE = 2;
        w->unk44 = 0;
        break;
    case 2:
        if (PoogieMenu_MotionDone()) {
            Actor_SetIdle(pl, 0, 8, 0);
            gTownMenuOpen = 0;
            CamFocus_End();
            gTownMode = 0;
            if (m->unkE4 == 2) {
                Actor_SetState(m->task2, 0, 0x58);
            } else {
                Actor_SetState(m->task2, 0, *(u16 *)&w->route[w->routeIdx].state);
            }
        }
        break;
    }
}

extern char *gPoogieMenuLabels[];
void PoogieMenu_OpenOutfitList(void);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);

void PoogieMenu_Draw(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    s16 x;
    s16 cx;
    s16 bx;
    s32 g;

    Text_SetSize(0);
    if (gOnlineBonusState == 0) {
        switch (m->unk0[0]) {
        case 0:
            if (Game_IsOnline() == 0) {
                Menu_DrawLabelList(0x1CC, (s8 **)(((u8 *)gPoogieMenuLabels)), 3, m->unk0[4], 0xFFB0B000);
            } else {
                Menu_DrawLabelList(0x1CC, (s8 **)(((u8 *)gPoogieMenuLabels)), 2, m->unk0[4], 0xFFB0B000);
            }
            break;
        case 1:
            if (m->unk0[4] == 2) {
                PoogieMenu_OpenOutfitList();
            } else if (m->unk0[4] == 1) {
                switch (m->unk0[1]) {
                case 0:
                    x = m->win.pos;
                    break;
                case 1:
                    x = 0x118;
                    break;
                }
                if (m->unk0[1] <= 1) {
                    PoogieMenu_DrawFeedList(x, 0x34);
                }
            }
            break;
        case 6:
            Font_SetSpacing(0x12, 0x12);
            Font_SetColor(0x807F7F28);
            Font_SetPos(0x14, 0xB4);
            switch (m->unkEE) {
            case 0:
                Text_PrintfColored(TXT(0x0082C990) /* the pet's mood bar label (green) */);
                break;
            case 1:
                Text_PrintfColored(TXT(0x0082C9B0) /* the pet's mood bar label (blue) */);
                break;
            case 2:
                Text_PrintfColored(TXT(0x0082C9D0) /* the pet's mood bar label (red) */);
                break;
            case 3:
                Text_PrintfColored(TXT(0x0082C9F0) /* the pet's mood bar label (purple) */);
                break;
            }
            cx = 0x14;
            g = m->mood[0];
            bx = g + (m->mood[1] - m->mood[2]) / 2 + 0x15;
            Gfx_DrawLine(0x14, 0xC8, g, 5, 0xFF36C018);
            cx += m->mood[0] + 1;
            Gfx_DrawLine(cx, 0xC8, m->mood[1], 5, 0xFF0000FF);
            cx += m->mood[1] + 1;
            Gfx_DrawLine(bx, 0xC8, m->mood[2], 5, 0xFFFF0000);
            Gfx_DrawLine(cx, 0xC8, m->mood[3], 5, 0xFFAA00FF);
            Font_SetPos(0x14, 0xD7);
            Text_PrintfColored(TXT(0x0082CA10) /* debug: the frame count */, m->mood[m->unkEE]);
            break;
        }
    }
}

void PoogieMenu_DrawCare(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    s16 x;

    Text_SetSize(0);
    switch (m->unk0[0]) {
    case 0:
        Menu_DrawLabelList(0x1CC, (s8 **)(((u8 *)gPoogieCareLabels)), 2, m->unk0[4], 0xFFB0B000);
        break;
    case 1:
        if (m->unk0[4] == 1) {
            switch (m->unk0[1]) {
            case 0:
                x = m->win.pos;
                break;
            case 1:
                x = 0x118;
                break;
            }
            if (m->unk0[1] < 2) {
                PoogieMenu_DrawFeedList(x, 0x34);
            }
        }
        break;
    }
}

s32 Motion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 Actor_HeadingDiff(void *obj, void *pos);
void Actor_SetMotion(void *pl, s32 motion, s32 frames, s32 c);
void ModelObj_SpawnOn(void *arg0, s32 arg1);

/* A frame of petting the Poogie (kind: Village_AreaKind); see
 * Poogie_UpdatePettingDebug for the results. */
s8 Poogie_UpdatePetting(Em *task)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    s16 *t;
    u8 *save;
    u8 kind;
    s16 lo;

    pl->weaponShown = 0;
    save = (u8 *)&gSave;
    (pl->cookTimer)++;
    pl->targetYaw += (s16)(u16)Actor_HeadingDiff(pl, &((Em *)pl->target)->posX) / 5;
    kind = Village_AreaKind();
    switch (kind) {
    case 1:
        t = gCookRangeKind1;
        break;
    case 2:
    case 0:
    default:
        t = gCookRangeDefault;
        break;
    }
    if (pl->cookTimer >= t[0] + t[1] + 1) {
        pl->cookResult = 3;
    } else {
        lo = t[0] + (t[1] - t[2]) / 2 + 1;
        if (pl->cookTimer >= lo && pl->cookTimer <= lo + t[2]) {
            pl->cookResult = 4;
        } else if (pl->cookTimer >= t[0] + 1) {
            if ((gPoogieHappyKind0 == 1 && kind == 0) || (gPoogieHappyKind1 == 1 && kind == 1)) {
                ModelObj_SpawnOn(pl, 8);
            }
            pl->cookResult = 2;
        } else if (pl->cookTimer >= 0xB4) {
            pl->cookResult = 1;
        } else {
            pl->cookResult = 0;
        }
    }
    if ((Motion_ReachedFrame(pl, 0, 130.0f) && (pad & 0x20))
        || pl->cookTimer >= t[3] + (t[0] + t[1])) {
        switch (pl->cookResult) {
        case 0:
        case 1:
            Actor_SetMotion(pl, 0x26B, 4, 0);
            Jingle_Play(pl, 0xE);
            Bgm_FadeToVolume(0xF);
            return 0;
        case 2:
            Actor_SetState(task, 0, 0x55);
            Actor_SetMotion(pl, 0x26B, 4, 0);
            Jingle_Play(pl, 5);
            Bgm_FadeToVolume(0xF);
            return 2;
        case 3:
            Actor_SetState(task, 0, 0x54);
            switch (Village_AreaKind()) {
            case 1:
                Notice_PushItem(TXT(0x0082CA20) /* notice: the pet got angry (the special pet) */);
                break;
            case 2:
                Notice_PushItem(TXT(0x0082CA50) /* notice: the pet got angry (the sheep outfit) */);
                break;
            case 0:
            default:
                if ((s8)save[0x2C98] != 0) {
                    Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CA70) /* notice: the pet got angry ($: its name) */, (char *)save + 0x2C98));
                }
                break;
            }
            Jingle_Play(pl, 0x10);
            Bgm_FadeToVolume(0xF);
            return 1;
        case 4:
            Actor_SetState(task, 0, 0x5E);
            Actor_SetMotion(pl, 0x26B, 4, 0);
            Jingle_Play(pl, 5);
            Bgm_FadeToVolume(0xF);
            return 2;
        }
    }
    return 3;
}


/* The debug twin of Poogie_UpdatePetting: the thresholds from the menu's
 * copies (mood[]), an English notice. A frame of petting the Poogie: counts
 * the frames (pl+0x822) against the thresholds into a result (pl+0x828); at the motion's frame 130 with the
 * button, or when too long, ends: returns 0 (results 0, 1), 1 (3: too long,
 * the Poogie gets angry), 2 (results 2, 4); 3 while petting. */
s32 Poogie_UpdatePettingDebug(Em *task)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    s16 lo;

    (pl->cookTimer)++;
    pl->targetYaw += (s16)(u16)Actor_HeadingDiff(pl, &((Em *)pl->target)->posX) / 5;
    if (pl->cookTimer >= m->mood[0] + m->mood[1] + 1) {
        pl->cookResult = 3;
    } else {
        lo = m->mood[0] + (m->mood[1] - m->mood[2]) / 2 + 1;
        if (pl->cookTimer >= lo && pl->cookTimer <= lo + m->mood[2]) {
            pl->cookResult = 4;
        } else if (pl->cookTimer >= m->mood[0] + 1) {
            ModelObj_SpawnOn(pl, 8);
            pl->cookResult = 2;
        } else if (pl->cookTimer >= 0xB4) {
            pl->cookResult = 1;
        } else {
            pl->cookResult = 0;
        }
    }
    if ((Motion_ReachedFrame(pl, 0, 130.0f) && (pad & 0x20))
        || pl->cookTimer >= m->mood[3] + (m->mood[0] + m->mood[1])) {
        switch (pl->cookResult) {
        case 0:
        case 1:
            Actor_SetMotion(pl, 0x26B, 4, 0);
            Jingle_Play(pl, 0xE);
            Bgm_FadeToVolume(0xF);
            return 0;
        case 2:
            Actor_SetState(task, 0, 0x55);
            Actor_SetMotion(pl, 0x26B, 4, 0);
            Jingle_Play(pl, 5);
            Bgm_FadeToVolume(0xF);
            return 2;
        case 3:
            Actor_SetState(task, 0, 0x54);
            Notice_PushItem(TXT(0x0082CA90) /* debug notice: the pet got angry */);
            Jingle_Play(pl, 0x10);
            Bgm_FadeToVolume(0xF);
            return 1;
        case 4:
            Actor_SetState(task, 0, 0x5E);
            Actor_SetMotion(pl, 0x26B, 4, 0);
            Jingle_Play(pl, 5);
            Bgm_FadeToVolume(0xF);
            return 2;
        }
    }
    return 3;
}

u8 PoogieMenu_RowsOnPage(PoogieMenu *m)
{
    u8 r;

    if (m->page == m->pages - 1) {
        if (m->rows <= 0) {
            r = 1;
        } else {
            r = m->rows - (m->pages - 1) * 7;
        }
    } else {
        r = 7;
    }
    return r;
}

void Cook_Reset(void)
{
    Player *p = &gPlayers[gScene.localPlayer];

    p->cookTimer = 0;
    p->cookResult = 0;
}

s32 PoogieMenu_CheckOutfits(u8 *owned)
{
    u8 i;
    s32 any = 0;

    for (i = 0; i < 4; i++) {
        owned[i] = 0;
        switch (i) {
        case 0:
            if (Save_TestEventFlag(0x74) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        case 1:
            if (Save_TestEventFlag(0x75) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        case 2:
            if (Save_TestEventFlag(0x76) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        case 3:
            if (Save_TestEventFlag(0x77) == 1) {
                owned[i] = 1;
                any = 1;
            }
            break;
        }
    }
    return any;
}

void PoogieMenu_OpenOutfitList(void)
{
    PoogieMenu *p = (PoogieMenu *)gMenuWork;
    s32 w;

    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    Shake_Update(0);
    w = (s32)Shake_GetOffset(0);
    gOutfitListBox.cursorColor = 0xA9182;
    PoogieMenu_BuildOutfitList();
    gOutfitListBox.items = p->lines;
    gOutfitListBox.lines = p->count;
    ListPanel_Draw(&gOutfitListBox, (s16 *)w, (char *)(TXT(0x0082CAB0) /* the outfit list's title, in colour 5 */), (u8)p->sel);
}

void PoogieMenu_BuildItemList(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    u8 i;

    memset(m->items, 0, sizeof(m->items));
    memset(m->prices, 0, sizeof(m->prices));
    m->rows = 0;
    for (i = 0; i < 20; i++) {
        if (Kitchen_IsIngredient(gSave.pouch[i].item) == 1) {
            m->items[m->rows] = gSave.pouch[i].item;
            m->prices[m->rows] = gItemData[gSave.pouch[i].item].price;
            m->rows++;
        }
    }
    m->unkD5[0] = 0;
    m->page = 0;
    m->pages = Menu_CountPages(m->rows, 7);
    if (m->rows <= 0) {
        m->pages = 1;
    }
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, s32 c1, s32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), PoogieMenu_DrawFeedList 194 of 209 words differ */
void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 n, u32 color);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);
extern char *gIngredientSelectTexts[];

void PoogieMenu_DrawFeedList(s16 x, s16 y)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    s16 x0 = x;
    u8 i;
    s32 item;
    s32 sel;
    u8 k;

    Win_Draw(x, y, 0x154, 0x120, 0xBDB59F6C, 0x80202020, 2);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x12E, 0x12, 0xC8, 0xFFB0B000, gIngredientSelectTexts[0], 2, 1);
    if (m->rows > 0) {
        Menu_DrawPageArrows(x0 + 0xF0, y + 10, m->page, m->pages, 0xBDB59F6C);
    }
    Font_SetColor(0x807F7F7F);
    if (m->rows <= 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetColor(0x802B7F7F);
        Font_SetPos(x + 0x14, y + 0x64);
        Text_PrintfColored(TXT(0x0082CAC0) /* notice: no food in the item pouch */);
    } else {
        x += 0x32;
        y += 0x1E;
        Font_SetSpacing(0x14, 0x14);
        for (i = 0; i < 7; i++) {
            k = i + m->page * 7;
            if (k >= m->rows) {
                y += 0x16;
            } else {
                item = m->items[k];
                if (i == m->unkD5[0]) {
                    sel = item;
                    Menu_DrawLine(x0 + 10, y - 1, 0x140, 0x18, 0);
                }
                Menu_DrawItemCountRow(x, y, item, 0x807F7F7F);
                y += 0x16;
            }
        }
    }
    if (m->rows > 0) {
        Gfx_DrawLine(x0 + 0x14, y + 2, 0x12C, 2, 0xBDB59F6C);
        ItemInfo_Draw(x - 0x28, y + 4, 0x154, m->unkD9, sel, 0, -1, 0x807F7F7F);
    } else {
        Gfx_DrawLine(x0 + 0x14, y + 0xD0, 0x12C, 2, 0xBDB59F6C);
    }
}

void PoogieMenu_BuildOutfitList(void)
{
    PoogieMenu *p = (PoogieMenu *)gMenuWork;
    u8 i;
    u8 n = 0;

    p->lines[n++] = gOutfitNames[0];
    for (i = 1; i < 5; i++) {
        if (p->owned[i] == 1) {
            p->lines[n++] = gOutfitNames[i];
        }
    }
    p->count = n;
}

s32 PoogieMenu_MotionDone(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    Em *npc;
    s8 kind;

    if (gScene.area == AREA_HUB_0x57 || gScene.area == AREA_HOUSE) {
        kind = m->unkE4;
        npc = m->task2;
    } else {
        kind = m->unk29;
        npc = m->task;
    }
    switch (kind) {
    case 2:
        if ((npc->state == 0x55 || npc->state == 0x5E) && npc->motion[0].playing > 0) {
            return 0;
        }
        break;
    case 1:
        if (npc->state == 0x54 && npc->motion[0].playing > 0) {
            return 0;
        }
        break;
    case 0:
        if (pl->motionId[0] == 0x26B && pl->motion[0].playing <= 0) {
            break;
        }
        return 0;
    default:
        return 1;
    }
    return 1;
}

u8 PoogieMenu_CursorOutfit(void)
{
    PoogieMenu *p = (PoogieMenu *)gMenuWork;
    s8 n = p->sel;
    u8 i;

    for (i = 0; i < 5; i++) {
        if (p->owned[i] == 1) {
            n--;
            if (n < 0) {
                return i;
            }
        }
    }
    return 0;
}

u8 PoogieMenu_LastCursor(void)
{
    PoogieMenu *p = (PoogieMenu *)gMenuWork;
    u8 n = 0;
    u8 i;

    for (i = 0; i < gSave.poogieOutfit + 1; i++) {
        if (p->owned[i] == 1) {
            n++;
        }
    }
    return n - 1;
}


s32 PoogieMenu_Feed(void)
{
    PoogieMenu *m = (PoogieMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 mood = Village_AreaKind();

    if (mood == 2) {
        return 0;
    }
    switch (m->unk0[2]) {
    case 0:
        Actor_SetState(m->task, 0, 0x2C);
        m->unkDA = 0x96;
        m->unk0[2]++;
        break;
    case 1:
        if (m->unkDA-- <= 0) {
            SavePouch_Add(m->items[m->unkD5[1] + m->page * 7], -1);
            Actor_SetMotion(pl, 0x26B, 4, 0);
            (&gLobby->poogieFeed)[mood] += m->prices[m->unkD5[1] + m->page * 7];
            if ((&gLobby->poogieFeed)[mood] >= (&gLobby->poogieFeedMax)[mood]) {
                Poogie_NotifyMood(0);
                (&gLobby->poogieFeed)[mood] = 0;
                if (mood == 1) {
                    gPoogieHappyKind1 = 1;
                } else {
                    gPoogieHappyKind0 = 1;
                }
                Actor_SetState(m->task, 0, 0x5E);
                Jingle_Play(pl, 5);
                Bgm_FadeToVolume(0xF);
            } else {
                if ((gPoogieHappyKind0 == 1 && mood != 1) || (gPoogieHappyKind1 == 1 && mood == 1)) {
                    Poogie_NotifyMood(3);
                    Actor_SetState(m->task, 0, 0x5E);
                } else {
                    Poogie_NotifyMood(0);
                    Actor_SetState(m->task, 0, 0x55);
                }
                Jingle_Play(pl, 5);
                Bgm_FadeToVolume(0xF);
            }
            return 0;
        }
        pl->weaponShown = 0;
        pl->targetYaw += (s16)(u16)Actor_HeadingDiff(pl, &((Em *)pl->target)->posX) / 5;
        break;
    }
    return 2;
}

void VillageGather_FindZone(f32 *pos, u8 *n)
{
    f32 a[3];
    f32 b[3];
    u8 k;

    if (*n == 6) {
        return;
    }
    k = (*n - 1) * 5;
    a[0] = gGatherZoneBounds[k];
    a[1] = -500.0f;
    a[2] = gGatherZoneBounds[k + 1];
    b[0] = gGatherZoneBounds[k + 2];
    b[1] = -500.0f;
    b[2] = gGatherZoneBounds[k + 3];
    if (Slab_Contains(pos, a, b, 552.0f, gGatherZoneBounds[k + 4]) == 1) {
        return;
    }
    (*n)++;
    VillageGather_FindZone(pos, n);
}

void VillageGather_DailyReset(void)
{
    u8 *save = (u8 *)&gSave;

    if (gSysSettings.unkE5 != 0) {
        gSysSettings.unkE5 = 0;
        gPoogieDigZone[0x17] = 0;
        gPoogieDigZone[0x18] = 0;
        gPoogieDigZone[0x19] = 0;
        gPoogieDigZone[0x1A] = 0;
        gPoogieDigZone[0x1B] = 0;
        gPoogieDigZone[0x1C] = 0;
        gPoogieDigZone[0x1D] = 0;
        gPoogieDigZone[0x1E] = 0;
        gPoogieDigZone[0x11] = 0;
        gPoogieDigZone[0x12] = 0;
        gPoogieDigZone[0x13] = 0;
        gPoogieDigZone[0x14] = 0;
        gPoogieDigZone[0x15] = 0;
        gPoogieDigZone[0x1F] = 0;
        gPoogieDigZone[0x20] = 0;
        gPoogieDigZone[0x21] = 0;
        gPoogieDigZone[0x23] = 0;
        if (*(s8 *)(save + 0x2C98) == 0) {
            gPoogieDigZone[0x22] = 1;
        } else {
            gPoogieDigZone[0x22] = 0;
        }
        gLobby->poogieFeedMax = VillageGather_RollValue(0);
        gLobby->poogieCareMax = VillageGather_RollValue(1);
    }
}

extern u16 gGatherItemsK0[];
extern u8 gGatherChanceK0[];
extern u16 gGatherItemsK1[];
extern u8 gGatherChanceSeason2[];
extern u8 gGatherChanceSeason0[];
extern u8 gGatherChanceSeason1[];
extern u8 gGatherChanceAltSeason2[];
extern u8 gGatherChanceAltSeason0[];
extern u8 gGatherChanceAltSeason1[];

u16 VillageGather_RollItem(u16 id, u8 k)
{
    u8 *chance;
    u8 n;
    u8 r;
    u8 i;
    u8 a;
    u8 c;
    u16 *items;

    if (k == 0) {
        a = gPoogieDigSpots[gPoogieDigSpotIdx];
        n = 4;
        items = &gGatherItemsK0[(u8)(a * 4)];
        chance = &gGatherChanceK0[(u8)(a * 3)];
    } else if (k == 1) {
        a = gPoogieDigSpotIdx;
        n = 6;
        c = a * 5;
        items = &gGatherItemsK1[(u8)(a * 6)];
        gPoogieDigSeason = LobbyWorld_GetSeason();
        if (gPoogieFedSeasonItem == 1) {
            switch (gPoogieDigSeason) {
            case 2:
            default:
                chance = &gGatherChanceAltSeason2[c];
                break;
            case 0:
                chance = &gGatherChanceAltSeason0[c];
                break;
            case 1:
                chance = &gGatherChanceAltSeason1[c];
                break;
            }
        } else {
            switch (gPoogieDigSeason) {
            case 2:
            default:
                chance = &gGatherChanceSeason2[c];
                break;
            case 0:
                chance = &gGatherChanceSeason0[c];
                break;
            case 1:
                chance = &gGatherChanceSeason1[c];
                break;
            }
        }
    } else {
        return 0;
    }
    r = (u8)Rand_Next(1) % 100 + 1;
    for (i = 0; i < n; i++) {
        if (r <= chance[i]) {
            return items[i];
        }
    }
    if (items[n - 1] != 0) {
        return items[n - 1];
    }
    return 0;
}

s32 VillageGather_IsSeasonItem(u16 item)
{
    u8 n;
    u16 *list;
    u8 i;

    switch (gPoogieDigSeason) {
    case 2:
    default:
        n = gSeasonItemCounts[0];
        list = gSeasonItems0;
        break;
    case 0:
        n = D_007E8301[0];
        list = gSeasonItems1;
        break;
    case 1:
        n = D_007E8302;
        list = gSeasonItems2;
        break;
    }
    for (i = 0; i < n; i++) {
        if (item == list[i]) {
            return 1;
        }
    }
    return 0;
}

s32 VillageGather_OverLimit(u8 kind)
{
    u8 i;
    u8 *req;
    u8 n;

    if (kind == 0) {
        n = 7;
        req = gGatherLimitReqA;
    } else if (kind == 1) {
        n = 4;
        req = gGatherLimitReqB;
    } else {
        return 1;
    }
    for (i = 0; i < n; i++) {
        if (gPoogieDigZone[i + 0x17] > req[i]) {
            return 1;
        }
    }
    return 0;
}

s16 VillageGather_RollValue(u8 kind)
{
    s32 *vals;
    u8 *thr;
    u8 r;

    switch (kind) {
    default:
        vals = gGatherValuesA;
        thr = gGatherValueRatesA;
        break;
    case 1:
        vals = gGatherValuesB;
        thr = gGatherValueRatesB;
        break;
    }
    r = (u8)Rand_Next(1) % 100 + 1;
    if (r <= thr[0]) {
        return vals[0];
    }
    if (thr[0] < r && r <= thr[1]) {
        return vals[1];
    }
    if (thr[1] < r && r <= thr[2]) {
        return vals[2];
    }
    return vals[3];
}


void Poogie_NotifyMood(u8 kind)
{
    u8 *save = (u8 *)&gSave;
    u8 mood = Village_AreaKind();
    s16 cur;
    s16 max;

    if (kind == 0) {
        if (mood != 2) {
            max = (&gLobby->poogieFeedMax)[mood];
            cur = (&gLobby->poogieFeed)[mood];
            if (cur >= max) {
                if (mood == 1) {
                    Notice_PushItem(TXT(0x0082CAF0) /* notice: the pet is delighted (the special pet) */);
                } else if ((s8)save[0x2C98] != 0) {
                    Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CB10) /* notice: the pet is delighted ($: its name) */, (char *)save + 0x2C98));
                }
            } else if (cur >= max / 2) {
                Notice_PushItem(TXT(0x0082CB20) /* the pet's mood: quite pleased */);
            } else {
                Notice_PushItem(TXT(0x0082CB40) /* the pet's mood: a little pleased */);
            }
        }
    } else if (kind == 1) {
        switch (gPoogiePetStreak) {
        case 1:
            Notice_PushItem(TXT(0x0082CB60) /* the pet's mood: enjoying the petting */);
            break;
        case 2:
            if ((s8)save[0x2C98] != 0) {
                Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CB10) /* notice: the pet is delighted ($: its name) */, (char *)save + 0x2C98));
            }
            break;
        }
    } else if (kind == 3) {
        if (mood == 1) {
            Notice_PushItem(TXT(0x0082CAF0) /* notice: the pet is delighted (the special pet) */);
        } else if ((s8)save[0x2C98] != 0) {
            Notice_PushItem(Text_FillDollarFmt(TXT(0x0082CB10) /* notice: the pet is delighted ($: its name) */, (char *)save + 0x2C98));
        }
    } else {
        Notice_PushItem(TXT(0x0082CB80) /* notice: the pet is delighted (the sheep outfit) */);
    }
}

s32 Cook_InBestWindow(void)
{
    Player *p = &gPlayers[gScene.localPlayer];
    s16 *range;
    s16 lo;

    switch (Village_AreaKind()) {
    case 1:
        range = gCookRangeKind1;
        break;
    case 2:
    case 0:
    default:
        range = gCookRangeDefault;
        break;
    }
    lo = range[0] + (range[1] - range[2]) / 2 + 1;
    if (p->cookTimer >= lo && p->cookTimer <= lo + 2) {
        return 1;
    }
    return 0;
}

s32 Npc_RollPresence(void)
{
    u8 base;
    u8 add;
    u8 r;

    if (LobbyTime_IsAm() == 1) {
        base = 0;
    } else {
        base = 3;
    }
    switch (LobbyWorld_GetSeason()) {
    case 2:
    default:
        add = 0;
        break;
    case 0:
        add = 1;
        break;
    case 1:
        add = 2;
        break;
    }
    base += add;
    r = (u8)Rand_Next(1) % 100 + 1;
    return gNpcPresenceRates[base] >= r;
}

s16 Cook_GetTime(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    if (pl->motionId[0] == 0x26B) {
        return 0;
    }
    switch ((u8)Village_AreaKind()) {
    case 1:
        return D_007E82A2[0];
    case 2:
    case 0:
    default:
        return D_007E829A[0];
    }
}

u8 Village_AreaKind(void)
{
    u8 r;
    u16 s = gScene.area;

    if (s == 0x83) {
        r = 1;
    } else if (s == 0x57 || s == 0x56) {
        r = 2;
    } else {
        r = 0;
    }
    return r;
}

s16 Poogie_Lookup(s32 id)
{
    PoogieTbl *t;
    u16 v;

    v = gVillageNpcRecsB[0].id;
    t = gVillageNpcRecsB;
    while (v != 0) {
        if (v == (u16)id) {
            return t->value;
        }
        t++;
        v = t->id;
    }
    return 0;
}

void Village_ClearNpcRec(u16 id)
{
    PoogieTbl *t;
    u16 v;
    s16 i;

    v = gVillageNpcRecsB[0].id;
    t = gVillageNpcRecsB;
    i = 0;
    while (v != 0) {
        if (v == id) {
            memset(&gSave.npcRecsB[i], 0, 4);
            return;
        }
        t++;
        v = t->id;
        i++;
    }
}

typedef struct QuestIdEntry {
    u16 id;
    u8 pad[6];
} QuestIdEntry;
extern QuestIdEntry D_007E9416[];
void Offer_CheckPointFlags(u32 before, u32 after, s32 idx)
{
    u16 id = D_007E9416[idx].id;
    PoogieTbl *t;
    u16 v;

    if (before / 24 < after / 24) {
        v = gVillageNpcRecsA[0].id;
        t = gVillageNpcRecsA;
        while (v != 0) {
            if (v == id && Save_TestEventFlag(t->flag) == 0) {
                Save_SetEventFlag(t->flag);
            }
            t++;
            v = t->id;
        }
    }
    if (Cond_Check(0x1A) == 1 && id == 1) {
        return;
    }
    if (before / 32 < after / 32) {
        v = gVillageNpcRecsB[0].id;
        t = gVillageNpcRecsB;
        while (v != 0) {
            if (v == id && Save_TestEventFlag(t->flag) == 0) {
                Save_SetEventFlag(t->flag);
            }
            t++;
            v = t->id;
        }
    }
}

void Village_CheckMilestone(u32 before, u32 after, s32 idx)
{
    Milestone *e = &gVillageMilestones[idx];

    if (before / e->div < after / e->div) {
        if (Village_MilestoneReady(idx) == 1) {
            if (Save_TestEventFlag(e->flag) == 0) {
                Save_SetEventFlag(e->flag);
                if (idx == 0) {
                    Save_ClearEventFlag(0x1CE);
                }
            }
        }
    }
}

s32 Village_MilestoneReady(s32 kind)
{
    switch (kind) {
    case 0:
        if ((u8)Village_FacilityLevel(3) < 2) {
            return 0;
        }
        break;
    case 4:
        if ((u8)Village_FacilityLevel(2) < 2) {
            return 0;
        }
        break;
    case 5:
        if (Cond_Check(0x1A) == 0) {
            return 0;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x49) == 0) {
            return 0;
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x79) == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

s32 Village_FacilityEventPending(void)
{
    switch ((u8)Village_FacilityLevel(4)) {
    case 1:
        if (Save_TestEventFlag(0x13B) == 0) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x13C) == 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Takes the set event flags of three tables into the save records. */
void Village_ProcessEventFlags(void)
{
    PoogieTbl *t;
    u16 v;
    s32 i;
    Milestone *e;

    if (Village_FacilityEventPending() == 0) {
        i = 0;
        v = gVillageNpcRecsB[0].id;
        t = gVillageNpcRecsB;
        while (v != 0) {
            if (Save_TestEventFlag(t->flag) == 1 && gSave.npcRecsB[i].flag == 0) {
                gSave.npcRecsB[i].flag = 1;
                gSave.npcRecsB[i].id = t->id;
                Save_ClearEventFlag(t->flag);
                break;
            }
            t++;
            v = t->id;
            i++;
        }
    }
    i = 0;
    v = gVillageNpcRecsA[0].id;
    t = gVillageNpcRecsA;
    while (v != 0) {
        if (Save_TestEventFlag(t->flag) == 1 && gSave.npcRecsA[i].state == 0) {
            gSave.npcRecsA[i].id = t->id;
            gSave.npcRecCount++;
            switch (gSave.npcRecCount) {
            case 3:
                gSave.npcRecsA[i].state = 2;
                gSave.npcRecsA[i].kind = 0;
                break;
            case 6:
                gSave.npcRecsA[i].state = 2;
                gSave.npcRecsA[i].kind = 1;
                break;
            case 10:
                gSave.npcRecsA[i].state = 2;
                gSave.npcRecsA[i].kind = 2;
                break;
            case 15:
                gSave.npcRecsA[i].state = 2;
                gSave.npcRecsA[i].kind = 3;
                break;
            case 20:
                gSave.npcRecsA[i].state = 2;
                gSave.npcRecsA[i].kind = 4;
                gSave.npcRecCount = 0;
                break;
            default:
                gSave.npcRecsA[i].state = 1;
                gSave.npcRecsA[i].kind = Rand_Next(1) % 5;
                break;
            }
            Save_ClearEventFlag(t->flag);
        }
        t++;
        v = t->id;
        i++;
    }
    e = gVillageMilestones;
    for (i = 0; i < 7; i++) {
        if (Save_TestEventFlag(e->flag) == 1 && gSave.villageTimers[i].state == 0) {
            gSave.villageTimers[i].state = 1;
            switch (i) {
            case 1:
                if (Rand_Next(1) % 100 < 30 && (u8)Village_FacilityLevel(5) > 1) {
                    gSave.villageTimers[i].state = 2;
                }
                break;
            }
            gSave.villageTimers[i].left = e->time;
            Save_ClearEventFlag(e->flag);
        }
        e++;
    }
    if (Save_TestEventFlag(0x49) == 1 && Save_TestEventFlag(0x4A) == 0) {
        gSave.villageTimers[2].state = 1;
        gSave.villageTimers[2].left = 1;
    }
}

void Village_SetFacilityLevel(s32 sub, u8 lvl);
void Village_QuestTick(void)
{
    u8 lvl;
    s32 i;
    s16 t;
    SaveData *sv;

    if (D_5D4924 != 7 && gSave.mine.active == 1) {
        t = gSave.mine.bonus;
        gSave.mine.active = 2;
        if (t > 0) {
            gSave.mine.bonus = t - gSave.mine.digs;
            if (gSave.mine.bonus <= 0) {
                lvl = Village_FacilityLevel(0);
                if (lvl < 9) {
                    Village_SetFacilityLevel(0, lvl + 1);
                    if (lvl + 1 >= 6) {
                        Save_SetEventFlag(0x1C7);
                    }
                }
                gSave.mine.bonus = 0;
            }
        }
    }
    i = 0;
    sv = &gSave;
    for (; i < 7; i++) {
        if (sv->villageTimers[i].left != 0) {
            if (--sv->villageTimers[i].left <= 0) {
                memset(&sv->villageTimers[i], 0, 8);
            }
        }
    }
    sv = &gSave;
    for (i = 0; i < 16; i++) {
        memset(&sv->npcRecsB[i], 0, 4);
    }
}

s16 Village_GetTimerState(s32 idx)
{
    return gSave.villageTimers[idx].state;
}

s32 Village_NpcRecA(u16 id)
{
    PoogieTbl *t;
    u16 v;
    s32 i;

    v = gVillageNpcRecsA[0].id;
    t = gVillageNpcRecsA;
    i = 0;
    while (v != 0) {
        if (id == v) {
            return gSave.npcRecsA[i].state != 0;
        }
        t++;
        v = t->id;
        i++;
    }
    return 0;
}

s32 Village_NpcRecB(u16 id)
{
    PoogieTbl *t = gVillageNpcRecsB;
    s32 i;
    u16 v;

    if (Cond_Check(0x1A) == 1 && id == 1) {
        return 0;
    }
    if (Village_FacilityEventPending() == 1) {
        return 0;
    }
    v = t->id;
    i = 0;
    while (v != 0) {
        if (id == v) {
            return gSave.npcRecsB[i].flag != 0;
        }
        t++;
        v = t->id;
        i++;
    }
    return 0;
}

typedef struct Pos16 {
    s16 x;
    s16 y;
} Pos16;
typedef struct Rec7E9410 {
    s8 kind;
    u8 unk1[5];
    u16 id;
} Rec7E9410;
typedef struct PoogieSpot {
    /* 0x0 */ Pos16 pos;
    /* 0x4 */ s16 idx;              /* in gVillageNpcRecsA */
} PoogieSpot;
extern Rec7E9410 gOfferNpcs[];
extern Pos16 *gOfferNpcPosByKind[];
extern Pos16 gOfferNpcPos[];
extern u8 gMenuInputLock;
extern u16 gLobbyEnterRand;
s32 Offer_IndexOf2(u16 id);
void Menu4E_Open(Em *npc)
{
    u8 *id = &npc->work;
    PoogieSpot *m = (PoogieSpot *)gMenuWork;
    PoogieTbl *t;
    s32 i;
    u16 v;
    s32 k;
    s8 st;

    memset(m, 0, 0xF00);
    i = 0;
    v = gVillageNpcRecsA[0].id;
    t = gVillageNpcRecsA;
    while (v != 0) {
        if (v == *id) {
            st = gSave.npcRecsA[i].state;
            if (st != 0) {
                if (st == 1) {
                    k = Offer_IndexOf2(v);
                    if (k == -1) {
                        return;
                    }
                    m->pos = gOfferNpcPosByKind[gOfferNpcs[k].kind][gSave.npcRecsA[i].kind];
                } else {
                    m->pos = gOfferNpcPos[gSave.npcRecsA[i].kind];
                }
                m->idx = i;
                break;
            } else {
                return;
            }
        }
        t++;
        v = t->id;
        i++;
    }
    gTownMenuOpen = 1;
    gTownMode = 0x4E;
    LobbyMenu_Init();
    gMenuInputLock = 6;
    CamFocus_Start(npc);
    Talk_Say(t->value + gLobbyEnterRand % 3, 0);
}

/* unk_6CB690's .data (0x7E7B70-0x7E84D0) */
extern char D_008261A0[];
extern char D_00826320[];
extern char D_00826490[];
extern char D_00826600[];
extern char D_00826780[];
extern char D_00826910[];
extern char D_00826AA0[];
extern char D_00826C40[];
extern char D_00826DF0[];
extern char D_00826F50[];
extern char D_008270D0[];
extern char D_00827240[];
extern char D_008273C0[];
extern char D_00827560[];
extern char D_008276B0[];
extern char D_00827810[];
extern char D_00827990[];
extern char D_00827B60[];
extern char D_00827CF0[];
extern char D_00827E80[];
extern char D_00828000[];
extern char D_00828170[];
extern char D_008282E0[];
extern char D_00828470[];
extern char D_008285F0[];
extern char D_00828770[];
extern char D_00828900[];
extern char D_00828A30[];
extern char D_00828BD0[];
extern char D_00828D70[];
extern char D_00828F00[];
extern char D_00829080[];
extern char D_00829210[];
extern char D_008293B0[];
extern char D_00829540[];
extern char D_008296E0[];
extern char D_00829880[];
extern char D_00829A20[];
extern char D_00829BE0[];
extern char D_00829D70[];
extern char D_00829F10[];
extern char D_0082A080[];
extern char D_0082A200[];
extern char D_0082A3A0[];
extern char D_0082A540[];
extern char D_0082A6E0[];
extern char D_0082A890[];
extern char D_0082AA10[];
extern char D_0082AB70[];
extern char D_0082ACD0[];
extern char D_0082AE50[];
extern char D_0082AFD0[];
extern char D_0082B170[];
extern char D_0082B2F0[];
extern char D_0082B480[];
extern char D_0082B5C0[];
extern char D_0082B760[];
extern char D_0082B900[];
extern char D_0082BA90[];
extern char D_0082BBC0[];
extern char D_0082BCF0[];
extern char D_0082BE40[];
extern char D_0082BF90[];
extern char D_0082C0C0[];
extern char D_0082C1E0[];
extern char D_0082C340[];
extern char D_0082C480[];
extern char D_0082C5B0[];
extern char D_0082C840[];
extern char D_0082C850[];
extern char D_0082C860[];
char *gDojoRulesTextsOn[32] = {
    D_008261A0, D_00826320, D_00826490, D_00826600,
    D_00826780, D_00826910, D_00826AA0, D_00826C40,
    D_00826DF0, D_00826F50, D_008270D0, D_00827240,
    D_008273C0, D_00827560, D_008276B0, D_00827810,
    D_00827990, D_00827B60, D_00827CF0, D_00827E80,
    D_00828000, D_00828170, D_008282E0, D_00828470,
    D_008285F0, D_00828770, D_00828900, D_00828A30,
    D_00828BD0, D_00828D70, NULL, NULL,
};
char *gDojoRulesTextsOff[28] = {
    D_00828F00, D_00829080, D_00829210, D_008293B0,
    D_00829540, D_008296E0, D_00829880, D_00829A20,
    D_00829BE0, D_00829D70, D_00829F10, D_0082A080,
    D_0082A200, D_0082A3A0, D_0082A540, D_0082A6E0,
    D_0082A890, D_0082AA10, D_0082AB70, D_0082ACD0,
    D_0082AE50, D_0082AFD0, D_0082B170, D_0082B2F0,
    D_0082B480, D_0082B5C0, D_0082B760, D_0082B900,
};
char *gDojoBasicsPages[12] = {
    D_0082BA90, D_0082BBC0, D_0082BCF0, D_0082BE40,
    D_0082BF90, D_0082C0C0, D_0082C1E0, D_0082C340,
    D_0082C480, D_0082C5B0, NULL, NULL,
};
u32 gPointShopStock0_0[16] = {
#include "lobby/007E7C90.inc"
};
u32 gPointShopStock0_1[20] = {
#include "lobby/007E7CD0.inc"
};
u32 gPointShopStock1_0[24] = {
#include "lobby/007E7D20.inc"
};
u32 gPointShopStock1_1[16] = {
#include "lobby/007E7D80.inc"
};
u32 gPointShopStock2_0[20] = {
#include "lobby/007E7DC0.inc"
};
u32 gPointShopStock2_1[24] = {
#include "lobby/007E7E10.inc"
};
PointLine *gPointShopStock0[2] = {
    (PointLine *)gPointShopStock0_0, (PointLine *)gPointShopStock0_1,
};
PointLine *gPointShopStock1[2] = {
    (PointLine *)gPointShopStock1_0, (PointLine *)gPointShopStock1_1,
};
PointLine *gPointShopStock2[2] = {
    (PointLine *)gPointShopStock2_0, (PointLine *)gPointShopStock2_1,
};
u16 gDojoQuestsCat1[6] = {
#include "lobby/007E7E88.inc"
};
u16 gDojoQuestsCat2[8] = {
#include "lobby/007E7EA0.inc"
};
u16 gDojoQuestsCat2Cond[12] = {
#include "lobby/007E7EB0.inc"
};
u16 gDojoQuestsCat3[4] = {
#include "lobby/007E7EC8.inc"
};
s16 gDojoRulesPagesOff[16] = {
#include "lobby/007E7ED0.inc"
};
s16 gDojoRulesPagesOn[16] = {
#include "lobby/007E7EF0.inc"
};
f32 gGatherZoneBounds[2] = {
#include "lobby/007E7F10.inc"
};
u32 D_007E7F18[2] = {
#include "lobby/007E7F18.inc"
};
u32 D_007E7F20[24] = {
#include "lobby/007E7F20.inc"
};
ListPanel gOutfitListBox = {
#include "lobby/007E7F80.inc"
};
char *gPoogieMenuLabels[4] = {
    D_0082C840, D_0082C850, D_0082C860, NULL,
};
char *gPoogieCareLabels[4] = {
    D_0082C840, D_0082C850, NULL, NULL,
};
char gOutfitNames[1][32] = {
#include "lobby/007E7FC0.inc"
};
u32 D_007E7FE0[32] = {
#include "lobby/007E7FE0.inc"
};
u8 gPoogieDigSpotCounts[8] = {
#include "lobby/007E8060.inc"
};
u8 gPoogieDigSpotsZone1[8] = {
#include "lobby/007E8068.inc"
};
u8 gPoogieDigSpotsZone2[8] = {
#include "lobby/007E8070.inc"
};
u8 gPoogieDigSpotsZone3[8] = {
#include "lobby/007E8078.inc"
};
u8 gPoogieDigSpotsZone4[8] = {
#include "lobby/007E8080.inc"
};
u8 gPoogieDigSpotsZone5[8] = {
#include "lobby/007E8088.inc"
};
u8 gPoogieDigSpotsZone6[8] = {
#include "lobby/007E8090.inc"
};
u8 gGatherLimitReqA[8] = {
#include "lobby/007E8098.inc"
};
u8 gGatherLimitReqB[16] = {
#include "lobby/007E80A0.inc"
};
u16 gGatherItemsK0[32] = {
#include "lobby/007E80B0.inc"
};
u8 gGatherChanceK0[32] = {
#include "lobby/007E80F0.inc"
};
u16 gGatherItemsK1[24] = {
#include "lobby/007E8110.inc"
};
u8 gGatherChanceSeason2[32] = {
#include "lobby/007E8140.inc"
};
u8 gGatherChanceSeason0[32] = {
#include "lobby/007E8160.inc"
};
u8 gGatherChanceSeason1[32] = {
#include "lobby/007E8180.inc"
};
u8 gGatherChanceAltSeason2[32] = {
#include "lobby/007E81A0.inc"
};
u8 gGatherChanceAltSeason0[32] = {
#include "lobby/007E81C0.inc"
};
u8 gGatherChanceAltSeason1[160] = {
#include "lobby/007E81E0.inc"
};
u8 gPoogieDigSpotsSeason2[8] = {
#include "lobby/007E8280.inc"
};
u8 gPoogieDigSpotsSeason0[8] = {
#include "lobby/007E8288.inc"
};
u8 gPoogieDigSpotsSeason1[8] = {
#include "lobby/007E8290.inc"
};
s16 gCookRangeDefault[4] = {
#include "lobby/007E8298.inc"
};
s16 gCookRangeKind1[4] = {
#include "lobby/007E82A0.inc"
};
u8 gNpcPresenceRates[8] = {
#include "lobby/007E82A8.inc"
};
u16 gSeasonItems0[16] = {
#include "lobby/007E82B0.inc"
};
u16 gSeasonItems1[8] = {
#include "lobby/007E82D0.inc"
};
u16 gSeasonItems2[16] = {
#include "lobby/007E82E0.inc"
};
u8 gSeasonItemCounts[16] = {
#include "lobby/007E8300.inc"
};
s32 gGatherValuesA[4] = {
#include "lobby/007E8310.inc"
};
u8 gGatherValueRatesA[16] = {
#include "lobby/007E8320.inc"
};
s32 gGatherValuesB[4] = {
#include "lobby/007E8330.inc"
};
u8 gGatherValueRatesB[16] = {
#include "lobby/007E8340.inc"
};
Milestone gVillageMilestones[8] = {
#include "lobby/007E8350.inc"
};
PoogieTbl gVillageNpcRecsA[16] = {
#include "lobby/007E8390.inc"
};
PoogieTbl gVillageNpcRecsB[13] = {
#include "lobby/007E83F0.inc"
};
Pos16 gOfferNpcPosByKind_0[8] = {
#include "lobby/007E8440.inc"
};
Pos16 gOfferNpcPosByKind_1[8] = {
#include "lobby/007E8460.inc"
};
Pos16 gOfferNpcPosByKind_2[6] = {
#include "lobby/007E8480.inc"
};
Pos16 *gOfferNpcPosByKind[3] = {
    gOfferNpcPosByKind_0, gOfferNpcPosByKind_1, gOfferNpcPosByKind_2,
};
Pos16 gOfferNpcPos[8] = {
#include "lobby/007E84B0.inc"
};
