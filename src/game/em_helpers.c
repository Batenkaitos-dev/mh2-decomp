/*
 * Monster helpers: area and player checks, hit points (Em_ComputeMaxHp,
 * Em_RegenHp), the level's attack and damage factors, the trap hold time,
 * motion rows, the per-area timers, and sound effects played at a bone when
 * the motion reaches a frame. The .data holds each area's ground height
 * (gAreaGroundY), the area point sets (gEmKindTables_68_0, gEmGoalPosSets,
 * gEmKindTables_68_2), the route sets (gEmRouteSets) and Em_PartEffectTbl.
 */

#include "quest.h"
#include "em.h"
#include "player.h"

typedef struct EmIdPair {
    s16 id;             /* 0x0: >= 10000: an exact Em.area + 10000; -1 ends the table */
    s16 value;          /* 0x2 */
} EmIdPair;


/* Per-kind rows (12 bytes), selected by Em.level. */
typedef struct EmScaleRow {
    f32 hpScale;        /* the base hp's factor (Em_ComputeMaxHp: hpMax) */
    f32 attackFactor;   /* Em.attackFactor's */
    f32 damageFactor;   /* Em.damageFactor's */
} EmScaleRow;

extern EmScaleRow *gEmScaleRows[];
extern s16 *gEmBaseHp[];
extern f32 *gEmVersusHpFactors[];
extern f32 gEmSizeLevelFactors[];
extern f32 gEmLowHpRatios[];
extern EmMotionRow *gEmMotionRows[];
extern EmIdPair *gEmAreaTimer2Values[];
extern EmIdPair *gEmAreaTimer1Values[];
extern s8 gSeVolumeByType[];

/* Effects shown when a part record changes state (0x14 bytes). */
typedef struct EmPartEffect {
    u8 kind;            /* 0x00: compared with Em.type through Em_KindMatches */
    u8 part;            /* 0x01: index into Em.parts */
    u8 state;           /* 0x02: matched with the state of that part */
    u8 flag;            /* 0x03: 1 sets Em.hitzoneSet */
    s16 bone;           /* 0x04: -1 when there is no effect */
    u16 breakPart;      /* 0x06: the part Em_BreakPart breaks; 0xFFFF none */
    f32 offset[3];      /* 0x08: from the bone */
} EmPartEffect;

extern EmPartEffect Em_PartEffectTbl[0x2A];

s32 Obj_SameArea(void *obj, Player *b);
s32 Quest_IsVersus(void);
s32 MonsterNote_Index(u16 id);
s8 Versus_Team(s8 a);
u16 Area_DayId(u16 id);
u8 MonsterGroup_First(u8 a);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
s32 Player_UpdateMotion(struct Player *em);
void Em_UpdateMatrix(Em *em);
s32 EmMotion_PassedFrameScaled(void *em, f32 frame, u8 a); /* unit's own declaration: with the definition's s32 (Player *p, f32 frame, s32 layer) (declared with a u8 layer as the two layer-frame sound functions pass it; with the definition's s32 their calls compile differently) */
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Mtx_GetTrans(f32 *out, f32 *m);
s32 Sound_PlayEmSe(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Em_PlaySeType(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Em_SetLayerMotion(Em *em, s32 motion, s32 a, s32 b, s32 layer);
u8 Player_InCurrentArea(void *obj);
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_Copy(f32 *out, f32 *in);
void Vec_RotateSelf(f32 *v, f32 *m);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Em_BreakPart(Em *em, s32 part);
s32 Em_KindMatches(u8 a, u8 b);
EmObj *Em_FindGoalAction(Em *em);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_ApplyActionStatus(Em *em, EmObj *action);
void EmScript_Run(Em *em);
void Em_RestartScript(Em *em);
void EmTail_Drop(struct StageObj *fx);
void Em_SetHitReactMode(Em *em, u8 a);
f32 Em_LevelAttackFactor(Em *em);
f32 Em_LevelDamageFactor(Em *em);
void Em_SetAreaTimer1(Em *em, EmIdPair *pair);
void Em_SetAreaTimer2(Em *em, EmIdPair *pair);
void Em_PlaySeAtBoneVol(Em *em, s32 se, s32 bone, s32 vol);

/*
 * Keeps a monster in combat in place while no hunter is in its area. Clears
 * moveFrozen. Unless an event runs in the scene's area, or an active hunter
 * in the quest shares its area (Obj_SameArea), it puts posX and posZ back to
 * prevPosX and prevPosZ and sets moveFrozen. It does not do this while the
 * monster is in state (`group`, `state`).
 */
void Em_HoldPosNoPlayers(Em *em, u16 exceptGroup, u16 exceptState)
{
    s8 i;

    em->moveFrozen = 0;
    if (gScene.eventRunning != 0 && gScene.area == em->area) {
        return;
    }
    if (em->combat != 1) {
        return;
    }
    for (i = 0; i < gScene.playerCount; i++) {
        if (Obj_SameArea(em, &gPlayers[i]) != 0 && gPlayers[i].active != 0 && gScene.inQuest[i] == 1) {
            return;
        }
    }
    if (em->stateGroup != exceptGroup || em->state != exceptState) {
        em->moveFrozen = 1;
        em->posX = em->prevPosX;
        em->posZ = em->prevPosZ;
    }
}

/*
 * For a local monster in combat: caps combatTimer at 900 while no player is
 * noticed, and counts it down. At 0 the monster leaves combat (Em_SetCombat);
 * in the air (EM_STANCE_AIR) it waits at 0 instead. On leaving, the drome
 * leaders, Kirin, Daimyo Hermitaur, Blangonga, Congalala, Rajang, Shogun
 * Ceanataur and Bulldrome restart their script (scriptPending) when no hunter
 * in the quest is in their area. Returns 1 when the monster left combat.
 */
s32 Em_UpdateCombatTimer(Em *em)
{
    s8 i;

    if (em->combat == 1 && em->remote == 0) {
        if (em->noticed == 0 && em->combatTimer > 900) {
            em->combatTimer = 900;
        }
        if (--em->combatTimer <= 0) {
            if (em->stance == 2) {
                em->combatTimer = 0;
            } else {
                Em_SetCombat(em, 0, 0);
                switch (em->type) {
                case EM_KIND_VELOCIDROME:
                case EM_KIND_GENDROME:
                case EM_KIND_IODROME:
                case EM_KIND_KIRIN:
                case EM_KIND_DAIMYO_HERMITAUR:
                case EM_KIND_BLANGONGA:
                case EM_KIND_CONGALALA:
                case EM_KIND_RAJANG:
                case EM_KIND_SHOGUN_CEANATAUR:
                case EM_KIND_BULLDROME:
                    for (i = 0; i < gScene.playerCount; i++) {
                        if (em->area == gPlayers[i].area && gPlayers[i].active != 0 && gScene.inQuest[i] == 1) {
                            break;
                        }
                    }
                    if (i >= gScene.playerCount) {
                        Em_RestartScript(em);
                        em->scriptPending = 1;
                    }
                    break;
                }
                return 1;
            }
        }
    }
    return 0;
}

/* Adds hit points; above the maximum (or wrapped below 0) it becomes the maximum. */
void Em_AddHp(Em *em, s16 n)
{
    em->hp += n;
    if (em->hp > em->hpMax || em->hp < 0) {
        em->hp = em->hpMax;
    }
}

/*
 * Returns 1 when every player is active and in the monster's area, and always
 * for a remote monster. Returns 0 with no target (targetNo -1). Rathalos's
 * and Kut-Ku's states end early when it is 0.
 */
u8 Em_AllPlayersHere(Em *em)
{
    Player *player = gPlayers;
    s8 i;

    if (em->remote != 0) {
        return 1;
    }
    if (em->targetNo == -1) {
        return 0;
    }
    for (i = 0; i < gScene.playerCount; i++, player++) {
        if ((u8)Obj_SameArea(em, player) == 0 || player->active == 0) {
            return 0;
        }
    }
    return 1;
}

/* Sets hitReactMode (enum EmHitReactMode). */
void Em_SetHitReactMode(Em *em, u8 mode)
{
    em->hitReactMode = mode;
}

/*
 * Returns the monster's maximum hp: the kind's base hp gEmBaseHp[type][idx]
 * times the hp scale of its level row (gEmScaleRows). In a versus quest it is
 * instead 1700 times the kind's monster-note factor (gEmVersusHpFactors),
 * gEmSizeLevelFactors[level] and its team's sizeScale percent; a kind with no
 * note gets 1700. The families pass idx 0 and store the result in hpMax and
 * hp.
 */
s32 Em_ComputeMaxHp(Em *em, s16 idx)
{
    EmScaleRow *rows = gEmScaleRows[em->type];
    s16 *baseHp = gEmBaseHp[em->type];
    f32 hp;

    if (Quest_IsVersus() == 1) {
        s32 noteIndex = MonsterNote_Index(em->type);

        if (noteIndex != 0xFF) {
            hp = 1700.0f * *gEmVersusHpFactors[noteIndex] * gEmSizeLevelFactors[em->level];
            hp *= gQuest.teams[Versus_Team(em->side)].sizeScale / 100.0f;
        } else {
            hp = 1700.0f;
        }
    } else {
        baseHp += idx;
        hp = *baseHp;
        rows += em->level;
        hp *= rows->hpScale;
    }
    return hp;
}

/*
 * Returns the attack factor of the monster's level (its gEmScaleRows row),
 * times its versus team's rageScale1 percent. Em_EndRage stores it in
 * attackFactor.
 */
f32 Em_LevelAttackFactor(Em *em)
{
    EmScaleRow *row;
    f32 factor;

    row = gEmScaleRows[em->type];
    row += em->level;
    factor = row->attackFactor;
    if (Quest_IsVersus() == 1) {
        factor *= gQuest.teams[Versus_Team(em->side)].rageScale1 / 100.0f;
    }
    return factor;
}

/*
 * Returns the damage factor of the monster's level, times its versus team's
 * rageScale2 percent. Em_EndRage stores it in damageFactor.
 */
f32 Em_LevelDamageFactor(Em *em)
{
    EmScaleRow *row;
    f32 factor;

    row = gEmScaleRows[em->type];
    row += em->level;
    factor = row->damageFactor;
    if (Quest_IsVersus() == 1) {
        factor *= gQuest.teams[Versus_Team(em->side)].rageScale2 / 100.0f;
    }
    return factor;
}

/*
 * Sets trapTimer, how long a trap holds the monster. EM_KIND_RAJANG gets 0x9C
 * frames. Otherwise it is 0x340 when the monster was out of combat before the
 * hit (*p is Em_UpdateDamage's out), 0x208 when calm, and 0x138 when enraged.
 * The families call it on EM_DMG_ATTACK9 and EM_DMG_ATTACK6, before their
 * trapped state.
 */
void Em_SetTrapTime(Em *em, u8 *wasInCombat)
{
    if (em->type == EM_KIND_RAJANG) {
        em->trapTimer = 0x9C;
    } else if (*wasInCombat == 0) {
        em->trapTimer = 0x340;
    } else if (em->rage == 0) {
        em->trapTimer = 0x208;
    } else {
        em->trapTimer = 0x138;
    }
}

/*
 * For a local monster, sets goal kind 12 with trapTimer / 4 as goalNo
 * (Em_ComputeGoal); Em_ComputeGoal2 sets trapTimer back from it. Likely this
 * makes the remote copies hold for as long.
 */
void Em_SetTrapTimeGoal(Em *em)
{
    if (em->remote == 0) {
        em->goalKind = 12;
        em->goalArg = 0;
        em->goalNo = em->trapTimer / 4;
        Em_ComputeGoal(em, em->goal);
    }
}

/*
 * Returns 1 when hp is at most hpMax times (the kind's gEmLowHpRatios ratio +
 * 0.1).
 */
s32 Em_NearLowHp(Em *em)
{
    f32 ratio = gEmLowHpRatios[em->type];

    ratio += 0.1f;
    return em->hp <= (s16)(em->hpMax * ratio);
}

/*
 * Returns 1 when an active EM_KIND_LAO_SHAN_LUNG, EM_KIND_ASHEN_LAO_SHAN_LUNG
 * or EM_KIND_SHEN_GAOREN has at most half its hit points.
 */
s32 EmList_AnyBelowHalfHp(void)
{
    Em *em = gEms;
    s8 i;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0) {
            switch (em->type) {
            case EM_KIND_LAO_SHAN_LUNG:
            case EM_KIND_ASHEN_LAO_SHAN_LUNG:
                if (em->hp <= em->hpMax / 2) {
                    return 1;
                }
                break;
            case EM_KIND_SHEN_GAOREN:
                if (em->hp <= em->hpMax / 2) {
                    return 1;
                }
                break;
            }
        }
    }
    return 0;
}

/* Every `interval` frames, adds `n` hit points up to `max`; 1 when it gets there. */
s32 Em_RegenHp(Em *em, s16 n, s16 max, s16 interval)
{
    if (interval != 0 && gScene.frame % interval != 0) {
        return 0;
    }
    if (em->hp >= max) {
        return 0;
    }
    em->hp += n;
    if (em->hp >= max) {
        em->hp = max;
        return 1;
    }
    return 0;
}

/*
 * For each part selected by `mask`, takes the first Em_PartEffectTbl row of
 * the monster's family (Em_KindMatches) for that part and its state. In the
 * current area it makes an effect (Effect02 type 10) at the row's bone and
 * offset. It then breaks the row's breakPart (Em_BreakPart), and sets
 * hitzoneSet to 1 for a row with flag 1.
 */
void Em_SpawnPartEffects(Em *em, u8 mask)
{
    f32 pos[4];
    f32 effectOfs[4];
    s16 *bone;
    f32 mtx[4][4];
    s16 i;
    s16 j;
    u8 bit = 1;
    EmPartEffect *partEffect;

    for (i = 0; i < 8; bit <<= 1, i++) {
        if (!(mask & bit)) {
            continue;
        }
        for (j = 0; j < 0x2A; j++) {
            if (Em_KindMatches(em->type, Em_PartEffectTbl[j].kind) && Em_PartEffectTbl[j].part == i
                && em->parts[i].state == Em_PartEffectTbl[j].state) {
                partEffect = &Em_PartEffectTbl[j];
                bone = &partEffect->bone;
                if (*bone != -1 && (u8)Player_InCurrentArea(em) != 0) {
                    Mtx_Copy((f32 *) mtx, (f32 *) (Em_Bone(em, *bone)));
                    Vec_Copy(effectOfs, partEffect->offset);
                    Vec_RotateSelf((f32 *) effectOfs, (f32 *) mtx);
                    pos[0] = mtx[3][0] + effectOfs[0];
                    pos[1] = mtx[3][1] + effectOfs[1];
                    pos[2] = mtx[3][2] + effectOfs[2];
                    HitSpark_SpawnForOwner(em, 0, 10, 0, pos, 1.0f);
                }
                if (partEffect->breakPart != 0xFFFF) {
                    Em_BreakPart(em, partEffect->breakPart);
                }
                if (partEffect->flag == 1) {
                    em->hitzoneSet = 1;
                }
                break;
            }
        }
    }
}

/*
 * Returns the monster to the ground pose: EM_STANCE_GROUND, fxFlags bit 1
 * set, mesh 1 hidden. Unless layer 0 already plays it, it sets motion 0x53 on
 * layers 0-2 and rebuilds the rotation matrix, the motion and the bone
 * matrices. Then it places its tail object (tailObj, EmTail_Drop).
 */
void Em_ResetToBasePose(Em *em)
{
    struct StageObj *p = em->tailObj;

    em->stance = 0;
    em->fxFlags |= 1;
    em->meshShown[1] = 0;
    if (em->motionId[0] != 0x43B) {
        Em_SetLayerMotion(em, 0x43B, 0, 0, 0);
        Em_SetLayerMotion(em, 0x503, 0, 0, 1);
        Em_SetLayerMotion(em, 0x5CB, 0, 0, 2);
        em->rotX = (u16)em->rotX;
        em->rotY = (u16)em->rotY;
        em->rotZ = (u16)em->rotZ;
        Mtx_FromAnglesYXZ(&em->rotX, (f32 *) em->rotMtx);
        Player_UpdateMotion((struct Player *)em);
        Em_UpdateMatrix(em);
    }
    EmTail_Drop(p);
}

/*
 * Asks for the monster's state to be sent (syncRequest; QSync_SendEm sends
 * it): mode 1 always, mode 2 unless a mode 1 request is pending.
 */
void Em_RequestSync(Em *em, u8 mode)
{
    switch (mode) {
    case 1:
        em->syncRequest = mode;
        break;
    case 2:
        if (em->syncRequest != 1) {
            em->syncRequest = mode;
        }
        break;
    }
}

/*
 * The monster eats the area object its action goal names (Em_FindGoalAction):
 * marks it done, applies its status (Em_ApplyActionStatus), and plain meat
 * (kind 0) heals 1% of hpMax. Em_StartGoalAction calls it.
 */
void Em_UseGoalAction(Em *em)
{
    EmObj *action = Em_FindGoalAction(em);

    if (action != NULL) {
        action->done = 1;
        Em_ApplyActionStatus(em, action);
        if (action->kind == 0) {
            Em_AddHp(em, 0.01f * em->hpMax);
        }
    }
}

/*
 * Starts row `row` of the kind's motion table (gEmMotionRows) from frame 0
 * with no blend, on the layers the row names: -1 all; 0-2 that one; 0x10,
 * 0x11, 0x12 all but layer 2, 1, 0. Keeps the row's blend in rowBlend, after
 * first rebuilding rotMtx. The last two arguments are unused.
 */
void Em_SetMotionRow(Em *em, s32 row, s32 unused2, s32 unused3)
{
    s32 i;
    EmMotionRow *rows = gEmMotionRows[em->type];

    Mtx_FromAnglesYXZ(&em->rotX, (f32 *) em->rotMtx);
    for (i = 0; i < em->motionLayerCount; i++) {
        switch (rows[row].layers) {
        case -1:
            break;
        case 0:
        case 1:
        case 2:
            if (i != rows[row].layers) {
                continue;
            }
            break;
        case 0x10:
            if (i == 2) {
                continue;
            }
            break;
        case 0x11:
            if (i == 1) {
                continue;
            }
            break;
        case 0x12:
            if (i == 0) {
                continue;
            }
            break;
        }
        em->motionId[i] = rows[row].motion + 1000 + i * 200;
        em->motionBlend[i] = 0;
        em->motionFrame[i] = 0;
        Player_BeginLayerMotion((Player *)em, em->motionFrame[i], em->motionBlend[i], i);
    }
    em->rowBlend = rows[row].nextBlend;
}

/*
 * Returns 1 when the two kinds belong to one family (the same
 * MonsterGroup_First).
 */
s32 Em_KindMatches(u8 kindA, u8 kindB)
{
    u8 groupA = MonsterGroup_First(kindA);
    u8 groupB = MonsterGroup_First(kindB);

    return groupA == groupB;
}

/*
 * Sets areaTimer1 from a list of {area, value} pairs: the value for the
 * monster's area (an id of 10000 or more matches area + 10000 exactly, a
 * smaller one by Area_DayId). With no match it uses the value of the -1 entry
 * that ends the list.
 */
void Em_SetAreaTimer1(Em *em, EmIdPair *pair)
{
    s16 id = pair->id;

    while (id != -1) {
        if (id >= 10000) {
            if (em->area == id - 10000) {
                em->areaTimer1 = pair->value;
                return;
            }
        } else if (Area_DayId(em->area) == Area_DayId(id)) {
            em->areaTimer1 = pair->value;
            return;
        }
        pair++;
        id = pair->id;
    }
    em->areaTimer1 = pair->value;
}

/* As Em_SetAreaTimer1, for areaTimer2. */
void Em_SetAreaTimer2(Em *em, EmIdPair *pair)
{
    s16 id = pair->id;

    while (id != -1) {
        if (id >= 10000) {
            if (em->area == id - 10000) {
                em->areaTimer2 = pair->value;
                return;
            }
        } else if (Area_DayId(em->area) == Area_DayId(id)) {
            em->areaTimer2 = pair->value;
            return;
        }
        pair++;
        id = pair->id;
    }
    em->areaTimer2 = pair->value;
}

/*
 * Sets areaTimer2 and areaTimer1 from the kind's lists in gEmAreaTimer2Values
 * and gEmAreaTimer1Values. gm_sub's monster set-up and the monsters' area moves call it.
 */
void Em_ApplyAreaValues(Em *em)
{
    Em_SetAreaTimer2(em, gEmAreaTimer2Values[em->type]);
    Em_SetAreaTimer1(em, gEmAreaTimer1Values[em->type]);
}

/*
 * Returns 1 for Yian Kut-Ku, Blue Yian Kut-Ku, Cephadrome, Gypceros, Purple
 * Gypceros, Basarios, Khezu, Red Khezu and Yian Garuga. For these,
 * PartFx_InitForEm's type 7 effect with mode 2 makes RathalosAttack 0x2F
 * instead of 0xE.
 */
s32 Em_TypeUsesK14(u8 type)
{
    switch (type) {
    case 6:
    case 0x26:
    case 8:
    case 0x14:
    case 0x27:
    case 0x16:
    case 0xF:
    case 0x2D:
    case 0x28:
        return 1;
    }
    return 0;
}

/*
 * Ends the rage: clears rageTimer, rage, rageBuildup, ragePending and
 * rageStarted. Sets attackFactor and damageFactor back to the level's values
 * (Em_LevelAttackFactor, Em_LevelDamageFactor) and motionSpeed to 1.
 */
void Em_EndRage(Em *em)
{
    em->rageTimer = 0;
    em->rage = 0;
    em->rageBuildup = 0;
    em->attackFactor = Em_LevelAttackFactor(em);
    em->damageFactor = Em_LevelDamageFactor(em);
    em->motionSpeed = 1.0f;
    em->ragePending = 0;
    em->rageStarted = 0;
}

/*
 * Counts scriptDelay down. Once it is 0, runs the script once (EmScript_Run)
 * if scriptPending is set, then clears scriptPending.
 */
void Em_UpdatePendingScript(Em *em)
{
    if (em->scriptDelay <= 0) {
        if (em->scriptPending != 0) {
            EmScript_Run(em);
            em->scriptPending = 0;
        }
    } else {
        em->scriptDelay--;
    }
}

/*
 * The per-frame reset: hitReactMode to EM_HIT_REACT_FLINCH and stateArmor to
 * 0.
 */
void Em_ClearHitReactMode(Em *em)
{
    Em_SetHitReactMode(em, 0);
    em->stateArmor = 0;
}

/* Clears blindTimer (the flash blindness) and poisonDamage. */
void Em_ClearPoison(Em *em)
{
    em->blindTimer = 0;
    em->poisonDamage = 0;
}

/*
 * Holds the monster in the second hold (holdTimer2 = 5, the hold
 * PlacedItem_RunTrap checks).
 */
void Em_SetHold2(Em *em)
{
    em->holdTimer2 = 5;
}

/*
 * Called each frame for a captured or held monster. Sets 5 frames unhittable
 * and unpushable, EM_HIT_REACT_WAKE and blind 2, and sets holdTimer (`a` 0)
 * or holdTimer2 (`a` 1) to 5.
 */
void Em_SetCaptured(Em *em, s32 useHold2)
{
    em->noHitTimer = 5;
    em->noPushTimer = 5;
    Em_SetHitReactMode(em, 2);
    em->blind = 2;
    if (useHold2 == 0) {
        em->holdTimer = 5;
    } else {
        em->holdTimer2 = 5;
    }
}

/*
 * Plays sound `se` at bone `bone` through Em_PlaySeType when the motion
 * passes `frame`. While layer `a`'s motion is between frame and frame2, it
 * plays it again every 4 frames with last argument 1.
 */
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 endFrame, s32 se, s32 bone, s32 vol, s32 layer)
{
    f32 pos[4];

    if (EmMotion_PassedFrameScaled(em, frame, 0)) {
        Mtx_GetTrans(pos, em->model->bones[bone].world);
        Em_PlaySeType(em, se, 0, pos, vol, 0);
    }
    if (EmMotion_InFrameRange(em, layer, frame, endFrame) && gScene.frame % 4 == 0) {
        Mtx_GetTrans(pos, em->model->bones[bone].world);
        Em_PlaySeType(em, se, 0, pos, vol, 1);
    }
}

/* Plays sound effect `se` at the position of bone `bone`. */
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone)
{
    f32 pos[4];
    s8 vol = gSeVolumeByType[em->type];

    Mtx_GetTrans(pos, em->model->bones[bone].world);
    Em_PlaySeChecked(em, se, 0, pos, vol, 0);
}

/* Em_PlaySeAtBone when the motion reaches `frame`. */
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone)
{
    if (EmMotion_PassedFrameScaled(em, frame, 0)) {
        Em_PlaySeAtBone(em, se, bone);
    }
}

/*
 * Plays sound `se` at bone `bone` through Em_PlaySeType, at the kind's volume
 * (gSeVolumeByType), when the motion passes `frame`.
 */
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone)
{
    f32 pos[4];

    if (EmMotion_PassedFrameScaled(em, frame, 0)) {
        s8 vol = gSeVolumeByType[em->type];

        Mtx_GetTrans(pos, em->model->bones[bone].world);
        Em_PlaySeType(em, se, 0, pos, vol, 0);
    }
}

/* Em_PlaySeAtBone when layer `flag`'s motion passes `frame`. */
void Em_PlaySeAtBoneOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 layer)
{
    f32 pos[4];

    if (EmMotion_PassedFrameScaled(em, frame, layer)) {
        s8 vol = gSeVolumeByType[em->type];

        Mtx_GetTrans(pos, em->model->bones[bone].world);
        Em_PlaySeChecked(em, se, 0, pos, vol, 0);
    }
}

/*
 * As Em_PlaySeAtBoneOnFrame, but through Sound_PlayEmSe (sound slot 1, bank
 * 0).
 */
void Em_PlayCommonSeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone)
{
    f32 pos[4];

    if (EmMotion_PassedFrameScaled(em, frame, 0)) {
        s8 vol = gSeVolumeByType[em->type];

        Mtx_GetTrans(pos, em->model->bones[bone].world);
        Sound_PlayEmSe(em, se, 0, pos, vol, 0);
    }
}

/* Em_PlaySeAtBone at volume vol instead of the kind's. */
void Em_PlaySeAtBoneVol(Em *em, s32 se, s32 bone, s32 vol)
{
    f32 pos[4];

    Mtx_GetTrans(pos, em->model->bones[bone].world);
    Em_PlaySeChecked(em, se, 0, pos, vol, 0);
}

/* Em_PlaySeAtBoneVol when the motion passes `frame`. */
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol)
{
    if (EmMotion_PassedFrameScaled(em, frame, 0)) {
        Em_PlaySeAtBoneVol(em, se, bone, vol);
    }
}

/* Em_PlaySeAtBoneVol when layer `flag`'s motion passes `frame`. */
void Em_PlaySeAtBoneVolOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 layer, s32 vol)
{
    f32 pos[4];

    if (EmMotion_PassedFrameScaled(em, frame, layer)) {
        Mtx_GetTrans(pos, em->model->bones[bone].world);
        Em_PlaySeChecked(em, se, 0, pos, vol, 0);
    }
}

/* Plays sound effect `se` at the monster's position. */
void Em_PlaySe(Em *em, s32 se)
{
    Em_PlaySeChecked(em, se, 0, &em->posX, gSeVolumeByType[em->type], 0);
}

/* Em_PlaySe when the motion passes `frame`. */
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se)
{
    if (EmMotion_PassedFrameScaled(em, frame, 0)) {
        Em_PlaySeChecked(em, se, 0, &em->posX, gSeVolumeByType[em->type], 0);
    }
}

/*
 * Returns 1 when the monster is small (large 0), `a` is nonzero, or the
 * monster is remote; returns 0 for a large local monster with `a` 0.
 * Blangonga's family counts its move actions only when it is 1.
 */
s32 Em_IsSyncFree(Em *em, u8 mode)
{
    if (em->large == 0 || mode != 0 || em->remote != 0) {
        return 1;
    }
    return 0;
}

/*
 * The file's data: the per-area tables the monsters'
 * area lookups read (em->tables, Em_AreaEntryValue, Em_FindAreaEntry):
 * ground heights, point sets, four-point sets and route sets; then
 * Em_PartEffectTbl.
 */
typedef struct MapRoutePt {
    f32 x, y, z;
    f32 r;
    s32 flag;
} MapRoutePt;
typedef struct MapRoute {
    s16 id;
    u16 n;
    MapRoutePt *pts;
    u32 pad[4];
} MapRoute;
/* {id, count, the count's positions (Vec3)}, up to {-1, 0, NULL} */
typedef struct MapPosSet {
    s16 id;
    u16 n;
    f32 *pos;
} MapPosSet;
typedef struct MapRouteSet {
    s16 id;
    u16 n;
    MapRoute *routes;
} MapRouteSet;
extern f32 gEmGoalPosSets_119[];
extern f32 gEmKindTables_68_2_72[];
extern MapRoute D_008C30C0[];
f32 gAreaGroundY[200] = {
#include "game/008BF9C0.inc"
};
f32 gEmKindTables_68_0_4[4] = {
#include "game/008BFCE0.inc"
};
f32 gEmKindTables_68_0_5[4] = {
#include "game/008BFCF0.inc"
};
f32 gEmKindTables_68_0_6[4] = {
#include "game/008BFD00.inc"
};
f32 gEmKindTables_68_0_7[4] = {
#include "game/008BFD10.inc"
};
f32 gEmKindTables_68_0_8[16] = {
#include "game/008BFD20.inc"
};
f32 gEmKindTables_68_0_9[12] = {
#include "game/008BFD60.inc"
};
f32 gEmKindTables_68_0_10[4] = {
#include "game/008BFD90.inc"
};
f32 gEmKindTables_68_0_11[4] = {
#include "game/008BFDA0.inc"
};
f32 gEmKindTables_68_0_14[4] = {
#include "game/008BFDB0.inc"
};
f32 gEmGoalPosSets_123[4] = {
#include "game/008BFDC0.inc"
};
f32 gEmKindTables_68_0_18[4] = {
#include "game/008BFDD0.inc"
};
f32 gEmKindTables_68_0_19[46] = {
#include "game/008BFDE0.inc"
};
f32 gEmKindTables_68_0_21[3] = {
#include "game/008BFE98.inc"
};
f32 gEmKindTables_68_0_22[18] = {
#include "game/008BFEB0.inc"
};
f32 gEmKindTables_68_0_23[3] = {
#include "game/008BFEF8.inc"
};
f32 gEmKindTables_68_0_24[16] = {
#include "game/008BFF10.inc"
};
f32 gEmKindTables_68_0_25[4] = {
#include "game/008BFF50.inc"
};
f32 gEmGoalPosSets_124[76] = {
#include "game/008BFF60.inc"
};
f32 gEmGoalPosSets_125[4] = {
#include "game/008C0090.inc"
};
f32 gEmGoalPosSets_126[8] = {
#include "game/008C00A0.inc"
};
f32 gEmGoalPosSets_127[12] = {
#include "game/008C00C0.inc"
};
f32 gEmGoalPosSets_128[12] = {
#include "game/008C00F0.inc"
};
f32 gEmGoalPosSets_129[22] = {
#include "game/008C0120.inc"
};
f32 gEmGoalPosSets_130[3] = {
#include "game/008C0178.inc"
};
f32 gEmGoalPosSets_131[8] = {
#include "game/008C0190.inc"
};
f32 gEmGoalPosSets_132[12] = {
#include "game/008C01B0.inc"
};
f32 gEmGoalPosSets_133[6] = {
#include "game/008C01E0.inc"
};
f32 gEmGoalPosSets_134[3] = {
#include "game/008C01F8.inc"
};
f32 gEmGoalPosSets_135[6] = {
#include "game/008C0210.inc"
};
f32 gEmKindTables_68_0_47[3] = {
#include "game/008C0228.inc"
};
f32 gEmKindTables_68_0_65[8] = {
#include "game/008C0240.inc"
};
f32 gEmKindTables_68_0_48[10] = {
#include "game/008C0260.inc"
};
f32 gEmKindTables_68_0_56[3] = {
#include "game/008C0288.inc"
};
f32 gEmKindTables_68_0_68[12] = {
#include "game/008C02A0.inc"
};
f32 gEmKindTables_68_0_69[34] = {
#include "game/008C02D0.inc"
};
f32 gEmKindTables_68_0_70[3] = {
#include "game/008C0358.inc"
};
f32 gEmKindTables_68_0_71[3] = {
#include "game/008C0368.inc"
};
f32 gEmKindTables_68_0_72[12] = {
#include "game/008C0380.inc"
};
f32 gEmKindTables_68_0_73[10] = {
#include "game/008C03B0.inc"
};
f32 gEmKindTables_68_0_0[3] = {
#include "game/008C03D8.inc"
};
f32 gEmKindTables_68_0_1[3] = {
#include "game/008C03E8.inc"
};
f32 gEmKindTables_68_0_2[10] = {
#include "game/008C0400.inc"
};
f32 gEmKindTables_68_0_3[3] = {
#include "game/008C0428.inc"
};
f32 gEmKindTables_68_0_12[3] = {
#include "game/008C0438.inc"
};
f32 gEmKindTables_68_0_13[12] = {
#include "game/008C0450.inc"
};
f32 gEmKindTables_68_0_16[4] = {
#include "game/008C0480.inc"
};
f32 gEmKindTables_68_0_17[4] = {
#include "game/008C0490.inc"
};
f32 gEmKindTables_68_0_20[4] = {
#include "game/008C04A0.inc"
};
f32 gEmKindTables_68_0_89[4] = {
#include "game/008C04B0.inc"
};
f32 gEmGoalPosSets_84[4] = {
#include "game/008C04C0.inc"
};
f32 gEmGoalPosSets_85[10] = {
#include "game/008C04D0.inc"
};
f32 gEmGoalPosSets_86[3] = {
#include "game/008C04F8.inc"
};
f32 gEmGoalPosSets_87[3] = {
#include "game/008C0508.inc"
};
f32 gEmGoalPosSets_88[10] = {
#include "game/008C0520.inc"
};
f32 gEmGoalPosSets_89[3] = {
#include "game/008C0548.inc"
};
f32 gEmGoalPosSets_90[3] = {
#include "game/008C0558.inc"
};
f32 gEmGoalPosSets_92[3] = {
#include "game/008C0568.inc"
};
f32 gEmGoalPosSets_93[3] = {
#include "game/008C0578.inc"
};
f32 gEmGoalPosSets_94[10] = {
#include "game/008C0590.inc"
};
f32 gEmGoalPosSets_95[3] = {
#include "game/008C05B8.inc"
};
f32 gEmGoalPosSets_96[3] = {
#include "game/008C05C8.inc"
};
f32 gEmGoalPosSets_97[3] = {
#include "game/008C05D8.inc"
};
f32 gEmGoalPosSets_98[3] = {
#include "game/008C05E8.inc"
};
f32 gEmGoalPosSets_99[10] = {
#include "game/008C0600.inc"
};
f32 gEmGoalPosSets_100[3] = {
#include "game/008C0628.inc"
};
f32 gEmGoalPosSets_101[3] = {
#include "game/008C0638.inc"
};
f32 gEmGoalPosSets_102[3] = {
#include "game/008C0648.inc"
};
f32 gEmGoalPosSets_103[3] = {
#include "game/008C0658.inc"
};
f32 gEmGoalPosSets_104[3] = {
#include "game/008C0668.inc"
};
f32 gEmGoalPosSets_105[3] = {
#include "game/008C0678.inc"
};
f32 gEmGoalPosSets_106[3] = {
#include "game/008C0688.inc"
};
f32 gEmGoalPosSets_107[3] = {
#include "game/008C0698.inc"
};
f32 gEmGoalPosSets_108[3] = {
#include "game/008C06A8.inc"
};
f32 gEmGoalPosSets_109[3] = {
#include "game/008C06B8.inc"
};
f32 gEmGoalPosSets_110[3] = {
#include "game/008C06C8.inc"
};
f32 gEmGoalPosSets_111[3] = {
#include "game/008C06D8.inc"
};
f32 gEmGoalPosSets_112[3] = {
#include "game/008C06E8.inc"
};
f32 gEmGoalPosSets_113[3] = {
#include "game/008C06F8.inc"
};
f32 gEmGoalPosSets_114[3] = {
#include "game/008C0708.inc"
};
f32 gEmGoalPosSets_115[3] = {
#include "game/008C0718.inc"
};
f32 gEmGoalPosSets_116[3] = {
#include "game/008C0728.inc"
};
f32 gEmGoalPosSets_117[18] = {
#include "game/008C0740.inc"
};
f32 gEmGoalPosSets_118[3] = {
#include "game/008C0788.inc"
};
f32 gEmGoalPosSets_119[8] = {
#include "game/008C07A0.inc"
};
MapPosSet gEmKindTables_68_0[143] = {
#include "game/008C07C0.inc"
};
f32 gEmGoalPosSets_4[3] = {
#include "game/008C0C38.inc"
};
f32 gEmGoalPosSets_5[3] = {
#include "game/008C0C48.inc"
};
f32 gEmGoalPosSets_6[3] = {
#include "game/008C0C58.inc"
};
f32 gEmGoalPosSets_7[3] = {
#include "game/008C0C68.inc"
};
f32 gEmGoalPosSets_8[3] = {
#include "game/008C0C78.inc"
};
f32 gEmGoalPosSets_9[3] = {
#include "game/008C0C88.inc"
};
f32 gEmGoalPosSets_10[3] = {
#include "game/008C0C98.inc"
};
f32 gEmGoalPosSets_11[3] = {
#include "game/008C0CA8.inc"
};
f32 gEmGoalPosSets_12[3] = {
#include "game/008C0CB8.inc"
};
f32 gEmGoalPosSets_15[3] = {
#include "game/008C0CC8.inc"
};
f32 gEmGoalPosSets_16[3] = {
#include "game/008C0CD8.inc"
};
f32 gEmGoalPosSets_19[3] = {
#include "game/008C0CE8.inc"
};
f32 gEmGoalPosSets_20[3] = {
#include "game/008C0CF8.inc"
};
f32 gEmGoalPosSets_22[3] = {
#include "game/008C0D08.inc"
};
f32 gEmGoalPosSets_23[3] = {
#include "game/008C0D18.inc"
};
f32 gEmGoalPosSets_24[3] = {
#include "game/008C0D28.inc"
};
f32 gEmGoalPosSets_25[3] = {
#include "game/008C0D38.inc"
};
f32 gEmGoalPosSets_26[3] = {
#include "game/008C0D48.inc"
};
f32 gEmGoalPosSets_27[3] = {
#include "game/008C0D58.inc"
};
f32 gEmGoalPosSets_28[3] = {
#include "game/008C0D68.inc"
};
f32 gEmGoalPosSets_29[3] = {
#include "game/008C0D78.inc"
};
f32 gEmGoalPosSets_30[3] = {
#include "game/008C0D88.inc"
};
f32 gEmGoalPosSets_31[3] = {
#include "game/008C0D98.inc"
};
f32 gEmGoalPosSets_32[3] = {
#include "game/008C0DA8.inc"
};
f32 gEmGoalPosSets_33[3] = {
#include "game/008C0DB8.inc"
};
f32 gEmGoalPosSets_34[3] = {
#include "game/008C0DC8.inc"
};
f32 gEmGoalPosSets_35[3] = {
#include "game/008C0DD8.inc"
};
f32 gEmGoalPosSets_36[3] = {
#include "game/008C0DE8.inc"
};
f32 gEmGoalPosSets_37[3] = {
#include "game/008C0DF8.inc"
};
f32 gEmGoalPosSets_38[3] = {
#include "game/008C0E08.inc"
};
f32 gEmGoalPosSets_39[3] = {
#include "game/008C0E18.inc"
};
f32 gEmGoalPosSets_40[3] = {
#include "game/008C0E28.inc"
};
f32 gEmGoalPosSets_41[3] = {
#include "game/008C0E38.inc"
};
f32 gEmGoalPosSets_42[3] = {
#include "game/008C0E48.inc"
};
f32 gEmGoalPosSets_43[3] = {
#include "game/008C0E58.inc"
};
f32 gEmGoalPosSets_44[3] = {
#include "game/008C0E68.inc"
};
f32 gEmGoalPosSets_45[3] = {
#include "game/008C0E78.inc"
};
f32 gEmGoalPosSets_46[3] = {
#include "game/008C0E88.inc"
};
f32 gEmGoalPosSets_47[3] = {
#include "game/008C0E98.inc"
};
f32 gEmGoalPosSets_48[3] = {
#include "game/008C0EA8.inc"
};
f32 gEmGoalPosSets_49[3] = {
#include "game/008C0EB8.inc"
};
f32 gEmGoalPosSets_50[3] = {
#include "game/008C0EC8.inc"
};
f32 gEmGoalPosSets_51[3] = {
#include "game/008C0ED8.inc"
};
f32 gEmGoalPosSets_52[3] = {
#include "game/008C0EE8.inc"
};
f32 gEmGoalPosSets_53[3] = {
#include "game/008C0EF8.inc"
};
f32 gEmGoalPosSets_54[3] = {
#include "game/008C0F08.inc"
};
f32 gEmGoalPosSets_55[3] = {
#include "game/008C0F18.inc"
};
f32 gEmGoalPosSets_56[3] = {
#include "game/008C0F28.inc"
};
f32 gEmGoalPosSets_57[3] = {
#include "game/008C0F38.inc"
};
f32 gEmGoalPosSets_58[3] = {
#include "game/008C0F48.inc"
};
f32 gEmGoalPosSets_59[3] = {
#include "game/008C0F58.inc"
};
f32 gEmGoalPosSets_60[3] = {
#include "game/008C0F68.inc"
};
f32 gEmGoalPosSets_61[3] = {
#include "game/008C0F78.inc"
};
f32 gEmGoalPosSets_62[3] = {
#include "game/008C0F88.inc"
};
f32 gEmGoalPosSets_63[3] = {
#include "game/008C0F98.inc"
};
f32 gEmGoalPosSets_64[3] = {
#include "game/008C0FA8.inc"
};
f32 gEmGoalPosSets_65[3] = {
#include "game/008C0FB8.inc"
};
f32 gEmGoalPosSets_66[3] = {
#include "game/008C0FC8.inc"
};
f32 gEmGoalPosSets_67[3] = {
#include "game/008C0FD8.inc"
};
f32 gEmGoalPosSets_68[3] = {
#include "game/008C0FE8.inc"
};
f32 gEmGoalPosSets_69[3] = {
#include "game/008C0FF8.inc"
};
f32 gEmGoalPosSets_70[3] = {
#include "game/008C1008.inc"
};
f32 gEmGoalPosSets_71[3] = {
#include "game/008C1018.inc"
};
f32 gEmGoalPosSets_72[3] = {
#include "game/008C1028.inc"
};
f32 gEmGoalPosSets_0[3] = {
#include "game/008C1038.inc"
};
f32 gEmGoalPosSets_1[3] = {
#include "game/008C1048.inc"
};
f32 gEmGoalPosSets_2[3] = {
#include "game/008C1058.inc"
};
f32 gEmGoalPosSets_3[3] = {
#include "game/008C1068.inc"
};
f32 gEmGoalPosSets_13[3] = {
#include "game/008C1078.inc"
};
f32 gEmGoalPosSets_14[3] = {
#include "game/008C1088.inc"
};
f32 gEmGoalPosSets_17[3] = {
#include "game/008C1098.inc"
};
f32 gEmGoalPosSets_18[12] = {
#include "game/008C10B0.inc"
};
f32 gEmGoalPosSets_21[4] = {
#include "game/008C10E0.inc"
};
f32 gEmGoalPosSets_83[4] = {
#include "game/008C10F0.inc"
};
MapPosSet gEmGoalPosSets[137] = {
#include "game/008C1100.inc"
};
f32 gEmKindTables_68_2_7[12] = {
#include "game/008C1550.inc"
};
f32 gEmKindTables_68_2_8[12] = {
#include "game/008C1580.inc"
};
f32 gEmKindTables_68_2_9[12] = {
#include "game/008C15B0.inc"
};
f32 gEmKindTables_68_2_10[12] = {
#include "game/008C15E0.inc"
};
f32 gEmKindTables_68_2_11[12] = {
#include "game/008C1610.inc"
};
f32 gEmKindTables_68_2_12[12] = {
#include "game/008C1640.inc"
};
f32 gEmKindTables_68_2_13[12] = {
#include "game/008C1670.inc"
};
f32 gEmKindTables_68_2_30[12] = {
#include "game/008C16A0.inc"
};
f32 gEmKindTables_68_2_31[12] = {
#include "game/008C16D0.inc"
};
f32 gEmKindTables_68_2_32[12] = {
#include "game/008C1700.inc"
};
f32 gEmKindTables_68_2_33[12] = {
#include "game/008C1730.inc"
};
f32 gEmKindTables_68_2_34[12] = {
#include "game/008C1760.inc"
};
f32 gEmKindTables_68_2_35[12] = {
#include "game/008C1790.inc"
};
f32 gEmKindTables_68_2_0[12] = {
#include "game/008C17C0.inc"
};
f32 gEmKindTables_68_2_1[12] = {
#include "game/008C17F0.inc"
};
f32 gEmKindTables_68_2_2[12] = {
#include "game/008C1820.inc"
};
f32 gEmKindTables_68_2_3[12] = {
#include "game/008C1850.inc"
};
f32 gEmKindTables_68_2_4[12] = {
#include "game/008C1880.inc"
};
f32 gEmKindTables_68_2_5[12] = {
#include "game/008C18B0.inc"
};
f32 gEmKindTables_68_2_6[12] = {
#include "game/008C18E0.inc"
};
f32 gEmKindTables_68_2_48[12] = {
#include "game/008C1910.inc"
};
f32 gEmKindTables_68_2_49[12] = {
#include "game/008C1940.inc"
};
f32 gEmKindTables_68_2_50[12] = {
#include "game/008C1970.inc"
};
f32 gEmKindTables_68_2_51[12] = {
#include "game/008C19A0.inc"
};
f32 gEmKindTables_68_2_52[12] = {
#include "game/008C19D0.inc"
};
f32 gEmKindTables_68_2_53[12] = {
#include "game/008C1A00.inc"
};
f32 gEmKindTables_68_2_54[12] = {
#include "game/008C1A30.inc"
};
f32 gEmKindTables_68_2_15[12] = {
#include "game/008C1A60.inc"
};
f32 gEmKindTables_68_2_16[12] = {
#include "game/008C1A90.inc"
};
f32 gEmKindTables_68_2_17[12] = {
#include "game/008C1AC0.inc"
};
f32 gEmKindTables_68_2_18[12] = {
#include "game/008C1AF0.inc"
};
f32 gEmKindTables_68_2_19[12] = {
#include "game/008C1B20.inc"
};
f32 gEmKindTables_68_2_20[12] = {
#include "game/008C1B50.inc"
};
f32 gEmKindTables_68_2_14[12] = {
#include "game/008C1B80.inc"
};
f32 gEmKindTables_68_2_25[12] = {
#include "game/008C1BB0.inc"
};
f32 gEmKindTables_68_2_26[12] = {
#include "game/008C1BE0.inc"
};
f32 gEmKindTables_68_2_27[12] = {
#include "game/008C1C10.inc"
};
f32 gEmKindTables_68_2_28[12] = {
#include "game/008C1C40.inc"
};
f32 gEmKindTables_68_2_29[12] = {
#include "game/008C1C70.inc"
};
f32 gEmKindTables_68_2_21[12] = {
#include "game/008C1CA0.inc"
};
f32 gEmKindTables_68_2_22[12] = {
#include "game/008C1CD0.inc"
};
f32 gEmKindTables_68_2_23[12] = {
#include "game/008C1D00.inc"
};
f32 gEmKindTables_68_2_24[12] = {
#include "game/008C1D30.inc"
};
f32 gEmKindTables_68_2_71[12] = {
#include "game/008C1D60.inc"
};
f32 gEmKindTables_68_2_72[12] = {
#include "game/008C1D90.inc"
};
MapPosSet gEmKindTables_68_2[76] = {
#include "game/008C1DC0.inc"
};
f32 EmCephadrome_VelYCurves_3[60] = {
#include "game/008C2020.inc"
};
MapRoutePt D_008C2110[6] = {
#include "game/008C2110.inc"
};
MapRoute D_008C2190[1] = {
#include "game/008C2190.inc"
};
MapRoutePt D_008C21B0[6] = {
#include "game/008C21B0.inc"
};
MapRoute D_008C2230[1] = {
#include "game/008C2230.inc"
};
MapRoutePt D_008C2250[5] = {
#include "game/008C2250.inc"
};
MapRoute D_008C22C0[1] = {
#include "game/008C22C0.inc"
};
MapRoutePt D_008C22E0[6] = {
#include "game/008C22E0.inc"
};
MapRoute D_008C2360[1] = {
#include "game/008C2360.inc"
};
MapRoutePt D_008C2380[6] = {
#include "game/008C2380.inc"
};
MapRoutePt D_008C2400[8] = {
#include "game/008C2400.inc"
};
MapRoute D_008C24A0[2] = {
#include "game/008C24A0.inc"
};
MapRoutePt D_008C24D0[6] = {
#include "game/008C24D0.inc"
};
MapRoute D_008C2550[1] = {
#include "game/008C2550.inc"
};
MapRoutePt D_008C2570[6] = {
#include "game/008C2570.inc"
};
MapRoute D_008C25F0[1] = {
#include "game/008C25F0.inc"
};
MapRoutePt D_008C2610[6] = {
#include "game/008C2610.inc"
};
MapRoute D_008C2690[1] = {
#include "game/008C2690.inc"
};
MapRoutePt D_008C26B0[5] = {
#include "game/008C26B0.inc"
};
MapRoute D_008C2720[1] = {
#include "game/008C2720.inc"
};
MapRoutePt D_008C2740[6] = {
#include "game/008C2740.inc"
};
MapRoute D_008C27C0[1] = {
#include "game/008C27C0.inc"
};
MapRoutePt D_008C27E0[4] = {
#include "game/008C27E0.inc"
};
MapRoute D_008C2830[1] = {
#include "game/008C2830.inc"
};
MapRoutePt D_008C2850[3] = {
#include "game/008C2850.inc"
};
MapRoutePt D_008C2890[3] = {
#include "game/008C2890.inc"
};
MapRoute D_008C28D0[2] = {
#include "game/008C28D0.inc"
};
MapRoutePt D_008C2900[4] = {
#include "game/008C2900.inc"
};
MapRoute D_008C2950[1] = {
#include "game/008C2950.inc"
};
MapRoutePt D_008C2970[7] = {
#include "game/008C2970.inc"
};
MapRoutePt D_008C2A00[3] = {
#include "game/008C2A00.inc"
};
MapRoute D_008C2A40[2] = {
#include "game/008C2A40.inc"
};
MapRoutePt D_008C2A70[3] = {
#include "game/008C2A70.inc"
};
MapRoutePt D_008C2AB0[3] = {
#include "game/008C2AB0.inc"
};
MapRoute D_008C2AF0[2] = {
#include "game/008C2AF0.inc"
};
MapRoutePt D_008C2B20[6] = {
#include "game/008C2B20.inc"
};
MapRoute D_008C2BA0[1] = {
#include "game/008C2BA0.inc"
};
MapRoutePt D_008C2BC0[6] = {
#include "game/008C2BC0.inc"
};
MapRoute D_008C2C40[1] = {
#include "game/008C2C40.inc"
};
MapRoutePt D_008C2C60[4] = {
#include "game/008C2C60.inc"
};
MapRoute D_008C2CB0[1] = {
#include "game/008C2CB0.inc"
};
MapRoutePt D_008C2CD0[4] = {
#include "game/008C2CD0.inc"
};
MapRoute D_008C2D20[1] = {
#include "game/008C2D20.inc"
};
MapRoutePt D_008C2D40[5] = {
#include "game/008C2D40.inc"
};
MapRoute D_008C2DB0[1] = {
#include "game/008C2DB0.inc"
};
MapRoutePt D_008C2DD0[7] = {
#include "game/008C2DD0.inc"
};
MapRoute D_008C2E60[1] = {
#include "game/008C2E60.inc"
};
MapRoutePt D_008C2E80[6] = {
#include "game/008C2E80.inc"
};
MapRoute D_008C2F00[1] = {
#include "game/008C2F00.inc"
};
MapRoutePt D_008C2F20[6] = {
#include "game/008C2F20.inc"
};
MapRoute D_008C2FA0[1] = {
#include "game/008C2FA0.inc"
};
MapRoutePt D_008C2FC0[5] = {
#include "game/008C2FC0.inc"
};
MapRoute D_008C3030[1] = {
#include "game/008C3030.inc"
};
MapRoutePt D_008C3050[5] = {
#include "game/008C3050.inc"
};
MapRoute D_008C30C0[1] = {
#include "game/008C30C0.inc"
};
MapRouteSet gEmRouteSets[26] = {
#include "game/008C30E0.inc"
};
EmPartEffect Em_PartEffectTbl[42] = {
#include "game/008C31B0.inc"
};
