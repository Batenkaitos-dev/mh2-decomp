/*
 * Game overlay code at 0x70A290-0x72E450, one of Capcom's units: the player's
 * states, actions and per-frame update (Player_Update), and monster helpers
 * (scripts, status, routes). Player_PickStatusState is static: its one caller,
 * Player_Update, keeps a0 across the call (docs/mwcc-matching.md, "Arguments kept
 * or reloaded across calls (static callees)"). Merged in round 20 from
 * unk_70A290.c, unk_716360.c and unk_722400.c; the part from 0x722400 on is
 * built at optimization level 4 ("The level can change inside one of
 * Capcom's units").
 */
#include "common.h"
#include "sprite.h"
#include "syssettings.h"
#include "scene.h"
#include "quest.h"
#include "scevent.h"
#include "em.h"
#include "player.h"
#include "item.h"
/* prototypes of callees (round 13) */
s32 ScEvent_LoadDemoCams(ScEventList *h);
s32 ScEvent_LoadCamera(ScEventList *h);
s32 ScEvent_Set(ScEvent *e, ScEventHead *rec);
s32 Player_TakeOfferedItem(Player *p);
s32 Quest_IsVersus(void);
s32 SpecialMine_IsFree(Player *unused0);

/* sub_main 0x5B1DD0: per player (Em +0xC), 11 records of 12 bytes, ended by flags 0: flags, and a pointer
 * to a position (x at [0], z at [2]). Player_TurnAlongWall and Player_TurnAlongWallSide turn the player to face a quarter
 * turn off the direction of the first one. Player +0x698 (0xE0000007) also picks the voice with it
 * (sub_main Motion_VoiceAtFrame). */
typedef struct NoiseSrc {
    u16 flags;
    u16 pad;
    f32 *pos;
    s32 unk8;
} NoiseSrc;
extern NoiseSrc gEmWallContacts[][11];

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
s16 Player_GetAreaAmbModel(Player *p);
s32 Pouch_HasItem28(Player *p);
s32 Player_LocalAreaOk_Unk(void);

/* Prototypes shared by the parts of this unit. */
void Effect06_SpawnAt(void *obj, s32 type, s32 bone, f32 *pos, f32 scale); /* unit's own declaration: with the definition's void (void *obj, s16 type, u8 bone, f32 *pos, f32 scale) (sub_main/unk_3CA790.c), Em_UpdateStatus 287 of 385 words differ */
u8 *EmScript_NextCmd(Em *em, u8 *p);
s32 Em_StartScript(Em *em);
void Em_StartTurn(Em *em, EmTurnParams *t, s32 back);
s32 Obj_InCurrentArea(void *obj);
s32 Obj_SameArea(void *obj, Player *b);
void Player_ClearFlag(void *obj, u32 flag);
void Player_FaceStick(Player *em);
s32 Player_InSceneEvent(void);
void Player_SetFlag(void *obj, u32 flag);
void Player_StartAct(Player *p);
void Player_StateMove(Player *em, s32 variant);
void Player_StateWallHold(Player *em, s32 arg1);
void Player_StateWallClimbUp(Player *player, s32 arg1);
u32 Player_TestFlag(void *obj, u32 flag);
void Player_HoldSpecialInput(Player *em, u8 mode);
void Player_UpdateMotionTime(Player *p, s16 step);
u8 Pouch_Add(Player *p, u16 item, s16 n);
AreaRecord *Stage_GetAreaRecord(s32 area); /* unit's own declaration: the definition (sub_main) is AreaRecord *(u16 x); u16 here adds masks in two functions */ /* unit's own declaration: with the definition's AreaRecord *(u16 x) (sub_main/stage_data.c), EmScript_Op1A 246 of 276 words, Em_PickRoutePoint 3 of 226 words differ */
s32 Player_TurnAlongWallSide(Player *em, s16 side);
s32 func_00723BC0(Em *em);
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
s32 Player_HasMealEffect(Player *pl, s16 kind);
void Player_GuardInput(Player *p);
s32 Player_Damage(Player *p, s16 damage, s16 lose);
s16 Player_CountSpecialItems(Player *p);
s16 Area_Temperature(u16 area);
s32 Player_NearEm40(Player *p);
/* unit's own declaration: with the definition's void (Player *p, s32 flag) (game/unk_88E760.c), Player_UpdateEnvironment: 2 of 405 words differ */
void StageObj_K29_TickType6(Player *p, u8 flag);
void Eff03_Create(void *obj, s32 kind, s8 arg2);
void Eff28_Create(Em *em, u8 kind, u8 n);
u16 Rand_Next(s32 stream);
void Player_AddSpawnOffset(Player *p, f32 *pos, u16 angle);
extern f32 gStartPos[][3];
extern f32 D_43F0A4[][3];
extern f32 D_43F0A8[][3];
extern u16 gStartAngle[];
extern ScEventRec ScEvent_Seq61A9;
extern ScEventRec ScEvent_Seq3EB;
extern ScEventRec ScEvent_Seq3ED;
extern ScEventRec ScEvent_Seq1389;
extern ScEventRec ScEvent_Seq3F4;
extern ScEventRec ScEvent_Seq3F8;
extern ScEventRec ScEvent_Seq138A;
extern ScEventRec ScEvent_Seq138C;
extern ScEventRec ScEvent_Seq138D;
extern ScEventRec ScEvent_Seq138F;
extern ScEventRec ScEvent_Seq1771;
extern ScEventRec ScEvent_Seq438;
extern ScEventRec ScEvent_Seq431;
extern ScEventRec ScEvent_Seq424;
extern ScEventRec ScEvent_Seq42F;
extern ScEventRec ScEvent_Seq3F0;
extern ScEventRec ScEvent_Seq422;
extern ScEventRec ScEvent_Seq406;
extern ScEventRec ScEvent_Seq42C;
extern ScEventRec ScEvent_Seq1390;
extern ScEventRec ScEvent_Seq436;
extern ScEventRec ScEvent_Seq426;
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
s32 Player_NearEmKind10(void *obj);
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
void Quest_Start(void);
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
void MsgEvent_Open(void);
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
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter s32 (the definition has s16); with the definition's type Player_StateSpitRoast by 418 of 469 words differ */
void Player_SetMoveMode(Player *, u16 mode, s16 flag1, s16 flag2);
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
void StageObj_K0C_CreateAtPlayer(Player *, s32 k);
void Dmcam_Init(void *ptinfo, s32 keep);
u8 *Dmcam_Load(s32 idx, s32 alt);
s32 Motion_ReachedFrame(void *, s32, f32);
s32 CamCtl_IsTurningBehind(void);
void CamShake_ReqA(void *, s32 i);
s32 Game_IsOnline(void);
s32 Quest_PhaseIsOver(s32);
void Vib_StartLocal(void *port, s32 i);
u16 Area_BaseId(u16);
u8 Notice_Push(u8, s16, s16);
void Player_BeginState(Player *p, s32 action, s32 a2, s32 a3);
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
void Effect02_SpawnOnOwner(void *owner, u8 type, u8 sub, f32 scale);
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

/* Flight steering (em_chameleos calls it with EmChameleos_FlyParam): banks (unkA8) toward the angle unk98E
 * and turns the yaw (unkA4) by the bank, keeps the height over the ground (Stage_GetAreaRecord) and
 * picks the flight motions from p by the bank. Flag bits skip parts: 2 the bank-scaled turn,
 * 4 the height, 8 the motions, 0x10 the roll (unkA0) return. */
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p)
{
    AreaRecord *ground;
    s32 k;
    s32 sel;
    s32 d;
    s32 a;
    s32 step;
    s32 hi;
    s32 lo;
    f32 f;
    f32 h;

    ground = Stage_GetAreaRecord(em->area);
    em->flightTurn = em->aimAngle;
    k = !(flags & 0x20);
    d = em->flightTurn;
    if (d >= 0x8000) {
        d = 0x10000 - d;
    }
    switch (k) {
    case 0:
        if (d <= p->lim0) {
            sel = p->a0;
        } else if (d <= p->lim1) {
            sel = p->a1;
        } else if (d <= p->lim2) {
            sel = p->a2;
        } else if (d <= p->lim3) {
            sel = p->a3;
        }
        if (em->velZ <= 130.0f) {
            break;
        }
    case 1:
        if (d <= p->lim0) {
            sel = p->b0;
        } else if (d <= p->lim1) {
            sel = p->b1;
        } else if (d <= p->lim2) {
            sel = p->b2;
        } else if (d <= p->lim3) {
            sel = p->b3;
        }
        break;
    }
    if (em->flightTurn != 0) {
        if (em->flightTurn <= 0x8000) {
            if (em->rotZ <= 0x4000) {
                em->rotZ = em->rotZ - p->pitch * 2;
            } else if (em->rotZ <= sel || em->rotZ > 0x10000 - sel) {
                em->rotZ = em->rotZ - p->pitch;
            } else if (em->rotZ != 0x10000 - sel) {
                em->rotZ = em->rotZ + p->pitch;
            }
        } else {
            if (em->rotZ >= 0xC000) {
                em->rotZ = em->rotZ + p->pitch * 2;
            } else if (em->rotZ < sel || em->rotZ >= 0x10000 - sel) {
                em->rotZ = em->rotZ + p->pitch;
            } else if (em->rotZ != sel) {
                em->rotZ = em->rotZ - p->pitch;
            }
        }
    } else if (em->rotZ != 0) {
        if (em->rotZ <= sel) {
            em->rotZ = em->rotZ - p->pitch;
        } else if (em->rotZ >= 0x10000 - sel) {
            em->rotZ = em->rotZ + p->pitch;
        }
    }
    if (!(flags & 2)) {
        a = em->rotZ;
        a = a < 0x8000 ? a : (u16) (0x10000 - a);
        if (a <= 0x1000) {
            step = p->turn;
        } else {
            step = p->turn + (a - 0x1000) / 16;
        }
    } else {
        step = p->turn;
    }
    if (em->rotZ != 0) {
        if (flags & 0x100) {
            hi = 0x10000;
            lo = 0;
        } else {
            hi = 0xF000;
            lo = 0x1000;
        }
        if (em->rotZ <= hi && em->rotZ >= 0xC000) {
            if (em->flightTurn <= step) {
                em->rotY = em->rotY + em->flightTurn;
                em->flightTurn = 0;
            } else {
                em->rotY = em->rotY + step;
                em->flightTurn = em->flightTurn - step;
            }
        } else if (em->rotZ >= lo && em->rotZ <= 0x4000) {
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
        a = em->rotZ;
        if (a >= 0x8000) {
            a = 0x10000 - a;
        }
        if (a < 0x1000) {
            f = 0.1f;
        } else {
            f = 6.2831855f * a / 65536.0f;
        }
        if (!(flags & 4)) {
            em->posY = em->posY - f;
            if (em->velZ >= 50.0f && f != 0.1f) {
                em->velZ = em->velZ - f;
            }
        }
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        }
    }
    if (!(flags & 4)) {
        if (flags & 0x400) {
            h = gAreaGroundY[em->area] - 2000.0f;
            if (h < ground->bottom) {
                h = ground->bottom;
            }
        } else {
            h = ground->bottom;
        }
        if (flags & 0x80) {
            if (p->mot[0] + 1000 == em->motionId[0]) {
                if (em->motion[0].playing == 0) {
                    Em_SetMotion(em, p->mot[1], 0, 0);
                }
                if (1000.0f + h >= em->posY) {
                    em->posY = em->posY + 5.0f;
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                } else if (1500.0f + h >= em->posY) {
                    em->posY = em->posY + 3.0f;
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else if (2000.0f + h >= em->posY) {
                    em->posY = em->posY + 2.0f;
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else {
                    if (2100.0f + h >= em->posY) {
                        em->posY = em->posY + 1.0f;
                    }
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                }
            } else if (2000.0f + h >= em->posY || em->velZ <= p->speedMin) {
                Em_SetMotion(em, p->mot[0], 0, 0);
            }
        } else if (flags & 0x200) {
            if (p->mot[0] + 1000 == em->motionId[0]) {
                if (em->motion[0].playing == 0) {
                    Em_SetMotion(em, p->mot[1], 0, 0);
                }
                if (em->goal[1] >= em->posY) {
                    em->posY = em->posY + 5.0f;
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                } else if (1500.0f + h >= em->posY) {
                    em->posY = em->posY + 3.0f;
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else if (2000.0f + h >= em->posY) {
                    em->posY = em->posY + 2.0f;
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 0.5f;
                    }
                } else {
                    if (2100.0f + h >= em->posY) {
                        em->posY = em->posY + 1.0f;
                    }
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                }
            } else if (2000.0f + h >= em->posY || em->velZ <= p->speedMin) {
                Em_SetMotion(em, p->mot[0], 0, 0);
            }
        } else {
            if (p->mot[0] + 1000 == em->motionId[0]) {
                if (em->motion[0].playing == 0) {
                    Em_SetMotion(em, p->mot[1], 0, 0);
                }
                if (em->goal[1] >= em->posY) {
                    em->posY = em->posY + 5.0f;
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                } else {
                    if (em->goal[1] >= em->posY) {
                        em->posY = em->posY + 1.0f;
                    }
                    if (em->velZ < p->speedMax) {
                        em->velZ = em->velZ + 1.0f;
                    }
                }
            } else if (em->goal[1] >= em->posY || em->velZ <= p->speedMin) {
                Em_SetMotion(em, p->mot[0], 0, 0);
            }
        }
    }
    if (!(flags & 8)) {
        if (p->mot[0] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
            a = em->rotZ;
            if (a < 0x8000) {
                if (a > p->up0) {
                    if (p->mot[2] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                        Em_SetMotion(em, p->mot[2], 0, 0);
                    }
                } else if (a > p->up1) {
                    if (p->mot[3] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                        Em_SetMotion(em, p->mot[3], 0, 0);
                    }
                } else {
                    if (p->mot[1] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                        Em_SetMotion(em, p->mot[1], 0, 0);
                    }
                }
            } else if (a < p->down0) {
                if (p->mot[4] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                    Em_SetMotion(em, p->mot[4], 0, 0);
                }
            } else if (a < p->down1) {
                if (p->mot[5] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                    Em_SetMotion(em, p->mot[5], 0, 0);
                }
            } else {
                if (p->mot[1] + 1000 != em->motionId[0] && em->motion[0].playing == 0) {
                    Em_SetMotion(em, p->mot[1], 0, 0);
                }
            }
        }
    }
    if (!(flags & 0x10)) {
        if (em->rotX != 0) {
            if (em->rotX < 0x8000) {
                em->rotX = em->rotX - p->roll;
            } else {
                em->rotX = em->rotX + p->roll;
            }
            em->rotX = (u16) em->rotX;
        }
    }
}

s32 EmList_FindKindNear(Em *em, u16 kind, s32 mode, f32 dist)
{
    s32 i;
    Em *m;

    for (i = 0, m = gEms; i < 20; i++, m++) {
        if (m->active != 0) {
            if (mode == 1) {
                if (Em_KindMatches(m->type, kind) == 0) {
                    continue;
                }
                if (kind != m->type) {
                    continue;
                }
            }
            if ((u8)Obj_SameArea(em, (Player *)m) != 0 && m->stance != 2 && Vec_DistanceXZ(&em->posX, &m->posX) <= dist) {
                return 1;
            }
        }
    }
    return 0;
}

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

void Em_SetTargetAngle(Em *em)
{
    f32 pos[3];
    f32 pos2[3];
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
        Area_ToWorld((s32) pl->area, pos, pos2);
        em->targetAngle = (u16) Math_AngleXZ(em->mapPos, pos2);
    }
}

s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight)
{
    u32 d;

    switch (em->turnStep) {
    case 0:
        em->turnStep++;
        em->lookAtTarget = 1;
        em->aimAngle = em->targetAngle;
        d = (u16) (em->targetAngle - em->rotY);
        if ((u16) (em->targetAngle + 0x200) <= 0x400) {
            return 1;
        }
        if ((u16) d <= range || (u16) d >= 0x10000 - range) {
            Player_SetFlag((Player *) em, 0x20000);
            Em_SetMotion(em, motFront, 0, 0);
        } else if ((u16) d >= 0x8000) {
            Em_SetMotion(em, motLeft, 0, 0);
        } else {
            Em_SetMotion(em, motRight, 0, 0);
        }
        break;
    case 1:
        d = (u16) (em->aimAngle - (u16) em->rotY);
        if (em->motion[0].playing == 0 || EmMotion_ReachedFrame(em, 0, 110.0f)) {
            if (d <= range || d >= 0x10000 - range) {
                Player_SetFlag((Player *) em, 0x20000);
                Em_SetMotion(em, motFront, 0, 0);
            } else {
                Player_ClearFlag((Player *) em, 0x20000);
                if (d >= 0x8000) {
                    Em_SetMotion(em, motLeft, 0, 0);
                } else {
                    Em_SetMotion(em, motRight, 0, 0);
                }
            }
        } else if (((d + 0x200) & 0xFFFF) < 0x400) {
            Player_ClearFlag((Player *) em, 0x20000);
            em->rotY = em->aimAngle;
            em->lookAtTarget = 0;
            return 1;
        } else if (d < 0x8000) {
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
    f32 a;
    f32 b;

    a = GmEm_ScaleBySize(em, x);
    b = GmEm_ScaleBySize(em, y);
    if (em->curGoalKind == 0) {
        em->hasGoal = 0;
        em->goalDist = a;
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
            em->goalDist = a;
            return;
        }
        em->goalDist -= b;
        if (em->goalDist <= 0.0f) {
            em->goalDist = 0.0f;
        }
    }
    if ((flags & 2) != 2 && em->curGoalKind == 7) {
        em->goalDist -= b;
        if (em->goalDist <= 0.0f) {
            em->goalDist = 0.0f;
        }
    }
}

s32 Em_StepToGoal(Em *em, u16 step, u8 flags)
{
    f32 v[4];
    f32 d;

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
            Em_MotionDelta(em, v);
            if ((flags & 4) != 4) {
                em->goalDist = em->goalDist - v[2];
            } else {
                d = v[2];
                if (d >= 0.0f) {
                    em->goalDist -= d;
                } else {
                    em->goalDist += d;
                }
            }
        }
    }
    return em->goalDist <= 0.0f;
}

u8 Em_TurnToGoal(Em *em, u16 step, u8 flags)
{
    f32 pos[4];
    f32 pos2[4];
    u8 done = 0;
    s8 i;
    u16 ang;
    u16 d;

    if ((flags & 1) && em->curGoalKind == 0) {
        return 1;
    }
    if (flags & 2) {
        i = em->targetNo;
        if (i == -1) {
            return 1;
        }
        Em_GetTargetPos(em, i, pos);
        Area_ToWorld((s32) ((Player *) &gPlayers[i])->area, pos, pos2);
        ang = Math_AngleXZ(em->mapPos, pos2);
    } else {
        ang = Math_AngleXZ(&em->posX, em->goal);
    }
    if (flags & 4) {
        ang += 0x8000;
    }
    d = ang - em->rotY;
    if (d <= 0x8000) {
        if (d < step) {
            em->rotY += d;
            done = 1;
        } else {
            em->rotY += step;
        }
    } else if (0x10000 - step < d) {
        em->rotY += d;
        done = 1;
    } else {
        em->rotY -= step;
    }
    em->rotY = (u16) em->rotY;
    return done;
}

void Em_StartTurn(Em *em, EmTurnParams *t, s32 back)
{
    u32 d;

    if (back == 0) {
        em->aimAngle = Math_AngleXZ((f32 *) &em->posX, em->goal);
    } else {
        em->aimAngle = (u16) Math_AngleXZ((f32 *) &em->posX, em->goal) - 0x8000;
    }
    d = (u16) (em->aimAngle - em->rotY);
    if (t->range != 0 && (d <= t->range || d >= 0x10000 - t->range)) {
        Player_SetFlag((Player *) em, 0x20000);
        Em_SetMotion(em, t->motFront, 0, 0);
    } else if (d >= 0x8000) {
        Em_SetMotion(em, t->motLeft, 0, 0);
    } else {
        Em_SetMotion(em, t->motRight, 0, 0);
    }
}

void Em_StartTurnToTarget(Em *em, EmTurnParams *t)
{
    Em_StartTurn(em, t, 0);
}

void Em_StartTurnAway(Em *em, EmTurnParams *t)
{
    Em_StartTurn(em, t, 0x1);
}

s32 Em_TurnStep(Em *em, EmTurnParams *w)
{
    u32 step;
    u32 d;
    s32 r;

    if (em->motion[0].blend != 0) {
        return 0;
    }
    if (w->from == 0.0f && w->to == 0.0f) {
        step = (em->speedBonus + 100) * (em->motionSpeed * (w->angle / (em->motion[0].frameCount / 2.0f))) / 100.0f;
    } else {
        step = (em->speedBonus + 100) * (em->motionSpeed * (w->angle / ((w->to - w->from) / 2.0f))) / 100.0f;
    }
    d = (u16) (em->aimAngle - (u16) em->rotY);
    if (w->frame <= 0.0f) {
        r = 0;
    } else {
        r = EmMotion_ReachedFrame(em, 0, w->frame);
    }
    if (em->motion[0].playing == 0 || r != 0) {
        if (((d + step) & 0xFFFF) < step * 2) {
            return 1;
        }
        if (w->range != 0 && (d <= w->range || d >= 0x10000 - w->range)) {
            Player_SetFlag((Player *) em, 0x20000);
            Em_SetMotion(em, w->motFront, 0, 0);
        } else {
            Player_ClearFlag((Player *) em, 0x20000);
            if (d >= 0x8000) {
                Em_SetMotion(em, w->motLeft, 0, 0);
            } else {
                Em_SetMotion(em, w->motRight, 0, 0);
            }
        }
    } else if ((w->from == 0.0f && w->to == 0.0f) || EmMotion_InFrameRange(em, 0, w->from, w->to) == 1) {
        if (((d + step) & 0xFFFF) < step * 2) {
            em->rotY = em->aimAngle;
        } else if (d < 0x8000) {
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

void ScEvent_Init(void)
{
    ScEventList *h = &gSceneEvents;

    gScene.eventRunning = 0;
    gSceneEvents.active = 0;
    *(s32 *) &gSceneEvents.ev[2] = 0;
    *(s32 *) &gSceneEvents.ev[1] = 0;
    *(s32 *) &gSceneEvents.ev[0] = 0;
    gSceneEvents.ended = 0;
    if (ScEvent_LoadDemoCams(h) & 0xFFFF) {
        return;
    }
    Dmcam_Init(gSysSettings.readBuf, 1);
    ScEvent_LoadCamera(h);
    switch (gScene.questId) {
    case 0x61A9:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq61A9.head);
        gScene.eventRunning = 1;
        break;
    case 0x12:
    case 0x3EA:
    case 0x3EB:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq3EB.head);
        break;
    case 0x3EC:
    case 0x3ED:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq3ED.head);
        break;
    case 0x400:
    case 0x1389:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq1389.head);
        break;
    case 0x3F3:
    case 0x3F4:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq3F4.head);
        break;
    case 0x3F6:
    case 0x3F8:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq3F8.head);
        break;
    case 0x138A:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq138A.head);
        break;
    case 0x138F:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq138F.head);
        break;
    case 0x138C:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq138C.head);
        break;
    case 0x138D:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq138D.head);
        break;
    case 0x1771:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq1771.head);
        break;
    case 0x437:
    case 0x438:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq438.head);
        break;
    case 0x430:
    case 0x431:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq431.head);
        break;
    case 0x423:
    case 0x424:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq424.head);
        break;
    case 0x42E:
    case 0x42F:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq42F.head);
        break;
    case 0x3EE:
    case 0x3F0:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq3F0.head);
        break;
    case 0x422:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq422.head);
        break;
    case 0x406:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq406.head);
        break;
    case 0x42C:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq42C.head);
        break;
    case 0x1390:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq1390.head);
        break;
    case 0x434:
    case 0x436:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq436.head);
        break;
    case 0x425:
    case 0x426:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq426.head);
        break;
    case 1:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq1.head);
        break;
    case 3:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq3.head);
        ScEvent_Set(&h->ev[1], &ScEvent_Seq3B.head);
        break;
    case 5:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq5.head);
        break;
    case 0x11:
        ScEvent_Set(&h->ev[0], &ScEvent_Seq11.head);
        break;
    }
}

s32 ScEvent_Set(ScEvent *e, ScEventHead *rec)
{
    u16 n;

    n = rec->doneFlag;
    if ((n != 0) && (Save_TestEventFlag((s32) n) == 1)) {
        return -1;
    }
    Dmcam_Load((s32) rec->camera, 0);
    e->on = 1;
    e->check = rec->check;
    e->run = rec->run;
    e->data = rec;
    return 0;
}

s32 ScEvent_LoadDemoCams(ScEventList *h)
{
    u8 ids[4];
    s32 i;
    s32 n;

    Save_ClearEventFlag(0xAD);
    Save_ClearEventFlag(0xB5);
    Save_ClearEventFlag(0xB0);
    Save_ClearEventFlag(0xB1);
    Save_ClearEventFlag(0xB2);
    Save_ClearEventFlag(0xAE);
    Save_ClearEventFlag(0xAF);
    Dmcam_Init(gSysSettings.readBuf, 0);
    n = Quest_GetStageEmKinds(ids);
    for (i = 0; i < n; i++) {
        switch (ids[i]) {
        case 7:
            ScEvent_Set(&h->ev[0], &ScEvent_DemoCam7.head);
            Dmcam_Load(0x7C, 0);
            Dmcam_Load(0x86, 0);
            return 7;
        case 0x32:
            ScEvent_Set(&h->ev[0], &ScEvent_DemoCam32.head);
            Dmcam_Load(0x7C, 0);
            Dmcam_Load(0x86, 0);
            return 0x32;
        case 2:
            ScEvent_Set(&h->ev[0], &ScEvent_DemoCam2.head);
            Dmcam_Load(0x83, 0);
            Dmcam_Load(0x89, 0);
            Dmcam_Load(0x8A, 0);
            return 2;
        case 0x24:
            ScEvent_Set(&h->ev[0], &ScEvent_DemoCam24.head);
            Dmcam_Load(0x83, 0);
            Dmcam_Load(0x89, 0);
            Dmcam_Load(0x8A, 0);
            return 0x24;
        case 0x47:
            ScEvent_Set(&h->ev[0], &ScEvent_DemoCam47.head);
            Dmcam_Load(0x83, 0);
            Dmcam_Load(0x89, 0);
            Dmcam_Load(0x8A, 0);
            return 0x47;
        case 0x37:
            ScEvent_Set(&h->ev[0], &ScEvent_DemoCam37.head);
            Dmcam_Load(0x7E, 0);
            Dmcam_Load(0x87, 0);
            return 0x37;
        case 0x3A:
            ScEvent_Set(&h->ev[0], &ScEvent_DemoCam3A.head);
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

s32 ScEvent_LoadCamera(ScEventList *h)
{
    s32 base;

    if (Game_IsOnline() != 1) {
        Dmcam_Init(gSysSettings.readBuf, 1);
        base = Area_BaseId((u16) gScene.map) & 0xFFFF;
        switch (base) {
        case 2:
            ScEvent_Set(&h->ev[2], &ScEvent_Cam2.head);
            break;
        case 3:
            ScEvent_Set(&h->ev[2], &ScEvent_Cam3.head);
            break;
        case 4:
            ScEvent_Set(&h->ev[2], &ScEvent_Cam4.head);
            break;
        case 5:
            ScEvent_Set(&h->ev[2], &ScEvent_Cam5.head);
            break;
        case 6:
            ScEvent_Set(&h->ev[2], &ScEvent_Cam6.head);
            break;
        case 11:
            ScEvent_Set(&h->ev[2], &ScEvent_CamB.head);
            break;
        case 13:
        case 14:
        case 15:
            ScEvent_Set(&h->ev[2], &ScEvent_CamF.head);
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
    ScEvent *e;

    gSceneEvents.ended = 0;
    switch (gSceneEvents.active) {
    case 1:
        if (w->ev[w->cur].run(&w->ev[w->cur]) == 0) {
            break;
        }
        gScene.eventRunning = 0;
        w->active = 0;
    case 0:
        for (e = w->ev, i = 0; i < 3; i++, e++) {
            if (e->on != 0 && e->check(e) > 0) {
                w->active = 1;
                w->cur = i;
                e->run(e);
                return;
            }
        }
        MsgEvent_Open();
        break;
    }
}

s32 ScEvent_CheckDay(ScEvent *e)
{
    if (Save_TestEventFlag(e->data->doneFlag) == 1) {
        *(s32 *) e = 0;
        return -1;
    }
    return e->data->area == Area_DayId(gScene.area);
}

s32 ScEvent_RunA(ScEvent *ev)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ScEventList *d = &gSceneEvents;

    switch (ev->step) {
    case 0:
        gScene.eventRunning = 1;
        ScEvent_SetCamera(ev->data->camera, ev->data->camGroup);
        switch (ev->data->music) {
        case 4:
            ev->timer = 0xCD;
            break;
        case 5:
            ev->timer = 0xC0;
            break;
        case 6:
            ev->timer = 0xEF;
            break;
        case 7:
            ev->timer = 0xC4;
            break;
        case 8:
            ev->timer = 4;
            Stream_Pause(0, 1);
            break;
        case 9:
            ev->timer = 0x15E;
            Stream_Pause(0, 1);
            break;
        case 10:
            ev->timer = 0x350;
            Stream_Pause(0, 1);
            break;
        case 3:
            ev->timer = 0x10E;
            Stream_Pause(0, 1);
            break;
        case 2:
            ev->timer = 0x15E;
        case 1:
            Stream_Pause(0, 1);
            break;
        case 0:
            break;
        }
        if (ev->data->jingleDay != 0) {
            if (Area_IsDay(gScene.area) == 1) {
                Jingle_Play(pl, ev->data->jingleDay);
            } else {
                Jingle_Play(pl, ev->data->jingleNight);
            }
        }
        ev->step++;
        break;
    case 1:
        if (ev->timer != 0) {
            ev->timer--;
            if (ev->timer == 0) {
                switch (ev->data->music) {
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
            ev->step++;
            gScene.eventRunning = 0;
            Save_SetEventFlag(ev->data->doneFlag);
            switch (ev->data->music) {
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
            d->ended = 1;
        }
        break;
    case 2:
        *(s32 *) ev = 0;
        return 1;
    }
    return 0;
}

s32 ScEvent_CheckAlwaysA(ScEvent *e)
{
    return 1;
}

s32 ScEvent_RunVersusEnd(ScEvent *e)
{
    ScEventList *d = &gSceneEvents;
    s32 i;

    gQuest.inputLock = 90;
    for (i = 0; i < gScene.playerCount; i++) {
        gPlayers[i].noHitTimer = 2;
    }
    switch (e->step) {
    case 0:
        gScene.eventRunning = 1;
        e->step++;
        break;
    case 1:
        if (gQuest.versusDefeat & 3) {
            e->step++;
            FadeA_Start(6);
        }
        break;
    case 2:
        if (Versus_BothDefeated() == 1) {
            e->step++;
            gScene.eventRunning = 0;
            Effect2C_Spawn(90);
        }
        break;
    case 3:
        CamReq_Cancel();
        d->ended = 1;
        *(s32 *) e = 0;
        return 1;
    }
    return 0;
}

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

s32 ScEvent_CurId(void)
{
    ScEventList *p = &gSceneEvents;
    ScEvent *e;

    if (p->active == 0) {
        return -1;
    }
    e = &p->ev[p->cur];
    return e->data->doneFlag;
}

s32 ScEvent_CurFlag1(void)
{
    ScEventList *p = &gSceneEvents;
    ScEvent *e;

    if (p->active != 0) {
        e = &p->ev[p->cur];
        return (e->data->flags & 1) != 0;
    }
    return 0;
}

void ScEvent_StartFrom(ScEventHead *rec)
{
    ScEvent *d;

    gScene.eventRunning = 0;
    gSceneEvents.active = 0;
    *(s32 *) &gSceneEvents.ev[2] = 0;
    *(s32 *) &gSceneEvents.ev[1] = 0;
    *(s32 *) &gSceneEvents.ev[0] = 0;
    Dmcam_Init(gSysSettings.readBuf, 0);
    d = &gSceneEvents.ev[0];
    Dmcam_Load(rec->camera, 0);
    d->on = 1;
    d->check = rec->check;
    d->run = rec->run;
    d->data = rec;
}

s32 ScEvent_CheckAlwaysB(ScEvent *e)
{
    return 1;
}

s32 ScEvent_RunB(ScEvent *ev)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ScEventList *d = &gSceneEvents;

    switch (ev->step) {
    case 0:
        gScene.eventRunning = 1;
        ScEvent_SetCamera(ev->data->camera, ev->data->camGroup);
        switch (ev->data->music) {
        case 8:
            ev->timer = 4;
            Stream_Pause(0, 1);
            break;
        case 9:
            ev->timer = 350;
            Stream_Pause(0, 1);
            break;
        case 10:
            ev->timer = 900;
            Stream_Pause(0, 1);
            break;
        case 3:
            ev->timer = 270;
            Stream_Pause(0, 1);
            break;
        case 2:
            ev->timer = 350;
        case 1:
            Stream_Pause(0, 1);
            break;
        case 0:
            break;
        }
        if (ev->data->jingleDay != 0) {
            if (Area_IsDay(gScene.area) == 1) {
                Jingle_Play(pl, ev->data->jingleDay);
            } else {
                Jingle_Play(pl, ev->data->jingleNight);
            }
        }
        ev->step++;
        break;
    case 1:
        if (ev->timer != 0) {
            ev->timer--;
            if (ev->timer == 0) {
                switch (ev->data->music) {
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
            ev->step++;
            gScene.eventRunning = 0;
            CamReq_Cancel();
            d->ended = 1;
        }
        break;
    case 2:
        *(s32 *) ev = 0;
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

void Player_UpdateEnvironment(Player *pl)
{
    f32 v[3];
    s16 n;
    s16 k;
    u8 f;
    u16 iv;

    if (pl->stateGroup == PL_GROUP_FAINT) {
        return;
    }
    pl->inPoisonArea = 0;
    if (pl->poison > 0) {
        if (--pl->poisonTimer <= 0) {
            pl->poison--;
            if (Player_HasSkill(pl, 0xA) == 1) {
                k = 60;
            } else if (Player_HasSkill(pl, 0xC) == 1) {
                k = 15;
            } else {
                k = 30;
            }
            pl->poisonTimer = k;
            if (Player_IsLocal(pl) == 1) {
                v[0] = 0.0f;
                v[1] = 20.0f;
                v[2] = 0.0f;
                Effect06_SpawnAt(pl, 3, 0x14, v, 1.0f);
            }
            if (Player_Damage(pl, 1, 0) == 1) {
                return;
            }
        }
    }
    if (Player_IsLocal(pl) == 0 && (pl->statusIcons & 1)) {
        k = Player_HasSkill(pl, 0xA) ? 60 : 30;
        if (gScene.frame % k == 0) {
            v[0] = 0.0f;
            v[1] = 20.0f;
            v[2] = 0.0f;
            Effect06_SpawnAt(pl, 3, 0x14, v, 1.0f);
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
        if (k == 1 && (Player_HasSkill(pl, 0x85) == 1 || Player_HasSkill(pl, 0x86) == 1)) {
            break;
        }
        if (k == 3 && Player_HasSkill(pl, 0x86) == 1) {
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
            } else if (Player_HasSkill(pl, 0x85) == 1) {
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
    if (pl->action == 2) {
        return;
    }
    if (pl->noHitTimer > 0) {
        return;
    }
    f = pl->unk669;
    if (f == 0 && Player_NearEm40(pl) == 1) {
        f = 2;
    }
    StageObj_K29_TickType6(pl, f);
    switch (f) {
    case 0:
        break;
    case 1:
        if (Player_HasSkill(pl, 0xB) == 0) {
            pl->inPoisonArea = 1;
            if (pl->poison < 10) {
                pl->poison = 10;
            }
        }
        break;
    case 2:
        pl->inPoisonArea = 1;
        iv = 8;
        if (Player_HasSkill(pl, 0xBF) == 1) {
            iv = 12;
        } else if (Player_HasSkill(pl, 0xC0) == 1) {
            iv = 24;
        } else if (Player_HasSkill(pl, 0xC1) == 1) {
            iv = 6;
        } else if (Player_HasSkill(pl, 0xC2) == 1) {
            iv = 4;
        }
        if (gScene.frame % iv == 0) {
            if (Player_Damage(pl, 1, 0) == 1) {
                return;
            }
        }
        break;
    }
}

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

void Player_UpdateItemSelect(Player *pl)
{
    u16 b;

    if (Player_IsLocal(pl) != 0) {
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STUN) == 0 && (u32) (pl->stateGroup - 3) > 1 && Quest_PhaseIsOver(1) != 1) {
            if (pl->input.held2 & 4) {
                if (pl->itemSelect == 0) {
                    Sound_PlaySeFlat(0, 0x11, 0);
                }
                pl->itemSelect = 1;
                if (Player_PouchHasSelectable(pl) != 0) {
                    b = pl->input.pressed2;
                    if (b & 0x20) {
                        Sound_PlaySeFlat(0, 0x12, 0);
                        pl->pouchCursor = Pouch_NextSelectable(pl, pl->pouchCursor, 0);
                        pl->pouchHudFlags |= 2;
                    } else if (b & 0x200) {
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

static s32 Player_AmmoTypeChanged(Player *pl)
{
    if ((pl->isGunner != 1) && ((u16) pl->ammoSlot == 0xFF)) {
        return 0;
    }
    if (pl->input.held2 & 4) {
        return 0;
    }
    if ((pl->ammoMenuOpen == 0) && (pl->savedAmmoType != pl->ammoType) && (pl->weaponOut != 0)) {
        return 1;
    }
    return 0;
}

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
    if (pl->unk816 != 0) {
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
    if (pl->input.held2 & 4) {
        if (pl->input.pressed2 & 0x40) {
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
        } else if (pl->input.pressed2 & 0x100) {
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

u8 Player_PushNotice3(s32 arg0)
{
    return Notice_Push(1, 3, arg0);
}

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

s32 Player_GetStickYaw(Player *pl, s32 i)
{
    s32 yaw = Camera_GetYaw() & 0xFFFF;

    return ((&pl->input.stickAngle)[i] + yaw) & 0xFFFF;
}

void Player_FaceStick(Player *pl)
{
    if (CamCtl_IsTurningBehind() != 1) {
        *(s16 *)&pl->targetYaw = (s16) (pl->input.stickAngle + (Camera_GetYaw() & 0xFFFF));
    }
}

u8 Player_StickMoveLevel(Player *pl, s32 unused)
{
    u8 lvl = 0;
    u16 v;

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
    v = pl->input.stickTilt;
    if (v >= 120) {
        lvl = 3;
    } else if (v >= 85) {
        lvl = 1;
    } else if (v >= 40) {
        lvl = 1;
    }
    if (pl->action == 0 && pl->weaponOut == 0 && (pl->input.held & 0x10) && lvl != 0
        && (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH) == 0 || pl->dashDelay == 0)) {
        return 5;
    }
    return lvl;
}

void Player_UpdateInBattle(Player *pl)
{
    s16 i;
    u8 bit;
    Em *m;
    s32 t;

    bit = 1 << pl->index;
    pl->prevInBattle = pl->inBattle;
    pl->inBattle = 0;
    for (i = 0, m = gEms; i < 20; i++, m++) {
        if (m->area == pl->area && m->active != 0 && m->visible != 0 && m->startsBattle != 0 && m->stateGroup != 5
            && (m->noticed & bit) && m->combat == 1) {
            m->combatPlayers |= bit;
            pl->inBattle = 1;
            pl->target = m;
            pl->targetAngle = (u16) (Vec_Heading(&m->posX, pl->pos) + 0x4000);
            t = m->type;
            if (t == 0x1B || t == 0x1C || t == 0x1F) {
                pl->unk73A_combat = 1;
            }
        }
    }
}

static s32 Player_EnteredBattle(Em *em)
{
    u8 temp_v1;

    temp_v1 = em->inBattle;
    if (temp_v1 != 0) {
        if ((temp_v1 != em->prevInBattle) && (em->combatPlayers == 0)) {
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
    Em *m;
    s16 i;
    f32 len;
    f32 lim;

    for (m = gEms, i = 0; i < 20; i++, m++) {
        if (pl->rideEm == m->slotNo && m->type != 0x37) {
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

s32 Player_WallClimbInput(Player *pl, s16 noTurn)
{
    u16 s = pl->input.stickDir;

    if (s & 0x2000) {
        if (pl->input.held & 0x10) {
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
    if (s & 0x1000) {
        if (pl->input.held & 0x10) {
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
    if (pl->input.pressed & 0x40) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_LET_GO, 0);
        return 1;
    }
    return 0;
}

void Player_StartSpotAction(Player *pl, s32 mode)
{
    s8 kind;
    u16 angle;
    f32 pos[3];
    f32 d[3];
    f32 v[3];
    s32 ang[3];
    f32 m[16];

    switch (Player_FindActionSpot((Player *)pl, (f32 *)pos, &angle, (u8 *)&kind)) {
    case 0:
    case 2:
    case 6:
    case 8:
    default:
        if ((u8) Player_NearEmKind10(pl)) {
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
        Mtx_FromAnglesXYZ(ang, m);
        Vec_Set(v, 120.0f * gSpotLineupOffsets[pl->index], 0.0f, 0.0f);
        Vec_Rotate(d, v, m);
        pl->moveTo[0] += d[0];
        pl->moveTo[1] += d[1];
        pl->moveTo[2] += d[2];
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

void Player_HoldSpecialInput(Player *pl, u8 mode)
{

    if (pl->input.held & 0x80) {
        if (mode == 0 || mode == 2) {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
                if (mode == 2 && (pl->carry & 0xF)) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
                }
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_START, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_GUNLANCE:
                if (mode == 2 && (pl->carry & 0xF)) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
                }
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_DRAW, 0);
                break;
            case WEAPON_DUAL_SWORDS:
                if (mode == 2 && (pl->carry & 0xF)) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
                }
                if (pl->demonMode == 0) {
                    DualSwords_DemonToggle(pl, 0);
                }
                break;
            case WEAPON_HAMMER:
                if (mode == 2 && (pl->carry & 0xF)) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
                }
                Hammer_StartCharge(pl);
                break;
            case WEAPON_HUNTING_HORN:
                if (mode == 2 && (pl->carry & 0xF)) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
                }
                HuntingHorn_StartPlaying(pl, 1);
                break;
            case WEAPON_LONG_SWORD:
                if (mode == 2 && (pl->carry & 0xF)) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
                }
                LongSword_SpiritDraw(pl);
                break;
            case WEAPON_BOW:
                if (mode == 2 && (pl->carry & 0xF)) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
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

void Player_TryGather(Player *pl, s32 a)
{
    f32 pos[3];
    u16 kind;
    u16 id;
    u16 r;

    if ((u8) Player_NearEmKind10(pl) && (u8) a != 2) {
        return;
    }
    if (Player_GetGatherPoint(pl, &kind, pos) == 0xFFFF) {
        goto other;
    }
    switch (kind) {
    case 0:
        if ((u8) a == 2) {
            goto other;
        }
        if ((u8) a == 0) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER, 0);
        } else {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_CROUCHED, 0);
        }
        break;
    case 3:
        if ((u8) a == 2) {
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
        if ((u8) a == 2) {
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
            if ((u8) a == 2) {
                Sound_PlaySeFlat(0, 0x15, 0);
            }
            break;
        }
        break;
    }
    if ((u8) a != 2) {
        r = MinePoint_FindNearPlayer(pl);
        if (r != 0xFFFF) {
            if ((u8) a == 0) {
                switch (r) {
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
                switch (r) {
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

s32 Player_CrouchInput(Player *pl)
{

    if (Player_IsLocal(pl) == 0 && Game_IsOnline() == 1) {
        return 0;
    }
    switch (gScene.questPhase) {
    case 4:
        if (pl->unk7E5 == 0) {
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
    if ((pl->input.pressed & 0x40) && pl->unk668 == 0) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_STAND_UP, 0);
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (pl->actRequest != 0) {
        Player_StartAct(pl);
    }
    if (pl->unk668 == 0) {
        if (pl->input.stickDirPressed & 0x3C) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
        }
        Player_HoldSpecialInput(pl, 0);
    }
    if ((pl->input.pressed & 0x200) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
        Player_TryGather(pl, 2);
        Player_UseItem(pl, 0);
        pl->pouchHudFlags |= 1;
    }
    if (pl->input.pressed & 0x20) {
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

void Player_GuardInput(Player *pl)
{
    u8 temp_a0;

    if (((Player_IsLocal(pl) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1)) {
        temp_a0 = pl->weaponType;
        switch (temp_a0) {
        case 3:
            if ((Player_StickMoveLevel(pl, 0) & 0xFF) && (pl->motionId[0] != 0x3F3)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GUARD_WALK, PL_SET_SNAP_POS);
            }
            if (pl->input.pressed & 1) {
                Lance_GuardAttack(pl, 0);
            }
            if (pl->input.stickDirPressed & 0x3C) {
                Lance_GuardAttack(pl, 1);
            }
            break;
        case 9:
            if ((Gunlance_GuardInput(pl, 0) == 0) && (Player_StickMoveLevel(pl, 0) & 0xFF) && (pl->motionId[0] != 0x3F3)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GUARD_WALK, PL_SET_SNAP_POS);
            }
            break;
        case 4:
            if ((pl->input.pressed & 0x40) && (pl->stamina >= 0x4B)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
            if (pl->input.stickDirPressed & 0x20) {
                SwordShield_GuardSlash(pl);
            }
            break;
        case 0:
        case 7:
            if ((pl->input.pressed & 0x40) && (pl->stamina >= 0x4B)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
            break;
        }
    }
}

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
            ang = Math_Heading(gEmWallContacts[pl->index][i].pos[0], gEmWallContacts[pl->index][i].pos[2], 0.0f, 0.0f);
            if ((flags & 2) && (pl->wallHitDirs & 0xE0000007)) {
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

s32 Player_TurnAlongWallSide(Player *pl, s16 side)
{
    s16 i;
    u16 flags;
    u16 ang;

    if ((u8)Obj_InCurrentArea(pl) == 0) {
        return 0;
    }
    if (pl->wallHitDirs & (side == 0 ? 0xE0000007 : 0x3E000)) {
        for (i = 0; i < 10; i++) {
            flags = gEmWallContacts[pl->index][i].flags;
            if (flags == 0) {
                break;
            }
            ang = Math_Heading(gEmWallContacts[pl->index][i].pos[0], gEmWallContacts[pl->index][i].pos[2], 0.0f, 0.0f);
            if ((flags & 2) && side == 0) {
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

/* Uses the selected pouch item (see config/symbols/game.txt). */
void Player_UseItem(Player *pl, u8 mode)
{
    f32 v[3];
    u16 ang;
    u8 k;

    if (Pouch_CursorCount(pl) <= 0) {
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_BeginState(pl, 0, 4, 0);
        }
        return;
    }
    if (mode == 2) {
        switch (pl->pouch[pl->pouchCursor].item) {
        case 0x61:
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
                Player_BeginState(pl, 0, 4, 0);
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
    case 0x5B:
    case 0x5C:
    case 0x5F:
    case 0x60:
        if (Item_CanUseSmallBarrelBomb() == 1) {
            goto drink;
        }
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    case 0x56:
        if (Scene_InNoBarrelBombArea() == 1) {
            goto drink;
        }
        Sound_PlaySeFlat(0, 0x15, 0);
        if (mode == 1) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    case 0x28:
    case 0x2C:
    case 0x2E:
    case 0x2D:
        if (Scene_CanPlaceKind3() == 1) {
        drink:
            if (pl->action == 0) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_KNEEL_TO_PLACE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            } else if (pl->action == 1) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_KNEEL_TO_PLACE_CROUCHED, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x5D:
    case 0x5E:
    case 0x29A:
        if (Item_CanPlaceLargeBarrelBomb() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x62:
        if (Item_CanPlaceKind1() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x55:
    case 0x296:
        if (Item_CanPlaceKind2() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x54:
    case 0x299:
        if (Player_GetAreaAmbModel(pl) >= 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
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
                Player_BeginState(pl, 0, 4, 0);
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
        if (Player_FindActionSpot((Player *)pl, (f32 *)v, &ang, (u8 *)&k) == 2) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            pl->targetYaw = ang;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_CAST, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x2A6:
        if (Player_FindActionSpot((Player *)pl, (f32 *)v, &ang, (u8 *)&k) == 0xB) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x29B:
        if (Player_FindActionSpot((Player *)pl, (f32 *)v, &ang, (u8 *)&k) == 6) {
            pl->moveTo[0] = v[0];
            pl->moveTo[1] = v[1];
            pl->moveTo[2] = v[2];
            pl->targetYaw = ang;
            Player_StartMoveTo(pl, 0x14);
            pl->stateRand = k;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM, PL_SET_KEEP_RAND);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x2B7:
        if (Player_FindActionSpot((Player *)pl, (f32 *)v, &ang, (u8 *)&k) == 0xA) {
            pl->targetYaw = Vec_Heading(pl->pos, v) - 0x4000;
            pl->stateRand = k;
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPOT_ITEM_STRIKE, PL_SET_KEEP_RAND);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0xB7:
    case 0xB:
    case 0x1A:
    case 0x7:
    case 0x8:
    case 0x292:
    case 0x9:
    case 0xA:
    case 0x1B:
    case 0x298:
    case 0x1C:
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x10:
    case 0x12:
    case 0x13:
    case 0x293:
    case 0x104:
    case 0xB8:
    case 0xC7:
    case 0xCE:
    case 0xCF:
    case 0x15:
    case 0x16:
    case 0x22:
    case 0x17:
    case 0xC:
    case 0xCA:
    case 0x11:
    case 0x14:
    case 0x1E:
    case 0xC8:
        if (func_00723BC0((Em *)pl) == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DRINK, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0x294:
    case 0x9C:
    case 0x9D:
    case 0x9F:
    case 0x9E:
    case 0xA0:
        if (Pouch_HasItem28(pl) == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_START, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
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
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0xAF:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_HORN_PLAY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_TEAM_ITEM_USE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case 0x2F:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0xEB:
    case 0xF7:
    case 0xF8:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3D:
    case 0x3E:
        if (func_00723BC0((Em *)pl) == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_EAT, 0);
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
            }
        }
        break;
    case 0xAD:
        if (Player_LocalAreaOk_Unk() == 1) {
            if (pl->action == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED, 0);
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS, 0);
            }
        } else {
            Sound_PlaySeFlat(0, 0x15, 0);
            if (mode == 1) {
                Player_BeginState(pl, 0, 4, 0);
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
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

extern s32 gPouchFullTakeText;
s32 Player_TakeOfferedItem(Player *pl)
{
    s16 i;
    Player *o;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (pl->offerNoticeTimer != 0) {
        return 0;
    }
    o = gPlayers;
    for (i = 0; i < gScene.playerCount; i++, o++) {
        if (i != pl->index && o->active != 0 && o->area == pl->area && Player_IsState(o, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM) != 0
            && o->offerTo == pl->index && Vec_Distance(pl->pos, o->pos) <= 300.0f && gItemData[o->offerItem].rarity < 3) {
            if (Pouch_Count(pl, o->offerItem) == 0) {
                if (Pouch_HasFreeSlot(pl) != 0) {
                    pl->offerItem = o->offerItem;
                    pl->offerCount = o->offerCount;
                    if (Player_IsLocal(pl) == 1) {
                        QSync_SendPlayer(pl, 7, o->index);
                    }
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM_WAIT, 0);
                    return 1;
                }
                Notice_PushItem((char *)gPouchFullTakeText);
                pl->offerNoticeTimer = 0x5A;
            } else if (Pouch_Room(pl, o->offerItem) >= o->offerCount) {
                pl->offerItem = o->offerItem;
                pl->offerCount = o->offerCount;
                if (Player_IsLocal(pl) == 1) {
                    QSync_SendPlayer(pl, 7, o->index);
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM_WAIT, 0);
                return 1;
            } else {
                Notice_Push(1, 0xF, o->offerItem);
                pl->offerNoticeTimer = 0x5A;
            }
            return 0;
        }
    }
    return 0;
}

void Player_IdleActions(Player *pl, s32 arg)
{
    Em *m = gEms;
    s16 i;
    u8 k;
    u8 r;
    u8 drew;
    u16 d;

    if (Game_IsOnline() == 0) {
        if (Player_IsLocal(pl) == 0) {
            return;
        }
    } else if (Player_IsLocal(pl) == 0) {
        return;
    }
    drew = 0;
    if (gScene.questPhase == 4 && pl->unk7E5 == 0) {
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_START, 0);
        return;
    }
    pl->freeState = 1;
    if (pl->rideState != 0) {
        k = pl->rideEm;
        for (i = 0; i < 20; i++, m++) {
            if (k == m->slotNo && (m->type == 7 || m->type == 0x32 || m->type == 0x3A || m->type == 0x48 ||
                                   m->type == 0x37)) {
                if (m->quakeTimer != 0) {
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
        if (pl->input.pressed & 0x20) {
            if (Player_TakeOfferedItem(pl) == 0) {
                Player_StartSpotAction(pl, 0);
                Player_TryGather(pl, 0);
            }
        }
        if ((pl->input.pressed & 0x200) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
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
            r = Player_StickMoveLevel(pl, 1);
            switch (r) {
            case 0:
                break;
            case 1:
            case 2:
            case 3:
                if (pl->state != 0x24 && pl->state != 3 && pl->state != 0x74 && pl->unk668 == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK_DRAWN, 0);
                }
                break;
            case 4:
                break;
            }
            if (r != 0 && pl->unk668 != 0 && pl->state != 0x3B) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
                if ((pl->input.pressed & 0x20) || ((pl->input.pressed & 0x10) && pl->input.stickTilt >= 0x28)) {
                    if (Player_TestFlag(pl, 0x1200) == 0) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE_MOVING, 0);
                    }
                } else if ((pl->input.pressed & 0x200) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
                    Player_TryGather(pl, 2);
                    Player_UseItem(pl, 0);
                    pl->pouchHudFlags |= 1;
                }
                break;
            default:
                if ((pl->input.pressed & 0x220) || ((pl->input.pressed & 0x10) && pl->input.stickTilt >= 0x28)) {
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
            r = Player_StickMoveLevel(pl, 0);
            switch (r) {
            case 0:
                break;
            case 1:
                if (pl->state != 0x49 && pl->state != 0x24 && pl->unk668 == 0) {
                    Player_FaceStick(pl);
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK_SLOW, 0);
                }
                break;
            case 2:
                if (pl->state != 2 && pl->state != 0x24 && pl->unk668 == 0) {
                    Player_FaceStick(pl);
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK, 0);
                }
                break;
            case 3:
                if (pl->state != 1 && pl->state != 0x3F && pl->state != 0x13 && pl->state != 0x24 &&
                    pl->state != 0x1F && pl->unk668 == 0) {
                    Player_FaceStick(pl);
                    d = pl->targetYaw + 0x10000 - pl->rot[1];
                    if (d >= 0x6000U && d < 0xA001U) {
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
                Player_StartAct(pl);
            }
            if (r != 0 && pl->unk668 != 0 && pl->state != 0x3B) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (pl->input.pressed & 0x40) {
                if (pl->input.stickTilt >= 0x55) {
                    if (pl->stamina >= 0x4B) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                    }
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, 0);
                }
            }
            if ((pl->input.pressed & 0x200) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
                Player_TryGather(pl, 2);
                Player_UseItem(pl, 0);
                pl->pouchHudFlags |= 1;
            }
            Player_HoldSpecialInput(pl, 0);
            if ((drew == 0 ? pl->input.pressed : pl->input.held) & 0x20) {
                Player_CheckClimb(pl);
            }
            if (pl->input.stickDirPressed & 0x3C) {
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
            if (pl->input.pressed & 1) {
                Player_SetState(pl, PL_GROUP_WEAPON, PL_WPN_SHARED_ATTACK, PL_SET_SNAP_POS);
            }
            if (pl->input.pressed & 0x20) {
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

void Player_BowgunAim(Player *pl)
{
    s8 s;
    s8 a;
    u16 da;

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
        if (pl->input.held & 4) {
            return;
        }
        if (!(pl->input.held & 8)) {
            return;
        }
    }
    if (pl->input.held & 0x3000) {
        a = 5;
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            a *= -1;
        }
        if (pl->input.held & 0x2000) {
            Bowgun_AddAimWeight(pl, a);
        } else {
            Bowgun_AddAimWeight(pl, (s8) -a);
        }
        pl->aimLockTimer = 5;
    }
    if (pl->input.held & 0x800) {
        pl->aimLockTimer = 10;
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            pl->targetYaw -= 0x240;
        } else {
            pl->targetYaw += 0x240;
        }
    }
    if (pl->input.held & 0x400) {
        pl->aimLockTimer = 10;
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            pl->targetYaw += 0x240;
        } else {
            pl->targetYaw -= 0x240;
        }
    }
    Player_SetAimLayers(pl, 0x3ED, 0x3EE);
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        if (!(pl->input.stickDir & 0x3C00) || pl->input.stickTilt < 0x28) {
            return;
        }
        s = 1;
        da = 0x4C;
        if (pl->input.stickTilt >= 0x78) {
            s = 8;
            da = 0x200;
        } else if (pl->input.stickTilt >= 0x5A) {
            s = 4;
            da = 0xC0;
        } else if (pl->input.stickTilt >= 0x3C) {
            s = 2;
            da = 0x80;
        }
        if (pl->input.stickDir & 0x3000) {
            if (gScene.aimReverse != 0) {
                s = -s;
            }
            s = (s8) s;
            if (pl->input.stickDir & 0x2000) {
                Bowgun_AddAimWeight(pl, s);
            } else {
                Bowgun_AddAimWeight(pl, (s8) -s);
            }
            pl->aimLockTimer = 5;
            Player_SetAimLayers(pl, 0x3ED, 0x3EE);
        }
        if (pl->input.stickDir & 0x800) {
            pl->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                pl->targetYaw -= da;
            } else {
                pl->targetYaw += da;
            }
        }
        if (pl->input.stickDir & 0x400) {
            pl->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                pl->targetYaw += da;
            } else {
                pl->targetYaw -= da;
            }
        }
    }
    Player_SetAimLayers(pl, 0x3ED, 0x3EE);
}

static void Player_AddAimPitch(Player *pl, s16 d)
{
    pl->aimPitch += d;
    if (d >= 0) {
        if (pl->aimPitch >= 0x2000) {
            pl->aimPitch = 0x2000;
        }
    } else {
        if (pl->aimPitch <= -0x2000) {
            pl->aimPitch = -0x2000;
        }
    }
}

static void Player_AddAimPitchPct(Player *pl, s8 d)
{
    f32 f;
    s8 v;

    pl->aimPitchPct += d;
    if (pl->aimPitchPct > 100) {
        pl->aimPitchPct = 100;
    }
    if (pl->aimPitchPct < -100) {
        pl->aimPitchPct = -100;
    }
    v = pl->aimPitchPct;
    f = v / 100.0f;
    if (v >= 0) {
        pl->aimPitch = 0.5f + (65536.0f * (45.0f * f)) / 360.0f;
        return;
    }
    pl->aimPitch = 0.5f + (65536.0f * (35.0f * f)) / 360.0f;
}

void Player_UpdateLean(Player *pl, s32 mot)
{
    s8 s;
    s8 a;
    u16 da;
    u16 d;

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
        if (pl->input.held & 4) {
            return;
        }
        if (!(pl->input.held & 8)) {
            return;
        }
    }
    if (pl->input.held & 0x3000) {
        a = 4;
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            a *= -1;
        }
        if (pl->input.held & 0x2000) {
            if ((u16) mot != 0x19E) {
                Player_AddAimPitch(pl, a << 8);
            } else {
                Player_AddAimPitchPct(pl, a);
            }
        } else {
            if ((u16) mot != 0x19E) {
                Player_AddAimPitch(pl, -a << 8);
            } else {
                Player_AddAimPitchPct(pl, (s8) -a);
            }
        }
        pl->aimLockTimer = 5;
    }
    if (pl->input.held & 0x800) {
        pl->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            pl->targetYaw -= 0x180;
        } else {
            pl->targetYaw += 0x180;
        }
    }
    if (pl->input.held & 0x400) {
        pl->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            pl->targetYaw += 0x180;
        } else {
            pl->targetYaw -= 0x180;
        }
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        if ((pl->input.stickDir & 0x3C00) && pl->input.stickTilt >= 0x28) {
            s = 1;
            da = 0x4C;
            if (pl->input.stickTilt >= 0x78) {
                s = 8;
                da = 0x200;
            } else if (pl->input.stickTilt >= 0x5A) {
                s = 4;
                da = 0xC0;
            } else if (pl->input.stickTilt >= 0x3C) {
                s = 2;
                da = 0x80;
            }
            if (pl->input.stickDir & 0x3000) {
                if (gScene.aimReverse != 0) {
                    s = -s;
                }
                s = (s8) s;
                if (pl->input.stickDir & 0x2000) {
                    if ((u16) mot != 0x19E) {
                        Player_AddAimPitch(pl, s << 8);
                    } else {
                        Player_AddAimPitchPct(pl, s);
                    }
                } else {
                    if ((u16) mot != 0x19E) {
                        Player_AddAimPitch(pl, -s << 8);
                    } else {
                        Player_AddAimPitchPct(pl, (s8) -s);
                    }
                }
                pl->aimLockTimer = 5;
            }
            if (pl->input.stickDir & 0x800) {
                pl->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    pl->targetYaw -= da;
                } else {
                    pl->targetYaw += da;
                }
            }
            if (pl->input.stickDir & 0x400) {
                pl->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    pl->targetYaw += da;
                } else {
                    pl->targetYaw -= da;
                }
            }
        }
    }
    if ((u16) mot == 0x19E) {
        d = pl->targetYaw - pl->yawCenter;
        if (d <= 0x8000U && d > 0x4000U) {
            pl->targetYaw = pl->yawCenter + 0x4000;
        } else if (d >= 0x8000U && d < 0xC000U) {
            pl->targetYaw = pl->yawCenter - 0x4000;
        }
    }
}

void Player_StateIdle(Player *pl, s32 unused)
{
    u16 motion = pl->motionId[0];

    switch (motion) {
    case 1:
    case 0x3F:
        if (pl->staminaMax < 76) {
            Player_SetMotionPair(pl, 0x193, 6, 0);
        } else if (pl->motionTime >= 240 && pl->gender == 1 && motion != 0x3F) {
            Player_SetMotionPair(pl, 0x3F, 2, 0);
        }
        break;
    case 0x15:
        if (pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, 5, 4, 0x90);
        }
        break;
    case 5:
        if (!(gScene.frame & 0x1F)) {
            Effect02_SpawnOnOwner(pl, 8, 0x14, 0.1f);
        }
        break;
    case 0x193:
        if (pl->staminaMax > 75) {
            Player_SetMotionPair(pl, 1, 4, 0);
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

void Player_StateMove(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];
    u8 r;
    u8 t;

    if (pl->step == 0) {
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 10;
        pl->timer = 10;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x1000);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 3, 6, 0);
            Player_SetFlag(pl, 0x2000000);
            break;
        case 1:
            Player_SetMotionPair(pl, 2, 4, 0);
            break;
        case 2:
        case 9:
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
            default:
                Player_SetMotionPair(pl, 0x3EC, 4, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_SetMotionPair(pl, 0x3EC, 4, 0);
                break;
            case WEAPON_HAMMER:
            case WEAPON_HUNTING_HORN:
                Player_SetMotionPair(pl, 0x3EC, 8, 0);
                break;
            case WEAPON_HEAVY_BOWGUN:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_LIGHT_BOWGUN:
            case WEAPON_DUAL_SWORDS:
            case WEAPON_BOW:
                Player_SetMotionPair(pl, 0x3EC, 6, 0);
                break;
            }
            break;
        case 3:
            Player_SetMotionPair(pl, 10, 6, 0);
            break;
        case 4:
            break;
        case 5:
            Player_SetMotionPair(pl, 10, 2, 0);
            pl->action = 1;
            break;
        case 6:
            pl->dashDelay = 30;
            if (pl->motionId[0] != 3) {
                Player_SetMotionPair(pl, 3, 0, 0x26);
            }
            Player_SetFlag(pl, 0x2000000);
            break;
        case 7:
            Player_SetMotionPair(pl, 0x31, 6, 0);
            break;
        case 8:
            Player_SetFlag(pl, 0x8000);
            Player_SetMotionPair(pl, 0x3F3, 4, 0);
            break;
        }
        pl->motionTime = 0;
    }
    if (Player_IsLocal(pl) == 0 && Game_IsOnline() == 1) {
        return;
    }
    r = Player_StickMoveLevel(pl, pl->weaponOut) & 0xFF;
    if (pl->stateVar1 != 0) {
        pl->stateVar1--;
    }
    if (pl->timer > 0) {
        pl->timer--;
    }
    t = r;
    if (t == 0 && pl->timer == 0) {
        if (pl->motionId[0] == 0x3EC) {
            if (variant == 9) {
                Player_SetState(pl, PL_GROUP_WEAPON, BOW_AIM_RESUME, PL_SET_SNAP_POS);
            } else {
                Player_BeginState(pl, 0, gDrawnWalkStopIdle[pl->weaponType], 0);
            }
        } else if (pl->motionId[0] == 3 || pl->motionId[0] == 4) {
            if (pl->stateVar1 == 0) {
                pl->freeState = 1;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            } else {
                Player_BeginState(pl, 0, 4, 0);
            }
        } else if (pl->motionId[0] == 0x3F3) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        } else {
            Player_BeginState(pl, 0, 4, 0);
        }
        return;
    }
    switch (pl->motionId[0]) {
    case 2:
        if (Motion_PassedFrame(pl, 0, 108.0f)) {
            pl->stateVar0++;
            if (pl->stateVar0 >= 5) {
                pl->stateVar0 = 0;
                Player_SetMotionPair(pl, 0x1D, 2, 0);
            }
        }
        break;
    case 3:
        if (t < 3) {
            pl->freeState = 1;
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            return;
        }
        break;
    case 0x1D:
        if (pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, 2, 6, 0x24);
        }
        break;
    case 0x3F3:
        if (!(pl->input.held & 0x80)) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        }
        break;
    }
    if (pl->unk668 != 0 && (variant != 5 || variant != 8)) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
    }
    if (t != 0) {
        Player_FaceStick(pl);
    }
    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (variant == 2 && pl->weaponType == WEAPON_HUNTING_HORN && Player_IsLocal(pl) == 1 && pl->motionId[0] == 0x3EC &&
        Player_HasMealEffect(pl, 0) == 1 && pl->mealEffect[0] > 0) {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 8.0f;
        Vec_Rotate(d, v, pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[2] += d[2];
    }
    if (variant == 2 && pl->weaponType == WEAPON_DUAL_SWORDS && Player_IsLocal(pl) == 1 && pl->motionId[0] == 0x3EC &&
        pl->demonMode != 0) {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 2.0f;
        Vec_Rotate(d, v, pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[2] += d[2];
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

void Player_StateDrawSheathe(Player *pl, u32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
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
        if (pl->carry & 0xF) {
            pl->carry &= 0xF0;
            StageObj_K0C_CreateAtPlayer(pl, 4);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_BeginState(pl, 0, gDrawMotions[pl->weaponType].next, 0);
                break;
            case 1:
                Player_BeginState(pl, 0, gSheatheMotions[pl->weaponType].next, 0);
                break;
            case 2:
                Player_BeginState(pl, 0, gSheatheMovingMotions[pl->weaponType].next, 0);
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
                Player_BeginState(pl, 0, gDrawMotions[pl->weaponType].next, 0);
                break;
            case 5:
                Player_BeginState(pl, 0, gDrawMovingMotions[pl->weaponType].next, 0);
                break;
            }
        }
        break;
    }
}

void Player_StateLeapOrFall(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];
    f32 t;

    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = 2;
        pl->aimLockTimer = 0;
        Player_ResetAim(pl);
        switch (variant) {
        default:
            Player_SetMotionPair(pl, 0xF, 0, 0);
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
            Player_SetMotionPair(pl, 0xF, 8, 0x20);
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
        Player_SetMoveMode(pl, 2, 0, 0);
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
        v[0] = pl->vel[0];
        v[1] = pl->vel[1];
        v[2] = pl->vel[2];
        Vec_Rotate(d, v, pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[1] += d[1];
        pl->pos[2] += d[2];
        if (pl->vel[1] < 0.0f) {
            if (Player_FloorAheadAbove(pl, &t, 55.0f, 180.0f, 30.0f) && pl->hp > 0 && pl->unk893 < 21) {
                pl->targetYaw = pl->rot[1];
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, PL_SET_SNAP_POS);
                break;
            }
            if (pl->floorY < pl->pos[1]) {
                break;
            }
            pl->pos[1] = pl->floorY;
            pl->action = 0;
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
            if (pl->weaponOut == 0 && (pl->input.held & 0x10) && (Player_StickMoveLevel(pl, 0) & 0xFF)) {
                pl->stateVar1 = 1;
                Player_SetMotionPair(pl, 0x10, 0, 0);
            } else {
                pl->stateVar1 = 0;
                Player_SetMotionPair(pl, 0x3E, 0, 0);
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->stateVar1 == 0) {
                Player_BeginState(pl, 0, 8, 0);
            } else if (pl->inBattle == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DASH_RESUME, 0);
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH, 0);
            }
        }
        break;
    }
}

void Player_StateClimbAirborne(Player *pl, s32 variant)
{
    f32 y;
    f32 v[3];
    f32 d[3];

    pl->timer++;
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
        default:
            pl->step++;
            Player_SetMotionPair(pl, 0xB, 2, 0);
            Player_SetMoveMode(pl, 0, 0, 0);
            break;
        case 2:
            pl->pos[1] -= 195.0f;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -30.0f;
            Vec_Rotate(d, v, (f32 *) pl->worldMtx);
            pl->pos[0] += d[0];
            pl->pos[2] += d[2];
        case 1:
            pl->step = 2;
            Player_SetMoveMode(pl, 2, 0, 0);
            Player_StopMove(pl);
            pl->vel[1] = -9.0f;
            pl->accel[1] = -8.0f / 11.0f;
            if (variant == 1) {
                Player_SetMotionPair(pl, 0xC, 4, 0x16);
            } else {
                Player_SetMotionPair(pl, 0xC, 0, 0x16);
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
            Player_SetMotionPair(pl, 0xC, 2, 0);
            pl->action = 2;
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
                pl->action = 0;
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
                Player_SetMotionPair(pl, 0xD, 0, 0);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 3, 0);
        }
        break;
    }
}

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
            Player_SetMotionPair(pl, 7, 2, 0);
            Player_SetMoveMode(pl, 0, 0, 0);
            pl->weaponOut = 0;
            carry = pl->carry;
            if (carry & 0xF) {
                pl->carry = carry & 0xF0;
                StageObj_K0C_CreateAtPlayer(pl, 4);
            }
            break;
        case 1:
            pl->step = 2;
            if (pl->motionId[0] == 0x19C) {
                Player_SetMotionPair(pl, 8, 8, 0);
            } else {
                Player_SetMotionPair(pl, 8, 2, 0);
            }
            Player_UpdateMotionTime(pl, 0);
            Player_SetMoveMode(pl, 0x8001, 0, 0);
            pl->weaponOut = 0;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 8, 6, 0);
            Player_SetMoveMode(pl, 0x8001, 0, 0);
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
        Player_SetMotionPair(pl, 1, 4, 0);
        pl->deliveryMenu = 0xFF;
        pl->menuDelay = 30;
        pl->menuCursor = 0;
        pl->menuPage = 0;
        break;
    case 1:
        if ((s32) pl->motionTime >= 0xF0 && pl->gender == 1 && pl->motionId[0] != 0x3F) {
            Player_SetMotionPair(pl, 0x3F, 2, 0);
        }
        if (Player_IsLocal(pl) == 1 && pl->timer <= 0) {
            if (Quest_PhaseIsOver(1) == 1 || pl->deliveryMenu == 0) {
                Player_BeginState(pl, 0, 2, 0);
            } else if (pl->deliveryMenu == 1) {
                pl->deliveryMenu = 0;
                switch (pl->menuCursor) {
                case 0:
                    Player_StartSpotAction(pl, 1);
                    break;
                case 1:
                    Quest_Start();
                    Player_BeginState(pl, 0, 2, 0);
                    break;
                case 2:
                    Quest_Abort();
                    Player_BeginState(pl, 0, 2, 0);
                    break;
                }
            }
        }
        break;
    }
}

void Player_StateToIdle(Player *pl, s32 unused)
{
    Player_BeginState(pl, 0, 0x2, 0);
}

void Player_StateThrowItem(Player *pl, s32 variant)
{
    s8 kind;
    u8 nouse;
    u8 k;

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x191, 2, 0);
        } else {
            Player_SetMotionPair(pl, 0x1A1, 2, 0);
        }
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
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
                Eff03_Create(pl, 0, kind);
                break;
            case 0x4E:
                Eff03_Create(pl, 7, kind);
                break;
            case 0x48:
                Eff03_Create(pl, 1, kind);
                break;
            case 0x297:
                Eff03_Create(pl, 1, kind);
                break;
            case 0x4B:
                Eff03_Create(pl, 5, kind);
                break;
            case 0x4D:
                Eff03_Create(pl, 0xb, kind);
                break;
            case 0x47:
                Eff03_Create(pl, 6, kind);
                break;
            case 0xD7:
                Eff03_Create(pl, 8, kind);
                break;
            case 0x49:
                Eff03_Create(pl, 9, kind);
                break;
            case 0x50:
                Eff03_Create(pl, 0xa, kind);
                break;
            case 0x41:
                Eff03_Create(pl, 0xc, kind);
                break;
            case 0x42:
                Eff03_Create(pl, 0xd, kind);
                break;
            case 0x43:
                Eff03_Create(pl, 0xe, kind);
                break;
            case 0x44:
                Eff03_Create(pl, 0xf, kind);
                break;
            case 0x45:
                Eff03_Create(pl, 0x15, kind);
                break;
            case 0x2BA:
                Eff03_Create(pl, 0x11, kind);
                break;
            case 0x2BB:
                Eff03_Create(pl, 0x12, kind);
                break;
            case 0x2BC:
                Eff03_Create(pl, 0x13, kind);
                break;
            case 0x4F:
                Eff03_Create(pl, 0x17, kind);
                break;
            case 0x40:
                if (pl->prevStateGroup == 0 && pl->prevState == 0x49) {
                    kind = 1;
                }
                if (pl->stateRand % 8) {
                    nouse = 1;
                }
                if (Player_HasSkill(pl, 0x2D) == 1 && !(Rand_Next(1) & 7)) {
                    nouse = 1;
                }
                if (nouse == 0 && Player_IsLocal(pl) == 1) {
                    Notice_Push(1, 5, 0x40);
                }
                Eff28_Create((Em *) pl, kind, nouse);
                break;
            }
            if (Quest_PhaseIsOver(1) == 0 && nouse == 0) {
                Pouch_Add(pl, pl->useItem, -1);
            }
        }
        if (pl->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_BeginState(pl, 0, 0xA, 0);
                break;
            case 1:
                Player_BeginState(pl, 0, 2, 0);
                break;
            case 2:
                if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
                    k = gScene.campArea;
                    pl->areaMoveState = 1;
                    pl->startPos[0] = gStartPos[k][0];
                    pl->startPos[1] = D_43F0A4[k][0];
                    pl->startPos[2] = D_43F0A8[k][0];
                    pl->startAngle = gStartAngle[k];
                    pl->nextArea = k;
                    Player_AddSpawnOffset(pl, pl->startPos, pl->startAngle);
                    pl->noHitTimer = 5;
                    QSync_SendPlayer(pl, 5, 0);
                } else {
                    Player_BeginState(pl, 0, 2, 0);
                }
                break;
            }
        }
        break;
    case 2:
        break;
    }
}

void Player_StateLedgeHang(Player *pl, s32 variant)
{
    f32 y;
    s32 ang[3];
    f32 v[3];
    f32 pos[3];
    f32 d[3];
    f32 m[16];

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->targetYaw = pl->rot[1];
        Player_SetMotionPair(pl, 0x11, 0, 0);
        pl->timer = 30;
        Player_SetMoveMode(pl, 0, 0, 0);
        if (variant == 1) {
            ang[0] = 0;
            ang[1] = pl->rot[1];
            ang[2] = 0;
            Mtx_FromAnglesXYZ(ang, m);
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 55.0f;
            Vec_Rotate(d, v, m);
            pos[0] = pl->pos[0] + d[0];
            pos[1] = pl->pos[1] + d[1];
            pos[2] = pl->pos[2] + d[2];
            if (Em_GetFloorYAbove((Em *) pl, pos, &y) == 1) {
                pl->pos[1] = y;
            }
            v[2] = 55.0f;
            Vec_Rotate(d, v, (f32 *) pl->worldMtx);
            pl->pos[0] += d[0];
            pl->pos[1] += d[1];
            pl->pos[2] += d[2];
        }
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB) == 1 && pl->prevStateGroup == 0 && pl->prevState == 9) {
            pl->unk893++;
        }
        break;
    case 1:
        if (pl->timer != 0) {
            pl->timer--;
            if (pl->timer <= 0) {
                pl->timer = 0;
            }
        }
        if (pl->motion[0].playing == 0 || (pl->input.pressed & 0x20) || (pl->input.stickDir & 0x2000)) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_PULL_UP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else if (Player_IsLocal(pl) == 1 && (pl->input.stickDirPressed & 0x1000) && pl->timer == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_DROP, PL_SET_SNAP_POS);
        }
        break;
    }
}

void Player_StateRideStagger(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x1A, 6, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (Player_CheckRideSlip(pl) == 0) {
            Player_BeginState(pl, 0, 8, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
        }
        break;
    }
}

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
            Player_SetMotionPair(pl, 8, 6, 0);
            Player_SetMoveMode(pl, 0, 0, 0);
            break;
        case 1:
            pl->step = 3;
            Player_SetMoveMode(pl, 1, 0, 0);
            Player_SetMotionPair(pl, 0x199, 0, 0x14);
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
            Player_SetMotionPair(pl, 9, 0, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 2, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, PL_SET_NO_SYNC);
        }
        break;
    }
}

void Player_StateKneelToPlace(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, 7, 2, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
            Player_SetMotionPair(pl, 8, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

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
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 30, 0, pl->pos, 1, 0);
        if (pl->carry & 0xF) {
            pl->carry &= 0xF0;
            StageObj_K0C_CreateAtPlayer(pl, 4);
        }
        if (variant == 0) {
            pl->step = 2;
            if ((pl->weaponType == WEAPON_HAMMER || pl->weaponType == WEAPON_HUNTING_HORN) && pl->weaponOut != 0) {
                Player_SetMotionPair(pl, 0x3F3, 2, 0);
            } else {
                Player_SetMotionPair(pl, 0x1C, 2, 0);
            }
        } else {
            pl->step++;
            Player_SetMotionPair(pl, 0x16, 0, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x1C, 0, 6);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
        } else if (Player_IsLocal(pl) == 1 && variant == 0 && pl->weaponOut != 0) {
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
            case WEAPON_DUAL_SWORDS:
                if ((pl->input.stickDirPressed & 0x20) && Motion_InFrameRange(pl, 0, 10.0f, 44.0f) && pl->stateVar0 == 0) {
                    pl->stateVar0 = 1;
                    if (pl->input.stickDir & 0x800) {
                        pl->stateVar0 = 2;
                    }
                    if (pl->input.stickDir & 0x400) {
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
                if (Motion_ReachedFrame(pl, 0, 40.0f) && (pl->input.held & 0x80)) {
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
    f32 v[3];
    f32 pos[4];
    f32 d[4];
    f32 y;
    f32 *vz;
    f32 *dy;
    f32 *dz;

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, 0x12, 0, 0x58);
        pl->targetYaw = pl->rot[1];
        v[0] = 0.0f;
        v[1] = 0.0f;
        vz = &v[2];
        *vz = 55.0f;
        Vec_Rotate(d, v, (f32 *) pl->worldMtx);
        dy = &d[1];
        dz = &d[2];
        pos[0] = pl->pos[0] + d[0];
        pos[1] = pl->pos[1] + *dy;
        pos[2] = pl->pos[2] + *dz;
        if (Em_GetFloorYAbove((Em *) pl, pos, &y) == 1) {
            pl->pos[1] = y;
        }
        *vz = 55.0f;
        Vec_Rotate(d, v, (f32 *) pl->worldMtx);
        pl->pos[0] += d[0];
        pl->pos[1] += *dy;
        pl->pos[2] += *dz;
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateDash(Player *pl, s32 variant)
{
    u8 r;
    f32 v[3];
    f32 d[3];

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x600);
        pl->motionTime = 0;
        pl->dashDelay = 30;
        switch (variant) {
        case 0:
            if (pl->motionId[0] != 3) {
                Player_SetMotionPair(pl, 3, 0, 0);
            }
            break;
        case 1:
            Player_SetMotionPair(pl, 3, 4, 30);
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
        if ((pl->input.pressed & 0x40) && pl->stamina >= 0x4B && Quest_PhaseIsOver(1) == 0) {
            pl->targetYaw = pl->rot[1];
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            break;
        }
        if (pl->input.stickTilt >= 0x28) {
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
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 5.0f;
        Vec_Rotate(d, v, (f32 *) pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[2] += d[2];
        if (Player_IsLocal(pl) != 0) {
            r = Player_StickMoveLevel(pl, 0);
            if (r != 5 && (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED) == 0 || pl->stamina > 0) {
                    if (r == 3) {
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
            if (pl->unk668 != 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    }
}

void Player_StateFleeDash(Player *pl, s32 variant)
{
    u8 r;
    f32 v[3];
    f32 d[3];

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x600);
        pl->motionTime = 0;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 0x26, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x27, 0, 0);
            break;
        case 2:
            Player_SetMotionPair(pl, 0x26, 4, 0);
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
        if ((pl->input.pressed & 0x40) && variant != 1 && pl->stamina >= 0x4B) {
            pl->targetYaw = pl->rot[1];
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DIVE, PL_SET_SNAP_POS);
            break;
        }
        if (pl->input.stickTilt >= 0x28) {
            Player_FaceStick(pl);
        }
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) && variant != 1) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEAP_OFF_LEDGE, 0);
            break;
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1.5f;
        Vec_Rotate(d, v, (f32 *) pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[2] += d[2];
        if (Player_IsLocal(pl) != 0) {
            r = Player_StickMoveLevel(pl, 0);
            if (r != 5 && (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED) == 0 || pl->stamina > 0) {
                    if (r == 3 && variant != 1) {
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
            if (pl->unk668 != 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    }
}

void Player_StateOutOfBreath(Player *pl, s32 variant)
{
    Player_AddStamina(pl, 4);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->weaponOut = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 5, 4, 0);
            pl->step = 2;
            pl->timer = 0x96;
            break;
        }
        Player_SetMotionPair(pl, 0x15, 8, 0);
        pl->timer = 0xA;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 5, 4, 0x90);
        }
        break;
    case 2:
        if (--pl->timer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, 6, 4, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_AddStamina(pl, (s32) pl->staminaMax);
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateTiredStop(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x27, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 5, 0);
        }
        break;
    }
}

void Player_StateHardLanding(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0x8000, 0, 0);
        pl->rideState = 0;
        Player_SetMotionPair(pl, 0x13, 2, 0);
        Vib_StartLocal(pl, 1);
        CamShake_ReqA(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
        }
        break;
    }
}

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
                Player_SetMotionPair(pl, 0x22, 0, 0x2E);
            } else {
                Player_SetMotionPair(pl, 0x22, 0, 0x5A);
            }
        } else if (Motion_PassedFrame(pl, 0, 0.0f) != 0) {
            Player_SetMotionPair(pl, 0x23, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0x23, 0, 0x2C);
        }
    } else {
        pl->stateVar0 = 1;
    }
    pl->noPushTimer = 2;
    Player_SetMoveMode(pl, 0x8002, 0, 0);
    Player_StepBack(pl, 1);
    Player_WallClimbInput(pl, 0);
}

void Player_StateWallClimbUp(Player *player, s32 variant)
{
    f32 h;
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
        Player_SetMoveMode((void *) player, 2, 0, 0);
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
    if (Player_FloorAheadAbove((void *) player, &h, 55.0f, 180.0f, 30.0f) != 0) {
        player->targetYaw = (s16) player->rot[1];
        Player_RequestState((void *) player, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, 0);
    }
    Player_StepBack(player, 1);
}

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
        Player_SetMoveMode(pl, 2, 0, 0);
        pl->rideState = 0;
        motion = pl->motionId[0];
        if (motion != 0x23) {
            if (motion == 0x22) {
                if (Motion_ReachedFrame(pl, 0, 86.0f) != 0) {
                    Player_SetMotionPair(pl, 0x23, 0, 0x2C);
                } else {
                    Player_SetMotionPair(pl, 0x23, 0, 0);
                }
            } else {
                Player_SetMotionPair(pl, 0x23, 4, 0);
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

void Player_StateWallDismount(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x24, 6, 0);
        Player_StepBack(pl, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
            break;
        }
        Player_StepBack(pl, 1);
        break;
    }
}

void Player_StateWallGrab(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 2, 0, 0);
        pl->rideState = 0;
        Player_TurnAlongWallSide(pl, 0);
        Player_SetMotionPair(pl, 0x22, 4, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 90.0f) != 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD, 0);
        }
        break;
    }
    Player_StepBack(pl, 1);
}

void Player_StateRunStop(Player *pl, s32 unused)
{
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x1E, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 0xA, 0);
        }
        break;
    }
}

void Player_StateSetTrap(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x195, -0xE, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 130.0f) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x19A, 4, 0);
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
            Player_BeginState(pl, 0, 0xA, 0);
        }
        break;
    }
}

void Player_StateSetTrapDone(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x19A, 0, 0x3C);
        if (Player_IsLocal(pl) == 1) {
            if (pl->useItem == 0x2A6) {
                Quest_DeliverFromPouch(pl, 1);
                break;
            }
            Item_UseMisc(pl);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x195, -4, 0xCE);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 0xA, 0);
        }
        break;
    }
}

void Player_StateSideEvade(Player *pl, s32 variant)
{
    u8 dir;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->noFlinchTimer = 0;
        Player_SetEvadeFrames(pl);
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 0x1E, 0, &pl->pos[0], 1, 0);
        pl->motionTime = 0;
        if (variant == 0) {
            if (pl->weaponType == WEAPON_LONG_SWORD) {
                Player_SetMotionPair(pl, 0x3EF, 2, 0);
                break;
            }
            Player_SetMotionPair(pl, 0x3F5, 2, 0);
            break;
        }
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            Player_SetMotionPair(pl, 0x3EE, 2, 0);
            break;
        }
        Player_SetMotionPair(pl, 0x3F4, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
            break;
        }
        if (Player_IsLocal(pl) == 1 && pl->weaponOut != 0) {
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
            case WEAPON_DUAL_SWORDS:
                if ((pl->input.stickDirPressed & 0x20) && Motion_InFrameRange(pl, 0, 10.0f, 42.0f) != 0 && pl->stateVar0 == 0) {
                    pl->stateVar0 = 1;
                    if (pl->input.stickDir & 0x800) {
                        pl->stateVar0 = 2;
                    }
                    if (pl->input.stickDir & 0x400) {
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

/* unk_70A290's .data (0x8C3500-0x8C3C90) */
extern ScEventRec ScEvent_Cam5;
ScEventRec ScEvent_Seq61A9 = {
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
ScEventRec ScEvent_Seq3EB = {
#include "game/008C3600.inc"
};
ScEventRec ScEvent_Seq3ED = {
#include "game/008C3620.inc"
};
ScEventRec ScEvent_Seq1389 = {
#include "game/008C3640.inc"
};
ScEventRec ScEvent_Seq3F4 = {
#include "game/008C3660.inc"
};
ScEventRec ScEvent_Seq3F8 = {
#include "game/008C3680.inc"
};
ScEventRec ScEvent_Seq138A = {
#include "game/008C36A0.inc"
};
ScEventRec ScEvent_Seq138C = {
#include "game/008C36C0.inc"
};
ScEventRec ScEvent_Seq138D = {
#include "game/008C36E0.inc"
};
ScEventRec ScEvent_Seq138F = {
#include "game/008C3700.inc"
};
ScEventRec ScEvent_Seq1771 = {
#include "game/008C3720.inc"
};
ScEventRec ScEvent_Seq438 = {
#include "game/008C3740.inc"
};
ScEventRec ScEvent_Seq431 = {
#include "game/008C3760.inc"
};
ScEventRec ScEvent_Seq424 = {
#include "game/008C3780.inc"
};
ScEventRec ScEvent_Seq42F = {
#include "game/008C37A0.inc"
};
ScEventRec ScEvent_Seq3F0 = {
#include "game/008C37C0.inc"
};
ScEventRec ScEvent_Seq422 = {
#include "game/008C37E0.inc"
};
ScEventRec ScEvent_Seq406 = {
#include "game/008C3800.inc"
};
ScEventRec ScEvent_Seq42C = {
#include "game/008C3820.inc"
};
ScEventRec ScEvent_Seq1390 = {
#include "game/008C3840.inc"
};
ScEventRec ScEvent_Seq436 = {
#include "game/008C3860.inc"
};
ScEventRec ScEvent_Seq426 = {
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
/* Read by plsel as a table of GalView pointers by gallery entry (Galex_QuestTask, unk_D74450.c). */
ScEventHead *gGalleryScEvents[48] = {
    &ScEvent_Seq3ED.head, &ScEvent_Seq3EB.head, &ScEvent_Seq3F0.head, &ScEvent_Seq1389.head,
    0, &ScEvent_Seq3F4.head, 0, &ScEvent_Seq3F8.head,
    &ScEvent_Seq138A.head, &ScEvent_Seq422.head, &ScEvent_Seq424.head, &ScEvent_Seq42F.head,
    &ScEvent_Seq406.head, &ScEvent_Seq138C.head, 0, &ScEvent_Seq138D.head,
    &ScEvent_Seq431.head, &ScEvent_Seq42C.head, &ScEvent_Seq138F.head, &ScEvent_Seq426.head,
    &ScEvent_Seq436.head, &ScEvent_Seq1390.head, &ScEvent_Seq438.head, &ScEvent_Seq1771.head,
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

/* ---- 0x716360 (was unk_716360.c) ---- */
/* prototypes of callees (round 13) */
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
void func_367A90(void);
s32 Player_GetHpSkillBonus(Player *p);
void Player_ClearBuffs(Player *pl);
void Player_PlaySeAtSelf(u8 *a0, s32 a1);
void CamReq_Cancel(void);
extern u16 gStartAngle[];
void Player_SetEvadeFrames(Player *em);
u8 Player_InCurrentArea(void *obj);
s32 Quest_GetDeliveryTarget(Player *p, s32 slot, ItemStack *out);
u16 Quest_DeliverItem(Player *pl, u16 item, s16 count);
s32 Player_NearEmKind10(void *obj);
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
void StageObj_Stun_CreatePlayer(Player *em, u8 k);
extern s8 gStunMashNeed[];
void Player_AddSharpness(Player *p, s16 n);
s32 Player_FloorAheadBelow(Player *em, u16 above, f32 fwd, f32 up);
s32 func_3B5440(Em *em);
s32 Player_GatherAtSpot(Player *em, u16 kind, u16 stat);
void Quest_GiveRewards(Player *p);
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
u8 Player_PushNotice3(s32 arg0);
void QuestStats_Add(s32 id, s32 n);
u8 Player_StickMoveLevel(Player *, s32);
s32 Player_GetStickYaw(Player *p, s32 i);
Em *EmSlot_Alloc(void);
s32 GmEm_Task(Em *em);
void CamReq_Mode01(void);
void Em_BonePos(void *obj, s16 bone, f32 *out);
s32 func_379E40(void);
void func_379E50(f32 *pos, u16 area);
u16 Vec_Heading(f32 *a, f32 *b);
void Item_UseTeamPowder(Player *pl);
void Effect0D_SpawnForPlayer(Player *p, s16 arg, s32 type);
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
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter s32 (the definition has s16); with the definition's type Player_StateSpitRoast by 418 of 469 words differ */
void Player_SetMoveMode(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_StartIdleMotion(Player *p, s32 a, s32 b);
void Quest_StartCountdown(Player *p);
void Eff00_Create(Player *, u8);
void Eff22_CreateForPlayer(Player *, u8 kind);
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
void StageObj_K0C_CreateAtPlayer(Player *, s32 k);
void StageObj_K1B_Create(s8, s16 *);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Motion_ReachedFrame(void *, s32, f32);
void CamCtl_TurnToObj_Unk(u8 *o);
void CamShake_ReqA(void *, s32 i);
void CamReq_Mode0A(void);
s32 ItemGet_Reserve(s32 arg0, u16 *arg1);
s32 Quest_PhaseIsOver(s32);
void Vib_StartLocal(void *port, s32 i);
void Player_BeginState(Player *p, s32 action, s32 a2, s32 a3);
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
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->motionTime = 0;
        Player_SetMotionPair(pl, 0x1AA, -6, 0);
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

void Player_StateBedSleep(Player *pl, s32 variant)
{
    f32 v[3];

    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->motionTime = 0;
        pl->timer = 120;
        Player_SetMotionPair(pl, 0x1AB, 0, 0);
        Effect06_SpawnAtBone(pl, 0, 0, 10, 4.0f);
        break;
    case 1:
        if (--pl->timer <= 0 && Player_IsLocal(pl) == 1) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_GET_UP, 0);
            break;
        }
        if (pl->motionId[0] == 0x1AB && !(Rand_Next(1) & 0x7F)) {
            Player_SetMotionPair(pl, 0x1AC, 4, 0);
        } else if (pl->motionId[0] == 0x1AC && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, 0x1AB, 4, 0);
        }
        v[0] = 0.0f;
        v[1] = 20.0f;
        v[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 20, v, 0.8f);
        }
        Player_AddHp(pl, 1);
        break;
    }
}

void Player_StateBedGetUp(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->motionTime = 0;
        pl->rot[1] = (pl->rot[1] + 0x7FFF + 1) & 0xFFFF;
        pl->targetYaw = (u16) pl->rot[1];
        Player_SetMotionPair(pl, 0x1AD, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateMountedGunAim(Player *pl, s32 variant)
{
    f32 pos[3];
    u16 angle;
    s8 kind;

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x19E, 4, 0);
            pl->aimPitch = 0;
            pl->aimPitchPct = 0;
        } else {
            Player_SetMotionPair(pl, 0x19E, 0, 0x42);
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
                Player_BeginState(pl, 0, 6, 0);
                break;
            }
            Player_UpdateLean(pl, 0x19E);
            if (pl->stateVar0 == 0 && (pl->input.pressed & 0x21) && Pouch_Count(pl, 0x29B) != 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_FIRE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW | PL_SET_SYNC_ALT);
                break;
            }
            if (pl->timer == 0 && (pl->input.pressed2 & 0x40) && pl->itemSelect == 0) {
                Player_BeginState(pl, 0, 6, 0);
            }
        }
        break;
    }
}

void Player_StateMountedGunFire(Player *pl, s32 variant)
{

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x19E, 0, 0x42);
        Player_UpdateMotionTime(pl, 0);
        pl->timer = 60;
        if (Player_IsLocal(pl) != 1 || Pouch_Count(pl, 0x29B) != 0) {
            Pouch_Add(pl, 0x29B, -1);
            Vib_StartLocal(pl, 2);
            Eff22_CreateForPlayer(pl, 0);
        }
        break;
    case 1:
        pl->timer -= 1;
        if (pl->timer == 0) {
            if (Pouch_Count(pl, 0x29B) > 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateCannonFire(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0xD3, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Vib_StartLocal(pl, 3);
        CamShake_ReqA(pl, 5);
        Eff22_CreateForPlayer(pl, 1U);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xD4, 2, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateCrouchStandUp(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 1, 0, 0);
        Player_SetMotionPair(pl, 9, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateLedgePullUp(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, 0x12, 4, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateCrouchWalk(Player *pl, s32 unused)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, 0xA, 8, 0);
        Player_SetMoveMode(pl, 1, 0, 0);
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
            if ((pl->input.pressed & 0x40) && (pl->stamina >= 0x4B) && (pl->unk668 == 0)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL_FROM_CROUCH, PL_SET_SNAP_POS);
                break;
            }
            Player_CrouchInput(pl);
        } else {
        }
        break;
    }
}

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
        Player_SetMoveMode(pl, 0x8001, 0, 0);
        Player_SetMotionPair(pl, 8, blend, 0);
        pl->timer = 0;
        break;
    case 1:
        if (pl->timer >= 12) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, 0);
        }
        break;
    }
}

void Player_StateCrouchWalkFast(Player *pl, s32 unused)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, 0x25, 6, 0);
        Player_SetMoveMode(pl, 1, 0, 0);
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
            if ((pl->input.pressed & 0x40) && (pl->stamina >= 0x4B) && (pl->unk668 == 0)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL_FROM_CROUCH, PL_SET_SNAP_POS);
                break;
            }
            Player_CrouchInput(pl);
        } else {
        }
        break;
    }
}

void Player_StateBattleAlert(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, 0x29, 2, 0);
        Player_SetMoveMode(pl, 0x8000, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
        }
        break;
    }
}

void Player_StateTurnAround(Player *pl, s32 unused)
{
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->rot[1] = (s32) pl->targetYaw;
        Player_SetMotionPair(pl, 0x2A, 0, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
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
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];

    ang[0] = 0;
    ang[2] = 0;
    ang[1] = pl->rot[1];
    Mtx_FromAnglesXYZ(ang, m);
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = -3.0f;
    if ((s16) fwd) {
        v[2] *= -1.0f;
    }
    Vec_Rotate(d, v, m);
    pl->pos[0] += d[0];
    pl->pos[2] += d[2];
}

void Player_UpdateWallSidle(Player *pl)
{
    s32 d;

    if (Player_IsLocal(pl) != 0) {
        switch (Player_StickMoveLevel(pl, 0)) {
        case 1:
        case 2:
        case 3:
            d = (u16) ((u16) Player_GetStickYaw(pl, 0) - *(u16 *) &pl->rot[1]);
            if (d >= 0x2000 && d <= 0x6000) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_POS) == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_POS, 0);
                }
            } else if (d >= 0xA000 && d <= 0xE000) {
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
        if ((pl->input.pressed & 0x40) || !(Player_TurnAlongWallSide(pl, 1) & 0xFF)) {
            pl->freeState = 1;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
        }
    }
}

void Player_StateWallPress(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x2B, 2, 0);
        } else {
            Player_SetMotionPair(pl, 0x2B, 2, 0x1E);
        }
        Player_SetMoveMode(pl, 0x8000, 0, 0);
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

void Player_StateWallSidle(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x2C, 2, 0);
        } else {
            Player_SetMotionPair(pl, 0x2D, 2, 0);
        }
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x1000);
        Player_StepBack(pl, 0);
        break;
    case 1:
        Player_StepBack(pl, 0);
        Player_UpdateWallSidle(pl);
        break;
    }
}

void Player_StateDive(Player *pl, s32 unused)
{
    f32 v[3];
    f32 d[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, 0x2E, 4, 0);
        Player_SetMoveMode(pl, 2, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 0x1E, 0, pl->pos, 1, 0);
        pl->noHitTimer = 3;
        break;
    case 1:
        pl->noHitTimer = 3;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x2F, 0, 0);
            pl->pos[1] += 100.0f;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 16.0f;
            Vec_Rotate(d, v, (f32 *) pl->worldMtx);
            pl->vel[0] = d[0];
            pl->vel[1] = 3.0f;
            pl->vel[2] = d[2];
            Player_ClearAccel(pl);
            Player_StartJump(pl, 8);
        }
        break;
    case 2:
        pl->noHitTimer = 3;
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= pl->floorY) {
            pl->step++;
            pl->action = 0;
            pl->pos[1] = pl->floorY;
            Player_SetMotionPair(pl, 0x30, 0, 0);
            Vib_StartLocal(pl, 1);
            CamShake_ReqA(pl, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->noHitTimer = 0;
            if (pl->input.held & 0x10) {
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
    s16 r;

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
                Notice_Push(0, 0, 0);
                break;
            case 1:
                Notice_Push(0, 0xE, 0);
                break;
            case 2:
                Notice_Push(0, 0x1B, 0);
                break;
            case 3:
                Notice_Push(0, 0, 0);
                break;
            }
        } else {
            Jingle_Play(pl, 0xA);
            Notice_PushItem((char *)gNothingMoreText);
        }
    } else {
        switch (item) {
        case 0x2A4:
            Notice_Push(1, 0x13, item);
            break;
        case 0x2A6:
            Notice_Push(1, 0x14, item);
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
            MsgEvent_Check(0xD);
        }
        r = Pouch_GiveItem(pl, item, pl->gotItem, 3);
        switch (r) {
        case 0:
        case 1:
        case 2:
            if (Item_CarryKind(item) & 0xFF) {
                pl->stateVar1 = 1;
                MsgEvent_Check(0x10);
                if (item == 0x2A3) {
                    Notice_Push(1, 0x14, item);
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
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        switch (variant) {
        case 0:
        case 1:
        case 6:
        case 7:
            Player_SetMotionPair(pl, 0x195, -0xE, 0);
            if (Player_HasSkill(pl, 0x97) == 1) {
                pl->motion[0].step = 2.6f;
                pl->motion[1].step = 2.6f;
            }
            break;
        case 2:
            if (Player_IsLocal(pl) == 1) {
                Notice_Push(0, 0xA, 0);
            }
            Player_SetMotionPair(pl, 0x195, -0xE, 0);
            if (Player_HasSkill(pl, 0x97) == 1) {
                pl->motion[0].step = 2.6f;
                pl->motion[1].step = 2.6f;
            }
            break;
        case 3:
            Player_SetMotionPair(pl, 0x1A3, -6, 0);
            break;
        case 4:
            Player_SetMotionPair(pl, 0x19F, 0, 0);
            if (Player_HasSkill(pl, 0x97) == 1) {
                pl->timer = 0x28;
                pl->motion[0].step = 2.8f;
                pl->motion[1].step = 2.8f;
            }
            break;
        case 5:
            Player_SetMotionPair(pl, 0x1A2, 0, 0);
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
                        Notice_Push(1, 0xE, pl->useItem);
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
                    Player_BeginState(pl, 0, 0xA, 0);
                    break;
                case 3:
                case 5:
                    Player_BeginState(pl, 0, 6, 0);
                    break;
                default:
                    Player_BeginState(pl, 0, 4, 0);
                    break;
                }
            } else {
                Player_UpdateCarryKind(pl);
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_PICKED_UP, 0);
            }
        } else if (Motion_PassedFrame(pl, 0, 100.0f) != 0 && variant == 3 && Player_HasSkill(pl, 0x97) == 1) {
            Player_SetMotionPair(pl, 0x1A3, -8, 0xE0);
        }
        break;
    }
}

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
        Player_SetMoveMode(pl, 1, 0, 0);
        switch (variant) {
        case 0:
        case 2:
        case 3:
            Player_SetMotionPair(pl, 0x19B, -6, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x1A6, -6, 0);
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
                Player_SetMotionPair(pl, 0x195, -4, 0x24);
                break;
            case 1:
                Player_SetMotionPair(pl, 0x1A3, 0, 0x44);
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
                Player_SetMotionPair(pl, 0x19C, -4, 0);
            }
            break;
        case 1:
            if (Motion_ReachedFrame(pl, 0, 294.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x1A7, -2, 0);
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

void Player_StateSpitRoastStart(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        pl->cookTimer = 0;
        pl->cookResult = 0;
        Player_SetMotionPair(pl, 0x320, -4, 0);
        CamCtl_TurnToObj_Unk((u8 *) pl);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST, PL_SET_NO_SYNC);
        }
        break;
    }
}

void Player_StateSpitRoastEnd(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x321, 0, 0);
            break;
        }
        Player_SetMotionPair(pl, 0x322, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_UpdateFishingInput(Player *pl)
{

    if (Player_IsLocal(pl) != 0) {
        if (pl->stateGroup != PL_GROUP_ACTION) {
            Player_BeginState(pl, 0, 4, 0);
            return;
        }
        if (Quest_PhaseIsOver(1) == 1) {
            Player_BeginState(pl, 0, 4, 0);
            return;
        }
        switch (pl->state) {
        case 0x4E:
        case 0x4F:
        case 0x53:
            break;
        case 0x50:
            if (pl->input.pressed & 0x20) {
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
            if (pl->input.pressed & 0x20) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL, 0);
            }
            break;
        }
    }
}

void Player_StateFishWait(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        pl->timer = 0;
        pl->stateVar1 = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x324, 0, 0);
            break;
        }
        Player_SetMotionPair(pl, 0x326, 2, 0);
        break;
    case 1:
        if (((s8) pl->fishTimer != 0) && (pl->stateVar1 == 0)) {
            pl->stateVar1 = 1;
        }
        Player_UpdateFishingInput(pl);
        break;
    }
}

void Player_StateFishCast(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x80000);
        pl->weaponOut = 0;
        pl->fishTimer = 0;
        pl->gotItemId = 0;
        pl->baited = 1;
        Player_SetMotionPair(pl, 0x325, -2, 0);
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
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x80000);
        pl->weaponOut = 0;
        for (i = 0; i < 20; i++, em++) {
            if (em->active != 0 && em->area == pl->area && em->hooked != 0 && em->hookedBy == pl->index) {
                pl->stateVar0 = 1;
            }
        }
        Player_SetMotionPair(pl, 0x327, 2, 0);
        break;
    case 1:
        if (pl->stateVar0 == 0) {
            if (Motion_PassedFrame(pl, 0, 118.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0x328, 2, 0);
                break;
            }
            for (i = 0; i < 20; i++, em++) {
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
                Player_SetMotionPair(pl, 0x32A, 4, 0);
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(pl, pl->useItem, -1);
                }
            }
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 120.0f) != 0 && Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(1, 0xB, pl->gotItemId);
            Jingle_Play(pl, 7);
            Pouch_Add(pl, pl->useItem, -1);
            i = pl->gotItem;
            if ((s16) Pouch_GiveItem(pl, pl->gotItemId, i, 0) == 3) {
                Player_PushNotice3(pl->gotItemId);
            }
            QuestStats_Add(0xD, 1);
        }
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 2, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xD8, 2, 0x24);
        }
        break;
    case 4:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateFishReelEmpty(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x329, 2, 0);
        break;
    case 1:
        if ((Motion_PassedFrame(pl, 0, 120.0f) != 0) && (Player_IsLocal(pl) == 1) && (Quest_PhaseIsOver(1) == 0)) {
            if (variant == 0) {
                Jingle_Play(pl, 0xA);
                Pouch_Add(pl, (s32) pl->useItem, -1);
                Notice_Push(0, 3, 0);
            } else {
                Notice_Push(0, 0xD, 0);
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 2, 0);
        }
        break;
    }
}

u8 SpitMeat_Kind(u16 item)
{
    u8 r = 0;

    switch (item) {
    case 0x294:
        r = 0;
        break;
    case 0x9C:
        r = 1;
        break;
    case 0x9D:
        r = 2;
        break;
    case 0x9E:
        r = 3;
        break;
    case 0x9F:
        r = 4;
        break;
    case 0xA0:
        r = 5;
        break;
    case 0xA4:
        r = 6;
        break;
    case 0xA5:
        r = 7;
        break;
    case 0xA6:
        r = 8;
        break;
    case 0xA7:
        r = 9;
        break;
    case 0xA3:
        r = 10;
        break;
    case 0xA8:
        r = 11;
        break;
    case 0xA9:
        r = 12;
        break;
    }
    return r;
}

void Player_StateSpitRoast(Player *pl, s32 unused)
{
    SpitMeat *m;
    s16 done;
    s16 doneLen;
    s16 gourmet;
    s16 gourmetLen;
    s16 limit;
    s32 j;
    s16 n;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    m = &gSpitMeats[SpitMeat_Kind(pl->useItem)];
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x323, 4, Rand_Next(1) & 0x3F);
        pl->cookTimer = 0;
        pl->cookResult = 0;
        Jingle_Play(pl, m->jingle);
        break;
    case 1:
        pl->cookTimer++;
        done = m->done;
        doneLen = m->doneLen;
        gourmet = m->gourmet;
        gourmetLen = m->gourmetLen;
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
        } else if (pl->cookTimer >= m->rare) {
            pl->cookResult = 1;
        } else {
            pl->cookResult = 0;
        }
        if (SpitMeat_Kind(pl->useItem) == 2) {
            limit = 180;
        } else {
            limit = 330;
        }
        if (Player_IsLocal(pl) == 1 && ((pl->input.pressed & 0x20) || pl->cookTimer >= limit)) {
            Bgm_StopEvent(pl);
            switch (pl->cookResult) {
            case 0:
                if (Quest_PhaseIsOver(1) == 0) {
                    Notice_Push(0, 0xB, 0);
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            case 1:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (m->raw == 0) {
                        Pouch_Add(pl, 0x28, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 0xA2, 1);
                    }
                    Notice_Push(1, 9, m->itemRare);
                    Jingle_Play(pl, 0xE);
                    n = pl->gotItem;
                    if ((s16) Pouch_GiveItem(pl, m->itemRare, n, 0) == 3) {
                        Player_PushNotice3(m->itemRare);
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            case 2:
            case 4:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (m->raw == 0) {
                        Pouch_Add(pl, 0x28, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 0xA2, 1);
                    }
                    if (pl->cookResult == 4) {
                        switch (m->jingle) {
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
                        switch (m->jingle) {
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
                        Notice_Push(1, 9, m->itemGourmet);
                        n = pl->gotItem;
                        if ((s16) Pouch_GiveItem(pl, m->itemGourmet, n, 0) == 3) {
                            Player_PushNotice3(m->itemGourmet);
                        }
                    } else {
                        Notice_Push(1, 9, m->itemDone);
                        n = pl->gotItem;
                        if ((s16) Pouch_GiveItem(pl, m->itemDone, n, 0) == 3) {
                            Player_PushNotice3(m->itemDone);
                        }
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_SUCCESS, 0);
                break;
            case 3:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (m->raw == 0) {
                        Pouch_Add(pl, 0x28, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 0xA2, 1);
                    }
                    Notice_Push(1, 0xA, m->itemBurnt);
                    Jingle_Play(pl, 0x10);
                    n = pl->gotItem;
                    if ((s16) Pouch_GiveItem(pl, m->itemBurnt, n, 0) == 3) {
                        Player_PushNotice3(m->itemBurnt);
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            }
        }
        break;
    }
}

void Player_StateSpotStrike(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x19F, 0, 0);
        if (variant == 0) {
            Quest_StartCountdown((Player *) pl);
            break;
        }
        Eff22_CreateForPlayer(pl, 7U);
        Pouch_Add(pl, (s32) pl->useItem, -1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateBoxMenu(Player *pl, s32 variant)
{
    u8 var_v0;

    pl->noPushTimer = 2;
    pl->noHitTimer = 5;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, 1, 6, 0);
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
            var_v0 = pl->supplyBoxOpen;
        } else {
            var_v0 = pl->deliveryOpen;
        }
        if (var_v0 & 0xFF) {
            if (Quest_PhaseIsOver(1) == 1) {
                goto block_17;
            }
        } else {
block_17:
            pl->noHitTimer = 0;
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateConsume(Player *pl, s32 variant)
{
    u8 stateVar0;

    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 0x196, 4, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x1A0, 4, 0);
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
            Player_BeginState(pl, 0, 4, 0);
            break;
        }
        if (variant == 1) {
            if ((Player_HasSkill(pl, 0xBD) == 1) && (Motion_PassedFrame(pl, 0, 62.0f) != 0)) {
                stateVar0 = pl->stateVar0;
                if ((s32) stateVar0 < 4) {
                    pl->stateVar0 = stateVar0 + 1;
                    pl->stateVar1 = 1;
                    Player_SetMotionPair(pl, 0x1A0, 4, 0x34);
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

void Player_StateApplyItem(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_Use(pl);
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateKneelToPlaceCrouched(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 1, 0, 0);
        Player_SetMotionPair(pl, 0x199, 2, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 20.0f) != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_ITEM_CROUCHED, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
            Player_SetMotionPair(pl, 8, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

static void Player_StartTradeTimer(Player *pl)
{
    if (pl->tradeCooldown <= 0) {
        pl->tradeCooldown = 9000;
    }
}

void Player_StateTrade(Player *pl, s32 variant)
{

    pl->noHitTimer = 3;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, 1, -6, 0);
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
            Player_BeginState(pl, 0, 2, 0);
        }
        break;
    }
}

void Player_StateInstrumentPlay(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x1A4, 4, 0);
            Player_SetFlag(pl, 0x10000);
        } else {
            Player_SetMotionPair(pl, 0x1A8, 4, 0);
        }
        pl->motionTime = 0;
        break;
    case 1:
        switch (variant) {
        case 0:
            if (pl->stateVar0 >= 3 || (Motion_ReachedFrame(pl, 0, 44.0f) != 0 && (pl->input.pressed & 0x40))) {
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

void Player_StateInstrumentEnd(Player *pl, s32 variant)
{
    u16 chance = 1;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        if (variant == 0) {
            chance = 5;
            Player_SetMotionPair(pl, 0x1A5, 2, 0);
        } else {
            Player_SetMotionPair(pl, 0x1A9, 4, 0);
            if (Player_IsLocal(pl) == 1) {
                Item_UseTeamPowder(pl);
            }
            switch (pl->useItem) {
            case 0xB0:
                chance = 12;
                break;
            case 0xB1:
                chance = 12;
                break;
            case 0xB2:
                chance = 8;
                break;
            case 0xB3:
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
                        Notice_Push(1, 0xE, pl->useItem);
                        Pouch_Add(pl, pl->useItem, -1);
                    }
                }
            }
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateSharpen(Player *pl, s32 unused)
{
    s16 n;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x19D, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (Player_HasSkill(pl, 0x26) == 1 && Motion_PassedFrame(pl, 0, 98.0f) != 0) {
            Player_SetMotionPair(pl, 0x19D, 0, 0xEE);
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
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateBinoculars(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 1, 0, 0);
        pl->aimMode = 1;
        pl->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, 7, 2, 0);
            break;
        }
        pl->step++;
        Player_SetMotionPair(pl, 8, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 8, 6, 0);
            Player_UpdateLean(pl, 8);
            break;
        }
        Player_UpdateLean(pl, 7);
        break;
    case 2:
        Player_UpdateLean(pl, 8);
        if (Player_IsLocal(pl) != 0) {
            if (!(pl->input.pressed & 0x40)) {
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

void Player_StateOfferItem(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x3C, 4, 0);
        pl->offerState = 0U;
        break;
    case 1:
        if ((Player_IsLocal(pl) == 1) && ((pl->input.pressed & 0x40) || (pl->offerState != 0))) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM_END, 0);
        }
        break;
    }
}

void Player_StateTakeItemWait(Player *pl, s32 unused)
{
    s32 timer;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, 1, 6, 0);
        }
        pl->offerState = 0U;
        pl->timer = 0;
        break;
    case 1:
        timer = pl->timer + 1;
        pl->timer = timer;
        if ((timer >= 0x259) || (pl->offerState != 0)) {
            if (pl->offerState == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM, 0);
                break;
            }
            Player_BeginState(pl, 0, 2, 0);
        } else {
        }
        break;
    }
}

void Player_StateOfferItemEnd(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0x3D, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateTakeItem(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0x280, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateLanceHop(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];
    u16 a;
    u16 b;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetEvadeFrames(pl);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 0x1E, 0, pl->pos, 1, 0);
        pl->stateVar1 = 0xFF;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 0x3F6, 2, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x3F7, 2, 0);
            break;
        case 2:
        case 3:
            Player_SetMotionPair(pl, 0x3F8, 2, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        case 1:
            if (pl->weaponType == WEAPON_GUNLANCE && Motion_ReachedFrame(pl, 0, 20.0f) == 0) {
                if (variant == 0) {
                    v[0] = 9.0f;
                } else {
                    v[0] = -9.0f;
                }
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_Rotate(d, v, pl->rotMtx);
                pl->pos[0] += d[0];
                pl->pos[2] += d[2];
            }
            break;
        case 3:
            if (Motion_ReachedFrame(pl, 0, 28.0f) == 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = -8.0f;
                Vec_Rotate(d, v, pl->rotMtx);
                pl->pos[0] += d[0];
                pl->pos[2] += d[2];
            }
            break;
        }
        if (Motion_ReachedFrame(pl, 0, pl->motion[0].frameCount - 30.0f) != 0 && Player_IsLocal(pl) == 1) {
            if (pl->input.stickDirPressed & 0x3C) {
                a = pl->input.rightStickAngle + 0x1555;
                switch (pl->weaponType) {
                case WEAPON_LANCE:
                    if (a < 0xAAAC) {
                        pl->stateVar1 = 0;
                    } else {
                        pl->stateVar1 = 1;
                    }
                    break;
                case WEAPON_GUNLANCE:
                    if (a < 0xAAAC) {
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
                if (pl->input.stickDir & 0x3C00) {
                    b = pl->input.stickAngle - 0x1555;
                    if (b >= 0xAAAB) {
                        if (pl->weaponType == WEAPON_GUNLANCE) {
                            pl->targetYaw -= 0x1555;
                        } else {
                            pl->targetYaw -= 0x1000;
                        }
                    } else if (b >= 0x5555) {
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
                Player_BeginState(pl, 0, 6, 0);
            } else {
                Player_BeginState(pl, 0, 8, 0);
            }
        }
        break;
    }
}

void Player_StatePlaceLargeItem(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, 0x1B0, 6, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
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
        Player_SetMotionPair(pl, 0x1B0, 0, 0x4E);
        break;
    }
}

void Player_StatePlaceLargeItemSet(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, 0x1B0, 0, 0x4E);
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_UsePlaced(pl);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
        }
        break;
    }
}

void Player_StateHornItemPlay(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 1;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x1A4, 4, 0);
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
            Player_SetMotionPair(pl, 0x1A5, 2, 0);
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
                Notice_Push(1, 0xE, pl->useItem);
                Pouch_Add(pl, pl->useItem, -1);
            }
        } else if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar0--;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

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
        Player_BeginState(pl, 0, 4, 0);
        break;
    }
}

void Player_StateSharedAttack(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x32, 4, 0);
        Eff00_Create(pl, 5U);
        Player_SetMoveMode(pl, 0, 1, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

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

void Player_StateKnockdownFront(Player *pl)
{
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0xCA, 0, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xD8, 6, 0x22);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

void Player_StateGuardRecoil(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x586, 2, 0);
        } else if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            Player_SetMotionPair(pl, 0x58B, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0x587, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Sound_PlayPlayerSe(pl, 0x67, 0, pl->pos, 1, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        Vib_StartLocal(pl, 1);
        Player_SetFlag(pl, 2);
        Player_SetFlag(pl, 0x8000);
        pl->weaponOut = 1;
        break;
    case 1:
        v[2] = 0.0f;
        if (variant == 0) {
            if (Motion_ReachedFrame(pl, 0, 30.0f) == 0) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                switch (pl->weaponType) {
                case WEAPON_LANCE:
                case WEAPON_GUNLANCE:
                case WEAPON_HEAVY_BOWGUN:
                    v[2] = -15.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                case WEAPON_GREAT_SWORD:
                case WEAPON_LONG_SWORD:
                case WEAPON_SWORD_SHIELD:
                    v[2] = -20.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                }
                Vec_Rotate(d, v, pl->rotMtx);
                pl->pos[0] += d[0];
                pl->pos[2] += d[2];
            }
        } else {
            v[0] = 0.0f;
            v[1] = 0.0f;
            switch (pl->weaponType) {
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
            case WEAPON_HEAVY_BOWGUN:
                if (Motion_ReachedFrame(pl, 0, 90.0f) == 0) {
                    v[2] = -16.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
                if (Motion_ReachedFrame(pl, 0, 104.0f) == 0) {
                    v[2] = -22.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case WEAPON_SWORD_SHIELD:
                if (Motion_ReachedFrame(pl, 0, 94.0f) == 0) {
                    v[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 94.0f)) / 360.0f);
                }
                break;
            }
            Vec_Rotate(d, v, pl->rotMtx);
            pl->pos[0] += d[0];
            pl->pos[2] += d[2];
        }
        if (pl->motion[0].playing == 0) {
            switch (pl->weaponType) {
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
                break;
            case WEAPON_SWORD_SHIELD:
                Player_BeginState(pl, 0, 6, 0);
                break;
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
            case WEAPON_HEAVY_BOWGUN:
            default:
                Player_BeginState(pl, 0, 0xA, 0);
                break;
            }
        }
        break;
    }
}

void Player_StateBlownBack(Player *pl)
{
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];
    f32 y;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, 0xCD, 0, 0x10);
        Em_GetBonePos(pl, 2, v);
        if (pl->pos[1] < v[1]) {
            pl->pos[1] = v[1];
        }
        Player_SetMoveMode(pl, 2, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = -11.17f;
        Vec_Rotate(d, v, m);
        pl->vel[0] = 2.0f * d[0];
        pl->vel[2] = 2.0f * d[2];
        pl->vel[1] = 6.0f;
        Player_StartJump(pl, 0xC);
        pl->vel[1] = 0.0f;
        pl->accel[1] = -1.0f / 3.0f;
        pl->timer = 16;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && pl->motionId[0] == 0xCD) {
            Player_SetMotionPair(pl, 0xD6, 0, 0);
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
            Sound_PlayArmorRattle(pl, 3);
            Player_SetMotionPair(pl, 0xCE, 0, 0);
            pl->action = 3;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->hp > 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0xCF, 2, 0);
            } else if (Player_IsLocal(pl) == 1) {
                Player_RequestStateSynced(pl, PL_GROUP_FAINT, PL_FAINT_FACE_UP, 0);
            } else {
                pl->step = 4;
                Player_SetFlag(pl, 0x20000);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
            pl->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

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
                Player_SetMotionPair(pl, 0x584, 3, 0);
                break;
            case WEAPON_SWORD_SHIELD:
                Player_SetMotionPair(pl, 0x3ED, 2, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_SetMotionPair(pl, 0x3ED, 2, 0);
                break;
            }
        } else {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            default:
            case WEAPON_LONG_SWORD:
                Player_SetMotionPair(pl, 0x584, 6, 0x28);
                break;
            case WEAPON_SWORD_SHIELD:
            case WEAPON_LANCE:
                Player_SetMotionPair(pl, 0x3EE, 2, 0);
                break;
            case WEAPON_GUNLANCE:
                if (variant == 1) {
                    Player_SetMotionPair(pl, 0x3EE, 2, 0);
                } else {
                    Player_SetMotionPair(pl, 0x3ED, 4, 4);
                }
                break;
            case WEAPON_HEAVY_BOWGUN:
                Player_SetMotionPair(pl, 0x58A, 2, 0);
                break;
            }
        }
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x8000);
        pl->weaponOut = 1;
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0) {
            if (pl->motionTime > 18 && !(pl->input.held & 0x80)) {
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_END, 0);
                break;
            }
        }
        switch (pl->weaponType) {
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
        case WEAPON_SWORD_SHIELD:
            if (pl->motionId[0] == 0x3ED && pl->motion[0].playing == 0) {
                Player_SetMotionPair(pl, 0x3EE, 2, 0);
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

void Player_StateRoarStrong(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, 0xE3, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
            pl->noHitTimer = 6;
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 40.0f) == 0 && pl->motionTime % 7 == 0) {
            Effect0D_SpawnForPlayer(pl, 5, 9);
            Effect0D_SpawnForPlayer(pl, 8, 9);
        }
        if (Motion_ReachedFrame(pl, 0, 64.0f) == 0) {
            pl->noHitTimer = 2;
        }
        break;
    }
}

void Player_StateBlownForward(Player *pl)
{
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];
    f32 y;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, 0xD0, 0, 0);
        Em_GetBonePos(pl, 2, v);
        if (pl->pos[1] < v[1]) {
            pl->pos[1] = v[1];
        }
        pl->targetYaw = pl->rot[1];
        Player_SetMoveMode(pl, 2, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = (u16) pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 11.5f;
        Vec_Rotate(d, v, m);
        pl->vel[0] = 2.0f * d[0];
        pl->vel[2] = 2.0f * d[2];
        pl->vel[1] = 0.0f;
        pl->accel[1] = -0.25f;
        pl->timer = 16;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && pl->motionId[0] == 0xD0) {
            Player_SetMotionPair(pl, 0xDC, 0, 0);
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
            Sound_PlayArmorRattle(pl, 3);
            Player_SetMotionPair(pl, 0xD1, 0, 0);
            pl->action = 3;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->hp > 0) {
                pl->step++;
                Player_SetMotionPair(pl, 0xD8, 0, 0);
            } else if (Player_IsLocal(pl) == 1) {
                Player_RequestStateSynced(pl, PL_GROUP_FAINT, PL_FAINT_FACE_DOWN, 0);
            } else {
                pl->step = 4;
                Player_SetFlag(pl, 0x20000);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

void Player_StateKnockdownBack(Player *pl)
{
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0xCC, 0, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xCF, 6, 0x3A);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

void Player_StateTossed(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    f32 m[16];
    f32 z;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0xD5, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0xDB, 0, 0);
        }
        Em_GetBonePos(pl, 2, v);
        if (pl->pos[1] < v[1]) {
            pl->pos[1] = v[1];
        }
        Player_SetMoveMode(pl, 2, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        if (variant != 0) {
            z = 7.0f;
        } else {
            z = -7.0f;
        }
        v[2] = z;
        Vec_Rotate(d, v, m);
        pl->vel[0] = 2.0f * d[0];
        pl->vel[2] = 2.0f * d[2];
        pl->vel[1] = 20.0f;
        pl->accel[1] = -8.0f / 11.0f;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && (pl->motionId[0] == 0xD5 || pl->motionId[0] == 0xDB)) {
            if (variant == 0) {
                Player_SetMotionPair(pl, 0xD6, 0, 0);
            } else {
                Player_SetMotionPair(pl, 0xDC, 0, 0);
            }
        }
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->step++;
            Sound_PlayPlayerSe(pl, 0x42, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, 3);
            Player_SetMotionPair(pl, 0xD7, 0, 0);
            pl->pos[1] = pl->floorY;
            pl->action = 3;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->rot[1] = (pl->rot[1] + 0x8000) & 0xFFFF;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

void Player_StateGuardEnd(Player *pl)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
        default:
        case WEAPON_LONG_SWORD:
            Player_SetMotionPair(pl, 0x586, 2, 0);
            break;
        case WEAPON_SWORD_SHIELD:
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
            Player_SetMotionPair(pl, 0x3EF, 2, 0);
            break;
        case WEAPON_HEAVY_BOWGUN:
            Player_SetMotionPair(pl, 0x58D, 2, 0);
            break;
        }
        Player_SetFlag(pl, 0x8000);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 2, 0);
        }
        break;
    }
}

void Player_StateFlinch(Player *pl, s32 variant)
{
    f32 v[3];
    s32 ang[3];
    f32 m[16];
    f32 z;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        if (pl->weaponOut != 0) {
            Player_SetMotionPair(pl, 0x4BB, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0xC9, 0, 0);
        }
        Player_SetMoveMode(pl, 0, 0, 1);
        ang[0] = 0;
        ang[2] = 0;
        ang[1] = pl->targetYaw;
        Mtx_FromAnglesXYZ(ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        if (variant != 0) {
            z = 4.0f;
        } else {
            z = -4.0f;
        }
        v[2] = z;
        Vec_Rotate(&pl->vel[0], v, m);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 16.0f) == 0) {
            Player_Move(pl);
        }
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateGuardPush(Player *pl)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            Player_SetMotionPair(pl, 0x58A, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0x585, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Sound_PlayPlayerSe(pl, 0x67, 0, &pl->pos[0], 1, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        Player_SetFlag(pl, 0x8000U);
        Vib_StartLocal(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                Player_BeginState(pl, 0, 6, 0);
                break;
            }
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        } else {
        }
        break;
    }
}

void Player_StateRoar(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, 0xDA, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 28.0f) != 0 && variant == 1) {
            pl->step = 3;
            Player_SetMotionPair(pl, 0xE2, 6, 0x14);
        } else if (Motion_ReachedFrame(pl, 0, 40.0f) == 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 42.0f)) / 360.0f);
            Vec_Rotate(d, v, pl->rotMtx);
            pl->pos[0] += d[0];
            pl->pos[2] += d[2];
            if (pl->motionTime % 7 == 0) {
                Effect0D_SpawnForPlayer(pl, 5, 9);
                Effect0D_SpawnForPlayer(pl, 8, 9);
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
                Player_SetMotionPair(pl, 0xE2, 2, 0);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 0xC, 0);
        }
        break;
    }
}

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
        Player_SetMotionPair(pl, 0xD3, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        pl->timer = 0;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar0++;
            if ((s32) pl->stateVar0 >= (s32) pl->stateVar1) {
                pl->step++;
                Player_SetMotionPair(pl, 0xD4, 2, 0);
                break;
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 0xC, 0);
        }
        break;
    }
}

void Player_StateFlinchCarrying(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        if (pl->weaponOut != 0) {
            Player_SetMotionPair(pl, 0x4BB, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0xC9, 0, 0);
        }
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

static void Player_ClearStun(Player *pl)
{
    pl->status5Delay = 0;
    pl->status5Buildup = 0;
    pl->itemUseDelay = 0x1E;
}

void Player_StateStun(Player *pl, s32 unused)
{
    s8 need;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        pl->weaponOut = 0;
        pl->noFlinchTimer = 0;
        Player_SetMotionPair(pl, 0x1A, 4, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        pl->timer = 300;
        pl->stateVar0 = 0;
        pl->stateVar1 = 5;
        pl->status5Delay = 0;
        pl->status5Buildup = 0;
        Player_ClearMash(pl);
        pl->targetYaw = pl->rot[1];
        StageObj_Stun_CreatePlayer(pl, 0);
        break;
    case 1:
        if (--pl->timer <= 0) {
            pl->stateVar0 = 0;
            pl->stateVar1--;
            pl->timer = 300;
        } else if ((pl->input.pressed & 0x3FF) || (pl->input.stickDirPressed & 0x3C3C)) {
            pl->stateVar0++;
            need = gStunMashNeed[pl->stateVar1];
            if (Player_HasSkill(pl, 9) == 1) {
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
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateReact14(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x197, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateStatus2(Player *pl, s32 variant)
{
    f32 v[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 0, 1);
        pl->weaponOut = 0;
        pl->status2Buildup = 0;
        Player_ClearStun(pl);
        Player_ClearMash(pl);
        Player_SetMotionPair(pl, 0xE1, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->timer = 240;
            if (Player_HasSkill(pl, 4) == 1) {
                pl->timer /= 2;
            }
            if (Player_HasSkill(pl, 6) == 1) {
                pl->timer *= 2;
            }
            Player_SetMotionPair(pl, 0xD2, 0, 0x9A);
        }
        break;
    case 2:
        if (pl->stateVar1 > 0) {
            pl->stateVar1--;
        }
        if (((pl->input.pressed & 0x3FF) || (pl->input.stickDirPressed & 0x3C3C)) && pl->stateVar1 == 0) {
            pl->stateVar1 = 3;
            pl->timer--;
        }
        if (--pl->timer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xCF, 0, 0);
            break;
        }
        v[0] = 0.0f;
        v[1] = 20.0f;
        v[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 20, v, 0.8f);
        }
        Player_AddStamina(pl, 2);
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateStatus4(Player *pl, s32 variant)
{

    pl->stateVar0++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetMoveMode(pl, 0, 0, 1);
        pl->weaponOut = 0;
        pl->status4Buildup = 0;
        pl->status2Buildup = 0;
        Player_ClearStun(pl);
        Player_ClearMash(pl);
        if (variant == 0) {
            Player_SetMotionPair(pl, 0xE0, 4, 0);
            pl->motionTime = 0;
            pl->stateVar0 = 0;
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
            pl->recoverTimer = 150;
            if (Player_HasSkill(pl, 1) == 1) {
                pl->recoverTimer /= 2;
            }
            if (Player_HasSkill(pl, 3) == 1) {
                pl->recoverTimer *= 2;
            }
            pl->noHitTimer = 2;
            break;
        }
        pl->step = 2;
        if (pl->recoverTimer <= 0) {
            pl->recoverTimer = 0;
        }
        Player_SetMotionPair(pl, 0xD2, 2, 0x96);
        break;
    case 1:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xD2, 2, 0x96);
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
        if (((pl->input.pressed & 0x3FF) || (pl->input.stickDirPressed & 0x3C3C)) && pl->stateVar1 == 0) {
            pl->stateVar1 = 3;
            pl->recoverTimer--;
        }
        if (--pl->recoverTimer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xCF, 0, 0);
            pl->noHitTimer = 2;
            break;
        }
        if (pl->motionId[0] == 0xD2 && Motion_PassedFrame(pl, 0, 170.0f) != 0 && !(Rand_Next(1) & 0x3F)) {
            Player_SetMotionPair(pl, 0xDF, 0, 0);
        } else if (pl->motionId[0] == 0xDF && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, 0xD2, 4, 0xAA);
        }
        if (pl->step < 3 && !(pl->stateVar0 & 0x1F)) {
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
        }
        break;
    case 3:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

void Player_StateGuardDraw(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x3FA, 2, 1);
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x8000);
        pl->weaponOut = 1;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        }
        break;
    }
}

void Player_UpdateMashEscape(Player *pl, s32 caller)
{
    s16 n;

    if (Player_IsLocal(pl) != 0) {
        if (pl->mashTimer == 0) {
            if ((pl->input.pressed & 0x3FF) || (pl->input.stickDirPressed & 0x3C3C)) {
                if (pl->mashLeft > 0) {
                    n = 10;
                    if (Player_HasSkill(pl, 0x10) == 1) {
                        n *= 2;
                    }
                    if (Player_HasSkill(pl, 0x12) == 1) {
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
            if ((pl->input.held & 0x10) && pl->dashDelay == 0) {
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

static void Player_ClearMash(Player *pl)
{
    pl->mashLeft = 0;
    pl->mashBuildup = 0;
    pl->mashTimer = 0;
}

void Player_StateMashIdle(Player *pl, s32 variant)
{
    if (pl->motionId[0] != 0x33) {
        Player_SetMotionPair(pl, 0x33, 4, 0);
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
        Player_SetMoveMode(pl, 0, 0, 1);
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

void Player_StateMashFree(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        Player_SetMoveMode(pl, 0, 0, 1);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x197, 6, 0);
        StageObj_K1B_Create(0, (s16 *) pl);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateQuake(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x1A, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 1);
        if (variant == 0) {
            pl->timer = 0x28;
            break;
        }
        pl->timer = 0x50;
        break;
    case 1:
        if (--pl->timer <= 0) {
            Player_BeginState(pl, 0, 0xC, 0);
        }
        break;
    }
}

void Player_StateMashWalk(Player *pl, s32 unused)
{
    f32 v[3];
    f32 d[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_SetMotionPair(pl, 0x34, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 2.0f;
        Vec_Rotate(d, v, pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[2] += d[2];
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateMashEscape(pl, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        StageObj_K1B_Create(1, (s16 *) pl);
    }
}

void Player_StateMashRun(Player *pl, s32 unused)
{
    f32 v[3];
    f32 d[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->gender == 0) {
            Player_SetMotionPair(pl, 0x35, 4, 0);
        } else {
            Player_SetMotionPair(pl, 0x3B, 4, 0);
        }
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x2000200);
        break;
    case 1:
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) != 0) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_JUMP_OFF, 0);
            return;
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 5.0f;
        Vec_Rotate(d, v, pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[2] += d[2];
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateMashEscape(pl, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        StageObj_K1B_Create(1, (s16 *) pl);
    }
}

void Player_StateMashAirborne(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];

    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = 2;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(pl, 0x38, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 0x13);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x38, 8, 0x20);
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
        Player_SetMoveMode(pl, 2, 0, 0);
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
        v[0] = pl->vel[0];
        v[1] = pl->vel[1];
        v[2] = pl->vel[2];
        Vec_Rotate(d, v, pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[1] += d[1];
        pl->pos[2] += d[2];
        if (pl->vel[1] < 0.0f) {
            pl->timer++;
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = 0;
                pl->rideState = 0;
                if (pl->timer >= 50) {
                    StageObj_K1B_Create(0, (s16 *) pl);
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
        StageObj_K1B_Create(1, (s16 *) pl);
    }
}

void Player_StateMashLand(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x39, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0x3A, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_IDLE, PL_SET_NO_SYNC);
        }
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        StageObj_K1B_Create(1, (s16 *) pl);
    }
}

void Player_StateInhaled(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];
    f32 t[3];
    s32 ang[3];
    f32 mtx[16];
    Em *m;
    s16 i;
    s16 found;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, 0xD5, 0, 0);
        pl->noHitTimer = 10;
        Em_GetBonePos(pl, 2, v);
        if (pl->pos[1] < v[1]) {
            pl->pos[1] = v[1];
        }
        Player_SetMoveMode(pl, 2, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        found = 0;
        m = gEms;
        for (i = 0; i < 20; i++, m++) {
            if (m->active && m->visible && (Player_InCurrentArea(m) & 0xFF) && m->type == 0x3A) {
                found = 1;
                Em_BonePos(m, 0x24, t);
                pl->rot[1] = Vec_Heading(pl->pos, t) + 0x4000;
                pl->targetYaw = pl->rot[1];
            }
        }
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, mtx);
        if (found == 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -7.0f;
            Vec_Rotate(d, v, mtx);
            pl->vel[0] = 2.0f * d[0];
            pl->vel[2] = 2.0f * d[2];
            pl->vel[1] = 20.0f;
            pl->accel[1] = -8.0f / 11.0f;
        } else {
            d[0] = (t[0] - pl->pos[0]) / 50.0f;
            d[1] = (t[1] - pl->pos[1]) / 50.0f;
            d[2] = (t[2] - pl->pos[2]) / 50.0f;
            pl->vel[0] = d[0];
            pl->vel[2] = d[2];
            if (d[1] <= 15.0f) {
                d[1] = 15.0f;
            }
            pl->vel[1] = 2.0f * d[1];
            pl->accel[1] = -8.0f / 11.0f;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0 && (pl->motionId[0] == 0xD5 || pl->motionId[0] == 0xDB)) {
            if (variant == 0) {
                Player_SetMotionPair(pl, 0xD6, 0, 0);
            } else {
                Player_SetMotionPair(pl, 0xDC, 0, 0);
            }
        }
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->noHitTimer = 2;
            pl->step++;
            Sound_PlayPlayerSe(pl, 0x42, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, 3);
            Player_SetMotionPair(pl, 0xD7, 0, 0);
            pl->pos[1] = pl->floorY;
            pl->action = 3;
        }
        break;
    case 2:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->rot[1] = (pl->rot[1] + 0x8000) & 0xFFFF;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

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
    Player_ClearFlag(pl, 0x80000);
    state = pl->state;
    if ((state != 0x11) && (state != 0x12)) {
        carry = pl->carry;
        if (carry & 0xF) {
            pl->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(pl, 4);
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
        Player_BeginState(pl, 0, 4, 0);
        break;
    }
}

extern s32 gRewardZeroText;
extern s32 gFaintedText;
extern s32 gRewardZeroText2;
extern s32 gNoMoreReviveText;
void Player_StateFaint(Player *pl, s32 variant)
{
    s8 f;
    s8 r;
    u8 k;

    pl->noPushTimer = 10;
    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    Player_ClearFlag(pl, 0x80000);
    if (pl->carry & 0xF) {
        pl->carry &= 0xF0;
        StageObj_K0C_CreateAtPlayer(pl, 4);
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, 0xD2, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, 0xD2, 0, 0x8C);
            break;
        case 2:
            Player_SetMotionPair(pl, 0xD1, 0, 0x8C);
            Player_SetFlag(pl, 0x20000);
            break;
        }
        Vib_StartLocal(pl, 3);
        Player_UpdateMotionTime(pl, 0);
        Player_PlaySeAtSelf((u8 *) pl, 0x22);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, 0x8000);
        pl->weaponOut = 0;
        switch (*(u8 *) &pl->stateRand) {
        default:
            f = 0;
            break;
        case 1:
            f = 1;
            break;
        }
        if (pl->side == 0) {
            r = Quest_PlayerFainted(pl, f);
            if (r != 0 && r != 3) {
                pl->step++;
            }
        } else {
            r = 2;
            Versus_MonsterDown((Em *) pl);
        }
        if (pl->faints <= 98) {
            pl->faints++;
        } else {
            pl->faints = 99;
        }
        if (Player_IsLocal(pl) == 0) {
            Notice_Push(3, 0, pl->index);
            if (r <= 1) {
                Quest_ShowValue();
            }
            if (gQuest.reward <= 0 && r == 1) {
                Notice_PushItem((char *)gRewardZeroText);
            }
        } else {
            CamReq_Mode0A();
            Notice_PushItem((char *)gFaintedText);
            if (r <= 1) {
                Quest_ShowValue();
            }
            if (pl->step <= 1) {
                Quest_ResetMusicIfIdle();
                func_367A90();
            } else {
                if (r == 1) {
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
            Player_SetMotionPair(pl, 0xDF, 0, 0);
        } else if (pl->motionId[0] == 0xDF && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, 0xD2, 4, 0xA4);
        }
        if (Quest_PhaseIsOver(2) == 1) {
            pl->step++;
            break;
        }
        if (pl->motionTime == 300 && Player_IsLocal(pl) == 1) {
            switch (gScene.area) {
            case 0x7C:
            case 0x7E:
            case 0x8A:
                Notice_Push(0, 0x1F, 0);
                break;
            default:
                Notice_Push(0, 7, 0);
                break;
            }
        }
        if (pl->motionTime == 600 && Player_IsLocal(pl) == 1) {
            switch (gScene.area) {
            case 0x7C:
                k = 0x54;
                pl->areaMoveState = 3;
                break;
            case 0x7E:
                k = 0x7D;
                pl->areaMoveState = 3;
                break;
            case 0x8A:
                k = 0x81;
                pl->areaMoveState = 3;
                break;
            default:
                k = gScene.campArea;
                pl->areaMoveState = 2;
                break;
            }
            pl->startPos[0] = gStartPos[k][0];
            pl->startPos[1] = D_43F0A4[k][0];
            pl->startPos[2] = D_43F0A8[k][0];
            pl->startAngle = gStartAngle[k];
            pl->nextArea = k;
            QSync_SendPlayer(pl, 4, 0);
        }
        break;
    case 2:
        break;
    case 3:
        if (pl->motionId[0] == 0xD2 && Motion_PassedFrame(pl, 0, 164.0f) != 0 && (Rand_Next(1) & 0x3F) == 0) {
            Player_SetMotionPair(pl, 0xDF, 0, 0);
        } else if (pl->motionId[0] == 0xDF && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, 0xD2, 4, 0xA4);
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
        Player_BeginState(pl, 0, 4, 0);
        break;
    }
}

void Player_StateCart(Player *pl, s32 variant)
{
    Em *m;
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
            if (gScene.emKind[i] == 0x12 && (m = EmSlot_Alloc()) != NULL) {
                m->modelSlot = i;
                m->type = gScene.emKind[i];
                m->area = gScene.area;
                m->variant = 0;
                m->variant = 0xFF;
                m->cartPlayer = pl->index;
                GmEm_Task(m);
                m->posX = pl->pos[0];
                m->posY = pl->pos[1];
                m->posZ = pl->pos[2];
                m->rotY = pl->rot[1];
                pl->cart = m;
                kind = gScene.emKind[i];
                break;
            }
        }
        if (kind != 0x12) {
            Player_BeginState(pl, 0, 4, 0);
        } else {
            pl->stateVar1 = 0;
            Player_SetMotionPair(pl, 0xDD, 0, 0);
        }
        if (Player_IsLocal(pl) == 1) {
            CamReq_Mode01();
        }
        break;
    case 1:
        m = pl->cart;
        if (m == NULL) {
            Player_BeginState(pl, 0, 4, 0);
            break;
        }
        Em_BonePos(m, 0x1D, &pl->pos[0]);
        if (m->motionId[0] == 0x3EB) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_CART_DROP, PL_SET_NO_SYNC);
        }
        pl->rot[1] = m->rotY;
        pl->targetYaw = pl->rot[1];
        break;
    }
}

void Player_StateCartDrop(Player *pl, s32 unused)
{
    f32 floorY;

    pl->noPushTimer = 0xA;
    pl->noHitTimer = 0xA;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0xDE, 0, 0);
        pl->action = 2;
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
            Player_SetMotionPair(pl, 0xD1, 0, 0x46);
            pl->action = 0;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 6, 0);
        }
        break;
    }
}

void Player_StateQuestStart(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->unk7E5 = 1;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x197, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateQuestEnd(Player *pl, s32 unused)
{
    u8 carry;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x26F, 6, 0);
        pl->motionTime = 0;
        carry = pl->carry;
        if (carry & 0xF) {
            pl->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(pl, 4);
        }
        if (Player_IsLocal(pl) == 1) {
            CamReq_Mode0A();
        }
        
    case 1:
        break;
    }
}

void Player_StateRevive(Player *pl, s32 unused)
{
    pl->noHitTimer = 4;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->weaponOut = 0;
        if (pl->motionId[0] == 0xD1) {
            Player_SetMotionPair(pl, 0xD8, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0xCF, 0, 0);
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->noHitTimer = 0x96;
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

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
    Player_ClearFlag(pl, 0x80000);
    pl->pos[1] = pl->floorY;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0xD2, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_PlaySeAtSelf((u8 *) pl, 0x22);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, 0x8000);
        pl->weaponOut = 0;
        pl->timer = 0x12C;
        carry = pl->carry;
        if (carry & 0xF) {
            pl->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(pl, 4);
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
    Player_ClearFlag(pl, 0x80000);
    pl->pos[1] = pl->floorY;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x15, 0, 0);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, 0x8000);
        pl->weaponOut = 0;
        pl->timer = 0x12C;
        carry = pl->carry;
        if (carry & 0xF) {
            pl->carry = carry & 0xF0;
            StageObj_K0C_CreateAtPlayer(pl, 4);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, 5, 4, 0x90);
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
        Player_BeginState(pl, 0, 4, 0);
        break;
    }
}

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
    pl->carry = (pl->carry & 0xF0) | 4;
}

void Player_UpdateCarryInput(Player *pl, s32 caller)
{
    f32 buf[3];
    ItemStack st;
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
                if ((pl->input.held & 0x10) && pl->dashDelay == 0) {
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
            if (pl->input.pressed & 0x40) {
                if (pl->input.stickTilt >= 85) {
                    if (pl->stamina >= 75) {
                        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                    }
                } else {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, 0);
                }
                return;
            }
            if (pl->input.stickDirPressed & 0x3C) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
                break;
            }
            if ((pl->input.pressed & 0x200) && pl->itemSelect == 0) {
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DROP, 0);
            }
            Player_HoldSpecialInput(pl, 2);
            if (pl->input.pressed & 0x20) {
                switch (Player_FindActionSpot((Player *)pl, (f32 *)buf, &ang, (u8 *)&k)) {
                case 7:
                    if (Quest_PhaseIsOver(0) == 1) {
                        break;
                    }
                    for (i = 0; i < 3; i++) {
                        if (Quest_GetDeliveryTarget(pl, i, &st) == 2 && st.item == (u16) Pouch_FindCarryItem(pl)) {
                            Quest_DeliverItem(pl, st.item, 1);
                        }
                    }
                    if ((u16) Pouch_FindCarryItem(pl) == 0xFFFF) {
                        Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DELIVER, 0);
                    }
                    break;
                case 8:
                    if ((u16) Pouch_FindCarryItem(pl) == 0x29C) {
                        pl->carry &= 0xF0;
                        Pouch_Add(pl, 0x29C, -10);
                        pl->targetYaw = ang;
                        pl->stateRand = k;
                        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CANNON_FIRE, PL_SET_KEEP_RAND);
                    }
                    break;
                default:
                    if (Player_NearEmKind10(pl) & 0xFF) {
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
            Player_SetMotionPair(pl, 0x33, 4, 0);
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

void Player_StateCarryWalk(Player *pl, s32 unused)
{
    f32 v[3];
    f32 d[3];

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_SetMotionPair(pl, 0x34, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        if (func_3B5440((Em *) pl) != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 2.0f;
            Vec_Rotate(d, v, pl->rotMtx);
            pl->pos[0] += d[0];
            pl->pos[2] += d[2];
        }
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateCarryInput(pl, 1);
        break;
    }
}

void Player_StateCarryRun(Player *pl, s32 unused)
{
    f32 v[3];
    f32 d[3];

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->gender == 0) {
            Player_SetMotionPair(pl, 0x35, 4, 0);
        } else {
            Player_SetMotionPair(pl, 0x3B, 4, 0);
        }
        pl->timer = 0x14;
        pl->dashDelay = 0x1E;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
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
        if (func_3B5440((Em *) pl) != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 5.0f;
            Vec_Rotate(d, v, pl->rotMtx);
            pl->pos[0] += d[0];
            pl->pos[2] += d[2];
        }
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateCarryInput(pl, 1);
        break;
    }
}

void Player_StateCarryPutDown(Player *pl, s32 variant)
{
    u16 item;

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 1) {
            Player_SetMotionPair(pl, 0x37, 4, 0);
        } else {
            Player_SetMotionPair(pl, 0x36, 4, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, variant == 1 ? 4 : 0x72) != 0) {
            pl->step++;
            pl->carry &= 0xF0;
            if (variant != 2) {
                StageObj_K0C_CreateAtPlayer(pl, 4);
            }
            if (variant == 0 && Player_IsLocal(pl) == 1) {
                item = Pouch_FindCarryItem(pl);
                if (item != 0xFFFF) {
                    Notice_Push(1, 0xC, item);
                    Pouch_Add(pl, item, -100);
                }
            }
            break;
        }
        Player_UpdateCarryKind(pl);
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 8, 0);
        }
        break;
    }
}

void Player_StateCarryAirborne(Player *pl, s32 variant)
{
    f32 v[3];
    f32 d[3];
    s16 hi;
    s16 lo;

    Player_UpdateCarryKind(pl);
    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = 2;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(pl, 0x38, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 0x13);
            break;
        case 1:
            Player_SetMotionPair(pl, 0x38, 8, 0x20);
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
        Player_SetMoveMode(pl, 2, 0, 0);
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
        v[0] = pl->vel[0];
        v[1] = pl->vel[1];
        v[2] = pl->vel[2];
        Vec_Rotate(d, v, pl->rotMtx);
        pl->pos[0] += d[0];
        pl->pos[1] += d[1];
        pl->pos[2] += d[2];
        if (pl->vel[1] < 0.0f) {
            pl->timer++;
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = 0;
                pl->rideState = 0;
                if (func_3B5440((Em *) pl) == 1) {
                    hi = 60;
                    lo = 40;
                } else {
                    hi = 50;
                    lo = 30;
                }
                if (pl->timer >= hi) {
                    pl->carry &= 0xF0;
                    StageObj_K0C_CreateAtPlayer(pl, 4);
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                } else if (pl->timer >= lo) {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_LAND_HARD, PL_SET_NO_SYNC);
                } else {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_LAND, PL_SET_NO_SYNC);
                }
            }
        }
        break;
    }
}

void Player_StateCarryLand(Player *pl, s32 variant)
{
    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, 0x39, 0, 0);
        } else {
            Player_SetMotionPair(pl, 0x3A, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_IDLE, PL_SET_NO_SYNC);
        }
        break;
    }
}

void Player_StateCarryHandOver(Player *pl, s32 unused)
{
    pl->noHitTimer = 3;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateCarryKind(pl);
        if (pl->motionId[0] != 0x33) {
            Player_SetMotionPair(pl, 0x33, 4, 0);
        }
        pl->tradeMsgTimer = 0x3C;
        pl->timer = 0x3C;
        break;
    case 1:
        Player_UpdateCarryKind(pl);
        if ((Pouch_FindCarryItem(pl) & 0xFFFF) == 0xFFFF) {
            pl->step++;
            Player_SetMotionPair(pl, 0x36, 4, 0);
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 114.0f) != 0) {
            pl->step++;
            pl->carry &= 0xF0;
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
            Player_BeginState(pl, 0, 2, 0);
        }
        break;
    }
}

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
        Player_BeginState(pl, 0, 4, 0);
        break;
    }
}

void Player_StateGestureLieDown(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0xD2, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if (((s32) pl->input.stickTilt >= 0x28) || (Quest_PhaseIsOver(1) == 1)) {
                Player_RequestState(pl, PL_GROUP_GESTURE, PL_GESTURE_GET_UP, 0);
                break;
            }
            if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        } else {
        }
        break;
    }
}

void Player_StateGestureGetUp(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0xCF, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateGestureScout(Player *pl, s32 variant)
{
    f32 pos[3];
    s32 d;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x26C, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            } else if (func_379E40() == 1 && pl->unk8FE == 0) {
                func_379E50(pos, pl->area);
                d = (u16) (pl->rot[1] - (u16) (Vec_Heading(pos, pl->pos) + 0x4000));
                if (d <= 0x1800 || d >= 0xE800) {
                    pl->unk8FE = 1;
                    pl->actDetectTimer = 300;
                }
            }
        }
        break;
    }
}

void Player_StateGesture03(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x26F, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 && Motion_ReachedFrame(pl, 0, 90.0f) != 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_RequestState(pl, PL_GROUP_GESTURE, PL_GESTURE_03_END, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGesture03End(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x270, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetFlag(pl, 0x20000);
            Player_SetMotionPair(pl, 0xCF, -4, 0x3C);
        }
        break;
    case 2:
        Player_SetFlag(pl, 0x20000);
        if (pl->motion[0].playing == 0) {
            Player_ClearFlag(pl, 0x20000);
            Player_BeginState(pl, 0, 4, 0);
        }
        break;
    }
}

void Player_StateGesture05(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x274, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Motion_PassedFrame(pl, 0, 48.0f) != 0) {
            Eff00_Create(pl, 0x60);
        }
        if (Motion_PassedFrame(pl, 0, 68.0f) != 0) {
            Eff00_Create(pl, 0x61);
        }
        if (Motion_PassedFrame(pl, 0, 168.0f) != 0) {
            Eff00_Create(pl, 0x61);
        }
        if (Motion_PassedFrame(pl, 0, 190.0f) != 0) {
            Eff00_Create(pl, 0x60);
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 0xA, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGesture06(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x278, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 0xA, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGesture07(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x26E, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 0x4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGesture08(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x273, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGestureOneShot(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, gGestureOneShots[variant * 2], 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_BeginState(pl, 0, gGestureOneShots[variant * 2 + 1], 0);
        }
        break;
    }
}

void Player_StateGesture0A(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x271, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 0x4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGesture0B(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x276, 0x10, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 0xA, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGesture0C(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x272, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

void Player_StateGesture10(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 0x279, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMoveMode(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_BeginState(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartAct(pl);
            }
        }
        break;
    }
}

/* The 0x716360 part's .data (0x8C3C90-0x8C3DE0) */
SpitMeat gSpitMeats[13] = {
#include "game/008C3C90.inc"
};
s8 gStunMashNeed[8] = {
#include "game/008C3DC8.inc"
};
s16 gGestureOneShots[8] = {
#include "game/008C3DD0.inc"
};

/* ---- 0x722400 (was unk_722400.c) ---- */
/* prototypes of callees (round 13) */
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
void Em_Add7F4(Em *em, s32 n);
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
void StageObj_K29_TickType9(Player *p);
void StageObj_K29_TickType9Late(Player *p);
void Player_UpdateHitStop(void *obj);
void Player_UpdateMotionStep(Player *p);
void Player_UpdateHitTimers(Player *p);
void Em_CollideField(struct Em *p, u8 keep);
s32 Em_GetFloorInfo(struct Em *o, f32 *pos, struct TriHdr *a, f32 *out, s32 *b);
void func_330DE0(void *p);
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
s16 *Em_IdSetLookup(Em *em);
void EmScript_Op7B(Em *em);
void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c);
u8 *EmScript_PickTrigger(Em *em);
u8 *EmScript_GetPos(Em *em);
u8 *EmScript_Call(Em *em, u8 *p);
u8 *EmScript_Op01(Em *em, u8 *p);
u8 *EmScript_Op02(Em *em, u8 *p);
u8 *EmScript_Op03(Em *em, u8 *p);
u8 *EmScript_Op06(Em *em, u8 *p);
u8 *EmScript_Op08(Em *em, u8 *p);
u8 *EmScript_Op09(Em *em, u8 *p);
u8 *EmScript_Op0A(Em *em, u8 *p);
u8 *EmScript_Op0B(Em *em, u8 *p);
u8 *EmScript_Op0C(Em *em, u8 *p);
u8 *EmScript_Op0D(Em *em, u8 *p);
u8 *EmScript_Op0E(Em *em, u8 *p);
u8 *EmScript_Op0F(Em *em, u8 *p);
u8 *EmScript_Op10(Em *em, u8 *p);
u8 *EmScript_Op11(Em *em, u8 *p);
u8 *EmScript_Op12(Em *em, u8 *p);
u8 *EmScript_Op13(Em *em, u8 *p);
u8 *EmScript_Op14(Em *em, u8 *p);
u8 *EmScript_Op15(Em *em, u8 *p);
u8 *EmScript_Op17(Em *em, u8 *p);
u8 *EmScript_Op18(Em *em, u8 *p);
u8 *EmScript_Op19(Em *em, u8 *p);
u8 *EmScript_Op1A(Em *em, u8 *p);
u8 *EmScript_Op1B(Em *em, u8 *p);
u8 *EmScript_Op1C(Em *em, u8 *p);
u8 *EmScript_Op1D(Em *em, u8 *p);
u8 *EmScript_Op1E(Em *em, u8 *p);
u8 *EmScript_Op1F(Em *em, u8 *p);
u8 *EmScript_Op20(Em *em, u8 *p);
u8 *EmScript_Op21(Em *em, u8 *p);
u8 *EmScript_Op22(Em *em, u8 *p);
u8 *EmScript_Op23(Em *em, u8 *p);
u8 *EmScript_Op24(Em *em, u8 *p);
u8 *EmScript_Op25(Em *em, u8 *p);
u8 *EmScript_Op26(Em *em, u8 *p);
u8 *EmScript_Op27(Em *em, u8 *p);
u8 *EmScript_Op28(Em *em, u8 *p);
u8 *EmScript_Op29(Em *em, u8 *p);
u8 *EmScript_Op2A(Em *em, u8 *p);
u8 *EmScript_Op2B(Em *em, u8 *p);
u8 *EmScript_Op2C(Em *em, u8 *p);
u8 *EmScript_Op2D(Em *em, u8 *p);
u8 *EmScript_Op2E(Em *em, u8 *p);
u8 *EmScript_Op2F(Em *em, u8 *p);
u8 *EmScript_Op30(Em *em, u8 *p);
u8 *EmScript_Op31(Em *em, u8 *p);
u8 *EmScript_Op32(Em *em, u8 *p);
u8 *EmScript_Op33(Em *em, u8 *p);
u8 *EmScript_Op34(Em *em, u8 *p);
u8 *EmScript_Op35(Em *em, u8 *p);
u8 *EmScript_Op36(Em *em, u8 *p);
u8 *EmScript_Op37(Em *em, u8 *p);
u8 *EmScript_Op38(Em *em, u8 *p);
u8 *EmScript_Op39(Em *em, u8 *p);
u8 *EmScript_Op3A(Em *em, u8 *p);
u8 *EmScript_Op3B(Em *em, u8 *p);
u8 *EmScript_Op3C(Em *em, u8 *p);
u8 *EmScript_Op3D(Em *em, u8 *p);
u8 *EmScript_Op3E(Em *em, u8 *p);
u8 *EmScript_Op3F(Em *em, u8 *p);
u8 *EmScript_Op40(Em *em, u8 *p);
u8 *EmScript_Op41(Em *em, u8 *p);
u8 *EmScript_Op42(Em *em, u8 *p);
u8 *EmScript_Op44(Em *em, u8 *p);
u8 *EmScript_Op45(Em *em, u8 *p);
u8 *EmScript_Op46(Em *em, u8 *p);
u8 *EmScript_Op47(Em *em, u8 *p);
u8 *EmScript_Op48(Em *em, u8 *p);
u8 *EmScript_Op49(Em *em, u8 *p);
u8 *EmScript_Op4A(Em *em, u8 *p);
u8 *EmScript_Op4B(Em *em, u8 *p);
u8 *EmScript_Op4C(Em *em, u8 *p);
u8 *EmScript_Op4D(Em *em, u8 *p);
u8 *EmScript_Op4E(Em *em, u8 *p);
u8 *EmScript_Op4F(Em *em, u8 *p);
u8 *EmScript_Op50(Em *em, u8 *p);
u8 *EmScript_Op51(Em *em, u8 *p);
u8 *EmScript_Op52(Em *em, u8 *p);
u8 *EmScript_Op53(Em *em, u8 *p);
u8 *EmScript_Op54(Em *em, u8 *p);
u8 *EmScript_Op55(Em *em, u8 *p);
u8 *EmScript_Op56(Em *em, u8 *p);
u8 *EmScript_Op57(Em *em, u8 *p);
u8 *EmScript_Op58(Em *em, u8 *p);
u8 *EmScript_Op59(Em *em, u8 *p);
u8 *EmScript_Op5A(Em *em, u8 *p);
u8 *EmScript_Op5B(Em *em, u8 *p);
u8 *EmScript_Op5C(Em *em, u8 *p);
u8 *EmScript_Op5D(Em *em, u8 *p);
u8 *EmScript_Op5E(Em *em, u8 *p);
u8 *EmScript_Op5F(Em *em, u8 *p);
u8 *EmScript_Op60(Em *em, u8 *p);
u8 *EmScript_Op61(Em *em, u8 *p);
u8 *EmScript_Op62(Em *em, u8 *p);
u8 *EmScript_Op63(Em *em, u8 *p);
u8 *EmScript_Op64(Em *em, u8 *p);
u8 *EmScript_Op65(Em *em, u8 *p);
u8 *EmScript_Op66(Em *em, u8 *p);
u8 *EmScript_Op67(Em *em, u8 *p);
s32 EmScript_Op68(Em *em, s32 p);
u8 *EmScript_Op69(Em *em, u8 *p);
u8 *EmScript_Op70(Em *em, u8 *p);
u8 *EmScript_Op71(Em *em, u8 *p);
u8 *EmScript_Op72(Em *em, u8 *p);
u8 *EmScript_Op73(Em *em, u8 *p);
u8 *EmScript_Op74(Em *em, u8 *p);
u8 *EmScript_Op75(Em *em, u8 *p);
u8 *EmScript_Op76(Em *em, u8 *p);
u8 *EmScript_Op77(Em *em, u8 *p);
u8 *EmScript_Op78(Em *em, u8 *p);
u8 *EmScript_Op79(Em *em, u8 *p);
u8 *EmScript_Op7A(Em *em, u8 *p);
u8 *EmScript_Op7C(Em *em, u8 *p);
u8 *EmScript_Op80(Em *em, u8 *p);
u8 *EmScript_Op81(Em *em, u8 *p);
u8 *EmScript_Op82(Em *em, u8 *p);
u8 *EmScript_Op83(Em *em, u8 *p);
u8 *EmScript_Op90(Em *em, u8 *p);
void *EmScript_Op91(Em *em, u8 *p);
s32 EmScript_Op92(s32 a, s32 p);
s32 EmScript_Op93(s32 a, s32 p);
u8 *EmScript_Op94(Em *em, u8 *p);
void *EmScript_Op99(Em *em, u8 *p);
u8 *EmScript_OpFF(Em *em, u8 *p);
s32 EmScript_Start(Em *em, u8 *p);
s32 func_3BCCB0(EquipItem *item, u8 which);
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
typedef struct EmView818 {
    u8 pad[0x818];
    f32 f[4];
} EmView818;
typedef struct EmHeadView {
    u8 pad[0x76E];
    u16 head;                   /* 0x76E */
    u8 found;                   /* 0x770 */
} EmHeadView;
u16 Vec_Heading(f32 *a, f32 *b);
typedef struct EmDamageView {
    u8 pad[0x6B2];
    s16 part[8];                /* 0x6B2 */
    s16 weak;                   /* 0x6C2 */
    u8 pad6C4[0x6D4 - 0x6C4];
    u8 partFlags[8];            /* 0x6D4 */
    u8 pad6DC[0x6FA - 0x6DC];
    s16 status5;                /* 0x6FA */
    u8 pad6FC[0x700 - 0x6FC];
    s16 status2;                /* 0x700 */
    u8 pad702[0x708 - 0x702];
    s16 poison;                 /* 0x708 */
    u8 pad70A[0x712 - 0x70A];
    s16 status4;                /* 0x712 */
    u8 pad714[0x71A - 0x714];
    s16 status1;                /* 0x71A */
} EmDamageView;
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
extern s32 D_008F5AB8;
void EmScript_SetPos(Em *obj, u8 *value);
void *EmScript_LoopScript(Em *obj, u8 i);
struct EmIdEntry *Em_FindAreaEntry(struct Em *em, struct EmIdEntry *entry, s16 id);
s32 Em_CanEnterArea(Em *em, u16 area);
u16 Area_IdForCurrentTime(u16);
s16 Player_IsState(void *obj, u8 state, u8 sub);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
u8 func_330E80(void *);
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
void StageObj_Stun_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
void Player_BeginState(Player *p, s32 action, s32 a2, s32 a3);
u16 Player_FindActionSpot(Player *, f32 *, u16 *arg, u8 *);
void Player_ResetAim(Player *p);
extern s8 gEmKindTimerIndex[];
extern s16 gEmBlindTimes[];
void *EmScript_SubScript(Em *obj, u8 i);
s32 Player_HasSkill(Player *p, u8 skill);
extern s16 gEmCombatInit[];
void Em_BreakPart(Em *em, s32 part);
extern s16 gEmKindTimerValues[];
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmScript_Op41: 53 of 79 words differ */
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
typedef struct EmDistView {
    u8 pad[0x764];
    f32 limit[8];
    u8 pad784[0x818 - 0x784];
    f32 dist[16];
} EmDistView;
s32 Versus_AddGauge(Em *em, u8 n);
extern s32 gEmNoticeThresholds[];
void Em_ComputeGoal(Em *em, f32 *out);
struct AreaTri;
/* unit's own declaration: the definition (sub_main unk_328CD0.c) returns u8; with it EmScript_Op5C differs (58 of 80 words) */
s32 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
u16 Area_BaseId(u16);
u8 Daytime_Get(void);
void Em_RequestSync(Em *em, u8 a);
void EmScript_Restart(Em *);
extern s32 gEmAreaValues8A0[];
extern s32 gEmGoalPoints[];
extern s32 gQSyncSendPeriod; /* gm_sub; this unit reads its low half */
s16 *Em_IdSetHas(Em *em, u16 area);
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
        Player_BeginState(pl, 0, 4, 0);
        break;
    }
}

void ObjAttach_UpdateAllTimers(void);
void Game_UpdatePlayers(void)
{
    s32 i;

    Player_UpdateInput();
    for (i = 0; i < 10; i++) {
        Player_Update(&gPlayers[i]);
    }
    ObjAttach_UpdateAllTimers();
}

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

void Player_CheckKushalaSeen(Player *pl)
{
    Em *m;
    s16 i;

    if (Player_IsLocal(pl) == 0 || Game_IsOnline() == 1) {
        return;
    }
    m = gEms;
    for (i = 0; i < 20; i++) {
        if (m->active != 0 && m->visible != 0 && m->type == 0x36 && m->area == pl->area && Save_TestEventFlag(0x81) == 0) {
            Save_SetEventFlag(0x81);
        }
        m++;
    }
}

void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
struct DrawSlot;
s32 OT_AddByDepth(u32 *table, struct DrawSlot *node, u32 n, s32 clamp);
s32 Player_LocalAreaOk_Unk(void);
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
    f32 d;
    f32 buf[3];
    f32 y;
    f32 fy;
    s32 clamp;
    u8 g;
    u8 st;
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
            pl->unk893 = 0;
        }
        Player_CheckTeamComboLost(pl);
    } else {
        pl->unk893 = 0;
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
        StageObj_K29_TickType9(pl);
    }
    if (pl->statusIcons & 0x800) {
        StageObj_K29_TickType9Late(pl);
    }
    pl->actRequest = 0;
    pl->offerRequest = 0;
    Player_UpdateHitStop(pl);
    if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) == 0 || (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) != 0 && pl->stateVar0 == 0)) {
        Player_UpdateMotionStep(pl);
    }
    Player_UpdateHitTimers(pl);
    (*(void (**)(Player *)) ((u8 *) pl->handlers + 0xC))(pl);
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
        func_330DE0(pl);
    }
    if (pl->action != 2 && pl->rideState == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_PULL_UP) == 0 && pl->stateGroup != PL_GROUP_EVENT && (u8) Obj_InCurrentArea(pl)) {
        fy = pl->floorY;
        y = pl->pos[1];
        if (y < fy) {
            pl->pos[1] = fy;
            pl->rideState = 0;
        } else if (y >= fy) {
            if (y - fy < 30.0f) {
                pl->pos[1] = fy;
                pl->rideState = 0;
            } else if (pl->hp > 0) {
            g = pl->stateGroup;
            if (g != 5) {
                if (g == 2) {
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
    g = pl->stateGroup;
    if (g != 2 && g != 3) {
        if (pl->weaponOut && (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) && (pl->input.pressed & 0x80)) {
            if (g == 0) {
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
            } else if (g != 6 && Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_MELEE) == 0) {
                pl->aimMode ^= 1;
            }
            if (pl->action == 2) {
                pl->aimMode = 0;
            }
            if (Player_LocalAreaOk_Unk() == 0) {
                if ((u8) Obj_InCurrentArea(pl) == 1) {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
                Player_ResetAim(pl);
            }
        } else if (pl->weaponOut == 0) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_FIRE) != 0) {
                if (pl->input.pressed & 0x80) {
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
        *(f32 *) ((u8 *) pl->drawSlot + 8) = pl->pos[0];
        *(f32 *) ((u8 *) pl->drawSlot + 0xC) = pl->pos[1];
        *(f32 *) ((u8 *) pl->drawSlot + 0x10) = pl->pos[2];
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
        CamCtl_GetPlayerFade(&d);
        clamp = Player_InSceneEvent() == 1;
        if (pl->nearCamera == 0 && !(d < 1.0f)) {
            OT_AddByDepth(gDrawOt32, pl->drawSlot, 0x20, clamp);
        } else {
            OT_AddByDepth(gDrawOtSorted, pl->drawSlot, 0x40, clamp);
        }
        if (pl->syncTimer != 0) {
            (pl->syncTimer)--;
        }
        n = *(s16 *)&gQSyncSendPeriod;
        if (pl->syncTimer == 0 && gFrameTick % n == 0) {
            QSync_SendPlayer(pl, 2, 0);
        }
    }
}

/* turns the monster (unkA4) toward unk00E by a step that depends on its state, and swings the
 * body angle at +0x69C (a u16) with it */
void Player_UpdateAutoTurn(Player *pl)
{
    s32 step;
    u32 d;
    u16 turn;
    s32 target;
    s32 dir;

    if (pl->areaMoveState != 0) {
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
    d = (u16) (target - (u16) dir);
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
    if ((u16) (d + step) < (u32) (step * 2)) {
        pl->rot[1] = target;
        pl->unk2B8 = 0;
        if (pl->lean + 0x300 <= 0x600) {
            pl->lean = 0;
        } else if (pl->lean < 0x8000) {
            pl->lean -= 0x300;
        } else {
            pl->lean += 0x300;
        }
    } else if (d < 0x8000U) {
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

void Player_FindLookTarget(Player *pl)
{
    EmHeadView *h = (EmHeadView *) pl;
    u32 i;
    u32 best;
    Em *em;
    f32 min;
    f32 d;
    u16 a;

    best = 0xFF;
    h->found = 0;
    min = 1500.0f;
    em = gEms;
    if (pl->motionId[0] == 0x26) {
        for (i = 0; i < 20; i++, em++) {
            if (em->active != 0 && em->visible != 0) {
                d = Vec_Distance(&pl->pos[0], &em->posX);
                if (!(min < d)) {
                    best = i;
                    min = d;
                    h->found = 1;
                }
            }
        }
    }
    if (h->found == 0) {
        if (h->head != 0) {
            if (h->head + 0x400 <= 0x800 && h->head + 0x400 >= 0) {
                h->head = 0;
            } else if ((s16) h->head >= 0) {
                h->head -= 0x400;
            } else {
                h->head += 0x400;
            }
        }
    } else {
        a = (u16) (Vec_Heading(&gEms[best].posX, &pl->pos[0]) + 0x4000) + 0x10000 - pl->rot[1];
        if (a >= 0x8000) {
            if (a < 0xD556) {
                a = -0x2AAA;
            }
            if (h->head >= 0x8000) {
                if (h->head - 0x800 <= a) {
                    h->head = a;
                } else {
                    h->head = h->head - 0x800;
                }
            } else {
                h->head -= 0x800;
            }
        } else {
            if (a > 0x2AAB) {
                a = 0x2AAB;
            }
            if (h->head < 0x8000) {
                if (h->head + 0x800 >= a) {
                    h->head = a;
                } else {
                    h->head = h->head + 0x800;
                }
            } else {
                h->head += 0x800;
            }
        }
    }
}

void Player_CheckPointMessages(u8 *script)
{
    f32 pos[3];
    u16 sp2C;
    s8 sp2F;
    s32 spot;

    if (Player_IsLocal(script) != 0) {
        spot = Player_FindActionSpot((Player *)script, (f32 *)pos, &sp2C, (u8 *)&sp2F) & 0xFFFF;
        switch (spot) {
        case 3:
            MsgEvent_Check(1);
            break;
        case 7:
            MsgEvent_Check(3);
            break;
        case 5:
            MsgEvent_Check(4);
            break;
        }
    }
}

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

/* With four hunters each wearing armour, looks up their four armour kinds (+0x8CB, in any order) in
 * the 14 combinations of gArmorCombos; a match sets Em +0x8CA/+0x8C9 and posts notice 0x1D. */
void Player_CheckTeamCombo(Player *self)
{
    u8 have[4];
    u8 left[4];
    u8 want[4];
    s16 i;
    s16 n;
    Player *pl;
    s16 m;
    u8 w;
    u8 *src;
    s16 j;
    u8 cb;
    u8 ca;
    ArmorCombo *c;
    s16 k;

    self->mealSkill = 0;
    self->mealSkillGroup = 0;
    if (Quest_IsVersus() != 1 && gScene.playerCount > 3) {
    for (pl = gPlayers, i = 0; i < 4; i++, pl++) {
        if (pl->active == 0) {
            return;
        }
        have[i] = 0xFF;
        n = 0;
        if (func_3BCCB0(&pl->equip[2], pl->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&pl->equip[3], pl->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&pl->equip[4], pl->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&pl->equip[5], pl->gender) == 1) {
            n++;
        }
        if (func_3BCCB0(&pl->equip[0], pl->gender) == 1) {
            n++;
        }
        if (n == 0) {
            return;
        }
        have[i] = pl->style;
    }
    for (i = 0, c = gArmorCombos; i < 14; i++, c++) {
        ca = c->a;
        cb = c->b;
        src = c->ids.id;
        for (j = 0; j < 4; j++) {
            left[j] = have[j];
            want[j] = *src++;
        }
        for (j = 0; j < 4; j++) {
            w = want[j];
            for (m = 0; m < 4; m++) {
                if (w != 0xFF && w == left[m]) {
                    want[j] = 0xFF;
                    left[m] = 0xFF;
                    break;
                }
            }
        }
        for (j = 0; j < 4; j++) {
            if (want[j] != 0xFF) {
                goto next;
            }
        }
        self->mealSkillGroup = ca;
        self->mealSkill = cb;
        Notice_Push(0, 0x1D, 0);
        return;
    next:;
    }
    }
}

void Player_CheckTeamComboLost(Player *pl)
{
    s16 i;

    if (pl->mealSkill != 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (gScene.inQuest[i] != 1) {
                pl->mealSkill = 0;
                pl->mealSkillGroup = 0;
                Notice_Push(0, 0x1E, 0);
                return;
            }
        }
    }
}

void Player_SetEvadeFrames(Player *pl)
{
    s16 v = 6;

    if (Player_HasSkill(pl, 0xAF) == 1) {
        v += 2;
    } else if (Player_HasSkill(pl, 0xB0) == 1) {
        v += 4;
    }
    pl->noHitTimer = v;
}

s32 func_00723BC0(Em *em)
{
    return (s16) em->spawns[0].goalKind > 0;
}

s32 func_00723BD0(Em *em)
{
    return em->spawns[0].timer > 0;
}

s32 Item_IsStealable(s32 item)
{
    switch (item & 0xFFFF) {
    case 0xB7:
    case 0xB:
    case 0x1A:
    case 7:
    case 8:
    case 0x292:
    case 9:
    case 0xA:
    case 0x1B:
    case 0x298:
    case 0x1C:
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x10:
    case 0x12:
    case 0x13:
    case 0x293:
    case 0x104:
    case 0xB8:
    case 0xC7:
    case 0xCE:
    case 0xCF:
    case 0x15:
    case 0x16:
    case 0x22:
    case 0x17:
    case 0xC:
    case 0xCA:
    case 0x11:
    case 0x14:
    case 0x1E:
    case 0xC8:
    case 0x2F:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0xEB:
    case 0xF7:
    case 0xF8:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3D:
    case 0x3E:
        return 1;
    }
    return 0;
}

u8 Em_PickStatus(Em *em)
{
    u8 ret = 0;
    s32 f;

    if (em->status == 0 && em->lastAttackKind != 6 && em->lastAttackKind != 9) {
        f = (u8) (em->statusFlags & ~4);
        if (f != 0) {
            if (em->stance == 2 || ((em->type == 0x36 || em->type == 0x3C || em->type == 0x40 || em->type == 0x41) && em->stance == 1)) {
                Em_EndStatus1And2(em);
                ret = 1;
            } else if ((u8) f & 8) {
                ret = 4;
            } else if ((u8) f & 1) {
                ret = 2;
            } else if ((u8) f & 2) {
                ret = 3;
            } else if ((u8) f & 0x10) {
                ret = 5;
            }
        }
    }
    return ret;
}

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

void Em_StartBlind(Em *em)
{
    em->blindTimer = gEmBlindTimes[em->type];
    switch (em->type) {
    case 0x1:
    case 0x6:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x1A:
    case 0x26:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2E:
    case 0x2F:
    case 0x31:
    case 0x27:
    case 0xB:
    case 0x25:
    case 0x11:
    case 0x8:
    case 0xE:
    case 0x22:
        StageObj_Stun_CreateEm(em, 2, 0x22, em->blindTimer, 150.0f, 3.0f);
        break;
    case 0x24:
    case 0x47:
    case 0x2:
        StageObj_Stun_CreateEm(em, 2, 0x33, em->blindTimer, 150.0f, 3.0f);
        break;
    case 0x9:
    case 0x17:
        StageObj_Stun_CreateEm(em, 2, 0, em->blindTimer, 90.0f, 1.0f);
        break;
    case 0x4:
        StageObj_Stun_CreateEm(em, 2, 0, em->blindTimer, 100.0f, 1.0f);
        break;
    case 0xD:
    case 0x10:
    case 0x1B:
    case 0x1C:
    case 0x1E:
    case 0x1F:
    case 0x23:
        StageObj_Stun_CreateEm(em, 2, 0xE, em->blindTimer, 50.0f, 1.0f);
        break;
    case 0xC:
    case 0x19:
        StageObj_Stun_CreateEm(em, 2, 0xF, em->blindTimer, 50.0f, 1.5f);
        break;
    case 0x3:
    case 0x21:
    case 0x45:
        StageObj_Stun_CreateEm(em, 2, 0x17, em->blindTimer, 50.0f, 1.5f);
        break;
    case 0x13:
    case 0x18:
        StageObj_Stun_CreateEm(em, 2, 8, em->blindTimer, 20.0f, 1.0f);
        break;
    case 0x34:
    case 0x35:
    case 0x3D:
    case 0x3E:
    case 0x33:
        StageObj_Stun_CreateEm(em, 2, 0x11, em->blindTimer, 100.0f, 2.0f);
        break;
    case 0x36:
    case 0x3C:
        StageObj_Stun_CreateEm(em, 2, 0x25, em->blindTimer, 150.0f, 3.0f);
        break;
    case 0x40:
    case 0x41:
        StageObj_Stun_CreateEm(em, 2, 0x25, em->blindTimer, 270.0f, 3.0f);
        break;
    case 0x39:
        StageObj_Stun_CreateEm(em, 2, 0xB, em->blindTimer, 30.0f, 1.0f);
        break;
    case 0x3B:
        StageObj_Stun_CreateEm(em, 2, 0x27, em->blindTimer, 200.0f, 3.5f);
        break;
    case 0x46:
        StageObj_Stun_CreateEm(em, 2, 0xE, em->blindTimer, 100.0f, 1.5f);
        
    case 0x0:
    default:
        break;
    }
}

void Em_ChangeState(Em *em, s32 arg1, u16 arg2, s32 arg3); /* unit's own declaration: Em_ApplyFlash
 * passes its u16 st as is; with unk_701810.c's s32 arg2 the call masks it (andi a2, s0, 0xFFFF) */
s32 EmBlangonga_IgnoresFlash(Em *em);
s32 EmBlangonga_InStateArmor(Em *em);
/* The monster's reaction to a flash (Game's hit dispatcher calls it when em->flashed was set):
 * by type and stance, returns early (states that ignore it) or picks the state st (7-10, or 2
 * with Em_StartBlind and the timer), then Em_ChangeState(em, 4, st, 3). Case 0x39's
 * `switch (em->stateGroup)` with its default first (falling into the return of 4-6) is what
 * leaves every conditional branch's delay slot empty in the whole function (docs/mwcc-matching.md,
 * "Delay slots filled or empty"); the other groups' tests are plain if/else and && chains.
 * Em_ChangeState is declared above with a u16 arg2, as the game's unit declared it
 * (unk_701810.c defines it with s32): with s32 the call masks st (`andi a2, s0, 0xFFFF`),
 * the original passes it as is.
 */
void Em_ApplyFlash(Em *em)
{
    u16 st;
    u8 g;

    switch (em->type) {
    case 0xE:
    case 0x1A:
    case 0x2B:
    case 0x2C:
        if (em->stance == 4) {
            return;
        }
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x6:
    case 0x8:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0x10:
    case 0x11:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x18:
    case 0x19:
    case 0x1B:
    case 0x1C:
    case 0x1E:
    case 0x1F:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2E:
    case 0x2F:
    case 0x31:
    case 0x36:
    case 0x3B:
    case 0x3C:
    case 0x40:
    case 0x41:
    case 0x45:
    case 0x46:
    case 0x47:
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
    case 0x4:
    case 0x9:
    case 0x17:
    case 0x39:
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
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x3D:
    case 0x3E:
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
    case 0x1:
    case 0xB:
    case 0x25:
    case 0x29:
    case 0x2A:
    case 0x31:
        if (em->stance == 2) {
            st = 9;
        } else {
            st = 10;
        }
        break;
    case 0x6:
    case 0x14:
    case 0x26:
    case 0x27:
        if (em->stance == 2) {
            st = 9;
        } else {
            st = 10;
        }
        break;
    case 0x11:
    case 0x16:
    case 0x2F:
        if (em->stance == 2) {
            st = 9;
        } else {
            st = 10;
        }
        break;
    case 0x28:
        if (em->stance == 2) {
            st = 9;
        } else {
            st = 10;
        }
        break;
    case 0xE:
    case 0x1A:
    case 0x2B:
    case 0x2C:
        if (em->stance == 2) {
            st = 9;
        } else {
            g = em->stateGroup;
            if (g == 4 || (g == 0 && em->state == 0x1C) || (g == 0 && em->state == 0x1D)
                || (g == 0 && em->state == 0x21) || (g == 4 && em->state == 6)
                || (g == 4 && em->state == 0xD) || (g == 4 && em->state == 0xE)
                || (g == 4 && em->state == 0x11) || (g == 4 && em->state == 0x12)) {
                return;
            }
            st = 10;
        }
        break;
    case 0x2:
    case 0x24:
    case 0x47:
        if (em->stance == 2) {
            st = 8;
        } else {
            st = 7;
        }
        break;
    case 0x9:
    case 0x17:
        if (em->stance == 0 && em->stateGroup != 4 && em->stateGroup != 5 && em->stateGroup != 6
            && Player_IsState((Player *) em, 4, 2) == 0) {
            st = 2;
            Em_StartBlind(em);
            em->timer = em->blindTimer;
            break;
        }
        return;
    case 0x39:
        switch (em->stateGroup) {
        default:
            if (Player_IsState((Player *) em, 4, 2) == 0 && Player_IsState((Player *) em, 4, 4) == 0) {
                if (em->stance == 0) {
                    st = 2;
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
    case 0x4:
        if (em->stance == 0 && em->stateGroup != 4 && em->stateGroup != 5 && em->stateGroup != 6
            && Player_IsState((Player *) em, 4, 4) == 0) {
            st = 4;
            Em_StartBlind(em);
            em->timer = em->blindTimer;
            break;
        }
        return;
    case 0xD:
    case 0x10:
    case 0x1E:
    case 0x23:
        if (em->stance == 2) {
            st = 1;
        } else {
            st = 5;
        }
        break;
    case 0x1B:
    case 0x1C:
    case 0x1F:
        if (em->stance == 2) {
            st = 1;
        } else {
            st = 5;
        }
        break;
    case 0xC:
    case 0x19:
    case 0x46:
        st = 5;
        break;
    case 0x15:
    case 0x2E:
        switch (em->stance) {
        case 0:
            st = 10;
            break;
        case 1:
            st = 4;
            break;
        case 4:
            st = 0x11;
            break;
        case 2:
            st = 9;
            break;
        default:
            st = 10;
            break;
        }
        break;
    case 0x8:
    case 0x22:
        switch (em->stance) {
        case 0:
            st = 10;
            break;
        case 1:
            st = 4;
            break;
        case 4:
            st = 0x11;
            break;
        case 2:
            st = 9;
            break;
        default:
            st = 10;
            break;
        }
        break;
    case 0x3:
    case 0x45:
        st = 4;
        break;
    case 0x13:
    case 0x18:
        if (em->stance != 2) {
            st = 5;
        } else {
            st = 4;
        }
        break;
    case 0x21:
        st = 4;
        break;
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x3D:
    case 0x3E:
        if (em->stance == 2) {
            st = 7;
        } else {
            if (EmBlangonga_IgnoresFlash(em) == 1) {
                return;
            }
            st = 1;
        }
        break;
    case 0x36:
    case 0x3C:
    case 0x40:
    case 0x41:
        if (em->stance == 2 || em->stance == 1) {
            st = 9;
        } else {
            st = 10;
        }
        break;
    case 0x3B:
        if (em->stance == 2) {
            st = 9;
        } else {
            st = 10;
        }
        break;
    }
    em->scriptPending = 0;
    /* the game reads a register it never set: st is s0, still the caller's, after case 0x39 with em->stance != 0 */
    Em_ChangeState(em, 4, st, 3);
}

void Em_ForceState19(Em *em)
{
    switch (em->type) {
    case 0x1:
    case 0x2A:
    case 0x25:
    case 0xB:
    case 0x29:
    case 0x31:
        EmRathalos_ChangeState(em, 0, 0x19, 4);
        break;
    case 0x6:
    case 0x26:
    case 0xE:
    case 0x2B:
    case 0xF:
    case 0x2D:
    case 0x11:
    case 0x2F:
    case 0x14:
    case 0x27:
    case 0x16:
    case 0x1A:
    case 0x2C:
    case 0x28:
        break;
    case 0x8:
    case 0x22:
        EmCephadrome_ChangeState(em, 0, 7, 4);
        break;
    case 0x9:
    case 0x17:
    case 0x4:
    case 0x5:
    case 0x44:
    case 0x20:
    case 0xD:
    case 0x10:
    case 0x23:
    case 0x1E:
    case 0x1B:
    case 0x1C:
    case 0x1F:
    case 0xC:
    case 0x46:
    case 0x19:
        break;
    case 0x15:
    case 0x2E:
        if (em->stance == 0) {
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    case 0x36:
    case 0x3C:
    case 0x40:
    case 0x41:
        break;
    }
}

void Em_UpdateStatus(Em *em)
{
    u8 *param = (u8 *) gEmStatusParams[em->type];
    EmStatusParam *p4 = gEmStatus4Params[em->type];
    EmStatusParam *p5 = gEmStatus5Params[em->type];
    EmStatusParam *p2;
    EmStatusParam *p1 = gEmStatus1Params[em->type];
    f32 pos[3];
    s32 bone;
    f32 scale;

    p2 = gEmStatus2Params[em->type];

    if (em->statusFlags & 4) {
        if (--em->poisonDamage > 0) {
            switch (em->type) {
            case 0x1:
            case 0x6:
            case 0x8:
            case 0xB:
            case 0xE:
            case 0xF:
            case 0x11:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x1A:
            case 0x22:
            case 0x25:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x29:
            case 0x2A:
            case 0x2B:
            case 0x2C:
            case 0x2D:
            case 0x2E:
            case 0x2F:
            case 0x31:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 0x22;
                scale = 2.0f;
                break;
            case 0x2:
            case 0x24:
            case 0x47:
                pos[0] = 0.0f;
                pos[1] = 200.0f;
                pos[2] = 0.0f;
                bone = 0x33;
                scale = 2.0f;
                break;
            case 0x3:
            case 0x45:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0x17;
                scale = 1.0f;
                break;
            case 0xC:
            case 0x19:
            case 0x46:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0xF;
                scale = 1.2f;
                break;
            case 0x9:
            case 0x17:
            case 0x39:
                pos[0] = 0.0f;
                pos[1] = 20.0f;
                pos[2] = 0.0f;
                bone = 0xA;
                scale = 1.2f;
                break;
            case 0xD:
            case 0x10:
            case 0x1B:
            case 0x1C:
            case 0x1E:
            case 0x1F:
            case 0x23:
                pos[0] = 0.0f;
                pos[1] = 20.0f;
                pos[2] = 0.0f;
                bone = 0xE;
                scale = 1.2f;
                break;
            case 0x4:
            case 0x5:
            case 0x20:
            case 0x44:
                pos[0] = 0.0f;
                pos[1] = 10.0f;
                pos[2] = 0.0f;
                bone = 0xB;
                scale = 1.0f;
                break;
            case 0x13:
            case 0x18:
                pos[0] = 0.0f;
                pos[1] = 0.0f;
                pos[2] = 0.0f;
                bone = 8;
                scale = 0.5f;
                break;
            case 0x21:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0x17;
                scale = 1.0f;
                break;
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x3D:
            case 0x3E:
                pos[0] = 0.0f;
                pos[1] = 40.0f;
                pos[2] = 45.0f;
                bone = 0x11;
                scale = 1.0f;
                break;
            case 0x36:
            case 0x3C:
            case 0x40:
            case 0x41:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 0x25;
                scale = 2.0f;
                break;
            case 0x30:
            case 0x43:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 100.0f;
                bone = 0x1F;
                scale = 2.0f;
                break;
            case 0x42:
            case 0x49:
                pos[0] = 0.0f;
                pos[1] = 40.0f;
                pos[2] = 90.0f;
                bone = 0x1F;
                scale = 1.0f;
                break;
            case 0x3B:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 0x25;
                scale = 2.0f;
                break;
            case 0x3F:
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
                em->poisonTimer = *(s16 *)(param + 0xA);
                em->hp -= *(s16 *)(param + 6);
                if (em->hp <= 0 && ((s8) em->guardTimer > 0 || em->large != 0)) {
                    em->hp = 1;
                }
            }
        } else {
            em->poisonDamage = 0;
            em->poisonTimer = 0;
            em->statusFlags &= ~4;
            if (em->poisonCount < 4) {
                em->poisonCount++;
                em->poisonThreshold += *(s16 *)(param + 0xC);
            }
        }
    } else if (em->poisonBuildup != 0) {
        if (--em->poisonTimer <= 0) {
            em->poisonTimer = *(s16 *)(param + 2);
            em->poisonBuildup -= *(s16 *)(param + 4);
            if (em->poisonBuildup <= 0) {
                em->poisonBuildup = 0;
                em->poisonTimer = 0;
            }
        }
    }
    if (!(em->statusFlags & 8) && em->status4Buildup != 0) {
        if (--em->status4Decay <= 0) {
            em->status4Decay = p4->decayInterval;
            em->status4Buildup -= p4->decayAmount;
            if (em->status4Buildup <= 0) {
                em->status4Buildup = 0;
                em->status4Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & 0x10) && em->status5Buildup != 0) {
        if (--em->status5Decay <= 0) {
            em->status5Decay = p5->decayInterval;
            em->status5Buildup -= p5->decayAmount;
            if (em->status5Buildup <= 0) {
                em->status5Buildup = 0;
                em->status5Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & 1) && em->status1Buildup != 0) {
        if (--em->status1Decay <= 0) {
            em->status1Decay = p1->decayInterval;
            em->status1Buildup -= p1->decayAmount;
            if (em->status1Buildup <= 0) {
                em->status1Buildup = 0;
                em->status1Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & 2) && em->status2Buildup != 0) {
        if (--em->status2Decay <= 0) {
            em->status2Decay = p2->decayInterval;
            em->status2Buildup -= p2->decayAmount;
            if (em->status2Buildup <= 0) {
                em->status2Buildup = 0;
                em->status2Decay = 0;
            }
        }
    }
}

void Em_AddPoison(Em *em, s32 arg)
{
    s16 max;
    u8 *param;

    param = (u8 *) gEmStatusParams[em->type];
    if (Versus_TeamHasFlag((void *) em, 1U) != 1 && arg != 0 && em->poisonThreshold > 0) {
        em->poisonBuildup = em->poisonBuildup + arg;
        if (em->statusFlags & 4) {
            max = em->poisonThreshold;
            if (em->poisonBuildup >= max) {
                em->poisonBuildup = max - 1;
            }
        } else {
            em->poisonTimer = *(s16 *)(param + 2);
            if (em->poisonBuildup >= em->poisonThreshold) {
                em->poisonBuildup = 0;
                em->statusFlags |= 4;
                em->poisonDamage = *(s16 *)(param + 8);
                em->poisonTimer = 0;
            }
        }
    }
}

void Em_AddStatus4(Em *em, s32 arg)
{
    EmStatusParam *param;

    param = gEmStatus4Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 0U) != 1 && arg != 0 && em->status4Threshold > 0) {
        if (!(em->statusFlags & 8)) {
            em->status4Buildup = em->status4Buildup + arg;
            em->status4Decay = param->decayInterval;
        }
        if (em->status4Buildup >= em->status4Threshold) {
            em->statusFlags |= 8;
        }
    }
}

void Em_AddStatus5(Em *em, s32 amount)
{
    EmStatusParam *prm = gEmStatus5Params[em->type];

    if (amount != 0 && em->status5Threshold > 0) {
        if (!(em->statusFlags & 0x10)) {
            em->status5Buildup = em->status5Buildup + amount;
            em->status5Decay = prm->decayInterval;
        }
        if (em->status5Buildup >= em->status5Threshold) {
            em->statusFlags |= 0x10;
        }
    }
}

void Em_AddStatus2(Em *em, s32 arg)
{
    EmStatusParam *param;

    param = gEmStatus2Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 2U) != 1 && arg != 0 && em->status2Threshold > 0) {
        if (!(em->statusFlags & 2)) {
            em->status2Buildup = em->status2Buildup + arg;
            em->status2Decay = param->decayInterval;
        }
        if (em->status2Buildup >= em->status2Threshold) {
            em->statusFlags |= 2;
        }
    }
}

void Em_AddStatus1(Em *em, s32 status1)
{
    EmStatusParam *param;

    param = gEmStatus1Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 2U) != 1 && status1 != 0 && em->status1Threshold > 0) {
        if (!(em->statusFlags & 1)) {
            em->status1Buildup = em->status1Buildup + status1;
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
    EmDamageView *v = (EmDamageView *) em;
    u8 best;
    u32 i;
    s16 max;
    s16 d;
    u8 *t;

    max = 0;
    best = 0;
    t = gEmDamagedPartTbls[em->type];
    for (i = 0; i < 8; i++) {
        d = v->part[i] * em->damageFactor;
        if (v->part[i] > 0 && d == 0) {
            d = 1;
        }
        if (d < 0) {
            *neg -= d;
        } else {
            *sum += d;
            if (max < v->part[i]) {
                max = v->part[i];
                best = i;
            }
        }
    }
    if (t != NULL) {
        if ((v->partFlags[t[0]] & t[1]) == t[1]) {
            v->weak = v->part[t[0]];
            if (v->weak < 0) {
                v->weak = 0;
            }
        } else {
            v->weak = 0;
        }
    }
    if (em->rage == 0 && *sum > 0) {
        Em_CheckRageByHp(em, *sum);
    }
    em->flinchPart = best;
    if ((s8) em->guardTimer <= 0) {
        Em_AddStatus1(em, v->status1);
        Em_AddStatus2(em, v->status2);
        Em_AddPoison(em, v->poison);
        Em_AddStatus4(em, v->status4);
        Em_AddStatus5(em, v->status5);
    }
}

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

/* Raw addresses kept here: the named form changes the code. */
void Em_FlagPartBreak(Em *em, s32 part)
{
    /* em->partsToBreak, kept as a byte offset: the field form does not match */
    *(u16 *)((u8 *)em + 0x9A6) = (u16) (*(u16 *)((u8 *)em + 0x9A6) | ((1 << (part & 0xFFFF)) & 0xFFFF));
}

s32 func_00725820(Em *em, s32 arg1)
{
    s32 temp_a0;

    temp_a0 = arg1 & 0xFF;
    if ((temp_a0 != 0) && (temp_a0 != 1) && (temp_a0 != 2)) {
        return 1;
    }
    return 0;
}

void Em_ResetPartHp(Em *em, u32 i);
u8 Em_UpdateDamage(Em *em, u8 *out)
{
    EmDamageView *v = (EmDamageView *) em;
    s16 sum;
    s16 neg;
    u8 broke;
    u8 first;
    s8 i;
    s16 d;
    u8 bit;
    s32 (*fn)(Em *);
    u8 s;
    u8 r;

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
                d = v->part[i] * em->damageFactor;
                if (v->part[i] > 0 && d == 0) {
                    d = 1;
                }
                em->parts[0].hp -= d;
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
                d = v->part[i] * em->damageFactor;
                if (v->part[i] > 0 && d == 0) {
                    d = 1;
                }
                em->parts[i].hp -= d;
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
                        if (v->part[i] > v->part[em->flinchPart]) {
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
                d = v->part[i] * em->damageFactor;
                if (v->part[i] > 0 && d == 0) {
                    d = 1;
                }
                em->parts[i].hp -= d;
                if (em->parts[i].hp <= 0) {
                    if (i == 8) {
                        fn = em->handlers->unk18;
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
                        if (i < 8 && v->part[i] > v->part[em->flinchPart]) {
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
            if (em->hitReactMode != 2) {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            if (!((em->type == 2 || em->type == 0x24 || em->type == 0x47) && (em->lastAttackKind == 9 || em->lastAttackKind == 6))) {
                return 0;
            }
        } else {
            if ((em->type == 7 || em->type == 0x32) && Area_DayId(em->area) != 0xC && em->hp < 1000) {
                em->hp = 1000;
            }
            if (em->type == 0x37 && Area_BaseId(gScene.map) == 1 && Area_DayId(em->area) != 0xC && em->hp < 1000) {
                em->hp = 1000;
            }
            if ((em->type == 0x3A || em->type == 0x48) && Area_BaseId(gScene.map) == 0xF && Area_DayId(em->area) != 0x7C && em->hp < 1000) {
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
            r = 0x12;
            goto end;
        }
        if ((s8) em->holdTimer == 0) {
            if (em->lastAttackKind == 9) {
                r = 3;
                goto end;
            }
            if (em->lastAttackKind == 6) {
                r = 4;
                goto end;
            }
        }
    }
    if ((s = Em_PickStatus(em)) != 0 || em->flashed != 0) {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
    if (s == 1) {
        r = 5;
    } else if (s == 4) {
        r = 6;
    } else if (s == 2 && Em_IsHeld(em) == 1 && Em_NearLowHp(em) == 1) {
        r = 7;
    } else {
        if (Em_IsHeld(em) == 0 || Em_NearLowHp(em) == 0) {
            em->statusFlags &= 0xFE;
        }
        if (s == 3) {
            r = 8;
        } else if (s == 5) {
            r = 0x11;
        } else if (em->flashed != 0) {
            em->flashed = 0;
            Em_ApplyFlash(em);
            r = 9;
        } else if (em->hitReceived != 0) {
            if (em->hitReactMode == 2) {
                r = 0xA;
            } else {
                if (em->hitReactMode != 4 && Em_IsHeld(em) == 0) {
                    em->hitReactMode = 0;
                }
                if (em->part8Down != 0) {
                    r = 0xB;
                } else if (em->stateGroup == 4) {
                    return 0;
                } else if (em->lastAttackKind == 5) {
                    if (broke) {
                        r = 0x10;
                    } else {
                        r = 0xF;
                    }
                } else if (broke) {
                    r = 0xC;
                } else {
                    if (em->hitReactMode == 3) {
                        r = 0xD;
                    } else {
                        if (Em_IsHeld(em) == 1) {
                            return 0;
                        }
                        r = 0xE;
                        Em_SelectSightRow(em, 0);
                        Em_LoadReactionSet(em, 0);
                        if (*out == 0 || em->hitReactMode != 0) {
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
    if (r != 0xE) {
        em->scriptPending = 0;
        Em_RestartScript(em);
        Player_ClearFlag((Player *)em, 0x20000);
    }
    Em_CheckKindRequest(em);
    return r;
}

static void EmScript_SetVar95E(Em *em, u8 *script, s32 v)
{
    em->scriptAbort = (u8) v;
}

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
    D_008F5AB8 = 0;
    em->scriptPos = em->mainScriptPos;
    em->goalKind = 0;
    em->goalArg = 0;
    em->goalNo = 0;
    em->curGoalKind = 0;
    em->curGoalArg = 0;
    em->curGoalNo = 0;
}

void EmScript_Run(Em *em)
{
    u8 run;
    u8 *script;
    s32 n;
    s32 a;
    s32 b;
    s32 self;

    run = 1;
    self = (s32) em;
    n = 0;
    if (em->stateGroup == 5) {
        return;
    }
    if (em->stateGroup == 6) {
        return;
    }
    em->noReact = 0;
    EmScript_Restart(em);
    if ((script = EmScript_PickTrigger(em)) == NULL) {
        script = EmScript_GetPos(em);
    }
    while (run) {
        n++;
        if (n >= 1000) {
            EmScript_SetVar95E(em, script, 2);
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
        case 0x01:
            script = EmScript_Op01(em, script + 1);
            break;
        case 0x02:
            script = EmScript_Op02(em, script + 1);
            break;
        case 0x03:
            script = EmScript_Op03(em, script + 1);
            break;
        case 0x04:
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            break;
        case 0x05:
            script++;
            a = *script++;
            b = *script++;
            em->routedStateArg = *script++;
            if (em->remote == 0) {
                Em_ChangeStateRouted(em, a, b, 1);
            }
            EmScript_SetPos(em, script);
            run = 0;
            break;
        case 0x06:
            script = EmScript_Op06(em, script + 1);
            break;
        case 0x07:
            script = (u8 *) EmScript_Start(em, script + 1);
            break;
        case 0x08:
            script = EmScript_Op08(em, script + 1);
            break;
        case 0x1F:
            script = EmScript_Op1F(em, script + 1);
            break;
        case 0x09:
            script = EmScript_Op09(em, script + 1);
            break;
        case 0x0A:
            script = EmScript_Op0A(em, script + 1);
            break;
        case 0x0B:
            script = EmScript_Op0B(em, script + 1);
            break;
        case 0x0C:
            script = EmScript_Op0C(em, script + 1);
            break;
        case 0x0D:
            script = EmScript_Op0D(em, script + 1);
            break;
        case 0x0E:
            script = EmScript_Op0E(em, script + 1);
            break;
        case 0x0F:
            script = EmScript_Op0F(em, script + 1);
            break;
        case 0x10:
            script = EmScript_Op10(em, script + 1);
            break;
        case 0x11:
            script = EmScript_Op11(em, script + 1);
            break;
        case 0x12:
            script = EmScript_Op12(em, script + 1);
            break;
        case 0x13:
            script = EmScript_Op13(em, script + 1);
            break;
        case 0x14:
            script = EmScript_Op14(em, script + 1);
            break;
        case 0x15:
            script = EmScript_Op15(em, script + 1);
            break;
        case 0x16:
            script = EmScript_Call(em, script + 1);
            break;
        case 0x17:
            script = EmScript_Op17(em, script + 1);
            break;
        case 0x18:
            script = EmScript_Op18(em, script + 1);
            break;
        case 0x19:
            script = EmScript_Op19(em, script + 1);
            break;
        case 0x1A:
            script = EmScript_Op1A(em, script + 1);
            break;
        case 0x1B:
            script = EmScript_Op1B(em, script + 1);
            break;
        case 0x1C:
            script = EmScript_Op1C(em, script + 1);
            break;
        case 0x1D:
            script = EmScript_Op1D(em, script + 1);
            break;
        case 0x1E:
            script = EmScript_Op1E(em, script + 1);
            break;
        case 0x20:
            script = EmScript_Op20(em, script + 1);
            break;
        case 0x21:
            script = EmScript_Op21(em, script + 1);
            break;
        case 0x22:
            script = EmScript_Op22(em, script + 1);
            break;
        case 0x36:
            script = EmScript_Op36(em, script + 1);
            break;
        case 0x23:
            script = EmScript_Op23(em, script + 1);
            break;
        case 0x24:
            script = EmScript_Op24(em, script + 1);
            break;
        case 0x25:
            script = EmScript_Op25(em, script + 1);
            break;
        case 0x26:
            script = EmScript_Op26(em, script + 1);
            break;
        case 0x27:
            script = EmScript_Op27(em, script + 1);
            break;
        case 0x28:
            script = EmScript_Op28(em, script + 1);
            break;
        case 0x29:
            script = EmScript_Op29(em, script + 1);
            break;
        case 0x2A:
            script = EmScript_Op2A(em, script + 1);
            break;
        case 0x2B:
            script = EmScript_Op2B(em, script + 1);
            break;
        case 0x2C:
            script = EmScript_Op2C(em, script + 1);
            break;
        case 0x70:
            script = EmScript_Op70(em, script + 1);
            break;
        case 0x2D:
            script = EmScript_Op2D(em, script + 1);
            break;
        case 0x2E:
            script = EmScript_Op2E(em, script + 1);
            break;
        case 0x2F:
            script = EmScript_Op2F(em, script + 1);
            break;
        case 0x30:
            script = EmScript_Op30(em, script + 1);
            break;
        case 0x31:
            script = EmScript_Op31(em, script + 1);
            break;
        case 0x32:
            script = EmScript_Op32(em, script + 1);
            break;
        case 0x33:
            script = EmScript_Op33(em, script + 1);
            break;
        case 0x34:
            script = EmScript_Op34(em, script + 1);
            break;
        case 0x35:
            script = EmScript_Op35(em, script + 1);
            break;
        case 0x37:
            script = EmScript_Op37(em, script + 1);
            break;
        case 0x38:
            script = EmScript_Op38(em, script + 1);
            break;
        case 0x39:
            script = EmScript_Op39(em, script + 1);
            break;
        case 0x3A:
            script = EmScript_Op3A(em, script + 1);
            break;
        case 0x3B:
            script = EmScript_Op3B(em, script + 1);
            break;
        case 0x3C:
            script = EmScript_Op3C(em, script + 1);
            break;
        case 0x3D:
            script = EmScript_Op3D(em, script + 1);
            break;
        case 0x3E:
            script = EmScript_Op3E(em, script + 1);
            break;
        case 0x3F:
            script = EmScript_Op3F(em, script + 1);
            break;
        case 0x40:
            script = EmScript_Op40(em, script + 1);
            break;
        case 0x41:
            script = EmScript_Op41(em, script + 1);
            break;
        case 0x42:
            script = EmScript_Op42(em, script + 1);
            break;
        case 0x69:
            script = EmScript_Op69(em, script + 1);
            break;
        case 0x44:
            script = EmScript_Op44(em, script + 1);
            break;
        case 0x45:
            script = EmScript_Op45(em, script + 1);
            break;
        case 0x46:
            script = EmScript_Op46(em, script + 1);
            break;
        case 0x47:
            script = EmScript_Op47(em, script + 1);
            break;
        case 0x48:
            script = EmScript_Op48(em, script + 1);
            break;
        case 0x49:
            script = EmScript_Op49(em, script + 1);
            break;
        case 0x4A:
            script = EmScript_Op4A(em, script + 1);
            break;
        case 0x4B:
            script = EmScript_Op4B(em, script + 1);
            break;
        case 0x4C:
            script = EmScript_Op4C(em, script + 1);
            break;
        case 0x4D:
            script = EmScript_Op4D(em, script + 1);
            break;
        case 0x4E:
            script = EmScript_Op4E(em, script + 1);
            break;
        case 0x4F:
            script = EmScript_Op4F(em, script + 1);
            break;
        case 0x50:
            script = EmScript_Op50(em, script + 1);
            break;
        case 0x51:
            script = EmScript_Op51(em, script + 1);
            break;
        case 0x52:
            script = EmScript_Op52(em, script + 1);
            break;
        case 0x53:
            script = EmScript_Op53(em, script + 1);
            break;
        case 0x54:
            script = EmScript_Op54(em, script + 1);
            break;
        case 0x55:
            script = EmScript_Op55(em, script + 1);
            break;
        case 0x56:
            script = EmScript_Op56(em, script + 1);
            break;
        case 0x57:
            script = EmScript_Op57(em, script + 1);
            break;
        case 0x58:
            script = EmScript_Op58(em, script + 1);
            break;
        case 0x59:
            script = EmScript_Op59(em, script + 1);
            break;
        case 0x5A:
            script = EmScript_Op5A(em, script + 1);
            break;
        case 0x5B:
            script = EmScript_Op5B(em, script + 1);
            break;
        case 0x5C:
            script = EmScript_Op5C(em, script + 1);
            break;
        case 0x5D:
            script = EmScript_Op5D(em, script + 1);
            break;
        case 0x5E:
            script = EmScript_Op5E(em, script + 1);
            break;
        case 0x5F:
            script = EmScript_Op5F(em, script + 1);
            break;
        case 0x60:
            script = EmScript_Op60(em, script + 1);
            break;
        case 0x61:
            script = EmScript_Op61(em, script + 1);
            break;
        case 0x62:
            script = EmScript_Op62(em, script + 1);
            break;
        case 0x63:
            script = EmScript_Op63(em, script + 1);
            break;
        case 0x64:
            script = EmScript_Op64(em, script + 1);
            break;
        case 0x65:
            script = EmScript_Op65(em, script + 1);
            break;
        case 0x66:
            script = EmScript_Op66(em, script + 1);
            break;
        case 0x67:
            script = EmScript_Op67(em, script + 1);
            break;
        case 0x68:
            script = (u8 *) EmScript_Op68(em, (s32) (script + 1));
            break;
        case 0x80:
            script = EmScript_Op80(em, script + 1);
            break;
        case 0x81:
            script = EmScript_Op81(em, script + 1);
            break;
        case 0x82:
            script = EmScript_Op82(em, script + 1);
            break;
        case 0x83:
            script = EmScript_Op83(em, script + 1);
            break;
        case 0x84:
        case 0x7B:
            EmScript_Op7B(em);
            script++;
            break;
        case 0xFF:
            script = EmScript_OpFF(em, script + 1);
            break;
        case 0x90:
            script = EmScript_Op90(em, script + 1);
            break;
        case 0x91:
            script = EmScript_Op91(em, script + 1);
            break;
        case 0x99:
            script = EmScript_Op99(em, script + 1);
            break;
        case 0x92:
            script = (u8 *) EmScript_Op92(self, (s32) (script + 1));
            break;
        case 0x93:
            script = (u8 *) EmScript_Op93(self, (s32) (script + 1));
            break;
        case 0x94:
            script = EmScript_Op94(em, script + 1);
            break;
        case 0x71:
            script = EmScript_Op71(em, script + 1);
            break;
        case 0x72:
            script = EmScript_Op72(em, script + 1);
            break;
        case 0x73:
            script = EmScript_Op73(em, script + 1);
            break;
        case 0x74:
            script = EmScript_Op74(em, script + 1);
            break;
        case 0x75:
            script = EmScript_Op75(em, script + 1);
            break;
        case 0x76:
            script = EmScript_Op76(em, script + 1);
            break;
        case 0x77:
            script = EmScript_Op77(em, script + 1);
            break;
        case 0x78:
            script = EmScript_Op78(em, script + 1);
            break;
        case 0x79:
            script = EmScript_Op79(em, script + 1);
            break;
        case 0x7A:
            script = EmScript_Op7A(em, script + 1);
            break;
        case 0x7C:
            script = EmScript_Op7C(em, script + 1);
            break;

        default:
            Em_RestartScript(em);
            run = 0;
            break;
        }
    }
    em->scriptOffset = (s32) em->mainScriptPos - (s32)em->scriptPos;
}

void EmScript_Op7B(Em *em)
{
    em->stateRand++;
}

u8 *EmScript_Op01(Em *em, u8 *script)
{
    s32 i;
    s32 n;

    switch (*script) {
    case 0:
        script++;
        for (i = 0, n = 0; i < gScene.playerCount; i++) {
            if (em->sensedPlayers & (1 << i)) {
                n++;
            }
        }
        if (n == 0) {
            for (;;) {
                if (script[0] == 1 && script[1] == 1) {
                    break;
                }
                if (script[0] == 1 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 1, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 1 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script + 1, 1);
        break;
    case 2:
        script++;
        break;
    }
    return script;
}

u8 *EmScript_Op02(Em *em, u8 *script)
{
    switch (*script) {
    case 0:
        script++;
        if (em->noticed == 0) {
            *(s8 *) &em->scriptPlayer = -1;
            for (;;) {
                if (script[0] == 2 && script[1] == 1) {
                    break;
                }
                if (script[0] == 2 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 2, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 2 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script + 1, 2);
        break;
    case 2:
        script++;
        break;
    }
    return script;
}

u8 *EmScript_Op03(Em *em, u8 *script)
{
    s32 ok;

    switch (*script) {
    case 0:
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
                if (script[0] == 3 && script[1] == 1) {
                    break;
                }
                if (script[0] == 3 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 3, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 3 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
            break;
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script + 1, 3);
        break;
    case 2:
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

u8 *EmScript_Op08(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->stance != 0) {
            for (;;) {
                if (script[0] == 8 && script[1] == 1) {
                    break;
                }
                if (script[0] == 8 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 8, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 8 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 8);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op1F(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->stance != 1) {
            for (;;) {
                if (script[0] == 0x1F && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x1F && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x1F, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x1F && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x1F);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op09(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->stance != 2) {
            for (;;) {
                if (script[0] == 9 && script[1] == 1) {
                    break;
                }
                if (script[0] == 9 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 9, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 9 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 9);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op0A(Em *em, u8 *script)
{
    em->hitReactMode = *script;
    return script + 1;
}

u8 *EmScript_Op0B(Em *em, u8 *script)
{
    u8 v;

    switch (*script++) {
    case 0:
        v = *script++;
        em->scriptCombat = em->combat;
        if (em->combat != v) {
            for (;;) {
                if (script[0] == 0xB && script[1] == 1) {
                    break;
                }
                if (script[0] == 0xB && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0xB, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0xB && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0xB);
        break;
    case 2:
        break;
    }
    return script;
}

/* Script op 0x0C: k and v are the two bytes at p; sets the Em byte k picks to v (1: blind,
 * 2: unk814, 3: senseDelay, 4: unk78E[0], 5/8/9: work, 6/7: work bytes 0x44/0x45; 5-9 only for
 * some types and when not remote); k == 0 sets all of them (k 1-10). w is set inside the type
 * checks of cases 6 and 7: the loop hoists &em->work from there into its preheader (between the
 * compare constants 55 and 45) and keeps 0x44/0x45 as the store offsets.
 */
u8 *EmScript_Op0C(Em *em, u8 *script)
{
    u8 k;
    u8 v;
    s32 i;
    s32 n;
    u8 *w;

    k = *script++;
    v = *script++;
    if (k == 0) {
        k = 1;
        n = 10;
    } else {
        n = 1;
    }
    for (i = 0; i < n; i++, k++) {
        switch (k) {
        case 1:
            em->blind = v;
            break;
        case 2:
            em->need2State = v;
            break;
        case 3:
            em->senseDelay = (s8) v;
            break;
        case 4:
            em->scriptVar = v;
            break;
        case 5:
            if (em->type == 7 || em->type == 0x32) {
                if (em->remote == 0) {
                    em->work = v;
                }
            }
            break;
        case 6:
            if (em->type == 0xF || em->type == 0x2D) {
                w = &em->work;
                if (em->remote == 0) {
                    w[0x44] = v;
                }
            }
            break;
        case 7:
            if (em->type == 0xF || em->type == 0x2D) {
                w = &em->work;
                if (em->remote == 0) {
                    w[0x45] = v;
                }
            }
            break;
        case 8:
            if (em->type == 0x37 && em->remote == 0) {
                em->work = v;
            }
            break;
        case 9:
            if (em->type == 0x3A || em->type == 0x48) {
                if (em->remote == 0) {
                    em->work = v;
                }
            }
            break;
        }
    }
    return script;
}

/* Script op 0x0D: clears the Em byte the byte at p picks (the fields of EmScript_Op0C; 10:
 * unk882 = 0xFFFF); 0 clears all of them. w is set as in EmScript_Op0C.
 */
u8 *EmScript_Op0D(Em *em, u8 *script)
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
                if (em->type == 7 || em->type == 0x32) {
                    if (em->remote == 0) {
                        em->work = 0;
                    }
                }
                break;
            case 6:
                if (em->type == 0xF || em->type == 0x2D) {
                    w = &em->work;
                    if (em->remote == 0) {
                        w[0x44] = 0;
                    }
                }
                break;
            case 7:
                if (em->type == 0xF || em->type == 0x2D) {
                    w = &em->work;
                    if (em->remote == 0) {
                        w[0x45] = 0;
                    }
                }
                break;
            case 8:
                if (em->type == 0x37 && em->remote == 0) {
                    em->work = 0;
                }
                break;
            case 9:
                if (em->type == 0x3A || em->type == 0x48) {
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

u8 *EmScript_Op0E(Em *em, u8 *script)
{
    u16 id;

    switch (*script++) {
    case 0:
        id = script[0] << 8;
        id |= script[1];
        script += 2;
        if (em->area != Area_IdForCurrentTime(id)) {
            for (;;) {
                if (script[0] == 0xE && script[1] == 1) {
                    break;
                }
                if (script[0] == 0xE && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0xE, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0xE && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0xE);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op0F(Em *em, u8 *script)
{
    u8 *q;
    u8 n;
    s16 *e;

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
        e = (s16 *)Em_FindAreaEntry(em, em->tables[1], em->area);
        if (e == NULL) {
            em->scriptFlags &= 0xFE;
            return script + 1;
        }
        if (em->loopMode != 1 && e[1] < n) {
            n = e[1];
        }
        em->loopCount = n;
        em->areaPointCount = e[1];
        em->loopScriptPos = EmScript_LoopScript(em, em->loopScript);
        break;
    }
    return em->loopScriptPos;
}

//---
u8 *EmScript_Op10(Em *em, u8 *script)
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

u8 *EmScript_Op11(Em *em, u8 *script)
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

u8 *EmScript_Op12(Em *em, u8 *script)
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

u8 *EmScript_Op13(Em *em, u8 *script)
{
    s8 v;

    em->goalKind = 1;
    em->goalArg = 0;
    v = (s8) em->scriptPlayer;
    if (v == -1) {
        *(u16 *) &em->goalNo = 0xFFFF;
    } else {
        em->goalNo = v & 0xF;
    }
    return script;
}

u8 *EmScript_Op14(Em *em, u8 *script)
{
    s32 ok;
    u8 k;
    u16 a;
    u16 d;
    u16 t;
    Player *pl;
    f32 pa[4];
    f32 pb[4];

    switch (*script++) {
    case 0:
        k = *script++;
        a = (s32) (65536.0f * k / 360.0f + 0.5f);
        ok = 0;
        if (em->goalKind == 1 && *(u16 *) &em->goalArg == 0 && (s16) (t = *(u16 *) &em->goalNo) != -1) {
            Em_GetTargetPos(em, t, pa);
            pl = &gPlayers[t];
            Area_ToWorld(pl->area, pa, pb);
            d = Math_AngleXZ(em->mapPos, pb) - em->rotY;
            if (d > 0x8000) {
                d = 0x10000 - d;
            }
            if (a > d) {
                ok = 1;
            }
        } else {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == 0x14 && script[1] == 1) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == 0x14 && script[1] == 2) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, 0x14, 2);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == 0x14 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x14 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x14, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x14 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x14);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op15(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u16 v;
    u8 ok;
    u16 d;
    u16 e;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = script[0] << 8;
                v |= script[1];
                script += 2;
                d = Area_DayId(v);
                e = Area_DayId(em->area);
                if (e == d) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x15, 3);
                if (script[1] == 2 || script[1] == 3) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        script += 2;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x15, 3);
            if (script[0] == 0x15 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Call(Em *em, u8 *script)
{
    em->subScriptNo = *script;
    em->subScriptReturn = script + 1;
    script = EmScript_SubScript(em, em->subScriptNo);
    em->scriptPos = script;
    return script;
}

/* Command 0x17 (4 argument bytes), once while +0x880 is -1: two pairs of bytes into +0x79A/+0x880 and
 * +0x79B/+0x7A0; a zero +0x880 becomes -1 again. The last step is two increments (`p += 2` is folded
 * into each return). */
u8 *EmScript_Op17(Em *em, u8 *script)
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
u8 *EmScript_Op18(Em *em, u8 *p)
{
    u8 done = 0;
    u16 v;

    if (em->areaSeqLen == -1) {
        return p;
    }
    em->areaSeqActive = 1;
    em->prevArea = 0xFFFF;
    do {
        v = ((u16 *)EmScript_AreaSeqList(em, em->areaSeqTable))[em->areaSeqPos];
        if (v >= 0x8000) {
            *(u16 *) &em->destArea = Area_IdForCurrentTime(*(u16 *) EmScript_PickWeighted(em, em->script[7][v & 0x7FFF]));
        } else {
            *(u16 *) &em->destArea = Area_IdForCurrentTime(v);
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
    return p;
other:
    em->areaSeqNext = p;
    p = EmScript_AreaSeqScript(em, em->areaSeqScript);
    em->scriptPos = p;
    return p;
}

u8 *EmScript_Op19(Em *em, u8 *script)
{
    u16 dest;

    em->goalKind = 3;
    dest = *(u16 *) &em->destArea;
    em->goalNo = (s16) dest;
    em->goalArg = (s16) dest;
    return script;
}

/* Script op 0x1A: walks the route toward area (the two bytes at p): up to 20 steps of
 * Em_PickRoutePoint's pick from the areas Em_IdSetLookup/Em_IdSetHas list, until an area passes
 * Em_CanEnterArea or the target is reached; the result goes to Em +0x986, +0x77C and +0x77A.
 * `area = area | *p++` (not `|=`) gives the game's stack slot for area. */
u8 *EmScript_Op1A(Em *em, u8 *script)
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
    AreaRecord *a;
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
    list = Em_IdSetLookup(em);
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
        a = Stage_GetAreaRecord(em->area);
        pts[8][0] = a->originX + a->sizeX / 2.0f;
        pts[8][2] = a->originZ + (f32) (a->sizeZ / 2.0f);
        a = Stage_GetAreaRecord(tgt);
        pts[9][0] = a->originX + a->sizeX / 2.0f;
        pts[9][2] = a->originZ + (f32) (a->sizeZ / 2.0f);
        for (i = 0, n = 0; i < 8; i++) {
            if (list[i] == -1) {
                break;
            }
            a = Stage_GetAreaRecord(list[i]);
            ids[i] = list[i];
            pts[i][0] = a->originX + a->sizeX / 2.0f;
            n++;
            pts[i][2] = a->originZ + (f32) (a->sizeZ / 2.0f);
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
        list = Em_IdSetHas(em, area);
        if (list == NULL) {
            *(u16 *) &em->destArea = Area_IdForCurrentTime(area);
            goto end;
        }
    }
end:
    em->goalArg = em->goalNo = *(u16 *) &em->destArea;
    return script;
}

u8 *EmScript_Op1B(Em *em, u8 *script)
{
    u8 v;

    switch (*script++) {
    case 0:
        v = *script++;
        if (v != em->requested) {
            for (;;) {
                if (script[0] == 0x1B && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x1B && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x1B, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x1B && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x1B);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op1C(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*script++) {
    case 0:
        x = em->hp * 100 / em->hpMax;
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (v > x) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x1C, 3);
                if (script[1] == 2 || script[1] == 3) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x1C, 3);
            if (script[0] == 0x1C && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op1D(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (em->requestId == v) {
                    break;
                }
                if (v < em->requestId) {
                    script = EmScript_SkipBlock(em, script, 0x1D, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x1D, 3);
                        if ((script[0] == 0x1D && script[1] == 2) || (script[0] == 0x1D && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x1D, 3);
            if (script[0] == 0x1D && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op1E(Em *em, u8 *script)
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

u8 *EmScript_Op20(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u16 v;
    u8 ok;
    u16 k;
    u16 ang;
    f32 a[4];
    f32 b[4];
    Player *pl;

    switch (*script++) {
    case 0:
        n = script[0];
        script += 3;
        if (em->goalKind == 1 && *(u16 *) &em->goalArg == 0 && (s16) (k = *(u16 *) &em->goalNo) != -1) {
            Em_GetTargetPos(em, k, a);
            pl = &gPlayers[k];
            Area_ToWorld(pl->area, a, b);
            ang = Math_AngleXZ(em->mapPos, b) - em->rotY;
        } else {
            ok = 1;
            do {
                script = EmScript_SkipBlock(em, script, 0x20, 3);
                if ((script[0] == 0x20 && script[1] == 2) || (script[0] == 0x20 && script[1] == 3)) {
                    ok = 0;
                }
                script = EmScript_NextCmd(em, script);
            } while (ok);
            break;
        }
        i = 0;
        if (i < n) {
            do {
                v = *script++ << 8;
                if (v >= ang) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x20, 3);
                if (script[1] == 2 || script[1] == 3) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x20, 3);
            if (script[0] == 0x20 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op21(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->need2Gauge > (s32) (0.3f * em->need2Max)) {
            for (;;) {
                if (script[0] == 0x21 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x21 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x21, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x21 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x21);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op22(Em *em, u8 *script)
{
    u8 v;
    f32 lim;

    switch (*script++) {
    case 0:
        lim = 100.0f * *script++;
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= lim)) {
            for (;;) {
                if (script[0] == 0x22 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x22 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x22, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x22 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x22);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op36(Em *em, u8 *script)
{
    u8 v;
    f32 lim;

    switch (*script++) {
    case 0:
        lim = 100.0f * *script++;
        if (!(Vec_Distance(&em->posX, em->goal) <= lim)) {
            for (;;) {
                if (script[0] == 0x36 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x36 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x36, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x36 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x36);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op23(Em *em, u8 *script)
{
    u8 n;
    u8 v;
    u8 found;
    s32 i;
    s32 j;
    u8 ok;
    Em *e;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                for (j = 0, found = 0, e = gEms; j < 20; j++, e++) {
                    if (e->active != 0) {
                        if (v == 0x4A) {
                            if (e->large == 0) {
                                continue;
                            }
                        } else if (Em_KindMatches(e->type, v) == 0) {
                            continue;
                        }
                        if ((u8) Obj_SameArea(em, (Player *) e) != 0) {
                            found = 1;
                            break;
                        }
                    }
                }
                if (found != 0) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x23, 3);
                if (script[1] == 2 || script[1] == 3) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x23, 3);
            if (script[0] == 0x23 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op24(Em *em, u8 *script)
{

    switch (*script++) {
    case 0:
        em->repeatCount = *script++;
        em->repeatPos = script;
        if (em->repeatCount <= 0) {
            for (;;) {
                if (script[0] == 0x24 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x24, 1);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x24 && script[1] == 1) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        if (--em->repeatCount > 0) {
            script = em->repeatPos;
        }
        break;
    }
    return script;
}

u8 *EmScript_Op25(Em *em, u8 *script)
{
    em->repeatCount = 0;
    return script;
}

u8 *EmScript_Op26(Em *em, u8 *script)
{
    em->noAlert = *script;
    return script + 1;
}

u8 *EmScript_Op27(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (em->variant == v) {
                    break;
                }
                if (v < em->variant) {
                    script = EmScript_SkipBlock(em, script, 0x27, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x27, 3);
                        if ((script[0] == 0x27 && script[1] == 2) || (script[0] == 0x27 && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x27, 3);
            if (script[0] == 0x27 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op75(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (em->spawnParam == v) {
                    break;
                }
                if (v < em->spawnParam) {
                    script = EmScript_SkipBlock(em, script, 0x75, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x75, 3);
                        if ((script[0] == 0x75 && script[1] == 2) || (script[0] == 0x75 && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x75, 3);
            if (script[0] == 0x75 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op76(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*script++) {
    case 0:
        x = Quest_GetSeason();
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (x == v) {
                    break;
                }
                if (v < x) {
                    script = EmScript_SkipBlock(em, script, 0x76, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x76, 3);
                        if ((script[0] == 0x76 && script[1] == 2) || (script[0] == 0x76 && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x76, 3);
            if (script[0] == 0x76 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op7A(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*script++) {
    case 0:
        x = Area_BaseId(gScene.map);
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = Area_BaseId(*script++);
                if (x == v) {
                    break;
                }
                if (v < x) {
                    script = EmScript_SkipBlock(em, script, 0x7A, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x7A, 3);
                        if ((script[0] == 0x7A && script[1] == 2) || (script[0] == 0x7A && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x7A, 3);
            if (script[0] == 0x7A && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op79(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;
    s32 k;
    u8 (*fn)(Em *, u8);

    switch (*script++) {
    case 0:
        fn = em->handlers->unk10;
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
                v = *script++;
                if (x == v) {
                    break;
                }
                if (v < x) {
                    script = EmScript_SkipBlock(em, script, 0x79, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x79, 3);
                        if ((script[0] == 0x79 && script[1] == 2) || (script[0] == 0x79 && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x79, 3);
            if (script[0] == 0x79 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op28(Em *em, u8 *script)
{
    Player *pl = gPlayers;
    s32 i;
    u8 ok;

    switch (*script++) {
    case 0:
        ok = 1;
        for (i = 0; i < gScene.playerCount; i++, pl++) {
            if (Obj_SameArea(em, pl) != 0 && pl->active != 0) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            while (ok) {
                if (script[0] == 0x28 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x28 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x28, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x28 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x28);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op29(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->areaTimer1 > 0) {
            for (;;) {
                if (script[0] == 0x29 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x29 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x29, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x29 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x29);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op2A(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->areaTimer2 > 0) {
            for (;;) {
                if (script[0] == 0x2A && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x2A && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x2A, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x2A && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x2A);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op2B(Em *em, u8 *script)
{
    u8 k;
    u8 v;
    u8 ne;

    switch (*script++) {
    case 0:
        k = script[0];
        v = script[1];
        script += 2;
        switch (k) {
        case 1:
            if (em->blind == v) {
                ne = 0;
            } else {
                ne = 1;
            }
            break;
        case 2:
            if (em->need2State == v) {
                ne = 0;
            } else {
                ne = 1;
            }
            break;
        case 3:
            if (em->senseDelay == v) {
                ne = 0;
            } else {
                ne = 1;
            }
            break;
        case 4:
            if (em->scriptVar == v) {
                ne = 0;
            } else {
                ne = 1;
            }
            break;
        case 5:
            if (em->type == 7 || em->type == 0x32) {
                if (em->work == v) {
                    ne = 0;
                } else {
                    ne = 1;
                }
            }
            break;
        case 6:
            if (em->type == 0xF || em->type == 0x2D) {
                if ((&em->work)[0x44] == v) {
                    ne = 0;
                } else {
                    ne = 1;
                }
            }
            break;
        case 7:
            if (em->type == 0xF || em->type == 0x2D) {
                if ((&em->work)[0x45] == v) {
                    ne = 0;
                } else {
                    ne = 1;
                }
            }
            break;
        case 8:
            if (em->type == 0x37) {
                if (em->work == v) {
                    ne = 0;
                } else {
                    ne = 1;
                }
            }
            break;
        case 9:
            if (em->type == 0x3A || em->type == 0x48) {
                if (em->work == v) {
                    ne = 0;
                } else {
                    ne = 1;
                }
            }
            break;
        }
        /* the game reads a register it never set: ne is s0, still the caller's, when the em->type tests fail or k has no case */
        if (!ne) {
            break;
        }
        while (ne) {
            if (script[0] == 0x2B && script[1] == 1) {
                break;
            }
            if (script[0] == 0x2B && script[1] == 2) {
                break;
            }
            script = EmScript_SkipBlock(em, script, 0x2B, 2);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == 0x2B && script[1] == 2) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x2B);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op2C(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                if (Em_KindMatches(em->type, *script++) != 0) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x2C, 3);
                if (script[1] == 2 || script[1] == 3) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x2C, 3);
            if (script[0] == 0x2C && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op70(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (em->type == v) {
                    break;
                }
                if (v < em->type) {
                    script = EmScript_SkipBlock(em, script, 0x70, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x70, 3);
                        if ((script[0] == 0x70 && script[1] == 2) || (script[0] == 0x70 && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x70, 3);
            if (script[0] == 0x70 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op2D(Em *em, u8 *script)
{
    em->goalKind = 7;
    em->goalArg = (s16) em->actionGoal[1];
    em->goalNo = (s16) em->actionGoal[2];
    return script;
}

u8 *EmScript_Op2E(Em *em, u8 *script)
{
    Em_SelectSightRow(em, *script);
    return script + 1;
}

u8 *EmScript_Op2F(Em *em, u8 *script)
{
    s8 i;
    u8 found;

    switch (*script++) {
    case 0:
        found = 0;
        for (i = 0; i < gScene.playerCount; i++) {
            if (gPlayers[i].carry & 0xF) {
                found = 1;
                break;
            }
        }
        if (!found) {
            for (;;) {
                if (script[0] == 0x2F && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x2F && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x2F, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x2F && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x2F);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op30(Em *em, u8 *script)
{
    s8 i;
    s32 found = 0;

    for (i = 0; i < gScene.playerCount; i++) {
        if (gPlayers[i].carry & 0xF) {
            found = 1;
            break;
        }
    }
    if (found) {
        em->need4 = 1;
    }
    return script;
}

u8 *EmScript_Op31(Em *em, u8 *script)
{
    em->goalKind = 8;
    return script;
}

u8 *EmScript_Op32(Em *em, u8 *script)
{
    u8 v;

    switch (*script++) {
    case 0:
        v = *script++;
        if (em->hitReactMode != v) {
            for (;;) {
                if (script[0] == 0x32 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x32 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x32, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x32 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x32);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op33(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (em->hitReactMode == v) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x33, 3);
                if (script[1] == 2 || script[1] == 3) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x33, 3);
            if (script[0] == 0x33 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op34(Em *em, u8 *script)
{
    u8 g;
    u8 s;

    switch (*script++) {
    case 0:
        g = script[0];
        s = script[1];
        script += 2;
        if (em->stateGroup != g || em->state != s) {
            for (;;) {
                if (script[0] == 0x34 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x34 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x34, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x34 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x34);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op35(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->rage == 0) {
            for (;;) {
                if (script[0] == 0x35 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x35 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x35, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x35 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x35);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op37(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (!(gQuest.flags & 0x200000)) {
            for (;;) {
                if (script[0] == 0x37 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x37 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x37, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x37 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x37);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op38(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (!func_330E80(em)) {
            for (;;) {
                if (script[0] == 0x38 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x38 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x38, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x38 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x38);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op39(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->blindTimer == 0) {
            for (;;) {
                if (script[0] == 0x39 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x39 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x39, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x39 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x39);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op3A(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->senseDelay != 0) {
            for (;;) {
                if (script[0] == 0x3A && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x3A && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x3A, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x3A && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x3A);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op3B(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->link == NULL) {
            for (;;) {
                if (script[0] == 0x3B && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x3B && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x3B, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x3B && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x3B);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op3C(Em *em, u8 *script)
{
    u8 ok;

    switch (*script++) {
    case 0:
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
                if (script[0] == 0x3C && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x3C && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x3C, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x3C && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x3C);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op3D(Em *em, u8 *script)
{
    u8 ok;
    u16 id;

    switch (*script++) {
    case 0:
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
            if (script[0] == 0x3D && script[1] == 1) {
                break;
            }
            if (script[0] == 0x3D && script[1] == 2) {
                break;
            }
            script = EmScript_SkipBlock(em, script, 0x3D, 2);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == 0x3D && script[1] == 2) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x3D);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op3E(Em *em, u8 *script)
{
    u16 x = em->prevArea;
    u8 n;
    s32 i;
    u16 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = script[0] << 8;
                v |= script[1];
                script += 2;
                if (x == Area_IdForCurrentTime(v) && x != 0xFFFF) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x3E, 3);
                if (script[1] == 2 || script[1] == 3) {
                    script += 2;
                    break;
                }
                script += 2;
                i++;
            } while (i < n);
        }
        break;
    case 1:
        script += 2;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x3E, 3);
            if (script[0] == 0x3E && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

/* Script command 0x3F: move to area p[0..1] (big-endian) by a route. Picks the route to that
 * area (the one starting nearest when the area has no id at this time), then the point to start
 * at: outside the scene's area the nearest flagged one, inside it the first flagged one in line
 * of sight (AreaHit_Raycast) at height p[2] * 10. */
u8 *EmScript_Op3F(Em *em, u8 *script)
{
    EmRoute *r;
    s16 j;
    s32 found;
    s16 idx;
    u16 area;
    s16 sel;
    EmRouteSet *set;
    s32 sel2;
    EmRoutePt *pt;
    f32 h;
    f32 best;
    f32 d;
    f32 v[6];
    f32 out[3];

    found = 0;
    em->routeMissCount = 0;
    area = script[0] << 8;
    area |= script[1];
    em->routeHeight = *(script += 2);
    h = 10.0f * em->routeHeight;
    if (em->area == Area_IdForCurrentTime(area)) {
        return script + 1;
    }
    set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
    r = set->routes;
    if (r == NULL) {
        return script + 1;
    }
    em->nextArea = Area_IdForCurrentTime(area);
    if (em->nextArea == 0xFFFF) {
        best = -1.0f;
        for (j = 0; j < set->n; j++, r++) {
            pt = r->pts;
            if (pt == NULL) {
                j = 0;
                em->routeResult = 7;
                goto end;
            }
            d = Vec_DistanceXZ(&em->posX, pt->pos);
            if (best == -1.0f) {
                sel = j;
                best = d;
            } else if (!(best <= d)) {
                sel = j;
                best = d;
            }
        }
        idx = sel;
    } else {
        for (idx = 0; idx < set->n; idx++, r++) {
            if (em->nextArea == Area_IdForCurrentTime(r->area)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return script + 1;
        }
    }
    em->routeNo = idx;
    r = set->routes;
    pt = r[em->routeNo].pts;
    found = 0;
    if (gScene.area != em->area) {
        sel2 = r[em->routeNo].n;
        best = -100.0f;
        for (j = 0; j < r[em->routeNo].n; j++, pt++) {
            if (pt->unk10 != 0) {
                d = Vec_DistanceXZ(&em->posX, pt->pos);
                if (best < 0.0f) {
                    sel2 = j;
                    best = d;
                } else if (d < best) {
                    sel2 = j;
                    best = d;
                }
            }
        }
        pt = r[em->routeNo].pts;
        pt += sel2;
        j = sel2;
        if (sel2 < r[em->routeNo].n) {
            found = 1;
        }
    } else {
        for (j = 0; j < r[em->routeNo].n; j++, pt++) {
            if (pt->unk10 != 0) {
                Vec_Set(v, em->posX, em->posY + h, em->posZ);
                Vec_Set(v + 3, pt->pos[0], pt->pos[1] + h, pt->pos[2]);
                if (!AreaHit_Raycast(v, v + 3, out, em->collMask)) {
                    found = 1;
                    break;
                }
            }
        }
    }
    if (!found) {
        pt--;
        em->routeResult = 2;
        j = r[em->routeNo].n - 1;
    }
end:
    if (em->routeResult != 0 && em->remote != 0) {
        em->routeResult = 0;
        j = 0;
        em->routeNo = 0;
        em->routePoint = 0;
        pt = set->routes[em->routeNo].pts;
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
    em->routePointScript = pt->unk12;
    return EmScript_RoutePointScript(em, em->routePointScript);
}

u8 *EmScript_Op40(Em *em, u8 *script)
{
    if (*script == 0) {
        Em_SetCombat(em, 0, 0);
    } else {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
    return script + 1;
}

u8 *EmScript_Op41(Em *em, u8 *script)
{
    u8 temp_a1;
    u8 type;

    type = em->type;
    temp_a1 = *script;
    switch (type) {
    case 27:
        if (!(temp_a1 & 0xFF)) {
            Em_AddHp((Em *) em, (s32) (0.15f * (f32) em->hpMax));
        }
        break;
    case 28:
        if (!(temp_a1 & 0xFF)) {
            Em_AddHp((Em *) em, (s32) (0.1f * (f32) em->hpMax));
        }
        break;
    case 31:
        if (!(temp_a1 & 0xFF)) {
            Em_AddHp((Em *) em, (s32) (0.1f * (f32) em->hpMax));
        }
        break;
    }
    return script + 1;
}

u8 *EmScript_Op42(Em *em, u8 *script)
{
    s32 ok;
    u8 k;
    u16 a;
    u16 d;

    switch (*script++) {
    case 0:
        Em_ComputeGoal(em, em->goal);
        k = *script++;
        a = (s32) (65536.0f * k / 360.0f + 0.5f);
        ok = 0;
        d = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (d > 0x8000) {
            d = 0x10000 - d;
        }
        if (a > d) {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == 0x42 && script[1] == 1) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == 0x42 && script[1] == 2) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, 0x42, 2);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == 0x42 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x42 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x42, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x42 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x42);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op69(Em *em, u8 *script)
{
    s32 ok;
    u8 k;
    u16 a;
    u16 d;

    switch (*script++) {
    case 0:
        Em_ComputeGoal(em, em->goal);
        k = *script++;
        a = (s32) (65536.0f * k / 360.0f + 0.5f);
        ok = 0;
        d = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (d > 0x8000) {
            d = 0x10000 - d;
        }
        if (a > d && a < 0x4000) {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == 0x69 && script[1] == 1) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == 0x69 && script[1] == 2) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, 0x69, 2);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == 0x69 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x69 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x69, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x69 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x69);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op44(Em *em, u8 *script)
{
    u8 ok;

    switch (*script++) {
    case 0:
        if (em->link == NULL) {
            ok = 1;
        } else {
            if (em->area == em->link->area) {
                break;
            }
            ok = 1;
        }
        while (ok) {
            if (script[0] == 0x44 && script[1] == 1) {
                break;
            }
            if (script[0] == 0x44 && script[1] == 2) {
                break;
            }
            script = EmScript_SkipBlock(em, script, 0x44, 2);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == 0x44 && script[1] == 2) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x44);
        break;
    case 2:
        break;
    }
    return script;
}

/* Command 0x45 (if/else/end): true when exactly one player is in the area and it passes flag 0x80000 and
 * Lure_PlayerHasMonsterBait; the monster then targets it (+0x798). */
u8 *EmScript_Op45(Em *em, u8 *script)
{
    Player *pl = gPlayers;
    u8 list[4];
    u8 ok;
    s8 i;
    s8 n;
    s8 cnt;
    u8 *w;

    switch (*script++) {
    case 0:
        for (i = 0, n = 0, cnt = 0, ok = 0, w = list; i < 4; i++, pl++) {
            if (pl->active != 0 && Obj_SameArea(em, pl) != 0) {
                cnt++;
                if (cnt > 1) {
                    ok = 0;
                    break;
                }
                if (Player_TestFlag(pl, 0x80000) != 0 && Lure_PlayerHasMonsterBait(pl) == 1) {
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
                if (script[0] == 0x45 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x45 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x45, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x45 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x45);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op46(Em *em, u8 *script)
{
    u8 g;
    u8 s;

    switch (*script++) {
    case 0:
        g = script[0];
        s = script[1];
        script += 2;
        if (Player_IsState(&gPlayers[(s8) em->scriptPlayer & 0xF], g, s) == 0) {
            for (;;) {
                if (script[0] == 0x46 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x46 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x46, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x46 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x46);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op47(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->hooked != 0) {
            em->hooked = 0;
        } else {
            for (;;) {
                if (script[0] == 0x47 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x47 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x47, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x47 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x47);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op48(Em *em, u8 *script)
{
    em->scriptDelay = *script;
    return script + 1;
}

u8 *EmScript_Op49(Em *em, u8 *script)
{
    s32 v = *script;

    if ((s8) em->scriptPlayer == -1) {
        em->goalKind = 1;
        em->goalArg = 0;
        *(u16 *) &em->goalNo = 0xFFFF;
    } else {
        em->goalKind = 0xB;
        em->goalArg = (s8) em->scriptPlayer & 0xF;
        em->goalNo = (u8) v;
    }
    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
    return script + 1;
}

u8 *EmScript_Op4A(Em *em, u8 *script)
{
    s8 k;

    switch (*script++) {
    case 0:
        k = em->scriptPlayer;
        if (!(em->seen & (1 << (k & 0xF))) || k == -1) {
            for (;;) {
                if (script[0] == 0x4A && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x4A && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x4A, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x4A && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x4A);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op4B(Em *em, u8 *script)
{
    s8 idx;

    idx = (s8) em->scriptPlayer;
    if (idx != -1) {
        em->alertBuildup[idx & 0xF] = 0;
    }
    return script;
}

u8 *EmScript_Op4C(Em *em, u8 *script)
{
    s8 idx;

    idx = (s8) em->scriptPlayer;
    if (idx != -1) {
        em->noticeCount[idx & 0xF] = 0;
    }
    return script;
}

u8 *EmScript_Op4D(Em *em, u8 *script)
{
    Em_ComputeGoal(em, em->goal);
    return script;
}

u8 *EmScript_Op4E(Em *em, u8 *script)
{
    switch (*script) {
    case 0:
        Em_AddNeed2(em, em->need2Max / 2);
        break;
    }
    return script + 1;
}

u8 *EmScript_Op4F(Em *em, u8 *script)
{
    switch (*script) {
    case 0:
        Em_AddNeed1(em, em->need1Max / 2);
        break;
    }
    return script + 1;
}

u8 *EmScript_Op50(Em *em, u8 *script)
{
    switch (*script) {
    case 0:
        Em_Add7F4(em, em->need3Max / 2);
        break;
    }
    return script + 1;
}

u8 *EmScript_Op51(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (em->stance != 4) {
            for (;;) {
                if (script[0] == 0x51 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x51 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x51, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x51 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x51);
        break;
    case 2:
        break;
    }
    return script;
}

/* Script command 0x52: picks the target player into +0x798 among those it is angry with (+0x7E3)
 * and can reach: anger (+0x86C) 50000 or more first, then 30000, by the largest +0x818; else the
 * angriest (none if all are 0); -1 when none. */
u8 *EmScript_Op52(Em *em, u8 *script)
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
                if (((EmView818 *) em)->f[sel] < ((EmView818 *) em)->f[list[i]]) {
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
                    if (((EmView818 *) em)->f[sel] < ((EmView818 *) em)->f[list[i]]) {
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

u8 *EmScript_Op53(Em *em, u8 *script)
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
                if (((EmView818 *) em)->f[sel] < ((EmView818 *) em)->f[list[i]]) {
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
                    if (((EmView818 *) em)->f[sel] < ((EmView818 *) em)->f[list[i]]) {
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

u8 *EmScript_Op54(Em *em, u8 *script)
{
    s8 k;
    Player *pl;

    switch (*script++) {
    case 0:
        k = em->scriptPlayer;
        if (k == -1 || Obj_SameArea(em, pl = &gPlayers[k]) == 0 || pl->active == 0) {
            for (;;) {
                if (script[0] == 0x54 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x54 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x54, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x54 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x54);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op55(Em *em, u8 *script)
{
    s8 k;

    switch (*script++) {
    case 0:
        k = em->scriptPlayer;
        if (k == -1 || em->noticeCount[k] < 30000) {
            for (;;) {
                if (script[0] == 0x55 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x55 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x55, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x55 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x55);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op56(Em *em, u8 *script)
{
    u16 day;

    switch (*script++) {
    case 0:
        day = script[0] << 8;
        day |= script[1];
        script += 2;
        if (gQuest.stage != day) {
            for (;;) {
                if (script[0] == 0x56 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x56 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x56, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x56 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x56);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op57(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u16 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = script[0] << 8;
                v |= script[1];
                script += 2;
                if (em->questKey == v) {
                    break;
                }
                if (v < em->questKey) {
                    script = EmScript_SkipBlock(em, script, 0x57, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x57, 3);
                        if ((script[0] == 0x57 && script[1] == 2) || (script[0] == 0x57 && script[1] == 3)) {
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
    case 1:
        script += 2;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x57, 3);
            if (script[0] == 0x57 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op58(Em *em, u8 *script)
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

u8 *EmScript_Op59(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if ((s8) em->ceilingTimer == 0) {
            for (;;) {
                if (script[0] == 0x59 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x59 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x59, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x59 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x59);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op5A(Em *em, u8 *script)
{
    u8 v;
    s8 k;

    switch (*script++) {
    case 0:
        v = *script++;
        if (em->curGoalKind == 0xB || em->curGoalKind == 1) {
            k = em->scriptPlayer;
            if (k != -1 && v == ((Player *) ((u8 *) gPlayers + (k & 0xF) * sizeof(Player)))->floorAttr) {
                break;
            }
        }
        for (;;) {
            if (script[0] == 0x5A && script[1] == 1) {
                break;
            }
            if (script[0] == 0x5A && script[1] == 2) {
                break;
            }
            script = EmScript_SkipBlock(em, script, 0x5A, 2);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == 0x5A && script[1] == 2) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x5A);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op5B(Em *em, u8 *script)
{
    u8 v = *script;
    s8 i;

    for (i = 0; i < gScene.playerCount; i++) {
        switch (v) {
        case 0:
            if (!(em->seen & (1 << i))) {
                em->noticeTimer[i] = 0;
            }
            break;
        }
    }
    return script + 1;
}

u8 *EmScript_Op5C(Em *em, u8 *script)
{
    f32 a;
    f32 b;

    switch (*script++) {
    case 0:
        if (em->area != gScene.area || AreaGrid2_GetCeiling(&em->posX, &a, (struct AreaTri *) &b) == 0) {
            for (;;) {
                if (script[0] == 0x5C && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x5C && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x5C, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x5C && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x5C);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op5D(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        Em_ComputeGoal(em, em->goal);
        if (em->goal[0] == 0.0f || em->goal[1] == 0.0f || em->goal[2] == 0.0f) {
            for (;;) {
                if (script[0] == 0x5D && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x5D && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x5D, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x5D && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x5D);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op5E(Em *em, u8 *script)
{
    u8 v;

    switch (*script++) {
    case 0:
        v = *script++;
        if (*((u8 *) em + 0x662) != v) {
            for (;;) {
                if (script[0] == 0x5E && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x5E && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x5E, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x5E && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x5E);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op5F(Em *em, u8 *script)
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
                if (((EmView818 *) em)->f[sel] < ((EmView818 *) em)->f[list[i]]) {
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
                    if (((EmView818 *) em)->f[sel] < ((EmView818 *) em)->f[list[i]]) {
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

u8 *EmScript_Op60(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (gScene.unk20A == 0) {
            for (;;) {
                if (script[0] == 0x60 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x60 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x60, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x60 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x60);
        break;
    case 2:
        break;
    }
    return script;
}

struct EmIdPair;
void Em_SetAreaValue8A0(Em *em, struct EmIdPair *pair);
u8 *EmScript_Op61(Em *em, u8 *script)
{
    Em_SetAreaValue8A0(em, (struct EmIdPair *)gEmAreaValues8A0[em->type]);
    return script;
}

//---
/* Command 0x62 (a branch on the target player, 0..3 then 0xFF): picks branch 1 (runs on), 2 or 3 from the
 * target player +0x662 and the monster +0x662, skipping to it; no target: branch 3. As in the game, go is
 * only set on that path (the others read it uninitialized). */
u8 *EmScript_Op62(Em *em, u8 *script)
{
    Player *pl;
    s8 k;
    u8 r;
    u8 go;
    u8 want;

    switch (*script++) {
    case 0:
        k = em->scriptPlayer;
        script += 2;
        if (k != -1) {
            pl = &gPlayers[k & 0xF];
        } else {
            go = 1;
            r = 3;
            goto found;
        }
        if (em->floorAttr == 1) {
            switch (*(u16 *) ((u8 *) pl + 0x662)) {
            default:
            case 0:
                r = 3;
                break;
            case 2:
            case 3:
                r = 2;
                break;
            }
        } else {
            switch (*(u16 *) ((u8 *) pl + 0x662)) {
            case 10:
            default:
                r = 3;
                break;
            case 4:
            case 5:
            case 7:
            case 9:
                r = 2;
                break;
            case 6:
            case 8:
                r = 1;
                break;
            }
        }
        if (r == 1) {
            break;
        }
    /* the game reads a register it never set: go is s1, still the caller's, when a player is set and r is 2 or 3 */
    found:
        while (go) {
            want = r;
            script = EmScript_SkipBlock(em, script, 0x62, 0xFF);
            if ((script[0] == 0x62 && script[1] == want) || (script[0] == 0x62 && script[1] == 0xFF)) {
                go = 0;
            }
            script = EmScript_NextCmd(em, script);
        }
        break;
    case 1:
    case 2:
    case 3:
        for (;;) {
            script = EmScript_SkipBlock(em, script, 0x62, 0xFF);
            if (script[1] == 0xFF) {
                script += 2;
                break;
            }
            script += 2;
        }
        break;
    case 0xFF:
        break;
    }
    return script;
}

u8 *EmScript_Op63(Em *em, u8 *script)
{
    u8 v;

    switch (*script++) {
    case 0:
        v = *script++;
        if (!(em->variant & v)) {
            for (;;) {
                if (script[0] == 0x63 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x63 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x63, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x63 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x63);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op77(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (Daytime_Get() & 0xFF) {
            for (;;) {
                if (script[0] == 0x77 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x77 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x77, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x77 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x77);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op7C(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if ((u32) ((Area_BaseId(gScene.map) & 0xFF) - 9) > 1) {
            for (;;) {
                if (script[0] == 0x7C && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x7C && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x7C, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x7C && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x7C);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op78(Em *em, u8 *script)
{
    s32 ok;
    u16 lo;
    u16 hi;
    u16 d;
    u16 t;
    Player *pl;
    f32 pa[4];
    f32 pb[4];

    switch (*script++) {
    case 0:
        lo = script[0];
        hi = script[1];
        ok = 0;
        script += 2;
        if (em->goalKind == 1 && *(u16 *) &em->goalArg == 0 && (s16) (t = *(u16 *) &em->goalNo) != -1) {
            Em_GetTargetPos(em, t, pa);
            pl = &gPlayers[t];
            Area_ToWorld(pl->area, pa, pb);
            d = Math_AngleXZ(em->mapPos, pb) - em->rotY;
            if ((u16) (lo << 8) > d || d > (u16) (hi << 8)) {
                ok = 1;
            }
        } else {
            ok = 1;
        }
        if (!ok) {
            if (script[0] == 0x78 && script[1] == 1) {
                script = EmScript_NextCmd(em, script);
                if (script[0] == 0x78 && script[1] == 2) {
                    script = EmScript_NextCmd(em, script);
                } else {
                    script = EmScript_SkipBlock(em, script, 0x78, 2);
                    script = EmScript_NextCmd(em, script);
                }
            }
        } else {
            for (;;) {
                if (script[0] == 0x78 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x78 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x78, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x78 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x78);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op64(Em *em, u8 *script)
{
    u8 v;
    s8 k;

    switch (*script++) {
    case 0:
        v = *script++;
        k = em->scriptPlayer;
        if (k == -1 || em->noticeCount[k] < gEmNoticeThresholds[v]) {
            for (;;) {
                if (script[0] == 0x64 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x64 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x64, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x64 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x64);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op65(Em *em, u8 *script)
{
    em->noticeCount[0] = 0;
    em->noticeCount[1] = 0;
    em->noticeCount[2] = 0;
    em->noticeCount[3] = 0;
    return script;
}

u8 *EmScript_Op66(Em *em, u8 *script)
{
    u8 ok = 1;
    Player *pl;
    s8 i;

    switch (*script++) {
    case 0:
        pl = gPlayers;
        for (i = 0; i < gScene.playerCount; i++, pl++) {
            if (pl->rideState != 0) {
                ok = 0;
            }
        }
        if (ok) {
            while (ok) {
                if (script[0] == 0x66 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x66 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x66, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x66 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x66);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op67(Em *em, u8 *script)
{
    s32 ok;

    switch (*script++) {
    case 0:
        ok = em->remote != 0;
        if (ok) {
            while (ok) {
                if (script[0] == 0x67 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x67 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x67, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x67 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x67);
        break;
    case 2:
        break;
    }
    return script;
}

s32 EmScript_Op68(Em *em, s32 p)
{
    Em_RestartScript(em);
    return p;
}

u8 *EmScript_Op80(Em *em, u8 *script)
{
    u16 i;
    u8 sum;
    u16 r;
    u8 n;
    u8 w;

    switch (*script) {
    case 0:
        n = script[1];
        r = em->stateRand & 0x1F;
        sum = 0;
        i = 0;
        script += 2;
        for (; i < n; i++) {
            script = EmScript_SkipBlock(em, script, 0x80, 0xFF);
            w = script[2];
            script += 3;
            if (!w) {
                continue;
            }
            if (w == 0xFF) {
                break;
            }
            sum += w;
            if (r < sum) {
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
            script = EmScript_SkipBlock(em, script, 0x80, 0xFF);
            if (script[1] == 0xFF) {
                script += 2;
                break;
            }
            script += 3;
        }
        break;
    case 0xFF:
        script++;
        break;
    }
    return script;
}

u8 *EmScript_Op81(Em *em, u8 *p)
{
    em->scriptCallNo = *p;
    if (em->scriptCallDepth == 0) {
        em->scriptCallReturn = p + 1;
    }
    em->scriptCallDepth = 1;
    p = em->script[1][em->scriptCallNo];
    em->scriptPos = p;
    return p;
}

/* Command 0x82 (2 argument bytes: a script table k and an entry): jumps to script entry of table
 * 0xF + k; the first such jump after a 0x81 jump remembers the return position (+0x760). */
u8 *EmScript_Op82(Em *em, u8 *p)
{
    u8 k;

    k = *p++;
    em->scriptCallEntry = *p++;
    if (em->scriptCallDepth == 1) {
        em->scriptJumpReturn = p;
    }
    em->scriptCallDepth = 2;
    p = em->script[k + 0xF][em->scriptCallEntry];
    em->scriptPos = p;
    return p;
}

u8 *EmScript_Op06(Em *em, u8 *script)
{
    s32 i;
    Em *m;

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
        for (i = 0, m = gEms; i < 20; i++, m++) {
            if (m->active != 0 && em->area == m->area && m != em && m->large != 0) {
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

/* Command 0x83 (a multi-way branch on distance, 0..5 then 0xFF): picks the first limit (+0x764) the
 * distance to the target reaches and runs that branch; -1: no target, the last one. */
u8 *EmScript_Op83(Em *em, u8 *script)
{
    s8 k;
    f32 d;
    u8 n;
    s32 i;
    s32 j;
    s32 off;

    switch (*script++) {
    case 0:
        k = em->scriptPlayer;
        if (k == -1) {
            d = -1.0f;
        } else if (em->curGoalKind == 0xD) {
            d = Vec_Distance(&em->posX, &gEms[k].posX);
        } else {
            off = (k & 0xF) * 4;
            d = *(f32 *) (off + (s32) em + 0x818);
        }
        n = *script++;
        if (d == -1.0f) {
            i = n;
        } else {
            em->targetDist = d;
            for (i = 0; i < n; i++) {
                if (em->targetDist <= ((EmDistView *) em)->limit[i] && ((EmDistView *) em)->limit[i] >= 0.0f) {
                    break;
                }
            }
        }
        em->scriptCaseCount = i;
        for (j = 0; j < em->scriptCaseCount; j++) {
            script = EmScript_NextCmd(em, script);
            script = EmScript_SkipBlock(em, script, 0x83, 0xFF);
        }
        script = EmScript_NextCmd(em, script);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        for (;;) {
            script = EmScript_SkipBlock(em, script, 0x83, 0xFF);
            if (script[1] == 0xFF) {
                script += 2;
                break;
            }
            script += 2;
        }
        break;
    case 0xFF:
        break;
    }
    return script;
}

u8 *EmScript_OpFF(Em *em, u8 *script)
{
    EmRouteSet *set;
    EmRoute *routes;
    EmRoutePt *pts;
    f32 d;
    f32 lim;
    s8 n;
    s8 i;
    u8 *q;
    s8 k;
    u8 buf[16];
    f32 v[4];

    switch (*script) {
    case 0:
        em->scriptNo = 0;
        script = (u8 *) Em_StartScript(em);
        em->scriptFlags = 0;
        Em_RequestSync(em, 2);
        break;
    case 1:
        em->scriptCallDepth = 0;
        script = em->scriptCallReturn;
        break;
    case 2:
        em->scriptCallDepth = 1;
        script = em->scriptJumpReturn;
        break;
    case 3:
        script = em->subScriptReturn;
        break;
    case 4:
    case 5:
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
                em->routePointScript = pts[em->routePoint].unk12;
                script = EmScript_RoutePointScript(em, em->routePointScript);
            } else {
                em->scriptNo = 0;
                script = (u8 *) Em_StartScript(em);
            }
            break;
        }
        if (em->type == 0x3A || em->type == 0x48) {
            d = Vec_Distance(&em->posX, pts[em->routePoint].pos);
        } else {
            d = Vec_DistanceXZ(&em->posX, pts[em->routePoint].pos);
        }
        if (!(d <= pts[em->routePoint].radius)) {
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
            em->routePointScript = pts[em->routePoint].unk12;
            script = EmScript_RoutePointScript(em, em->routePointScript);
            em->scriptPos = script;
        } else if (em->routePoint == 0) {
        last:
            if (*script == 4 && (s16) routes[em->curGoalArg].area != -1 && em->large != 0) {
                em->posX = routes[em->routeNo].pos[0];
                em->posY = routes[em->routeNo].pos[1];
                em->posZ = routes[em->routeNo].pos[2];
                Vec_Copy(&em->prevPosX, &em->posX);
                em->rotY = routes[em->curGoalArg].unk14;
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
            em->routePointScript = pts[em->routePoint].unk12;
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
    case 0xF8:
        em->scriptFlags &= 0xF7;
        if (em->scriptFlags & 1) {
            script = EmScript_LoopScript(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case 0xF9:
        em->scriptFlags &= 0xFB;
        if (em->scriptFlags & 1) {
            script = EmScript_LoopScript(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            Em_RequestSync(em, 2);
        }
        break;
    case 0xFA:
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
    case 0xFB:
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
    case 0xFD:
        em->scriptFlags &= 0xEF;
        if (em->scriptFlags & 1) {
            script = EmScript_LoopScript(em, em->loopEndScript);
        } else {
            em->scriptNo = 0;
            script = (u8 *) Em_StartScript(em);
            em->syncRequest = 2;
        }
        break;
    case 0xFE:
        Em_ComputeGoal(em, v);
        d = Vec_DistanceXZ(v, &em->posX);
        if (em->type == 2 || em->type == 0x24 || em->type == 0x47 || em->type == 0x37 ||
            em->type == 7 || em->type == 0x32 || em->type == 0x3A || em->type == 0x48) {
            lim = 1000.0f;
        } else {
            lim = 500.0f;
        }
        if (d <= lim) {
            em->loopIndex++;
            switch (em->loopMode) {
            case 0:
                i = 0;
                n = 0;
                if (i < (s8) em->areaPointCount) {
                    k = em->loopPoint;
                    q = buf;
                    do {
                        if (k != i) {
                            *q++ = i;
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
    case 0xFF:
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

u8 *EmScript_Op90(Em *em, u8 *script)
{
    f32 *v = (f32 *) (gEmGoalPoints[em->type] + *script * sizeof(f32[3]));

    em->posX = v[0];
    em->posY = v[1];
    em->posZ = v[2];
    em->prevPosX = em->posX;
    em->prevPosY = em->posY;
    em->prevPosZ = em->posZ;
    return script + 1;
}

void * EmScript_Op91(Em *em, u8 *script)
{
    em->rotY = Math_AngleXZ((f32 *) &em->posX, (f32 *) (gEmGoalPoints[em->type] + (*script * 0xC))) & 0xFFFF;
    return script + 1;
}

void * EmScript_Op99(Em *em, u8 *script)
{
    em->rotY = *script << 8;
    return script + 1;
}

s32 EmScript_Op92(s32 unused, s32 p)
{
    return p;
}

s32 EmScript_Op93(s32 unused, s32 p)
{
    return p;
}

u8 *EmScript_Op94(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                if (D_008F5AB8 == v) {
                    break;
                }
                if (v < D_008F5AB8) {
                    script = EmScript_SkipBlock(em, script, 0x94, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x94, 3);
                        if ((script[0] == 0x94 && script[1] == 2) || (script[0] == 0x94 && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x94, 3);
            if (script[0] == 0x94 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op71(Em *em, u8 *script)
{
    u8 v;

    switch (*script++) {
    case 0:
        v = *script++;
        if (em->level < v) {
            for (;;) {
                if (script[0] == 0x71 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x71 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x71, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x71 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x71);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op72(Em *em, u8 *script)
{
    switch (*script++) {
    case 0:
        if (Quest_IsVersus() != 1) {
            for (;;) {
                if (script[0] == 0x72 && script[1] == 1) {
                    break;
                }
                if (script[0] == 0x72 && script[1] == 2) {
                    break;
                }
                script = EmScript_SkipBlock(em, script, 0x72, 2);
            }
            script = EmScript_NextCmd(em, script);
            if (script[0] == 0x72 && script[1] == 2) {
                script = EmScript_NextCmd(em, script);
            }
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x72);
        break;
    case 2:
        break;
    }
    return script;
}

u8 *EmScript_Op73(Em *em, u8 *script)
{
    u8 n;
    s32 i;
    u8 v;
    u8 ok;
    u8 x;

    switch (*script++) {
    case 0:
        n = script[0];
        i = 0;
        script += 3;
        if (i < n) {
            do {
                v = *script++;
                x = em->itemUseKind & ~0x80;
                if (x == v) {
                    em->itemUseKind &= 0x80;
                    break;
                }
                if (v < x) {
                    script = EmScript_SkipBlock(em, script, 0x73, 3);
                    if (script[1] == 2 || script[1] == 3) {
                        script += 2;
                        break;
                    }
                    script += 2;
                } else {
                    ok = 1;
                    do {
                        script = EmScript_SkipBlock(em, script, 0x73, 3);
                        if ((script[0] == 0x73 && script[1] == 2) || (script[0] == 0x73 && script[1] == 3)) {
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
    case 1:
        script++;
    case 2:
        ok = 1;
        do {
            script = EmScript_SkipBlock(em, script, 0x73, 3);
            if (script[0] == 0x73 && script[1] == 3) {
                ok = 0;
            }
            script = EmScript_NextCmd(em, script);
        } while (ok);
        break;
    case 3:
        break;
    }
    return script;
}

u8 *EmScript_Op74(Em *em, u8 *script)
{
    u8 v;
    u8 n;

    switch (*script++) {
    case 0:
        v = *script++;
        if (Quest_IsVersus() == 1 && (n = em->itemUseKind & ~0x80) > v) {
            if (Versus_AddGauge(em, n) == 1) {
                break;
            }
            em->itemUseKind &= 0x80;
        }
        for (;;) {
            if (script[0] == 0x74 && script[1] == 1) {
                break;
            }
            if (script[0] == 0x74 && script[1] == 2) {
                break;
            }
            script = EmScript_SkipBlock(em, script, 0x74, 2);
        }
        script = EmScript_NextCmd(em, script);
        if (script[0] == 0x74 && script[1] == 2) {
            script = EmScript_NextCmd(em, script);
        }
        break;
    case 1:
        script = EmScript_SkipToEnd(em, script, 0x74);
        break;
    case 2:
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

u8 *EmScript_SkipBlock(Em *em, u8 *script, u8 op, u8 end)
{
    for (;;) {
        while (script[0] != op) {
            script = EmScript_NextCmd(em, script);
        }
        if (script[0] != op || script[1] != 0) {
            break;
        }
        script = EmScript_NextCmd(em, script);
        for (;;) {
            if (script[0] == op && script[1] == end) {
                script = EmScript_NextCmd(em, script);
                break;
            }
            if (script[0] == op) {
                script = EmScript_NextCmd(em, script);
            }
            script = EmScript_SkipBlock(em, script, op, end);
        }
    }
    return script;
}

/* Skips the command at p (its arguments, and a sub-command where it has one); an unknown command
 * stops the script (EmScript_SetVar95E with 4). The game keeps p in a1 across that call: EmScript_SetVar95E
 * is static and defined earlier in this file (catalogue "Arguments kept or reloaded across calls"). */
u8 *EmScript_NextCmd(Em *em, u8 *script)
{
    switch (*script) {
    case 1:
        script += 2;
        break;
    case 2:
        script += 2;
        break;
    case 3:
        script += 2;
        break;
    case 4:
        script += 1;
        break;
    case 5:
        script += 4;
        break;
    case 6:
        if (script[1] == 3) {
            script += 5;
        } else {
            script += 4;
        }
        break;
    case 7:
        script += 2;
        break;
    case 8:
        script += 2;
        break;
    case 0x1F:
        script += 2;
        break;
    case 9:
        script += 2;
        break;
    case 0x0A:
        script += 2;
        break;
    case 0x0B:
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
    case 0x0C:
        script += 3;
        break;
    case 0x0D:
        script += 2;
        break;
    case 0x0E:
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
    case 0x0F:
        script += 6;
        break;
    case 0x10:
        script += 1;
        break;
    case 0x11:
        script += 1;
        break;
    case 0x12:
        script += 1;
        break;
    case 0x13:
        script += 1;
        break;
    case 0x14:
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
    case 0x15:
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
    case 0x16:
        script += 2;
        break;
    case 0x17:
        script += 5;
        break;
    case 0x18:
        script += 1;
        break;
    case 0x19:
        script += 1;
        break;
    case 0x1A:
        script += 3;
        break;
    case 0x1B:
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
    case 0x1C:
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
    case 0x1D:
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
    case 0x1E:
        script += 1;
        break;
    case 0x20:
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
    case 0x21:
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
    case 0x22:
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
    case 0x36:
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
    case 0x23:
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
    case 0x24:
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
    case 0x25:
        script += 1;
        break;
    case 0x26:
        script += 2;
        break;
    case 0x27:
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
    case 0x28:
        script += 2;
        break;
    case 0x29:
        script += 2;
        break;
    case 0x2A:
        script += 2;
        break;
    case 0x2B:
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
    case 0x2C:
    case 0x70:
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
    case 0x2D:
        script += 1;
        break;
    case 0x2E:
        script += 2;
        break;
    case 0x2F:
        script += 2;
        break;
    case 0x30:
        script += 1;
        break;
    case 0x31:
        script += 1;
        break;
    case 0x32:
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
    case 0x33:
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
    case 0x34:
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
    case 0x35:
        script += 2;
        break;
    case 0x37:
        script += 2;
        break;
    case 0x38:
        script += 2;
        break;
    case 0x39:
        script += 2;
        break;
    case 0x3A:
        script += 2;
        break;
    case 0x3B:
        script += 2;
        break;
    case 0x3C:
        script += 2;
        break;
    case 0x3D:
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
    case 0x3E:
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
    case 0x3F:
        script += 4;
        break;
    case 0x40:
        script += 2;
        break;
    case 0x41:
        script += 2;
        break;
    case 0x42:
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
    case 0x69:
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
    case 0x44:
        script += 2;
        break;
    case 0x45:
        script += 2;
        break;
    case 0x46:
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
    case 0x47:
        script += 2;
        break;
    case 0x48:
        script += 2;
        break;
    case 0x49:
        script += 2;
        break;
    case 0x4A:
        script += 2;
        break;
    case 0x4B:
        script += 1;
        break;
    case 0x4C:
        script += 1;
        break;
    case 0x4D:
        script += 1;
        break;
    case 0x4E:
    case 0x4F:
    case 0x50:
        script += 2;
        break;
    case 0x51:
        script += 2;
        break;
    case 0x52:
        script += 1;
        break;
    case 0x53:
        script += 1;
        break;
    case 0x54:
        script += 2;
        break;
    case 0x55:
        script += 2;
        break;
    case 0x56:
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
    case 0x57:
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
    case 0x58:
        script += 1;
        break;
    case 0x59:
        script += 2;
        break;
    case 0x5A:
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
    case 0x5B:
        script += 2;
        break;
    case 0x5C:
        script += 2;
        break;
    case 0x5D:
        script += 2;
        break;
    case 0x5E:
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
    case 0x5F:
        script += 1;
        break;
    case 0x60:
        script += 2;
        break;
    case 0x61:
        script += 1;
        break;
    case 0x62:
        script += 2;
        break;
    case 0x63:
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
    case 0x64:
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
    case 0x65:
        script += 1;
        break;
    case 0x66:
        script += 2;
        break;
    case 0x67:
        script += 2;
        break;
    case 0x68:
        script += 1;
        break;
    case 0x80:
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
    case 0x81:
        script += 2;
        break;
    case 0x82:
        script += 3;
        break;
    case 0x83:
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
    case 0x84:
    case 0x7B:
        script += 1;
        break;
    case 0xFF:
        script += 2;
        break;
    case 0x90:
        script += 2;
        break;
    case 0x91:
    case 0x99:
        script += 2;
        break;
    case 0x92:
    case 0x93:
        script += 1;
        break;
    case 0x94:
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
    case 0x71:
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
    case 0x72:
        script += 2;
        break;
    case 0x73:
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
    case 0x74:
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
    case 0x75:
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
    case 0x76:
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
    case 0x77:
        script += 2;
        break;
    case 0x78:
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
    case 0x79:
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
    case 0x7A:
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
    case 0x7C:
        script += 2;
        break;
    default:
        EmScript_SetVar95E(em, script, 4);
        break;
    }
    return script;
}

u8 *EmScript_SkipToEnd(Em *em, u8 *script, u8 op)
{
    Em *e;

    if (script[0] == op && script[1] == 2) {
        script = EmScript_NextCmd(em, script);
    } else {
        e = em; /* a copy for the loop (found by the permuter): with em itself MWCC lays the loop out differently */
        for (;;) {
            if (script[0] == op && script[1] == 2) {
                script = EmScript_NextCmd(e, script);
                break;
            }
            script = EmScript_SkipBlock(e, script, op, 2);
        }
    }
    return script;
}

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
    AreaRecord *a;
    f32 x;
    f32 z;

    em->prevArea = *(u8 *) &em->area;
    em->area = em->nextArea;
    Map_GetPosition(v[0], &v[0][2], em->prevArea);
    Map_GetPosition(v[1], &v[1][2], em->area);
    Stage_GetAreaRecord(em->prevArea);
    a = Stage_GetAreaRecord(em->area);
    switch ((u16) ((u16) (Math_AngleXZ(v[0], v[1]) + 0x1000) >> 13)) {
    case 4:
        z = a->sizeZ;
        x = a->sizeX / 2.0f;
        break;
    case 5:
        x = a->sizeX;
        z = a->sizeZ;
        break;
    case 6:
        x = a->sizeX;
        z = a->sizeZ / 2.0f;
        break;
    case 7:
        z = 0.0f;
        x = a->sizeX;
        break;
    case 0:
        z = 0.0f;
        x = a->sizeX / 2.0f;
        break;
    case 1:
        z = x = 0.0f;
        break;
    case 2:
        x = 0.0f;
        z = a->sizeZ / 2.0f;
        break;
    case 3:
        z = a->sizeZ;
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

/* Picks the next area of the monster's route: unless its area (Em +0x684) is one of the up to eight
 * areas Em_IdSetLookup lists, the listed area whose centre lies closest in angle to the direction from
 * area +0x680 to area +0x684 (skipping area +0x882) becomes +0x684. pts[8] and pts[9] are those two
 * centres and ang[8] that direction (the game keeps them in the same arrays). */
void Em_PickRoutePoint(Em *em)
{
    s16 *list;
    s8 i;
    s32 found;
    s16 n;
    s16 best;
    u16 bestd;
    AreaRecord *a;
    f32 pts[10][3];
    u16 ang[9];
    u16 diff[8];
    u16 ids[8];

    if (*(u16 *) &em->destArea == 0xFFFF) {
        *(u16 *) &em->destArea = *(u8 *) &em->nextArea;
    }
    list = Em_IdSetLookup(em);
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
    a = Stage_GetAreaRecord(em->area);
    pts[8][0] = a->originX + a->sizeX / 2.0f;
    pts[8][2] = a->originZ + (f32) (a->sizeZ / 2.0f);
    a = Stage_GetAreaRecord(em->nextArea);
    pts[9][0] = a->originX + a->sizeX / 2.0f;
    pts[9][2] = a->originZ + (f32) (a->sizeZ / 2.0f);
    for (i = 0, n = 0; i < 8; i++) {
        if (list[i] == -1) {
            break;
        }
        a = Stage_GetAreaRecord(list[i]);
        ids[i] = list[i];
        pts[i][0] = a->originX + a->sizeX / 2.0f;
        n++;
        pts[i][2] = a->originZ + (f32) (a->sizeZ / 2.0f);
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
 * em_blangonga.c declares the parameter EmWork *): the two areas' map positions (Map_GetPosition)
 * relative to em->area's record (Stage_GetAreaRecord: origin [0], [1], size [4], [5]), moved
 * onto the nearer edge along the line between them; out[1] is the area's height (gAreaGroundY).
 * `t = z / ax` is the original's (it divides by ax there, not az). a[4] and a[5] are read at
 * each use (one load each): a local for a[4] takes f3, where the original has the reload of az.
 */
void Em_AreaExitPoint(Em *em, f32 *out)
{
    AreaRecord *a;
    f32 ax;
    f32 az;
    f32 bx;
    f32 bz;
    f32 s;
    f32 r;
    f32 x;
    f32 t;
    f32 d;
    f32 z;
    f32 w;

    a = Stage_GetAreaRecord(em->area);
    Stage_GetAreaRecord(em->nextArea);
    Map_GetPosition(&ax, &az, em->area);
    Map_GetPosition(&bx, &bz, em->nextArea);
    w = bx - ax;
    d = bz - az;
    ax = ax - a->originX;
    az = az - a->originZ;
    out[0] = w + ax;
    out[2] = d + az;
    x = out[0];
    if (!(x <= 0.0f) && x < a->sizeX) {
        z = out[2];
        if (!(z <= 0.0f) && z < a->sizeZ) {
            r = a->sizeZ / a->sizeX;
            if (x < ax) {
                s = x / ax;
            } else {
                s = (a->sizeX - x) / (a->sizeX - ax);
            }
            if (z < az) {
                t = z / ax;
            } else {
                t = (a->sizeZ - z) / (a->sizeZ - az);
            }
            t = t * r;
            if (s <= t) {
                if (x < ax) {
                    out[0] = 0.0f;
                } else {
                    x = a->sizeX - x;
                    out[0] = a->sizeX;
                }
                r = r * (s / t);
                if (out[2] < az) {
                    out[2] = out[2] - x * r;
                } else {
                    out[2] = out[2] + x * r;
                }
            } else {
                if (z < az) {
                    out[2] = 0.0f;
                } else {
                    z = a->sizeZ - z;
                    out[2] = a->sizeZ;
                }
                t = t / s / r;
                if (out[0] < ax) {
                    out[0] = out[0] - z * t;
                } else {
                    out[0] = out[0] + z * t;
                }
            }
        }
    }
    out[1] = gAreaGroundY[em->area];
}

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

u8 *EmScript_PickWeighted(Em *em, u8 *script)
{
    u16 n;
    u16 i;
    u8 sum;
    u16 r;
    u8 w;

    n = *(u16 *) (script + 4);
    sum = 0;
    r = em->stateRand & 0x1F;
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
        if (r < sum) {
            break;
        }
        script += 2;
    }
    return script;
}

/* The 0x722400 part's .data (0x8C3DE0-0x8C3E50) */
ArmorCombo gArmorCombos[14] = {
#include "game/008C3DE0.inc"
};
s32 gEmNoticeThresholds[3] = {
#include "game/008C3E38.inc"
};
