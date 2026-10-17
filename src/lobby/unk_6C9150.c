/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"

#include "lobby_menu.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "player.h"
/* The menu opened by OfficialDesk_Open (work area gMenuWork, id 0x45). */
void *memset(void *dst, s32 c, u32 n);

extern u8 gSkillPreview[];
extern u16 *gHrGateQuests[];
extern s16 gGalleryCourseFees[];
extern u8 gMenuWork[];
void Save_AddMoney(s32 n);
void LobbySe_Play(s32 se);
void Equip_Refresh(SaveData *save);
void Player_CalcSkills(struct Player *pl);
void SkillList_Build(struct Player *p, struct Player *q);
u8 SkillList_PageCount(void);
u8 Village_FacilityLevel(s32 k);
s32 Save_TestEventFlag(s32 flag);
s32 Save_IsQuestCleared(u16 id);
u8 SavePouch_Add(u16 item, s16 n);
void KitchenGuest_Close(void);
struct Em *Npc_Find(u8 kind);
struct NpcRouteView;
void Npc_AdvanceRoute(struct Em *npc);
extern s32 gTownMode;
extern LobbyMenu *gLobbyMenu;
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
void CamFocus_SetAside(s8 arg0);
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;

void KitchenGuest_OnDone(void) {
    struct Em *npc;

    npc = Npc_Find(0x71);
    if (npc != 0) {
        Npc_AdvanceRoute(npc);
    }
    if ((gTownMode == 0x49) && ((s32) gLobbyMenu->subStep < 3)) {
        KitchenGuest_Close();
    }
}

void OfficialDesk_NextTest(u8 *m, s32 idx);
void OfficialDesk_ListElderRaid(u8 *m);
s32 OfficialDesk_TestSet(u8 *npc);
void OfficialDesk_ListEvents(SlotMenu *m);
void OfficialDesk_Draw(void *);
void OfficialDesk_Draw2(void *);
s32 ElderRaid_InState1(void);
s32 ElderRaid_InState2(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
s32 Equip_GunHasNoAmmo(struct Player *pl, struct SaveData *save);
s32 Menu_PushNoticeOnce(s32);

void OfficialDesk_Open(u8 *npc)
{
    SlotMenu *m = (SlotMenu *)gMenuWork;
    s32 k;

    gTownMenuOpen = 1;
    gTownMode = 0x45;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(OfficialDesk_Draw, OfficialDesk_Draw2, 0, 0);
    WinSlide_Step(&m->win, 0x280, 0x118);
    CamFocus_SetAside(1);
    m->npc = npc;
    if (gLobby->dlQuestsReady != 0) {
        m->row[0] = 0;
        OfficialDesk_ListEvents(m);
    } else {
        m->row[0] = 3;
    }
    if (ElderRaid_InState1() == 1 || ElderRaid_InState2() == 1) {
        m->row[2] = 2;
        OfficialDesk_ListElderRaid((u8 *)m);
    } else {
        m->row[2] = 3;
    }
    k = OfficialDesk_TestSet(npc);
    if (k != -1) {
        m->row[1] = 1;
        OfficialDesk_NextTest((u8 *)m, k);
    } else {
        m->row[1] = 3;
    }
    if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave) == 1) {
        Menu_PushNoticeOnce(0x13);
    }
    LobbySe_Play(0x16);
}

extern u8 gQuestFiles[];
void *Quest_DownloadedInfo(u8 slot)
{
    u8 *p = gQuestFiles + slot * 0x300;
    return (u8 *)*(s32 *)p + (s32)p;
}

void OfficialDesk_ListEvents(SlotMenu *m)
{
    s16 i;
    u8 *p;

    memset(m->list[0], 0, sizeof(m->list[0]));
    for (i = 0; i < gLobby->dlQuestCount; i++) {
        p = Quest_DownloadedInfo(i);
        if (p != NULL) {
            m->list[0][i] = *(u16 *)(p + 0x2A);
        }
    }
}

void OfficialDesk_NextTest(u8 *m, s32 idx)
{
    u16 *p = gHrGateQuests[idx];

    while (*p != 0) {
        if (Save_IsQuestCleared(*p) == 0) {
            *(u16 *)(m + 0x54) = *p;
            return;
        }
        p++;
    }
}

s32 Cond_IsLatest(u16 id);
s32 OfficialDesk_TestSet(u8 *npc)
{
    switch (npc[0x438]) {
    case 0x40:
        if (Cond_IsLatest(0x5A) != 0) {
            return 0;
        }
        if (Cond_IsLatest(0x5D) != 0) {
            return 1;
        }
        if (Cond_IsLatest(0x62) != 0) {
            return 2;
        }
        break;
    case 0x4E:
        if (Cond_IsLatest(0x65) != 0) {
            return 3;
        }
        if (Cond_IsLatest(0x68) != 0 || Cond_IsLatest(0x69) != 0) {
            return 4;
        }
        break;
    }
    return -1;
}

extern u8 gElderRaidEvent;
void OfficialDesk_ListElderRaid(u8 *m)
{
    u8 k = (*(u8 **)(m + 0x38))[0x438];
    s16 *w = (s16 *)&gElderRaidEvent;

    switch (k) {
    case 0x40:
        *(s16 *)(m + 0x66) = w[0x11];
        break;
    case 0x4E:
        *(s16 *)(m + 0x66) = w[0x12];
        *(s16 *)(m + 0x68) = w[0x13];
        break;
    }
}

void OfficialDesk_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_SetAside(0);
    CamFocus_End();
    gMenuInputLock = 0x14;
    LobbySe_Play(4);
    gFieldControl = 1;
}