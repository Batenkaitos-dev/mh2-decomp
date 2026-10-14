/*
 * A village menu of up to five entries (work area gMenuWork, menu 0x35):
 * QuestDesk_Open opens it, QuestDesk_HasItem says whether entry k is offered.
 */
#include "common.h"
#include "item.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "lobby_menu.h"

extern u8 gMenuWork[];
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void *memset(void *dst, s32 c, u32 n);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void QuestDesk_Draw(u8 *task);
void QuestDesk_Draw2(u8 *task);
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
s32 Equip_GunHasNoAmmo(Player *pl, SaveData *save);
s32 Menu_PushNoticeOnce(s32);
s32 Game_IsOnline(void);
s32 Cond_Check(u16 id);
void ElderForecast_Load(u16 *out, s8 b); /* unit's own declaration: with the definition's void (u16 *out, s32 k) (lobby/unk_6FE760.c), QuestDesk_HasItem 1 of 84 words differ */
s32 QuestDesk_HasItem(s16 k);

void QuestDesk_Open(u8 *task)
{
    QuestDesk *m;
    u8 *p;
    s32 i;

    gTownMenuOpen = 1;
    m = (QuestDesk *)gMenuWork;
    p = task + 0x438;
    gTownMode = 0x35;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(QuestDesk_Draw, QuestDesk_Draw2, 0, 0);
    m->season = LobbyWorld_GetSeason();
    m->pm = LobbyTime_IsPm();
    if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], &gSave) == 1) {
        Menu_PushNoticeOnce(0x13);
    }
    switch (*p) {
    case 'O':
        m->unk76 = 1;
        break;
    default:
        m->unk76 = 0;
        break;
    }
    m->count = 0;
    for (i = 0; i < 5; i++) {
        if (QuestDesk_HasItem(i) == 1) {
            m->kinds[m->count] = i;
            if (i == 2) {
                m->kindSel = m->count;
            }
            m->count++;
        }
    }
}

/* whether entry k of the menu is offered */
s32 QuestDesk_HasItem(s16 k)
{
    QuestDesk *m = (QuestDesk *)gMenuWork;

    switch (k) {
    case 0:
        return 1;
    case 1:
        if (Game_IsOnline() == 0 && Cond_Check(0xD) == 0) {
            return 0;
        }
        if (gElderForecastHeard != 0) {
            ElderForecast_Load(m->forecast, m->season);
            return 1;
        }
        break;
    case 2:
        return 1;
    case 3:
        if (Game_IsOnline() == 1) {
            return 1;
        }
        if (Cond_Check(7) == 1) {
            return 1;
        }
        break;
    case 4:
        if (Game_IsOnline() == 1) {
            return 1;
        }
        if (Cond_Check(0xA) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}
