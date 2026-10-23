/*
 * The monsters' script interpreter. EmScript_Init loads a monster's script
 * set at its set-up and starts its main script; EmScript_Run runs the
 * script from where it stopped until a command ends the run, reading each
 * op byte and calling its handler, which returns the next command. The
 * handlers follow: the tests (EmScript_If*), which choose the branch that
 * runs, the switches (EmScript_Switch*), the goals (EmScript_Goal*, the
 * point the monster heads for), and the commands that set its variables,
 * needs, timers, loops, area sequences and routes. EmScript_SkipBlock,
 * EmScript_NextCmd and EmScript_SkipToEnd step over commands;
 * Em_StartScript starts a script, and Em_RestartScript the main one again.
 * Last come the route helpers: Em_EnterNextArea, Em_PickRoutePoint,
 * Em_AreaExitPoint, Em_TargetPlayerInRange and EmScript_PickWeighted.
 *
 * The .data is gEmNoticeThresholds, the notice counts
 * EmScript_IfScriptPlayerNoticeOver compares with.
 *
 * The file is built at optimization level 4: EmScript_StartRoute,
 * EmScript_PickNoticedPlayerInArea, EmScript_PickNoticedPlayerSameFloor and
 * Em_AreaExitPoint compile as the game's code only at that level (the
 * script set's accessors and helpers match at either level).
 *
 * After the interpreter come the accessors of a monster's script set
 * (Em.script, loaded with the monster's data by GmLoad_GetEmData), which
 * the interpreter and the trigger picker EmScript_PickTrigger read.
 *
 * A script set is an array of tables of script pointers, one table per kind:
 *   [0]  the main scripts (Em.scriptNo; Em_StartScript)
 *   [1]  the subscripts of command EMOP_CALL_SCRIPT (Em.scriptCallNo)
 *   [2]  the loop scripts (Em.loopScript, Em.loopEndScript), also read as
 *        the loop's point lists (Em.loopPointTable)
 *   [3]  the scripts of trigger EM_TRIGGER_PART_BUILDUP, [4] of trigger EM_TRIGGER_NOTICED
 *   [5]  the area sequences' area lists, [6] their scripts
 *   [7]  the weighted area choices (EmScript_PickWeighted)
 *   [8]  the scripts of trigger 2
 *   [9]  the scripts EmScript_Call runs (Em.subScriptNo)
 *   [10] the scripts of trigger 4, [11] of trigger 8
 *   [12] the route points' scripts (Em.routePointScript)
 *   [13] the script of trigger EM_TRIGGER_PLAYER_AREA, [14] of trigger EM_TRIGGER_BLOCK_AREA (one each)
 *   [15 + k] the jump tables of command EMOP_JUMP_ENTRY
 * A table the monster has no use for is NULL, and its accessor returns NULL;
 * EmScript_AreaSeqScript is the one that doesn't check.
 *
 * Em.scriptFlags says which script thread runs (0x20 or 0x10 a trigger's
 * script, 0x01 the loop, none the main script); EmScript_GetPos and
 * EmScript_SetPos keep each thread's position apart.
 *
 * After the accessors come the script's runtime helpers that use them:
 * EmScript_PickTrigger starts the script of a pending trigger
 * (Em.triggers), EmScript_Restart starts the main script again,
 * Em_SetCombat enters and leaves combat (which stops the trigger threads),
 * and Em_CanEnterArea checks a move against the kind's area lists
 * (by kind and by map).
 */

#include "common.h"
#include "quest.h"
#include "em.h"
#include "em_script.h"
#include "player.h"
#include "area.h"
#include "hud.h"
#include "save.h"
#include "scene.h"
#include "engine_math.h"
#include "gm_load.h"
#include "vec_math.h"
#pragma optimization_level 4

AreaRecord *Stage_GetAreaRecord(s32 area); /* unit's own declaration: the area declared s32; with the definition's u16 the callers would mask it to 16 bits first (EmScript_GoalAreaToward, Em_PickRoutePoint), which the game's code doesn't */
void Vec_Copy(f32 *out, f32 *in);
s16 Player_IsState(void *obj, u8 state, u8 sub);
struct TriHdr;
u8 AreaHit_Raycast(f32 *a, f32 *b, f32 *out, u16 mask);
void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c);
extern s32 gEmScriptSwitchValue;
struct EmIdEntry *Em_FindAreaEntry(struct Em *em, struct EmIdEntry *entry, s16 id);
u16 Area_IdForCurrentTime(u16);
extern s16 gEmCombatInit[];
/* unit's own declaration: n declared s32; with the definition's s16 EmScript_AddHp's call compiles differently */
void Em_AddHp(Em *em, s32 n);
extern s32 gEmNoticeThresholds[];
struct AreaTri;
/* unit's own declaration: declared returning s32; the definition returns u8, with which EmScript_IfCeilingAbove compiles differently */
s32 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
extern EmAreaValue *gEmAreaTimer2Values[];
extern s32 gEmGoalPoints[];

/*
 * Sets scriptAbort to value (script is unused). Nonzero makes EmScript_Run
 * restart the monster's script (Em_RestartScript) before its next command.
 * EmScript_Run passes 2 when one run reaches 1000 commands.
 */
static void EmScript_SetAbort(Em *em, u8 *script, s32 value)
{
    em->scriptAbort = (u8) value;
}

/*
 * Resets the monster's script state at its set-up: no call or restart
 * pending, scriptPlayer -1 (none), the kind's script set (GmLoad_GetEmData)
 * loaded into script, script 0 started as the main script, the loop and the
 * area sequence off, destArea and prevArea 65535 (none), the goal cleared.
 */
void EmScript_Init(Em *em)
{

    em->scriptNo = 0;
    em->scriptCallNo = 0;
    em->scriptCallEntry = 0;
    em->scriptCallDepth = 0;
    em->scriptRestart = 0;
    *(s8 *) &em->scriptPlayer = -1;
    em->script = GmLoad_GetEmData(em->type);
    em->mainScriptPos = (u8 *) Em_StartScript(em);
    em->loopPoint = -1;
    em->loopCount = -1;
    em->areaSeqPos = -1;
    em->areaSeqLen = -1;
    *(u16 *) &em->destArea = 65535;
    em->prevArea = 65535;
    em->unk7A8 = 0;
    gEmScriptSwitchValue = 0;
    em->scriptPos = em->mainScriptPos;
    em->goalKind = 0;
    em->goalArg = 0;
    em->goalNo = 0;
    em->curGoalKind = 0;
    em->curGoalArg = 0;
    em->curGoalNo = 0;
}

/*
 * Runs the monster's script from where it stopped until a command ends the
 * run; nothing in EM_GROUP_DEFEAT and EM_GROUP_EVENT. It applies a requested
 * restart (EmScript_Restart), then starts at the first pending trigger's
 * script (EmScript_PickTrigger) or the thread's saved position
 * (EmScript_GetPos). Each pass reads the op byte and calls its handler, which
 * returns the next command. Command 5 reads a group, a state and
 * routedStateArg, changes state (Em_ChangeStateRouted, not when remote),
 * saves the position and ends the run. scriptAbort, a NULL pointer or an
 * unknown op restart the script; 1000 commands in one run set scriptAbort.
 */
void EmScript_Run(Em *em)
{
    u8 run;
    u8 *script;
    s32 commandCount;
    s32 group;
    s32 state;
    s32 self;

    run = 1;
    self = (s32) em;
    commandCount = 0;
    if (em->stateGroup == EM_GROUP_DEFEAT) {
        return;
    }
    if (em->stateGroup == EM_GROUP_EVENT) {
        return;
    }
    em->noReact = 0;
    EmScript_Restart(em);
    if ((script = EmScript_PickTrigger(em)) == NULL) {
        script = EmScript_GetPos(em);
    }
    while (run) {
        commandCount++;
        if (commandCount >= 1000) {
            EmScript_SetAbort(em, script, 2);
        }
        if (em->scriptAbort != 0) {
            Em_RestartScript(em);
            break;
        }
        if (script == NULL) {
            em->scriptAbort = 0;
            Em_RestartScript(em);
            break;
        }
        switch (*script) {
        case EMOP_IF_SENSED:
            script = EmScript_IfSensed(em, script + 1);
            break;
        case EMOP_IF_NOTICED:
            script = EmScript_IfNoticed(em, script + 1);
            break;
        case EMOP_IF_CAN_ENTER_NEXT_AREA:
            script = EmScript_IfCanEnterNextArea(em, script + 1);
            break;
        case EMOP_RESTART:
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            break;
        case EMOP_CHANGE_STATE:
            script++;
            group = *script++;
            state = *script++;
            em->routedStateArg = *script++;
            if (em->remote == 0) {
                Em_ChangeStateRouted(em, group, state, 1);
            }
            EmScript_SetPos(em, script);
            run = 0;
            break;
        case EMOP_SET_GOAL:
            script = EmScript_SetGoal(em, script + 1);
            break;
        case EMOP_START:
            script = (u8 *) EmScript_Start(em, script + 1);
            break;
        case EMOP_IF_ON_GROUND:
            script = EmScript_IfOnGround(em, script + 1);
            break;
        case EMOP_IF_STANCE_FAMILY:
            script = EmScript_IfStanceFamily(em, script + 1);
            break;
        case EMOP_IF_IN_AIR:
            script = EmScript_IfInAir(em, script + 1);
            break;
        case EMOP_SET_HIT_REACT_MODE:
            script = EmScript_SetHitReactMode(em, script + 1);
            break;
        case EMOP_IF_COMBAT_IS:
            script = EmScript_IfCombatIs(em, script + 1);
            break;
        case EMOP_SET_VAR:
            script = EmScript_SetVar(em, script + 1);
            break;
        case EMOP_CLEAR_VAR:
            script = EmScript_ClearVar(em, script + 1);
            break;
        case EMOP_IF_IN_AREA:
            script = EmScript_IfInArea(em, script + 1);
            break;
        case EMOP_START_LOOP:
            script = EmScript_StartLoop(em, script + 1);
            break;
        case EMOP_NEXT_LOOP_POINT:
            script = EmScript_NextLoopPoint(em, script + 1);
            break;
        case EMOP_GOAL_ALERTED_PLAYER:
            script = EmScript_GoalAlertedPlayer(em, script + 1);
            break;
        case EMOP_GOAL_NOTICED_PLAYER:
            script = EmScript_GoalNoticedPlayer(em, script + 1);
            break;
        case EMOP_GOAL_SCRIPT_PLAYER:
            script = EmScript_GoalScriptPlayer(em, script + 1);
            break;
        case EMOP_IF_TARGET_OFF_HEADING:
            script = EmScript_IfTargetOffHeading(em, script + 1);
            break;
        case EMOP_SWITCH_AREA:
            script = EmScript_SwitchArea(em, script + 1);
            break;
        case EMOP_CALL:
            script = EmScript_Call(em, script + 1);
            break;
        case EMOP_SET_AREA_SEQ:
            script = EmScript_SetAreaSeq(em, script + 1);
            break;
        case EMOP_STEP_AREA_SEQ:
            script = EmScript_StepAreaSeq(em, script + 1);
            break;
        case EMOP_GOAL_DEST_AREA:
            script = EmScript_GoalDestArea(em, script + 1);
            break;
        case EMOP_GOAL_AREA_TOWARD:
            script = EmScript_GoalAreaToward(em, script + 1);
            break;
        case EMOP_IF_REQUESTED_IS:
            script = EmScript_IfRequestedIs(em, script + 1);
            break;
        case EMOP_SWITCH_HP_PERCENT:
            script = EmScript_SwitchHpPercent(em, script + 1);
            break;
        case EMOP_SWITCH_REQUEST_ID:
            script = EmScript_SwitchRequestId(em, script + 1);
            break;
        case EMOP_SET_REQUEST:
            script = EmScript_SetRequest(em, script + 1);
            break;
        case EMOP_SWITCH_TARGET_ANGLE:
            script = EmScript_SwitchTargetAngle(em, script + 1);
            break;
        case EMOP_IF_NEED2_LOW:
            script = EmScript_IfNeed2Low(em, script + 1);
            break;
        case EMOP_IF_GOAL_NEAR_XZ:
            script = EmScript_IfGoalNearXZ(em, script + 1);
            break;
        case EMOP_IF_GOAL_NEAR:
            script = EmScript_IfGoalNear(em, script + 1);
            break;
        case EMOP_SWITCH_KIND_IN_AREA:
            script = EmScript_SwitchKindInArea(em, script + 1);
            break;
        case EMOP_REPEAT:
            script = EmScript_Repeat(em, script + 1);
            break;
        case EMOP_STOP_REPEAT:
            script = EmScript_StopRepeat(em, script + 1);
            break;
        case EMOP_SET_NO_ALERT:
            script = EmScript_SetNoAlert(em, script + 1);
            break;
        case EMOP_SWITCH_VARIANT:
            script = EmScript_SwitchVariant(em, script + 1);
            break;
        case EMOP_IF_PLAYER_IN_AREA:
            script = EmScript_IfPlayerInArea(em, script + 1);
            break;
        case EMOP_IF_AREA_TIMER1_OVER:
            script = EmScript_IfAreaTimer1Over(em, script + 1);
            break;
        case EMOP_IF_AREA_TIMER2_OVER:
            script = EmScript_IfAreaTimer2Over(em, script + 1);
            break;
        case EMOP_IF_VAR_IS:
            script = EmScript_IfVarIs(em, script + 1);
            break;
        case EMOP_SWITCH_KIND_GROUP:
            script = EmScript_SwitchKindGroup(em, script + 1);
            break;
        case EMOP_SWITCH_TYPE:
            script = EmScript_SwitchType(em, script + 1);
            break;
        case EMOP_GOAL_ACTION_GOAL:
            script = EmScript_GoalActionGoal(em, script + 1);
            break;
        case EMOP_SELECT_SIGHT_ROW:
            script = EmScript_SelectSightRow(em, script + 1);
            break;
        case EMOP_IF_PLAYER_CARRYING:
            script = EmScript_IfPlayerCarrying(em, script + 1);
            break;
        case EMOP_SET_NEED4:
            script = EmScript_SetNeed4(em, script + 1);
            break;
        case EMOP_SET_GOAL_KIND:
            script = EmScript_SetGoalKind(em, script + 1);
            break;
        case EMOP_IF_HIT_REACT_MODE:
            script = EmScript_IfHitReactMode(em, script + 1);
            break;
        case EMOP_SWITCH_HIT_REACT_MODE:
            script = EmScript_SwitchHitReactMode(em, script + 1);
            break;
        case EMOP_IF_STATE:
            script = EmScript_IfState(em, script + 1);
            break;
        case EMOP_IF_RAGING:
            script = EmScript_IfRaging(em, script + 1);
            break;
        case EMOP_IF_DRILL:
            script = EmScript_IfDrill(em, script + 1);
            break;
        case EMOP_IF_ON_PASS_THROUGH_FLOOR:
            script = EmScript_IfOnPassThroughFloor(em, script + 1);
            break;
        case EMOP_IF_BLIND:
            script = EmScript_IfBlind(em, script + 1);
            break;
        case EMOP_IF_NO_SENSE_DELAY:
            script = EmScript_IfNoSenseDelay(em, script + 1);
            break;
        case EMOP_IF_LINKED:
            script = EmScript_IfLinked(em, script + 1);
            break;
        case EMOP_IF_LINK_COMBAT_SAME_AREA:
            script = EmScript_IfLinkCombatSameArea(em, script + 1);
            break;
        case EMOP_IF_PREV_AREA:
            script = EmScript_IfPrevArea(em, script + 1);
            break;
        case EMOP_SWITCH_PREV_AREA:
            script = EmScript_SwitchPrevArea(em, script + 1);
            break;
        case EMOP_START_ROUTE:
            script = EmScript_StartRoute(em, script + 1);
            break;
        case EMOP_SET_COMBAT:
            script = EmScript_SetCombat(em, script + 1);
            break;
        case EMOP_ADD_HP:
            script = EmScript_AddHp(em, script + 1);
            break;
        case EMOP_IF_GOAL_ANGLE_OVER:
            script = EmScript_IfGoalAngleOver(em, script + 1);
            break;
        case EMOP_IF_GOAL_ANGLE_OVER_CAPPED:
            script = EmScript_IfGoalAngleOverCapped(em, script + 1);
            break;
        case EMOP_IF_LINK_SAME_AREA:
            script = EmScript_IfLinkSameArea(em, script + 1);
            break;
        case EMOP_IF_LONE_PLAYER_BAIT:
            script = EmScript_IfLonePlayerBait(em, script + 1);
            break;
        case EMOP_IF_SCRIPT_PLAYER_STATE:
            script = EmScript_IfScriptPlayerState(em, script + 1);
            break;
        case EMOP_IF_HOOKED:
            script = EmScript_IfHooked(em, script + 1);
            break;
        case EMOP_SET_DELAY:
            script = EmScript_SetDelay(em, script + 1);
            break;
        case EMOP_GOAL_PLAYER_FLOOR_SPOT:
            script = EmScript_GoalPlayerFloorSpot(em, script + 1);
            break;
        case EMOP_IF_SCRIPT_PLAYER_SEEN:
            script = EmScript_IfScriptPlayerSeen(em, script + 1);
            break;
        case EMOP_SET_ALERT_BUILDUP:
            script = EmScript_SetAlertBuildup(em, script + 1);
            break;
        case EMOP_SET_NOTICE_COUNT:
            script = EmScript_SetNoticeCount(em, script + 1);
            break;
        case EMOP_COMPUTE_GOAL:
            script = EmScript_ComputeGoal(em, script + 1);
            break;
        case EMOP_ADD_NEED2:
            script = EmScript_AddNeed2(em, script + 1);
            break;
        case EMOP_ADD_NEED1:
            script = EmScript_AddNeed1(em, script + 1);
            break;
        case EMOP_ADD_NEED3:
            script = EmScript_AddNeed3(em, script + 1);
            break;
        case EMOP_IF_STANCE_UNDER:
            script = EmScript_IfStanceUnder(em, script + 1);
            break;
        case EMOP_PICK_NOTICED_PLAYER_REACHABLE:
            script = EmScript_PickNoticedPlayerReachable(em, script + 1);
            break;
        case EMOP_PICK_NOTICED_PLAYER_IN_AREA:
            script = EmScript_PickNoticedPlayerInArea(em, script + 1);
            break;
        case EMOP_IF_SCRIPT_PLAYER_IN_AREA:
            script = EmScript_IfScriptPlayerInArea(em, script + 1);
            break;
        case EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH:
            script = EmScript_IfScriptPlayerNoticeHigh(em, script + 1);
            break;
        case EMOP_IF_QUEST_ID:
            script = EmScript_IfQuestId(em, script + 1);
            break;
        case EMOP_SWITCH_QUEST_KEY:
            script = EmScript_SwitchQuestKey(em, script + 1);
            break;
        case EMOP_GOAL_LINK_TARGET:
            script = EmScript_GoalLinkTarget(em, script + 1);
            break;
        case EMOP_IF_ON_CEILING:
            script = EmScript_IfOnCeiling(em, script + 1);
            break;
        case EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR:
            script = EmScript_IfScriptPlayerFloorAttr(em, script + 1);
            break;
        case EMOP_SET_NOTICE_TIMER:
            script = EmScript_SetNoticeTimer(em, script + 1);
            break;
        case EMOP_IF_CEILING_ABOVE:
            script = EmScript_IfCeilingAbove(em, script + 1);
            break;
        case EMOP_IF_GOAL_VALID:
            script = EmScript_IfGoalValid(em, script + 1);
            break;
        case EMOP_IF_FLOOR_ATTR:
            script = EmScript_IfFloorAttr(em, script + 1);
            break;
        case EMOP_PICK_NOTICED_PLAYER_SAME_FLOOR:
            script = EmScript_PickNoticedPlayerSameFloor(em, script + 1);
            break;
        case EMOP_IF_GATE_OPEN:
            script = EmScript_IfGateOpen(em, script + 1);
            break;
        case EMOP_SET_AREA_TIMER2:
            script = EmScript_SetAreaTimer2(em, script + 1);
            break;
        case EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR:
            script = EmScript_SwitchScriptPlayerFloorAttr(em, script + 1);
            break;
        case EMOP_IF_VARIANT_BITS:
            script = EmScript_IfVariantBits(em, script + 1);
            break;
        case EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER:
            script = EmScript_IfScriptPlayerNoticeOver(em, script + 1);
            break;
        case EMOP_CLEAR_NOTICE_COUNTS:
            script = EmScript_ClearNoticeCounts(em, script + 1);
            break;
        case EMOP_IF_ANY_PLAYER_RIDING:
            script = EmScript_IfAnyPlayerRiding(em, script + 1);
            break;
        case EMOP_IF_NOT_REMOTE:
            script = EmScript_IfNotRemote(em, script + 1);
            break;
        case EMOP_RESTART_MAIN:
            script = (u8 *) EmScript_RestartMain(em, (s32) (script + 1));
            break;
        case EMOP_RANDOM_BRANCH:
            script = EmScript_RandomBranch(em, script + 1);
            break;
        case EMOP_CALL_SCRIPT:
            script = EmScript_CallScript(em, script + 1);
            break;
        case EMOP_JUMP_ENTRY:
            script = EmScript_JumpEntry(em, script + 1);
            break;
        case EMOP_SWITCH_TARGET_DIST:
            script = EmScript_SwitchTargetDist(em, script + 1);
            break;
        case EMOP_ADVANCE_STATE_RAND:
        case EMOP_ADVANCE_STATE_RAND_ALT:
            EmScript_AdvanceStateRand(em);
            script++;
            break;
        case EMOP_END:
            script = EmScript_End(em, script + 1);
            break;
        case EMOP_PLACE_AT_POINT:
            script = EmScript_PlaceAtPoint(em, script + 1);
            break;
        case EMOP_FACE_POINT:
            script = EmScript_FacePoint(em, script + 1);
            break;
        case EMOP_SET_HEADING:
            script = EmScript_SetHeading(em, script + 1);
            break;
        case EMOP_NOP:
            script = (u8 *) EmScript_Nop(self, (s32) (script + 1));
            break;
        case EMOP_NOP_ALT:
            script = (u8 *) EmScript_NopAlt(self, (s32) (script + 1));
            break;
        case EMOP_SWITCH_SCRIPT_VALUE:
            script = EmScript_SwitchScriptValue(em, script + 1);
            break;
        case EMOP_IF_LEVEL_AT_LEAST:
            script = EmScript_IfLevelAtLeast(em, script + 1);
            break;
        case EMOP_IF_VERSUS:
            script = EmScript_IfVersus(em, script + 1);
            break;
        case EMOP_SWITCH_ITEM_USE_KIND:
            script = EmScript_SwitchItemUseKind(em, script + 1);
            break;
        case EMOP_IF_VERSUS_ITEM_USED:
            script = EmScript_IfVersusItemUsed(em, script + 1);
            break;
        case EMOP_SWITCH_SPAWN_PARAM:
            script = EmScript_SwitchSpawnParam(em, script + 1);
            break;
        case EMOP_SWITCH_SEASON:
            script = EmScript_SwitchSeason(em, script + 1);
            break;
        case EMOP_IF_DAY:
            script = EmScript_IfDay(em, script + 1);
            break;
        case EMOP_IF_GOAL_PLAYER_IN_ARC:
            script = EmScript_IfGoalPlayerInArc(em, script + 1);
            break;
        case EMOP_SWITCH_KIND_QUERY:
            script = EmScript_SwitchKindQuery(em, script + 1);
            break;
        case EMOP_SWITCH_BASE_AREA:
            script = EmScript_SwitchBaseArea(em, script + 1);
            break;
        case EMOP_IF_IN_ARENA:
            script = EmScript_IfInArena(em, script + 1);
            break;

        default:
            Em_RestartScript(em);
            run = 0;
            break;
        }
    }
    em->scriptOffset = (s32) em->mainScriptPos - (s32)em->scriptPos;
}

void EmScript_AdvanceStateRand(Em *em)
{
    em->stateRand++;
}

/*
 * Test op EMOP_IF_SENSED: true when a player is within the monster's sense reach (a
 * sensedPlayers bit of a quest player).
 */
u8 *EmScript_IfSensed(Em *em, u8 *script)
{
    s32 i;
    s32 n;

    switch (*script) {
    case EMBLK_OPEN:
        script++;
        for (i = 0, n = 0; i < gScene.playerCount; i++) {
            if (em->sensedPlayers & (1 << i)) {
                n++;
            }
        }
        if (n == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_SENSED && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_SENSED && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_SENSED, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_SENSED && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script + 1, EMOP_IF_SENSED);
        break;
    case EMBLK_END_IF:
        script++;
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_NOTICED: true when the monster has noticed a player (noticed);
 * otherwise it also sets scriptPlayer to -1.
 */
u8 *EmScript_IfNoticed(Em *em, u8 *script)
{
    switch (*script) {
    case EMBLK_OPEN:
        script++;
        if (em->noticed == 0) {
            *(s8 *) &em->scriptPlayer = -1;
            for (;;) {
                if (script[0] == EMOP_IF_NOTICED && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_NOTICED && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_NOTICED, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_NOTICED && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script + 1, EMOP_IF_NOTICED);
        break;
    case EMBLK_END_IF:
        script++;
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_CAN_ENTER_NEXT_AREA: true when the monster may move on to nextArea (a real area
 * other than its own, Em_CanEnterArea); otherwise it also clears the goal.
 */
u8 *EmScript_IfCanEnterNextArea(Em *em, u8 *script)
{
    s32 ok;

    switch (*script) {
    case EMBLK_OPEN:
        script++;
        if (em->area == em->nextArea || em->nextArea == 0xFFFF) {
            goto skip;
        }
        ok = Em_CanEnterArea(em, em->nextArea) == 1;
        switch (ok) {
        case 1:
        case 2:
            break;
        case 0:
        skip:
            em->goalKind = 0;
            em->goalArg = 0;
            em->goalNo = 0;
            em->curGoalKind = 0;
            em->curGoalArg = 0;
            em->curGoalNo = 0;
            for (;;) {
                if (script[0] == EMOP_IF_CAN_ENTER_NEXT_AREA && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_CAN_ENTER_NEXT_AREA && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_CAN_ENTER_NEXT_AREA, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_CAN_ENTER_NEXT_AREA && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
            break;
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script + 1, EMOP_IF_CAN_ENTER_NEXT_AREA);
        break;
    case EMBLK_END_IF:
        script++;
        break;
    }
    return script;
}

/* Script command: start script *p (Em_StartScript). */
s32 EmScript_Start(Em *em, u8 *script)
{
    em->scriptNo = *script;
    return Em_StartScript(em);
}

/* Test op EMOP_IF_ON_GROUND: true when stance is EM_STANCE_GROUND. */
u8 *EmScript_IfOnGround(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->stance != 0) {
            for (;;) {
                if (script[0] == EMOP_IF_ON_GROUND && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_ON_GROUND && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_ON_GROUND, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_ON_GROUND && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_ON_GROUND);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_STANCE_FAMILY: true when stance is EM_STANCE_FAMILY (the family's own
 * stance).
 */
u8 *EmScript_IfStanceFamily(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->stance != 1) {
            for (;;) {
                if (script[0] == EMOP_IF_STANCE_FAMILY && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_STANCE_FAMILY && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_STANCE_FAMILY, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_STANCE_FAMILY && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_STANCE_FAMILY);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_IN_AIR: true when stance is EM_STANCE_AIR. */
u8 *EmScript_IfInAir(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->stance != 2) {
            for (;;) {
                if (script[0] == EMOP_IF_IN_AIR && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_IN_AIR && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_IN_AIR, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_IN_AIR && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_IN_AIR);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

u8 *EmScript_SetHitReactMode(Em *em, u8 *script)
{
    em->hitReactMode = *script;
    return script + 1;
}

/*
 * Test op EMOP_IF_COMBAT_IS: true when combat equals the byte (1 fighting the hunters);
 * also copies combat to scriptCombat.
 */
u8 *EmScript_IfCombatIs(Em *em, u8 *script)
{
    u8 wanted;

    switch (*script++) {
    case EMBLK_OPEN:
        wanted = *script++;
        em->scriptCombat = em->combat;
        if (em->combat != wanted) {
            for (;;) {
                if (script[0] == EMOP_IF_COMBAT_IS && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_COMBAT_IS && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_COMBAT_IS, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_COMBAT_IS && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_COMBAT_IS);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Script op EMOP_SET_VAR: k and v are the two bytes at p; sets the Em byte k picks to v (1: blind,
 * 2: unk814, 3: senseDelay, 4: unk78E[0], 5/8/9: work, 6/7: work bytes 0x44/0x45; 5-9 only for
 * some types and when not remote); k == 0 sets all of them (k 1-10). w is set inside the type
 * checks of cases 6 and 7: the loop hoists &em->work from there into its preheader (between the
 * compare constants 55 and 45) and keeps 0x44/0x45 as the store offsets.
 */
u8 *EmScript_SetVar(Em *em, u8 *script)
{
    u8 k;
    u8 value;
    s32 i;
    s32 n;
    u8 *w;

    k = *script++;
    value = *script++;
    if (k == 0) {
        k = 1;
        n = 10;
    } else {
        n = 1;
    }
    for (i = 0; i < n; i++, k++) {
        switch (k) {
        case 1:
            em->blind = value;
            break;
        case 2:
            em->need2State = value;
            break;
        case 3:
            em->senseDelay = (s8) value;
            break;
        case 4:
            em->scriptVar = value;
            break;
        case 5:
            if (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG) {
                if (em->remote == 0) {
                    em->work = value;
                }
            }
            break;
        case 6:
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                w = &em->work;
                if (em->remote == 0) {
                    w[68] = value;
                }
            }
            break;
        case 7:
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                w = &em->work;
                if (em->remote == 0) {
                    w[69] = value;
                }
            }
            break;
        case 8:
            if (em->type == EM_KIND_SHEN_GAOREN && em->remote == 0) {
                em->work = value;
            }
            break;
        case 9:
            if (em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
                if (em->remote == 0) {
                    em->work = value;
                }
            }
            break;
        }
    }
    return script;
}

/* Script op EMOP_CLEAR_VAR: clears the Em byte the byte at p picks (the fields of EmScript_SetVar; 10:
 * unk882 = 65535); 0 clears all of them. w is set as in EmScript_SetVar.
 */
u8 *EmScript_ClearVar(Em *em, u8 *script)
{
    u8 k;
    s32 i;
    s32 n;
    u8 *w;

    k = script[0];
    if (k == 0) {
        k = 1;
        n = 10;
    } else {
        n = 1;
    }
    i = 0;
    if (i < n) {
        do {
            switch (k) {
            case 1:
                em->blind = 0;
                break;
            case 2:
                em->need2State = 0;
                break;
            case 3:
                em->senseDelay = 0;
                break;
            case 4:
                em->scriptVar = 0;
                break;
            case 5:
                if (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG) {
                    if (em->remote == 0) {
                        em->work = 0;
                    }
                }
                break;
            case 6:
                if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                    w = &em->work;
                    if (em->remote == 0) {
                        w[68] = 0;
                    }
                }
                break;
            case 7:
                if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                    w = &em->work;
                    if (em->remote == 0) {
                        w[69] = 0;
                    }
                }
                break;
            case 8:
                if (em->type == EM_KIND_SHEN_GAOREN && em->remote == 0) {
                    em->work = 0;
                }
                break;
            case 9:
                if (em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
                    if (em->remote == 0) {
                        em->work = 0;
                    }
                }
                break;
            case 10:
                em->prevArea = 65535;
                break;
            }
            i++;
            k++;
        } while (i < n);
    }
    return script + 1;
}

/*
 * Test op EMOP_IF_IN_AREA: true when the monster's area is the area of the next two
 * bytes (high byte first) at the current time of day (Area_IdForCurrentTime).
 */
u8 *EmScript_IfInArea(Em *em, u8 *script)
{
    u16 id;

    switch (*script++) {
    case EMBLK_OPEN:
        id = script[0] << 8;
        id |= script[1];
        script += 2;
        if (em->area != Area_IdForCurrentTime(id)) {
            for (;;) {
                if (script[0] == EMOP_IF_IN_AREA && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_IN_AREA && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_IN_AREA, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_IN_AREA && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_IN_AREA);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Command EMOP_START_LOOP, five bytes: the loop mode (0 to 2, over the points of the
 * monster's area), the passes, the point list, the script run at each point
 * and the one run when the loop ends. Saves the thread's resume point, sets
 * scriptFlags bit 1 (the loop thread) and looks up the area's entry
 * (Em_FindAreaEntry in table 1): with none the loop is dropped and the script
 * goes on; else loopCount and areaPointCount are set and it returns the start
 * of the per-point script.
 */
u8 *EmScript_StartLoop(Em *em, u8 *script)
{
    u8 n;
    s16 *areaEntry;

    em->loopIndex = 0;
    em->loopMode = script[0];
    n = script[1];
    em->loopPointTable = *(script += 2);
    em->loopScript = script[1];
    em->loopEndScript = *(script += 2);
    EmScript_SetPos(em, script + 1);
    em->scriptFlags |= 1;
    switch (em->loopMode) {
    case 0:
    case 1:
    case 2:
        areaEntry = (s16 *)Em_FindAreaEntry(em, em->tables[1], em->area);
        if (areaEntry == NULL) {
            em->scriptFlags &= 0xFE;
            return script + 1;
        }
        if (em->loopMode != 1 && areaEntry[1] < n) {
            n = areaEntry[1];
        }
        em->loopCount = n;
        em->areaPointCount = areaEntry[1];
        em->loopScriptPos = EmScript_LoopScript(em, em->loopScript);
        break;
    }
    return em->loopScriptPos;
}

//---
u8 *EmScript_NextLoopPoint(Em *em, u8 *script)
{
    u8 list[16];
    s8 n;
    s8 i;

    switch (em->loopMode) {
    case 0:
        if (em->loopPoint == -1 || em->loopPoint >= em->areaPointCount) {
            for (i = 0, n = 0; i < em->areaPointCount; i++) {
                list[n++] = i;
            }
            if (n == 0) {
                em->loopPoint = 0;
                Em_RestartScript(em);
            } else {
                em->loopPoint = list[(s8) (em->stateRand % n)];
            }
        }
        break;
    case 1:
        if (em->loopPoint == -1 || em->loopPoint >= em->areaPointCount) {
            em->loopPoint = *(u8 *)EmScript_LoopPointList(em, em->loopPointTable);
        }
        break;
    case 2:
        if (em->loopPoint == -1 || em->loopPoint >= em->areaPointCount) {
            em->loopPoint = 0;
        }
        break;
    }
    em->goalKind = 2;
    em->goalArg = 1;
    em->goalNo = em->loopPoint;
    return script;
}

/*
 * Sets the goal to the highest-numbered player in alertedPlayers (goalKind
 * 1); when not remote also pulses the sync (syncPulse[0] 2).
 */
u8 *EmScript_GoalAlertedPlayer(Em *em, u8 *script)
{
    s32 i;

    for (i = 0; i < gScene.playerCount; i++) {
        if (em->alertedPlayers & (1 << i)) {
            em->goalNo = i;
        }
    }
    if (em->remote == 0) {
        em->syncPulse[0] = 2;
    }
    em->goalKind = 1;
    em->goalArg = 0;
    return script;
}

/*
 * Sets the goal to the highest-numbered player in noticed (goalKind 1), which
 * also becomes scriptPlayer; pulses the sync and ends the loop and
 * trigger-0x10 threads (scriptFlags bits 1 and 0x10).
 */
u8 *EmScript_GoalNoticedPlayer(Em *em, u8 *script)
{
    s32 i;

    for (i = 0; i < gScene.playerCount; i++) {
        if (em->noticed & (1 << i)) {
            em->goalNo = i;
            em->scriptPlayer = (u16) em->goalNo;
        }
    }
    em->goalKind = 1;
    em->goalArg = 0;
    if (em->remote == 0) {
        em->syncPulse[1] = 2;
    }
    em->scriptFlags &= 0xEE;
    return script;
}

/*
 * Sets the goal to the picked player (goalKind 1, goalNo the low nibble of
 * scriptPlayer), or to no player (65535) when scriptPlayer is -1.
 */
u8 *EmScript_GoalScriptPlayer(Em *em, u8 *script)
{
    s8 player;

    em->goalKind = 1;
    em->goalArg = 0;
    player = (s8) em->scriptPlayer;
    if (player == -1) {
        *(u16 *) &em->goalNo = 65535;
    } else {
        em->goalNo = player & 0xF;
    }
    return script;
}

/*
 * Test op EMOP_IF_TARGET_OFF_HEADING with a byte k in degrees: the block runs when the goal
 * player's direction is at least k degrees off the heading rotY (the block is
 * skipped for a goal that is not a player).
 */
u8 *EmScript_IfTargetOffHeading(Em *em, u8 *script)
{
    s32 ok;
    u8 k;
    u16 limit;
    u16 angleDelta;
    u16 targetNo;
    Player *pl;
    f32 targetPos[4];
    f32 targetWorldPos[4];

    switch (*script++) {
    case EMBLK_OPEN:
        k = *script++;
        limit = (s32) (65536.0f * k / 360.0f + 0.5f);
        ok = 0;
        if (em->goalKind == 1 && *(u16 *) &em->goalArg == 0 && (s16) (targetNo = *(u16 *) &em->goalNo) != -1) {
            Em_GetTargetPos(em, targetNo, targetPos);
            pl = &gPlayers[targetNo];
            Area_ToWorld(pl->area, targetPos, targetWorldPos);
            angleDelta = Math_AngleXZ(em->mapPos, targetWorldPos) - em->rotY;
            if (angleDelta > (ANGLE_TURN / 2)) {
                angleDelta = ANGLE_TURN - angleDelta;
            }
            if (limit > angleDelta) {
                ok = 1;
            }
        } else {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == EMOP_IF_TARGET_OFF_HEADING && script[1] == EMBLK_ELSE) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == EMOP_IF_TARGET_OFF_HEADING && script[1] == EMBLK_END_IF) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, EMOP_IF_TARGET_OFF_HEADING, EMBLK_END_IF);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == EMOP_IF_TARGET_OFF_HEADING && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_TARGET_OFF_HEADING && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_TARGET_OFF_HEADING, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_TARGET_OFF_HEADING && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_TARGET_OFF_HEADING);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_AREA on the monster's area: each case is an area id (two bytes),
 * and the first with the same day id as area (Area_DayId: an area's day and
 * night forms match) runs its block.
 */
u8 *EmScript_SwitchArea(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u16 caseArea;
    u8 ok;
    u16 caseDayId;
    u16 areaDayId;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                caseArea = script[0] << 8;
                caseArea |= script[1];
                script += 2;
                caseDayId = Area_DayId(caseArea);
                areaDayId = Area_DayId(em->area);
                if (areaDayId == caseDayId) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_AREA, EMBLK_END_SWITCH);
                if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script += 2;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_AREA, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_AREA && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Command EMOP_CALL: calls subscript number byte of table 9 (EmScript_SubScript),
 * saving the return in subScriptReturn (EMEND_SUBSCRIPT returns there).
 * Returns the subscript's start, also stored in scriptPos.
 */
u8 *EmScript_Call(Em *em, u8 *script)
{
    em->subScriptNo = *script;
    em->subScriptReturn = script + 1;
    script = EmScript_SubScript(em, em->subScriptNo);
    em->scriptPos = script;
    return script;
}

/* Command EMOP_SET_AREA_SEQ (4 argument bytes), once while areaSeqLen is -1: two pairs of bytes into areaSeqTable/areaSeqLen and
 * areaSeqScript/areaSeqMode; a zero areaSeqLen becomes -1 again. The last step is two increments (`p += 2` is folded
 * into each return). */
u8 *EmScript_SetAreaSeq(Em *em, u8 *script)
{
    if (em->areaSeqLen != -1) {
        return script + 4;
    }
    em->areaSeqTable = script[0];
    em->areaSeqLen = script[1];
    em->areaSeqScript = *(script += 2);
    em->areaSeqMode = script[1];
    em->areaSeqPos = 0;
    script++;
    script++;
    if (em->areaSeqLen == 0) {
        em->areaSeqLen = -1;
        return script;
    }
    return script;
}

//---
u8 *EmScript_StepAreaSeq(Em *em, u8 *pos)
{
    u8 done = 0;
    u16 entry;

    if (em->areaSeqLen == -1) {
        return pos;
    }
    em->areaSeqActive = 1;
    em->prevArea = 65535;
    do {
        entry = ((u16 *)EmScript_AreaSeqList(em, em->areaSeqTable))[em->areaSeqPos];
        if (entry >= 0x8000) {
            *(u16 *) &em->destArea = Area_IdForCurrentTime(*(u16 *) EmScript_PickWeighted(em, em->script[7][entry & 0x7FFF]));
        } else {
            *(u16 *) &em->destArea = Area_IdForCurrentTime(entry);
        }
        if (*(u16 *) &em->destArea != em->area) {
            goto other;
        }
        em->areaSeqPos++;
        if (em->areaSeqLen <= em->areaSeqPos) {
            if (em->areaSeqMode == 0) {
                done = 1;
                em->areaSeqPos = -1;
                em->areaSeqLen = -1;
            } else {
                em->areaSeqPos = 0;
            }
        }
    } while (done != 1);
    return pos;
other:
    em->areaSeqNext = pos;
    pos = EmScript_AreaSeqScript(em, em->areaSeqScript);
    em->scriptPos = pos;
    return pos;
}

/*
 * Sets the goal to another area (goalKind 3): goalArg and goalNo are
 * destArea.
 */
u8 *EmScript_GoalDestArea(Em *em, u8 *script)
{
    u16 dest;

    em->goalKind = 3;
    dest = *(u16 *) &em->destArea;
    em->goalNo = (s16) dest;
    em->goalArg = (s16) dest;
    return script;
}

/* Script op EMOP_GOAL_AREA_TOWARD: walks the route toward area (the two bytes at p): up to 20 steps of
 * Em_PickRoutePoint's pick from the areas Em_NextAreas/Em_NextAreasFrom list, until an area passes
 * Em_CanEnterArea or the target is reached; the result goes to destArea, goalNo and goalArg.
 * `area = area | *p++` (not `|=`) gives the game's stack slot for area. */
u8 *EmScript_GoalAreaToward(Em *em, u8 *script)
{
    s16 *list;
    u16 tgt;
    s32 k;
    u16 from;
    s16 best;
    u16 bestd;
    u16 area;
    s32 i;
    s16 n;
    u16 skip;
    AreaRecord *areaRec;
    f32 pts[10][3];
    u16 ang[9];
    u16 diff[8];
    u16 ids[8];

    area = *script++ << 8;
    area = area | *script++;
    skip = em->prevArea;
    from = em->area;
    em->goalKind = 3;
    tgt = area;
    list = Em_NextAreas(em);
    if (list == NULL) {
        *(u16 *) &em->destArea = Area_IdForCurrentTime(area);
        goto end;
    }
    for (k = 0; k < 20; k++) {
        for (i = 0; i < 8; i++) {
            if (Area_IdForCurrentTime(tgt) == Area_IdForCurrentTime(list[i])) {
                *(u16 *) &em->destArea = Area_IdForCurrentTime(tgt);
                goto end;
            }
        }
        areaRec = Stage_GetAreaRecord(em->area);
        pts[8][0] = areaRec->originX + areaRec->sizeX / 2.0f;
        pts[8][2] = areaRec->originZ + (f32) (areaRec->sizeZ / 2.0f);
        areaRec = Stage_GetAreaRecord(tgt);
        pts[9][0] = areaRec->originX + areaRec->sizeX / 2.0f;
        pts[9][2] = areaRec->originZ + (f32) (areaRec->sizeZ / 2.0f);
        for (i = 0, n = 0; i < 8; i++) {
            if (list[i] == -1) {
                break;
            }
            areaRec = Stage_GetAreaRecord(list[i]);
            ids[i] = list[i];
            pts[i][0] = areaRec->originX + areaRec->sizeX / 2.0f;
            n++;
            pts[i][2] = areaRec->originZ + (f32) (areaRec->sizeZ / 2.0f);
        }
        if (n == 1) {
            area = Area_IdForCurrentTime(ids[0]);
        } else {
            best = -1;
            ang[8] = Math_AngleXZ(pts[8], pts[9]);
            for (i = 0; i < n; i++) {
                if (skip != 0xFFFF && skip == Area_IdForCurrentTime(list[i])) {
                    continue;
                }
                ang[i] = Math_AngleXZ(pts[8], pts[i]);
                diff[i] = ang[i] - ang[8];
                if (diff[i] > 0x8000) {
                    diff[i] = 0x10000 - diff[i];
                }
                if (best == -1) {
                    bestd = diff[i];
                    best = i;
                } else if (diff[i] < bestd) {
                    best = i;
                    bestd = diff[i];
                }
            }
            if (list[best] != -1) {
                area = list[best];
            }
        }
        if (Em_CanEnterArea(em, Area_IdForCurrentTime(area)) == 1) {
            *(u16 *) &em->destArea = Area_IdForCurrentTime(area);
            goto end;
        }
        skip = from;
        from = Area_IdForCurrentTime(area);
        list = Em_NextAreasFrom(em, area);
        if (list == NULL) {
            *(u16 *) &em->destArea = Area_IdForCurrentTime(area);
            goto end;
        }
    }
end:
    em->goalArg = em->goalNo = *(u16 *) &em->destArea;
    return script;
}

/*
 * Test op EMOP_IF_REQUESTED_IS: true when requested (1 while a request is pending) equals the
 * byte.
 */
u8 *EmScript_IfRequestedIs(Em *em, u8 *script)
{
    u8 wanted;

    switch (*script++) {
    case EMBLK_OPEN:
        wanted = *script++;
        if (wanted != em->requested) {
            for (;;) {
                if (script[0] == EMOP_IF_REQUESTED_IS && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_REQUESTED_IS && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_REQUESTED_IS, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_REQUESTED_IS && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_REQUESTED_IS);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_HP_PERCENT on the monster's hp percent (hp * 100 / hpMax): the first
 * case whose limit byte is above it runs its block.
 */
u8 *EmScript_SwitchHpPercent(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 limit;
    u8 ok;
    u8 x;

    switch (*script++) {
    case EMBLK_OPEN:
        x = em->hp * 100 / em->hpMax;
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                limit = *script++;
                if (limit > x) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_HP_PERCENT, EMBLK_END_SWITCH);
                if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_HP_PERCENT, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_HP_PERCENT && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_REQUEST_ID on requestId; the case values ascend, and a larger value
 * ends the search.
 */
u8 *EmScript_SwitchRequestId(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 caseId;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                caseId = *script++;
                if (em->requestId == caseId) {
                    break;
                }
                if (caseId < em->requestId) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_REQUEST_ID, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_REQUEST_ID, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_REQUEST_ID && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_REQUEST_ID && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_REQUEST_ID, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_REQUEST_ID && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Command EMOP_SET_REQUEST: cancels the pending request (requested, requestId,
 * requestPrio, lowHpNeed and the four needs and their states cleared).
 */
u8 *EmScript_SetRequest(Em *em, u8 *script)
{
    em->requested = 0;
    em->requestId = 0;
    em->requestPrio = 0;
    em->lowHpNeed = 0;
    em->need4 = 0;
    em->need1 = 0;
    em->need2 = 0;
    em->need3 = 0;
    em->need1State = 0;
    em->need2State = 0;
    em->need3State = 0;
    return script;
}

/*
 * Switch op EMOP_SWITCH_TARGET_ANGLE on the goal player's direction from the heading (0 straight
 * ahead, ANGLE_TURN a turn): the first case whose byte * 256 is not below it
 * runs its block; none for a goal that is not a player.
 */
u8 *EmScript_SwitchTargetAngle(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u16 limit;
    u8 ok;
    u16 k;
    u16 ang;
    f32 targetPos[4];
    f32 targetWorldPos[4];
    Player *pl;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        script += 3;
        if (em->goalKind == 1 && *(u16 *) &em->goalArg == 0 && (s16) (k = *(u16 *) &em->goalNo) != -1) {
            Em_GetTargetPos(em, k, targetPos);
            pl = &gPlayers[k];
            Area_ToWorld(pl->area, targetPos, targetWorldPos);
            ang = Math_AngleXZ(em->mapPos, targetWorldPos) - em->rotY;
        } else {
            ok = 1;
            do {
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TARGET_ANGLE, EMBLK_END_SWITCH);
                if ((script[0] == EMOP_SWITCH_TARGET_ANGLE && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_TARGET_ANGLE && script[1] == EMBLK_END_SWITCH)) {
                    ok = 0;
                }
                script = EmScript_NextCmd(em, script);
            } while (ok);
            break;
        }
        i = 0;
        if (i < n) {
            do {
                limit = *script++ << 8;
                if (limit >= ang) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TARGET_ANGLE, EMBLK_END_SWITCH);
                if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TARGET_ANGLE, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_TARGET_ANGLE && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_NEED2_LOW: true when need2Gauge is at or below 30 percent of need2Max.
 */
u8 *EmScript_IfNeed2Low(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->need2Gauge > (s32) (0.3f * em->need2Max)) {
            for (;;) {
                if (script[0] == EMOP_IF_NEED2_LOW && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_NEED2_LOW && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_NEED2_LOW, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_NEED2_LOW && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_NEED2_LOW);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_GOAL_NEAR_XZ: true when the goal is within 100 times the byte on the ground
 * plane (Vec_DistanceXZ).
 */
u8 *EmScript_IfGoalNearXZ(Em *em, u8 *script)
{
    f32 lim;

    switch (*script++) {
    case EMBLK_OPEN:
        lim = 100.0f * *script++;
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= lim)) {
            for (;;) {
                if (script[0] == EMOP_IF_GOAL_NEAR_XZ && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_GOAL_NEAR_XZ && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_NEAR_XZ, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_GOAL_NEAR_XZ && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_GOAL_NEAR_XZ);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_GOAL_NEAR: true when the goal is within 100 times the byte
 * (Vec_Distance).
 */
u8 *EmScript_IfGoalNear(Em *em, u8 *script)
{
    f32 lim;

    switch (*script++) {
    case EMBLK_OPEN:
        lim = 100.0f * *script++;
        if (!(Vec_Distance(&em->posX, em->goal) <= lim)) {
            for (;;) {
                if (script[0] == EMOP_IF_GOAL_NEAR && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_GOAL_NEAR && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_NEAR, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_GOAL_NEAR && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_GOAL_NEAR);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_KIND_IN_AREA on the monsters sharing the area: a case's kind byte (0x4A:
 * any large monster, else Em_KindMatches) runs its block when such an active
 * monster is in the same area (the monster itself included).
 */
u8 *EmScript_SwitchKindInArea(Em *em, u8 *script)
{
    u8 n;
    u8 kind;
    u8 found;
    s32 i;
    s32 j;
    u8 ok;
    Em *other;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                kind = *script++;
                for (j = 0, found = 0, other = gEms; j < 20; j++, other++) {
                    if (other->active != 0) {
                        if (kind == 74) {
                            if (other->large == 0) {
                                continue;
                            }
                        } else if (Em_KindMatches(other->type, kind) == 0) {
                            continue;
                        }
                        if ((u8) Obj_SameArea(em, (Player *) other) != 0) {
                            found = 1;
                            break;
                        }
                    }
                }
                if (found != 0) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_KIND_IN_AREA, EMBLK_END_SWITCH);
                if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_KIND_IN_AREA, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_KIND_IN_AREA && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Command EMOP_REPEAT, a counted loop: sub-byte 0 reads the count into repeatCount
 * and the block's start into repeatPos (0 skips the block); sub-byte 1 at the
 * block's end counts down and jumps back while above 0.
 */
u8 *EmScript_Repeat(Em *em, u8 *script)
{

    switch (*script++) {
    case EMBLK_OPEN:
        em->repeatCount = *script++;
        em->repeatPos = script;
        if (em->repeatCount <= 0) {
            for (;;) {
                if (script[0] == EMOP_REPEAT && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_REPEAT, EMBLK_END_REPEAT);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_REPEAT && script[1] == EMBLK_END_REPEAT) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_END_REPEAT:
        if (--em->repeatCount > 0) {
            script = em->repeatPos;
        }
        break;
    }
    return script;
}

u8 *EmScript_StopRepeat(Em *em, u8 *script)
{
    em->repeatCount = 0;
    return script;
}

u8 *EmScript_SetNoAlert(Em *em, u8 *script)
{
    em->noAlert = *script;
    return script + 1;
}

/*
 * Switch op EMOP_SWITCH_VARIANT on variant (the quest monster entry's); the case values
 * ascend.
 */
u8 *EmScript_SwitchVariant(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 caseVariant;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                caseVariant = *script++;
                if (em->variant == caseVariant) {
                    break;
                }
                if (caseVariant < em->variant) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_VARIANT, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_VARIANT, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_VARIANT && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_VARIANT && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_VARIANT, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_VARIANT && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_SPAWN_PARAM on spawnParam (the quest monster entry's spawn byte); the
 * case values ascend.
 */
u8 *EmScript_SwitchSpawnParam(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 caseParam;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                caseParam = *script++;
                if (em->spawnParam == caseParam) {
                    break;
                }
                if (caseParam < em->spawnParam) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SPAWN_PARAM, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SPAWN_PARAM, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_SPAWN_PARAM && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_SPAWN_PARAM && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SPAWN_PARAM, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_SPAWN_PARAM && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_SEASON on the quest's season (Quest_GetSeason); the case values
 * ascend.
 */
u8 *EmScript_SwitchSeason(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 caseSeason;
    u8 ok;
    u8 x;

    switch (*script++) {
    case EMBLK_OPEN:
        x = Quest_GetSeason();
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                caseSeason = *script++;
                if (x == caseSeason) {
                    break;
                }
                if (caseSeason < x) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SEASON, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SEASON, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_SEASON && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_SEASON && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SEASON, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_SEASON && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_BASE_AREA on the base area (Map_DayId) of gScene.map; each case's
 * area is reduced the same way, and the values ascend.
 */
u8 *EmScript_SwitchBaseArea(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 caseBase;
    u8 ok;
    u8 x;

    switch (*script++) {
    case EMBLK_OPEN:
        x = Map_DayId(gScene.map);
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                caseBase = Map_DayId(*script++);
                if (x == caseBase) {
                    break;
                }
                if (caseBase < x) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_BASE_AREA, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_BASE_AREA, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_BASE_AREA && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_BASE_AREA && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_BASE_AREA, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_BASE_AREA && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_KIND_QUERY on a value the kind reports: a query byte is passed to the
 * kind's scriptQuery handler (0 with none), and the case
 * equal to its answer runs its block.
 */
u8 *EmScript_SwitchKindQuery(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 caseValue;
    u8 ok;
    u8 x;
    s32 k;
    u8 (*fn)(Em *, u8);

    switch (*script++) {
    case EMBLK_OPEN:
        fn = em->handlers->scriptQuery;
        n = *script++;
        k = *script++;
        if (fn != NULL) {
            x = fn(em, k);
        } else {
            x = 0;
        }
        script += 2;
        i = 0;
        if (i < n) {
            do {
                caseValue = *script++;
                if (x == caseValue) {
                    break;
                }
                if (caseValue < x) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_KIND_QUERY, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_KIND_QUERY, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_KIND_QUERY && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_KIND_QUERY && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_KIND_QUERY, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_KIND_QUERY && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/* Test op EMOP_IF_PLAYER_IN_AREA: true when an active quest player is in the monster's area. */
u8 *EmScript_IfPlayerInArea(Em *em, u8 *script)
{
    Player *pl = gPlayers;
    s32 i;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        ok = 1;
        for (i = 0; i < gScene.playerCount; i++, pl++) {
            if (Obj_SameArea(em, pl) != 0 && pl->active != 0) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            while (ok) {
                if (script[0] == EMOP_IF_PLAYER_IN_AREA && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_PLAYER_IN_AREA && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_PLAYER_IN_AREA, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_PLAYER_IN_AREA && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_PLAYER_IN_AREA);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_AREA_TIMER1_OVER: true when areaTimer1 has run out. */
u8 *EmScript_IfAreaTimer1Over(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->areaTimer1 > 0) {
            for (;;) {
                if (script[0] == EMOP_IF_AREA_TIMER1_OVER && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_AREA_TIMER1_OVER && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_AREA_TIMER1_OVER, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_AREA_TIMER1_OVER && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_AREA_TIMER1_OVER);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_AREA_TIMER2_OVER: true when areaTimer2 has run out. */
u8 *EmScript_IfAreaTimer2Over(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->areaTimer2 > 0) {
            for (;;) {
                if (script[0] == EMOP_IF_AREA_TIMER2_OVER && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_AREA_TIMER2_OVER && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_AREA_TIMER2_OVER, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_AREA_TIMER2_OVER && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_AREA_TIMER2_OVER);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_VAR_IS with bytes k and v: true when the value EmScript_SetVar sets
 * under k (1 blind, 2 need2State, 3 senseDelay, 4 scriptVar, 5 to 9 the
 * kinds' work bytes) equals v.
 */
u8 *EmScript_IfVarIs(Em *em, u8 *script)
{
    u8 k;
    u8 value;
    u8 differs;

    switch (*script++) {
    case EMBLK_OPEN:
        k = script[0];
        value = script[1];
        script += 2;
        switch (k) {
        case 1:
            if (em->blind == value) {
                differs = 0;
            } else {
                differs = 1;
            }
            break;
        case 2:
            if (em->need2State == value) {
                differs = 0;
            } else {
                differs = 1;
            }
            break;
        case 3:
            if (em->senseDelay == value) {
                differs = 0;
            } else {
                differs = 1;
            }
            break;
        case 4:
            if (em->scriptVar == value) {
                differs = 0;
            } else {
                differs = 1;
            }
            break;
        case 5:
            if (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG) {
                if (em->work == value) {
                    differs = 0;
                } else {
                    differs = 1;
                }
            }
            break;
        case 6:
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                if ((&em->work)[68] == value) {
                    differs = 0;
                } else {
                    differs = 1;
                }
            }
            break;
        case 7:
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                if ((&em->work)[69] == value) {
                    differs = 0;
                } else {
                    differs = 1;
                }
            }
            break;
        case 8:
            if (em->type == EM_KIND_SHEN_GAOREN) {
                if (em->work == value) {
                    differs = 0;
                } else {
                    differs = 1;
                }
            }
            break;
        case 9:
            if (em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
                if (em->work == value) {
                    differs = 0;
                } else {
                    differs = 1;
                }
            }
            break;
        }
        /* the game reads a register it never set: ne is s0, still the caller's, when the em->type tests fail or k has no case */
        if (!differs) {
            break;
        }
        while (differs) {
            if (script[0] == EMOP_IF_VAR_IS && script[1] == EMBLK_ELSE) {
                break;
            }
            if (script[0] == EMOP_IF_VAR_IS && script[1] == EMBLK_END_IF) {
                break;
            }
            script = EmScript_SkipBlock(em, script, EMOP_IF_VAR_IS, EMBLK_END_IF);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == EMOP_IF_VAR_IS && script[1] == EMBLK_END_IF) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_VAR_IS);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_KIND_GROUP on the monster's kind: the first case whose kind is of the
 * same group as type (Em_KindMatches) runs its block.
 */
u8 *EmScript_SwitchKindGroup(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                if (Em_KindMatches(em->type, *script++) != 0) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_KIND_GROUP, EMBLK_END_SWITCH);
                if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_KIND_GROUP, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_KIND_GROUP && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/* Switch op EMOP_SWITCH_TYPE on type, exactly; the case values ascend. */
u8 *EmScript_SwitchType(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 caseType;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                caseType = *script++;
                if (em->type == caseType) {
                    break;
                }
                if (caseType < em->type) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TYPE, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TYPE, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_TYPE && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_TYPE && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TYPE, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_TYPE && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Sets the goal to the action spot found before (goalKind 7, goalArg and
 * goalNo from actionGoal).
 */
u8 *EmScript_GoalActionGoal(Em *em, u8 *script)
{
    em->goalKind = 7;
    em->goalArg = (s16) em->actionGoal[1];
    em->goalNo = (s16) em->actionGoal[2];
    return script;
}

u8 *EmScript_SelectSightRow(Em *em, u8 *script)
{
    Em_SelectSightRow(em, *script);
    return script + 1;
}

/*
 * Test op EMOP_IF_PLAYER_CARRYING: true when any quest player carries an item (the low nibble of
 * carry).
 */
u8 *EmScript_IfPlayerCarrying(Em *em, u8 *script)
{
    s8 i;
    u8 found;

    switch (*script++) {
    case EMBLK_OPEN:
        found = 0;
        for (i = 0; i < gScene.playerCount; i++) {
            if (gPlayers[i].carry & PL_CARRY_HOLD_MASK) {
                found = 1;
                break;
            }
        }
        if (!found) {
            for (;;) {
                if (script[0] == EMOP_IF_PLAYER_CARRYING && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_PLAYER_CARRYING && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_PLAYER_CARRYING, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_PLAYER_CARRYING && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_PLAYER_CARRYING);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Command EMOP_SET_NEED4: sets need4 (Em_PickRequest turns it into the kind's need-4
 * request) when any player carries an item (the low nibble of carry).
 */
u8 *EmScript_SetNeed4(Em *em, u8 *script)
{
    s8 i;
    s32 found = 0;

    for (i = 0; i < gScene.playerCount; i++) {
        if (gPlayers[i].carry & PL_CARRY_HOLD_MASK) {
            found = 1;
            break;
        }
    }
    if (found) {
        em->need4 = 1;
    }
    return script;
}

u8 *EmScript_SetGoalKind(Em *em, u8 *script)
{
    em->goalKind = 8;
    return script;
}

/*
 * Test op EMOP_IF_HIT_REACT_MODE: true when hitReactMode equals the byte (enum EmHitReactMode).
 */
u8 *EmScript_IfHitReactMode(Em *em, u8 *script)
{
    u8 mode;

    switch (*script++) {
    case EMBLK_OPEN:
        mode = *script++;
        if (em->hitReactMode != mode) {
            for (;;) {
                if (script[0] == EMOP_IF_HIT_REACT_MODE && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_HIT_REACT_MODE && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_HIT_REACT_MODE, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_HIT_REACT_MODE && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_HIT_REACT_MODE);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Switch op EMOP_SWITCH_HIT_REACT_MODE on hitReactMode: n values, each with its block. */
u8 *EmScript_SwitchHitReactMode(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 mode;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                mode = *script++;
                if (em->hitReactMode == mode) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_HIT_REACT_MODE, EMBLK_END_SWITCH);
                if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_HIT_REACT_MODE, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_HIT_REACT_MODE && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/* Test op EMOP_IF_STATE: true when stateGroup and state equal the two bytes. */
u8 *EmScript_IfState(Em *em, u8 *script)
{
    u8 wantGroup;
    u8 wantState;

    switch (*script++) {
    case EMBLK_OPEN:
        wantGroup = script[0];
        wantState = script[1];
        script += 2;
        if (em->stateGroup != wantGroup || em->state != wantState) {
            for (;;) {
                if (script[0] == EMOP_IF_STATE && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_STATE && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_STATE, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_STATE && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_STATE);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_RAGING: true while the monster is enraged (rage). */
u8 *EmScript_IfRaging(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->rage == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_RAGING && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_RAGING && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_RAGING, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_RAGING && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_RAGING);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_DRILL: true in a drill quest (gQuest.flags has QUEST_FLAG_DRILL). */
u8 *EmScript_IfDrill(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (!(gQuest.flags & 0x200000)) {
            for (;;) {
                if (script[0] == EMOP_IF_DRILL && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_DRILL && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_DRILL, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_DRILL && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_DRILL);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_ON_PASS_THROUGH_FLOOR: true when the kind of the floor under the monster (floorKind,
 * in its area) has flag byte 4 set in gAreaKindFlags (sub_main's
 * Obj_OnPassThroughFloor). Em_GetFloorInfo does not stand monsters on such triangles;
 * AreaGrid2_GetFloorHeight gives their height to effects.
 */
u8 *EmScript_IfOnPassThroughFloor(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (!Obj_OnPassThroughFloor(em)) {
            for (;;) {
                if (script[0] == EMOP_IF_ON_PASS_THROUGH_FLOOR && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_ON_PASS_THROUGH_FLOOR && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_ON_PASS_THROUGH_FLOOR, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_ON_PASS_THROUGH_FLOOR && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_ON_PASS_THROUGH_FLOOR);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_BLIND: true while the monster is blinded by a flash (blindTimer). */
u8 *EmScript_IfBlind(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->blindTimer == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_BLIND && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_BLIND && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_BLIND, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_BLIND && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_BLIND);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_NO_SENSE_DELAY: true when senseDelay is 0 (the monster senses the players).
 */
u8 *EmScript_IfNoSenseDelay(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->senseDelay != 0) {
            for (;;) {
                if (script[0] == EMOP_IF_NO_SENSE_DELAY && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_NO_SENSE_DELAY && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_NO_SENSE_DELAY, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_NO_SENSE_DELAY && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_NO_SENSE_DELAY);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_LINKED: true when the monster has a linked monster (link;
 * Em_FindLink).
 */
u8 *EmScript_IfLinked(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->link == NULL) {
            for (;;) {
                if (script[0] == EMOP_IF_LINKED && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_LINKED && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_LINKED, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_LINKED && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_LINKED);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_LINK_COMBAT_SAME_AREA: true when the linked monster exists, fights (combat 1) and is
 * in the same area.
 */
u8 *EmScript_IfLinkCombatSameArea(Em *em, u8 *script)
{
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        ok = 1;
        if (em->link == NULL) {
            ok = 1;
        } else {
            if (em->link->combat == 1 && em->link->area == em->area) {
                break;
            }
            ok = 1;
        }
        {
            while (ok) {
                if (script[0] == EMOP_IF_LINK_COMBAT_SAME_AREA && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_LINK_COMBAT_SAME_AREA && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_LINK_COMBAT_SAME_AREA, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_LINK_COMBAT_SAME_AREA && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_LINK_COMBAT_SAME_AREA);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_PREV_AREA: true when prevArea, the area the monster came from, is the
 * area of the two bytes (Area_IdForCurrentTime); 0xFFFF never matches.
 */
u8 *EmScript_IfPrevArea(Em *em, u8 *script)
{
    u8 ok;
    u16 id;

    switch (*script++) {
    case EMBLK_OPEN:
        id = script[0] << 8;
        id |= script[1];
        script += 2;
        if (id == 0xFFFF) {
            ok = 1;
        } else {
            if (em->prevArea == Area_IdForCurrentTime(id)) {
                break;
            }
            ok = 1;
        }
        while (ok) {
            if (script[0] == EMOP_IF_PREV_AREA && script[1] == EMBLK_ELSE) {
                break;
            }
            if (script[0] == EMOP_IF_PREV_AREA && script[1] == EMBLK_END_IF) {
                break;
            }
            script = EmScript_SkipBlock(em, script, EMOP_IF_PREV_AREA, EMBLK_END_IF);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == EMOP_IF_PREV_AREA && script[1] == EMBLK_END_IF) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_PREV_AREA);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_PREV_AREA on prevArea: n area ids (two bytes each,
 * Area_IdForCurrentTime), each with its block; a prevArea of 0xFFFF (none)
 * matches nothing.
 */
u8 *EmScript_SwitchPrevArea(Em *em, u8 *script)
{
    u16 x = em->prevArea;
    u8 n;
    s32 i;
    u16 areaId;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                areaId = script[0] << 8;
                areaId |= script[1];
                script += 2;
                if (x == Area_IdForCurrentTime(areaId) && x != 0xFFFF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_SWITCH_PREV_AREA, EMBLK_END_SWITCH);
                if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script += 2;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_PREV_AREA, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_PREV_AREA && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/* Script command EMOP_START_ROUTE: move to area p[0..1] (big-endian) by a route. Picks the route to that
 * area (the one starting nearest when the area has no id at this time), then the point to start
 * at: outside the scene's area the nearest flagged one, inside it the first flagged one in line
 * of sight (AreaHit_Raycast) at height p[2] * 10. */
u8 *EmScript_StartRoute(Em *em, u8 *script)
{
    EmRoute *route;
    s16 j;
    s32 found;
    s16 idx;
    u16 area;
    s16 sel;
    EmRouteSet *set;
    s32 nearPoint;
    EmRoutePt *point;
    f32 height;
    f32 best;
    f32 dist;
    f32 rayPts[6];
    f32 out[3];

    found = 0;
    em->routeMissCount = 0;
    area = script[0] << 8;
    area |= script[1];
    em->routeHeight = *(script += 2);
    height = 10.0f * em->routeHeight;
    if (em->area == Area_IdForCurrentTime(area)) {
        return script + 1;
    }
    set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
    route = set->routes;
    if (route == NULL) {
        return script + 1;
    }
    em->nextArea = Area_IdForCurrentTime(area);
    if (em->nextArea == 0xFFFF) {
        best = -1.0f;
        for (j = 0; j < set->n; j++, route++) {
            point = route->pts;
            if (point == NULL) {
                j = 0;
                em->routeResult = 7;
                goto end;
            }
            dist = Vec_DistanceXZ(&em->posX, point->pos);
            if (best == -1.0f) {
                sel = j;
                best = dist;
            } else if (!(best <= dist)) {
                sel = j;
                best = dist;
            }
        }
        idx = sel;
    } else {
        for (idx = 0; idx < set->n; idx++, route++) {
            if (em->nextArea == Area_IdForCurrentTime(route->area)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return script + 1;
        }
    }
    em->routeNo = idx;
    route = set->routes;
    point = route[em->routeNo].pts;
    found = 0;
    if (gScene.area != em->area) {
        nearPoint = route[em->routeNo].n;
        best = -100.0f;
        for (j = 0; j < route[em->routeNo].n; j++, point++) {
            if (point->canStart != 0) {
                dist = Vec_DistanceXZ(&em->posX, point->pos);
                if (best < 0.0f) {
                    nearPoint = j;
                    best = dist;
                } else if (dist < best) {
                    nearPoint = j;
                    best = dist;
                }
            }
        }
        point = route[em->routeNo].pts;
        point += nearPoint;
        j = nearPoint;
        if (nearPoint < route[em->routeNo].n) {
            found = 1;
        }
    } else {
        for (j = 0; j < route[em->routeNo].n; j++, point++) {
            if (point->canStart != 0) {
                Vec_Set(rayPts, em->posX, em->posY + height, em->posZ);
                Vec_Set(rayPts + 3, point->pos[0], point->pos[1] + height, point->pos[2]);
                if (!AreaHit_Raycast(rayPts, rayPts + 3, out, em->collMask)) {
                    found = 1;
                    break;
                }
            }
        }
    }
    if (!found) {
        point--;
        em->routeResult = 2;
        j = route[em->routeNo].n - 1;
    }
end:
    if (em->routeResult != 0 && em->remote != 0) {
        em->routeResult = 0;
        j = 0;
        em->routeNo = 0;
        em->routePoint = 0;
        point = set->routes[em->routeNo].pts;
    }
    if (em->routeResult != 0) {
        em->routeResult = 0;
    }
    em->routePoint = j;
    em->goalKind = 9;
    em->goalArg = em->routeNo;
    em->goalNo = em->routePoint;
    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
    em->routeResume = script + 1;
    em->routePointScript = point->script;
    return EmScript_RoutePointScript(em, em->routePointScript);
}

/*
 * Command EMOP_SET_COMBAT, one byte: 0 takes the monster out of combat (Em_SetCombat
 * mode 0), else puts it in combat for the kind's time (gEmCombatInit[type]).
 */
u8 *EmScript_SetCombat(Em *em, u8 *script)
{
    if (*script == 0) {
        Em_SetCombat(em, 0, 0);
    } else {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
    return script + 1;
}

/*
 * Command EMOP_ADD_HP, one byte: with 0 heals the drome leaders by a share of hpMax
 * (Em_AddHp: 15 percent for EM_KIND_VELOCIDROME, 10 for EM_KIND_GENDROME and
 * EM_KIND_IODROME); otherwise nothing.
 */
u8 *EmScript_AddHp(Em *em, u8 *script)
{
    u8 mode;
    u8 type;

    type = em->type;
    mode = *script;
    switch (type) {
    case 27:
        if (!(mode & 0xFF)) {
            Em_AddHp((Em *) em, (s32) (0.15f * (f32) em->hpMax));
        }
        break;
    case 28:
        if (!(mode & 0xFF)) {
            Em_AddHp((Em *) em, (s32) (0.1f * (f32) em->hpMax));
        }
        break;
    case 31:
        if (!(mode & 0xFF)) {
            Em_AddHp((Em *) em, (s32) (0.1f * (f32) em->hpMax));
        }
        break;
    }
    return script + 1;
}

/*
 * Test op EMOP_IF_GOAL_ANGLE_OVER with k in degrees: recomputes the goal (Em_ComputeGoal) and is
 * true when its direction is k degrees or more off the heading rotY.
 */
u8 *EmScript_IfGoalAngleOver(Em *em, u8 *script)
{
    s32 ok;
    u8 k;
    u16 limit;
    u16 angleDelta;

    switch (*script++) {
    case EMBLK_OPEN:
        Em_ComputeGoal(em, em->goal);
        k = *script++;
        limit = (s32) (65536.0f * k / 360.0f + 0.5f);
        ok = 0;
        angleDelta = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (angleDelta > (ANGLE_TURN / 2)) {
            angleDelta = ANGLE_TURN - angleDelta;
        }
        if (limit > angleDelta) {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == EMOP_IF_GOAL_ANGLE_OVER && script[1] == EMBLK_ELSE) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == EMOP_IF_GOAL_ANGLE_OVER && script[1] == EMBLK_END_IF) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_ANGLE_OVER, EMBLK_END_IF);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == EMOP_IF_GOAL_ANGLE_OVER && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_GOAL_ANGLE_OVER && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_ANGLE_OVER, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_GOAL_ANGLE_OVER && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_GOAL_ANGLE_OVER);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_GOAL_ANGLE_OVER_CAPPED, one byte in degrees: like EmScript_IfGoalAngleOver, true when
 * the goal's direction (Em_ComputeGoal) is at least that many degrees off the
 * heading rotY. A limit of 90 degrees or more is always true.
 */
u8 *EmScript_IfGoalAngleOverCapped(Em *em, u8 *script)
{
    s32 ok;
    u8 limitDegrees;
    u16 limit;
    u16 angleDelta;

    switch (*script++) {
    case EMBLK_OPEN:
        Em_ComputeGoal(em, em->goal);
        limitDegrees = *script++;
        limit = (s32) (65536.0f * limitDegrees / 360.0f + 0.5f);
        ok = 0;
        angleDelta = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (angleDelta > (ANGLE_TURN / 2)) {
            angleDelta = ANGLE_TURN - angleDelta;
        }
        if (limit > angleDelta && limit < 0x4000) {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == EMOP_IF_GOAL_ANGLE_OVER_CAPPED && script[1] == EMBLK_ELSE) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == EMOP_IF_GOAL_ANGLE_OVER_CAPPED && script[1] == EMBLK_END_IF) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_ANGLE_OVER_CAPPED, EMBLK_END_IF);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == EMOP_IF_GOAL_ANGLE_OVER_CAPPED && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_GOAL_ANGLE_OVER_CAPPED && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_ANGLE_OVER_CAPPED, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_GOAL_ANGLE_OVER_CAPPED && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_GOAL_ANGLE_OVER_CAPPED);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_LINK_SAME_AREA: true when the linked monster exists and is in the same area.
 */
u8 *EmScript_IfLinkSameArea(Em *em, u8 *script)
{
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        if (em->link == NULL) {
            ok = 1;
        } else {
            if (em->area == em->link->area) {
                break;
            }
            ok = 1;
        }
        while (ok) {
            if (script[0] == EMOP_IF_LINK_SAME_AREA && script[1] == EMBLK_ELSE) {
                break;
            }
            if (script[0] == EMOP_IF_LINK_SAME_AREA && script[1] == EMBLK_END_IF) {
                break;
            }
            script = EmScript_SkipBlock(em, script, EMOP_IF_LINK_SAME_AREA, EMBLK_END_IF);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == EMOP_IF_LINK_SAME_AREA && script[1] == EMBLK_END_IF) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_LINK_SAME_AREA);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Command EMOP_IF_LONE_PLAYER_BAIT (if/else/end): true when exactly one player is in the area, is fishing (PLF_FISHING) and
 * Lure_PlayerHasMonsterBait passes; the monster then targets it (scriptPlayer). */
u8 *EmScript_IfLonePlayerBait(Em *em, u8 *script)
{
    Player *pl = gPlayers;
    u8 list[4];
    u8 ok;
    s8 i;
    s8 n;
    s8 cnt;
    u8 *w;

    switch (*script++) {
    case EMBLK_OPEN:
        for (i = 0, n = 0, cnt = 0, ok = 0, w = list; i < 4; i++, pl++) {
            if (pl->active != 0 && Obj_SameArea(em, pl) != 0) {
                cnt++;
                if (cnt > 1) {
                    ok = 0;
                    break;
                }
                if (Player_TestFlag(pl, PLF_FISHING) != 0 && Lure_PlayerHasMonsterBait(pl) == 1) {
                    *w++ = i;
                    n++;
                    ok = 1;
                }
            }
        }
        if (ok) {
            em->scriptPlayer = list[em->stateRand % n];
        } else {
            for (;;) {
                if (script[0] == EMOP_IF_LONE_PLAYER_BAIT && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_LONE_PLAYER_BAIT && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_LONE_PLAYER_BAIT, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_LONE_PLAYER_BAIT && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_LONE_PLAYER_BAIT);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_SCRIPT_PLAYER_STATE: true when the picked player (scriptPlayer) is in the state
 * group and state of the two bytes (Player_IsState); no check for "none
 * picked".
 */
u8 *EmScript_IfScriptPlayerState(Em *em, u8 *script)
{
    u8 group;
    u8 state;

    switch (*script++) {
    case EMBLK_OPEN:
        group = script[0];
        state = script[1];
        script += 2;
        if (Player_IsState(&gPlayers[(s8) em->scriptPlayer & 0xF], group, state) == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_STATE && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_STATE && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_SCRIPT_PLAYER_STATE, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_SCRIPT_PLAYER_STATE && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_SCRIPT_PLAYER_STATE);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_HOOKED: true when the monster took a player's bait (hooked; the
 * Plesioth), and clears it.
 */
u8 *EmScript_IfHooked(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->hooked != 0) {
            em->hooked = 0;
        } else {
            for (;;) {
                if (script[0] == EMOP_IF_HOOKED && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_HOOKED && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_HOOKED, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_HOOKED && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_HOOKED);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

u8 *EmScript_SetDelay(Em *em, u8 *script)
{
    em->scriptDelay = *script;
    return script + 1;
}

/*
 * Command EMOP_GOAL_PLAYER_FLOOR_SPOT, one byte: with a picked player, the goal is goalKind 11, a
 * point by that player's floorAttr (Em_ComputeGoal) with preset byte; with
 * none, goalKind 1 with no player.
 */
u8 *EmScript_GoalPlayerFloorSpot(Em *em, u8 *script)
{
    s32 preset = *script;

    if ((s8) em->scriptPlayer == -1) {
        em->goalKind = 1;
        em->goalArg = 0;
        *(u16 *) &em->goalNo = 65535;
    } else {
        em->goalKind = 11;
        em->goalArg = (s8) em->scriptPlayer & 0xF;
        em->goalNo = (u8) preset;
    }
    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
    return script + 1;
}

/*
 * Test op EMOP_IF_SCRIPT_PLAYER_SEEN: true when a player is picked and in the monster's sight (its
 * bit in seen).
 */
u8 *EmScript_IfScriptPlayerSeen(Em *em, u8 *script)
{
    s8 k;

    switch (*script++) {
    case EMBLK_OPEN:
        k = em->scriptPlayer;
        if (!(em->seen & (1 << (k & 0xF))) || k == -1) {
            for (;;) {
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_SEEN && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_SEEN && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_SCRIPT_PLAYER_SEEN, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_SCRIPT_PLAYER_SEEN && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_SCRIPT_PLAYER_SEEN);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Command EMOP_SET_ALERT_BUILDUP: resets the picked player's alertBuildup to 0 (nothing with
 * none picked).
 */
u8 *EmScript_SetAlertBuildup(Em *em, u8 *script)
{
    s8 idx;

    idx = (s8) em->scriptPlayer;
    if (idx != -1) {
        em->alertBuildup[idx & 0xF] = 0;
    }
    return script;
}

/*
 * Command EMOP_SET_NOTICE_COUNT: resets the picked player's noticeCount to 0 (nothing with
 * none picked).
 */
u8 *EmScript_SetNoticeCount(Em *em, u8 *script)
{
    s8 idx;

    idx = (s8) em->scriptPlayer;
    if (idx != -1) {
        em->noticeCount[idx & 0xF] = 0;
    }
    return script;
}

u8 *EmScript_ComputeGoal(Em *em, u8 *script)
{
    Em_ComputeGoal(em, em->goal);
    return script;
}

/*
 * Command EMOP_ADD_NEED2, one byte: with 0 adds half of need2Max to need 2's gauge
 * (Em_AddNeed2).
 */
u8 *EmScript_AddNeed2(Em *em, u8 *script)
{
    switch (*script) {
    case 0:
        Em_AddNeed2(em, em->need2Max / 2);
        break;
    }
    return script + 1;
}

/*
 * Command EMOP_ADD_NEED1, one byte: with 0 adds half of need1Max to need 1's gauge
 * (Em_AddNeed1).
 */
u8 *EmScript_AddNeed1(Em *em, u8 *script)
{
    switch (*script) {
    case 0:
        Em_AddNeed1(em, em->need1Max / 2);
        break;
    }
    return script + 1;
}

/* Command EMOP_ADD_NEED3, one byte: with 0 adds half of need3Max to need 3's gauge. */
u8 *EmScript_AddNeed3(Em *em, u8 *script)
{
    switch (*script) {
    case 0:
        Em_AddNeed3(em, em->need3Max / 2);
        break;
    }
    return script + 1;
}

/*
 * Test op EMOP_IF_STANCE_UNDER: true while the monster is under the sand or the water
 * (EM_STANCE_UNDER).
 */
u8 *EmScript_IfStanceUnder(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (em->stance != 4) {
            for (;;) {
                if (script[0] == EMOP_IF_STANCE_UNDER && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_STANCE_UNDER && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_STANCE_UNDER, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_STANCE_UNDER && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_STANCE_UNDER);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Script command EMOP_PICK_NOTICED_PLAYER_REACHABLE: picks the target player into scriptPlayer among those it has noticed (noticed)
 * and can reach: noticeCount 50000 or more first, then 30000, by the largest playerDist; else the
 * angriest (none if all are 0); -1 when none. */
u8 *EmScript_PickNoticedPlayerReachable(Em *em, u8 *script)
{
    s32 i;
    s32 n;
    u8 sel;
    u8 zero;
    u8 list[4];

    if (em->noticed != 0) {

        for (i = 0, n = 0; i < gScene.playerCount; i++) {
            if (em->noticeCount[i] >= 50000 && (em->noticed & (1 << i))
                && Side_Match(em->side, gPlayers[i].side, 1) != 1
                && Em_CanEnterArea(em, gPlayers[i].area) != 0) {
                list[n++] = i;
            }
        }
        if (n >= 2) {
            sel = list[0];
            for (i = 1; i < n; i++) {
                if (em->playerDist[sel] < em->playerDist[list[i]]) {
                    sel = list[i];
                }
            }
        } else if (n == 1) {
            sel = list[0];
        } else {

            for (i = 0, n = 0; i < gScene.playerCount; i++) {
                if (em->noticeCount[i] >= 30000 && (em->noticed & (1 << i))
                    && Side_Match(em->side, gPlayers[i].side, 1) != 1
                    && Em_CanEnterArea(em, gPlayers[i].area) != 0) {
                    list[n++] = i;
                }
            }
            sel = 255;
            if (n >= 2) {
                sel = list[0];
                for (i = 1; i < n; i++) {
                    if (em->playerDist[sel] < em->playerDist[list[i]]) {
                        sel = list[i];
                    }
                }
            } else if (n == 1) {
                sel = list[0];
            } else {

                for (i = 0, n = 0; i < gScene.playerCount; i++) {
                    if ((em->noticed & (1 << i))
                        && Side_Match(em->side, gPlayers[i].side, 1) != 1
                        && Em_CanEnterArea(em, gPlayers[i].area) != 0) {
                        list[n++] = i;
                    }
                }
                if (n == 1) {
                    sel = list[0];
                } else if (n != 0) {
                    sel = list[0];
                    zero = 0;
                    if (em->noticeCount[sel] == 0) {
                        zero = 1;
                    }
                    for (i = 1; i < n; i++) {
                        if (em->noticeCount[sel] < em->noticeCount[list[i]]) {
                            sel = list[i];
                            zero = 0;
                        }
                    }
                    if (zero == 1) {
                        sel = 255;
                    }
                }
            }
        }
        if (sel == 255) {
            *(s8 *) &em->scriptPlayer = -1;
        }
        em->scriptPlayer = sel;
    } else {
        *(s8 *) &em->scriptPlayer = -1;
    }
    return script;
}

/*
 * Command EMOP_PICK_NOTICED_PLAYER_IN_AREA: picks the target into scriptPlayer (-1 for none) among the
 * noticed, active players in the monster's area not on its side (Side_Match):
 * a notice count of 50000 or more first, then 30000 or more (the farthest by
 * playerDist), else the highest count, or at random when all are 0.
 */
u8 *EmScript_PickNoticedPlayerInArea(Em *em, u8 *script)
{
    s32 i;
    s32 n;
    u8 mask;
    u8 sel;
    u8 zero;
    u8 list[4];

    if (em->noticed != 0) {
        mask = 0;
        for (i = 0; i < gScene.playerCount; i++) {
            if ((em->noticed & (1 << i)) && Obj_SameArea(em, &gPlayers[i]) != 0 && gPlayers[i].active != 0
                && Side_Match(em->side, gPlayers[i].side, 1) != 1) {
                mask |= (u8) (1 << i);
            }
        }
        if (mask == 0) {
            *(s8 *) &em->scriptPlayer = -1;
            return script;
        }
        for (i = 0, n = 0; i < gScene.playerCount; i++) {
            if (em->noticeCount[i] >= 50000 && (mask & (1 << i))) {
                list[n++] = i;
            }
        }
        if (n >= 2) {
            sel = list[0];
            for (i = 1; i < n; i++) {
                if (em->playerDist[sel] < em->playerDist[list[i]]) {
                    sel = list[i];
                }
            }
        } else if (n == 1) {
            sel = list[0];
        } else {
            for (i = 0, n = 0; i < gScene.playerCount; i++) {
                if (em->noticeCount[i] >= 30000 && (mask & (1 << i))) {
                    list[n++] = i;
                }
            }
            sel = 255;
            if (n >= 2) {
                sel = list[0];
                for (i = 1; i < n; i++) {
                    if (em->playerDist[sel] < em->playerDist[list[i]]) {
                        sel = list[i];
                    }
                }
            } else if (n == 1) {
                sel = list[0];
            } else {
                for (i = 0, n = 0; i < gScene.playerCount; i++) {
                    if (mask & (1 << i)) {
                        list[n++] = i;
                    }
                }
                if (n == 1) {
                    sel = list[0];
                } else if (n != 0) {
                    sel = list[0];
                    zero = 0;
                    if (em->noticeCount[sel] == 0) {
                        zero = 1;
                    }
                    for (i = 1; i < n; i++) {
                        if (em->noticeCount[sel] < em->noticeCount[list[i]]) {
                            sel = list[i];
                            zero = 0;
                        }
                    }
                    if (zero == 1) {
                        for (i = 0, n = 0; i < gScene.playerCount; i++) {
                            if (mask & (1 << i)) {
                                list[n++] = i;
                            }
                        }
                        if (n != 0) {
                            sel = list[em->stateRand % n];
                        } else {
                            sel = 255;
                        }
                    }
                }
            }
        }
        em->scriptPlayer = sel;
    } else {
        *(s8 *) &em->scriptPlayer = -1;
    }
    return script;
}

/*
 * Test op EMOP_IF_SCRIPT_PLAYER_IN_AREA: true when a player is picked, active and in the monster's
 * area.
 */
u8 *EmScript_IfScriptPlayerInArea(Em *em, u8 *script)
{
    s8 k;
    Player *pl;

    switch (*script++) {
    case EMBLK_OPEN:
        k = em->scriptPlayer;
        if (k == -1 || Obj_SameArea(em, pl = &gPlayers[k]) == 0 || pl->active == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_IN_AREA && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_IN_AREA && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_SCRIPT_PLAYER_IN_AREA, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_SCRIPT_PLAYER_IN_AREA && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_SCRIPT_PLAYER_IN_AREA);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH: true when the picked player's noticeCount is 30000 or more.
 */
u8 *EmScript_IfScriptPlayerNoticeHigh(Em *em, u8 *script)
{
    s8 k;

    switch (*script++) {
    case EMBLK_OPEN:
        k = em->scriptPlayer;
        if (k == -1 || em->noticeCount[k] < 30000) {
            for (;;) {
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_QUEST_ID: true when gQuest.stage, the quest id, equals the two bytes.
 */
u8 *EmScript_IfQuestId(Em *em, u8 *script)
{
    u16 questId;

    switch (*script++) {
    case EMBLK_OPEN:
        questId = script[0] << 8;
        questId |= script[1];
        script += 2;
        if (gQuest.stage != questId) {
            for (;;) {
                if (script[0] == EMOP_IF_QUEST_ID && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_QUEST_ID && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_QUEST_ID, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_QUEST_ID && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_QUEST_ID);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_QUEST_KEY on questKey (the key of the quest's monster entry): n two-
 * byte values, ascending, each with its block.
 */
u8 *EmScript_SwitchQuestKey(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u16 listedKey;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                listedKey = script[0] << 8;
                listedKey |= script[1];
                script += 2;
                if (em->questKey == listedKey) {
                    break;
                }
                if (listedKey < em->questKey) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_QUEST_KEY, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_QUEST_KEY, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_QUEST_KEY && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_QUEST_KEY && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script += 2;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_QUEST_KEY, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_QUEST_KEY && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Command EMOP_GOAL_LINK_TARGET: the linked monster's target player (link->targetNo) becomes
 * the goal (goalKind 1; 65535 with no link) and scriptPlayer.
 */
u8 *EmScript_GoalLinkTarget(Em *em, u8 *script)
{
    Em *link = em->link;

    em->goalKind = 1;
    em->goalArg = 0;
    if (link == NULL) {
        *(u16 *) &em->goalNo = 65535;
    } else {
        em->goalNo = link->targetNo;
    }
    em->scriptPlayer = *(u16 *) &em->goalNo;
    return script;
}

/* Test op EMOP_IF_ON_CEILING: true while the monster is on the ceiling (ceilingTimer). */
u8 *EmScript_IfOnCeiling(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if ((s8) em->ceilingTimer == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_ON_CEILING && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_ON_CEILING && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_ON_CEILING, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_ON_CEILING && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_ON_CEILING);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR, one byte: true when the current goal is a player (curGoalKind
 * 1 or 11), a player is picked and its floorAttr equals the byte.
 */
u8 *EmScript_IfScriptPlayerFloorAttr(Em *em, u8 *script)
{
    u8 attr;
    s8 k;

    switch (*script++) {
    case EMBLK_OPEN:
        attr = *script++;
        if (em->curGoalKind == 11 || em->curGoalKind == 1) {
            k = em->scriptPlayer;
            if (k != -1 && attr == ((Player *) ((u8 *) gPlayers + (k & 0xF) * sizeof(Player)))->floorAttr) {
                break;
            }
        }
        for (;;) {
            if (script[0] == EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR && script[1] == EMBLK_ELSE) {
                break;
            }
            if (script[0] == EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR && script[1] == EMBLK_END_IF) {
                break;
            }
            script = EmScript_SkipBlock(em, script, EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR, EMBLK_END_IF);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR && script[1] == EMBLK_END_IF) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Command EMOP_SET_NOTICE_TIMER, one byte: with 0, zeroes noticeTimer of every player out of
 * the monster's sight this frame.
 */
u8 *EmScript_SetNoticeTimer(Em *em, u8 *script)
{
    u8 mode = *script;
    s8 i;

    for (i = 0; i < gScene.playerCount; i++) {
        switch (mode) {
        case 0:
            if (!(em->seen & (1 << i))) {
                em->noticeTimer[i] = 0;
            }
            break;
        }
    }
    return script + 1;
}

/*
 * Test op EMOP_IF_CEILING_ABOVE: true when the monster is in gScene.area and
 * AreaGrid2_GetCeiling finds a ceiling over it.
 */
u8 *EmScript_IfCeilingAbove(Em *em, u8 *script)
{
    f32 ceilingY;
    f32 ceilingInfo;

    switch (*script++) {
    case EMBLK_OPEN:
        if (em->area != gScene.area || AreaGrid2_GetCeiling(&em->posX, &ceilingY, (struct AreaTri *) &ceilingInfo) == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_CEILING_ABOVE && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_CEILING_ABOVE && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_CEILING_ABOVE, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_CEILING_ABOVE && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_CEILING_ABOVE);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_GOAL_VALID: recomputes the goal (Em_ComputeGoal into goal) and is true
 * when none of its three coordinates is 0.
 */
u8 *EmScript_IfGoalValid(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        Em_ComputeGoal(em, em->goal);
        if (em->goal[0] == 0.0f || em->goal[1] == 0.0f || em->goal[2] == 0.0f) {
            for (;;) {
                if (script[0] == EMOP_IF_GOAL_VALID && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_GOAL_VALID && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_VALID, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_GOAL_VALID && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_GOAL_VALID);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_FLOOR_ATTR, one byte: true when the monster's floorAttr equals the byte.
 */
u8 *EmScript_IfFloorAttr(Em *em, u8 *script)
{
    u8 attr;

    switch (*script++) {
    case EMBLK_OPEN:
        attr = *script++;
        if ((u8) em->floorAttr != attr) {
            for (;;) {
                if (script[0] == EMOP_IF_FLOOR_ATTR && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_FLOOR_ATTR && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_FLOOR_ATTR, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_FLOOR_ATTR && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_FLOOR_ATTR);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Command EMOP_PICK_NOTICED_PLAYER_SAME_FLOOR: picks the target into scriptPlayer (-1 for none) among the
 * noticed, active players in the monster's area standing on the same kind of
 * floor (floorAttr 1 to 3, or 4 to 10; none on floorAttr 0), by notice count
 * as EmScript_PickNoticedPlayerInArea.
 */
u8 *EmScript_PickNoticedPlayerSameFloor(Em *em, u8 *script)
{
    s32 i;
    s32 n;
    u8 mask;
    u8 sel;
    u8 list[4];

    if (em->noticed != 0) {
        mask = 0;
        for (i = 0; i < gScene.playerCount; i++) {
            if ((em->noticed & (1 << i)) && Obj_SameArea(em, &gPlayers[i]) != 0 && gPlayers[i].active != 0) {
                mask |= (u8) (1 << i);
            }
        }
        if (mask == 0 || em->floorAttr == 0) {
            *(s8 *) &em->scriptPlayer = -1;
            return script;
        }
        if (em->floorAttr > 0 && em->floorAttr < 4) {
            for (i = 0; i < gScene.playerCount; i++) {
                if (!(mask & (1 << i)) || *(u16 *) &gPlayers[i].floorAttr <= 0 || *(u16 *) &gPlayers[i].floorAttr > 3) {
                    mask &= (u8) ~(1 << i);
                }
            }
        } else {
            for (i = 0; i < gScene.playerCount; i++) {
                if (!(mask & (1 << i)) || *(u16 *) &gPlayers[i].floorAttr < 4 || *(u16 *) &gPlayers[i].floorAttr > 10) {
                    mask &= (u8) ~(1 << i);
                }
            }
        }
        for (i = 0, n = 0; i < gScene.playerCount; i++) {
            if (em->noticeCount[i] == 50000 && (mask & (1 << i))) {
                list[n++] = i;
            }
        }
        if (n >= 2) {
            sel = list[0];
            for (i = 1; i < n; i++) {
                if (em->playerDist[sel] < em->playerDist[list[i]]) {
                    sel = list[i];
                }
            }
        } else if (n == 1) {
            sel = list[0];
        } else {
            for (i = 0, n = 0; i < gScene.playerCount; i++) {
                if (em->noticeCount[i] >= 30000 && (mask & (1 << i))) {
                    list[n++] = i;
                }
            }
            sel = 255;
            if (n >= 2) {
                sel = list[0];
                for (i = 1; i < n; i++) {
                    if (em->playerDist[sel] < em->playerDist[list[i]]) {
                        sel = list[i];
                    }
                }
            } else if (n == 1) {
                sel = list[0];
            } else {
                for (i = 0, n = 0; i < gScene.playerCount; i++) {
                    if (mask & (1 << i)) {
                        list[n++] = i;
                    }
                }
                if (n == 1) {
                    sel = list[0];
                } else if (n != 0) {
                    sel = list[0];
                    for (i = 1; i < n; i++) {
                        if (em->noticeCount[sel] < em->noticeCount[list[i]]) {
                            sel = list[i];
                        }
                    }
                }
            }
        }
        em->scriptPlayer = sel;
    } else {
        *(s8 *) &em->scriptPlayer = -1;
    }
    return script;
}

/*
 * Test op EMOP_IF_GATE_OPEN: true when gScene.skipFlaggedTris is set. CastleGate_Create clears it
 * for quest 3004 and sets it for every other quest; with it set, the area's
 * collision ignores the triangles flagged 0x4000 and the Fatalis area's
 * dropping object runs (likely a gate).
 */
u8 *EmScript_IfGateOpen(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (gScene.skipFlaggedTris == 0) {
            for (;;) {
                if (script[0] == EMOP_IF_GATE_OPEN && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_GATE_OPEN && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_GATE_OPEN, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_GATE_OPEN && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_GATE_OPEN);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

struct EmAreaValue;
void Em_SetAreaTimer2(Em *em, struct EmAreaValue *pair);
u8 *EmScript_SetAreaTimer2(Em *em, u8 *script)
{
    Em_SetAreaTimer2(em, gEmAreaTimer2Values[em->type]);
    return script;
}

//---
/*
 * Switch op EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR on the floor under the target player (scriptPlayer). After
 * the sub-op byte come two unused bytes. Block 1 follows the header, blocks 2
 * and 3 follow the markers EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR 2 and EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR 3, and EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR 255 ends the switch.
 * When the monster's floorAttr is 1, it runs block 2 for a player floorAttr
 * of 2 or 3, else block 3. Otherwise it runs block 1 for 6 or 8, block 2 for
 * 4, 5, 7 or 9, else block 3. With no target it runs block 3. A block that
 * reaches a later marker skips to the end. With a target and block 2 or 3,
 * searching is read before it is set (as in the game). The player's floorAttr
 * is read as a u16 with the byte after it, as the game's code reads it (the
 * monsters' floorAttr is a u16).
 */
u8 *EmScript_SwitchScriptPlayerFloorAttr(Em *em, u8 *script)
{
    Player *pl;
    s8 targetNo;
    u8 branch;
    u8 searching;
    u8 want;

    switch (*script++) {
    case EMBLK_OPEN:
        targetNo = em->scriptPlayer;
        script += 2;
        if (targetNo != -1) {
            pl = &gPlayers[targetNo & 0xF];
        } else {
            searching = 1;
            branch = 3;
            goto found;
        }
        if (em->floorAttr == 1) {
            switch (*(u16 *) &pl->floorAttr) {
            default:
            case 0:
                branch = 3;
                break;
            case 2:
            case 3:
                branch = 2;
                break;
            }
        } else {
            switch (*(u16 *) &pl->floorAttr) {
            case 10:
            default:
                branch = 3;
                break;
            case 4:
            case 5:
            case 7:
            case 9:
                branch = 2;
                break;
            case 6:
            case 8:
                branch = 1;
                break;
            }
        }
        if (branch == 1) {
            break;
        }
    /* the game reads a register it never set: go is s1, still the caller's, when a player is set and r is 2 or 3 */
    found:
        while (searching) {
            want = branch;
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR, EMBLK_END_BRANCH);
            if ((script[0] == EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR && script[1] == want) || (script[0] == EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR && script[1] == EMBLK_END_BRANCH)) {
                searching = 0;
            }
            script = EmScript_NextCmd(em, script);
        }
        break;
    case 1:
    case 2:
    case 3:
        for (;;) {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR, EMBLK_END_BRANCH);
            if (script[1] == EMBLK_END_BRANCH) {
                script += 2;
                break;
            }
            script += 2;
        }
        break;
    case EMBLK_END_BRANCH:
        break;
    }
    return script;
}

/* Test op EMOP_IF_VARIANT_BITS, one byte: true when variant has any bit of the byte. */
u8 *EmScript_IfVariantBits(Em *em, u8 *script)
{
    u8 mask;

    switch (*script++) {
    case EMBLK_OPEN:
        mask = *script++;
        if (!(em->variant & mask)) {
            for (;;) {
                if (script[0] == EMOP_IF_VARIANT_BITS && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_VARIANT_BITS && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_VARIANT_BITS, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_VARIANT_BITS && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_VARIANT_BITS);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_DAY: true by day (Daytime_Get 0). */
u8 *EmScript_IfDay(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (Daytime_Get() & 0xFF) {
            for (;;) {
                if (script[0] == EMOP_IF_DAY && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_DAY && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_DAY, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_DAY && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_DAY);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_IN_ARENA: true on an arena stage, when Map_DayId(gScene.map) is
 * MAP_GREAT_ARENA or MAP_ARENA.
 */
u8 *EmScript_IfInArena(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if ((u32) ((Map_DayId(gScene.map) & 0xFF) - MAP_GREAT_ARENA) > 1) {
            for (;;) {
                if (script[0] == EMOP_IF_IN_ARENA && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_IN_ARENA && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_IN_ARENA, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_IN_ARENA && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_IN_ARENA);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_GOAL_PLAYER_IN_ARC, bytes lo and hi: true when the goal is a player whose
 * direction from the heading rotY lies between lo * 256 and hi * 256.
 */
u8 *EmScript_IfGoalPlayerInArc(Em *em, u8 *script)
{
    s32 ok;
    u16 arcStart;
    u16 arcEnd;
    u16 angle;
    u16 targetNo;
    Player *pl;
    f32 localPos[4];
    f32 worldPos[4];

    switch (*script++) {
    case EMBLK_OPEN:
        arcStart = script[0];
        arcEnd = script[1];
        ok = 0;
        script += 2;
        if (em->goalKind == 1 && *(u16 *) &em->goalArg == 0 && (s16) (targetNo = *(u16 *) &em->goalNo) != -1) {
            Em_GetTargetPos(em, targetNo, localPos);
            pl = &gPlayers[targetNo];
            Area_ToWorld(pl->area, localPos, worldPos);
            angle = Math_AngleXZ(em->mapPos, worldPos) - em->rotY;
            if ((u16) (arcStart << 8) > angle || angle > (u16) (arcEnd << 8)) {
                ok = 1;
            }
        } else {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == EMOP_IF_GOAL_PLAYER_IN_ARC && script[1] == EMBLK_ELSE) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == EMOP_IF_GOAL_PLAYER_IN_ARC && script[1] == EMBLK_END_IF) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_PLAYER_IN_ARC, EMBLK_END_IF);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == EMOP_IF_GOAL_PLAYER_IN_ARC && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_GOAL_PLAYER_IN_ARC && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_GOAL_PLAYER_IN_ARC, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_GOAL_PLAYER_IN_ARC && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_GOAL_PLAYER_IN_ARC);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER, one byte: true when the picked player's noticeCount is at
 * least gEmNoticeThresholds[byte].
 */
u8 *EmScript_IfScriptPlayerNoticeOver(Em *em, u8 *script)
{
    u8 index;
    s8 k;

    switch (*script++) {
    case EMBLK_OPEN:
        index = *script++;
        k = em->scriptPlayer;
        if (k == -1 || em->noticeCount[k] < gEmNoticeThresholds[index]) {
            for (;;) {
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Command EMOP_CLEAR_NOTICE_COUNTS: clears every player's noticeCount. */
u8 *EmScript_ClearNoticeCounts(Em *em, u8 *script)
{
    em->noticeCount[0] = 0;
    em->noticeCount[1] = 0;
    em->noticeCount[2] = 0;
    em->noticeCount[3] = 0;
    return script;
}

/*
 * Test op EMOP_IF_ANY_PLAYER_RIDING: true when any player rides a monster (rideState; not
 * necessarily this one).
 */
u8 *EmScript_IfAnyPlayerRiding(Em *em, u8 *script)
{
    u8 ok = 1;
    Player *pl;
    s8 i;

    switch (*script++) {
    case EMBLK_OPEN:
        pl = gPlayers;
        for (i = 0; i < gScene.playerCount; i++, pl++) {
            if (pl->rideState != 0) {
                ok = 0;
            }
        }
        if (ok) {
            while (ok) {
                if (script[0] == EMOP_IF_ANY_PLAYER_RIDING && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_ANY_PLAYER_RIDING && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_ANY_PLAYER_RIDING, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_ANY_PLAYER_RIDING && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_ANY_PLAYER_RIDING);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_NOT_REMOTE: true when this machine runs the monster (remote 0). */
u8 *EmScript_IfNotRemote(Em *em, u8 *script)
{
    s32 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        ok = em->remote != 0;
        if (ok) {
            while (ok) {
                if (script[0] == EMOP_IF_NOT_REMOTE && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_NOT_REMOTE && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_NOT_REMOTE, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_NOT_REMOTE && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_NOT_REMOTE);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

s32 EmScript_RestartMain(Em *em, s32 script)
{
    Em_RestartScript(em);
    return script;
}

/*
 * Op EMOP_RANDOM_BRANCH, a weighted random branch: after the branch count, each branch is a
 * marker with a weight; the roll is stateRand & 0x1F and the first branch
 * whose running total passes it runs (weight 0 never, 255 always). A branch
 * that reaches the next marker skips to the closing sub-byte 255.
 */
u8 *EmScript_RandomBranch(Em *em, u8 *script)
{
    u16 i;
    u8 sum;
    u16 roll;
    u8 n;
    u8 w;

    switch (*script) {
    case EMBLK_OPEN:
        n = script[1];
        roll = em->stateRand & 0x1F;
        sum = 0;
        i = 0;
        script += 2;
        for (; i < n; i++) {
            script = EmScript_SkipBlock(em, script, EMOP_RANDOM_BRANCH, EMBLK_END_BRANCH);
            w = script[2];
            script += 3;
            if (!w) {
                continue;
            }
            if (w == 255) {
                break;
            }
            sum += w;
            if (roll < sum) {
                break;
            }
        }
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        script += 2;
        for (;;) {
            script = EmScript_SkipBlock(em, script, EMOP_RANDOM_BRANCH, EMBLK_END_BRANCH);
            if (script[1] == EMBLK_END_BRANCH) {
                script += 2;
                break;
            }
            script += 3;
        }
        break;
    case EMBLK_END_BRANCH:
        script++;
        break;
    }
    return script;
}

/*
 * Command EMOP_CALL_SCRIPT, one byte: calls subscript byte of table 1; the first call
 * saves the return in scriptCallReturn (EMEND_CALL returns there).
 * Returns the subscript's start, also in scriptPos.
 */
u8 *EmScript_CallScript(Em *em, u8 *pos)
{
    em->scriptCallNo = *pos;
    if (em->scriptCallDepth == 0) {
        em->scriptCallReturn = pos + 1;
    }
    em->scriptCallDepth = 1;
    pos = em->script[1][em->scriptCallNo];
    em->scriptPos = pos;
    return pos;
}

/* Command EMOP_JUMP_ENTRY (2 argument bytes: a script table k and an entry): jumps to script entry of table
 * 15 + k; the first such jump after an EMOP_CALL_SCRIPT call remembers the return position
 * (scriptJumpReturn). */
u8 *EmScript_JumpEntry(Em *em, u8 *pos)
{
    u8 k;

    k = *pos++;
    em->scriptCallEntry = *pos++;
    if (em->scriptCallDepth == 1) {
        em->scriptJumpReturn = pos;
    }
    em->scriptCallDepth = 2;
    pos = em->script[k + 15][em->scriptCallEntry];
    em->scriptPos = pos;
    return pos;
}

/*
 * Command EMOP_SET_GOAL: sets the goal from the bytes (goalKind, goalArg, goalNo).
 * Kind 3, an area, reads a two-byte area id (Area_IdForCurrentTime) into
 * nextArea and clears destArea; kind 10 becomes kind 3 toward the picked
 * player's area; kind 13 makes the first other large monster in the area the
 * goal.
 */
u8 *EmScript_SetGoal(Em *em, u8 *script)
{
    s32 i;
    Em *other;

    em->goalKind = script[0];
    switch (em->goalKind) {
    case 0:
        em->goalKind = 0;
        em->goalArg = script[1];
        em->goalNo = *(script += 2);
        script++;
        break;
    case 1:
        em->goalKind = 1;
        em->goalArg = script[1];
        em->goalNo = *(script += 2);
        script++;
        break;
    case 2:
    case 4:
    case 5:
    case 6:
        em->goalArg = script[1];
        em->goalNo = *(script += 2);
        script++;
        break;
    case 3:
        em->goalArg = script[1];
        em->goalNo = *(script += 2);
        em->goalNo = *(u16 *) &em->goalNo << 8;
        *(u16 *) &em->goalNo |= script[1];
        script += 2;
        em->goalNo = Area_IdForCurrentTime(*(u16 *) &em->goalNo);
        em->goalArg = *(u16 *) &em->goalNo;
        em->nextArea = *(u16 *) &em->goalNo;
        *(u16 *) &em->destArea = 65535;
        break;
    case 10:
        em->goalKind = 3;
        em->goalArg = script[1];
        em->goalNo = *(script += 2);
        script++;
        if ((s8) em->scriptPlayer == -1) {
            em->goalNo = *(u8 *) &em->area;
        } else {
            em->goalNo = ((u8 *)&gPlayers[0].area)[(s8) em->scriptPlayer * sizeof(Player)];
        }
        em->goalArg = *(u16 *) &em->goalNo;
        em->nextArea = *(u16 *) &em->goalNo;
        *(u16 *) &em->destArea = 65535;
        break;
    case 13:
        em->goalKind = 13;
        em->goalArg = script[1];
        em->goalNo = *(script += 2);
        script++;
        em->goalNo = 0;
        for (i = 0, other = gEms; i < 20; i++, other++) {
            if (other->active != 0 && em->area == other->area && other != em && other->large != 0) {
                break;
            }
        }
        if (i <= 19) {
            em->goalNo = i;
        } else {
            *(u16 *) &em->goalNo = 65535;
        }
        break;
    default:
        em->goalArg = script[1];
        em->goalNo = *(script += 2);
        script++;
        break;
    }
    return script;
}

/* Command EMOP_SWITCH_TARGET_DIST (a multi-way branch on distance, 0..5 then 255): picks the first of the reaction
 * distances that the distance to the target is within and runs that branch; with no target, the
 * last one. The distances are read as one array from reactDist on (the third is nearRadius), as
 * the game's code indexes them. */
u8 *EmScript_SwitchTargetDist(Em *em, u8 *script)
{
    s8 k;
    f32 dist;
    u8 n;
    s32 i;
    s32 j;

    switch (*script++) {
    case EMBLK_OPEN:
        k = em->scriptPlayer;
        if (k == -1) {
            dist = -1.0f;
        } else if (em->curGoalKind == 13) {
            dist = Vec_Distance(&em->posX, &gEms[k].posX);
        } else {
            dist = em->playerDist[k & 0xF];
        }
        n = *script++;
        if (dist == -1.0f) {
            i = n;
        } else {
            em->targetDist = dist;
            for (i = 0; i < n; i++) {
                if (em->targetDist <= em->reactDist[i] && em->reactDist[i] >= 0.0f) {
                    break;
                }
            }
        }
        em->scriptCaseCount = i;
        for (j = 0; j < em->scriptCaseCount; j++) {
            script = EmScript_NextCmd(em, script);
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TARGET_DIST, EMBLK_END_BRANCH);
        }
        script = EmScript_NextCmd(em, script);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        for (;;) {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_TARGET_DIST, EMBLK_END_BRANCH);
            if (script[1] == EMBLK_END_BRANCH) {
                script += 2;
                break;
            }
            script += 2;
        }
        break;
    case EMBLK_END_BRANCH:
        break;
    }
    return script;
}

/*
 * Op EMOP_END, the end of a script thread or block; the sub-byte says which
 * (enum EmScriptEnd). EMEND_MAIN restarts the main script; EMEND_CALL,
 * EMEND_JUMP and EMEND_SUBSCRIPT return from a call; EMEND_ROUTE_ENTER and
 * EMEND_ROUTE_STEP step along the route of EmScript_StartRoute; 245 to 247
 * and 252 restart the main script with scriptFlags cleared; the thread ends
 * EMEND_RAGE, EMEND_SENSED, EMEND_AREA_OBJ and EMEND_PART_BUILDUP clear their
 * thread's bit; EMEND_AREA_SEQ_STEP ends a step of the area sequence,
 * EMEND_LOOP_PASS a loop pass, and EMEND_LOOP repeats the loop script for
 * loopCount passes. Returns the new position, also in scriptPos.
 */
u8 *EmScript_End(Em *em, u8 *script)
{
    EmRouteSet *set;
    EmRoute *routes;
    EmRoutePt *pts;
    f32 dist;
    f32 lim;
    s8 n;
    s8 i;
    u8 *listPtr;
    s8 k;
    u8 buf[16];
    f32 goalPos[4];

    switch (*script) {
    case EMEND_MAIN:
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case EMEND_CALL:
        em->scriptCallDepth = 0;
        script = em->scriptCallReturn;
        break;
    case EMEND_JUMP:
        em->scriptCallDepth = 1;
        script = em->scriptJumpReturn;
        break;
    case EMEND_SUBSCRIPT:
        script = em->subScriptReturn;
        break;
    case EMEND_ROUTE_ENTER:
    case EMEND_ROUTE_STEP:
        set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
        routes = set->routes;
        if (routes == NULL) {
            script = em->routeResume;
            em->routeResume = NULL;
            em->goalKind = 0;
            em->curGoalKind = 0;
            break;
        }
        if (em->routeNo >= set->n) {
            em->routeNo = set->n - 1;
        }
        pts = routes[em->routeNo].pts;
        if (em->routePoint >= routes->n) {
            em->routePoint = routes->n - 1;
        }
        if (em->remote != 0) {
            if (em->curGoalKind == 9) {
                em->routePointScript = pts[em->routePoint].script;
                script = EmScript_RoutePointScript(em, em->routePointScript);
            } else {
                em->scriptNo = 0;
                script = (u8 *) Em_StartScript(em);
            }
            break;
        }
        if (em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
            dist = Vec_Distance(&em->posX, pts[em->routePoint].pos);
        } else {
            dist = Vec_DistanceXZ(&em->posX, pts[em->routePoint].pos);
        }
        if (!(dist <= pts[em->routePoint].radius)) {
            if (++*(s8 *) &em->routeMissCount >= 10) {
                if (em->routePoint == 0) {
                    goto last;
                }
                em->routePoint = 0;
                em->routeMissCount = 0;
            }
            em->goalKind = 9;
            em->goalArg = em->routeNo;
            em->goalNo = em->routePoint;
            em->curGoalKind = em->goalKind;
            em->curGoalArg = *(u16 *) &em->goalArg;
            em->curGoalNo = *(u16 *) &em->goalNo;
            em->routePointScript = pts[em->routePoint].script;
            script = EmScript_RoutePointScript(em, em->routePointScript);
            em->scriptPos = script;
        } else if (em->routePoint == 0) {
        last:
            if (*script == EMOP_RESTART && (s16) routes[em->curGoalArg].area != -1 && em->large != 0) {
                em->posX = routes[em->routeNo].pos[0];
                em->posY = routes[em->routeNo].pos[1];
                em->posZ = routes[em->routeNo].pos[2];
                Vec_Copy(&em->prevPosX, &em->posX);
                em->rotY = routes[em->curGoalArg].rotY;
                em->prevArea = (u8) em->area;
                em->area = Area_IdForCurrentTime(routes[em->curGoalArg].area);
                Em_TypeSetup(em);
                Em_RequestSync(em, 1);
                Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *) &em->floorKind, &em->floorY, &em->surfaceY);
            }
            script = em->routeResume;
            em->routeResume = NULL;
            em->goalKind = 0;
            em->curGoalKind = 0;
        } else {
            em->routeMissCount = 0;
            em->routePoint--;
            em->goalKind = 9;
            em->goalArg = em->routeNo;
            em->goalNo = em->routePoint;
            em->curGoalKind = em->goalKind;
            em->curGoalArg = *(u16 *) &em->goalArg;
            em->curGoalNo = *(u16 *) &em->goalNo;
            em->routePointScript = pts[em->routePoint].script;
            script = EmScript_RoutePointScript(em, em->routePointScript);
        }
        break;
    case 245:
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 246:
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 247:
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        if ((s8) em->scriptPlayer != -1) {
            s32 k = (s8) em->scriptPlayer & 0xF;
            em->noticed &= ~(1 << k);
            em->noticeTimer[k] = 0;
            em->alertBuildup[k] = 0;
            em->noticeCount[k] = 0;
        }
        break;
    case EMEND_RAGE:
        em->scriptFlags &= 0xF7;
        if (em->scriptFlags & 1) {
            script = EmScript_LoopScript(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case EMEND_SENSED:
        em->scriptFlags &= 0xFB;
        if (em->scriptFlags & 1) {
            script = EmScript_LoopScript(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case EMEND_AREA_OBJ:
        Em_StartScript(em);
        em->scriptFlags &= 0xFD;
        Em_SelectSightRow(em, 0);
        if (em->scriptFlags & 1) {
            script = EmScript_LoopScript(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case EMEND_AREA_SEQ_STEP:
        em->areaSeqActive = 0;
        script = em->areaSeqNext;
        em->areaSeqPos++;
        if (em->areaSeqLen <= em->areaSeqPos) {
            if (em->areaSeqMode == 0) {
                em->areaSeqLen = em->areaSeqPos = -1;
            } else {
                em->areaSeqPos = 0;
            }
        }
        break;
    case 252:
        if (em->remote == 0) {
            Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            em->scriptCombat = em->combat;
        }
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case EMEND_PART_BUILDUP:
        em->scriptFlags &= 0xEF;
        if (em->scriptFlags & 1) {
            script = EmScript_LoopScript(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            em->syncRequest = 2;
        }
        break;
    case EMEND_LOOP_PASS:
        Em_ComputeGoal(em, goalPos);
        dist = Vec_DistanceXZ(goalPos, &em->posX);
        if (em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C || em->type == EM_KIND_SHEN_GAOREN ||
            em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
            lim = 1000.0f;
        } else {
            lim = 500.0f;
        }
        if (dist <= lim) {
            em->loopIndex++;
            switch (em->loopMode) {
            case 0:
                i = 0;
                n = 0;
                if (i < (s8) em->areaPointCount) {
                    k = em->loopPoint;
                    listPtr = buf;
                    do {
                        if (k != i) {
                            *listPtr++ = i;
                            n++;
                        }
                        i++;
                    } while (i < (s8) em->areaPointCount);
                }
                if (n == 0) {
                    em->loopPoint = 0;
                } else {
                    em->loopPoint = buf[(s8) (em->stateRand % n)];
                }
                break;
            case 1:
                em->loopPoint = ((u8 *)EmScript_LoopPointList(em, em->loopPointTable))[(s8) em->loopIndex];
                break;
            case 2:
                em->loopPoint++;
                break;
            }
        }
        script = EmScript_LoopScript(em, em->loopEndScript);
        break;
    case EMEND_LOOP:
        if ((s8) em->loopIndex < em->loopCount) {
            script = EmScript_LoopScript(em, em->loopScript);
        } else {
            em->scriptFlags &= 0xFE;
            script = EmScript_GetPos(em);
        }
        break;
    }
    em->scriptPos = script;
    return script;
}

/*
 * Command EMOP_PLACE_AT_POINT, one byte: moves the monster (and its previous position) to
 * that point of gEmGoalPoints[type].
 */
u8 *EmScript_PlaceAtPoint(Em *em, u8 *script)
{
    f32 *point = (f32 *) (gEmGoalPoints[em->type] + *script * sizeof(f32[3]));

    em->posX = point[0];
    em->posY = point[1];
    em->posZ = point[2];
    em->prevPosX = em->posX;
    em->prevPosY = em->posY;
    em->prevPosZ = em->posZ;
    return script + 1;
}

void * EmScript_FacePoint(Em *em, u8 *script)
{
    em->rotY = Math_AngleXZ((f32 *) &em->posX, (f32 *) (gEmGoalPoints[em->type] + (*script * 12))) & 0xFFFF;
    return script + 1;
}

void * EmScript_SetHeading(Em *em, u8 *script)
{
    em->rotY = *script << 8;
    return script + 1;
}

/*
 * Op EMOP_NOP: does nothing and returns script, the next command. EmScript_Run
 * passes the monster as an integer, unused.
 */
s32 EmScript_Nop(s32 unused, s32 script)
{
    return script;
}

/* Op EMOP_NOP_ALT: the same as EmScript_Nop. */
s32 EmScript_NopAlt(s32 unused, s32 script)
{
    return script;
}

/*
 * Switch op EMOP_SWITCH_SCRIPT_VALUE on gEmScriptSwitchValue, a value EmScript_Init sets to 0 and nothing
 * else writes. The header EMOP_SWITCH_SCRIPT_VALUE 0 holds the case count and two more bytes.
 * Then come ascending case values, each a value byte and its block; EMOP_SWITCH_SCRIPT_VALUE 1
 * starts each later case, EMOP_SWITCH_SCRIPT_VALUE 2 the default and EMOP_SWITCH_SCRIPT_VALUE 3 the end. It runs the
 * block of the equal value. A listed value above it sends it to the default
 * block (or past the end), and a block that reaches the next case or the
 * default skips to the end.
 */
u8 *EmScript_SwitchScriptValue(Em *em, u8 *script)
{
    u8 caseCount;
    s32 i;
    u8 listedValue;
    u8 ok;

    switch (*script++) {
    case EMBLK_OPEN:
        caseCount = script[0];
        i = 0;
        script += 3;
        if (i < caseCount) {
            do {
                listedValue = *script++;
                if (gEmScriptSwitchValue == listedValue) {
                    break;
                }
                if (listedValue < gEmScriptSwitchValue) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SCRIPT_VALUE, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SCRIPT_VALUE, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_SCRIPT_VALUE && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_SCRIPT_VALUE && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < caseCount);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_SCRIPT_VALUE, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_SCRIPT_VALUE && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_LEVEL_AT_LEAST, one byte: true when the monster's level is at least the byte.
 */
u8 *EmScript_IfLevelAtLeast(Em *em, u8 *script)
{
    u8 minLevel;

    switch (*script++) {
    case EMBLK_OPEN:
        minLevel = *script++;
        if (em->level < minLevel) {
            for (;;) {
                if (script[0] == EMOP_IF_LEVEL_AT_LEAST && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_LEVEL_AT_LEAST && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_LEVEL_AT_LEAST, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_LEVEL_AT_LEAST && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_LEVEL_AT_LEAST);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Test op EMOP_IF_VERSUS: true in a versus quest (Quest_IsVersus). */
u8 *EmScript_IfVersus(Em *em, u8 *script)
{
    switch (*script++) {
    case EMBLK_OPEN:
        if (Quest_IsVersus() != 1) {
            for (;;) {
                if (script[0] == EMOP_IF_VERSUS && script[1] == EMBLK_ELSE) {
                    break;
                }
                if (script[0] == EMOP_IF_VERSUS && script[1] == EMBLK_END_IF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, EMOP_IF_VERSUS, EMBLK_END_IF);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == EMOP_IF_VERSUS && script[1] == EMBLK_END_IF) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_VERSUS);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/*
 * Switch op EMOP_SWITCH_ITEM_USE_KIND on the item use kind (itemUseKind without bit 0x80): n
 * values, ascending; the use is cleared once taken.
 */
u8 *EmScript_SwitchItemUseKind(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 listedKind;
    u8 ok;
    u8 x;

    switch (*script++) {
    case EMBLK_OPEN:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                listedKind = *script++;
                x = em->itemUseKind & ~0x80;
                if (x == listedKind) {
                    em->itemUseKind &= 0x80;
                    break;
                }
                if (listedKind < x) {
                    script = EmScript_SkipBlock(em, script, EMOP_SWITCH_ITEM_USE_KIND, EMBLK_END_SWITCH);
                    if (script[1] == EMBLK_DEFAULT || script[1] == EMBLK_END_SWITCH) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, EMOP_SWITCH_ITEM_USE_KIND, EMBLK_END_SWITCH);
                        if ((script[0] == EMOP_SWITCH_ITEM_USE_KIND && script[1] == EMBLK_DEFAULT) || (script[0] == EMOP_SWITCH_ITEM_USE_KIND && script[1] == EMBLK_END_SWITCH)) {
                            ok = 0;
                        }
                        script = EmScript_NextCmd(em, script);
                    } while (ok);
                    break;
                }
                i++;
            } while (i < n);
        }
        break;
    case EMBLK_CASE:
        script++;
    case EMBLK_DEFAULT:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, EMOP_SWITCH_ITEM_USE_KIND, EMBLK_END_SWITCH);
            if (script[0] == EMOP_SWITCH_ITEM_USE_KIND && script[1] == EMBLK_END_SWITCH) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case EMBLK_END_SWITCH:
        break;
    }
    return script;
}

/*
 * Test op EMOP_IF_VERSUS_ITEM_USED, one byte: in a versus quest, when the item use kind is above
 * the byte, adds to the versus gauge (Versus_AddGauge) and is true.
 */
u8 *EmScript_IfVersusItemUsed(Em *em, u8 *script)
{
    u8 minKind;
    u8 n;

    switch (*script++) {
    case EMBLK_OPEN:
        minKind = *script++;
        if (Quest_IsVersus() == 1 && (n = em->itemUseKind & ~0x80) > minKind) {
            if (Versus_AddGauge(em, n) == 1) {
                break;
            }
            em->itemUseKind &= 0x80;
        }
        for (;;) {
            if (script[0] == EMOP_IF_VERSUS_ITEM_USED && script[1] == EMBLK_ELSE) {
                break;
            }
            if (script[0] == EMOP_IF_VERSUS_ITEM_USED && script[1] == EMBLK_END_IF) {
                break;
            }
            script = EmScript_SkipBlock(em, script, EMOP_IF_VERSUS_ITEM_USED, EMBLK_END_IF);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == EMOP_IF_VERSUS_ITEM_USED && script[1] == EMBLK_END_IF) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case EMBLK_ELSE:
        script = EmScript_SkipToEnd(em, script, EMOP_IF_VERSUS_ITEM_USED);
        break;
    case EMBLK_END_IF:
        break;
    }
    return script;
}

/* Starts script em->scriptNo of the monster's command scripts. */
s32 Em_StartScript(Em *em)
{
    u8 *pos;

    em->scriptCallDepth = 0;
    pos = (*em->script)[em->scriptNo];
    em->scriptPos = pos;
    return (s32)pos;
}

/*
 * Skips whole commands from script to the next command of op opcode whose
 * sub-byte is not 0, stepping over nested blocks of the same op (sub-byte 0
 * opens one, end closes it), and returns it: the else, end or case marker of
 * the block skipped. end is 2 for the tests, 3 for the switches and 255 for
 * the branches.
 */
u8 *EmScript_SkipBlock(Em *em, u8 *script, u8 opcode, u8 end)
{
    for (;;) {
        while (script[0] != opcode) {
            script = EmScript_NextCmd(em, script);
        }
        if (script[0] != opcode || script[1] != 0) {
            break;
        }
        script = EmScript_NextCmd(em, script);
        for (;;) {
            if (script[0] == opcode && script[1] == end) {
                script = EmScript_NextCmd(em, script);
                break;
            }
            if (script[0] == opcode) {
                script = EmScript_NextCmd(em, script);
            }
            script = EmScript_SkipBlock(em, script, opcode, end);
        }
    }
    return script;
}

/* Skips the command at p (its arguments, and a sub-command where it has one); an unknown command
 * stops the script (EmScript_SetAbort with 4). The game keeps p in a1 across that call: EmScript_SetAbort
 * is static and defined earlier in this file (catalogue "Arguments kept or reloaded across calls"). */
u8 *EmScript_NextCmd(Em *em, u8 *script)
{
    switch (*script) {
    case EMOP_IF_SENSED:
        script += 2;
        break;
    case EMOP_IF_NOTICED:
        script += 2;
        break;
    case EMOP_IF_CAN_ENTER_NEXT_AREA:
        script += 2;
        break;
    case EMOP_RESTART:
        script += 1;
        break;
    case EMOP_CHANGE_STATE:
        script += 4;
        break;
    case EMOP_SET_GOAL:
        if (script[1] == 3) {
            script += 5;
        } else {
            script += 4;
        }
        break;
    case EMOP_START:
        script += 2;
        break;
    case EMOP_IF_ON_GROUND:
        script += 2;
        break;
    case EMOP_IF_STANCE_FAMILY:
        script += 2;
        break;
    case EMOP_IF_IN_AIR:
        script += 2;
        break;
    case EMOP_SET_HIT_REACT_MODE:
        script += 2;
        break;
    case EMOP_IF_COMBAT_IS:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SET_VAR:
        script += 3;
        break;
    case EMOP_CLEAR_VAR:
        script += 2;
        break;
    case EMOP_IF_IN_AREA:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_START_LOOP:
        script += 6;
        break;
    case EMOP_NEXT_LOOP_POINT:
        script += 1;
        break;
    case EMOP_GOAL_ALERTED_PLAYER:
        script += 1;
        break;
    case EMOP_GOAL_NOTICED_PLAYER:
        script += 1;
        break;
    case EMOP_GOAL_SCRIPT_PLAYER:
        script += 1;
        break;
    case EMOP_IF_TARGET_OFF_HEADING:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_AREA:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 2;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_CALL:
        script += 2;
        break;
    case EMOP_SET_AREA_SEQ:
        script += 5;
        break;
    case EMOP_STEP_AREA_SEQ:
        script += 1;
        break;
    case EMOP_GOAL_DEST_AREA:
        script += 1;
        break;
    case EMOP_GOAL_AREA_TOWARD:
        script += 3;
        break;
    case EMOP_IF_REQUESTED_IS:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_HP_PERCENT:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_SWITCH_REQUEST_ID:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_SET_REQUEST:
        script += 1;
        break;
    case EMOP_SWITCH_TARGET_ANGLE:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_IF_NEED2_LOW:
        script += 1;
        switch (*script) {
        case 0:
            script += 1;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_GOAL_NEAR_XZ:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_GOAL_NEAR:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_KIND_IN_AREA:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_REPEAT:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
            script += 1;
            break;
        }
        break;
    case EMOP_STOP_REPEAT:
        script += 1;
        break;
    case EMOP_SET_NO_ALERT:
        script += 2;
        break;
    case EMOP_SWITCH_VARIANT:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_IF_PLAYER_IN_AREA:
        script += 2;
        break;
    case EMOP_IF_AREA_TIMER1_OVER:
        script += 2;
        break;
    case EMOP_IF_AREA_TIMER2_OVER:
        script += 2;
        break;
    case EMOP_IF_VAR_IS:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_KIND_GROUP:
    case EMOP_SWITCH_TYPE:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_GOAL_ACTION_GOAL:
        script += 1;
        break;
    case EMOP_SELECT_SIGHT_ROW:
        script += 2;
        break;
    case EMOP_IF_PLAYER_CARRYING:
        script += 2;
        break;
    case EMOP_SET_NEED4:
        script += 1;
        break;
    case EMOP_SET_GOAL_KIND:
        script += 1;
        break;
    case EMOP_IF_HIT_REACT_MODE:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_HIT_REACT_MODE:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_IF_STATE:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_RAGING:
        script += 2;
        break;
    case EMOP_IF_DRILL:
        script += 2;
        break;
    case EMOP_IF_ON_PASS_THROUGH_FLOOR:
        script += 2;
        break;
    case EMOP_IF_BLIND:
        script += 2;
        break;
    case EMOP_IF_NO_SENSE_DELAY:
        script += 2;
        break;
    case EMOP_IF_LINKED:
        script += 2;
        break;
    case EMOP_IF_LINK_COMBAT_SAME_AREA:
        script += 2;
        break;
    case EMOP_IF_PREV_AREA:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_PREV_AREA:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
        case 1:
            script += 3;
            break;
        case 2:
        case 3:
            script += 1;
            break;
        }
        break;
    case EMOP_START_ROUTE:
        script += 4;
        break;
    case EMOP_SET_COMBAT:
        script += 2;
        break;
    case EMOP_ADD_HP:
        script += 2;
        break;
    case EMOP_IF_GOAL_ANGLE_OVER:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_GOAL_ANGLE_OVER_CAPPED:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_LINK_SAME_AREA:
        script += 2;
        break;
    case EMOP_IF_LONE_PLAYER_BAIT:
        script += 2;
        break;
    case EMOP_IF_SCRIPT_PLAYER_STATE:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_HOOKED:
        script += 2;
        break;
    case EMOP_SET_DELAY:
        script += 2;
        break;
    case EMOP_GOAL_PLAYER_FLOOR_SPOT:
        script += 2;
        break;
    case EMOP_IF_SCRIPT_PLAYER_SEEN:
        script += 2;
        break;
    case EMOP_SET_ALERT_BUILDUP:
        script += 1;
        break;
    case EMOP_SET_NOTICE_COUNT:
        script += 1;
        break;
    case EMOP_COMPUTE_GOAL:
        script += 1;
        break;
    case EMOP_ADD_NEED2:
    case EMOP_ADD_NEED1:
    case EMOP_ADD_NEED3:
        script += 2;
        break;
    case EMOP_IF_STANCE_UNDER:
        script += 2;
        break;
    case EMOP_PICK_NOTICED_PLAYER_REACHABLE:
        script += 1;
        break;
    case EMOP_PICK_NOTICED_PLAYER_IN_AREA:
        script += 1;
        break;
    case EMOP_IF_SCRIPT_PLAYER_IN_AREA:
        script += 2;
        break;
    case EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH:
        script += 2;
        break;
    case EMOP_IF_QUEST_ID:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_QUEST_KEY:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
        case 1:
            script += 3;
            break;
        case 2:
        case 3:
            script += 1;
            break;
        }
        break;
    case EMOP_GOAL_LINK_TARGET:
        script += 1;
        break;
    case EMOP_IF_ON_CEILING:
        script += 2;
        break;
    case EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SET_NOTICE_TIMER:
        script += 2;
        break;
    case EMOP_IF_CEILING_ABOVE:
        script += 2;
        break;
    case EMOP_IF_GOAL_VALID:
        script += 2;
        break;
    case EMOP_IF_FLOOR_ATTR:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_PICK_NOTICED_PLAYER_SAME_FLOOR:
        script += 1;
        break;
    case EMOP_IF_GATE_OPEN:
        script += 2;
        break;
    case EMOP_SET_AREA_TIMER2:
        script += 1;
        break;
    case EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR:
        script += 2;
        break;
    case EMOP_IF_VARIANT_BITS:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_CLEAR_NOTICE_COUNTS:
        script += 1;
        break;
    case EMOP_IF_ANY_PLAYER_RIDING:
        script += 2;
        break;
    case EMOP_IF_NOT_REMOTE:
        script += 2;
        break;
    case EMOP_RESTART_MAIN:
        script += 1;
        break;
    case EMOP_RANDOM_BRANCH:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            script += 2;
            break;
        case 255:
            script += 1;
            break;
        }
        break;
    case EMOP_CALL_SCRIPT:
        script += 2;
        break;
    case EMOP_JUMP_ENTRY:
        script += 3;
        break;
    case EMOP_SWITCH_TARGET_DIST:
        script += 1;
        switch (*script) {
        case 0:
            script += 4;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 255:
            script += 1;
            break;
        }
        break;
    case EMOP_ADVANCE_STATE_RAND:
    case EMOP_ADVANCE_STATE_RAND_ALT:
        script += 1;
        break;
    case EMOP_END:
        script += 2;
        break;
    case EMOP_PLACE_AT_POINT:
        script += 2;
        break;
    case EMOP_FACE_POINT:
    case EMOP_SET_HEADING:
        script += 2;
        break;
    case EMOP_NOP:
    case EMOP_NOP_ALT:
        script += 1;
        break;
    case EMOP_SWITCH_SCRIPT_VALUE:
        script += 1;
        switch (*script++) {
        case 0:
            script += 2;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_IF_LEVEL_AT_LEAST:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_IF_VERSUS:
        script += 2;
        break;
    case EMOP_SWITCH_ITEM_USE_KIND:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_IF_VERSUS_ITEM_USED:
        script += 1;
        switch (*script) {
        case 0:
            script += 2;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_SPAWN_PARAM:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_SWITCH_SEASON:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_IF_DAY:
        script += 2;
        break;
    case EMOP_IF_GOAL_PLAYER_IN_ARC:
        script += 1;
        switch (*script) {
        case 0:
            script += 3;
            break;
        case 1:
        case 2:
            script += 1;
            break;
        }
        break;
    case EMOP_SWITCH_KIND_QUERY:
        script += 1;
        switch (*script++) {
        case 0:
            script += 4;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_SWITCH_BASE_AREA:
        script += 1;
        switch (*script++) {
        case 0:
            script += 3;
        case 1:
            script += 1;
            break;
        case 2:
        case 3:
            break;
        }
        break;
    case EMOP_IF_IN_ARENA:
        script += 2;
        break;
    default:
        EmScript_SetAbort(em, script, 4);
        break;
    }
    return script;
}

/*
 * At a test's else (sub-byte 1) reached while running: returns the position
 * past the same op's end (sub-byte 2), stepping over nested blocks.
 */
u8 *EmScript_SkipToEnd(Em *em, u8 *script, u8 opcode)
{
    Em *emCopy;

    if (script[0] == opcode && script[1] == 2) {
        script = EmScript_NextCmd(em, script);
    } else {
        emCopy = em; /* kept for the match: with em itself MWCC lays the loop out differently */
        for (;;) {
            if (script[0] == opcode && script[1] == 2) {
                script = EmScript_NextCmd(emCopy, script);
                break;
            }
            script = EmScript_SkipBlock(emCopy, script, opcode, 2);
        }
    }
    return script;
}

/*
 * Restarts the monster's script from main script 0 (sets scriptRestart for
 * EmScript_Restart); on the machine that runs it, with no sync pending, also
 * asks for a state sync (Em_RequestSync 2).
 */
void Em_RestartScript(Em *em)
{
    if ((em->remote == 0) && (em->syncRequest == 0)) {
        Em_RequestSync(em, 2U);
    }
    em->scriptRestart = 1;
    EmScript_Restart(em);
}

/* The monster enters area unk684 (the one it left goes to unk882): placed on the side of the new area
 * rectangle that faces the old area (by the direction between the two, in eighths), on its floor. */
void Em_EnterNextArea(Em *em)
{
    f32 v[2][3];
    AreaRecord *areaRec;
    f32 x;
    f32 z;

    em->prevArea = *(u8 *) &em->area;
    em->area = em->nextArea;
    Map_GetPosition(v[0], &v[0][2], em->prevArea);
    Map_GetPosition(v[1], &v[1][2], em->area);
    Stage_GetAreaRecord(em->prevArea);
    areaRec = Stage_GetAreaRecord(em->area);
    switch ((u16) ((u16) (Math_AngleXZ(v[0], v[1]) + 0x1000) >> 13)) {
    case 4:
        z = areaRec->sizeZ;
        x = areaRec->sizeX / 2.0f;
        break;
    case 5:
        x = areaRec->sizeX;
        z = areaRec->sizeZ;
        break;
    case 6:
        x = areaRec->sizeX;
        z = areaRec->sizeZ / 2.0f;
        break;
    case 7:
        z = 0.0f;
        x = areaRec->sizeX;
        break;
    case 0:
        z = 0.0f;
        x = areaRec->sizeX / 2.0f;
        break;
    case 1:
        z = x = 0.0f;
        break;
    case 2:
        x = 0.0f;
        z = areaRec->sizeZ / 2.0f;
        break;
    case 3:
        z = areaRec->sizeZ;
        x = 0.0f;
        break;
    }
    em->posX = x;
    em->posZ = z;
    em->prevPosX = x;
    em->prevPosZ = z;
    em->velZ = 50.0f;
    em->posY = em->prevPosY = gAreaGroundY[em->area];
}

/* Picks the next area of the monster's route: unless its next area (nextArea) is one of the up to eight
 * areas Em_NextAreas lists, the listed area whose centre lies closest in angle to the direction from
 * area to nextArea (skipping prevArea) becomes nextArea. pts[8] and pts[9] are those two
 * centres and ang[8] that direction (the game keeps them in the same arrays). */
void Em_PickRoutePoint(Em *em)
{
    s16 *list;
    s8 i;
    s32 found;
    s16 n;
    s16 best;
    u16 bestd;
    AreaRecord *areaRec;
    f32 pts[10][3];
    u16 ang[9];
    u16 diff[8];
    u16 ids[8];

    if (*(u16 *) &em->destArea == 0xFFFF) {
        *(u16 *) &em->destArea = *(u8 *) &em->nextArea;
    }
    list = Em_NextAreas(em);
    found = 0;
    if (list != NULL) {
        for (i = 0; i < 8; i++) {
            if (em->nextArea == Area_IdForCurrentTime(list[i])) {
                found = 1;
                break;
            }
            if (list[i] == -1) {
                break;
            }
        }
    } else {
        found = 1;
    }
    if (found) {
        return;
    }
    areaRec = Stage_GetAreaRecord(em->area);
    pts[8][0] = areaRec->originX + areaRec->sizeX / 2.0f;
    pts[8][2] = areaRec->originZ + (f32) (areaRec->sizeZ / 2.0f);
    areaRec = Stage_GetAreaRecord(em->nextArea);
    pts[9][0] = areaRec->originX + areaRec->sizeX / 2.0f;
    pts[9][2] = areaRec->originZ + (f32) (areaRec->sizeZ / 2.0f);
    for (i = 0, n = 0; i < 8; i++) {
        if (list[i] == -1) {
            break;
        }
        areaRec = Stage_GetAreaRecord(list[i]);
        ids[i] = list[i];
        pts[i][0] = areaRec->originX + areaRec->sizeX / 2.0f;
        n++;
        pts[i][2] = areaRec->originZ + (f32) (areaRec->sizeZ / 2.0f);
    }
    if (n == 1) {
        em->nextArea = Area_IdForCurrentTime(ids[0]);
        return;
    }
    best = -1;
    ang[8] = Math_AngleXZ(pts[8], pts[9]);
    for (i = 0; i < n; i++) {
        if (em->prevArea != 0xFFFF && em->prevArea == Area_IdForCurrentTime(list[i])) {
            continue;
        }
        ang[i] = Math_AngleXZ(pts[8], pts[i]);
        diff[i] = ang[i] - ang[8];
        if (diff[i] > 0x8000) {
            diff[i] = 0x10000 - diff[i];
        }
        if (best == -1) {
            bestd = diff[i];
            best = i;
        } else if (diff[i] < bestd) {
            best = i;
            bestd = diff[i];
        }
    }
    if (list[best] != -1) {
        em->nextArea = Area_IdForCurrentTime(list[best]);
    }
}

/* A point on the edge of em->area toward em->nextArea, into out[0..2] (an f32 vector, though
 * Blangonga's family declares the parameter EmWork *): the two areas' map positions (Map_GetPosition)
 * relative to em->area's record (Stage_GetAreaRecord: origin [0], [1], size [4], [5]), moved
 * onto the nearer edge along the line between them; out[1] is the area's height (gAreaGroundY).
 * `t = z / ax` is the original's (it divides by ax there, not az). a[4] and a[5] are read at
 * each use (one load each): a local for a[4] takes f3, where the original has the reload of az.
 */
void Em_AreaExitPoint(Em *em, f32 *out)
{
    AreaRecord *areaRec;
    f32 fromX;
    f32 fromZ;
    f32 toX;
    f32 toZ;
    f32 ratioX;
    f32 aspect;
    f32 x;
    f32 ratioZ;
    f32 deltaZ;
    f32 z;
    f32 deltaX;

    areaRec = Stage_GetAreaRecord(em->area);
    Stage_GetAreaRecord(em->nextArea);
    Map_GetPosition(&fromX, &fromZ, em->area);
    Map_GetPosition(&toX, &toZ, em->nextArea);
    deltaX = toX - fromX;
    deltaZ = toZ - fromZ;
    fromX = fromX - areaRec->originX;
    fromZ = fromZ - areaRec->originZ;
    out[0] = deltaX + fromX;
    out[2] = deltaZ + fromZ;
    x = out[0];
    if (!(x <= 0.0f) && x < areaRec->sizeX) {
        z = out[2];
        if (!(z <= 0.0f) && z < areaRec->sizeZ) {
            aspect = areaRec->sizeZ / areaRec->sizeX;
            if (x < fromX) {
                ratioX = x / fromX;
            } else {
                ratioX = (areaRec->sizeX - x) / (areaRec->sizeX - fromX);
            }
            if (z < fromZ) {
                ratioZ = z / fromX;
            } else {
                ratioZ = (areaRec->sizeZ - z) / (areaRec->sizeZ - fromZ);
            }
            ratioZ = ratioZ * aspect;
            if (ratioX <= ratioZ) {
                if (x < fromX) {
                    out[0] = 0.0f;
                } else {
                    x = areaRec->sizeX - x;
                    out[0] = areaRec->sizeX;
                }
                aspect = aspect * (ratioX / ratioZ);
                if (out[2] < fromZ) {
                    out[2] = out[2] - x * aspect;
                } else {
                    out[2] = out[2] + x * aspect;
                }
            } else {
                if (z < fromZ) {
                    out[2] = 0.0f;
                } else {
                    z = areaRec->sizeZ - z;
                    out[2] = areaRec->sizeZ;
                }
                ratioZ = ratioZ / ratioX / aspect;
                if (out[0] < fromX) {
                    out[0] = out[0] - z * ratioZ;
                } else {
                    out[0] = out[0] + z * ratioZ;
                }
            }
        }
    }
    out[1] = gAreaGroundY[em->area];
}

/*
 * For the families' ChangeState functions on the machine that runs the
 * monster: sets targetFound from routedStateArg (command 5's third byte): 0
 * clears it; 1 makes the first player in sense reach (sensedPlayers) the
 * current goal, or clears it with none.
 */
void Em_TargetPlayerInRange(Em *em)
{
    s32 i;
    s32 n;

    switch (em->routedStateArg) {
    case 0:
        em->targetFound = 0;
        break;
    case 1:
        for (i = 0, n = 0; i < gScene.playerCount; i++) {
            if (em->sensedPlayers & (1 << i)) {
                n++;
            }
        }
        if (n == 0) {
            em->targetFound = 0;
            break;
        }
        em->targetFound = 1;
        em->curGoalKind = 1;
        em->curGoalArg = 0;
        for (i = 0; i < gScene.playerCount; i++) {
            if (em->sensedPlayers & (1 << i)) {
                break;
            }
        }
        em->curGoalNo = i;
        break;
    }
}

/*
 * Picks an entry of a weighted table of the script set (table 7, the area
 * choices of EmScript_StepAreaSeq): the count is the u16 at +4, then 8-byte entries
 * with a weight at +4 and an area id at +6. The roll is stateRand & 0x1F:
 * weight 0 is skipped, 255 taken at once, else the first entry whose running
 * total passes the roll. Returns a pointer to its area id.
 */
u8 *EmScript_PickWeighted(Em *em, u8 *script)
{
    u16 n;
    u16 i;
    u8 sum;
    u16 roll;
    u8 w;

    n = *(u16 *) (script + 4);
    sum = 0;
    roll = em->stateRand & 0x1F;
    script += 6;
    for (i = 0; i < n; i++) {
        w = script[4];
        script += 6;
        if (!w) {
            script += 2;
            continue;
        }
        if (w == 255) {
            break;
        }
        sum += w;
        if (roll < sum) {
            break;
        }
        script += 2;
    }
    return script;
}

/* Returns the position of the running script thread. */
u8 *EmScript_GetPos(Em *em)
{
    u8 flags = em->scriptFlags;

    if (flags & 0x20) {
        return em->trigger20Pos;
    }
    if (flags & 0x10) {
        return em->trigger10Pos;
    }
    if (flags & 0x01) {
        return em->loopScriptPos;
    }
    return em->mainScriptPos;
}

/* Stores pos as the position of the running script thread. */
void EmScript_SetPos(Em *em, u8 *pos)
{
    u8 flags = em->scriptFlags;

    if (flags & 0x20) {
        em->trigger20Pos = pos;
    } else if (flags & 0x10) {
        em->trigger10Pos = pos;
    } else if (flags & 0x01) {
        em->loopScriptPos = pos;
    } else {
        em->mainScriptPos = pos;
    }
}

/* Returns loop script `entry` (table 2), or NULL. */
void *EmScript_LoopScript(Em *em, u8 entry)
{
    if (em->script[2] == NULL) {
        return NULL;
    }
    return em->script[2][entry];
}

/* Returns the script of trigger EM_TRIGGER_PART_BUILDUP numbered `entry` (table 3), or NULL. */
void *EmScript_Trigger10Script(Em *em, u8 entry)
{
    if (em->script[3] == NULL) {
        return NULL;
    }
    return em->script[3][entry];
}

/* Returns the script of trigger EM_TRIGGER_NOTICED numbered `entry` (table 4), or NULL. */
void *EmScript_Trigger20Script(Em *em, u8 entry)
{
    if (em->script[4] == NULL) {
        return NULL;
    }
    return em->script[4][entry];
}

/* Returns the script of trigger 2 numbered `entry` (table 8), or NULL. */
void *EmScript_Trigger02Script(Em *em, u8 entry)
{
    if (em->script[8] == NULL) {
        return NULL;
    }
    return em->script[8][entry];
}

/* Returns the script of trigger 4 numbered `entry` (table 10), or NULL. */
void *EmScript_Trigger04Script(Em *em, u8 entry)
{
    if (em->script[10] == NULL) {
        return NULL;
    }
    return em->script[10][entry];
}

/* Returns the script of trigger 8 numbered `entry` (table 11), or NULL. */
void *EmScript_Trigger08Script(Em *em, u8 entry)
{
    if (em->script[11] == NULL) {
        return NULL;
    }
    return em->script[11][entry];
}

/* Returns the script EmScript_Call runs, `entry` of table 9, or NULL. */
void *EmScript_SubScript(Em *em, u8 entry)
{
    if (em->script[9] == NULL) {
        return NULL;
    }
    return em->script[9][entry];
}

/* Returns area sequence `entry`'s list of areas (u16 ids, table 5), or NULL. */
void *EmScript_AreaSeqList(Em *em, u8 entry)
{
    if (em->script[5] == NULL) {
        return NULL;
    }
    return em->script[5][entry];
}

/* Returns area sequence `entry`'s script (table 6); the table is not checked. */
void *EmScript_AreaSeqScript(Em *em, u8 entry)
{
    return em->script[6][entry];
}

/* Returns the loop's point list `entry` (bytes: area point numbers; table 2,
 * the table of the loop scripts), or NULL. */
void *EmScript_LoopPointList(Em *em, u8 entry)
{
    if (em->script[2] == NULL) {
        return NULL;
    }
    return em->script[2][entry];
}

/* Returns route point script `entry` (table 12), or NULL. */
void *EmScript_RoutePointScript(Em *em, u8 entry)
{
    if (em->script[12] == NULL) {
        return NULL;
    }
    return em->script[12][entry];
}

/* Returns the script of trigger EM_TRIGGER_PLAYER_AREA (table 13 holds one), or NULL. */
void *EmScript_Trigger80Script(Em *em)
{
    if (em->script[13] == NULL) {
        return NULL;
    }
    return *em->script[13];
}

/* Returns the script of trigger EM_TRIGGER_BLOCK_AREA (table 14 holds one), or NULL. */
void *EmScript_Trigger40Script(Em *em)
{
    if (em->script[14] == NULL) {
        return NULL;
    }
    return *em->script[14];
}

/*
 * Starts the script of the most urgent pending trigger (Em.triggers): 0x40
 * and 0x80 replace the running script thread outright, the others (0x20,
 * 0x10, 8, 4, 2, in that order) are added to the running threads in
 * Em.scriptFlags. A trigger whose thread already runs (Em_ScriptFlagsFrom)
 * waits. Returns the script started first, or NULL; it becomes Em.scriptPos.
 */
u8 *EmScript_PickTrigger(Em *em)
{
    void *script;
    s32 pending;

    pending = em->triggers;
    if (pending == 0) {
        return 0;
    }
    script = NULL;
    if (((u8) pending & 0x40) && Em_ScriptFlagsFrom(em, 0x40) == 0) {
        script = EmScript_Trigger40Script(em);
        em->triggers = 0;
        em->scriptFlags = 0x40;
    }
    if ((em->triggers & 0x80) && Em_ScriptFlagsFrom(em, 0x80) == 0 && script == NULL) {
        script = EmScript_Trigger80Script(em);
        em->triggers = 0;
        em->scriptFlags = 0x80;
    }
    if ((em->triggers & 0x20) && Em_ScriptFlagsFrom(em, 0x20) == 0) {
        em->trigger20Pos = EmScript_Trigger20Script(em, 0);
        if (script == NULL) {
            script = em->trigger20Pos;
        }
        em->triggers &= 0xCE;
        em->scriptFlags |= 0x20;
    }
    if ((em->triggers & 0x10) && Em_ScriptFlagsFrom(em, 0x10) == 0) {
        em->trigger10Pos = EmScript_Trigger10Script(em, 0);
        if (script == NULL) {
            script = em->trigger10Pos;
        }
        em->triggers &= 0xEF;
        em->scriptFlags |= 0x10;
    }
    if ((em->triggers & 8) && Em_ScriptFlagsFrom(em, 8) == 0) {
        em->trigger08Pos = EmScript_Trigger08Script(em, 0);
        if (script == NULL) {
            script = em->trigger08Pos;
        }
        em->triggers &= 0xF7;
        em->scriptFlags |= 8;
    }
    if ((em->triggers & 4) && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->trigger04Pos = EmScript_Trigger04Script(em, 0);
        if (script == NULL) {
            script = em->trigger04Pos;
        }
        em->triggers &= 0xC8;
        em->scriptFlags |= 4;
    }
    if ((em->triggers & 2) && Em_ScriptFlagsFrom(em, 2) == 0) {
        em->trigger02Pos = EmScript_Trigger02Script(em, 0);
        if (script == NULL) {
            script = em->trigger02Pos;
        }
        em->triggers &= 0xCC;
        em->scriptFlags |= 2;
    }
    if (script != NULL) {
        em->scriptPos = script;
    }
    return script;
}

/* Restarts the monster's script from main script 0 when Em.scriptRestart is
 * set: the threads, the loop, the area sequence and the destination area are
 * cleared. */
void EmScript_Restart(Em *em)
{
    if (em->scriptRestart != 0) {
        em->scriptPending = 0;
        em->scriptRestart = 0;
        em->scriptFlags = 0;
        em->scriptDelay = 0;
        em->scriptNo = 0;
        em->mainScriptPos = (u8 *) Em_StartScript(em);
        em->loopPoint = -1;
        em->loopCount = -1;
        em->areaSeqPos = -1;
        em->areaSeqLen = -1;
        *(u16 *) &em->destArea = 65535;
        em->prevArea = 65535;
        em->unk7A8 = 0;
    }
}

/*
 * Sets the monster's combat mode (Em.combat) to `mode`, for `frames` frames
 * when frames > 0 (Em.combatTimer). The trigger threads below 0x40 stop.
 * Leaving combat (mode 0) clears what it had noticed of the players, and
 * the players it fought leave the battle. Returns 1 when the mode changed,
 * 0 when it was already set.
 */
s32 Em_SetCombat(Em *em, u8 mode, s16 frames)
{
    s8 i;

    if (em->combat == mode) {
        return 0;
    }
    if (frames > 0) {
        em->combatTimer = frames;
    }
    em->combat = mode;
    em->scriptFlags &= 0xC0;
    if (em->combat == 0) {
        em->seen = 0;
        em->noticed = 0;
        for (i = 0; i < 4; i++) {
            em->alertBuildup[i] = 0;
            em->noticeCount[i] = 0;
            em->noticeTimer[i] = 0;
            if ((em->combatPlayers & (1 << i)) && gPlayers[i].active != 0 && gPlayers[i].unk73A_combat != 0) {
                gPlayers[i].unk73A_combat = 0;
            }
        }
        em->combatPlayers = 0;
    }
    return 1;
}

/* 1 if the monster may go to area `area`: its own area, or one of the list of its kind for the
 * current quest row (ids from 10000 on name an area, the others a day id), ending with 0xFFFF. */
s32 Em_CanEnterArea(Em *em, u16 area)
{
    u16 **kindAreas = gEmEnterAreaLists[em->type];
    u16 *entry;

    if (em->area == area) {
        return 1;
    }
    if (kindAreas == NULL) {
        return 0;
    }
    kindAreas += Map_DayId(gScene.map);
    entry = *kindAreas;
    if (entry == NULL) {
        return 0;
    }
    while (*entry != 0xFFFF) {
        if (*entry >= 10000) {
            if (*entry - 10000 == area) {
                return 1;
            }
        } else if (Area_DayId(*entry) == Area_DayId(area)) {
            return 1;
        }
        entry++;
    }
    return 0;
}

/* The notice counts EmScript_IfScriptPlayerNoticeOver compares with. */
s32 gEmNoticeThresholds[3] = {
#include "game/008C3E38.inc"
};
