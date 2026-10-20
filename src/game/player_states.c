/*
 * The player's states, actions and per-frame update (Player_Update), the
 * scene events, and the monsters' helpers: turning, status, damage, routes
 * and the script interpreter (EmScript_*). One of Capcom's units, in three
 * parts: the player's states, the part from Player_StateBedLieDown on, and
 * the part from Player_RunGestureState on, which Capcom's build compiled at
 * a different optimization level (4) from the rest.
 * Player_PickStatusState is static: its one caller, Player_Update, keeps its
 * first argument in the same register across the call, which the compiler
 * does only when the callee is static.
 */
#include "common.h"
#include "stageobj.h"
#include "msg_event.h"
#include "sprite.h"
#include "syssettings.h"
#include "scene.h"
#include "quest.h"
#include "scevent.h"
#include "em.h"
#include "em_script.h"
#include "pad.h"
#include "player.h"
#include "item.h"
#include "area.h"
#include "objattach.h"
#include "qsync.h"
#include "chat_log.h"
/* Declarations of the functions called below. */
s32 ScEvent_LoadDemoCams(ScEventList *h);
s32 ScEvent_LoadCamera(ScEventList *h);
s32 ScEvent_Set(ScEvent *e, ScEventHead *rec);
s32 Player_TakeOfferedItem(Player *p);
s32 Quest_IsVersus(void);
s32 SpecialMine_IsFree(Player *unused0);

/* The walls each player touches (WallContact): Player_TurnAlongWall and
 * Player_TurnAlongWallSide turn the player a quarter turn off the first one. */
extern WallContact gEmWallContacts[][WALL_CONTACTS];

/* prototypes */
s16 Pouch_CursorCount(Player *p);
s32 Item_RideItemUsable(void);
s32 Item_UsableOutsideCamp(void);
s32 Item_CanUseSmallBarrelBomb(void);
s32 Scene_InNoBarrelBombArea(void);
s32 Scene_CanPlaceKind3(void);
s32 Item_CanPlaceLargeBarrelBomb(void);
s32 Item_CanPlaceKind1(void);
s32 Item_CanPlaceKind2(void);
s16 Player_GetAreaPitModel(Player *p);
s32 Pouch_HasMeat(Player *p);
s32 Player_LocalCanUseBinoculars(void);

/* Prototypes shared by the parts of this unit. */
void Effect06_SpawnAt(void *obj, s32 type, s32 bone, f32 *pos, f32 scale); /* unit's own declaration: type and bone declared s32 as the callers pass them; with the definition's s16 type and u8 bone the calls compile differently (Em_UpdateStatus most of all) */
u8 *EmScript_NextCmd(Em *em, u8 *p);
s32 Em_StartScript(Em *em);
void Em_StartTurn(Em *em, EmTurnParams *t, s32 back);
s32 Obj_InCurrentArea(void *obj);
s32 Obj_SameArea(void *obj, Player *b);
void Player_ClearFlag(void *obj, u32 flag);
void Player_FaceStick(Player *em);
s32 Player_InSceneEvent(void);
void Player_SetFlag(void *obj, u32 flag);
void Player_StartGesture(Player *p);
void Player_StateMove(Player *em, s32 variant);
void Player_StateWallHold(Player *em, s32 arg1);
void Player_StateWallClimbUp(Player *player, s32 arg1);
u32 Player_TestFlag(void *obj, u32 flag);
void Player_HoldSpecialInput(Player *em, u8 mode);
void Player_UpdateMotionTime(Player *p, s16 step);
u8 Pouch_Add(Player *p, u16 item, s16 n);
AreaRecord *Stage_GetAreaRecord(s32 area); /* unit's own declaration: the area declared s32; with the definition's u16 the callers would mask it to 16 bits first (EmScript_GoalAreaToward, Em_PickRoutePoint), which the game's code doesn't */
s32 Player_TurnAlongWallSide(Player *em, s16 side);
s32 Player_Status6Active(Player *pl);
void QSync_SendPlayer(Player *p, s32 kind, u16 mode);
extern f32 gAreaGroundY[];
s32 Player_Status5Full(Player *em);
s32 Player_HasMashBuildup(Player *em);
void CamCtl_SetTargetObj(u8 *o);
void Bowgun_DrawAttack(Player *em, s32 a);
void GreatSword_DrawAttack(Player *em);
void Bow_DrawAttack(Player *em);
void SwordShield_DrawAttack(Player *em);
void DualSwords_DrawAttack(Player *em);
void Lance_DrawAttack(Player *em);
void Gunlance_DrawAttack(Player *em);
void Hammer_DrawAttack(Player *em);
void HuntingHorn_DrawAttack(Player *em);
void LongSword_DrawAttack(Player *em);
extern s16 gDrawnWalkStopIdle[];
s32 Player_HasSongEffect(Player *pl, s16 kind);
void Player_GuardInput(Player *p);
s32 Player_Damage(Player *p, s16 damage, s16 lose);
s16 Player_CountSpecialItems(Player *p);
s16 Area_Temperature(u16 area);
s32 Player_InEmAura(Player *p);
/* unit's own declaration: the flag declared u8 as Player_UpdateEnvironment passes it; with the definition's s32 its call compiles differently */
void PlayerFx_TickFloorHazard(Player *p, u8 flag);
void ItemAttack_Create(void *obj, s32 kind, s8 arg2);
void Boomerang_Create(Em *em, u8 kind, u8 n);
u16 Rand_Next(s32 stream);
void Player_AddSpawnOffset(Player *p, f32 *pos, u16 angle);
extern f32 gStartPos[][3];
extern u16 gStartAngle[];
extern ScEventRec ScEvent_VersusEnd;
extern ScEventRec ScEvent_VelocidromeSighting;
extern ScEventRec ScEvent_BulldromeSighting;
extern ScEventRec ScEvent_DaimyoHermitaurSighting;
extern ScEventRec ScEvent_CongalalaSighting;
extern ScEventRec ScEvent_RathianSighting;
extern ScEventRec ScEvent_KushalaJungleSighting;
extern ScEventRec ScEvent_KushalaSnowSighting;
extern ScEventRec ScEvent_LunastraTeostraVolcano8Sighting;
extern ScEventRec ScEvent_LunastraTeostraTowerSighting;
extern ScEventRec ScEvent_GarugaSighting;
extern ScEventRec ScEvent_ChameleosSighting;
extern ScEventRec ScEvent_LunastraTeostraVolcano7Sighting;
extern ScEventRec ScEvent_BlangongaSighting;
extern ScEventRec ScEvent_ShogunCeanataurSighting;
extern ScEventRec ScEvent_KutkuSighting;
extern ScEventRec ScEvent_KhezuSighting;
extern ScEventRec ScEvent_DiablosSighting;
extern ScEventRec ScEvent_GraviosSighting;
extern ScEventRec ScEvent_MonoblosSighting;
extern ScEventRec ScEvent_RathalosSighting;
extern ScEventRec ScEvent_KirinSighting;
extern ScEventRec ScEvent_Seq1;
extern ScEventRec ScEvent_Seq5;
extern ScEventRec ScEvent_Seq3;
extern ScEventRec ScEvent_Seq3B;
extern ScEventRec ScEvent_Seq11;
u16 Player_GetGatherPoint(Player *em, u16 *id, f32 *pos);
u16 MinePoint_FindNearPlayer(void *obj);
void Player_TryGather(Player *em, s32 a);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Player_StopMove(void *obj);
s32 Player_StartJump(void *obj, s16 frames);
void Player_MoveAccel(Player *p);
void Player_Die(Player *em);
void Player_UseStamina(Player *p, s32 n);
s32 Player_FloorAheadBelow(Player *em, u16 above, f32 fwd, f32 up);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Bowgun_LoadAmmo(Player *p, s32 a);
void Hammer_StartCharge(Player *em);
void HuntingHorn_StartPlaying(Player *em, s32 a);
void DualSwords_DemonToggle(Player *em, s32 a);
void LongSword_SpiritDraw(Player *p);
void Bow_DrawToAim(Player *em);
void Player_ResetAim(Player *p);
void Hammer_ChargeFromRoll(Player *em);
u16 Player_FindActionSpot(Player *, f32 *, u16 *arg, u8 *);
s32 Player_NearTrader(void *obj);
void Player_StartMoveTo(Player *em, s16 frames);
void Vec_Copy(f32 *out, f32 *in);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
extern f32 gSpotLineupOffsets[];
f32 *Mtx_FromAnglesXYZ(s32 *ang, f32 *mtx);
s16 Pouch_Count(Player *p, u16 item);
s16 Pouch_Room(Player *p, u16 item);
s16 Pouch_HasFreeSlot(Player *p);
u8 Notice_PushItem(char *item);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void Quest_ClearBySubTarget(void);
void Quest_Abort(void);
void Player_StartSpotAction(Player *p, s32 mode);
s32 Em_GetFloorYAbove(Em *em, f32 *pos, f32 *y);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
s32 GreatSword_AttackInput(Player *em);
s32 LongSword_AttackInput(Player *em);
s32 Bowgun_AttackInput(Player *em);
s32 Bow_AttackInput(Player *em, s32 a);
s32 Hammer_AttackInput(Player *em);
s32 HuntingHorn_AttackInput(Player *em);
s32 Lance_AttackInput(Player *em);
s32 Gunlance_AttackInput(Player *em);
s32 SwordShield_AttackInput(Player *em);
s32 DualSwords_AttackInput(Player *em);
f32 GmEm_ScaleBySize(Em *em, f32 v);
void Em_MotionDelta(Em *em, f32 *out);
f32 Vec_Distance(f32 *a, f32 *b);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
f32 Vec_Length(f32 *v);
u16 Vec_Heading(f32 *from, f32 *to);
void MsgEvent_OpenPopup(void);
void ScEvent_SetCamera(u8 arg0, u8 kind);
s32 Area_IsDay(u16 area);
void Bgm_EnterArea(u16 area);
void Bgm_Play(s32 id);
void Jingle_Play(void *obj, s32 id);
void Save_SetEventFlag(s32 n);
void Stream_Pause(s32 ch, s32 pause);
s32 CamReq_State(void);
void CamReq_Cancel(void);
void FadeA_Start(s32 id);
s32 Versus_BothDefeated(void);
void Effect2C_Spawn(s16);
typedef struct WeaponMotion { s16 motion; s16 blend; s16 flag; s16 next; } WeaponMotion;
extern WeaponMotion gDrawMotions[];
extern WeaponMotion gSheatheMotions[];
extern WeaponMotion gSheatheMovingMotions[];
extern WeaponMotion gDrawMovingMotions[];
void Player_UseItem(Player *em, u8 mode);
s32 Player_PouchHasSelectable(Player *p);
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Save_ClearEventFlag(s32 n);
s32 Quest_GetStageEmKinds(u8 *ids);
extern ScEventRec ScEvent_DemoCam7;
extern ScEventRec ScEvent_DemoCam32;
extern ScEventRec ScEvent_DemoCam2;
extern ScEventRec ScEvent_DemoCam24;
extern ScEventRec ScEvent_DemoCam47;
extern ScEventRec ScEvent_DemoCam3A;
extern ScEventRec ScEvent_DemoCam37;
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Em_KindMatches(u8 a, u8 b);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u16 Camera_GetYaw(void);
void Item_UseMisc(Player *pl);
void Item_UseTool(Player *pl);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
void Player_AddStamina(Player *p, s16 n);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_IsLocal(void *);
void Player_RequestState(Player *p, u16 state, u8 sub, u16 flags);
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter declared s32 (the definition's is s16); with the s16 the calls compile differently (Player_StateSpitRoast most of all) */
void Player_SetPosture(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Quest_DeliverFromPouch(Player *pl, s32 mode);
s32 Save_TestEventFlag(s32);
void Lance_GuardAttack(Player *p, s32 arg1);
void SwordShield_GuardSlash(Player *p);
s32 Gunlance_GuardInput(Player *p, s32 arg1);
u8 Player_StickMoveLevel(Player *, s32);
s32 Player_WallClimbInput(Player *, s16);
s32 Player_CrouchInput(Player *);
void Player_StepBack(Player *, s32);
void ItemProp_CreateAtPlayer(Player *, s32 k);
void Dmcam_Init(void *ptinfo, s32 keep);
u8 *Dmcam_Load(s32 idx, s32 alt);
s32 Motion_ReachedFrame(void *, s32, f32);
s32 CamCtl_IsTurningBehind(void);
void CamShake_ReqA(void *, s32 i);
s32 Game_IsOnline(void);
s32 Quest_PhaseIsOver(s32);
void Vib_StartLocal(void *port, s32 i);
u16 Map_DayId(u16);
u8 Notice_Push(u8, s16, s16);
void Player_ReturnToIdle(Player *p, s32 action, s32 a2, s32 a3);
s32 Player_FloorAheadAbove(Player *p, f32 *out, f32 fwd, f32 up, f32 range);
extern ScEventRec ScEvent_CamF;
extern ScEventRec ScEvent_Cam6;
extern ScEventRec ScEvent_CamB;
extern ScEventRec ScEvent_Cam2;
extern ScEventRec ScEvent_Cam3;
extern ScEventRec ScEvent_Cam4;
extern ScEventRec ScEvent_Cam5;
s32 ScEvent_RunB(ScEvent *ev);
u16 Math_AngleXZ(f32 *from, f32 *to);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Area_ToWorld(s32 area, f32 *pos, f32 *out);
u16 Area_DayId(u16 area);
void CamReq_Set(s32, void *arg);
u8 MonsterGroup_First(u8 group);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
s16 Player_CheckRideSlip(Player *);
void HitSpark_SpawnOnOwner(void *owner, u8 type, u8 sub, f32 scale);
static void Player_SetAimLayers(Player *, u16, u16);
void Player_IdleActions(Player *, s32);
void Player_BowgunAim(Player *);
void Player_SetEvadeFrames(Player *em);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
void SwordShield_EvadeAttack(Player *em);
void DualSwords_EvadeAttack(Player *em);
s32 Motion_InFrameRange(void *, s32, f32, f32);
s8 Player_TurnAlongWall(Player *, s16);
/* end of prototypes */

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
            if (em->rotZ <= 0x4000) {
                em->rotZ = em->rotZ - fly->pitch * 2;
            } else if (em->rotZ <= sel || em->rotZ > 0x10000 - sel) {
                em->rotZ = em->rotZ - fly->pitch;
            } else if (em->rotZ != 0x10000 - sel) {
                em->rotZ = em->rotZ + fly->pitch;
            }
        } else {
            if (em->rotZ >= 0xC000) {
                em->rotZ = em->rotZ + fly->pitch * 2;
            } else if (em->rotZ < sel || em->rotZ >= 0x10000 - sel) {
                em->rotZ = em->rotZ + fly->pitch;
            } else if (em->rotZ != sel) {
                em->rotZ = em->rotZ - fly->pitch;
            }
        }
    } else if (em->rotZ != 0) {
        if (em->rotZ <= sel) {
            em->rotZ = em->rotZ - fly->pitch;
        } else if (em->rotZ >= 0x10000 - sel) {
            em->rotZ = em->rotZ + fly->pitch;
        }
    }
    if (!(flags & 2)) {
        bank = em->rotZ;
        bank = bank < 0x8000 ? bank : (u16) (0x10000 - bank);
        if (bank <= 0x1000) {
            step = fly->turn;
        } else {
            step = fly->turn + (bank - 0x1000) / 16;
        }
    } else {
        step = fly->turn;
    }
    if (em->rotZ != 0) {
        if (flags & 0x100) {
            bankHigh = 0x10000;
            bankLow = 0;
        } else {
            bankHigh = 0xF000;
            bankLow = 0x1000;
        }
        if (em->rotZ <= bankHigh && em->rotZ >= 0xC000) {
            if (em->flightTurn <= step) {
                em->rotY = em->rotY + em->flightTurn;
                em->flightTurn = 0;
            } else {
                em->rotY = em->rotY + step;
                em->flightTurn = em->flightTurn - step;
            }
        } else if (em->rotZ >= bankLow && em->rotZ <= 0x4000) {
            if (em->flightTurn >= 0x10000 - step) {
                em->rotY = em->rotY - (0x10000 - em->flightTurn);
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
        if (bank < 0x1000) {
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
            if (em->rotX < 0x8000) {
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
 * range, else motLeft when it is 0x8000 or more, else motRight. Step 1 turns
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
        if ((u16) angleDelta <= range || (u16) angleDelta >= 0x10000 - range) {
            Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
            Em_SetMotion(em, motFront, 0, 0);
        } else if ((u16) angleDelta >= 0x8000) {
            Em_SetMotion(em, motLeft, 0, 0);
        } else {
            Em_SetMotion(em, motRight, 0, 0);
        }
        break;
    case 1:
        angleDelta = (u16) (em->aimAngle - (u16) em->rotY);
        if (em->motion[0].playing == 0 || EmMotion_ReachedFrame(em, 0, 110.0f)) {
            if (angleDelta <= range || angleDelta >= 0x10000 - range) {
                Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
                Em_SetMotion(em, motFront, 0, 0);
            } else {
                Player_ClearFlag((Player *) em, PLF_NO_ROOT_MOVE);
                if (angleDelta >= 0x8000) {
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
        } else if (angleDelta < 0x8000) {
            em->rotY = (u16) (em->rotY + 0x200);
        } else {
            em->rotY = (u16) (em->rotY - 0x200);
        }
        break;
    }
    return 0;
}

//---
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
 * plus 0x8000), and bit 0 returns 1 at once when the monster has no goal
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
        ang += 0x8000;
    }
    angleDelta = ang - em->rotY;
    if (angleDelta <= 0x8000) {
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
 * within it, else motLeft when the angle left is 0x8000 or more, else
 * motRight. Em_TurnStep carries the turn on.
 */
void Em_StartTurn(Em *em, EmTurnParams *turn, s32 back)
{
    u32 angleDelta;

    if (back == 0) {
        em->aimAngle = Math_AngleXZ((f32 *) &em->posX, em->goal);
    } else {
        em->aimAngle = (u16) Math_AngleXZ((f32 *) &em->posX, em->goal) - 0x8000;
    }
    angleDelta = (u16) (em->aimAngle - em->rotY);
    if (turn->range != 0 && (angleDelta <= turn->range || angleDelta >= 0x10000 - turn->range)) {
        Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
        Em_SetMotion(em, turn->motFront, 0, 0);
    } else if (angleDelta >= 0x8000) {
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
    Em_StartTurn(em, turn, 0x1);
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
        if (w->range != 0 && (angleDelta <= w->range || angleDelta >= 0x10000 - w->range)) {
            Player_SetFlag((Player *) em, PLF_NO_ROOT_MOVE);
            Em_SetMotion(em, w->motFront, 0, 0);
        } else {
            Player_ClearFlag((Player *) em, PLF_NO_ROOT_MOVE);
            if (angleDelta >= 0x8000) {
                Em_SetMotion(em, w->motLeft, 0, 0);
            } else {
                Em_SetMotion(em, w->motRight, 0, 0);
            }
        }
    } else if ((w->from == 0.0f && w->to == 0.0f) || EmMotion_InFrameRange(em, 0, w->from, w->to) == 1) {
        if (((angleDelta + step) & 0xFFFF) < step * 2) {
            em->rotY = em->aimAngle;
        } else if (angleDelta < 0x8000) {
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
 * Sets up the scene events of the quest being started: empties gSceneEvents
 * and the running flag, then runs ScEvent_LoadDemoCams and stops if that
 * started a monster's introduction event. Otherwise it loads the area camera
 * event (ScEvent_LoadCamera) and, by gScene.questId, starts the quest's own
 * event record in slot 0 with ScEvent_Set (one quest fills slot 1 too, and
 * one also sets gScene.eventRunning at once).
 */
void ScEvent_Init(void)
{
    ScEventList *list = &gSceneEvents;

    gScene.eventRunning = 0;
    gSceneEvents.active = 0;
    *(s32 *) &gSceneEvents.ev[2] = 0;
    *(s32 *) &gSceneEvents.ev[1] = 0;
    *(s32 *) &gSceneEvents.ev[0] = 0;
    gSceneEvents.ended = 0;
    if (ScEvent_LoadDemoCams(list) & 0xFFFF) {
        return;
    }
    Dmcam_Init(gSysSettings.readBuf, 1);
    ScEvent_LoadCamera(list);
    switch (gScene.questId) {
    case 0x61A9:
        ScEvent_Set(&list->ev[0], &ScEvent_VersusEnd.head);
        gScene.eventRunning = 1;
        break;
    case QUEST_ID_VELOCIDROME_SCENE_1:
    case QUEST_ID_VELOCIDROME_SCENE_2:
    case QUEST_ID_VELOCIDROME_SCENE_3:
        ScEvent_Set(&list->ev[0], &ScEvent_VelocidromeSighting.head);
        break;
    case QUEST_ID_BULLDROME_SCENE_1:
    case QUEST_ID_BULLDROME_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_BulldromeSighting.head);
        break;
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_1:
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_DaimyoHermitaurSighting.head);
        break;
    case QUEST_ID_CONGALALA_SCENE_1:
    case QUEST_ID_CONGALALA_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_CongalalaSighting.head);
        break;
    case QUEST_ID_RATHIAN_SCENE_1:
    case QUEST_ID_RATHIAN_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_RathianSighting.head);
        break;
    case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_KushalaJungleSighting.head);
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_LunastraTeostraTowerSighting.head);
        break;
    case QUEST_ID_KUSHALA_SNOW_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_KushalaSnowSighting.head);
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_LunastraTeostraVolcano8Sighting.head);
        break;
    case QUEST_ID_GARUGA_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_GarugaSighting.head);
        break;
    case QUEST_ID_CHAMELEOS_SCENE_1:
    case QUEST_ID_CHAMELEOS_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_ChameleosSighting.head);
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_1:
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_LunastraTeostraVolcano7Sighting.head);
        break;
    case QUEST_ID_BLANGONGA_SCENE_1:
    case QUEST_ID_BLANGONGA_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_BlangongaSighting.head);
        break;
    case QUEST_ID_SHOGUN_CEANATAUR_SCENE_1:
    case QUEST_ID_SHOGUN_CEANATAUR_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_ShogunCeanataurSighting.head);
        break;
    case QUEST_ID_KUTKU_SCENE_1:
    case QUEST_ID_KUTKU_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_KutkuSighting.head);
        break;
    case QUEST_ID_KHEZU_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_KhezuSighting.head);
        break;
    case QUEST_ID_DIABLOS_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_DiablosSighting.head);
        break;
    case QUEST_ID_GRAVIOS_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_GraviosSighting.head);
        break;
    case QUEST_ID_MONOBLOS_SCENE:
        ScEvent_Set(&list->ev[0], &ScEvent_MonoblosSighting.head);
        break;
    case QUEST_ID_RATHALOS_SCENE_1:
    case QUEST_ID_RATHALOS_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_RathalosSighting.head);
        break;
    case QUEST_ID_KIRIN_SCENE_1:
    case QUEST_ID_KIRIN_SCENE_2:
        ScEvent_Set(&list->ev[0], &ScEvent_KirinSighting.head);
        break;
    case 1:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq1.head);
        break;
    case 3:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq3.head);
        ScEvent_Set(&list->ev[1], &ScEvent_Seq3B.head);
        break;
    case 5:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq5.head);
        break;
    case 0x11:
        ScEvent_Set(&list->ev[0], &ScEvent_Seq11.head);
        break;
    }
}

/*
 * Starts the event record rec in slot: returns -1 and does nothing when the
 * record's doneFlag is nonzero and already set in the save
 * (Save_TestEventFlag). Otherwise loads the record's camera (Dmcam_Load),
 * switches the slot on with the record's check and run callbacks and a
 * pointer to the record, and returns 0.
 */
s32 ScEvent_Set(ScEvent *slot, ScEventHead *rec)
{
    u16 n;

    n = rec->doneFlag;
    if ((n != 0) && (Save_TestEventFlag((s32) n) == 1)) {
        return -1;
    }
    Dmcam_Load((s32) rec->camera, 0);
    slot->on = 1;
    slot->check = rec->check;
    slot->run = rec->run;
    slot->data = rec;
    return 0;
}

/*
 * Prepares the monsters' introduction events of the stage: clears seven save
 * event flags and the demo cameras, then walks the stage's monster kinds
 * (Quest_GetStageEmKinds). The first of EM_KIND_LAO_SHAN_LUNG,
 * EM_KIND_ASHEN_LAO_SHAN_LUNG, EM_KIND_FATALIS, EM_KIND_FATALIS_B,
 * EM_KIND_FATALIS_C, EM_KIND_SHEN_GAOREN or EM_KIND_YAMA_TSUKAMI puts its
 * record (ScEvent_DemoCam*) in slot 0 of list, loads that kind's demo cameras
 * and is returned. The Kushala kinds and Chameleos, and Lunastra and Teostra,
 * only load a camera and the walk goes on. Returns 0 when no introduction was
 * started.
 */
s32 ScEvent_LoadDemoCams(ScEventList *list)
{
    u8 ids[4];
    s32 i;
    s32 n;

    Save_ClearEventFlag(EVENT_FLAG_LAO_SHAN_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_ASHEN_LAO_SHAN_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_FATALIS_CASTLE_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_FATALIS_TOWER_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_SHEN_GAOREN_SCENE);
    Save_ClearEventFlag(EVENT_FLAG_YAMA_TSUKAMI_TOWER_SCENE);
    Dmcam_Init(gSysSettings.readBuf, 0);
    n = Quest_GetStageEmKinds(ids);
    for (i = 0; i < n; i++) {
        switch (ids[i]) {
        case 7:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam7.head);
            Dmcam_Load(0x7C, 0);
            Dmcam_Load(0x86, 0);
            return 7;
        case 0x32:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam32.head);
            Dmcam_Load(0x7C, 0);
            Dmcam_Load(0x86, 0);
            return 0x32;
        case 2:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam2.head);
            Dmcam_Load(0x83, 0);
            Dmcam_Load(0x89, 0);
            Dmcam_Load(0x8A, 0);
            return 2;
        case 0x24:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam24.head);
            Dmcam_Load(0x83, 0);
            Dmcam_Load(0x89, 0);
            Dmcam_Load(0x8A, 0);
            return 0x24;
        case 0x47:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam47.head);
            Dmcam_Load(0x83, 0);
            Dmcam_Load(0x89, 0);
            Dmcam_Load(0x8A, 0);
            return 0x47;
        case 0x37:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam37.head);
            Dmcam_Load(0x7E, 0);
            Dmcam_Load(0x87, 0);
            return 0x37;
        case 0x3A:
            ScEvent_Set(&list->ev[0], &ScEvent_DemoCam3A.head);
            Dmcam_Load(0x88, 0);
            return 0x3A;
        case 0x36:
        case 0x3C:
        case 0x3B:
            Dmcam_Load(0x7F, 0);
            break;
        case 0x40:
        case 0x41:
            Dmcam_Load(0x80, 0);
            break;
        }
    }
    return 0;
}

/*
 * In an offline game, reinitializes the demo cameras (Dmcam_Init) and, by the
 * area's base id (Map_DayId of gScene.map), puts the area's camera event
 * record (ScEvent_Cam2 to ScEvent_CamF) into slot 2 of list; areas without one
 * get none. Always returns 0.
 */
s32 ScEvent_LoadCamera(ScEventList *list)
{
    s32 base;

    if (Game_IsOnline() != 1) {
        Dmcam_Init(gSysSettings.readBuf, 1);
        base = Map_DayId((u16) gScene.map) & 0xFFFF;
        switch (base) {
        case 2:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam2.head);
            break;
        case 3:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam3.head);
            break;
        case 4:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam4.head);
            break;
        case 5:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam5.head);
            break;
        case 6:
            ScEvent_Set(&list->ev[2], &ScEvent_Cam6.head);
            break;
        case 11:
            ScEvent_Set(&list->ev[2], &ScEvent_CamB.head);
            break;
        case 13:
        case 14:
        case 15:
            ScEvent_Set(&list->ev[2], &ScEvent_CamF.head);
            break;
        }
    }
    return 0;
}

//---
void ScEvent_Update(void)
{
    ScEventList *w = &gSceneEvents;
    s32 i;
    ScEvent *slot;

    gSceneEvents.ended = 0;
    switch (gSceneEvents.active) {
    case 1:
        if (w->ev[w->cur].run(&w->ev[w->cur]) == 0) {
            break;
        }
        gScene.eventRunning = 0;
        w->active = 0;
    case 0:
        for (slot = w->ev, i = 0; i < 3; i++, slot++) {
            if (slot->on != 0 && slot->check(slot) > 0) {
                w->active = 1;
                w->cur = i;
                slot->run(slot);
                return;
            }
        }
        MsgEvent_OpenPopup();
        break;
    }
}

/*
 * Check callback of an event tied to an area: if the record's doneFlag is
 * already set in the save it clears the slot and returns -1 (drop the event);
 * otherwise returns 1 (start it) when the record's area is the scene's
 * current area by day id (Area_DayId(gScene.area)), else 0.
 */
s32 ScEvent_CheckDay(ScEvent *event)
{
    if (Save_TestEventFlag(event->data->doneFlag) == 1) {
        *(s32 *) event = 0;
        return -1;
    }
    return event->data->area == Area_DayId(gScene.area);
}

/*
 * Run callback of an event record, one step of event->step per call; returns 1
 * when the event is over, else 0. Step 0 sets gScene.eventRunning, points the
 * camera (ScEvent_SetCamera), sets ev->timer and pauses the stream
 * (Stream_Pause) by the record's music value (the timer for 2 to 10, the
 * pause for 1 to 3 and 8 to 10), and plays the record's day or night jingle
 * if it has one. Step 1 counts the timer down and at 0 plays the music for
 * the value; when the camera request ends (CamReq_State() <= 0) it clears
 * eventRunning, sets the record's doneFlag in the save, restores the music
 * (1: Bgm_EnterArea; 2, 3, 8 to 10: gScene.musicState 5, musicTimer 150),
 * cancels the camera and sets gSceneEvents.ended. Step 2 clears the slot's
 * first word and returns 1.
 */
s32 ScEvent_RunA(ScEvent *event)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ScEventList *list = &gSceneEvents;

    switch (event->step) {
    case 0:
        gScene.eventRunning = 1;
        ScEvent_SetCamera(event->data->camera, event->data->camGroup);
        switch (event->data->music) {
        case 4:
            event->timer = 0xCD;
            break;
        case 5:
            event->timer = 0xC0;
            break;
        case 6:
            event->timer = 0xEF;
            break;
        case 7:
            event->timer = 0xC4;
            break;
        case 8:
            event->timer = 4;
            Stream_Pause(0, 1);
            break;
        case 9:
            event->timer = 0x15E;
            Stream_Pause(0, 1);
            break;
        case 10:
            event->timer = 0x350;
            Stream_Pause(0, 1);
            break;
        case 3:
            event->timer = 0x10E;
            Stream_Pause(0, 1);
            break;
        case 2:
            event->timer = 0x15E;
        case 1:
            Stream_Pause(0, 1);
            break;
        case 0:
            break;
        }
        if (event->data->jingleDay != 0) {
            if (Area_IsDay(gScene.area) == 1) {
                Jingle_Play(pl, event->data->jingleDay);
            } else {
                Jingle_Play(pl, event->data->jingleNight);
            }
        }
        event->step++;
        break;
    case 1:
        if (event->timer != 0) {
            event->timer--;
            if (event->timer == 0) {
                switch (event->data->music) {
                case 0:
                case 1:
                    break;
                case 4:
                    Jingle_Play(pl, 0xDD);
                    break;
                case 5:
                    Jingle_Play(pl, 0xDD);
                    break;
                case 6:
                    Jingle_Play(pl, 0xDD);
                    break;
                case 7:
                    Jingle_Play(pl, 0xDD);
                    break;
                case 2:
                    Bgm_Play(0xBC);
                    break;
                case 3:
                    Bgm_Play(0xC2);
                    break;
                case 8:
                    Bgm_Play(0xCE);
                    break;
                case 9:
                    Bgm_Play(0xD0);
                    break;
                case 10:
                    Bgm_Play(0xC2);
                    break;
                }
            }
        }
        if (CamReq_State() <= 0) {
            event->step++;
            gScene.eventRunning = 0;
            Save_SetEventFlag(event->data->doneFlag);
            switch (event->data->music) {
            case 1:
                Bgm_EnterArea(gScene.area);
                break;
            case 0:
                break;
            case 2:
            case 3:
            case 8:
            case 9:
            case 10:
                gScene.musicState = 5;
                gScene.musicTimer = 0x96;
                break;
            }
            CamReq_Cancel();
            list->ended = 1;
        }
        break;
    case 2:
        *(s32 *) event = 0;
        return 1;
    }
    return 0;
}

s32 ScEvent_CheckAlwaysA(ScEvent *event)
{
    return 1;
}

/*
 * Run callback of the ending event of a versus match, one step of event->step per
 * call; returns 1 when it is over, else 0. Every frame it sets
 * gQuest.inputLock to 90 and noHitTimer to 2 on every player. Step 0 sets
 * gScene.eventRunning; step 1 waits for a defeat bit in gQuest.versusDefeat
 * (mask 3) and starts a fade (FadeA_Start); step 2 waits for
 * Versus_BothDefeated(), then clears eventRunning and spawns an effect
 * (Effect2C_Spawn); step 3 cancels the camera request, sets
 * gSceneEvents.ended, clears the slot and returns 1.
 */
s32 ScEvent_RunVersusEnd(ScEvent *event)
{
    ScEventList *list = &gSceneEvents;
    s32 i;

    gQuest.inputLock = 90;
    for (i = 0; i < gScene.playerCount; i++) {
        gPlayers[i].noHitTimer = 2;
    }
    switch (event->step) {
    case 0:
        gScene.eventRunning = 1;
        event->step++;
        break;
    case 1:
        if (gQuest.versusDefeat & 3) {
            event->step++;
            FadeA_Start(6);
        }
        break;
    case 2:
        if (Versus_BothDefeated() == 1) {
            event->step++;
            gScene.eventRunning = 0;
            Effect2C_Spawn(90);
        }
        break;
    case 3:
        CamReq_Cancel();
        list->ended = 1;
        *(s32 *) event = 0;
        return 1;
    }
    return 0;
}

/*
 * Requests the demo camera req (CamReq_Set). With kind 0 the camera has no
 * target; otherwise it follows the first active monster whose group's first
 * kind (MonsterGroup_First of its type) is kind, and no request is made when
 * there is none.
 */
void ScEvent_SetCamera(u8 req, u8 kind)
{
    Em *em;
    s32 i;

    if (kind == 0) {
        CamReq_Set(req, 0);
        return;
    }
    em = gEms;
    for (i = 20; i != 0; i--) {
        if (em->active != 0 && kind == (u8) MonsterGroup_First(em->type)) {
            CamReq_Set(req, em);
            return;
        }
        em++;
    }
}

/*
 * Returns the doneFlag (the event flag id) of the record of the event now
 * running in gSceneEvents, or -1 when none runs.
 */
s32 ScEvent_CurId(void)
{
    ScEventList *list = &gSceneEvents;
    ScEvent *event;

    if (list->active == 0) {
        return -1;
    }
    event = &list->ev[list->cur];
    return event->data->doneFlag;
}

/*
 * Returns 1 when an event is running and its record's flags bit 0 is set (the
 * monsters hold still while it runs), else 0.
 */
s32 ScEvent_CurFlag1(void)
{
    ScEventList *list = &gSceneEvents;
    ScEvent *event;

    if (list->active != 0) {
        event = &list->ev[list->cur];
        return (event->data->flags & 1) != 0;
    }
    return 0;
}

/*
 * Starts the record rec at once in slot 0: empties gSceneEvents and clears
 * gScene.eventRunning, reinitializes the demo cameras (Dmcam_Init), loads the
 * record's camera and switches the slot on with the record's callbacks.
 * Unlike ScEvent_Set it does not look at doneFlag; plsel starts a record it
 * picked this way.
 */
void ScEvent_StartFrom(ScEventHead *rec)
{
    ScEvent *slot;

    gScene.eventRunning = 0;
    gSceneEvents.active = 0;
    *(s32 *) &gSceneEvents.ev[2] = 0;
    *(s32 *) &gSceneEvents.ev[1] = 0;
    *(s32 *) &gSceneEvents.ev[0] = 0;
    Dmcam_Init(gSysSettings.readBuf, 0);
    slot = &gSceneEvents.ev[0];
    Dmcam_Load(rec->camera, 0);
    slot->on = 1;
    slot->check = rec->check;
    slot->run = rec->run;
    slot->data = rec;
}

s32 ScEvent_CheckAlwaysB(ScEvent *event)
{
    return 1;
}

/*
 * Run callback of a record whose callbacks ScEvent_SetupB set (plsel's
 * viewer), one step of event->step per call; returns 1 when the event is over,
 * else 0. Step 0 is ScEvent_RunA's, with its own waits by the music value.
 * Step 1 counts the timer down and plays the music at 0; when the camera
 * request ends it clears eventRunning, cancels the camera and sets
 * gSceneEvents.ended, but sets no save flag and restores no music. Step 2
 * clears the slot's first word and returns 1.
 */
s32 ScEvent_RunB(ScEvent *event)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ScEventList *list = &gSceneEvents;

    switch (event->step) {
    case 0:
        gScene.eventRunning = 1;
        ScEvent_SetCamera(event->data->camera, event->data->camGroup);
        switch (event->data->music) {
        case 8:
            event->timer = 4;
            Stream_Pause(0, 1);
            break;
        case 9:
            event->timer = 350;
            Stream_Pause(0, 1);
            break;
        case 10:
            event->timer = 900;
            Stream_Pause(0, 1);
            break;
        case 3:
            event->timer = 270;
            Stream_Pause(0, 1);
            break;
        case 2:
            event->timer = 350;
        case 1:
            Stream_Pause(0, 1);
            break;
        case 0:
            break;
        }
        if (event->data->jingleDay != 0) {
            if (Area_IsDay(gScene.area) == 1) {
                Jingle_Play(pl, event->data->jingleDay);
            } else {
                Jingle_Play(pl, event->data->jingleNight);
            }
        }
        event->step++;
        break;
    case 1:
        if (event->timer != 0) {
            event->timer--;
            if (event->timer == 0) {
                switch (event->data->music) {
                case 2:
                    Bgm_Play(0xBC);
                    break;
                case 3:
                    Bgm_Play(0xC2);
                    break;
                case 8:
                    Bgm_Play(0xCE);
                    break;
                case 9:
                    Bgm_Play(0xD0);
                    break;
                case 10:
                    Bgm_Play(0xC2);
                    break;
                }
            }
        }
        if (CamReq_State() <= 0) {
            event->step++;
            gScene.eventRunning = 0;
            CamReq_Cancel();
            list->ended = 1;
        }
        break;
    case 2:
        *(s32 *) event = 0;
        return 1;
    }
    return 0;
}

void ScEvent_SetupB(ScEventHead *rec)
{
    rec->check = ScEvent_CheckAlwaysB;
    rec->run = ScEvent_RunB;
}

s32 Player_InSceneEvent(void)
{
    return gSceneEvents.ended != 0;
}

/*
 * Sets the part motion overrides 0 and 1 (partAnimOn, partAnimMotion,
 * partAnimWeight) from pl->aimWeight: both off at weight 0. A positive weight
 * uses motion motA on part 0 and motA + 100 on part 1, a negative weight motB
 * the same way, with the weight's size (at most 100) as the override weight;
 * a motA of 0x581 or a motB of 0x582 becomes 0x580 with part 0 left off.
 */
static void Player_SetAimLayers(Player *pl, u16 motA, u16 motB)
{

    if (pl->aimWeight == 0) {
        pl->partAnimOn[0] = 0;
        pl->partAnimOn[1] = 0;
        return;
    }
    pl->partAnimOn[0] = 1;
    pl->partAnimOn[1] = 1;
    if (pl->aimWeight > 0) {
        if (motA == 0x581) {
            pl->partAnimMotion[0] = 0x580;
            pl->partAnimOn[0] = 0;
        } else {
            pl->partAnimMotion[0] = motA;
        }
        pl->partAnimMotion[1] = motA + 100;
        pl->partAnimWeight[0] = pl->aimWeight;
        pl->partAnimWeight[1] = pl->aimWeight;
    } else {
        if (motB == 0x582) {
            pl->partAnimMotion[0] = 0x580;
            pl->partAnimOn[0] = 0;
        } else {
            pl->partAnimMotion[0] = motB;
        }
        pl->partAnimMotion[1] = motB + 100;
        pl->partAnimWeight[0] = -pl->aimWeight;
        pl->partAnimWeight[1] = -pl->aimWeight;
    }
    if (pl->partAnimWeight[0] > 100) {
        pl->partAnimWeight[0] = 100;
    }
    if (pl->partAnimWeight[1] > 100) {
        pl->partAnimWeight[1] = 100;
    }
}

/* Adds delta to pl->aimWeight, keeping it within -100 to 100. */
static void Bowgun_AddAimWeight(Player *pl, s8 delta)
{

    pl->aimWeight += delta;
    if (pl->aimWeight >= 100) {
        pl->aimWeight = 100;
    }
    if (pl->aimWeight < -99) {
        pl->aimWeight = -100;
    }
}

/*
 * The per-frame damage the player takes from poison and the surroundings;
 * does nothing for a fainted player, and stops early when Player_Damage
 * returns 1. It clears pl->inPoisonArea and ticks the poison (every
 * pl->poisonTimer frames: poison down by 1, 1 damage, the poison effect;
 * another player's copy only shows the effect while PL_STATUS_ICON_POISON is
 * set). For the local player it takes 1 damage per special item carried
 * (Player_CountSpecialItems) every 30 frames, and in a hot area
 * (Area_Temperature 1 or 3) 1 damage every pl->envTimer frames (set from the
 * area kind and the skills 0x84 to 0x88) unless coolDrinkTimer runs or skill
 * 0x85 or 0x86 protects it. Then, outside the reactions other than the
 * guard's, the faint, the quest events and noHitTimer, it applies the ground
 * hazard pl->floorHazard, or 2 when a Lunastra or Teostra with wind is close
 * (Player_InEmAura): hazard 1 raises poison to at least 10 (and sets
 * inPoisonArea) unless SKILL_POISON_IMMUNE; hazard 2 deals 1 damage every
 * damageInterval frames (8, changed by the skills 0xBF to 0xC2).
 */
void Player_UpdateEnvironment(Player *pl)
{
    f32 effectPos[3];
    s16 n;
    s16 k;
    u8 hazard;
    u16 damageInterval;

    if (pl->stateGroup == PL_GROUP_FAINT) {
        return;
    }
    pl->inPoisonArea = 0;
    if (pl->poison > 0) {
        if (--pl->poisonTimer <= 0) {
            pl->poison--;
            if (Player_HasSkill(pl, SKILL_POISON_HALF) == 1) {
                k = 60;
            } else if (Player_HasSkill(pl, SKILL_POISON_DOUBLE) == 1) {
                k = 15;
            } else {
                k = 30;
            }
            pl->poisonTimer = k;
            if (Player_IsLocal(pl) == 1) {
                effectPos[0] = 0.0f;
                effectPos[1] = 20.0f;
                effectPos[2] = 0.0f;
                Effect06_SpawnAt(pl, 3, 0x14, effectPos, 1.0f);
            }
            if (Player_Damage(pl, 1, 0) == 1) {
                return;
            }
        }
    }
    if (Player_IsLocal(pl) == 0 && (pl->statusIcons & 1)) {
        k = Player_HasSkill(pl, SKILL_POISON_HALF) ? 60 : 30;
        if (gScene.frame % k == 0) {
            effectPos[0] = 0.0f;
            effectPos[1] = 20.0f;
            effectPos[2] = 0.0f;
            Effect06_SpawnAt(pl, 3, 0x14, effectPos, 1.0f);
        }
    }
    if (Player_IsLocal(pl) == 1) {
        n = Player_CountSpecialItems(pl);
        if (n > 0 && gScene.frame % 30 == 0) {
            if (Player_Damage(pl, n, 0) == 1) {
                return;
            }
        }
    }
    if (Player_IsLocal(pl) != 1) {
        return;
    }
    k = Area_Temperature(pl->area);
    switch (k) {
    case 1:
    case 3:
        if (pl->coolDrinkTimer != 0) {
            break;
        }
        if (k == 1 && (Player_HasSkill(pl, SKILL_HEAT_RES) == 1 || Player_HasSkill(pl, SKILL_HEAT_RES2) == 1)) {
            break;
        }
        if (k == 3 && Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
            break;
        }
        if (--pl->envTimer > 0) {
            break;
        }
        if (k == 1) {
            pl->envTimer = 90;
            if (Player_HasSkill(pl, 0x87) == 1) {
                pl->envTimer = 60;
            } else if (Player_HasSkill(pl, 0x88) == 1) {
                pl->envTimer = 45;
            } else if (Player_HasSkill(pl, 0x84) == 1) {
                pl->envTimer = 180;
            }
        } else {
            pl->envTimer = 60;
            if (Player_HasSkill(pl, 0x87) == 1) {
                pl->envTimer = 40;
            } else if (Player_HasSkill(pl, 0x88) == 1) {
                pl->envTimer = 30;
            } else if (Player_HasSkill(pl, 0x84) == 1) {
                pl->envTimer = 120;
            } else if (Player_HasSkill(pl, SKILL_HEAT_RES) == 1) {
                pl->envTimer = 180;
            }
        }
        if (Player_Damage(pl, 1, 0) == 1) {
            return;
        }
        break;
    case 2:
    case 4:
        break;
    }
    if (pl->stateGroup == PL_GROUP_REACT) {
        switch (pl->state) {
        case 1:
        case 3:
        case 8:
        case 0xA:
        case 0xB:
        case 0xC:
        case 0x26:
            break;
        default:
            return;
        }
    }
    if (pl->stateGroup == PL_GROUP_FAINT) {
        return;
    }
    if (pl->stateGroup == PL_GROUP_EVENT) {
        return;
    }
    if (pl->action == PL_POSTURE_AIRBORNE) {
        return;
    }
    if (pl->noHitTimer > 0) {
        return;
    }
    hazard = pl->floorHazard;
    if (hazard == 0 && Player_InEmAura(pl) == 1) {
        hazard = 2;
    }
    PlayerFx_TickFloorHazard(pl, hazard);
    switch (hazard) {
    case 0:
        break;
    case 1:
        if (Player_HasSkill(pl, SKILL_POISON_IMMUNE) == 0) {
            pl->inPoisonArea = 1;
            if (pl->poison < 10) {
                pl->poison = 10;
            }
        }
        break;
    case 2:
        pl->inPoisonArea = 1;
        damageInterval = 8;
        if (Player_HasSkill(pl, 0xBF) == 1) {
            damageInterval = 12;
        } else if (Player_HasSkill(pl, 0xC0) == 1) {
            damageInterval = 24;
        } else if (Player_HasSkill(pl, 0xC1) == 1) {
            damageInterval = 6;
        } else if (Player_HasSkill(pl, 0xC2) == 1) {
            damageInterval = 4;
        }
        if (gScene.frame % damageInterval == 0) {
            if (Player_Damage(pl, 1, 0) == 1) {
                return;
            }
        }
        break;
    }
}

/*
 * Returns 1 when one of the 20 pouch slots holds an item with a count above 0
 * whose item data flags have bit 0 set (usable from the item selector), else
 * 0.
 */
s32 Player_PouchHasSelectable(Player *pl)
{
    s16 i;

    for (i = 0; i < 20; i++) {
        if (pl->pouch[i].item != 0 && pl->pouch[i].count > 0 && (gItemData[pl->pouch[i].item].flags & 1)) {
            return 1;
        }
    }
    return 0;
}

/*
 * Runs the item selector of the local player. Unless the player is stunned,
 * fainted, in a quest event or the quest is over, holding PAD_L2 opens it
 * (pl->itemSelect = 1, with a sound) and, when the pouch has a selectable
 * item (Player_PouchHasSelectable), PAD_CIRCLE moves pl->pouchCursor to the
 * next selectable item (pouchHudFlags |= 2) and PAD_SQUARE to the one before
 * (|= 4); releasing PAD_L2 closes it with a sound. In the excluded states it
 * just clears itemSelect.
 */
void Player_UpdateItemSelect(Player *pl)
{
    u16 pressed;

    if (Player_IsLocal(pl) != 0) {
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STUN) == 0 && (u32) (pl->stateGroup - 3) > 1 && Quest_PhaseIsOver(1) != 1) {
            if (pl->input.held2 & PAD_L2) {
                if (pl->itemSelect == 0) {
                    Sound_PlaySeFlat(0, 0x11, 0);
                }
                pl->itemSelect = 1;
                if (Player_PouchHasSelectable(pl) != 0) {
                    pressed = pl->input.pressed2;
                    if (pressed & 0x20) {
                        Sound_PlaySeFlat(0, 0x12, 0);
                        pl->pouchCursor = Pouch_NextSelectable(pl, pl->pouchCursor, 0);
                        pl->pouchHudFlags |= 2;
                    } else if (pressed & 0x200) {
                        Sound_PlaySeFlat(0, 0x12, 0);
                        pl->pouchCursor = Pouch_NextSelectable(pl, pl->pouchCursor, 1);
                        pl->pouchHudFlags |= 4;
                    }
                }
            } else if (pl->itemSelect != 0) {
                Sound_PlaySeFlat(0, 0x14, 0);
                pl->itemSelect = 0;
            }
            return;
        }
        pl->itemSelect = 0;
    }
}

/*
 * Returns 1 when the loaded ammo type differs from savedAmmoType (saved when
 * the ammo menu opened) while the ammo menu is closed, PAD_L2 is not held and
 * the weapon is drawn; else 0 (also for a player who is not a gunner).
 * Player_IdleActions skips the weapon's attack input while it returns 1.
 */
static s32 Player_AmmoTypeChanged(Player *pl)
{
    if ((pl->isGunner != 1) && ((u16) pl->ammoSlot == 0xFF)) {
        return 0;
    }
    if (pl->input.held2 & PAD_L2) {
        return 0;
    }
    if ((pl->ammoMenuOpen == 0) && (pl->savedAmmoType != pl->ammoType) && (pl->weaponOut != 0)) {
        return 1;
    }
    return 0;
}

/*
 * The ammo menu of a gunner (the bow's coatings). Does nothing for a non-
 * local player, a non-gunner, a stunned or fainted player, a quest event,
 * after the quest's end (Quest_PhaseIsOver(1)), with pl->ammoMenuLock set, and in
 * the shooting and stabbing steps of the weapon states (BOW_FIRE and
 * BOW_FIRE_QUICK before step 2; BOW_STAB, BOW_STAB_2, BOW_DRAW_STAB;
 * BOWGUN_FIRE_RECOIL_0 to _2 before step 2 and BOWGUN_RAPID_FIRE before step
 * 3). Holding PAD_L2 opens the menu (ammoMenuOpen; the ammo state goes to the
 * saved* fields) and PAD_CROSS or PAD_TRIANGLE steps the ammo slot either way
 * (Bowgun_NextAmmoSlot, then Bowgun_LoadAmmo, with a sound and a
 * pouchHudFlags bit). Releasing it closes the menu: a changed ammo type is
 * loaded, an unchanged one gets its saved count, clip size and rounds back.
 * Always returns 0.
 */
s32 Player_BowgunAmmoSwitch(Player *pl)
{

    if (Player_IsLocal(pl) == 0) {
        return 0;
    }
    if (pl->isGunner != 1) {
        return 0;
    }
    if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STUN) != 0) {
        return 0;
    }
    if (pl->stateGroup == PL_GROUP_FAINT || pl->stateGroup == PL_GROUP_EVENT) {
        return 0;
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (pl->ammoMenuLock != 0) {
        return 0;
    }
    if (pl->stateGroup == PL_GROUP_WEAPON) {
        if (pl->weaponType == WEAPON_BOW) {
            switch (pl->state) {
            case 0:
            case 14:
                if (pl->step < 2) {
                    return 0;
                }
                break;
            case 3:
            case 4:
            case 13:
                return 0;
            }
        } else {
            switch (pl->state) {
            case 0:
            case 4:
            case 5:
                if (pl->step < 2) {
                    return 0;
                }
                break;
            case 8:
                if (pl->step < 3) {
                    return 0;
                }
                break;
            }
        }
    }
    if (pl->ammoMenuOpen == 0) {
        pl->ammoMenuOpen = 1;
        pl->savedAmmoType = pl->ammoType;
        pl->savedAmmoCount = pl->ammoCount;
        pl->savedClipSize = pl->clipSize;
        pl->savedRoundsLoaded = pl->roundsLoaded;
        pl->savedCoatingSlot = pl->coatingSlot;
    }
    if (pl->input.held2 & PAD_L2) {
        if (pl->input.pressed2 & PAD_CROSS) {
            if (pl->ammoSlot == 0xFF) {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 0, 3);
            } else {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 1);
            }
            Bowgun_LoadAmmo(pl, 0);
            if (pl->ammoSlot != 0xFF) {
                pl->pouchHudFlags |= 8;
                Sound_PlaySeFlat(0, 0x12, 0);
            }
        } else if (pl->input.pressed2 & PAD_TRIANGLE) {
            if (pl->ammoSlot == 0xFF) {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 0, 2);
            } else {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 0);
            }
            Bowgun_LoadAmmo(pl, 0);
            if (pl->ammoSlot != 0xFF) {
                pl->pouchHudFlags |= 0x10;
                Sound_PlaySeFlat(0, 0x12, 0);
            }
        }
    } else if (pl->ammoMenuOpen != 0) {
        pl->ammoMenuOpen = 0;
        if (pl->savedAmmoType != pl->ammoType) {
            Bowgun_LoadAmmo(pl, 0);
        } else {
            pl->ammoCount = pl->savedAmmoCount;
            pl->clipSize = pl->savedClipSize;
            pl->roundsLoaded = pl->savedRoundsLoaded;
        }
    }
    if (pl->savedAmmoType == pl->ammoType) {
        pl->ammoCount = pl->savedAmmoCount;
        pl->clipSize = pl->savedClipSize;
        pl->roundsLoaded = pl->savedRoundsLoaded;
        pl->coatingSlot = pl->savedCoatingSlot;
    }
    return 0;
}

/*
 * Shows the banner that item `item` didn't fit in the pouch; the item states
 * call it when Pouch_GiveItem returns 3 (no room). Returns Notice_Push's result.
 */
u8 Player_NoticePouchFull(s32 item)
{
    return Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_POUCH_FULL, item);
}

/*
 * Returns 1 when the player's status 2 build-up is above 0, 2 when status 4's
 * is, else 0; Player_Update turns the result into PL_REACT_STATUS2 or
 * PL_REACT_STATUS4.
 */
static s32 Player_PickStatusState(Player *pl)
{
    if (pl->status2Buildup > 0) {
        return 1;
    }
    if (pl->status4Buildup > 0) {
        return 2;
    }
    return 0;
}

/*
 * Returns the heading a stick points to in the world: the stick's angle plus
 * the camera's yaw (Camera_GetYaw), as 16 bits. i 0 is the left stick
 * (input.stickAngle), 1 the right stick (input.rightStickAngle).
 */
s32 Player_GetStickYaw(Player *pl, s32 i)
{
    s32 yaw = Camera_GetYaw() & 0xFFFF;

    return ((&pl->input.stickAngle)[i] + yaw) & 0xFFFF;
}

/*
 * Sets pl->targetYaw to the left stick's heading (input.stickAngle plus the
 * camera's yaw), unless the camera is turning behind the player
 * (CamCtl_IsTurningBehind).
 */
void Player_FaceStick(Player *pl)
{
    if (CamCtl_IsTurningBehind() != 1) {
        *(s16 *)&pl->targetYaw = (s16) (pl->input.stickAngle + (Camera_GetYaw() & 0xFFFF));
    }
}

/*
 * Returns how far the left stick is pushed: 0 for none (also for a gunner
 * with the weapon drawn and pl->aimMode 1), 1 for a tilt (input.stickTilt) of
 * 40 or more, 3 for 120 or more (a run), and 5 when the sheathed player in
 * PL_GROUP_ACTION holds PAD_R2 with the stick pushed (the dash; not while
 * PL_ACT_RUN_AFTER_DASH waits out dashDelay). The second parameter is not
 * used.
 */
u8 Player_StickMoveLevel(Player *pl, s32 unused)
{
    u8 lvl = 0;
    u16 tilt;

    switch (pl->weaponType) {
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
    case WEAPON_BOW:
        if (pl->weaponOut != 0 && pl->aimMode == 1) {
            return 0;
        }
        break;
    case WEAPON_GREAT_SWORD:
    case WEAPON_HAMMER:
    case WEAPON_LANCE:
    case WEAPON_SWORD_SHIELD:
    case WEAPON_DUAL_SWORDS:
    case WEAPON_LONG_SWORD:
    case WEAPON_HUNTING_HORN:
    case WEAPON_GUNLANCE:
        break;
    }
    tilt = pl->input.stickTilt;
    if (tilt >= 120) {
        lvl = 3;
    } else if (tilt >= 85) {
        lvl = 1;
    } else if (tilt >= 40) {
        lvl = 1;
    }
    if (pl->action == PL_POSTURE_STANDING && pl->weaponOut == 0 && (pl->input.held & PAD_R2) && lvl != 0
        && (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH) == 0 || pl->dashDelay == 0)) {
        return 5;
    }
    return lvl;
}

/*
 * Works out whether a monster fights the player. First copies inBattle to
 * prevInBattle and clears it. For each monster in the player's area that is
 * active, visible, flagged startsBattle, not in EM_GROUP_DEFEAT, has noticed
 * this player and is in combat, it sets the player's bit in the monster's
 * combatPlayers, pl->inBattle = 1, pl->target to the monster and
 * pl->targetAngle to the angle from the monster to the player plus 0x4000;
 * for Velocidrome, Gendrome and Iodrome it also sets pl->unk73A_combat.
 */
void Player_UpdateInBattle(Player *pl)
{
    s16 i;
    u8 bit;
    Em *monster;
    s32 monsterType;

    bit = 1 << pl->index;
    pl->prevInBattle = pl->inBattle;
    pl->inBattle = 0;
    for (i = 0, monster = gEms; i < 20; i++, monster++) {
        if (monster->area == pl->area && monster->active != 0 && monster->visible != 0 && monster->startsBattle != 0 && monster->stateGroup != 5
            && (monster->noticed & bit) && monster->combat == 1) {
            monster->combatPlayers |= bit;
            pl->inBattle = 1;
            pl->target = monster;
            pl->targetAngle = (u16) (Vec_Heading(&monster->posX, pl->pos) + 0x4000);
            monsterType = monster->type;
            if (monsterType == 0x1B || monsterType == 0x1C || monsterType == 0x1F) {
                pl->unk73A_combat = 1;
            }
        }
    }
}

/*
 * Called with a player (typed Em): returns 1 on the frame a battle begins
 * (inBattle set and not prevInBattle, and the byte Player.unk73A_combat still
 * 0, which it sets to 1), 0xFF while in a battle otherwise, 0 outside a
 * battle.
 */
static s32 Player_EnteredBattle(Em *em)
{
    u8 battleFlag;

    battleFlag = em->inBattle;
    if (battleFlag != 0) {
        if ((battleFlag != em->prevInBattle) && (em->combatPlayers == 0)) {
            em->combatPlayers = 1;
            return 1;
        }
        return 0xFF;
    }
    return 0;
}

//---
s16 Player_CheckRideSlip(Player *pl)
{
    Em *monster;
    s16 i;
    f32 len;
    f32 lim;

    for (monster = gEms, i = 0; i < 20; i++, monster++) {
        if (pl->rideEm == monster->slotNo && monster->type != 0x37) {
            return 0;
        }
    }
    len = Vec_Length(pl->rideDelta);
    if (pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_BED_MENU) {
        lim = 10.0f;
    } else if (pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_RIDE_STAGGER) {
        lim = 4.0f;
    } else {
        lim = 7.0f;
    }
    if (!(len < lim)) {
        if (pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_RIDE_STAGGER) {
            return 1;
        }
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RIDE_STAGGER, 0);
        return 1;
    }
    return 0;
}

/*
 * Takes the stamina cost of a dash (Player_AddStamina): 15, or 8 with skill
 * 0xB6, or 18 with skill 0xB7.
 */
void Player_UseDashStamina(Player *pl)
{
    if (Player_HasSkill(pl, 0xB6) == 1) {
        Player_AddStamina(pl, -8);
        return;
    }
    if (Player_HasSkill(pl, 0xB7) == 1) {
        Player_AddStamina(pl, -0x12);
        return;
    }
    Player_AddStamina(pl, -0xF);
}

/*
 * The input on a climbable wall; returns 1 when it started a state, else 0.
 * The stick up (input.stickDir & PAD_UP) requests PL_ACT_WALL_CLIMB_UP_FAST
 * when PAD_R2 is held (taking the dash stamina, Player_UseDashStamina, unless
 * noTurn is nonzero), else PL_ACT_WALL_CLIMB_UP; the stick down does the same
 * with the DOWN states; PAD_CROSS requests PL_ACT_WALL_LET_GO.
 */
s32 Player_WallClimbInput(Player *pl, s16 noTurn)
{
    u16 stick = pl->input.stickDir;

    if (stick & 0x2000) {
        if (pl->input.held & PAD_R2) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP_FAST) == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP_FAST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                if (noTurn == 0) {
                    Player_UseDashStamina(pl);
                }
            }
        } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP) == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        return 1;
    }
    if (stick & 0x1000) {
        if (pl->input.held & PAD_R2) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN_FAST) == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN_FAST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                if (noTurn == 0) {
                    Player_UseDashStamina(pl);
                }
            }
        } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN) == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        return 1;
    }
    if (pl->input.pressed & PAD_CROSS) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_LET_GO, 0);
        return 1;
    }
    return 0;
}

/*
 * Starts the action of the action spot the player stands at
 * (Player_FindActionSpot gives its position, angle and kind), when mode
 * allows. Kind 1 moves to the spot and starts PL_ACT_SPIT_ROAST_START; 3
 * PL_ACT_SUPPLY_BOX; 4 PL_ACT_TAKE_REWARDS; 7 PL_ACT_DELIVERY_BOX (not once
 * the quest is over); 9 PL_ACT_START_COUNTDOWN (while gScene.countdownOn is
 * 0); 5 (the bed) opens PL_ACT_BED_MENU with mode 0, else moves to a place
 * beside the spot by the player's index (gSpotLineupOffsets) and starts
 * PL_ACT_BED_LIE_DOWN. Other kinds, or none, start PL_ACT_TRADE when a trader
 * is near (Player_NearTrader).
 */
void Player_StartSpotAction(Player *pl, s32 mode)
{
    s8 kind;
    u16 angle;
    f32 pos[3];
    f32 worldOffset[3];
    f32 localOffset[3];
    s32 ang[3];
    f32 rotMtx[16];

    switch (Player_FindActionSpot((Player *)pl, (f32 *)pos, &angle, (u8 *)&kind)) {
    case 0:
    case 2:
    case 6:
    case 8:
    default:
        if ((u8) Player_NearTrader(pl)) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TRADE, 0);
        }
        break;
    case 1:
        Vec_Copy(pl->moveTo, pos);
        Player_StartMoveTo(pl, 0x14);
        pl->targetYaw = angle;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_START, 0);
        break;
    case 3:
        if (pl->itemSelect == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SUPPLY_BOX, 0);
        }
        break;
    case 4:
        pl->targetYaw = angle;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_REWARDS, 0);
        break;
    case 5:
        if (mode == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_MENU, 0);
            break;
        }
        Vec_Copy(pl->moveTo, pos);
        ang[0] = 0;
        ang[1] = angle;
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, rotMtx);
        Vec_Set(localOffset, 120.0f * gSpotLineupOffsets[pl->index], 0.0f, 0.0f);
        Vec_Rotate(worldOffset, localOffset, rotMtx);
        pl->moveTo[0] += worldOffset[0];
        pl->moveTo[1] += worldOffset[1];
        pl->moveTo[2] += worldOffset[2];
        pl->targetYaw = angle;
        Player_StartMoveTo(pl, 0x14);
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_LIE_DOWN, 0);
        break;
    case 9:
        if (gScene.countdownOn == 0) {
            pl->targetYaw = Vec_Heading(&pl->pos[0], pos) - 0x4000;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_START_COUNTDOWN, 0);
        }
        break;
    case 7:
        if (Quest_PhaseIsOver(0) != 1 && pl->itemSelect == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DELIVERY_BOX, 0);
        }
        break;
    }
}

/*
 * PAD_R1 held: each weapon's own action. mode 0 and 2 are for a sheathed
 * weapon (it is drawn into the action), 1 for a drawn one; mode 2 first drops
 * a carried item. Great sword: PL_REACT_GUARD_START (the lance, sword and
 * shield and gunlance draw into PL_REACT_GUARD_DRAW when sheathed); hammer:
 * Hammer_StartCharge; hunting horn: HuntingHorn_StartPlaying; dual swords:
 * the demon mode (DualSwords_DemonToggle; mode 1 can also end it); long
 * sword: LongSword_SpiritDraw; bow: Bow_DrawToAim (sheathed only).
 */
void Player_HoldSpecialInput(Player *pl, u8 mode)
{

    if (pl->input.held & PAD_R1) {
        if (mode == 0 || mode == 2) {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_START, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_GUNLANCE:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_DRAW, 0);
                break;
            case WEAPON_DUAL_SWORDS:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                if (pl->demonMode == 0) {
                    DualSwords_DemonToggle(pl, 0);
                }
                break;
            case WEAPON_HAMMER:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Hammer_StartCharge(pl);
                break;
            case WEAPON_HUNTING_HORN:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                HuntingHorn_StartPlaying(pl, 1);
                break;
            case WEAPON_LONG_SWORD:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                LongSword_SpiritDraw(pl);
                break;
            case WEAPON_BOW:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Bow_DrawToAim(pl);
                break;
            }
        } else {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            case WEAPON_LANCE:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_GUNLANCE:
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_START, 0);
                break;
            case WEAPON_HAMMER:
                Hammer_StartCharge(pl);
                break;
            case WEAPON_HUNTING_HORN:
                HuntingHorn_StartPlaying(pl, 0);
                break;
            case WEAPON_DUAL_SWORDS:
                if (pl->demonMode == 0) {
                    DualSwords_DemonToggle(pl, 0);
                } else {
                    DualSwords_DemonToggle(pl, 1);
                }
                break;
            }
        }
    }
}

/*
 * Starts gathering at a gather point or a mine point. mode 0 is PAD_CIRCLE
 * standing, 1 crouched, 2 the item button with a tool selected; nothing
 * happens near the trader (Player_NearTrader) unless mode is 2. At a gather
 * point (Player_GetGatherPoint) of kind 0, modes 0 and 1 enter PL_ACT_GATHER
 * or PL_ACT_GATHER_CROUCHED; mode 2 at kind 3 with a pickaxe selected faces
 * the point and enters PL_ACT_MINE, at kind 4 with a net PL_ACT_BUG_NET, and
 * the wrong tool plays sound 0x15. In modes 0 and 1 a mine point under the
 * player (MinePoint_FindNearPlayer) then enters PL_ACT_GATHER_DROP,
 * PL_ACT_GATHER_SPECIAL (type 5) or PL_ACT_CARVE (type 2), or their crouched
 * forms.
 */
void Player_TryGather(Player *pl, s32 mode)
{
    f32 pos[3];
    u16 kind;
    u16 id;
    u16 mineType;

    if ((u8) Player_NearTrader(pl) && (u8) mode != 2) {
        return;
    }
    if (Player_GetGatherPoint(pl, &kind, pos) == 0xFFFF) {
        goto other;
    }
    switch (kind) {
    case 0:
        if ((u8) mode == 2) {
            goto other;
        }
        if ((u8) mode == 0) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER, 0);
        } else {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_CROUCHED, 0);
        }
        break;
    case 3:
        if ((u8) mode == 2) {
            switch (pl->pouch[pl->pouchCursor].item) {
            case 150:
            case 151:
            case 152:
                pl->targetYaw = Vec_Heading(&pl->pos[0], pos) - 0x4000;
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_MINE, 0);
                break;
            case 153:
            case 154:
            case 155:
                Sound_PlaySeFlat(0, 0x15, 0);
                break;
            }
        }
        break;
    case 4:
        if ((u8) mode == 2) {
            id = pl->pouch[pl->pouchCursor].item;
            switch (id) {
            case 153:
            case 154:
            case 155:
                pl->useItem = id;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_BUG_NET, 0);
                break;
            case 150:
            case 151:
            case 152:
                Sound_PlaySeFlat(0, 0x15, 0);
                break;
            }
        }
        break;
    default:
    other:
        switch (pl->pouch[pl->pouchCursor].item) {
        case 150:
        case 151:
        case 152:
        case 153:
        case 154:
        case 155:
            if ((u8) mode == 2) {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
            break;
        }
        break;
    }
    if ((u8) mode != 2) {
        mineType = MinePoint_FindNearPlayer(pl);
        if (mineType != 0xFFFF) {
            if ((u8) mode == 0) {
                switch (mineType) {
                default:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_DROP, 0);
                    break;
                case 5:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_SPECIAL, 0);
                    break;
                case 2:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CARVE, 0);
                    break;
                }
            } else {
                switch (mineType) {
                default:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_DROP_CROUCHED, 0);
                    break;
                case 5:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_SPECIAL_CROUCHED, 0);
                    break;
                case 2:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CARVE_CROUCHED, 0);
                    break;
                }
            }
        }
    }
}

/*
 * The input of the crouched states; always returns 0 (another player's copy
 * online does nothing). By gScene.questPhase it requests the quest's start
 * (phase 4, while questStartDone is 0), end (5, 6), abort (7) or leave (8) event
 * state. Otherwise it sets freeState and, by Player_StickMoveLevel, walks
 * crouched (PL_ACT_CROUCH_WALK, or PL_ACT_CROUCH_WALK_FAST at level 3) or
 * stops (PL_ACT_CROUCH_STOP); PAD_CROSS stands up (PL_ACT_CROUCH_STAND_UP)
 * unless mustCrouch is set. Unless the quest is over, it then starts requested
 * gestures, draws the weapon on a right-stick flick (PL_ACT_DRAW), runs
 * Player_HoldSpecialInput, uses an item on PAD_SQUARE (Player_TryGather mode
 * 2, Player_UseItem) and handles PAD_CIRCLE (an offered item, a spot,
 * gathering) or a pending offer (PL_ACT_OFFER_ITEM).
 */
s32 Player_CrouchInput(Player *pl)
{

    if (Player_IsLocal(pl) == 0 && Game_IsOnline() == 1) {
        return 0;
    }
    switch (gScene.questPhase) {
    case 4:
        if (pl->questStartDone == 0) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_START, 0);
            return 0;
        }
        break;
    case 6:
    case 5:
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_END, 0);
        return 0;
    case 7:
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_ABORT, 0);
        return 0;
    case 8:
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 0);
        return 0;
    }
    pl->freeState = 1;
    switch ((u8) Player_StickMoveLevel(pl, 0)) {
    case 1:
    case 2:
        Player_FaceStick(pl);
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK) == 0) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST) != 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_STOP, 0);
            } else {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    case 3:
        Player_FaceStick(pl);
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST) == 0) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST, 0);
        }
        break;
    case 0:
    default:
        if ((Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST) != 0) && pl->timer == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_STOP, 0);
        }
        break;
    }
    if ((pl->input.pressed & PAD_CROSS) && pl->mustCrouch == 0) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_STAND_UP, 0);
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (pl->actRequest != 0) {
        Player_StartGesture(pl);
    }
    if (pl->mustCrouch == 0) {
        if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
        }
        Player_HoldSpecialInput(pl, 0);
    }
    if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
        Player_TryGather(pl, 2);
        Player_UseItem(pl, 0);
        pl->pouchHudFlags |= 1;
    }
    if (pl->input.pressed & PAD_CIRCLE) {
        if (Player_TakeOfferedItem(pl) == 0) {
            Player_StartSpotAction(pl, 0);
            Player_TryGather(pl, 1);
        }
    } else if (pl->offerRequest != 0) {
        pl->freeState = 0;
        pl->offerStarted = 1;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM, PL_SET_SYNC_OFFER);
    }
    return 0;
}

/* The weapons input checks (the states cancel into another action), for the local player or a human one
 * when input is allowed. */
void Player_WeaponAttackInput(Player *pl, s32 arg)
{

    if ((Player_IsLocal(pl) != 0 || Game_IsOnline() != 1) && pl->itemSelect == 0 && Quest_PhaseIsOver(1) != 1) {
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
            GreatSword_AttackInput(pl);
            break;
        case WEAPON_LONG_SWORD:
            LongSword_AttackInput(pl);
            break;
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_LIGHT_BOWGUN:
            Bowgun_AttackInput(pl);
            break;
        case WEAPON_BOW:
            Bow_AttackInput(pl, arg);
            break;
        case WEAPON_HAMMER:
            Hammer_AttackInput(pl);
            break;
        case WEAPON_HUNTING_HORN:
            HuntingHorn_AttackInput(pl);
            break;
        case WEAPON_LANCE:
            Lance_AttackInput(pl);
            break;
        case WEAPON_GUNLANCE:
            Gunlance_AttackInput(pl);
            break;
        case WEAPON_SWORD_SHIELD:
            SwordShield_AttackInput(pl);
            break;
        case WEAPON_DUAL_SWORDS:
            DualSwords_AttackInput(pl);
            return;
        }
    }
}

/*
 * The input while the guard is held, for the local player (any player
 * offline) before the quest's end. Lance: the stick walks (PL_ACT_GUARD_WALK,
 * unless motion 0x3F3 plays), PAD_R3 and a right-stick flick start
 * Lance_GuardAttack (0 and 1). Gunlance: Gunlance_GuardInput, else the same
 * guard walk. Sword and shield: PAD_CROSS with stamina 75 or more rolls
 * (PL_ACT_ROLL), a flick up gives SwordShield_GuardSlash. Great sword and
 * long sword: PAD_CROSS with stamina 75 or more rolls.
 */
void Player_GuardInput(Player *pl)
{
    u8 weapon;

    if (((Player_IsLocal(pl) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1)) {
        weapon = pl->weaponType;
        switch (weapon) {
        case 3:
            if ((Player_StickMoveLevel(pl, 0) & 0xFF) && (pl->motionId[0] != 0x3F3)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GUARD_WALK, PL_SET_SNAP_POS);
            }
            if (pl->input.pressed & PAD_R3) {
                Lance_GuardAttack(pl, 0);
            }
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                Lance_GuardAttack(pl, 1);
            }
            break;
        case 9:
            if ((Gunlance_GuardInput(pl, 0) == 0) && (Player_StickMoveLevel(pl, 0) & 0xFF) && (pl->motionId[0] != 0x3F3)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GUARD_WALK, PL_SET_SNAP_POS);
            }
            break;
        case 4:
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
            if (pl->input.stickDirPressed & PAD_RSTICK_UP) {
                SwordShield_GuardSlash(pl);
            }
            break;
        case 0:
        case 7:
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
            break;
        }
    }
}

/*
 * Turns the player to run along the wall it touches (rot[1] and targetYaw set
 * to the heading of the wall's normal plus or minus 0x4000); returns 0 for none (not
 * in the current area, or no wall hit), 1 or 2. It reads the first of the
 * player's wall contacts (gEmWallContacts, ended by flags 0): a contact with
 * WALL_CONTACT_SPECIAL and a wall ahead (WALL_DIRS_AHEAD) turns by
 * -0x4000 for side 0 or +0x4000 for side 1 and returns 1; any other contact
 * turns by +0x4000 only for side 1 and returns 2. Player_CheckClimb starts
 * PL_ACT_WALL_GRAB on 1.
 */
s8 Player_TurnAlongWall(Player *pl, s16 side)
{
    s16 i;
    u16 flags;
    u16 ang;

    if ((u8)Obj_InCurrentArea(pl) == 0) {
        return 0;
    }
    if (pl->wallHitDirs != 0) {
        for (i = 0; i < 10; i++) {
            flags = gEmWallContacts[pl->index][i].flags;
            if (flags == 0) {
                break;
            }
            ang = Math_Heading(gEmWallContacts[pl->index][i].normal[0], gEmWallContacts[pl->index][i].normal[2], 0.0f, 0.0f);
            if ((flags & WALL_CONTACT_SPECIAL) && (pl->wallHitDirs & WALL_DIRS_AHEAD)) {
                if (side == 0) {
                    pl->rot[1] = (u16) (ang - 0x4000);
                } else {
                    pl->rot[1] = (u16) (ang + 0x4000);
                }
                pl->targetYaw = pl->rot[1];
                return 1;
            }
            if (flags != 0) {
                if (side == 1) {
                    pl->rot[1] = (u16) (ang + 0x4000);
                    pl->targetYaw = pl->rot[1];
                }
                return 2;
            }
        }
    }
    return 0;
}

/*
 * As Player_TurnAlongWall for the wall at one side: side 0 needs a wall ahead
 * (WALL_DIRS_AHEAD), side 1 one behind (WALL_DIRS_BEHIND). Side 0
 * with a WALL_CONTACT_SPECIAL contact turns by -0x4000 and returns 1, side 1 turns by
 * +0x4000 and returns 2. Returns 0 otherwise.
 */
s32 Player_TurnAlongWallSide(Player *pl, s16 side)
{
    s16 i;
    u16 flags;
    u16 ang;

    if ((u8)Obj_InCurrentArea(pl) == 0) {
        return 0;
    }
    if (pl->wallHitDirs & (side == 0 ? WALL_DIRS_AHEAD : WALL_DIRS_BEHIND)) {
        for (i = 0; i < 10; i++) {
            flags = gEmWallContacts[pl->index][i].flags;
            if (flags == 0) {
                break;
            }
            ang = Math_Heading(gEmWallContacts[pl->index][i].normal[0], gEmWallContacts[pl->index][i].normal[2], 0.0f, 0.0f);
            if ((flags & WALL_CONTACT_SPECIAL) && side == 0) {
                pl->rot[1] = (u16) (ang - 0x4000);
                pl->targetYaw = pl->rot[1];
                return 1;
            }
            if (side == 1) {
                pl->rot[1] = (u16) (ang + 0x4000);
                pl->targetYaw = pl->rot[1];
                return 2;
            }
        }
    }
    return 0;
}

/*
 * Starts a climb or a hop when the player runs at a wall or a ledge; returns
 * 1 when it started a state, else 0. Player_TurnAlongWall returning 1 enters
 * PL_ACT_WALL_GRAB. The floor 55 units ahead (Player_FloorAheadAbove) decides
 * the rest: 60 to 180 above the player requests PL_ACT_STEP_UP, 180 to 210
 * PL_ACT_LEDGE_GRAB, 210 to 300 PL_ACT_JUMP_TO_LEDGE.
 */
u8 Player_CheckClimb(Player *pl)
{
    f32 dist;
    u8 ret = 0;

    switch (Player_TurnAlongWall(pl, 0)) {
    case 0:
        break;
    case 1:
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_GRAB, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
        break;
    }
    if (Player_FloorAheadAbove(pl, &dist, 55.0f, 60.0f, 120.0f) != 0) {
        pl->targetYaw = pl->rot[1];
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_STEP_UP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
    } else if (Player_FloorAheadAbove(pl, &dist, 55.0f, 180.0f, 30.0f) != 0) {
        pl->targetYaw = pl->rot[1];
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
    } else if (Player_FloorAheadAbove(pl, &dist, 55.0f, 210.0f, 90.0f) != 0) {
        pl->targetYaw = pl->rot[1];
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_JUMP_TO_LEDGE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
    }
    return ret;
}

/* Uses the selected pouch item: in mode 2 only ITEM_ID_MONSTER_BOMB (when Item_RideItemUsable
 * allows it); otherwise each item checks where it may be used (out of camp, a free mine point,
 * an action spot, no status 6) and starts its action state, or plays the refusal sound. */
void Player_UseItem(Player *pl, u8 mode)
{
    f32 spotPos[3];
    u16 ang;
    u8 k;

    if (Pouch_CursorCount(pl) <= 0) {
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        return;
    }
    if (mode == 2) {
        switch (pl->pouch[pl->pouchCursor].item) {
        case ITEM_ID_MONSTER_BOMB:
            if (Item_RideItemUsable() == 1) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            } else {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
            break;
        default:
            Sound_PlaySeFlat(0, 0x15, 0);
            break;
        }
        return;
    }
    switch (pl->pouch[pl->pouchCursor].item) {
    case 0x4C:
    case 0x4B:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        if (!Player_TestFlag(pl, 0x2000600)) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM_ALT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        break;
    case 0x4D:
    case 0x2BA:
    case 0x4F:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM_ALT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case 0x4E:
        if (Item_UsableOutsideCamp() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RETURN_TO_CAMP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0x48:
    case 0x297:
    case 0x47:
    case 0xD7:
    case 0x49:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x2BB:
    case 0x2BC:
    case 0x40:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case 0x50:
    case 0x45:
        if (Quest_IsVersus() == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        break;
    case ITEM_ID_SMALL_BARREL_BOMB:
    case ITEM_ID_SMALL_BARREL_BOMB_B:
    case ITEM_ID_LAUNCHED_BARREL_BOMB:
    case ITEM_ID_LAUNCHED_BARREL_BOMB_B:
        if (Item_CanUseSmallBarrelBomb() == 1) {
            goto drink;
        }
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    case ITEM_ID_NO_BOMB_AREA_ITEM:
        if (Scene_InNoBarrelBombArea() == 1) {
            goto drink;
        }
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    case ITEM_ID_MEAT:
    case ITEM_ID_BAIT_POISON:
    case ITEM_ID_BAIT_STATUS2:
    case ITEM_ID_BAIT_STATUS4:
        if (Scene_CanPlaceKind3() == 1) {
        drink:
            if (pl->action == PL_POSTURE_STANDING) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_KNEEL_TO_PLACE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            } else if (pl->action == PL_POSTURE_CROUCHED) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_KNEEL_TO_PLACE_CROUCHED, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_LARGE_BARREL_BOMB:
    case ITEM_ID_LARGE_BARREL_BOMB_B:
    case ITEM_ID_SUPPLY_LARGE_BARREL_BOMB:
        if (Item_CanPlaceLargeBarrelBomb() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_PLACED_BOMB:
        if (Item_CanPlaceKind1() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_TRAP:
    case ITEM_ID_SUPPLY_TRAP:
        if (Item_CanPlaceKind2() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_PITFALL_TRAP:
    case ITEM_ID_SUPPLY_PITFALL_TRAP:
        if (Player_GetAreaPitModel(pl) >= 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0x2F8:
    case 0x2F9:
    case 0x30A:
    case 0x30B:
    case 0x30D:
    case 0x2FA:
    case 0x2FB:
    case 0x2FC:
    case 0x2FF:
    case 0x300:
    case 0x301:
    case 0x306:
    case 0x307:
    case 0x308:
    case 0x309:
    case 0x314:
    case 0x318:
    case 0x30E:
    case 0x30F:
    case 0x310:
    case 0x311:
    case 0x312:
    case 0x315:
    case 0x316:
    case 0x317:
    case 0x30C:
    case 0x319:
    case 0x313:
        if (SpecialMine_IsFree(pl) == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0x8E:
    case 0x8F:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 2) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            pl->targetYaw = ang;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_CAST, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_DELIVERY:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 0xB) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0x29B:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 6) {
            pl->moveTo[0] = spotPos[0];
            pl->moveTo[1] = spotPos[1];
            pl->moveTo[2] = spotPos[2];
            pl->targetYaw = ang;
            Player_StartMoveTo(pl, 0x14);
            pl->stateRand = k;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM, PL_SET_KEEP_RAND);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0x2B7:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 0xA) {
            pl->targetYaw = Vec_Heading(pl->pos, spotPos) - 0x4000;
            pl->stateRand = k;
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPOT_ITEM_STRIKE, PL_SET_KEEP_RAND);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_HEAL_20:
    case ITEM_ID_CURE_POISON:
    case ITEM_ID_HEAL_10_CURE:
    case ITEM_ID_HEAL_30:
    case ITEM_ID_HEAL_50:
    case ITEM_ID_SUPPLY_HEAL_30:
    case ITEM_ID_MAX_HP_10:
    case ITEM_ID_MAX_HP_20:
    case ITEM_ID_MAX_HP_FULL:
    case ITEM_ID_SUPPLY_MAX_HP_FULL:
    case ITEM_ID_MAX_ALL:
    case ITEM_ID_STAMINA_LOCK:
    case ITEM_ID_STAMINA_LOCK_LONG:
    case ITEM_ID_ATTACK_DRUG:
    case ITEM_ID_ATTACK_DRUG_STRONG:
    case ITEM_ID_DEFENSE_DRUG:
    case ITEM_ID_DEFENSE_DRUG_STRONG:
    case ITEM_ID_SUPPLY_MAX_STAMINA_75:
    case ITEM_ID_HURT_CURE_CHANCE:
    case ITEM_ID_CURE_CHANCE:
    case ITEM_ID_POISON_GAMBLE:
    case ITEM_ID_ATTACK_SHARED:
    case ITEM_ID_DEFENSE_SHARED:
    case ITEM_ID_COOL_DRINK:
    case ITEM_ID_HOT_DRINK:
    case ITEM_ID_HEAL_50_TEAM:
    case ITEM_ID_DETECT:
    case ITEM_ID_REGEN:
    case ITEM_ID_MAX_STAMINA_75_B:
    case ITEM_ID_ATTACK_BURST:
    case ITEM_ID_DEFENSE_BURST:
    case ITEM_ID_MAX_STAMINA_75:
    case ITEM_ID_RANDOM:
        if (Player_Status6Active(pl) == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DRINK, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0x294:
    case 0x9C:
    case 0x9D:
    case 0x9F:
    case 0x9E:
    case 0xA0:
        if (Pouch_HasMeat(pl) == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_START, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA3:
    case 0xA8:
    case 0xA9:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_START, 0);
        break;
    case 0xAC:
    case 0x295:
    case 0xEA:
    case 0xF2:
        if (pl->isGunner != 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SHARPEN, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0xAF:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_HORN_PLAY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case ITEM_ID_TEAM_HEAL:
    case ITEM_ID_TEAM_CURE:
    case ITEM_ID_TEAM_ATTACK:
    case ITEM_ID_TEAM_DEFENSE:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_TEAM_ITEM_USE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case ITEM_ID_FOOD_STAMINA_75:
    case ITEM_ID_FOOD_STAMINA_150:
    case ITEM_ID_FOOD_STAMINA_FULL:
    case ITEM_ID_FOOD_STAMINA_GAMBLE:
    case ITEM_ID_FOOD_HEAL_20:
    case ITEM_ID_FOOD_PRIZE_A:
    case ITEM_ID_FOOD_PRIZE_B:
    case ITEM_ID_FOOD_HOT:
    case ITEM_ID_FOOD_HOT_STAMINA:
    case ITEM_ID_FOOD_COOL:
    case ITEM_ID_FOOD_COOL_STAMINA:
    case ITEM_ID_FOOD_CURE:
    case ITEM_ID_FOOD_CURE_STAMINA:
    case ITEM_ID_FOOD_RESTORE:
    case ITEM_ID_FOOD_REGEN_SHORT:
    case ITEM_ID_FOOD_REGEN:
    case ITEM_ID_FOOD_REGEN_B:
    case ITEM_ID_FOOD_REGEN_LONG:
    case ITEM_ID_FOOD_RESTORE_B:
        if (Player_Status6Active(pl) == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_EAT, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0xAD:
        if (Player_LocalCanUseBinoculars() == 1) {
            if (pl->action == PL_POSTURE_CROUCHED) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED, 0);
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS, 0);
            }
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 0x96:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9A:
    case 0x9B:
        break;
    case 0x2B0:
    case 0x2B1:
    case 0x2B2:
    case 0x2B3:
    case 0x2B4:
    case 0x2B5:
    case 0x2B6:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_HORN_ITEM_PLAY, 0);
        break;
    default:
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

extern s32 gPouchFullTakeText;
/*
 * Returns 1 when the player starts taking an item another player holds out to
 * it, else 0 (always 0 offline, and while pl->offerNoticeTimer runs). The
 * giver is another active player in the same area, in PL_ACT_OFFER_ITEM,
 * offering to this player (offerTo), within 300 units, with an item of rarity
 * below 3. If the pouch has room it copies offerItem and offerCount, tells
 * the giver when the player is local (QSYNC_PL_ITEM_ASK) and requests
 * PL_ACT_TAKE_ITEM_WAIT; otherwise it pushes a notice and sets
 * offerNoticeTimer to 90.
 */
s32 Player_TakeOfferedItem(Player *pl)
{
    s16 i;
    Player *giver;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (pl->offerNoticeTimer != 0) {
        return 0;
    }
    giver = gPlayers;
    for (i = 0; i < gScene.playerCount; i++, giver++) {
        if (i != pl->index && giver->active != 0 && giver->area == pl->area && Player_IsState(giver, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM) != 0
            && giver->offerTo == pl->index && Vec_Distance(pl->pos, giver->pos) <= 300.0f && gItemData[giver->offerItem].rarity < 3) {
            if (Pouch_Count(pl, giver->offerItem) == 0) {
                if (Pouch_HasFreeSlot(pl) != 0) {
                    pl->offerItem = giver->offerItem;
                    pl->offerCount = giver->offerCount;
                    if (Player_IsLocal(pl) == 1) {
                        QSync_SendPlayer(pl, QSYNC_PL_ITEM_ASK, giver->index);
                    }
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM_WAIT, 0);
                    return 1;
                }
                Notice_PushItem((char *)gPouchFullTakeText);
                pl->offerNoticeTimer = 0x5A;
            } else if (Pouch_Room(pl, giver->offerItem) >= giver->offerCount) {
                pl->offerItem = giver->offerItem;
                pl->offerCount = giver->offerCount;
                if (Player_IsLocal(pl) == 1) {
                    QSync_SendPlayer(pl, QSYNC_PL_ITEM_ASK, giver->index);
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM_WAIT, 0);
                return 1;
            } else {
                Notice_Push(NOTICE_KIND_ITEM, 0xF, giver->offerItem);
                pl->offerNoticeTimer = 0x5A;
            }
            return 0;
        }
    }
    return 0;
}

/*
 * The idle input of the local player (others return at once);
 * Player_StateIdle calls it with arg 0, Player_StateMove with 1 (variant 9),
 * and arg goes to Bow_AttackInput as needPress. By gScene.questPhase it
 * requests the quest's start, end, abort or leave state. Riding a monster
 * (rideState) it throws the player off (PL_REACT_TOSSED_BACK) while the giant
 * it stands on has quakeTimer set, else walks (PL_ACT_RIDE_WALK) and handles
 * the offer, spot, gather and item buttons. On foot with the weapon drawn it
 * walks (PL_ACT_WALK_DRAWN), sheathes on PAD_CIRCLE, PAD_SQUARE or PAD_R2
 * with the stick, and runs Player_HoldSpecialInput and
 * Player_WeaponAttackInput. Sheathed, Player_StickMoveLevel picks the walks,
 * the run (or PL_ACT_TURN_AROUND) or the dash (PL_ACT_FLEE_DASH when fighting
 * a monster not ahead, else PL_ACT_DASH); PAD_CROSS rolls or crouches,
 * PAD_SQUARE gathers or uses an item, PAD_CIRCLE climbs or takes the offer,
 * spot or gather point, a right-stick flick draws or attacks as it draws, and
 * PAD_R3 starts PL_WPN_SHARED_ATTACK. Both also start an item offer, the
 * battle alert (PL_ACT_BATTLE_ALERT, when Player_EnteredBattle returns 1),
 * the stun (PL_REACT_STUN) and the mash status (PL_REACT_MASH_START).
 */
void Player_IdleActions(Player *pl, s32 arg)
{
    Em *monster = gEms;
    s16 i;
    u8 k;
    u8 stickLevel;
    u8 drew;
    u16 yawDelta;

    if (Game_IsOnline() == 0) {
        if (Player_IsLocal(pl) == 0) {
            return;
        }
    } else if (Player_IsLocal(pl) == 0) {
        return;
    }
    drew = 0;
    if (gScene.questPhase == 4 && pl->questStartDone == 0) {
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_START, 0);
        return;
    }
    pl->freeState = 1;
    if (pl->rideState != 0) {
        k = pl->rideEm;
        for (i = 0; i < 20; i++, monster++) {
            if (k == monster->slotNo && (monster->type == 7 || monster->type == 0x32 || monster->type == 0x3A || monster->type == 0x48 ||
                                   monster->type == 0x37)) {
                if (monster->quakeTimer != 0) {
                    Player_SetState(pl, PL_GROUP_REACT, PL_REACT_TOSSED_BACK, 0);
                    return;
                }
                break;
            }
        }
        if ((u8) Player_StickMoveLevel(pl, 1) > 0 && pl->state != 0x12) {
            Player_FaceStick(pl);
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RIDE_WALK, 0);
        }
        Player_CheckRideSlip(pl);
        if (Quest_PhaseIsOver(1) == 1) {
            return;
        }
        if (pl->input.pressed & PAD_CIRCLE) {
            if (Player_TakeOfferedItem(pl) == 0) {
                Player_StartSpotAction(pl, 0);
                Player_TryGather(pl, 0);
            }
        }
        if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
            Player_UseItem(pl, 2);
            pl->pouchHudFlags |= 1;
        }
    } else {
        if (gScene.questPhase == 6 || gScene.questPhase == 5) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_END, 0);
            return;
        }
        if (gScene.questPhase == 7) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_ABORT, 0);
            return;
        }
        if (gScene.questPhase == 8) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 0);
            return;
        }
        if (pl->weaponOut != 0) {
            stickLevel = Player_StickMoveLevel(pl, 1);
            switch (stickLevel) {
            case 0:
                break;
            case 1:
            case 2:
            case 3:
                if (pl->state != 0x24 && pl->state != 3 && pl->state != 0x74 && pl->mustCrouch == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK_DRAWN, 0);
                }
                break;
            case 4:
                break;
            }
            if (stickLevel != 0 && pl->mustCrouch != 0 && pl->state != 0x3B) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
                if ((pl->input.pressed & PAD_CIRCLE) || ((pl->input.pressed & PAD_R2) && pl->input.stickTilt >= PL_STICK_HELD)) {
                    if (Player_TestFlag(pl, 0x1200) == 0) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE_MOVING, 0);
                    }
                } else if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
                    Player_TryGather(pl, 2);
                    Player_UseItem(pl, 0);
                    pl->pouchHudFlags |= 1;
                }
                break;
            default:
                if ((pl->input.pressed & (PAD_SQUARE | PAD_CIRCLE)) || ((pl->input.pressed & PAD_R2) && pl->input.stickTilt >= PL_STICK_HELD)) {
                    if (Player_TestFlag(pl, 0x1200) == 0) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE_MOVING, 0);
                    }
                    pl->pouchHudFlags |= 1;
                }
                break;
            }
            Player_HoldSpecialInput(pl, 1);
            if ((Player_AmmoTypeChanged(pl) & 0xFF) == 0) {
                Player_WeaponAttackInput(pl, arg);
            }
            if (pl->offerRequest != 0) {
                pl->freeState = 0;
                pl->offerStarted = 1;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM, PL_SET_SYNC_OFFER);
            }
            if (Player_Status5Full(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_STUN, 0);
                return;
            }
            if (Player_HasMashBuildup(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_START, 0);
            }
        } else {
            stickLevel = Player_StickMoveLevel(pl, 0);
            switch (stickLevel) {
            case 0:
                break;
            case 1:
                if (pl->state != 0x49 && pl->state != 0x24 && pl->mustCrouch == 0) {
                    Player_FaceStick(pl);
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK_SLOW, 0);
                }
                break;
            case 2:
                if (pl->state != 2 && pl->state != 0x24 && pl->mustCrouch == 0) {
                    Player_FaceStick(pl);
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK, 0);
                }
                break;
            case 3:
                if (pl->state != 1 && pl->state != 0x3F && pl->state != 0x13 && pl->state != 0x24 &&
                    pl->state != 0x1F && pl->mustCrouch == 0) {
                    Player_FaceStick(pl);
                    yawDelta = pl->targetYaw + 0x10000 - pl->rot[1];
                    if (yawDelta >= 0x6000U && yawDelta < 0xA001U) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_TURN_AROUND, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN, 0);
                    }
                }
                break;
            case 4:
                break;
            case 5:
                if (Player_TestFlag(pl, 0x200) == 0) {
                    if (pl->inBattle != 0 && (u16) (pl->targetAngle - pl->rot[1] + 0x2000) > 0x4000) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DASH, 0);
                    }
                    drew = 1;
                }
                break;
            }
            if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
            if (stickLevel != 0 && pl->mustCrouch != 0 && pl->state != 0x3B) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (pl->input.pressed & PAD_CROSS) {
                if (pl->input.stickTilt >= 0x55) {
                    if (pl->stamina >= 0x4B) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                    }
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, 0);
                }
            }
            if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
                Player_TryGather(pl, 2);
                Player_UseItem(pl, 0);
                pl->pouchHudFlags |= 1;
            }
            Player_HoldSpecialInput(pl, 0);
            if ((drew == 0 ? pl->input.pressed : pl->input.held) & 0x20) {
                Player_CheckClimb(pl);
            }
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                if (Player_TestFlag(pl, 0x2000600) == 0) {
                    if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                        Bowgun_DrawAttack(pl, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
                    }
                } else {
                    switch (pl->weaponType) {
                    case WEAPON_GREAT_SWORD:
                    default:
                        GreatSword_DrawAttack(pl);
                        break;
                    case WEAPON_HEAVY_BOWGUN:
                    case WEAPON_LIGHT_BOWGUN:
                        Bowgun_DrawAttack(pl, 1);
                        break;
                    case WEAPON_BOW:
                        Bow_DrawAttack(pl);
                        break;
                    case WEAPON_SWORD_SHIELD:
                        SwordShield_DrawAttack(pl);
                        break;
                    case WEAPON_DUAL_SWORDS:
                        DualSwords_DrawAttack(pl);
                        break;
                    case WEAPON_LANCE:
                        Lance_DrawAttack(pl);
                        break;
                    case WEAPON_GUNLANCE:
                        Gunlance_DrawAttack(pl);
                        break;
                    case WEAPON_HAMMER:
                        Hammer_DrawAttack(pl);
                        break;
                    case WEAPON_HUNTING_HORN:
                        HuntingHorn_DrawAttack(pl);
                        break;
                    case WEAPON_LONG_SWORD:
                        LongSword_DrawAttack(pl);
                        break;
                    }
                }
            }
            if (pl->input.pressed & PAD_R3) {
                Player_SetState(pl, PL_GROUP_WEAPON, PL_WPN_SHARED_ATTACK, PL_SET_SNAP_POS);
            }
            if (pl->input.pressed & PAD_CIRCLE) {
                if (Player_TakeOfferedItem(pl) == 0) {
                    Player_StartSpotAction(pl, 0);
                    Player_TryGather(pl, 0);
                }
            } else if (pl->offerRequest != 0) {
                pl->freeState = 0;
                pl->offerStarted = 1;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM, PL_SET_SYNC_OFFER);
            }
            if ((Player_EnteredBattle((Em *) pl) & 0xFF) == 1) {
                pl->freeState = 0;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_BATTLE_ALERT, 0);
                CamCtl_SetTargetObj((u8 *) pl);
            }
            if (Player_Status5Full(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_STUN, 0);
                return;
            }
            if (Player_HasMashBuildup(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_START, 0);
            }
        }
    }
}

/*
 * The aim of a bowgun in its drawn idle (motion 0x3E9) for the local gunner.
 * It sets aimLockTimer (10, or 5 after a vertical change) while the aim mode
 * or a recent aim holds, and blends the aim layers (Player_SetAimLayers,
 * motions 0x3ED and 0x3EE); without them it needs PAD_L1 held and PAD_L2 not.
 * The D-pad up and down change pl->aimWeight by 5 (Bowgun_AddAimWeight,
 * reversed by gScene.aimReverse in the aim mode), left and right turn
 * pl->targetYaw by 0x240. A left-stick tilt of 0x28 or more does the same in
 * steps that grow with the tilt.
 */
void Player_BowgunAim(Player *pl)
{
    s8 stickStep;
    s8 dpadStep;
    u16 yawStep;

    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (pl->isGunner != 1) {
        return;
    }
    if (pl->motionId[0] != 0x3E9) {
        return;
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        pl->aimLockTimer = 10;
    }
    Player_SetAimLayers(pl, 0x3ED, 0x3EE);
    if (pl->aimLockTimer == 0 && pl->aimMode == 0) {
        if (pl->input.held & PAD_L2) {
            return;
        }
        if (!(pl->input.held & PAD_L1)) {
            return;
        }
    }
    if (pl->input.held & (PAD_UP | PAD_DOWN)) {
        dpadStep = 5;
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            dpadStep *= -1;
        }
        if (pl->input.held & PAD_UP) {
            Bowgun_AddAimWeight(pl, dpadStep);
        } else {
            Bowgun_AddAimWeight(pl, (s8) -dpadStep);
        }
        pl->aimLockTimer = 5;
    }
    if (pl->input.held & PAD_LEFT) {
        pl->aimLockTimer = 10;
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            pl->targetYaw -= 0x240;
        } else {
            pl->targetYaw += 0x240;
        }
    }
    if (pl->input.held & PAD_RIGHT) {
        pl->aimLockTimer = 10;
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            pl->targetYaw += 0x240;
        } else {
            pl->targetYaw -= 0x240;
        }
    }
    Player_SetAimLayers(pl, 0x3ED, 0x3EE);
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        if (!(pl->input.stickDir & PAD_DPAD) || pl->input.stickTilt < PL_STICK_HELD) {
            return;
        }
        stickStep = 1;
        yawStep = 0x4C;
        if (pl->input.stickTilt >= 0x78) {
            stickStep = 8;
            yawStep = 0x200;
        } else if (pl->input.stickTilt >= 0x5A) {
            stickStep = 4;
            yawStep = 0xC0;
        } else if (pl->input.stickTilt >= 0x3C) {
            stickStep = 2;
            yawStep = 0x80;
        }
        if (pl->input.stickDir & (PAD_UP | PAD_DOWN)) {
            if (gScene.aimReverse != 0) {
                stickStep = -stickStep;
            }
            stickStep = (s8) stickStep;
            if (pl->input.stickDir & PAD_UP) {
                Bowgun_AddAimWeight(pl, stickStep);
            } else {
                Bowgun_AddAimWeight(pl, (s8) -stickStep);
            }
            pl->aimLockTimer = 5;
            Player_SetAimLayers(pl, 0x3ED, 0x3EE);
        }
        if (pl->input.stickDir & PAD_LEFT) {
            pl->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                pl->targetYaw -= yawStep;
            } else {
                pl->targetYaw += yawStep;
            }
        }
        if (pl->input.stickDir & PAD_RIGHT) {
            pl->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                pl->targetYaw += yawStep;
            } else {
                pl->targetYaw -= yawStep;
            }
        }
    }
    Player_SetAimLayers(pl, 0x3ED, 0x3EE);
}

/*
 * Adds delta to pl->aimPitch, keeping it within 0x2000 (45 degrees) either
 * way.
 */
static void Player_AddAimPitch(Player *pl, s16 delta)
{
    pl->aimPitch += delta;
    if (delta >= 0) {
        if (pl->aimPitch >= 0x2000) {
            pl->aimPitch = 0x2000;
        }
    } else {
        if (pl->aimPitch <= -0x2000) {
            pl->aimPitch = -0x2000;
        }
    }
}

/*
 * Adds delta to pl->aimPitchPct (kept within -100 to 100) and sets
 * pl->aimPitch from it: the percentage of 45 degrees upward when it is
 * positive, of 35 degrees downward when negative, as a 16-bit turn.
 */
static void Player_AddAimPitchPct(Player *pl, s8 delta)
{
    f32 fraction;
    s8 pct;

    pl->aimPitchPct += delta;
    if (pl->aimPitchPct > 100) {
        pl->aimPitchPct = 100;
    }
    if (pl->aimPitchPct < -100) {
        pl->aimPitchPct = -100;
    }
    pct = pl->aimPitchPct;
    fraction = pct / 100.0f;
    if (pct >= 0) {
        pl->aimPitch = 0.5f + (65536.0f * (45.0f * fraction)) / 360.0f;
        return;
    }
    pl->aimPitch = 0.5f + (65536.0f * (35.0f * fraction)) / 360.0f;
}

/*
 * The aim input of the local player whose current motion is mot: the
 * binoculars (motions 7 and 8) and the mounted gun (motion 0x19E). It needs
 * aimLockTimer or aimMode, else PAD_L1 held and PAD_L2 not. The D-pad up and
 * down change the pitch by 0x400 a frame (Player_AddAimPitch; the mounted gun
 * 4 percent, Player_AddAimPitchPct), left and right turn pl->targetYaw by
 * 0x180; a left-stick tilt of 0x28 or more does the same in larger steps by
 * the tilt. For the mounted gun targetYaw stays within 0x4000 of
 * pl->yawCenter. gScene.aimReverse flips the directions.
 */
void Player_UpdateLean(Player *pl, s32 mot)
{
    s8 stickStep;
    s8 dpadStep;
    u16 yawStep;
    u16 yawOffset;

    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (pl->motionId[0] != (u16) mot) {
        return;
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        pl->aimLockTimer = 10;
    }
    if (pl->aimLockTimer == 0 && pl->aimMode == 0) {
        if (pl->input.held & PAD_L2) {
            return;
        }
        if (!(pl->input.held & PAD_L1)) {
            return;
        }
    }
    if (pl->input.held & (PAD_UP | PAD_DOWN)) {
        dpadStep = 4;
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            dpadStep *= -1;
        }
        if (pl->input.held & PAD_UP) {
            if ((u16) mot != 0x19E) {
                Player_AddAimPitch(pl, dpadStep << 8);
            } else {
                Player_AddAimPitchPct(pl, dpadStep);
            }
        } else {
            if ((u16) mot != 0x19E) {
                Player_AddAimPitch(pl, -dpadStep << 8);
            } else {
                Player_AddAimPitchPct(pl, (s8) -dpadStep);
            }
        }
        pl->aimLockTimer = 5;
    }
    if (pl->input.held & PAD_LEFT) {
        pl->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            pl->targetYaw -= 0x180;
        } else {
            pl->targetYaw += 0x180;
        }
    }
    if (pl->input.held & PAD_RIGHT) {
        pl->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            pl->targetYaw += 0x180;
        } else {
            pl->targetYaw -= 0x180;
        }
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        if ((pl->input.stickDir & PAD_DPAD) && pl->input.stickTilt >= PL_STICK_HELD) {
            stickStep = 1;
            yawStep = 0x4C;
            if (pl->input.stickTilt >= 0x78) {
                stickStep = 8;
                yawStep = 0x200;
            } else if (pl->input.stickTilt >= 0x5A) {
                stickStep = 4;
                yawStep = 0xC0;
            } else if (pl->input.stickTilt >= 0x3C) {
                stickStep = 2;
                yawStep = 0x80;
            }
            if (pl->input.stickDir & (PAD_UP | PAD_DOWN)) {
                if (gScene.aimReverse != 0) {
                    stickStep = -stickStep;
                }
                stickStep = (s8) stickStep;
                if (pl->input.stickDir & PAD_UP) {
                    if ((u16) mot != 0x19E) {
                        Player_AddAimPitch(pl, stickStep << 8);
                    } else {
                        Player_AddAimPitchPct(pl, stickStep);
                    }
                } else {
                    if ((u16) mot != 0x19E) {
                        Player_AddAimPitch(pl, -stickStep << 8);
                    } else {
                        Player_AddAimPitchPct(pl, (s8) -stickStep);
                    }
                }
                pl->aimLockTimer = 5;
            }
            if (pl->input.stickDir & PAD_LEFT) {
                pl->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    pl->targetYaw -= yawStep;
                } else {
                    pl->targetYaw += yawStep;
                }
            }
            if (pl->input.stickDir & PAD_RIGHT) {
                pl->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    pl->targetYaw += yawStep;
                } else {
                    pl->targetYaw -= yawStep;
                }
            }
        }
    }
    if ((u16) mot == 0x19E) {
        yawOffset = pl->targetYaw - pl->yawCenter;
        if (yawOffset <= 0x8000U && yawOffset > 0x4000U) {
            pl->targetYaw = pl->yawCenter + 0x4000;
        } else if (yawOffset >= 0x8000U && yawOffset < 0xC000U) {
            pl->targetYaw = pl->yawCenter - 0x4000;
        }
    }
}

/*
 * PL_ACT_IDLE. By the current motion: the idle motions 1 and 0x3F switch to
 * the tired idle 0x193 when staminaMax is below 76 (and back once it is above
 * 75), motion 1 switches to 0x3F after 240 frames for gender 1; motion 0x15
 * ends in motion 5, which makes an effect every 32 frames; a gunner's drawn
 * idle 0x3E9 blends the aim layers. A bowgun user then runs Player_BowgunAim;
 * every player runs Player_IdleActions.
 */
void Player_StateIdle(Player *pl, s32 unused)
{
    u16 motion = pl->motionId[0];

    switch (motion) {
    case 1:
    case 0x3F:
        if (pl->staminaMax < 76) {
            Player_SetMotionPair(pl, PL_MOT_IDLE_TIRED, 6, 0);
        } else if (pl->motionTime >= 240 && pl->gender == 1 && motion != 0x3F) {
            Player_SetMotionPair(pl, PL_MOT_IDLE_ALT, 2, 0);
        }
        break;
    case 0x15:
        if (pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 0x90);
        }
        break;
    case 5:
        if (!(gScene.frame & 0x1F)) {
            HitSpark_SpawnOnOwner(pl, 8, 0x14, 0.1f);
        }
        break;
    case 0x193:
        if (pl->staminaMax > 75) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, 4, 0);
        }
        break;
    case 0x3E9:
        if (pl->weaponOut != 0 && pl->isGunner == 1 && pl->weaponType != WEAPON_BOW) {
            Player_SetAimLayers(pl, 0x3ED, 0x3EE);
        }
        break;
    }
    switch (pl->weaponType) {
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
        Player_BowgunAim(pl);
        break;
    }
    Player_IdleActions(pl, 0);
}

/*
 * The walks and runs. variant: 0 PL_ACT_RUN, 1 PL_ACT_WALK, 2
 * PL_ACT_WALK_DRAWN, 3 PL_ACT_RIDE_WALK, 4 PL_ACT_MOVE_KEEP_MOTION, 5
 * PL_ACT_CROUCH_WALK_ALT, 6 PL_ACT_RUN_AFTER_DASH, 7 PL_ACT_WALK_SLOW, 8
 * PL_ACT_GUARD_WALK, 9 PL_ACT_BOW_WALK. Step 0 starts the variant's motion
 * (another player's copy online only does that). Once 10 frames have passed
 * with the stick released (Player_StickMoveLevel 0) the state ends: a run in
 * PL_ACT_RUN_STOP, the drawn walk in the idle of gDrawnWalkStopIdle (variant
 * 9: BOW_AIM_RESUME), the guard walk in PL_REACT_GUARD_HOLD (also when PAD_R1
 * is let go), the rest in idle. The stick turns the player
 * (Player_FaceStick); the drawn walk takes extra steps for the hunting horn
 * and the dual swords' demon mode; Player_IdleActions (Player_GuardInput for
 * variant 8) reads the input last.
 */
void Player_StateMove(Player *pl, s32 variant)
{
    f32 move[3];
    f32 delta[3];
    u8 stickLevel;
    u8 level;

    if (pl->step == 0) {
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 10;
        pl->timer = 10;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_RUN, 6, 0);
            Player_SetFlag(pl, 0x2000000);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_WALK, 4, 0);
            break;
        case 2:
        case 9:
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
            default:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 4, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 4, 0);
                break;
            case WEAPON_HAMMER:
            case WEAPON_HUNTING_HORN:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 8, 0);
                break;
            case WEAPON_HEAVY_BOWGUN:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_LIGHT_BOWGUN:
            case WEAPON_DUAL_SWORDS:
            case WEAPON_BOW:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 6, 0);
                break;
            }
            break;
        case 3:
            Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK, 6, 0);
            break;
        case 4:
            break;
        case 5:
            Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK, 2, 0);
            pl->action = PL_POSTURE_CROUCHED;
            break;
        case 6:
            pl->dashDelay = 30;
            if (pl->motionId[0] != 3) {
                Player_SetMotionPair(pl, PL_MOT_RUN, 0, 0x26);
            }
            Player_SetFlag(pl, 0x2000000);
            break;
        case 7:
            Player_SetMotionPair(pl, PL_MOT_WALK_SLOW, 6, 0);
            break;
        case 8:
            Player_SetFlag(pl, PLF_GUARD);
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_WALK, 4, 0);
            break;
        }
        pl->motionTime = 0;
    }
    if (Player_IsLocal(pl) == 0 && Game_IsOnline() == 1) {
        return;
    }
    stickLevel = Player_StickMoveLevel(pl, pl->weaponOut) & 0xFF;
    if (pl->stateVar1 != 0) {
        pl->stateVar1--;
    }
    if (pl->timer > 0) {
        pl->timer--;
    }
    level = stickLevel;
    if (level == 0 && pl->timer == 0) {
        if (pl->motionId[0] == 0x3EC) {
            if (variant == 9) {
                Player_SetState(pl, PL_GROUP_WEAPON, BOW_AIM_RESUME, PL_SET_SNAP_POS);
            } else {
                Player_ReturnToIdle(pl, 0, gDrawnWalkStopIdle[pl->weaponType], 0);
            }
        } else if (pl->motionId[0] == 3 || pl->motionId[0] == 4) {
            if (pl->stateVar1 == 0) {
                pl->freeState = 1;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            } else {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        } else if (pl->motionId[0] == 0x3F3) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        } else {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        return;
    }
    switch (pl->motionId[0]) {
    case 2:
        if (Motion_PassedFrame(pl, 0, 108.0f)) {
            pl->stateVar0++;
            if (pl->stateVar0 >= 5) {
                pl->stateVar0 = 0;
                Player_SetMotionPair(pl, PL_MOT_WALK_VARIATION, 2, 0);
            }
        }
        break;
    case 3:
        if (level < 3) {
            pl->freeState = 1;
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            return;
        }
        break;
    case 0x1D:
        if (pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_WALK, 6, 0x24);
        }
        break;
    case 0x3F3:
        if (!(pl->input.held & PAD_R1)) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        }
        break;
    }
    if (pl->mustCrouch != 0 && (variant != 5 || variant != 8)) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
    }
    if (level != 0) {
        Player_FaceStick(pl);
    }
    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (variant == 2 && pl->weaponType == WEAPON_HUNTING_HORN && Player_IsLocal(pl) == 1 && pl->motionId[0] == 0x3EC &&
        Player_HasSongEffect(pl, PL_SONG_SPEED) == 1 && pl->songEffect[PL_SONG_SPEED] > 0) {
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 8.0f;
        Vec_Rotate(delta, move, pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
    }
    if (variant == 2 && pl->weaponType == WEAPON_DUAL_SWORDS && Player_IsLocal(pl) == 1 && pl->motionId[0] == 0x3EC &&
        pl->demonMode != 0) {
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 2.0f;
        Vec_Rotate(delta, move, pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
    }
    if (variant != 8) {
        if (variant == 9) {
            Player_IdleActions(pl, 1);
        } else {
            Player_IdleActions(pl, 0);
        }
    } else {
        Player_GuardInput(pl);
    }
}

/*
 * Draws or sheathes the weapon. variant: 0 PL_ACT_DRAW, 1 PL_ACT_SHEATHE, 2
 * PL_ACT_SHEATHE_MOVING, 3 PL_ACT_SHEATHE_TO_CROUCH, 4
 * PL_ACT_SHEATHE_TO_USE_ITEM, 5 PL_ACT_DRAW_MOVING. Step 0 takes the motion,
 * blend and new weaponOut from the weapon type's row of gDrawMotions,
 * gSheatheMotions, gSheatheMovingMotions or gDrawMovingMotions, and drops a
 * carried item. When the motion ends variant 3 requests PL_ACT_CROUCH,
 * variant 4 has the local player use the selected item (Player_UseItem, mode
 * 1), the others return to idle.
 */
void Player_StateDrawSheathe(Player *pl, u32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        switch (variant) {
        case 0:
            pl->weaponOut = gDrawMotions[pl->weaponType].flag;
            Player_SetMotionPair(pl, gDrawMotions[pl->weaponType].motion, gDrawMotions[pl->weaponType].blend, 0);
            break;
        case 1:
        case 3:
        case 4:
            pl->weaponOut = gSheatheMotions[pl->weaponType].flag;
            Player_SetMotionPair(pl, gSheatheMotions[pl->weaponType].motion, gSheatheMotions[pl->weaponType].blend, 0);
            break;
        case 2:
            Player_SetFlag(pl, 0x1000);
            pl->weaponOut = gSheatheMovingMotions[pl->weaponType].flag;
            if (pl->weaponType == WEAPON_HUNTING_HORN) {
                Player_SetMotionPair(pl, gSheatheMovingMotions[pl->weaponType].motion, gSheatheMovingMotions[pl->weaponType].blend, 0xA);
            } else {
                Player_SetMotionPair(pl, gSheatheMovingMotions[pl->weaponType].motion, gSheatheMovingMotions[pl->weaponType].blend, 0);
            }
            break;
        case 5:
            pl->weaponOut = gDrawMovingMotions[pl->weaponType].flag;
            Player_SetMotionPair(pl, gDrawMovingMotions[pl->weaponType].motion, gDrawMovingMotions[pl->weaponType].blend, 0);
            break;
        }
        if (pl->carry & PL_CARRY_HOLD_MASK) {
            pl->carry &= PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_ReturnToIdle(pl, 0, gDrawMotions[pl->weaponType].next, 0);
                break;
            case 1:
                Player_ReturnToIdle(pl, 0, gSheatheMotions[pl->weaponType].next, 0);
                break;
            case 2:
                Player_ReturnToIdle(pl, 0, gSheatheMovingMotions[pl->weaponType].next, 0);
                break;
            case 3:
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, PL_SET_NO_SYNC);
                break;
            case 4:
                if (Player_IsLocal(pl) == 1) {
                    Player_UseItem(pl, 1);
                    pl->pouchHudFlags |= 1;
                    break;
                }
                Player_ReturnToIdle(pl, 0, gDrawMotions[pl->weaponType].next, 0);
                break;
            case 5:
                Player_ReturnToIdle(pl, 0, gDrawMovingMotions[pl->weaponType].next, 0);
                break;
            }
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_LEAP_OFF_LEDGE: a jump forward off a ledge (forward 20, up
 * 18; a ridden monster is left); 1 PL_ACT_FALL (forward 8, down 10).
 * pl->timer counts the time aloft. Falling, the player grabs a ledge ahead
 * (PL_ACT_LEDGE_GRAB, with hp above 0 and fallGrabCount under 21), else lands on
 * pl->floorY: the local player at 0 health dies (Player_Die), 30 or more
 * frames aloft request PL_ACT_LAND_HARD, else a landing motion plays and the
 * state returns to idle, or resumes the dash (PL_ACT_DASH_RESUME, or
 * PL_ACT_FLEE_DASH in battle) when the dash was held on landing.
 */
void Player_StateLeapOrFall(Player *pl, s32 variant)
{
    f32 move[3];
    f32 delta[3];
    f32 ledgeY;

    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = PL_POSTURE_AIRBORNE;
        pl->aimLockTimer = 0;
        Player_ResetAim(pl);
        switch (variant) {
        default:
            Player_SetMotionPair(pl, PL_MOT_LEAP_AIRBORNE, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 0x13);
            if (pl->rideState) {
                pl->rideState = 2;
            }
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_LEAP_AIRBORNE, 8, 0x20);
            pl->rideState = 0;
            pl->vel[0] = 0.0f;
            pl->vel[2] = 8.0f;
            pl->vel[1] = -10.0f;
            pl->accel[2] = 0.0f;
            pl->accel[0] = 0.0f;
            pl->accel[1] = -0.72727275f;
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        pl->vel[0] += pl->accel[0];
        pl->vel[1] += pl->accel[1];
        pl->vel[2] += pl->accel[2];
        if (pl->vel[0] < 0.0f) {
            pl->vel[0] = 0.0f;
        }
        if (pl->vel[2] < 0.0f) {
            pl->vel[2] = 0.0f;
        }
        move[0] = pl->vel[0];
        move[1] = pl->vel[1];
        move[2] = pl->vel[2];
        Vec_Rotate(delta, move, pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[1] += delta[1];
        pl->pos[2] += delta[2];
        if (pl->vel[1] < 0.0f) {
            if (Player_FloorAheadAbove(pl, &ledgeY, 55.0f, 180.0f, 30.0f) && pl->hp > 0 && pl->fallGrabCount < 21) {
                pl->targetYaw = pl->rot[1];
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, PL_SET_SNAP_POS);
                break;
            }
            if (pl->floorY < pl->pos[1]) {
                break;
            }
            pl->pos[1] = pl->floorY;
            pl->action = PL_POSTURE_STANDING;
            pl->rideState = 0;
            if (pl->hp <= 0 && Player_IsLocal(pl) == 1) {
                Player_Die(pl);
                break;
            }
            if (pl->timer >= 30) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                break;
            }
            pl->step++;
            if (pl->weaponOut == 0 && (pl->input.held & PAD_R2) && (Player_StickMoveLevel(pl, 0) & 0xFF)) {
                pl->stateVar1 = 1;
                Player_SetMotionPair(pl, PL_MOT_LAND_INTO_DASH, 0, 0);
            } else {
                pl->stateVar1 = 0;
                Player_SetMotionPair(pl, PL_MOT_LAND, 0, 0);
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->stateVar1 == 0) {
                Player_ReturnToIdle(pl, 0, 8, 0);
            } else if (pl->inBattle == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DASH_RESUME, 0);
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH, 0);
            }
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_JUMP_TO_LEDGE (a crouch, then a jump up toward a high
 * ledge), 1 PL_ACT_WALL_LET_GO, 2 PL_ACT_LEDGE_DROP (first moves the player
 * 195 down and 30 back). Then it moves with Player_MoveAccel, grabs a ledge
 * ahead while falling (not after PL_ACT_LEDGE_DROP) and lands as
 * Player_StateLeapOrFall does.
 */
void Player_StateClimbAirborne(Player *pl, s32 variant)
{
    f32 y;
    f32 move[3];
    f32 delta[3];

    pl->timer++;
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
        default:
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_JUMP_UP_CROUCH, 2, 0);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
            break;
        case 2:
            pl->pos[1] -= 195.0f;
            move[0] = 0.0f;
            move[1] = 0.0f;
            move[2] = -30.0f;
            Vec_Rotate(delta, move, (f32 *) pl->worldMtx);
            pl->pos[0] += delta[0];
            pl->pos[2] += delta[2];
        case 1:
            pl->step = 2;
            Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
            Player_StopMove(pl);
            pl->vel[1] = -9.0f;
            pl->accel[1] = -8.0f / 11.0f;
            if (variant == 1) {
                Player_SetMotionPair(pl, PL_MOT_CLIMB_AIRBORNE, 4, 0x16);
            } else {
                Player_SetMotionPair(pl, PL_MOT_CLIMB_AIRBORNE, 0, 0x16);
            }
            break;
        }
        Player_ResetAim(pl);
        Player_UpdateMotionTime(pl, 0);
        pl->timer = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_StopMove(pl);
            pl->vel[1] = 2.0f * (15.0f * (pl->weaponOut ? 0.8f : 1.0f));
            Player_StartJump(pl, 0x14);
            Player_SetMotionPair(pl, PL_MOT_CLIMB_AIRBORNE, 2, 0);
            pl->action = PL_POSTURE_AIRBORNE;
            if (pl->rideState != 0) {
                pl->rideState = 2;
            }
            Player_UpdateMotionTime(pl, 0);
            pl->timer = 0;
        }
        break;
    case 2:
        Player_MoveAccel(pl);
        if (pl->vel[1] < 0.0f) {
            if (variant != 2 && Player_FloorAheadAbove(pl, &y, 55.0f, 180.0f, 30.0f) && pl->hp > 0) {
                pl->targetYaw = pl->rot[1];
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = PL_POSTURE_STANDING;
                pl->rideState = 0;
                if (pl->hp <= 0 && Player_IsLocal(pl) == 1) {
                    Player_Die(pl);
                    break;
                }
                if (pl->timer >= 30) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                    break;
                }
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_CLIMB_LAND, 0, 0);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 3, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_CROUCH: crouches down (motion 7, then the held crouch,
 * motion 8), sheathes and drops a carried item; 1 PL_ACT_CROUCH_HOLD:
 * straight to the held crouch. From step 2 Player_CrouchInput reads the
 * crouched controls every frame.
 */
void Player_StateCrouch(Player *pl, s32 variant)
{
    u8 carry;

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->step++;
            pl->motionTime = 0;
            Player_SetMotionPair(pl, PL_MOT_CROUCH_DOWN, 2, 0);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
            pl->weaponOut = 0;
            carry = pl->carry;
            if (carry & PL_CARRY_HOLD_MASK) {
                pl->carry = carry & PL_CARRY_KIND_MASK;
                ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
            }
            break;
        case 1:
            pl->step = 2;
            if (pl->motionId[0] == 0x19C) {
                Player_SetMotionPair(pl, PL_MOT_CROUCH, 8, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_CROUCH, 2, 0);
            }
            Player_UpdateMotionTime(pl, 0);
            Player_SetPosture(pl, PL_POSTURE_CROUCHED | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
            pl->weaponOut = 0;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
            Player_SetPosture(pl, PL_POSTURE_CROUCHED | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
            Player_UpdateMotionTime(pl, 0);
        }
        break;
    case 2:
        Player_CrouchInput(pl);
        break;
    }
}

//---
void Player_StateBedMenu(Player *pl, s32 unused)
{

    pl->timer--;
    if (pl->timer <= 0) {
        pl->timer = 0;
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 30;
        Player_SetMotionPair(pl, PL_MOT_IDLE, 4, 0);
        pl->deliveryMenu = 0xFF;
        pl->menuDelay = 30;
        pl->menuCursor = 0;
        pl->menuPage = 0;
        break;
    case 1:
        if ((s32) pl->motionTime >= 0xF0 && pl->gender == 1 && pl->motionId[0] != 0x3F) {
            Player_SetMotionPair(pl, PL_MOT_IDLE_ALT, 2, 0);
        }
        if (Player_IsLocal(pl) == 1 && pl->timer <= 0) {
            if (Quest_PhaseIsOver(1) == 1 || pl->deliveryMenu == 0) {
                Player_ReturnToIdle(pl, 0, 2, 0);
            } else if (pl->deliveryMenu == 1) {
                pl->deliveryMenu = 0;
                switch (pl->menuCursor) {
                case 0:
                    Player_StartSpotAction(pl, 1);
                    break;
                case 1:
                    Quest_ClearBySubTarget();
                    Player_ReturnToIdle(pl, 0, 2, 0);
                    break;
                case 2:
                    Quest_Abort();
                    Player_ReturnToIdle(pl, 0, 2, 0);
                    break;
                }
            }
        }
        break;
    }
}

void Player_StateToIdle(Player *pl, s32 unused)
{
    Player_ReturnToIdle(pl, 0, 0x2, 0);
}

/*
 * Throws pl->useItem. variant: 0 PL_ACT_THROW_ITEM, 1 PL_ACT_THROW_ITEM_ALT,
 * 2 PL_ACT_RETURN_TO_CAMP. At the release frame it makes the thrown object
 * (ItemAttack_Create, the type by the item); one item makes the effect
 * Boomerang_Create instead and is used up only on a 1 in 8 roll. While the quest
 * runs one of the item is used up. When the motion ends variant 2 sends the
 * local player to the camp area (gScene.campArea: PL_AREA_MOVE_ENTER, nextArea,
 * that area's start position, QSync_SendPlayer); the others return to idle.
 */
void Player_StateThrowItem(Player *pl, s32 variant)
{
    s8 kind;
    u8 nouse;
    u8 k;

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_THROW_ITEM, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_THROW_ITEM_ALT, 2, 0);
        }
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, variant ? 20.0f : 24.0f)) {
            kind = 0;
            if (pl->prevStateGroup == 0) {
                if (pl->prevState == 1) {
                    kind = 2;
                }
                if (pl->prevState == 2) {
                    kind = 1;
                }
            }
            nouse = 0;
            switch (pl->useItem) {
            case 0x4C:
                ItemAttack_Create(pl, 0, kind);
                break;
            case 0x4E:
                ItemAttack_Create(pl, 7, kind);
                break;
            case 0x48:
                ItemAttack_Create(pl, 1, kind);
                break;
            case 0x297:
                ItemAttack_Create(pl, 1, kind);
                break;
            case 0x4B:
                ItemAttack_Create(pl, 5, kind);
                break;
            case 0x4D:
                ItemAttack_Create(pl, 0xb, kind);
                break;
            case 0x47:
                ItemAttack_Create(pl, 6, kind);
                break;
            case 0xD7:
                ItemAttack_Create(pl, 8, kind);
                break;
            case 0x49:
                ItemAttack_Create(pl, 9, kind);
                break;
            case 0x50:
                ItemAttack_Create(pl, 0xa, kind);
                break;
            case 0x41:
                ItemAttack_Create(pl, 0xc, kind);
                break;
            case 0x42:
                ItemAttack_Create(pl, 0xd, kind);
                break;
            case 0x43:
                ItemAttack_Create(pl, 0xe, kind);
                break;
            case 0x44:
                ItemAttack_Create(pl, 0xf, kind);
                break;
            case 0x45:
                ItemAttack_Create(pl, 0x15, kind);
                break;
            case 0x2BA:
                ItemAttack_Create(pl, 0x11, kind);
                break;
            case 0x2BB:
                ItemAttack_Create(pl, 0x12, kind);
                break;
            case 0x2BC:
                ItemAttack_Create(pl, 0x13, kind);
                break;
            case 0x4F:
                ItemAttack_Create(pl, 0x17, kind);
                break;
            case 0x40:
                if (pl->prevStateGroup == 0 && pl->prevState == 0x49) {
                    kind = 1;
                }
                if (pl->stateRand % 8) {
                    nouse = 1;
                }
                if (Player_HasSkill(pl, SKILL_THROW_UP) == 1 && !(Rand_Next(1) & 7)) {
                    nouse = 1;
                }
                if (nouse == 0 && Player_IsLocal(pl) == 1) {
                    Notice_Push(NOTICE_KIND_ITEM, 5, 0x40);
                }
                Boomerang_Create((Em *) pl, kind, nouse);
                break;
            }
            if (Quest_PhaseIsOver(1) == 0 && nouse == 0) {
                Pouch_Add(pl, pl->useItem, -1);
            }
        }
        if (pl->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_ReturnToIdle(pl, 0, 0xA, 0);
                break;
            case 1:
                Player_ReturnToIdle(pl, 0, 2, 0);
                break;
            case 2:
                if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
                    k = gScene.campArea;
                    pl->areaMoveState = PL_AREA_MOVE_ENTER;
                    pl->startPos[0] = gStartPos[k][0];
                    pl->startPos[1] = gStartPos[k][1];
                    pl->startPos[2] = gStartPos[k][2];
                    pl->startAngle = gStartAngle[k];
                    pl->nextArea = k;
                    Player_AddSpawnOffset(pl, pl->startPos, pl->startAngle);
                    pl->noHitTimer = 5;
                    QSync_SendPlayer(pl, QSYNC_PL_RELOCATED, 0);
                } else {
                    Player_ReturnToIdle(pl, 0, 2, 0);
                }
                break;
            }
        }
        break;
    case 2:
        break;
    }
}

/*
 * variant 0 PL_ACT_LEDGE_HANG, 1 PL_ACT_LEDGE_GRAB (first steps 55 forward
 * onto the ledge at the floor's height, Em_GetFloorYAbove). Hangs from a
 * ledge (motion 0x11); a grab after PL_ACT_FALL counts pl->fallGrabCount up. Pulls
 * up (PL_ACT_LEDGE_PULL_UP) when the motion ends, on PAD_CIRCLE or with the
 * stick up; the local player lets go (PL_ACT_LEDGE_DROP) on a new stick-down
 * press once pl->timer (30) is 0.
 */
void Player_StateLedgeHang(Player *pl, s32 variant)
{
    f32 y;
    s32 ang[3];
    f32 offset[3];
    f32 pos[3];
    f32 delta[3];
    f32 yawMtx[16];

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->targetYaw = pl->rot[1];
        Player_SetMotionPair(pl, PL_MOT_LEDGE_HANG, 0, 0);
        pl->timer = 30;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 1) {
            ang[0] = 0;
            ang[1] = pl->rot[1];
            ang[2] = 0;
            Mtx_FromAnglesXYZ(ang, yawMtx);
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 55.0f;
            Vec_Rotate(delta, offset, yawMtx);
            pos[0] = pl->pos[0] + delta[0];
            pos[1] = pl->pos[1] + delta[1];
            pos[2] = pl->pos[2] + delta[2];
            if (Em_GetFloorYAbove((Em *) pl, pos, &y) == 1) {
                pl->pos[1] = y;
            }
            offset[2] = 55.0f;
            Vec_Rotate(delta, offset, (f32 *) pl->worldMtx);
            pl->pos[0] += delta[0];
            pl->pos[1] += delta[1];
            pl->pos[2] += delta[2];
        }
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB) == 1 && pl->prevStateGroup == 0 && pl->prevState == 9) {
            pl->fallGrabCount++;
        }
        break;
    case 1:
        if (pl->timer != 0) {
            pl->timer--;
            if (pl->timer <= 0) {
                pl->timer = 0;
            }
        }
        if (pl->motion[0].playing == 0 || (pl->input.pressed & PAD_CIRCLE) || (pl->input.stickDir & PAD_UP)) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_PULL_UP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else if (Player_IsLocal(pl) == 1 && (pl->input.stickDirPressed & PAD_DOWN) && pl->timer == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_DROP, PL_SET_SNAP_POS);
        }
        break;
    }
}

/*
 * PL_ACT_RIDE_STAGGER: the stagger of a player standing on a monster that
 * carries it fast (Player_CheckRideSlip). Plays motion 0x1A until the
 * distance the monster moves the player (rideDelta) is under the limit again,
 * then returns to idle.
 */
void Player_StateRideStagger(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_STAGGER, 6, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_CheckRideSlip(pl) == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_PLACE_ITEM, 1 PL_ACT_PLACE_ITEM_CROUCHED: sets a tool item
 * down (a barrel bomb). At entry the local player runs Item_UseTool, which
 * makes the placed object for pl->useItem and uses one up. Variant 0 returns
 * to idle, variant 1 to PL_ACT_CROUCH_HOLD.
 */
void Player_StatePlaceItem(Player *pl, s32 variant)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->motionTime = 0;
        pl->timer = 0;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
            break;
        case 1:
            pl->step = 3;
            Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
            Player_SetMotionPair(pl, PL_MOT_PLACE_ITEM_CROUCHED, 0, 0x14);
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_UseTool(pl);
        }
        break;
    case 1:
        timer = pl->timer + 1;
        pl->timer = timer;
        if (timer >= 0xE) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH_STAND_UP, 0, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, PL_SET_NO_SYNC);
        }
        break;
    }
}

/*
 * PL_ACT_KNEEL_TO_PLACE: kneels (motion 7) before placing an item; then the
 * local player requests PL_ACT_PLACE_ITEM, and another player's copy holds
 * the crouch pose.
 */
void Player_StateKneelToPlace(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, PL_MOT_CROUCH_DOWN, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

/*
 * The roll. variant 0 PL_ACT_ROLL, 1 PL_ACT_ROLL_FROM_CROUCH (stands up
 * first). Costs 75 stamina, sets the hit-free frames (Player_SetEvadeFrames)
 * and drops a carried item. With the weapon drawn the local player's sword
 * and shield and dual swords take a flick up (PAD_RSTICK_UP) in frames 10-44
 * and then turn 0x2000 toward the held side (PAD_LEFT, PAD_RIGHT) into
 * SwordShield_EvadeAttack or DualSwords_EvadeAttack; the hammer charges from
 * the roll (Hammer_ChargeFromRoll) at frame 40 with PAD_R1 held. Returns to
 * idle when the motion ends.
 */
void Player_StateRoll(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->motionTime = 0;
        Player_SetEvadeFrames(pl);
        pl->timer = 0;
        Player_ResetAim(pl);
        pl->noFlinchTimer = 0;
        pl->stateVar0 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 30, 0, pl->pos, 1, 0);
        if (pl->carry & PL_CARRY_HOLD_MASK) {
            pl->carry &= PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        if (variant == 0) {
            pl->step = 2;
            if ((pl->weaponType == WEAPON_HAMMER || pl->weaponType == WEAPON_HUNTING_HORN) && pl->weaponOut != 0) {
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_WALK, 2, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_ROLL, 2, 0);
            }
        } else {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_ROLL_FROM_CROUCH, 0, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_ROLL, 0, 6);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        } else if (Player_IsLocal(pl) == 1 && variant == 0 && pl->weaponOut != 0) {
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
            case WEAPON_DUAL_SWORDS:
                if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && Motion_InFrameRange(pl, 0, 10.0f, 44.0f) && pl->stateVar0 == 0) {
                    pl->stateVar0 = 1;
                    if (pl->input.stickDir & PAD_LEFT) {
                        pl->stateVar0 = 2;
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->stateVar0 = 3;
                    }
                }
                if (Motion_ReachedFrame(pl, 0, 44.0f) && pl->stateVar0 != 0) {
                    if ((pl->stateVar0 & 0xFF) == 2) {
                        pl->rot[1] += 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->stateVar0 == 3) {
                        pl->rot[1] -= 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->weaponType == WEAPON_SWORD_SHIELD) {
                        SwordShield_EvadeAttack(pl);
                    } else {
                        DualSwords_EvadeAttack(pl);
                    }
                }
                break;
            case WEAPON_HAMMER:
                if (Motion_ReachedFrame(pl, 0, 40.0f) && (pl->input.held & PAD_R1)) {
                    Hammer_ChargeFromRoll(pl);
                }
                break;
            }
        }
        break;
    }
}

//---
void Player_StateStepUp(Player *pl, s32 unused)
{
    f32 offset[3];
    f32 pos[4];
    f32 delta[4];
    f32 y;
    f32 *offsetZ;
    f32 *dy;
    f32 *dz;

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_LEDGE_CLIMB_UP, 0, 0x58);
        pl->targetYaw = pl->rot[1];
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offsetZ = &offset[2];
        *offsetZ = 55.0f;
        Vec_Rotate(delta, offset, (f32 *) pl->worldMtx);
        dy = &delta[1];
        dz = &delta[2];
        pos[0] = pl->pos[0] + delta[0];
        pos[1] = pl->pos[1] + *dy;
        pos[2] = pl->pos[2] + *dz;
        if (Em_GetFloorYAbove((Em *) pl, pos, &y) == 1) {
            pl->pos[1] = y;
        }
        *offsetZ = 55.0f;
        Vec_Rotate(delta, offset, (f32 *) pl->worldMtx);
        pl->pos[0] += delta[0];
        pl->pos[1] += *dy;
        pl->pos[2] += *dz;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The sprint (PAD_R2, sheathed). variant 0 PL_ACT_DASH, 1 PL_ACT_DASH_RESUME
 * (picked up mid-stride). Uses stamina each frame: under 76 requests
 * PL_ACT_DASH_TIRED; PAD_CROSS with 75 or more rolls, a drop ahead leaps off
 * (PL_ACT_LEAP_OFF_LEDGE), and in battle with the heading more than 0x2000
 * off pl->targetAngle it becomes PL_ACT_FLEE_DASH. With PAD_R2 released the
 * local player goes to PL_ACT_RUN_AFTER_DASH (full stick, once dashDelay is
 * 0) or PL_ACT_RUN_STOP.
 */
void Player_StateDash(Player *pl, s32 variant)
{
    u8 stickLevel;
    f32 move[3];
    f32 delta[3];

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x600);
        pl->motionTime = 0;
        pl->dashDelay = 30;
        switch (variant) {
        case 0:
            if (pl->motionId[0] != 3) {
                Player_SetMotionPair(pl, PL_MOT_RUN, 0, 0);
            }
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_RUN, 4, 30);
            break;
        }
        pl->motion[0].step = 2.5f;
        pl->motion[1].step = 2.5f;
        pl->targetYaw = pl->rot[1];
        break;
    case 1:
        pl->motion[0].step = 2.5f;
        pl->motion[1].step = 2.5f;
        Player_UseStamina(pl, -1);
        if (pl->stamina < 0x4C) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED, 0);
            break;
        }
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 0x4B && Quest_PhaseIsOver(1) == 0) {
            pl->targetYaw = pl->rot[1];
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            break;
        }
        if (pl->input.stickTilt >= PL_STICK_HELD) {
            Player_FaceStick(pl);
        }
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f)) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEAP_OFF_LEDGE, 0);
            break;
        }
        if (pl->inBattle != 0 && (u16) (pl->targetAngle - pl->rot[1] + 0x2000) > 0x4000) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH, 0);
            break;
        }
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 5.0f;
        Vec_Rotate(delta, move, (f32 *) pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
        if (Player_IsLocal(pl) != 0) {
            stickLevel = Player_StickMoveLevel(pl, 0);
            if (stickLevel != 5 && (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED) == 0 || pl->stamina > 0) {
                    if (stickLevel == 3) {
                        if (pl->dashDelay == 0) {
                            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH, 0);
                        } else {
                            Player_IdleActions(pl, 0);
                        }
                    } else {
                        pl->freeState = 1;
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
                    }
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_OUT_OF_BREATH, 0);
                }
            } else {
                Player_IdleActions(pl, 0);
            }
            if (pl->mustCrouch != 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    }
}

/*
 * The sprint away from the monster being fought. variant: 0 PL_ACT_FLEE_DASH,
 * 1 PL_ACT_DASH_TIRED, 2 PL_ACT_FLEE_DASH_RESUME (after the dive). Costs
 * stamina each frame. Except when tired: under 76 stamina requests
 * PL_ACT_DASH_TIRED, leaving the battle or facing pl->targetAngle again goes
 * to PL_ACT_DASH_RESUME, PAD_CROSS dives (PL_ACT_DIVE), a drop ahead leaps
 * off. With PAD_R2 released it goes to PL_ACT_RUN_AFTER_DASH or
 * PL_ACT_RUN_STOP; a tired player with no stamina to PL_ACT_OUT_OF_BREATH.
 */
void Player_StateFleeDash(Player *pl, s32 variant)
{
    u8 stickLevel;
    f32 move[3];
    f32 delta[3];

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x600);
        pl->motionTime = 0;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_FLEE_DASH, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_DASH_TIRED, 0, 0);
            break;
        case 2:
            Player_SetMotionPair(pl, PL_MOT_FLEE_DASH, 4, 0);
            break;
        }
        pl->targetYaw = pl->rot[1];
        break;
    case 1:
        if (variant != 1) {
            Player_UseStamina(pl, -2);
            if (pl->stamina < 0x4C) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED, 0);
                break;
            }
            if (pl->inBattle == 0 || (u16) (pl->targetAngle - pl->rot[1] + 0x2000) <= 0x4000) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DASH_RESUME, 0);
                break;
            }
        } else {
            Player_UseStamina(pl, -1);
        }
        if ((pl->input.pressed & PAD_CROSS) && variant != 1 && pl->stamina >= 0x4B) {
            pl->targetYaw = pl->rot[1];
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DIVE, PL_SET_SNAP_POS);
            break;
        }
        if (pl->input.stickTilt >= PL_STICK_HELD) {
            Player_FaceStick(pl);
        }
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) && variant != 1) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEAP_OFF_LEDGE, 0);
            break;
        }
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 1.5f;
        Vec_Rotate(delta, move, (f32 *) pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
        if (Player_IsLocal(pl) != 0) {
            stickLevel = Player_StickMoveLevel(pl, 0);
            if (stickLevel != 5 && (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED) == 0 || pl->stamina > 0) {
                    if (stickLevel == 3 && variant != 1) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH, 0);
                    } else {
                        pl->freeState = 1;
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
                    }
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_OUT_OF_BREATH, 0);
                }
            } else {
                Player_IdleActions(pl, 0);
            }
            if (pl->mustCrouch != 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_OUT_OF_BREATH: the stamina ran out. Sheathes and pants for 150
 * frames, then refills the stamina to staminaMax and returns to idle.
 */
void Player_StateOutOfBreath(Player *pl, s32 variant)
{
    Player_AddStamina(pl, 4);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->weaponOut = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 0);
            pl->step = 2;
            pl->timer = 0x96;
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_START, 8, 0);
        pl->timer = 0xA;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 0x90);
        }
        break;
    case 2:
        if (--pl->timer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_RECOVER, 4, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_AddStamina(pl, (s32) pl->staminaMax);
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TIRED_STOP: plays the tired sprint's motion once, then returns to
 * idle (no entry seen).
 */
void Player_StateTiredStop(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_DASH_TIRED, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 5, 0);
        }
        break;
    }
}

/*
 * PL_ACT_LAND_HARD: the landing from a long fall. Clears rideState, shakes
 * the pad and the camera, and returns to idle when the motion ends.
 */
void Player_StateHardLanding(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        pl->rideState = 0;
        Player_SetMotionPair(pl, PL_MOT_LAND_HARD, 2, 0);
        Vib_StartLocal(pl, 1);
        CamShake_ReqA(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * PL_ACT_WALL_HOLD: holds still on a climbable wall in the pose of the climb
 * it came from (PLF_NO_ROOT_MOVE). Every frame it presses the player into the
 * wall (Player_StepBack) and Player_WallClimbInput starts the climbs or lets
 * go.
 */
void Player_StateWallHold(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    if (step == 0) {
        pl->step = step + 1;
        Player_SetFlag(pl, 0x20000U);
        pl->stateVar0 = 0;
        if (pl->motionId[0] == 0x22) {
            if (Motion_PassedFrame(pl, 0, 46.0f) != 0) {
                Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_UP, 0, 0x2E);
            } else {
                Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_UP, 0, 0x5A);
            }
        } else if (Motion_PassedFrame(pl, 0, 0.0f) != 0) {
            Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 0x2C);
        }
    } else {
        pl->stateVar0 = 1;
    }
    pl->noPushTimer = 2;
    Player_SetPosture(pl, PL_POSTURE_AIRBORNE | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
    Player_StepBack(pl, 1);
    Player_WallClimbInput(pl, 0);
}

/*
 * variant 0 PL_ACT_WALL_CLIMB_UP, 1 PL_ACT_WALL_CLIMB_UP_FAST (faster, each
 * cycle costs the dash stamina). At frames 46 and 90 the local player reads
 * the stick (Player_WallClimbInput; no input holds, PL_ACT_WALL_HOLD). No
 * stamina left requests PL_ACT_WALL_LET_GO; a ledge top ahead
 * PL_ACT_LEDGE_GRAB.
 */
void Player_StateWallClimbUp(Player *player, s32 variant)
{
    f32 ledgeY;
    u16 motion;
    u8 step;

    *(s16 *)&player->noPushTimer = 2;
    if (variant == 0) {
        player->motion[0].step = 1.6f;
        player->motion[1].step = 1.6f;
    } else {
        player->motion[0].step = 2.5f;
        player->motion[1].step = 2.5f;
    }
    step = player->step;
    if (step == 0) {
        player->step = step + 1;
        Player_SetPosture((void *) player, PL_POSTURE_AIRBORNE, 0, 0);
        *(s8 *)&player->rideState = 0;
        motion = player->motionId[0];
        if (motion != 0x22) {
            if (motion == 0x23) {
                if (Motion_ReachedFrame((void *) player, 0, 40.0f) != 0) {
                    Player_SetMotionPair((void *) player, 0x22, 0, 0x5A);
                } else {
                    Player_SetMotionPair((void *) player, 0x22, 0, 0x2E);
                }
            } else {
                Player_SetMotionPair((void *) player, 0x22, 4, 0);
            }
        }
    } else if (((Motion_PassedFrame((void *) player, 0, 46.0f) != 0) || (Motion_PassedFrame((void *) player, 0, 90.0f) != 0)) && (Player_IsLocal(player) == 1)) {
        if (!(Player_WallClimbInput(player, 1) & 0xFF)) {
            Player_RequestState((void *) player, PL_GROUP_ACTION, PL_ACT_WALL_HOLD, 0);
        } else if (variant == 1) {
            Player_UseDashStamina(player);
        }
    }
    if (player->stamina <= 0) {
        Player_RequestState((void *) player, PL_GROUP_ACTION, PL_ACT_WALL_LET_GO, 0);
        return;
    }
    if (Player_FloorAheadAbove((void *) player, &ledgeY, 55.0f, 180.0f, 30.0f) != 0) {
        player->targetYaw = (s16) player->rot[1];
        Player_RequestState((void *) player, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, 0);
    }
    Player_StepBack(player, 1);
}

/*
 * variant 0 PL_ACT_WALL_CLIMB_DOWN, 1 PL_ACT_WALL_CLIMB_DOWN_FAST; as
 * Player_StateWallClimbUp, downward. No stamina left requests
 * PL_ACT_WALL_LET_GO; reaching pl->floorY PL_ACT_WALL_DISMOUNT.
 */
void Player_StateWallClimbDown(Player *pl, s32 variant)
{
    u16 motion;

    pl->noPushTimer = 2;
    if (variant == 0) {
        pl->motion[0].step = 1.6f;
        pl->motion[1].step = 1.6f;
    } else {
        pl->motion[0].step = 2.6f;
        pl->motion[1].step = 2.6f;
    }
    if (pl->step == 0) {
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->rideState = 0;
        motion = pl->motionId[0];
        if (motion != 0x23) {
            if (motion == 0x22) {
                if (Motion_ReachedFrame(pl, 0, 86.0f) != 0) {
                    Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 0x2C);
                } else {
                    Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 0);
                }
            } else {
                Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 4, 0);
            }
        }
    } else if (Motion_PassedFrame(pl, 0, 0.0f) != 0 || Motion_PassedFrame(pl, 0, 44.0f) != 0) {
        if (Player_IsLocal(pl) == 1) {
            if (!(Player_WallClimbInput(pl, 1) & 0xFF)) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD, 0);
            } else if (variant == 1) {
                Player_UseDashStamina(pl);
            }
        }
    }
    if (pl->stamina <= 0) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_LET_GO, 0);
        return;
    }
    if (pl->pos[1] <= pl->floorY) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_DISMOUNT, 0);
    }
    Player_StepBack(pl, 1);
}

/*
 * PL_ACT_WALL_DISMOUNT: steps off the foot of the wall, then returns to idle.
 */
void Player_StateWallDismount(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_WALL_DISMOUNT, 6, 0);
        Player_StepBack(pl, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        Player_StepBack(pl, 1);
        break;
    }
}

/*
 * PL_ACT_WALL_GRAB: starts a wall climb (from Player_CheckClimb): turns to
 * face the wall (Player_TurnAlongWallSide, side 0) and at frame 90 of the
 * climb motion requests PL_ACT_WALL_HOLD.
 */
void Player_StateWallGrab(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->rideState = 0;
        Player_TurnAlongWallSide(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_UP, 4, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 90.0f) != 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD, 0);
        }
        break;
    }
    Player_StepBack(pl, 1);
}

/* PL_ACT_RUN_STOP: the skid at the end of a run or a sprint, then idle. */
void Player_StateRunStop(Player *pl, s32 unused)
{
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_RUN_STOP, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xA, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SET_TRAP: kneels to set a trap or a device for pl->useItem
 * (Player_UseItem sends the placed devices, the special mine point items and
 * the delivery item of an action spot of kind 0xB here). At frame 60 of the
 * second motion the local player requests PL_ACT_SET_TRAP_DONE; another
 * player's copy finishes the motion and returns to idle.
 */
void Player_StateSetTrap(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -0xE, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 130.0f) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_SET_TRAP, 4, 0);
        }
        break;
    case 2:
        if (Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP_DONE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xA, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SET_TRAP_DONE: at entry the local player applies the item: the
 * quest's delivery item is delivered (Quest_DeliverFromPouch, mode 1), any
 * other goes through Item_UseMisc (its effect or its special mine point, one
 * used up). Then stands up and returns to idle.
 */
void Player_StateSetTrapDone(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_SET_TRAP, 0, 0x3C);
        if (Player_IsLocal(pl) == 1) {
            if (pl->useItem == ITEM_ID_DELIVERY) {
                Quest_DeliverFromPouch(pl, 1);
                break;
            }
            Item_UseMisc(pl);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -4, 0xCE);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xA, 0);
        }
        break;
    }
}

/*
 * The sidestep with the weapon drawn. variant 0 PL_ACT_EVADE_RIGHT, 1
 * PL_ACT_EVADE_LEFT. Costs 75 stamina and sets the hit-free frames; the long
 * sword has its own motions. The sword and shield and the dual swords chain
 * their evade attack as in Player_StateRoll (frames 10-42). Returns to idle
 * when the motion ends.
 */
void Player_StateSideEvade(Player *pl, s32 variant)
{
    u8 dir;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->noFlinchTimer = 0;
        Player_SetEvadeFrames(pl);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 0x1E, 0, &pl->pos[0], 1, 0);
        pl->motionTime = 0;
        if (variant == 0) {
            if (pl->weaponType == WEAPON_LONG_SWORD) {
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_END, 2, 0);
                break;
            }
            Player_SetMotionPair(pl, PL_MOT_WPN_EVADE_RIGHT, 2, 0);
            break;
        }
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_WPN_EVADE_LEFT, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            break;
        }
        if (Player_IsLocal(pl) == 1 && pl->weaponOut != 0) {
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
            case WEAPON_DUAL_SWORDS:
                if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && Motion_InFrameRange(pl, 0, 10.0f, 42.0f) != 0 && pl->stateVar0 == 0) {
                    pl->stateVar0 = 1;
                    if (pl->input.stickDir & PAD_LEFT) {
                        pl->stateVar0 = 2;
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->stateVar0 = 3;
                    }
                }
                if (Motion_ReachedFrame(pl, 0, 42.0f) != 0 && (dir = pl->stateVar0)) {
                    if (dir == 2) {
                        pl->rot[1] += 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->stateVar0 == 3) {
                        pl->rot[1] -= 0x2000;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->weaponType == WEAPON_SWORD_SHIELD) {
                        SwordShield_EvadeAttack(pl);
                        break;
                    }
                    DualSwords_EvadeAttack(pl);
                }
                break;
            }
        }
        break;
    }
}

/*
 * The first part's data: the scene events' records
 * (ScEventRec). ScEvent_VersusEnd is the arena cup's versus quest's: no
 * area, done flag or camera; ScEvent_CheckAlwaysA starts it and
 * ScEvent_RunVersusEnd runs the end of the match. The <Monster>Sighting
 * records are the scenes a story quest starts with (ScEvent_Init picks one
 * by gScene.questId; plsel's gallery replays them through
 * gGalleryScEvents). Each plays once per save, the first time the hunter
 * enters the record's area (ScEvent_CheckDay), and then sets its doneFlag.
 * ScEvent_RunA shows the record's camera following the first monster of
 * camGroup, with its jingle and music, while the monster's Update holds it
 * in EM_GROUP_EVENT.
 */
extern ScEventRec ScEvent_Cam5;
ScEventRec ScEvent_VersusEnd = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0 }, ScEvent_CheckAlwaysA, ScEvent_RunVersusEnd },
};
ScEventRec ScEvent_DemoCam7 = {
#include "game/008C3520.inc"
};
ScEventRec ScEvent_DemoCam32 = {
#include "game/008C3540.inc"
};
ScEventRec ScEvent_DemoCam2 = {
#include "game/008C3560.inc"
};
ScEventRec ScEvent_DemoCam24 = {
#include "game/008C3580.inc"
};
ScEventRec ScEvent_DemoCam47 = {
#include "game/008C35A0.inc"
};
ScEventRec ScEvent_DemoCam3A = {
#include "game/008C35C0.inc"
};
ScEventRec ScEvent_DemoCam37 = {
#include "game/008C35E0.inc"
};
ScEventRec ScEvent_VelocidromeSighting = {
#include "game/008C3600.inc"
};
ScEventRec ScEvent_BulldromeSighting = {
#include "game/008C3620.inc"
};
ScEventRec ScEvent_DaimyoHermitaurSighting = {
#include "game/008C3640.inc"
};
ScEventRec ScEvent_CongalalaSighting = {
#include "game/008C3660.inc"
};
ScEventRec ScEvent_RathianSighting = {
#include "game/008C3680.inc"
};
ScEventRec ScEvent_KushalaJungleSighting = {
#include "game/008C36A0.inc"
};
ScEventRec ScEvent_KushalaSnowSighting = {
#include "game/008C36C0.inc"
};
ScEventRec ScEvent_LunastraTeostraVolcano8Sighting = {
#include "game/008C36E0.inc"
};
ScEventRec ScEvent_LunastraTeostraTowerSighting = {
#include "game/008C3700.inc"
};
ScEventRec ScEvent_GarugaSighting = {
#include "game/008C3720.inc"
};
ScEventRec ScEvent_ChameleosSighting = {
#include "game/008C3740.inc"
};
ScEventRec ScEvent_LunastraTeostraVolcano7Sighting = {
#include "game/008C3760.inc"
};
ScEventRec ScEvent_BlangongaSighting = {
#include "game/008C3780.inc"
};
ScEventRec ScEvent_ShogunCeanataurSighting = {
#include "game/008C37A0.inc"
};
ScEventRec ScEvent_KutkuSighting = {
#include "game/008C37C0.inc"
};
ScEventRec ScEvent_KhezuSighting = {
#include "game/008C37E0.inc"
};
ScEventRec ScEvent_DiablosSighting = {
#include "game/008C3800.inc"
};
ScEventRec ScEvent_GraviosSighting = {
#include "game/008C3820.inc"
};
ScEventRec ScEvent_MonoblosSighting = {
#include "game/008C3840.inc"
};
ScEventRec ScEvent_RathalosSighting = {
#include "game/008C3860.inc"
};
ScEventRec ScEvent_KirinSighting = {
#include "game/008C3880.inc"
};
ScEventRec ScEvent_Seq1 = {
#include "game/008C38A0.inc"
};
ScEventRec ScEvent_Seq5 = {
#include "game/008C38C0.inc"
};
ScEventRec ScEvent_Seq3 = {
#include "game/008C38E0.inc"
};
ScEventRec ScEvent_Seq3B = {
#include "game/008C3900.inc"
};
ScEventRec ScEvent_Seq11 = {
#include "game/008C3920.inc"
};
ScEventRec ScEvent_CamF = {
#include "game/008C3940.inc"
};
ScEventRec ScEvent_Cam6 = {
#include "game/008C3960.inc"
};
ScEventRec ScEvent_CamB = {
#include "game/008C3980.inc"
};
ScEventRec ScEvent_Cam2 = {
#include "game/008C39A0.inc"
};
ScEventRec ScEvent_Cam3 = {
#include "game/008C39C0.inc"
};
ScEventRec ScEvent_Cam4 = {
#include "game/008C39E0.inc"
};
ScEventRec ScEvent_Cam5 = {
#include "game/008C3A00.inc"
};
/* Read by plsel's gallery as a table of GalView pointers by gallery entry (Galex_QuestTask). */
ScEventHead *gGalleryScEvents[48] = {
    &ScEvent_BulldromeSighting.head, &ScEvent_VelocidromeSighting.head, &ScEvent_KutkuSighting.head, &ScEvent_DaimyoHermitaurSighting.head,
    0, &ScEvent_CongalalaSighting.head, 0, &ScEvent_RathianSighting.head,
    &ScEvent_KushalaJungleSighting.head, &ScEvent_KhezuSighting.head, &ScEvent_BlangongaSighting.head, &ScEvent_ShogunCeanataurSighting.head,
    &ScEvent_DiablosSighting.head, &ScEvent_KushalaSnowSighting.head, 0, &ScEvent_LunastraTeostraVolcano8Sighting.head,
    &ScEvent_LunastraTeostraVolcano7Sighting.head, &ScEvent_GraviosSighting.head, &ScEvent_LunastraTeostraTowerSighting.head, &ScEvent_KirinSighting.head,
    &ScEvent_RathalosSighting.head, &ScEvent_MonoblosSighting.head, &ScEvent_ChameleosSighting.head, &ScEvent_GarugaSighting.head,
    &ScEvent_DemoCam7.head, &ScEvent_DemoCam37.head, &ScEvent_DemoCam2.head, &ScEvent_DemoCam24.head,
    &ScEvent_DemoCam47.head, &ScEvent_DemoCam3A.head, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, &ScEvent_Cam6.head,
    &ScEvent_Cam3.head, &ScEvent_Cam4.head, &ScEvent_CamB.head, &ScEvent_Cam5.head,
    &ScEvent_CamF.head, &ScEvent_Cam2.head, 0, 0,
};
f32 gSpotLineupOffsets[4] = {
#include "game/008C3AE0.inc"
};
s16 gDrawnWalkStopIdle[16] = {
#include "game/008C3AF0.inc"
};
WeaponMotion gDrawMotions[12] = {
#include "game/008C3B10.inc"
};
WeaponMotion gSheatheMotions[12] = {
#include "game/008C3B70.inc"
};
WeaponMotion gSheatheMovingMotions[12] = {
#include "game/008C3BD0.inc"
};
WeaponMotion gDrawMovingMotions[12] = {
#include "game/008C3C30.inc"
};

/* ---- The second part: from Player_StateBedLieDown on ---- */
/* Declarations of the functions called below. */
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void GreatSword_RunState(Player *em);
void LongSword_RunState(Player *em);
void Bowgun_RunState(Player *em);
void Bow_RunState(Player *em);
void Hammer_RunState(Player *em);
void HuntingHorn_RunState(Player *em);
void Lance_RunState(Player *em);
void Gunlance_RunState(Player *em);
void SwordShield_RunState(Player *em);
void DualSwords_RunState(Player *em);
void Player_Move(void *obj);

/* prototypes */
typedef struct SpitMeat {
    s16 raw;                    /* 0x00: 0: raw meat (item 0x28) is used up, else the item and a bone (0xA2) back */
    s16 jingle;                 /* 0x02: played at the start; picks the result jingles */
    s16 rare;                   /* 0x04: the frame it is rare */
    s16 done;                   /* 0x06: the frame it is done */
    s16 doneLen;                /* 0x08: frames it stays done (burnt after) */
    s16 gourmet;                /* 0x0A: the gourmet window, from done + gourmet */
    s16 gourmetLen;             /* 0x0C */
    s16 unk0E;
    u16 itemRare;               /* 0x10: the items given */
    u16 itemDone;               /* 0x12 */
    u16 itemGourmet;            /* 0x14 */
    u16 itemBurnt;              /* 0x16 */
} SpitMeat;
extern SpitMeat gSpitMeats[];
void Bgm_StopEvent(void *obj);
s32 Quest_PlayerFainted(Player *pl, s32 other);
void Versus_MonsterDown(Em *em);
void Quest_ShowValue(void);
void Quest_ResetMusicIfIdle(void);
void Bgm_PlayFaintMusic(void);
s32 Player_GetHpSkillBonus(Player *p);
void Player_ClearBuffs(Player *pl);
void Player_PlaySeAtSelf(u8 *a0, s32 a1);
void CamReq_Cancel(void);
extern u16 gStartAngle[];
void Player_SetEvadeFrames(Player *em);
u8 Player_InCurrentArea(void *obj);
s32 Quest_GetDeliveryTarget(Player *p, s32 slot, ItemStack *out);
u16 Quest_DeliverItem(Player *pl, u16 item, s16 count);
s32 Player_NearTrader(void *obj);
u16 Gather_AtPoint(Player *p);
u16 Gather_AtMinePoint(Player *p);
u8 Notice_PushItem(char *item);
s32 MsgEvent_Check(u16 id);
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
f32 Math_Cos(f32 x);
s32 Game_IsOnline(void);
void Player_GuardInput(Player *p);
void Player_RequestStateSynced(Player *p, u16 state, u8 sub, u16 flags);
void Sound_PlayArmorRattle(Player *p, s32 a);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Player_AddStamina(Player *p, s16 n);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
void Player_ClearAccel(Player *p);
s32 Player_StartJump(void *obj, s16 frames);
void Player_MoveAccel(Player *p);
void Player_UseStamina(Player *p, s32 n);
void StunStars_CreatePlayer(Player *em, u8 k);
extern s8 gStunMashNeed[];
void Player_AddSharpness(Player *p, s16 n);
s32 Player_FloorAheadBelow(Player *em, u16 above, f32 fwd, f32 up);
s32 Player_HasCarryBonus(Player *pl);
s32 Player_GatherAtSpot(Player *em, u16 kind, u16 stat);
void Quest_GiveRewards(Player *p);
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
u8 Player_NoticePouchFull(s32 item);
void QuestStats_Add(s32 id, s32 n);
u8 Player_StickMoveLevel(Player *, s32);
s32 Player_GetStickYaw(Player *p, s32 i);
Em *EmSlot_Alloc(void);
s32 GmEm_Task(Em *em);
void CamReq_Cart(void);
void Em_BonePos(void *obj, s16 bone, f32 *out);
s32 Area_HasFlyer(void);
void Area_GetFlyerPos(f32 *pos, u16 area);
u16 Vec_Heading(f32 *a, f32 *b);
void Item_UseTeamPowder(Player *pl);
void PartFx_SpawnForPlayer(Player *p, s16 arg, s32 type);
u8 Notice_Push(u8, s16, s16);
static void Player_ClearMash(Player *em);
void Effect06_SpawnAtBone(Player *em, s16 type, u8 sub, s16 bone, f32 scale);
void Player_AddHp(Player *p, s16 n);
void Item_Use(Player *pl);
void Item_UsePlaced(Player *pl);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_IsLocal(void *);
void Player_RequestState(Player *p, u16 state, u8 sub, u16 flags);
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter declared s32 (the definition's is s16); with the s16 the calls compile differently (Player_StateSpitRoast most of all) */
void Player_SetPosture(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_StartIdleMotion(Player *p, s32 a, s32 b);
void Quest_StartCountdown(Player *p);
void PlayerAttack_Create(Player *, u8);
void StageShot_CreateForPlayer(Player *, u8 kind);
s32 Player_CrouchInput(Player *);
void Player_UpdateLean(Player *, s32);
void Player_StateIdle(Player *, s32);
void Player_StateDrawSheathe(Player *, u32);
void Player_StateLeapOrFall(Player *, s32);
void Player_StateClimbAirborne(Player *, s32);
void Player_StateCrouch(Player *, s32);
void Player_StateBedMenu(Player *, s32);
void Player_StateToIdle(Player *em, s32 arg1);
void Player_StateThrowItem(Player *, s32);
void Player_StateLedgeHang(Player *, s32);
void Player_StateRideStagger(Player *, s32);
void Player_StatePlaceItem(Player *, s32);
void Player_StateKneelToPlace(Player *em, s32 arg1);
void Player_StateRoll(Player *, s32);
void Player_StateStepUp(Player *, s32);
void Player_StateDash(Player *, s32);
void Player_StateFleeDash(Player *, s32);
void Player_StateOutOfBreath(Player *em, s32 arg1);
void Player_StateTiredStop(Player *em, s32 arg1);
void Player_StateHardLanding(Player *em, s32 arg1);
void Player_StateWallClimbDown(Player *, s32);
void Player_StateWallDismount(Player *em, s32 arg1);
void Player_StateWallGrab(Player *, s32);
void Player_StateRunStop(Player *, s32);
void Player_StateSetTrap(Player *, s32);
void Player_StateSetTrapDone(Player *, s32);
void Player_StateSideEvade(Player *, s32);
void Player_StateBedSleep(Player *, s32);
void Player_StateMountedGunAim(Player *, s32);
void Player_StateMountedGunFire(Player *, s32);
void Player_StateCrouchStop(Player *, s32);
void Player_StepBack(Player *, s32);
void Player_UpdateWallSidle(Player *em);
void Player_StateDive(Player *, s32);
void Player_StateGather(Player *, u32);
void Player_StateGatherCrouched(Player *, s32);
void Player_UpdateFishingInput(Player *);
void Player_StateFishReel(Player *, s32);
void Player_StateFishReelEmpty(Player *em, s32 arg1);
void Player_StateSpitRoast(Player *, s32);
void Player_StateTrade(Player *, s32);
void Player_StateInstrumentPlay(Player *, s32);
void Player_StateInstrumentEnd(Player *, s32);
void Player_StateSharpen(Player *, s32);
void Player_StateLanceHop(Player *, s32);
void Player_StateHornItemPlay(Player *, s32);
void Player_StateFaint(Player *, s32);
void Player_StateCart(Player *, s32);
void Player_StateLeave(Player *em, s32 arg1);
void Player_UpdateCarryKind(Player *);
void Player_UpdateCarryInput(Player *, s32);
void Player_StateCarryWalk(Player *, s32);
void Player_StateCarryRun(Player *, s32);
void Player_StateCarryPutDown(Player *, s32);
void Player_StateCarryAirborne(Player *, s32);
void ItemProp_CreateAtPlayer(Player *, s32 k);
void MashDebris_Create(s8, s16 *);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Motion_ReachedFrame(void *, s32, f32);
void CamCtl_TurnToPlayerFront(u8 *o);
void CamShake_ReqA(void *, s32 i);
void CamReq_PlayerDown(void);
s32 ItemGet_Reserve(s32 arg0, u16 *arg1);
s32 Quest_PhaseIsOver(s32);
void Vib_StartLocal(void *port, s32 i);
void Player_ReturnToIdle(Player *p, s32 action, s32 a2, s32 a3);
u16 Pouch_FindCarryItem(Player *);
void Player_ResetAim(Player *p);
void Lure_Spawn(Player *pl, u8 k);
void Jingle_Play(void *obj, s32 id);
extern s16 gGestureOneShots[];
u16 Rand_Next(s32 stream);
u16 Player_FindActionSpot(Player *, f32 *, u16 *arg, u8 *);
s16 Pouch_Count(Player *p, u16 item);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u8 Item_CarryKind(u16);
/* end of prototypes */

void Player_StateBedLieDown(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_BED_LIE_DOWN, -6, 0);
        pl->poison = 0;
        pl->mashBuildup = 0;
        pl->mashLeft = 0;
        pl->status7Timer = 0;
        pl->status6Timer = 0;
        pl->status5Buildup = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_SLEEP, 0);
        }
        break;
    }
}

/*
 * PL_ACT_BED_SLEEP: sleeps on the bed, regaining 1 health a frame
 * (Player_AddHp) with a puff effect three times a second; the local player
 * gets up (PL_ACT_BED_GET_UP) after 120 frames. Walls are ignored
 * (fieldCollide 0).
 */
void Player_StateBedSleep(Player *pl, s32 variant)
{
    f32 fxOffset[3];

    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->motionTime = 0;
        pl->timer = 120;
        Player_SetMotionPair(pl, PL_MOT_BED_SLEEP, 0, 0);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
        break;
    case 1:
        if (--pl->timer <= 0 && Player_IsLocal(pl) == 1) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_GET_UP, 0);
            break;
        }
        if (pl->motionId[0] == 0x1AB && !(Rand_Next(1) & 0x7F)) {
            Player_SetMotionPair(pl, PL_MOT_BED_SLEEP_STIR, 4, 0);
        } else if (pl->motionId[0] == 0x1AC && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_BED_SLEEP, 4, 0);
        }
        fxOffset[0] = 0.0f;
        fxOffset[1] = 20.0f;
        fxOffset[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 20, fxOffset, 0.8f);
        }
        Player_AddHp(pl, 1);
        break;
    }
}

/*
 * PL_ACT_BED_GET_UP: turns half a turn and gets up from the bed, then idle.
 */
void Player_StateBedGetUp(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->motionTime = 0;
        pl->rot[1] = (pl->rot[1] + 0x7FFF + 1) & 0xFFFF;
        pl->targetYaw = (u16) pl->rot[1];
        Player_SetMotionPair(pl, PL_MOT_BED_GET_UP, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Aims the mounted gun of an action spot of kind 6. variant 0
 * PL_ACT_MOUNTED_GUN_AIM (resets the pitch), 1 PL_ACT_MOUNTED_GUN_REAIM
 * (after a shot, keeps it). yawCenter is the spot's angle; the local player
 * aims with Player_UpdateLean. After 30 frames PAD_CIRCLE or PAD_R3 with the
 * gun's ammo item in the pouch requests PL_ACT_MOUNTED_GUN_FIRE; after 60
 * frames PAD_CROSS (selector closed) or the quest's end returns to idle.
 */
void Player_StateMountedGunAim(Player *pl, s32 variant)
{
    f32 pos[3];
    u16 angle;
    s8 kind;

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_MOUNTED_GUN, 4, 0);
            pl->aimPitch = 0;
            pl->aimPitchPct = 0;
        } else {
            Player_SetMotionPair(pl, PL_MOT_MOUNTED_GUN, 0, 0x42);
        }
        Player_UpdateMotionTime(pl, 0);
        pl->timer = 60;
        pl->stateVar0 = 30;
        if (Player_FindActionSpot((Player *)pl, (f32 *)pos, &angle, (u8 *)&kind) == 6) {
            pl->yawCenter = angle;
            break;
        }
        pl->yawCenter = pl->targetYaw;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (pl->stateVar0 > 0) {
            pl->stateVar0--;
        }
        if (Player_IsLocal(pl) == 1) {
            if (Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            Player_UpdateLean(pl, 0x19E);
            if (pl->stateVar0 == 0 && (pl->input.pressed & (PAD_CIRCLE | PAD_R3)) && Pouch_Count(pl, 0x29B) != 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_FIRE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW | PL_SET_SYNC_ALT);
                break;
            }
            if (pl->timer == 0 && (pl->input.pressed2 & PAD_CROSS) && pl->itemSelect == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_MOUNTED_GUN_FIRE: uses up one of the gun's ammo item, shakes the pad
 * and makes the shot (StageShot_CreateForPlayer type 0). After 60 frames it aims
 * again (PL_ACT_MOUNTED_GUN_REAIM) while ammo is left, else returns to idle.
 */
void Player_StateMountedGunFire(Player *pl, s32 variant)
{

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_MOUNTED_GUN, 0, 0x42);
        Player_UpdateMotionTime(pl, 0);
        pl->timer = 60;
        if (Player_IsLocal(pl) != 1 || Pouch_Count(pl, 0x29B) != 0) {
            Pouch_Add(pl, 0x29B, -1);
            Vib_StartLocal(pl, 2);
            StageShot_CreateForPlayer(pl, STAGESHOT_MOUNTED_GUN);
        }
        break;
    case 1:
        pl->timer -= 1;
        if (pl->timer == 0) {
            if (Pouch_Count(pl, 0x29B) > 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_CANNON_FIRE: fires the cannon of an action spot of kind 8 (from the
 * carry state): pad and camera shakes and the shot StageShot_CreateForPlayer(pl,
 * 1), then idle.
 */
void Player_StateCannonFire(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_BRACE, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Vib_StartLocal(pl, 3);
        CamShake_ReqA(pl, 5);
        StageShot_CreateForPlayer(pl, STAGESHOT_CANNON);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_BRACE_END, 2, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/* PL_ACT_CROUCH_STAND_UP: stands up from the crouch, then idle. */
void Player_StateCrouchStandUp(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_CROUCH_STAND_UP, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_ACT_LEDGE_PULL_UP: climbs up from the ledge hang (the step-up motion),
 * then idle.
 */
void Player_StateLedgePullUp(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_LEDGE_CLIMB_UP, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_CROUCH_WALK: walks crouched. The local player rolls out
 * (PL_ACT_ROLL_FROM_CROUCH) on PAD_CROSS with 75 stamina and mustCrouch clear;
 * otherwise Player_CrouchInput reads the controls, and stops the walk only
 * once pl->timer (15) is 0.
 */
void Player_StateCrouchWalk(Player *pl, s32 unused)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK, 8, 0);
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        pl->weaponOut = 0;
        Player_SetFlag(pl, 0x1000U);
        pl->freeState = 1;
        pl->timer = 0xF;
        break;
    case 1:
        timer = pl->timer;
        if (timer != 0) {
            pl->timer = timer - 1;
        }
        if (Player_IsLocal(pl) == 1) {
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B) && (pl->mustCrouch == 0)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL_FROM_CROUCH, PL_SET_SNAP_POS);
                break;
            }
            Player_CrouchInput(pl);
        } else {
        }
        break;
    }
}

/*
 * PL_ACT_CROUCH_STOP: stops the crouched walk; after 12 frames it switches to
 * PL_ACT_CROUCH_HOLD.
 */
void Player_StateCrouchStop(Player *pl, s32 variant)
{
    s16 blend;

    pl->timer++;
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] == 0x25) {
            blend = 12;
        } else {
            blend = 8;
        }
        Player_SetPosture(pl, PL_POSTURE_CROUCHED | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_CROUCH, blend, 0);
        pl->timer = 0;
        break;
    case 1:
        if (pl->timer >= 12) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, 0);
        }
        break;
    }
}

/*
 * PL_ACT_CROUCH_WALK_FAST: the faster crouched walk (the stick fully pushed);
 * controls as Player_StateCrouchWalk.
 */
void Player_StateCrouchWalkFast(Player *pl, s32 unused)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK_FAST, 6, 0);
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        Player_SetFlag(pl, 0x02000000U);
        pl->freeState = 1;
        pl->timer = 0xF;
        break;
    case 1:
        timer = pl->timer;
        if (timer != 0) {
            pl->timer = timer - 1;
        }
        if (Player_IsLocal(pl) == 1) {
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 0x4B) && (pl->mustCrouch == 0)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL_FROM_CROUCH, PL_SET_SNAP_POS);
                break;
            }
            Player_CrouchInput(pl);
        } else {
        }
        break;
    }
}

/*
 * PL_ACT_BATTLE_ALERT: the start of a battle with the weapon sheathed: one
 * motion, then idle.
 */
void Player_StateBattleAlert(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_BATTLE_ALERT, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TURN_AROUND: the quick half turn when the stick is pushed back
 * against a run (Player_IdleActions). Takes targetYaw at once, then requests
 * PL_ACT_RUN_AFTER_DASH.
 */
void Player_StateTurnAround(Player *pl, s32 unused)
{
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->rot[1] = (s32) pl->targetYaw;
        Player_SetMotionPair(pl, PL_MOT_TURN_AROUND, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH, 0);
        }
        break;
    }
}

/* moves the player 3 units back along its heading (forward when fwd is set) */
void Player_StepBack(Player *pl, s32 fwd)
{
    f32 stepVec[3];
    f32 stepWorld[3];
    s32 ang[3];
    f32 yawMtx[16];

    ang[0] = 0;
    ang[2] = 0;
    ang[1] = pl->rot[1];
    Mtx_FromAnglesXYZ(ang, yawMtx);
    stepVec[0] = 0.0f;
    stepVec[1] = 0.0f;
    stepVec[2] = -3.0f;
    if ((s16) fwd) {
        stepVec[2] *= -1.0f;
    }
    Vec_Rotate(stepWorld, stepVec, yawMtx);
    pl->pos[0] += stepWorld[0];
    pl->pos[2] += stepWorld[2];
}

/*
 * The local player's input with its back to a wall: the left stick's world
 * heading (Player_GetStickYaw) against the player's picks
 * PL_ACT_WALL_SIDLE_POS (0x2000 to 0x6000 off), PL_ACT_WALL_SIDLE_NEG (0xA000
 * to 0xE000) or PL_ACT_WALL_PRESS_HOLD. PAD_CROSS, or no wall behind
 * (Player_TurnAlongWallSide, side 1), requests PL_ACT_RUN_STOP.
 */
void Player_UpdateWallSidle(Player *pl)
{
    s32 angleDelta;

    if (Player_IsLocal(pl) != 0) {
        switch (Player_StickMoveLevel(pl, 0)) {
        case 1:
        case 2:
        case 3:
            angleDelta = (u16) ((u16) Player_GetStickYaw(pl, 0) - *(u16 *) &pl->rot[1]);
            if (angleDelta >= 0x2000 && angleDelta <= 0x6000) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_POS) == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_POS, 0);
                }
            } else if (angleDelta >= 0xA000 && angleDelta <= 0xE000) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_NEG) == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_NEG, 0);
                }
            } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD) == 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD, 0);
            }
            break;
        default:
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD) == 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD, 0);
            }
            break;
        }
        if ((pl->input.pressed & PAD_CROSS) || !(Player_TurnAlongWallSide(pl, 1) & 0xFF)) {
            pl->freeState = 1;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
        }
    }
}

/*
 * variant 0 PL_ACT_WALL_PRESS (puts its back to a wall), 1
 * PL_ACT_WALL_PRESS_HOLD (from frame 30 of the same motion). Pressed back
 * into the wall every frame (Player_StepBack); from frame 30
 * Player_UpdateWallSidle reads the stick.
 */
void Player_StateWallPress(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_WALL_PRESS, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WALL_PRESS, 2, 0x1E);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        Player_StepBack(pl, 0);
        break;
    case 1:
        Player_StepBack(pl, 0);
        if (Motion_ReachedFrame(pl, 0, 30.0f) != 0) {
            Player_UpdateWallSidle(pl);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_WALL_SIDLE_NEG, 1 PL_ACT_WALL_SIDLE_POS: sidles along a
 * wall with its back to it, pressed into it every frame;
 * Player_UpdateWallSidle reads the stick.
 */
void Player_StateWallSidle(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_WALL_SIDLE_NEG, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WALL_SIDLE_POS, 2, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        Player_StepBack(pl, 0);
        break;
    case 1:
        Player_StepBack(pl, 0);
        Player_UpdateWallSidle(pl);
        break;
    }
}

/*
 * PL_ACT_DIVE: the forward dive from the flee sprint. Costs 75 stamina and is
 * hit-free until the landing; the player leaps 16 forward and lands on
 * pl->floorY with a pad and camera shake. Then PL_ACT_FLEE_DASH_RESUME with
 * PAD_R2 held, else PL_ACT_RUN_STOP.
 */
void Player_StateDive(Player *pl, s32 unused)
{
    f32 move[3];
    f32 delta[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_DIVE_START, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 0x1E, 0, pl->pos, 1, 0);
        pl->noHitTimer = 3;
        break;
    case 1:
        pl->noHitTimer = 3;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_DIVE_AIR, 0, 0);
            pl->pos[1] += 100.0f;
            move[0] = 0.0f;
            move[1] = 0.0f;
            move[2] = 16.0f;
            Vec_Rotate(delta, move, (f32 *) pl->worldMtx);
            pl->vel[0] = delta[0];
            pl->vel[1] = 3.0f;
            pl->vel[2] = delta[2];
            Player_ClearAccel(pl);
            Player_StartJump(pl, 8);
        }
        break;
    case 2:
        pl->noHitTimer = 3;
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= pl->floorY) {
            pl->step++;
            pl->action = PL_POSTURE_STANDING;
            pl->pos[1] = pl->floorY;
            Player_SetMotionPair(pl, PL_MOT_DIVE_LAND, 0, 0);
            Vib_StartLocal(pl, 1);
            CamShake_ReqA(pl, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->noHitTimer = 0;
            if (pl->input.held & PAD_R2) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH_RESUME, 0);
            } else {
                pl->freeState = 1;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            }
        } else if (Motion_ReachedFrame(pl, 0, 30.0f) == 0) {
            pl->noHitTimer = 2;
        }
        break;
    }
}

extern s32 gNothingMoreText;
/* gathering at the player's spot (kind 0: a gather point, 1-3: a mining point); stat: 1, 2 count quest stats 0xE, 0xF. 1: an item was taken */
s32 Player_GatherAtSpot(Player *pl, u16 kind, u16 stat)
{
    u16 item;
    u16 sel;
    u16 id;
    s16 giveResult;

    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    switch (kind) {
    case 0:
        item = Gather_AtPoint(pl);
        break;
    case 1:
    case 2:
    case 3:
        item = Gather_AtMinePoint(pl);
        break;
    }
    if (item == 0 || item == 0xFFFE || item == 0xFFFF) {
        if (item != 0xFFFE) {
            switch (kind) {
            case 0:
                Notice_Push(NOTICE_KIND_MESSAGE, 0, 0);
                break;
            case 1:
                Notice_Push(NOTICE_KIND_MESSAGE, 0xE, 0);
                break;
            case 2:
                Notice_Push(NOTICE_KIND_MESSAGE, 0x1B, 0);
                break;
            case 3:
                Notice_Push(NOTICE_KIND_MESSAGE, 0, 0);
                break;
            }
        } else {
            Jingle_Play(pl, 0xA);
            Notice_PushItem((char *)gNothingMoreText);
        }
    } else {
        switch (item) {
        case 0x2A4:
            Notice_Push(NOTICE_KIND_ITEM, 0x13, item);
            break;
        case ITEM_ID_DELIVERY:
            Notice_Push(NOTICE_KIND_ITEM, 0x14, item);
            break;
        }
        switch (stat) {
        case 0:
            break;
        case 1:
            QuestStats_Add(0xE, 1);
            break;
        case 2:
            QuestStats_Add(0xF, 1);
            break;
        }
        if (kind == 1) {
            MsgEvent_Check(MSGEV_MINING);
        }
        giveResult = Pouch_GiveItem(pl, item, pl->gotItem, 3);
        switch (giveResult) {
        case 0:
        case 1:
        case 2:
            if (Item_CarryKind(item) & 0xFF) {
                pl->stateVar1 = 1;
                MsgEvent_Check(MSGEV_CARRY_ITEM);
                if (item == 0x2A3) {
                    Notice_Push(NOTICE_KIND_ITEM, 0x14, item);
                }
            }
            sel = pl->pouchCursor;
            id = pl->pouch[sel].item;
            if (id != 0 && !(gItemData[id].flags & 1)) {
                pl->pouchCursor = Pouch_NextSelectable(pl, sel, 0);
            }
            return 1;
        }
    }
    return 0;
}

/*
 * The standing gathers. variant: 0 PL_ACT_GATHER, 2 PL_ACT_TAKE_REWARDS
 * (Quest_GiveRewards), 3 PL_ACT_CARVE, 4 PL_ACT_MINE and 5 PL_ACT_BUG_NET
 * (with the tool pl->useItem), 6 PL_ACT_GATHER_SPECIAL, 7 PL_ACT_GATHER_DROP.
 * When pl->timer runs out (60 frames, 40 with a tool) the local player
 * gathers once (Player_GatherAtSpot); the pickaxe and the net break on a 1 in
 * n roll by the tool (skills change the odds). Skill 0x97 speeds the motion.
 * When the motion ends it returns to idle, or picks up a gathered carried
 * item (PL_GROUP_CARRY, PL_CARRY_PICKED_UP).
 */
void Player_StateGather(Player *pl, u32 variant)
{
    u16 n;

    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->timer = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        switch (variant) {
        case 0:
        case 1:
        case 6:
        case 7:
            Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -0xE, 0);
            if (Player_HasSkill(pl, 0x97) == 1) {
                pl->motion[0].step = 2.6f;
                pl->motion[1].step = 2.6f;
            }
            break;
        case 2:
            if (Player_IsLocal(pl) == 1) {
                Notice_Push(NOTICE_KIND_MESSAGE, 0xA, 0);
            }
            Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -0xE, 0);
            if (Player_HasSkill(pl, 0x97) == 1) {
                pl->motion[0].step = 2.6f;
                pl->motion[1].step = 2.6f;
            }
            break;
        case 3:
            Player_SetMotionPair(pl, PL_MOT_CARVE, -6, 0);
            break;
        case 4:
            Player_SetMotionPair(pl, PL_MOT_PICKAXE_SWING, 0, 0);
            if (Player_HasSkill(pl, 0x97) == 1) {
                pl->timer = 0x28;
                pl->motion[0].step = 2.8f;
                pl->motion[1].step = 2.8f;
            }
            break;
        case 5:
            Player_SetMotionPair(pl, PL_MOT_NET_SWING, 0, 0);
            if (Player_HasSkill(pl, 0x97) == 1) {
                pl->timer = 0x28;
                pl->motion[0].step = 2.8f;
                pl->motion[1].step = 2.8f;
            }
            break;
        }
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if (pl->timer > 0) {
                pl->timer--;
            }
            if (pl->timer == 0 && pl->stateVar0 == 0 && Quest_PhaseIsOver(1) == 0) {
                n = 1;
                pl->stateVar0 = 1;
                switch (variant) {
                case 4:
                case 5:
                    switch (pl->useItem) {
                    case 0x96:
                        n = 3;
                        break;
                    case 0x97:
                        n = 10;
                        break;
                    case 0x98:
                        n = 15;
                        break;
                    case 0x99:
                        n = 3;
                        break;
                    case 0x9A:
                        n = 10;
                        break;
                    case 0x9B:
                        n = 15;
                        break;
                    }
                    if (Player_HasSkill(pl, 0x9A) == 1 && (Rand_Next(1) & 3) == 0) {
                        n = n / 2U + 1;
                    }
                    if (Player_HasSkill(pl, 0x9B) == 1 && (Rand_Next(1) & 1) == 0) {
                        n = n / 2U + 1;
                    }
                    if (Rand_Next(1) % n == 0
                        && (Player_HasSkill(pl, 0x98) != 1 || (Rand_Next(1) & 3))
                        && (Player_HasSkill(pl, 0x99) != 1 || (Rand_Next(1) & 1))) {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_TOOL_LOST, pl->useItem);
                        Pouch_Add(pl, pl->useItem, -1);
                    }
                default:
                    switch (variant) {
                    default:
                        Player_GatherAtSpot(pl, 0, 0);
                        break;
                    case 4:
                        Player_GatherAtSpot(pl, 0, 1);
                        break;
                    case 5:
                        Player_GatherAtSpot(pl, 0, 2);
                        break;
                    }
                    break;
                case 6:
                    Player_GatherAtSpot(pl, 2, 0);
                    break;
                case 7:
                    Player_GatherAtSpot(pl, 3, 0);
                    break;
                case 2:
                    Quest_GiveRewards(pl);
                    break;
                case 3:
                    Player_GatherAtSpot(pl, 1, 0);
                    break;
                }
                break;
            }
        }
        if (pl->motion[0].playing == 0) {
            if (pl->stateVar1 == 0) {
                switch (variant) {
                case 0:
                case 1:
                case 2:
                case 6:
                case 7:
                    Player_ReturnToIdle(pl, 0, 0xA, 0);
                    break;
                case 3:
                case 5:
                    Player_ReturnToIdle(pl, 0, 6, 0);
                    break;
                default:
                    Player_ReturnToIdle(pl, 0, 4, 0);
                    break;
                }
            } else {
                Player_UpdateCarryKind(pl);
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_PICKED_UP, 0);
            }
        } else if (Motion_PassedFrame(pl, 0, 100.0f) != 0 && variant == 3 && Player_HasSkill(pl, 0x97) == 1) {
            Player_SetMotionPair(pl, PL_MOT_CARVE, -8, 0xE0);
        }
        break;
    }
}

/*
 * The crouched forms of Player_StateGather's gather, carve, special and drop.
 * variant: 0 PL_ACT_GATHER_CROUCHED, 1 PL_ACT_CARVE_CROUCHED, 2
 * PL_ACT_GATHER_SPECIAL_CROUCHED, 3 PL_ACT_GATHER_DROP_CROUCHED; the variant
 * is also the kind passed to Player_GatherAtSpot, which the local player
 * calls once at pl->timer 60. Ends in PL_ACT_CROUCH_HOLD, or the pickup of a
 * carried item.
 */
void Player_StateGatherCrouched(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->timer = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        switch (variant) {
        case 0:
        case 2:
        case 3:
            Player_SetMotionPair(pl, PL_MOT_GATHER_FROM_CROUCH, -6, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_CARVE_FROM_CROUCH, -6, 0);
            break;
        }
        if (Player_HasSkill(pl, 0x97) == 1) {
            pl->motion[0].step = 2.6f;
            pl->motion[1].step = 2.6f;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            switch (variant) {
            case 0:
            case 2:
            case 3:
                Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -4, 0x24);
                break;
            case 1:
                Player_SetMotionPair(pl, PL_MOT_CARVE, 0, 0x44);
                break;
            }
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->timer >= 0x3C) && (pl->stateVar0 == 0) && (Quest_PhaseIsOver(1) == 0)) {
            pl->stateVar0 = 1;
            Player_GatherAtSpot(pl, variant & 0xFFFF, 0);
        }
        switch (variant) {
        case 0:
        case 2:
        case 3:
            if (Motion_ReachedFrame(pl, 0, 200.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_GATHER_TO_CROUCH, -4, 0);
            }
            break;
        case 1:
            if (Motion_ReachedFrame(pl, 0, 294.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_CARVE_TO_CROUCH, -2, 0);
            }
            break;
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            if (pl->stateVar1 == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, PL_SET_NO_SYNC);
                break;
            }
            Player_UpdateCarryKind(pl);
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_PICKED_UP, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SPIT_ROAST_START: sets up the spit roast (Player_UseItem with a
 * meat, or an action spot of kind 1): clears cookTimer and cookResult and
 * turns the camera to the spit (CamCtl_TurnToPlayerFront); then
 * PL_ACT_SPIT_ROAST.
 */
void Player_StateSpitRoastStart(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        pl->cookTimer = 0;
        pl->cookResult = 0;
        Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_START, -4, 0);
        CamCtl_TurnToPlayerFront((u8 *) pl);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST, PL_SET_NO_SYNC);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_SPIT_ROAST_SUCCESS (done or gourmet), 1
 * PL_ACT_SPIT_ROAST_FAIL (raw, rare or burnt): one motion, then idle.
 */
void Player_StateSpitRoastEnd(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_SUCCESS, 0, 0);
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_FAIL, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The fishing input (Player_StateFishWait), local player: returns to idle
 * when the state group changed or the quest is over. In PL_ACT_FISH_WAIT,
 * PAD_CIRCLE reels in: PL_ACT_FISH_REEL with a bite on the line (fishTimer
 * above 0), else PL_ACT_FISH_REEL_BAITED or PL_ACT_FISH_REEL_EMPTY by baited.
 */
void Player_UpdateFishingInput(Player *pl)
{

    if (Player_IsLocal(pl) != 0) {
        if (pl->stateGroup != PL_GROUP_ACTION) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            return;
        }
        if (Quest_PhaseIsOver(1) == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            return;
        }
        switch (pl->state) {
        case 0x4E:
        case 0x4F:
        case 0x53:
            break;
        case 0x50:
            if (pl->input.pressed & PAD_CIRCLE) {
                if (pl->fishTimer > 0) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL, 0);
                } else if (pl->baited == 1) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL_BAITED, 0);
                } else {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL_EMPTY, 0);
                }
            }
            break;
        case 0x51:
        case 0x52:
            if (pl->input.pressed & PAD_CIRCLE) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL, 0);
            }
            break;
        }
    }
}

/*
 * variant 0 PL_ACT_FISH_READY, 1 PL_ACT_FISH_WAIT (the line out): notes a
 * bite in stateVar1 and runs Player_UpdateFishingInput.
 */
void Player_StateFishWait(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        pl->timer = 0;
        pl->stateVar1 = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_FISH_READY, 0, 0);
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_FISH_WAIT, 2, 0);
        break;
    case 1:
        if (((s8) pl->fishTimer != 0) && (pl->stateVar1 == 0)) {
            pl->stateVar1 = 1;
        }
        Player_UpdateFishingInput(pl);
        break;
    }
}

/*
 * PL_ACT_FISH_CAST: casts the line at an action spot of kind 2 (sets baited);
 * at frame 104 makes the lure (Lure_Spawn), then PL_ACT_FISH_WAIT.
 */
void Player_StateFishCast(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        pl->fishTimer = 0;
        pl->gotItemId = 0;
        pl->baited = 1;
        Player_SetMotionPair(pl, PL_MOT_FISH_CAST, -2, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 104.0f) != 0) {
            if (pl->useItem == 0x91) {
                Lure_Spawn(pl, 1);
            } else {
                Lure_Spawn(pl, 0);
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_WAIT, 0);
        }
        break;
    }
}

/*
 * PL_ACT_FISH_REEL: reels in. A monster that took this player's bait (hooked,
 * hookedBy) is kept in stateVar0. With none, the catch: the local player uses
 * up the bait, gets gotItemId (Pouch_GiveItem) with a jingle and counts the
 * quest stat; with a monster on the line the reel repeats twice and the bait
 * is lost. Then idle.
 */
void Player_StateFishReel(Player *pl, s32 variant)
{
    Em *em = gEms;
    s16 i;

    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->msgTimer = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->area == pl->area && em->hooked != 0 && em->hookedBy == pl->index) {
                pl->stateVar0 = 1;
            }
        }
        Player_SetMotionPair(pl, PL_MOT_FISH_REEL, 2, 0);
        break;
    case 1:
        if (pl->stateVar0 == 0) {
            if (Motion_PassedFrame(pl, 0, 118.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_FISH_CATCH, 2, 0);
                break;
            }
            for (i = 0; i < EM_COUNT; i++, em++) {
                if (em->area == pl->area && em->hooked != 0 && em->hookedBy == pl->index) {
                    pl->stateVar0 = 1;
                }
            }
            break;
        }
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar1++;
            if (pl->stateVar1 >= 2) {
                pl->step = 3;
                Player_SetMotionPair(pl, PL_MOT_FISH_PULLED_DOWN, 4, 0);
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(pl, pl->useItem, -1);
                }
            }
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 120.0f) != 0 && Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_FISHED, pl->gotItemId);
            Jingle_Play(pl, 7);
            Pouch_Add(pl, pl->useItem, -1);
            i = pl->gotItem;
            if ((s16) Pouch_GiveItem(pl, pl->gotItemId, i, 0) == 3) {
                Player_NoticePouchFull(pl->gotItemId);
            }
            QuestStats_Add(0xD, 1);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 2, 0x24);
        }
        break;
    case 4:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_FISH_REEL_EMPTY (the bait lost), 1 PL_ACT_FISH_REEL_BAITED
 * (the bait kept): reels in nothing; the local player gets a notice, variant
 * 0 uses up the bait. Then idle.
 */
void Player_StateFishReelEmpty(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_FISH_REEL_EMPTY, 2, 0);
        break;
    case 1:
        if ((Motion_PassedFrame(pl, 0, 120.0f) != 0) && (Player_IsLocal(pl) == 1) && (Quest_PhaseIsOver(1) == 0)) {
            if (variant == 0) {
                Jingle_Play(pl, 0xA);
                Pouch_Add(pl, (s32) pl->useItem, -1);
                Notice_Push(NOTICE_KIND_MESSAGE, 3, 0);
            } else {
                Notice_Push(NOTICE_KIND_MESSAGE, 0xD, 0);
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * Returns the item's row of gSpitMeats (the spit roast's timings and
 * results), 0 to 12; 0 for the first meat item and for any item not in the
 * list.
 */
u8 SpitMeat_Kind(u16 item)
{
    u8 kind = 0;

    switch (item) {
    case 0x294:
        kind = 0;
        break;
    case 0x9C:
        kind = 1;
        break;
    case 0x9D:
        kind = 2;
        break;
    case 0x9E:
        kind = 3;
        break;
    case 0x9F:
        kind = 4;
        break;
    case 0xA0:
        kind = 5;
        break;
    case 0xA4:
        kind = 6;
        break;
    case 0xA5:
        kind = 7;
        break;
    case 0xA6:
        kind = 8;
        break;
    case 0xA7:
        kind = 9;
        break;
    case 0xA3:
        kind = 10;
        break;
    case 0xA8:
        kind = 11;
        break;
    case 0xA9:
        kind = 12;
        break;
    }
    return kind;
}

/*
 * PL_ACT_SPIT_ROAST: turns the meat. The row m of gSpitMeats (SpitMeat_Kind
 * of pl->useItem) times cookResult as cookTimer counts: 0 raw, 1 rare (from
 * m->rare), 2 done (m->done), 4 gourmet (m->gourmetLen frames from m->gourmet
 * after done), 3 burnt (m->doneLen after done); skills 0xC6 to 0xC8 shift the
 * windows. The local player takes the meat off with PAD_CIRCLE or at the time
 * limit: the meat is used up and, from rare on, the result's item given with
 * a jingle and a notice. Raw, rare and burnt request PL_ACT_SPIT_ROAST_FAIL,
 * done and gourmet PL_ACT_SPIT_ROAST_SUCCESS.
 */
void Player_StateSpitRoast(Player *pl, s32 unused)
{
    SpitMeat *meat;
    s16 done;
    s16 doneLen;
    s16 gourmet;
    s16 gourmetLen;
    s16 limit;
    s32 j;
    s16 n;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    meat = &gSpitMeats[SpitMeat_Kind(pl->useItem)];
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_TURN, 4, Rand_Next(1) & 0x3F);
        pl->cookTimer = 0;
        pl->cookResult = 0;
        Jingle_Play(pl, meat->jingle);
        break;
    case 1:
        pl->cookTimer++;
        done = meat->done;
        doneLen = meat->doneLen;
        gourmet = meat->gourmet;
        gourmetLen = meat->gourmetLen;
        if (Player_HasSkill(pl, 0xC6) == 1) {
            done -= 2;
            doneLen += 4;
            if (gourmet > 0) {
                gourmet -= 1;
                gourmetLen += 2;
            }
        } else if (Player_HasSkill(pl, 0xC7) == 1) {
            done -= 4;
            doneLen += 8;
            if (gourmet > 0) {
                gourmet -= 2;
                gourmetLen += 4;
            }
        } else if (Player_HasSkill(pl, 0xC8) == 1) {
            doneLen /= 2;
            if (gourmet > 0) {
                gourmet /= 2;
                gourmetLen /= 2;
            }
        }
        if (pl->cookTimer >= done + doneLen) {
            pl->cookResult = 3;
        } else if (pl->cookTimer >= done) {
            if (gourmet > 0 && pl->cookTimer >= done + gourmet && pl->cookTimer < gourmetLen + (done + gourmet)) {
                pl->cookResult = 4;
            } else {
                pl->cookResult = 2;
            }
        } else if (pl->cookTimer >= meat->rare) {
            pl->cookResult = 1;
        } else {
            pl->cookResult = 0;
        }
        if (SpitMeat_Kind(pl->useItem) == 2) {
            limit = 180;
        } else {
            limit = 330;
        }
        if (Player_IsLocal(pl) == 1 && ((pl->input.pressed & PAD_CIRCLE) || pl->cookTimer >= limit)) {
            Bgm_StopEvent(pl);
            switch (pl->cookResult) {
            case 0:
                if (Quest_PhaseIsOver(1) == 0) {
                    Notice_Push(NOTICE_KIND_MESSAGE, 0xB, 0);
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            case 1:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (meat->raw == 0) {
                        Pouch_Add(pl, 0x28, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 0xA2, 1);
                    }
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_COOKED, meat->itemRare);
                    Jingle_Play(pl, 0xE);
                    n = pl->gotItem;
                    if ((s16) Pouch_GiveItem(pl, meat->itemRare, n, 0) == 3) {
                        Player_NoticePouchFull(meat->itemRare);
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            case 2:
            case 4:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (meat->raw == 0) {
                        Pouch_Add(pl, 0x28, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 0xA2, 1);
                    }
                    if (pl->cookResult == 4) {
                        switch (meat->jingle) {
                        case 0x1E:
                            j = 0x22;
                            break;
                        case 0x19:
                            j = 0x1B;
                            break;
                        case 0x16:
                            j = 0x18;
                            break;
                        case 0x13:
                            j = 0x15;
                            break;
                        case 0x1F:
                        case 0x20:
                        default:
                            j = 0x12;
                            break;
                        }
                    } else {
                        switch (meat->jingle) {
                        case 0x1E:
                            j = 0x21;
                            break;
                        case 0x19:
                            j = 0x1A;
                            break;
                        case 0x16:
                            j = 0x17;
                            break;
                        case 0x13:
                            j = 0x14;
                            break;
                        case 0x1F:
                        case 0x20:
                        default:
                            j = 0xF;
                            break;
                        }
                    }
                    Jingle_Play(pl, j);
                    if (pl->cookResult == 4) {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_COOKED, meat->itemGourmet);
                        n = pl->gotItem;
                        if ((s16) Pouch_GiveItem(pl, meat->itemGourmet, n, 0) == 3) {
                            Player_NoticePouchFull(meat->itemGourmet);
                        }
                    } else {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_COOKED, meat->itemDone);
                        n = pl->gotItem;
                        if ((s16) Pouch_GiveItem(pl, meat->itemDone, n, 0) == 3) {
                            Player_NoticePouchFull(meat->itemDone);
                        }
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_SUCCESS, 0);
                break;
            case 3:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (meat->raw == 0) {
                        Pouch_Add(pl, 0x28, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 0xA2, 1);
                    }
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_BURNT, meat->itemBurnt);
                    Jingle_Play(pl, 0x10);
                    n = pl->gotItem;
                    if ((s16) Pouch_GiveItem(pl, meat->itemBurnt, n, 0) == 3) {
                        Player_NoticePouchFull(meat->itemBurnt);
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            }
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_START_COUNTDOWN (strikes the countdown spot, an action
 * spot of kind 9: Quest_StartCountdown), 1 PL_ACT_SPOT_ITEM_STRIKE (the spot
 * item at an action spot of kind 0xA: StageShot_CreateForPlayer type 7, one item
 * used up). Then idle.
 */
void Player_StateSpotStrike(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_PICKAXE_SWING, 0, 0);
        if (variant == 0) {
            Quest_StartCountdown((Player *) pl);
            break;
        }
        StageShot_CreateForPlayer(pl, STAGESHOT_ARENA_STRIKE);
        Pouch_Add(pl, (s32) pl->useItem, -1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_SUPPLY_BOX, 1 PL_ACT_DELIVERY_BOX: stands hit-free while
 * the box's menu is open (supplyBoxOpen, or deliveryOpen with the cursor
 * reset); returns to idle once the menu clears its flag or the quest is over.
 */
void Player_StateBoxMenu(Player *pl, s32 variant)
{
    u8 isOpen;

    pl->noPushTimer = 2;
    pl->noHitTimer = 5;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, 6, 0);
        }
        if (variant == 0) {
            pl->supplyBoxOpen = 1;
        } else {
            pl->deliveryOpen = 1;
            pl->menuCursor = 0;
            pl->menuPage = 0;
        }
        pl->menuDelay = 0;
        if (Player_IsLocal(pl) == 1) {
            Sound_PlaySeFlat(1, 0x8E, 0);
        }
        break;
    case 1:
        if (variant == 0) {
            isOpen = pl->supplyBoxOpen;
        } else {
            isOpen = pl->deliveryOpen;
        }
        if (isOpen & 0xFF) {
            if (Quest_PhaseIsOver(1) == 1) {
                goto block_17;
            }
        } else {
block_17:
            pl->noHitTimer = 0;
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_DRINK, 1 PL_ACT_EAT. When the motion ends the local player
 * requests PL_ACT_APPLY_ITEM, another player's copy returns to idle. Skill
 * 0xBC speeds the eating; skill 0xBD replays part of it up to 4 times.
 */
void Player_StateConsume(Player *pl, s32 variant)
{
    u8 stateVar0;

    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_DRINK, 4, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_EATING, 4, 0);
            break;
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_APPLY_ITEM, 0);
                break;
            }
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (variant == 1) {
            if ((Player_HasSkill(pl, 0xBD) == 1) && (Motion_PassedFrame(pl, 0, 62.0f) != 0)) {
                stateVar0 = pl->stateVar0;
                if ((s32) stateVar0 < 4) {
                    pl->stateVar0 = stateVar0 + 1;
                    pl->stateVar1 = 1;
                    Player_SetMotionPair(pl, PL_MOT_EATING, 4, 0x34);
                }
            }
            if ((Motion_ReachedFrame(pl, 0, 38.0f) != 0) && (Motion_ReachedFrame(pl, 0, 180.0f) == 0) && (Player_HasSkill(pl, 0xBC) == 1)) {
                pl->motion[0].step = 3.6f;
                pl->motion[1].step = 3.6f;
                break;
            }
            pl->motion[0].step = 2.0f;
            pl->motion[1].step = 2.0f;
        } else {
        }
        break;
    }
}

/*
 * PL_ACT_APPLY_ITEM: at entry the local player applies the item just consumed
 * (Item_Use of pl->useItem); then idle.
 */
void Player_StateApplyItem(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_Use(pl);
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_ACT_KNEEL_TO_PLACE_CROUCHED: as Player_StateKneelToPlace from the
 * crouch; at frame 20 the local player requests PL_ACT_PLACE_ITEM_CROUCHED.
 */
void Player_StateKneelToPlaceCrouched(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_PLACE_ITEM_CROUCHED, 2, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 20.0f) != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_ITEM_CROUCHED, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

/*
 * Starts the trade cooldown: pl->tradeCooldown = 9000 frames unless it still
 * runs.
 */
static void Player_StartTradeTimer(Player *pl)
{
    if (pl->tradeCooldown <= 0) {
        pl->tradeCooldown = 9000;
    }
}

/*
 * PL_ACT_TRADE: trades with the trader (EM_KIND_TRADER; from
 * Player_StartSpotAction). Stands hit-free while its message shows
 * (tradeMsgTimer 60), then idle. tradeCount counts the trades up to 7; from 4
 * on half of them jump to 7, and reaching 7 starts the cooldown
 * (Player_StartTradeTimer).
 */
void Player_StateTrade(Player *pl, s32 variant)
{

    pl->noHitTimer = 3;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, -6, 0);
        }
        pl->tradeMsgTimer = 60;
        if (pl->tradeCount < 7) {
            pl->tradeCount++;
            if (pl->tradeCount >= 4 && (Rand_Next(1) & 1)) {
                pl->tradeCount = 7;
                Player_StartTradeTimer(pl);
            }
        } else {
            pl->tradeCount = 7;
            Player_StartTradeTimer(pl);
        }
        pl->timer = 60;
        break;
    case 1:
        if (pl->tradeMsgTimer == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_HORN_PLAY (the horn item), 1 PL_ACT_TEAM_ITEM_USE (a team
 * item). The motion repeats: the horn up to 3 times (PAD_CROSS from frame 44
 * ends it), then PL_ACT_HORN_END; the team item twice, then
 * PL_ACT_TEAM_ITEM_END.
 */
void Player_StateInstrumentPlay(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_HORN_PLAY, 4, 0);
            Player_SetFlag(pl, 0x10000);
        } else {
            Player_SetMotionPair(pl, PL_MOT_TEAM_ITEM_USE, 4, 0);
        }
        pl->motionTime = 0;
        break;
    case 1:
        switch (variant) {
        case 0:
            if (pl->stateVar0 >= 3 || (Motion_ReachedFrame(pl, 0, 44.0f) != 0 && (pl->input.pressed & PAD_CROSS))) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_HORN_END, 0);
            } else if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
                pl->stateVar0++;
            }
            break;
        case 1:
            if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
                pl->stateVar0++;
                if (pl->stateVar0 >= 2) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TEAM_ITEM_END, 0);
                }
            }
            break;
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_HORN_END, 1 PL_ACT_TEAM_ITEM_END (the local player applies
 * the team item, Item_UseTeamPowder). At entry the local player may use up
 * pl->useItem on a 1 in n roll by the item; skills 0x98 to 0x9B and the
 * hunting horn change the odds. Then idle.
 */
void Player_StateInstrumentEnd(Player *pl, s32 variant)
{
    u16 chance = 1;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            chance = 5;
            Player_SetMotionPair(pl, PL_MOT_HORN_END, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_TEAM_ITEM_END, 4, 0);
            if (Player_IsLocal(pl) == 1) {
                Item_UseTeamPowder(pl);
            }
            switch (pl->useItem) {
            case ITEM_ID_TEAM_HEAL:
                chance = 12;
                break;
            case ITEM_ID_TEAM_CURE:
                chance = 12;
                break;
            case ITEM_ID_TEAM_ATTACK:
                chance = 8;
                break;
            case ITEM_ID_TEAM_DEFENSE:
                chance = 8;
                break;
            }
        }
        if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            if (Player_HasSkill(pl, 0x9A) == 1 && !(Rand_Next(1) & 3)) {
                chance = chance / 2 + 1;
            }
            if (Player_HasSkill(pl, 0x9B) == 1 && !(Rand_Next(1) & 1)) {
                chance = chance / 2 + 1;
            }
            if (Rand_Next(1) % chance == 0) {
                if (Player_HasSkill(pl, 0x98) != 1 || (Rand_Next(1) & 3)) {
                    if ((Player_HasSkill(pl, 0x99) != 1 || (Rand_Next(1) & 1)) && (pl->weaponType != WEAPON_HUNTING_HORN || (Rand_Next(1) & 3))) {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_TOOL_LOST, pl->useItem);
                        Pouch_Add(pl, pl->useItem, -1);
                    }
                }
            }
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SHARPEN: sharpens with the whetstone pl->useItem. At frame 260 one
 * item is used up and Player_AddSharpness adds 100 to 200 by the item (half
 * with skill 0x27); skill 0x26 skips part of the motion. Then idle.
 */
void Player_StateSharpen(Player *pl, s32 unused)
{
    s16 n;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_SHARPEN, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (Player_HasSkill(pl, 0x26) == 1 && Motion_PassedFrame(pl, 0, 98.0f) != 0) {
            Player_SetMotionPair(pl, PL_MOT_SHARPEN, 0, 0xEE);
        }
        if (Motion_PassedFrame(pl, 0, 260.0f) != 0 && Quest_PhaseIsOver(1) == 0) {
            Pouch_Add(pl, pl->useItem, -1);
            switch (pl->useItem) {
            case 0xF2:
                n = 200;
                break;
            case 0xAC:
            case 0xEA:
                n = 150;
                break;
            case 0x295:
                n = 100;
                break;
            }
            if (Player_HasSkill(pl, 0x27) == 1) {
                n = n / 2;
            }
            Player_AddSharpness(pl, n);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Looks through the binoculars in the aim view (aimMode 1). variant 0
 * PL_ACT_BINOCULARS (crouches first), 1 PL_ACT_BINOCULARS_CROUCHED.
 * Player_UpdateLean aims; PAD_CROSS or the quest's end ends it
 * (Player_ResetAim, PL_ACT_CROUCH_HOLD).
 */
void Player_StateBinoculars(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        pl->aimMode = 1;
        pl->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_CROUCH_DOWN, 2, 0);
            break;
        }
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_CROUCH, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
            Player_UpdateLean(pl, 8);
            break;
        }
        Player_UpdateLean(pl, 7);
        break;
    case 2:
        Player_UpdateLean(pl, 8);
        if (Player_IsLocal(pl) != 0) {
            if (!(pl->input.pressed & PAD_CROSS)) {
                if (Quest_PhaseIsOver(1) == 1) {
                    goto block_15;
                }
            } else {
block_15:
                Player_ResetAim(pl);
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_OFFER_ITEM: holds an item out to another player (offerItem,
 * offerCount, offerTo). The local player ends it (PL_ACT_OFFER_ITEM_END) with
 * PAD_CROSS or once offerState leaves PL_OFFER_WAITING (a taker asked:
 * QSYNC_PL_ITEM_ASK).
 */
void Player_StateOfferItem(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_OFFER_ITEM, 4, 0);
        pl->offerState = PL_OFFER_WAITING;
        break;
    case 1:
        if ((Player_IsLocal(pl) == 1) && ((pl->input.pressed & PAD_CROSS) || (pl->offerState != PL_OFFER_WAITING))) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM_END, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TAKE_ITEM_WAIT: waits for the giver's answer
 * (Player_TakeOfferedItem). PL_OFFER_RECEIVED (the item arrived:
 * QSYNC_PL_ITEM_ANSWER) requests PL_ACT_TAKE_ITEM; PL_OFFER_REFUSED or 601
 * frames without an answer return to idle.
 */
void Player_StateTakeItemWait(Player *pl, s32 unused)
{
    s32 timer;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, 6, 0);
        }
        pl->offerState = PL_OFFER_WAITING;
        pl->timer = 0;
        break;
    case 1:
        timer = pl->timer + 1;
        pl->timer = timer;
        if ((timer >= 0x259) || (pl->offerState != PL_OFFER_WAITING)) {
            if (pl->offerState == PL_OFFER_RECEIVED) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM, 0);
                break;
            }
            Player_ReturnToIdle(pl, 0, 2, 0);
        } else {
        }
        break;
    }
}

/* PL_ACT_OFFER_ITEM_END: the giver's closing motion, then idle. */
void Player_StateOfferItemEnd(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_OFFER_ITEM_END, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TAKE_ITEM: the taker's motion once the item has arrived, then idle.
 */
void Player_StateTakeItem(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_TAKE_ITEM, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The lance's and the gunlance's hop. variant: 0 PL_ACT_LANCE_HOP_SIDE_A, 1
 * PL_ACT_LANCE_HOP_SIDE_B, 2 PL_ACT_LANCE_HOP_BACK, 3
 * PL_ACT_LANCE_HOP_BACK_LONG. Costs 75 stamina and sets the hit-free frames.
 * From 30 frames before the motion's end a flick of the attack stick picks
 * the next weapon state into stateVar1 (0xFF none): down LANCE_UPPER_THRUST,
 * or for the gunlance GLANCE_HOP_RELOAD (clip not full) or
 * GLANCE_UPPER_THRUST; any other flick LANCE_THRUST or GLANCE_THRUST. From 10
 * frames before the end the left stick steers and the picked state starts in
 * PL_GROUP_WEAPON; with none it returns to idle.
 */
void Player_StateLanceHop(Player *pl, s32 variant)
{
    f32 move[3];
    f32 delta[3];
    u16 flickAngle;
    u16 steerAngle;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetEvadeFrames(pl);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 0x1E, 0, pl->pos, 1, 0);
        pl->stateVar1 = 0xFF;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_LANCE_HOP_SIDE_A, 2, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_LANCE_HOP_SIDE_B, 2, 0);
            break;
        case 2:
        case 3:
            Player_SetMotionPair(pl, PL_MOT_LANCE_HOP_BACK, 2, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        case 1:
            if (pl->weaponType == WEAPON_GUNLANCE && Motion_ReachedFrame(pl, 0, 20.0f) == 0) {
                if (variant == 0) {
                    move[0] = 9.0f;
                } else {
                    move[0] = -9.0f;
                }
                move[1] = 0.0f;
                move[2] = 0.0f;
                Vec_Rotate(delta, move, pl->rotMtx);
                pl->pos[0] += delta[0];
                pl->pos[2] += delta[2];
            }
            break;
        case 3:
            if (Motion_ReachedFrame(pl, 0, 28.0f) == 0) {
                move[0] = 0.0f;
                move[1] = 0.0f;
                move[2] = -8.0f;
                Vec_Rotate(delta, move, pl->rotMtx);
                pl->pos[0] += delta[0];
                pl->pos[2] += delta[2];
            }
            break;
        }
        if (Motion_ReachedFrame(pl, 0, pl->motion[0].frameCount - 30.0f) != 0 && Player_IsLocal(pl) == 1) {
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                flickAngle = pl->input.rightStickAngle + 0x1555;
                switch (pl->weaponType) {
                case WEAPON_LANCE:
                    if (flickAngle < 0xAAAC) {
                        pl->stateVar1 = 0;
                    } else {
                        pl->stateVar1 = 1;
                    }
                    break;
                case WEAPON_GUNLANCE:
                    if (flickAngle < 0xAAAC) {
                        pl->stateVar1 = 0;
                    } else if (pl->roundsLoaded < pl->clipSize) {
                        pl->stateVar1 = 0x1B;
                    } else {
                        pl->stateVar1 = 4;
                    }
                    break;
                }
            }
            if (Motion_ReachedFrame(pl, 0, pl->motion[0].frameCount - 10.0f) != 0 && pl->stateVar1 != 0xFF) {
                if (pl->input.stickDir & PAD_DPAD) {
                    steerAngle = pl->input.stickAngle - 0x1555;
                    if (steerAngle >= 0xAAAB) {
                        if (pl->weaponType == WEAPON_GUNLANCE) {
                            pl->targetYaw -= 0x1555;
                        } else {
                            pl->targetYaw -= 0x1000;
                        }
                    } else if (steerAngle >= 0x5555) {
                        if (pl->weaponType == WEAPON_GUNLANCE) {
                            pl->targetYaw += 0x1555;
                        } else {
                            pl->targetYaw += 0x1000;
                        }
                    }
                }
                Player_SetState(pl, PL_GROUP_WEAPON, pl->stateVar1, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        }
        if (pl->motion[0].playing == 0) {
            if (variant == 2) {
                Player_ReturnToIdle(pl, 0, 6, 0);
            } else {
                Player_ReturnToIdle(pl, 0, 8, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_PLACE_LARGE_ITEM: crouches to place a large item (pl->useItem); at
 * frame 78 the local player requests PL_ACT_PLACE_LARGE_ITEM_SET.
 */
void Player_StatePlaceLargeItem(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, PL_MOT_PLACE_LARGE_ITEM, 6, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 78.0f) != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM_SET, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
        }
        break;
    case 2:
        Player_SetMotionPair(pl, PL_MOT_PLACE_LARGE_ITEM, 0, 0x4E);
        break;
    }
}

/*
 * PL_ACT_PLACE_LARGE_ITEM_SET: the local player puts it down (Item_UsePlaced:
 * the placed object, one item used up); then idle.
 */
void Player_StatePlaceLargeItemSet(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, PL_MOT_PLACE_LARGE_ITEM, 0, 0x4E);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_UsePlaced(pl);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * PL_ACT_HORN_ITEM_PLAY: plays one of the horn items, as PL_ACT_HORN_PLAY:
 * the item sets the rounds (stateVar0, 1 to 3) and the use-up odds
 * (stateVar1: 1 in 8, 12 or 16). At the end the local player may use the item
 * up (skills and the hunting horn save it sometimes); then idle.
 */
void Player_StateHornItemPlay(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 1;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_HORN_PLAY, 4, 0);
        Player_SetFlag(pl, 0x10000);
        pl->motionTime = 0;
        switch (pl->useItem) {
        case 0x2B0:
        case 0x2B1:
        case 0x2B4:
        case 0x2B5:
            pl->stateVar0 = 2;
            pl->stateVar1 = 16;
            break;
        case 0x2B2:
            pl->stateVar0 = 1;
            pl->stateVar1 = 12;
            break;
        case 0x2B3:
        case 0x2B6:
            pl->stateVar0 = 3;
            pl->stateVar1 = 8;
            break;
        }
        break;
    case 1:
        if (pl->stateVar0 <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_HORN_END, 2, 0);
            if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0 && Rand_Next(1) % pl->stateVar1 == 0) {
                if (Player_HasSkill(pl, 0x98) == 1) {
                    if (!(Rand_Next(1) & 3)) {
                        break;
                    }
                }
                if (Player_HasSkill(pl, 0x99) == 1) {
                    if (!(Rand_Next(1) & 1)) {
                        break;
                    }
                }
                if (pl->weaponType == WEAPON_HUNTING_HORN) {
                    if (!(Rand_Next(1) & 3)) {
                        break;
                    }
                }
                Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_TOOL_LOST, pl->useItem);
                Pouch_Add(pl, pl->useItem, -1);
            }
        } else if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar0--;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_ACTION (idle, moving,
 * climbing, items, the bed, fishing, gathering, the lance hops). States that
 * share a handler pass a variant (each handler's comment lists them); a state
 * with no case returns to idle.
 */
void Player_RunActionState(Player *pl)
{
    switch (pl->state) {
    case PL_ACT_IDLE:
        Player_StateIdle(pl, 0);
        break;
    case PL_ACT_RUN:
        Player_StateMove(pl, 0U);
        break;
    case PL_ACT_WALK:
        Player_StateMove(pl, 1U);
        break;
    case PL_ACT_WALK_DRAWN:
        Player_StateMove(pl, 2U);
        break;
    case PL_ACT_DRAW:
        Player_StateDrawSheathe(pl, 0U);
        break;
    case PL_ACT_SHEATHE:
        Player_StateDrawSheathe(pl, 1U);
        break;
    case PL_ACT_LEAP_OFF_LEDGE:
        Player_StateLeapOrFall(pl, 0);
        break;
    case PL_ACT_GUARD_WALK:
        Player_StateMove(pl, 8U);
        break;
    case PL_ACT_CROUCH:
        Player_StateCrouch(pl, 0);
        break;
    case PL_ACT_FALL:
        Player_StateLeapOrFall(pl, 1);
        break;
    case PL_ACT_SHEATHE_MOVING:
        Player_StateDrawSheathe(pl, 2U);
        break;
    case PL_ACT_THROW_ITEM_ALT:
        Player_StateThrowItem(pl, 1);
        break;
    case PL_ACT_BED_MENU:
        Player_StateBedMenu(pl, 0);
        break;
    case PL_ACT_TO_IDLE:
        Player_StateToIdle(pl, 0);
        break;
    case PL_ACT_THROW_ITEM:
        Player_StateThrowItem(pl, 0);
        break;
    case PL_ACT_SHEATHE_TO_CROUCH:
        Player_StateDrawSheathe(pl, 3U);
        break;
    case PL_ACT_SHEATHE_TO_USE_ITEM:
        Player_StateDrawSheathe(pl, 4U);
        break;
    case PL_ACT_LEDGE_HANG:
        Player_StateLedgeHang(pl, 0);
        break;
    case PL_ACT_RIDE_WALK:
        Player_StateMove(pl, 3U);
        break;
    case PL_ACT_MOVE_KEEP_MOTION:
        Player_StateMove(pl, 4U);
        break;
    case PL_ACT_PLACE_ITEM_CROUCHED:
        Player_StatePlaceItem(pl, 1);
        break;
    case PL_ACT_JUMP_TO_LEDGE:
        Player_StateClimbAirborne(pl, 0);
        break;
    case PL_ACT_RIDE_STAGGER:
        Player_StateRideStagger(pl, 0);
        break;
    case PL_ACT_DRAW_MOVING:
        Player_StateDrawSheathe(pl, 5U);
        break;
    case PL_ACT_PLACE_ITEM:
        Player_StatePlaceItem(pl, 0);
        break;
    case PL_ACT_KNEEL_TO_PLACE:
        Player_StateKneelToPlace(pl, 0);
        break;
    case PL_ACT_CROUCH_WALK_ALT:
        Player_StateMove(pl, 5U);
        break;
    case PL_ACT_LEDGE_GRAB:
        Player_StateLedgeHang(pl, 1);
        break;
    case PL_ACT_ROLL:
        Player_StateRoll(pl, 0);
        break;
    case PL_ACT_CROUCH_HOLD:
        Player_StateCrouch(pl, 1);
        break;
    case PL_ACT_STEP_UP:
        Player_StateStepUp(pl, 0);
        break;
    case PL_ACT_DASH:
        Player_StateDash(pl, 0);
        break;
    case PL_ACT_FLEE_DASH:
        Player_StateFleeDash(pl, 0);
        break;
    case PL_ACT_OUT_OF_BREATH:
        Player_StateOutOfBreath(pl, 0);
        break;
    case PL_ACT_DASH_TIRED:
        Player_StateFleeDash(pl, 1);
        break;
    case PL_ACT_RUN_AFTER_DASH:
        Player_StateMove(pl, 6U);
        break;
    case PL_ACT_TIRED_STOP:
        Player_StateTiredStop(pl, 0);
        break;
    case PL_ACT_LAND_HARD:
        Player_StateHardLanding(pl, 0);
        break;
    case PL_ACT_WALL_HOLD:
        Player_StateWallHold(pl, 0);
        break;
    case PL_ACT_WALL_CLIMB_UP:
        Player_StateWallClimbUp(pl, 0);
        break;
    case PL_ACT_WALL_CLIMB_DOWN:
        Player_StateWallClimbDown(pl, 0);
        break;
    case PL_ACT_WALL_DISMOUNT:
        Player_StateWallDismount(pl, 0);
        break;
    case PL_ACT_WALL_GRAB:
        Player_StateWallGrab(pl, 0);
        break;
    case PL_ACT_RUN_STOP:
        Player_StateRunStop(pl, 0);
        break;
    case PL_ACT_ROLL_FROM_CROUCH:
        Player_StateRoll(pl, 1);
        break;
    case PL_ACT_SET_TRAP:
        Player_StateSetTrap(pl, 0);
        break;
    case PL_ACT_SET_TRAP_DONE:
        Player_StateSetTrapDone(pl, 0);
        break;
    case PL_ACT_WALL_LET_GO:
        Player_StateClimbAirborne(pl, 1);
        break;
    case PL_ACT_EVADE_RIGHT:
        Player_StateSideEvade(pl, 0);
        break;
    case PL_ACT_EVADE_LEFT:
        Player_StateSideEvade(pl, 1);
        break;
    case PL_ACT_BED_LIE_DOWN:
        Player_StateBedLieDown(pl, 0);
        break;
    case PL_ACT_BED_SLEEP:
        Player_StateBedSleep(pl, 0);
        break;
    case PL_ACT_BED_GET_UP:
        Player_StateBedGetUp(pl, 0);
        break;
    case PL_ACT_MOUNTED_GUN_AIM:
        Player_StateMountedGunAim(pl, 0);
        break;
    case PL_ACT_CANNON_FIRE:
        Player_StateCannonFire(pl, 0);
        break;
    case PL_ACT_LEDGE_DROP:
        Player_StateClimbAirborne(pl, 2);
        break;
    case PL_ACT_CROUCH_STAND_UP:
        Player_StateCrouchStandUp(pl, 0);
        break;
    case PL_ACT_LEDGE_PULL_UP:
        Player_StateLedgePullUp(pl, 0);
        break;
    case PL_ACT_CROUCH_WALK:
        Player_StateCrouchWalk(pl, 0);
        break;
    case PL_ACT_CROUCH_STOP:
        Player_StateCrouchStop(pl, 0);
        break;
    case PL_ACT_CROUCH_WALK_FAST:
        Player_StateCrouchWalkFast(pl, 0);
        break;
    case PL_ACT_BATTLE_ALERT:
        Player_StateBattleAlert(pl, 0);
        break;
    case PL_ACT_TURN_AROUND:
        Player_StateTurnAround(pl, 0);
        break;
    case PL_ACT_WALL_PRESS:
        Player_StateWallPress(pl, 0);
        break;
    case PL_ACT_WALL_SIDLE_NEG:
        Player_StateWallSidle(pl, 0);
        break;
    case PL_ACT_WALL_SIDLE_POS:
        Player_StateWallSidle(pl, 1);
        break;
    case PL_ACT_WALL_PRESS_HOLD:
        Player_StateWallPress(pl, 1);
        break;
    case PL_ACT_DIVE:
        Player_StateDive(pl, 0);
        break;
    case PL_ACT_FLEE_DASH_RESUME:
        Player_StateFleeDash(pl, 2);
        break;
    case PL_ACT_DASH_RESUME:
        Player_StateDash(pl, 1);
        break;
    case PL_ACT_GATHER:
        Player_StateGather(pl, 0U);
        break;
    case PL_ACT_MOUNTED_GUN_FIRE:
        Player_StateMountedGunFire(pl, 0);
        break;
    case PL_ACT_WALK_SLOW:
        Player_StateMove(pl, 7U);
        break;
    case PL_ACT_CARVE:
        Player_StateGather(pl, 3U);
        break;
    case PL_ACT_SPIT_ROAST_FAIL:
        Player_StateSpitRoastEnd(pl, 1);
        break;
    case PL_ACT_SPIT_ROAST_START:
        Player_StateSpitRoastStart(pl, 0);
        break;
    case PL_ACT_SPIT_ROAST_SUCCESS:
        Player_StateSpitRoastEnd(pl, 0);
        break;
    case PL_ACT_FISH_READY:
        Player_StateFishWait(pl, 0);
        break;
    case PL_ACT_FISH_CAST:
        Player_StateFishCast(pl, 0);
        break;
    case PL_ACT_FISH_WAIT:
        Player_StateFishWait(pl, 1);
        break;
    case PL_ACT_MINE:
        Player_StateGather(pl, 4U);
        break;
    case PL_ACT_BUG_NET:
        Player_StateGather(pl, 5U);
        break;
    case PL_ACT_FISH_REEL:
        Player_StateFishReel(pl, 0);
        break;
    case PL_ACT_FISH_REEL_EMPTY:
        Player_StateFishReelEmpty(pl, 0);
        break;
    case PL_ACT_SPIT_ROAST:
        Player_StateSpitRoast(pl, 0);
        break;
    case PL_ACT_START_COUNTDOWN:
        Player_StateSpotStrike(pl, 0);
        break;
    case PL_ACT_SUPPLY_BOX:
        Player_StateBoxMenu(pl, 0);
        break;
    case PL_ACT_DRINK:
        Player_StateConsume(pl, 0);
        break;
    case PL_ACT_KNEEL_TO_PLACE_CROUCHED:
        Player_StateKneelToPlaceCrouched(pl, 0);
        break;
    case PL_ACT_EAT:
        Player_StateConsume(pl, 1);
        break;
    case PL_ACT_TRADE:
        Player_StateTrade(pl, 0);
        break;
    case PL_ACT_TAKE_REWARDS:
        Player_StateGather(pl, 2U);
        break;
    case PL_ACT_GATHER_CROUCHED:
        Player_StateGatherCrouched(pl, 0);
        break;
    case PL_ACT_HORN_PLAY:
        Player_StateInstrumentPlay(pl, 0);
        break;
    case PL_ACT_HORN_END:
        Player_StateInstrumentEnd(pl, 0);
        break;
    case PL_ACT_CARVE_CROUCHED:
        Player_StateGatherCrouched(pl, 1);
        break;
    case PL_ACT_SHARPEN:
        Player_StateSharpen(pl, 0);
        break;
    case PL_ACT_TEAM_ITEM_USE:
        Player_StateInstrumentPlay(pl, 1);
        break;
    case PL_ACT_TEAM_ITEM_END:
        Player_StateInstrumentEnd(pl, 1);
        break;
    case PL_ACT_FISH_REEL_BAITED:
        Player_StateFishReelEmpty(pl, 1);
        break;
    case PL_ACT_BINOCULARS:
        Player_StateBinoculars(pl, 0);
        break;
    case PL_ACT_BINOCULARS_CROUCHED:
        Player_StateBinoculars(pl, 1);
        break;
    case PL_ACT_OFFER_ITEM:
        Player_StateOfferItem(pl, 0);
        break;
    case PL_ACT_TAKE_ITEM_WAIT:
        Player_StateTakeItemWait(pl, 0);
        break;
    case PL_ACT_OFFER_ITEM_END:
        Player_StateOfferItemEnd(pl, 0);
        break;
    case PL_ACT_TAKE_ITEM:
        Player_StateTakeItem(pl, 0);
        break;
    case PL_ACT_LANCE_HOP_SIDE_A:
        Player_StateLanceHop(pl, 0);
        break;
    case PL_ACT_LANCE_HOP_SIDE_B:
        Player_StateLanceHop(pl, 1);
        break;
    case PL_ACT_LANCE_HOP_BACK:
        Player_StateLanceHop(pl, 2);
        break;
    case PL_ACT_LANCE_HOP_BACK_LONG:
        Player_StateLanceHop(pl, 3);
        break;
    case PL_ACT_PLACE_LARGE_ITEM:
        Player_StatePlaceLargeItem(pl, 0);
        break;
    case PL_ACT_PLACE_LARGE_ITEM_SET:
        Player_StatePlaceLargeItemSet(pl, 0);
        break;
    case PL_ACT_APPLY_ITEM:
        Player_StateApplyItem(pl, 0);
        break;
    case PL_ACT_RETURN_TO_CAMP:
        Player_StateThrowItem(pl, 2);
        break;
    case PL_ACT_HORN_ITEM_PLAY:
        Player_StateHornItemPlay(pl, 0);
        break;
    case PL_ACT_BOW_WALK:
        Player_StateMove(pl, 9U);
        break;
    case PL_ACT_GATHER_SPECIAL:
        Player_StateGather(pl, 6U);
        break;
    case PL_ACT_GATHER_SPECIAL_CROUCHED:
        Player_StateGatherCrouched(pl, 2);
        break;
    case PL_ACT_WALL_CLIMB_UP_FAST:
        Player_StateWallClimbUp(pl, 1);
        break;
    case PL_ACT_WALL_CLIMB_DOWN_FAST:
        Player_StateWallClimbDown(pl, 1);
        break;
    case PL_ACT_SPOT_ITEM_STRIKE:
        Player_StateSpotStrike(pl, 1);
        break;
    case PL_ACT_MOUNTED_GUN_REAIM:
        Player_StateMountedGunAim(pl, 1);
        break;
    case PL_ACT_DELIVERY_BOX:
        Player_StateBoxMenu(pl, 1);
        break;
    case PL_ACT_GATHER_DROP:
        Player_StateGather(pl, 7U);
        break;
    case PL_ACT_GATHER_DROP_CROUCHED:
        Player_StateGatherCrouched(pl, 3);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * PL_WPN_SHARED_ATTACK, the attack every weapon shares: one motion with
 * PlayerAttack 5, then idle. The second parameter is unused.
 */
void Player_StateSharedAttack(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_SHARED_ATTACK, 4, 0);
        PlayerAttack_Create(pl, 5U);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_WEAPON:
 * PL_WPN_SHARED_ATTACK goes to Player_StateSharedAttack, any other state to
 * the drawn weapon's dispatcher by pl->weaponType (the two bowguns share
 * Bowgun_RunState). Clears pl->unk310 first.
 */
void Player_RunWeaponState(Player *pl)
{
    pl->unk310 = 0;
    if (pl->state == 0x63) {
        Player_StateSharedAttack(pl, 0);
    } else {
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
            GreatSword_RunState(pl);
            return;
        case WEAPON_LONG_SWORD:
            LongSword_RunState(pl);
            return;
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_LIGHT_BOWGUN:
            Bowgun_RunState(pl);
            return;
        case WEAPON_BOW:
            Bow_RunState(pl);
            return;
        case WEAPON_HAMMER:
            Hammer_RunState(pl);
            return;
        case WEAPON_HUNTING_HORN:
            HuntingHorn_RunState(pl);
            return;
        case WEAPON_LANCE:
            Lance_RunState(pl);
            return;
        case WEAPON_GUNLANCE:
            Gunlance_RunState(pl);
            return;
        case WEAPON_SWORD_SHIELD:
            SwordShield_RunState(pl);
            return;
        case WEAPON_DUAL_SWORDS:
            DualSwords_RunState(pl);
            return;
        }
    }
}

/*
 * PL_REACT_KNOCKDOWN_FRONT: falls, gets up and returns to idle, unhittable
 * throughout (noHitTimer).
 */
void Player_StateKnockdownFront(Player *pl)
{
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_KNOCKDOWN_FACE_DOWN, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 6, 0x22);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * A guarded hit slides the player back with the guard sound and a pad shake.
 * variant 0 PL_REACT_GUARD_STAGGER (a short slide), 1
 * PL_REACT_GUARD_KNOCKBACK (a long one, the heavy bowgun with its own
 * motion). The lance, gunlance and heavy bowgun slide less. Ends in
 * PL_REACT_GUARD_HOLD for the lance and gunlance, else in idle.
 */
void Player_StateGuardRecoil(Player *pl, s32 variant)
{
    f32 slideLocal[3];
    f32 slideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_STAGGER, 2, 0);
        } else if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            Player_SetMotionPair(pl, PL_MOT_HBG_GUARD_KNOCKBACK, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_KNOCKBACK, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Sound_PlayPlayerSe(pl, 0x67, 0, pl->pos, 1, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        Vib_StartLocal(pl, 1);
        Player_SetFlag(pl, 2);
        Player_SetFlag(pl, PLF_GUARD);
        pl->weaponOut = 1;
        break;
    case 1:
        slideLocal[2] = 0.0f;
        if (variant == 0) {
            if (Motion_ReachedFrame(pl, 0, 30.0f) == 0) {
                slideLocal[0] = 0.0f;
                slideLocal[1] = 0.0f;
                switch (pl->weaponType) {
                case WEAPON_LANCE:
                case WEAPON_GUNLANCE:
                case WEAPON_HEAVY_BOWGUN:
                    slideLocal[2] = -15.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                case WEAPON_GREAT_SWORD:
                case WEAPON_LONG_SWORD:
                case WEAPON_SWORD_SHIELD:
                    slideLocal[2] = -20.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                }
                Vec_Rotate(slideWorld, slideLocal, pl->rotMtx);
                pl->pos[0] += slideWorld[0];
                pl->pos[2] += slideWorld[2];
            }
        } else {
            slideLocal[0] = 0.0f;
            slideLocal[1] = 0.0f;
            switch (pl->weaponType) {
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
            case WEAPON_HEAVY_BOWGUN:
                if (Motion_ReachedFrame(pl, 0, 90.0f) == 0) {
                    slideLocal[2] = -16.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
                if (Motion_ReachedFrame(pl, 0, 104.0f) == 0) {
                    slideLocal[2] = -22.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case WEAPON_SWORD_SHIELD:
                if (Motion_ReachedFrame(pl, 0, 94.0f) == 0) {
                    slideLocal[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 94.0f)) / 360.0f);
                }
                break;
            }
            Vec_Rotate(slideWorld, slideLocal, pl->rotMtx);
            pl->pos[0] += slideWorld[0];
            pl->pos[2] += slideWorld[2];
        }
        if (pl->motion[0].playing == 0) {
            switch (pl->weaponType) {
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
                break;
            case WEAPON_SWORD_SHIELD:
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
            case WEAPON_HEAVY_BOWGUN:
            default:
                Player_ReturnToIdle(pl, 0, 0xA, 0);
                break;
            }
        }
        break;
    }
}

/*
 * PL_REACT_BLOWN_BACK: flung backward onto the back. Ends the demon mode and
 * launches the player up and back; it lands near pl->floorY (impact sound,
 * armour rattle), lies, gets up and returns to idle. With no health left the
 * local player faints (PL_GROUP_FAINT, PL_FAINT_FACE_UP); another player's
 * copy waits on the ground. Unhittable throughout.
 */
void Player_StateBlownBack(Player *pl)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 y;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, PL_MOT_BLOWN_BACK_LAUNCH, 0, 0x10);
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        kickLocal[0] = 0.0f;
        kickLocal[1] = 0.0f;
        kickLocal[2] = -11.17f;
        Vec_Rotate(kickWorld, kickLocal, yawMtx);
        pl->vel[0] = 2.0f * kickWorld[0];
        pl->vel[2] = 2.0f * kickWorld[2];
        pl->vel[1] = 6.0f;
        Player_StartJump(pl, 0xC);
        pl->vel[1] = 0.0f;
        pl->accel[1] = -1.0f / 3.0f;
        pl->timer = 16;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && pl->motionId[0] == 0xCD) {
            Player_SetMotionPair(pl, PL_MOT_FLUNG_BACK_AIR, 0, 0);
        }
        Player_MoveAccel(pl);
        if (pl->timer > 0) {
            pl->timer--;
            y = 63.0f + pl->floorY;
            if (pl->pos[1] <= y) {
                pl->pos[1] = y;
            }
        } else if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->step++;
            pl->pos[1] = pl->floorY;
            Sound_PlayPlayerSe(pl, 0x42, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_BLOWN_BACK_LAND, 0, 0);
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->hp > 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 2, 0);
            } else if (Player_IsLocal(pl) == 1) {
                Player_RequestStateSynced(pl, PL_GROUP_FAINT, PL_FAINT_FACE_UP, 0);
            } else {
                pl->step = 4;
                Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            pl->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

/*
 * The guard. variant 0 PL_REACT_GUARD_START, 1 PL_REACT_GUARD_HOLD, 2
 * PL_REACT_GUARD_HOLD_ALT (differs for the gunlance only). The motions are
 * the weapon's. For the local player (any player offline), from frame 18
 * letting go of PAD_R1 requests PL_REACT_GUARD_END, and Player_GuardInput
 * starts the guard's actions (not for the heavy bowgun).
 */
void Player_StateGuard(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            default:
            case WEAPON_LONG_SWORD:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD, 3, 0);
                break;
            case WEAPON_SWORD_SHIELD:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_START, 2, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_START, 2, 0);
                break;
            }
        } else {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            default:
            case WEAPON_LONG_SWORD:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD, 6, 0x28);
                break;
            case WEAPON_SWORD_SHIELD:
            case WEAPON_LANCE:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
                break;
            case WEAPON_GUNLANCE:
                if (variant == 1) {
                    Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
                } else {
                    Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_START, 4, 4);
                }
                break;
            case WEAPON_HEAVY_BOWGUN:
                Player_SetMotionPair(pl, PL_MOT_HBG_GUARD, 2, 0);
                break;
            }
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_GUARD);
        pl->weaponOut = 1;
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0) {
            if (pl->motionTime > 18 && !(pl->input.held & PAD_R1)) {
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_END, 0);
                break;
            }
        }
        switch (pl->weaponType) {
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
        case WEAPON_SWORD_SHIELD:
            if (pl->motionId[0] == 0x3ED && pl->motion[0].playing == 0) {
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
            }
            if (pl->motionTime > 18) {
                Player_GuardInput(pl);
            }
            break;
        case WEAPON_GREAT_SWORD:
        case WEAPON_LONG_SWORD:
            if (pl->motionTime > 18) {
                Player_GuardInput(pl);
            }
            break;
        }
        break;
    }
}

/*
 * PL_REACT_ROAR_STRONG: braces against the strongest roar, unhittable until
 * frame 64, with effects until frame 40; then idle.
 */
void Player_StateRoarStrong(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, PL_MOT_ROAR_STRONG_BRACE, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 40.0f) == 0 && pl->motionTime % 7 == 0) {
            PartFx_SpawnForPlayer(pl, 5, 9);
            PartFx_SpawnForPlayer(pl, 8, 9);
        }
        if (Motion_ReachedFrame(pl, 0, 64.0f) == 0) {
            pl->noHitTimer = 2;
        }
        break;
    }
}

/*
 * PL_REACT_BLOWN_FORWARD: flung forward onto the face; as
 * Player_StateBlownBack without the jump, and the local player faints into
 * PL_FAINT_FACE_DOWN.
 */
void Player_StateBlownForward(Player *pl)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 y;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, PL_MOT_BLOWN_FORWARD_LAUNCH, 0, 0);
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        pl->targetYaw = pl->rot[1];
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = (u16) pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        kickLocal[0] = 0.0f;
        kickLocal[1] = 0.0f;
        kickLocal[2] = 11.5f;
        Vec_Rotate(kickWorld, kickLocal, yawMtx);
        pl->vel[0] = 2.0f * kickWorld[0];
        pl->vel[2] = 2.0f * kickWorld[2];
        pl->vel[1] = 0.0f;
        pl->accel[1] = -0.25f;
        pl->timer = 16;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && pl->motionId[0] == 0xD0) {
            Player_SetMotionPair(pl, PL_MOT_FLUNG_FORWARD_AIR, 0, 0);
        }
        Player_MoveAccel(pl);
        if (pl->timer > 0) {
            pl->timer--;
            y = 63.0f + pl->floorY;
            if (pl->pos[1] <= y) {
                pl->pos[1] = y;
            }
        } else if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->step++;
            pl->pos[1] = pl->floorY;
            Sound_PlayPlayerSe(pl, 0x42, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_DOWN, 0, 0);
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->hp > 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
            } else if (Player_IsLocal(pl) == 1) {
                Player_RequestStateSynced(pl, PL_GROUP_FAINT, PL_FAINT_FACE_DOWN, 0);
            } else {
                pl->step = 4;
                Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

/*
 * PL_REACT_KNOCKDOWN_BACK: falls, gets up and returns to idle, unhittable
 * throughout.
 */
void Player_StateKnockdownBack(Player *pl)
{
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_KNOCKDOWN_FACE_UP, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 6, 0x3A);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * Thrown into the air, landing lying down. variant 0 PL_REACT_TOSSED_BACK, 1
 * PL_REACT_TOSSED_FORWARD: the direction and the motions. Ends the demon mode
 * and launches the player; on landing it turns half a turn, gets up and
 * returns to idle. Unhittable throughout.
 */
void Player_StateTossed(Player *pl, s32 variant)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 z;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_TOSSED_BACK_LAUNCH, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_TOSSED_FORWARD_LAUNCH, 0, 0);
        }
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        kickLocal[0] = 0.0f;
        kickLocal[1] = 0.0f;
        if (variant != 0) {
            z = 7.0f;
        } else {
            z = -7.0f;
        }
        kickLocal[2] = z;
        Vec_Rotate(kickWorld, kickLocal, yawMtx);
        pl->vel[0] = 2.0f * kickWorld[0];
        pl->vel[2] = 2.0f * kickWorld[2];
        pl->vel[1] = 20.0f;
        pl->accel[1] = -8.0f / 11.0f;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && (pl->motionId[0] == 0xD5 || pl->motionId[0] == 0xDB)) {
            if (variant == 0) {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_BACK_AIR, 0, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_FORWARD_AIR, 0, 0);
            }
        }
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->step++;
            Sound_PlayPlayerSe(pl, 0x42, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_TOSSED_LAND, 0, 0);
            pl->pos[1] = pl->floorY;
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->rot[1] = (pl->rot[1] + 0x8000) & 0xFFFF;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * PL_REACT_GUARD_END: lowers the guard with the weapon's motion, then idle.
 */
void Player_StateGuardEnd(Player *pl)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
        default:
        case WEAPON_LONG_SWORD:
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_STAGGER, 2, 0);
            break;
        case WEAPON_SWORD_SHIELD:
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_END, 2, 0);
            break;
        case WEAPON_HEAVY_BOWGUN:
            Player_SetMotionPair(pl, PL_MOT_HBG_GUARD_END, 2, 0);
            break;
        }
        Player_SetFlag(pl, PLF_GUARD);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * The light hit. variant 0 PL_REACT_FLINCH_FRONT (pushed back), 1
 * PL_REACT_FLINCH_BACK (pushed forward), relative to pl->targetYaw until
 * frame 16; the motion depends on weaponOut. Then idle.
 */
void Player_StateFlinch(Player *pl, s32 variant)
{
    f32 pushLocal[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 z;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        if (pl->weaponOut != 0) {
            Player_SetMotionPair(pl, PL_MOT_FLINCH_DRAWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_FLINCH, 0, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        ang[0] = 0;
        ang[2] = 0;
        ang[1] = pl->targetYaw;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        pushLocal[0] = 0.0f;
        pushLocal[1] = 0.0f;
        if (variant != 0) {
            z = 4.0f;
        } else {
            z = -4.0f;
        }
        pushLocal[2] = z;
        Vec_Rotate(&pl->vel[0], pushLocal, yawMtx);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 16.0f) == 0) {
            Player_Move(pl);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_REACT_GUARD_PUSH: a light guarded hit: a short guard motion, the guard
 * sound and a pad shake, then PL_REACT_GUARD_HOLD (the heavy bowgun returns
 * to idle).
 */
void Player_StateGuardPush(Player *pl)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            Player_SetMotionPair(pl, PL_MOT_HBG_GUARD, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_PUSH, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Sound_PlayPlayerSe(pl, 0x67, 0, &pl->pos[0], 1, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        Player_SetFlag(pl, 0x8000U);
        Vib_StartLocal(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        } else {
        }
        break;
    }
}

/*
 * The roar that holds the player in place. variant 0 PL_REACT_ROAR, 1
 * PL_REACT_ROAR_SHORT. Pushed back until frame 40 with effects; the short
 * form ends at its first pass of frame 44, the long one at its second. Then
 * idle.
 */
void Player_StateRoar(Player *pl, s32 variant)
{
    f32 slideLocal[3];
    f32 slideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, PL_MOT_ROAR_COVER, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 28.0f) != 0 && variant == 1) {
            pl->step = 3;
            Player_SetMotionPair(pl, PL_MOT_ROAR_RECOVER, 6, 0x14);
        } else if (Motion_ReachedFrame(pl, 0, 40.0f) == 0) {
            slideLocal[0] = 0.0f;
            slideLocal[1] = 0.0f;
            slideLocal[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 42.0f)) / 360.0f);
            Vec_Rotate(slideWorld, slideLocal, pl->rotMtx);
            pl->pos[0] += slideWorld[0];
            pl->pos[2] += slideWorld[2];
            if (pl->motionTime % 7 == 0) {
                PartFx_SpawnForPlayer(pl, 5, 9);
                PartFx_SpawnForPlayer(pl, 8, 9);
            }
        } else {
            pl->step++;
        }
        break;
    case 2:
        if (Motion_ReachedFrame(pl, 0, 44.0f) != 0) {
            pl->stateVar0++;
            if (variant == 1 || (variant == 0 && pl->stateVar0 >= 2)) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_ROAR_RECOVER, 2, 0);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xC, 0);
        }
        break;
    }
}

/*
 * The wind pressure: the player braces in a looping motion. variant 0
 * PL_REACT_WIND (4 loops), 1 PL_REACT_WIND_SHORT (one); stateVar0 counts
 * them. Then an ending motion and idle.
 */
void Player_StateWind(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        if (variant == 0) {
            pl->stateVar1 = 4;
        } else {
            pl->stateVar1 = 1;
        }
        Player_SetMotionPair(pl, PL_MOT_BRACE, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->timer = 0;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar0++;
            if ((s32) pl->stateVar0 >= (s32) pl->stateVar1) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_BRACE_END, 2, 0);
                break;
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 0xC, 0);
        }
        break;
    }
}

/*
 * PL_REACT_FLINCH_CARRYING and PL_REACT_FLINCH_CARRYING2: a flinch in place
 * that keeps the carried item (Player_RunReactState drops it in every other
 * react state). Then idle.
 */
void Player_StateFlinchCarrying(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        if (pl->weaponOut != 0) {
            Player_SetMotionPair(pl, PL_MOT_FLINCH_DRAWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_FLINCH, 0, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Ends the stun build-up (pl->status5Buildup and its delay) and blocks the
 * item button for 30 frames (itemUseDelay).
 */
static void Player_ClearStun(Player *pl)
{
    pl->status5Delay = 0;
    pl->status5Buildup = 0;
    pl->itemUseDelay = 0x1E;
}

/*
 * PL_REACT_STUN: dazed, with the stun effect, in 5 stages (stateVar1 counts
 * down). A stage ends after 300 frames, or sooner once the presses counted in
 * stateVar0 reach gStunMashNeed[stage] (doubled with skill 9). After the last
 * stage it clears the stun and returns to idle.
 */
void Player_StateStun(Player *pl, s32 unused)
{
    s8 need;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        pl->weaponOut = 0;
        pl->noFlinchTimer = 0;
        Player_SetMotionPair(pl, PL_MOT_STAGGER, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->timer = 300;
        pl->stateVar0 = 0;
        pl->stateVar1 = 5;
        pl->status5Delay = 0;
        pl->status5Buildup = 0;
        Player_ClearMash(pl);
        pl->targetYaw = pl->rot[1];
        StunStars_CreatePlayer(pl, 0);
        break;
    case 1:
        if (--pl->timer <= 0) {
            pl->stateVar0 = 0;
            pl->stateVar1--;
            pl->timer = 300;
        } else if ((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) {
            pl->stateVar0++;
            need = gStunMashNeed[pl->stateVar1];
            if (Player_HasSkill(pl, SKILL_STATUS5_DOUBLE) == 1) {
                need *= 2;
            }
            if (pl->stateVar0 >= need) {
                pl->stateVar0 = 0;
                pl->stateVar1--;
                pl->timer = 300;
            }
        }
        if (pl->stateVar1 == 0) {
            Player_ClearStun(pl);
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_REACT_14: one motion with the weapon sheathed, then idle (no entry
 * seen).
 */
void Player_StateReact14(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_REACT_STATUS2: falls, then lies for 240 frames (halved with skill 4,
 * doubled with skill 6) regaining stamina; each press takes a frame off (once
 * per 3 frames). Then gets up and returns to idle. Entry clears the status 2
 * build-up, the stun and the mash status.
 */
void Player_StateStatus2(Player *pl, s32 variant)
{
    f32 effectOffset[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
        pl->status2Buildup = 0;
        Player_ClearStun(pl);
        Player_ClearMash(pl);
        Player_SetMotionPair(pl, PL_MOT_FALL_ASLEEP, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->timer = 240;
            if (Player_HasSkill(pl, SKILL_STATUS2_HALF) == 1) {
                pl->timer /= 2;
            }
            if (Player_HasSkill(pl, SKILL_STATUS2_DOUBLE) == 1) {
                pl->timer *= 2;
            }
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 0x9A);
        }
        break;
    case 2:
        if (pl->stateVar1 > 0) {
            pl->stateVar1--;
        }
        if (((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) && pl->stateVar1 == 0) {
            pl->stateVar1 = 3;
            pl->timer--;
        }
        if (--pl->timer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
            break;
        }
        effectOffset[0] = 0.0f;
        effectOffset[1] = 20.0f;
        effectOffset[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 20, effectOffset, 0.8f);
        }
        Player_AddStamina(pl, 2);
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * The status 4 fall. variant 0 PL_REACT_STATUS4 (falls, unhittable, then
 * lies), 1 PL_REACT_STATUS4_HIT (hit while down: lies at once and keeps
 * recoverTimer). Lies for recoverTimer frames (150, halved with skill 1,
 * doubled with skill 3); each press takes a frame off. Then gets up,
 * unhittable, and returns to idle.
 */
void Player_StateStatus4(Player *pl, s32 variant)
{

    pl->stateVar0++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
        pl->status4Buildup = 0;
        pl->status2Buildup = 0;
        Player_ClearStun(pl);
        Player_ClearMash(pl);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_STATUS4_COLLAPSE, 4, 0);
            pl->motionTime = 0;
            pl->stateVar0 = 0;
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
            pl->recoverTimer = 150;
            if (Player_HasSkill(pl, SKILL_STATUS4_HALF) == 1) {
                pl->recoverTimer /= 2;
            }
            if (Player_HasSkill(pl, SKILL_STATUS4_DOUBLE) == 1) {
                pl->recoverTimer *= 2;
            }
            pl->noHitTimer = 2;
            break;
        }
        pl->step = 2;
        if (pl->recoverTimer <= 0) {
            pl->recoverTimer = 0;
        }
        Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 2, 0x96);
        break;
    case 1:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 2, 0x96);
            pl->noHitTimer = 0;
        }
        if (pl->step < 3 && !(pl->stateVar0 & 0x1F)) {
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
        }
        break;
    case 2:
        if (pl->stateVar1 > 0) {
            pl->stateVar1--;
        }
        if (((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) && pl->stateVar1 == 0) {
            pl->stateVar1 = 3;
            pl->recoverTimer--;
        }
        if (--pl->recoverTimer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
            pl->noHitTimer = 2;
            break;
        }
        if (pl->motionId[0] == 0xD2 && Motion_PassedFrame(pl, 0, 170.0f) != 0 && !(Rand_Next(1) & 0x3F)) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP_STIR, 0, 0);
        } else if (pl->motionId[0] == 0xDF && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 0xAA);
        }
        if (pl->step < 3 && !(pl->stateVar0 & 0x1F)) {
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
        }
        break;
    case 3:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * PL_REACT_GUARD_DRAW (lance, sword and shield, gunlance): draws the weapon
 * straight into the guard, then PL_REACT_GUARD_HOLD.
 */
void Player_StateGuardDraw(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_DRAW, 2, 1);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_GUARD);
        pl->weaponOut = 1;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        }
        break;
    }
}

/*
 * The mash status's input for the local player. caller: 0 from
 * Player_StateMashIdle, 1 from the walk and the run. A press (once per 2
 * frames, mashTimer) takes 10 off pl->mashLeft (doubled with SKILL_MASH_FAST,
 * halved with SKILL_MASH_SLOW); at 0 the mash ends (PL_STATUS_ICON_MASH cleared)
 * with PL_REACT_MASH_FREE. Otherwise the left stick turns the player and
 * requests PL_REACT_MASH_RUN (PAD_R2) or PL_REACT_MASH_WALK; released, it
 * goes back to PL_REACT_MASH_IDLE (for caller 1 once pl->timer is 0).
 */
void Player_UpdateMashEscape(Player *pl, s32 caller)
{
    s16 n;

    if (Player_IsLocal(pl) != 0) {
        if (pl->mashTimer == 0) {
            if ((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) {
                if (pl->mashLeft > 0) {
                    n = 10;
                    if (Player_HasSkill(pl, SKILL_MASH_FAST) == 1) {
                        n *= 2;
                    }
                    if (Player_HasSkill(pl, SKILL_MASH_SLOW) == 1) {
                        n /= 2;
                    }
                    pl->mashLeft -= n;
                    if (pl->mashLeft < 0) {
                        pl->mashLeft = 0;
                    }
                    pl->mashTimer = 1;
                }
            }
        } else {
            pl->mashTimer--;
        }
        if (pl->mashLeft <= 0) {
            Player_ClearMash(pl);
            pl->statusIcons &= 0xFFBF;
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_FREE, 0);
            return;
        }
        if (pl->input.stickTilt >= 40) {
            Player_FaceStick(pl);
            if ((pl->input.held & PAD_R2) && pl->dashDelay == 0) {
                if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_MASH_RUN) == 0) {
                    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_RUN, 0);
                }
            } else if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_MASH_WALK) == 0 && pl->dashDelay == 0) {
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_WALK, 0);
            }
        } else if (Player_TestFlag(pl, 0x2001200) != 0) {
            if ((s16) caller != 1 || pl->timer == 0) {
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_IDLE, 0);
            }
        }
    }
}

/* Ends the mash status: clears pl->mashLeft, mashBuildup and mashTimer. */
static void Player_ClearMash(Player *pl)
{
    pl->mashLeft = 0;
    pl->mashBuildup = 0;
    pl->mashTimer = 0;
}

/*
 * Standing in the mash status. variant 0 PL_REACT_MASH_START (the status
 * begins: clears the status 2 and 4 build-ups, the stun and the flinch
 * timers, mashLeft 900; presses count after 15 frames), 1 PL_REACT_MASH_IDLE
 * (back from walking, running or landing). The local player runs
 * Player_UpdateMashEscape each frame.
 */
void Player_StateMashIdle(Player *pl, s32 variant)
{
    if (pl->motionId[0] != 0x33) {
        Player_SetMotionPair(pl, PL_MOT_CARRY_IDLE, 4, 0);
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        if (variant == 0) {
            pl->status4Buildup = 0;
            pl->status2Buildup = 0;
            Player_ClearStun(pl);
            Player_ClearMash(pl);
            pl->mashLeft = 900;
            pl->noFlinchTimer = 0;
            pl->noFlinchItemTimer = 0;
            pl->timer = 15;
        } else {
            pl->timer = 30;
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
    case 1:
        if (--pl->timer <= 0) {
            pl->step++;
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            Player_UpdateMashEscape(pl, 0);
        }
        break;
    }
}

/*
 * PL_REACT_MASH_FREE: breaks free of the mash status, with the status effect
 * object (MashDebris_Create type 0); then idle.
 */
void Player_StateMashFree(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 6, 0);
        MashDebris_Create(0, (s16 *) pl);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Shaken by a tremor, unable to act. variant 0 PL_REACT_QUAKE (40 frames), 1
 * PL_REACT_QUAKE_LONG (80); then idle.
 */
void Player_StateQuake(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_STAGGER, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        if (variant == 0) {
            pl->timer = 0x28;
            break;
        }
        pl->timer = 0x50;
        break;
    case 1:
        if (--pl->timer <= 0) {
            Player_ReturnToIdle(pl, 0, 0xC, 0);
        }
        break;
    }
}

/*
 * PL_REACT_MASH_WALK: walks in the mash status, running
 * Player_UpdateMashEscape(pl, 1) each frame, with the status effect object
 * every tenth frame.
 */
void Player_StateMashWalk(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_SetMotionPair(pl, PL_MOT_CARRY_WALK, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        strideLocal[0] = 0.0f;
        strideLocal[1] = 0.0f;
        strideLocal[2] = 2.0f;
        Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
        pl->pos[0] += strideWorld[0];
        pl->pos[2] += strideWorld[2];
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateMashEscape(pl, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * PL_REACT_MASH_RUN: runs in the mash status; a drop ahead requests
 * PL_REACT_MASH_JUMP_OFF. Otherwise as Player_StateMashWalk.
 */
void Player_StateMashRun(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->gender == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN, 4, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN_ALT, 4, 0);
        }
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x2000200);
        break;
    case 1:
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) != 0) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_JUMP_OFF, 0);
            return;
        }
        strideLocal[0] = 0.0f;
        strideLocal[1] = 0.0f;
        strideLocal[2] = 5.0f;
        Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
        pl->pos[0] += strideWorld[0];
        pl->pos[2] += strideWorld[2];
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateMashEscape(pl, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * The mash status in the air. variant 0 PL_REACT_MASH_JUMP_OFF (a hop off a
 * ledge), 1 PL_REACT_MASH_FALL. pl->timer counts the time aloft: landing
 * after 50 breaks the mash (Player_ClearMash) with PL_ACT_LAND_HARD, after 30
 * PL_REACT_MASH_LAND_HARD, else PL_REACT_MASH_LAND.
 */
void Player_StateMashAirborne(Player *pl, s32 variant)
{
    f32 velLocal[3];
    f32 moveWorld[3];

    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = PL_POSTURE_AIRBORNE;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 0x13);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 8, 0x20);
            pl->rideState = 0;
            pl->vel[0] = 0.0f;
            pl->vel[2] = 8.0f;
            pl->vel[1] = -10.0f;
            pl->accel[2] = 0.0f;
            pl->accel[0] = 0.0f;
            pl->accel[1] = -8.0f / 11.0f;
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        pl->vel[0] += pl->accel[0];
        pl->vel[1] += pl->accel[1];
        pl->vel[2] += pl->accel[2];
        if (pl->vel[0] < 0.0f) {
            pl->vel[0] = 0.0f;
        }
        if (pl->vel[2] < 0.0f) {
            pl->vel[2] = 0.0f;
        }
        velLocal[0] = pl->vel[0];
        velLocal[1] = pl->vel[1];
        velLocal[2] = pl->vel[2];
        Vec_Rotate(moveWorld, velLocal, pl->rotMtx);
        pl->pos[0] += moveWorld[0];
        pl->pos[1] += moveWorld[1];
        pl->pos[2] += moveWorld[2];
        if (pl->vel[1] < 0.0f) {
            pl->timer++;
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = PL_POSTURE_STANDING;
                pl->rideState = 0;
                if (pl->timer >= 50) {
                    MashDebris_Create(0, (s16 *) pl);
                    Player_ClearMash(pl);
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                } else if (pl->timer >= 30) {
                    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_LAND_HARD, PL_SET_NO_SYNC);
                } else {
                    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_LAND, PL_SET_NO_SYNC);
                }
            }
        }
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * The landings in the mash status: variant 0 PL_REACT_MASH_LAND_HARD, 1
 * PL_REACT_MASH_LAND (the motion). Then PL_REACT_MASH_IDLE.
 */
void Player_StateMashLand(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND_HARD, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_IDLE, PL_SET_NO_SYNC);
        }
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * PL_REACT_INHALED: pulled toward a Yama Tsukami. Looks for a visible
 * EM_KIND_YAMA_TSUKAMI in the player's area and launches the player toward
 * its bone 0x24 (with none, thrown back as PL_REACT_TOSSED_BACK). Lands,
 * turns half a turn, gets up and returns to idle.
 */
void Player_StateInhaled(Player *pl, s32 variant)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    f32 pullPos[3];
    s32 ang[3];
    f32 mtx[16];
    Em *inhaler;
    s16 i;
    s16 found;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, PL_MOT_TOSSED_BACK_LAUNCH, 0, 0);
        pl->noHitTimer = 10;
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        found = 0;
        inhaler = gEms;
        for (i = 0; i < 20; i++, inhaler++) {
            if (inhaler->active && inhaler->visible && (Player_InCurrentArea(inhaler) & 0xFF) && inhaler->type == 0x3A) {
                found = 1;
                Em_BonePos(inhaler, 0x24, pullPos);
                pl->rot[1] = Vec_Heading(pl->pos, pullPos) + 0x4000;
                pl->targetYaw = pl->rot[1];
            }
        }
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, mtx);
        if (found == 0) {
            kickLocal[0] = 0.0f;
            kickLocal[1] = 0.0f;
            kickLocal[2] = -7.0f;
            Vec_Rotate(kickWorld, kickLocal, mtx);
            pl->vel[0] = 2.0f * kickWorld[0];
            pl->vel[2] = 2.0f * kickWorld[2];
            pl->vel[1] = 20.0f;
            pl->accel[1] = -8.0f / 11.0f;
        } else {
            kickWorld[0] = (pullPos[0] - pl->pos[0]) / 50.0f;
            kickWorld[1] = (pullPos[1] - pl->pos[1]) / 50.0f;
            kickWorld[2] = (pullPos[2] - pl->pos[2]) / 50.0f;
            pl->vel[0] = kickWorld[0];
            pl->vel[2] = kickWorld[2];
            if (kickWorld[1] <= 15.0f) {
                kickWorld[1] = 15.0f;
            }
            pl->vel[1] = 2.0f * kickWorld[1];
            pl->accel[1] = -8.0f / 11.0f;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0 && (pl->motionId[0] == 0xD5 || pl->motionId[0] == 0xDB)) {
            if (variant == 0) {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_BACK_AIR, 0, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_FORWARD_AIR, 0, 0);
            }
        }
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->noHitTimer = 2;
            pl->step++;
            Sound_PlayPlayerSe(pl, 0x42, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_TOSSED_LAND, 0, 0);
            pl->pos[1] = pl->floorY;
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->rot[1] = (pl->rot[1] + 0x8000) & 0xFFFF;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_REACT. First it leaves
 * the aim mode, closes the box and trade menus, clears noFlinchTimer and
 * PLF_FISHING, and drops a carried item (ItemProp_CreateAtPlayer) in every
 * state but the carrying flinches. States sharing a handler pass a variant; a
 * state with no case returns to idle.
 */
void Player_RunReactState(Player *pl)
{
    u8 state;
    u8 carry;

    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    pl->noFlinchTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    state = pl->state;
    if ((state != 0x11) && (state != 0x12)) {
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
    }
    switch (pl->state) {
    case PL_REACT_KNOCKDOWN_FRONT:
        Player_StateKnockdownFront(pl);
        break;
    case PL_REACT_GUARD_STAGGER:
        Player_StateGuardRecoil(pl, 0);
        break;
    case PL_REACT_BLOWN_BACK:
        Player_StateBlownBack(pl);
        break;
    case PL_REACT_GUARD_START:
        Player_StateGuard(pl, 0);
        break;
    case PL_REACT_ROAR_STRONG:
        Player_StateRoarStrong(pl, 0);
        break;
    case PL_REACT_BLOWN_FORWARD:
        Player_StateBlownForward(pl);
        break;
    case PL_REACT_KNOCKDOWN_BACK:
        Player_StateKnockdownBack(pl);
        break;
    case PL_REACT_TOSSED_BACK:
        Player_StateTossed(pl, 0);
        break;
    case PL_REACT_GUARD_END:
        Player_StateGuardEnd(pl);
        break;
    case PL_REACT_FLINCH_FRONT:
        Player_StateFlinch(pl, 0);
        break;
    case PL_REACT_GUARD_KNOCKBACK:
        Player_StateGuardRecoil(pl, 1);
        break;
    case PL_REACT_GUARD_PUSH:
        Player_StateGuardPush(pl);
        break;
    case PL_REACT_GUARD_HOLD:
        Player_StateGuard(pl, 1);
        break;
    case PL_REACT_FLINCH_BACK:
        Player_StateFlinch(pl, 1);
        break;
    case PL_REACT_TOSSED_FORWARD:
        Player_StateTossed(pl, 1);
        break;
    case PL_REACT_ROAR:
        Player_StateRoar(pl, 0);
        break;
    case PL_REACT_WIND:
        Player_StateWind(pl, 0);
        break;
    case PL_REACT_FLINCH_CARRYING:
        Player_StateFlinchCarrying(pl, 0);
        break;
    case PL_REACT_FLINCH_CARRYING2:
        Player_StateFlinchCarrying(pl, 1);
        break;
    case PL_REACT_STUN:
        Player_StateStun(pl, 0);
        break;
    case PL_REACT_14:
        Player_StateReact14(pl, 0);
        break;
    case PL_REACT_STATUS2:
        Player_StateStatus2(pl, 0);
        break;
    case PL_REACT_STATUS4:
        Player_StateStatus4(pl, 0);
        break;
    case PL_REACT_GUARD_DRAW:
        Player_StateGuardDraw(pl, 0);
        break;
    case PL_REACT_STATUS4_HIT:
        Player_StateStatus4(pl, 1);
        break;
    case PL_REACT_WIND_SHORT:
        Player_StateWind(pl, 1);
        break;
    case PL_REACT_ROAR_SHORT:
        Player_StateRoar(pl, 1);
        break;
    case PL_REACT_MASH_START:
        Player_StateMashIdle(pl, 0);
        break;
    case PL_REACT_MASH_FREE:
        Player_StateMashFree(pl, 0);
        break;
    case PL_REACT_QUAKE:
        Player_StateQuake(pl, 0);
        break;
    case PL_REACT_MASH_IDLE:
        Player_StateMashIdle(pl, 1);
        break;
    case PL_REACT_MASH_WALK:
        Player_StateMashWalk(pl, 0);
        break;
    case PL_REACT_MASH_RUN:
        Player_StateMashRun(pl, 0);
        break;
    case PL_REACT_MASH_JUMP_OFF:
        Player_StateMashAirborne(pl, 0);
        break;
    case PL_REACT_MASH_LAND_HARD:
        Player_StateMashLand(pl, 0);
        break;
    case PL_REACT_MASH_LAND:
        Player_StateMashLand(pl, 1);
        break;
    case PL_REACT_MASH_FALL:
        Player_StateMashAirborne(pl, 1);
        break;
    case PL_REACT_QUAKE_LONG:
        Player_StateQuake(pl, 1);
        break;
    case PL_REACT_GUARD_HOLD_ALT:
        Player_StateGuard(pl, 2);
        break;
    case PL_REACT_INHALED:
        Player_StateInhaled(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

extern s32 gRewardZeroText;
extern s32 gFaintedText;
extern s32 gRewardZeroText2;
extern s32 gNoMoreReviveText;
/*
 * The faint, PL_GROUP_FAINT. variant 0 PL_FAINT, 1 PL_FAINT_FACE_UP, 2
 * PL_FAINT_FACE_DOWN (the lying motion). Step 0 counts the faint:
 * Quest_PlayerFainted for a hunter (FAINT_FAILED and FAINT_QUEST_OVER end in
 * a wait), Versus_MonsterDown in versus; pl->faints counts up and the notices
 * are posted. Then the player lies: at 600 frames the local player is sent to
 * the camp area (QSync_SendPlayer). In versus it revives after 450 frames
 * instead (PL_EVENT_REVIVE).
 */
void Player_StateFaint(Player *pl, s32 variant)
{
    s8 noPenalty;
    s8 result;
    u8 k;

    pl->noPushTimer = 10;
    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    if (pl->carry & PL_CARRY_HOLD_MASK) {
        pl->carry &= PL_CARRY_KIND_MASK;
        ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 0x8C);
            break;
        case 2:
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_DOWN, 0, 0x8C);
            Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            break;
        }
        Vib_StartLocal(pl, 3);
        Player_UpdateMotionTime(pl, 0);
        Player_PlaySeAtSelf((u8 *) pl, 0x22);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, PLF_GUARD);
        pl->weaponOut = 0;
        switch (*(u8 *) &pl->stateRand) {
        default:
            noPenalty = 0;
            break;
        case 1:
            noPenalty = 1;
            break;
        }
        if (pl->side == 0) {
            result = Quest_PlayerFainted(pl, noPenalty);
            if (result != 0 && result != 3) {
                pl->step++;
            }
        } else {
            result = 2;
            Versus_MonsterDown((Em *) pl);
        }
        if (pl->faints <= 98) {
            pl->faints++;
        } else {
            pl->faints = 99;
        }
        if (Player_IsLocal(pl) == 0) {
            Notice_Push(NOTICE_KIND_PLAYER, 0, pl->index);
            if (result <= 1) {
                Quest_ShowValue();
            }
            if (gQuest.reward <= 0 && result == 1) {
                Notice_PushItem((char *)gRewardZeroText);
            }
        } else {
            CamReq_PlayerDown();
            Notice_PushItem((char *)gFaintedText);
            if (result <= 1) {
                Quest_ShowValue();
            }
            if (pl->step <= 1) {
                Quest_ResetMusicIfIdle();
                Bgm_PlayFaintMusic();
            } else {
                if (result == 1) {
                    Notice_PushItem((char *)gRewardZeroText2);
                }
                Notice_PushItem((char *)gNoMoreReviveText);
            }
        }
        break;
    case 1:
        if (pl->side != 0) {
            pl->step = 3;
            pl->timer = 0;
        }
        if (pl->motionId[0] == 0xD2 && Motion_PassedFrame(pl, 0, 164.0f) != 0 && (Rand_Next(1) & 0x3F) == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP_STIR, 0, 0);
        } else if (pl->motionId[0] == 0xDF && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 0xA4);
        }
        if (Quest_PhaseIsOver(2) == 1) {
            pl->step++;
            break;
        }
        if (pl->motionTime == 300 && Player_IsLocal(pl) == 1) {
            switch (gScene.area) {
            case AREA_TOWER_0x7C:
            case AREA_TOWER_4:
            case AREA_TOWER_0x8A:
                Notice_Push(NOTICE_KIND_MESSAGE, 0x1F, 0);
                break;
            default:
                Notice_Push(NOTICE_KIND_MESSAGE, 7, 0);
                break;
            }
        }
        if (pl->motionTime == 600 && Player_IsLocal(pl) == 1) {
            switch (gScene.area) {
            case AREA_TOWER_0x7C:
                k = 0x54;
                pl->areaMoveState = PL_AREA_MOVE_RESPAWN;
                break;
            case AREA_TOWER_4:
                k = 0x7D;
                pl->areaMoveState = PL_AREA_MOVE_RESPAWN;
                break;
            case AREA_TOWER_0x8A:
                k = 0x81;
                pl->areaMoveState = PL_AREA_MOVE_RESPAWN;
                break;
            default:
                k = gScene.campArea;
                pl->areaMoveState = PL_AREA_MOVE_CART;
                break;
            }
            pl->startPos[0] = gStartPos[k][0];
            pl->startPos[1] = gStartPos[k][1];
            pl->startPos[2] = gStartPos[k][2];
            pl->startAngle = gStartAngle[k];
            pl->nextArea = k;
            QSync_SendPlayer(pl, QSYNC_PL_FAINT_TO_CAMP, 0);
        }
        break;
    case 2:
        break;
    case 3:
        if (pl->motionId[0] == 0xD2 && Motion_PassedFrame(pl, 0, 164.0f) != 0 && (Rand_Next(1) & 0x3F) == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP_STIR, 0, 0);
        } else if (pl->motionId[0] == 0xDF && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 0xA4);
        }
        if (Player_IsLocal(pl) == 1) {
            if (++pl->timer >= 450) {
                CamReq_Cancel();
                pl->hpMax = (s16)Player_GetHpSkillBonus(pl) + 100;
                if (pl->hpMax > 150) {
                    pl->hpMax = 150;
                }
                pl->hp = pl->hpMax;
                pl->hpRecoverable = pl->hpMax;
                pl->hpMaxBase = pl->hpMax;
                pl->staminaMax = 300;
                pl->staminaTimer = 0x2A30;
                pl->stamina = pl->staminaMax;
                Player_ClearBuffs(pl);
                pl->weaponSeMute = 0;
                Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_REVIVE, PL_SET_NO_SYNC);
            }
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_FAINT: the three faints
 * go to Player_StateFaint (variants 0, 1, 2) after closing the box and trade
 * menus.
 */
void Player_RunFaintState(Player *pl)
{
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    switch (pl->state) {
    case PL_FAINT:
        Player_StateFaint(pl, 0);
        break;
    case PL_FAINT_FACE_UP:
        Player_StateFaint(pl, 1);
        break;
    case PL_FAINT_FACE_DOWN:
        Player_StateFaint(pl, 2);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * PL_EVENT_CART: the cart carries the fainted player away. Step 0 makes the
 * cart, an EM_KIND_FELYNE_CART monster at the player (pl->cart; with none the
 * state returns to idle); then the player follows the cart's bone 0x1D and
 * requests PL_EVENT_CART_DROP when it drops it. The local camera takes mode
 * 1.
 */
void Player_StateCart(Player *pl, s32 variant)
{
    Em *cartEm;
    s16 i;
    s16 kind;

    pl->noPushTimer = 10;
    pl->noHitTimer = 10;
    pl->visible = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        kind = -1;
        pl->stateVar0 = 0;
        for (i = 0; i < 4; i++) {
            if (gScene.emKind[i] == EM_KIND_FELYNE_CART && (cartEm = EmSlot_Alloc()) != NULL) {
                cartEm->modelSlot = i;
                cartEm->type = gScene.emKind[i];
                cartEm->area = gScene.area;
                cartEm->variant = 0;
                cartEm->variant = 0xFF;
                cartEm->cartPlayer = pl->index;
                GmEm_Task(cartEm);
                cartEm->posX = pl->pos[0];
                cartEm->posY = pl->pos[1];
                cartEm->posZ = pl->pos[2];
                cartEm->rotY = pl->rot[1];
                pl->cart = cartEm;
                kind = gScene.emKind[i];
                break;
            }
        }
        if (kind != EM_KIND_FELYNE_CART) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else {
            pl->stateVar1 = 0;
            Player_SetMotionPair(pl, PL_MOT_CART_RIDE, 0, 0);
        }
        if (Player_IsLocal(pl) == 1) {
            CamReq_Cart();
        }
        break;
    case 1:
        cartEm = pl->cart;
        if (cartEm == NULL) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        Em_BonePos(cartEm, 0x1D, &pl->pos[0]);
        if (cartEm->motionId[0] == 0x3EB) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_CART_DROP, PL_SET_NO_SYNC);
        }
        pl->rot[1] = cartEm->rotY;
        pl->targetYaw = pl->rot[1];
        break;
    }
}

/*
 * PL_EVENT_CART_DROP: the cart drops the player at the camp; the player sinks
 * to the floor, lies, gets up and returns to idle.
 */
void Player_StateCartDrop(Player *pl, s32 unused)
{
    f32 floorY;

    pl->noPushTimer = 0xA;
    pl->noHitTimer = 0xA;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_CART_DROPPED, 0, 0);
        pl->action = PL_POSTURE_AIRBORNE;
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 28.0f) != 0) {
            pl->pos[1] -= 4.0f;
        }
        floorY = pl->floorY;
        if (pl->pos[1] <= floorY) {
            pl->pos[1] = floorY;
        }
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_DOWN, 0, 0x46);
            pl->action = PL_POSTURE_STANDING;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_EVENT_QUEST_START: the pose at the quest's start (sets questStartDone), then
 * idle.
 */
void Player_StateQuestStart(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->questStartDone = 1;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_EVENT_QUEST_END: the quest is cleared or failed: sheathes, plays one
 * motion, drops a carried item and sets the local camera's mode 0xA; then
 * waits until the scene changes.
 */
void Player_StateQuestEnd(Player *pl, s32 unused)
{
    u8 carry;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SIT, 6, 0);
        pl->motionTime = 0;
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        if (Player_IsLocal(pl) == 1) {
            CamReq_PlayerDown();
        }
        
    case 1:
        break;
    }
}

/*
 * PL_EVENT_REVIVE: the versus revive: gets up, then 150 hit-free frames and
 * idle.
 */
void Player_StateRevive(Player *pl, s32 unused)
{
    pl->noHitTimer = 4;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        if (pl->motionId[0] == 0xD1) {
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->noHitTimer = 0x96;
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_EVENT_LEAVE: collapses and vanishes: after 300 frames pl->active and
 * pl->visible are cleared.
 */
void Player_StateLeave(Player *pl, s32 unused)
{
    u8 carry;

    pl->noHitTimer = 0xA;
    pl->noPushTimer = 0xA;
    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    pl->pos[1] = pl->floorY;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_PlaySeAtSelf((u8 *) pl, 0x22);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, PLF_GUARD);
        pl->weaponOut = 0;
        pl->timer = 0x12C;
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        if (Game_IsOnline() != 1) {
            return;
        }
        return;
    case 1:
        if (--pl->timer <= 0) {
            pl->step++;
            return;
        }
        break;
    case 2:
        pl->active = 0;
        pl->visible = 0;
        break;
    }
}

/*
 * PL_EVENT_ABORT: the quest is abandoned: two motions, then after 300 frames
 * pl->active and pl->visible are cleared.
 */
void Player_StateQuestAbort(Player *pl, s32 unused)
{
    u8 carry;

    pl->noHitTimer = 0xA;
    pl->noPushTimer = 0xA;
    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    pl->pos[1] = pl->floorY;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_START, 0, 0);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, PLF_GUARD);
        pl->weaponOut = 0;
        pl->timer = 0x12C;
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 0x90);
        }
        break;
    case 2:
        if (--pl->timer <= 0) {
            pl->step++;
        }
        break;
    case 3:
        pl->active = 0;
        pl->visible = 0;
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_EVENT (the cart, the
 * start, the end, the revive, leaving and abandoning), after closing the box
 * and trade menus.
 */
void Player_RunEventState(Player *pl)
{
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->tradeMsgTimer = 0;
    pl->deliveryOpen = 0;
    switch (pl->state) {
    case PL_EVENT_CART:
        Player_StateCart(pl, 0);
        break;
    case PL_EVENT_CART_DROP:
        Player_StateCartDrop(pl, 0);
        break;
    case PL_EVENT_QUEST_START:
        Player_StateQuestStart(pl, 0);
        break;
    case PL_EVENT_QUEST_END:
        Player_StateQuestEnd(pl, 0);
        break;
    case PL_EVENT_REVIVE:
        Player_StateRevive(pl, 0);
        break;
    case PL_EVENT_LEAVE:
        Player_StateLeave(pl, 0);
        break;
    case PL_EVENT_ABORT:
        Player_StateQuestAbort(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * Keeps pl->carry current while carrying: for the local player (not while
 * delivering or handing over) its high nibble is the pouch's carried item
 * (Item_CarryKind of Pouch_FindCarryItem); the low nibble is set to
 * PL_CARRY_HOLD, the mark the react and event states test when they drop the
 * item.
 */
void Player_UpdateCarryKind(Player *pl)
{
    u16 slot;

    if (Player_IsLocal(pl) == 1 && Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_DELIVER) == 0 && Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_HAND_OVER) == 0) {
        slot = Pouch_FindCarryItem(pl);
        if (slot != 0xFFFF) {
            pl->carry = ((Item_CarryKind(slot) & 0xFF) - 1) * 0x10;
        } else {
            pl->carry = 0;
        }
    }
    pl->carry = (pl->carry & PL_CARRY_KIND_MASK) | PL_CARRY_HOLD;
}

/*
 * The carry states' input for the local player. caller: 0 from
 * Player_StateCarryIdle, 1 from the walk and the run (they stop only once
 * pl->timer is 0). The quest phases 5 to 8 request the end, abort and leave
 * events; no carried item left requests PL_CARRY_DROP. The stick walks or
 * runs (PAD_R2); PAD_CROSS rolls or crouches, a flick draws (PL_ACT_DRAW),
 * PAD_SQUARE drops the item, and PAD_CIRCLE delivers it at the delivery box
 * (an action spot of kind 7, PL_CARRY_DELIVER), fires the cannon with it at
 * kind 8 (PL_ACT_CANNON_FIRE), or hands it to a trader (EM_KIND_TRADER,
 * PL_CARRY_HAND_OVER).
 */
void Player_UpdateCarryInput(Player *pl, s32 caller)
{
    f32 buf[3];
    ItemStack wanted;
    u16 ang;
    u8 k;
    s16 i;

    if (Player_IsLocal(pl) != 0) {
        switch (gScene.questPhase) {
        case 6:
        case 5:
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_END, 0);
            break;
        case 7:
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_ABORT, 0);
            break;
        case 8:
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 0);
            break;
        default:
            if ((u16) Pouch_FindCarryItem(pl) == 0xFFFF) {
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DROP, 0);
                break;
            }
            if (pl->input.stickTilt >= 40) {
                Player_FaceStick(pl);
                if ((pl->input.held & PAD_R2) && pl->dashDelay == 0) {
                    if (Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_RUN) == 0) {
                        Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_RUN, 0);
                    }
                } else if (Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_WALK) == 0 && pl->dashDelay == 0) {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_WALK, 0);
                }
            } else if (Player_TestFlag(pl, 0x2001200) != 0) {
                if ((s16) caller != 1 || pl->timer == 0) {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_IDLE, 0);
                }
            }
            if (Quest_PhaseIsOver(1) == 1) {
                break;
            }
            if (pl->input.pressed & PAD_CROSS) {
                if (pl->input.stickTilt >= 85) {
                    if (pl->stamina >= 75) {
                        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                    }
                } else {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, 0);
                }
                return;
            }
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
                break;
            }
            if ((pl->input.pressed & PAD_SQUARE) && pl->itemSelect == 0) {
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DROP, 0);
            }
            Player_HoldSpecialInput(pl, 2);
            if (pl->input.pressed & PAD_CIRCLE) {
                switch (Player_FindActionSpot((Player *)pl, (f32 *)buf, &ang, (u8 *)&k)) {
                case 7:
                    if (Quest_PhaseIsOver(0) == 1) {
                        break;
                    }
                    for (i = 0; i < 3; i++) {
                        if (Quest_GetDeliveryTarget(pl, i, &wanted) == 2 && wanted.item == (u16) Pouch_FindCarryItem(pl)) {
                            Quest_DeliverItem(pl, wanted.item, 1);
                        }
                    }
                    if ((u16) Pouch_FindCarryItem(pl) == 0xFFFF) {
                        Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DELIVER, 0);
                    }
                    break;
                case 8:
                    if ((u16) Pouch_FindCarryItem(pl) == 0x29C) {
                        pl->carry &= PL_CARRY_KIND_MASK;
                        Pouch_Add(pl, 0x29C, -10);
                        pl->targetYaw = ang;
                        pl->stateRand = k;
                        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CANNON_FIRE, PL_SET_KEEP_RAND);
                    }
                    break;
                default:
                    if (Player_NearTrader(pl) & 0xFF) {
                        Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_HAND_OVER, 0);
                        return;
                    }
                    break;
                }
            }
            break;
        }
    }
}

/*
 * Standing with the item. variant 0 PL_CARRY_IDLE, 1 PL_CARRY_PICKED_UP
 * (input held off 30 frames). Runs Player_UpdateCarryKind and
 * Player_UpdateCarryInput(pl, 0).
 */
void Player_StateCarryIdle(Player *pl, s32 variant)
{
    s32 timer;

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 0x1E;
        pl->dashDelay = 0;
        
    case 1:
        if (pl->motionId[0] != 0x33) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_IDLE, 4, 0);
        }
        if (variant == 1) {
            timer = pl->timer;
            if (timer <= 0) {
                Player_UpdateCarryInput(pl, 0);
                break;
            }
            pl->timer = timer - 1;
            break;
        }
        Player_UpdateCarryInput(pl, 0);
        break;
    }
}

/*
 * PL_CARRY_WALK: walks with the item; Player_HasCarryBonus (SKILL_CARRY_BONUS or carry kind
 * 6) adds a stride. Runs Player_UpdateCarryInput(pl, 1).
 */
void Player_StateCarryWalk(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_SetMotionPair(pl, PL_MOT_CARRY_WALK, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        if (Player_HasCarryBonus(pl) != 0) {
            strideLocal[0] = 0.0f;
            strideLocal[1] = 0.0f;
            strideLocal[2] = 2.0f;
            Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
            pl->pos[0] += strideWorld[0];
            pl->pos[2] += strideWorld[2];
        }
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateCarryInput(pl, 1);
        break;
    }
}

/*
 * PL_CARRY_RUN: runs with the item, using stamina (none left:
 * PL_CARRY_DROP_TIRED); a drop ahead requests PL_CARRY_JUMP_OFF. Runs
 * Player_UpdateCarryInput(pl, 1).
 */
void Player_StateCarryRun(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->gender == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN, 4, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN_ALT, 4, 0);
        }
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x2000200);
        break;
    case 1:
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) != 0) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_JUMP_OFF, 0);
            break;
        }
        Player_UseStamina(pl, -1);
        if (pl->stamina <= 0) {
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DROP_TIRED, 0);
            break;
        }
        if (Player_HasCarryBonus(pl) != 0) {
            strideLocal[0] = 0.0f;
            strideLocal[1] = 0.0f;
            strideLocal[2] = 5.0f;
            Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
            pl->pos[0] += strideWorld[0];
            pl->pos[2] += strideWorld[2];
        }
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateCarryInput(pl, 1);
        break;
    }
}

/*
 * Puts the item down. variant 0 PL_CARRY_DROP (the dropped item is made and
 * the local player's carried item leaves the pouch, with a notice), 1
 * PL_CARRY_DROP_TIRED (dropped but kept in the pouch), 2 PL_CARRY_DELIVER
 * (handed in at the delivery box: nothing is dropped). Then idle.
 */
void Player_StateCarryPutDown(Player *pl, s32 variant)
{
    u16 item;

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 1) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_DROP_TIRED, 4, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_PUT_DOWN, 4, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, variant == 1 ? 4 : 0x72) != 0) {
            pl->step++;
            pl->carry &= PL_CARRY_KIND_MASK;
            if (variant != 2) {
                ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
            }
            if (variant == 0 && Player_IsLocal(pl) == 1) {
                item = Pouch_FindCarryItem(pl);
                if (item != 0xFFFF) {
                    Notice_Push(NOTICE_KIND_ITEM, 0xC, item);
                    Pouch_Add(pl, item, -100);
                }
            }
            break;
        }
        Player_UpdateCarryKind(pl);
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * The carrier in the air. variant 0 PL_CARRY_JUMP_OFF, 1 PL_CARRY_FALL; as
 * Player_StateMashAirborne. Landing after 60 frames (50 without
 * Player_HasCarryBonus) drops the item with PL_ACT_LAND_HARD, after 40 (30)
 * PL_CARRY_LAND_HARD, else PL_CARRY_LAND.
 */
void Player_StateCarryAirborne(Player *pl, s32 variant)
{
    f32 velLocal[3];
    f32 moveWorld[3];
    s16 dropTime;
    s16 hardTime;

    Player_UpdateCarryKind(pl);
    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = PL_POSTURE_AIRBORNE;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 0x13);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 8, 0x20);
            pl->rideState = 0;
            pl->vel[0] = 0.0f;
            pl->vel[2] = 8.0f;
            pl->vel[1] = -10.0f;
            pl->accel[2] = 0.0f;
            pl->accel[0] = 0.0f;
            pl->accel[1] = -8.0f / 11.0f;
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        pl->vel[0] += pl->accel[0];
        pl->vel[1] += pl->accel[1];
        pl->vel[2] += pl->accel[2];
        if (pl->vel[0] < 0.0f) {
            pl->vel[0] = 0.0f;
        }
        if (pl->vel[2] < 0.0f) {
            pl->vel[2] = 0.0f;
        }
        velLocal[0] = pl->vel[0];
        velLocal[1] = pl->vel[1];
        velLocal[2] = pl->vel[2];
        Vec_Rotate(moveWorld, velLocal, pl->rotMtx);
        pl->pos[0] += moveWorld[0];
        pl->pos[1] += moveWorld[1];
        pl->pos[2] += moveWorld[2];
        if (pl->vel[1] < 0.0f) {
            pl->timer++;
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = PL_POSTURE_STANDING;
                pl->rideState = 0;
                if (Player_HasCarryBonus(pl) == 1) {
                    dropTime = 60;
                    hardTime = 40;
                } else {
                    dropTime = 50;
                    hardTime = 30;
                }
                if (pl->timer >= dropTime) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                } else if (pl->timer >= hardTime) {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_LAND_HARD, PL_SET_NO_SYNC);
                } else {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_LAND, PL_SET_NO_SYNC);
                }
            }
        }
        break;
    }
}

/*
 * The landings with the item: variant 0 PL_CARRY_LAND_HARD, 1 PL_CARRY_LAND
 * (the motion). Then PL_CARRY_IDLE.
 */
void Player_StateCarryLand(Player *pl, s32 variant)
{
    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND_HARD, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_IDLE, PL_SET_NO_SYNC);
        }
        break;
    }
}

/*
 * PL_CARRY_HAND_OVER: gives the carried item to the trader (EM_KIND_TRADER):
 * waits for it to leave the pouch, puts it down, waits for tradeMsgTimer and
 * returns to idle.
 */
void Player_StateCarryHandOver(Player *pl, s32 unused)
{
    pl->noHitTimer = 3;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateCarryKind(pl);
        if (pl->motionId[0] != 0x33) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_IDLE, 4, 0);
        }
        pl->tradeMsgTimer = 0x3C;
        pl->timer = 0x3C;
        break;
    case 1:
        Player_UpdateCarryKind(pl);
        if ((Pouch_FindCarryItem(pl) & 0xFFFF) == 0xFFFF) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CARRY_PUT_DOWN, 4, 0);
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 114.0f) != 0) {
            pl->step++;
            pl->carry &= PL_CARRY_KIND_MASK;
            break;
        }
        Player_UpdateCarryKind(pl);
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_StartIdleMotion(pl, 8, 0);
        }
        break;
    case 4:
        if (pl->tradeMsgTimer == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_CARRY, the weapon
 * sheathed. Shared handlers get a variant: Player_StateCarryPutDown (drop,
 * tired drop, deliver), Player_StateCarryAirborne, Player_StateCarryLand,
 * Player_StateCarryIdle (also PL_CARRY_PICKED_UP). Other states return to
 * idle.
 */
void Player_RunCarryState(Player *pl)
{
    pl->weaponOut = 0;
    switch (pl->state) {
    case PL_CARRY_IDLE:
        Player_StateCarryIdle(pl, 0);
        break;
    case PL_CARRY_WALK:
        Player_StateCarryWalk(pl, 0);
        break;
    case PL_CARRY_RUN:
        Player_StateCarryRun(pl, 0);
        break;
    case PL_CARRY_DROP:
        Player_StateCarryPutDown(pl, 0);
        break;
    case PL_CARRY_DROP_TIRED:
        Player_StateCarryPutDown(pl, 1);
        break;
    case PL_CARRY_JUMP_OFF:
        Player_StateCarryAirborne(pl, 0);
        break;
    case PL_CARRY_FALL:
        Player_StateCarryAirborne(pl, 1);
        break;
    case PL_CARRY_LAND_HARD:
        Player_StateCarryLand(pl, 0);
        break;
    case PL_CARRY_LAND:
        Player_StateCarryLand(pl, 1);
        break;
    case PL_CARRY_PICKED_UP:
        Player_StateCarryIdle(pl, 1);
        break;
    case PL_CARRY_DELIVER:
        Player_StateCarryPutDown(pl, 2);
        break;
    case PL_CARRY_HAND_OVER:
        Player_StateCarryHandOver(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * PL_GESTURE_LIE_DOWN: lies down; the local player gets up
 * (PL_GESTURE_GET_UP) with the left stick or at the quest's end, or starts a
 * requested gesture (Player_StartGesture).
 */
void Player_StateGestureLieDown(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if (((s32) pl->input.stickTilt >= PL_STICK_HELD) || (Quest_PhaseIsOver(1) == 1)) {
                Player_RequestState(pl, PL_GROUP_GESTURE, PL_GESTURE_GET_UP, 0);
                break;
            }
            if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        } else {
        }
        break;
    }
}

/* PL_GESTURE_GET_UP: gets up, then idle. */
void Player_StateGestureGetUp(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_SCOUT: the scouting gesture, held at least 60 frames, then left
 * as the other gestures are. When the area's flyer (Area_HasFlyer,
 * Area_GetFlyerPos) is within about 33 degrees of the facing, it sets pl->scoutSpotted
 * and actDetectTimer (300) once.
 */
void Player_StateGestureScout(Player *pl, s32 variant)
{
    f32 pos[3];
    s32 angleOff;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SCOUT, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            } else if (Area_HasFlyer() == 1 && pl->scoutSpotted == 0) {
                Area_GetFlyerPos(pos, pl->area);
                angleOff = (u16) (pl->rot[1] - (u16) (Vec_Heading(pos, pl->pos) + 0x4000));
                if (angleOff <= 0x1800 || angleOff >= 0xE800) {
                    pl->scoutSpotted = 1;
                    pl->actDetectTimer = 300;
                }
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_03: a looped gesture; from frame 90 the local player moves to
 * PL_GESTURE_03_END with the left stick or at the quest's end, or starts a
 * requested gesture.
 */
void Player_StateGesture03(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SIT, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 && Motion_ReachedFrame(pl, 0, 90.0f) != 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_RequestState(pl, PL_GROUP_GESTURE, PL_GESTURE_03_END, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_03_END: the end of PL_GESTURE_03, then idle. */
void Player_StateGesture03End(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SIT_END, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, -4, 0x3C);
        }
        break;
    case 2:
        Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
        if (pl->motion[0].playing == 0) {
            Player_ClearFlag(pl, PLF_NO_ROOT_MOVE);
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_05: a gesture that makes PlayerAttack 0x60 and 0x61 at set
 * frames; held at least 60 frames, then left as Player_StateGesture06.
 */
void Player_StateGesture05(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x274, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Motion_PassedFrame(pl, 0, 48.0f) != 0) {
            PlayerAttack_Create(pl, 0x60);
        }
        if (Motion_PassedFrame(pl, 0, 68.0f) != 0) {
            PlayerAttack_Create(pl, 0x61);
        }
        if (Motion_PassedFrame(pl, 0, 168.0f) != 0) {
            PlayerAttack_Create(pl, 0x61);
        }
        if (Motion_PassedFrame(pl, 0, 190.0f) != 0) {
            PlayerAttack_Create(pl, 0x60);
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 0xA, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_06 and PL_GESTURE_06_ALT: a gesture held at least 60 frames;
 * then the local player returns to idle with the left stick or at the quest's
 * end, or starts a requested gesture (Player_StartGesture).
 */
void Player_StateGesture06(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x278, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 0xA, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_07: a gesture; as Player_StateGesture06. */
void Player_StateGesture07(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x26E, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 0x4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_08: a gesture; as Player_StateGesture06. */
void Player_StateGesture08(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x273, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * The one-shot gestures: one motion, then idle. variant: the row of
 * gGestureOneShots (0 PL_GESTURE_ONESHOT0, 1 PL_GESTURE_ONESHOT1, 3
 * PL_GESTURE_ONESHOT3).
 */
void Player_StateGestureOneShot(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, gGestureOneShots[variant * 2], 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, gGestureOneShots[variant * 2 + 1], 0);
        }
        break;
    }
}

/* PL_GESTURE_0A: a gesture; as Player_StateGesture06. */
void Player_StateGesture0A(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x271, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 0x4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_0B: a gesture; as Player_StateGesture06. */
void Player_StateGesture0B(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x276, 0x10, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 0xA, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_0C: a gesture; as Player_StateGesture06. */
void Player_StateGesture0C(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x272, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_10: a gesture; as Player_StateGesture06. */
void Player_StateGesture10(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x279, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* The second part's data. */
SpitMeat gSpitMeats[13] = {
#include "game/008C3C90.inc"
};
s8 gStunMashNeed[8] = {
#include "game/008C3DC8.inc"
};
s16 gGestureOneShots[8] = {
#include "game/008C3DD0.inc"
};

/* ---- The third part: from Player_RunGestureState on, at optimization level 4 ---- */
/* Declarations of the functions called below. */
void Player_UpdateInput(void);
void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmPlesioth_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_SpawnPartEffects(Em *em, u8 mask);
s32 Em_NearLowHp(Em *em);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_CheckKindRequest(Em *em);
void Em_AddNeed2(Em *em, s32 n);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed3(Em *em, s32 n);
void Vec_Copy(f32 *out, f32 *in);
void Em_TypeSetup(Em *em);
struct TriHdr;
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, struct TriHdr *info, f32 *out, f32 *depth);
void Player_TickTimers(Player *p);
void Player_CalcAttack(Player *p);
void Player_CalcDefense(Player *p);
void Player_CalcResists(Player *p);
void Player_UpdateEnvironment(Player *p);
void Player_UpdateMoveTo(Player *p);
void Player_UpdateStatusIcons(Player *p);
void PlayerFx_TickStatus6(Player *p);
void PlayerFx_TickStatus7(Player *p);
void Player_UpdateHitStop(void *obj);
void Player_UpdateMotionStep(Player *p);
void Player_UpdateHitTimers(Player *p);
void Em_CollideField(struct Em *p, u8 keep);
s32 Em_GetFloorInfo(struct Em *o, f32 *pos, struct TriHdr *a, f32 *out, s32 *b);
struct ObjHead;
void Obj_UpdateFloorEffects(struct ObjHead *obj);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
/* Level 4 from here to the end of the unit (Player_Update, Player_CheckTeamCombo
 * need it; the parts before need level 3). */
#pragma optimization_level 4

/* prototypes */
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Player_Update(Player *p);
void Player_UpdateAutoTurn(Player *p);
void Player_FindLookTarget(Player *em);
void Player_CheckPointMessages(u8 *arg0);
void Player_CheckTeamComboLost(Player *em);
u8 AreaHit_Raycast(f32 *a, f32 *b, f32 *out, u16 mask);
void *EmScript_RoutePointScript(Em *em, u8 i);
s16 *Em_NextAreas(Em *em);
void EmScript_AdvanceStateRand(Em *em);
void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c);
u8 *EmScript_PickTrigger(Em *em);
u8 *EmScript_GetPos(Em *em);
u8 *EmScript_Call(Em *em, u8 *p);
u8 *EmScript_IfSensed(Em *em, u8 *p);
u8 *EmScript_IfNoticed(Em *em, u8 *p);
u8 *EmScript_IfCanEnterNextArea(Em *em, u8 *p);
u8 *EmScript_SetGoal(Em *em, u8 *p);
u8 *EmScript_IfOnGround(Em *em, u8 *p);
u8 *EmScript_IfInAir(Em *em, u8 *p);
u8 *EmScript_SetHitReactMode(Em *em, u8 *p);
u8 *EmScript_IfCombatIs(Em *em, u8 *p);
u8 *EmScript_SetVar(Em *em, u8 *p);
u8 *EmScript_ClearVar(Em *em, u8 *p);
u8 *EmScript_IfInArea(Em *em, u8 *p);
u8 *EmScript_StartLoop(Em *em, u8 *p);
u8 *EmScript_NextLoopPoint(Em *em, u8 *p);
u8 *EmScript_GoalAlertedPlayer(Em *em, u8 *p);
u8 *EmScript_GoalNoticedPlayer(Em *em, u8 *p);
u8 *EmScript_GoalScriptPlayer(Em *em, u8 *p);
u8 *EmScript_IfTargetOffHeading(Em *em, u8 *p);
u8 *EmScript_SwitchArea(Em *em, u8 *p);
u8 *EmScript_SetAreaSeq(Em *em, u8 *p);
u8 *EmScript_StepAreaSeq(Em *em, u8 *p);
u8 *EmScript_GoalDestArea(Em *em, u8 *p);
u8 *EmScript_GoalAreaToward(Em *em, u8 *p);
u8 *EmScript_IfRequestedIs(Em *em, u8 *p);
u8 *EmScript_SwitchHpPercent(Em *em, u8 *p);
u8 *EmScript_SwitchRequestId(Em *em, u8 *p);
u8 *EmScript_SetRequest(Em *em, u8 *p);
u8 *EmScript_IfStanceFamily(Em *em, u8 *p);
u8 *EmScript_SwitchTargetAngle(Em *em, u8 *p);
u8 *EmScript_IfNeed2Low(Em *em, u8 *p);
u8 *EmScript_IfGoalNearXZ(Em *em, u8 *p);
u8 *EmScript_SwitchKindInArea(Em *em, u8 *p);
u8 *EmScript_Repeat(Em *em, u8 *p);
u8 *EmScript_StopRepeat(Em *em, u8 *p);
u8 *EmScript_SetNoAlert(Em *em, u8 *p);
u8 *EmScript_SwitchVariant(Em *em, u8 *p);
u8 *EmScript_IfPlayerInArea(Em *em, u8 *p);
u8 *EmScript_IfAreaTimer1Over(Em *em, u8 *p);
u8 *EmScript_IfAreaTimer2Over(Em *em, u8 *p);
u8 *EmScript_IfVarIs(Em *em, u8 *p);
u8 *EmScript_SwitchKindGroup(Em *em, u8 *p);
u8 *EmScript_GoalActionGoal(Em *em, u8 *p);
u8 *EmScript_SelectSightRow(Em *em, u8 *p);
u8 *EmScript_IfPlayerCarrying(Em *em, u8 *p);
u8 *EmScript_SetNeed4(Em *em, u8 *p);
u8 *EmScript_SetGoalKind(Em *em, u8 *p);
u8 *EmScript_IfHitReactMode(Em *em, u8 *p);
u8 *EmScript_SwitchHitReactMode(Em *em, u8 *p);
u8 *EmScript_IfState(Em *em, u8 *p);
u8 *EmScript_IfRaging(Em *em, u8 *p);
u8 *EmScript_IfGoalNear(Em *em, u8 *p);
u8 *EmScript_IfDrill(Em *em, u8 *p);
u8 *EmScript_IfOnPassThroughFloor(Em *em, u8 *p);
u8 *EmScript_IfBlind(Em *em, u8 *p);
u8 *EmScript_IfNoSenseDelay(Em *em, u8 *p);
u8 *EmScript_IfLinked(Em *em, u8 *p);
u8 *EmScript_IfLinkCombatSameArea(Em *em, u8 *p);
u8 *EmScript_IfPrevArea(Em *em, u8 *p);
u8 *EmScript_SwitchPrevArea(Em *em, u8 *p);
u8 *EmScript_StartRoute(Em *em, u8 *p);
u8 *EmScript_SetCombat(Em *em, u8 *p);
u8 *EmScript_AddHp(Em *em, u8 *p);
u8 *EmScript_IfGoalAngleOver(Em *em, u8 *p);
u8 *EmScript_IfLinkSameArea(Em *em, u8 *p);
u8 *EmScript_IfLonePlayerBait(Em *em, u8 *p);
u8 *EmScript_IfScriptPlayerState(Em *em, u8 *p);
u8 *EmScript_IfHooked(Em *em, u8 *p);
u8 *EmScript_SetDelay(Em *em, u8 *p);
u8 *EmScript_GoalPlayerFloorSpot(Em *em, u8 *p);
u8 *EmScript_IfScriptPlayerSeen(Em *em, u8 *p);
u8 *EmScript_SetAlertBuildup(Em *em, u8 *p);
u8 *EmScript_SetNoticeCount(Em *em, u8 *p);
u8 *EmScript_ComputeGoal(Em *em, u8 *p);
u8 *EmScript_AddNeed2(Em *em, u8 *p);
u8 *EmScript_AddNeed1(Em *em, u8 *p);
u8 *EmScript_AddNeed3(Em *em, u8 *p);
u8 *EmScript_IfStanceUnder(Em *em, u8 *p);
u8 *EmScript_PickNoticedPlayerReachable(Em *em, u8 *p);
u8 *EmScript_PickNoticedPlayerInArea(Em *em, u8 *p);
u8 *EmScript_IfScriptPlayerInArea(Em *em, u8 *p);
u8 *EmScript_IfScriptPlayerNoticeHigh(Em *em, u8 *p);
u8 *EmScript_IfQuestId(Em *em, u8 *p);
u8 *EmScript_SwitchQuestKey(Em *em, u8 *p);
u8 *EmScript_GoalLinkTarget(Em *em, u8 *p);
u8 *EmScript_IfOnCeiling(Em *em, u8 *p);
u8 *EmScript_IfScriptPlayerFloorAttr(Em *em, u8 *p);
u8 *EmScript_SetNoticeTimer(Em *em, u8 *p);
u8 *EmScript_IfCeilingAbove(Em *em, u8 *p);
u8 *EmScript_IfGoalValid(Em *em, u8 *p);
u8 *EmScript_IfFloorAttr(Em *em, u8 *p);
u8 *EmScript_PickNoticedPlayerSameFloor(Em *em, u8 *p);
u8 *EmScript_IfGateOpen(Em *em, u8 *p);
u8 *EmScript_SetAreaTimer2(Em *em, u8 *p);
u8 *EmScript_SwitchScriptPlayerFloorAttr(Em *em, u8 *p);
u8 *EmScript_IfVariantBits(Em *em, u8 *p);
u8 *EmScript_IfScriptPlayerNoticeOver(Em *em, u8 *p);
u8 *EmScript_ClearNoticeCounts(Em *em, u8 *p);
u8 *EmScript_IfAnyPlayerRiding(Em *em, u8 *p);
u8 *EmScript_IfNotRemote(Em *em, u8 *p);
s32 EmScript_RestartMain(Em *em, s32 p);
u8 *EmScript_IfGoalAngleOverCapped(Em *em, u8 *p);
u8 *EmScript_SwitchType(Em *em, u8 *p);
u8 *EmScript_IfLevelAtLeast(Em *em, u8 *p);
u8 *EmScript_IfVersus(Em *em, u8 *p);
u8 *EmScript_SwitchItemUseKind(Em *em, u8 *p);
u8 *EmScript_IfVersusItemUsed(Em *em, u8 *p);
u8 *EmScript_SwitchSpawnParam(Em *em, u8 *p);
u8 *EmScript_SwitchSeason(Em *em, u8 *p);
u8 *EmScript_IfDay(Em *em, u8 *p);
u8 *EmScript_IfGoalPlayerInArc(Em *em, u8 *p);
u8 *EmScript_SwitchKindQuery(Em *em, u8 *p);
u8 *EmScript_SwitchBaseArea(Em *em, u8 *p);
u8 *EmScript_IfInArena(Em *em, u8 *p);
u8 *EmScript_RandomBranch(Em *em, u8 *p);
u8 *EmScript_CallScript(Em *em, u8 *p);
u8 *EmScript_JumpEntry(Em *em, u8 *p);
u8 *EmScript_SwitchTargetDist(Em *em, u8 *p);
u8 *EmScript_PlaceAtPoint(Em *em, u8 *p);
void *EmScript_FacePoint(Em *em, u8 *p);
s32 EmScript_Nop(s32 a, s32 p);
s32 EmScript_NopAlt(s32 a, s32 p);
u8 *EmScript_SwitchScriptValue(Em *em, u8 *p);
void *EmScript_SetHeading(Em *em, u8 *p);
u8 *EmScript_End(Em *em, u8 *p);
s32 EmScript_Start(Em *em, u8 *p);
s32 Armor_HasPartFlag(EquipItem *item, u8 which);
s32 Quest_IsVersus(void);
typedef struct ArmorIds {
    u8 id[4];
} ArmorIds;
typedef struct ArmorCombo {
    u8 a;
    u8 b;
    ArmorIds ids;
} ArmorCombo;
extern ArmorCombo gArmorCombos[14];
s32 Side_Match(u8 a, u8 b, u8 none);
u16 Vec_Heading(f32 *a, f32 *b);
extern u8 *gEmDamagedPartTbls[];
void Em_CheckRageByHp(Em *em, s16 n);
void *EmScript_LoopPointList(Em *em, u8 i);
void Em_RestartScript(Em *em);
s32 Lure_PlayerHasMonsterBait(void *pl);
void *EmScript_AreaSeqList(Em *em, u8 i);
void *EmScript_AreaSeqScript(Em *em, u8 i);
u8 *EmScript_PickWeighted(Em *em, u8 *p);
u16 Area_DayId(u16 id);
s32 Em_KindMatches(u8 type, u8 v);
u16 Math_AngleXZ(f32 *from, f32 *to);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Area_ToWorld(s32 area, f32 *pos, f32 *out);
s32 Quest_GetSeason(void);
void *GmLoad_GetEmData(s32 i);
extern s32 gEmScriptSwitchValue;
void EmScript_SetPos(Em *obj, u8 *value);
void *EmScript_LoopScript(Em *obj, u8 i);
struct EmIdEntry *Em_FindAreaEntry(struct Em *em, struct EmIdEntry *entry, s16 id);
s32 Em_CanEnterArea(Em *em, u16 area);
u16 Area_IdForCurrentTime(u16);
s16 Player_IsState(void *obj, u8 state, u8 sub);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
u8 Obj_OnPassThroughFloor(void *);
s16 Pouch_Count(Player *p, u16 item);
u16 ItemGet_Result(void);
void Em_EndStatus1And2(Em *em);
u8 Notice_Push(u8, s16, s16);
s32 Player_IsLocal(void *);
s32 MsgEvent_Check(u16 kind);
void Em_ComputeGoal(Em *em, f32 *target);
void Player_RunActionState(Player *);
void Player_RunWeaponState(Player *);
void Player_RunReactState(Player *);
void Player_RunFaintState(Player *em);
void Player_RunEventState(Player *);
void Player_RunCarryState(Player *);
void Player_StateGestureLieDown(Player *em, s32 arg1);
void Player_StateGestureGetUp(Player *em, s32 arg1);
void Player_StateGestureScout(Player *, s32);
void Player_StateGesture03(Player *, s32);
void Player_StateGesture03End(Player *em, s32 arg1);
void Player_StateGesture05(Player *, s32);
void Player_StateGesture06(Player *, s32);
void Player_StateGesture07(Player *, s32);
void Player_StateGesture08(Player *, s32);
void Player_StateGestureOneShot(Player *, s32);
void Player_StateGesture0A(Player *, s32);
void Player_StateGesture0B(Player *, s32);
void Player_StateGesture0C(Player *, s32);
void Player_StateGesture10(Player *, s32);
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
void Player_ReturnToIdle(Player *p, s32 action, s32 a2, s32 a3);
u16 Player_FindActionSpot(Player *, f32 *, u16 *arg, u8 *);
void Player_ResetAim(Player *p);
extern s8 gEmKindTimerIndex[];
extern s16 gEmBlindTimes[];
void *EmScript_SubScript(Em *obj, u8 i);
s32 Player_HasSkill(Player *p, u8 skill);
extern s16 gEmCombatInit[];
void Em_BreakPart(Em *em, s32 part);
extern s16 gEmKindTimerValues[];
/* unit's own declaration: n declared s32; with the definition's s16 EmScript_AddHp's call compiles differently */
void Em_AddHp(Em *em, s32 n);
s32 Versus_TeamHasFlag(Em *em, s32 kind);
extern s32 gEmStatusParams[];
void Save_SetEventFlag(s32 n);
s32 Save_TestEventFlag(s32);
s32 Game_IsOnline(void);
u8 *EmScript_SkipBlock(Em *em, u8 *p, u8 op, u8 end);
u8 *EmScript_SkipToEnd(Em *em, u8 *p, u8 op);
s32 Map_GetPosition(f32 *x, f32 *z, s32 area);
extern f32 gAreaGroundY[];
s32 Versus_AddGauge(Em *em, u8 n);
extern s32 gEmNoticeThresholds[];
void Em_ComputeGoal(Em *em, f32 *out);
struct AreaTri;
/* unit's own declaration: declared returning s32; the definition returns u8, with which EmScript_IfCeilingAbove compiles differently */
s32 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
u16 Map_DayId(u16);
u8 Daytime_Get(void);
void Em_RequestSync(Em *em, u8 a);
void EmScript_Restart(Em *);
extern s32 gEmAreaTimer2Values[];
extern s32 gEmGoalPoints[];
extern s32 gQSyncSendPeriod; /* gm_sub; this unit reads its low half */
s16 *Em_NextAreasFrom(Em *em, u16 area);
/* end of prototypes */

void Player_RunGestureState(Player *pl)
{
    u8 state;

    Player_ResetAim(pl);
    state = pl->state;
    switch (state) {
    case PL_GESTURE_LIE_DOWN:
        Player_StateGestureLieDown(pl, 0);
        break;
    case PL_GESTURE_GET_UP:
        Player_StateGestureGetUp(pl, 0);
        break;
    case PL_GESTURE_SCOUT:
        Player_StateGestureScout(pl, 0);
        break;
    case PL_GESTURE_03:
        Player_StateGesture03(pl, 0);
        break;
    case PL_GESTURE_03_END:
        Player_StateGesture03End(pl, 0);
        break;
    case PL_GESTURE_05:
        Player_StateGesture05(pl, 0);
        break;
    case PL_GESTURE_06:
        Player_StateGesture06(pl, 0);
        break;
    case PL_GESTURE_07:
        Player_StateGesture07(pl, 0);
        break;
    case PL_GESTURE_08:
        Player_StateGesture08(pl, 0);
        break;
    case PL_GESTURE_ONESHOT0:
        Player_StateGestureOneShot(pl, 0);
        break;
    case PL_GESTURE_0A:
        Player_StateGesture0A(pl, 0);
        break;
    case PL_GESTURE_0B:
        Player_StateGesture0B(pl, 0);
        break;
    case PL_GESTURE_0C:
        Player_StateGesture0C(pl, 0);
        break;
    case PL_GESTURE_ONESHOT1:
        Player_StateGestureOneShot(pl, 1);
        break;
    case PL_GESTURE_06_ALT:
        Player_StateGesture06(pl, 0);
        break;
    case PL_GESTURE_ONESHOT3:
        Player_StateGestureOneShot(pl, 3);
        break;
    case PL_GESTURE_10:
        Player_StateGesture10(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

void ObjAttach_UpdateAllTimers(void);
/*
 * The players' frame: reads the pad (Player_UpdateInput), updates the
 * PLAYER_SLOTS players (Player_Update), then the attack objects' timers
 * (ObjAttach_UpdateAllTimers).
 */
void Game_UpdatePlayers(void)
{
    s32 i;

    Player_UpdateInput();
    for (i = 0; i < 10; i++) {
        Player_Update(&gPlayers[i]);
    }
    ObjAttach_UpdateAllTimers();
}

/*
 * Runs the handler of the player's state group (Player_RunActionState ...
 * Player_RunGestureState for PL_GROUP_ACTION to PL_GROUP_GESTURE), after
 * setting freeState 0 and fieldCollide 1.
 */
void Player_RunState(Player *pl)
{
    pl->freeState = 0;
    pl->fieldCollide = 1;
    switch (pl->stateGroup) {
    case 0:
        Player_RunActionState(pl);
        break;
    case 1:
        Player_RunWeaponState(pl);
        break;
    case 2:
        Player_RunReactState(pl);
        break;
    case 3:
        Player_RunFaintState(pl);
        break;
    case 4:
        Player_RunEventState(pl);
        break;
    case 5:
        Player_RunCarryState(pl);
        break;
    case 6:
        Player_RunGestureState(pl);
        
    default:
        break;
    }
}

/*
 * Offline, for the local player: when a visible EM_KIND_KUSHALA_DAORA is in
 * the player's area, sets EVENT_FLAG_KUSHALA_SEEN (the Kushala Daora has been
 * seen).
 */
void Player_CheckKushalaSeen(Player *pl)
{
    Em *em;
    s16 i;

    if (Player_IsLocal(pl) == 0 || Game_IsOnline() == 1) {
        return;
    }
    em = gEms;
    for (i = 0; i < EM_COUNT; i++) {
        if (em->active != 0 && em->visible != 0 && em->type == EM_KIND_KUSHALA_DAORA && em->area == pl->area && Save_TestEventFlag(EVENT_FLAG_KUSHALA_SEEN) == 0) {
            Save_SetEventFlag(EVENT_FLAG_KUSHALA_SEEN);
        }
        em++;
    }
}

void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
struct DrawSlot;
s32 OT_AddByDepth(u32 *table, struct DrawSlot *node, u32 n, s32 clamp);
s32 Player_LocalCanUseBinoculars(void);
f32 Camera_Distance(void *out);
s16 HuntingHorn_PlayKind(Player *);
extern u16 gFrameTick; /* sub_main: the frame counter */

s32 CamCtl_GetPlayerFade(f32 *out);
void Player_RecordActionSpot(Player *p);
void Player_UpdateAreaPos(void *obj);
void Player_UpdateNearMonster(Player *p);
s32 Em_FindFloorTri(Em *em, f32 *out, s32 apply);
void HuntingHorn_ClearNotes(struct Player *em);
void Player_ReactToHit(struct Player *em);
/* A player's per-frame update (Game_UpdatePlayers). */
void Player_Update(Player *pl)
{
    f32 floorY;
    f32 pos[3];
    f32 fade;
    f32 buf[3];
    f32 y;
    f32 groundY;
    s32 clamp;
    u8 group;
    u8 w;
    s16 n;

    if (pl->active == 0) {
        return;
    }
    Player_CheckKushalaSeen(pl);
    Player_UpdateInBattle(pl);
    Player_TickTimers(pl);
    pl->pouchHudFlags = 0;
    pl->offerStarted = 0;
    if (pl->supplyBoxOpen == 0 || pl->deliveryMenu != 0) {
        Player_UpdateItemSelect(pl);
        Player_BowgunAmmoSwitch(pl);
    }
    ((Em *) pl)->prevPosX = pl->pos[0];
    ((Em *) pl)->prevPosY = pl->pos[1];
    ((Em *) pl)->prevPosZ = pl->pos[2];
    pl->unk58C = -1;
    pl->unk58E = -1;
    pl->unk3F8 = 0;
    pl->bowgunReady = 0;
    pl->freeState = 0;
    if (Player_IsLocal(pl) == 1) {
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_FALL) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB) == 0) {
            pl->fallGrabCount = 0;
        }
        Player_CheckTeamComboLost(pl);
    } else {
        pl->fallGrabCount = 0;
    }
    Player_ReactToHit((struct Player *)pl);
    pl->prevFreeState = 0;
    switch ((u8) Player_PickStatusState(pl)) {
    case 1:
        Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS2, 0);
        break;
    case 2:
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4) == 0 && Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT) == 0) {
            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS4, 0);
        }
        break;
    }
    Player_CalcAttack(pl);
    Player_CalcDefense(pl);
    Player_CalcResists(pl);
    if (pl->hitStop == 0) {
        Player_UpdateEnvironment(pl);
        Player_RunState(pl);
        if (pl->stateRequest != 0) {
            Player_RunState(pl);
            pl->stateRequest = 0;
        }
        Player_UpdateAutoTurn(pl);
        Player_FindLookTarget(pl);
        if (pl->stateGroup != PL_GROUP_EVENT) {
            Player_UpdateMoveTo(pl);
        }
    }
    Player_CheckPointMessages((u8 *) pl);
    pl->prevFreeState = pl->freeState;
    if (pl->weaponOut == 0) {
        pl->demonMode = 0;
    }
    Player_UpdateStatusIcons(pl);
    if (pl->statusIcons & 0x400) {
        PlayerFx_TickStatus6(pl);
    }
    if (pl->statusIcons & 0x800) {
        PlayerFx_TickStatus7(pl);
    }
    pl->actRequest = 0;
    pl->offerRequest = 0;
    Player_UpdateHitStop(pl);
    if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) == 0 || (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) != 0 && pl->stateVar0 == 0)) {
        Player_UpdateMotionStep(pl);
    }
    Player_UpdateHitTimers(pl);
    pl->handlers->motionFxStep(pl);
    if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB) == 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_STEP_UP) == 0) {
        if ((u8) Obj_InCurrentArea(pl)) {
            Em_CollideField((struct Em *)pl, 0);
        }
    }
    if (pl->stateGroup == PL_GROUP_ACTION && (pl->state == 1 || pl->state == 2 || pl->state == 3 || pl->state == 0x13) && (u8) Obj_InCurrentArea(pl)) {
        Em_FindFloorTri((Em *)pl, buf, 1);
    }
    Player_RecordActionSpot(pl);
    if (Em_GetFloorInfo((struct Em *)pl, pl->pos, (struct TriHdr *)(&(pl)->floorKind), &floorY, (s32 *)(&(pl)->surfaceY)) == 1 && Player_IsState(pl, PL_GROUP_EVENT, PL_EVENT_CART) == 0) {
        pl->floorY = floorY;
    }
    if ((u8) Obj_InCurrentArea(pl)) {
        Obj_UpdateFloorEffects((struct ObjHead *) pl);
    }
    if (pl->action != PL_POSTURE_AIRBORNE && pl->rideState == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_PULL_UP) == 0 && pl->stateGroup != PL_GROUP_EVENT && (u8) Obj_InCurrentArea(pl)) {
        groundY = pl->floorY;
        y = pl->pos[1];
        if (y < groundY) {
            pl->pos[1] = groundY;
            pl->rideState = 0;
        } else if (y >= groundY) {
            if (y - groundY < 30.0f) {
                pl->pos[1] = groundY;
                pl->rideState = 0;
            } else if (pl->hp > 0) {
            group = pl->stateGroup;
            if (group != 5) {
                if (group == 2) {
                switch (pl->state) {
                case 27:
                case 30:
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                    Player_SetState(pl, PL_GROUP_REACT, PL_REACT_MASH_FALL, 0);
                    break;
                default:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FALL, 0);
                    break;
                }
            } else {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FALL, 0);
            }
            } else {
                Player_SetState(pl, PL_GROUP_CARRY, PL_CARRY_FALL, 0);
            }
            }
        }
    }
    pl->unk417 = 1;
    Player_UpdateAreaPos(pl);
    group = pl->stateGroup;
    if (group != 2 && group != 3) {
        if (pl->weaponOut && (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) && (pl->input.pressed & PAD_R1)) {
            if (group == 0) {
                switch (pl->state) {
                case 0x1C:
                case 0x67:
                case 0x68:
                case 0x69:
                case 0x6A:
                    break;
                case 0x1B:
                case 0x3A:
                    pl->aimMode = 0;
                    break;
                default:
                    pl->aimMode ^= 1;
                    break;
                }
            } else if (group != 6 && Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_MELEE) == 0) {
                pl->aimMode ^= 1;
            }
            if (pl->action == PL_POSTURE_AIRBORNE) {
                pl->aimMode = 0;
            }
            if (Player_LocalCanUseBinoculars() == 0) {
                if ((u8) Obj_InCurrentArea(pl) == 1) {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
                Player_ResetAim(pl);
            }
        } else if (pl->weaponOut == 0) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_FIRE) != 0) {
                if (pl->input.pressed & PAD_R1) {
                    pl->aimMode ^= 1;
                }
            } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED) != 0) {
                pl->aimMode = 1;
            } else {
                Player_ResetAim(pl);
            }
        } else if (pl->isGunner != 1) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED) != 0) {
                pl->aimMode = 1;
            } else {
                Player_ResetAim(pl);
            }
        }
    }
    if (pl->aimMode == 0) {
        pl->aimLockTimer = 0;
    }
    if (pl->weaponType == WEAPON_HUNTING_HORN && HuntingHorn_PlayKind(pl) == 0) {
        HuntingHorn_ClearNotes((struct Player *)pl);
    }
    if (pl->area == gScene.area) {
        Player_UpdateNearMonster(pl);
        pl->drawSlot->pos[0] = pl->pos[0];
        pl->drawSlot->pos[1] = pl->pos[1];
        pl->drawSlot->pos[2] = pl->pos[2];
        if (Player_IsLocal(pl) == 1) {
            pl->nearCamera = 0;
        } else {
            pos[0] = pl->pos[0];
            pos[1] = pl->pos[1] + 80.0f;
            pos[2] = pl->pos[2];
            if (Camera_Distance(pos) <= 200.0f) {
                pl->nearCamera = 1;
            } else {
                pl->nearCamera = 0;
            }
        }
        if (Player_IsLocal(pl) == 0 && Player_TestFlag(pl, 0x02001600) != 0) {
            pl->rot[1] = (u16) (Vec_Heading(pl->pos, pl->prevPos) + 0x4000);
        }
        CamCtl_GetPlayerFade(&fade);
        clamp = Player_InSceneEvent() == 1;
        if (pl->nearCamera == 0 && !(fade < 1.0f)) {
            OT_AddByDepth(gDrawOt32, pl->drawSlot, 0x20, clamp);
        } else {
            OT_AddByDepth(gDrawOtSorted, pl->drawSlot, 0x40, clamp);
        }
        if (pl->syncTimer != 0) {
            (pl->syncTimer)--;
        }
        n = *(s16 *)&gQSyncSendPeriod;
        if (pl->syncTimer == 0 && gFrameTick % n == 0) {
            QSync_SendPlayer(pl, QSYNC_PL_POS, 0);
        }
    }
}

/*
 * Turns the player (rot[1]) toward targetYaw by a step that depends on the
 * state (the run and the drawn weapon's first actions turn fastest, the dash
 * slower, the turn-around not at all), and swings the body's lean with the
 * turn.
 */
void Player_UpdateAutoTurn(Player *pl)
{
    s32 step;
    u32 angleDelta;
    u16 turn;
    s32 target;
    s32 dir;

    if (pl->areaMoveState != PL_AREA_MOVE_NONE) {
        return;
    }
    if (pl->weaponOut != 0 && pl->stateGroup == PL_GROUP_ACTION && (
        (u32) (pl->state - 1) <= 1
        || pl->state == 12)) {
        step = 0x71C;
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_RUN) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH) != 0) {
        step = 0x71C;
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH) != 0) {
        step = 0x5B0;
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_TURN_AROUND) != 0) {
        step = 0;
    } else {
        step = 0xFA4;
    }
turn:
    dir = pl->rot[1];
    target = pl->targetYaw;
    angleDelta = (u16) (target - (u16) dir);
    switch (pl->motionId[0]) {
    case 3:
        turn = 0x80;
        break;
    case 4:
        turn = 0xA0;
        break;
    default:
        turn = 0;
        break;
    }
    if ((u16) (angleDelta + step) < (u32) (step * 2)) {
        pl->rot[1] = target;
        pl->unk2B8 = 0;
        if (pl->lean + 0x300 <= 0x600) {
            pl->lean = 0;
        } else if (pl->lean < 0x8000) {
            pl->lean -= 0x300;
        } else {
            pl->lean += 0x300;
        }
    } else if (angleDelta < 0x8000U) {
        pl->rot[1] = (u16) (dir + step);
        pl->lean -= turn;
    } else {
        pl->rot[1] = (u16) (dir - step);
        pl->lean += turn;
    }
    if (pl->lean <= 0xF600 && pl->lean >= 0xA00) {
        if (pl->lean >= 0x8000) {
            pl->lean = 0xF600;
        } else {
            pl->lean = 0xA00;
        }
    }
}

/*
 * The head turn toward the nearest monster (pl->lookYaw): in motion 0x26
 * only, picks the nearest active, visible monster within 1500 units
 * (lookFound) and turns the head toward it by 0x800 a frame, to about 60
 * degrees either side; without one the turn unwinds. The turn is read and
 * written as a u16 here, as the game's code does (its tests are unsigned).
 */
void Player_FindLookTarget(Player *pl)
{
    u32 i;
    u32 best;
    Em *em;
    f32 min;
    f32 dist;
    u16 relAngle;

    best = 0xFF;
    pl->lookFound = 0;
    min = 1500.0f;
    em = gEms;
    if (pl->motionId[0] == 0x26) {
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->visible != 0) {
                dist = Vec_Distance(&pl->pos[0], &em->posX);
                if (!(min < dist)) {
                    best = i;
                    min = dist;
                    pl->lookFound = 1;
                }
            }
        }
    }
    if (pl->lookFound == 0) {
        if (*(u16 *) &pl->lookYaw != 0) {
            if (*(u16 *) &pl->lookYaw + 0x400 <= 0x800 && *(u16 *) &pl->lookYaw + 0x400 >= 0) {
                *(u16 *) &pl->lookYaw = 0;
            } else if ((s16) *(u16 *) &pl->lookYaw >= 0) {
                *(u16 *) &pl->lookYaw -= 0x400;
            } else {
                *(u16 *) &pl->lookYaw += 0x400;
            }
        }
    } else {
        relAngle = (u16) (Vec_Heading(&gEms[best].posX, &pl->pos[0]) + 0x4000) + 0x10000 - pl->rot[1];
        if (relAngle >= 0x8000) {
            if (relAngle < 0xD556) {
                relAngle = -0x2AAA;
            }
            if (*(u16 *) &pl->lookYaw >= 0x8000) {
                if (*(u16 *) &pl->lookYaw - 0x800 <= relAngle) {
                    *(u16 *) &pl->lookYaw = relAngle;
                } else {
                    *(u16 *) &pl->lookYaw = *(u16 *) &pl->lookYaw - 0x800;
                }
            } else {
                *(u16 *) &pl->lookYaw -= 0x800;
            }
        } else {
            if (relAngle > 0x2AAB) {
                relAngle = 0x2AAB;
            }
            if (*(u16 *) &pl->lookYaw < 0x8000) {
                if (*(u16 *) &pl->lookYaw + 0x800 >= relAngle) {
                    *(u16 *) &pl->lookYaw = relAngle;
                } else {
                    *(u16 *) &pl->lookYaw = *(u16 *) &pl->lookYaw + 0x800;
                }
            } else {
                *(u16 *) &pl->lookYaw += 0x800;
            }
        }
    }
}

/*
 * For the local player: the advice message of the action spot it stands at
 * (MsgEvent_Check: 1 at a spot of kind 3, 3 at the delivery box, 4 at the
 * bed).
 */
void Player_CheckPointMessages(u8 *script)
{
    f32 pos[3];
    u16 spotAngle;
    s8 spotType;
    s32 spot;

    if (Player_IsLocal(script) != 0) {
        spot = Player_FindActionSpot((Player *)script, (f32 *)pos, &spotAngle, (u8 *)&spotType) & 0xFFFF;
        switch (spot) {
        case 3:
            MsgEvent_Check(MSGEV_SUPPLY_BOX);
            break;
        case 7:
            MsgEvent_Check(MSGEV_DELIVERY_BOX);
            break;
        case 5:
            MsgEvent_Check(MSGEV_BED);
            break;
        }
    }
}

/*
 * Returns how many of three special items the player holds (0 to 3);
 * Player_UpdateEnvironment takes that much health every 30 frames, and the
 * HUD flashes the health bar.
 */
s16 Player_CountSpecialItems(Player *pl)
{
    s16 n = 0;

    if (Pouch_Count(pl, 0x2A4) > 0 || ItemGet_Result() == 0x2A4) {
        n++;
    }
    if (Pouch_Count(pl, 0x2A6) > 0 || ItemGet_Result() == 0x2A6) {
        n++;
    }
    if (Pouch_Count(pl, 0x2A3) > 0) {
        n++;
    }
    return n;
}

/* With four hunters each wearing armour, looks up their four armour styles (Player.style, in any
 * order) in the 14 combinations of gArmorCombos; a match sets the team combo skill (teamSkillGroup,
 * teamSkill) and shows its message. */
void Player_CheckTeamCombo(Player *self)
{
    u8 have[4];
    u8 left[4];
    u8 want[4];
    s16 i;
    s16 n;
    Player *pl;
    s16 slot;
    u8 w;
    u8 *src;
    s16 j;
    u8 skill;
    u8 skillGroup;
    ArmorCombo *combo;
    s16 k;

    self->teamSkill = 0;
    self->teamSkillGroup = 0;
    if (Quest_IsVersus() != 1 && gScene.playerCount > 3) {
    for (pl = gPlayers, i = 0; i < 4; i++, pl++) {
        if (pl->active == 0) {
            return;
        }
        have[i] = 0xFF;
        n = 0;
        if (Armor_HasPartFlag(&pl->equip[2], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[3], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[4], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[5], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[0], pl->gender) == 1) {
            n++;
        }
        if (n == 0) {
            return;
        }
        have[i] = pl->style;
    }
    for (i = 0, combo = gArmorCombos; i < 14; i++, combo++) {
        skillGroup = combo->a;
        skill = combo->b;
        src = combo->ids.id;
        for (j = 0; j < 4; j++) {
            left[j] = have[j];
            want[j] = *src++;
        }
        for (j = 0; j < 4; j++) {
            w = want[j];
            for (slot = 0; slot < 4; slot++) {
                if (w != 0xFF && w == left[slot]) {
                    want[j] = 0xFF;
                    left[slot] = 0xFF;
                    break;
                }
            }
        }
        for (j = 0; j < 4; j++) {
            if (want[j] != 0xFF) {
                goto next;
            }
        }
        self->teamSkillGroup = skillGroup;
        self->teamSkill = skill;
        Notice_Push(NOTICE_KIND_MESSAGE, 0x1D, 0);
        return;
    next:;
    }
    }
}

/*
 * Ends the team combo skill (teamSkill, teamSkillGroup) with a message when
 * any of the quest's players has left the quest.
 */
void Player_CheckTeamComboLost(Player *pl)
{
    s16 i;

    if (pl->teamSkill != 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (gScene.inQuest[i] != 1) {
                pl->teamSkill = 0;
                pl->teamSkillGroup = 0;
                Notice_Push(NOTICE_KIND_MESSAGE, 0x1E, 0);
                return;
            }
        }
    }
}

/*
 * The hit-free frames of an evasion: noHitTimer 6, plus 2 with skill 0xAF or
 * else plus 4 with skill 0xB0.
 */
void Player_SetEvadeFrames(Player *pl)
{
    s16 frames = 6;

    if (Player_HasSkill(pl, 0xAF) == 1) {
        frames += 2;
    } else if (Player_HasSkill(pl, 0xB0) == 1) {
        frames += 4;
    }
    pl->noHitTimer = frames;
}

/* 1 while status 6 lasts (Player.status6Timer); the drink and eat items
 * refuse to start then. */
s32 Player_Status6Active(Player *pl)
{
    return pl->status6Timer > 0;
}

/* 1 while status 7 lasts (Player.status7Timer; the HUD's status icon). */
s32 Player_Status7Active(Player *pl)
{
    return pl->status7Timer > 0;
}

/*
 * Returns 1 when the item id is in a fixed list, else 0: the Chameleos
 * prefers these when it steals from the pouch, and Item_IsUsable refuses them
 * while status 6 lasts (Player_Status6Active).
 */
s32 Item_IsStealable(s32 item)
{
    switch (item & 0xFFFF) {
    case ITEM_ID_HEAL_20:
    case ITEM_ID_CURE_POISON:
    case ITEM_ID_HEAL_10_CURE:
    case ITEM_ID_HEAL_30:
    case ITEM_ID_HEAL_50:
    case ITEM_ID_SUPPLY_HEAL_30:
    case ITEM_ID_MAX_HP_10:
    case ITEM_ID_MAX_HP_20:
    case ITEM_ID_MAX_HP_FULL:
    case ITEM_ID_SUPPLY_MAX_HP_FULL:
    case ITEM_ID_MAX_ALL:
    case ITEM_ID_STAMINA_LOCK:
    case ITEM_ID_STAMINA_LOCK_LONG:
    case ITEM_ID_ATTACK_DRUG:
    case ITEM_ID_ATTACK_DRUG_STRONG:
    case ITEM_ID_DEFENSE_DRUG:
    case ITEM_ID_DEFENSE_DRUG_STRONG:
    case ITEM_ID_SUPPLY_MAX_STAMINA_75:
    case ITEM_ID_HURT_CURE_CHANCE:
    case ITEM_ID_CURE_CHANCE:
    case ITEM_ID_POISON_GAMBLE:
    case ITEM_ID_ATTACK_SHARED:
    case ITEM_ID_DEFENSE_SHARED:
    case ITEM_ID_COOL_DRINK:
    case ITEM_ID_HOT_DRINK:
    case ITEM_ID_HEAL_50_TEAM:
    case ITEM_ID_DETECT:
    case ITEM_ID_REGEN:
    case ITEM_ID_MAX_STAMINA_75_B:
    case ITEM_ID_ATTACK_BURST:
    case ITEM_ID_DEFENSE_BURST:
    case ITEM_ID_MAX_STAMINA_75:
    case ITEM_ID_RANDOM:
    case ITEM_ID_FOOD_STAMINA_75:
    case ITEM_ID_FOOD_STAMINA_150:
    case ITEM_ID_FOOD_STAMINA_FULL:
    case ITEM_ID_FOOD_STAMINA_GAMBLE:
    case ITEM_ID_FOOD_HEAL_20:
    case ITEM_ID_FOOD_PRIZE_A:
    case ITEM_ID_FOOD_PRIZE_B:
    case ITEM_ID_FOOD_HOT:
    case ITEM_ID_FOOD_HOT_STAMINA:
    case ITEM_ID_FOOD_COOL:
    case ITEM_ID_FOOD_COOL_STAMINA:
    case ITEM_ID_FOOD_CURE:
    case ITEM_ID_FOOD_CURE_STAMINA:
    case ITEM_ID_FOOD_RESTORE:
    case ITEM_ID_FOOD_REGEN_SHORT:
    case ITEM_ID_FOOD_REGEN:
    case ITEM_ID_FOOD_REGEN_B:
    case ITEM_ID_FOOD_REGEN_LONG:
    case ITEM_ID_FOOD_RESTORE_B:
        return 1;
    }
    return 0;
}

/*
 * Picks the status a monster begins, or 0: only when none runs (em->status 0)
 * and the last hit was not attack kind 6 or 9. In the air (EM_STANCE_AIR, or
 * EM_STANCE_FAMILY for the Kushala family) it ends statuses 1 and 2
 * (Em_EndStatus1And2) and returns 1; else 4 for a status 4 build-up, 2 for
 * status 1, 3 for status 2, 5 for status 5 (the stun). Em_UpdateDamage turns
 * these into EM_DMG_*.
 */
u8 Em_PickStatus(Em *em)
{
    u8 ret = 0;
    s32 pending;

    if (em->status == 0 && em->lastAttackKind != 6 && em->lastAttackKind != 9) {
        pending = (u8) (em->statusFlags & ~EM_STATUSF_POISONED);
        if (pending != 0) {
            if (em->stance == 2 || ((em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA || em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && em->stance == 1)) {
                Em_EndStatus1And2(em);
                ret = 1;
            } else if ((u8) pending & 8) {
                ret = 4;
            } else if ((u8) pending & 1) {
                ret = 2;
            } else if ((u8) pending & 2) {
                ret = 3;
            } else if ((u8) pending & 0x10) {
                ret = 5;
            }
        }
    }
    return ret;
}

/* Returns 1 while the monster is held (holdTimer or holdTimer2), else 0. */
s32 Em_IsHeld(Em *em)
{
    if (*(s8 *) &em->holdTimer) {
        return 1;
    }
    if (*(s8 *) &em->holdTimer2) {
        return 1;
    }
    return 0;
}

/*
 * Starts the monster's flash blindness: blindTimer from gEmBlindTimes[type],
 * with the stun stars over its head (StunStars_CreateEm, type 2) at a
 * height and size per kind; kinds with no case get none.
 */
void Em_StartBlind(Em *em)
{
    em->blindTimer = gEmBlindTimes[em->type];
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_YIAN_GARUGA:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_RATHALOS:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_GRAVIOS:
    case EM_KIND_CEPHADROME:
    case EM_KIND_DIABLOS:
    case EM_KIND_CEPHALOS:
        StunStars_CreateEm(em, 2, 0x22, em->blindTimer, 150.0f, 3.0f);
        break;
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
    case EM_KIND_FATALIS:
        StunStars_CreateEm(em, 2, 0x33, em->blindTimer, 150.0f, 3.0f);
        break;
    case 0x9:
    case EM_KIND_MELYNX:
        StunStars_CreateEm(em, 2, 0, em->blindTimer, 90.0f, 1.0f);
        break;
    case EM_KIND_MOSSWINE:
        StunStars_CreateEm(em, 2, 0, em->blindTimer, 100.0f, 1.0f);
        break;
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IOPREY:
    case EM_KIND_IODROME:
    case EM_KIND_GIAPREY:
        StunStars_CreateEm(em, 2, 0xE, em->blindTimer, 50.0f, 1.0f);
        break;
    case 0xC:
    case EM_KIND_APCEROS:
        StunStars_CreateEm(em, 2, 0xF, em->blindTimer, 50.0f, 1.5f);
        break;
    case EM_KIND_KELBI:
    case EM_KIND_KIRIN:
    case EM_KIND_ANTEKA:
        StunStars_CreateEm(em, 2, 0x17, em->blindTimer, 50.0f, 1.5f);
        break;
    case EM_KIND_VESPOID:
    case EM_KIND_HORNETAUR:
        StunStars_CreateEm(em, 2, 8, em->blindTimer, 20.0f, 1.0f);
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
    case EM_KIND_BLANGONGA:
        StunStars_CreateEm(em, 2, 0x11, em->blindTimer, 100.0f, 2.0f);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
        StunStars_CreateEm(em, 2, 0x25, em->blindTimer, 150.0f, 3.0f);
        break;
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        StunStars_CreateEm(em, 2, 0x25, em->blindTimer, 270.0f, 3.0f);
        break;
    case EM_KIND_SHAKALAKA:
        StunStars_CreateEm(em, 2, 0xB, em->blindTimer, 30.0f, 1.0f);
        break;
    case EM_KIND_CHAMELEOS:
        StunStars_CreateEm(em, 2, 0x27, em->blindTimer, 200.0f, 3.5f);
        break;
    case EM_KIND_POPO:
        StunStars_CreateEm(em, 2, 0xE, em->blindTimer, 100.0f, 1.5f);
        
    case 0x0:
    default:
        break;
    }
}

void Em_ChangeState(Em *em, s32 arg1, u16 arg2, s32 arg3); /* unit's own declaration: the state declared
 * u16, so Em_ApplyFlash passes its u16 st as it is; with the definition's s32 the call would mask it to 16 bits */
s32 EmBlangonga_IgnoresFlash(Em *em);
s32 EmBlangonga_InStateArmor(Em *em);
/* The monster's reaction to a flash (Game's hit dispatcher calls it when em->flashed was set):
 * by type and stance, returns early (states that ignore it) or picks the state st (7-10, or 2
 * with Em_StartBlind and the timer), then Em_ChangeState(em, 4, st, 3). Case 0x39's
 * `switch (em->stateGroup)` is written with its default first (falling into the return of
 * 4-6): with that form the compiler lays out every branch of the function as the game's code
 * has it. The other groups' tests are plain if/else and && chains. Em_ChangeState is declared
 * above with a u16 state (its definition takes an s32): with the s32 the call would mask st
 * to 16 bits, which the game's code doesn't.
 */
void Em_ApplyFlash(Em *em)
{
    u16 flashState;
    u8 group;

    switch (em->type) {
    case EM_KIND_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
        if (em->stance == 4) {
            return;
        }
    case EM_KIND_RATHIAN:
    case EM_KIND_FATALIS:
    case EM_KIND_KELBI:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_CEPHADROME:
    case EM_KIND_RATHALOS:
    case 0xC:
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GRAVIOS:
    case EM_KIND_VESPOID:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_BASARIOS:
    case EM_KIND_HORNETAUR:
    case EM_KIND_APCEROS:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IOPREY:
    case EM_KIND_IODROME:
    case EM_KIND_KIRIN:
    case EM_KIND_CEPHALOS:
    case EM_KIND_GIAPREY:
    case EM_KIND_FATALIS_B:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_YIAN_GARUGA:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_CHAMELEOS:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
    case EM_KIND_ANTEKA:
    case EM_KIND_POPO:
    case EM_KIND_FATALIS_C:
        em->blind = 10;
        Em_StartBlind(em);
        if (em->remote != 0) {
            return;
        }
        if (Em_IsHeld(em) == 1) {
            return;
        }
        if (!(em->statusFlags & 8)) {
            break;
        }
        return;
    case EM_KIND_MOSSWINE:
    case 0x9:
    case EM_KIND_MELYNX:
    case EM_KIND_SHAKALAKA:
        if (em->remote != 0) {
            return;
        }
        if (Em_IsHeld(em) == 1) {
            return;
        }
        if (!(em->statusFlags & 8)) {
            break;
        }
        return;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        if (EmBlangonga_InStateArmor(em) == 1) {
            return;
        }
        em->blind = 10;
        Em_StartBlind(em);
        if (em->remote != 0) {
            return;
        }
        if (Em_IsHeld(em) == 1) {
            return;
        }
        if (!(em->statusFlags & 8)) {
            break;
        }
        return;
    default:
        return;
    }
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_AZURE_RATHALOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_PURPLE_GYPCEROS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BASARIOS:
    case EM_KIND_BLACK_GRAVIOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_YIAN_GARUGA:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            group = em->stateGroup;
            if (group == 4 || (group == 0 && em->state == 0x1C) || (group == 0 && em->state == 0x1D)
                || (group == 0 && em->state == 0x21) || (group == 4 && em->state == 6)
                || (group == 4 && em->state == 0xD) || (group == 4 && em->state == 0xE)
                || (group == 4 && em->state == 0x11) || (group == 4 && em->state == 0x12)) {
                return;
            }
            flashState = 10;
        }
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        if (em->stance == 2) {
            flashState = 8;
        } else {
            flashState = 7;
        }
        break;
    case 0x9:
    case EM_KIND_MELYNX:
        if (em->stance == 0 && em->stateGroup != 4 && em->stateGroup != 5 && em->stateGroup != 6
            && Player_IsState((Player *) em, 4, 2) == 0) {
            flashState = 2;
            Em_StartBlind(em);
            em->timer = em->blindTimer;
            break;
        }
        return;
    case EM_KIND_SHAKALAKA:
        switch (em->stateGroup) {
        default:
            if (Player_IsState((Player *) em, 4, 2) == 0 && Player_IsState((Player *) em, 4, 4) == 0) {
                if (em->stance == 0) {
                    flashState = 2;
                }
                em->blind = 10;
                Em_StartBlind(em);
                em->timer = em->blindTimer;
                break;
            }
        case 6:
        case 5:
        case 4:
            return;
        }
        break;
    case EM_KIND_MOSSWINE:
        if (em->stance == 0 && em->stateGroup != 4 && em->stateGroup != 5 && em->stateGroup != 6
            && Player_IsState((Player *) em, 4, 4) == 0) {
            flashState = 4;
            Em_StartBlind(em);
            em->timer = em->blindTimer;
            break;
        }
        return;
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_IOPREY:
    case EM_KIND_GIAPREY:
        if (em->stance == 2) {
            flashState = 1;
        } else {
            flashState = 5;
        }
        break;
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
        if (em->stance == 2) {
            flashState = 1;
        } else {
            flashState = 5;
        }
        break;
    case 0xC:
    case EM_KIND_APCEROS:
    case EM_KIND_POPO:
        flashState = 5;
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        switch (em->stance) {
        case 0:
            flashState = 10;
            break;
        case 1:
            flashState = 4;
            break;
        case 4:
            flashState = 0x11;
            break;
        case 2:
            flashState = 9;
            break;
        default:
            flashState = 10;
            break;
        }
        break;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        switch (em->stance) {
        case 0:
            flashState = 10;
            break;
        case 1:
            flashState = 4;
            break;
        case 4:
            flashState = 0x11;
            break;
        case 2:
            flashState = 9;
            break;
        default:
            flashState = 10;
            break;
        }
        break;
    case EM_KIND_KELBI:
    case EM_KIND_ANTEKA:
        flashState = 4;
        break;
    case EM_KIND_VESPOID:
    case EM_KIND_HORNETAUR:
        if (em->stance != 2) {
            flashState = 5;
        } else {
            flashState = 4;
        }
        break;
    case EM_KIND_KIRIN:
        flashState = 4;
        break;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        if (em->stance == 2) {
            flashState = 7;
        } else {
            if (EmBlangonga_IgnoresFlash(em) == 1) {
                return;
            }
            flashState = 1;
        }
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        if (em->stance == 2 || em->stance == 1) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_CHAMELEOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    }
    em->scriptPending = 0;
    /* the game reads a register it never set: st is s0, still the caller's, after case 0x39 with em->stance != 0 */
    Em_ChangeState(em, 4, flashState, 3);
}

/*
 * Called by GmEm_Update when a grounded monster's flash blindness
 * (blindTimer) runs out outside EM_GROUP_REACT: puts it into an idle state
 * that plays one motion. The Rathalos family goes to RATH_IDLE_SHAKE_OFF
 * (0x19), the Cephadrome kinds to state 7, the Plesioth kinds to state 7 only
 * on the ground; other kinds are left as they are.
 */
void Em_ForceState19(Em *em)
{
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_ChangeState(em, 0, 0x19, 4);
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_YIAN_GARUGA:
        break;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        EmCephadrome_ChangeState(em, 0, 7, 4);
        break;
    case 0x9:
    case EM_KIND_MELYNX:
    case EM_KIND_MOSSWINE:
    case EM_KIND_BULLFANGO:
    case EM_KIND_BULLDROME:
    case 0x20:
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
    case EM_KIND_IOPREY:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
    case 0xC:
    case EM_KIND_POPO:
    case EM_KIND_APCEROS:
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        if (em->stance == 0) {
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        break;
    }
}

/*
 * The monster's per-frame poison and status build-ups, from the kind's
 * parameter tables (GmEm_Update, outside EM_GROUP_DEFEAT). While poisoned
 * (EM_STATUSF_POISONED) it counts poisonDamage down, shows the poison effect
 * once a second, and at each poisonTimer tick takes the kind's poison damage
 * off hp (hp stays at 1 under guardTimer and for a large monster). When the
 * poison ends, up to four times (poisonCount) poisonThreshold rises by the
 * kind's step. The build-ups of poison and of statuses 4, 5, 1 and 2 not
 * running lose decayAmount every decayInterval frames.
 */
void Em_UpdateStatus(Em *em)
{
    u8 *poisonParam = (u8 *) gEmStatusParams[em->type];
    EmStatusParam *status4Param = gEmStatus4Params[em->type];
    EmStatusParam *status5Param = gEmStatus5Params[em->type];
    EmStatusParam *status2Param;
    EmStatusParam *status1Param = gEmStatus1Params[em->type];
    f32 pos[3];
    s32 bone;
    f32 scale;

    status2Param = gEmStatus2Params[em->type];

    if (em->statusFlags & EM_STATUSF_POISONED) {
        if (--em->poisonDamage > 0) {
            switch (em->type) {
            case EM_KIND_RATHIAN:
            case EM_KIND_YIAN_KUT_KU:
            case EM_KIND_CEPHADROME:
            case EM_KIND_RATHALOS:
            case EM_KIND_DIABLOS:
            case EM_KIND_KHEZU:
            case EM_KIND_GRAVIOS:
            case EM_KIND_GYPCEROS:
            case EM_KIND_PLESIOTH:
            case EM_KIND_BASARIOS:
            case EM_KIND_MONOBLOS:
            case EM_KIND_CEPHALOS:
            case EM_KIND_PINK_RATHIAN:
            case EM_KIND_BLUE_YIAN_KUT_KU:
            case EM_KIND_PURPLE_GYPCEROS:
            case EM_KIND_YIAN_GARUGA:
            case EM_KIND_SILVER_RATHALOS:
            case EM_KIND_GOLD_RATHIAN:
            case EM_KIND_BLACK_DIABLOS:
            case EM_KIND_WHITE_MONOBLOS:
            case EM_KIND_RED_KHEZU:
            case EM_KIND_GREEN_PLESIOTH:
            case EM_KIND_BLACK_GRAVIOS:
            case EM_KIND_AZURE_RATHALOS:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 0x22;
                scale = 2.0f;
                break;
            case EM_KIND_FATALIS:
            case EM_KIND_FATALIS_B:
            case EM_KIND_FATALIS_C:
                pos[0] = 0.0f;
                pos[1] = 200.0f;
                pos[2] = 0.0f;
                bone = 0x33;
                scale = 2.0f;
                break;
            case EM_KIND_KELBI:
            case EM_KIND_ANTEKA:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0x17;
                scale = 1.0f;
                break;
            case 0xC:
            case EM_KIND_APCEROS:
            case EM_KIND_POPO:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0xF;
                scale = 1.2f;
                break;
            case 0x9:
            case EM_KIND_MELYNX:
            case EM_KIND_SHAKALAKA:
                pos[0] = 0.0f;
                pos[1] = 20.0f;
                pos[2] = 0.0f;
                bone = 0xA;
                scale = 1.2f;
                break;
            case EM_KIND_GENPREY:
            case EM_KIND_VELOCIPREY:
            case EM_KIND_VELOCIDROME:
            case EM_KIND_GENDROME:
            case EM_KIND_IOPREY:
            case EM_KIND_IODROME:
            case EM_KIND_GIAPREY:
                pos[0] = 0.0f;
                pos[1] = 20.0f;
                pos[2] = 0.0f;
                bone = 0xE;
                scale = 1.2f;
                break;
            case EM_KIND_MOSSWINE:
            case EM_KIND_BULLFANGO:
            case 0x20:
            case EM_KIND_BULLDROME:
                pos[0] = 0.0f;
                pos[1] = 10.0f;
                pos[2] = 0.0f;
                bone = 0xB;
                scale = 1.0f;
                break;
            case EM_KIND_VESPOID:
            case EM_KIND_HORNETAUR:
                pos[0] = 0.0f;
                pos[1] = 0.0f;
                pos[2] = 0.0f;
                bone = 8;
                scale = 0.5f;
                break;
            case EM_KIND_KIRIN:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0x17;
                scale = 1.0f;
                break;
            case EM_KIND_BLANGONGA:
            case EM_KIND_CONGALALA:
            case EM_KIND_RAJANG:
            case EM_KIND_BLANGO:
            case EM_KIND_CONGA:
                pos[0] = 0.0f;
                pos[1] = 40.0f;
                pos[2] = 45.0f;
                bone = 0x11;
                scale = 1.0f;
                break;
            case EM_KIND_KUSHALA_DAORA:
            case EM_KIND_RUSTED_KUSHALA_DAORA:
            case EM_KIND_LUNASTRA:
            case EM_KIND_TEOSTRA:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 0x25;
                scale = 2.0f;
                break;
            case EM_KIND_DAIMYO_HERMITAUR:
            case EM_KIND_SHOGUN_CEANATAUR:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 100.0f;
                bone = 0x1F;
                scale = 2.0f;
                break;
            case EM_KIND_HERMITAUR:
            case EM_KIND_CEANATAUR:
                pos[0] = 0.0f;
                pos[1] = 40.0f;
                pos[2] = 90.0f;
                bone = 0x1F;
                scale = 1.0f;
                break;
            case EM_KIND_CHAMELEOS:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 0x25;
                scale = 2.0f;
                break;
            case EM_KIND_REMOBRA:
                pos[0] = 0.0f;
                pos[1] = 25.0f;
                pos[2] = 45.0f;
                bone = 0x13;
                scale = 0.7f;
                break;
            default:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0;
                scale = 1.0f;
                break;
            }
            if (gScene.frame % 60 == 0) {
                Effect06_SpawnAt(em, 3, bone, pos, scale);
            }
            if (--em->poisonTimer <= 0) {
                em->poisonTimer = *(s16 *)(poisonParam + 0xA);
                em->hp -= *(s16 *)(poisonParam + 6);
                if (em->hp <= 0 && ((s8) em->guardTimer > 0 || em->large != 0)) {
                    em->hp = 1;
                }
            }
        } else {
            em->poisonDamage = 0;
            em->poisonTimer = 0;
            em->statusFlags &= ~EM_STATUSF_POISONED;
            if (em->poisonCount < 4) {
                em->poisonCount++;
                em->poisonThreshold += *(s16 *)(poisonParam + 0xC);
            }
        }
    } else if (em->poisonBuildup != 0) {
        if (--em->poisonTimer <= 0) {
            em->poisonTimer = *(s16 *)(poisonParam + 2);
            em->poisonBuildup -= *(s16 *)(poisonParam + 4);
            if (em->poisonBuildup <= 0) {
                em->poisonBuildup = 0;
                em->poisonTimer = 0;
            }
        }
    }
    if (!(em->statusFlags & 8) && em->status4Buildup != 0) {
        if (--em->status4Decay <= 0) {
            em->status4Decay = status4Param->decayInterval;
            em->status4Buildup -= status4Param->decayAmount;
            if (em->status4Buildup <= 0) {
                em->status4Buildup = 0;
                em->status4Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & EM_STATUSF_STUN) && em->status5Buildup != 0) {
        if (--em->status5Decay <= 0) {
            em->status5Decay = status5Param->decayInterval;
            em->status5Buildup -= status5Param->decayAmount;
            if (em->status5Buildup <= 0) {
                em->status5Buildup = 0;
                em->status5Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & 1) && em->status1Buildup != 0) {
        if (--em->status1Decay <= 0) {
            em->status1Decay = status1Param->decayInterval;
            em->status1Buildup -= status1Param->decayAmount;
            if (em->status1Buildup <= 0) {
                em->status1Buildup = 0;
                em->status1Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & 2) && em->status2Buildup != 0) {
        if (--em->status2Decay <= 0) {
            em->status2Decay = status2Param->decayInterval;
            em->status2Buildup -= status2Param->decayAmount;
            if (em->status2Buildup <= 0) {
                em->status2Buildup = 0;
                em->status2Decay = 0;
            }
        }
    }
}

/*
 * Adds amount to the monster's poison build-up; nothing for 0, an immune kind
 * (poisonThreshold 0) or a versus team with flag 1 (Versus_TeamHasFlag).
 * While poisoned the build-up stays just under the threshold; reaching it
 * poisons the monster (EM_STATUSF_POISONED, poisonDamage the poison's duration,
 * the first tick at once).
 */
void Em_AddPoison(Em *em, s32 amount)
{
    s16 max;
    u8 *param;

    param = (u8 *) gEmStatusParams[em->type];
    if (Versus_TeamHasFlag((void *) em, 1U) != 1 && amount != 0 && em->poisonThreshold > 0) {
        em->poisonBuildup = em->poisonBuildup + amount;
        if (em->statusFlags & EM_STATUSF_POISONED) {
            max = em->poisonThreshold;
            if (em->poisonBuildup >= max) {
                em->poisonBuildup = max - 1;
            }
        } else {
            em->poisonTimer = *(s16 *)(param + 2);
            if (em->poisonBuildup >= em->poisonThreshold) {
                em->poisonBuildup = 0;
                em->statusFlags |= EM_STATUSF_POISONED;
                em->poisonDamage = *(s16 *)(param + 8);
                em->poisonTimer = 0;
            }
        }
    }
}

/*
 * Adds amount to the status 4 build-up; nothing for 0, an immune kind
 * (status4Threshold 0) or a versus team with flag 0. Only while status 4 is
 * not pending (statusFlags bit 8); reaching status4Threshold sets that bit,
 * which Em_PickStatus turns into the status.
 */
void Em_AddStatus4(Em *em, s32 amount)
{
    EmStatusParam *param;

    param = gEmStatus4Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 0U) != 1 && amount != 0 && em->status4Threshold > 0) {
        if (!(em->statusFlags & 8)) {
            em->status4Buildup = em->status4Buildup + amount;
            em->status4Decay = param->decayInterval;
        }
        if (em->status4Buildup >= em->status4Threshold) {
            em->statusFlags |= 8;
        }
    }
}

/*
 * As Em_AddStatus4 for status 5, the stun (EM_STATUSF_STUN,
 * status5Threshold), without the versus check.
 */
void Em_AddStatus5(Em *em, s32 amount)
{
    EmStatusParam *prm = gEmStatus5Params[em->type];

    if (amount != 0 && em->status5Threshold > 0) {
        if (!(em->statusFlags & EM_STATUSF_STUN)) {
            em->status5Buildup = em->status5Buildup + amount;
            em->status5Decay = prm->decayInterval;
        }
        if (em->status5Buildup >= em->status5Threshold) {
            em->statusFlags |= EM_STATUSF_STUN;
        }
    }
}

/*
 * As Em_AddStatus4 for status 2 (statusFlags bit 2, status2Threshold; versus
 * flag 2).
 */
void Em_AddStatus2(Em *em, s32 amount)
{
    EmStatusParam *param;

    param = gEmStatus2Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 2U) != 1 && amount != 0 && em->status2Threshold > 0) {
        if (!(em->statusFlags & 2)) {
            em->status2Buildup = em->status2Buildup + amount;
            em->status2Decay = param->decayInterval;
        }
        if (em->status2Buildup >= em->status2Threshold) {
            em->statusFlags |= 2;
        }
    }
}

/*
 * As Em_AddStatus4 for status 1 (statusFlags bit 1, status1Threshold; versus
 * flag 2).
 */
void Em_AddStatus1(Em *em, s32 amount)
{
    EmStatusParam *param;

    param = gEmStatus1Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 2U) != 1 && amount != 0 && em->status1Threshold > 0) {
        if (!(em->statusFlags & 1)) {
            em->status1Buildup = em->status1Buildup + amount;
            em->status1Decay = param->decayInterval;
        }
        if (em->status1Buildup >= em->status1Threshold) {
            em->statusFlags |= 1;
        }
    }
}

/* The monster's timer to the kind's status 4 duration (Em_BeginStatus4's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus4Timer(Em *em)
{
    em->timer = gEmStatus4Params[em->type]->duration;
    em->scriptPending = 0;
}

/* The monster's timer to the kind's status 5 duration (Em_BeginStatus5's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus5Timer(Em *em)
{
    em->timer = gEmStatus5Params[em->type]->duration;
    em->scriptPending = 0;
}

/* The monster's timer to the kind's status 2 duration (Em_BeginStatus2's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus2Timer(Em *em)
{
    em->timer = gEmStatus2Params[em->type]->duration;
    em->scriptPending = 0;
}

/* The monster's timer to the kind's status 1 duration (Em_BeginStatus1's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus1Timer(Em *em)
{
    em->timer = gEmStatus1Params[em->type]->duration;
    em->scriptPending = 0;
}

/*
 * Sets timer for the reaction to attack kind 0x13 (EM_DMG_ATTACK13): the
 * kind's row of gEmKindTimerValues (by gEmKindTimerIndex[type]), or 10 when
 * it has none. Also clears scriptPending.
 */
void Em_ResetKindTimer(Em *em)
{
    s8 i = gEmKindTimerIndex[em->type];

    em->scriptPending = 0;
    if (i == -1) {
        em->timer = 10;
    } else {
        em->timer = gEmKindTimerValues[i];
    }
}

s32 Em_HasKindTimer(Em *em)
{
    return gEmKindTimerIndex[em->type] != -1;
}

/* sums the damage taken per part (scaled by unk728) into *sum and *neg, and passes it on */
void Em_PickDamagedPart(Em *em, s16 *sum, s16 *neg)
{
    u8 best;
    u32 i;
    s16 max;
    s16 damage;
    u8 *weakRow;

    max = 0;
    best = 0;
    weakRow = gEmDamagedPartTbls[em->type];
    for (i = 0; i < 8; i++) {
        damage = em->hitPartDamage[i] * em->damageFactor;
        if (em->hitPartDamage[i] > 0 && damage == 0) {
            damage = 1;
        }
        if (damage < 0) {
            *neg -= damage;
        } else {
            *sum += damage;
            if (max < em->hitPartDamage[i]) {
                max = em->hitPartDamage[i];
                best = i;
            }
        }
    }
    if (weakRow != NULL) {
        if ((em->partFlags[weakRow[0]] & weakRow[1]) == weakRow[1]) {
            em->hitPartDamage[8] = em->hitPartDamage[weakRow[0]];
            if (em->hitPartDamage[8] < 0) {
                em->hitPartDamage[8] = 0;
            }
        } else {
            em->hitPartDamage[8] = 0;
        }
    }
    if (em->rage == 0 && *sum > 0) {
        Em_CheckRageByHp(em, *sum);
    }
    em->flinchPart = best;
    if ((s8) em->guardTimer <= 0) {
        Em_AddStatus1(em, em->status1Hit);
        Em_AddStatus2(em, em->status2Hit);
        Em_AddPoison(em, em->poisonHit);
        Em_AddStatus4(em, em->status4Hit);
        Em_AddStatus5(em, em->status5Hit);
    }
}

/*
 * Breaks each part (0 to 15) set in partsToBreak and not yet in partsBroken
 * (Em_BreakPart), then adds partsToBreak to partsBroken.
 */
void Em_ApplyPartBreaks(Em *em)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        if ((em->partsToBreak & (1 << i)) && !(em->partsBroken & (1 << i))) {
            Em_BreakPart(em, i);
        }
    }
    em->partsBroken = em->partsBroken | em->partsToBreak;
}

s32 Em_PartBroken(Em *em, s32 part)
{
    return (em->partsBroken & (1 << (part & 0xFFFF))) != 0;
}

/* Flags part (0 to 15) to break (partsToBreak); Em_ApplyPartBreaks breaks it. */
void Em_FlagPartBreak(Em *em, s32 part)
{
    em->partsToBreak |= (1 << (part & 0xFFFF)) & 0xFFFF;
}

/*
 * Returns 1 when result, an Em_UpdateDamage result, is a reaction (anything
 * but EM_DMG_NONE, EM_DMG_KILLED and EM_DMG_DEAD), else 0; em is unused. The
 * Kushala, Blangonga and Chameleos families' Update functions break parts
 * when it returns 1.
 */
s32 Em_DamageResultIsReaction(Em *em, s32 result)
{
    s32 code;

    code = result & 0xFF;
    if ((code != 0) && (code != 1) && (code != 2)) {
        return 1;
    }
    return 0;
}

void Em_ResetPartHp(Em *em, u32 i);
/*
 * Settles the monster's last hit and statuses and returns an EmDamageResult
 * (EM_DMG_NONE when there is nothing to react to); *out gets combat as it was
 * on entry. In EM_GROUP_DEFEAT it returns EM_DMG_NONE, and EM_DMG_DEAD at 0
 * hp with no hit pending. On a hit it drops the pending triggers 0x02, 0x10,
 * 0x20 and 0x40, takes the damage of Em_PickDamagedPart off hp and the parts'
 * hp by partDamageMode, and breaks or flinches the parts that run out. Under
 * guardTimer hp stays at 1 (EM_DMG_NONE); else 0 hp is EM_DMG_KILLED (a large
 * monster dies only in the scene's area) and the hit starts combat. The
 * result then comes from the attack kind (0x13, 9, 6), Em_PickStatus, a flash
 * (EM_DMG_FLASH) or hitReactMode and the broken parts. Every result but
 * EM_DMG_HIT also restarts the script (Em_RestartScript); Em_CheckKindRequest
 * runs last.
 */
u8 Em_UpdateDamage(Em *em, u8 *out)
{
    s16 sum;
    s16 neg;
    u8 broke;
    u8 first;
    s8 i;
    s16 damage;
    u8 bit;
    s32 (*fn)(Em *);
    u8 status;
    u8 result;

    sum = 0;
    neg = 0;
    *out = em->combat;
    broke = 0;
    if (em->stateGroup == 5) {
        return 0;
    }
    em->hitPartMask = 0;
    if (em->hp <= 0 && em->hitReceived == 0) {
        em->scriptPending = 0;
        em->combatPlayers = 0;
        return 2;
    }
    if (em->hitReceived != 0) {
        em->triggers &= 0x8D;
        Em_PickDamagedPart(em, &sum, &neg);
        if (em->hp > 0) {
            em->hp = em->hp - sum + neg;
            if (em->hpMax < em->hp) {
                em->hp = em->hpMax;
            }
        }
        first = 0;
        switch (em->partDamageMode) {
        case 1:
            for (i = 0; i < 8; i++) {
                damage = em->hitPartDamage[i] * em->damageFactor;
                if (em->hitPartDamage[i] > 0 && damage == 0) {
                    damage = 1;
                }
                em->parts[0].hp -= damage;
                if (em->parts[0].hp <= 0) {
                    broke = 1;
                    Em_ResetPartHp(em, 0);
                    if (!(em->hitPartMask & (1 << i))) {
                        em->hitPartMask |= 1 << i;
                        em->parts[0].state++;
                        if (em->parts[0].state >= 0x63) {
                            em->parts[0].state = 0x63;
                        }
                    }
                }
            }
            break;
        case 8:
            for (i = 0; i < 8; i++) {
                damage = em->hitPartDamage[i] * em->damageFactor;
                if (em->hitPartDamage[i] > 0 && damage == 0) {
                    damage = 1;
                }
                em->parts[i].hp -= damage;
                if (em->parts[i].hp <= 0) {
                    broke = 1;
                    Em_ResetPartHp(em, i);
                    if (!(em->hitPartMask & (1 << i))) {
                        em->hitPartMask |= 1 << i;
                        em->parts[i].state++;
                        if (em->parts[i].state >= 0x63) {
                            em->parts[i].state = 0x63;
                        }
                    }
                    if (first != 0) {
                        if (em->hitPartDamage[i] > em->hitPartDamage[em->flinchPart]) {
                            em->flinchPart = i;
                        }
                    } else {
                        em->flinchPart = i;
                        first = 1;
                    }
                }
            }
            break;
        case 9:
            em->part8Down = 0;
            for (i = 0; i < 9; i++) {
                damage = em->hitPartDamage[i] * em->damageFactor;
                if (em->hitPartDamage[i] > 0 && damage == 0) {
                    damage = 1;
                }
                em->parts[i].hp -= damage;
                if (em->parts[i].hp <= 0) {
                    if (i == 8) {
                        fn = em->handlers->part8DownQuery;
                        if (fn != NULL) {
                            if (fn(em) == 0 || (s8) em->guardTimer != 0) {
                                em->parts[i].hp = 1;
                                continue;
                            }
                            em->part8Down = 1;
                            goto reset;
                        } else {
                            em->parts[i].hp = 1;
                        }
                        continue;
                    }
                    if (!(em->hitPartMask & (1 << i))) {
                        em->hitPartMask |= 1 << i;
                        em->parts[i].state++;
                        if (em->parts[i].state >= 0x63) {
                            em->parts[i].state = 0x63;
                        }
                    }
                    broke = 1;
                reset:
                    Em_ResetPartHp(em, i);
                    if (first != 0) {
                        if (i < 8 && em->hitPartDamage[i] > em->hitPartDamage[em->flinchPart]) {
                            em->flinchPart = i;
                        }
                    } else {
                        em->flinchPart = i;
                        first = 1;
                    }
                }
            }
            break;
        }
        Em_SpawnPartEffects(em, em->hitPartMask);
        if ((s8) em->guardTimer != 0) {
            if (em->hp <= 0) {
                em->hp = 1;
            }
            if (em->hitReactMode != EM_HIT_REACT_WAKE) {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            if (!((em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C) && (em->lastAttackKind == 9 || em->lastAttackKind == 6))) {
                return 0;
            }
        } else {
            if ((em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG) && Area_DayId(em->area) != AREA_FORTRESS_5 && em->hp < 1000) {
                em->hp = 1000;
            }
            if (em->type == EM_KIND_SHEN_GAOREN && Map_DayId(gScene.map) == MAP_FORTRESS && Area_DayId(em->area) != AREA_FORTRESS_5 && em->hp < 1000) {
                em->hp = 1000;
            }
            if ((em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) && Map_DayId(gScene.map) == MAP_TOWER_C && Area_DayId(em->area) != AREA_TOWER_0x7C && em->hp < 1000) {
                em->hp = 1000;
            }
            if (em->hp <= 0) {
                em->scriptPending = 0;
                if (em->large != 0) {
                    if (gScene.area == em->area) {
                        em->hp = 0;
                        em->combatPlayers = 0;
                        return 1;
                    }
                    em->hp = 1;
                } else {
                    em->hp = 0;
                    em->combatPlayers = 0;
                    return 1;
                }
            }
            Em_SetCombat(em, 1, gEmCombatInit[em->type]);
        }
        if (Em_IsHeld(em) != 1 && em->lastAttackKind == 0x13 && Em_HasKindTimer(em) == 1) {
            result = 0x12;
            goto end;
        }
        if ((s8) em->holdTimer == 0) {
            if (em->lastAttackKind == 9) {
                result = 3;
                goto end;
            }
            if (em->lastAttackKind == 6) {
                result = 4;
                goto end;
            }
        }
    }
    if ((status = Em_PickStatus(em)) != 0 || em->flashed != 0) {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
    if (status == 1) {
        result = 5;
    } else if (status == 4) {
        result = 6;
    } else if (status == 2 && Em_IsHeld(em) == 1 && Em_NearLowHp(em) == 1) {
        result = 7;
    } else {
        if (Em_IsHeld(em) == 0 || Em_NearLowHp(em) == 0) {
            em->statusFlags &= 0xFE;
        }
        if (status == 3) {
            result = 8;
        } else if (status == 5) {
            result = 0x11;
        } else if (em->flashed != 0) {
            em->flashed = 0;
            Em_ApplyFlash(em);
            result = 9;
        } else if (em->hitReceived != 0) {
            if (em->hitReactMode == EM_HIT_REACT_WAKE) {
                result = 0xA;
            } else {
                if (em->hitReactMode != EM_HIT_REACT_STATUS && Em_IsHeld(em) == 0) {
                    em->hitReactMode = EM_HIT_REACT_FLINCH;
                }
                if (em->part8Down != 0) {
                    result = 0xB;
                } else if (em->stateGroup == 4) {
                    return 0;
                } else if (em->lastAttackKind == 5) {
                    if (broke) {
                        result = 0x10;
                    } else {
                        result = 0xF;
                    }
                } else if (broke) {
                    result = 0xC;
                } else {
                    if (em->hitReactMode == EM_HIT_REACT_NO_FLINCH) {
                        result = 0xD;
                    } else {
                        if (Em_IsHeld(em) == 1) {
                            return 0;
                        }
                        result = 0xE;
                        Em_SelectSightRow(em, 0);
                        Em_LoadReactionSet(em, 0);
                        if (*out == 0 || em->hitReactMode != EM_HIT_REACT_FLINCH) {
                            Em_RestartScript(em);
                            em->scriptPending = 1;
                        }
                    }
                }
            }
        } else {
            return 0;
        }
    }
end:
    if (result != 0xE) {
        em->scriptPending = 0;
        Em_RestartScript(em);
        Player_ClearFlag((Player *)em, PLF_NO_ROOT_MOVE);
    }
    Em_CheckKindRequest(em);
    return result;
}

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
 * area sequence off, destArea and prevArea 0xFFFF (none), the goal cleared.
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
    *(u16 *) &em->destArea = 0xFFFF;
    em->prevArea = 0xFFFF;
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
                    w[0x44] = value;
                }
            }
            break;
        case 7:
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                w = &em->work;
                if (em->remote == 0) {
                    w[0x45] = value;
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
 * unk882 = 0xFFFF); 0 clears all of them. w is set as in EmScript_SetVar.
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
                        w[0x44] = 0;
                    }
                }
                break;
            case 7:
                if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                    w = &em->work;
                    if (em->remote == 0) {
                        w[0x45] = 0;
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
                em->prevArea = 0xFFFF;
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
 * scriptPlayer), or to no player (0xFFFF) when scriptPlayer is -1.
 */
u8 *EmScript_GoalScriptPlayer(Em *em, u8 *script)
{
    s8 player;

    em->goalKind = 1;
    em->goalArg = 0;
    player = (s8) em->scriptPlayer;
    if (player == -1) {
        *(u16 *) &em->goalNo = 0xFFFF;
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
            if (angleDelta > 0x8000) {
                angleDelta = 0x10000 - angleDelta;
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
    em->prevArea = 0xFFFF;
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
 * ahead, 0x10000 a turn): the first case whose byte * 0x100 is not below it
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
                        if (kind == 0x4A) {
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
                if ((&em->work)[0x44] == value) {
                    differs = 0;
                } else {
                    differs = 1;
                }
            }
            break;
        case 7:
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                if ((&em->work)[0x45] == value) {
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
        if (angleDelta > 0x8000) {
            angleDelta = 0x10000 - angleDelta;
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
        if (angleDelta > 0x8000) {
            angleDelta = 0x10000 - angleDelta;
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
 * Command EMOP_GOAL_PLAYER_FLOOR_SPOT, one byte: with a picked player, the goal is goalKind 0xB, a
 * point by that player's floorAttr (Em_ComputeGoal) with preset byte; with
 * none, goalKind 1 with no player.
 */
u8 *EmScript_GoalPlayerFloorSpot(Em *em, u8 *script)
{
    s32 preset = *script;

    if ((s8) em->scriptPlayer == -1) {
        em->goalKind = 1;
        em->goalArg = 0;
        *(u16 *) &em->goalNo = 0xFFFF;
    } else {
        em->goalKind = 0xB;
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
            sel = 0xFF;
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
                        sel = 0xFF;
                    }
                }
            }
        }
        if (sel == 0xFF) {
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
            sel = 0xFF;
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
                            sel = 0xFF;
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
 * the goal (goalKind 1; 0xFFFF with no link) and scriptPlayer.
 */
u8 *EmScript_GoalLinkTarget(Em *em, u8 *script)
{
    Em *link = em->link;

    em->goalKind = 1;
    em->goalArg = 0;
    if (link == NULL) {
        *(u16 *) &em->goalNo = 0xFFFF;
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
 * 1 or 0xB), a player is picked and its floorAttr equals the byte.
 */
u8 *EmScript_IfScriptPlayerFloorAttr(Em *em, u8 *script)
{
    u8 attr;
    s8 k;

    switch (*script++) {
    case EMBLK_OPEN:
        attr = *script++;
        if (em->curGoalKind == 0xB || em->curGoalKind == 1) {
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
            sel = 0xFF;
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
 * for quest 0xBBC and sets it for every other quest; with it set, the area's
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

struct EmIdPair;
void Em_SetAreaTimer2(Em *em, struct EmIdPair *pair);
u8 *EmScript_SetAreaTimer2(Em *em, u8 *script)
{
    Em_SetAreaTimer2(em, (struct EmIdPair *)gEmAreaTimer2Values[em->type]);
    return script;
}

//---
/*
 * Switch op EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR on the floor under the target player (scriptPlayer). After
 * the sub-op byte come two unused bytes. Block 1 follows the header, blocks 2
 * and 3 follow the markers EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR 2 and EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR 3, and EMOP_SWITCH_SCRIPT_PLAYER_FLOOR_ATTR 0xFF ends the switch.
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
 * whose running total passes it runs (weight 0 never, 0xFF always). A branch
 * that reaches the next marker skips to the closing sub-byte 0xFF.
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
            if (w == 0xFF) {
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
 * 0xF + k; the first such jump after an EMOP_CALL_SCRIPT call remembers the return position
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
    pos = em->script[k + 0xF][em->scriptCallEntry];
    em->scriptPos = pos;
    return pos;
}

/*
 * Command EMOP_SET_GOAL: sets the goal from the bytes (goalKind, goalArg, goalNo).
 * Kind 3, an area, reads a two-byte area id (Area_IdForCurrentTime) into
 * nextArea and clears destArea; kind 0xA becomes kind 3 toward the picked
 * player's area; kind 0xD makes the first other large monster in the area the
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
        *(u16 *) &em->destArea = 0xFFFF;
        break;
    case 0xA:
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
        *(u16 *) &em->destArea = 0xFFFF;
        break;
    case 0xD:
        em->goalKind = 0xD;
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
            *(u16 *) &em->goalNo = 0xFFFF;
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

/* Command EMOP_SWITCH_TARGET_DIST (a multi-way branch on distance, 0..5 then 0xFF): picks the first of the reaction
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
        } else if (em->curGoalKind == 0xD) {
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
 * EMEND_ROUTE_STEP step along the route of EmScript_StartRoute; 0xF5 to 0xF7
 * and 0xFC restart the main script with scriptFlags cleared; the thread ends
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
    case 0xF5:
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 0xF6:
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 0xF7:
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
    case 0xFC:
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
    em->rotY = Math_AngleXZ((f32 *) &em->posX, (f32 *) (gEmGoalPoints[em->type] + (*script * 0xC))) & 0xFFFF;
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
 * the block skipped. end is 2 for the tests, 3 for the switches and 0xFF for
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
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            script += 2;
            break;
        case 0xFF:
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
        case 0xFF:
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
 * weight 0 is skipped, 0xFF taken at once, else the first entry whose running
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
        if (w == 0xFF) {
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

/* The third part's data. */
ArmorCombo gArmorCombos[14] = {
#include "game/008C3DE0.inc"
};
s32 gEmNoticeThresholds[3] = {
#include "game/008C3E38.inc"
};
