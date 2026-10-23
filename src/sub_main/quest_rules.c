/*
 * The quest's rules beyond its script (sub_main): the versus quests (two
 * teams of players, each with its monster: Versus_*), the quest's checks
 * (Quest_IsVersus, Quest_KeepsRecords ...), the save's hunting records
 * after a quest (Records_*, Quest_UpdateSaveRecords), the play style
 * points a quest earns, the return-to-camp item's test, the countdown
 * before a quest's end, the quest's event and the persistent health of
 * the special monsters.
 * The quest's state is gQuest (quest.h), the players are gPlayers
 * (player.h); Quest_Update and its helpers run the quest itself.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "quest.h"
#include "syssettings.h"
#include "area.h"
#include "objattach.h"
#include "camera.h"
#include "qsync.h"
#include "game_util.h"
#include "rand.h"
#include "screen_fade.h"

#pragma optimization_level 4

/* The monster array (em.h names it gEms, which only the game overlay links). */
extern Em gEms[];

extern u8 gPersistentEmKinds[16];
extern MonsterBit gMonsterFlagBits[];
extern u8 gStyleSymbolColors[];
extern RecordCountEntry gRecordCountEntries[];

extern NextQuest gNextQuest;
extern TeamEntry gTeamItemBits[];
extern TeamEntry *gMonsterNoteMarks[];
extern s8 gVersusLevelOffsets[];

u8 Pouch_Add(Player *p, u16 item, s16 n);
void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c);

/* 1 in the versus quest (QUEST_ID_ARENA_CUP). */
s32 Quest_IsVersus(void)
{
    if (gQuest.stage == QUEST_ID_ARENA_CUP && Game_IsOnline() == 1) {
        return 1;
    }
    return 0;
}

/* The team (0 or 1) of a versus side (1 to 4); side 0 gives 0. */
s8 Versus_Team(s8 side)
{
    if (side == 0) {
        return 0;
    }
    return (side - 1) & 1;
}

/*
 * A monster of `em`'s side went down: its team has one monster fewer, and
 * every active local monster of that side loses 30 of its versus gauge.
 */
void Versus_MonsterDown(Em *em)
{
    s32 i;
    Em *other = gEms;

    gQuest.sideEmLeft[Versus_Team(em->side)]--;
    for (i = 0; i < EM_COUNT; i++, other++) {
        if (other->active != 0 && Side_Match(em->side, other->side, 1) == 1 && other->remote == 0) {
            if (other->versusGauge < 30) {
                other->versusGauge = 0;
            } else {
                other->versusGauge -= 30;
            }
        }
    }
}

/*
 * Applies the set bits of a team item's entry to a versus team: bit 0 one
 * of three random effects; bit 1 a larger size (to the kind's maximum,
 * gEmAppearMax); bit 2 the base level + 1; bit 3 the gauge level + 1
 * (not for the variant kinds below); bits 4-6 and 28-30 the team's flags;
 * bits 7-11, 12-21 and 22-27 its rage and size scales. The switch on
 * `bits & (1 << i)` tests one bit per case.
 */
void Versus_ApplyTeamBits(QuestTeam *team, u32 bits, u16 kind)
{
    s32 i;
    u16 *sizeMax = &gEmAppearMax[kind];

    for (i = 0; i < 32; i++) {
        switch (bits & (1 << i)) {
        case 0:
            break;
        case 1:
            switch (Quest_Random() % 3) {
            case 1:
                team->rageScale1 = 125;
                team->sizeScale = 115;
                break;
            case 2:
                team->baseLevel++;
                if (team->baseLevel > 12) {
                    team->baseLevel = 12;
                }
                break;
            default:
                team->sizePct -= Quest_Random() % 10 + 1;
                if (team->sizePct < 75) {
                    team->sizePct = 75;
                }
                break;
            }
            break;
        case 2:
            if (team->sizePct < *sizeMax) {
                team->sizePct += Quest_Random() % 10 + 1;
                if (team->sizePct > *sizeMax) {
                    team->sizePct = *sizeMax;
                }
            }
            break;
        case 4:
            team->baseLevel++;
            if (team->baseLevel > 12) {
                team->baseLevel = 12;
            }
            break;
        case 8:
            switch (kind) {
            case EM_KIND_RAJANG:
            case EM_KIND_PINK_RATHIAN:
            case EM_KIND_BLUE_YIAN_KUT_KU:
            case EM_KIND_PURPLE_GYPCEROS:
            case EM_KIND_SILVER_RATHALOS:
            case EM_KIND_GOLD_RATHIAN:
            case EM_KIND_BLACK_DIABLOS:
            case EM_KIND_RED_KHEZU:
            case EM_KIND_BLACK_GRAVIOS:
            case EM_KIND_AZURE_RATHALOS:
                break;
            default:
                team->gaugeLevel++;
                if (team->gaugeLevel > 4) {
                    team->gaugeLevel = 4;
                }
                break;
            }
            break;
        case 16:
            team->flags |= 1;
            break;
        case 32:
            team->flags |= 2;
            break;
        case 64:
            team->flags |= 4;
            break;
        case 128:
            team->rageScale1 = 105;
            break;
        case 256:
            team->rageScale1 = 110;
            break;
        case 512:
            team->rageScale1 = 115;
            break;
        case 1024:
            team->rageScale1 = 120;
            break;
        case 2048:
            team->rageScale1 = 125;
            break;
        case 4096:
            team->rageScale2 = 97;
            break;
        case 8192:
            team->rageScale2 = 95;
            break;
        case 16384:
            team->rageScale2 = 90;
            break;
        case 0x8000:
            team->rageScale2 = 103;
            break;
        case 0x10000:
            team->rageScale2 = 105;
            break;
        case 0x20000:
            team->rageScale2 = 108;
            break;
        case 0x40000:
            team->rageScale2 = 110;
            break;
        case 0x80000:
            team->rageScale2 = 115;
            break;
        case 0x100000:
            team->rageScale2 = 120;
            break;
        case 0x200000:
            team->rageScale2 = 125;
            break;
        case 0x400000:
            team->sizeScale = 90;
            break;
        case 0x800000:
            team->sizeScale = 105;
            break;
        case 0x1000000:
            team->sizeScale = 110;
            break;
        case 0x2000000:
            team->sizeScale = 115;
            break;
        case 0x4000000:
            team->sizeScale = 120;
            break;
        case 0x8000000:
            team->sizeScale = 125;
            break;
        case 0x10000000:
            team->flags |= 8;
            break;
        case 0x20000000:
            team->flags |= 0x10;
            break;
        case 0x40000000:
            team->flags |= 0x20;
            break;
        }
    }
}

/*
 * Sets up the two versus teams from gNextQuest: each team's monster as a
 * hunt goal; its gauge level, base level and size, the scales at 100; the
 * team item's bits (gTeamItemBits, then the monster's note list); the
 * monster's level (base plus the rolled offset, 0 to 15); then three
 * monsters a side, a 90-frame input lock and the time limit in minutes.
 */
void Versus_Init(void)
{
    s32 i;
    u16 item;
    TeamEntry *entry;
    s32 noteIdx;

    for (i = 0; i < 2; i++) {
        QuestGoal_Add(QUEST_GOAL_SLAY | QUEST_GOAL_HUNT, gNextQuest.teams[i].emKind, 1);
        gQuest.teams[i].pose = 0;
        gQuest.teams[i].gaugeLevel = gNextQuest.teams[i].gaugeLevel;
        gQuest.teams[i].baseLevel = gNextQuest.teams[i].baseLevel;
        gQuest.teams[i].rageScale1 = 100;
        gQuest.teams[i].rageScale2 = 100;
        gQuest.teams[i].sizeScale = 100;
        gQuest.teams[i].sizePct = gNextQuest.teams[i].sizePct;
        gQuest.teams[i].flags = 0;
        gQuest.teams[i].item = item = gNextQuest.teamItems[i];
        if (item == 0) {
            item = 679;
        }
        for (entry = gTeamItemBits; entry->key != 0; entry++) {
            if (item == entry->key) {
                break;
            }
        }
        if (entry->key == 0) {
            noteIdx = MonsterNote_Index(gNextQuest.teams[i].emKind);
            if (noteIdx != MONSTER_NOTE_NONE) {
                for (entry = gMonsterNoteMarks[noteIdx]; entry->key != 0; entry++) {
                    if (item == entry->key) {
                        break;
                    }
                }
            }
        }
        if (entry->key != 0) {
            gQuest.teams[i].pose = entry->pose;
            Versus_ApplyTeamBits(&gQuest.teams[i], entry->bits, gNextQuest.teams[i].emKind);
        } else {
            Versus_ApplyTeamBits(&gQuest.teams[i], 0, gNextQuest.teams[i].emKind);
        }
        gQuest.teams[i].emLevel = gQuest.teams[i].baseLevel + gVersusLevelOffsets[gNextQuest.teams[i].levelRoll];
        if (gQuest.teams[i].emLevel > 15) {
            gQuest.teams[i].emLevel = 15;
        }
        if (gQuest.teams[i].emLevel < 0) {
            gQuest.teams[i].emLevel = 0;
        }
    }
    gQuest.nextSide = 1;
    gQuest.sideEmLeft[0] = 3;
    gQuest.sideEmLeft[1] = 3;
    gQuest.inputLock = 90;
    gQuest.info->timeLimit = gNextQuest.timeLimit * 1800;
}

/*
 * After a versus quest that used an arena record: takes one of the local
 * player's team item from the pouch.
 */
void Versus_ConsumeTeamItem(void)
{
    Player *pl;
    u16 item;

    if (gNextQuest.arenaRecord == -1) {
        return;
    }
    pl = &gPlayers[gScene.localPlayer];
    /* QuestTeam.item read as the team's u16 7: `.item` folds the offset into the load, not the address */
    item = ((u16 *)&gQuest.teams[Versus_Team(pl->side)])[7];
    if (item != 0) {
        Pouch_Add(pl, item, -1);
    }
}

/*
 * After a versus quest (`lost`: the local team lost): the arena record
 * used loses 15% of its vigor (to 50 at least) and the others gain 8% (to
 * 300); the versus minutes and the win or loss count go to the save; the
 * record used takes its team's level, size and gauge, and on a win may
 * raise its gauge level (a chance out of 128 from gArenaGaugeUpChances, by
 * the minutes played and the gauge level).
 */
void Versus_UpdateRecords(s32 lost)
{
    s32 i;
    ArenaEm *rec;
    QuestTeam *team;
    u8 *tbl;
    s32 noteIdx;

    for (i = 0; i < 3; i++) {
        rec = &gSave.arenaEms[i];
        if (rec->emKind != 0) {
            if (gNextQuest.arenaRecord == i) {
                rec->conditionPoints = 0.85f * rec->conditionPoints;
                if (rec->conditionPoints < 50) {
                    rec->conditionPoints = 50;
                }
            } else {
                rec->conditionPoints = 1.08f * rec->conditionPoints;
                if (rec->conditionPoints > 300) {
                    rec->conditionPoints = 300;
                }
            }
        }
    }
    gSave.versusMinutes += (s32)gQuest.elapsed / 1800;
    if (gSave.versusMinutes > 59999) {
        gSave.versusMinutes = 59999;
    }
    if (lost == 0) {
        if (++gSave.versusWins > 9999) {
            gSave.versusWins = 9999;
        }
    } else {
        if (++gSave.versusLosses > 9999) {
            gSave.versusLosses = 9999;
        }
    }
    if (gNextQuest.arenaRecord == -1) {
        return;
    }
    rec = &gSave.arenaEms[gNextQuest.arenaRecord];
    team = &gQuest.teams[Versus_Team(gPlayers[gScene.localPlayer].side)];
    rec->minutes += (s32)gQuest.elapsed / 1800;
    if (rec->minutes > 59999) {
        rec->minutes = 59999;
    }
    rec->level = team->baseLevel;
    rec->sizePct = team->sizePct;
    rec->gaugeLevel = team->gaugeLevel;
    if (lost == 0) {
        if (++rec->wins > 9999) {
            rec->wins = 9999;
        }
        if (rec->gaugeLevel < 4) {
            noteIdx = MonsterNote_Index(rec->emKind);
            if (noteIdx != MONSTER_NOTE_NONE) {
                tbl = gArenaGaugeUpChances[noteIdx];
                if (rec->minutes > 180) {
                    tbl += 16;
                } else if (rec->minutes > 120) {
                    tbl += 12;
                } else if (rec->minutes > 60) {
                    tbl += 8;
                } else if (rec->minutes > 30) {
                    tbl += 4;
                }
                tbl += rec->gaugeLevel;
                if ((Rand_Next(0) & 0x7FU) < *tbl) {
                    rec->gaugeLevel++;
                }
            }
        }
    } else {
        if (++rec->losses > 9999) {
            rec->losses = 9999;
        }
    }
}

/*
 * For a local versus monster with its gauge in use: adds 10 to 30 by the
 * team's gauge level (90 at most). Returns 1.
 */
s32 Versus_AddGauge(Em *em, u8 useKind)
{
    s32 gauge;

    if (em->remote == 0) {
        gauge = em->versusGauge;
        if (gauge != 255 && useKind < 8) {
            switch (gQuest.teams[Versus_Team(em->side)].gaugeLevel) {
            case 0:
                gauge += 10;
                break;
            case 1:
                gauge += 15;
                break;
            case 2:
                gauge += 20;
                break;
            case 3:
                gauge += 25;
                break;
            case 4:
                gauge += 30;
                break;
            }
            if (gauge > 90) {
                gauge = 90;
            }
            em->versusGauge = gauge;
        }
    }
    return 1;
}

/* In a versus quest: 1 when bit `bit` (0-5) of the flags of `em`'s team is set (the team item sets them, Versus_ApplyTeamBits). */
s32 Versus_TeamHasFlag(Em *em, s32 bit)
{
    if (Quest_IsVersus() == 1) {
        switch (bit) {
        case 0:
            if (gQuest.teams[Versus_Team(em->side)].flags & 1) {
                return 1;
            }
            break;
        case 1:
            if (gQuest.teams[Versus_Team(em->side)].flags & 2) {
                return 1;
            }
            break;
        case 2:
            if (gQuest.teams[Versus_Team(em->side)].flags & 4) {
                return 1;
            }
            break;
        case 3:
            if (gQuest.teams[Versus_Team(em->side)].flags & 8) {
                return 1;
            }
            break;
        case 4:
            if (gQuest.teams[Versus_Team(em->side)].flags & 0x10) {
                return 1;
            }
            break;
        case 5:
            if (gQuest.teams[Versus_Team(em->side)].flags & 0x20) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/*
 * In versus: gives `em` the next side (1 and 2 by turns) and its team's
 * level and size.
 */
void Versus_AssignEmTeam(Em *em)
{
    if (Quest_IsVersus() == 1) {
        em->side = gQuest.nextSide++;
        if (gQuest.nextSide > 2) {
            gQuest.nextSide = 1;
        }
        em->level = gQuest.teams[Versus_Team(em->side)].emLevel;
        em->sizePct = gQuest.teams[Versus_Team(em->side)].sizePct;
    }
}

/*
 * The first monster slot on `side`, or NULL (also for side 0). It does not
 * test Em.active.
 */
Em *Em_FindOnSide(u8 side)
{
    Em *em;
    s32 i;

    if (side == 0) {
        return NULL;
    }
    em = gEms;
    for (i = 20; i != 0; i--, em++) {
        if (Side_Match(side, em->side, 1) == 1) {
            return em;
        }
    }
    return NULL;
}

/*
 * Ends the versus quest, won by `side`: the versus-end state, the wait for
 * the others' acknowledgements, the result (cleared with the end-screen
 * capture when the local player is on `side`, else failed), and the
 * versus-end message to the others.
 */
void Versus_End(s8 side)
{
    gQuest.step = 0;
    gQuest.ackTimer = 1800; /* frames: 1 minute */
    gQuest.acks |= 1 << gScene.localPlayer;
    gQuest.state = QSTATE_VERSUS_END;
    if (Side_Match(gPlayers[gScene.localPlayer].side, side, 1) == 1) {
        gSysSettings.clearCapture = 129;
        gQuest.result = QUEST_PHASE_CLEARED;
    } else {
        gQuest.result = QUEST_PHASE_FAILED;
    }
    gQuest.msgArg1 = side;
    gQuest.msgType = QMSG_VERSUS_END;
    gQuest.msgArg0 = 0;
    gQuest.msgArg2 = 0;
    gQuest.msgArea = 0;
    QSync_SendState(6, gScene.localPlayer);
}

/*
 * Puts a new versus monster into its first versus state (eating the bait),
 * its senses delayed, fighting all four players.
 */
void Versus_InitEm(Em *em)
{
    Em_ChangeStateRouted(em, EM_GROUP_VERSUS, 0, EM_SYNC_LOCAL); /* state 0: eating the bait */
    em->senseDelay = 10;
    em->scriptPending = 0;
    em->combatPlayers = 15; /* all four players' bits */
}

/*
 * Picks the versus monster's pose state (1 to 3) from its team's pose,
 * which the team item set.
 */
void Versus_RouteEmByTeam(Em *em)
{
    switch (gQuest.teams[Versus_Team(em->side)].pose) {
    case 0:
        Em_ChangeStateRouted(em, EM_GROUP_VERSUS, 1, EM_SYNC_LOCAL);
        break;
    case 1:
        Em_ChangeStateRouted(em, EM_GROUP_VERSUS, 2, EM_SYNC_LOCAL);
        break;
    case 2:
        Em_ChangeStateRouted(em, EM_GROUP_VERSUS, 3, EM_SYNC_LOCAL);
        break;
    default:
        Em_ChangeStateRouted(em, EM_GROUP_VERSUS, 1, EM_SYNC_LOCAL);
        break;
    }
    em->senseDelay = 10;
    em->scriptPending = 0;
}

/*
 * When a versus monster ends: team 0's always gets defeat camera 2 (bit 2
 * of gQuest.versusDefeat), team 1's camera 3 (bit 3) once team 0 is
 * defeated; returns 1 when a camera was set.
 */
s32 Versus_DefeatCamera(Em *em)
{
    if (Versus_Team(em->side) == 0) {
        gQuest.versusDefeat |= 4;
        CamReq_VersusDefeat(em, 2);
        return 1;
    }
    if (gQuest.versusDefeat & 1) {
        gQuest.versusDefeat |= 8;
        CamReq_VersusDefeat(em, 3);
        return 1;
    }
    return 0;
}

/*
 * A versus monster is defeated: the first defeat fades to black; a local
 * monster marks its team defeated and sends the defeat bits to the others.
 */
void Versus_MarkDefeat(Em *em)
{
    if ((gQuest.versusDefeat & 3) == 0) {
        FadeA_Start(FADE_BLACK);
    }
    if (em->remote == 0) {
        if (Versus_Team(em->side) == 0) {
            gQuest.versusDefeat |= 1;
        } else {
            gQuest.versusDefeat |= 2;
        }
        gQuest.msgArg0 = 0;
        gQuest.msgArg2 = 0;
        gQuest.msgType = QMSG_VERSUS_DEFEAT;
        gQuest.msgArg1 = gQuest.versusDefeat;
        QSync_SendState(6, gScene.localPlayer);
    }
}

s32 Versus_BothDefeated(void)
{
    return (gQuest.versusDefeat & 3) == 3;
}

/*
 * 1 for an offline quest that is neither a drill nor training: the save's
 * records count it.
 */
s32 Quest_KeepsRecords(void)
{
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_IsDrill() == 1) {
        return 0;
    }
    if (Quest_IsTraining() == 1) {
        return 0;
    }
    return 1;
}

s32 Quest_IsTraining(void)
{
    return (gQuest.flags & QUEST_FLAG_TRAINING) != 0;
}

s32 Quest_HasAdvice(void)
{
    return (gQuest.flags & (QUEST_FLAG_TRAINING | QUEST_FLAG_ADVICE)) != 0;
}

s32 Quest_HasDrillFlag(void)
{
    return (gQuest.flags & QUEST_FLAG_DRILL) != 0;
}

s32 Quest_IsMustOrder(void)
{
    return (gQuest.flags & QUEST_FLAG_MUST_ORDER) != 0;
}

/*
 * 1 in versus, or for a built-in online quest with the must-order flag.
 * The callers pass an argument it ignores.
 */
s32 Quest_IsMustOrderOrVersus(void)
{
    if (Quest_IsVersus() == 1) {
        return 1;
    }
    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (gQuest.stage >= QUEST_ID_DOWNLOADED) {
        return 0;
    }
    if (gQuest.flags & QUEST_FLAG_MUST_ORDER) {
        return 1;
    }
    return 0;
}

/*
 * When the quest keeps records: sets the save's monster flag of each
 * gMonsterFlagBits entry whose monster's group was hunted or captured.
 */
void Records_UpdateMonsterFlags(void)
{
    s32 i;
    MonsterBit *pair = gMonsterFlagBits;

    if (Quest_KeepsRecords()) {
        for (i = 0; i < 32; i++, pair++) {
            if (pair->em == 0) {
                break;
            }
            if (gQuest.hunted[MonsterGroup_First(pair->em)] > 0) {
                gSave.monsterFlags |= 1 << pair->bit;
            }
            if (gQuest.captured[MonsterGroup_First(pair->em)] > 0) {
                gSave.monsterFlags |= 1 << pair->bit;
            }
        }
    }
}

/* Counts the quest in the save (to 9999) when it keeps records. */
void Records_CountQuest(void)
{
    if (Quest_KeepsRecords() && gSave.questCount < 9999) {
        gSave.questCount++;
    }
}

/* Counts a drill quest in the save (to 9999), cleared or not. */
void Records_CountFlagged(void)
{
    if ((gQuest.flags & QUEST_FLAG_DRILL) && gSave.drillQuests < 9999) {
        gSave.drillQuests++;
    }
}

/*
 * Outside versus: adds the quest's hunts and captures per monster kind to
 * the save; for a quest cleared by its main goal (not a drill nor
 * training), counts each sub-quest goal met.
 */
void Records_AddQuestKills(void)
{
    s32 i;

    if (Quest_IsVersus() == 1) {
        return;
    }
    for (i = 0; i < 74; i++) {
        if (gQuest.hunted[i] > 0) {
            Record_AddHunted(&gSave, i, gQuest.hunted[i]);
        }
        if (gQuest.captured[i] > 0) {
            gSave.captured[i] += gQuest.captured[i];
        }
    }
    if (Quest_IsDrill() == 1) {
        return;
    }
    if (Quest_IsTraining() == 1) {
        return;
    }
    switch (gQuest.clearType) {
    case QUEST_CLEAR_MAIN:
        for (i = 1; i < 3; i++) {
            if ((gQuest.goals[i].flags & QUEST_GOAL_USED_DONE) == QUEST_GOAL_USED_DONE && gSave.questKills < 9999) {
                gSave.questKills++;
            }
        }
        break;
    case QUEST_CLEAR_ANY:
    case QUEST_CLEAR_FIRST_TWO:
    case QUEST_CLEAR_ALL:
        break;
    }
}

/*
 * Raises the save's record counters: each delivery goal met raises the
 * counter its item maps to (gRecordCountEntries), large captures counter
 * 16; unless `failed`, the counters of two stage groups
 * and one per map.
 */
void Records_UpdateCounters(s32 failed)
{
    s32 i;
    s32 j;
    s32 key;
    RecordCountEntry *entry;

    for (i = 0; i < 3; i++) {
        if ((gQuest.goals[i].flags & QUEST_GOAL_DELIVER_DONE) == QUEST_GOAL_DELIVER_DONE) {
            entry = gRecordCountEntries;
            for (j = 0; (key = entry[j].key) != 0; j++) {
                if (gQuest.goals[i].target == key && gSave.recordCounts[entry[j].counter] < 9999) {
                    gSave.recordCounts[entry[j].counter]++;
                }
            }
        }
    }
    for (i = 0; i < 74; i++) {
        if (gEmIsLarge[i] != 0 && gQuest.captured[i] > 0) {
            gSave.recordCounts[16] += gQuest.captured[i];
            if (gSave.recordCounts[16] > 9999) {
                gSave.recordCounts[16] = 9999;
            }
        }
    }
    if (failed != 0) {
        return;
    }
    switch (gQuest.stage) {
    case 20002:
    case 20003:
        if (gSave.recordCounts[14] < 9999) {
            gSave.recordCounts[14]++;
        }
        break;
    }
    switch (gQuest.stage) {
    case 20007:
    case 20008:
    case 20009:
    case 20010:
        if (gSave.recordCounts[15] < 9999) {
            gSave.recordCounts[15]++;
        }
        break;
    }
    switch (Map_DayId(gScene.map)) {
    case MAP_FOREST_HILLS:
        if (gSave.recordCounts[13] < 9999) {
            gSave.recordCounts[13]++;
        }
        break;
    case MAP_JUNGLE:
        if (gSave.recordCounts[8] < 9999) {
            gSave.recordCounts[8]++;
        }
        break;
    case MAP_DESERT:
        if (gSave.recordCounts[9] < 9999) {
            gSave.recordCounts[9]++;
        }
        break;
    case MAP_SWAMP:
        if (gSave.recordCounts[10] < 9999) {
            gSave.recordCounts[10]++;
        }
        break;
    case MAP_SNOWY_MOUNTAINS:
        if (gSave.recordCounts[11] < 9999) {
            gSave.recordCounts[11]++;
        }
        break;
    case MAP_VOLCANO:
        if (gSave.recordCounts[12] < 9999) {
            gSave.recordCounts[12]++;
        }
        break;
    }
}

/*
 * After a quest (`failed`: not cleared): the arena records recover 8% of their
 * vigor outside versus (Versus_UpdateRecords in versus), and each
 * record's condition is rolled again from it; the record counters; unless
 * failed, the monster flags and the clears of stages 20007 to 20010.
 */
void Quest_UpdateSaveRecords(s32 failed)
{
    s32 i;
    ArenaEm *rec;

    if (Quest_IsVersus() == 0) {
        for (i = 0; i < 3; i++) {
            rec = &gSave.arenaEms[i];
            if (rec->emKind != 0) {
                rec->conditionPoints = 1.08f * rec->conditionPoints;
                if (rec->conditionPoints > 300) {
                    rec->conditionPoints = 300;
                }
            }
        }
    } else {
        Versus_UpdateRecords(failed);
    }
    for (i = 0; i < 3; i++) {
        rec = &gSave.arenaEms[i];
        if (rec->emKind != 0) {
            if (rec->conditionPoints < 80) {
                if (Rand_Next(0) % 100 < 10) {
                    rec->condition = 0;
                } else {
                    rec->condition = 1;
                }
            } else if (rec->conditionPoints < 150) {
                rec->condition = 2;
            } else if (Rand_Next(0) % 100 < 10) {
                rec->condition = 4;
            } else {
                rec->condition = 3;
            }
        }
    }
    Records_CountQuest();
    Records_CountFlagged();
    Records_AddQuestKills();
    Records_UpdateCounters(failed);
    if (failed == 0) {
        Records_UpdateMonsterFlags();
        switch (gQuest.stage) {
        case 20007:
        case 20008:
        case 20009:
        case 20010:
            if (gSave.unk2724 < 200) {
                gSave.unk2724++;
            }
            break;
        }
    }
}

/*
 * When an equipment piece goes into the box and no play style is earned
 * yet: an armour piece whose model has part flag 0 earns the style with
 * the most points (its flag and symbol colour, a notice), and the points
 * are cleared.
 */
void Save_PickBestCounter(EquipItem *piece, u8 gender)
{
    u8 best;
    s32 i;

    if (gSave.styleBits != 0) {
        return;
    }
    if (Armor_HasPartFlag(piece, gender) != 1) {
        return;
    }
    best = 0;
    for (i = 1; i < 3; i++) {
        if (gSave.stylePoints[best] < gSave.stylePoints[i]) {
            best = i;
        }
    }
    gSave.styleFlag = gStyleSymbolColors[best];
    RateFlag_SetAndNotify(gSave.styleFlag);
    gSave.symbolColor = gStyleSymbolColors[best];
    gSave.stylePoints[0] = 0;
    gSave.stylePoints[1] = 0;
    gSave.styleBits |= 1 << (best + 1);
    gSave.stylePoints[2] = 0;
    gSave.stylePoints[best] -= 30;
}

/*
 * Outside versus, drills and training: adds the quest's play style
 * points (captures, large hunts, deliveries, sub-quest hunts); halves them
 * once any reaches 30000 while no style is earned, else earns any style
 * that reaches 100. Returns RateFlag_SetQuiet's result.
 */
s32 Save_AddCounterPoints(void)
{
    s32 rateResult = 0;
    s32 i;
    u8 style;

    if (Quest_IsVersus() == 0 && Quest_IsDrill() == 0 && Quest_IsTraining() == 0) {
        for (i = 0; i < 74; i++) {
            if (gEmIsLarge[i] != 0) {
                if (gQuest.hunted[i] > 0) {
                    gSave.stylePoints[2] += gQuest.hunted[i] * 2;
                }
                if (gQuest.captured[i] > 0) {
                    gSave.stylePoints[0] += gQuest.captured[i] * 5;
                }
            }
        }
        for (i = 0; i < 3; i++) {
            if ((gQuest.goals[i].flags & QUEST_GOAL_DELIVER_DONE) == QUEST_GOAL_DELIVER_DONE) {
                gSave.stylePoints[1] += 3;
            }
        }
        for (i = 1; i < 3; i++) {
            if ((gQuest.goals[i].flags & QUEST_GOAL_HUNT_DONE) == QUEST_GOAL_HUNT_DONE) {
                gSave.stylePoints[2] += 1;
            }
        }
    }
    if (gSave.styleBits == 0) {
        for (style = 0; style < 3; style++) {
            if (gSave.stylePoints[style] >= 30000) {
                gSave.stylePoints[0] /= 2;
                gSave.stylePoints[1] /= 2;
                gSave.stylePoints[2] /= 2;
                break;
            }
        }
    } else {
        for (style = 0; style < 3; style++) {
            if (gSave.stylePoints[style] >= 100) {
                gSave.styleFlag = gStyleSymbolColors[style];
                rateResult = RateFlag_SetQuiet(gSave.styleFlag);
                gSave.stylePoints[0] = 0;
                gSave.stylePoints[1] = 0;
                gSave.stylePoints[2] = 0;
                gSave.styleBits |= 1 << (style + 1);
                gSave.stylePoints[style] -= 30;
            }
        }
    }
    return rateResult;
}

/*
 * 0 in the base camp, with quest flag 0x1000, or in versus; else 1 (the
 * return-to-camp item's test).
 */
s32 Item_UsableOutsideCamp(void)
{
    if (gScene.campArea == gScene.area) {
        return 0;
    }
    if (gQuest.flags & 0x1000) {
        return 0;
    }
    if (Quest_IsVersus() == 1) {
        return 0;
    }
    return 1;
}

/*
 * Starts the quest's 18000-frame (5-minute) countdown; for the local
 * player, with the area intro camera and the message to the others.
 */
void Quest_StartCountdown(Player *pl)
{
    gScene.countdownOn = 1;
    gQuest.countdown = 18000; /* frames: 5 minutes */
    if (Player_IsLocal(pl)) {
        CamReq_AreaIntro();
        gQuest.msgArg0 = 0;
        gQuest.msgType = QMSG_COUNTDOWN_START;
        gQuest.msgArg1 = 0;
        gQuest.msgArg2 = 0;
        QSync_SendState(6, gScene.localPlayer);
    }
}

/*
 * Runs the countdown: when it reaches exactly 0 it stops and the next
 * message is sent to the others; below 0 it just stops.
 */
void Quest_UpdateCountdown(void)
{
    if (gScene.countdownOn == 1 && gQuest.countdown <= 0) {
        if (gQuest.countdown == 0) {
            gScene.countdownOn = 0;
            gQuest.msgType = QMSG_COUNTDOWN_MSG;
            gQuest.msgArg0 = 0;
            gQuest.msgArg2 = 0;
            gQuest.countdownMsg++;
            gQuest.msgArg1 = gQuest.countdownMsg;
            QSync_SendState(6, gScene.localPlayer);
        } else {
            gScene.countdownOn = 0;
        }
    }
}

/* Marks the quest's event as triggered (gQuest.eventTriggered). Kushala,
 * Lao-Shan, Shen Gaoren and Yama Tsukami call it with themselves; the
 * monster is not used. */
void Quest_TriggerEvent(Em *em)
{
    gQuest.eventTriggered = 1;
}

/*
 * Offline with quest flag 0x20: the index of monster kind `id` in the
 * persistent monsters' table (gPersistentEmKinds), else 255.
 */
u8 Quest_FindSpecialMonster(u8 emKind)
{
    u8 *entry = gPersistentEmKinds;
    u8 i;

    if (Game_IsOnline() == 1) {
        return 255;
    }
    if (gQuest.flags & 0x20) {
        for (i = 0; i < 16; i++, entry++) {
            if (*entry == 0) {
                return 255;
            }
            if (*entry == emKind) {
                return i;
            }
        }
    }
    return 255;
}

/*
 * For a persistent monster: its maximum health from the table, its
 * health from the save (full when 0, at least min(max / 10, 1000)), its
 * broken parts; and the snapshot of them in gQuest.
 */
void QuestEm_LoadPersistentHp(Em *em)
{
    s32 slot = Quest_FindSpecialMonster(em->type);
    PersistentEm *rec;

    if (slot != 255) {
        rec = &gSave.persistentEms[slot];
        if (rec->hp == 0) {
            rec->hp = gPersistentEmHp[slot];
        }
        em->hpMax = gPersistentEmHp[slot];
        em->hp = rec->hp;
        if (em->hp < gPersistentEmHp[slot] / 10) {
            em->hp = gPersistentEmHp[slot] / 10;
            if (em->hp > 1000) {
                em->hp = 1000;
            }
        }
        em->partsBroken = rec->partsBroken;
        gQuest.parts[em->type] = em->partsBroken;
        gQuest.savedEmHp = em->hp;
        gQuest.savedEmParts = em->partsBroken;
        gQuest.savedEmType = em->type;
    }
}

/*
 * Saves the persistent monster's health snapshot and broken parts (health
 * 0 or less resets the record), and drops the parts broken in earlier
 * quests from gQuest.parts, so the rewards count only this quest's.
 */
void QuestEm_SavePersistentHp(void)
{
    u8 slot = Quest_FindSpecialMonster(gQuest.savedEmType);
    u16 old;

    if (slot != 255) {
        old = gQuest.parts[gQuest.savedEmType];
        gQuest.parts[gQuest.savedEmType] &= ~gQuest.savedEmParts;
        gSave.persistentEms[slot].partsBroken = old;
        gSave.persistentEms[slot].hp = gQuest.savedEmHp;
        if (gSave.persistentEms[slot].hp <= 0) {
            gSave.persistentEms[slot].hp = 0;
            gSave.persistentEms[slot].partsBroken = 0;
        }
        gSave.lastPersistentEm = slot;
    }
}

/* The file's tables. */
u16 gQuestIdLists_0[72] = {
#include "sub_main/0044AFD0.inc"
};
u16 gQuestIdLists_1[48] = {
#include "sub_main/0044B060.inc"
};
u16 gQuestIdLists_2[48] = {
#include "sub_main/0044B0C0.inc"
};
u16 gQuestIdLists_3[72] = {
#include "sub_main/0044B120.inc"
};
u16 gQuestIdLists_4[32] = {
#include "sub_main/0044B1B0.inc"
};
u16 gQuestIdLists_5[24] = {
#include "sub_main/0044B1F0.inc"
};
u16 gQuestIdLists_6[24] = {
#include "sub_main/0044B220.inc"
};
u16 gQuestIdLists_7[48] = {
#include "sub_main/0044B250.inc"
};
u16 *gQuestIdLists[8] = {
#include "sub_main/0044B2B0.inc"
};
RecordCountEntry gRecordCountEntries[48] = {
#include "sub_main/0044B2D0.inc"
};
s8 gEmTallyClass[80] = {
#include "sub_main/0044B390.inc"
};
u16 gMarkMonsterCands[40] = {
#include "sub_main/0044B3E0.inc"
};
u32 gDrillPayoutThresholds[6] = {
#include "sub_main/0044B430.inc"
};
s8 gVersusLevelOffsets[8] = {
#include "sub_main/0044B448.inc"
};
