#include "common.h"
#include "m2c_macros.h"
#include "em.h"
#include "lobby_globals.h"
#pragma optimization_level 4

void *memset(void *dst, s32 c, u32 n);
/* unit's own declaration: Offer_IndexOf reads a0 = unused0, the u8 id Offer_GetQuest and Offer_FindActive were given (passing it adds a word each); a (u16 id) prototype changes Offer_AddPoints by 193 words */
s32 Offer_IndexOf();
#include "save.h"
#include "quest.h"
#include "lobby_state.h"

/* 16 records of 0x14 bytes at gSave + 0x299C */

/* 0x1BA bytes per entry, three titles and three texts each */
typedef struct Offer {
    s16 unk0[0x2D];
    char title[3][0x1D];        /* 0x5A */
    char text[3][0x58];         /* 0xB1 */
    u8 pad1B9;
} Offer;
extern Offer gOfferTable[];

extern LobbyMenu *gLobbyMenu;
extern u8 gMailBlockList[];
extern u8 gAskUserId[];
s32 Menu_YesNoInput(void);
s32 IdList_Find(s8 *arg0, s32 arg1, void *key);
struct Rec18;
s32 IdList_Remove(struct Rec18 *r, u8 idx, u8 n);
void LobbyText_Open(s32);
void LobbySe_Play(s32);
s32 BlockList_RemoveInput(u16 pad)
{
    if (pad & 0x8000) {
        return 4;
    }
    gLobbyMenu->redraw = 1;
    switch (gLobbyMenu->subStep) {
    case 0:
        switch (Menu_YesNoInput()) {
        case 0:
            gLobbyMenu->subStep++;
            IdList_Remove((struct Rec18 *) gMailBlockList, IdList_Find((s8 *) gMailBlockList, 8, gAskUserId), 8);
            LobbyText_Open(0x39);
            break;
        case 3:
            gLobbyMenu->subStep = 2;
            LobbyText_Open(0x38);
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            gLobbyMenu->subStep = 0;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(0);
            gFieldControl = 1;
            return 0;
        }
        break;
    case 2:
        if (pad & 0x20) {
            gLobbyMenu->subStep = 0;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(0);
            gFieldControl = 1;
            return 3;
        }
        break;
    }
    return 2;
}

typedef struct Rec7E9410 {
    s8 kind;
    u8 unk1[5];
    u16 id;
} Rec7E9410;
extern Rec7E9410 gOfferNpcs[];
s32 Cond_Check(u16 id);
u8 Village_FacilityLevel(s32 k);
s32 LobbyWorld_GetSeasonDay(void);
u16 Rand_Next(s32 stream);
s32 Offer_Count(void);
s8 Offer_PickVariant(u16 id);
u16 Offer_PickNpc(u16 seed, s8 kind);
s32 Bits_TakeOf9(u16 *arg0, u16 arg1);
u16 Offer_PickRandomItem(u16 seed);
extern s8 gOfferDayRolls[];
extern s8 D_007E9489[];
extern s8 gOfferCountRolls3[];
extern s8 gOfferCountRolls4[];

/* Offers new requests (Cond_Check(7), the facility levels, random draws
 * into the save records at gSave + 0x299C); the count is kept in i before
 * i counts the new offers. */
void Offer_Generate(void)
{
    s16 *q;
    OfferRec *rec;
    s8 have;
    s16 sum;
    u16 tmp;
    s8 j;
    u16 sel;
    s8 *tbl;
    s8 kind;
    s8 m;
    s32 id;
    u16 cnt2;
    s8 k;
    s32 ofs;
    s8 i;
    s8 c;
    s8 *e;
    s8 cnt;
    s32 r;

    if (Cond_Check(7) == 0) {
        return;
    }
    if ((s8)((u8)Village_FacilityLevel(1) + (u8)Village_FacilityLevel(2) + (u8)Village_FacilityLevel(3)) >= 2) {
        i = 4;
        tbl = gOfferCountRolls4;
    } else {
        i = 3;
        tbl = gOfferCountRolls3;
    }
    have = Offer_Count();
    if (have >= i) {
        return;
    }
    r = LobbyWorld_GetSeasonDay();
    c = gOfferDayRolls[r * 2];
    k = D_007E9489[r * 2];
    if (Rand_Next(1) % 100 > c) {
        return;
    }
    for (j = 0; j < i; j++) {
        c = tbl[j * 2];
        if (Rand_Next(1) % 100 <= c) {
            cnt = tbl[j * 2 + 1];
            break;
        }
    }
    /* the game reads a register it never set: cnt is s3, still the caller's, when no roll succeeds */
    if (cnt + have > i) {
        cnt = i - have;
    }
    for (i = 0; i < cnt; i++) {
        kind = Offer_PickVariant(Rand_Next(1));
        id = Offer_PickNpc(Rand_Next(1), kind);
        if (id == 0xFFFF) {
            continue;
        }
        if (k == 0) {
            rec = (OfferRec *)((u8 *)&gSave + id * 0x14 + 0x299C);
            memset(rec, 0, 0x14);
            e = (s8 *)&gOfferNpcs[id];
            if (Cond_Check(0x15) == 1 && (Rand_Next(1) & 1)) {
                sel = e[(Rand_Next(1) & 1) + 3];
            } else {
                sel = e[(Rand_Next(1) & 1) + 1];
            }
            cnt2 = Rand_Next(1) % 3 + 1;
            tmp = 0;
            sum = 0;
            for (m = 0; m < cnt2; m++) {
                ofs = (s16)Bits_TakeOf9(&tmp, Rand_Next(1)) * 5;
                ((u16 *)rec)[m * 2] = *(u16 *)&gOfferTable[sel].unk0[ofs];
                q = &gOfferTable[sel].unk0[ofs] + (Rand_Next(1) & 1) * 2;
                ((u16 *)rec)[m * 2 + 1] = q[1];
                sum += q[2];
            }
            rec->unk10 = sel;
            rec->unkC = Rand_Next(1) % 3;
            *(s16 *)rec->unkE = sum;
            rec->unkD = 1;
        } else {
            *(u16 *)((u8 *)&gSave + 0x2ADC + (u16)id * 2) = Offer_PickRandomItem(Rand_Next(1));
        }
        k ^= 1;
        gSave.unk2CAB |= 1 << kind;
    }
}

extern u16 *gQuestIdLists[6];
s32 Quest_IsOffered(u16 id, s32 a, s32 b);
s32 ElderForecast_HasQuest(u16 id);
s32 QuestId_IsStandard(s32 id);
u16 Offer_PickRandomItem(u16 seed)
{
    s32 k;
    s32 n;
    u16 *p;
    u16 buf[100];

    memset(buf, 0, sizeof(buf));
    n = 0;
    for (k = 0; k < 6; k++) {
        for (p = gQuestIdLists[k]; *p != 0; p++) {
            if (Quest_IsOffered(*p, 0, 0) == 1 && ElderForecast_HasQuest(*p) == 0 && QuestId_IsStandard(*p) == 1) {
                buf[n] = *p;
                n++;
            }
            if (n >= 100) {
                break;
            }
        }
        if (n >= 100) {
            break;
        }
    }
    return buf[seed % n];
}

void Offer_ClearAll(void)
{
    s32 i;

    memset(&gSave.offers, 0, 0x140);
    memset(&gSave.offerQuests, 0, 0x20);
    for (i = 0; i < 20; i++) {
        gEms[i].unk43C = 0;
    }
}

s32 Offer_IndexOf(id)
    u16 id;
{
    u16 k = gOfferNpcs[0].id;
    Rec7E9410 *p = gOfferNpcs;
    s32 i = 0;

    while (k != 0) {
        if (k == id) {
            return i;
        }
        p++;
        k = p->id;
        i++;
    }
    return -1;
}

u16 Offer_GetQuest(u8 unused0)
{
    s32 idx;
    u16 ret;

    idx = Offer_IndexOf();
    if (idx != -1) {
        ret = *(M2C_UNK *)((u8 *)&gSave.offerQuests + (idx * 2));
        if (ret != 0) {
            return ret;
        }
    }
    return 0xFFFFU;
}

s32 Offer_FindActive(u8 unused0)
{
    s32 ret;

    ret = Offer_IndexOf();
    /* gSave.offers[temp_v0].unkD: the field form changes the address arithmetic */
    if ((ret != -1) && (M2C_FIELD(((u8 *)&gSave + (ret * 0x14)), s8 *, 0x29A9) != 0)) {
        return ret;
    }
    return -1;
}

char *Request_Title(s16 i)
{
    return gOfferTable[gSave.offers[i].unk10].title[gSave.offers[i].unkC];
}

char *Request_Text(s16 i)
{
    return gOfferTable[gSave.offers[i].unk10].text[gSave.offers[i].unkC];
}

s32 Offer_IndexOf2(u16 id)
{
    s32 i = 0;
    Rec7E9410 *p = gOfferNpcs;
    u16 k = gOfferNpcs[0].id;

    while (k != 0) {
        if (k == id) {
            return i;
        }
        p++;
        k = p->id;
        i++;
    }
    return -1;
}

s32 Bits_TakeOf9(u16 *tmp, u16 n)
{
    s32 bit;
    u16 bits;

    bits = *tmp;
    bit = ((s32) (n & 0xFFFF) % 9) & 0xFFFF;
    if (bits & (1 << bit)) {
        bit = ((s32) (bit + 2) % 9) & 0xFFFF;
        if (bits & (1 << bit)) {
            bit = ((s32) (bit + 2) % 9) & 0xFFFF;
        }
    }
    *tmp = bits | (1 << (bit & 0xFFFF));
    return (s16)(bit);
}

s32 Npc_IsPresent(u8 kind, s32 id, u8 force);
u16 Offer_PickNpc(u16 seed, s8 kind)
{
    Rec7E9410 *p = gOfferNpcs;
    s32 i = 0;
    u16 k = gOfferNpcs[0].id;
    s32 n = 0;
    u16 buf[16];

    while (k != 0) {
        if (p->kind == kind && Npc_IsPresent(k, 0x89, 1) == 1) {
            buf[n] = i;
            n++;
        }
        p++;
        k = p->id;
        i++;
    }
    if (n == 0) {
        return 0xFFFF;
    }
    return buf[seed % n];
}

s8 Offer_PickVariant(u16 id)
{
    s32 i;
    s32 n;
    u8 bits;
    s8 k;

    if (gSave.unk2CAB == 7) {
        gSave.unk2CAB = 0;
    }
    bits = gSave.unk2CAB;
    n = id;
    for (i = 0; i < 3; i++) {
        k = n % 3;
        if (!(bits & (1 << k))) {
            return k;
        }
        n++;
    }
    return 0;
}

s32 Offer_Count(void)
{
    u8 *offer;
    u8 *quest;
    s32 i;
    s32 j;
    s32 ret;

    ret = 0;
    offer = (u8 *)gSave.offers;
    i = 0;
    do {
        if (*(s8 *)((u8 *)offer + 0xD) != 0) {
            ret += 1;
        }
        i += 1;
        offer += 0x14;
    } while (i < 0x10);
    j = 0;
    quest = (u8 *)&gSave;
    do {
        if (*(u16 *)(quest + 0x2ADC) != 0) {
            ret += 1;
        }
        j += 1;
        quest += 2;
    } while (j < 0x10);
    return ret;
}

void Offer_ClearSaved(s32 unused0)
{
    s32 idx;

    idx = Offer_IndexOf(unused0);
    if (idx != -1) {
        memset((u8 *)&gSave + (idx * 0x14) + 0x299C, 0, 0x14);
    }
}

/* gSave.offerQuests as its own symbol: &gSave.offerQuests[i] folds 0x2ADC
 * into the loads. */

extern s32 D_007EB29C[];
QuestInfo *Quest_GetDiscInfo(u16 id);
void Offer_AddPoints(s32 id, s32 v);
Em *Npc_Find(u8 id);
/* gSave.offerQuests: the field form shares the base address differently. */
void Offer_Complete(s32 id)
{
    s32 i = Offer_IndexOf(id);
    u16 *q;
    s32 stars;
    s32 v;
    Em *e;
    QuestInfo *info;

    if (i != -1) {
        q = (u16 *)gSave.offerQuests + i;
        if (*q != 0) {
            info = Quest_GetDiscInfo(*q);
            stars = info->stars;
        } else {
            stars = 4;
        }
        if (stars < 5 && stars > 0) {
            v = D_007EB29C[stars];
        } else {
            v = 0;
        }
        Offer_AddPoints(id, v);
        *q = 0;
        e = Npc_Find(id);
        if (e != NULL) {
            e->unk43C = 0;
        }
    }
}

typedef struct Milestone {
    u32 need;
    u32 unk4;
    s32 flag;
    s32 done;
} Milestone;
extern Milestone gOfferMilestonesA[];
extern Milestone gOfferMilestonesB[];
void *memcpy(void *dst, const void *src, u32 n);
s32 Cond_Check(u16 id);
s32 Save_TestEventFlag(s32 flag);
void Save_SetEventFlag(s32 flag);
void Offer_CheckPointFlags(u32 before, u32 after, s32 idx);
void Village_CheckMilestone(u32 before, u32 after, s32 idx);
void Offer_AddPoints(s32 id, s32 v)
{
    u32 kind[4];
    u32 offer[16];
    SaveData *s;
    Rec7E9410 *r;
    s32 i;
    Milestone *tbl = NULL;
    s32 j;

    i = Offer_IndexOf(id);
    if (i == -1) {
        return;
    }
    memcpy(kind, gSave.offerKindPoints, sizeof(kind));
    memcpy(offer, gSave.offerPoints, sizeof(offer));
    gSave.offerPoints[i] += v;
    r = &gOfferNpcs[i];
    gSave.offerKindPoints[r->kind] += v;
    switch (r->kind) {
    case 0:
        break;
    case 1:
        tbl = gOfferMilestonesA;
        break;
    case 2:
        tbl = gOfferMilestonesB;
        break;
    }
    if (Cond_Check(10) == 1 && tbl != NULL) {
        for (j = 0; j < 5; j++) {
            if (!Save_TestEventFlag(tbl[j].done)) {
                if (!Save_TestEventFlag(tbl[j].flag)) {
                    gSave.offerMilePoints[r->kind] += v;
                    if (gSave.offerMilePoints[r->kind] >= tbl[j].need) {
                        Save_SetEventFlag(tbl[j].flag);
                    }
                }
                break;
            }
        }
    }
    gSave.offerKindPoints[3] += v;
    gSave.offerMilePoints[3] += v;
    Village_CheckMilestone(kind[2], gSave.offerKindPoints[2], 0);
    Village_CheckMilestone(kind[3], gSave.offerKindPoints[3], 1);
    Village_CheckMilestone(kind[3], gSave.offerKindPoints[3], 2);
    Village_CheckMilestone(kind[3], gSave.offerKindPoints[3], 3);
    Village_CheckMilestone(kind[1], gSave.offerKindPoints[1], 4);
    Village_CheckMilestone(kind[3], gSave.offerKindPoints[3], 5);
    Village_CheckMilestone(kind[3], gSave.offerKindPoints[3], 6);
    s = &gSave;
    for (j = 0; j < 16; j++) {
        Offer_CheckPointFlags(offer[j], s->offerPoints[j], j);
        if (s->offerPoints[j] > 2000) {
            s->offerPoints[j] = 0;
        }
    }
    s = &gSave;
    for (j = 0; j < 4; j++) {
        if (s->offerMilePoints[j] > 2000) {
            s->offerMilePoints[j] = 0;
        }
        if (s->offerKindPoints[j] > 100000) {
            s->offerKindPoints[j] = 0;
        }
    }
}

void VillageBoard_MarkRead(s16 n)
{
    u16 bit = n % 16;
    u16 mask = 1LL << bit;
    u16 idx = n / 16;

    gSave.unk2D0C[idx] |= mask;
}

s32 VillageBoard_IsRead(u16 n)
{
    s32 w = gSave.unk2D0C[(u16)(n / 16)];

    return (w & (1LL << (u16)(n % 16))) != 0;
}

/* unk_6E7100's .data (0x7E9410-0x7EB2C0) */
Rec7E9410 gOfferNpcs[15] = {
#include "lobby/007E9410.inc"
};
s8 gOfferDayRolls[8] = {
#include "lobby/007E9488.inc"
};
s8 gOfferCountRolls3[8] = {
#include "lobby/007E9490.inc"
};
s8 gOfferCountRolls4[8] = {
#include "lobby/007E9498.inc"
};
Milestone gOfferMilestonesA[5] = {
#include "lobby/007E94A0.inc"
};
Milestone gOfferMilestonesB[5] = {
#include "lobby/007E94F0.inc"
};
Offer gOfferTable[17] = {
#include "lobby/007E9540.inc"
};
u32 D_007EB2A0[8] = {
#include "lobby/007EB2A0.inc"
};
