/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "scene.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "em.h"
#include "lobby_menu.h"

s32 Save_TestEventFlag(s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
void CamReq_Cancel(void);
void StageModels_Free(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void CamFocus_SetAside(s8 arg0);
void HairMenu_LoadPage(s16 page);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
/* The HairMenuWork views below stay casts: with &gMenuWork.hair MWCC addresses the fields from
 * the symbol (lui and offset at each access) where the game keeps the base in a register
 * (HairMenu_MovePlayer, HairMenu_Update). */
extern MenuWork gMenuWork;
void Npc_SetRouteToHairChair(Em *arg0);
void Actor_SetState(void *, s32, s32);

s32 HairMenu_IsOpen(void) {
    return gTownMode == 0x3C;
}

void *memset(void *d, s32 c, u32 n);
void LobbyMenu_Init(void);
void HairMenu_Draw(u8 *);
void HairMenu_ResetColor(void);
void Talk_Say(s32 msg, s8 a);
extern u16 gTalkLine;
extern char *gTalkLineText;
extern s8 gTownJingleOn;
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *arg0, s32 arg1);
void Jingle_Play(void *obj, s32 id);
s32 func_367690(void);
void HairMenu_Open(Em *npc, s32 b)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;

    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(HairMenu_Draw, 0, 0, 0);
    gTownMode = 0x3C;
    gTownMenuOpen = 1;
    Talk_Say(0, 1);
    if (b != 0) {
        gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), 500);
    }
    m->withNpc = b;
    m->npc = npc;
    if (m->withNpc == 1 && func_367690() == 0) {
        Jingle_Play(pl, 0xB1);
        gTownJingleOn = 1;
    }
    HairMenu_ResetColor();
    m->toggle = 1;
    gFieldControl = 0;
}

void HairMenu_Close(void)
{
    Player *p;

    gTownMenuOpen = 0;
    gTownMode = 0;
    p = &gPlayers[gScene.localPlayer];
    LobbyMenu_SetDraw(0, 0, 0, 0);
    p->target = NULL;
    StageModels_Free();
    CamFocus_End();
    CamReq_Cancel();
    gFieldControl = 1;
}

void HairMenu_MovePlayer(void)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];

    m->timer = 0x14;
    Npc_SetRouteToHairChair((Em *)((u8 *) m->npc));
    p->moveTo[0] = 2480.0f;
    p->moveTo[1] = 0.0f;
    p->moveTo[2] = 2876.0f;
    Actor_SetState(p, 0, 0x9C);
}

s32 HairMenu_IsIdle(void) {
    if ((gMenuWork.hair.step > 0) && (gMenuWork.hair.step != 8)) {
        return 0;
    }
    return 1;
}

u16 LobbyPad_Get(s32 a);
s32 Talk_Update(void);
void HairMenu_ShowList(void);
s32 HairMenu_ListInput(u16 pad);
void CamReq_HairMenu(void);
s32 Motion_ReachedFrame(void *, s32, f32);
void HairMenu_Apply(void);
s32 Talk_RunText(void);

void HairMenu_Update(void)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);

    switch (m->step) {
    case 0:
        switch (Talk_Update()) {
        case 0:
            if (*(u16 *)((u8 *)&gSave + 0x20AA) != 0) {
                m->step = 8;
                if (m->withNpc == 1) {
                    Actor_SetState(m->npc, 0, 0x97);
                }
                Talk_Say(1, 0);
            } else {
                if (m->withNpc == 1) {
                    if (gSave.money < 500) {
                        m->step = 8;
                        Talk_Say(5, 0);
                        break;
                    }
                    m->step = 2;
                } else {
                    (m->step)++;
                    HairMenu_ShowList();
                }
                HairMenu_MovePlayer();
            }
            CamFocus_End();
            break;
        case 3:
            m->step = 8;
            Talk_Say(4, 0);
            if (m->withNpc == 1 && func_367690() == 0) {
                Jingle_Play(p, 0xB1);
                gTownJingleOn = 1;
            }
            break;
        }
        break;
    case 1:
        switch (HairMenu_ListInput(pad)) {
        case 0:
            (m->step)++;
            break;
        case 3:
            m->step = 8;
            Talk_Say(4, 0);
            break;
        }
        break;
    case 2:
        if (m->timer > 0) {
            (m->timer)--;
        }
        if (m->npc->state == 0x97 && m->timer == 0 && p->state == 1) {
            Actor_SetState(m->npc, 0, 0x99);
            CamReq_HairMenu();
            (m->step)++;
        }
        break;
    case 3:
        if (m->npc->motionId[0] == 0x3E9) {
            (m->step)++;
            Actor_SetState(p, 0, 0x9A);
        }
        break;
    case 4:
        if (p->motionId[0] == 0x261) {
            (m->step)++;
            Actor_SetState(m->npc, 0, 0x9B);
        }
        break;
    case 5:
        if (m->npc->motionId[0] == 0x43D && m->unk1C6 == 0 &&
            Motion_ReachedFrame(m->npc, 0, 118.0f)) {
            HairMenu_Apply();
            m->unk1C6 = 1;
            Talk_Say(3, 0);
            (m->step)++;
        }
        break;
    case 6:
        if (m->npc->motionId[0] != 0x43D) {
            Talk_RunText();
        }
        if (m->npc->state == 0x98) {
            (m->step)++;
            Actor_SetState(p, 0, 0x15);
        }
        break;
    case 7:
        HairMenu_Close();
        break;
    case 8:
        switch (Talk_Update()) {
        case 0:
        case 3:
            HairMenu_Close();
            break;
        }
        break;
    }
}

s32 HairMenu_IsUnlocked(s32 id)
{
    s32 style;

    style = (s16)(id);
    if ((style == 0x13) && (Save_TestEventFlag(0x7C) == 0)) {
        return 0;
    }
    if (style == gSave.hairStyle) {
        return 0;
    }
    return 1;
}


typedef struct Rec7EB8C0 {
    /* 0x00 */ s16 id;              /* -1 ends the table */
    /* 0x02 */ u8 unk2[0x42];
} Rec7EB8C0;
extern Rec7EB8C0 gHairStyles[];
void *memcpy(void *d, void *s, u32 n);
s32 Menu_CountPages(s16 n, s16 per);
/* n and count are declared (and zeroed) before first and last: the other
 * order gave page * 6 other temporaries. */
void HairMenu_LoadPage(s16 page)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    Rec7EB8C0 *r = gHairStyles;
    s16 n = 0;
    s16 count = 0;
    s16 first = page * 6;
    s16 last = first + 5;
    s32 id = gHairStyles[0].id;
    u8 *dst;

    if (id != -1) {
        dst = (u8 *)m;
        do {
            if (HairMenu_IsUnlocked(id) == 1) {
                if (n >= first && n <= last) {
                    memcpy(dst + 0x14, r, 0x44);
                    dst += 0x44;
                    count++;
                }
                n++;
            }
            r++;
            id = r->id;
        } while (id != -1);
    }
    m->count = count;
    m->page = page;
    m->pages = Menu_CountPages(n, 6);
    if (m->sel >= count) {
        m->sel = count - 1;
    }
}

void HairMenu_ShowList(void) {
    WinSlide_Step(&gMenuWork.hair.win, -0x140, 0xA);
    CamFocus_SetAside(1);
    HairMenu_LoadPage(0);
}

/* unk_6F2610's .data (0x7EB8C0-0x7EBE10) */
Rec7EB8C0 gHairStyles[20] = {
#include "lobby/007EB8C0.inc"
};
