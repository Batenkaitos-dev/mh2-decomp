#include "common.h"
#include "scene.h"
#include "lobby_globals.h"
#include "save.h"
#include "area.h"

s32 Cond_Check(u16 id);

extern s16 D_007EC5F0[];

typedef struct MonsterBit {
    u16 em;
    u16 bit;
} MonsterBit;
typedef struct PickEntry {
    s16 unk0;
    s16 unk2;
    u8 unk4[4];
} PickEntry;

u16 Rand_Next(s32 stream);
extern MonsterBit gMonsterFlagBits[];
u16 Monster_FlagBit(u16 em);
s32 Rate_RandomGroup(void);
/* unit's own declaration: an s32 id changes Rate_UpdatePrices by 70 words */
s32 Rate_Get(s16 id);
typedef struct MonsterPick {
    s16 a;
    s16 id;
    s16 b;
    s16 c;
} MonsterPick;
u16 Records_HuntedTotal(struct SaveData *save, u8 group, u8 captured);
s32 Monster_IsHunted(u16 em);
void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
s16 Rotation_RandomDays(s32 kind);
extern s16 *gRotationHuntLists[];
extern s16 *D_007EC73C;
extern s16 *gRotationHuntList3;
extern PickEntry *gRotationPickListsC10[];
extern PickEntry *D_007EC854;
extern PickEntry *gRotationPickList3C10;
extern PickEntry *gRotationPickLists[];
extern PickEntry *D_007EC7AC;
extern PickEntry *gRotationPickList3;
extern PickEntry gRotationKind2PicksC10[];          /* kind 2's list (condition 0x10) */
extern PickEntry gRotationKind2Picks[];          /* kind 2's list */
void Rate_ListAdd(s16 id, s32 a);
/* a price table (gRateGroups, five of them): the bounds and the entries */
typedef struct PriceEntry {
    s16 bias;
    s16 id;
} PriceEntry;
typedef struct PriceTable {
    s32 min;
    s32 max;
    s32 count;
    PriceEntry e[8];
} PriceTable;
extern PriceTable gRateGroups[5];
void Rate_Set(s16 id, s32 price); /* unit's own declaration: with the definition's void (s32 kind, s32 val) (main/unk_18DA50.c), Rate_UpdatePrices 48 of 142 words differ */
extern u8 D_007EC68A;                   /* the fixed prices' count */
extern u8 gFixedPriceIds[];                 /* their ids */
extern s32 gFixedPrices[];                /* their prices */

PickEntry *PickList_Random(PickEntry *list)
{
    s32 n = 0;
    PickEntry *p;

    for (p = list; p->unk2 != 0; p++) {
        n++;
    }
    return &list[Rand_Next(1) % n];
}

/* a random value by kind: 0 gives 3 or 4, 1 gives 2, 3 or 4, 3 gives 3, 4
 * or 5 (about a quarter, a quarter and a half), others 5 */
s16 Rotation_RandomDays(s32 kind)
{
    s16 v;
    u16 r;

    r = Rand_Next(1) % 100;
    v = 3;
    switch (kind) {
    case 0:
        if (r > 50) {
            v = 4;
            return v;
        }
        return v;
    case 1:
        if (r > 50) {
            return 4;
        } else if (r > 25) {
            return v;
        } else {
            v = 2;
        }
        return v;
    case 3:
        if (r > 50) {
            return 5;
        } else if (r > 25) {
            v = 4;
        }
        return v;
    default:
        return 5;
    }
}

u16 Monster_FlagBit(u16 em)
{
    s32 i;

    for (i = 0; gMonsterFlagBits[i].em != 0; i++) {
        if (gMonsterFlagBits[i].em == em) {
            return gMonsterFlagBits[i].bit;
        }
    }
    return 0xFFFF;
}

s32 Monster_IsHunted(u16 em)
{
    u16 b = Monster_FlagBit(em);

    if (b == 0xFFFF) {
        return 0;
    }
    if (gSave.monsterFlags & (1 << b)) {
        return 1;
    }
    return 0;
}

/* of the hunted monsters in list (0-terminated), the one with the least
 * Records_HuntedTotal count: {1, its id, 1, 1.5 * Rate_Get(id)} */
MonsterPick Rotation_PickLeastHunted(s16 *list)
{
    s32 counts[34];
    MonsterPick r;
    s32 best;
    s16 *p;
    s32 id;
    s32 *q;

    best = -1;
    p = list;
    id = 0;
    if (*p != 0) {
        q = counts;
        do {
            if (Monster_IsHunted(*p) != 0) {
                *q = Records_HuntedTotal((struct SaveData *)&gSave, (u8)*p, 1);
            } else {
                *q = 0;
            }
            if (*q != 0 && (best == -1 || *q < best)) {
                id = *p;
                best = *q;
            }
            p++;
            q++;
        } while (*p != 0);
    }
    r.a = 1;
    r.b = 1;
    r.id = id;
    r.c = 1.5f * Rate_Get(id);
    return r;
}

/* under condition 7, counts kind's days down (sub_main 0x5D8142); at 0 it
 * either clears the kind's three records (gSave + 0x369A + kind * 8, 0x18
 * apart) and draws new days (Rotation_RandomDays), or, when the first record is
 * empty, fills them: kind 0 with the hunted monsters of three lists
 * (Rotation_PickLeastHunted), kinds 1 and 2 with random entries (condition 0x10
 * chooses the lists), and sets the days to 1 */
void Rotation_Update(s32 kind)
{
    s16 *days;
    u8 *rec;
    MonsterPick *first;

    if (Cond_Check(7) != 0) {
        rec = (u8 *)&gSave + kind * 8; /* gSave.days[0..2][kind] at 0x369A, 0x36B2, 0x36CA: the field form changes the code */
        days = (s16 *)gSave.rotationDays + kind;
        first = (MonsterPick *)(rec + 0x369A);
        if (*days > 0) {
            (*days)--;
        }
        if (*days <= 0) {
            if (first->id == 0) {
                switch (kind) {
                case 0:
                    *(MonsterPick *)gSave.days[0][0] = Rotation_PickLeastHunted(gRotationHuntLists[0]);
                    *(MonsterPick *)gSave.days[1][0] = Rotation_PickLeastHunted(D_007EC73C);
                    *(MonsterPick *)gSave.days[2][0] = Rotation_PickLeastHunted(gRotationHuntList3);
                    break;
                case 1:
                    if (Cond_Check(0x10) == 1) {
                        memcpy(gSave.days[0][1], PickList_Random(gRotationPickListsC10[0]), 8);
                        memcpy(gSave.days[1][1], PickList_Random(D_007EC854), 8);
                        memcpy(gSave.days[2][1], PickList_Random(gRotationPickList3C10), 8);
                    } else {
                        memcpy(gSave.days[0][1], PickList_Random(gRotationPickLists[0]), 8);
                        memcpy(gSave.days[1][1], PickList_Random(D_007EC7AC), 8);
                        memcpy(gSave.days[2][1], PickList_Random(gRotationPickList3), 8);
                    }
                    break;
                case 2:
                    if (Cond_Check(0x10) == 1) {
                        memcpy(gSave.days[0][2], PickList_Random(gRotationKind2PicksC10), 8);
                        memcpy(gSave.days[1][2], PickList_Random(gRotationKind2PicksC10), 8);
                        memcpy(gSave.days[2][2], PickList_Random(gRotationKind2PicksC10), 8);
                    } else {
                        memcpy(gSave.days[0][2], PickList_Random(gRotationKind2Picks), 8);
                        memcpy(gSave.days[1][2], PickList_Random(gRotationKind2Picks), 8);
                        memcpy(gSave.days[2][2], PickList_Random(gRotationKind2Picks), 8);
                    }
                    break;
                }
                *days = 1;
            } else {
                memset(first, 0, 8);
                memset(rec + 0x36B2, 0, 8);
                memset(rec + 0x36CA, 0, 8);
                *days = Rotation_RandomDays(kind);
            }
        }
    }
}

/* a random index: 0..n-1, n from 1 to 5 by the conditions 7, 0xA, 0x15, 0x4F */
s32 Rate_RandomGroup(void)
{
    s32 n;

    if (Cond_Check(0x4F) == 1) {
        n = 5;
    } else if (Cond_Check(0x15) == 1) {
        n = 4;
    } else if (Cond_Check(0xA) == 1) {
        n = 3;
    } else if (Cond_Check(7) == 1) {
        n = 2;
    } else {
        n = 1;
    }
    return Rand_Next(1) % n;
}

/* under condition 3, once or twice at random: of the ids of a random entry
 * of gRateGroups (0x2C bytes: a count at +8, s16 ids from +0xE, 4 bytes
 * apart) the one Rate_Get rates highest goes to Rate_ListAdd */
void Rate_RaiseRandom(void)
{
    s32 v;
    u8 *t;
    s32 i;
    s32 best;
    s32 id;
    s32 k;
    s32 r;

    if (Cond_Check(3) != 0) {
        for (k = Rand_Next(1) % 2; k >= 0; k--) {
            t = (u8 *)&gRateGroups[Rate_RandomGroup()];
            best = 0;
            id = 0;
            for (i = 0; i < *(s32 *)(t + 8); i++) {
                v = *(s16 *)(t + i * 4 + 0xE);
                r = Rate_Get(v);
                if (best < r) {
                    id = v;
                    best = r;
                }
            }
            if (id != 0) {
                Rate_ListAdd(id, 1);
            }
        }
    }
}

/* the prices: for each of the five tables (gRateGroups), when the save's 16
 * bytes for it (gSave + 0x27B8 + table * 0x10) hold any id, each entry's
 * price (Rate_Get) moves by its bias minus 100 times its share of those
 * ids (-20% to +20%), kept within the table's min and max (Rate_Set);
 * then the fixed prices of gFixedPriceIds / gFixedPrices.
 * The first loop reads each id byte into k (that gives k and the entry
 * walker their registers).
 * The fixed prices loop's compiled-out check of the id puts its two walkers
 * in the game's s2/s3 (without it, 4-6 words). */
void Rate_UpdatePrices(void)
{
    SaveData *save;
    s32 i;
    s32 n;
    s16 id;
    PriceTable *t;
    u8 *rec;
    s32 k;
    s32 adj;
    s32 j;
    s32 m;
    s32 v;
    u8 fid;

    save = &gSave;
    /* rec + 0x27B8: gSave.rateLists[i] */
    for (i = 0, t = gRateGroups, rec = (u8 *)save; i < 5; i++, t++, rec += 0x10) {
        n = 0;
        for (j = 0; j < 0x10; j++) {
            k = rec[0x27B8 + j];
            if (k != 0) {
                n++;
            }
        }
        if (n > 0) {
            for (k = 0; k < t->count; k++) {
                id = t->e[k].id;
                m = 0;
                for (j = 0; j < 0x10; j++) {
                    if (id == rec[0x27B8 + j]) {
                        m++;
                    }
                }
                adj = m * 100 / n;
                adj = t->e[k].bias - adj;
                if (adj < -20) {
                    adj = -20;
                }
                if (adj > 20) {
                    adj = 20;
                }
                v = Rate_Get(id);
                v = v + v * adj / 100;
                if (v < t->min) {
                    v = t->min;
                }
                if (t->max < v) {
                    v = t->max;
                }
                Rate_Set(id, v);
            }
        }
    }
    save->rateFree = 10;
    n = D_007EC68A;
    for (i = 0; i < n; i++) {
        fid = gFixedPriceIds[i];
        if (fid == 0 || fid > 0xF0) {} /* compiled-out check */
        Rate_Set(fid, gFixedPrices[i]);
    }
}

extern u8 gMenuWork[];
extern s8 gMenuInputLock;
extern s16 gPriceMonstersOnline[];
extern s16 gPriceMonsters[];
extern s16 gPriceShown[];
extern s16 gPriceShownOnline[];
extern s32 D_007EC930[];
extern s32 D_007EC950[];
extern s32 gRateSnapshot[];
s32 Game_IsOnline(void);
void LobbySe_Play(s32 se);
s32 Rate_GetSnapshot(s32 id);
s32 PriceInfo_IsListed(int);
s32 Menu_CountPages(s16 n, s16 per);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
struct TextBox;
void PriceInfo_Draw(struct TextBox *);
void PriceInfo_Draw2(struct TextBox *);
s8 TownLists_Count(void);
void CamReq_Lobby1B(s32 a);

/* the monster list screen's setup: the monsters of gPriceMonstersOnline/640 (hunted,
 * or in gPriceShownOnline/670: shown; others negated), then five extra entries;
 * the page count, the town lists, the area's music (CamReq_Lobby1B).
 * The extra entries' test keeps 1 in a register (`li v0, 1; sltu v0, cnt,
 * v0`) that is set only when the list has a monster: `one`, like k, is unset
 * when the list is empty, and the game reads both unset there (its own bug;
 * this code is the game's byte for byte, so its register for the 1 is set
 * on that branch only). Every form that sets the threshold on all paths
 * folds the 1 (rounds 22 to 30: docs/notes/nm/PriceInfo_Open.md). The search
 * loop's bnez/b/b shape comes from the unsigned compare (u32)id == *p. */
void PriceInfo_Open(void)
{
    s16 k;
    u8 *w = gMenuWork;
    u8 *src = (u8 *)&gTownQuestLists;
    s32 i;
    s16 *list;
    s32 n;
    s32 found;
    u8 *a;
    u8 *b;
    s32 id;
    s16 *p;
    s32 *tbl;
    u8 *c;
    s32 area;
    u32 cnt;
    u32 one;                            /* 1 once the list has a monster; unset with k, as in the game */

    memset(w, 0, 0xF00);
    LobbySe_Play(0x2C);
    if (Game_IsOnline() == 1) {
        list = gPriceMonstersOnline;
    } else {
        list = gPriceMonsters;
    }
    id = *list;
    n = 0;
    i = 0;
    if (id != 0) {
        a = w;
        b = w;
        one = 1;
        do {
            if (i >= 0x4A) {
                break;
            }
            k = id;
            *(s32 *)(a + 0x160) = Rate_GetSnapshot(k);
            found = 0;
            if (Game_IsOnline() == 1) {
                p = gPriceShownOnline;
            } else {
                p = gPriceShown;
            }
            while (*p != 0) {
                if ((u32)id == *p) {
                    found = 1;
                    break;
                }
                p++;
            }
            PriceInfo_IsListed(id);
            if (gRateSnapshot[id] != 0) {
                if (gSave.hunted[id] + gSave.captured[id] != 0 || found == 1) {
                    *(s16 *)(b + 0xCA) = id;
                } else {
                    *(s16 *)(b + 0xCA) = -id;
                }
                b += 2;
                n++;
            }
            id = *++list;
            a += 4;
            i++;
        } while (id != 0);
    }
    if (Game_IsOnline() == 1) {
        tbl = D_007EC930;
    } else {
        tbl = D_007EC950;
    }
    /* FAKEMATCH: list and p, dead after the first loop, hold the counts'
     * addresses, and area (not yet used) holds n * 2: as later webs of list
     * and p around the counter's, and area's first web, they take the game's
     * registers (captured a2, i a1, hunted a0, n * 2 a0). */
    i = 0;
    area = n * 2;
    c = w + area;
    list = (s16 *)&gSave.captured[k];
    p = (s16 *)&gSave.hunted[k];
    for (; i < 5; i++) {
        cnt = (u16)*p + (u16)*list;
        if (cnt >= one) {
            *(s16 *)(c + 0xCA) = *tbl;
        }
        tbl++;
        c += 2;
        n++;
    }
    w[0xC7] = Menu_CountPages(n, 0xB);
    LobbyMenu_SetDraw(PriceInfo_Draw, PriceInfo_Draw2, 0, 0);
    w[0xC8] = TownLists_Count();
    memcpy(w, src + 4, 0x48);
    memcpy(w + 0x48, src + 0x4C, 0x48);
    gMenuInputLock = 9;
    switch (gScene.area) {
    case AREA_HUB_0x89:
        area = 0;
        break;
    case AREA_HUB_0x86:
        area = 1;
        break;
    case AREA_HUB_0x88:
        area = 2;
        break;
    case AREA_HUB_0x83:
        area = 3;
        break;
    case AREA_HUB_0x57:
        area = 4;
        break;
    default:
        area = 0xFF;
        break;
    }
    if (area != 0xFF) {
        CamReq_Lobby1B(area);
    }
}

/* 1 when monster id is in the list D_007EC5F0 (0-terminated, 0x4A at most) */
s32 PriceInfo_IsListed(s32 idIn)
{
    s16 id = idIn;
    s16 i;
    s16 *p;

    for (i = 0, p = D_007EC5F0; i < 0x4A; i++, p++) {
        if (*p == 0) {
            return 0;
        }
        if (id == *p) {
            return 1;
        }
    }
    return 0;
}

/* unk_7106D0's .data (0x7EC5F0-0x7EC968) */
s16 D_007EC5F0[16] = {
#include "lobby/007EC5F0.inc"
};
s16 gPriceMonstersOnline[24] = {
#include "lobby/007EC610.inc"
};
s16 gPriceMonsters[24] = {
#include "lobby/007EC640.inc"
};
s16 gPriceShown[4] = {
#include "lobby/007EC670.inc"
};
s16 gPriceShownOnline[6] = {
#include "lobby/007EC678.inc"
};
u32 gPricePageCounts[2] = {
#include "lobby/007EC688.inc"
};
u32 gPricePageCountsOnline[2] = {
#include "lobby/007EC690.inc"
};
u32 gOnlinePriceIds[2] = {
#include "lobby/007EC698.inc"
};
u8 gFixedPriceIds[16] = {
#include "lobby/007EC6A0.inc"
};
s32 gFixedPrices[8] = {
#include "lobby/007EC6B0.inc"
};
u32 gRotationHuntLists_0[8] = {
#include "lobby/007EC6D0.inc"
};
u32 gRotationHuntLists_1[8] = {
#include "lobby/007EC6F0.inc"
};
s16 D_007EC710[20] = {
#include "lobby/007EC710.inc"
};
s16 *gRotationHuntLists[2] = {
    (s16 *)gRotationHuntLists_0, (s16 *)gRotationHuntLists_1,
};
s16 *gRotationHuntList3 = D_007EC710;
u32 gRotationPickLists_0[8] = {
#include "lobby/007EC750.inc"
};
u32 gRotationPickLists_1[8] = {
#include "lobby/007EC770.inc"
};
PickEntry D_007EC790[3] = {
#include "lobby/007EC790.inc"
};
PickEntry *gRotationPickLists[2] = {
    (PickEntry *)gRotationPickLists_0, (PickEntry *)gRotationPickLists_1,
};
PickEntry *gRotationPickList3 = D_007EC790;
u32 gRotationPickListsC10_0[12] = {
#include "lobby/007EC7C0.inc"
};
u32 gRotationPickListsC10_1[12] = {
#include "lobby/007EC7F0.inc"
};
PickEntry D_007EC820[6] = {
#include "lobby/007EC820.inc"
};
PickEntry *gRotationPickListsC10[2] = {
    (PickEntry *)gRotationPickListsC10_0, (PickEntry *)gRotationPickListsC10_1,
};
PickEntry *gRotationPickList3C10 = D_007EC820;
PickEntry gRotationKind2Picks[10] = {
#include "lobby/007EC860.inc"
};
PickEntry gRotationKind2PicksC10[13] = {
#include "lobby/007EC8B0.inc"
};
u32 gPriceBoardColors[3] = {
#include "lobby/007EC918.inc"
};
s32 D_007EC930[8] = {
#include "lobby/007EC930.inc"
};
s32 D_007EC950[6] = {
#include "lobby/007EC950.inc"
};
