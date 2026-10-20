/*
 * The town's market: the hunt rotation, the monster rates and prices, and
 * the price board that shows them (TOWN_MODE_PRICE_INFO).
 *
 * Rotation_Update picks the wanted monsters and items of the next days
 * (gSave.days) from pick lists (WantedRec), preferring the monsters hunted
 * least. Rate_RaiseRandom and Rate_UpdatePrices move each rate group's
 * prices (gRateGroups, PriceTable) by the ids the save has collected
 * (gSave.rateLists) when Rate_UpdateDue says so; the town's update calls
 * them offline.
 *
 * PriceInfo_Open opens the board: its work in gMenuWork is a BoardMenu
 * (the season's wanted lines, the listed monsters' ids and prices).
 * PriceInfo_Update runs it and PriceInfo_Draw and PriceInfo_Draw2 draw it.
 */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "scene.h"
#include "area.h"
#include "lobby_text.h"
#include "sprite.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "pad.h"
#include "texture.h"
#include "lobby_labels.h"
/* gPriceInfoTexts from PRICE_INFO_WANTED_A, under a name of its own (a linker
 * alias): read as an index into gPriceInfoTexts, the address is computed in
 * another order and PriceInfo_DrawWanted compiles differently. */
extern char *gPriceWantedLines[];

s32 Cond_Check(u16 id);
extern s16 gPriceListedMonsters[];
/* A rotation or wanted record, laid out as lobby_globals.h's TownEntry: the
 * kind (1 a monster, 3 an item), the monster's or item's id (0 ends a pick
 * list), the count wanted and the reward. */
typedef struct WantedRec {
    s16 kind;
    s16 id;
    s16 count;
    s16 reward;
} WantedRec;
/* From a record of the first hub's lists to the same record of the second's
 * (BoardMenu.wanted to wanted2, TownQuestLists.a to b). */
#define WANTED_SECOND_HUB   (WANTED_SEASONS * WANTED_ENTRIES)
#define MONSTER_BIT_NONE    0xFFFF  /* Monster_FlagBit: the monster has no bit */
#define RATE_GROUP_COUNT    5       /* gRateGroups and the save's rate lists */
#define RATE_LIST_LEN       0x10    /* the entries of one save rate list (SaveData.rateLists) */
#define RATE_FREE_START     10      /* SaveData.rateFree after the prices change (Rate_UpdateDue waits for 0) */
#define PRICE_LIST_MAX      0x4A    /* the price list's lines (BoardMenu.ids, gRateSnapshot) */
#define PRICE_PAGE_LINES    0xB     /* the price list's lines per page */
#define BOARD_LINES         4       /* the board's lines: the price list and the three wanted lines */
u16 Rand_Next(s32 stream);
extern MonsterBit gMonsterFlagBits[];
/* unit's own declaration: the rate id is taken as an s16 here; with an s32 id Rate_UpdatePrices compiles differently */
s32 Rate_Get(s16 id);
u16 Records_HuntedTotal(struct SaveData *save, u8 group, u8 captured);
void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
extern s16 *gRotationHuntLists[];
extern s16 *gRotationHuntList3;
extern WantedRec *gRotationPickListsC10[];
extern WantedRec *gRotationPickList3C10;
extern WantedRec *gRotationPickLists[];
extern WantedRec *gRotationPickList3;
extern WantedRec gRotationKind2PicksC10[];          /* kind 2's list (condition 0x10) */
extern WantedRec gRotationKind2Picks[];          /* kind 2's list */
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
void Rate_Set(s16 id, s32 price); /* unit's own declaration: declared with an s16 id and an s32 price as Rate_UpdatePrices passes them; with the definition's two s32 the call compiles differently */
extern u8 gFixedPriceIds[];                 /* their ids */
extern s32 gFixedPrices[];                /* their prices */
void CamReq_Cancel(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
int abs(int);
void Font_SetSpacing(u8 a, u8 b);
void Text_DrawShadowed(s16 x, s32 y, u32 color, s32 a, char *s); /* unit's own declaration: declared with an s32 y and the colours as u32 and s32, as PriceInfo_DrawWanted passes them; with the definition's types the call compiles differently */
typedef struct TextBox { u8 pad[0x18]; s32 size; } TextBox;
extern u8 gMenuWork[];
void Text_SetSize(s32 size);
s32 LobbyWorld_GetSeason(void);
s32 Wanted_IsSet(s16 *p);
/* the board screen's state (gMenuWork) */
typedef struct BoardMenu {
    /* 0x00 */ WantedRec wanted[3][3];   /* gTownQuestLists.a: [season][line], the first hub's lists */
    /* 0x48 */ WantedRec wanted2[3][3];  /* gTownQuestLists.b: the second hub's, reached as wanted + WANTED_SECOND_HUB */
    /* 0x90 */ WinSlide win[2];      /* the list window and the detail window */
    /* 0xA0 */ WinSlide s[4];        /* the four windows */
    /* 0xC0 */ s8 page;
    /* 0xC1 */ s8 step;                /* PriceInfo_SlideWindows */
    /* 0xC2 */ s8 timer;
    /* 0xC3 */ s8 sel;
    /* 0xC4 */ u8 padC4[2];
    /* 0xC6 */ s8 listPage;     /* the list page shown */
    /* 0xC7 */ s8 listPages;    /* the list's page count (Menu_CountPages, 11 lines a page) */
    /* 0xC8 */ s8 wantedCount;   /* the wanted entries set (Wanted_Count) */
    /* 0xC9 */ u8 padC9;
    /* 0xCA */ s16 ids[PRICE_LIST_MAX];          /* the list lines: an item, negative for none */
} BoardMenu;
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 colour, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
extern u8 gPricePageCounts[];                 /* the pages' line counts (Game_IsOnline == 0) */
extern u8 gPricePageCountsOnline[];                 /* the same, Game_IsOnline != 0 */
extern u8 gOnlinePriceIds[];                 /* page 2's items (Game_IsOnline != 0) */
extern s32 gRateSnapshot[];                  /* the items' prices */
extern char *gMonsterNames[];                /* the items' names */
s32 Game_IsOnline(void);
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
typedef struct Colours3 {
    u32 c[3];
} Colours3;
extern Colours3 gPriceLegendColors;             /* the legend's colours */
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, M2C_UNK se);
extern u8 gLobbyRateStates[];                 /* the entries' levels and marks */
void Font_SetColor(s32 color);
extern char gPriceStrFmt[];               /* "%s" */
extern u32 gPriceBoardColors[];                /* the board texts' colours */
void Icon_DrawRotated(s16 x, s16 y, s16 w, s16 h, u32 colour, u16 flags, s32 spr); /* unit's own declaration: declared with s16 sizes, a u32 colour and u16 flags, as PriceInfo_DrawRate passes them; with the definition's s32 parameters the call compiles differently */
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 colour, s32 spr, u8 a);
void Font_SetPos(s16, s16);
void Text_PrintfColored(char *s, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);

/*
 * Rename to Rate_UpdateDue. 1 when the save's free count of the rate lists
 * is 0 (Rate_ListAdd counts it down as kinds are added), meaning the
 * prices are due for an update (Rate_UpdatePrices, which sets the count
 * back to 10); else 0.
 */
s32 Rate_UpdateDue(void)
{
    return gSave.rateFree == 0;
}

/*
 * One entry of a pick list (ended by an entry whose id is 0), chosen at
 * random with random stream 1; returns a pointer to it.
 */
WantedRec *PickList_Random(WantedRec *list)
{
    s32 n = 0;
    WantedRec *entry;

    for (entry = list; entry->id != 0; entry++) {
        n++;
    }
    return &list[Rand_Next(1) % n];
}

/*
 * The days until a rotation kind picks again, drawn at random: kind 0
 * gives 3 or 4 (about half each), kind 1 gives 2, 3 or 4 (a quarter, a
 * quarter, a half), kind 3 gives 3, 4 or 5 (a quarter, a quarter, a half),
 * any other kind 5.
 */
s16 Rotation_RandomDays(s32 kind)
{
    s16 days;
    u16 roll;

    roll = Rand_Next(1) % 100;
    days = 3;
    switch (kind) {
    case 0:
        if (roll > 50) {
            days = 4;
            return days;
        }
        return days;
    case 1:
        if (roll > 50) {
            return 4;
        } else if (roll > 25) {
            return days;
        } else {
            days = 2;
        }
        return days;
    case 3:
        if (roll > 50) {
            return 5;
        } else if (roll > 25) {
            days = 4;
        }
        return days;
    default:
        return 5;
    }
}

/*
 * The bit of the save's monster flags that stands for monster em (looked
 * up in gMonsterFlagBits, ended by em 0); 0xFFFF when the monster has
 * none.
 */
u16 Monster_FlagBit(u16 em)
{
    s32 i;

    for (i = 0; gMonsterFlagBits[i].em != 0; i++) {
        if (gMonsterFlagBits[i].em == em) {
            return gMonsterFlagBits[i].bit;
        }
    }
    return MONSTER_BIT_NONE;
}

/*
 * 1 when monster em's bit in the save's monster flags is set (it has been
 * hunted), 0 when it is clear or the monster has no bit.
 */
s32 Monster_IsHunted(u16 em)
{
    u16 bit = Monster_FlagBit(em);

    if (bit == MONSTER_BIT_NONE) {
        return 0;
    }
    if (gSave.monsterFlags & (1 << bit)) {
        return 1;
    }
    return 0;
}

/*
 * Of the hunted monsters in list (ended by 0), the one with the smallest
 * nonzero hunted-and-captured total (Records_HuntedTotal); returns a
 * wanted entry for it: kind 1 (a monster), its id, count 1, reward 1.5
 * times its rate (Rate_Get). Id 0 when none of them was hunted.
 */
WantedRec Rotation_PickLeastHunted(s16 *list)
{
    s32 counts[34];
    WantedRec pick;
    s32 best;
    s16 *emPtr;
    s32 id;
    s32 *countPtr;

    best = -1;
    emPtr = list;
    id = 0;
    if (*emPtr != 0) {
        countPtr = counts;
        do {
            if (Monster_IsHunted(*emPtr) != 0) {
                *countPtr = Records_HuntedTotal((struct SaveData *)&gSave, (u8)*emPtr, 1);
            } else {
                *countPtr = 0;
            }
            if (*countPtr != 0 && (best == -1 || *countPtr < best)) {
                id = *emPtr;
                best = *countPtr;
            }
            emPtr++;
            countPtr++;
        } while (*emPtr != 0);
    }
    pick.kind = WANTED_KIND_MONSTER;
    pick.count = 1;
    pick.id = id;
    pick.reward = 1.5f * Rate_Get(id);
    return pick;
}

/*
 * Under condition 7, counts the days of rotation kind down
 * (gSave.rotationDays). At 0: when the first day's entry of the kind is
 * empty it fills the kind's entry on all three days
 * (gSave.days[0..2][kind]): kind 0 the least hunted of three hunt lists,
 * kinds 1 and 2 random picks (condition 0x10 chooses the later lists), and
 * sets the days to 1; otherwise it clears the three entries and draws new
 * days (Rotation_RandomDays). The entries are reached through a byte
 * pointer because the field form changes the code.
 */
void Rotation_Update(s32 kind)
{
    s16 *days;
    u8 *rec;
    WantedRec *first;

    if (Cond_Check(7) != 0) {
        rec = (u8 *)&gSave + kind * 8; /* gSave.days[0..2][kind] at 0x369A, 0x36B2, 0x36CA: the field form changes the code */
        days = (s16 *)gSave.rotationDays + kind;
        first = (WantedRec *)(rec + 0x369A);
        if (*days > 0) {
            (*days)--;
        }
        if (*days <= 0) {
            if (first->id == 0) {
                switch (kind) {
                case 0:
                    *(WantedRec *)gSave.days[0][0] = Rotation_PickLeastHunted(gRotationHuntLists[0]);
                    *(WantedRec *)gSave.days[1][0] = Rotation_PickLeastHunted(gRotationHuntLists[1]);
                    *(WantedRec *)gSave.days[2][0] = Rotation_PickLeastHunted(gRotationHuntList3);
                    break;
                case 1:
                    if (Cond_Check(0x10) == 1) {
                        memcpy(gSave.days[0][1], PickList_Random(gRotationPickListsC10[0]), 8);
                        memcpy(gSave.days[1][1], PickList_Random(gRotationPickListsC10[1]), 8);
                        memcpy(gSave.days[2][1], PickList_Random(gRotationPickList3C10), 8);
                    } else {
                        memcpy(gSave.days[0][1], PickList_Random(gRotationPickLists[0]), 8);
                        memcpy(gSave.days[1][1], PickList_Random(gRotationPickLists[1]), 8);
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

/*
 * Under condition 3, once or twice at random: takes a random rate group
 * (gRateGroups, Rate_RandomGroup) and adds its monster with the highest
 * rate (Rate_Get) to the rate lists (Rate_ListAdd).
 */
void Rate_RaiseRandom(void)
{
    s32 value;
    u8 *table;
    s32 i;
    s32 best;
    s32 id;
    s32 k;
    s32 rate;

    if (Cond_Check(3) != 0) {
        for (k = Rand_Next(1) % 2; k >= 0; k--) {
            table = (u8 *)&gRateGroups[Rate_RandomGroup()];
            best = 0;
            id = 0;
            for (i = 0; i < *(s32 *)(table + 8); i++) {
                value = *(s16 *)(table + i * 4 + 0xE);
                rate = Rate_Get(value);
                if (best < rate) {
                    id = value;
                    best = rate;
                }
            }
            if (id != 0) {
                Rate_ListAdd(id, 1);
            }
        }
    }
}

/*
 * The prices: for each of the five rate groups whose save list
 * (gSave.rateLists) holds any monster, each entry's rate moves by its bias
 * minus 100 times its share of the listed monsters, clamped to -20..+20
 * percent, and kept within the group's min and max (Rate_Set). Then resets
 * the free count to 10 and sets the fixed prices (gFixedPriceIds /
 * gFixedPrices). The first loop reads each id byte into k, and the fixed
 * prices loop keeps an empty check of the id: both forms give the game's
 * register choice.
 * The first loop reads each id byte into a local of its own, and the fixed
 * prices' loop keeps a check of the id that compiles to nothing: both give
 * the loops' pointers the registers the game's code uses.
 */
void Rate_UpdatePrices(void)
{
    SaveData *save;
    s32 i;
    s32 n;
    s16 id;
    PriceTable *table;
    u8 *saveBytes;
    s32 k;
    s32 adj;
    s32 j;
    s32 hits;
    s32 value;
    u8 fid;

    save = &gSave;
    /* rec + 0x27B8: gSave.rateLists[i] */
    for (i = 0, table = gRateGroups, saveBytes = (u8 *)save; i < RATE_GROUP_COUNT; i++, table++, saveBytes += RATE_LIST_LEN) {
        n = 0;
        for (j = 0; j < RATE_LIST_LEN; j++) {
            k = saveBytes[0x27B8 + j];
            if (k != 0) {
                n++;
            }
        }
        if (n > 0) {
            for (k = 0; k < table->count; k++) {
                id = table->e[k].id;
                hits = 0;
                for (j = 0; j < RATE_LIST_LEN; j++) {
                    if (id == saveBytes[0x27B8 + j]) {
                        hits++;
                    }
                }
                adj = hits * 100 / n;
                adj = table->e[k].bias - adj;
                if (adj < -20) {
                    adj = -20;
                }
                if (adj > 20) {
                    adj = 20;
                }
                value = Rate_Get(id);
                value = value + value * adj / 100;
                if (value < table->min) {
                    value = table->min;
                }
                if (table->max < value) {
                    value = table->max;
                }
                Rate_Set(id, value);
            }
        }
    }
    save->rateFree = RATE_FREE_START;
    n = gPricePageCounts[2];
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
extern s32 gPriceExtraIdsOnline[];
extern s32 gPriceExtraIds[];
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
s8 Wanted_Count(void);
void CamReq_TownArea(s32 a);

/*
 * Opens the price board: clears the menu work, plays its sound, fills the
 * list lines with the price-board monsters (online or offline list) that
 * have a price: their id when hunted, captured or always shown, minus
 * their id otherwise; then up to five extra lines, the page count, the
 * wanted count, the copies of the town's wanted entries, the input lock
 * and the hub's camera. The extra lines' test keeps the constant 1 in a
 * register set only when the list has a monster (unset with k when it is
 * empty, as in the game, which reads both unset there); every form that
 * sets it on all paths folds the 1. The search loop's branch shape comes
 * from the unsigned compare of id with *p.
 * The extra entries' test keeps its constant 1 in a register that is set
 * only when the list has a monster: when the list is empty the game's code
 * reads that register (and the count) unset, and this C does the same; a
 * form that sets it on every path folds the constant into the compare. The
 * search compares the id unsigned, which gives the game's branches.
 */
void PriceInfo_Open(void)
{
    s16 k;
    u8 *w = gMenuWork;
    TownQuestLists *src = &gTownQuestLists;
    s32 i;
    s16 *list;
    s32 n;
    s32 found;
    u8 *priceOut;
    u8 *idOut;
    s32 id;
    s16 *scan;
    s32 *tbl;
    u8 *extraOut;
    s32 area;
    u32 cnt;
    u32 one;                            /* 1 once the list has a monster; unset with k, as in the game */

    memset(w, 0, MENU_WORK_SIZE);
    LobbySe_Play(LOBBY_SE_LIST_OPEN);
    if (Game_IsOnline() == 1) {
        list = gPriceMonstersOnline;
    } else {
        list = gPriceMonsters;
    }
    id = *list;
    n = 0;
    i = 0;
    if (id != 0) {
        priceOut = w;
        idOut = w;
        one = 1;
        do {
            if (i >= PRICE_LIST_MAX) {
                break;
            }
            k = id;
            *(s32 *)(priceOut + 0x160) = Rate_GetSnapshot(k);
            found = 0;
            if (Game_IsOnline() == 1) {
                scan = gPriceShownOnline;
            } else {
                scan = gPriceShown;
            }
            while (*scan != 0) {
                if ((u32)id == *scan) {
                    found = 1;
                    break;
                }
                scan++;
            }
            PriceInfo_IsListed(id);
            if (gRateSnapshot[id] != 0) {
                if (gSave.hunted[id] + gSave.captured[id] != 0 || found == 1) {
                    *(s16 *)(idOut + 0xCA) = id;
                } else {
                    *(s16 *)(idOut + 0xCA) = -id;
                }
                idOut += 2;
                n++;
            }
            id = *++list;
            priceOut += 4;
            i++;
        } while (id != 0);
    }
    if (Game_IsOnline() == 1) {
        tbl = gPriceExtraIdsOnline;
    } else {
        tbl = gPriceExtraIds;
    }
    /* FAKEMATCH: list and p, dead after the first loop, hold the counts'
     * addresses, and area (not yet used) holds n * 2: as later webs of list
     * and p around the counter's, and area's first web, they take the game's
     * registers (captured a2, i a1, hunted a0, n * 2 a0). */
    i = 0;
    area = n * 2;
    extraOut = w + area;
    list = (s16 *)&gSave.captured[k];
    scan = (s16 *)&gSave.hunted[k];
    for (; i < 5; i++) {
        cnt = (u16)*scan + (u16)*list;
        if (cnt >= one) {
            *(s16 *)(extraOut + 0xCA) = *tbl;
        }
        tbl++;
        extraOut += 2;
        n++;
    }
    ((BoardMenu *)w)->listPages = Menu_CountPages(n, PRICE_PAGE_LINES);
    LobbyMenu_SetDraw(PriceInfo_Draw, PriceInfo_Draw2, 0, 0);
    ((BoardMenu *)w)->wantedCount = Wanted_Count();
    memcpy(((BoardMenu *)w)->wanted, src->a, sizeof(src->a));
    memcpy(((BoardMenu *)w)->wanted2, src->b, sizeof(src->b));
    gMenuInputLock = 9;
    switch (gScene.area) {
    case AREA_HUB_0x89:
        area = 0;
        break;
    case AREA_TAVERN:
        area = 1;
        break;
    case AREA_ELDER_HALL:
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
        CamReq_TownArea(area);
    }
}

/*
 * 1 when monster id is in gPriceListedMonsters (ended by 0, at most PRICE_LIST_MAX
 * entries), else 0.
 */
s32 PriceInfo_IsListed(s32 idIn)
{
    s16 id = idIn;
    s16 i;
    s16 *entry;

    for (i = 0, entry = gPriceListedMonsters; i < PRICE_LIST_MAX; i++, entry++) {
        if (*entry == 0) {
            return 0;
        }
        if (id == *entry) {
            return 1;
        }
    }
    return 0;
}

/* The price board's and the hunt rotation's data. */
s16 gPriceListedMonsters[16] = {
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
u8 gPricePageCounts[8] = {
#include "lobby/007EC688.inc"
};
u8 gPricePageCountsOnline[8] = {
#include "lobby/007EC690.inc"
};
u8 gOnlinePriceIds[8] = {
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
s16 gRotationHuntList3Data[20] = {
#include "lobby/007EC710.inc"
};
s16 *gRotationHuntLists[2] = {
    (s16 *)gRotationHuntLists_0, (s16 *)gRotationHuntLists_1,
};
s16 *gRotationHuntList3 = gRotationHuntList3Data;
u32 gRotationPickLists_0[8] = {
#include "lobby/007EC750.inc"
};
u32 gRotationPickLists_1[8] = {
#include "lobby/007EC770.inc"
};
WantedRec gRotationPickList3Data[3] = {
#include "lobby/007EC790.inc"
};
WantedRec *gRotationPickLists[2] = {
    (WantedRec *)gRotationPickLists_0, (WantedRec *)gRotationPickLists_1,
};
WantedRec *gRotationPickList3 = gRotationPickList3Data;
u32 gRotationPickListsC10_0[12] = {
#include "lobby/007EC7C0.inc"
};
u32 gRotationPickListsC10_1[12] = {
#include "lobby/007EC7F0.inc"
};
WantedRec gRotationPickList3C10Data[6] = {
#include "lobby/007EC820.inc"
};
WantedRec *gRotationPickListsC10[2] = {
    (WantedRec *)gRotationPickListsC10_0, (WantedRec *)gRotationPickListsC10_1,
};
WantedRec *gRotationPickList3C10 = gRotationPickList3C10Data;
WantedRec gRotationKind2Picks[10] = {
#include "lobby/007EC860.inc"
};
WantedRec gRotationKind2PicksC10[13] = {
#include "lobby/007EC8B0.inc"
};
u32 gPriceBoardColors[3] = {
#include "lobby/007EC918.inc"
};
s32 gPriceExtraIdsOnline[8] = {
#include "lobby/007EC930.inc"
};
s32 gPriceExtraIds[6] = {
#include "lobby/007EC950.inc"
};

/*
 * Leaves the price board: back to the town's free mode, the menu closed,
 * the draw callbacks cleared and the camera request cancelled.
 */
void PriceInfo_Close(void)
{
    gTownMode = TOWN_MODE_FREE;
    gTownMenuOpen = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
}

/* The price board's functions from here on are compiled at optimization
 * level 4, which their code needs; the rates' functions above need the
 * default level (at level 4 Rate_UpdatePrices gets other registers). */
#pragma optimization_level 4

/* slides the board's four windows in from x from to to, one after the
 * other; 0 once none moves, 2 while they do */
s32 PriceInfo_SlideWindows(s16 fromX, s16 toX)
{
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s16 i;
    s16 busy = 0;

    for (i = 0; i < BOARD_LINES; i++) {
        if (menu->s[i].state) {
            WinSlide_Step(&menu->s[i], fromX, toX);
            busy = 1;
        }
    }
    switch (menu->step) {
    case 0:
        menu->timer = 0;
        WinSlide_Step(&menu->s[0], fromX, toX);
        menu->s[3].pos = fromX;
        busy = 1;
        menu->s[2].pos = fromX;
        menu->s[1].pos = fromX;
        menu->step++;
        break;
    case 1:
        if (++menu->timer > 0) {
            menu->step++;
            menu->timer = 0;
            WinSlide_Step(&menu->s[1], fromX, toX);
            busy = 1;
        }
        break;
    case 2:
        if (++menu->timer > 0) {
            menu->step++;
            menu->timer = 0;
            WinSlide_Step(&menu->s[2], fromX, toX);
            busy = 1;
        }
        break;
    case 3:
        if (++menu->timer > 0) {
            menu->step++;
            menu->timer = 0;
            WinSlide_Step(&menu->s[3], fromX, toX);
            busy = 1;
        }
        break;
    }
    if (busy == 0) {
        menu->step = 0;
        return 0;
    }
    return 2;
}

void Chat_SetWindowMode(s8 a, s8 b);
u16 LobbyPad_Get(s32 a);
s32 PriceInfo_ListInput(u16 pad);
s32 Menu_WrapLeftRightSe(s32 cur, s32 n, M2C_UNK se);
void PriceInfo_Close(void);
s32 PriceInfo_SlideWindows(s16 from, s16 to);

/*
 * The board screen each frame (BoardMenu.page): the line windows slide in,
 * a line is chosen (PriceInfo_ListInput), the list or detail window slides
 * in (and the second detail window when both wanted entries of the line
 * are set), the list's pages turn with left and right, cross goes back to
 * the lines, and on leaving the windows slide out and the board closes.
 */
void PriceInfo_Update(void)
{
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s32 season = LobbyWorld_GetSeason();
    u16 pad = LobbyPad_Get(0);
    s8 i;
    WantedRec *wanted;

    Chat_SetWindowMode(1, 2);
    switch (menu->page) {
    case 0:
        switch (PriceInfo_SlideWindows(0x280, 0x118)) {
        case 0:
            menu->page++;
            break;
        }
        break;
    case 1:
        switch (PriceInfo_ListInput(pad)) {
        case 0:
            menu->page++;
            WinSlide_Step(&menu->win[0], 0x280, 0x140);
            menu->listPage = 0;
            menu->win[1].pos = menu->win[0].pos;
            gMenuInputLock = 3;
            break;
        case 3:
            PriceInfo_SlideWindows(0x118, 0x280);
            menu->page = 5;
            break;
        }
        break;
    case 2:
        switch (WinSlide_Step(&menu->win[0], 0x280, 0x140)) {
        case 0:
            i = menu->sel - 1;
            menu->page = 4;
            wanted = &menu->wanted[season][i];
            if (Wanted_IsSet(&(wanted + WANTED_SECOND_HUB)->kind) == 1 && Wanted_IsSet(&wanted->kind) == 1) {
                menu->page = 3;
                WinSlide_Step(&menu->win[1], 0x140, 0x1E);
                return;
            }
            break;
        }
        wanted = &menu->wanted[season][i];
        if (Wanted_IsSet(&(wanted + WANTED_SECOND_HUB)->kind) == 1 && Wanted_IsSet(&wanted->kind) == 1) {
            if (menu->win[0].timer < 0) {
                WinSlide_Step(&menu->win[1], 0x140, 0x1E);
            } else {
                menu->win[1].pos = menu->win[0].pos;
            }
        }
        break;
    case 3:
        switch (WinSlide_Step(&menu->win[1], 0x140, 0x1E)) {
        case 0:
            menu->page++;
            break;
        }
        break;
    case 4:
        if (pad & PAD_CROSS) {
            menu->page = 1;
            LobbySe_Play(LOBBY_SE_LIST_BACK);
        } else if (menu->sel == 0) {
            menu->listPage = Menu_WrapLeftRightSe(menu->listPage, menu->listPages, LOBBY_SE_LIST_CURSOR);
        }
        break;
    case 5:
        switch (PriceInfo_SlideWindows(0x118, 0x280)) {
        case 0:
            menu->page++;
            PriceInfo_Close();
            break;
        }
        break;
    }
}

/*
 * The board's line choice: circle takes the chosen line (the price list,
 * or a wanted line when either of its entries is set; MENU_RUNNING with
 * the refuse sound when neither is) and returns MENU_DONE; cross returns
 * MENU_CANCEL; up and down move the cursor over the four lines
 * (MENU_RUNNING).
 */
s32 PriceInfo_ListInput(u16 pad)
{
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s32 season;
    WantedRec *wanted;

    season = LobbyWorld_GetSeason();
    if (pad & PAD_CIRCLE) {
        switch (menu->sel) {
        case 1:
        case 2:
        case 3:
            wanted = &menu->wanted[season][(s8)(menu->sel - 1)];
            if (Wanted_IsSet(&wanted->kind) == 0 && Wanted_IsSet(&(wanted + WANTED_SECOND_HUB)->kind) == 0) {
                LobbySe_Play(LOBBY_SE_REFUSE);
                return 2;
            }
            LobbySe_Play(LOBBY_SE_LIST_ENTRY);
            break;
        default:
            LobbySe_Play(LOBBY_SE_LIST_SELECT);
            break;
        }
        return 0;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return 3;
    }
    menu->sel = Menu_WrapUpDownSe(menu->sel, BOARD_LINES, LOBBY_SE_LIST_CURSOR);
    return 2;
}

/*
 * Draws the title of wanted line k (gPriceWantedLines: gPriceInfoTexts from PRICE_INFO_WANTED_A) centred on x,
 * shadowed, in color.
 */
void PriceInfo_DrawWanted(s16 x, s32 y, s16 line, s32 color)
{
    char **text;
    s32 textX;

    Font_SetSpacing(0x16, 0x16);
    text = &gPriceWantedLines[line];
    textX = Text_CenterX(*text, x, 0x16);
    Text_DrawShadowed(textX, y, 0xFF000000, color, *text);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/market", gPriceStrFmt);
void Tex_Bind(s32 i);

/*
 * The label of board line n, centred on x + 0xAA at y + 0x1D: line 0 the
 * price list's label, the others the wanted line's title in its colour
 * (gPriceBoardColors), grey when neither of its entries is set. Returns
 * the line frame's colour. Text_CenterX returns s16: its result stored in
 * x is extended in x's register (an int result goes straight into a0).
 * Text_CenterX returns an s16, so its result is sign-extended into x's
 * register, as in the game's code.
 */
u32 PriceInfo_DrawTitle(s16 x, s16 y, s16 n)
{
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s32 season;
    s16 i;
    WantedRec *wanted;

    season = LobbyWorld_GetSeason();
    Font_SetSpacing(0x16, 0x16);
    switch (n) {
    case 0:
        Font_SetColor(0x80101010);
        x = Text_CenterX(gPriceInfoTexts[PRICE_INFO_TITLE], (s16)(x + 0xAA), 0x16);
        Font_SetPos(x, y + 0x1D);
        Text_PrintfColored(gPriceStrFmt, gPriceInfoTexts[PRICE_INFO_TITLE]);
        return 0xFFE0D8C8;
    default:
        i = n - 1;
        wanted = &menu->wanted[season][i];
        if (Wanted_IsSet(&wanted->kind) == 0 && Wanted_IsSet(&(wanted + WANTED_SECOND_HUB)->kind) == 0) {
            PriceInfo_DrawWanted((s16)(x + 0xAA), (s16)(y + 0x1D), i, 0x80404040);
            return 0xFF808080;
        }
        PriceInfo_DrawWanted((s16)(x + 0xAA), (s16)(y + 0x1D), i, gPriceBoardColors[i]);
        return 0xFFE0D8C8;
    }
}

/*
 * The board's four lines, one per window of win: the frame and label
 * (PriceInfo_DrawTitle), 1 to 3 icons, and blinking arrows on the chosen
 * line.
 */
void PriceInfo_DrawCategories(WinSlide *win, s16 y)
{
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s16 i;
    s16 blink;
    s16 iconY;
    s16 x;

    for (i = 0; i < BOARD_LINES; i++) {
        x = win->pos;
        Icon_DrawMode(x, y, 0x154, 0x50, PriceInfo_DrawTitle(x, y, i), 0, i);
        iconY = y + 0xA;
        switch (i) {
        case 3:
            Icon_Draw(x + 0x8C, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
            break;
        case 2:
            Icon_Draw(x + 0x5A, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
            Icon_Draw(x + 0xBE, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
            break;
        default:
            Icon_Draw(x + 0x28, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
            Icon_Draw(x + 0xF0, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
            Icon_Draw(x + 0x8C, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
            break;
        }
        if (i == menu->sel) {
            if (gFrameTick & 0x10) {
                blink = 10;
            } else {
                blink = 0;
            }
            Icon_Draw(x + 0xA + blink, y + 0x18, 0x20, 0x20, 0xFFA0A040, 0x29);
            Icon_Draw(x + 0x12A - blink, y + 0x18, 0x20, 0x20, 0xFFA0A040, 0x28);
        }
        y += 0x5A;
        win++;
    }
}

/*
 * The rate bar of monster id on a list line at x, y: its length and colour
 * by the level (low 3 bits of gLobbyRateStates[id]: red, green or blue),
 * and a rise or fall mark from the next bits.
 */
void PriceInfo_DrawRate(s16 x, s16 y, s16 id)
{
    u8 state;
    s32 level;
    u8 marks;
    u32 colour;
    u16 flip;

    state = gLobbyRateStates[id];
    marks = state >> 3;
    level = state & 7;
    if (level < 2) {
        colour = 0xFF802000;
    } else if (level < 4) {
        colour = 0xFF387846;
    } else {
        colour = 0xFF4444A0;
    }
    if (id & 2) {
        flip = 0x8000;
    } else {
        flip = 0;
    }
    Icon_DrawRotated(x, y + 0xC, (level + 1) * 0x18, 8, colour, flip, 0x6F);
    if (marks & 1) {
        Icon_DrawMode(x + 0xE6, y - 1, 0x1E, 0x16, 0xFF4444A0, 0x1D, 2);
    } else if (marks & 2) {
        Icon_DrawMode(x + 0xE6, y + 2, 0x1E, 0x16, 0xFF802020, 0x1E, 3);
    }
}

/*
 * The board's legend at x, y: three texts (gPriceLegendTexts) and three
 * colour bars (gPriceLegendColors).
 */
void PriceInfo_DrawLegend(s16 x, s16 y)
{
    Colours3 cols;
    s32 i;
    s32 lineY;

    cols = gPriceLegendColors;
    Font_SetPos(x - 4, y);
    Text_PrintfColored(gPriceLegendTexts[0]);
    lineY = y;
    Font_SetPos(x - 4, lineY + 0x17);
    Text_PrintfColored(gPriceLegendTexts[1]);
    Text_PrintfColored(gPriceLegendTexts[2]);
    x -= 10;
    for (i = 0; i < 3; i++) {
        Icon_Draw(x, lineY + 0x24, 0x3C, 0xA, cols.c[i], 0x6E);
        x += 0x36;
    }
}

/*
 * The board's list window at x, y: page listPage's lines (pages 0-2 have
 * the line counts of gPricePageCounts / gPricePageCountsOnline, later
 * pages 11 each), each a monster's name and price with its rate bar, or
 * the unknown-name text. Page 2 lists the fixed-price monsters
 * (gFixedPriceIds and gFixedPrices offline, gOnlinePriceIds online), named
 * once hunted. It needs optimization level 4 with PriceInfo_DrawRate
 * compiled from C for the game's registers.
 * It matches only with PriceInfo_DrawRate compiled from C in the same
 * file: at optimization level 4 the compiler uses what it knows of a
 * callee's registers.
 */
void PriceInfo_DrawList(s16 x, s16 y)
{
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s16 i;
    s16 cnt;
    s16 start;
    u8 *counts;
    s16 em;
    s16 lineIdx;
    s32 price;

    Menu_DrawQuestSheet(x - 0x14, y, 0x140, 0x168, 0, menu->listPage, -1, -1);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum((s16)(x - 0x14) + 0xFA, y + 2, menu->listPage, menu->listPages, 1);
    Tex_Bind(0x16B);
    Font_SetSpacing(0x14, 0x14);
    i = y + 0x24;
    PriceInfo_DrawLegend((s16)(x + 0x14), i);
    y += 0x42;
    if (menu->listPage > 2) {
        cnt = PRICE_PAGE_LINES;
        start = menu->listPage * PRICE_PAGE_LINES;
    } else {
        if (!Game_IsOnline()) {
            counts = gPricePageCounts;
        } else {
            counts = gPricePageCountsOnline;
        }
        start = 0;
        cnt = counts[menu->listPage];
        for (i = 0; i < menu->listPage; i++) {
            start += *counts++;
        }
    }
    for (i = 0; i < cnt; i++) {
        lineIdx = start + i;
        if (lineIdx >= PRICE_LIST_MAX) {
            return;
        }
        if (menu->listPage == 2) {
            if (!Game_IsOnline()) {
                price = gFixedPrices[i];
                em = gFixedPriceIds[i];
            } else {
                em = gOnlinePriceIds[i];
                price = gRateSnapshot[em];
            }
            y += 0x17;
            Font_SetPos(x + 0x14, y);
            if (gSave.hunted[em] > 0) {
                Text_PrintfColored("%-16s:%4dz", gMonsterNames[em], price);
            } else {
                Text_PrintfColored(gPriceStrFmt, gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME]);
            }
        } else {
            em = menu->ids[lineIdx];
            price = gRateSnapshot[abs(em)];
            if (em != 0 && price != 0) {
                y += 0x17;
                Font_SetPos(x + 0x14, y);
                if (0 < em) {
                    Text_PrintfColored("%-16s:%4dz", gMonsterNames[em], gRateSnapshot[em]);
                    PriceInfo_DrawRate(x + 0x14, y, em);
                } else {
                    Text_PrintfColored(gPriceStrFmt, gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME]);
                }
            }
        }
    }
}

extern char *gItemNames[];
extern char *gMonsterClassTexts[];
s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
void Text_SetColor(s16 color);
s32 Lobby_HrAtLeast31(void);
void Menu_DrawBarRotated(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s16 cx, s16 cy, u8 k);
void Menu_DrawFrameBox(s16 x, s16 y, s16 w, s16 h, u32 color);
void Tex_Prepare(s32 n, s32 tex);
s32 WantedPicture_GetUV(s32 *uv0, s32 *uv1, u16 id);
s32 Sprite_Draw(Sprite *s);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
/*
 * A wanted entry's detail card at x, y for wanted line line: the frame
 * (lighter for the second list), the icons, the line's title, the hall
 * header, the target's name (an item, or a monster with its class on line
 * 0), the count, its picture and the reward. For the second list below
 * hunter rank 31 the target, count, picture and reward are hidden behind
 * the unknown-name text.
 */
void PriceInfo_DrawDetails(s16 x, s16 y, void *wantedIn, u8 listIn, s8 line)
{
    WantedRec *wanted = wantedIn;
    Sprite spr;
    char buf[0x40];
    char wbuf[0x20];
    s16 centerX;
    u32 *col;
    s16 textX;
    s32 list = listIn;
    s16 iconY;
    char *text;
    BoardMenu *menu = (BoardMenu *)gMenuWork;

    if (list == 1) {
        Menu_DrawBarRotated(x, y, 0x118, 0x13E, 0xFFCFBDBD, 0, 0, 0, wanted->id & 3);
    } else {
        Menu_DrawBarRotated(x, y, 0x118, 0x13E, 0xFFE0D8C8, 0, 0, 0, wanted->id & 3);
    }
    iconY = y + 0x4A;
    switch (menu->sel) {
    case 3:
        Icon_Draw(x + 0x6E, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
        break;
    case 2:
        Icon_Draw(x + 0x3C, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
        Icon_Draw(x + 0xA0, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
        break;
    default:
        Icon_Draw(x + 0x28, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
        Icon_Draw(x + 0xB4, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
        Icon_Draw(x + 0x6E, iconY, 0x3C, 0x3C, 0x40000000, 0x1F);
        break;
    }
    centerX = x + 0x8C;
    Font_SetSpacing(0x16, 0x16);
    col = &gPriceBoardColors[line];
    Font_SetColor(*col);
    PriceInfo_DrawWanted(centerX, y += 0x14, line, *col);
    Icon_Draw(x + 0x14, y + 0xF, 0xF0, 0xC, 0x99106060, wanted->id % 6 + 0x6E);
    if (Game_IsOnline() == 1) {
        text = gPriceInfoTexts[PRICE_INFO_HALL_TAVERN + list];
    } else {
        text = gPriceInfoTexts[PRICE_INFO_HALL_WANTED];
    }
    textX = Text_CenterX(text, centerX, 0x16);
    Font_SetPos(textX, y += 0x23);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    Text_PrintfColored(text);
    Font_SetSpacing(0x14, 0x14);
    if (wanted->kind == WANTED_KIND_ITEM) {
        text = gItemNames[wanted->id];
    } else {
        if (line == 0) {
            sprintf(buf, "%s%s", gMonsterClassTexts[wanted->id], gMonsterNames[wanted->id]);
        } else {
            sprintf(buf, gPriceStrFmt, gMonsterNames[wanted->id]);
        }
        text = buf;
    }
    if (Lobby_HrAtLeast31() == 0 && list == 1) {
        text = gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME];
    }
    textX = Text_CenterX(text, centerX, 0x14);
    Font_SetPos(textX, y += 0x22);
    Text_PrintfColored(text);
    sprintf(buf, gPriceInfoTexts[PRICE_INFO_NONE + wanted->kind], wanted->count);
    Sjis_Widen(buf, wbuf);
    textX = Text_CenterX(wbuf, centerX, 0x14);
    Font_SetPos(textX, y += 0x19);
    if (Lobby_HrAtLeast31() == 0 && list == 1) {
        sprintf(wbuf, gPriceStrFmt, gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME]);
    }
    Text_PrintfColored(wbuf);
    Font_SetSpacing(0x16, 0x16);
    spr.y = y + 0x21;
    spr.w = 0x60;
    spr.h = 0x60;
    spr.color = -1;
    if (wanted->kind == WANTED_KIND_ITEM) {
        spr.x = centerX - 60.0f;
        Menu_DrawFrameBox(spr.x - 0xA, spr.y - 5, 0x8C, 0x6A, 0xFF493120);
        if (Lobby_HrAtLeast31() == 0 && list == 1) {
            spr.x = centerX - 0x30;
            Menu_DrawFrameBox(spr.x - 0xA, spr.y - 5, 0x74, 0x6A, 0xFF493120);
            Tex_Prepare(1, TEX_SLOT_PICTURES);
            Tex_Bind(TEX_SLOT_PICTURES);
            WantedPicture_GetUV((s32 *)&spr.u0, (s32 *)&spr.u1, 0);
            Sprite_Draw(&spr);
        } else {
            Item_DrawIcon(spr.x, spr.y, spr.w, wanted->id);
        }
    } else {
        spr.x = centerX - 0x30;
        Menu_DrawFrameBox(spr.x - 0xA, spr.y - 5, 0x74, 0x6A, 0xFF493120);
        Tex_Prepare(1, TEX_SLOT_PICTURES);
        Tex_Bind(TEX_SLOT_PICTURES);
        if (Lobby_HrAtLeast31() == 0 && list == 1) {
            WantedPicture_GetUV((s32 *)&spr.u0, (s32 *)&spr.u1, 0);
        } else {
            WantedPicture_GetUV((s32 *)&spr.u0, (s32 *)&spr.u1, wanted->id);
        }
        Sprite_Draw(&spr);
    }
    if (Lobby_HrAtLeast31() == 0 && list == 1) {
        sprintf(buf, "%s%sz", gQuestSheetTexts[QUEST_SHEET_TEXT_REWARD], gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME] + 4);
    } else {
        sprintf(buf, "%s%dz", gQuestSheetTexts[QUEST_SHEET_TEXT_REWARD], wanted->reward);
    }
    Sjis_Widen(buf, wbuf);
    centerX = Text_CenterX(wbuf, centerX, 0x16);
    Text_SetColor(1);
    y += 0x91;
    Font_SetPos(centerX + 3, y + 2);
    Text_PrintfColored(wbuf);
    Font_SetColor(*col);
    Font_SetPos(centerX, y);
    Text_PrintfColored(wbuf);
    Icon_Draw(centerX, y + 0xF, 0xD2, 0xC, 0x99106060, (wanted->id + 1) % 6 + 0x6E);
}

/*
 * The board's first window: the title bar (gPriceBoardTitle) and money; on
 * pages 0, 1 and 5 the four lines (PriceInfo_DrawCategories); on pages 2
 * to 4 the price list (PriceInfo_DrawList) for line 0, or the chosen
 * wanted line's first entry (PriceInfo_DrawDetails).
 */
void PriceInfo_Draw(TextBox *box)
{
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s32 season;
    s16 x;

    season = LobbyWorld_GetSeason();
    Text_SetSize(box->size);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, gGameMenuLabels[MENU_LABEL_PRICE_INFO], 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    x = menu->win[0].pos;
    switch (menu->page) {
    case 0:
    case 1:
    case 5:
        PriceInfo_DrawCategories(menu->s, 0x36);
        break;
    case 2:
    case 3:
    case 4:
        if (menu->sel == 0) {
            PriceInfo_DrawList(x, 0x3A);
        } else {
            PriceInfo_DrawDetails(x, 0x46, &menu->wanted[season][(s16)(menu->sel - 1)], 0, menu->sel - 1);
        }
        break;
    }
}

/*
 * The board's second window: on pages 2 to 4, for a wanted line, its
 * second entry (PriceInfo_DrawDetails) when both of the line's entries are
 * set, at the second window's x (the first window's x on page 2).
 */
void PriceInfo_Draw2(TextBox *box)
{
    WantedRec *wanted;
    BoardMenu *menu = (BoardMenu *)gMenuWork;
    s16 x;
    s32 season;
    s16 line;
    WantedRec *wanted2;

    season = LobbyWorld_GetSeason();
    Text_SetSize(box->size);
    x = menu->win[1].pos;
    switch (menu->page) {
    case 2:
        x = menu->win[0].pos;
    case 3:
    case 4:
        if (menu->sel != 0) {
            line = menu->sel - 1;
            wanted = &menu->wanted[season][line];
            wanted2 = wanted + WANTED_SECOND_HUB;
            if (Wanted_IsSet(&wanted2->kind) == 1 && Wanted_IsSet(&wanted->kind) == 1) {
                PriceInfo_DrawDetails(x, 0x46, wanted2, 1, line);
            }
        }
        break;
    }
}


/* The price legend's colours. */
Colours3 gPriceLegendColors = {
#include "lobby/007EC968.inc"
};
