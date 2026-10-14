/*
 * Monster helpers: the status effects (begin, end, thresholds), per-kind
 * parameter lookups, lookups keyed by the monster's +0x680, and a few
 * resets and clamped counters. See include/em.h.
 */
#include "em.h"

typedef struct EmIdEntry {
    s16 id;             /* 0x0: >= 10000: exact +0x680 value + 10000; -1 ends the table */
    s16 pad;
    f32 *value;         /* 0x4: the area's position table (float triples) */
} EmIdEntry;

typedef struct EmIdEntryU {
    u16 id;             /* 0x0: 0xFF ends the table */
    u16 pad;
    s16 *value;         /* 0x4: the area list (Em_IdSetLookup, Em_IdSetHas) */
} EmIdEntryU;

typedef struct EmIdSet {
    s32 key;            /* 0x0: matched with Em.unk964; -1 is the default */
    EmIdEntryU *entries;
} EmIdSet;

extern EmRageRow *gEmRageTbl[];
extern EmStatusParam *gEmStatus1Params[];
extern EmStatusParam *gEmStatusParams[];
extern EmStatusParam *gEmStatus4Params[];
extern EmStatusParam *gEmStatus2Params[];
extern EmStatusParam *gEmStatus5Params[];
extern u16 gEmCollMasks[];
extern u8 gEmIsLarge[];
extern s16 *gEmRequestIds[];
extern u8 gEmIsLarge2[];
extern EmIdSet **gEmIdSets[];
extern u8 gEmStartsBattle[];
extern f32 gEffect06Scales[];

u16 Area_DayId(u16 id);
void Effect06_SpawnAtBone(Em *em, s32 a, s32 b, u8 c, f32 f); /* unit's own declaration: with the definition's void (Player *p, s16 type, u8 sub, s16 bone, f32 scale) (sub_main/unk_3CA790.c), Em_SpawnEffect06Bone 11 of 23 words differ */
void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale);
void Em_AddPoison(Em *em, s32 arg);
void Em_AddStatus4(Em *em, s32 arg);
void Em_AddStatus2(Em *em, s32 arg);
void Em_EndStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);

void Em_InitRage(Em *em)
{
    EmRageRow *p = gEmRageTbl[em->type];

    if (p != NULL) {
        em->rageThreshold = p->threshold;
    } else {
        em->rageThreshold = -1;
    }
    em->rageBuildup = 0;
    em->rageTimer = 0;
    em->rage = 0;
    em->rageStep = 0;
    em->ragePending = 0;
}

void Em_InitStatusThresholds(Em *em)
{
    EmStatusParam *s1 = gEmStatus1Params[em->type];
    EmStatusParam *s3 = gEmStatusParams[em->type];
    EmStatusParam *s4 = gEmStatus4Params[em->type];
    EmStatusParam *s2 = gEmStatus2Params[em->type];
    EmStatusParam *s5 = gEmStatus5Params[em->type];

    em->status1Threshold = s1->threshold;
    em->status2Threshold = s2->threshold;
    em->poisonThreshold = s3->threshold;
    em->status4Threshold = s4->threshold;
    em->status5Threshold = s5->threshold;
}

void Em_BeginStatus2(Em *em)
{
    s32 i;

    if (em->statusFlags & 8) {
        Em_EndStatus4(em);
    }
    if (em->statusFlags & 1) {
        Em_EndStatus1(em);
    }
    if (em->statusFlags & 0x10) {
        Em_EndStatus5(em);
    }
    for (i = 0; i < gScene.playerCount; i++) {
        em->noticeCount[i] = 0;
        em->alertBuildup[i] = 0;
    }
    em->need3 = 0;
    em->status2Buildup = 0;
    em->status2Decay = 0;
    em->need3Gauge = em->need3Max;
    em->hitReactMode = 2;
    em->status = 2;
    em->statusFlags |= 2;
    em->noReact = 1;
    Em_SetCombat(em, 0, 0);
    em->guardTimer = 30;
}

void Em_BeginStatus1(Em *em)
{
    s32 i;

    if (em->statusFlags & 8) {
        Em_EndStatus4(em);
    }
    if (em->statusFlags & 2) {
        Em_EndStatus2(em);
    }
    if (em->statusFlags & 0x10) {
        Em_EndStatus5(em);
    }
    for (i = 0; i < gScene.playerCount; i++) {
        em->noticeCount[i] = 0;
        em->alertBuildup[i] = 0;
    }
    em->need3 = 0;
    em->status1Buildup = 0;
    em->status1Decay = 0;
    em->need3Gauge = em->need3Max;
    em->hitReactMode = 2;
    em->status = 1;
    em->statusFlags |= 1;
    em->noReact = 1;
    Em_SetCombat(em, 0, 0);
}

void Em_EndStatus2(Em *em)
{
    EmStatusParam *param = gEmStatus2Params[em->type];

    em->hitReactMode = 0;
    em->statusFlags &= ~3;
    em->noReact = 0;
    em->need3 = 0;
    if ((em->status & 2) && em->status2Count < 4) {
        em->status2Count++;
        em->status2Threshold += param->increment;
    }
    em->status = 0;
}

void Em_EndStatus1(Em *em)
{
    EmStatusParam *param = gEmStatus1Params[em->type];

    em->hitReactMode = 0;
    em->statusFlags &= ~3;
    em->noReact = 0;
    em->need3 = 0;
    if ((em->status & 1) && em->status1Count < 4) {
        em->status1Count++;
        em->status1Threshold += param->increment;
    }
    em->status = 0;
}

void Em_BeginStatus4(Em *em)
{
    em->hitReactMode = 4;
    em->status = 4;
    em->statusFlags |= 8;
    em->noReact = 1;
}

void Em_EndStatus4(Em *em)
{
    EmStatusParam *param = gEmStatus4Params[em->type];

    em->status4Buildup = 0;
    em->status4Decay = 0;
    em->statusFlags &= ~8;
    em->noReact = 0;
    em->hitReactMode = 0;
    if ((em->status & 4) && em->status4Count < 4) {
        em->status4Count++;
        em->status4Threshold += param->increment;
    }
    em->status = 0;
}

void Em_EndAllStatus(Em *em)
{
    if (em->status == 1) {
        Em_EndStatus1(em);
    }
    if (em->status == 2) {
        Em_EndStatus2(em);
    }
    if (em->statusFlags & 8) {
        Em_EndStatus4(em);
    }
    if (em->statusFlags & 0x10) {
        Em_EndStatus5(em);
    }
}

void Em_EndStatus1And2(Em *em)
{
    if (em->status == 1) {
        Em_EndStatus1(em);
    }
    if (em->status == 2) {
        Em_EndStatus2(em);
    }
}

void Em_BeginStatus5(Em *em)
{
    em->hitReactMode = 4;
    em->status = 5;
    em->statusFlags |= 0x10;
    em->noReact = 1;
}

void Em_EndStatus5(Em *em)
{
    EmStatusParam *param = gEmStatus5Params[em->type];

    em->status5Buildup = 0;
    em->status5Decay = 0;
    em->statusFlags &= ~0x10;
    em->noReact = 0;
    em->hitReactMode = 0;
    if ((em->status & 5) && em->status5Count < 4) {
        em->status5Count++;
        em->status5Threshold += param->increment;
    }
    em->status = 0;
}

/* The entry for `id`: ids below 10000 match through Area_DayId. */
EmIdEntry *Em_FindAreaEntry(Em *em, EmIdEntry *entry, s16 id)
{
    s16 i;

    for (i = 0; i < 0xC5; i++, entry++) {
        if (entry->id == -1) {
            return entry;
        }
        if (entry->id >= 10000) {
            if (entry->id - 10000 == id) {
                return entry;
            }
        } else if (Area_DayId(entry->id) == Area_DayId(id)) {
            return entry;
        }
    }
    return NULL;
}

/* As Em_FindAreaEntry for the monster's +0x680, returning the value. */
f32 *Em_AreaEntryValue(Em *em, EmIdEntry *entry)
{
    s16 i;

    for (i = 0; i < 0xC5; i++, entry++) {
        if (entry->id == -1) {
            return entry->value;
        }
        if (entry->id >= 10000) {
            if (entry->id - 10000 == em->area) {
                return entry->value;
            }
        } else if (Area_DayId(entry->id) == Area_DayId(em->area)) {
            return entry->value;
        }
    }
    return 0;
}

void Em_InitCollMask(Em *em)
{
    em->collMask = gEmCollMasks[em->type];
}

/* Links some kinds to the first active monster of a companion kind. */
void Em_FindLink(Em *em)
{
    s8 i;
    Em *other = gEms;
    u8 kind;

    em->link = NULL;
    switch (em->type) {
    case 0x10:
    case 0x23:
        kind = 0x1B;
        break;
    case 0xD:
        kind = 0x1C;
        break;
    case 0x1E:
        kind = 0x1F;
        break;
    case 5:
        kind = 0x44;
        break;
    default:
        return;
    }
    for (i = 0; i < 20; i++, other++) {
        if (other->active != 0 && other->type == kind) {
            em->link = other;
            return;
        }
    }
}

void Em_ApplyActionStatus(Em *em, EmAction *action)
{
    switch (action->kind) {
    case 0:
        return;
    case 1:
        Em_AddPoison(em, action->arg);
        break;
    case 2:
        Em_AddStatus2(em, action->arg);
        break;
    case 3:
        Em_AddStatus4(em, action->arg);
        break;
    }
}

void Em_InitKindParams(Em *em)
{
    em->large = gEmIsLarge[em->type];
    em->showOnMap = gEmIsLarge2[em->type];
    em->startsBattle = gEmStartsBattle[em->type];
}

void Em_ClearLowHpNeed(Em *em)
{
    em->lowHpNeed = 0;
}

void Em_CheckKindRequest(Em *em)
{
    s32 found = 0;
    s16 *ids;

    if (em->requested == 1 && em->requestId == gEmKindInfo[em->type].lowHpRequest) {
        ids = gEmRequestIds[em->type];
        if (ids != NULL) {
            if (em->combat == 1) {
                while (*ids != -1) {
                    if (*ids >= 10000) {
                        if (*ids - 10000 == em->area) {
                            found = 1;
                        }
                    } else if (Area_DayId(*ids) == Area_DayId(em->area)) {
                        found = 1;
                    }
                    if (found) {
                        em->lowHpNeed = 0;
                        em->requested = 0;
                        em->requestId = 0;
                        em->requestPrio = 0;
                        return;
                    }
                    ids++;
                }
            }
        } else {
            em->lowHpNeed = 0;
            em->requested = 0;
            em->requestId = 0;
            em->requestPrio = 0;
        }
    }
}

void Em_Clear7F0State(Em *em)
{
    em->need1 = 0;
    em->need1State = 0;
    em->hitReactMode = 0;
}

void Em_Clear7ECState(Em *em)
{
    em->need2 = 0;
    em->need2State = 0;
    em->hitReactMode = 0;
}

void Em_Refill7F4(Em *em)
{
    em->need3Gauge = em->need3Max;
    em->need3 = 0;
    em->status2Buildup = 0;
    em->status2Decay = 0;
    em->status1Buildup = 0;
    em->status1Decay = 0;
    em->hitReactMode = 0;
}

void Em_ResetNotice(Em *em)
{
    s32 i;

    for (i = 0; i < gScene.playerCount; i++) {
        em->noticeCount[i] = 0;
        em->alertBuildup[i] = 0;
    }
    em->need3 = 0;
    em->status2Buildup = 0;
    em->status2Decay = 0;
    em->need3Gauge = em->need3Max;
    em->hitReactMode = 2;
    em->noReact = 1;
    Em_SetCombat(em, 0, 0);
}

/* On frames 0, 10 and 20 of every 90: Effect06_SpawnAt (kind 4) at the bone, offset by pos, scaled. */
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale)
{
    u16 t = gScene.frame % 90;

    if (t == 0 || t == 10 || t == 20) {
        Effect06_SpawnAt(em, 4, bone, pos, scale);
    }
}

/* Every 32 frames. */
void Em_SpawnEffect06Bone(Em *em, s32 a)
{
    if ((gScene.frame & 0x1F) == 0) {
        Effect06_SpawnAtBone(em, 5, 0, a, em->scaleX * gEffect06Scales[em->type]);
    }
}

s16 *Em_IdSetLookup(Em *em)
{
    EmIdSet **sets = gEmIdSets[em->type];
    EmIdSet *set;
    EmIdEntryU *entry;
    s16 i;

    if (sets == NULL) {
        return 0;
    }
    set = sets[gScene.map];
    for (;;) {
        if (set->key == -1 || set->key == em->questKey) {
            entry = set->entries;
            break;
        }
        set++;
    }
    for (i = 0; i < 0xC5; i++, entry++) {
        if (entry->id == 0xFF || Area_DayId(entry->id) == Area_DayId(em->area)) {
            return entry->value;
        }
    }
    return 0;
}

s16 *Em_IdSetHas(Em *em, u16 id)
{
    EmIdSet **sets = gEmIdSets[em->type];
    EmIdSet *set;
    EmIdEntryU *entry;
    s16 i;

    if (sets == NULL) {
        return 0;
    }
    set = sets[gScene.map];
    for (;;) {
        if (set->key == -1 || set->key == em->questKey) {
            entry = set->entries;
            break;
        }
        set++;
    }
    for (i = 0; i < 0xC5; i++, entry++) {
        if (entry->id == 0xFF || Area_DayId(entry->id) == Area_DayId(id)) {
            return entry->value;
        }
    }
    return 0;
}

void Em_Add7F0(Em *em, s32 n)
{
    em->need1Gauge += n;
    if (em->need1Gauge > em->need1Max) {
        em->need1Gauge = em->need1Max;
    }
}

void Em_Add7EC(Em *em, s32 n)
{
    em->need2Gauge += n;
    if (em->need2Gauge > em->need2Max) {
        em->need2Gauge = em->need2Max;
    }
}

void Em_Add7F4(Em *em, s32 n)
{
    em->need3Gauge += n;
    if (em->need3Gauge > em->need3Max) {
        em->need3Gauge = em->need3Max;
    }
}

void Em_RaiseTrigger80Lax(Em *em)
{
    if (em->remote == 0 && gScene.area == em->area) {
        if (em->enterDelay > 0) {
            if (--em->enterDelay <= 0) {
                em->enterDelay = 0;
            }
            return;
        }
        if (em->stance == 2 || em->stance == 4) {
            return;
        }
        if ((u32)(em->stateGroup - 5) <= 1 || em->stateGroup == 7) {
            return;
        }
        if (em->floorAttr != 0 && !(em->floorAttr & 0x8000)) {
            return;
        }
        if (Em_ScriptFlagsFrom(em, 0x80) == 0 && em->noAlert == 0 && em->noReact == 0) {
            em->scriptRestart = 1;
            em->triggers |= 0x80;
        }
    }
}

void Em_RaiseTrigger80(Em *em)
{
    if (em->remote == 0 && gScene.area == em->area) {
        if (em->enterDelay > 0) {
            if (--em->enterDelay <= 0) {
                em->enterDelay = 0;
            }
            return;
        }
        if (em->stance == 2 || em->stance == 4) {
            return;
        }
        if ((u32)(em->stateGroup - 5) <= 1 || em->stateGroup == 7) {
            return;
        }
        if (em->floorAttr != 0) {
            return;
        }
        if (Em_ScriptFlagsFrom(em, 0x80) == 0 && em->noAlert == 0 && em->noReact == 0) {
            em->scriptRestart = 1;
            em->triggers |= 0x80;
        }
    }
}

/* unk_7077A0's .data (0x8BCC98-0x8BF450) */
extern u32 D_008BC8F8[];
extern u32 D_008BC908[];
extern u32 D_008BC918[];
extern u32 D_008BC928[];
extern u32 D_008BC938[];
extern u32 D_008BC948[];
extern u32 D_008BC958[];
extern u32 D_008BC968[];
extern u32 D_008BC978[];
extern u32 D_008BC988[];
extern u32 D_008BC998[];
extern u32 D_008BC9A8[];
extern u32 D_008BC9B8[];
extern u32 D_008BC9C8[];
extern u32 D_008BC9D8[];
extern u32 D_008BC9E8[];
extern u32 D_008BC9F8[];
extern u32 D_008BCA08[];
extern u32 D_008BCA18[];
extern u32 D_008BCA28[];
extern u32 D_008BCA38[];
extern u32 D_008BCA48[];
extern u32 D_008BCA58[];
extern u32 D_008BCA68[];
extern u32 D_008BCA78[];
extern u32 D_008BCA88[];
extern u32 D_008BCA98[];
extern u32 D_008BCAA8[];
extern u32 D_008BCAB8[];
extern u32 D_008BCAC8[];
extern u32 D_008BCAD8[];
extern u32 D_008BCAE8[];
extern u32 D_008BCAF8[];
extern u32 D_008BCB08[];
extern u32 D_008BCB18[];
extern u32 D_008BCB28[];
extern u32 D_008BCB38[];
extern u32 D_008BCB48[];
extern u32 D_008BCB58[];
extern u32 D_008BCB68[];
extern u32 D_008BCB78[];
extern u32 D_008BCB88[];
extern u32 D_008BCB98[];
extern u32 D_008BCBA8[];
extern u32 D_008BCBB8[];
extern u32 D_008BCBC8[];
extern u32 D_008BCBD8[];
extern u32 D_008BCBE8[];
extern u32 D_008BCBF8[];
extern u32 D_008BCC08[];
extern u32 D_008BCC18[];
extern u32 D_008BCC28[];
extern u32 D_008BCC38[];
extern u32 D_008BCC48[];
extern u32 D_008BCC58[];
extern u32 D_008BCC68[];
extern u32 D_008BCC78[];
extern u32 D_008BCC88[];
extern u32 gEmAreaTimer1Values_68[];
extern u32 gEmAreaValues8A0_68[];
u32 gEmStatusParams_73[3] = {
#include "game/008BCC98.inc"
};
EmStatusParam *gEmStatusParams[74] = {
    NULL, (EmStatusParam *)D_008BC908, (EmStatusParam *)D_008BC928, (EmStatusParam *)D_008BC938,
    (EmStatusParam *)D_008BC948, (EmStatusParam *)D_008BC958, (EmStatusParam *)D_008BC968, (EmStatusParam *)D_008BC8F8,
    (EmStatusParam *)D_008BC978, (EmStatusParam *)D_008BC988, (EmStatusParam *)D_008BC8F8, (EmStatusParam *)D_008BC918,
    (EmStatusParam *)D_008BC998, (EmStatusParam *)D_008BC9A8, (EmStatusParam *)D_008BC9B8, (EmStatusParam *)D_008BC9C8,
    (EmStatusParam *)D_008BC9D8, (EmStatusParam *)D_008BC9E8, (EmStatusParam *)D_008BC8F8, (EmStatusParam *)D_008BC9F8,
    (EmStatusParam *)D_008BCA08, (EmStatusParam *)D_008BCA18, (EmStatusParam *)D_008BCA28, (EmStatusParam *)D_008BCA38,
    (EmStatusParam *)D_008BCA48, (EmStatusParam *)D_008BCA58, (EmStatusParam *)D_008BCA68, (EmStatusParam *)D_008BCA78,
    (EmStatusParam *)D_008BCA88, (EmStatusParam *)D_008BC8F8, (EmStatusParam *)D_008BCA98, (EmStatusParam *)D_008BCAA8,
    (EmStatusParam *)D_008BC8F8, (EmStatusParam *)D_008BCAB8, (EmStatusParam *)D_008BCAC8, (EmStatusParam *)D_008BCAD8,
    (EmStatusParam *)D_008BC928, (EmStatusParam *)D_008BCAE8, (EmStatusParam *)D_008BCAF8, (EmStatusParam *)D_008BCB08,
    (EmStatusParam *)D_008BC8F8, (EmStatusParam *)D_008BCB18, (EmStatusParam *)D_008BCB28, (EmStatusParam *)D_008BCB38,
    (EmStatusParam *)D_008BCB48, (EmStatusParam *)D_008BCB58, (EmStatusParam *)D_008BCB68, (EmStatusParam *)D_008BCB78,
    (EmStatusParam *)D_008BCB88, (EmStatusParam *)D_008BCB98, (EmStatusParam *)D_008BC8F8, (EmStatusParam *)D_008BCBA8,
    (EmStatusParam *)D_008BCBB8, (EmStatusParam *)D_008BCBC8, (EmStatusParam *)D_008BCBD8, (EmStatusParam *)D_008BC8F8,
    (EmStatusParam *)D_008BCBE8, (EmStatusParam *)D_008BCBF8, (EmStatusParam *)D_008BC8F8, (EmStatusParam *)D_008BCC08,
    (EmStatusParam *)D_008BCBD8, (EmStatusParam *)D_008BCC18, (EmStatusParam *)D_008BCC18, (EmStatusParam *)D_008BCC28,
    (EmStatusParam *)D_008BCC38, (EmStatusParam *)D_008BCC48, (EmStatusParam *)D_008BCC58, (EmStatusParam *)D_008BCC68,
    (EmStatusParam *)D_008BCC78, (EmStatusParam *)D_008BC938, (EmStatusParam *)D_008BC998, (EmStatusParam *)D_008BCC88,
    (EmStatusParam *)D_008BC8F8, (EmStatusParam *)gEmStatusParams_73,
};
EmStatusParam gEmStatus4Params_7 = {
#include "game/008BCDD8.inc"
};
EmStatusParam gEmStatus4Params_1 = {
#include "game/008BCDE8.inc"
};
EmStatusParam gEmStatus4Params_11 = {
#include "game/008BCDF8.inc"
};
EmStatusParam gEmStatus4Params_2 = {
#include "game/008BCE08.inc"
};
EmStatusParam gEmStatus4Params_3 = {
#include "game/008BCE18.inc"
};
EmStatusParam gEmStatus4Params_4 = {
#include "game/008BCE28.inc"
};
EmStatusParam gEmStatus4Params_5 = {
#include "game/008BCE38.inc"
};
EmStatusParam gEmStatus4Params_6 = {
#include "game/008BCE48.inc"
};
EmStatusParam gEmStatus4Params_8 = {
#include "game/008BCE58.inc"
};
EmStatusParam gEmStatus4Params_9 = {
#include "game/008BCE68.inc"
};
EmStatusParam gEmStatus4Params_12 = {
#include "game/008BCE78.inc"
};
EmStatusParam gEmStatus4Params_13 = {
#include "game/008BCE88.inc"
};
EmStatusParam gEmStatus4Params_14 = {
#include "game/008BCE98.inc"
};
EmStatusParam gEmStatus4Params_15 = {
#include "game/008BCEA8.inc"
};
EmStatusParam gEmStatus4Params_16 = {
#include "game/008BCEB8.inc"
};
EmStatusParam gEmStatus4Params_17 = {
#include "game/008BCEC8.inc"
};
EmStatusParam gEmStatus4Params_19 = {
#include "game/008BCED8.inc"
};
EmStatusParam gEmStatus4Params_20 = {
#include "game/008BCEE8.inc"
};
EmStatusParam gEmStatus4Params_21 = {
#include "game/008BCEF8.inc"
};
EmStatusParam gEmStatus4Params_22 = {
#include "game/008BCF08.inc"
};
EmStatusParam gEmStatus4Params_23 = {
#include "game/008BCF18.inc"
};
EmStatusParam gEmStatus4Params_24 = {
#include "game/008BCF28.inc"
};
EmStatusParam gEmStatus4Params_25 = {
#include "game/008BCF38.inc"
};
EmStatusParam gEmStatus4Params_26 = {
#include "game/008BCF48.inc"
};
EmStatusParam gEmStatus4Params_27 = {
#include "game/008BCF58.inc"
};
EmStatusParam gEmStatus4Params_28 = {
#include "game/008BCF68.inc"
};
EmStatusParam gEmStatus4Params_30 = {
#include "game/008BCF78.inc"
};
EmStatusParam gEmStatus4Params_31 = {
#include "game/008BCF88.inc"
};
EmStatusParam gEmStatus4Params_34 = {
#include "game/008BCF98.inc"
};
EmStatusParam gEmStatus4Params_35 = {
#include "game/008BCFA8.inc"
};
EmStatusParam gEmStatus4Params_37 = {
#include "game/008BCFB8.inc"
};
EmStatusParam gEmStatus4Params_38 = {
#include "game/008BCFC8.inc"
};
EmStatusParam gEmStatus4Params_39 = {
#include "game/008BCFD8.inc"
};
EmStatusParam gEmStatus4Params_40 = {
#include "game/008BCFE8.inc"
};
EmStatusParam gEmStatus4Params_41 = {
#include "game/008BCFF8.inc"
};
EmStatusParam gEmStatus4Params_42 = {
#include "game/008BD008.inc"
};
EmStatusParam gEmStatus4Params_43 = {
#include "game/008BD018.inc"
};
EmStatusParam gEmStatus4Params_44 = {
#include "game/008BD028.inc"
};
EmStatusParam gEmStatus4Params_45 = {
#include "game/008BD038.inc"
};
EmStatusParam gEmStatus4Params_46 = {
#include "game/008BD048.inc"
};
EmStatusParam gEmStatus4Params_47 = {
#include "game/008BD058.inc"
};
EmStatusParam gEmStatus4Params_48 = {
#include "game/008BD068.inc"
};
EmStatusParam gEmStatus4Params_49 = {
#include "game/008BD078.inc"
};
EmStatusParam gEmStatus4Params_51 = {
#include "game/008BD088.inc"
};
EmStatusParam gEmStatus4Params_52 = {
#include "game/008BD098.inc"
};
EmStatusParam gEmStatus4Params_53 = {
#include "game/008BD0A8.inc"
};
EmStatusParam gEmStatus4Params_54 = {
#include "game/008BD0B8.inc"
};
EmStatusParam gEmStatus4Params_59 = {
#include "game/008BD0C8.inc"
};
EmStatusParam gEmStatus4Params_61 = {
#include "game/008BD0D8.inc"
};
EmStatusParam gEmStatus4Params_62 = {
#include "game/008BD0E8.inc"
};
EmStatusParam gEmStatus4Params_63 = {
#include "game/008BD0F8.inc"
};
EmStatusParam gEmStatus4Params_64 = {
#include "game/008BD108.inc"
};
EmStatusParam gEmStatus4Params_65 = {
#include "game/008BD118.inc"
};
EmStatusParam gEmStatus4Params_66 = {
#include "game/008BD128.inc"
};
EmStatusParam gEmStatus4Params_67 = {
#include "game/008BD138.inc"
};
EmStatusParam gEmStatus4Params_68 = {
#include "game/008BD148.inc"
};
EmStatusParam gEmStatus4Params_71 = {
#include "game/008BD158.inc"
};
EmStatusParam gEmStatus4Params_73 = {
#include "game/008BD168.inc"
};
EmStatusParam *gEmStatus4Params[74] = {
    NULL, &gEmStatus4Params_1, &gEmStatus4Params_2, &gEmStatus4Params_3,
    &gEmStatus4Params_4, &gEmStatus4Params_5, &gEmStatus4Params_6, &gEmStatus4Params_7,
    &gEmStatus4Params_8, &gEmStatus4Params_9, &gEmStatus4Params_7, &gEmStatus4Params_11,
    &gEmStatus4Params_12, &gEmStatus4Params_13, &gEmStatus4Params_14, &gEmStatus4Params_15,
    &gEmStatus4Params_16, &gEmStatus4Params_17, &gEmStatus4Params_7, &gEmStatus4Params_19,
    &gEmStatus4Params_20, &gEmStatus4Params_21, &gEmStatus4Params_22, &gEmStatus4Params_23,
    &gEmStatus4Params_24, &gEmStatus4Params_25, &gEmStatus4Params_26, &gEmStatus4Params_27,
    &gEmStatus4Params_28, &gEmStatus4Params_7, &gEmStatus4Params_30, &gEmStatus4Params_31,
    &gEmStatus4Params_4, &gEmStatus4Params_7, &gEmStatus4Params_34, &gEmStatus4Params_35,
    &gEmStatus4Params_2, &gEmStatus4Params_37, &gEmStatus4Params_38, &gEmStatus4Params_39,
    &gEmStatus4Params_40, &gEmStatus4Params_41, &gEmStatus4Params_42, &gEmStatus4Params_43,
    &gEmStatus4Params_44, &gEmStatus4Params_45, &gEmStatus4Params_46, &gEmStatus4Params_47,
    &gEmStatus4Params_48, &gEmStatus4Params_49, &gEmStatus4Params_7, &gEmStatus4Params_51,
    &gEmStatus4Params_52, &gEmStatus4Params_53, &gEmStatus4Params_54, &gEmStatus4Params_7,
    &gEmStatus4Params_7, &gEmStatus4Params_9, &gEmStatus4Params_7, &gEmStatus4Params_59,
    &gEmStatus4Params_54, &gEmStatus4Params_61, &gEmStatus4Params_62, &gEmStatus4Params_63,
    &gEmStatus4Params_64, &gEmStatus4Params_65, &gEmStatus4Params_66, &gEmStatus4Params_67,
    &gEmStatus4Params_68, &gEmStatus4Params_3, &gEmStatus4Params_12, &gEmStatus4Params_71,
    &gEmStatus4Params_7, &gEmStatus4Params_73,
};
EmStatusParam gEmStatus5Params_2 = {
#include "game/008BD2A8.inc"
};
EmStatusParam gEmStatus5Params_1 = {
#include "game/008BD2B8.inc"
};
EmStatusParam gEmStatus5Params_11 = {
#include "game/008BD2C8.inc"
};
EmStatusParam gEmStatus5Params_5 = {
#include "game/008BD2D8.inc"
};
EmStatusParam gEmStatus5Params_6 = {
#include "game/008BD2E8.inc"
};
EmStatusParam gEmStatus5Params_8 = {
#include "game/008BD2F8.inc"
};
EmStatusParam gEmStatus5Params_9 = {
#include "game/008BD308.inc"
};
EmStatusParam gEmStatus5Params_13 = {
#include "game/008BD318.inc"
};
EmStatusParam gEmStatus5Params_14 = {
#include "game/008BD328.inc"
};
EmStatusParam gEmStatus5Params_15 = {
#include "game/008BD338.inc"
};
EmStatusParam gEmStatus5Params_16 = {
#include "game/008BD348.inc"
};
EmStatusParam gEmStatus5Params_17 = {
#include "game/008BD358.inc"
};
EmStatusParam gEmStatus5Params_20 = {
#include "game/008BD368.inc"
};
EmStatusParam gEmStatus5Params_21 = {
#include "game/008BD378.inc"
};
EmStatusParam gEmStatus5Params_22 = {
#include "game/008BD388.inc"
};
EmStatusParam gEmStatus5Params_23 = {
#include "game/008BD398.inc"
};
EmStatusParam gEmStatus5Params_26 = {
#include "game/008BD3A8.inc"
};
EmStatusParam gEmStatus5Params_27 = {
#include "game/008BD3B8.inc"
};
EmStatusParam gEmStatus5Params_28 = {
#include "game/008BD3C8.inc"
};
EmStatusParam gEmStatus5Params_30 = {
#include "game/008BD3D8.inc"
};
EmStatusParam gEmStatus5Params_31 = {
#include "game/008BD3E8.inc"
};
EmStatusParam gEmStatus5Params_34 = {
#include "game/008BD3F8.inc"
};
EmStatusParam gEmStatus5Params_35 = {
#include "game/008BD408.inc"
};
EmStatusParam gEmStatus5Params_37 = {
#include "game/008BD418.inc"
};
EmStatusParam gEmStatus5Params_38 = {
#include "game/008BD428.inc"
};
EmStatusParam gEmStatus5Params_39 = {
#include "game/008BD438.inc"
};
EmStatusParam gEmStatus5Params_40 = {
#include "game/008BD448.inc"
};
EmStatusParam gEmStatus5Params_43 = {
#include "game/008BD458.inc"
};
EmStatusParam gEmStatus5Params_44 = {
#include "game/008BD468.inc"
};
EmStatusParam gEmStatus5Params_45 = {
#include "game/008BD478.inc"
};
EmStatusParam gEmStatus5Params_46 = {
#include "game/008BD488.inc"
};
EmStatusParam gEmStatus5Params_47 = {
#include "game/008BD498.inc"
};
EmStatusParam gEmStatus5Params_48 = {
#include "game/008BD4A8.inc"
};
EmStatusParam gEmStatus5Params_49 = {
#include "game/008BD4B8.inc"
};
EmStatusParam gEmStatus5Params_51 = {
#include "game/008BD4C8.inc"
};
EmStatusParam gEmStatus5Params_54 = {
#include "game/008BD4D8.inc"
};
EmStatusParam gEmStatus5Params_57 = {
#include "game/008BD4E8.inc"
};
EmStatusParam gEmStatus5Params_59 = {
#include "game/008BD4F8.inc"
};
EmStatusParam gEmStatus5Params_64 = {
#include "game/008BD508.inc"
};
EmStatusParam gEmStatus5Params_66 = {
#include "game/008BD518.inc"
};
EmStatusParam gEmStatus5Params_68 = {
#include "game/008BD528.inc"
};
EmStatusParam gEmStatus5Params_73 = {
#include "game/008BD538.inc"
};
EmStatusParam *gEmStatus5Params[74] = {
    NULL, &gEmStatus5Params_1, &gEmStatus5Params_2, &gEmStatus5Params_2,
    &gEmStatus5Params_2, &gEmStatus5Params_5, &gEmStatus5Params_6, &gEmStatus5Params_2,
    &gEmStatus5Params_8, &gEmStatus5Params_9, &gEmStatus5Params_2, &gEmStatus5Params_11,
    &gEmStatus5Params_2, &gEmStatus5Params_13, &gEmStatus5Params_14, &gEmStatus5Params_15,
    &gEmStatus5Params_16, &gEmStatus5Params_17, &gEmStatus5Params_2, &gEmStatus5Params_2,
    &gEmStatus5Params_20, &gEmStatus5Params_21, &gEmStatus5Params_22, &gEmStatus5Params_23,
    &gEmStatus5Params_2, &gEmStatus5Params_2, &gEmStatus5Params_26, &gEmStatus5Params_27,
    &gEmStatus5Params_28, &gEmStatus5Params_2, &gEmStatus5Params_30, &gEmStatus5Params_31,
    &gEmStatus5Params_2, &gEmStatus5Params_2, &gEmStatus5Params_34, &gEmStatus5Params_35,
    &gEmStatus5Params_2, &gEmStatus5Params_37, &gEmStatus5Params_38, &gEmStatus5Params_39,
    &gEmStatus5Params_40, &gEmStatus5Params_11, &gEmStatus5Params_1, &gEmStatus5Params_43,
    &gEmStatus5Params_44, &gEmStatus5Params_45, &gEmStatus5Params_46, &gEmStatus5Params_47,
    &gEmStatus5Params_48, &gEmStatus5Params_49, &gEmStatus5Params_2, &gEmStatus5Params_51,
    &gEmStatus5Params_51, &gEmStatus5Params_51, &gEmStatus5Params_54, &gEmStatus5Params_2,
    &gEmStatus5Params_2, &gEmStatus5Params_57, &gEmStatus5Params_2, &gEmStatus5Params_59,
    &gEmStatus5Params_54, &gEmStatus5Params_51, &gEmStatus5Params_51, &gEmStatus5Params_2,
    &gEmStatus5Params_64, &gEmStatus5Params_64, &gEmStatus5Params_66, &gEmStatus5Params_48,
    &gEmStatus5Params_68, &gEmStatus5Params_2, &gEmStatus5Params_2, &gEmStatus5Params_2,
    &gEmStatus5Params_2, &gEmStatus5Params_73,
};
EmStatusParam gEmStatus2Params_7 = {
#include "game/008BD678.inc"
};
EmStatusParam gEmStatus2Params_1 = {
#include "game/008BD688.inc"
};
EmStatusParam gEmStatus2Params_11 = {
#include "game/008BD698.inc"
};
EmStatusParam gEmStatus2Params_2 = {
#include "game/008BD6A8.inc"
};
EmStatusParam gEmStatus2Params_3 = {
#include "game/008BD6B8.inc"
};
EmStatusParam gEmStatus2Params_4 = {
#include "game/008BD6C8.inc"
};
EmStatusParam gEmStatus2Params_6 = {
#include "game/008BD6D8.inc"
};
EmStatusParam gEmStatus2Params_8 = {
#include "game/008BD6E8.inc"
};
EmStatusParam gEmStatus2Params_9 = {
#include "game/008BD6F8.inc"
};
EmStatusParam gEmStatus2Params_12 = {
#include "game/008BD708.inc"
};
EmStatusParam gEmStatus2Params_14 = {
#include "game/008BD718.inc"
};
EmStatusParam gEmStatus2Params_15 = {
#include "game/008BD728.inc"
};
EmStatusParam gEmStatus2Params_13 = {
#include "game/008BD738.inc"
};
EmStatusParam gEmStatus2Params_17 = {
#include "game/008BD748.inc"
};
EmStatusParam gEmStatus2Params_19 = {
#include "game/008BD758.inc"
};
EmStatusParam gEmStatus2Params_20 = {
#include "game/008BD768.inc"
};
EmStatusParam gEmStatus2Params_21 = {
#include "game/008BD778.inc"
};
EmStatusParam gEmStatus2Params_22 = {
#include "game/008BD788.inc"
};
EmStatusParam gEmStatus2Params_23 = {
#include "game/008BD798.inc"
};
EmStatusParam gEmStatus2Params_24 = {
#include "game/008BD7A8.inc"
};
EmStatusParam gEmStatus2Params_25 = {
#include "game/008BD7B8.inc"
};
EmStatusParam gEmStatus2Params_26 = {
#include "game/008BD7C8.inc"
};
EmStatusParam gEmStatus2Params_27 = {
#include "game/008BD7D8.inc"
};
EmStatusParam gEmStatus2Params_28 = {
#include "game/008BD7E8.inc"
};
EmStatusParam gEmStatus2Params_31 = {
#include "game/008BD7F8.inc"
};
EmStatusParam gEmStatus2Params_33 = {
#include "game/008BD808.inc"
};
EmStatusParam gEmStatus2Params_34 = {
#include "game/008BD818.inc"
};
EmStatusParam gEmStatus2Params_35 = {
#include "game/008BD828.inc"
};
EmStatusParam gEmStatus2Params_36 = {
#include "game/008BD838.inc"
};
EmStatusParam gEmStatus2Params_37 = {
#include "game/008BD848.inc"
};
EmStatusParam gEmStatus2Params_38 = {
#include "game/008BD858.inc"
};
EmStatusParam gEmStatus2Params_39 = {
#include "game/008BD868.inc"
};
EmStatusParam gEmStatus2Params_40 = {
#include "game/008BD878.inc"
};
EmStatusParam gEmStatus2Params_41 = {
#include "game/008BD888.inc"
};
EmStatusParam gEmStatus2Params_42 = {
#include "game/008BD898.inc"
};
EmStatusParam gEmStatus2Params_43 = {
#include "game/008BD8A8.inc"
};
EmStatusParam gEmStatus2Params_44 = {
#include "game/008BD8B8.inc"
};
EmStatusParam gEmStatus2Params_45 = {
#include "game/008BD8C8.inc"
};
EmStatusParam gEmStatus2Params_46 = {
#include "game/008BD8D8.inc"
};
EmStatusParam gEmStatus2Params_47 = {
#include "game/008BD8E8.inc"
};
EmStatusParam gEmStatus2Params_48 = {
#include "game/008BD8F8.inc"
};
EmStatusParam gEmStatus2Params_49 = {
#include "game/008BD908.inc"
};
EmStatusParam gEmStatus2Params_51 = {
#include "game/008BD918.inc"
};
EmStatusParam gEmStatus2Params_52 = {
#include "game/008BD928.inc"
};
EmStatusParam gEmStatus2Params_53 = {
#include "game/008BD938.inc"
};
EmStatusParam gEmStatus2Params_54 = {
#include "game/008BD948.inc"
};
EmStatusParam gEmStatus2Params_59 = {
#include "game/008BD958.inc"
};
EmStatusParam gEmStatus2Params_61 = {
#include "game/008BD968.inc"
};
EmStatusParam gEmStatus2Params_62 = {
#include "game/008BD978.inc"
};
EmStatusParam gEmStatus2Params_64 = {
#include "game/008BD988.inc"
};
EmStatusParam gEmStatus2Params_65 = {
#include "game/008BD998.inc"
};
EmStatusParam gEmStatus2Params_66 = {
#include "game/008BD9A8.inc"
};
EmStatusParam gEmStatus2Params_67 = {
#include "game/008BD9B8.inc"
};
EmStatusParam gEmStatus2Params_68 = {
#include "game/008BD9C8.inc"
};
EmStatusParam gEmStatus2Params_71 = {
#include "game/008BD9D8.inc"
};
EmStatusParam gEmStatus2Params_73 = {
#include "game/008BD9E8.inc"
};
EmStatusParam *gEmStatus2Params[74] = {
    NULL, &gEmStatus2Params_1, &gEmStatus2Params_2, &gEmStatus2Params_3,
    &gEmStatus2Params_4, &gEmStatus2Params_4, &gEmStatus2Params_6, &gEmStatus2Params_7,
    &gEmStatus2Params_8, &gEmStatus2Params_9, &gEmStatus2Params_7, &gEmStatus2Params_11,
    &gEmStatus2Params_12, &gEmStatus2Params_13, &gEmStatus2Params_14, &gEmStatus2Params_15,
    &gEmStatus2Params_13, &gEmStatus2Params_17, &gEmStatus2Params_7, &gEmStatus2Params_19,
    &gEmStatus2Params_20, &gEmStatus2Params_21, &gEmStatus2Params_22, &gEmStatus2Params_23,
    &gEmStatus2Params_24, &gEmStatus2Params_25, &gEmStatus2Params_26, &gEmStatus2Params_27,
    &gEmStatus2Params_28, &gEmStatus2Params_7, &gEmStatus2Params_13, &gEmStatus2Params_31,
    &gEmStatus2Params_7, &gEmStatus2Params_33, &gEmStatus2Params_34, &gEmStatus2Params_35,
    &gEmStatus2Params_36, &gEmStatus2Params_37, &gEmStatus2Params_38, &gEmStatus2Params_39,
    &gEmStatus2Params_40, &gEmStatus2Params_41, &gEmStatus2Params_42, &gEmStatus2Params_43,
    &gEmStatus2Params_44, &gEmStatus2Params_45, &gEmStatus2Params_46, &gEmStatus2Params_47,
    &gEmStatus2Params_48, &gEmStatus2Params_49, &gEmStatus2Params_7, &gEmStatus2Params_51,
    &gEmStatus2Params_52, &gEmStatus2Params_53, &gEmStatus2Params_54, &gEmStatus2Params_7,
    &gEmStatus2Params_7, &gEmStatus2Params_9, &gEmStatus2Params_7, &gEmStatus2Params_59,
    &gEmStatus2Params_54, &gEmStatus2Params_61, &gEmStatus2Params_62, &gEmStatus2Params_7,
    &gEmStatus2Params_64, &gEmStatus2Params_65, &gEmStatus2Params_66, &gEmStatus2Params_67,
    &gEmStatus2Params_68, &gEmStatus2Params_3, &gEmStatus2Params_12, &gEmStatus2Params_71,
    &gEmStatus2Params_7, &gEmStatus2Params_73,
};
EmStatusParam gEmStatus1Params_2 = {
#include "game/008BDB28.inc"
};
EmStatusParam gEmStatus1Params_1 = {
#include "game/008BDB38.inc"
};
EmStatusParam *gEmStatus1Params[74] = {
    NULL, &gEmStatus1Params_1, &gEmStatus1Params_2, &gEmStatus1Params_2,
    &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_1, &gEmStatus1Params_2,
    &gEmStatus1Params_1, &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_1,
    &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_1, &gEmStatus1Params_1,
    &gEmStatus1Params_2, &gEmStatus1Params_1, &gEmStatus1Params_2, &gEmStatus1Params_2,
    &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_2,
    &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_1, &gEmStatus1Params_1,
    &gEmStatus1Params_1, &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_1,
    &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2,
    &gEmStatus1Params_2, &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_1,
    &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_1,
    &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_1,
    &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_2, &gEmStatus1Params_1,
    &gEmStatus1Params_1, &gEmStatus1Params_1, &gEmStatus1Params_2, &gEmStatus1Params_2,
    &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2,
    &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2,
    &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_1,
    &gEmStatus1Params_1, &gEmStatus1Params_2, &gEmStatus1Params_2, &gEmStatus1Params_2,
    &gEmStatus1Params_2, &gEmStatus1Params_2,
};
u32 gEmKindTimerValues[2] = {
#include "game/008BDC78.inc"
};
u32 gEmKindTimerIndex[20] = {
#include "game/008BDC80.inc"
};
EmRageRow gEmRageTbl_1 = {
#include "game/008BDCD0.inc"
};
EmRageRow gEmRageTbl_2 = {
#include "game/008BDD10.inc"
};
EmRageRow gEmRageTbl_5 = {
#include "game/008BDD50.inc"
};
EmRageRow gEmRageTbl_6 = {
#include "game/008BDD90.inc"
};
EmRageRow gEmRageTbl_7 = {
#include "game/008BDDD0.inc"
};
EmRageRow gEmRageTbl_8 = {
#include "game/008BDE10.inc"
};
EmRageRow gEmRageTbl_11 = {
#include "game/008BDE50.inc"
};
EmRageRow gEmRageTbl_14 = {
#include "game/008BDE90.inc"
};
EmRageRow gEmRageTbl_15 = {
#include "game/008BDED0.inc"
};
EmRageRow gEmRageTbl_17 = {
#include "game/008BDF10.inc"
};
EmRageRow gEmRageTbl_20 = {
#include "game/008BDF50.inc"
};
EmRageRow gEmRageTbl_21 = {
#include "game/008BDF90.inc"
};
EmRageRow gEmRageTbl_22 = {
#include "game/008BDFD0.inc"
};
EmRageRow gEmRageTbl_26 = {
#include "game/008BE010.inc"
};
EmRageRow gEmRageTbl_27 = {
#include "game/008BE050.inc"
};
EmRageRow gEmRageTbl_28 = {
#include "game/008BE090.inc"
};
EmRageRow gEmRageTbl_31 = {
#include "game/008BE0D0.inc"
};
EmRageRow gEmRageTbl_33 = {
#include "game/008BE110.inc"
};
EmRageRow gEmRageTbl_36 = {
#include "game/008BE150.inc"
};
EmRageRow gEmRageTbl_37 = {
#include "game/008BE190.inc"
};
EmRageRow gEmRageTbl_38 = {
#include "game/008BE1D0.inc"
};
EmRageRow gEmRageTbl_39 = {
#include "game/008BE210.inc"
};
EmRageRow gEmRageTbl_40 = {
#include "game/008BE250.inc"
};
EmRageRow gEmRageTbl_41 = {
#include "game/008BE290.inc"
};
EmRageRow gEmRageTbl_42 = {
#include "game/008BE2D0.inc"
};
EmRageRow gEmRageTbl_43 = {
#include "game/008BE310.inc"
};
EmRageRow gEmRageTbl_44 = {
#include "game/008BE350.inc"
};
EmRageRow gEmRageTbl_45 = {
#include "game/008BE390.inc"
};
EmRageRow gEmRageTbl_46 = {
#include "game/008BE3D0.inc"
};
EmRageRow gEmRageTbl_47 = {
#include "game/008BE410.inc"
};
EmRageRow gEmRageTbl_48 = {
#include "game/008BE450.inc"
};
EmRageRow gEmRageTbl_49 = {
#include "game/008BE490.inc"
};
EmRageRow gEmRageTbl_50 = {
#include "game/008BE4D0.inc"
};
EmRageRow gEmRageTbl_51 = {
#include "game/008BE510.inc"
};
EmRageRow gEmRageTbl_52 = {
#include "game/008BE550.inc"
};
EmRageRow gEmRageTbl_53 = {
#include "game/008BE590.inc"
};
EmRageRow gEmRageTbl_54 = {
#include "game/008BE5D0.inc"
};
EmRageRow gEmRageTbl_55 = {
#include "game/008BE610.inc"
};
EmRageRow gEmRageTbl_58 = {
#include "game/008BE650.inc"
};
EmRageRow gEmRageTbl_59 = {
#include "game/008BE690.inc"
};
EmRageRow gEmRageTbl_61 = {
#include "game/008BE6D0.inc"
};
EmRageRow gEmRageTbl_64 = {
#include "game/008BE710.inc"
};
EmRageRow gEmRageTbl_67 = {
#include "game/008BE750.inc"
};
EmRageRow *gEmRageTbl[74] = {
    NULL, &gEmRageTbl_1, &gEmRageTbl_2, NULL,
    NULL, &gEmRageTbl_5, &gEmRageTbl_6, &gEmRageTbl_7,
    &gEmRageTbl_8, NULL, NULL, &gEmRageTbl_11,
    NULL, NULL, &gEmRageTbl_14, &gEmRageTbl_15,
    NULL, &gEmRageTbl_17, NULL, NULL,
    &gEmRageTbl_20, &gEmRageTbl_21, &gEmRageTbl_22, NULL,
    NULL, NULL, &gEmRageTbl_26, &gEmRageTbl_27,
    &gEmRageTbl_28, NULL, NULL, &gEmRageTbl_31,
    NULL, &gEmRageTbl_33, NULL, NULL,
    &gEmRageTbl_36, &gEmRageTbl_37, &gEmRageTbl_38, &gEmRageTbl_39,
    &gEmRageTbl_40, &gEmRageTbl_41, &gEmRageTbl_42, &gEmRageTbl_43,
    &gEmRageTbl_44, &gEmRageTbl_45, &gEmRageTbl_46, &gEmRageTbl_47,
    &gEmRageTbl_48, &gEmRageTbl_49, &gEmRageTbl_50, &gEmRageTbl_51,
    &gEmRageTbl_52, &gEmRageTbl_53, &gEmRageTbl_54, &gEmRageTbl_55,
    NULL, NULL, &gEmRageTbl_58, &gEmRageTbl_59,
    &gEmRageTbl_54, &gEmRageTbl_61, NULL, NULL,
    &gEmRageTbl_64, &gEmRageTbl_64, NULL, &gEmRageTbl_67,
    &gEmRageTbl_5, NULL, NULL, &gEmRageTbl_36,
    &gEmRageTbl_11, NULL,
};
u32 gEmDamagedPartTbls_1[2] = {
#include "game/008BE8B8.inc"
};
u32 gEmDamagedPartTbls_48[2] = {
#include "game/008BE8C0.inc"
};
u32 gEmDamagedPartTbls_51[2] = {
#include "game/008BE8C8.inc"
};
u32 gEmDamagedPartTbls_53[2] = {
#include "game/008BE8D0.inc"
};
u32 gEmDamagedPartTbls_54[2] = {
#include "game/008BE8D8.inc"
};
u32 gEmDamagedPartTbls_59[2] = {
#include "game/008BE8E0.inc"
};
u32 gEmDamagedPartTbls_64[2] = {
#include "game/008BE8E8.inc"
};
u32 gEmDamagedPartTbls[76] = {
    0, (u32)gEmDamagedPartTbls_1, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, (u32)gEmDamagedPartTbls_1,
    0, 0, (u32)gEmDamagedPartTbls_1, 0,
    0, (u32)gEmDamagedPartTbls_1, 0, 0,
    0, 0, (u32)gEmDamagedPartTbls_1, 0,
    0, 0, (u32)gEmDamagedPartTbls_1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, (u32)gEmDamagedPartTbls_1, 0, 0,
    (u32)gEmDamagedPartTbls_1, (u32)gEmDamagedPartTbls_1, (u32)gEmDamagedPartTbls_1, (u32)gEmDamagedPartTbls_1,
    (u32)gEmDamagedPartTbls_1, 0, 0, (u32)gEmDamagedPartTbls_1,
    (u32)gEmDamagedPartTbls_48, (u32)gEmDamagedPartTbls_1, 0, (u32)gEmDamagedPartTbls_51,
    0, (u32)gEmDamagedPartTbls_53, (u32)gEmDamagedPartTbls_54, 0,
    0, 0, 0, (u32)gEmDamagedPartTbls_59,
    (u32)gEmDamagedPartTbls_54, 0, 0, 0,
    (u32)gEmDamagedPartTbls_64, (u32)gEmDamagedPartTbls_64, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};
u32 gEmCombatInit[40] = {
#include "game/008BEA20.inc"
};
u32 gEmBlindTimes[38] = {
#include "game/008BEAC0.inc"
};
u32 gEmAreaValues8A0_0[2] = {
#include "game/008BEB58.inc"
};
u32 gEmAreaValues8A0[74] = {
#include "game/008BEB60.inc"
};
u32 gEmAreaTimer1Values_0[2] = {
#include "game/008BEC88.inc"
};
u32 gEmAreaTimer1Values[76] = {
#include "game/008BEC90.inc"
};
u16 gEmCollMasks[80] = {
#include "game/008BEDC0.inc"
};
u8 gEmIsLarge[80] = {
#include "game/008BEE60.inc"
};
u32 gEmCarryReact[20] = {
#include "game/008BEEB0.inc"
};
EmKindInfo gEmKindInfo[74] = {
#include "game/008BEF00.inc"
};
s16 gEmRequestIds_1[8] = {
#include "game/008BF150.inc"
};
s16 gEmRequestIds_6[4] = {
#include "game/008BF160.inc"
};
s16 gEmRequestIds_8[4] = {
#include "game/008BF168.inc"
};
s16 gEmRequestIds_15[4] = {
#include "game/008BF170.inc"
};
s16 gEmRequestIds_20[4] = {
#include "game/008BF178.inc"
};
s16 gEmRequestIds_51[4] = {
#include "game/008BF180.inc"
};
s16 gEmRequestIds_54[4] = {
#include "game/008BF188.inc"
};
s16 gEmRequestIds_64[4] = {
#include "game/008BF190.inc"
};
s16 gEmRequestIds_68[4] = {
#include "game/008BF198.inc"
};
s16 *gEmRequestIds[76] = {
    NULL, gEmRequestIds_1, NULL, NULL,
    NULL, NULL, gEmRequestIds_6, NULL,
    gEmRequestIds_8, NULL, NULL, gEmRequestIds_1,
    NULL, NULL, NULL, gEmRequestIds_15,
    NULL, NULL, NULL, NULL,
    gEmRequestIds_20, gEmRequestIds_1, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gEmRequestIds_1, gEmRequestIds_6, gEmRequestIds_20,
    gEmRequestIds_1, gEmRequestIds_1, gEmRequestIds_1, NULL,
    NULL, gEmRequestIds_15, gEmRequestIds_1, NULL,
    NULL, gEmRequestIds_1, NULL, gEmRequestIds_51,
    gEmRequestIds_51, gEmRequestIds_51, gEmRequestIds_54, NULL,
    NULL, NULL, NULL, NULL,
    gEmRequestIds_54, NULL, NULL, NULL,
    gEmRequestIds_64, gEmRequestIds_64, NULL, NULL,
    gEmRequestIds_68, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u8 gEmIsLarge2[80] = {
#include "game/008BF2D0.inc"
};
EmIdSet* *gEmIdSets[76] = {
#include "game/008BF320.inc"
};
