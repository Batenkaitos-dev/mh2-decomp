/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "save.h"
#include "em.h"
#include "lobby_globals.h"

s32 Cond_Check(u16);
s32 Game_IsOnline(void);
void *memset(void *dst, s32 c, u32 n);
void Npc_RefreshOffers(void);
void QuestList_Build(s32);
extern s32 gTownMode;
extern s8 gQuestListRebuildPending;
extern M2C_UNK gQuestLists;

s32 ElderForecast_IsUnlocked(void) {
    if (Game_IsOnline() == 1) {
        if (Cond_Check(0x61) == 0) {
            return 0;
        }
        goto block_6;
    }
    if (Cond_Check(0x15) == 0) {
        return 0;
    }
block_6:
    return 1;
}

s32 LobbyWorld_GetSeason(void);
u16 Rand_Next(s32 stream);

s32 ElderForecast_QuestAppears(u16 id, s32 kind)
{
    TownQuestLists *t = &gTownQuestLists;
    s32 row;
    s32 i;
    s32 k;

    if (ElderForecast_IsUnlocked() == 0) {
        return 0;
    }
    if (id == 0x2F2E && gSave.hunted[11] == 0) {
        return 0;
    }
    if ((u32)kind <= 1 || kind == 3) {
        row = 0;
    } else {
        row = 1;
    }
    k = LobbyWorld_GetSeason();
    for (i = 0; i < 5; i++) {
        if (t->quests[row][k][i] == id && Rand_Next(1) % 100 < 40) {
            return 1;
        }
    }
    return 0;
}

extern s8 gOffersRefreshPending;
struct OfferTask;
s16 OfferMenu_BuildList(struct OfferTask *e, s32 b);
s32 MyHouse_IsOwner(void);
s32 Village_MealPointsReady(void);

void Npc_RefreshOffers(void)
{
    u8 *w;
    s32 i;
    Em *e;

    if (gTownMode == 0 || gTownMode == 4) {
        for (i = 0, e = gEms; i < 20; i++, e++) {
            if (e->active != 0) {
                w = &e->work;
                switch (OfferMenu_BuildList((struct OfferTask *)e, 1)) {
                default:
                    w[4] = 1;
                    break;
                case 0:
                    w[4] = 0;
                    break;
                case 4:
                    w[4] = 2;
                    break;
                }
                if (w[0] == 0x71 && MyHouse_IsOwner() != 0 && Village_MealPointsReady() == 1) {
                    w[4] = 2;
                }
            }
        }
    } else {
        gOffersRefreshPending = 1;
    }
}

void QuestList_Rebuild(void) {
    if (gTownMode == 0x35) {
        gQuestListRebuildPending = 1;
        return;
    }
    memset(&gQuestLists, 0, 0xF0);
    if (Game_IsOnline() == 0) {
        if (gSave.unk36E8 != 0) {
            QuestList_Build(3);
        } else {
            QuestList_Build(0);
        }
    } else {
        QuestList_Build(1);
        QuestList_Build(2);
    }
    Npc_RefreshOffers();
}
