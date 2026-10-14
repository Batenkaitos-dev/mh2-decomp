/*
 * Small requests to the state machine at CamReq_Set (it stores a state
 * byte in gCamCtl.request and an argument in gCamCtl.requestArg): one function per
 * state, some chosen from an object's type. Semantics not yet known.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "net_menu.h"

extern u8 D_0041CEB0[];
extern u8 gCamRequests[0xBC];
extern u8 gSysBusy;

void CamReq_QuestTarget(s32 a);
void CamReq_Set(s32 state, void *arg);
void CamReq_Cancel(void);
s32 CamCtl_IsRailMode(void);
u16 Area_DayId(u16 a);
void *memset(void *p, s32 c, u32 n);

void CamReq_TargetDown(void)
{
    CamReq_QuestTarget(0);
}

void CamReq_BossHunted(void)
{
    CamReq_QuestTarget(1);
}

void CamReq_ForMonster(Em *em)
{
    s32 state;

    switch (em->type) {
    case 7:
    case 0x32:
        state = 0x7C;
        break;
    case 2:
    case 0x24:
    case 0x47:
        if (em->stance != 2) {
            state = 0x83;
        } else {
            state = 0x89;
        }
        break;
    case 0x37:
        state = 0x7E;
        break;
    case 0x36:
    case 0x3C:
    case 0x3B:
        state = 0x7F;
        break;
    case 0x40:
    case 0x41:
        state = 0x80;
        break;
    default:
        return;
    }
    CamReq_Set(state, em);
}

void CamReq_Mode0A(void)
{
    if (CamCtl_IsRailMode() != 1) {
        CamReq_Set(0xA, NULL);
    }
}

void CamReq_Mode01(void)
{
    if (gScene.map != 0xC && gScene.map != 0x16) {
        CamReq_Set(1, NULL);
    }
}

void CamReq_AreaIntro(void)
{
    s32 state;

    switch (Area_DayId(gScene.area)) {
    case 0xC:
        state = 2;
        break;
    case 0x19:
        state = 3;
        break;
    case 0xB0:
        state = 4;
        break;
    default:
        return;
    }
    CamReq_Set(state, NULL);
}

void CamReq_Mode05(void *arg)
{
    CamReq_Set(5, arg);
}

void CamReq_LobbyEvent(void)
{
    CamReq_Set(0x54, NULL);
}

void CamReq_TownPc(void)
{
    CamReq_Set(0x55, NULL);
}

void CamReq_Contest(s32 a, s32 b)
{
    if (b == 0) {
        a += 0xE;
    } else {
        a += 0xB;
    }
    CamReq_Cancel();
    CamReq_Set(a, NULL);
}

void CamReq_Minigame(s32 a)
{
    CamReq_Cancel();
    CamReq_Set(a + 0x17, NULL);
}

void CamReq_Workshop(s32 a)
{
    CamReq_Set(a + 0x11, NULL);
}

void CamReq_HairMenu(void)
{
    CamReq_Set(0x16, NULL);
}

void CamReq_Lobby1B(s32 a)
{
    CamReq_Set(a + 0x1B, NULL);
}

void CamReq_VillageBoard(void)
{
    CamReq_Set(0x20, NULL);
}

void CamReq_Kitchen(s32 a)
{
    CamReq_Set(a + 0x21, NULL);
}

void CamReq_Mode25(void)
{
    CamReq_Cancel();
    CamReq_Set(0x25, NULL);
}

void CamReq_FromTable(u16 a, u16 b)
{
    CamReq_Cancel();
    CamReq_Set(D_0041CEB0[a] + b, NULL);
}

void CamReq_Player(void *arg, u8 b)
{
    CamReq_Cancel();
    CamReq_Set(b + 0x62, arg);
}

void CamReq_Lobby44(s32 a)
{
    CamReq_Set(a + 0x44, NULL);
}

void Cam_ResetRequests_Unk(void)
{
    memset(&gCardMenu, 0, sizeof(gCardMenu));
    memset(gCamRequests, 0, sizeof(gCamRequests));
    gSysBusy = 0;
}

s32 func_003240D0(void)
{
    return 1;
}

/* unk_323C90's .data (0x41CEB0-0x41CF00) */
u8 D_0041CEB0[16] = {
#include "sub_main/0041CEB0.inc"
};
u32 gEffectColors[8] = {
#include "sub_main/0041CEC0.inc"
};
u32 gFishColors[8] = {
#include "sub_main/0041CEE0.inc"
};
