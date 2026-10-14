/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "scene.h"
#include "quest.h"

void CamFocus_SetAside(s8 arg0);
void Talk_Say(s32, s8);
s64 abs(s32);
extern u16 gLobbyEnterRand;

typedef struct {
    u8 unk0[0x4A];
    s16 items[5];   /* 0x4A */
    u8 unk54[0x6E - 0x54];
    s16 cursor;     /* 0x6E */
    s16 list;       /* 0x70 */
    s16 listCount;  /* 0x72 */
    u8 unk74[2];
    s8 unk76;       /* 0x76 */
    u8 unk77;
    u16 lists[12][12]; /* 0x78 */
} VMenu;
extern u8 gMenuWork[];
extern s16 gQuestMapGroups[];
extern u16 gQuestLists[][60];
extern u16 D_5BFA68[12];
extern u16 gQuestDeskShown[12];
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, void *src, u32 n);
QuestInfo *Quest_GetDiscInfo(u16 id);
u16 Area_MapGroup(u16 id);
s32 QuestDesk_CountRanks(void);
s32 Game_IsOnline(void);
s32 Cond_Check(u16 id);
s32 Save_TestEventFlag(s32 flag);

void QuestDesk_SayBye(void) {
    Talk_Say(1, 0);
    CamFocus_SetAside(0);
}

void QuestDesk_BuildLists(void)
{
    VMenu *m = (VMenu *)gMenuWork;
    s32 n;
    s32 i;
    u16 *p;
    s32 kind;

    memset(m->lists, 0, 0x120);
    switch (m->items[m->cursor]) {
    case 4:
        for (i = 1; i < 7; i++) {
            n = 0;
            for (p = gQuestLists[m->unk76]; *p != 0; p++) {
                if (Quest_GetDiscInfo(*p)->type == i) {
                    m->lists[i - 1][n++] = *p;
                    if (n >= 12) {
                        break;
                    }
                }
            }
        }
        m->listCount = 5;
        break;
    case 3:
        for (i = 0; i < 7; i++) {
            kind = gQuestMapGroups[i];
            n = 0;
            for (p = gQuestLists[m->unk76]; *p != 0; p++) {
                if (kind == Area_MapGroup(Quest_GetDiscInfo(*p)->map)) {
                    m->lists[i][n++] = *p;
                    if (n >= 12) {
                        break;
                    }
                }
            }
        }
        m->listCount = QuestDesk_CountRanks();
        break;
    case 2:
        m->list = 0;
        switch (gScene.area) {
        case 0x88:
            memcpy(m->lists[m->list], D_5BFA68, 0x18);
            break;
        default:
            memcpy(m->lists[m->list], gQuestDeskShown, 0x18);
            break;
        }
        break;
    case 0:
        break;
    }
}

s32 QuestDesk_CountRanks(void)
{
    if (Game_IsOnline() == 0) {
        if (Cond_Check(0x4F) == 1) {
            return gScene.area == 0x57 ? 7 : 6;
        }
        if (Cond_Check(0x1A) == 1) {
            return 6;
        }
        if (Cond_Check(0x15) == 1) {
            return 5;
        }
        if (Cond_Check(0x10) == 1) {
            return 4;
        }
        if (Cond_Check(0xA) == 1) {
            return 3;
        }
        return Cond_Check(7) == 1 ? 2 : 1;
    } else {
        if (Save_TestEventFlag(0x122) == 1) {
            return 7;
        }
        if (Cond_Check(0x60) == 1) {
            return 6;
        }
        return 5;
    }
}

s32 Lobby_EnterRollRare(void) {
    return ((s64) (abs((s32) gLobbyEnterRand % 100) << 0x30) >> 0x30) < 0xB;
}
