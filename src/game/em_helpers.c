/*
 * Monster helpers: area and player checks, hit points (Em_ComputeMaxHp,
 * Em_RegenHp), the level's attack and damage factors, the trap hold time,
 * motion rows, the per-area timers, and sound effects played at a bone when
 * the motion reaches a frame. Then the monsters' movement: flight steering
 * (Em_UpdateFlight), turning toward the target player or the goal
 * (Em_TurnToTargetMotion, Em_TurnToGoal, and the turning motions of
 * Em_StartTurn and Em_TurnStep), the walk to the goal (Em_StepToGoal), and
 * the search for a monster of a kind nearby (EmList_FindKindNear). The
 * .data holds each area's ground height (gAreaGroundY), the area point sets
 * (gEmKindTables_68_0, gEmGoalPosSets, gEmKindTables_68_2), the route sets
 * (gEmRouteSets) and Em_PartEffectTbl.
 */

#include "quest.h"
#include "save.h"
#include "em.h"
#include "player.h"
#include "capsule.h"
#include "area.h"
#include "engine_math.h"
#include "gm_em.h"
#include "hit_spark.h"
#include "vec_math.h"
#include "em_script.h"
#include "sound.h"

extern EmAreaValue *gEmAreaTimer2Values[];
extern EmAreaValue *gEmAreaTimer1Values[];
extern s8 gSeVolumeByType[];

/* Effects shown when a part record changes state (20 bytes). */
typedef struct EmPartEffect {
    u8 kind;            /* 0x00: compared with Em.type through Em_KindMatches */
    u8 part;            /* 0x01: index into Em.parts */
    u8 state;           /* 0x02: matched with the state of that part */
    u8 flag;            /* 0x03: 1 sets Em.hitzoneSet */
    s16 bone;           /* 0x04: -1 when there is no effect */
    u16 breakPart;      /* 0x06: the part Em_BreakPart breaks; 0xFFFF none */
    f32 offset[3];      /* 0x08: from the bone */
} EmPartEffect;

extern EmPartEffect Em_PartEffectTbl[42];

void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
s32 Player_UpdateMotion(struct Player *em);
s32 EmMotion_PassedFrameScaled(void *em, f32 frame, u8 a); /* unit's own declaration: with the definition's s32 (Player *p, f32 frame, s32 layer) (declared with a u8 layer as the two layer-frame sound functions pass it; with the definition's s32 their calls compile differently) */
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
EmBone *Em_Bone(void *obj, s16 bone);
void Em_SetAreaTimer1(Em *em, EmAreaValue *pair);
void Em_SetAreaTimer2(Em *em, EmAreaValue *pair);
AreaRecord *Stage_GetAreaRecord(s32 area); /* unit's own declaration: the area declared s32; with the definition's u16 the callers would mask it to 16 bits first (Em_UpdateFlight), which the game's code doesn't */
f32 Vec_Distance(f32 *a, f32 *b);

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

        if (noteIndex != MONSTER_NOTE_NONE) {
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
 * Sets trapTimer, how long a trap holds the monster. EM_KIND_RAJANG gets 156
 * frames. Otherwise it is 832 when the monster was out of combat before the
 * hit (*p is Em_UpdateDamage's out), 520 when calm, and 312 when enraged.
 * The families call it on EM_DMG_ATTACK9 and EM_DMG_ATTACK6, before their
 * trapped state.
 */
void Em_SetTrapTime(Em *em, u8 *wasInCombat)
{
    if (em->type == EM_KIND_RAJANG) {
        em->trapTimer = 156;
    } else if (*wasInCombat == 0) {
        em->trapTimer = 832;
    } else if (em->rage == 0) {
        em->trapTimer = 520;
    } else {
        em->trapTimer = 312;
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
        for (j = 0; j < 42; j++) {
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
 * set, mesh 1 hidden. Unless layer 0 already plays it, it sets motion 83 on
 * layers 0-2 and rebuilds the rotation matrix, the motion and the bone
 * matrices. Then it places its tail object (tailObj, EmTail_Drop).
 */
void Em_ResetToBasePose(Em *em)
{
    struct StageObj *p = em->tailObj;

    em->stance = 0;
    em->fxFlags |= 1;
    em->meshShown[1] = 0;
    if (em->motionId[0] != 1083) {
        Em_SetLayerMotion(em, 1083, 0, 0, 0);
        Em_SetLayerMotion(em, 1283, 0, 0, 1);
        Em_SetLayerMotion(em, 1483, 0, 0, 2);
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
 * with no blend, on the layers the row names: -1 all; 0-2 that one; 16,
 * 17, 18 all but layer 2, 1, 0. Keeps the row's blend in rowBlend, after
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
        case 16:
            if (i == 2) {
                continue;
            }
            break;
        case 17:
            if (i == 1) {
                continue;
            }
            break;
        case 18:
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
void Em_SetAreaTimer1(Em *em, EmAreaValue *pair)
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
void Em_SetAreaTimer2(Em *em, EmAreaValue *pair)
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
 * PartFx_InitForEm's type 7 effect with mode 2 makes RathalosAttack 47
 * instead of 14.
 */
s32 Em_TypeUsesK14(u8 type)
{
    switch (type) {
    case 6:
    case 38:
    case 8:
    case 20:
    case 39:
    case 22:
    case 15:
    case 45:
    case 40:
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

/* Flight steering (em_chameleos calls it with EmChameleos_FlyParam): banks (em->rotZ) toward the
 * angle em->aimAngle (copied to em->flightTurn) and turns the heading (em->rotY) by the bank, keeps
 * the height over the ground (Stage_GetAreaRecord) and picks the flight motions from fly by the
 * bank. Flag bits skip parts: 2 the bank-scaled turn, 4 the height, 8 the motions, 0x10 the
 * return of em->rotX to 0 (by fly->roll a frame). */
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *fly)
{
    AreaRecord *ground;
    s32 k;
    s32 sel;
    s32 turnSize;
    s32 bank;
    s32 step;
    s32 bankHigh;
    s32 bankLow;
    f32 sink;
    f32 minHeight;

    ground = Stage_GetAreaRecord(em->area);
    em->flightTurn = em->aimAngle;
    k = !(flags & 0x20);
    turnSize = em->flightTurn;
    if (turnSize >= 0x8000) {
        turnSize = 0x10000 - turnSize;
    }
    switch (k) {
    case 0:
        if (turnSize <= fly->lim0) {
            sel = fly->a0;
        } else if (turnSize <= fly->lim1) {
            sel = fly->a1;
        } else if (turnSize <= fly->lim2) {
            sel = fly->a2;
        } else if (turnSize <= fly->lim3) {
            sel = fly->a3;
        }
        if (em->velZ <= 130.0f) {
            break;
        }
    case 1:
        if (turnSize <= fly->lim0) {
            sel = fly->b0;
        } else if (turnSize <= fly->lim1) {
            sel = fly->b1;
        } else if (turnSize <= fly->lim2) {
            sel = fly->b2;
        } else if (turnSize <= fly->lim3) {
            sel = fly->b3;
        }
        break;
    }
    if (em->flightTurn != 0) {
        if (em->flightTurn <= 0x8000) {
            if (em->rotZ <= ANGLE_TURN / 4) {
                em->rotZ = em->rotZ - fly->pitch * 2;
            } else if (em->rotZ <= sel || em->rotZ > ANGLE_TURN - sel) {
                em->rotZ = em->rotZ - fly->pitch;
            } else if (em->rotZ != ANGLE_TURN - sel) {
                em->rotZ = em->rotZ + fly->pitch;
            }
        } else {
            if (em->rotZ >= ANGLE_TURN * 3 / 4) {
                em->rotZ = em->rotZ + fly->pitch * 2;
            } else if (em->rotZ < sel || em->rotZ >= ANGLE_TURN - sel) {
                em->rotZ = em->rotZ + fly->pitch;
            } else if (em->rotZ != sel) {
                em->rotZ = em->rotZ - fly->pitch;
            }
        }
    } else if (em->rotZ != 0) {
        if (em->rotZ <= sel) {
            em->rotZ = em->rotZ - fly->pitch;
        } else if (em->rotZ >= ANGLE_TURN - sel) {
            em->rotZ = em->rotZ + fly->pitch;
        }
    }
    if (!(flags & 2)) {
        bank = em->rotZ;
        bank = bank < 0x8000 ? bank : (u16) (0x10000 - bank);
        if (bank <= 4096) {
            step = fly->turn;
        } else {
            step = fly->turn + (bank - 4096) / 16;
        }
    } else {
        step = fly->turn;
    }
    if (em->rotZ != 0) {
        if (flags & 0x100) {
            bankHigh = 0x10000;
            bankLow = 0;
        } else {
            bankHigh = 61440;
            bankLow = 4096;
        }
        if (em->rotZ <= bankHigh && em->rotZ >= ANGLE_TURN * 3 / 4) {
            if (em->flightTurn <= step) {
                em->rotY = em->rotY + em->flightTurn;
                em->flightTurn = 0;
            } else {
                em->rotY = em->rotY + step;
                em->flightTurn = em->flightTurn - step;
            }
        } else if (em->rotZ >= bankLow && em->rotZ <= ANGLE_TURN / 4) {
            if (em->flightTurn >= 0x10000 - step) {
                em->rotY = em->rotY - (ANGLE_TURN - em->flightTurn);
                em->flightTurn += 0x10000 - em->flightTurn;
            } else {
                em->rotY = em->rotY - step;
                em->flightTurn = em->flightTurn + step;
            }
        }
    }
    em->rotY = (u16) em->rotY;
    em->flightTurn = (u16) em->flightTurn;
    if (flags & 0x40) {
        bank = em->rotZ;
        if (bank >= 0x8000) {
            bank = 0x10000 - bank;
        }
        if (bank < 4096) {
            sink = 0.1f;
        } else {
            sink = 6.2831855f * bank / 65536.0f;
        }
        if (!(flags & 4)) {
            em->posY = em->posY - sink;
            if (em->velZ >= 50.0f && sink != 0.1f) {
                em->velZ = em->velZ - sink;
            }
        }
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        }
    }
    if (!(flags & 4)) {
        if (flags & 0x400) {
            minHeight = gAreaGroundY[em->area] - 2000.0f;
            if (minHeight < ground->bottom) {
                minHeight = ground->bottom;
            }
        } else {
            minHeight = ground->bottom;
        }
        if (flags & 0x80) {
            if (fly->mot[0] + 1000 == em->motionId[0]) {
                if (em->motion[0].playing == 0) {
                    Em_SetMotion(em, fly->mot[1], 0, 0);
                }
                if (1000.0f + minHeight >= em->posY) {
                    em->posY = em->posY + 5.0f;
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                } else if (1500.0f + minHeight >= em->posY) {
                    em->posY = em->posY + 3.0f;
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else if (2000.0f + minHeight >= em->posY) {
                    em->posY = em->posY + 2.0f;
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else {
                    if (2100.0f + minHeight >= em->posY) {
                        em->posY = em->posY + 1.0f;
                    }
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                }
            } else if (2000.0f + minHeight >= em->posY || em->velZ <= fly->speedMin) {
                Em_SetMotion(em, fly->mot[0], 0, 0);
            }
        } else if (flags & 0x200) {
            if (fly->mot[0] + 1000 == em->motionId[0]) {
                if (em->motion[0].playing == 0) {
                    Em_SetMotion(em, fly->mot[1], 0, 0);
                }
                if (em->goal[1] >= em->posY) {
                    em->posY = em->posY + 5.0f;
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                } else if (1500.0f + minHeight >= em->posY) {
                    em->posY = em->posY + 3.0f;
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else if (2000.0f + minHeight >= em->posY) {
                    em->posY = em->posY + 2.0f;
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else {
                    if (2100.0f + minHeight >= em->posY) {
                        em->posY = em->posY + 1.0f;
                    }
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                }
            } else if (2000.0f + minHeight >= em->posY || em->velZ <= fly->speedMin) {
                Em_SetMotion(em, fly->mot[0], 0, 0);
            }
        } else {
            if (fly->mot[0] + 1000 == em->motionId[0]) {
                if (em->motion[0].playing == 0) {
                    Em_SetMotion(em, fly->mot[1], 0, 0);
                }
                if (em->goal[1] >= em->posY) {
                    em->posY = em->posY + 5.0f;
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                } else {
                    if (em->goal[1] >= em->posY) {
                        em->posY = em->posY + 1.0f;
                    }
                    if (em->velZ < fly->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                }
            } else if (em->goal[1] >= em->posY || em->velZ <= fly->speedMin) {
                Em_SetMotion(em, fly->mot[0], 0, 0);
            }
        }
    }
    if (!(flags & 8)) {
        if (fly->mot[0] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
            bank = em->rotZ;
            if (bank < 0x8000) {
                if (bank > fly->up0) {
                    if (fly->mot[2] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                        Em_SetMotion(em, fly->mot[2], 0, 0);
                    }
                } else if (bank > fly->up1) {
                    if (fly->mot[3] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                        Em_SetMotion(em, fly->mot[3], 0, 0);
                    }
                } else {
                    if (fly->mot[1] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                        Em_SetMotion(em, fly->mot[1], 0, 0);
                    }
                }
            } else if (bank < fly->down0) {
                if (fly->mot[4] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                    Em_SetMotion(em, fly->mot[4], 0, 0);
                }
            } else if (bank < fly->down1) {
                if (fly->mot[5] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                    Em_SetMotion(em, fly->mot[5], 0, 0);
                }
            } else {
                if (fly->mot[1] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                    Em_SetMotion(em, fly->mot[1], 0, 0);
                }
            }
        }
    }
    if (!(flags & 0x10)) {
        if (em->rotX != 0) {
            if (em->rotX < (ANGLE_TURN / 2)) {
                em->rotX = em->rotX - fly->roll;
            } else {
                em->rotX = em->rotX + fly->roll;
            }
            em->rotX = (u16) em->rotX;
        }
    }
}

/*
 * Returns 1 when some active monster in em's area, not in the air (stance 2),
 * lies within dist of em on the ground plane (Vec_DistanceXZ), else 0. With
 * mode 1 only a monster of exactly type kind counts (it must also pass
 * Em_KindMatches); with any other mode every kind counts. em itself is not
 * skipped.
 */
s32 EmList_FindKindNear(Em *em, u16 kind, s32 mode, f32 dist)
{
    s32 i;
    Em *other;

    for (i = 0, other = gEms; i < 20; i++, other++) {
        if (other->active != 0) {
            if (mode == 1) {
                if (Em_KindMatches(other->type, kind) == 0) {
                    continue;
                }
                if (kind != other->type) {
                    continue;
                }
            }
            if ((u8)Obj_SameArea(em, (Player *)other) != 0 && other->stance != 2 && Vec_DistanceXZ(&em->posX, &other->posX) <= dist) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 * Sets em->aimAngle to the turn still needed to face the target player
 * (em->targetNo): the angle from em->mapPos to the target's point in map
 * coordinates, minus the heading rotY. Does nothing with no target (targetNo
 * -1).
 */
void Em_AimAtTarget(Em *em)
{
    f32 pos[3];
    f32 world[3];
    s8 targetNo;

    targetNo = em->targetNo;
    if (targetNo != -1) {
        Em_GetTargetPos(em, targetNo & 0xFF, pos);
        Area_ToWorld((s32) ((Player *) &gPlayers[targetNo])->area, pos, world);
        em->aimAngle = Math_AngleXZ(em->mapPos, world);
        em->aimAngle = em->aimAngle - em->rotY;
    }
}

/*
 * Prepares a turn toward the target: clears em->turnStep, then sets
 * em->targetAngle to the angle from em->mapPos to the target player's point
 * in map coordinates (and em->target to that player), or to em->stateRand
 * when there is no target (targetNo -1).
 */
void Em_SetTargetAngle(Em *em)
{
    f32 pos[3];
    f32 targetWorld[3];
    Player *pl;
    s8 i;

    em->turnStep = 0;
    i = em->targetNo;
    if (i == -1) {
        em->targetAngle = em->stateRand;
    } else {
        pl = &gPlayers[i];
        em->target = pl;
        Em_GetTargetPos(em, (u8) em->targetNo, pos);
        Area_ToWorld((s32) pl->area, pos, targetWorld);
        em->targetAngle = (u16) Math_AngleXZ(em->mapPos, targetWorld);
    }
}

/*
 * Turns the monster to em->targetAngle with turning motions, one call per
 * frame; returns 1 when the turn is done (at once if targetAngle is within
 * 0x200 of 0), else 0. Step 0 (em->turnStep) sets lookAtTarget and aimAngle
 * and plays motFront (setting PLF_NO_ROOT_MOVE) when the angle left is within
 * range, else motLeft when it is half a turn or more, else motRight. Step 1 turns
 * rotY by 0x200 a frame toward aimAngle and repeats the choice when the
 * motion ends or reaches frame 110. Within 0x200 of aimAngle it snaps rotY to
 * it, clears lookAtTarget and PLF_NO_ROOT_MOVE and returns 1.
 */
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight)
{
    u32 angleDelta;

    switch (em->turnStep) {
    case 0:
        em->turnStep++;
        em->lookAtTarget = 1;
        em->aimAngle = em->targetAngle;
        angleDelta = (u16) (em->targetAngle - em->rotY);
        if ((u16) (em->targetAngle + 0x200) <= 0x400) {
            return 1;
        }
        if ((u16) angleDelta <= range || (u16) angleDelta >= ANGLE_TURN - range) {
            Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
            Em_SetMotion(em, motFront, 0, 0);
        } else if ((u16) angleDelta >= ANGLE_TURN / 2) {
            Em_SetMotion(em, motLeft, 0, 0);
        } else {
            Em_SetMotion(em, motRight, 0, 0);
        }
        break;
    case 1:
        angleDelta = (u16) (em->aimAngle - (u16) em->rotY);
        if (em->motion[0].playing == 0 || EmMotion_ReachedFrame(em, 0, 110.0f)) {
            if (angleDelta <= range || angleDelta >= ANGLE_TURN - range) {
                Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
                Em_SetMotion(em, motFront, 0, 0);
            } else {
                Player_ClearFlag((Player *) em, PLF_NO_ROOT_MOVE);
                if (angleDelta >= ANGLE_TURN / 2) {
                    Em_SetMotion(em, motLeft, 0, 0);
                } else {
                    Em_SetMotion(em, motRight, 0, 0);
                }
            }
        } else if (((angleDelta + 0x200) & 0xFFFF) < 0x400) {
            Player_ClearFlag((Player *) em, PLF_NO_ROOT_MOVE);
            em->rotY = em->aimAngle;
            em->lookAtTarget = 0;
            return 1;
        } else if (angleDelta < (ANGLE_TURN / 2)) {
            em->rotY = (u16) (em->rotY + 0x200);
        } else {
            em->rotY = (u16) (em->rotY - 0x200);
        }
        break;
    }
    return 0;
}

/*
 * Sets em->goalDist, the distance left to the goal, with x and y scaled by
 * the monster's size (GmEm_ScaleBySize). With no goal it is x, and hasGoal is
 * 0. Otherwise it is the distance to the goal (flags 8: in three dimensions,
 * else on the ground), less the margin y for goal kind 1 with no argument
 * (unless flags 4; with no target player, x and no goal) and for goal kind 7
 * (unless flags 2), never below 0.
 */
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y)
{
    f32 noGoalDist;
    f32 stopMargin;

    noGoalDist = GmEm_ScaleBySize(em, x);
    stopMargin = GmEm_ScaleBySize(em, y);
    if (em->curGoalKind == 0) {
        em->hasGoal = 0;
        em->goalDist = noGoalDist;
        return;
    }
    em->hasGoal = 1;
    if ((flags & 8) == 8) {
        em->goalDist = Vec_Distance(&em->posX, em->goal);
    } else {
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
    }
    if ((flags & 4) != 4 && em->curGoalKind == 1 && em->curGoalArg == 0) {
        if (em->targetNo == -1) {
            em->hasGoal = 0;
            em->goalDist = noGoalDist;
            return;
        }
        em->goalDist -= stopMargin;
        if (em->goalDist <= 0.0f) {
            em->goalDist = 0.0f;
        }
    }
    if ((flags & 2) != 2 && em->curGoalKind == 7) {
        em->goalDist -= stopMargin;
        if (em->goalDist <= 0.0f) {
            em->goalDist = 0.0f;
        }
    }
}

/*
 * Moves the monster toward its goal for one frame: when em->hasGoal is set it
 * first turns toward the goal by up to step (Em_TurnToGoal), then takes this
 * frame's forward movement off em->goalDist. flags bit 0: count the movement
 * only when hasGoal is set; bit 1: the movement is em->velZ, else the
 * motion's own (Em_MotionDelta); bit 2: count its size, so moving backward
 * also shortens the distance. Returns 1 once goalDist is 0 or less (the goal
 * is reached), else 0.
 */
s32 Em_StepToGoal(Em *em, u16 step, u8 flags)
{
    f32 motionDelta[4];
    f32 forwardMove;

    if (em->hasGoal != 0) {
        Em_TurnToGoal(em, step, 0);
    }
    if ((flags & 1) != 1 || (em->hasGoal != 0 && (flags & 1) == 1)) {
        if ((flags & 2) == 2) {
            if ((flags & 4) != 4) {
                em->goalDist -= em->velZ;
            } else if (em->velZ >= 0.0f) {
                em->goalDist -= em->velZ;
            } else {
                em->goalDist += em->velZ;
            }
        } else {
            Em_MotionDelta(em, motionDelta);
            if ((flags & 4) != 4) {
                em->goalDist = em->goalDist - motionDelta[2];
            } else {
                forwardMove = motionDelta[2];
                if (forwardMove >= 0.0f) {
                    em->goalDist -= forwardMove;
                } else {
                    em->goalDist += forwardMove;
                }
            }
        }
    }
    return em->goalDist <= 0.0f;
}

/*
 * Turns em->rotY toward an angle by at most step; returns 1 once it faces it
 * (the angle left is within step, and rotY is set to it), else 0. The angle
 * is the one to em->goal, or with flags bit 1 the one to the target player's
 * point (returning 1 with no target); bit 2 turns away instead (the angle
 * plus half a turn), and bit 0 returns 1 at once when the monster has no goal
 * (curGoalKind 0).
 */
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags)
{
    f32 pos[4];
    f32 targetWorld[4];
    u8 done = 0;
    s8 i;
    u16 ang;
    u16 angleDelta;

    if ((flags & 1) && em->curGoalKind == 0) {
        return 1;
    }
    if (flags & 2) {
        i = em->targetNo;
        if (i == -1) {
            return 1;
        }
        Em_GetTargetPos(em, i, pos);
        Area_ToWorld((s32) ((Player *) &gPlayers[i])->area, pos, targetWorld);
        ang = Math_AngleXZ(em->mapPos, targetWorld);
    } else {
        ang = Math_AngleXZ(&em->posX, em->goal);
    }
    if (flags & 4) {
        ang += ANGLE_TURN / 2;
    }
    angleDelta = ang - em->rotY;
    if (angleDelta <= ANGLE_TURN / 2) {
        if (angleDelta < step) {
            em->rotY += angleDelta;
            done = 1;
        } else {
            em->rotY += step;
        }
    } else if (0x10000 - step < angleDelta) {
        em->rotY += angleDelta;
        done = 1;
    } else {
        em->rotY -= step;
    }
    em->rotY = (u16) em->rotY;
    return done;
}

/*
 * Starts a turning motion toward em->goal (away from it when back is
 * nonzero): sets em->aimAngle to that angle, then plays row turn's motFront
 * (setting PLF_NO_ROOT_MOVE) when turn->range is nonzero and the angle left is
 * within it, else motLeft when the angle left is half a turn or more, else
 * motRight. Em_TurnStep carries the turn on.
 */
void Em_StartTurn(Em *em, EmTurnParams *turn, s32 back)
{
    u32 angleDelta;

    if (back == 0) {
        em->aimAngle = Math_AngleXZ((f32 *) &em->posX, em->goal);
    } else {
        em->aimAngle = (u16) Math_AngleXZ((f32 *) &em->posX, em->goal) - ANGLE_TURN / 2;
    }
    angleDelta = (u16) (em->aimAngle - em->rotY);
    if (turn->range != 0 && (angleDelta <= turn->range || angleDelta >= ANGLE_TURN - turn->range)) {
        Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
        Em_SetMotion(em, turn->motFront, 0, 0);
    } else if (angleDelta >= ANGLE_TURN / 2) {
        Em_SetMotion(em, turn->motLeft, 0, 0);
    } else {
        Em_SetMotion(em, turn->motRight, 0, 0);
    }
}

void Em_StartTurnToTarget(Em *em, EmTurnParams *turn)
{
    Em_StartTurn(em, turn, 0);
}

void Em_StartTurnAway(Em *em, EmTurnParams *turn)
{
    Em_StartTurn(em, turn, 1);
}

/*
 * Carries on a turn begun by Em_StartTurn, once a frame; returns 1 when the
 * turn is finished, else 0 (always 0 while the motion blends). Each frame it
 * turns em->rotY toward em->aimAngle by a step worked out from row w
 * (w->angle spread over half the motion's frames, or over half of w->from to
 * w->to, scaled by speedBonus and motionSpeed), only within the frames from
 * to when those are set. When the motion has ended or reached frame w->frame
 * (if above 0) it returns 1 if the angle left is within one step, else starts
 * the next turn motion as Em_StartTurn does.
 */
s32 Em_TurnStep(Em *em, EmTurnParams *w)
{
    u32 step;
    u32 angleDelta;
    s32 reachedFrame;

    if (em->motion[0].blend != 0) {
        return 0;
    }
    if (w->from == 0.0f && w->to == 0.0f) {
        step = (em->speedBonus + 100) * (em->motionSpeed * (w->angle / (em->motion[0].frameCount / 2.0f))) / 100.0f;
    } else {
        step = (em->speedBonus + 100) * (em->motionSpeed * (w->angle / ((w->to - w->from) / 2.0f))) / 100.0f;
    }
    angleDelta = (u16) (em->aimAngle - (u16) em->rotY);
    if (w->frame <= 0.0f) {
        reachedFrame = 0;
    } else {
        reachedFrame = EmMotion_ReachedFrame(em, 0, w->frame);
    }
    if (em->motion[0].playing == 0 || reachedFrame != 0) {
        if (((angleDelta + step) & 0xFFFF) < step * 2) {
            return 1;
        }
        if (w->range != 0 && (angleDelta <= w->range || angleDelta >= ANGLE_TURN - w->range)) {
            Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
            Em_SetMotion(em, w->motFront, 0, 0);
        } else {
            Player_ClearFlag((Player *) em, PLF_NO_ROOT_MOVE);
            if (angleDelta >= ANGLE_TURN / 2) {
                Em_SetMotion(em, w->motLeft, 0, 0);
            } else {
                Em_SetMotion(em, w->motRight, 0, 0);
            }
        }
    } else if ((w->from == 0.0f && w->to == 0.0f) || EmMotion_InFrameRange(em, 0, w->from, w->to) == 1) {
        if (((angleDelta + step) & 0xFFFF) < step * 2) {
            em->rotY = em->aimAngle;
        } else if (angleDelta < (ANGLE_TURN / 2)) {
            em->rotY = (u16) (em->rotY + step);
        } else {
            em->rotY = (u16) (em->rotY - step);
        }
    }
    return 0;
}

void Em_ClearTilt(Em *em, s32 unused)
{
    em->rotX = 0;
    em->rotZ = 0;
}

/*
 * The file's data: the per-area tables the monsters'
 * area lookups read (em->tables, Em_AreaEntryValue, Em_FindAreaEntry):
 * ground heights, point sets, four-point sets and route sets; then
 * Em_PartEffectTbl. The point sets are EmAreaPoints lists whose points are
 * three floats each; a route set (EmRouteSet) lists an area's routes
 * (EmRoute), each a list of route points (EmRoutePt): em.h.
 */
extern f32 gEmGoalPosSets_119[];
extern f32 gEmKindTables_68_2_72[][3];
extern EmRoute gEmRouteSets_24[];
f32 gAreaGroundY[200] = {
#include "game/008BF9C0.inc"
};
f32 gEmKindTables_68_0_4[1][3] = {
#include "game/008BFCE0.inc"
};
f32 gEmKindTables_68_0_5[1][3] = {
#include "game/008BFCF0.inc"
};
f32 gEmKindTables_68_0_6[1][3] = {
#include "game/008BFD00.inc"
};
f32 gEmKindTables_68_0_7[1][3] = {
#include "game/008BFD10.inc"
};
f32 gEmKindTables_68_0_8[5][3] = {
#include "game/008BFD20.inc"
};
f32 gEmKindTables_68_0_9[4][3] = {
#include "game/008BFD60.inc"
};
f32 gEmKindTables_68_0_10[1][3] = {
#include "game/008BFD90.inc"
};
f32 gEmKindTables_68_0_11[1][3] = {
#include "game/008BFDA0.inc"
};
f32 gEmKindTables_68_0_14[1][3] = {
#include "game/008BFDB0.inc"
};
f32 gEmGoalPosSets_123[1][3] = {
#include "game/008BFDC0.inc"
};
f32 gEmKindTables_68_0_18[1][3] = {
#include "game/008BFDD0.inc"
};
f32 gEmKindTables_68_0_19[15][3] = {
#include "game/008BFDE0.inc"
};
f32 gEmKindTables_68_0_21[1][3] = {
#include "game/008BFE98.inc"
};
f32 gEmKindTables_68_0_22[6][3] = {
#include "game/008BFEB0.inc"
};
f32 gEmKindTables_68_0_23[1][3] = {
#include "game/008BFEF8.inc"
};
f32 gEmKindTables_68_0_24[5][3] = {
#include "game/008BFF10.inc"
};
f32 gEmKindTables_68_0_25[1][3] = {
#include "game/008BFF50.inc"
};
f32 gEmGoalPosSets_124[25][3] = {
#include "game/008BFF60.inc"
};
f32 gEmGoalPosSets_125[1][3] = {
#include "game/008C0090.inc"
};
f32 gEmGoalPosSets_126[2][3] = {
#include "game/008C00A0.inc"
};
f32 gEmGoalPosSets_127[4][3] = {
#include "game/008C00C0.inc"
};
f32 gEmGoalPosSets_128[4][3] = {
#include "game/008C00F0.inc"
};
f32 gEmGoalPosSets_129[7][3] = {
#include "game/008C0120.inc"
};
f32 gEmGoalPosSets_130[1][3] = {
#include "game/008C0178.inc"
};
f32 gEmGoalPosSets_131[2][3] = {
#include "game/008C0190.inc"
};
f32 gEmGoalPosSets_132[4][3] = {
#include "game/008C01B0.inc"
};
f32 gEmGoalPosSets_133[2][3] = {
#include "game/008C01E0.inc"
};
f32 gEmGoalPosSets_134[1][3] = {
#include "game/008C01F8.inc"
};
f32 gEmGoalPosSets_135[2][3] = {
#include "game/008C0210.inc"
};
f32 gEmKindTables_68_0_47[1][3] = {
#include "game/008C0228.inc"
};
f32 gEmKindTables_68_0_65[2][3] = {
#include "game/008C0240.inc"
};
f32 gEmKindTables_68_0_48[3][3] = {
#include "game/008C0260.inc"
};
f32 gEmKindTables_68_0_56[1][3] = {
#include "game/008C0288.inc"
};
f32 gEmKindTables_68_0_68[4][3] = {
#include "game/008C02A0.inc"
};
f32 gEmKindTables_68_0_69[11][3] = {
#include "game/008C02D0.inc"
};
f32 gEmKindTables_68_0_70[1][3] = {
#include "game/008C0358.inc"
};
f32 gEmKindTables_68_0_71[1][3] = {
#include "game/008C0368.inc"
};
f32 gEmKindTables_68_0_72[4][3] = {
#include "game/008C0380.inc"
};
f32 gEmKindTables_68_0_73[3][3] = {
#include "game/008C03B0.inc"
};
f32 gEmKindTables_68_0_0[1][3] = {
#include "game/008C03D8.inc"
};
f32 gEmKindTables_68_0_1[1][3] = {
#include "game/008C03E8.inc"
};
f32 gEmKindTables_68_0_2[3][3] = {
#include "game/008C0400.inc"
};
f32 gEmKindTables_68_0_3[1][3] = {
#include "game/008C0428.inc"
};
f32 gEmKindTables_68_0_12[1][3] = {
#include "game/008C0438.inc"
};
f32 gEmKindTables_68_0_13[4][3] = {
#include "game/008C0450.inc"
};
f32 gEmKindTables_68_0_16[1][3] = {
#include "game/008C0480.inc"
};
f32 gEmKindTables_68_0_17[1][3] = {
#include "game/008C0490.inc"
};
f32 gEmKindTables_68_0_20[1][3] = {
#include "game/008C04A0.inc"
};
f32 gEmKindTables_68_0_89[1][3] = {
#include "game/008C04B0.inc"
};
f32 gEmGoalPosSets_84[1][3] = {
#include "game/008C04C0.inc"
};
f32 gEmGoalPosSets_85[3][3] = {
#include "game/008C04D0.inc"
};
f32 gEmGoalPosSets_86[1][3] = {
#include "game/008C04F8.inc"
};
f32 gEmGoalPosSets_87[1][3] = {
#include "game/008C0508.inc"
};
f32 gEmGoalPosSets_88[3][3] = {
#include "game/008C0520.inc"
};
f32 gEmGoalPosSets_89[1][3] = {
#include "game/008C0548.inc"
};
f32 gEmGoalPosSets_90[1][3] = {
#include "game/008C0558.inc"
};
f32 gEmGoalPosSets_92[1][3] = {
#include "game/008C0568.inc"
};
f32 gEmGoalPosSets_93[1][3] = {
#include "game/008C0578.inc"
};
f32 gEmGoalPosSets_94[3][3] = {
#include "game/008C0590.inc"
};
f32 gEmGoalPosSets_95[1][3] = {
#include "game/008C05B8.inc"
};
f32 gEmGoalPosSets_96[1][3] = {
#include "game/008C05C8.inc"
};
f32 gEmGoalPosSets_97[1][3] = {
#include "game/008C05D8.inc"
};
f32 gEmGoalPosSets_98[1][3] = {
#include "game/008C05E8.inc"
};
f32 gEmGoalPosSets_99[3][3] = {
#include "game/008C0600.inc"
};
f32 gEmGoalPosSets_100[1][3] = {
#include "game/008C0628.inc"
};
f32 gEmGoalPosSets_101[1][3] = {
#include "game/008C0638.inc"
};
f32 gEmGoalPosSets_102[1][3] = {
#include "game/008C0648.inc"
};
f32 gEmGoalPosSets_103[1][3] = {
#include "game/008C0658.inc"
};
f32 gEmGoalPosSets_104[1][3] = {
#include "game/008C0668.inc"
};
f32 gEmGoalPosSets_105[1][3] = {
#include "game/008C0678.inc"
};
f32 gEmGoalPosSets_106[1][3] = {
#include "game/008C0688.inc"
};
f32 gEmGoalPosSets_107[1][3] = {
#include "game/008C0698.inc"
};
f32 gEmGoalPosSets_108[1][3] = {
#include "game/008C06A8.inc"
};
f32 gEmGoalPosSets_109[1][3] = {
#include "game/008C06B8.inc"
};
f32 gEmGoalPosSets_110[1][3] = {
#include "game/008C06C8.inc"
};
f32 gEmGoalPosSets_111[1][3] = {
#include "game/008C06D8.inc"
};
f32 gEmGoalPosSets_112[1][3] = {
#include "game/008C06E8.inc"
};
f32 gEmGoalPosSets_113[1][3] = {
#include "game/008C06F8.inc"
};
f32 gEmGoalPosSets_114[1][3] = {
#include "game/008C0708.inc"
};
f32 gEmGoalPosSets_115[1][3] = {
#include "game/008C0718.inc"
};
f32 gEmGoalPosSets_116[1][3] = {
#include "game/008C0728.inc"
};
f32 gEmGoalPosSets_117[6][3] = {
#include "game/008C0740.inc"
};
f32 gEmGoalPosSets_118[1][3] = {
#include "game/008C0788.inc"
};
f32 gEmGoalPosSets_119[8] = {
#include "game/008C07A0.inc"
};
EmAreaPoints gEmKindTables_68_0[143] = {
#include "game/008C07C0.inc"
};
f32 gEmGoalPosSets_4[1][3] = {
#include "game/008C0C38.inc"
};
f32 gEmGoalPosSets_5[1][3] = {
#include "game/008C0C48.inc"
};
f32 gEmGoalPosSets_6[1][3] = {
#include "game/008C0C58.inc"
};
f32 gEmGoalPosSets_7[1][3] = {
#include "game/008C0C68.inc"
};
f32 gEmGoalPosSets_8[1][3] = {
#include "game/008C0C78.inc"
};
f32 gEmGoalPosSets_9[1][3] = {
#include "game/008C0C88.inc"
};
f32 gEmGoalPosSets_10[1][3] = {
#include "game/008C0C98.inc"
};
f32 gEmGoalPosSets_11[1][3] = {
#include "game/008C0CA8.inc"
};
f32 gEmGoalPosSets_12[1][3] = {
#include "game/008C0CB8.inc"
};
f32 gEmGoalPosSets_15[1][3] = {
#include "game/008C0CC8.inc"
};
f32 gEmGoalPosSets_16[1][3] = {
#include "game/008C0CD8.inc"
};
f32 gEmGoalPosSets_19[1][3] = {
#include "game/008C0CE8.inc"
};
f32 gEmGoalPosSets_20[1][3] = {
#include "game/008C0CF8.inc"
};
f32 gEmGoalPosSets_22[1][3] = {
#include "game/008C0D08.inc"
};
f32 gEmGoalPosSets_23[1][3] = {
#include "game/008C0D18.inc"
};
f32 gEmGoalPosSets_24[1][3] = {
#include "game/008C0D28.inc"
};
f32 gEmGoalPosSets_25[1][3] = {
#include "game/008C0D38.inc"
};
f32 gEmGoalPosSets_26[1][3] = {
#include "game/008C0D48.inc"
};
f32 gEmGoalPosSets_27[1][3] = {
#include "game/008C0D58.inc"
};
f32 gEmGoalPosSets_28[1][3] = {
#include "game/008C0D68.inc"
};
f32 gEmGoalPosSets_29[1][3] = {
#include "game/008C0D78.inc"
};
f32 gEmGoalPosSets_30[1][3] = {
#include "game/008C0D88.inc"
};
f32 gEmGoalPosSets_31[1][3] = {
#include "game/008C0D98.inc"
};
f32 gEmGoalPosSets_32[1][3] = {
#include "game/008C0DA8.inc"
};
f32 gEmGoalPosSets_33[1][3] = {
#include "game/008C0DB8.inc"
};
f32 gEmGoalPosSets_34[1][3] = {
#include "game/008C0DC8.inc"
};
f32 gEmGoalPosSets_35[1][3] = {
#include "game/008C0DD8.inc"
};
f32 gEmGoalPosSets_36[1][3] = {
#include "game/008C0DE8.inc"
};
f32 gEmGoalPosSets_37[1][3] = {
#include "game/008C0DF8.inc"
};
f32 gEmGoalPosSets_38[1][3] = {
#include "game/008C0E08.inc"
};
f32 gEmGoalPosSets_39[1][3] = {
#include "game/008C0E18.inc"
};
f32 gEmGoalPosSets_40[1][3] = {
#include "game/008C0E28.inc"
};
f32 gEmGoalPosSets_41[1][3] = {
#include "game/008C0E38.inc"
};
f32 gEmGoalPosSets_42[1][3] = {
#include "game/008C0E48.inc"
};
f32 gEmGoalPosSets_43[1][3] = {
#include "game/008C0E58.inc"
};
f32 gEmGoalPosSets_44[1][3] = {
#include "game/008C0E68.inc"
};
f32 gEmGoalPosSets_45[1][3] = {
#include "game/008C0E78.inc"
};
f32 gEmGoalPosSets_46[1][3] = {
#include "game/008C0E88.inc"
};
f32 gEmGoalPosSets_47[1][3] = {
#include "game/008C0E98.inc"
};
f32 gEmGoalPosSets_48[1][3] = {
#include "game/008C0EA8.inc"
};
f32 gEmGoalPosSets_49[1][3] = {
#include "game/008C0EB8.inc"
};
f32 gEmGoalPosSets_50[1][3] = {
#include "game/008C0EC8.inc"
};
f32 gEmGoalPosSets_51[1][3] = {
#include "game/008C0ED8.inc"
};
f32 gEmGoalPosSets_52[1][3] = {
#include "game/008C0EE8.inc"
};
f32 gEmGoalPosSets_53[1][3] = {
#include "game/008C0EF8.inc"
};
f32 gEmGoalPosSets_54[1][3] = {
#include "game/008C0F08.inc"
};
f32 gEmGoalPosSets_55[1][3] = {
#include "game/008C0F18.inc"
};
f32 gEmGoalPosSets_56[1][3] = {
#include "game/008C0F28.inc"
};
f32 gEmGoalPosSets_57[1][3] = {
#include "game/008C0F38.inc"
};
f32 gEmGoalPosSets_58[1][3] = {
#include "game/008C0F48.inc"
};
f32 gEmGoalPosSets_59[1][3] = {
#include "game/008C0F58.inc"
};
f32 gEmGoalPosSets_60[1][3] = {
#include "game/008C0F68.inc"
};
f32 gEmGoalPosSets_61[1][3] = {
#include "game/008C0F78.inc"
};
f32 gEmGoalPosSets_62[1][3] = {
#include "game/008C0F88.inc"
};
f32 gEmGoalPosSets_63[1][3] = {
#include "game/008C0F98.inc"
};
f32 gEmGoalPosSets_64[1][3] = {
#include "game/008C0FA8.inc"
};
f32 gEmGoalPosSets_65[1][3] = {
#include "game/008C0FB8.inc"
};
f32 gEmGoalPosSets_66[1][3] = {
#include "game/008C0FC8.inc"
};
f32 gEmGoalPosSets_67[1][3] = {
#include "game/008C0FD8.inc"
};
f32 gEmGoalPosSets_68[1][3] = {
#include "game/008C0FE8.inc"
};
f32 gEmGoalPosSets_69[1][3] = {
#include "game/008C0FF8.inc"
};
f32 gEmGoalPosSets_70[1][3] = {
#include "game/008C1008.inc"
};
f32 gEmGoalPosSets_71[1][3] = {
#include "game/008C1018.inc"
};
f32 gEmGoalPosSets_72[1][3] = {
#include "game/008C1028.inc"
};
f32 gEmGoalPosSets_0[1][3] = {
#include "game/008C1038.inc"
};
f32 gEmGoalPosSets_1[1][3] = {
#include "game/008C1048.inc"
};
f32 gEmGoalPosSets_2[1][3] = {
#include "game/008C1058.inc"
};
f32 gEmGoalPosSets_3[1][3] = {
#include "game/008C1068.inc"
};
f32 gEmGoalPosSets_13[1][3] = {
#include "game/008C1078.inc"
};
f32 gEmGoalPosSets_14[1][3] = {
#include "game/008C1088.inc"
};
f32 gEmGoalPosSets_17[1][3] = {
#include "game/008C1098.inc"
};
f32 gEmGoalPosSets_18[4][3] = {
#include "game/008C10B0.inc"
};
f32 gEmGoalPosSets_21[1][3] = {
#include "game/008C10E0.inc"
};
f32 gEmGoalPosSets_83[1][3] = {
#include "game/008C10F0.inc"
};
EmAreaPoints gEmGoalPosSets[137] = {
#include "game/008C1100.inc"
};
f32 gEmKindTables_68_2_7[4][3] = {
#include "game/008C1550.inc"
};
f32 gEmKindTables_68_2_8[4][3] = {
#include "game/008C1580.inc"
};
f32 gEmKindTables_68_2_9[4][3] = {
#include "game/008C15B0.inc"
};
f32 gEmKindTables_68_2_10[4][3] = {
#include "game/008C15E0.inc"
};
f32 gEmKindTables_68_2_11[4][3] = {
#include "game/008C1610.inc"
};
f32 gEmKindTables_68_2_12[4][3] = {
#include "game/008C1640.inc"
};
f32 gEmKindTables_68_2_13[4][3] = {
#include "game/008C1670.inc"
};
f32 gEmKindTables_68_2_30[4][3] = {
#include "game/008C16A0.inc"
};
f32 gEmKindTables_68_2_31[4][3] = {
#include "game/008C16D0.inc"
};
f32 gEmKindTables_68_2_32[4][3] = {
#include "game/008C1700.inc"
};
f32 gEmKindTables_68_2_33[4][3] = {
#include "game/008C1730.inc"
};
f32 gEmKindTables_68_2_34[4][3] = {
#include "game/008C1760.inc"
};
f32 gEmKindTables_68_2_35[4][3] = {
#include "game/008C1790.inc"
};
f32 gEmKindTables_68_2_0[4][3] = {
#include "game/008C17C0.inc"
};
f32 gEmKindTables_68_2_1[4][3] = {
#include "game/008C17F0.inc"
};
f32 gEmKindTables_68_2_2[4][3] = {
#include "game/008C1820.inc"
};
f32 gEmKindTables_68_2_3[4][3] = {
#include "game/008C1850.inc"
};
f32 gEmKindTables_68_2_4[4][3] = {
#include "game/008C1880.inc"
};
f32 gEmKindTables_68_2_5[4][3] = {
#include "game/008C18B0.inc"
};
f32 gEmKindTables_68_2_6[4][3] = {
#include "game/008C18E0.inc"
};
f32 gEmKindTables_68_2_48[4][3] = {
#include "game/008C1910.inc"
};
f32 gEmKindTables_68_2_49[4][3] = {
#include "game/008C1940.inc"
};
f32 gEmKindTables_68_2_50[4][3] = {
#include "game/008C1970.inc"
};
f32 gEmKindTables_68_2_51[4][3] = {
#include "game/008C19A0.inc"
};
f32 gEmKindTables_68_2_52[4][3] = {
#include "game/008C19D0.inc"
};
f32 gEmKindTables_68_2_53[4][3] = {
#include "game/008C1A00.inc"
};
f32 gEmKindTables_68_2_54[4][3] = {
#include "game/008C1A30.inc"
};
f32 gEmKindTables_68_2_15[4][3] = {
#include "game/008C1A60.inc"
};
f32 gEmKindTables_68_2_16[4][3] = {
#include "game/008C1A90.inc"
};
f32 gEmKindTables_68_2_17[4][3] = {
#include "game/008C1AC0.inc"
};
f32 gEmKindTables_68_2_18[4][3] = {
#include "game/008C1AF0.inc"
};
f32 gEmKindTables_68_2_19[4][3] = {
#include "game/008C1B20.inc"
};
f32 gEmKindTables_68_2_20[4][3] = {
#include "game/008C1B50.inc"
};
f32 gEmKindTables_68_2_14[4][3] = {
#include "game/008C1B80.inc"
};
f32 gEmKindTables_68_2_25[4][3] = {
#include "game/008C1BB0.inc"
};
f32 gEmKindTables_68_2_26[4][3] = {
#include "game/008C1BE0.inc"
};
f32 gEmKindTables_68_2_27[4][3] = {
#include "game/008C1C10.inc"
};
f32 gEmKindTables_68_2_28[4][3] = {
#include "game/008C1C40.inc"
};
f32 gEmKindTables_68_2_29[4][3] = {
#include "game/008C1C70.inc"
};
f32 gEmKindTables_68_2_21[4][3] = {
#include "game/008C1CA0.inc"
};
f32 gEmKindTables_68_2_22[4][3] = {
#include "game/008C1CD0.inc"
};
f32 gEmKindTables_68_2_23[4][3] = {
#include "game/008C1D00.inc"
};
f32 gEmKindTables_68_2_24[4][3] = {
#include "game/008C1D30.inc"
};
f32 gEmKindTables_68_2_71[4][3] = {
#include "game/008C1D60.inc"
};
f32 gEmKindTables_68_2_72[4][3] = {
#include "game/008C1D90.inc"
};
EmAreaPoints gEmKindTables_68_2[76] = {
#include "game/008C1DC0.inc"
};
f32 EmCephadrome_VelYCurves_3[60] = {
#include "game/008C2020.inc"
};
EmRoutePt gEmRouteSets_0_0[6] = {
#include "game/008C2110.inc"
};
EmRoute gEmRouteSets_0[1] = {
#include "game/008C2190.inc"
};
EmRoutePt gEmRouteSets_1_0[6] = {
#include "game/008C21B0.inc"
};
EmRoute gEmRouteSets_1[1] = {
#include "game/008C2230.inc"
};
EmRoutePt gEmRouteSets_2_0[5] = {
#include "game/008C2250.inc"
};
EmRoute gEmRouteSets_2[1] = {
#include "game/008C22C0.inc"
};
EmRoutePt gEmRouteSets_3_0[6] = {
#include "game/008C22E0.inc"
};
EmRoute gEmRouteSets_3[1] = {
#include "game/008C2360.inc"
};
EmRoutePt gEmRouteSets_4_0[6] = {
#include "game/008C2380.inc"
};
EmRoutePt gEmRouteSets_4_1[8] = {
#include "game/008C2400.inc"
};
EmRoute gEmRouteSets_4[2] = {
#include "game/008C24A0.inc"
};
EmRoutePt gEmRouteSets_5_0[6] = {
#include "game/008C24D0.inc"
};
EmRoute gEmRouteSets_5[1] = {
#include "game/008C2550.inc"
};
EmRoutePt gEmRouteSets_6_0[6] = {
#include "game/008C2570.inc"
};
EmRoute gEmRouteSets_6[1] = {
#include "game/008C25F0.inc"
};
EmRoutePt gEmRouteSets_7_0[6] = {
#include "game/008C2610.inc"
};
EmRoute gEmRouteSets_7[1] = {
#include "game/008C2690.inc"
};
EmRoutePt gEmRouteSets_8_0[5] = {
#include "game/008C26B0.inc"
};
EmRoute gEmRouteSets_8[1] = {
#include "game/008C2720.inc"
};
EmRoutePt gEmRouteSets_9_0[6] = {
#include "game/008C2740.inc"
};
EmRoute gEmRouteSets_9[1] = {
#include "game/008C27C0.inc"
};
EmRoutePt gEmRouteSets_10_0[4] = {
#include "game/008C27E0.inc"
};
EmRoute gEmRouteSets_10[1] = {
#include "game/008C2830.inc"
};
EmRoutePt gEmRouteSets_11_0[3] = {
#include "game/008C2850.inc"
};
EmRoutePt gEmRouteSets_11_1[3] = {
#include "game/008C2890.inc"
};
EmRoute gEmRouteSets_11[2] = {
#include "game/008C28D0.inc"
};
EmRoutePt gEmRouteSets_12_0[4] = {
#include "game/008C2900.inc"
};
EmRoute gEmRouteSets_12[1] = {
#include "game/008C2950.inc"
};
EmRoutePt gEmRouteSets_13_0[7] = {
#include "game/008C2970.inc"
};
EmRoutePt gEmRouteSets_13_1[3] = {
#include "game/008C2A00.inc"
};
EmRoute gEmRouteSets_13[2] = {
#include "game/008C2A40.inc"
};
EmRoutePt gEmRouteSets_14_0[3] = {
#include "game/008C2A70.inc"
};
EmRoutePt gEmRouteSets_14_1[3] = {
#include "game/008C2AB0.inc"
};
EmRoute gEmRouteSets_14[2] = {
#include "game/008C2AF0.inc"
};
EmRoutePt gEmRouteSets_15_0[6] = {
#include "game/008C2B20.inc"
};
EmRoute gEmRouteSets_15[1] = {
#include "game/008C2BA0.inc"
};
EmRoutePt gEmRouteSets_16_0[6] = {
#include "game/008C2BC0.inc"
};
EmRoute gEmRouteSets_16[1] = {
#include "game/008C2C40.inc"
};
EmRoutePt gEmRouteSets_17_0[4] = {
#include "game/008C2C60.inc"
};
EmRoute gEmRouteSets_17[1] = {
#include "game/008C2CB0.inc"
};
EmRoutePt gEmRouteSets_18_0[4] = {
#include "game/008C2CD0.inc"
};
EmRoute gEmRouteSets_18[1] = {
#include "game/008C2D20.inc"
};
EmRoutePt gEmRouteSets_19_0[5] = {
#include "game/008C2D40.inc"
};
EmRoute gEmRouteSets_19[1] = {
#include "game/008C2DB0.inc"
};
EmRoutePt gEmRouteSets_20_0[7] = {
#include "game/008C2DD0.inc"
};
EmRoute gEmRouteSets_20[1] = {
#include "game/008C2E60.inc"
};
EmRoutePt gEmRouteSets_21_0[6] = {
#include "game/008C2E80.inc"
};
EmRoute gEmRouteSets_21[1] = {
#include "game/008C2F00.inc"
};
EmRoutePt gEmRouteSets_22_0[6] = {
#include "game/008C2F20.inc"
};
EmRoute gEmRouteSets_22[1] = {
#include "game/008C2FA0.inc"
};
EmRoutePt gEmRouteSets_23_0[5] = {
#include "game/008C2FC0.inc"
};
EmRoute gEmRouteSets_23[1] = {
#include "game/008C3030.inc"
};
EmRoutePt gEmRouteSets_24_0[5] = {
#include "game/008C3050.inc"
};
EmRoute gEmRouteSets_24[1] = {
#include "game/008C30C0.inc"
};
EmRouteSet gEmRouteSets[26] = {
#include "game/008C30E0.inc"
};
EmPartEffect Em_PartEffectTbl[42] = {
#include "game/008C31B0.inc"
};
