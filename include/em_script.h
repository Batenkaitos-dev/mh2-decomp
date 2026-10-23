#ifndef EM_SCRIPT_H
#define EM_SCRIPT_H
#include "em.h"

/*
 * The monster script's commands (Em.scriptPos). A script is a byte stream:
 * each command is its op byte, then a sub-op byte for the commands with
 * blocks (0: the test or the header; then the markers of the else, the
 * later cases, the default and the end, which differ by command), then the
 * command's own bytes. EmScript_Run reads the op and calls its handler,
 * named in each entry; the If and Switch commands skip the blocks they
 * don't run with EmScript_SkipBlock and EmScript_SkipToEnd, which take the
 * op to look for.
 */
enum EmScriptOp {
    EMOP_IF_SENSED = 0x01,                          /* EmScript_IfSensed */
    EMOP_IF_NOTICED = 0x02,                         /* EmScript_IfNoticed */
    EMOP_IF_CAN_ENTER_NEXT_AREA = 0x03,             /* EmScript_IfCanEnterNextArea */
    EMOP_RESTART = 0x04,                            /* Em_StartScript */
    EMOP_CHANGE_STATE = 0x05,                       /* Em_ChangeStateRouted */
    EMOP_SET_GOAL = 0x06,                           /* EmScript_SetGoal */
    EMOP_START = 0x07,                              /* EmScript_Start */
    EMOP_IF_ON_GROUND = 0x08,                       /* EmScript_IfOnGround */
    EMOP_IF_IN_AIR = 0x09,                          /* EmScript_IfInAir */
    EMOP_SET_HIT_REACT_MODE = 0x0A,                 /* EmScript_SetHitReactMode */
    EMOP_IF_COMBAT_IS = 0x0B,                       /* EmScript_IfCombatIs */
    EMOP_SET_VAR = 0x0C,                            /* EmScript_SetVar */
    EMOP_CLEAR_VAR = 0x0D,                          /* EmScript_ClearVar */
    EMOP_IF_IN_AREA = 0x0E,                         /* EmScript_IfInArea */
    EMOP_START_LOOP = 0x0F,                         /* EmScript_StartLoop */
    EMOP_NEXT_LOOP_POINT = 0x10,                    /* EmScript_NextLoopPoint */
    EMOP_GOAL_ALERTED_PLAYER = 0x11,                /* EmScript_GoalAlertedPlayer */
    EMOP_GOAL_NOTICED_PLAYER = 0x12,                /* EmScript_GoalNoticedPlayer */
    EMOP_GOAL_SCRIPT_PLAYER = 0x13,                 /* EmScript_GoalScriptPlayer */
    EMOP_IF_TARGET_OFF_HEADING = 0x14,              /* EmScript_IfTargetOffHeading */
    EMOP_SWITCH_AREA = 0x15,                        /* EmScript_SwitchArea */
    EMOP_CALL = 0x16,                               /* EmScript_Call */
    EMOP_SET_AREA_SEQ = 0x17,                       /* EmScript_SetAreaSeq */
    EMOP_STEP_AREA_SEQ = 0x18,                      /* EmScript_StepAreaSeq */
    EMOP_GOAL_DEST_AREA = 0x19,                     /* EmScript_GoalDestArea */
    EMOP_GOAL_AREA_TOWARD = 0x1A,                   /* EmScript_GoalAreaToward */
    EMOP_IF_REQUESTED_IS = 0x1B,                    /* EmScript_IfRequestedIs */
    EMOP_SWITCH_HP_PERCENT = 0x1C,                  /* EmScript_SwitchHpPercent */
    EMOP_SWITCH_REQUEST_ID = 0x1D,                  /* EmScript_SwitchRequestId */
    EMOP_SET_REQUEST = 0x1E,                        /* EmScript_SetRequest */
    EMOP_IF_STANCE_FAMILY = 0x1F,                   /* EmScript_IfStanceFamily */
    EMOP_SWITCH_TARGET_ANGLE = 0x20,                /* EmScript_SwitchTargetAngle */
    EMOP_IF_NEED2_LOW = 0x21,                       /* EmScript_IfNeed2Low */
    EMOP_IF_GOAL_NEAR_XZ = 0x22,                    /* EmScript_IfGoalNearXZ */
    EMOP_SWITCH_KIND_IN_AREA = 0x23,                /* EmScript_SwitchKindInArea */
    EMOP_REPEAT = 0x24,                             /* EmScript_Repeat */
    EMOP_STOP_REPEAT = 0x25,                        /* EmScript_StopRepeat */
    EMOP_SET_NO_ALERT = 0x26,                       /* EmScript_SetNoAlert */
    EMOP_SWITCH_VARIANT = 0x27,                     /* EmScript_SwitchVariant */
    EMOP_IF_PLAYER_IN_AREA = 0x28,                  /* EmScript_IfPlayerInArea */
    EMOP_IF_AREA_TIMER1_OVER = 0x29,                /* EmScript_IfAreaTimer1Over */
    EMOP_IF_AREA_TIMER2_OVER = 0x2A,                /* EmScript_IfAreaTimer2Over */
    EMOP_IF_VAR_IS = 0x2B,                          /* EmScript_IfVarIs */
    EMOP_SWITCH_KIND_GROUP = 0x2C,                  /* EmScript_SwitchKindGroup */
    EMOP_GOAL_ACTION_GOAL = 0x2D,                   /* EmScript_GoalActionGoal */
    EMOP_SELECT_SIGHT_ROW = 0x2E,                   /* EmScript_SelectSightRow */
    EMOP_IF_PLAYER_CARRYING = 0x2F,                 /* EmScript_IfPlayerCarrying */
    EMOP_SET_NEED4 = 0x30,                          /* EmScript_SetNeed4 */
    EMOP_SET_GOAL_KIND = 0x31,                      /* EmScript_SetGoalKind */
    EMOP_IF_HIT_REACT_MODE = 0x32,                  /* EmScript_IfHitReactMode */
    EMOP_SWITCH_HIT_REACT_MODE = 0x33,              /* EmScript_SwitchHitReactMode */
    EMOP_IF_STATE = 0x34,                           /* EmScript_IfState */
    EMOP_IF_RAGING = 0x35,                          /* EmScript_IfRaging */
    EMOP_IF_GOAL_NEAR = 0x36,                       /* EmScript_IfGoalNear */
    EMOP_IF_DRILL = 0x37,                           /* EmScript_IfDrill */
    EMOP_IF_ON_PASS_THROUGH_FLOOR = 0x38,           /* EmScript_IfOnPassThroughFloor */
    EMOP_IF_BLIND = 0x39,                           /* EmScript_IfBlind */
    EMOP_IF_NO_SENSE_DELAY = 0x3A,                  /* EmScript_IfNoSenseDelay */
    EMOP_IF_LINKED = 0x3B,                          /* EmScript_IfLinked */
    EMOP_IF_LINK_COMBAT_SAME_AREA = 0x3C,           /* EmScript_IfLinkCombatSameArea */
    EMOP_IF_PREV_AREA = 0x3D,                       /* EmScript_IfPrevArea */
    EMOP_SWITCH_PREV_AREA = 0x3E,                   /* EmScript_SwitchPrevArea */
    EMOP_START_ROUTE = 0x3F,                        /* EmScript_StartRoute */
    EMOP_SET_COMBAT = 0x40,                         /* EmScript_SetCombat */
    EMOP_ADD_HP = 0x41,                             /* EmScript_AddHp */
    EMOP_IF_GOAL_ANGLE_OVER = 0x42,                 /* EmScript_IfGoalAngleOver */
    EMOP_IF_LINK_SAME_AREA = 0x44,                  /* EmScript_IfLinkSameArea */
    EMOP_IF_LONE_PLAYER_BAIT = 0x45,                /* EmScript_IfLonePlayerBait */
    EMOP_IF_SCRIPT_PLAYER_STATE = 0x46,             /* EmScript_IfScriptPlayerState */
    EMOP_IF_HOOKED = 0x47,                          /* EmScript_IfHooked */
    EMOP_SET_DELAY = 0x48,                          /* EmScript_SetDelay */
    EMOP_GOAL_PLAYER_FLOOR_SPOT = 0x49,             /* EmScript_GoalPlayerFloorSpot */
    EMOP_IF_SCRIPT_PLAYER_SEEN = 0x4A,              /* EmScript_IfScriptPlayerSeen */
    EMOP_SET_ALERT_BUILDUP = 0x4B,                  /* EmScript_SetAlertBuildup */
    EMOP_SET_NOTICE_COUNT = 0x4C,                   /* EmScript_SetNoticeCount */
    EMOP_COMPUTE_GOAL = 0x4D,                       /* EmScript_ComputeGoal */
    EMOP_ADD_NEED2 = 0x4E,                          /* EmScript_AddNeed2 */
    EMOP_ADD_NEED1 = 0x4F,                          /* EmScript_AddNeed1 */
    EMOP_ADD_NEED3 = 0x50,                          /* EmScript_AddNeed3 */
    EMOP_IF_STANCE_UNDER = 0x51,                    /* EmScript_IfStanceUnder */
    EMOP_PICK_NOTICED_PLAYER_REACHABLE = 0x52,      /* EmScript_PickNoticedPlayerReachable */
    EMOP_PICK_NOTICED_PLAYER_IN_AREA = 0x53,        /* EmScript_PickNoticedPlayerInArea */
    EMOP_IF_SCRIPT_PLAYER_IN_AREA = 0x54,           /* EmScript_IfScriptPlayerInArea */
    EMOP_IF_SCRIPT_PLAYER_NOTICE_HIGH = 0x55,       /* EmScript_IfScriptPlayerNoticeHigh */
    EMOP_IF_QUEST_ID = 0x56,                        /* EmScript_IfQuestId */
    EMOP_SWITCH_QUEST_KEY = 0x57,                   /* EmScript_SwitchQuestKey */
    EMOP_GOAL_LINK_TARGET = 0x58,                   /* EmScript_GoalLinkTarget */
    EMOP_IF_ON_CEILING = 0x59,                      /* EmScript_IfOnCeiling */
    EMOP_IF_SCRIPT_PLAYER_FLOOR_ATTR = 0x5A,        /* EmScript_IfScriptPlayerFloorAttr */
    EMOP_SET_NOTICE_TIMER = 0x5B,                   /* EmScript_SetNoticeTimer */
    EMOP_IF_CEILING_ABOVE = 0x5C,                   /* EmScript_IfCeilingAbove */
    EMOP_IF_GOAL_VALID = 0x5D,                      /* EmScript_IfGoalValid */
    EMOP_IF_FLOOR_ATTR = 0x5E,                      /* EmScript_IfFloorAttr */
    EMOP_PICK_NOTICED_PLAYER_SAME_FLOOR = 0x5F,     /* EmScript_PickNoticedPlayerSameFloor */
    EMOP_IF_GATE_OPEN = 0x60,                       /* EmScript_IfGateOpen */
    EMOP_SET_AREA_TIMER2 = 0x61,                    /* EmScript_SetAreaTimer2 */
    EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR = 0x62,    /* EmScript_SwitchScriptPlayerFloorAttr */
    EMOP_IF_VARIANT_BITS = 0x63,                    /* EmScript_IfVariantBits */
    EMOP_IF_SCRIPT_PLAYER_NOTICE_OVER = 0x64,       /* EmScript_IfScriptPlayerNoticeOver */
    EMOP_CLEAR_NOTICE_COUNTS = 0x65,                /* EmScript_ClearNoticeCounts */
    EMOP_IF_ANY_PLAYER_RIDING = 0x66,               /* EmScript_IfAnyPlayerRiding */
    EMOP_IF_NOT_REMOTE = 0x67,                      /* EmScript_IfNotRemote */
    EMOP_RESTART_MAIN = 0x68,                       /* EmScript_RestartMain */
    EMOP_IF_GOAL_ANGLE_OVER_CAPPED = 0x69,          /* EmScript_IfGoalAngleOverCapped */
    EMOP_SWITCH_TYPE = 0x70,                        /* EmScript_SwitchType */
    EMOP_IF_LEVEL_AT_LEAST = 0x71,                  /* EmScript_IfLevelAtLeast */
    EMOP_IF_VERSUS = 0x72,                          /* EmScript_IfVersus */
    EMOP_SWITCH_ITEM_USE_KIND = 0x73,               /* EmScript_SwitchItemUseKind */
    EMOP_IF_VERSUS_ITEM_USED = 0x74,                /* EmScript_IfVersusItemUsed */
    EMOP_SWITCH_SPAWN_PARAM = 0x75,                 /* EmScript_SwitchSpawnParam */
    EMOP_SWITCH_SEASON = 0x76,                      /* EmScript_SwitchSeason */
    EMOP_IF_DAY = 0x77,                             /* EmScript_IfDay */
    EMOP_IF_GOAL_PLAYER_IN_ARC = 0x78,              /* EmScript_IfGoalPlayerInArc */
    EMOP_SWITCH_KIND_QUERY = 0x79,                  /* EmScript_SwitchKindQuery */
    EMOP_SWITCH_BASE_AREA = 0x7A,                   /* EmScript_SwitchBaseArea */
    EMOP_ADVANCE_STATE_RAND_ALT = 0x7B,             /* EmScript_AdvanceStateRand */
    EMOP_IF_IN_ARENA = 0x7C,                        /* EmScript_IfInArena */
    EMOP_RANDOM_BRANCH = 0x80,                      /* EmScript_RandomBranch */
    EMOP_CALL_SCRIPT = 0x81,                        /* EmScript_CallScript */
    EMOP_JUMP_ENTRY = 0x82,                         /* EmScript_JumpEntry */
    EMOP_SWITCH_TARGET_DIST = 0x83,                 /* EmScript_SwitchTargetDist */
    EMOP_ADVANCE_STATE_RAND = 0x84,                 /* EmScript_AdvanceStateRand */
    EMOP_PLACE_AT_POINT = 0x90,                     /* EmScript_PlaceAtPoint */
    EMOP_FACE_POINT = 0x91,                         /* EmScript_FacePoint */
    EMOP_NOP = 0x92,                                /* EmScript_Nop */
    EMOP_NOP_ALT = 0x93,                            /* EmScript_NopAlt */
    EMOP_SWITCH_SCRIPT_VALUE = 0x94,                /* EmScript_SwitchScriptValue */
    EMOP_SET_HEADING = 0x99,                        /* EmScript_SetHeading */
    EMOP_END = 0xFF                                 /* EmScript_End */
};

/*
 * The sub-op byte of a command with blocks: which part of the block the
 * command marks. EmScript_SkipBlock steps over nested blocks of the same op
 * and stops at the next marker; its last argument is the block's end marker.
 * The If commands (EMOP_IF_*) have a test, an else and an end; the Switch
 * commands (EMOP_SWITCH_*) a header with the case count, then the later
 * cases (each with its value), a default and an end; EMOP_RANDOM_BRANCH a
 * header, then the branches, and an end; EMOP_REPEAT a header with the
 * count and an end.
 */
#define EMBLK_OPEN 0            /* the test, the switch's header or the branches' header; its first block follows */
#define EMBLK_ELSE 1            /* an If's else block */
#define EMBLK_END_IF 2          /* an If's end */
#define EMBLK_CASE 1            /* a Switch's next case, its value after the marker */
#define EMBLK_DEFAULT 2         /* a Switch's default block */
#define EMBLK_END_SWITCH 3      /* a Switch's end */
#define EMBLK_END_BRANCH 0xFF   /* EMOP_RANDOM_BRANCH's end */
#define EMBLK_END_REPEAT 1      /* EMOP_REPEAT's end: counts a pass down and jumps back */

/*
 * EMOP_END's sub-byte: what the command ends (EmScript_End). The thread ends
 * clear their EmTrigger bit of Em.scriptFlags, then go back to the loop
 * script when one runs (scriptFlags 0x01), else restart the main script.
 * 0xF5 and 0xF6 restart the main script as EMEND_MAIN does, 0xF7 also forgets
 * the script player's notice, 0xFC also resets the combat gauge: which
 * scripts end with them isn't known.
 */
enum EmScriptEnd {
    EMEND_MAIN = 0x00,          /* the main script: restarts it, scriptFlags cleared */
    EMEND_CALL = 0x01,          /* returns from EMOP_CALL_SCRIPT (scriptCallReturn) */
    EMEND_JUMP = 0x02,          /* returns from EMOP_JUMP_ENTRY (scriptJumpReturn) */
    EMEND_SUBSCRIPT = 0x03,     /* returns from EMOP_CALL (subScriptReturn) */
    EMEND_ROUTE_ENTER = 0x04,   /* steps along the route, moving a large monster into the route's area */
    EMEND_ROUTE_STEP = 0x05,    /* steps along the route of EmScript_StartRoute */
    EMEND_RAGE = 0xF8,          /* the EM_TRIGGER_RAGE thread */
    EMEND_SENSED = 0xF9,        /* the EM_TRIGGER_SENSED thread */
    EMEND_AREA_OBJ = 0xFA,      /* the EM_TRIGGER_AREA_OBJ thread */
    EMEND_AREA_SEQ_STEP = 0xFB, /* a step of the area sequence */
    EMEND_PART_BUILDUP = 0xFD,  /* the EM_TRIGGER_PART_BUILDUP thread */
    EMEND_LOOP_PASS = 0xFE,     /* a pass of the loop script */
    EMEND_LOOP = 0xFF           /* the loop script: repeats it for loopCount passes */
};

void EmScript_Init(Em *em);
void EmScript_Run(Em *em);
void EmScript_AdvanceStateRand(Em *em);
u8 *EmScript_IfSensed(Em *em, u8 *script);
u8 *EmScript_IfNoticed(Em *em, u8 *script);
u8 *EmScript_IfCanEnterNextArea(Em *em, u8 *script);
s32 EmScript_Start(Em *em, u8 *script);
u8 *EmScript_IfOnGround(Em *em, u8 *script);
u8 *EmScript_IfStanceFamily(Em *em, u8 *script);
u8 *EmScript_IfInAir(Em *em, u8 *script);
u8 *EmScript_SetHitReactMode(Em *em, u8 *script);
u8 *EmScript_IfCombatIs(Em *em, u8 *script);
u8 *EmScript_SetVar(Em *em, u8 *script);
u8 *EmScript_ClearVar(Em *em, u8 *script);
u8 *EmScript_IfInArea(Em *em, u8 *script);
u8 *EmScript_StartLoop(Em *em, u8 *script);
u8 *EmScript_NextLoopPoint(Em *em, u8 *script);
u8 *EmScript_GoalAlertedPlayer(Em *em, u8 *script);
u8 *EmScript_GoalNoticedPlayer(Em *em, u8 *script);
u8 *EmScript_GoalScriptPlayer(Em *em, u8 *script);
u8 *EmScript_IfTargetOffHeading(Em *em, u8 *script);
u8 *EmScript_SwitchArea(Em *em, u8 *script);
u8 *EmScript_Call(Em *em, u8 *script);
u8 *EmScript_SetAreaSeq(Em *em, u8 *script);
u8 *EmScript_StepAreaSeq(Em *em, u8 *pos);
u8 *EmScript_GoalDestArea(Em *em, u8 *script);
u8 *EmScript_GoalAreaToward(Em *em, u8 *script);
u8 *EmScript_IfRequestedIs(Em *em, u8 *script);
u8 *EmScript_SwitchHpPercent(Em *em, u8 *script);
u8 *EmScript_SwitchRequestId(Em *em, u8 *script);
u8 *EmScript_SetRequest(Em *em, u8 *script);
u8 *EmScript_SwitchTargetAngle(Em *em, u8 *script);
u8 *EmScript_IfNeed2Low(Em *em, u8 *script);
u8 *EmScript_IfGoalNearXZ(Em *em, u8 *script);
u8 *EmScript_IfGoalNear(Em *em, u8 *script);
u8 *EmScript_SwitchKindInArea(Em *em, u8 *script);
u8 *EmScript_Repeat(Em *em, u8 *script);
u8 *EmScript_StopRepeat(Em *em, u8 *script);
u8 *EmScript_SetNoAlert(Em *em, u8 *script);
u8 *EmScript_SwitchVariant(Em *em, u8 *script);
u8 *EmScript_SwitchSpawnParam(Em *em, u8 *script);
u8 *EmScript_SwitchSeason(Em *em, u8 *script);
u8 *EmScript_SwitchBaseArea(Em *em, u8 *script);
u8 *EmScript_SwitchKindQuery(Em *em, u8 *script);
u8 *EmScript_IfPlayerInArea(Em *em, u8 *script);
u8 *EmScript_IfAreaTimer1Over(Em *em, u8 *script);
u8 *EmScript_IfAreaTimer2Over(Em *em, u8 *script);
u8 *EmScript_IfVarIs(Em *em, u8 *script);
u8 *EmScript_SwitchKindGroup(Em *em, u8 *script);
u8 *EmScript_SwitchType(Em *em, u8 *script);
u8 *EmScript_GoalActionGoal(Em *em, u8 *script);
u8 *EmScript_SelectSightRow(Em *em, u8 *script);
u8 *EmScript_IfPlayerCarrying(Em *em, u8 *script);
u8 *EmScript_SetNeed4(Em *em, u8 *script);
u8 *EmScript_SetGoalKind(Em *em, u8 *script);
u8 *EmScript_IfHitReactMode(Em *em, u8 *script);
u8 *EmScript_SwitchHitReactMode(Em *em, u8 *script);
u8 *EmScript_IfState(Em *em, u8 *script);
u8 *EmScript_IfRaging(Em *em, u8 *script);
u8 *EmScript_IfDrill(Em *em, u8 *script);
u8 *EmScript_IfOnPassThroughFloor(Em *em, u8 *script);
u8 *EmScript_IfBlind(Em *em, u8 *script);
u8 *EmScript_IfNoSenseDelay(Em *em, u8 *script);
u8 *EmScript_IfLinked(Em *em, u8 *script);
u8 *EmScript_IfLinkCombatSameArea(Em *em, u8 *script);
u8 *EmScript_IfPrevArea(Em *em, u8 *script);
u8 *EmScript_SwitchPrevArea(Em *em, u8 *script);
u8 *EmScript_StartRoute(Em *em, u8 *script);
u8 *EmScript_SetCombat(Em *em, u8 *script);
u8 *EmScript_AddHp(Em *em, u8 *script);
u8 *EmScript_IfGoalAngleOver(Em *em, u8 *script);
u8 *EmScript_IfGoalAngleOverCapped(Em *em, u8 *script);
u8 *EmScript_IfLinkSameArea(Em *em, u8 *script);
u8 *EmScript_IfLonePlayerBait(Em *em, u8 *script);
u8 *EmScript_IfScriptPlayerState(Em *em, u8 *script);
u8 *EmScript_IfHooked(Em *em, u8 *script);
u8 *EmScript_SetDelay(Em *em, u8 *script);
u8 *EmScript_GoalPlayerFloorSpot(Em *em, u8 *script);
u8 *EmScript_IfScriptPlayerSeen(Em *em, u8 *script);
u8 *EmScript_SetAlertBuildup(Em *em, u8 *script);
u8 *EmScript_SetNoticeCount(Em *em, u8 *script);
u8 *EmScript_ComputeGoal(Em *em, u8 *script);
u8 *EmScript_AddNeed2(Em *em, u8 *script);
u8 *EmScript_AddNeed1(Em *em, u8 *script);
u8 *EmScript_AddNeed3(Em *em, u8 *script);
u8 *EmScript_IfStanceUnder(Em *em, u8 *script);
u8 *EmScript_PickNoticedPlayerReachable(Em *em, u8 *script);
u8 *EmScript_PickNoticedPlayerInArea(Em *em, u8 *script);
u8 *EmScript_IfScriptPlayerInArea(Em *em, u8 *script);
u8 *EmScript_IfScriptPlayerNoticeHigh(Em *em, u8 *script);
u8 *EmScript_IfQuestId(Em *em, u8 *script);
u8 *EmScript_SwitchQuestKey(Em *em, u8 *script);
u8 *EmScript_GoalLinkTarget(Em *em, u8 *script);
u8 *EmScript_IfOnCeiling(Em *em, u8 *script);
u8 *EmScript_IfScriptPlayerFloorAttr(Em *em, u8 *script);
u8 *EmScript_SetNoticeTimer(Em *em, u8 *script);
u8 *EmScript_IfCeilingAbove(Em *em, u8 *script);
u8 *EmScript_IfGoalValid(Em *em, u8 *script);
u8 *EmScript_IfFloorAttr(Em *em, u8 *script);
u8 *EmScript_PickNoticedPlayerSameFloor(Em *em, u8 *script);
u8 *EmScript_IfGateOpen(Em *em, u8 *script);
u8 *EmScript_SetAreaTimer2(Em *em, u8 *script);
u8 *EmScript_SwitchScriptPlayerFloorAttr(Em *em, u8 *script);
u8 *EmScript_IfVariantBits(Em *em, u8 *script);
u8 *EmScript_IfDay(Em *em, u8 *script);
u8 *EmScript_IfInArena(Em *em, u8 *script);
u8 *EmScript_IfGoalPlayerInArc(Em *em, u8 *script);
u8 *EmScript_IfScriptPlayerNoticeOver(Em *em, u8 *script);
u8 *EmScript_ClearNoticeCounts(Em *em, u8 *script);
u8 *EmScript_IfAnyPlayerRiding(Em *em, u8 *script);
u8 *EmScript_IfNotRemote(Em *em, u8 *script);
s32 EmScript_RestartMain(Em *em, s32 script);
u8 *EmScript_RandomBranch(Em *em, u8 *script);
u8 *EmScript_CallScript(Em *em, u8 *pos);
u8 *EmScript_JumpEntry(Em *em, u8 *pos);
u8 *EmScript_SetGoal(Em *em, u8 *script);
u8 *EmScript_SwitchTargetDist(Em *em, u8 *script);
u8 *EmScript_End(Em *em, u8 *script);
u8 *EmScript_PlaceAtPoint(Em *em, u8 *script);
void *EmScript_FacePoint(Em *em, u8 *script);
void *EmScript_SetHeading(Em *em, u8 *script);
s32 EmScript_Nop(s32 unused, s32 script);
s32 EmScript_NopAlt(s32 unused, s32 script);
u8 *EmScript_SwitchScriptValue(Em *em, u8 *script);
u8 *EmScript_IfLevelAtLeast(Em *em, u8 *script);
u8 *EmScript_IfVersus(Em *em, u8 *script);
u8 *EmScript_SwitchItemUseKind(Em *em, u8 *script);
u8 *EmScript_IfVersusItemUsed(Em *em, u8 *script);
s32 Em_StartScript(Em *em);
u8 *EmScript_SkipBlock(Em *em, u8 *script, u8 opcode, u8 end);
u8 *EmScript_NextCmd(Em *em, u8 *script);
u8 *EmScript_SkipToEnd(Em *em, u8 *script, u8 opcode);
void Em_RestartScript(Em *em);
void Em_EnterNextArea(Em *em);
void Em_PickRoutePoint(Em *em);
void Em_AreaExitPoint(Em *em, f32 *out);
void Em_TargetPlayerInRange(Em *em);
u8 *EmScript_PickWeighted(Em *em, u8 *script);
u8 *EmScript_GetPos(Em *em);
void EmScript_SetPos(Em *em, u8 *pos);
void *EmScript_LoopScript(Em *em, u8 entry);
void *EmScript_SubScript(Em *em, u8 entry);
void *EmScript_AreaSeqList(Em *em, u8 entry);
void *EmScript_AreaSeqScript(Em *em, u8 entry);
void *EmScript_LoopPointList(Em *em, u8 entry);
void *EmScript_RoutePointScript(Em *em, u8 entry);
u8 *EmScript_PickTrigger(Em *em);
void EmScript_Restart(Em *em);
s32 Em_SetCombat(Em *em, u8 mode, s16 frames);
s32 Em_CanEnterArea(Em *em, u16 area);

#endif /* EM_SCRIPT_H */
